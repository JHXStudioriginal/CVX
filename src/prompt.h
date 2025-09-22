#ifndef PROMPT_H
#define PROMPT_H

extern char current_dir[512];
extern char cvx_prompt[2048];

void print_prompt(void);
const char* get_prompt(void);

#endif

