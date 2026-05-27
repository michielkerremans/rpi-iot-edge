#ifndef GPIOD_H
#define GPIOD_H

const char *GPIOD_GetErrorMessage();
void GPIOD_Init(int pin, int mode);
int GPIOD_Read(int pin, int *value);

#endif