/*
 * Database functions
 *
 */
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include "database.h"
#include "logger.h"
#include <libpq-fe.h>
#include "common.h"
#include <string.h>
#include "config.h"
#include <mysql.h>
#include "weather_processing.h"

// PGconn *psql;
MYSQL  mysql;


// static int connect_to_postgreql (dbase_config_t *dbconfig)
// {
//     /* init connection */
//     psql = PQconnectdb(connectionInfo);

//     // Connection failed
//     if (PQstatus (psql) != CONNECTION_OK) {
//         // Connection error, put the error to error output
//         logger (LOG_ERROR, __func__, "libpq error: PQstatus(psql) != CONNECTION_OK", NULL);
//         logger (LOG_ERROR, __func__, "PSQL Error Message: '%s'", PQerrorMessage(psql));
//         return -1;
//     }
//     else
//     {
//         logger (LOG_DBASE, __func__, "Connection made to the database", NULL);
//         return 0;
//     }
// }

// static int database_insert_postsql_weather_data (char *table, weather_t *data)
// {
//     PGresult *dbResult;
//     int ret;
//
//     asprintf (&queryString,
//               "INSERT INTO %s (time, in_humidity, out_humidity, in_temperature,"
//               "out_temperature, out_dew_temperature, windchill_temperature, wind_speed"
//               ", wind_gust, wind_direction, pressure, rel_pressure, rain_last_hour"
//               ", rain_last_24h, rain_total, bytes) values (now(), %d, %d, %0.1f, %0.1f" \
//               ", '%0.1f', '%0.1f', '%0.1f', '%0.1f', '%s', '%0.1f', '%0.1f', '%0.1f','%0.1f', '%0.1f', '%s');",
//               table,
//               data->in_humidity,
//               data->out_humidity,
//               get_temperature(data->in_temp, UNIT_TYPE_IS_METRIC),
//               get_temperature(data->out_temp, UNIT_TYPE_IS_METRIC),
//               get_dew_point (data->out_temp, data->out_humidity, UNIT_TYPE_IS_METRIC),
//               get_wind_chill (data, UNIT_TYPE_IS_METRIC),
//               get_wind_speed(data->wind_speed, UNIT_TYPE_IS_METRIC),
//               get_wind_speed(data->wind_gust, UNIT_TYPE_IS_METRIC),
//               get_wind_direction (data->wind_dir, WIND_AS_TEXT),
//               get_pressure (data->abs_pressure),
//               get_pressure (data->rel_pressure),
//               get_rainfall (data->last_hour_rain_fall, UNIT_TYPE_IS_METRIC),
//               get_rainfall (data->last_24_hr_rain_fall, UNIT_TYPE_IS_METRIC),
//               get_rainfall (data->total_rain_fall, UNIT_TYPE_IS_METRIC),
//               data->readBytes);
//
//    dbResult = PQexec (psql, queryString);
//
//    // If the command was successful
//    if (PQresultStatus (dbResult) == PGRES_COMMAND_OK )
//    {
//        logger (LOG_DBASE, __func__, "Entered into Database ok", NULL);
//    }
//    else
//    {
//        logger (LOG_ERROR, __func__, "Command Failed:: %s", PQerrorMessage(psql));
//        ret = -1;
//    }
//    free (queryString);
//
//    return ret;
// }

static int connect_to_mysql (dbase_config_t *dbconfig)
{
    char *debug_con_string; // For debugging only, does not contain password
    int ret;

    // Do a separate buffer for screen output that does not contain the password
    asprintf (&debug_con_string, "host=%s dbname=%s port=%d user=%s password=%s", dbconfig->host, dbconfig->dbname,
              dbconfig->port, dbconfig->user, "******");

    logger (LOG_DBASE, __func__, "Sending connection string:: %s", debug_con_string);
    logger (LOG_DBASE, __func__, "Attempting to connect to database server", NULL);

    if (mysql_init (&mysql) == NULL)
    {
        logger (LOG_ERROR, __func__, "Couldn't initialise mysql handle");
        ret = -1;
    }
    else
    {
        if (!mysql_real_connect (&mysql, dbconfig->host, dbconfig->user, dbconfig->password, dbconfig->dbname, dbconfig->port, NULL, 0))
        {
            logger (LOG_ERROR, __func__, "Couldn't connect to mysql");
            ret = -1;
        }
        else
        {
            logger (LOG_DBASE, __func__, "Connection made to the database", NULL);
        }
    }

    free (debug_con_string);

    return ret;
}

// makes the database connection
static int database_connect (dbase_config_t *dbconfig)
{
    /* TODO: If enough interest, could do a switch here to connect to postgresql database server instead */
    return connect_to_mysql (dbconfig);
}

