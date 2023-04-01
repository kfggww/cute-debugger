#include "debugger.h"
#include "command.h"
#include "linenoise.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <elf.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>

/*Debugger manipulates breakpoint*/
static RetCode add_breakpoint1(Debugger *d, void *addr) { return 0; }
static RetCode add_breakpoint2(Debugger *d, int lineno) { return 0; }
static RetCode add_breakpoint3(Debugger *d, const char *fn_name) { return 0; }
static RetCode remove_breakpoint(Debugger *d, int id) {}
static RetCode enable_breakpoint(Debugger *d, int id) {}
static RetCode disable_breakpoint(Debugger *d, int id) {}

/*Debugger manipulates tracee*/
RetCode start_tracee(Debugger *d) {
    pid_t pid = fork();
    if (pid == 0) { // In tracee
        printf("starting tracee: %s\n", d->tracee_name);

        long ret = ptrace(PTRACE_TRACEME, 0, NULL, NULL);
        if (ret == -1) {
            printf("ptrace PTRACE_TRACEME fail\n");
            exit(EXIT_FAILURE);
        }

        execl(d->tracee_name, d->tracee_name, NULL);

        printf("You should NEVER see this...\n");
        exit(EXIT_FAILURE);
    } else if (pid < 0) {
        return QDB_ERROR;
    }

    d->tracee_pid = pid;
    return QDB_SUCCESS;
}

RetCode wait_tracee(Debugger *d) {
    int status;
    pid_t pid = waitpid(d->tracee_pid, &status, 0);
    if (pid == -1 || pid != d->tracee_pid) {
        return QDB_ERROR;
    }

    if (WIFEXITED(status))
        return QDB_TRACEE_EXIT;
    else if (WIFSTOPPED(status)) {
        //TODO: check if hit breakpoint, disable, stepi, wait, enable
        return QDB_TRACEE_STOP;
    } else
        return QDB_TRACEE_UNKNOW;
}

static BreakPointOps default_breakpoint_ops = {
    .add_breakpoint_by_addr = add_breakpoint1,
    .add_breakpoint_by_lineno = add_breakpoint2,
    .add_breakpoint_by_fn = add_breakpoint3,
    .remove_breakpoint = remove_breakpoint,
    .enable_breakpoint = enable_breakpoint,
    .disable_breakpoint = disable_breakpoint,
};

static TraceeOps default_tracee_ops = {
    .start_tracee = start_tracee,
    .wait_tracee = wait_tracee,
};

/*Debugger APIs*/
Debugger *create_debugger() {
    Debugger *d = malloc(sizeof(*d));
    memset(d, 0, sizeof(*d));
    return d;
}

void init_debugger(Debugger *d, const char *tracee_name) {
    d->tracee_name = tracee_name;
    d->tracee_pid = -1;
    d->info_manager = create_info_manager(tracee_name);

    d->breakpoint_index = 0;
    d->breakpoint_ops = &default_breakpoint_ops;
    d->tracee_ops = &default_tracee_ops;

    for (int i = 0; i < DEBUGGER_NBREAKPOINTS; i++)
        d->breakpoints[i].state = BREAKPOINT_UNUSED;
}

void run_debugger(Debugger *d) {
    d->tracee_ops->start_tracee(d);
    RetCode ret = d->tracee_ops->wait_tracee(d);

    char *line = NULL;
    while (!(ret == QDB_TRACEE_EXIT || ret == QDB_TRACEE_UNKNOW)) {
        line = linenoise("(qdb) ");
        CommandArgument arg;
        CommandType cmd_type = command_type_of(line, &arg);
        RetCode cmd_ret = command_handlers[cmd_type](d, &arg);
        linenoiseFree(line);

        if (cmd_ret == QDB_DEBUGGER_MORE_INPUT)
            continue;
        else if (cmd_ret == QDB_DEBUGGER_EXIT)
            break;

        ret = d->tracee_ops->wait_tracee(d);
    }
}

void destroy_debugger(Debugger *d) {
    destroy_info_manager(d->info_manager);
    free(d);
}