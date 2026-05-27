#ifndef GPIOD_H
#define GPIOD_H

void GPIOD_Init(int pin, int mode);
int GPIOD_Read(int pin);
int GPIOD_Log(int pin, int *value);

#endif