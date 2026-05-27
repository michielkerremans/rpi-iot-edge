#ifndef MQTT_H
#define MQTT_H

int mqtt_init(const char *address, const char *clientid);
int mqtt_publish(const char *topic, const char *payload);
void mqtt_subscribe(const char *topic);
void mqtt_cleanup();
const char *mqtt_get_last_payload(void);
const char *mqtt_get_error_message(void);

#endif