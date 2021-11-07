/*
  Re-write of the wwsr program
  Grant McEwan
  Version 0.10
*/
// Version number, used by the help output

// V0.10
// Refactoring code more

// Include standard libraries
#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <stdbool.h>
#include <ctype.h>

// Include all user defined libraries
#include "common.h"
#include "config.h"
#include "wwsr.h"
#include "wwsr_usb.h"
#include "logger.h"
#include "weather_processing.h"
#include "database.h"
#include "wunderground.h"

#define WS_STORED_READINGS 27
#define WS_CURRENT_ENTRY  30
#define WS_TIME_DATE 43
#define WS_ABS_PRESSURE 34
#define WS_REL_PRESSURE 32
#define WS_MIN_ENTRY_ADDR 0x0100
#define WS_MAX_ENTRY_ADDR 0xFFF0

#define HOURS 3
#define MINUTES 4

#define BUF_SIZE 0x10

// the device might get claimed by the kernel, add the following udev rule so that we can access it without
// being root. Add in /etc/udev/rules.d/<rule_name> eg /etc/udev/rules.d/90-weather.rules
// # WH-1081 Weather Station
// SUBSYSTEM=="usb", ENV{DEVTYPE}=="usb_device", ATTRS{idVendor}=="1941", ATTRS{idProduct}=="8021", GROUP="plugdev", MODE="660"
// LABEL="weather_station_end"

static void putToDatabase (weather_t *weather)
{
    if (connectToDatabase())
    {
        logger (LOG_DEBUG, logType, "putToDatabase", "Connection to database successful", NULL);

        logger (LOG_DEBUG, logType, "putToDatabase", "Sending database values", NULL);

        insertIntoDatabase(weather);
    }
    else
    {
        logger (LOG_ERROR, logType, "putToDatabase", "Error connecting to database", NULL);
    }
}

static void wwsr_get_current_record_data (uint16_t *address, uint8_t *buffer, uint8_t size)
{
    //check for buffer owerflow
    if ((*address > WS_MAX_ENTRY_ADDR) || (*address < WS_MIN_ENTRY_ADDR))
    {
        *address = (*address && 0xFFFF) + WS_MIN_ENTRY_ADDR;
    }

    wwsr_usb_read (*address, buffer, size);
}

static
void wwsr_get_first_record_data (uint16_t *address, uint16_t stored_readings,
                                 uint8_t *buffer, uint8_t size)
{
    uint16_t _address = *address - ((stored_readings - 1) * 16);

    if ((_address > WS_MAX_ENTRY_ADDR) || (_address < WS_MIN_ENTRY_ADDR))
    {
        _address = (_address && 0xFFFF) + WS_MIN_ENTRY_ADDR;
    }

    wwsr_usb_read (_address, buffer, size);
}

static
void wwsr_get_1hr_record_data (uint16_t *address, uint16_t periods,
                                 uint8_t *buffer, uint8_t size)
{
    // 1 Hr pointer address
    // (60mins / 5min records) * 16 bytes of data
    uint16_t _address = *address - ((periods + 5) * 16);

    //check for buffer owerflow
    if ((_address > WS_MAX_ENTRY_ADDR) || (_address < WS_MIN_ENTRY_ADDR))
    {
        _address = (_address && 0xFFFF) + WS_MIN_ENTRY_ADDR;
    }

    //read -1h buffer (in real 30-59 min ago)
    wwsr_usb_read (_address, buffer, size);
}

static
void wwsr_get_24hr_record_data (uint16_t *address, uint8_t *buffer, uint8_t size)
{
    // 24 hr pointer address
    // ((60mins / 5min records) * 24 Hrs) * 16 bytes of data
    uint16_t _address = *address - 4608;

    //check for buffer owerflow
    if ((_address > WS_MAX_ENTRY_ADDR) || (_address < WS_MIN_ENTRY_ADDR))
    {
        _address = (_address && 0xFFFF) + WS_MIN_ENTRY_ADDR;
    }

    //read -1h buffer (in real 30-59 min ago)
    wwsr_usb_read (_address, buffer, size);
}

