#include "commands.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/wait.h>

extern char current_dir[1024];

int cmd_cd(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "cd: no argument\n");
        return 1;
    }

    char *target = argv[1];
    char resolved_path[1024];

    if (target[0] == '~') {
        const char *home = getenv("HOME");
        if (!home) home = "/";
        snprintf(resolved_path, sizeof(resolved_path), "%s%s", home, target + 1);
        target = resolved_path;
    }

    if (chdir(target) != 0) {
        perror("cd error");
        return 1;
    }

    if (getcwd(current_dir, sizeof(current_dir)) == NULL) {
        perror("getcwd error");
        current_dir[0] = '\0';
    }

    return 0;
}

int cmd_pwd(int argc, char **argv) {
    int physical = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-P") == 0) {
            physical = 1;
        } else if (strcmp(argv[i], "-L") == 0) {
            physical = 0;
        } else if (strcmp(argv[i], "--help") == 0) {
            printf("Usage: pwd [OPTION]...\n");
            printf("Print the name of the current working directory.\n\n");
            printf("Options:\n");
            printf("  -L      print the logical current working directory (default)\n");
            printf("  -P      print the physical current working directory (resolving symlinks)\n");
            printf("      --help     display this help and exit\n");
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

    return 0;
}

int cmd_export(int argc, char **argv) {
    if (argc < 2) {
        printf("export: usage: export VAR=value\n");
        return 1;
    }

    for (int i = 1; i < argc; i++) {
        char *eq = strchr(argv[i], '=');
        if (eq) {
            *eq = '\0';
            const char *var = argv[i];
            const char *val = eq + 1;
            if (setenv(var, val, 1) != 0) {
                perror("export");
            }
        } else {
            fprintf(stderr, "export: invalid format '%s'\n", argv[i]);
        }
    }

    return 0;
}

int cmd_help(int argc, char **argv) {
    (void)argc;
    (void)argv;

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
    printf("  help, cvx --help, cvx -help, cvx -h           - show this help message\n");
    printf("\nExternal commands can be executed as usual.\n");

    return 0;
}

int cmd_ls(int argc, char **argv) {
    bool has_color = false;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--color=auto") == 0) {
            has_color = true;
            break;
        }
    }

    char *args[128];
    int new_argc = argc;

    for (int i = 0; i < argc; i++) args[i] = argv[i];

    if (!has_color && argc < 127) {
        args[new_argc] = "--color=auto";
        new_argc++;
    }
    args[new_argc] = NULL;

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork error");
        return 1;
    }

    if (pid == 0) {
        execvp("ls", args);
        perror("execvp ls error");
        exit(EXIT_FAILURE);
    } else {
        int status;
        waitpid(pid, &status, 0);
        return WIFEXITED(status) ? WEXITSTATUS(status) : 1;
    }
}

int cmd_history(int argc, char **argv) {
    (void)argc;
    (void)argv;

    const char *home = getenv("HOME");
    if (!home) {
        fprintf(stderr, "history: HOME environment variable not set\n");
        return 1;
    }

    char path[1024];
    snprintf(path, sizeof(path), "%s/.cvx_history", home);

    FILE *f = fopen(path, "r");
    if (!f) {
        perror("history: cannot open ~/.cvx_history");
        return 1;
    }

    char line[1024];
    while (fgets(line, sizeof(line), f)) {
        printf("%s", line);
    }
    fclose(f);

    return 0;
}

int cmd_echo(int argc, char **argv) {
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '$') {
            const char *var = getenv(argv[i] + 1);
            if (var) printf("%s", var);
        } else {
            printf("%s", argv[i]);
        }
        if (i < argc - 1) printf(" ");
    }
    printf("\n");
    return 0;
}
