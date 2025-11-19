// Copyright (c) 2025 JHXStudioriginal
// This file is part of the Elasna Open Source License v2.
// All original author information and file headers must be preserved.
// For full license text, see: [https://github.com/JHXStudioriginal/Elasna-Ownership-License/blob/main/LICENSE]


#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include "signals.h"

pid_t child_pid = -1;

void sigint_handler(int signo) {
    (void)signo;
    if (child_pid > 0)
        kill(child_pid, SIGINT);
}

void sigtstp_handler(int signo) {
    (void)signo;
    if (child_pid > 0)
        kill(child_pid, SIGTSTP);
}

void sigchld_handler(int signo) {
    int status;
    pid_t pid;
    (void)signo;

    while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0) {
        if (pid == child_pid) {
            if (WIFEXITED(status) || WIFSIGNALED(status))
                child_pid = -1;
        }
    }
}

void setup_signals(void) {
    struct sigaction sigint_sa, sigtstp_sa, sigchld_sa;

    sigint_sa.sa_handler = sigint_handler;
    sigemptyset(&sigint_sa.sa_mask);
    sigint_sa.sa_flags = SA_RESTART;
    sigaction(SIGINT, &sigint_sa, NULL);

    sigtstp_sa.sa_handler = sigtstp_handler;
    sigemptyset(&sigtstp_sa.sa_mask);
    sigtstp_sa.sa_flags = SA_RESTART;
    sigaction(SIGTSTP, &sigtstp_sa, NULL);

    sigchld_sa.sa_handler = sigchld_handler;
    sigemptyset(&sigchld_sa.sa_mask);
    sigchld_sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    sigaction(SIGCHLD, &sigchld_sa, NULL);

    signal(SIGQUIT, SIG_IGN);
}
