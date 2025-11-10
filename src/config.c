// Copyright (c) 2025 JHXStudioriginal
// This file is part of the Elasna Open Source License v2.
// All original author information and file headers must be preserved.
// For full license text, see: [https://github.com/JHXStudioriginal/Elasna-Ownership-License/blob/main/LICENSE]


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>
#include <stdbool.h>
#include <ctype.h>
#include "config.h"

char current_dir[512] = "";
char cvx_prompt[2048] = "";
Alias aliases[MAX_ALIASES];
int alias_count = 0;
bool history_enabled = true;
char start_dir[1024] = "";

void config() {
    strncpy(cvx_prompt, "DEFAULT_PROMPT", sizeof(cvx_prompt)-1);
    cvx_prompt[sizeof(cvx_prompt)-1] = '\0';

    if (getcwd(current_dir, sizeof(current_dir)) == NULL) {
        perror("getcwd error");
        current_dir[0] = '\0';
    }

    strncpy(start_dir, current_dir, sizeof(start_dir)-1);
    start_dir[sizeof(start_dir)-1] = '\0';

    history_enabled = true;
    alias_count = 0;

    FILE *f = fopen("/etc/cvx.conf", "r");
    if (!f) return;

    char line[1024];
    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\n")] = 0;
        char *ptr = line;
        while (*ptr && isspace(*ptr)) ptr++;
        if (*ptr == '#' || *ptr == '\0') continue;

        if (strncmp(ptr, "PROMPT=", 7) == 0) {
            char *value = ptr + 7;
            if (strncmp(value, "default", 7) == 0 && (value[7] == '\0' || isspace(value[7]))) {
                strncpy(cvx_prompt, "DEFAULT_PROMPT", sizeof(cvx_prompt)-1);
                cvx_prompt[sizeof(cvx_prompt)-1] = '\0';
            } else if (*value == '"') {
                value++;
                char *end = strchr(value, '"');
                if (end) *end = '\0';
                strncpy(cvx_prompt, value, sizeof(cvx_prompt)-1);
                cvx_prompt[sizeof(cvx_prompt)-1] = '\0';
            }
        } 
        else if (strncmp(ptr, "ALIAS=", 6) == 0) {
            char *value = ptr + 6;
            if (*value != '"') continue;
            value++;
            char *end = strchr(value, '"');
            if (!end) continue;
            *end = '\0';

            char *sep = strchr(value, '-');
            if (!sep) continue;
            *sep = '\0';
            char *name = value;
            char *command = sep + 1;

            if (alias_count < MAX_ALIASES) {
                aliases[alias_count].name = strdup(name);
                aliases[alias_count].command = strdup(command);
                alias_count++;
            }
        } 
        else if (strncmp(ptr, "STARTDIR=", 9) == 0) {
            char *value = ptr + 9;
            if (*value == '"') value++;
            char *end = strchr(value, '"');
            if (end) *end = '\0';
            strncpy(start_dir, value, sizeof(start_dir)-1);
            start_dir[sizeof(start_dir)-1] = '\0';
            chdir(start_dir);
            if (getcwd(current_dir, sizeof(current_dir)) == NULL) {
                perror("getcwd error");
                current_dir[0] = '\0';
            }
        } 
        else if (strncmp(ptr, "HISTORY=", 8) == 0) {
            char *value = ptr + 8;
            if (strcmp(value, "true") == 0) history_enabled = true;
            else if (strcmp(value, "false") == 0) history_enabled = false;
        }
    }

    fclose(f);
}