static int database_insert_mysql_weather_data (char *table, weather_t *data)
{
    char *queryString;

    asprintf (&queryString,
              "INSERT INTO %s (time, in_humidity, out_humidity, in_temperature,"
              "out_temperature, out_dew_temperature, windchill_temperature, wind_speed"
              ", wind_gust, wind_direction, pressure, rel_pressure, rain_last_hour"
              ", rain_last_24h, rain_total, bytes) values (now(), %d, %d, %0.1f, %0.1f" \
              ", '%0.1f', '%0.1f', '%0.1f', '%0.1f', '%s', '%0.1f', '%0.1f', '%0.1f','%0.1f', '%0.1f', '%s');",
              table,
              data->in_humidity,
              data->out_humidity,
              get_temperature(data->in_temp, UNIT_TYPE_IS_METRIC),
              get_temperature(data->out_temp, UNIT_TYPE_IS_METRIC),
              get_dew_point (data->out_temp, data->out_humidity, UNIT_TYPE_IS_METRIC),
              get_wind_chill (data, UNIT_TYPE_IS_METRIC),
              get_wind_speed(data->wind_speed, UNIT_TYPE_IS_METRIC),
              get_wind_speed(data->wind_gust, UNIT_TYPE_IS_METRIC),
              get_wind_direction (data->wind_dir, WIND_AS_TEXT),
              get_pressure (data->abs_pressure),
              get_pressure (data->rel_pressure),
              get_rainfall (data->last_hour_rain_fall, UNIT_TYPE_IS_METRIC),
              get_rainfall (data->last_24_hr_rain_fall, UNIT_TYPE_IS_METRIC),
              get_rainfall (data->total_rain_fall, UNIT_TYPE_IS_METRIC),
              data->readBytes);

    logger(LOG_DEBUG, __func__, "Query String:: %s", queryString);

    if (mysql_query (&mysql, queryString) == 0)
    {
        logger(LOG_INFO, __func__, "Entered into Database ok", NULL);
    }
    else
    {
        logger(LOG_ERROR, __func__, "Command Failed:: %s", mysql_error(&mysql));
    }

    return 0;
}

static int database_insert_weather_data (char *table, weather_t *data)
{
    /* TODO: If enough interest, could do a switch here to insert to postgresql database server instead */
    return database_insert_mysql_weather_data (table, data);
}

// inserts collected values into the database
int database_insert (dbase_config_t *dbconfig, weather_t *weather)
{
    if (!weather)
    {
        logger (LOG_ERROR, __func__, "Weather param was NULL", NULL);
        return -1;
    }

    if (database_connect (dbconfig) < 0)
    {
        logger (LOG_ERROR, __func__, "Error connecting to database", NULL);
        return -1;
    }

    logger (LOG_DBASE, __func__, "Connection to database successful", NULL);

    logger (LOG_DBASE, __func__, "Sending database values", NULL);

    return database_insert_weather_data (dbconfig->table, weather);
}

static int check_config_value (char *name, char *value)
{
    char *secret = "******";
    if (value == NULL)
    {
        logger (LOG_ERROR, __func__, "Config file is missing option %s", name);
        return -1;
    }

    logger (LOG_DEBUG, __func__, "Config file key %s = %s", name,
            strcmp("password", name) == 0 ? secret : value);

    return 0;
}

int validate_db_config (dbase_config_t *dbase_config)
{
    dbase_fields_t field = DB_USER;
    int ret = 0;

    logger (LOG_DEBUG, __func__, "Validating configuration options from config file", NULL);

    while (field < DB_FIELD_MAX && ret >= 0)
    {
        switch (field)
        {
        case DB_USER:
            RETURN_IF_ERROR (check_config_value ("dbuser", dbase_config->user));
            break;

        case DB_NAME:
            RETURN_IF_ERROR (check_config_value ("dbname", dbase_config->dbname));
            break;

        case DB_TABLE:
            RETURN_IF_ERROR (check_config_value ("table", dbase_config->table));
            break;

        case DB_PORT:
            if (dbase_config->port == 0)
            {
                logger (LOG_ERROR, __func__, "Config file is missing option %s", "port");
                ret = -1;
            }
            break;

        case DB_HOST:
            RETURN_IF_ERROR (check_config_value ("host", dbase_config->host));
            break;

        case DB_PASSWORD:
            RETURN_IF_ERROR (check_config_value ("password", dbase_config->password));
            break;
        default:
            ret = -1;
        }
        field++;
    }

    return ret;
}

static void database_copy_config_value (char *src, char **dest, char *name)
{
    char *secret = "******";

    if (asprintf (dest, "%s", src))
    {
        logger (LOG_DEBUG, __func__, "Found %s=`%s`", name, strcmp ("password", name) == 0 ? secret : src);
    }
}

int database_init (FILE *config_file, dbase_config_t *dbase_config)
{
    int i;
    char _buffer[BUFSIZ];
    int ret;
    char line[BUFSIZ];

    logger (LOG_DEBUG, __func__, "Looking for database options");

    // Get a line from the configuration file
    while (fgets (line, sizeof(line), config_file) != NULL)
    {
        logger (LOG_DEBUG, __func__, "Checking for parameters line #", rtrim (line));

        // Detect if there is a comment present
        if (line[0] != '#')
        {

            i = config_get_next_line (line, &_buffer[0]);

            // Copy the value to the database array
            if (strstr (line, "host"))
            {
                i > 0 ? database_copy_config_value (_buffer, &dbase_config->host, "host") : NULL;
            }

            if (strstr (line, "dbname"))
            {
                i > 0 ? database_copy_config_value (_buffer, &dbase_config->dbname, "dbname") : NULL;
            }

            if (strstr (line, "table"))
            {
                i > 0 ? database_copy_config_value (_buffer, &dbase_config->table, "table") : NULL;
            }

            if (strstr (line, "port"))
            {
                i > 0 ? dbase_config->port = strtol (_buffer, NULL, 10) : 0;
            }

            if (strstr (line, "dbuser"))
            {
                i > 0 ? database_copy_config_value (_buffer, &dbase_config->user, "user") : NULL;
            }

            if (strstr (line, "password"))
            {
                i > 0 ? database_copy_config_value (_buffer, &dbase_config->password, "password") : NULL;
            }

            // Clear our buffer for next time around
            memset (_buffer, 0, sizeof(_buffer));
        }
    }

    /* Are we missing something ? */
    ret = validate_db_config (dbase_config);

    return ret;
}
