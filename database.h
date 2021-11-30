#ifndef DATABASE_H
#define DATABASE_H

#include "common.h"

typedef struct dbase_config
{
   char * user;
   char * dbname;
   char * table;
   unsigned int port;
   char * host;
   char * password;
} dbase_config_t;

typedef enum dbase_fields
{
   DB_USER = 0,
   DB_NAME,
   DB_TABLE,
   DB_PORT,
   DB_HOST,
   DB_PASSWORD,
   DB_FIELD_MAX,
} dbase_fields_t;

int database_insert (dbase_config_t * dbconfig, weather_t * weather);
int database_init (FILE * config_file, dbase_config_t * dbase_config);
int validate_db_config (dbase_config_t * dbase_config);

#endif /* End of header guard */