// Copyright (c) 2025 JHXStudioriginal
// This file is part of the Elasna Open Source License v2.
// All original author information and file headers must be preserved.
// For full license text, see: [https://github.com/JHXStudioriginal/Elasna-Ownership-License/blob/main/LICENSE]

#ifndef EXEC_H
#define EXEC_H

#include "parser.h"
#include "commands.h"
#include "config.h"
#include "signals.h"
#include "linenoise.h"

int exec_command(char *cmdline);
int execute_pipeline(char **cmds, int n);

#endif
