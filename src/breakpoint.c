#include <stdlib.h>

#include <sys/ptrace.h>

#include "types.h"
#include "breakpoint.h"

void enable_breakpoint(pid_t pid, BreakPoint *bpt) {
    if (bpt == NULL || bpt->enabled) {
        return;
    }

    long data = ptrace(PTRACE_PEEKDATA, pid, bpt->addr, NULL);
    bpt->original_data = data;

    data = (data & ~0xff) | 0xcc;
    ptrace(PTRACE_POKEDATA, pid, bpt->addr, (void *)data);

    bpt->enabled = 1;
}

void disable_breakpoint(pid_t pid, BreakPoint *bpt) {
    if (bpt == NULL || !bpt->enabled) {
        return;
    }

    ptrace(PTRACE_POKEDATA, pid, bpt->addr, bpt->original_data);
    bpt->enabled = 0;
}