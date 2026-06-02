#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <string.h>
#include <stdbool.h>
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

static int desired_temperature = 20;     // default desired temperature
static int telemetry_interval_ms = 5000; // default telemetry interval (ms); 0 disables periodic telemetry

const int loop_tick_ms = 100;     // control loop tick (ms)
static uint32_t elapsed_ms = 0;   // accumulator for telemetry interval
static bool mode_changed = false; // mode changed and pending telemetry

static void TwinCallback(DEVICE_TWIN_UPDATE_STATE updateState, const unsigned char *payload, size_t size, void *userContextCallback)
{
    printf("Twin update received: %.*s\n", (int)size, payload);

    const char *found;
    if ((found = strstr((const char *)payload, "\"desired_temperature\":")))
        desired_temperature = atoi(found + strlen("\"desired_temperature\":"));

    if ((found = strstr((const char *)payload, "\"telemetry_interval_ms\":")))
    {
        int new_interval = atoi(found + strlen("\"telemetry_interval_ms\":"));
        if (new_interval != telemetry_interval_ms)
        {
            telemetry_interval_ms = new_interval;
            /* initialize elapsed_ms so first tick triggers after one loop tick */
            if (telemetry_interval_ms > 0)
                elapsed_ms = (telemetry_interval_ms > loop_tick_ms) ? telemetry_interval_ms - loop_tick_ms : 0;
            else
                elapsed_ms = 0;
        }
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

    /* initialize elapsed_ms so first tick triggers after one loop tick */
    if (telemetry_interval_ms > 0)
        elapsed_ms = (telemetry_interval_ms > loop_tick_ms) ? telemetry_interval_ms - loop_tick_ms : 0;
    else
        elapsed_ms = 0;

    uint8_t temp;
    char payload[128];

    Mode last_mode = MODE_OFF;
    Mode current_mode = MODE_OFF;

    while (1)
    {
        bool read_ok = (TC74_Read(0x48, &temp) == 0);

        if (read_ok)
        {
            if (temp < desired_temperature)
            {
                GPIO_Write(GPIO_HEAT, 1);
                GPIO_Write(GPIO_COOL, 0);
                current_mode = MODE_HEAT;
            }
            else if (temp > desired_temperature)
            {
                GPIO_Write(GPIO_HEAT, 0);
                GPIO_Write(GPIO_COOL, 1);
                current_mode = MODE_COOL;
            }
            else
            {
                GPIO_Write(GPIO_HEAT, 0);
                GPIO_Write(GPIO_COOL, 0);
                current_mode = MODE_OFF;
            }

            if (current_mode != last_mode)
            {
                mode_changed = true; // mark mode to be reported at next telemetry tick
                last_mode = current_mode;
            }
        }
        else
            printf("Failed to read temperature\n");

        // process the queue
        IoTHubModuleClient_LL_DoWork(client);

        // counter-based telemetry scheduling (0 disables periodic telemetry)
        if (telemetry_interval_ms > 0)
        {
            elapsed_ms += loop_tick_ms;
            if (elapsed_ms >= (uint32_t)telemetry_interval_ms)
            {
                if (read_ok)
                {
                    snprintf(payload, sizeof(payload), "{\"temperature\": %d}", temp);
                    send_telemetry(client, payload);
                }
                else
                {
                    snprintf(payload, sizeof(payload), "{\"error\": \"Failed to read temperature\"}");
                    send_telemetry(client, payload);
                }

                if (mode_changed)
                {
                    if (current_mode == MODE_HEAT)
                        send_telemetry(client, "{\"mode\": \"HEAT\"}");
                    else if (current_mode == MODE_COOL)
                        send_telemetry(client, "{\"mode\": \"COOL\"}");
                    else
                        send_telemetry(client, "{\"mode\": \"OFF\"}");

                    mode_changed = false;
                }

                elapsed_ms = 0;
            }
        }

        ThreadAPI_Sleep(loop_tick_ms);
    }

    GPIO_Cleanup();
    IoTHubModuleClient_LL_Destroy(client);
    IoTHub_Deinit();
    return 0;
}