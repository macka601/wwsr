/* 
  Re-write of the wwsr program 
  Grant McEwan  
  Version 0.08
  
*/
// Version number, used by the help output
#define VERSION "0.09"
// V0.09
// Fixed a small issue with sending the password to the database and displaying *** on screen
// V0.08
// Improving on logging
// V0.07
// Fixed the bug where temperature over 25 deg would not be reported properly
// V0.06
// Splitting files up for another re-write	
// V0.05
// Fixed bug where temperature is out of range
// V0.04
// Added support for wunderground weather

// Include standard libraries
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <stdbool.h>

// Include all user defined libraries
#include "common.h"
#include "wwsr.h"
#include "usbFunctions.h"
#include "logger.h"
#include "weatherProcessing.h"
#include "database.h"
#include "wunderground.h"

// the device might get claimed by the kernel, add the following udev rule so that we can access it without
// being root. Add in /etc/udev/rules.d/<rule_name>
// # WH-1081 Weather Station
// #ACTION!="add|change", GOTO="weather_station_end"
// SUBSYSTEM=="usb_device", ATTRS{idVendor}=="1941", ATTRS{idProduct}=="8021", GROUP="plugdev", MODE="660"

// LABEL="weather_station_end"

// Global declerations
// -----
usb_dev_handle *DeviceHandle;

// usbStatus = 1 for open, 0 for closed
int usbStatus;
// default position in log is "now". Altering this can lead to read some of stored values in weather station
int position;	

