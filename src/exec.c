// Copyright (c) 2025 JHXStudioriginal
// This file is part of the Elasna Open Source License v2.
// All original author information and file headers must be preserved.
// For full license text, see: [https://github.com/JHXStudioriginal/Elasna-Ownership-License/blob/main/LICENSE]


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdbool.h>
#include "exec.h"
#include "commands.h"
#include "config.h"
#include "signals.h"

static const char *last_cmd = NULL;

char* expand_tilde(const char *path) {
    if (!path || path[0] != '~')
        return strdup(path);

    const char *home = getenv("HOME");
    if (!home) home = "/";

    char expanded[1024];
    snprintf(expanded, sizeof(expanded), "%s%s", home, path + 1);
    return strdup(expanded);
}

char* unescape_string(const char *s) {
    if (!s) return NULL;
    char *buf = malloc(strlen(s) + 1);
    if (!buf) return NULL;
    int j = 0;
    for (int i = 0; s[i]; i++) {
        if (s[i] == '\\' && s[i+1]) {
            i++;
            switch (s[i]) {
                case 'n': buf[j++] = '\n'; break;
                case 't': buf[j++] = '\t'; break;
                case 'r': buf[j++] = '\r'; break;
                case 'a': buf[j++] = '\a'; break;
                case 'v': buf[j++] = '\v'; break;
                case '\\': buf[j++] = '\\'; break;
                case '"': buf[j++] = '"'; break;
                case '\'': buf[j++] = '\''; break;
                case '$': buf[j++] = '$'; break;
                case 'x': {
                    int val = 0;
                    for (int k = 0; k < 2 && isxdigit((unsigned char)s[i+1]); k++, i++)
                        val = val*16 + (isdigit(s[i+1]) ? s[i+1]-'0' : (tolower(s[i+1])-'a'+10));
                    buf[j++] = (char)val;
                    break;
                }
                default: buf[j++] = s[i]; break;
            }
        } else {
            buf[j++] = s[i];
        }
    }
    buf[j] = '\0';
    return buf;
}

int split_args(const char *line, char *args[], int max_args) {
    int argc = 0;
    const char *p = line;
    char buffer[1024];
    int buf_i = 0;
    bool in_quotes = false;
    char quote_char = '\0';

    while (*p) {
        if (*p == '#' && !in_quotes) break;
        if (argc >= max_args - 1) break;

        if ((*p == '<' && p[1] == '<') || (*p == '>' && p[1] == '>')) {
            if (buf_i > 0) {
                buffer[buf_i] = '\0';
                args[argc++] = strdup(buffer);
                buf_i = 0;
            }
            char tmp[3] = { *p, *(p+1), '\0' };
            args[argc++] = strdup(tmp);
            p += 2;
            continue;
        }

        if (*p == '<' || *p == '>') {
            if (buf_i > 0) {
                buffer[buf_i] = '\0';
                args[argc++] = strdup(buffer);
                buf_i = 0;
            }
            char tmp[2] = { *p, '\0' };
            args[argc++] = strdup(tmp);
            p++;
            continue;
        }

        if ((*p == '"' || *p == '\'') && !in_quotes) {
            in_quotes = true;
            quote_char = *p;
            p++;
            continue;
        }
        if (*p == quote_char && in_quotes) {
            in_quotes = false;
            quote_char = '\0';
            p++;
            continue;
        }

        if (*p == ' ' && !in_quotes) {
            if (buf_i > 0) {
                buffer[buf_i] = '\0';
                args[argc++] = strdup(buffer);
                buf_i = 0;
            }
            p++;
            continue;
        }

        buffer[buf_i++] = *p;
        p++;
    }

    if (buf_i > 0) {
        buffer[buf_i] = '\0';
        args[argc++] = strdup(buffer);
    }

    args[argc] = NULL;
    return argc;
}

void free_args(char *args[], int argc) {
    for (int i = 0; i < argc; i++) {
        free(args[i]);
        args[i] = NULL;
    }
}

