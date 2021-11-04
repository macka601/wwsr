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

void windDirection(char *windDirBuf, int windByte, int returnAsDegrees)
{ 
      // Number of options avaliable to us
      int dir_opt_count = 16;
   
      // Wind Direction lookup array text
      char *dir_opt_text[16]=
      {
	  "N","NNE","NE","ENE","E","ESE","SE","SSE",
	  "S","SSW","SW","WSW","W","WNW","NW","NNW"
      };
      
      // Wind Direction lookup array as degrees
      char *dir_opt_degrees[16]=
      {
	  "0","22.5","45","67.5","90","112.5","135","157.5",
	  "180","202.5","225","247.5","270","292.5","315","337.5"
      };
      
      // make sure that the value is within range of the array of options
      if( windByte < dir_opt_count )
      {
	if(returnAsDegrees == 0)
	{
	  // this is probably going to be safe because we know what the max length could be.
	  // strcpy(*windDirBuf, dir_opt_text[windByte]);    
	  strcpy(windDirBuf, dir_opt_text[windByte]);
	}
	else
	{
	  // Cheats way of doing this
	  strcpy(windDirBuf, dir_opt_degrees[windByte]);         
	}
      }	     
}

float getTemperature(uint8_t *byte, int insideTemp, int asImperial)
{
  
  // combine the two bytes 
  int t_byte;
  if(insideTemp == 1)
  {
    //t_byte = byte[INSIDE_TEMP_LOW_BYTE] + byte[INSIDE_TEMP_HIGH_BYTE];
	t_byte = (byte[INSIDE_TEMP_HIGH_BYTE] << 8) | (byte[INSIDE_TEMP_LOW_BYTE] & 0xff);
  }
  else
  {
//    t_byte = byte[OUTSIDE_TEMP_LOW_BYTE] + byte[OUTSIDE_TEMP_HIGH_BYTE];
      t_byte = (byte[OUTSIDE_TEMP_HIGH_BYTE] << 8) | (byte[OUTSIDE_TEMP_LOW_BYTE] & 0xff);

  }
  float temperature;

  // Check to see if the most significant bit of the high byte is set
  // Indicates that it's a negative number
  if (t_byte & 0x8000) 
  {
    //weather station uses top bit for sign and not normal
    t_byte = t_byte & 0x7FFF;  

    // Number was negative take it away from 1 to get it into negatives.
    t_byte = 0 - t_byte;
  }
  else    
  {
    //signed short, so we need to correct this with xor			
    t_byte = t_byte ^ 0x0000;   
  } 
	
  // Multiply by 0.1 deg to get temperature	
  temperature = t_byte * 0.1;

  // Check to see if the result is normal
  if((temperature > 50) || (temperature < -30))
  {
    if(log_sort.all) logger( LOG_DEBUG, logType, "weatherProcessing", "Error: outside temp is not within bounds", NULL);
    // exit!
    exit(0);
  }
	
  // Check if the imperial flag is set
  if(asImperial == 1)
  {
    // return temperature as Fahrenhiet
    temperature = temperature * 9/5 + 32;
  }
	
  return temperature;
}

float getWindSpeed(uint8_t *byte)
{
  float windspeed;
  
  // Convert to km/h
  // cast the buffer as a float to get the 0.1 accuracy
  // Then convert into km/h by multiplying by 3.6

  windspeed = ((float) byte[WIND_SPEED_BYTE] / 10) * 3.6;

  return windspeed;
}

float getWindGust(int windGustByte)
{
  float windgust;
  
  // cast the buffer as a float to get the 0.1 accuracy
  // Then convert into km/h by multiplying by 3.6
  windgust = ((float) windGustByte / 10) * 3.6;
  
  return windgust;
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
  if(humidity > 0x64) {
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

float getWindChill(uint8_t *byte, int asImperial)//float temperature, float windSpeed, int humidity)
{
  // --- Wind Chill Calculation --- //
  float windChill;
  
  float temperature;
  
  float windSpeed;
  
  int humidity;
  
  temperature = getTemperature(byte, OUTSIDE_TEMPERATURE, asImperial);
  
  windSpeed = getWindSpeed(byte);
  
  humidity = getHumidity(byte[OUTSIDE_HUMIDITY_BYTE]);
  
  // check for which formula to use 
  if(temperature < 11 && windSpeed > 4)
  {
    if(log_sort.all) logger( LOG_DEBUG, logType, "processData", "Using Wind Chill Temperature forumla", NULL);
    // this formula only works for temps less than 10 deg, and a min windspeed of 5km/h
    windChill = 13.12 + 0.6215 * temperature - 11.37 * pow(windSpeed, 0.16)  
	  + 0.3965 * temperature * pow(windSpeed, 0.16);	  
  }
  else
  {
    // Use Apparent temperature
    if(log_sort.all) logger( LOG_DEBUG, logType, "processData", "Using Apparent Temperature forumla", NULL);

    // Version including the effects of temperature, humidity, and wind:
    // AT = Ta + 0.33×e − 0.70×ws − 4.00
    // e = rh / 100 × 6.105 × exp ( 17.27 × Ta / ( 237.7 + Ta ) )

    float e;

    e = (humidity/100.0) * 6.105 * exp(17.27 * temperature / (237.7 + temperature));

    if(log_sort.all) logger(LOG_DEBUG, logType, "processData", "e value is %f", e);

    windChill = round(temperature + 0.33 * e - 0.7 * (windSpeed/3.6) - 4.00);

    if(log_sort.all) logger(LOG_DEBUG, logType, "processData", "Apparent Temperature is %f", windChill);

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

  if(asImperial == 1)
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
  if(asImperial == 1)
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
  if(asImperial == 1)
  {
    total = total * 0.011;
  }

  return total;
}