int main( int argc, char **argv )
{  
	// variable that holds the options
	int option;  

	bool printToScreen = 0;
	bool sendToWunderGround = 0;
	g_show_debug_bytes = 0;
	log_sort.usb = 0;
	log_sort.all = 0;
	log_sort.database = 0;

	// Global switch between imperial and metric measurements
	g_AsImperial = 0;

	// redirecting the logging output 
	_log_error=stderr;

	// go into a while loop looking for options from the user
	while( ( option = getopt (argc, argv, "daupvxhwi?") ) != -1 )
	{
		// Change the ? to an h
		if (option == '?')
		{
			option = 'h';
		}

		switch( option )
		{   
			// Turn on database debugging
			case 'd':
				//redirecting the log output of _log_debug to the standard output				
				logType = LOG_DEBUG;
				log_sort.database = 1;
				// inform user that we are redirecting output
				logger(LOG_INFO, logType, "Main", "database debugging Enabled", NULL);		
			break;

			// Prints values, but doesn't put them to database
			case 'p':
				printf("Showing the current values from the weather station\n");
				printToScreen = 1;
			break;

			// All debug information on
			case 'a':
				logType = LOG_DEBUG;
				log_sort.all = 1;
				logger(LOG_DEBUG, logType, "Main", "debug flag ON", NULL);	     
			break;

			case 'x':
				logType = LOG_DEBUG;
				g_show_debug_bytes = 1;
				logger(LOG_DEBUG, logType, "Main", "g_debug_bytes flag ON", NULL);	 
			break;
			case 'w':
				sendToWunderGround = 1;		
			break;
	
			// Show as imperial
			case 'i':
				g_AsImperial = 1;		
			break;	   

			// g_debug information on
			case 'u':
				logType = LOG_USB;
				log_sort.usb = 1;
				logger(LOG_DEBUG, logType, "Main", "USB debug flag ON", NULL);	     
				break;

			// Print the options to the user
			case 'h':
				printf("Wireless Weather Station Reader\n");
				printf("Version: \t%s\t\n", VERSION );
				printf("Author:\t\tGrant McEwan\n\n" );
				printf("options are\n" );
				printf("-? -h\t\tDisplay this help\n" );
				printf("-x\t\tShow debug bytes\n" );		
				printf("-u\t\tShow usb debug information\n" );	
				printf("-d\t\tShow database debug information\n" );					
				printf("-a\t\tShow all debug Information\n");  
				printf("-p\t\tPrint values to screen only\n");
				printf("-w\t\tSend Values to WunderGround (see config file)\n");
				printf("-i\t\tShow values in imperial units\n");
				return 1;

			//set the default to abort
			default:
				abort();
		}	 
	} 

	if(log_sort.all || log_sort.database) logger( LOG_DEBUG, logType, "Main", "Opening the configuration file", NULL );

	openConfigFile();
	

	if(log_sort.usb) logger( LOG_DEBUG, logType, "Main", "Attempting to Opening the USB", NULL );

	usbStatus = usbOpen( &DeviceHandle, VENDOR_ID, PRODUCT_ID );

	// if the usbStatus is 0 - it returned opened
	if( usbStatus == 0 )
	{
		if(log_sort.usb == 1)
		{
			logger( LOG_DEBUG, logType, "Main", "Usb opened successfully", NULL );
			logger( LOG_DEBUG, logType, "Main", "Retrieving data", NULL );
		}
		
		// initialise address positions
		// current address in device, new address and new address -1h and - 24h (for rainfall computation)
		uint16_t pMemoryAddress, pFirstRecord, pCurrentRecord, p1HrRecord, p24HrRecord, pStoredReadings; 	   

		uint16_t pTimeAndDate;

		// set the size of the buffers
		uint8_t _CurrentBuffer[BUF_SIZE];

		// 30-60 min ago for 1h rainfall computation
		uint8_t _1HrBuffer[BUF_SIZE];


		// 1410-1440 min ago for 24h rainfall computation
		uint8_t _24HrBuffer[BUF_SIZE]; 

		// First record created.
		uint8_t _FirstRecordBuffer[BUF_SIZE]; 

		uint16_t  _absPressure;

		uint16_t _relPressure;

		char _TimeAndDate[5]; 

		// The current address being written to is held at WS_CURRENT_ENTRY
		// get the current memory address being written to in the device
		usbStatus = usbRead(DeviceHandle, WS_CURRENT_ENTRY, (unsigned char *)&pMemoryAddress, sizeof(pMemoryAddress));

		usbStatus = usbRead(DeviceHandle, WS_STORED_READINGS, (unsigned char *)&pStoredReadings, sizeof(pStoredReadings)); 

		// Get the current time from the device
		if(log_sort.all) logger( LOG_DEBUG, logType, "Main", "Getting Weather Stations Time", NULL);

		usbStatus = usbRead(DeviceHandle, WS_TIME_DATE, _TimeAndDate, sizeof(_TimeAndDate));

		char _buff[BUFSIZ];

		sprintf(_buff, "Weather Station's Time: %02X:%02X ", _TimeAndDate[HOURS], \
		_TimeAndDate[MINUTES]);

		if(log_sort.all) logger( LOG_DEBUG, logType, "Main", _buff, NULL);

		// divide the current time stamp by the amount of 5 min intervals that could have occurred 
		int timeDifference = hex2dec(_TimeAndDate[4])/5;	  	 	  	    

		sprintf(_buff, "Number of 5 min periods so far this hour = %d", timeDifference);

		if(log_sort.all) logger(LOG_DEBUG, logType, "Main", _buff, NULL);

		sprintf(_buff, "1hr pointer will be 0x%04X", pMemoryAddress - ((timeDifference + 5) * 16));

		if(log_sort.all) logger(LOG_DEBUG, logType, "Main", _buff, NULL);

		// current address 
		pCurrentRecord = pMemoryAddress;         	   	    

		// 1 Hr pointer address 
		// (60mins / 5min records) * 16 bytes of data
		p1HrRecord = pMemoryAddress - ((timeDifference + 5) * 16);   

		// 24 hr pointer address 
		// ((60mins / 5min records) * 24 Hrs) * 16 bytes of data
		p24HrRecord = pMemoryAddress - 4608; 

		pFirstRecord = pMemoryAddress - ((pStoredReadings - 1) * 16);	   	   

		//check for buffer owerflow
		if ((pCurrentRecord > WS_MAX_ENTRY_ADDR) || (pCurrentRecord < WS_MIN_ENTRY_ADDR))     
		{
			pCurrentRecord = (pCurrentRecord && 0xFFFF) + WS_MIN_ENTRY_ADDR;	     
		}
		//check for buffer owerflow
		if ((p1HrRecord > WS_MAX_ENTRY_ADDR) || (p1HrRecord < WS_MIN_ENTRY_ADDR))   
		{
			p1HrRecord = (p1HrRecord && 0xFFFF) + WS_MIN_ENTRY_ADDR; 
		}
		//check for buffer owerflow
		if ((p24HrRecord > WS_MAX_ENTRY_ADDR) || (p24HrRecord < WS_MIN_ENTRY_ADDR))
		{
			p24HrRecord = (p24HrRecord && 0xFFFF) + WS_MIN_ENTRY_ADDR; 
		}

		if ((pFirstRecord > WS_MAX_ENTRY_ADDR) || (pFirstRecord < WS_MIN_ENTRY_ADDR))
		{
			pFirstRecord = (pFirstRecord && 0xFFFF) + WS_MIN_ENTRY_ADDR; 
		}	   

		//read current position
		if (usbStatus == 0)
		{
			usbStatus = usbRead(DeviceHandle, pCurrentRecord, _CurrentBuffer, sizeof(_CurrentBuffer));       	        

			//read -1h buffer (in real 30-59 min ago)		
			usbStatus = usbRead(DeviceHandle, p1HrRecord, _1HrBuffer, sizeof(_1HrBuffer));      

			//read -24h buffer (in real 23,5-24 h ago)		
			usbStatus = usbRead(DeviceHandle, p24HrRecord, _24HrBuffer, sizeof(_24HrBuffer));

			// Get the first record data
			usbStatus = usbRead(DeviceHandle, pFirstRecord, _FirstRecordBuffer, sizeof(_FirstRecordBuffer));     
		}		

		// over writing the pressure values with what is on the screen instead here.
		usbStatus = usbRead(DeviceHandle, WS_ABS_PRESSURE, (unsigned char *)&_absPressure, sizeof(_absPressure));

		usbStatus = usbRead(DeviceHandle, WS_REL_PRESSURE, (unsigned char *)&_relPressure, sizeof(_relPressure));

		// Over write the current buffer with the values from the screen    
		_CurrentBuffer[PRESSURE_LOW_BYTE] = _relPressure;

		_CurrentBuffer[ABS_PRESSURE_LOW_BYTE] = _absPressure;

		_CurrentBuffer[ABS_PRESSURE_HIGH_BYTE] = _absPressure >> 8;

		int i = 0;

		// Process the data
		i = processData(_FirstRecordBuffer, _CurrentBuffer, _1HrBuffer, _24HrBuffer);

		// if process data failed
		if(i == 0)
		{
			if(log_sort.all) logger(LOG_DEBUG, logType, "Main", "Error processing data!", NULL);
			return;
		}

		weather.bytePtr = weather.readBytes;

		for(i = 0; i<=15; i++)
		{
			if(i > 0) 
			{
				weather.bytePtr += sprintf(weather.bytePtr, ":");
			}
			weather.bytePtr += sprintf(weather.bytePtr, "%02X", _CurrentBuffer[i]);
		}

		weather.bytePtr += sprintf(weather.bytePtr, '\0');	   

		// This shows all the buffer information
		// Only comes on when the g_debug option (d) is set or when to show bytes (x))
		if(log_sort.all == 1 || g_show_debug_bytes == 1)
		{	     
			printf("byteString to go into database = %s\n", weather.readBytes);

			printf("Currently have %04X (%d) stored readings\n", pStoredReadings, pStoredReadings);

			printf("\t\t\tByte  | 00 | 01 | 02 | 03 | 04 | 05 | 06 | 07 | 08 | 09 | 0A | 0B | 0C | 0D | 0E | 0F |\n");

			printf("First Buffer Address\t%04X: ", pFirstRecord);

			for(i=0; i<sizeof(_FirstRecordBuffer); i++)
			{
				printf("| %02X ", _FirstRecordBuffer[i]);
			}

			printf("|\n");

			printf("Current Buffer Address\t%04X: ", pMemoryAddress);

			for(i=0; i<sizeof(_CurrentBuffer); i++)
			{
				printf("| %02X ", _CurrentBuffer[i]);
			}

			printf("|\n");

			printf("1Hr Buffer Address\t%04X: ", p1HrRecord);

			for(i=0; i<sizeof(_1HrBuffer); i++)
			{
				printf("| %02X ", _1HrBuffer[i]);
			}

			printf("|\n");

			printf("24Hr Buffer Address\t%04X: ", p24HrRecord);

			for(i=0; i<sizeof(_24HrBuffer); i++)
			{
				printf("| %02X ", _24HrBuffer[i]);
			}

			printf("|\n\n");
		} 	     

		if(sendToWunderGround == 1)
		{
			// sending to wunderground	     
			if(log_sort.all || log_sort.database) logger (LOG_DEBUG, logType, "Main", "values going to WunderGround", NULL);
			createAndSendToWunderGround(&weather, printToScreen);  
		}
		// check to see if we should only print to the screen
		if(printToScreen == 1)
		{		   
			putToScreen();
		}
		else
		{
			// Put data to database
			if(log_sort.all || log_sort.database)  logger (LOG_DEBUG, logType, "Main", "Putting values to the database", NULL);

			putToDatabase();	    	     
		}

	}
	// Otherwise it returned error'd
	else
	{
		if(log_sort.all || log_sort.usb) logger( LOG_DEBUG, logType, "Main", "Usb Failed to open", NULL );
	}

	usbClose( DeviceHandle );

	if(log_sort.all || log_sort.usb) logger( LOG_DEBUG, logType, "Main", "Closing the USB", NULL );

}


