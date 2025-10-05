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
