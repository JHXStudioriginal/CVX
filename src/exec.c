// Copyright (c) 2025 JHXStudioriginal
// This file is part of the Elasna Open Source License v2.
// All original author information and file headers must be preserved.
// For full license text, see: [https://github.com/JHXStudioriginal/Elasna-Ownership-License/blob/main/LICENSE]

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdbool.h>
#include "parser.h"
#include "commands.h"
#include "config.h"
#include "signals.h"
#include "linenoise.h"

static const char *last_cmd = NULL;

int exec_command(char *cmdline) {
    if (!cmdline || !*cmdline) return 0;

    char *args[64];
    int argc = split_args(cmdline, args, 64);

    replace_alias(args, &argc);
    unescape_args(args, argc);

    for (int i = 0; i < argc; i++) {
        char *tmp = expand_variables(args[i]);
        free(args[i]);
        args[i] = tmp;
    }

    for (int i = 0; i < argc; i++) {
        if (strcmp(args[i], "!!") == 0) {
            if (last_cmd) { free(args[i]); args[i] = strdup(last_cmd); }
            else { printf("No command in history\n"); free_args(args, argc); return 1; }
        }
    }

    linenoiseHistoryAdd(cmdline);
    if (last_cmd) free((void*)last_cmd);
    last_cmd = strdup(cmdline);

    bool has_redirect = false;
    for (int i = 0; i < argc; i++) {
        if (strcmp(args[i], ">") == 0 || strcmp(args[i], ">>") == 0 ||
            strcmp(args[i], "<") == 0 || strcmp(args[i], "<<") == 0) {
            has_redirect = true;
            break;
        }
    }

    if (!has_redirect) {
        if (strcmp(args[0], "cd") == 0) return cmd_cd(argc, args);
        if (strcmp(args[0], "pwd") == 0) return cmd_pwd(argc, args);
        if (strcmp(args[0], "export") == 0) return cmd_export(argc, args);
        if (strcmp(args[0], "help") == 0) return cmd_help(argc, args);
        if (strcmp(args[0], "ls") == 0) return cmd_ls(argc, args);
        if (strcmp(args[0], "history") == 0) return cmd_history(argc, args);
        if (strcmp(args[0], "echo") == 0) return cmd_echo(argc, args);
    }

    pid_t pid = fork();
    if (pid < 0) { perror("fork error"); free_args(args, argc); return 1; }

    if (pid == 0) {
        for (int i = 0; i < argc; i++) {
            char *tmp = expand_tilde(args[i]);
            free(args[i]);
            args[i] = tmp;
        }

        handle_redirection(args, &argc);

        if (strcmp(args[0], "cd") == 0) exit(cmd_cd(argc, args));
        if (strcmp(args[0], "pwd") == 0) exit(cmd_pwd(argc, args));
        if (strcmp(args[0], "export") == 0) exit(cmd_export(argc, args));
        if (strcmp(args[0], "help") == 0) exit(cmd_help(argc, args));
        if (strcmp(args[0], "ls") == 0) exit(cmd_ls(argc, args));
        if (strcmp(args[0], "history") == 0) exit(cmd_history(argc, args));
        if (strcmp(args[0], "echo") == 0) exit(cmd_echo(argc, args));

        execvp(args[0], args);
        perror("exec error");
        free_args(args, argc);
        exit(EXIT_FAILURE);
    } else {
        child_pid = pid;

        int status;
        waitpid(pid, &status, WUNTRACED);
        child_pid = -1;
        free_args(args, argc);
        return WIFEXITED(status) ? WEXITSTATUS(status) : 1;
    }
}

int execute_pipeline(char **cmds, int n) {
    int in_fd = 0;
    int pipefd[2];
    pid_t pids[16];

    for (int i = 0; i < n; i++) {
        if (strcmp(cmds[i], "!!") == 0) {
            if (last_cmd) { free(cmds[i]); cmds[i] = strdup(last_cmd); }
            else { printf("No command in history\n"); return 1; }
        }
    }

    for (int i = 0; i < n; i++) {
        if (i != n - 1 && pipe(pipefd) < 0) { perror("pipe error"); return 1; }

        pid_t pid = fork();
        if (pid == 0) {
            char *args[64];
            int argc = split_args(cmds[i], args, 64);

            replace_alias(args, &argc);
            unescape_args(args, argc);

            for (int j = 0; j < argc; j++) {
                char *tmp = expand_variables(args[j]);
                free(args[j]);
                args[j] = tmp;
            }

            for (int j = 0; j < argc; j++) {
                char *tmp = expand_tilde(args[j]);
                free(args[j]);
                args[j] = tmp;
            }

            if (in_fd != 0) { dup2(in_fd, STDIN_FILENO); close(in_fd); }
            if (i != n - 1) { close(pipefd[0]); dup2(pipefd[1], STDOUT_FILENO); close(pipefd[1]); }

            handle_redirection(args, &argc);

            if (strcmp(args[0], "cd") == 0) exit(cmd_cd(argc, args));
            if (strcmp(args[0], "pwd") == 0) exit(cmd_pwd(argc, args));
            if (strcmp(args[0], "export") == 0) exit(cmd_export(argc, args));
            if (strcmp(args[0], "help") == 0) exit(cmd_help(argc, args));
            if (strcmp(args[0], "ls") == 0) exit(cmd_ls(argc, args));
            if (strcmp(args[0], "history") == 0) exit(cmd_history(argc, args));
            if (strcmp(args[0], "echo") == 0) exit(cmd_echo(argc, args));

            execvp(args[0], args);
            perror("exec error");
            free_args(args, argc);
            exit(EXIT_FAILURE);
        } else if (pid > 0) {
            if (i == n - 1)
                child_pid = pid;
        } else if (pid < 0) {
            perror("fork error");
            return 1;
        }
        
        if (in_fd != 0) close(in_fd);
        if (i != n - 1) { close(pipefd[1]); in_fd = pipefd[0]; }
        pids[i] = pid;        
    }

    for (int i = 0; i < n; i++) {
        int status;
        waitpid(pids[i], &status, WUNTRACED);
    }

    child_pid = -1;

    return 0;
}
