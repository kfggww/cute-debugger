#ifndef DEBUGGER_H
#define DEBUGGER_H

#include <stddef.h>
#include <sys/types.h>
#include <libdwarf/dwarf.h>
#include <libdwarf/libdwarf.h>

#define DEBUGGER_NBREAKPOINTS 16

typedef struct BreakPoint {
    void *addr;
    long data;
    int valid;
} BreakPoint;

typedef struct DebugInfoManager {
    Dwarf_Debug dbg;
} DebugInfoManager;

typedef struct Debugger {
    const char *tracee_name;
    pid_t tracee_pid;
    DebugInfoManager *dbg_info_mgr;
    int bpt_idx;
    BreakPoint breakpoints[DEBUGGER_NBREAKPOINTS];
} Debugger;

/*Debugger APIs*/
Debugger *create_debugger();
void init_debugger(Debugger *debugger, const char *tracee_name);
void run_debugger(Debugger *debugger);
void destroy_debugger(Debugger *debugger);

#endif