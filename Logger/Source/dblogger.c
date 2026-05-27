#include "dblogger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mariadb/mysql.h>
#include <time.h>

#define LOG_FILE_NAME "error.log"
#define LOG_SESSION_START 2000
#define LOG_SESSION_END 2001
#define ERR_MYSQL_INIT 1001
#define ERR_MYSQL_CONNECT 1002
#define ERR_DB_CREATE 1003
#define ERR_DB_SELECT 1004
#define ERR_TABLE_CREATE 1005
#define ERR_INSERT 1006

static MYSQL *conn = NULL;

void DB_Log(int code, const char *msg)
{
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  char timestr[20];
  strftime(timestr, sizeof(timestr), "%Y-%m-%d %H:%M:%S", t);
  printf("%s [%04d] %s\n", timestr, code, msg);
  FILE *logf = fopen(LOG_FILE_NAME, "a");
  if (!logf)
    return;
  fprintf(logf, "%s [%04d] %s\n", timestr, code, msg);
  fclose(logf);
}

#define CHECK_DB(expr, code)           \
  if ((expr))                          \
  {                                    \
    DB_Log((code), mysql_error(conn)); \
    return 0;                          \
  }

int DB_Init(const char *host, const char *user, const char *pass, const char *dbname)
{
  DB_Log(LOG_SESSION_START, "Starting new logging session");
  conn = mysql_init(NULL);
  if (!conn)
  {
    DB_Log(ERR_MYSQL_INIT, "mysql_init() failed");
    return 0;
  }
  if (!mysql_real_connect(conn, host, user, pass, NULL, 0, NULL, 0))
  {
    DB_Log(ERR_MYSQL_CONNECT, mysql_error(conn));
    return 0;
  }
  char query[256];
  snprintf(query, sizeof(query), "CREATE DATABASE IF NOT EXISTS %s", dbname);
  CHECK_DB(mysql_query(conn, query), ERR_DB_CREATE);
  CHECK_DB(mysql_select_db(conn, dbname), ERR_DB_SELECT);
  return 1;
}

int DB_Insert(const char *table, int pin, int value)
{
  if (!conn)
    return 0;
  char query[256];
  snprintf(query, sizeof(query),
           "CREATE TABLE IF NOT EXISTS %s ("
           "id INT AUTO_INCREMENT PRIMARY KEY,"
           "datetime DATETIME DEFAULT CURRENT_TIMESTAMP,"
           "pin INT NOT NULL,"
           "value INT NOT NULL"
           ")",
           table);
  CHECK_DB(mysql_query(conn, query), ERR_TABLE_CREATE);
  snprintf(query, sizeof(query),
           "INSERT INTO %s (pin, value) VALUES (%d, %d)", table, pin, value);
  CHECK_DB(mysql_query(conn, query), ERR_INSERT);
  return 1;
}

void DB_Close()
{
  if (conn)
  {
    mysql_close(conn);
    conn = NULL;
  }
  DB_Log(LOG_SESSION_END, "Ending logging session");
}