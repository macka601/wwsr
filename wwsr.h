#ifndef _WWSR_H
#define _WWSR_H

/*
  wwsr.h 
  
  Contains all the byte locations
  
*/
#include <stdint.h>

int logType;
int g_AsImperial;
bool g_show_debug_bytes;

size_t wwsr_get_time (char* str, size_t len );
int changeWindDirectionToDegrees (char *directionAsText);

#endif /* end of header guard */
