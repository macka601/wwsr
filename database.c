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

PGconn *psql;

// inserts collected values into the database
int insertIntoDatabase ()
{
    log_event log_level = config_get_log_level ();
    // initialise the result pointer
    PGresult *dbResult;

    // Query String buffer
    char queryString[BUFSIZ];

    // Date string buffer
    char dateString[60];

    // Get the date string
    getTime(dateString, sizeof(dateString));

    /* TODO: Weather items now stored as ints etc.. change this over */
    // this is the query string, writes all variables to the string
    // snprintf(queryString, sizeof(queryString), "INSERT INTO %s (\"time\", \"in_humidity\", \"out_humidity\", \"in_temperature\","
    //  "\"out_temperature\", \"out_dew_temperature\", \"windchill_temperature\", \"wind_speed\""
    //  ", \"wind_gust\", \"wind_direction\", \"pressure\", \"rel_pressure\", \"rain_last_hour\""
    //  ", \"rain_last_24h\", \"rain_total\",\"bytes\") values (now(), '%d', '%d', '%0.1f', %0.1f" \
    //  ", '%0.1f', '%0.1f', '%0.1f', '%0.1f', '%s', '%0.1f', '%0.1f', '%0.1f','%0.1f', '%0.1f', '%s');",
    //  dbase.table, w->in_humidity, w->out_humidity, w->in_temp,
    //  w->out_temp, w->dew_point, w->wind_chill, w->wind_speed, w->wind_gust,
    //  w->wind_dir, w->abs_pressure, w->rel_pressure, w->last_hour_rain_fall,
    //  w->last_24_hr_rain_fall, w->total_rain_fall, w->readBytes);

    // If debug turned on, show the connection string
    if (log_sort.all || log_sort.database) logger (LOG_DEBUG, log_level, "insertIntoDatabase", "Query String:: %s", queryString);

    // execute the connection string
    dbResult = PQexec (psql, queryString);

    // If the command was successful
    if (PQresultStatus (dbResult) == PGRES_COMMAND_OK )
    {
        // If debug turned on, announce success
        if (log_sort.all || log_sort.database) logger (LOG_DEBUG, log_level, "insertIntoDatabase", "Entered into Database ok", NULL);
    }
    else
    {
        // command failed, write error to error output
        if (log_sort.all || log_sort.database) logger (LOG_DEBUG, log_level, "insertIntoDatabase", "Command Failed:: %s", PQerrorMessage(psql));
    }
}

// makes the database connection
int connectToDatabase ()
{
    log_event log_level = config_get_log_level ();
    // This is the buffer going to the server
    char connectionInfo[BUFSIZ];
    // This is the buffer going to screen.. changes the password, so its not displayed on screen
    char debugConnectionInfo[BUFSIZ];

    // sprintf(connectionInfo, "host=%s dbname=%s port=%s user=%s password=%s", dbase.host, dbase.dbname,
    //  dbase.port, dbase.user, dbase.password);

    // sprintf(debugConnectionInfo, "host=%s dbname=%s port=%s user=%s password=%s", dbase.host, dbase.dbname,
    //  dbase.port, dbase.user, "*****");

    // writes the connection string information to debug
    if (log_sort.all || log_sort.database) logger (LOG_DEBUG, log_level, "connectToDatabase", "Sending connection string:: %s", debugConnectionInfo);
    if (log_sort.all || log_sort.database) logger (LOG_DEBUG, log_level, "connectToDatabase", "Attempting to connect to database server", NULL);

    /* init connection */
    psql = PQconnectdb(connectionInfo);

    // Connection failed
    if (PQstatus (psql) != CONNECTION_OK) {
        // Connection error, put the error to error output
        if (log_sort.all || log_sort.database) {
            logger (LOG_DEBUG, log_level, "connectToDatabase", "libpq error: PQstatus(psql) != CONNECTION_OK", NULL);
            logger (LOG_DEBUG, log_level, "connectToDatabase", "PSQL Error Message: '%s'", PQerrorMessage(psql));
        }
        exit(0);
    } else {
        if (log_sort.all || log_sort.database) logger (LOG_DEBUG, log_level, "connectToDatabase", "Connection made to the database", NULL);
        // connection to the database was good, write to debug output
        return 1;
    }
}

static int check_config_value (char *name, char *value)
{
    char *secret = "******";
    log_event log_level = config_get_log_level ();
    if (value == NULL)
    {
        logger (LOG_ERROR, log_level, __func__, "Config file is missing option %s", name);
        return -1;
    }

    logger (LOG_INFO, log_level, __func__, "Config file key %s = %s", name,
            strcmp("password", name) == 0 ? secret : value);

    return 0;
}

int validate_db_config (dbase_config_t *dbase_config)
{
    dbase_fields_t field = DB_USER;
    int ret = 0;

    logger (LOG_INFO, config_get_log_level (), __func__, "Validating configuration options from config file", NULL);

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
            RETURN_IF_ERROR (check_config_value ("port", dbase_config->port));
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
    log_event log_level = config_get_log_level();
    char *secret = "******";

    if (asprintf (dest, "%s", src))
    {
        logger (LOG_DEBUG, log_level, __func__, "Found %s=`%s`", name, strcmp ("password", name) == 0 ? secret : src);
    }
}

int database_init (FILE *config_file, dbase_config_t *dbase_config)
{
    int i;
    char _buffer[BUFSIZ];
    int ret;
    char line[BUFSIZ];

    log_event log_level = config_get_log_level ();

    logger (LOG_DEBUG, log_level, __func__, "Looking for database options");

    // Get a line from the configuration file
    while (fgets (line, sizeof(line), config_file) != NULL)
    {
        logger (LOG_DEBUG, log_level, __func__, "Checking for parameters line #", rtrim (line));

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
                i > 0 ? database_copy_config_value (_buffer, &dbase_config->port, "port") : NULL;
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
