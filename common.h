#ifndef COMMON_H
#define COMMON_H

#define VERSION "0.10"

#include <stdbool.h>
#include <string.h>
#include <stdint.h>

#include "database.h"
#include "wunderground.h"
/*
 *
 * Common variables used across the weather program
 */

#define RETURN_IF_ERROR(f) do { \
  int retval = (f); \
  if (retval < 0) { \
    return -1;      \
  }                 \
} while (0)         \

typedef struct weather {
    char last_read[3];
    uint8_t in_humidity;
    uint8_t out_humidity;
    uint16_t in_temp;
    uint16_t out_temp;
    float wind_chill;
    uint8_t wind_speed;
    uint8_t wind_gust;
    uint8_t wind_dir;
    float abs_pressure;
    float rel_pressure;
    int rainfallTicks;
    float last_hour_rain_fall;
    float last_24_hr_rain_fall;
    float total_rain_fall;
    int resultIndex;
    char readBytes[46];
    char *bytePtr;
} weather_t;

#define NO_OPTIONS_SELECTED -1

typedef struct log_type
{
    bool usb;
    bool bytes;
    bool database;
    bool all;
} log_type_t;

/* Configuration options */
typedef struct config {
    bool print_to_screen;
    bool send_to_wunderground;
    bool show_debug_bytes; /* TODO: this probably should be a log level? */
    bool show_as_imperial;
    int log_level;
    log_type_t log_type;
    dbase_config_t dbase_config;
    wunderground_config_t wunderground_config;
} config_t;

#define UNIT_TYPE_IS_METRIC     0
#define UNIT_TYPE_IS_IMPERIAL   1

#define WIND_AS_DEGREES         0
#define WIND_AS_TEXT            1

// Buffer value positions
#define LAST_READ_BYTE    0x00
#define INSIDE_HUMIDITY_BYTE  0x01
#define INSIDE_TEMP_LOW_BYTE  0x02
#define INSIDE_TEMP_HIGH_BYTE   0x03
#define OUTSIDE_HUMIDITY_BYTE   0x04
#define OUTSIDE_TEMP_LOW_BYTE   0x05
#define OUTSIDE_TEMP_HIGH_BYTE 0x06
#define PRESSURE_LOW_BYTE   0x07
#define PRESSURE_HIGH_BYTE  0x08
#define WIND_SPEED_BYTE   0x09
#define WIND_GUST_BYTE    0x0A
#define ABS_PRESSURE_HIGH_BYTE 0x0B
#define WIND_DIR_BYTE     0x0C
#define RAIN_TICK_LOW_BYTE  0x0D
#define RAIN_TICK_HIGH_BYTE   0x0E
#define ABS_PRESSURE_LOW_BYTE  0x0F

#define INSIDE_TEMPERATURE  1
#define OUTSIDE_TEMPERATURE 0


#endif /* End of header guard */
