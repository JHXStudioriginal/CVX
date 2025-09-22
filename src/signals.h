#ifndef SIGNALS_H
#define SIGNALS_H

#include <sys/types.h>
#include <signal.h>

extern pid_t child_pid;

void sigint_handler(int signo);

#endif
