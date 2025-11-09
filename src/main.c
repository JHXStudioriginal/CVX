// Copyright (c) [2025] [JHXStudioriginal]
// All rights reserved. This file is distributed under the Elasna Ownership License 1.0.
// Preserve author information and file headers. Do not claim ownership of the code.
// Do not sell this software or any derivative work without explicit permission.
// Full license: [https://github.com/JHXStudioriginal/CVX-Shell/blob/main/LICENSE]


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include "config.h"
#include "prompt.h"
#include "exec.h"
#include "signals.h"
#include "linenoise.h"

static void load_profile(const char *path) {
    if (access(path, R_OK) == 0) {
        char cmd[1024];
        snprintf(cmd, sizeof(cmd), "sh %s", path);
        system(cmd);
    }
}

static void process_command_line(char *line) {
    char *seq_cmds[16];
    int seq_count = 0;
    char *saveptr1;

    char *part = strtok_r(line, "&&", &saveptr1);
    while (part && seq_count < 16) {
        while (*part == ' ') part++;
        char *end = part + strlen(part) - 1;
        while (end > part && *end == ' ') *end-- = '\0';
        seq_cmds[seq_count++] = part;
        part = strtok_r(NULL, "&&", &saveptr1);
    }

    for (int i = 0; i < seq_count; i++) {
        char *pipe_cmds[16];
        int pipe_count = 0;
        char *saveptr2;

        char *p = strtok_r(seq_cmds[i], "|", &saveptr2);
        while (p && pipe_count < 16) {
            while (*p == ' ') p++;
            char *end = p + strlen(p) - 1;
            while (end > p && *end == ' ') *end-- = '\0';
            pipe_cmds[pipe_count++] = p;
            p = strtok_r(NULL, "|", &saveptr2);
        }

        if (pipe_count > 1) {
            execute_pipeline(pipe_cmds, pipe_count);
        } else if (pipe_count == 1) {
            exec_command(pipe_cmds[0]);
        }        
    }
}

int main(int argc, char *argv[]) {
    if (argc > 1 &&
        (strcmp(argv[1], "--version") == 0 ||
         strcmp(argv[1], "-v") == 0 ||
         strcmp(argv[1], "-version") == 0)) {

        printf("CVX Shell beta-8\n");
        printf("Copyright (C) 2025 JHX Studio's\n");
        printf("License: GNU GPL v3.0\n");
        return 0;
    }

    if (argc > 1 &&
        (strcmp(argv[1], "--help") == 0 ||
         strcmp(argv[1], "-help") == 0 ||
         strcmp(argv[1], "-h") == 0)) {
    
        process_command_line("help");
        return 0;
    }
    

    if (argc > 2 && strcmp(argv[1], "-c") == 0) {
        char cmd[4096] = {0};
        for (int i = 2; i < argc; i++) {
            strncat(cmd, argv[i], sizeof(cmd) - strlen(cmd) - 1);
            if (i < argc - 1)
                strncat(cmd, " ", sizeof(cmd) - strlen(cmd) - 1);
        }
    
        process_command_line(cmd);
        return 0;
    }    

    if (argc > 1 && strcmp(argv[1], "-l") == 0) {
        load_profile("/etc/profile");
        const char *home = getenv("HOME");
        if (home) {
            char user_profile[1024];
            snprintf(user_profile, sizeof(user_profile), "%s/.profile", home);
            load_profile(user_profile);
        }
    }

    if (argc > 1 && argv[1][0] != '-') {
        FILE *f = fopen(argv[1], "r");
        if (!f) {
            perror("Cannot open file.");
            return 1;
        }
    
        char line[4096];
        while (fgets(line, sizeof(line), f)) {
            line[strcspn(line, "\n")] = 0;
            if (line[0] != '\0')
                process_command_line(line);
        }
        fclose(f);
        return 0;
    }

    char *line;
    signal(SIGINT, sigint_handler);
    setenv("TERM", "xterm", 1);

    config();

    const char *history_file = ".cvx_history";
    char history_path[1024];
    const char *home = getenv("HOME");
    if (!home) home = ".";
    snprintf(history_path, sizeof(history_path), "%s/%s", home, history_file);
    linenoiseHistoryLoad(history_path);
    getcwd(current_dir, sizeof(current_dir));

    while (1) {
        const char *prompt = get_prompt();
        line = linenoise(prompt);
        if (line == NULL) break;

        line[strcspn(line, "\n")] = 0;

        if (strcmp(line, "exit") == 0) {
            free(line);
            break;
        }

        if (line[0] != '\0' && history_enabled) {
            linenoiseHistoryAdd(line);
            linenoiseHistorySave(history_path);
        }

        process_command_line(line);

        free(line);
    }

    return 0;
}