static void putToScreen (weather_t *weather)
{
    char Date[BUFSIZ];

    getTime (Date, sizeof(Date));

    printf ("Mins since last stored reading::  %s\n", weather->last_read);
    printf ("Current Time::                    %s\n", Date);
    printf ("Humidity Inside::                 %d%%\n", weather->in_humidity);
    printf ("Humidity Outside::                %d%%\n", weather->out_humidity);
    printf ("Temperature Inside::              %0.1fºC\n", get_temperature(weather->in_temp, UNIT_TYPE_IS_METRIC));
    printf ("Temperature Outside::             %0.1fºC\n", get_temperature(weather->out_temp, UNIT_TYPE_IS_METRIC));
    printf ("Dew Point Temperature::           %0.1fºC\n", weather->dew_point);
    printf ("Feels like Temperature::          %0.1fºC\n", weather->wind_chill);
    printf ("Wind Speed::                      %0.1f km/h\n", get_wind_speed(weather->wind_speed, UNIT_TYPE_IS_METRIC));
    printf ("Wind Gust::                       %0.1f km/h\n", get_wind_speed(weather->wind_gust, UNIT_TYPE_IS_METRIC));
    printf ("Wind Direction::                  %s\n", get_wind_direction (weather->wind_dir, WIND_AS_TEXT));
    printf ("Wind Direction (Degrees)::        %s\n", get_wind_direction (weather->wind_dir, WIND_AS_DEGREES));
    printf ("Abs Pressure::                    %0.1f hPa\n", weather->abs_pressure);
    printf ("Relative Pressure::               %0.1f hPa\n", weather->rel_pressure);
    printf ("Last 1Hr Rain Fall::              %0.1f mm\n", weather->last_hour_rain_fall);
    printf ("Last 24Hr Rain Fall::             %0.1f mm\n", weather->last_24_hr_rain_fall);
    printf ("Total Rain Fall::                 %0.1f mm\n", weather->total_rain_fall);
}

static void wwsr_show_bytes (bool show_header, char* title, uint16_t address, uint8_t *byte_array)
{
    char buffer[220];
    int pos = 0;
    int i;

    if (show_header)
    {
        pos = sprintf(buffer, "\t %-*s| 00 | 01 | 02 | 03 | 04 | 05 | 06 | 07 | 08 | 09 | 0A | 0B | 0C | 0D | 0E | 0F |\n", 15, " ");
    }

    pos += sprintf (buffer + pos, "%-15s %3.04X:   ", title, address);

    for (i = 0; i <= 0x0f; i++)
    {
        pos += sprintf (buffer + pos, "| %02X ", byte_array[i]);
    }

    sprintf(buffer + pos, "|");
    printf ("%s\n", buffer);
}

static int hex2dec (int hexByte)
{
    int decimalValue;

    int hexByteLow = hexByte & 0x0F;

    int hexByteHigh = (hexByte >> 4) & 0x0F;

    decimalValue = 0;

    while (hexByteHigh-- > 0)
    {
        decimalValue += 10;
    }

    while (hexByteLow-- > 0)
    {
        decimalValue++;
    }

    return decimalValue;
}

