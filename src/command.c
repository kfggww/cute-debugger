#include "command.h"

#include <string.h>

#define SET_BREAKPOINT_LINENO 0
#define SET_BREAKPOINT_ADDR 1
#define SET_BREAKPOINT_FNNAME 2

/*Command handlers*/
static RetCode handle_break_cmd(Debugger *d, CommandArgument *arg) {}
static RetCode handle_break_info_cmd(Debugger *d, CommandArgument *arg) {}
static RetCode handle_break_enable_cmd(Debugger *d, CommandArgument *arg) {}
static RetCode handle_break_disable_cmd(Debugger *d, CommandArgument *arg) {}
static RetCode handle_break_delete_cmd(Debugger *d, CommandArgument *arg) {}
static RetCode handle_step_cmd(Debugger *d, CommandArgument *arg) {}
static RetCode handle_stepi_cmd(Debugger *d, CommandArgument *arg) {}
static RetCode handle_continue_cmd(Debugger *d, CommandArgument *arg) {}
static RetCode handle_list_source_cmd(Debugger *d, CommandArgument *arg) {}
static RetCode handle_quit_cmd(Debugger *d, CommandArgument *arg) {}
static RetCode handle_unknown_cmd(Debugger *d, CommandArgument *arg) {}

CommandType command_type_of(const char *line, CommandArgument *arg) {
    if (strstr(line, "break")) {
        return CMD_BREAK;
    } else if (strstr(line, "info")) {
        return CMD_BREAK_INFO;
    } else if (strstr(line, "enable")) {
        return CMD_BREAK_ENABLE;
    } else if (strstr(line, "disable")) {
        return CMD_BREAK_DISABLE;
    } else if (strstr(line, "delete")) {
        return CMD_BREAK_DELETE;
    } else if (strstr(line, "step")) {
        return CMD_STEP;
    } else if (strstr(line, "stepi")) {
        return CMD_STEPI;
    } else if (strstr(line, "continue")) {
        return CMD_CONTINUE;
    } else if (strstr(line, "list")) {
        return CMD_LIST_SOURCE;
    } else if (strstr(line, "quit")) {
        return CMD_QUIT;
    } else {
        return CMD_UNKNOWN;
    }
}

CommandHandler command_handlers[] = {
    handle_break_cmd,         handle_break_info_cmd,   handle_break_enable_cmd,
    handle_break_disable_cmd, handle_break_delete_cmd, handle_step_cmd,
    handle_stepi_cmd,         handle_continue_cmd,     handle_list_source_cmd,
    handle_quit_cmd,          handle_unknown_cmd,
};