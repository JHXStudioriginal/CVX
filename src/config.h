// Copyright (c) [2025] [JHXStudioriginal]
// All rights reserved. This file is distributed under the Elasna Ownership License 1.0.
// Preserve author information and file headers. Do not claim ownership of the code.
// Do not sell this software or any derivative work without explicit permission.
// Full license: [https://github.com/JHXStudioriginal/CVX-Shell/blob/main/LICENSE]


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

