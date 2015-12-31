/* Wunderground weather functions */

#include <stdio.h>
#include <curl/curl.h>
#include <string.h>

#include "common.h"
#include "wunderground.h"
#include "database.h"
#include "logger.h"

// you must call processData before this
int createAndSendToWunderGround(struct weather* w, int printToScreen)
{	
/*	
URL_FORMAT "http://weatherstation.wunderground.com/weatherstation/updateweatherstation.php?
  ID=%s
  &PASSWORD=%s
  &dateutc=%s
  &windir=%s
  &windspeedmph=%0.1f
  &windgustmph=%0.1f
  &tempf=%0.1f
  &rainin=%0.1f
  &humidity=%d
  &dewptf=%0.1f
  &dailyrainin=%0.1f
  &baromin=%0.1f
  &action=updateraw"

*/
  char url[URL_SIZE];
  
  char *text;

  char date[BUFSIZ];
	
  getTime(date, sizeof(date));

  // convert all the values 
  float out_temp = w->out_temp * 9/5 + 32;

  float dew_point = w->dew_point * 9/5 + 32;

  float wind_speed = w->wind_speed * 0.6213;

  float wind_gust = w->wind_gust * 0.6213;
  
  float pressure = w->rel_pressure * 0.02953553;

  float last_hour_rain_fall = w->last_hour_rain_fall * 0.03937;

  float last_24_hr_rain_fall = w->last_24_hr_rain_fall * 0.03937;

  stripWhiteSpace(date);
  
  snprintf(url, URL_SIZE, URL_FORMAT, 
		 dbase.wgId, 
		 dbase.wgPassword, 
		 date, 
		 w->wind_in_degrees,
		 wind_speed, 
		 wind_gust,
		 out_temp, 
		 last_hour_rain_fall,
		 w->out_humidity,
		 dew_point,
		 last_24_hr_rain_fall,
		 pressure
  );

  logger(LOG_DEBUG, logType, "createAndSendToWunderGround", "URL sent:: %s", url);	    

  CURL *curl;
  CURLcode res;
      
  // setup curl
  curl = curl_easy_init();
     	
  if(curl) 
  { 
    /* to keep the response */
    char *response = NULL;
    
    //curl_easy_setopt(curl, CURLOPT_URL, "http://example.com");
    curl_easy_setopt(curl, CURLOPT_URL, url); 
    
    /* setting a callback function to return the data */
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback_func);

    /* passing the pointer to the response as the callback parameter */
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    /* Perform the request, res will get the return code */ 
    res = curl_easy_perform(curl);
  
    printf("Results sent to wunderground:: %s", response);
    	  
    /* Check for errors */ 
    if(response != "success")
    {
      logger(LOG_DEBUG, logType, "createAndSendToWunderGround", "Command Failed:: %s", curl_easy_strerror(res));	    
      logger(LOG_DEBUG, logType, "createAndSendToWunderGround", "URL sent:: %s", url);	    
    }
    else
    {
      logger(LOG_DEBUG, logType, "createAndSendToWunderGround", "Command state:: %s", response);
      
      if(printToScreen == 1)
      {
	  printf("Results sent to wunderground:: %s", response);
      }
    }
      
    /* always cleanup */ 
    curl_easy_cleanup(curl);
  }
}

void stripWhiteSpace(char *string)
{
  int i;
  
  char dest[BUFSIZ];
  
  for(i = 0; string[i] != '\0'; i++)
  {
      // printf("char is %c\n", string[i]);

      if(string[i] == ' ')
      {
          // printf("Replacing string");
          dest[i++] = '%';
          dest[i++] = '2';
          dest[i] = '0';
      }
      else
      {
          dest[i] = string[i];
      }
  }
  
  strcpy(string, dest);
}

/* the function to invoke as the data recieved */
size_t static write_callback_func(void *buffer, size_t size, size_t nmemb, void *userp)
{
    char **response_ptr =  (char**)userp;

    /* assuming the response is a string */
    *response_ptr = strndup(buffer, (size_t)(size *nmemb));

}
