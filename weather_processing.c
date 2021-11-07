/*
 * Weather processing file
 *
 */
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
const char *get_wind_direction (uint8_t windByte, bool returnAsDegrees)
{
  char *direction;

  // Number of options avaliable to us
  int dir_opt_count = 16;

  // Wind Direction lookup array text
  char *dir_opt_text[16] =
  {
    "N", "NNE", "NE", "ENE", "E", "ESE", "SE", "SSE",
    "S", "SSW", "SW", "WSW", "W", "WNW", "NW", "NNW"
  };

  // Wind Direction lookup array as degrees
  char *dir_opt_degrees[16] =
  {
    "0", "22.5", "45", "67.5", "90", "112.5", "135", "157.5",
    "180", "202.5", "225", "247.5", "270", "292.5", "315", "337.5"
  };

  // make sure that the value is within range of the array of options
  if ( windByte < dir_opt_count )
  {
    if (returnAsDegrees == WIND_AS_DEGREES)
    {
      // this is probably going to be safe because we know what the max length could be.
      // strcpy(*windDirBuf, dir_opt_text[windByte]);
      asprintf(&direction, "%s", dir_opt_text[windByte]);
    }
    else
    {
      // Cheats way of doing this
      asprintf(&direction, "%s", dir_opt_degrees[windByte]);
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

  windspeed = ((float) byte / 10) * 3.6;

  if (unit_type == UNIT_TYPE_IS_IMPERIAL)
  {
    windspeed = windspeed * 0.62;
  }

  return windspeed;
}

float getAbsPressure(uint8_t *byte)
{
  float AbsPressure;

  AbsPressure = (byte[ABS_PRESSURE_LOW_BYTE] + (byte[ABS_PRESSURE_HIGH_BYTE] << 8)) * 0.1;

  return AbsPressure;
}

float getRelPressure(uint8_t *byte)
{
  float relPressure;

  relPressure = (byte[PRESSURE_LOW_BYTE] + (byte[PRESSURE_HIGH_BYTE] << 8)) * 0.1;

  return relPressure;
}

unsigned char getHumidity(unsigned char humidity)
{
  if (humidity > 0x64) {
    printf("humidity greater than 100%\n");
    humidity = 0x64;
  }
  return humidity;
}

float getDewPoint(float temperature, float humidity)
{
  // --- Dew Point Calculation --- //
  float gama;

  float dewPoint;

  //in case of 0% humidity
  if (humidity == 0)
  {
    // gama = aT/(b+T) + ln (RH/100)
    // Set humidity to 0.001 (very low!)
    gama = (17.271 * temperature) / (237.7 + temperature) + log (0.001);
  }
  else
  {
    // gama = aT/(b+T) + ln (RH/100)
    gama = (17.271 * temperature) / (237.7 + temperature) + log ((float)humidity / 100);
  }

  // dewPoint= (b * gama) / (a - gama)
  dewPoint = (237.7 * gama) / (17.271 - gama);

  return dewPoint;
}

float getWindChill (weather_t *weather, int asImperial)
{
  // --- Wind Chill Calculation --- //
  float windChill;

  float temperature;

  float windSpeed;

  int humidity;

  logger( LOG_DEBUG, logType, "processData", "Units will be in %s", asImperial == UNIT_TYPE_IS_METRIC ? "Metric" : "Imperial");

  temperature = get_temperature(weather->out_temp, UNIT_TYPE_IS_METRIC);

  /* TODO: convert the weather struct to use bytes, not floats */
  windSpeed = get_wind_speed(weather->wind_speed, UNIT_TYPE_IS_METRIC);

  humidity = weather->out_humidity; //getHumidity(byte[OUTSIDE_HUMIDITY_BYTE]);

  // check for which formula to use
  if (temperature < 11 && windSpeed > 4)
  {
    if (log_sort.all) logger( LOG_DEBUG, logType, "processData", "Using Wind Chill Temperature forumla", NULL);
    // this formula only works for temps less than 10 deg, and a min windspeed of 5km/h
    windChill = 13.12 + 0.6215 * temperature - 11.37 * pow(windSpeed, 0.16)
                + 0.3965 * temperature * pow(windSpeed, 0.16);
  }
  else
  {
    // Use Apparent temperature
    logger( LOG_DEBUG, logType, "processData", "Using Apparent Temperature forumla", NULL);

    // Version including the effects of temperature, humidity, and wind:
    // AT = Ta + 0.33×e − 0.70×ws − 4.00
    // e = rh / 100 × 6.105 × exp ( 17.27 × Ta / ( 237.7 + Ta ) )

    float e;

    e = (humidity / 100.0) * 6.105 * exp(17.27 * temperature / (237.7 + temperature));

    if (log_sort.all) logger(LOG_DEBUG, logType, "processData", "e value is %f", e);

    windChill = round(temperature + 0.33 * e - 0.7 * (windSpeed / 3.6) - 4.00);

    if (log_sort.all) logger(LOG_DEBUG, logType, "processData", "Apparent Temperature is %f", windChill);

  }

  return windChill;
}

float getLastHoursRainFall(uint8_t *byteCurrent, uint8_t *byteHourly, int asImperial)
{
  float last_hour_rain_fall;

  int Rain1Hr;

  int RainTotal;

  Rain1Hr = (byteHourly[RAIN_TICK_LOW_BYTE] + (byteHourly[RAIN_TICK_HIGH_BYTE] << 8));

  // Get the total rain record (which is just the current one)
  RainTotal = (byteCurrent[RAIN_TICK_LOW_BYTE] + (byteCurrent[RAIN_TICK_HIGH_BYTE] << 8));

  // So now we want to get the previous hours rainfall
  // RainCurrent(Total) - the record from an hr ago.
  last_hour_rain_fall = (RainTotal - Rain1Hr) * 0.3;

  if (asImperial == 1)
  {
    last_hour_rain_fall = last_hour_rain_fall * 0.011;
  }

  return last_hour_rain_fall;

}

float getLast24HoursRainFall(uint8_t *byteCurrent, uint8_t *byte24Hour, int asImperial)
{
  int RainTotal;

  int Rain24Hr;

  float last24HoursRainFall;

  // Get the total rain record (which is just the current one)
  RainTotal = (byteCurrent[RAIN_TICK_LOW_BYTE] + (byteCurrent[RAIN_TICK_HIGH_BYTE] << 8));

  // Get the 24Hr rain record
  Rain24Hr = (byte24Hour[RAIN_TICK_LOW_BYTE] + (byte24Hour[RAIN_TICK_HIGH_BYTE] << 8));

  // And then we want the last 24hr rain fall buffer
  // RainCurrent(Total) - the record from an 24hrs ago.
  last24HoursRainFall = (RainTotal - Rain24Hr) * 0.3;

  // check if returning in imperial units
  if (asImperial == 1)
  {
    last24HoursRainFall = last24HoursRainFall * 0.011;
  }

  return last24HoursRainFall;
}

float getTotalRainFall(uint8_t *byte, int asImperial)
{
  int RainTotal;

  float total;

  RainTotal = (byte[RAIN_TICK_LOW_BYTE] + (byte[RAIN_TICK_HIGH_BYTE] << 8));

  // Rain Total is the current buffer - the first record.
  total = RainTotal * 0.3;

  // check if returning in imperial units
  if (asImperial == 1)
  {
    total = total * 0.011;
  }

  return total;
}

int processData (weather_t *weather, int8_t *bufferCurrent, uint8_t *buffer1Hr, uint8_t *buffer24Hr)
{
  log_event log_level;

  log_level = config_get_log_level ();

  // Get the last stored value time
  sprintf (weather->last_read, "%d", bufferCurrent[LAST_READ_BYTE]);

  logger (LOG_DEBUG, log_level, __func__, "Processing Data", NULL);

  weather->out_temp = convert_temperature (bufferCurrent[OUTSIDE_TEMP_HIGH_BYTE], bufferCurrent[OUTSIDE_TEMP_LOW_BYTE]);

  weather->in_temp = convert_temperature (bufferCurrent[INSIDE_TEMP_HIGH_BYTE], bufferCurrent[INSIDE_TEMP_LOW_BYTE]);

  weather->wind_speed = bufferCurrent[WIND_SPEED_BYTE];

  weather->wind_gust = bufferCurrent[WIND_GUST_BYTE];

  weather->wind_dir = bufferCurrent[WIND_DIR_BYTE];

  // ----- //

  // --- Humidity Calculations --- //
  // Get the humidity values out of the buffer

  weather->in_humidity = getHumidity(bufferCurrent[INSIDE_HUMIDITY_BYTE]);

  weather->out_humidity = getHumidity(bufferCurrent[OUTSIDE_HUMIDITY_BYTE]);

  // --- Dew Point Calculation --- //
  weather->dew_point = getDewPoint(weather->out_temp, weather->out_humidity);

  // --- Air Pressure Calculation --- //
  //weather.pressure = (bufferCurrent[PRESSURE_LOW_BYTE] + (bufferCurrent[PRESSURE_HIGH_BYTE] << 8)) / 10;
  weather->abs_pressure = getAbsPressure(bufferCurrent);

  // Relative pressure
  // abs. pressure
  weather->rel_pressure = getRelPressure(bufferCurrent);

  weather->wind_chill = getWindChill (weather, UNIT_TYPE_IS_METRIC);

  // --- Rain Calculation --- //
  // Rain is recorded in ticks since the batteries were inserted.
  weather->total_rain_fall = getTotalRainFall(bufferCurrent, UNIT_TYPE_IS_METRIC);

  // So now we want to get the previous hours rainfall
  weather->last_hour_rain_fall = getLastHoursRainFall(bufferCurrent, buffer1Hr, UNIT_TYPE_IS_METRIC);

  // So now we want to get the previous 24 hours rainfall
  weather->last_24_hr_rain_fall = getLast24HoursRainFall(bufferCurrent, buffer24Hr, UNIT_TYPE_IS_METRIC);

  logger (LOG_DEBUG, log_level, "ProcessData", "processed %d results", sizeof(weather));

  // success!
  return 1;
}