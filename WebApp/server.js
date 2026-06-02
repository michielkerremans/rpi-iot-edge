const express = require('express');
const http = require('http');
const WebSocket = require('ws');
const path = require('path');
const EventHubReader = require('./scripts/event-hub-reader.js');

const { Registry } = require("azure-iothub");
const { promisify } = require("util");

const iotHubConnectionString = process.env.IotHubConnectionString;
if (!iotHubConnectionString)
{
  console.error(`Environment variable IotHubConnectionString must be specified.`);
  return;
}
console.log(`Using IoT Hub connection string [${iotHubConnectionString}]`);

const eventHubConsumerGroup = process.env.EventHubConsumerGroup;
console.log(eventHubConsumerGroup);
if (!eventHubConsumerGroup)
{
  console.error(`Environment variable EventHubConsumerGroup must be specified.`);
  return;
}
console.log(`Using event hub consumer group [${eventHubConsumerGroup}]`);

// Read the desired temperature from the module twin for a given device.
const moduleId = process.env.ModuleId || 'remotemodule';
const registry = Registry.fromConnectionString(iotHubConnectionString);
const getModuleTwinAsync = promisify(registry.getModuleTwin).bind(registry);

async function readModuleSettings(deviceId)
{
  try
  {
    const twin = await getModuleTwinAsync(deviceId, moduleId);
    const desired = twin?.properties?.desired;

    return {
      desiredTemperature: typeof desired?.desired_temperature === 'number' ? desired.desired_temperature : null,
      telemetryIntervalMs: typeof desired?.telemetry_interval_ms === 'number' ? desired.telemetry_interval_ms : null
    };
  } catch (err)
  {
    console.error('Module twin read failed for %s/%s: %s', deviceId, moduleId, err.message || err);
    return {
      desiredTemperature: null,
      telemetryIntervalMs: null
    };
  }
}

// Redirect requests to the public subdirectory to the root
const app = express();
app.use(express.json());
app.use(express.static(path.join(__dirname, 'public')));

app.post('/api/telemetry-interval', (req, res) =>
{
  const { deviceId, interval } = req.body;
  if (!deviceId || !Number.isFinite(interval) || interval < 0)
  {
    res.status(400).json({ error: 'deviceId and non-negative numeric interval are required.' });
    return;
  }

  const patch = {
    properties: {
      desired: {
        telemetry_interval_ms: interval
      }
    }
  };

  registry.updateModuleTwin(deviceId, moduleId, patch, '*', (err) =>
  {
    if (err)
    {
      res.status(500).json({ error: err.message || String(err) });
      return;
    }
    res.json({ ok: true, deviceId, telemetry_interval_ms: interval });
  });
});

app.use((req, res /* , next */) =>
{
  res.redirect('/');
});

const server = http.createServer(app);
const wss = new WebSocket.Server({ server });

wss.broadcast = (data) =>
{
  wss.clients.forEach((client) =>
  {
    if (client.readyState === WebSocket.OPEN)
    {
      try
      {
        console.log(`Broadcasting data ${data}`);
        client.send(data);
      } catch (e)
      {
        console.error(e);
      }
    }
  });
};

server.listen(process.env.PORT || '3000', () =>
{
  console.log('Listening on %d.', server.address().port);
});

const eventHubReader = new EventHubReader(iotHubConnectionString, eventHubConsumerGroup);

(async () =>
{
  //await eventHubReader.startReadMessage((message, date, deviceId) =>
  await eventHubReader.startReadMessage(async (message, date, deviceId) =>
  {
    try
    {
      const moduleSettings = await readModuleSettings(deviceId);

      const payload = {
        IotData: message,
        MessageDate: date || Date.now().toISOString(),
        DeviceId: deviceId,
        DesiredTemperature: moduleSettings.desiredTemperature,
        TelemetryIntervalMs: moduleSettings.telemetryIntervalMs
      };

      wss.broadcast(JSON.stringify(payload));
    } catch (err)
    {
      console.error('Error broadcasting: [%s] from [%s].', err, message);
    }
  });
})().catch();