int main (int argc, char **argv)
{
    config_t config;
    g_show_debug_bytes = 0;
    log_sort.usb = 0;
    log_sort.all = 0;
    log_sort.database = 0;
    int ret;
    log_event log_level;
    char *buf = NULL;

    weather_t weather;

    struct {
        uint16_t entry_address;
        uint16_t num_of_stored_readings;
        char last_store_time[5]; // Format
        int five_min_periods;
    } current;

    // Global switch between imperial and metric measurements
    g_AsImperial = 0;

    ret = config_get_options (argc, argv, &config);

    if (ret == NO_OPTIONS_SELECTED)
    {
        exit(1);
    }

    log_level = config_get_log_level ();

    logger (LOG_DEBUG, logType, "Main", "Attempting to Opening the USB", NULL);

    // if the usbStatus is 0 - it returned opened
    if (wwsr_usb_open () != 0)
    {
        logger (LOG_DEBUG, logType, "Main", "Usb Failed to open", NULL);
    }
    else
    {
        logger (LOG_USB, log_level, __func__, "Usb opened successfully, retrieving data", NULL);

        // initialise address positions
        // current address in device, new address and new address -1h and - 24h (for rainfall computation)
        uint16_t pFirstRecord, current_base_ptr, p1HrRecord, p24HrRecord;

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

        // The current address being written to is held at WS_CURRENT_ENTRY
        // get the current memory address being written to in the device
        wwsr_usb_read_value (WS_CURRENT_ENTRY, &current.entry_address);

        wwsr_usb_read_value (WS_STORED_READINGS, &current.num_of_stored_readings);

        // Get the current time from the device
        logger (LOG_DEBUG, log_level, __func__, "Getting Weather Stations Time", NULL);

        wwsr_usb_read (WS_TIME_DATE, &current.last_store_time[0], sizeof(current.last_store_time));

        asprintf (&buf, "Weather Station's last memory store time: %02X:%02X ", current.last_store_time[HOURS], current.last_store_time[MINUTES]);

        logger (LOG_DEBUG, log_level, __func__, buf, NULL);

        // divide the current time stamp by the amount of 5 min intervals that could have occurred
        current.five_min_periods = hex2dec (current.last_store_time[MINUTES]) / 5;

        asprintf (&buf, "Number of 5 min periods so far this hour = %d", current.five_min_periods);

        logger (LOG_DEBUG, log_level, __func__, buf, NULL);

        asprintf (&buf, "1hr pointer will be 0x%04X", current.entry_address - ((current.five_min_periods + 5) * 16));

        logger (LOG_DEBUG, log_level, __func__, buf, NULL);

        // current address
        current_base_ptr = current.entry_address;

        wwsr_get_current_record_data (&current.entry_address, &_CurrentBuffer[0], sizeof(_CurrentBuffer));

        wwsr_get_first_record_data (&current.entry_address, current.num_of_stored_readings,
                                   &_FirstRecordBuffer[0], sizeof(_FirstRecordBuffer));

        wwsr_get_1hr_record_data (&current.entry_address, current.five_min_periods,
                                   &_1HrBuffer[0], sizeof(_1HrBuffer));

        wwsr_get_24hr_record_data (&current.entry_address, &_24HrBuffer[0], sizeof(_24HrBuffer));

        // over writing the pressure values with what is on the screen instead here.
        wwsr_usb_read (WS_ABS_PRESSURE, (unsigned char *)&_absPressure, sizeof(_absPressure));

        wwsr_usb_read (WS_REL_PRESSURE, (unsigned char *)&_relPressure, sizeof(_relPressure));

        // Over write the current buffer with the values from the screen
        _CurrentBuffer[PRESSURE_LOW_BYTE] = _relPressure;

        _CurrentBuffer[ABS_PRESSURE_LOW_BYTE] = _absPressure;

        _CurrentBuffer[ABS_PRESSURE_HIGH_BYTE] = _absPressure >> 8;

        int i = 0;

        // Process the data
        i = processData (&weather, _CurrentBuffer, _1HrBuffer, _24HrBuffer);

        // if process data failed
        if (i == 0)
        {
            if (log_sort.all) logger (LOG_DEBUG, logType, "Main", "Error processing data!", NULL);
            return -1;
        }

        weather.bytePtr = weather.readBytes;

        for (i = 0; i <= 15; i++)
        {
            if (i > 0)
            {
                weather.bytePtr += sprintf (weather.bytePtr, ":");
            }
            weather.bytePtr += sprintf (weather.bytePtr, "%02X", _CurrentBuffer[i]);
        }

        // weather.bytePtr += sprintf (weather.bytePtr, '\0');

        if (config.log_type.bytes || log_level == LOG_DEBUG)
        {
            printf ("byteString to go into database = %s\n", weather.readBytes);

            printf ("Currently have %04X (%d) stored readings\n", current.num_of_stored_readings, current.num_of_stored_readings);

            wwsr_show_bytes (true, "First Record", pFirstRecord, &_FirstRecordBuffer[0]);
            wwsr_show_bytes (false, "Current Record", current_base_ptr, &_CurrentBuffer[0]);
            wwsr_show_bytes (false, "1Hr Record", p1HrRecord, &_1HrBuffer[0]);
            wwsr_show_bytes (false, "24Hr Record", p24HrRecord, &_24HrBuffer[0]);
            // Tidies up the last output, gives a bit of space
            printf("\n");
        }

        if (config.send_to_wunderground)
        {
            send_to_wunderground (&config.wunderground_config, &weather);
        }

        // check to see if we should only print to the screen
        if (config.print_to_screen)
        {
            putToScreen (&weather);
        }
        else
        {
            // Put data to database
            logger (LOG_DEBUG, logType, "Main", "Putting values to the database", NULL);

            putToDatabase(&weather);
        }
    }

    wwsr_usb_close ();

    if (log_sort.all || log_sort.usb) logger (LOG_DEBUG, logType, "Main", "Closing the USB", NULL);

}

size_t getTime ( char* str, size_t len )
{
    // initialise the time struct.
    time_t t;

    // get the time from the pc
    time (&t);

    // return the value.
    return strftime ( str, len, "%Y-%m-%d %H:%M:%S", localtime(&t) );
}

size_t getEpochTime (char* str, size_t len)
{
    // initialise the time struct.
    time_t t;

    // get the time from the pc
    time (&t);

    // return the value.
    return strftime( str, len, "%s", localtime(&t));
}
