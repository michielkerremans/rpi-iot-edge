#ifndef DBLOGGER_H
#define DBLOGGER_H

void DB_Log(int code, const char *msg);
int DB_Init(const char *host, const char *user, const char *pass, const char *dbname);
int DB_Insert(const char *table, int pin, int value);
void DB_Close();

#endif