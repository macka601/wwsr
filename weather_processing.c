/*
 * Weather processing file
 *
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "common.h"
#include "weather_processing.h"
#include "logger.h"
#include "config.h"

/* Returns the wind direction in either degrees or text */
const char * get_wind_direction (uint8_t windByte, bool returnAsDegrees)
{
   char * direction;

   // Number of options avaliable to us
   int dir_opt_count = 16;

   // Wind Direction lookup array text
   char * dir_opt_text[16] = {
      "N", "NNE", "NE", "ENE", "E", "ESE", "SE", "SSE",
      "S", "SSW", "SW", "WSW", "W", "WNW", "NW", "NNW"
   };

   // Wind Direction lookup array as degrees
   char * dir_opt_degrees[16] = { "0",   "22.5",  "45",  "67.5",
                                  "90",  "112.5", "135", "157.5",
                                  "180", "202.5", "225", "247.5",
                                  "270", "292.5", "315", "337.5" };

   // make sure that the value is within range of the array of options
   if (windByte < dir_opt_count)
   {
      if (returnAsDegrees == WIND_AS_DEGREES)
      {
         // this is probably going to be safe because we know what the max
         // length could be. strcpy(*windDirBuf, dir_opt_text[windByte]);
         asprintf (&direction, "%s", dir_opt_text[windByte]);
      }
      else
      {
         // Cheats way of doing this
         asprintf (&direction, "%s", dir_opt_degrees[windByte]);
      }
   }

   return direction;
}

float get_temperature (uint16_t temp_as_bytes, bool unit_type)
{
   float temperature;

   // Multiply by 0.1 deg to get temperature
   temperature = temp_as_bytes * 0.1;

   if (unit_type == UNIT_TYPE_IS_IMPERIAL)
   {
      // Multiply by 0.1 deg to get temperature
      temperature = temperature * 9 / 5 + 32;
   }

   return temperature;
}

/* Convert the two temperature bytes into one 16 bit byte */
static uint16_t convert_temperature (uint8_t high_byte, uint8_t low_byte)
{
   int t_byte;

   t_byte = (high_byte << 8) | (low_byte & 0xff);

   // Check the MSB see, if it negative
   if (t_byte & 0x8000)
   {
      // weather station uses top bit for sign and not normal
      t_byte = t_byte & 0x7FFF;

      // Number was negative take it away from 1 to get it into negatives.
      t_byte = 0 - t_byte;
   }
   else
   {
      // signed short, so we need to correct this with xor
      t_byte = t_byte ^ 0x0000;
   }

   return t_byte;
}

float get_wind_speed (uint8_t byte, bool unit_type)
{
   float windspeed;

   // Convert to km/h
   // cast the buffer as a float to get the 0.1 accuracy
   // Then convert into km/h by multiplying by 3.6

   windspeed = ((float)byte / 10) * 3.6;

   if (unit_type == UNIT_TYPE_IS_IMPERIAL)
   {
      windspeed = windspeed * 0.62;
   }

   return windspeed;
}

float get_pressure (uint16_t byte)
{
   return (float)(byte * 0.1);
}

static uint8_t check_humidity (uint8_t humidity)
{
   if (humidity > 0x64)
   {
      logger (LOG_ERROR, __func__, "humidity greater than 100%");
      humidity = 0x64;
   }

   return humidity;
}

float get_dew_point (uint16_t tbyte, uint8_t humidity, bool unit_type)
{
   // --- Dew Point Calculation --- //
   float gama;
   float dewPoint;
   float temperature;

   temperature = get_temperature (tbyte, UNIT_TYPE_IS_METRIC);

   // in case of 0% humidity
   if (humidity == 0)
   {
      // gama = aT/(b+T) + ln (RH/100)
      // Set humidity to 0.001 (very low!)
      gama = (17.271 * temperature) / (237.7 + temperature) + log (0.001);
   }
   else
   {
      // gama = aT/(b+T) + ln (RH/100)
      gama = (17.271 * temperature) / (237.7 + temperature) +
             log ((float)humidity / 100);
   }

   // dewPoint= (b * gama) / (a - gama)
   dewPoint = (237.7 * gama) / (17.271 - gama);

   if (unit_type == UNIT_TYPE_IS_IMPERIAL)
   {
      // Convert dew point to an integer and use existing temp function
      return get_temperature ((uint16_t) (dewPoint * 10), UNIT_TYPE_IS_IMPERIAL);
   }

   return dewPoint;
}

