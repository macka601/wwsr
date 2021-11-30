/* logging functions that can be provided to a program
 */

#include <stdarg.h>
#include <stdio.h>
#include "logger.h"
#include "config.h"

void logger (log_type_t log_type, const char *function, char *msg, ...)
{
    va_list args;
    va_start (args, msg);
    logs_enabled_t logs;

    logs = config_get_enabled_logs ();

    // Special case, where we don't set this as an enabled log type
    if ((log_type & LOG_ERROR) == LOG_ERROR)
    {
        fprintf (stdout, "error: wwsr.%s - ", function);
    }
    else
    {
        switch (log_type & logs)
        {
            case LOG_USB:
                fprintf (stdout, "usb: wwsr.%s - ", function);
            break;

            case LOG_DBASE:
                fprintf (stdout, "database: wwsr.%s - ", function);
            break;

            case LOG_BYTES:
                fprintf (stdout, "bytes: wwsr.%s - ", function);
            break;

            case LOG_INFO:
                fprintf (stdout, "info: wwsr.%s - ", function);
            break;

            case LOG_DEBUG:
                fprintf (stdout, "debug: wwsr.%s - ", function);
            break;
            case LOG_NONE:
            default:
                return;
            break;
        }
    }

    vfprintf (stdout, msg, args);
    fprintf (stdout, "\n");
    va_end (args);
}
