#include <signal.h>
#include <unistd.h>
#include "signals.h"

pid_t child_pid = -1;

void sigint_handler(int signo __attribute__((unused))) {
    if (child_pid > 0) kill(child_pid, SIGINT);
}