int processData(uint8_t *bufferFirst, uint8_t *bufferCurrent,uint8_t *buffer1Hr,uint8_t *buffer24Hr)
{
	// Get the last stored value time
	sprintf(weather.last_read, "%d", bufferCurrent[LAST_READ_BYTE]);
	
	if(log_sort.all) logger(LOG_DEBUG, logType, "processData", "Processing Data", NULL);

	weather.out_temp = getTemperature(bufferCurrent, OUTSIDE_TEMPERATURE, g_AsImperial);
		
	weather.in_temp = getTemperature(bufferCurrent, INSIDE_TEMPERATURE, g_AsImperial);
	
	// --- wind speed calculation --- //
	
	weather.wind_speed = getWindSpeed(bufferCurrent);

	// cast the buffer as a float to get the 0.1 accuracy
	// Then convert into km/h by multiplying by 3.6
	weather.wind_gust = getWindGust(bufferCurrent[WIND_GUST_BYTE]);

	// get the wind direction
//	char *ptr_dir = weather.wind_dir;
	
	windDirection(&weather.wind_dir[0], bufferCurrent[WIND_DIR_BYTE], 0);
	
//	ptr_dir = weather.wind_in_degrees;
	
	windDirection(&weather.wind_in_degrees[0], bufferCurrent[WIND_DIR_BYTE], 1);

	// ----- //
	
	// --- Humidity Calculations --- //
	// Get the humidity values out of the buffer

	weather.in_humidity = getHumidity(bufferCurrent[INSIDE_HUMIDITY_BYTE]);
	
	weather.out_humidity = getHumidity(bufferCurrent[OUTSIDE_HUMIDITY_BYTE]);
	
	// --- Dew Point Calculation --- //
	weather.dew_point = getDewPoint(weather.out_temp, weather.out_humidity);		 
		
	// --- Air Pressure Calculation --- //
	//weather.pressure = (bufferCurrent[PRESSURE_LOW_BYTE] + (bufferCurrent[PRESSURE_HIGH_BYTE] << 8)) / 10;
	weather.abs_pressure = getAbsPressure(bufferCurrent);
	
	// Relative pressure
	// abs. pressure
	weather.rel_pressure = getRelPressure(bufferCurrent); 

	weather.wind_chill = getWindChill(bufferCurrent, g_AsImperial);
			
	// --- Rain Calculation --- //
	// Rain is recorded in ticks since the batteries were inserted.
	weather.total_rain_fall = getTotalRainFall(bufferCurrent, g_AsImperial);
	
	// So now we want to get the previous hours rainfall
	weather.last_hour_rain_fall = getLastHoursRainFall(bufferCurrent, buffer1Hr, g_AsImperial);
	
	// So now we want to get the previous 24 hours rainfall
	weather.last_24_hr_rain_fall = getLast24HoursRainFall(bufferCurrent, buffer24Hr, g_AsImperial);
	
	if(log_sort.all) logger(LOG_DEBUG, logType, "ProcessData", "processed %d results", sizeof(weather));
	
	// success!
	return 1;
}

