#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include "iothub_module_client_ll.h"
#include "iothub_message.h"
#include "azure_c_shared_utility/threadapi.h"
#include "iothubtransportmqtt.h"
#include "iothub.h"
#include "i2c/tc74.h"

static int desired_temperature = 20; // default desired temperature

static void TwinCallback(DEVICE_TWIN_UPDATE_STATE updateState, const unsigned char *payload, size_t size, void *userContextCallback)
{
    printf("Twin update received: %.*s\n", (int)size, payload);
    const char *key = "\"desired_temperature\":";
    const char *found = strstr((const char *)payload, key);
    if (found)
    {
        desired_temperature = atoi(found + strlen(key));
        printf("Updated desired_temperature to %d\n", desired_temperature);
    }
}

void send_telemetry(IOTHUB_MODULE_CLIENT_LL_HANDLE client, const char *payload)
{
    IOTHUB_MESSAGE_HANDLE message = IoTHubMessage_CreateFromString(payload);
    if (message == NULL)
    {
        printf("Failed to create message\n");
        return;
    }
    // Queue the message to be sent asynchronously
    if (IoTHubModuleClient_LL_SendEventToOutputAsync(client, message, "output1", NULL, NULL) != IOTHUB_CLIENT_OK)
    {
        printf("Failed to send telemetry\n");
    }
    else
    {
        printf("Telemetry sent: %s\n", payload);
    }
    IoTHubMessage_Destroy(message);
}

int main(void)
{
    // Initialize the Azure IoT SDK platform (not the client)
    if (IoTHub_Init() != 0)
    {
        printf("Failed to initialize platform\n");
        return 1;
    }

    // Create the IoT Hub module client using environment variables
    IOTHUB_MODULE_CLIENT_LL_HANDLE client = IoTHubModuleClient_LL_CreateFromEnvironment(MQTT_Protocol);
    if (client == NULL)
    {
        printf("Failed to create module client\n");
        IoTHub_Deinit();
        return 1;
    }

    // Set the device twin callback to receive desired property updates
    IoTHubModuleClient_LL_SetModuleTwinCallback(client, TwinCallback, NULL);

    send_telemetry(client, "{\"status\": \"Module started\"}");

    // Initialize TC74 sensor
    TC74_Init();

    uint8_t temp, last_temp = 0xFF; // 0xFF is an impossible value for TC74
    char payload[64];

    while (1)
    {
        if (TC74_Read(0x48, &temp) == 0)
        {
            if (temp != last_temp)
            {
                snprintf(payload, sizeof(payload), "{\"temperature\": \"%d\"}", temp);
                send_telemetry(client, payload);
                last_temp = temp;
            }
        }
        else
        {
            printf("Failed to read temperature\n");
            send_telemetry(client, "{\"error\": \"Failed to read temperature\"}");
        }

        // Process the queue
        IoTHubModuleClient_LL_DoWork(client);
        ThreadAPI_Sleep(100);
    }

    IoTHubModuleClient_LL_Destroy(client);
    IoTHub_Deinit();
    return 0;
}