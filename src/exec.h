// Copyright (c) [2025] [JHXStudioriginal]
// All rights reserved. This file is distributed under the Elasna Ownership License 1.0.
// Preserve author information and file headers. Do not claim ownership of the code.
// Do not sell this software or any derivative work without explicit permission.
// Full license: [https://github.com/JHXStudioriginal/CVX-Shell/blob/main/LICENSE]


#ifndef EXEC_H
#define EXEC_H
#include "linenoise.h"

int split_args(const char *line, char *args[], int max_args);
void free_args(char *args[], int argc);
void handle_redirection(char *args[], int *argc);
void replace_alias(char *args[], int *argc);
int exec_command(char *cmdline);
int execute_pipeline(char **cmds, int n);

#endif