void putToDatabase()
{			
	if(connectToDatabase())
	{
	  if(log_sort.all || log_sort.database) logger(LOG_DEBUG, logType, "putToDatabase", "Connection to database successful", NULL);
	  
	  if(log_sort.all || log_sort.database) logger(LOG_DEBUG, logType, "putToDatabase", "Sending database values", NULL);

	  insertIntoDatabase(&weather);	  
	}
	else
	{
	  if(log_sort.all || log_sort.database) logger(LOG_ERROR, logType, "putToDatabase", "Error connecting to database", NULL);
	}
}


void putToScreen()
{	
	char Date[BUFSIZ];
	
	getTime(Date, sizeof(Date));
	
	printf("Mins since last stored reading::	%s\n", weather.last_read);
	printf("Current Time::				%s\n", Date);
	printf("Humidity Inside::			%d%%\n", weather.in_humidity);
	printf("Humidity Outside::			%d%%\n", weather.out_humidity);
	printf("Temperature Inside::			%0.1fºC\n", weather.in_temp);
	printf("Temperature Outside::			%0.1fºC\n", weather.out_temp);	
	printf("Dew Point Temperature::			%0.1fºC\n", weather.dew_point);	
	printf("Feels like Temperature::		%0.1fºC\n", weather.wind_chill);	
	printf("Wind Speed::				%0.1f km/h\n", weather.wind_speed);	
	printf("Wind Gust::				%0.1f km/h\n", weather.wind_gust);	
	printf("Wind Direction::			%s\n", weather.wind_dir);	
	printf("Wind Direction (Degrees)::		%s\n", weather.wind_in_degrees);	
	printf("Abs Pressure::				%0.1f hPa\n", weather.abs_pressure);	
	printf("Relative Pressure::			%0.1f hPa\n", weather.rel_pressure);
	printf("Last 1Hr Rain Fall::			%0.1f mm\n", weather.last_hour_rain_fall);	
	printf("Last 24Hr Rain Fall::			%0.1f mm\n", weather.last_24_hr_rain_fall);	
	printf("Total Rain Fall::			%0.1f mm\n", weather.total_rain_fall);	
}


