#ifndef CONFIG_H
#define CONFIG_H

#include "logger.h"
#include "database.h"
#include "wunderground.h"

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

char *rtrim(char *const s);
int config_get_options (int argc, char **argv, config_t *config);
int config_get_log_level (void);
int config_get_next_line (char *line, char *value);

#endif /* End of header guard */