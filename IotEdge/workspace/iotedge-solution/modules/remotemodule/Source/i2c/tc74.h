#ifndef TC74_H
#define TC74_H

#include "i2c1.h"
#include <stdint.h>

#define TC74_I2C_ADDR 0x48

void TC74_Init();
int TC74_Read(uint8_t i2c_addr, uint8_t *temp);

#endif