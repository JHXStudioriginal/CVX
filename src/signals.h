// Copyright (c) [2025] [JHXStudioriginal]
// All rights reserved. This file is distributed under the Elasna Ownership License 1.0.
// Preserve author information and file headers. Do not claim ownership of the code.
// Do not sell this software or any derivative work without explicit permission.
// Full license: [https://github.com/JHXStudioriginal/CVX-Shell/blob/main/LICENSE]


#ifndef SIGNALS_H
#define SIGNALS_H

#include <sys/types.h>
#include <signal.h>

extern pid_t child_pid;

void sigint_handler(int signo);

#endif
