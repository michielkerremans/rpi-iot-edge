#include "PJ_RPI.h"
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include "gpio/gpio.h"
#include "gpio/gpiod.h"
#include "i2c/tc74.h"
#include "util/logger.h"
#include "mqtt/mqtt.h"
#include "gtk/gui.h"

int gpio_26_val = -1;
int gpio_27_val = -1;

// Function prototypes
void *toggle_gpio17(void *arg);
void *toggle_gpio19(void *arg);
void *mqtt_payload_loop(void *arg);

int main(int argc, char *argv[])
{
	int use_gpiod = (argc > 1 && strcmp(argv[1], "--gpiod") == 0);
	int use_temp = (argc > 1 && strcmp(argv[1], "--temp") == 0);
	int use_mqtt = (argc > 1 && strcmp(argv[1], "--mqtt") == 0);
	int use_gtk = (argc > 2 && strcmp(argv[2], "--gtk") == 0);

	// Interval for temperature publishing
	int interval = 2;
	if (use_temp && argc > 2)
	{
		int val = atoi(argv[2]);
		if (val > 0)
			interval = val;
	}

	char clientid[64] = "TempSensorClient";

	// Intervals for GPIO toggling
	int gpio17_interval = 2;
	if (use_mqtt && argc > 2)
	{
		int v = atoi(argv[2]);
		if (v > 0)
			gpio17_interval = v;
	}
	int gpio19_interval = 3;
	if (use_mqtt && argc > 3)
	{
		int v = atoi(argv[3]);
		if (v > 0)
			gpio19_interval = v;
	}

	log_msg("error.log", "Session started.");

	if (use_gpiod)
	{
		printf("Using libgpiod for GPIO access.\n");

		GPIOD_Init(26, 0); // Set GPIO 26 as input
		GPIOD_Init(27, 0); // Set GPIO 27 as input

		GPIOD_Init(-30, 0); // Test invalid pin to trigger error handling
		log_msg("error.log", GPIOD_GetErrorMessage());

		while (1)
		{
			if (!GPIOD_Read(26, &gpio_26_val))
				log_msg("error.log", GPIOD_GetErrorMessage());

			if (!GPIOD_Read(27, &gpio_27_val))
				log_msg("error.log", GPIOD_GetErrorMessage());

			sleep(1);
		}
	}
	else if (use_temp)
	{
		GPIO_Init(); // Initialize GPIO memory mapping
		TC74_Init(); // Initialize TC74 memory mapping

		GPIO_Alt(2, 0); // Set GPIO 2 (SDA) to ALT0 (I2C1 SDA)
		GPIO_Alt(3, 0); // Set GPIO 3 (SCL) to ALT0 (I2C1 SCL)

		dump_bsc1_status();

		// Initialize MQTT client
		snprintf(clientid, sizeof(clientid), "TempSensorPublisher-%d", getpid());
		if (mqtt_init("tcp://localhost:1883", clientid) != 0)
		{
			printf("MQTT error: %s\n", mqtt_get_error_message());
			log_msg("error.log", mqtt_get_error_message());
			return 1;
		}

		uint8_t temp;
		char payload[32];

		while (1)
		{
			TC74_Read(0x48, &temp); // Read temperature from TC74 (address 0x48)
			snprintf(payload, sizeof(payload), "%d", temp);
			if (mqtt_publish("sensor/temperature", payload) != 0)
			{
				printf("MQTT error: %s\n", mqtt_get_error_message());
				log_msg("error.log", mqtt_get_error_message());
			}
			sleep(interval);
		}
	}
	else if (use_mqtt)
	{
		// Initialize MQTT client
		snprintf(clientid, sizeof(clientid), "TempSensorSubscriber-%d", getpid());
		if (mqtt_init("tcp://localhost:1883", clientid) != 0)
		{
			printf("MQTT error: %s\n", mqtt_get_error_message());
			log_msg("error.log", mqtt_get_error_message());
			return 1;
		}

		printf("Subscribing to sensor/temperature...\n");
		mqtt_subscribe("sensor/temperature");

		GPIO_Init();
		GPIO_Mode(17, 1); // Set GPIO 17 as output
		GPIO_Mode(19, 1); // Set GPIO 19 as output

		if (use_gtk)
		{
			launch_gtk_gui();
		}
		else
		{
			pthread_t t1, t2, t3;
			pthread_create(&t1, NULL, toggle_gpio17, &gpio17_interval);
			pthread_create(&t2, NULL, toggle_gpio19, &gpio19_interval);
			pthread_create(&t3, NULL, mqtt_payload_loop, NULL);

			// Keep main thread alive
			while (1)
				sleep(10);
		}
	}
	else
	{
		GPIO_Init();			// Initialize GPIO memory mapping
		GPIO_Mode(26, 0); // Set GPIO 26 as input
		GPIO_Mode(27, 0); // Set GPIO 27 as input

		while (1)
		{
			GPIO_Read(26, &gpio_26_val);
			GPIO_Read(27, &gpio_27_val);
			sleep(1);
		}
	}

	log_msg("error.log", "Session ended.");
	return 0;
}

void *toggle_gpio17(void *arg)
{
	int interval = *(int *)arg;
	int state = 0;
	while (1)
	{
		state = !state;
		GPIO_Write(17, state);
		printf("Toggled GPIO 17, now %s\n", state ? "HIGH" : "LOW");
		sleep(interval);
	}
	return NULL;
}

void *toggle_gpio19(void *arg)
{
	int interval = *(int *)arg;
	int state = 0;
	while (1)
	{
		state = !state;
		GPIO_Write(19, state);
		printf("Toggled GPIO 19, now %s\n", state ? "HIGH" : "LOW");
		sleep(interval);
	}
	return NULL;
}

void *mqtt_payload_loop(void *arg)
{
	char prev_payload[32] = {0};
	while (1)
	{
		const char *current = mqtt_get_last_payload();
		if (current[0] && strcmp(current, prev_payload) != 0)
		{
			printf("Temperature changed: %s\n", current);
			strncpy(prev_payload, current, sizeof(prev_payload));
		}
		sleep(1);
	}
	return NULL;
}