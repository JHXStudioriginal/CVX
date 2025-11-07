#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdbool.h>
#include "exec.h"
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

int split_args(const char *line, char *args[], int max_args) {
    int argc = 0;
    const char *p = line;
    char buffer[1024];
    int buf_i = 0;
    bool in_quotes = false;
    char quote_char = '\0';

    while (*p) {
        if (*p == '#' && !in_quotes) {
            break;
        }
        if (argc >= max_args - 1) break;

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

            char *line = NULL;
            size_t len = 0;
            ssize_t read;
            while ((read = getline(&line, &len, stdin)) != -1) {
                if (strncmp(line, delimiter, strlen(delimiter)) == 0 &&
                    (line[strlen(delimiter)] == '\n' || line[strlen(delimiter)] == '\0')) {
                    break;
                }
                write(tmp_fd, line, read);
            }
            free(line);
            lseek(tmp_fd, 0, SEEK_SET);

            in_fd = tmp_fd;

            free(args[i]);
            free(args[i + 1]);
            for (int j = i; j + 2 <= *argc; j++) args[j] = args[j + 2];
            *argc -= 2;
            i--;
        }
    }

    if (in_fd >= 0) {
        dup2(in_fd, STDIN_FILENO);
        close(in_fd);
    }
    if (out_fd >= 0) {
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

int exec_command(char *cmdline) {
    if (!cmdline || !*cmdline) return 0;

    char *args[64];
    int argc = split_args(cmdline, args, 64);
    replace_alias(args, &argc);

    for (int i = 0; i < argc; i++) {
        if (strcmp(args[i], "!!") == 0) {
            if (last_cmd) {
                free(args[i]);
                args[i] = strdup(last_cmd);
            } else {
                printf("No command in history\n");
                free_args(args, argc);
                return 1;
            }
        }
    }

    linenoiseHistoryAdd(cmdline);
    if (last_cmd) free((void*)last_cmd);
    last_cmd = strdup(cmdline);

    if (argc == 0) return 0;

    if (strcmp(args[0], "export") == 0) {
        if (argc < 2) {
            printf("export: usage: export VAR=value\n");
            free_args(args, argc);
            return 1;
        }

        for (int i = 1; i < argc; i++) {
            char *eq = strchr(args[i], '=');
            if (eq) {
                *eq = '\0';
                const char *var = args[i];
                const char *val = eq + 1;
                if (setenv(var, val, 1) != 0) {
                    perror("export");
                }
            } else {
                fprintf(stderr, "export: invalid format '%s'\n", args[i]);
            }
        }

        free_args(args, argc);
        return 0;
    }

    if (strcmp(args[0], "cd") == 0) {
        if (argc < 2) {
            fprintf(stderr, "cd: no argument\n");
            free_args(args, argc);
            return 1;
        }
        char *target = args[1];
        char resolved_path[1024];
        if (target[0] == '~') {
            const char *home = getenv("HOME");
            if (!home) home = "/";
            snprintf(resolved_path, sizeof(resolved_path), "%s%s", home, target + 1);
            target = resolved_path;
        }
        if (chdir(target) != 0) {
            perror("cd error");
            free_args(args, argc);
            return 1;
        }
        if (getcwd(current_dir, sizeof(current_dir)) == NULL) {
            perror("getcwd error");
            current_dir[0] = '\0';
        }
        free_args(args, argc);
        return 0;
    }

    if (strcmp(args[0], "pwd") == 0) {
        bool physical = false;

        for (int i = 1; i < argc; i++) {
            if (strcmp(args[i], "-P") == 0) {
                physical = true;
            } else if (strcmp(args[i], "-L") == 0) {
                physical = false;
            } else if (strcmp(args[i], "--help") == 0) {
                printf("Usage: pwd [OPTION]...\n");
                printf("Print the name of the current working directory.\n\n");
                printf("Options:\n");
                printf("  -L      print the logical current working directory (default)\n");
                printf("  -P      print the physical current working directory (resolving symlinks)\n");
                printf("      --help     display this help and exit\n");
                free_args(args, argc);
                return 0;
            }
        }

        if (physical) {
            char real[1024];
            if (realpath(current_dir, real)) {
                printf("%s\n", real);
            } else {
                perror("pwd -P error");
            }
        } else {
            printf("%s\n", current_dir);
        }

        free_args(args, argc);
        return 0;
    }

    if (strcmp(args[0], "help") == 0) {
        printf("\nCVX Shell Help:\n");
        printf("Built-in commands:\n");
        printf("  cd [dir]       - change current directory\n");
        printf("  pwd [-L|-P|-help]    - print current directory\n");
        printf("  ls             - list files\n");
        printf("  history        - show command history (~/.cvx_history)\n");
        printf("  echo [args]    - print arguments\n");
        printf("  export [VAR=value] - set environment variable\n");
        printf("  !!             - repeat last command\n");
        printf("  && and |       - command chaining and pipelines\n");
        printf("  # [comment]      - inline comments\n");
        printf("  cvx --version, cvx -version, cvx -v  - show shell version\n");
        printf("  help           - show this help message\n");
        printf("\nExternal commands can be executed as usual.\n");
        free_args(args, argc);
        return 0;
    }

    if (strcmp(args[0], "ls") == 0) {
        bool has_color = false;
        for (int i = 1; i < argc; i++) {
            if (strcmp(args[i], "--color=auto") == 0) {
                has_color = true;
                break;
            }
        }
        if (!has_color && argc < 63) {
            args[argc] = strdup("--color=auto");
            args[argc+1] = NULL;
            argc++;
        }
    }

    if (strcmp(args[0], "history") == 0) {
        const char *home = getenv("HOME");
        if (!home) {
            fprintf(stderr, "history: HOME environment variable not set\n");
            free_args(args, argc);
            return 1;
        }

        char path[1024];
        snprintf(path, sizeof(path), "%s/.cvx_history", home);

        FILE *f = fopen(path, "r");
        if (!f) {
            perror("history: cannot open ~/.cvx_history");
            free_args(args, argc);
            return 1;
        }

        char line[1024];
        while (fgets(line, sizeof(line), f)) {
            printf("%s", line);
        }
        fclose(f);
        free_args(args, argc);
        return 0;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork error");
        free_args(args, argc);
        return 1;
    }

    if (pid == 0) {
        for (int i = 0; i < argc; i++) {
            char *tmp = expand_tilde(args[i]);
            free(args[i]);
            args[i] = tmp;
        }

        handle_redirection(args, &argc);

        if (strcmp(args[0], "echo") == 0 && argc > 1) {
            for (int i = 1; i < argc; i++) {
                if (args[i][0] == '$') {
                    const char *var = getenv(args[i] + 1);
                    if (var) printf("%s", var);
                } else {
                    printf("%s", args[i]);
                }
                if (i < argc - 1) printf(" ");
            }
            printf("\n");
            free_args(args, argc);
            exit(0);
        }

        execvp(args[0], args);
        perror("execvp error");
        free_args(args, argc);
        exit(EXIT_FAILURE);
    } else {
        child_pid = pid;
        int status;
        waitpid(pid, &status, 0);
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
            if (last_cmd) {
                free(cmds[i]);
                cmds[i] = strdup(last_cmd);
            } else {
                printf("No command in history\n");
                return 1;
            }
        }
    }

    for (int i = 0; i < n; i++) {
        if (i != n - 1) {
            if (pipe(pipefd) < 0) {
                perror("pipe error");
                return 1;
            }
        }

        pid_t pid = fork();
        if (pid == 0) {
            char *args[64];
            int argc = split_args(cmds[i], args, 64);
            replace_alias(args, &argc);

            for (int j = 0; j < argc; j++) {
                char *tmp = expand_tilde(args[j]);
                free(args[j]);
                args[j] = tmp;
            }

            if (in_fd != 0) {
                dup2(in_fd, STDIN_FILENO);
                close(in_fd);
            }

            if (i != n - 1) {
                close(pipefd[0]);
                dup2(pipefd[1], STDOUT_FILENO);
                close(pipefd[1]);
            }

            handle_redirection(args, &argc);

            execvp(args[0], args);
            perror("execvp error");
            free_args(args, argc);
            exit(EXIT_FAILURE);
        } else if (pid < 0) {
            perror("fork error");
            return 1;
        }

        if (in_fd != 0) close(in_fd);
        if (i != n - 1) {
            close(pipefd[1]);
            in_fd = pipefd[0];
        }

        pids[i] = pid;
    }

    for (int i = 0; i < n; i++) {
        int status;
        waitpid(pids[i], &status, 0);
    }

    return 0;
}
