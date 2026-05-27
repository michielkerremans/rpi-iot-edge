#include "gpio.h"
#include "PJ_RPI.h"

extern struct bcm2835_peripheral gpio;

int GPIO_Init()
{
  if (map_peripheral(&gpio) == -1)
  {
    printf("Failed to map GPIO peripheral.\n");
    return -1;
  }
  return 0;
}

void GPIO_Mode(int pin, int mode)
{
  INP_GPIO(pin);
  if (mode == 1)
    OUT_GPIO(pin);
}

void GPIO_Alt(int pin, int alt)
{
  INP_GPIO(pin); // set pin to input mode (reset state)
  SET_GPIO_ALT(pin, alt);
}

int GPIO_Read(int pin, int *value)
{
  INP_GPIO(pin);
  int new_value = (GPIO_READ(pin)) >> pin; // the parentheses here are NOT optional!
  if (new_value != *value)
  {
    printf("GPIO %d is %s\n", pin, new_value ? "HIGH" : "LOW");
    *value = new_value;
    return 1;
  }
  return 0;
}

void GPIO_Write(int pin, int value)
{
  if (value)
    *(gpio.addr + 7) = 1 << pin; // Set pin HIGH
  else
    *(gpio.addr + 10) = 1 << pin; // Set pin LOW
}

void GPIO_Cleanup()
{
  unmap_peripheral(&gpio);
}