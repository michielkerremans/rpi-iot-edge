#include "PJ_RPI.h"
#include <stdio.h>

int main()
{
	if (map_peripheral(&gpio) == -1)
	{
		printf("Failed to map the physical GPIO registers into the virtual memory space.\n");
		return -1;
	}

	// Define gpio 17 as output
	INP_GPIO(17);
	OUT_GPIO(17);

	while (1)
	{
		// Toggle 17 (blink a led!)
		if (GPIO_READ(27))
		{
			printf("GPIO 27 is HIGH\n");
		}
		else
		{
			printf("GPIO 27 is LOW\n");
		}
		sleep(5);
	}

	return 0;
}
