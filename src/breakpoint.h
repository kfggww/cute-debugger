#ifndef BREAKPOINT_H
#define BREAKPOINT_H

#include <sys/types.h>

typedef struct BreakPoint {
    void *addr;
    long original_data;
    int enabled;
    int oneshot;
    int hitcount;
    int lineno;
    char *func_name;
} BreakPoint;

void enable_breakpoint(pid_t pid, BreakPoint *bpt);
void disable_breakpoint(pid_t pid, BreakPoint *bpt);

#endif
