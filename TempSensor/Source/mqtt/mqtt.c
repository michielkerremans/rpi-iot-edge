#include "mqtt.h"
#include <MQTTClient.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define QOS 1
#define TIMEOUT 10000L

static MQTTClient client;
static char last_payload[32] = {0};
static char mqtt_last_error[128] = {0};

static int messageArrived(void *context, char *topicName, int topicLen, MQTTClient_message *message)
{
  printf("Received on topic %s: %.*s\n", topicName, message->payloadlen, (char *)message->payload);
  snprintf(last_payload, sizeof(last_payload), "%.*s", message->payloadlen, (char *)message->payload);
  MQTTClient_freeMessage(&message);
  MQTTClient_free(topicName);
  return 1;
}

int mqtt_init(const char *address, const char *clientid)
{
  MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
  int rc;

  MQTTClient_create(&client, address, clientid, MQTTCLIENT_PERSISTENCE_NONE, NULL);
  MQTTClient_setCallbacks(client, NULL, NULL, messageArrived, NULL); // Set the message callback for incoming messages
  rc = MQTTClient_connect(client, &conn_opts);
  if (rc != MQTTCLIENT_SUCCESS)
  {
    snprintf(mqtt_last_error, sizeof(mqtt_last_error), "Failed to connect to MQTT broker at %s (code %d)", address, rc);
    printf("%s\n", mqtt_last_error);
    return rc;
  }
  return 0;
}

int mqtt_publish(const char *topic, const char *payload)
{
  MQTTClient_message pubmsg = MQTTClient_message_initializer;
  pubmsg.payload = (void *)payload;
  pubmsg.payloadlen = strlen(payload);
  pubmsg.qos = QOS;
  pubmsg.retained = 0;
  int rc = MQTTClient_publishMessage(client, topic, &pubmsg, NULL);
  if (rc != MQTTCLIENT_SUCCESS)
  {
    snprintf(mqtt_last_error, sizeof(mqtt_last_error),
             "MQTT publish failed for topic %s (code %d)", topic, rc);
    printf("%s\n", mqtt_last_error);
    return rc;
  }
  return 0;
}

void mqtt_subscribe(const char *topic)
{
  int rc = MQTTClient_subscribe(client, topic, QOS);
  if (rc != MQTTCLIENT_SUCCESS)
  {
    snprintf(mqtt_last_error, sizeof(mqtt_last_error), "Failed to subscribe to topic %s (code %d)", topic, rc);
    printf("%s\n", mqtt_last_error);
    exit(1);
  }
}

void mqtt_cleanup()
{
  MQTTClient_disconnect(client, TIMEOUT);
  MQTTClient_destroy(&client);
}

const char *mqtt_get_last_payload(void)
{
  return last_payload;
}

const char *mqtt_get_error_message(void)
{
  return mqtt_last_error;
}