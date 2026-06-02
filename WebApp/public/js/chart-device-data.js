/* eslint-disable max-classes-per-file */
/* eslint-disable no-restricted-globals */
/* eslint-disable no-undef */
$(document).ready(() =>
{
  // if deployed to a site supporting SSL, use wss://
  const protocol = document.location.protocol.startsWith('https') ? 'wss://' : 'ws://';
  const webSocket = new WebSocket(protocol + location.host);

  // A class for holding the last N points of telemetry for a device
  class DeviceData
  {
    constructor(deviceId)
    {
      this.deviceId = deviceId;
      this.maxLen = 50;
      this.timeData = new Array(this.maxLen);
      this.temperatureData = new Array(this.maxLen);
      // this.humidityData = new Array(this.maxLen);
      this.desiredTemperature = null;
      this.desiredTemperatureData = new Array(this.maxLen);
      this.tempTelemetryEnabled = null;
    }

    addData(time, temperature, humidity)
    {
      this.timeData.push(time);
      this.temperatureData.push(temperature);
      // this.humidityData.push(humidity || null);
      this.desiredTemperatureData.push(this.desiredTemperature);

      if (this.timeData.length > this.maxLen)
      {
        this.timeData.shift();
        this.temperatureData.shift();
        // this.humidityData.shift();
        this.desiredTemperatureData.shift();
      }
    }

    setDesiredTemperature(value)
    {
      this.desiredTemperature = value;
    }

    setTempTelemetryEnabled(enabled)
    {
      this.tempTelemetryEnabled = enabled;
    }
  }

  // All the devices in the list (those that have been sending telemetry)
  class TrackedDevices
  {
    constructor()
    {
      this.devices = [];
    }

    // Find a device based on its Id
    findDevice(deviceId)
    {
      for (let i = 0; i < this.devices.length; ++i)
      {
        if (this.devices[i].deviceId === deviceId)
        {
          return this.devices[i];
        }
      }

      return undefined;
    }

    getDevicesCount()
    {
      return this.devices.length;
    }
  }

  const trackedDevices = new TrackedDevices();

  // Define the chart axes
  const chartData = {
    datasets: [
      {
        fill: false,
        label: 'Temperature',
        yAxisID: 'Temperature',
        borderColor: 'rgba(255, 204, 0, 1)',
        pointBoarderColor: 'rgba(255, 204, 0, 1)',
        backgroundColor: 'rgba(255, 204, 0, 0.4)',
        pointHoverBackgroundColor: 'rgba(255, 204, 0, 1)',
        pointHoverBorderColor: 'rgba(255, 204, 0, 1)',
        spanGaps: true,
      },
      // {
      //   fill: false,
      //   label: 'Humidity',
      //   yAxisID: 'Humidity',
      //   borderColor: 'rgba(24, 120, 240, 1)',
      //   pointBoarderColor: 'rgba(24, 120, 240, 1)',
      //   backgroundColor: 'rgba(24, 120, 240, 0.4)',
      //   pointHoverBackgroundColor: 'rgba(24, 120, 240, 1)',
      //   pointHoverBorderColor: 'rgba(24, 120, 240, 1)',
      //   spanGaps: true,
      // },
      {
        fill: false,
        label: 'Desired Temperature',
        // yAxisID: 'Temperature',
        yAxisID: 'TemperatureRight',
        borderColor: 'rgba(220, 50, 47, 1)',
        backgroundColor: 'rgba(220, 50, 47, 0.2)',
        borderDash: [8, 6],
        pointRadius: 0,
        spanGaps: true,
      }
    ]
  };

  const chartOptions = {
    scales: {
      yAxes: [{
        id: 'Temperature',
        type: 'linear',
        scaleLabel: {
          labelString: 'Temperature (ºC)',
          display: true,
        },
        position: 'left',
        ticks: {
          suggestedMin: 0,
          suggestedMax: 50,
          beginAtZero: true
        }
      },
      // {
      //   id: 'Humidity',
      //   type: 'linear',
      //   scaleLabel: {
      //     labelString: 'Humidity (%)',
      //     display: true,
      //   },
      //   position: 'right',
      //   ticks: {
      //     suggestedMin: 0,
      //     suggestedMax: 100,
      //     beginAtZero: true
      //   }
      // }
      {
        id: 'TemperatureRight',
        type: 'linear',
        scaleLabel: {
          labelString: 'Temperature (ºC)',
          display: true,
        },
        position: 'right',
        ticks: {
          suggestedMin: 0,
          suggestedMax: 50,
          beginAtZero: true
        }
      }]
    }
  };

  // Get the context of the canvas element we want to select
  const ctx = document.getElementById('iotChart').getContext('2d');
  const myLineChart = new Chart(
    ctx,
    {
      type: 'line',
      data: chartData,
      options: chartOptions,
    });

  // Manage a list of devices in the UI, and update which device data the chart is showing
  // based on selection
  let needsAutoSelect = true;
  const deviceCount = document.getElementById('deviceCount');
  const listOfDevices = document.getElementById('listOfDevices');

  const tempTelemetryToggle = document.getElementById('tempTelemetryToggle');
  let tempTelemetryEnabled = false;

  function OnSelectionChange()
  {
    const device = trackedDevices.findDevice(listOfDevices[listOfDevices.selectedIndex].text);

    if (device?.tempTelemetryEnabled !== null)
    {
      tempTelemetryEnabled = device.tempTelemetryEnabled;
      tempTelemetryToggle.textContent = tempTelemetryEnabled ? 'ON' : 'OFF';
    }

    chartData.labels = device.timeData;
    chartData.datasets[0].data = device.temperatureData;
    // chartData.datasets[1].data = device.humidityData;
    // chartData.datasets[2].data = device.desiredTemperatureData;
    chartData.datasets[1].data = device.desiredTemperatureData;
    myLineChart.update();
  }
  listOfDevices.addEventListener('change', OnSelectionChange, false);

  tempTelemetryToggle.addEventListener('click', async () =>
  {
    const selectedOption = listOfDevices[listOfDevices.selectedIndex];
    if (!selectedOption)
    {
      return;
    }

    const deviceId = selectedOption.text;
    const enabled = !tempTelemetryEnabled;

    try
    {
      const response = await fetch('/api/temp-telemetry', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json'
        },
        body: JSON.stringify({
          deviceId,
          enabled
        })
      });

      if (!response.ok)
      {
        throw new Error('Failed to update temp_telemetry.');
      }

      tempTelemetryEnabled = enabled;
      tempTelemetryToggle.textContent = tempTelemetryEnabled ? 'ON' : 'OFF';

      const selectedDevice = trackedDevices.findDevice(deviceId);
      if (selectedDevice)
      {
        selectedDevice.setTempTelemetryEnabled(enabled);
      }
    } catch (err)
    {
      console.error(err);
    }
  });

  // When a web socket message arrives:
  // 1. Unpack it
  // 2. Validate it has date/time and temperature
  // 3. Find or create a cached device to hold the telemetry data
  // 4. Append the telemetry data
  // 5. Update the chart UI
  webSocket.onmessage = function onMessage(message)
  {
    try
    {
      const messageData = JSON.parse(message.data);
      console.log(messageData);

      const hasTemperature = messageData.IotData.temperature !== undefined;

      if (!messageData.MessageDate)
      {
        return;
      }

      // find or add device to list of tracked devices
      const existingDeviceData = trackedDevices.findDevice(messageData.DeviceId);

      if (existingDeviceData)
      {
        if (typeof messageData.DesiredTemperature === 'number')
          existingDeviceData.setDesiredTemperature(messageData.DesiredTemperature);

        if (typeof messageData.TempTelemetryEnabled === 'boolean')
          existingDeviceData.setTempTelemetryEnabled(messageData.TempTelemetryEnabled);

        if (hasTemperature)
          existingDeviceData.addData(messageData.MessageDate, messageData.IotData.temperature, messageData.IotData.humidity);
      }
      else
      {
        const newDeviceData = new DeviceData(messageData.DeviceId);
        trackedDevices.devices.push(newDeviceData);
        const numDevices = trackedDevices.getDevicesCount();
        deviceCount.innerText = numDevices === 1 ? `${numDevices} device` : `${numDevices} devices`;

        if (typeof messageData.DesiredTemperature === 'number')
          newDeviceData.setDesiredTemperature(messageData.DesiredTemperature);

        if (typeof messageData.TempTelemetryEnabled === 'boolean')
          newDeviceData.setTempTelemetryEnabled(messageData.TempTelemetryEnabled);

        if (hasTemperature)
          newDeviceData.addData(messageData.MessageDate, messageData.IotData.temperature, messageData.IotData.humidity);

        // add device to the UI list
        const node = document.createElement('option');
        const nodeText = document.createTextNode(messageData.DeviceId);
        node.appendChild(nodeText);
        listOfDevices.appendChild(node);

        // if this is the first device being discovered, auto-select it
        if (needsAutoSelect)
        {
          needsAutoSelect = false;
          listOfDevices.selectedIndex = 0;
          OnSelectionChange();
        }
      }

      myLineChart.update();
    } catch (err)
    {
      console.error(err);
    }
  };
});
