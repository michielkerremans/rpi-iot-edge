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

void send_telemetry(IOTHUB_MODULE_CLIENT_LL_HANDLE client, const char *payload)
{
    IOTHUB_MESSAGE_HANDLE message = IoTHubMessage_CreateFromString(payload);
    if (message == NULL)
    {
        printf("Failed to create message\n");
        return;
    }
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
    if (IoTHub_Init() != 0)
    {
        printf("Failed to initialize platform\n");
        return 1;
    }

    IOTHUB_MODULE_CLIENT_LL_HANDLE client = IoTHubModuleClient_LL_CreateFromEnvironment(MQTT_Protocol);
    if (client == NULL)
    {
        printf("Failed to create module client\n");
        IoTHub_Deinit();
        return 1;
    }

    send_telemetry(client, "{\"status\": \"Module started\"}");

    // Initialize TC74 sensor
    TC74_Init();

    uint8_t temp;
    char payload[64];

    // Periodically read and send temperature
    for (int i = 0; i < 10; ++i) // Send 10 times as an example
    {
        if (TC74_Read(0x48, &temp) == 0)
        {
            snprintf(payload, sizeof(payload), "{\"temperature\": \"%d\"}", temp);
            send_telemetry(client, payload);
        }
        else
        {
            printf("Failed to read temperature\n");
            send_telemetry(client, "{\"error\": \"Failed to read temperature\"}");
        }

        for (int j = 0; j < 10; ++j)
        {
            IoTHubModuleClient_LL_DoWork(client);
            ThreadAPI_Sleep(100);
        }
    }

    IoTHubModuleClient_LL_Destroy(client);
    IoTHub_Deinit();
    return 0;
}