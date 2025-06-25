#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdbool.h>
#include <pwd.h>
#include <fcntl.h>
#include "linenoise.h"

pid_t child_pid = -1;

void sigint_handler(int signo) {
    if (child_pid > 0) {
        kill(child_pid, SIGINT);
    }
}

void print_prompt() {
    char cwd[1024];
    char home[1024];
    getcwd(cwd, sizeof(cwd));

    struct passwd *pw = getpwuid(getuid());
    const char *username = pw ? pw->pw_name : "unknown";

    char hostname[256];
    gethostname(hostname, sizeof(hostname));

    strncpy(home, getenv("HOME") ? getenv("HOME") : "", sizeof(home));
    home[sizeof(home)-1] = '\0';

    if (strncmp(cwd, home, strlen(home)) == 0) {
        printf("\033[1;32m%s@%s\033[0m:\033[1;34m~%s\033[0m$ ", username, hostname, cwd + strlen(home));
    } else {
        printf("\033[1;32m%s@%s\033[0m:\033[1;34m%s\033[0m$ ", username, hostname, cwd);
    }
}

const char* get_prompt(void) {
    static char prompt[2048];
    char cwd[1024];
    char home[1024];
    getcwd(cwd, sizeof(cwd));

    struct passwd *pw = getpwuid(getuid());
    const char *username = pw ? pw->pw_name : "unknown";

    char hostname[256];
    gethostname(hostname, sizeof(hostname));

    strncpy(home, getenv("HOME") ? getenv("HOME") : "", sizeof(home));
    home[sizeof(home) - 1] = '\0';

    if (strncmp(cwd, home, strlen(home)) == 0) {
        snprintf(prompt, sizeof(prompt),
            "%s@%s:~%s$ ",
            username, hostname, cwd + strlen(home));
    } else {
        snprintf(prompt, sizeof(prompt),
            "%s@%s:%s$ ",
            username, hostname, cwd);
    }
    return prompt;
}

int split_args(const char *line, char *args[], int max_args) {
    int argc = 0;
    const char *p = line;
    char buffer[1024];
    int buf_i = 0;
    bool in_quotes = false;
    char quote_char = '\0';

    while (*p) {
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

void handle_redirection(char *args[], int *argc) {
    int in_fd = -1, out_fd = -1;
    for (int i = 0; i < *argc; i++) {
        if (strcmp(args[i], "<") == 0 && i + 1 < *argc) {
            in_fd = open(args[i + 1], O_RDONLY);
            if (in_fd < 0) perror("open input");

            free(args[i]); free(args[i+1]);
            for (int j = i; j + 2 <= *argc; j++) args[j] = args[j + 2];
            *argc -= 2;
            args[*argc] = NULL;
            i--;
        } else if (strcmp(args[i], ">>") == 0 && i + 1 < *argc) {
            out_fd = open(args[i + 1], O_WRONLY | O_CREAT | O_APPEND, 0644);
            if (out_fd < 0) perror("open append output");
            free(args[i]); free(args[i+1]);
            for (int j = i; j + 2 <= *argc; j++) args[j] = args[j + 2];
            *argc -= 2;
            args[*argc] = NULL;
            i--;
        } else if (strcmp(args[i], ">") == 0 && i + 1 < *argc) {
            out_fd = open(args[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (out_fd < 0) perror("open output");
            free(args[i]); free(args[i+1]);
            for (int j = i; j + 2 <= *argc; j++) args[j] = args[j + 2];
            *argc -= 2;
            args[*argc] = NULL;
            i--;
        }
    }

    if (in_fd > 0) {
        dup2(in_fd, STDIN_FILENO);
        close(in_fd);
    }
    if (out_fd > 0) {
        dup2(out_fd, STDOUT_FILENO);
        close(out_fd);
    }
}

int exec_command(char *cmdline) {
    char *args[64];
    int argc = split_args(cmdline, args, 64);

    if (args[0] == NULL) return 0;

    if (strcmp(args[0], "cd") == 0) {
        if (argc < 2) {
            fprintf(stderr, "cd: no argument\n");
            return 1;
        }
        if (chdir(args[1]) != 0) {
            perror("cd error");
            return 1;
        }
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
            args[argc] = "--color=auto";
            args[argc+1] = NULL;
            argc++;
        }
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork error");
        return 1;
    }

    if (pid == 0) {

        handle_redirection(args, &argc);

        execvp(args[0], args);
        perror("exec error");
        exit(EXIT_FAILURE);
    } else {
        child_pid = pid;
        int status;
        waitpid(pid, &status, 0);
        child_pid = -1;
        if (WIFEXITED(status)) {
            return WEXITSTATUS(status);
        }
        return 1;
    }
}

int execute_pipeline(char **cmds, int n) {
    int in_fd = 0;
    int pipefd[2];
    pid_t pids[16];

    for (int i = 0; i < n; i++) {
        if (i != n - 1) {
            pipe(pipefd);
        }

        pid_t pid = fork();
        if (pid == 0) {
            if (in_fd != 0) {
                dup2(in_fd, STDIN_FILENO);
                close(in_fd);
            }
            if (i != n - 1) {
                close(pipefd[0]);
                dup2(pipefd[1], STDOUT_FILENO);
                close(pipefd[1]);
            }
            char *args[64];
            int argc = split_args(cmds[i], args, 64);

            handle_redirection(args, &argc);

            execvp(args[0], args);
            perror("exec error");
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
        waitpid(pids[i], NULL, 0);
    }
}    

    int main() {
        char *line;
        signal(SIGINT, sigint_handler);
        setenv("TERM", "xterm", 1);
    
        const char *history_file = ".my_shell_history";
        char history_path[1024];
        snprintf(history_path, sizeof(history_path), "%s/%s", getenv("HOME"), history_file);
        linenoiseHistoryLoad(history_path);
    
        while (1) {
            const char *prompt = get_prompt();
            line = linenoise(prompt);
            if (line == NULL) break;
    
            line[strcspn(line, "\n")] = 0;
    
            if (strcmp(line, "exit") == 0) {
                free(line);
                break;
            }
    
            if (line[0] != '\0') {
                linenoiseHistoryAdd(line);
                linenoiseHistorySave(history_path);
            }
    
            char *seq_cmds[16];
            int seq_count = 0;
            char *part = strtok(line, "&&");
            while (part && seq_count < 16) {
                while (*part == ' ') part++;
                char *end = part + strlen(part) - 1;
                while (end > part && (*end == ' ')) *end-- = '\0';
                seq_cmds[seq_count++] = part;
                part = strtok(NULL, "&&");
            }
    
            for (int i = 0; i < seq_count; i++) {

                char *pipe_cmds[16];
                int pipe_count = 0;
                char *p = strtok(seq_cmds[i], "|");
                while (p && pipe_count < 16) {
                    while (*p == ' ') p++;
                    char *end = p + strlen(p) - 1;
                    while (end > p && (*end == ' ')) *end-- = '\0';
                    pipe_cmds[pipe_count++] = p;
                    p = strtok(NULL, "|");
                }
                if (pipe_count > 1) {
                    if (fork() == 0) {
                        execute_pipeline(pipe_cmds, pipe_count);
                        exit(0);
                    } else {
                        wait(NULL);
                    }
                } else {
                    exec_command(seq_cmds[i]);
                }
            }
    
            free(line);
        }
    
        return 0;
    }
    