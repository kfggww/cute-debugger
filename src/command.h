#ifndef COMMAND_H
#define COMMAND_H

#include "debugger.h"

typedef enum CommandType {
    CMD_BREAK,
    CMD_BREAK_INFO,
    CMD_BREAK_ENABLE,
    CMD_BREAK_DISABLE,
    CMD_BREAK_DELETE,
    CMD_STEP,
    CMD_STEPI,
    CMD_CONTINUE,
    CMD_LIST_SOURCE,
    CMD_QUIT,
    CMD_UNKNOWN,
} CommandType;

typedef struct CommandArgument {
    int type;
    union {
        struct { /*set breakpoint*/
            int set_method;
            int lineno;
            void *addr;
            char fnname[32];
        } set_brkpt;
        struct { /*enable breakpoint*/
            int id;
        } enable_brkpt;
        struct { /*disable breakpoint*/
            int id;
        } disable_brkpt;
        struct { /*delete breakpoint*/
            int id;
        } delete_brkpt;
    } data;
} CommandArgument;

/*Check the user command type, fill the command argument*/
CommandType command_type_of(const char *line, CommandArgument *arg);

/*Handle user input commands*/
typedef RetCode (*CommandHandler)(Debugger *d, CommandArgument *arg);
extern CommandHandler command_handlers[];

#endif