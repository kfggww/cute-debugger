#ifndef DEBUGGER_H
#define DEBUGGER_H

#include "breakpoint.h"
#include "dbginfo.h"
#include <sys/types.h>

#define DEBUGGER_NBREAKPOINTS 16

typedef enum RetCode {
    QDB_SUCCESS,
    QDB_ERROR,
    QDB_TRACEE_EXIT,
    QDB_TRACEE_STOP,
    QDB_TRACEE_UNKNOW,
    QDB_DEBUGGER_MORE_INPUT,
    QDB_DEBUGGER_CONTINUE,
    QDB_DEBUGGER_EXIT,
} RetCode;

struct BreakPointOps;
struct TraceeOps;

typedef struct Debugger {
    const char *tracee_name;
    pid_t tracee_pid;
    DebugInfoManager *info_manager;
    BreakPoint breakpoints[DEBUGGER_NBREAKPOINTS];
    struct BreakPointOps *breakpoint_ops;
    struct TraceeOps *tracee_ops;
} Debugger;

struct user_regs_struct;

typedef struct BreakPointOps {
    RetCode (*add_breakpoint_by_addr)(Debugger *d, void *addr);
    RetCode (*add_breakpoint_by_lineno)(Debugger *d, int lineno);
    RetCode (*add_breakpoint_by_fn)(Debugger *d, const char *fn_name);
    RetCode (*remove_breakpoint)(Debugger *d, int id);
    RetCode (*enable_breakpoint)(Debugger *d, int id);
    RetCode (*disable_breakpoint)(Debugger *d, int id);
    RetCode (*on_breakpoint_hit)(Debugger *d, int id,
                                 struct user_regs_struct *user_regs);
} BreakPointOps;

typedef struct TraceeOps {
    RetCode (*start_tracee)(Debugger *d);
    RetCode (*wait_tracee)(Debugger *d);
    RetCode (*continue_tracee)(Debugger *d);
} TraceeOps;

/*Debugger APIs*/
Debugger *create_debugger();
void init_debugger(Debugger *d, const char *tracee_name);
void run_debugger(Debugger *d);
void destroy_debugger(Debugger *d);

#endif