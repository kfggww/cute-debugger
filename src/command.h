#ifndef COMMAND_H
#define COMMAND_H

enum CommandType {
    kCommandQuit,
    kCommandStart,
    kCommandRun,
    kCommandBreak,
    kCommandTBreak,
    kCommandStep,
    kCommandNext,
    kCommandStepi,
    kCommandNexti,
    kCommandContinue,
    kCommandFinish,
    kCommandPrint,
    kCommandSet,
    kCommandInfo,
    kCommandList,
    kCommandUnknown,
};

typedef struct Command {
    int type;
    int nparam;
    char **params;
} Command;

Command *parse_command(const char *line);
void command_parser_init();

#endif