#ifndef DEBUGGER_H
#define DEBUGGER_H

#include <stdint.h>
#include <sys/types.h>
#include <libdwarf/dwarf.h>
#include <libdwarf/libdwarf.h>

#define DEBUGGER_NBREAKPOINTS 16

typedef struct BreakPoint {
    void *addr;
    long data;
    int state;
    int hits;
} BreakPoint;

typedef struct DebugInfoManager {
    Dwarf_Debug dbg;
    uint16_t elf_type;
} DebugInfoManager;

struct BreakPointOps;
struct TraceeOps;

typedef struct Debugger {
    const char *tracee_name;
    pid_t tracee_pid;
    DebugInfoManager *info_manager;
    int breakpoint_index;
    BreakPoint breakpoints[DEBUGGER_NBREAKPOINTS];
    struct BreakPointOps *breakpoint_ops;
    struct TraceeOps *tracee_ops;
} Debugger;

typedef struct BreakPointOps {
    int (*add_breakpoint_by_addr)(Debugger *d, void *addr);
    int (*add_breakpoint_by_lineno)(Debugger *d, int lineno);
    int (*add_breakpoint_by_fn)(Debugger *d, const char *fn_name);
    int (*remove_breakpoint)(Debugger *d, int id);
    int (*enable_breakpoint)(Debugger *d, int id);
    int (*disable_breakpoint)(Debugger *d, int id);
} BreakPointOps;

typedef struct TraceeOps {
    int (*start_tracee)(Debugger *d);
    int (*wait_tracee)(Debugger *d);
    void (*handle_tracee_hits)(Debugger *d);
} TraceeOps;

/*Debugger APIs*/
Debugger *create_debugger();
void init_debugger(Debugger *d, const char *tracee_name);
void run_debugger(Debugger *d);
void destroy_debugger(Debugger *d);

#endif