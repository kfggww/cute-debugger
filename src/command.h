#ifndef COMMAND_H
#define COMMAND_H

#include "list.h"

enum {
    CMD_UNKNOWN,
    CMD_QUIT,
    CMD_BREAK,
    CMD_TBREAK,
    CMD_START,
    CMD_RUN,
    CMD_STEP,
    CMD_NEXT,
    CMD_STEPI,
    CMD_NEXTI,
    CMD_CONTINUE,
    CMD_FINISH,
    CMD_PRINT,
    CMD_SET,
    CMD_LIST,
    CMD_INFO,
};

typedef struct Command {
    int type;
    union {
        char **start_args;
        char **run_args;
    } args;
} Command;

typedef int CommandHandlerFunc(Command *cmd, void *privdata);

typedef struct CommandHandler {
    int type;
    CommandHandlerFunc *func;
} CommandHandler;

void register_cmd_handler(int type, CommandHandlerFunc *handler, List *li);

Command *parse_command(char *line);

#endif