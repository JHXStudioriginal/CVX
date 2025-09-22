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

int main() {
    char *line;
    signal(SIGINT, sigint_handler);
    setenv("TERM", "xterm", 1);

    config();

    const char *history_file = ".cvx_history";
    char history_path[1024];
    snprintf(history_path, sizeof(history_path), "%s/%s", getenv("HOME"), history_file);
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
                    int status;
                    wait(&status);
                }
            } else {
                exec_command(pipe_cmds[0]);
            }
        }

        free(line);
    }

    return 0;
}
