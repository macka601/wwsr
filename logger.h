/* Provides logging functions to the program
 */

#ifndef _LOGGER_H
#define _LOGGER_H

#include <stdbool.h>

// define the log event types
typedef enum log_event
{
    LOG_DEBUG = 0,
    LOG_WARNING,
    LOG_ERROR,
    LOG_INFO,
    LOG_USB,
    LOG_NONE,
} log_event;

struct log_sort
{
    bool usb;
    bool bytes;
    bool database;
    bool all;
};

struct log_sort log_sort;
// include the logger module
void logger (log_event event, log_event logType, const char *function, char *msg,...);

static FILE *_log_debug=NULL,*_log_warning=NULL,*_log_error=NULL,*_log_info=NULL, *_log_usb=NULL;

#endif  /* end of logger header guard */