void handle_redirection(char *args[], int *argc) {
    int in_fd = -1, out_fd = -1;
    char heredoc_file[] = "/tmp/cvx_heredoc_XXXXXX";

    for (int i = 0; i < *argc; i++) {
        if (strcmp(args[i], "<") == 0 && i + 1 < *argc) {

            in_fd = open(args[i + 1], O_RDONLY);
            if (in_fd < 0) perror("open input");

            free(args[i]);
            free(args[i + 1]);
            for (int j = i; j + 2 <= *argc; j++) args[j] = args[j + 2];
            *argc -= 2;
            i--;

        } else if (strcmp(args[i], ">>") == 0 && i + 1 < *argc) {

            out_fd = open(args[i + 1], O_WRONLY | O_CREAT | O_APPEND, 0644);
            if (out_fd < 0) perror("open append output");

            free(args[i]);
            free(args[i + 1]);
            for (int j = i; j + 2 <= *argc; j++) args[j] = args[j + 2];
            *argc -= 2;
            i--;

        } else if (strcmp(args[i], ">") == 0 && i + 1 < *argc) {

            out_fd = open(args[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (out_fd < 0) perror("open output");

            free(args[i]);
            free(args[i + 1]);
            for (int j = i; j + 2 <= *argc; j++) args[j] = args[j + 2];
            *argc -= 2;
            i--;

        } else if (strcmp(args[i], "<<") == 0 && i + 1 < *argc) {

            int tmp_fd = mkstemp(heredoc_file);
            if (tmp_fd < 0) {
                perror("heredoc temp");
                continue;
            }

            char *delimiter = args[i + 1];
            printf("> enter lines, end with '%s'\n", delimiter);
            fflush(stdout);

            while (1) {
                char buf[1024];
                printf("> ");
                fflush(stdout);

                if (!fgets(buf, sizeof(buf), stdin))
                break;

                buf[strcspn(buf, "\n")] = 0;
                char *line = strdup(buf);


                if (!line) break;

                if (strcmp(line, delimiter) == 0) {
                    free(line);
                    break;
                }

                write(tmp_fd, line, strlen(line));
                write(tmp_fd, "\n", 1);

                free(line);
            }

            lseek(tmp_fd, 0, SEEK_SET);

            in_fd = tmp_fd;
            unlink(heredoc_file);

            free(args[i]);
            free(args[i + 1]);
            for (int j = i; j + 2 <= *argc; j++)
                args[j] = args[j + 2];
            *argc -= 2;
            i--;
        }
    }

    if (in_fd != -1) {
        dup2(in_fd, STDIN_FILENO);
        close(in_fd);
    }

    if (out_fd != -1) {
        dup2(out_fd, STDOUT_FILENO);
        close(out_fd);
    }
}


void replace_alias(char *args[], int *argc) {
    if (!args[0]) return;

    for (int i = 0; i < alias_count; i++) {
        if (strcmp(args[0], aliases[i].name) == 0) {
            char *buf = strdup(aliases[i].command);
            char *tok;
            int new_argc = 0;
            char *tmp_args[64];

            tok = strtok(buf, " ");
            while (tok && new_argc < 63) {
                tmp_args[new_argc++] = strdup(tok);
                tok = strtok(NULL, " ");
            }

            for (int j = 1; j < *argc && new_argc < 64; j++) {
                tmp_args[new_argc++] = strdup(args[j]);
            }

            tmp_args[new_argc] = NULL;

            for (int j = 0; j < *argc; j++) free(args[j]);

            for (int j = 0; j < new_argc; j++) args[j] = tmp_args[j];
            args[new_argc] = NULL;
            *argc = new_argc;

            free(buf);
            break;
        }
    }
}

char* expand_variables(const char *input) {
    if (!input) return NULL;
    char buffer[4096];
    int j = 0;

    for (int i = 0; input[i]; i++) {
        if (input[i] == '$') {
            i++;
            if (input[i] == '{') {
                i++;
                char varname[128];
                int k = 0;
                while (input[i] && input[i] != '}' && k < 127) varname[k++] = input[i++];
                varname[k] = '\0';
                if (input[i] == '}') i++;
                char *val = getenv(varname);
                if (val) {
                    for (int l = 0; val[l]; l++) buffer[j++] = val[l];
                }
            } else {
                char varname[128];
                int k = 0;
                while ((isalnum(input[i]) || input[i]=='_') && k < 127) varname[k++] = input[i++];
                varname[k] = '\0';
                i--;
                char *val = getenv(varname);
                if (val) {
                    for (int l = 0; val[l]; l++) buffer[j++] = val[l];
                }
            }
        } else {
            buffer[j++] = input[i];
        }
    }
    buffer[j] = '\0';
    return strdup(buffer);
}

static void unescape_args(char *args[], int argc) {
    for (int i = 0; i < argc; i++) {
        char *tmp = unescape_string(args[i]);
        free(args[i]);
        args[i] = tmp;
    }
}

int exec_command(char *cmdline) {
    if (!cmdline || !*cmdline) return 0;

    char *args[64];
    int argc = split_args(cmdline, args, 64);
    replace_alias(args, &argc);
    unescape_args(args, argc);

    for (int i = 0; i < argc; i++) {
        char *tmp2 = expand_variables(args[i]);
        free(args[i]);
        args[i] = tmp2;
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
        if (strcmp(args[i], ">") == 0 || strcmp(args[i], ">>") == 0 || strcmp(args[i], "<") == 0 || strcmp(args[i], "<<") == 0) {
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
        int status;
        waitpid(pid, &status, 0);
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
                char *tmp2 = expand_variables(args[j]);
                free(args[j]);
                args[j] = tmp2;
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
        } else if (pid < 0) { perror("fork error"); return 1; }

        if (in_fd != 0) close(in_fd);
        if (i != n - 1) { close(pipefd[1]); in_fd = pipefd[0]; }
        pids[i] = pid;
    }

    for (int i = 0; i < n; i++) {
        int status;
        waitpid(pids[i], &status, 0);
    }

    return 0;
}
