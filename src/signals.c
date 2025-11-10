// Copyright (c) 2025 JHXStudioriginal
// This file is part of the Elasna Open Source License v2.
// All original author information and file headers must be preserved.
// For full license text, see: [https://github.com/JHXStudioriginal/Elasna-Ownership-License/blob/main/LICENSE]


#include <signal.h>
#include <unistd.h>
#include "signals.h"

pid_t child_pid = -1;

void sigint_handler(int signo __attribute__((unused))) {
    if (child_pid > 0) kill(child_pid, SIGINT);
}
