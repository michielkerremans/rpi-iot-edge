#include "gpiod.h"
#include <gpiod.h>
#include <stdio.h>
#include "dblogger.h"

#define ERR_GPIOD_CHIP_OPEN 3001
#define ERR_GPIOD_GET_LINE 3002
#define ERR_GPIOD_REQ_OUTPUT 3003
#define ERR_GPIOD_REQ_INPUT 3004
#define ERR_GPIOD_GET_VALUE 3005

#define CHIP_NAME "gpiochip0"
#define MAX_PINS 64

static struct gpiod_chip *chip = NULL;     // pointer to the GPIO chip
static struct gpiod_line *lines[MAX_PINS]; // on-demand array to hold pointers to the GPIO lines

void GPIOD_Init(int pin, int mode)
{
  if (!chip)
    chip = gpiod_chip_open_by_name(CHIP_NAME);

  if (!chip)
  {
    char msg[64];
    snprintf(msg, sizeof(msg), "Failed to open GPIO chip: %s", CHIP_NAME);
    DB_Log(ERR_GPIOD_CHIP_OPEN, msg);
    return;
  }

  lines[pin] = gpiod_chip_get_line(chip, pin); // get the line for the specified pin
  if (!lines[pin])
  {
    char msg[64];
    snprintf(msg, sizeof(msg), "Failed to get GPIO line: %d", pin);
    DB_Log(ERR_GPIOD_GET_LINE, msg);
    return;
  }

  if (mode == 1)
  {
    if (gpiod_line_request_output(lines[pin], "gpiod", 0) < 0) // "gpiod" is the consumer label for this process
    {
      char msg[64];
      snprintf(msg, sizeof(msg), "Failed to request line %d as output", pin);
      DB_Log(ERR_GPIOD_REQ_OUTPUT, msg);
    }
  }
  else
  {
    if (gpiod_line_request_input(lines[pin], "gpiod") < 0) // "gpiod" is the consumer label for this process
    {
      char msg[64];
      snprintf(msg, sizeof(msg), "Failed to request line %d as input", pin);
      DB_Log(ERR_GPIOD_REQ_INPUT, msg);
    }
  }
}

int GPIOD_Read(int pin)
{
  if (!lines[pin])
    return -1;
  int value = gpiod_line_get_value(lines[pin]); // read the value of the line
  if (value < 0)
  {
    char msg[64];
    snprintf(msg, sizeof(msg), "Failed to read GPIO line: %d", pin);
    DB_Log(ERR_GPIOD_GET_VALUE, msg);
  }
  return value;
}

int GPIOD_Log(int pin, int *value)
{
  int new_value = GPIOD_Read(pin);
  if (new_value != *value)
  {
    printf("GPIO %d is %s\n", pin, new_value ? "HIGH" : "LOW");
    *value = new_value;
    return 1;
  }
  return 0;
}