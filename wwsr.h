#ifndef _WWSR_H
#define _WWSR_H

/*
  wwsr.h 
  
  Contains all the byte locations
  
*/
#include <stdint.h>

#define USB_OPEN	1
#define USB_CLOSED	0
#define BUF_SIZE 0x10

// Database defines
#define HOST 		0
#define PORT 		1
#define DBNAME 		2
#define USERNAME 	3
#define PASSWORD 	4

int logType;
extern struct log_sort log_sort;
char *rtrim(char *const s);
int g_AsImperial;
bool g_show_debug_bytes;

void Database();
size_t getTime( char* str, size_t len );
size_t getEpochTime( char* str, size_t len );
size_t getTimeDifference( int timeDifference );
int hex2dec(int hexByte);
int changeWindDirectionToDegrees(char *directionAsText);


#endif /* end of header guard */
