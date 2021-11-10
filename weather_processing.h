#ifndef WEATHER_PROCESSING_H
#define WEATHER_PROCESSING_H

#include <stdint.h>

void process_data (weather_t *weather, int8_t *bufferCurrent, uint8_t *buffer1Hr, uint8_t *buffer24Hr);
void windDirection(char *windDirBuf, int windByte, int returnAsDegrees);
float get_temperature (uint16_t byte, bool unit_type);
float get_wind_speed (uint8_t byte, bool unit_type);
const char *get_wind_direction (uint8_t windByte, bool returnAsDegrees);
float get_pressure(uint16_t byte);
float get_dew_point(uint16_t temperature, uint8_t humidity, bool unit_type);
float get_wind_chill (weather_t *weather, bool unit_type);
float get_rainfall(uint16_t byte, bool unit_type);

extern int logType;

#endif /* end of header guard */
