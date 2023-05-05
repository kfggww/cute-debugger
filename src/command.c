#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <regex.h>

#include "command.h"

#define NPARAM_MAX 8

static struct CommandPattern {
    int type;
    regex_t pattern;
} command_patterns[kCommandUnknown];

static int first_token(const char *line, char **token) {
    int i = 0;
    int j = 0;
    while (line[i] != '\0' && isspace(line[i]))
        i++;
    j = i;
    while (line[j] != '\0' && !isspace(line[j]))
        j++;

    int n = j - i + 1;
    if (token != NULL) {
        char *cmd = malloc(sizeof(char) * n);
        strncpy(cmd, line, n);
        cmd[n - 1] = '\0';
        *token = cmd;
    }

    return n - 1;
}

static int typeof_command(const char *line) {
    int type = kCommandUnknown;
    if (line == NULL)
        return type;

    /* Get command name */
    char *cmd = NULL;
    first_token(line, &cmd);

    /* Check command type */
    for (int i = 0; i < kCommandUnknown; i++) {
        int err = regexec(&command_patterns[i].pattern, cmd, 0, NULL, 0);
        if (err == 0) {
            type = command_patterns[i].type;
            break;
        }
    }

    if (cmd)
        free(cmd);

    return type;
}

Command *parse_command(const char *line) {
    Command *cmd = malloc(sizeof(*cmd));

    int type = typeof_command(line);
    cmd->type = type;

    char *params[NPARAM_MAX];

    int start = first_token(line, NULL);
    int i = start;
    int j = start;
    int n = 0;

    while (line[j] != '\0') {
        while (line[i] != '\0' && isspace(line[i]))
            i++;
        j = i;
        while (line[j] != '\0' && !isspace(line[j]))
            j++;

        if (j > i) {
            params[n] = malloc(sizeof(char) * (j - i + 1));
            strncpy(params[n], line + i, j - i + 1);
            params[n][j - i] = '\0';
            n++;
            if (n >= NPARAM_MAX)
                break;
        }
    }

    cmd->nparam = n;
    cmd->params = malloc(sizeof(char *) * (n + 1));
    cmd->params[n] = NULL;
    for (int k = 0; k < n; k++) {
        cmd->params[k] = params[k];
    }

    return cmd;
}

void command_parser_init() {
    static int initialized = 0;
    if (initialized)
        return;

    int types[] = {
        kCommandQuit,   kCommandStart,    kCommandRun,    kCommandBreak,
        kCommandTBreak, kCommandStep,     kCommandNext,   kCommandStepi,
        kCommandNexti,  kCommandContinue, kCommandFinish, kCommandPrint,
        kCommandSet,    kCommandInfo,     kCommandList,   kCommandUnknown,
    };

    char *patterns[] = {
        "^quit$", "^start$", "^run$",   "^break$",    "^tbreak$", "^step$",
        "^next$", "^stepi$", "^nexti$", "^continue$", "^finish$", "^print$",
        "^set$",  "^info$",  "^list$",  ".*",
    };

    for (int i = 0; i < kCommandUnknown; i++) {
        command_patterns[i].type = types[i];
        regcomp(&command_patterns[i].pattern, patterns[i], REG_EXTENDED);
    }

    initialized = 1;
}
