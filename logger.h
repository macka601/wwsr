/* Provides logging functions to the program
 */

#ifndef _LOGGER_H
#define _LOGGER_H

#include <stdbool.h>

typedef enum log_type
{
   LOG_NONE = 0,
   LOG_USB = 1,
   LOG_BYTES = 2,
   LOG_DBASE = 4,
   LOG_INFO = 8,
   LOG_ERROR = 16,
   LOG_DEBUG = 32
} log_type_t;

typedef int logs_enabled_t;

// include the logger module
void logger (log_type_t log_type, const char * function, char * msg, ...);

#endif /* end of logger header guard */
