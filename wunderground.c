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
  log_event log_level = config_get_log_level ();
  char url[URL_SIZE];

  char *text;

  char date[BUFSIZ];

  getTime(date, sizeof(date));

  // convert all the values
  float out_temp = w->out_temp * 9 / 5 + 32;

  float dew_point = w->dew_point * 9 / 5 + 32;

  float wind_speed = w->wind_speed * 0.6213;

  float wind_gust = w->wind_gust * 0.6213;

  float pressure = w->rel_pressure * 0.02953553;

  float last_hour_rain_fall = w->last_hour_rain_fall * 0.03937;

  float last_24_hr_rain_fall = w->last_24_hr_rain_fall * 0.03937;

  stripWhiteSpace(date);

  // snprintf(url, URL_SIZE, URL_FORMAT,
  //  dbase.wgId,
  //  dbase.wgPassword,
  //  date,
  //  w->wind_in_degrees,
  //  wind_speed,
  //  wind_gust,
  //  out_temp,
  //  last_hour_rain_fall,
  //  w->out_humidity,
  //  dew_point,
  //  last_24_hr_rain_fall,
  //  pressure
  // );

  logger(LOG_DEBUG, log_level, "createAndSendToWunderGround", "URL sent:: %s", url);

  CURL *curl;
  CURLcode res;

  // setup curl
  curl = curl_easy_init();

  if (curl)
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
    if (response != "success")
    {
      logger(LOG_DEBUG, log_level, "createAndSendToWunderGround", "Command Failed:: %s", curl_easy_strerror(res));
      logger(LOG_DEBUG, log_level, "createAndSendToWunderGround", "URL sent:: %s", url);
    }
    else
    {
      logger(LOG_DEBUG, log_level, "createAndSendToWunderGround", "Command state:: %s", response);

      if (printToScreen == 1)
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
  char *position;
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
