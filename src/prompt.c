#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>
#include "prompt.h"

void print_prompt() {
    char home[1024];
    struct passwd *pw = getpwuid(getuid());
    const char *username = pw ? pw->pw_name : "unknown";

    char hostname[256];
    gethostname(hostname, sizeof(hostname));

    strncpy(home, getenv("HOME") ? getenv("HOME") : "", sizeof(home));
    home[sizeof(home)-1] = '\0';

    if (strncmp(current_dir, home, strlen(home)) == 0) {

        printf("%s@%s:~%s$ ", username, hostname, current_dir + strlen(home));
    } else {
        printf("%s@%s:%s$ ", username, hostname, current_dir);
    }
}

const char* get_prompt(void) {
    static char prompt[2048];

    if (strcmp(cvx_prompt, "DEFAULT_PROMPT") == 0) {
        char home[1024], hostname[256];
        struct passwd *pw = getpwuid(getuid());
        const char *username = pw ? pw->pw_name : "unknown";
        gethostname(hostname, sizeof(hostname));
        strncpy(home, getenv("HOME") ? getenv("HOME") : "", sizeof(home));
        home[sizeof(home)-1] = '\0';

        if (strncmp(current_dir, home, strlen(home)) == 0)
            snprintf(prompt, sizeof(prompt), "%s@%s:~%s$ ", username, hostname, current_dir + strlen(home));
        else
            snprintf(prompt, sizeof(prompt), "%s@%s:%s$ ", username, hostname, current_dir);
    } else {

        char buf[2048];
        strncpy(buf, cvx_prompt, sizeof(buf)-1);
        buf[sizeof(buf)-1] = '\0';
        char *p = buf;
        char tmp[2048];
        tmp[0] = '\0';

        while (*p) {
            if (*p == '$') {
                if (strncmp(p, "$USER", 5) == 0) {
                    strncat(tmp, getenv("USER") ? getenv("USER") : "", sizeof(tmp)-strlen(tmp)-1);
                    p += 5;
                    continue;
                } else if (strncmp(p, "$HOST", 5) == 0) {
                    char hostname[256];
                    gethostname(hostname, sizeof(hostname));
                    strncat(tmp, hostname, sizeof(tmp)-strlen(tmp)-1);
                    p += 5;
                    continue;
                }
            }
            strncat(tmp, p, 1);
            p++;
        }
        snprintf(prompt, sizeof(prompt), "%.*s$ ", (int)(sizeof(prompt)-3), tmp);
    }

    return prompt;
}
