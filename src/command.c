#include "command.h"

#include <stdio.h>
#include <string.h>
#include <sys/ptrace.h>

#define SET_BREAKPOINT_LINENO 0
#define SET_BREAKPOINT_ADDR 1
#define SET_BREAKPOINT_FNNAME 2

/*Command handlers*/
static RetCode handle_break_cmd(Debugger *d, CommandArgument *arg) {
    RetCode ret = QDB_ERROR;
    if (arg->data.set_brkpt.set_method == SET_BREAKPOINT_ADDR) {
        ret = d->breakpoint_ops->add_breakpoint_by_addr(
            d, arg->data.set_brkpt.addr);
        return ret;
    } else if (arg->data.set_brkpt.set_method == SET_BREAKPOINT_FNNAME) {
        ret = d->breakpoint_ops->add_breakpoint_by_fn(
            d, arg->data.set_brkpt.fnname);
        return ret;
    } else if (arg->data.set_brkpt.set_method == SET_BREAKPOINT_LINENO) {
        ret = d->breakpoint_ops->add_breakpoint_by_lineno(
            d, arg->data.set_brkpt.lineno);
        return ret;
    }
}

static RetCode handle_break_info_cmd(Debugger *d, CommandArgument *arg) {
    printf("BREAKPOINT INFO:\n");
    for (int i = 0; i < DEBUGGER_NBREAKPOINTS; i++) {
        BreakPoint *bpt = &d->breakpoints[i];
        show_breakpoint_info(bpt, i);
    }
}

static RetCode handle_break_enable_cmd(Debugger *d, CommandArgument *arg) {}

static RetCode handle_break_disable_cmd(Debugger *d, CommandArgument *arg) {}

static RetCode handle_break_delete_cmd(Debugger *d, CommandArgument *arg) {}

static RetCode handle_step_cmd(Debugger *d, CommandArgument *arg) {}

static RetCode handle_stepi_cmd(Debugger *d, CommandArgument *arg) {
    RetCode ret = QDB_ERROR;
    if (d->hit_index >= 0 && d->hit_index < DEBUGGER_NBREAKPOINTS) {
        ret = d->breakpoint_ops->on_breakpoint_hit(d);
        if (ret == QDB_ERROR)
            return QDB_TRACEE_UNKNOW;
        return ret;
    } else {
        long err = ptrace(PTRACE_SINGLESTEP, d->tracee_pid, NULL, NULL);
        if (err == -1)
            return QDB_TRACEE_UNKNOW;
        ret = d->tracee_ops->wait_tracee(d);
        return ret;
    }
}

static RetCode handle_continue_cmd(Debugger *d, CommandArgument *arg) {
    RetCode ret = QDB_ERROR;
    if (d->hit_index >= 0 && d->hit_index < DEBUGGER_NBREAKPOINTS) {
        ret = d->breakpoint_ops->on_breakpoint_hit(d);
        if (ret == QDB_ERROR)
            return QDB_TRACEE_UNKNOW;
    }

    long err = -1;
    err = ptrace(PTRACE_CONT, d->tracee_pid, NULL, NULL);
    if (err == -1) {
        return QDB_TRACEE_UNKNOW;
    }
    ret = d->tracee_ops->wait_tracee(d);
    return ret;
}

static RetCode handle_list_source_cmd(Debugger *d, CommandArgument *arg) {}

static RetCode handle_quit_cmd(Debugger *d, CommandArgument *arg) {
    return QDB_DEBUGGER_EXIT;
}

static RetCode handle_unknown_cmd(Debugger *d, CommandArgument *arg) {}

CommandType command_type_of(const char *line, CommandArgument *arg) {
    if (strstr(line, "break")) {
        arg->type = CMD_BREAK;
        char cmd[16];
        unsigned long addr;
        unsigned long lineno;
        char func_name[32];

        int n = sscanf(line, "%s *%lx", cmd, &addr);
        if (n == 2) {
            arg->data.set_brkpt.addr = (void *)addr;
            arg->data.set_brkpt.set_method = SET_BREAKPOINT_ADDR;
            return CMD_BREAK;
        }

        n = sscanf(line, "%s%lu", cmd, &lineno);
        if (n == 2) {
            arg->data.set_brkpt.lineno = lineno;
            arg->data.set_brkpt.set_method = SET_BREAKPOINT_LINENO;
            return CMD_BREAK;
        }

        n = sscanf(line, "%s%s", cmd, func_name);
        if (n == 2) {
            memcpy(arg->data.set_brkpt.fnname, func_name, 32);
            arg->data.set_brkpt.set_method = SET_BREAKPOINT_FNNAME;
            return CMD_BREAK;
        }

        return CMD_UNKNOWN;
    } else if (strstr(line, "info")) {
        return CMD_BREAK_INFO;
    } else if (strstr(line, "enable")) {
        return CMD_BREAK_ENABLE;
    } else if (strstr(line, "disable")) {
        return CMD_BREAK_DISABLE;
    } else if (strstr(line, "delete")) {
        return CMD_BREAK_DELETE;
    } else if (strstr(line, "stepi")) {
        return CMD_STEPI;
    } else if (strstr(line, "step")) {
        return CMD_STEP;
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