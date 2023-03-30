#include "debugger.h"
#include "linenoise.h"

#include <stdio.h>
#include <stdlib.h>

#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

/*Debugger add new breakpoint*/
static int add_breakpoint_addr(Debugger *debugger, void *addr) { return 0; }
static int add_breakpoint_func(Debugger *debugger, const char *func_name) {
    return 0;
}
static int add_breakpoint_line(Debugger *debugger, int linenum) { return 0; }

/*Manipulate breakpoint*/
static int enable_breakpoint(BreakPoint *bp) { return 0; }
static int disable_breakpoint(BreakPoint *bp) { return 0; }
static int delete_breakpoint(BreakPoint *bp) { return 0; }

/*Search funcition lowpc in debug info manager*/
static void *search_func_lowpc(DebugInfoManager *mgr, const char *func_name) {
    return NULL;
}

/*Get the base memory map of tracee*/
static void *tracee_map_base(Debugger *debugger) { return NULL; }

/*Debugger APIs*/
Debugger *create_debugger() { return NULL; }
void init_debugger(Debugger *debugger, const char *tracee_name) {}
void run_debugger(Debugger *debugger) {}
void destroy_debugger(Debugger *debugger) {}