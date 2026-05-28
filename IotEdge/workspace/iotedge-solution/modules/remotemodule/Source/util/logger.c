#include "logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

void get_time(char *timestr, int timestr_len)
{
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  strftime(timestr, timestr_len, "%Y-%m-%d %H:%M:%S", t);
}

void log_msg(const char *filename, const char *msg)
{
  FILE *logf = fopen(filename, "a");
  if (!logf)
    return;
  char timestr[20];
  get_time(timestr, sizeof(timestr));
  fprintf(logf, "%s - %s\n", timestr, msg);
  fclose(logf);
}