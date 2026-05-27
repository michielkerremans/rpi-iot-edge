#include "gpio.h"
#include "PJ_RPI.h"

extern struct bcm2835_peripheral gpio;

void GPIO_Init(int pin, int mode)
{
  INP_GPIO(pin);
  if (mode == 1)
    OUT_GPIO(pin);
}

int GPIO_Read(int pin)
{
  INP_GPIO(pin);
  return (GPIO_READ(pin)) >> pin; // the parentheses here are NOT optional!
}

int GPIO_Log(int pin, int *value)
{
  int new_value = GPIO_Read(pin);
  if (new_value != *value)
  {
    printf("GPIO %d is %s\n", pin, new_value ? "HIGH" : "LOW");
    *value = new_value;
    return 1;
  }
  return 0;
}