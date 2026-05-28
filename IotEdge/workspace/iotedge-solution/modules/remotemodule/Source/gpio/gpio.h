#ifndef GPIO_H
#define GPIO_H

int GPIO_Init();
void GPIO_Mode(int pin, int mode);
void GPIO_Alt(int pin, int alt);
int GPIO_Read(int pin, int *value);
void GPIO_Write(int pin, int value);
void GPIO_Cleanup();

#endif