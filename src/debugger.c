#include "debugger.h"
#include "command.h"
#include "linenoise.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <elf.h>
#include <signal.h>
#include <sys/user.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>

/*Debugger manipulates breakpoint*/
static RetCode add_breakpoint1(Debugger *d, void *addr) {
    /*Find a unused breakpoint*/
    BreakPoint *bpt = NULL;
    for (int i = 0; i < DEBUGGER_NBREAKPOINTS; i++) {
        if (d->breakpoints[i].state == BREAKPOINT_UNUSED) {
            bpt = &d->breakpoints[i];
            break;
        }
    }
    if (bpt == NULL)
        return QDB_ERROR;

    /*Peek the original data at addr*/
    bpt->addr = addr;
    bpt->data_original = ptrace(PTRACE_PEEKDATA, d->tracee_pid, addr, NULL);

    /*Write trap data*/
    bpt->data_trap = (bpt->data_original & ~0xFF) | 0xCC;
    long err =
        ptrace(PTRACE_POKEDATA, d->tracee_pid, addr, (void *)bpt->data_trap);
    if (err == -1)
        return QDB_ERROR;

    /*Change the state of breakpoint*/
    bpt->state = BREAKPOINT_ENABLED;
    return QDB_SUCCESS;
}

static RetCode add_breakpoint2(Debugger *d, int lineno) { return 0; }

static RetCode add_breakpoint3(Debugger *d, const char *fn_name) { return 0; }

static RetCode remove_breakpoint(Debugger *d, int id) {
    BreakPoint *bpt = &d->breakpoints[id];
    long err = -1;
    if (bpt->state == BREAKPOINT_ENABLED) {
        err = d->breakpoint_ops->disable_breakpoint(d, id);
        if (err == -1)
            return QDB_ERROR;
    }
    bpt->state = BREAKPOINT_UNUSED;
}

static RetCode enable_breakpoint(Debugger *d, int id) {
    long err = -1;
    BreakPoint *bpt = &d->breakpoints[id];
    err = ptrace(PTRACE_POKEDATA, d->tracee_pid, bpt->addr,
                 (void *)bpt->data_trap);
    if (err == -1)
        return QDB_ERROR;
    bpt->state = BREAKPOINT_ENABLED;
    return QDB_SUCCESS;
}

static RetCode disable_breakpoint(Debugger *d, int id) {
    long err = -1;
    BreakPoint *bpt = &d->breakpoints[id];
    err = ptrace(PTRACE_POKEDATA, d->tracee_pid, bpt->addr,
                 (void *)bpt->data_original);
    if (err == -1)
        return QDB_ERROR;
    bpt->state = BREAKPOINT_DISABLED;
    return QDB_SUCCESS;
}

static RetCode on_breakpoint_hit(Debugger *d, int id,
                                 struct user_regs_struct *user_regs) {
    /*Disable the breakpoint*/
    d->breakpoint_ops->disable_breakpoint(d, id);

    /*Set pc to pc - 1*/
    user_regs->rip -= 1;
    long err = ptrace(PTRACE_SETREGS, d->tracee_pid, NULL, user_regs);
    if (err == -1)
        return QDB_ERROR;

    /*Step one instruction*/
    err = ptrace(PTRACE_SINGLESTEP, d->tracee_pid, NULL, NULL);
    if (err == -1)
        return QDB_ERROR;

    /*Wait tracee*/
    int status;
    waitpid(d->tracee_pid, &status, 0);

    if (WIFSTOPPED(status)) { /*Enable breakpoint*/
        d->breakpoint_ops->enable_breakpoint(d, id);
        d->breakpoints[id].hits += 1;
        return QDB_SUCCESS;
    } else {
        return QDB_ERROR;
    }
}

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
        if (WSTOPSIG(status) == SIGTRAP) {
            /*Get current pc*/
            struct user_regs_struct user_regs;
            if (ptrace(PTRACE_GETREGS, d->tracee_pid, NULL, &user_regs) == -1)
                return QDB_TRACEE_UNKNOW;
            void *pc = (void *)user_regs.rip;

            /*Find the breakpoint that the tracee hits*/
            int hit_index = -1;
            for (int i = 0; i < DEBUGGER_NBREAKPOINTS; i++) {
                BreakPoint *bpt = &d->breakpoints[i];
                if (bpt->state == BREAKPOINT_ENABLED && bpt->addr == pc - 1) {
                    hit_index = i;
                    break;
                }
            }

            if (hit_index != -1) {
                RetCode ret = d->breakpoint_ops->on_breakpoint_hit(d, hit_index,
                                                                   &user_regs);
                return ret == QDB_SUCCESS ? QDB_TRACEE_STOP : QDB_TRACEE_UNKNOW;
            }
        }
        return QDB_TRACEE_STOP;
    } else
        return QDB_TRACEE_UNKNOW;
}

RetCode continue_tracee(Debugger *d) {
    long err = -1;
    err = ptrace(PTRACE_CONT, d->tracee_pid, NULL, NULL);
    return err != -1 ? QDB_SUCCESS : QDB_ERROR;
}

static BreakPointOps default_breakpoint_ops = {
    .add_breakpoint_by_addr = add_breakpoint1,
    .add_breakpoint_by_lineno = add_breakpoint2,
    .add_breakpoint_by_fn = add_breakpoint3,
    .remove_breakpoint = remove_breakpoint,
    .enable_breakpoint = enable_breakpoint,
    .disable_breakpoint = disable_breakpoint,
    .on_breakpoint_hit = on_breakpoint_hit,
};

static TraceeOps default_tracee_ops = {
    .start_tracee = start_tracee,
    .wait_tracee = wait_tracee,
    .continue_tracee = continue_tracee,
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

    d->breakpoint_ops = &default_breakpoint_ops;
    d->tracee_ops = &default_tracee_ops;

    for (int i = 0; i < DEBUGGER_NBREAKPOINTS; i++)
        d->breakpoints[i].state = BREAKPOINT_UNUSED;
}

void run_debugger(Debugger *d) {
    d->tracee_ops->start_tracee(d);
    RetCode ret = d->tracee_ops->wait_tracee(d);

    char *line = NULL;
    while (ret != QDB_TRACEE_EXIT && ret != QDB_TRACEE_UNKNOW) {
        line = linenoise("(qdb) ");
        CommandArgument arg;
        CommandType cmd_type = command_type_of(line, &arg);
        RetCode cmd_ret = command_handlers[cmd_type](d, &arg);
        linenoiseFree(line);

        if (cmd_ret == QDB_DEBUGGER_MORE_INPUT)
            continue;
        else if (cmd_ret == QDB_DEBUGGER_EXIT)
            break;

        d->tracee_ops->continue_tracee(d);
        ret = d->tracee_ops->wait_tracee(d);
    }
}

void destroy_debugger(Debugger *d) {
    destroy_info_manager(d->info_manager);
    free(d);
}