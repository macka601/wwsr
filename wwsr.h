#ifndef _WWSR_H
#define _WWSR_H

/*
  wwsr.h 
  
  Contains all the byte locations
  
*/
#include <stdint.h>
// If you have a different usb device, change the codes here
#define VENDOR_ID    0x1941
#define PRODUCT_ID   0x8021
#define USB_OPEN	1
#define USB_CLOSED	0
#define BUF_SIZE 0x10

#define HOURS	3
#define MINUTES 4
#define WS_STORED_READINGS 27
#define WS_CURRENT_ENTRY  30
#define WS_TIME_DATE 43
#define WS_ABS_PRESSURE 34
#define WS_REL_PRESSURE 32
#define WS_MIN_ENTRY_ADDR 0x0100
#define WS_MAX_ENTRY_ADDR 0xFFF0

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
int processData(uint8_t *buffer,uint8_t *buffer2,uint8_t *buffer3, uint8_t *buffer4);
void putToDatabase();
void putToScreen();
void Database();
size_t getTime( char* str, size_t len );
size_t getEpochTime( char* str, size_t len );
size_t getTimeDifference( int timeDifference );
int hex2dec(int hexByte);
int changeWindDirectionToDegrees(char *directionAsText);


#endif /* end of header guard */
