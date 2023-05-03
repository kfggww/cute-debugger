#include <stdlib.h>
#include <string.h>

#include "list.h"
#include "command.h"

void register_cmd_handler(int type, CommandHandlerFunc func, List *li) {
    CommandHandler *handler = malloc(sizeof(*handler));
    handler->type = type;
    handler->func = func;

    List *node = malloc(sizeof(*node));
    node->data = handler;

    list_append(li, node);
}

Command *parse_command(char *buf) {
    Command *cmd = malloc(sizeof(*cmd));
    if (strstr(buf, "quit")) {
        cmd->type = CMD_QUIT;
    }

    return cmd;
}