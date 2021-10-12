#ifndef CONFIG_H
#define CONFIG_H

#include "common.h"

char *rtrim(char *const s);
int config_get_options (int argc, char **argv, config_t *config);
int config_get_log_level (void);
int config_get_next_line (char *line, char *value);

#endif /* End of header guard */