size_t getTime( char* str, size_t len )
{
	// initialise the time struct.
    time_t t;
	
	// get the time from the pc
    time( &t );
	
	// return the value.
    return strftime( str, len, "%Y-%m-%d %H:%M:%S", localtime(&t) );
}

size_t getEpochTime( char* str, size_t len )
{
	// initialise the time struct.
    time_t t;
	
	// get the time from the pc
    time( &t );
	
	// return the value.
    return strftime( str, len, "%s", localtime(&t));
}

size_t getTimeDifference( int timeDifference )
{
	// initialise the time struct.
    time_t t;
	// get the time from the pc
    time( &t );
  
    //printf("time variable %ld\n", t);
	
}

int hex2dec(int hexByte)
{
    int decimalValue;
	
    int hexByteLow = hexByte & 0x0F;
    
    int hexByteHigh = (hexByte >> 4)& 0x0F;    
    
    decimalValue = 0;
    
    while(hexByteHigh-- > 0)
    {        
	decimalValue += 10;
    }
    
    while(hexByteLow-- >0)
    {
	decimalValue++;
    }
    
    return decimalValue;
}

/* Remove trailing whitespaces */
char *rtrim(char *const s)
{
        if(s && *s) {
                char *cur = s + strlen(s) - 1;
 
                while(cur != s && isspace(*cur))
                        --cur;
 
                cur[isspace(*cur) ? 0 : 1] = '\0';
        }
 
        return s;
}

