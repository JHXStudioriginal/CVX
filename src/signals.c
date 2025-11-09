// Copyright (c) [2025] [JHXStudioriginal]
// All rights reserved. This file is distributed under the Elasna Ownership License 1.0.
// Preserve author information and file headers. Do not claim ownership of the code.
// Do not sell this software or any derivative work without explicit permission.
// Full license: [https://github.com/JHXStudioriginal/Elasna-Ownership-License/blob/main/LICENSE]


#include <signal.h>
#include <unistd.h>
#include "signals.h"

pid_t child_pid = -1;

void sigint_handler(int signo __attribute__((unused))) {
    if (child_pid > 0) kill(child_pid, SIGINT);
}