float get_wind_chill (weather_t * weather, bool unit_type)
{
   float windChill;
   float temperature;
   float windSpeed;

   logger (
      LOG_DEBUG,
      __func__,
      "Units will be in %s",
      unit_type == UNIT_TYPE_IS_METRIC ? "Metric" : "Imperial");

   temperature = get_temperature (weather->out_temp, UNIT_TYPE_IS_METRIC);

   windSpeed = get_wind_speed (weather->wind_speed, UNIT_TYPE_IS_METRIC);

   // check for which formula to use
   if (temperature < 11 && windSpeed > 4)
   {
      logger (LOG_DEBUG, __func__, "Using Wind Chill Temperature forumla", NULL);
      // this formula only works for temps less than 10 deg, and a min windspeed
      // of 5km/h
      windChill = 13.12 + 0.6215 * temperature - 11.37 * pow (windSpeed, 0.16) +
                  0.3965 * temperature * pow (windSpeed, 0.16);
   }
   else
   {
      // Use Apparent temperature
      logger (LOG_DEBUG, __func__, "Using Apparent Temperature forumla", NULL);

      // Version including the effects of temperature, humidity, and wind:
      // AT = Ta + 0.33×e − 0.70×ws − 4.00
      // e = rh / 100 × 6.105 × exp ( 17.27 × Ta / ( 237.7 + Ta ) )

      float e;

      e = (weather->out_humidity / 100.0) * 6.105 *
          exp (17.27 * temperature / (237.7 + temperature));

      logger (LOG_DEBUG, __func__, "e value is %f", e);

      windChill =
         round (temperature + 0.33 * e - 0.7 * (windSpeed / 3.6) - 4.00);

      logger (LOG_DEBUG, __func__, "Apparent Temperature is %f", windChill);
   }

   return windChill;
}

static uint16_t convert_to_16bit (
   uint8_t * buffer,
   uint8_t index1,
   uint8_t index2)
{
   return buffer[index1] << 8 | buffer[index2] & 0xff;
}

static uint16_t process_rainfall_diff (uint16_t rain_a, uint8_t * rain_b)
{
   uint16_t temp;

   temp = convert_to_16bit (rain_b, RAIN_TICK_HIGH_BYTE, RAIN_TICK_LOW_BYTE);

   return rain_a - temp;
}

float get_rainfall (uint16_t byte, bool unit_type)
{
   float total;

   // Bucket size is 0.3mm per tip
   total = byte * 0.2965;

   // check if returning in imperial units
   if (unit_type == UNIT_TYPE_IS_IMPERIAL)
   {
      return total * 0.011;
   }

   return total;
}

void process_data (
   weather_t * weather,
   int8_t * bufferCurrent,
   uint8_t * buffer1Hr,
   uint8_t * buffer24Hr)
{
   // Get the last stored value time
   sprintf (weather->last_read, "%d", bufferCurrent[LAST_READ_BYTE]);

   logger (LOG_DEBUG, __func__, "Processing Data", NULL);

   weather->out_temp = convert_temperature (
      bufferCurrent[OUTSIDE_TEMP_HIGH_BYTE],
      bufferCurrent[OUTSIDE_TEMP_LOW_BYTE]);

   weather->in_temp = convert_temperature (
      bufferCurrent[INSIDE_TEMP_HIGH_BYTE],
      bufferCurrent[INSIDE_TEMP_LOW_BYTE]);

   weather->wind_speed = bufferCurrent[WIND_SPEED_BYTE];

   weather->wind_gust = bufferCurrent[WIND_GUST_BYTE];

   weather->wind_dir = bufferCurrent[WIND_DIR_BYTE];

   weather->in_humidity = check_humidity (bufferCurrent[INSIDE_HUMIDITY_BYTE]);

   weather->out_humidity =
      check_humidity (bufferCurrent[OUTSIDE_HUMIDITY_BYTE]);

   weather->abs_pressure = convert_to_16bit (
      bufferCurrent,
      ABS_PRESSURE_HIGH_BYTE,
      ABS_PRESSURE_LOW_BYTE);

   weather->rel_pressure =
      convert_to_16bit (bufferCurrent, PRESSURE_HIGH_BYTE, PRESSURE_LOW_BYTE);

   // Rain is recorded in ticks since the batteries were inserted.
   weather->total_rain_fall =
      convert_to_16bit (bufferCurrent, RAIN_TICK_HIGH_BYTE, RAIN_TICK_LOW_BYTE);

   weather->last_hour_rain_fall =
      process_rainfall_diff (weather->total_rain_fall, buffer1Hr);

   weather->last_24_hr_rain_fall =
      process_rainfall_diff (weather->total_rain_fall, buffer24Hr);

   logger (LOG_DEBUG, __func__, "processed %d results", sizeof (weather));
}