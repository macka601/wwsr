#ifndef WUNDERGROUND_H
#define WUNDERGROUND_H

/* Wunderground functions */

typedef struct wunderground_config {
	char *wgUserName;
	char *wgPassword;
	char *wgId;
} wunderground_config_t;

int send_to_wunderground(wunderground_config_t *wg_config, weather_t *w);
void stripWhiteSpace(char *string);
size_t static write_callback_func(void *buffer, size_t size, size_t nmemb, void *userp);

int wunderground_init (FILE *config_file, wunderground_config_t *dbase_config);

#endif /* end of header guard */
