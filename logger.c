/* logging functions that can be provided to a program
 */

#include <stdarg.h>
#include <stdio.h>
#include "logger.h"

void logger (log_event event, log_event logType, const char *function, char *msg, ...)
{
    va_list args;

    va_start (args, msg);

    switch (event)
    {
    case LOG_DEBUG:
        if (logType == LOG_DEBUG)
        {
            fprintf (stdout , "message: wwsr.%s - ", function);
            vfprintf (stdout, msg, args);
            fprintf (stdout, "\n");
        }
        break;

    case LOG_USB:
        if (logType == LOG_USB || logType == LOG_DEBUG)
        {
            fprintf (stdout, "usb: wwsr.%s - ", function);
            vfprintf (stdout, msg, args);
            fprintf (stdout, "\n");
        }
        break;

    case LOG_WARNING:
        if (logType == LOG_WARNING)
        {
            fprintf (stdout, "warning: wwsr.%s - ", function);
            vfprintf (stdout, msg, args);
            fprintf (stdout, "\n");
        }
        break;

    case LOG_ERROR:
        if (logType == LOG_ERROR || logType == LOG_DEBUG)
        {
            fprintf (stdout, "error: wwsr.%s - ", function);
            vfprintf (stdout, msg, args);
            fprintf (stdout, "\n");
        }
        break;

    case LOG_INFO:
        if (logType == LOG_INFO || logType == LOG_DEBUG)
        {
            fprintf (stdout, "info: wwsr.%s - ", function);
            vfprintf (stdout, msg, args);
            fprintf (stdout, "\n");
        }
        break;

    }

    va_end (args);
}
