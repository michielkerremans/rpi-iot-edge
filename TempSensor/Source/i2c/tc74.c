#include "tc74.h"

void TC74_Init()
{
  if (i2c1_init() == -1)
  {
    printf("Failed to initialize I2C1 peripheral.\n");
    return;
  }
}

int TC74_Read(uint8_t i2c_addr, uint8_t *temp)
{
  if (i2c1_read_register(i2c_addr, TC74_I2C_ADDR, temp, 1) == 0)
  {
    printf("TC74 temperature: %d °C\n", *temp);
    return 0;
  }
  else
  {
    printf("Failed to read TC74 temperature\n");
    return -1;
  }
}