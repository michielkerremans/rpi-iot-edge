#ifndef GPIO_H
#define GPIO_H

void GPIO_Init(int pin, int mode);
int GPIO_Read(int pin);
int GPIO_Log(int pin, int *value);

#endif