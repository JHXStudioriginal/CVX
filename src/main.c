// Copyright (c) 2025 JHXStudioriginal
// This file is part of the Elasna Open Source License v2.
// All original author information and file headers must be preserved.
// For full license text, see: [https://github.com/JHXStudioriginal/Elasna-Ownership-License/blob/main/LICENSE]


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
#include "parser.h"
#include "linenoise.h"

static void load_profile(const char *path) {
    if (access(path, R_OK) == 0) {
        char cmd[1024];
        snprintf(cmd, sizeof(cmd), "sh %s", path);
        system(cmd);
    }
}

void process_command_line(char *line) {
    if (!line) return;

    char *comment = strchr(line, '#');
    if (comment) *comment = '\0';

    while (*line == ' ') line++;
    char *end = line + strlen(line) - 1;
    while (end > line && (*end == ' ' || *end == '\n')) *end-- = '\0';
    if (*line == '\0') return;

    char *cmds[32];
    char operators[32];
    int count = 0;

    char *p = line;
    while (*p && count < 32) {
        char *next = strstr(p, "&&");
        char *next_or = strstr(p, "||");
        char *split = NULL;
        char op = 0;

        if (next && (!next_or || next < next_or)) {
            split = next;
            op = 'A';
        } else if (next_or) {
            split = next_or;
            op = 'O';
        }

        if (split) {
            *split = '\0';
            while (*p == ' ') p++;
            char *end_cmd = split - 1;
            while (end_cmd > p && (*end_cmd == ' ' || *end_cmd == '\n')) *end_cmd-- = '\0';
            cmds[count] = p;
            operators[count] = op;
            count++;
            p = split + 2;
        } else {
            while (*p == ' ') p++;
            cmds[count] = p;
            operators[count] = 0;
            count++;
            break;
        }
    }

    int last_status = 0;

    for (int i = 0; i < count; i++) {
        if (i > 0) {
            if (operators[i-1] == 'A' && last_status != 0) continue;
            if (operators[i-1] == 'O' && last_status == 0) continue;
        }

        char *pipe_cmds[16];
        int pipe_count = 0;
        char *saveptr;
        char *q = strtok_r(cmds[i], "|", &saveptr);
        while (q && pipe_count < 16) {
            while (*q == ' ') q++;
            char *end3 = q + strlen(q) - 1;
            while (end3 > q && (*end3 == ' ' || *end3 == '\n')) *end3-- = '\0';
            pipe_cmds[pipe_count++] = q;
            q = strtok_r(NULL, "|", &saveptr);
        }

        if (pipe_count > 1) last_status = execute_pipeline(pipe_cmds, pipe_count);
        else if (pipe_count == 1) last_status = exec_command(pipe_cmds[0]);
    }
}

int main(int argc, char *argv[]) {
    if (argc > 1 &&
        (strcmp(argv[1], "--version") == 0 ||
         strcmp(argv[1], "-v") == 0 ||
         strcmp(argv[1], "-version") == 0)) {

        printf("CVX Shell beta 0.8.3\n");
        printf("Copyright (C) 2025 JHX Studio's\n");
        printf("License: Elasna Open Source License v2\n");
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
    setenv("TERM", "xterm", 1);

    config();

    setup_signals();

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
