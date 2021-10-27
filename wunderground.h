#ifndef WUNDERGROUND_H
#define WUNDERGROUND_H

/* Wunderground functions */

typedef struct wunderground_config {
	char *wgUserName;
	char *wgPassword;
	char *wgId;
} wunderground_config_t;


#define URL_SIZE 512
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

int createAndSendToWunderGround(struct weather* w);
void stripWhiteSpace(char *string);
size_t static write_callback_func(void *buffer, size_t size, size_t nmemb, void *userp);

int wunderground_init (FILE *config_file, wunderground_config_t *dbase_config);

#endif /* end of header guard */
