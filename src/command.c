#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <regex.h>

#include "list.h"
#include "qstring.h"
#include "command.h"

static int match_quit(char **args, Command *cmd) {
    int result = 0;

    regex_t reg;
    int err = regcomp(&reg, "^quit$", REG_EXTENDED);
    if (err) {
        return result;
    }

    err = regexec(&reg, args[0], 0, NULL, 0);
    if (err == 0) {
        result = 1;
        cmd->type = CMD_QUIT;
    }

    return result;
}

static int match_start(char **args, Command *cmd) {
    int result = 0;

    regex_t reg;
    int err = regcomp(&reg, "^start$", REG_EXTENDED);
    if (err) {
        return result;
    }

    err = regexec(&reg, args[0], 0, NULL, 0);
    if (err == 0) {
        result = 1;
        cmd->type = CMD_START;
        int n = len(args) - 1;
        if (n) {
            cmd->args.start_args = malloc(sizeof(char *) * (n + 1));
            cmd->args.start_args[n] = NULL;
            for (int i = 0; i < n; i++) {
                cmd->args.start_args[i] = malloc(strlen(args[i + 1]));
                strcpy(cmd->args.start_args[i], args[i + 1]);
            }
        } else
            cmd->args.start_args = NULL;
    }

    return result;
}

static int match_run(char **args, Command *cmd) { return 0; }
static int match_break(char **args, Command *cmd) { return 0; }
static int match_tbreak(char **args, Command *cmd) { return 0; }
static int match_step(char **args, Command *cmd) { return 0; }
static int match_next(char **args, Command *cmd) { return 0; }
static int match_stepi(char **args, Command *cmd) { return 0; }
static int match_nexti(char **args, Command *cmd) { return 0; }
static int match_continue(char **args, Command *cmd) {

    int result = 0;

    regex_t reg;
    int err = regcomp(&reg, "^continue$", REG_EXTENDED);
    if (err) {
        return result;
    }

    err = regexec(&reg, args[0], 0, NULL, 0);
    if (err == 0) {
        cmd->type = CMD_CONTINUE;
        result = 1;
    }

    return result;
}

static int match_finish(char **args, Command *cmd) { return 0; }
static int match_info(char **args, Command *cmd) { return 0; }
static int match_list(char **args, Command *cmd) { return 0; }
static int match_print(char **args, Command *cmd) { return 0; }
static int match_set(char **args, Command *cmd) { return 0; }

void register_cmd_handler(int type, CommandHandlerFunc *func, List *li) {
    CommandHandler *handler = malloc(sizeof(*handler));
    handler->type = type;
    handler->func = func;

    List *node = malloc(sizeof(*node));
    node->data = handler;

    list_append(li, node);
}

static struct CommandParserTable {
    int type;
    int (*match)(char **args, Command *cmd);
} parser_table[] = {
    {CMD_QUIT, match_quit},     {CMD_START, match_start},
    {CMD_RUN, match_run},       {CMD_BREAK, match_break},
    {CMD_TBREAK, match_tbreak}, {CMD_STEP, match_step},
    {CMD_NEXT, match_next},     {CMD_STEPI, match_stepi},
    {CMD_NEXTI, match_nexti},   {CMD_CONTINUE, match_continue},
    {CMD_FINISH, match_finish}, {CMD_INFO, match_info},
    {CMD_LIST, match_list},     {CMD_PRINT, match_print},
    {CMD_SET, match_set},
};

Command *parse_command(char *line) {
    Command *cmd = malloc(sizeof(*cmd));
    cmd->type = CMD_UNKNOWN;

    char **args = split(line);

    int n = sizeof(parser_table) / sizeof(parser_table[0]);
    for (int i = 0; i < n; i++) {
        struct CommandParserTable *table_entry = parser_table + i;
        if (table_entry->match && table_entry->match(args, cmd)) {
            break;
        }
    }

    free_all(args);
    return cmd;
}