/*
 *Parses configuration from the command line
 */
#define __STDC_WANT_LIB_EXT2__ 1  //Define you want TR 24731-2:2010 extensions
#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include "config.h"
#include "common.h"
#include "logger.h"
#include "database.h"

// Hard code the file name for the config file
static char fileName[] = "config";
static int log_level = LOG_ERROR;

int config_get_next_line (char *line, char *value)
{
    int i;
    char *position;
    // Look for the delimiter that denotes where the value is
    position = strchr(line, '=');

    // initialise i
    i = 0;

    while (line[(++position) - line] != ';')
    {
        // put the value from the line
        value[i++] = line[position - line];
    }

    return i;
}

/* Remove trailing whitespaces */
char *rtrim (char *const s)
{
    if (s && *s)
    {
        char *cur = s + strlen (s) - 1;

        while (cur != s && isspace (*cur))
        {
            --cur;
        }

        cur[isspace (*cur) ? 0 : 1] = '\0';
    }

    return s;
}

static void copy_config_value (char *src, char **dest, char *name)
{
    if (asprintf (dest, "%s", src))
    {
        /* Treat the password a little differently */
        if (strcmp ("password", name) == 0)
        {
            logger (LOG_DEBUG, log_level, __func__, "Found %s=`*****`", name);
        }
        else
        {
            logger (LOG_DEBUG, log_level, __func__, "Found %s=`%s`", name, src);
        }

    }
}

static FILE * open_config_file (char *filename)
{
    FILE *config_file_handle = NULL;
    // Open the configuration file up for reading
    config_file_handle = fopen (filename, "r");

    // If the config file didn't open
    if (!config_file_handle)
    {
        // Spit an error
        logger (LOG_ERROR, log_level, __func__, "Error opening config file", NULL);

        return NULL;
    }

    return config_file_handle;
}

/* Takes command line args and turns them into configurations */
int config_get_options (int argc, char **argv, config_t *config)
{
    int option;
    int ret = 0;
    FILE *file_handle;

    while ((option = getopt (argc, argv, "daupvxhwi?")) != -1 && !config->print_to_screen)
    {
        // Change the ? to an h
        if (option == '?')
        {
            option = 'h';
        }

        switch (option)
        {
        case 'n':
            // User requested no log output
            log_level = LOG_NONE;
            break;
        // Turn on database debugging
        case 'd':
            //redirecting the log output of _log_debug to the standard output
            // config->log_level = LOG_DEBUG;
            config->log_type.database = 1;
            // inform user that we are redirecting output
            logger (LOG_INFO, LOG_DEBUG, __func__, "database debugging Enabled", NULL);
            break;

        // Prints values, but doesn't put them to database
        case 'p':
            printf ("Showing the current values from the weather station\n");
            config->print_to_screen = 1;
            break;

        // All debug information on
        case 'a':
            log_level = LOG_DEBUG;
            config->log_type.all = 1;
            logger (LOG_DEBUG, log_level, __func__, "debug flag ON", NULL);
            break;

        case 'x':
            // config->log_level = LOG_DEBUG;
            config->log_type.bytes = 1;
            logger (LOG_DEBUG, LOG_DEBUG, __func__, "g_debug_bytes flag ON", NULL);
            break;

        case 'w':
            config->send_to_wunderground = 1;
            break;

        case 'i':
            config->show_as_imperial = 1;
            break;

        case 'u':
            log_level = LOG_USB;
            config->log_type.usb = 1;
            logger (LOG_DEBUG, LOG_USB, __func__, "USB debug flag ON", NULL);
            break;

        // Print the options to the user
        case 'h':
        default:
            printf ("Wireless Weather Station Reader\n");
            printf ("Version: \t%s\t\n", VERSION);
            printf ("Author:\t\tGrant McEwan\n\n");
            printf ("options are\n");
            printf ("-? -h\t\tDisplay this help\n");
            printf ("-x\t\tShow debug bytes\n");
            printf ("-u\t\tShow usb debug information\n");
            printf ("-d\t\tShow database debug information\n");
            printf ("-a\t\tShow all debug Information\n");
            printf ("-n\t\tNo log output\n");
            printf ("-p\t\tPrint values to screen only\n");
            printf ("-w\t\tSend Values to WunderGround (see config file)\n");
            printf ("-i\t\tShow values in imperial units\n");
            return NO_OPTIONS_SELECTED;
        }
    }

    /* If we're not just printing directly to the screen, do some init */
    if (!config->print_to_screen)
    {
        file_handle = open_config_file (fileName);
        if (!file_handle)
        {
            logger (LOG_ERROR, log_level, __func__, "Error opening the config file", NULL);
            return -1;
        }

        /* Parse and get database configuration */
        ret = database_init (file_handle, &config->dbase_config);
        if (ret < 0)
        {
            fclose(file_handle);
            /* return early, dbase config invalid */
            return ret;
        }

        rewind (file_handle);
        
        if (config->send_to_wunderground)
        {
            ret = wunderground_init (file_handle, &config->wunderground_config);
            if (ret < 0)
            {
                fclose (file_handle);
                /* return early, wunderground config invalid */
                return ret;
            }
        }

        fclose (file_handle);
    }

    return ret;
}

int config_get_log_level (void)
{
    return log_level;
}
