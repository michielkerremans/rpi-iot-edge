#include "gpiod.h"
#include <gpiod.h>
#include <stdio.h>

#define CHIP_NAME "gpiochip0"
#define MAX_PINS 64

static struct gpiod_chip *chip = NULL;     // pointer to the GPIO chip
static struct gpiod_line *lines[MAX_PINS]; // on-demand array to hold pointers to the GPIO lines

static char err_msg[128]; // buffer to hold error messages

const char *GPIOD_GetErrorMessage()
{
  return err_msg;
}

void GPIOD_Init(int pin, int mode)
{
  if (!chip)
    chip = gpiod_chip_open_by_name(CHIP_NAME);

  if (!chip)
  {
    snprintf(err_msg, sizeof(err_msg), "Failed to open GPIO chip: %s", CHIP_NAME);
    printf("%s\n", err_msg);
    return;
  }

  lines[pin] = gpiod_chip_get_line(chip, pin); // get the line for the specified pin
  if (!lines[pin])
  {
    snprintf(err_msg, sizeof(err_msg), "Failed to get GPIO line: %d", pin);
    printf("%s\n", err_msg);
    return;
  }

  if (mode == 1)
  {
    if (gpiod_line_request_output(lines[pin], "gpiod", 0) < 0) // "gpiod" is the consumer label for this process
    {
      snprintf(err_msg, sizeof(err_msg), "Failed to request line %d as output", pin);
      printf("%s\n", err_msg);
    }
  }
  else
  {
    if (gpiod_line_request_input(lines[pin], "gpiod") < 0) // "gpiod" is the consumer label for this process
    {
      snprintf(err_msg, sizeof(err_msg), "Failed to request line %d as input", pin);
      printf("%s\n", err_msg);
    }
  }
}

int GPIOD_Read(int pin, int *value)
{
  if (lines[pin])
  {
    int new_value = gpiod_line_get_value(lines[pin]); // read the value of the line
    if (new_value >= 0)
    {
      if (new_value != *value)
      {
        printf("GPIO %d is %s\n", pin, new_value ? "HIGH" : "LOW");
        *value = new_value;
      }
      return 1; // success
    }
  }

  snprintf(err_msg, sizeof(err_msg), "Failed to read GPIO line: %d", pin);
  printf("%s\n", err_msg);
  return 0; // failure
}