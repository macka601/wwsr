/* Wunderground weather functions */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <curl/curl.h>
#include <string.h>

#include "common.h"
#include "wunderground.h"
#include "database.h"
#include "logger.h"
#include "config.h"
#include "weather_processing.h"

#define URL_FORMAT "http://weatherstation.wunderground.com/weatherstation/updateweatherstation.php?\
  ID=%s\
  &PASSWORD=%s\
  &dateutc=%s\
  &winddir=%s\
  &windspeedmph=%0.1f\
  &windgustmph=%0.1f\
  &tempf=%0.1f\
  &rainin=%0.1f\
  &humidity=%d\
  &dewptf=%0.1f\
  &dailyrainin=%0.1f\
  &baromin=%0.1f\
  &action=updateraw&softwaretype=vws"

int send_to_wunderground(wunderground_config_t *wg_config, struct weather* w)
{
  CURL *curl;
  CURLcode res;

  log_event log_level;
  log_level = config_get_log_level ();

  float out_temp;
  float dew_point;
  float wind_speed;
  float wind_gust;
  float pressure;
  float last_hour_rain_fall;
  float last_24_hr_rain_fall;

  char *url;
  char date[BUFSIZ];

  // convert all the values
  out_temp = get_temperature (w->out_temp, UNIT_TYPE_IS_IMPERIAL);

  dew_point = w->dew_point * 9 / 5 + 32;

  wind_speed = get_wind_speed (w->wind_speed, UNIT_TYPE_IS_IMPERIAL);

  wind_gust = get_wind_speed (w->wind_gust, UNIT_TYPE_IS_IMPERIAL);

  pressure = w->rel_pressure * 0.02953553;

  last_hour_rain_fall = w->last_hour_rain_fall * 0.03937;

  last_24_hr_rain_fall = w->last_24_hr_rain_fall * 0.03937;

  getTime (date, sizeof(date));

  logger (LOG_DEBUG, log_level, __func__, "values going to WunderGround", NULL);

  stripWhiteSpace (date);

  asprintf (&url, URL_FORMAT,
   wg_config->wgId,
   wg_config->wgPassword,
   date,
   get_wind_direction(w->wind_dir, WIND_AS_DEGREES),
   wind_speed,
   wind_gust,
   out_temp,
   last_hour_rain_fall,
   w->out_humidity,
   dew_point,
   last_24_hr_rain_fall,
   pressure
  );

  logger (LOG_DEBUG, log_level, __func__, "URL sent:: %s", url);

  // setup curl
  curl = curl_easy_init();

  if (curl)
  {
    /* to keep the response */
    char *response = NULL;

    //curl_easy_setopt(curl, CURLOPT_URL, "http://example.com");
    curl_easy_setopt (curl, CURLOPT_URL, url);

    /* setting a callback function to return the data */
    curl_easy_setopt (curl, CURLOPT_WRITEFUNCTION, write_callback_func);

    /* passing the pointer to the response as the callback parameter */
    curl_easy_setopt (curl, CURLOPT_WRITEDATA, &response);

    /* Perform the request, res will get the return code */
    res = curl_easy_perform (curl);

    printf ("Results sent to wunderground:: %s", response);

    /* Check for errors */
    if (response != "success")
    {
      logger (LOG_DEBUG, log_level, __func__, "Command Failed:: %s", curl_easy_strerror (res));
      logger (LOG_DEBUG, log_level, __func__, "URL sent:: %s", url);
    }
    else
    {
      logger (LOG_DEBUG, log_level, __func__, "Command state:: %s", response);
    }

    /* always cleanup */
    curl_easy_cleanup (curl);
  }

  free (url);
}

void stripWhiteSpace(char *string)
{
  int i;

  char dest[BUFSIZ];

  for (i = 0; string[i] != '\0'; i++)
  {
    // printf("char is %c\n", string[i]);

    if (string[i] == ' ')
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
  *response_ptr = strndup(buffer, (size_t)(size * nmemb));
}


static int check_config_value (char *name, char *value)
{
  char *secret = "******";
  log_event log_level = config_get_log_level ();
  if (value == NULL)
  {
    logger (LOG_ERROR, log_level, __func__, "Config file is missing option %s", name);
    return -1;
  }

  logger (LOG_INFO, log_level, __func__, "Config file key %s = %s", name,
          strcmp ("wgPassword", name) == 0 ? secret : value);

  return 0;
}

static int validate_config (wunderground_config_t *wg_config)
{
  logger (LOG_INFO, config_get_log_level (), __func__, "Validating configuration options from config file", NULL);

  RETURN_IF_ERROR (check_config_value ("wgUserName", wg_config->wgUserName));

  RETURN_IF_ERROR (check_config_value ("wgPassword", wg_config->wgPassword));

  RETURN_IF_ERROR (check_config_value ("wgId", wg_config->wgId));

  return 0;
}

static void wunderground_copy_config_value (char *src, char **dest, char *name)
{
  log_event log_level = config_get_log_level ();
  char *secret = "******";

  if (asprintf (dest, "%s", src))
  {
    logger(LOG_DEBUG, log_level, __func__, "Found %s=`%s`", name, strcmp("wgPassword", name) == 0 ? secret : src);
  }
}

int wunderground_init (FILE *config_file, wunderground_config_t *wg_config)
{
  char _buffer[BUFSIZ];
  char line[BUFSIZ];
  int ret = 0;
  int i;
  log_event log_level = config_get_log_level ();

  logger(LOG_DEBUG, log_level, __func__, "Looking for wundeground config options");

  // Get a line from the configuration file
  while (fgets (line, sizeof(line), config_file) != NULL)
  {
    logger (LOG_DEBUG, log_level, __func__, "Checking for parameters line #", rtrim (line));

    // Detect if there is a comment present
    if (line[0] != '#')
    {
      i = config_get_next_line (line, &_buffer[0]);

      if (strstr (line, "wgUserName"))
      {
        i > 0 ? wunderground_copy_config_value (_buffer, &wg_config->wgUserName, "wgUserName") : NULL;
      }

      if (strstr (line, "wgPassword"))
      {
        i > 0 ? wunderground_copy_config_value (_buffer, &wg_config->wgPassword, "wgPassword") : NULL;
      }

      if (strstr (line, "wgId"))
      {
        i > 0 ? wunderground_copy_config_value (_buffer, &wg_config->wgId, "wgId") : NULL;
      }
    }
    // Clear our buffer for next time around
    memset (_buffer, 0, sizeof(_buffer));
  }

  ret = validate_config (wg_config);

  return ret;
}
