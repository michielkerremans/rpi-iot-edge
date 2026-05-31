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
#include "PJ_RPI.h"
#include "gpio/gpio.h"

#define GPIO_HEAT 17 // connected to pin 27
#define GPIO_COOL 19 // connected to pin 26

static int desired_temperature = 20; // default desired temperature
static int temp_telemetry = 0;       // 0 = off, 1 = on for temperature telemetry

static void TwinCallback(DEVICE_TWIN_UPDATE_STATE updateState, const unsigned char *payload, size_t size, void *userContextCallback)
{
    printf("Twin update received: %.*s\n", (int)size, payload);

    const char *found;
    if ((found = strstr((const char *)payload, "\"desired_temperature\":")))
        desired_temperature = atoi(found + strlen("\"desired_temperature\":"));

    if ((found = strstr((const char *)payload, "\"temp_telemetry\":")))
        temp_telemetry = atoi(found + strlen("\"temp_telemetry\":"));
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

typedef enum
{
    MODE_OFF,
    MODE_HEAT,
    MODE_COOL
} Mode;

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
    printf("Module started.\n");

    GPIO_Init(); // Initialize GPIO memory mapping
    send_telemetry(client, "{\"status\": \"GPIO initialized successfully\"}");
    printf("GPIO initialized successfully.\n");

    TC74_Init(); // Initialize TC74 memory mapping
    send_telemetry(client, "{\"status\": \"TC74 initialized successfully\"}");
    printf("TC74 initialized successfully.\n");

    GPIO_Alt(2, 0); // Set GPIO 2 (SDA) to ALT0 (I2C1 SDA)
    GPIO_Alt(3, 0); // Set GPIO 3 (SCL) to ALT0 (I2C1 SCL)

    GPIO_Write(GPIO_HEAT, 0); // Ensure heater is off
    GPIO_Write(GPIO_COOL, 0); // Ensure cooler is off
    send_telemetry(client, "{\"status\": \"GPIOs configured successfully\"}");
    printf("GPIOs configured successfully.\n");

    uint8_t temp, last_temp = 0xFF; // 0xFF is an impossible value for TC74
    char payload[64];

    Mode last_mode = MODE_OFF;
    Mode current_mode = MODE_OFF;

    while (1)
    {
        if (TC74_Read(0x48, &temp) == 0)
        {
            if (temp != last_temp)
            {
                snprintf(payload, sizeof(payload), "{\"temperature\": \"%d\"}", temp);
                if (temp_telemetry)
                    send_telemetry(client, payload);
                last_temp = temp;

                if (temp < desired_temperature)
                {
                    GPIO_Write(GPIO_HEAT, 1); // Turn on heater
                    GPIO_Write(GPIO_COOL, 0); // Turn off cooler
                    current_mode = MODE_HEAT;
                }
                else if (temp > desired_temperature)
                {
                    GPIO_Write(GPIO_HEAT, 0); // Turn off heater
                    GPIO_Write(GPIO_COOL, 1); // Turn on cooler
                    current_mode = MODE_COOL;
                }
                else
                {
                    GPIO_Write(GPIO_HEAT, 0); // Turn off heater
                    GPIO_Write(GPIO_COOL, 0); // Turn off cooler
                    current_mode = MODE_OFF;
                }

                if (current_mode != last_mode)
                {
                    if (current_mode == MODE_HEAT)
                        send_telemetry(client, "{\"mode\": \"HEAT\"}");
                    else if (current_mode == MODE_COOL)
                        send_telemetry(client, "{\"mode\": \"COOL\"}");
                    else
                        send_telemetry(client, "{\"mode\": \"OFF\"}");

                    last_mode = current_mode;
                }
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

    GPIO_Cleanup();
    IoTHubModuleClient_LL_Destroy(client);
    IoTHub_Deinit();
    return 0;
}