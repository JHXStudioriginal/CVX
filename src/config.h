#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>

extern char current_dir[512];
extern char cvx_prompt[2048];
#define MAX_ALIASES 64
typedef struct { char *name; char *command; } Alias;
extern Alias aliases[MAX_ALIASES];
extern int alias_count;
extern bool history_enabled;
extern char start_dir[1024];

void config(void);

#endif

