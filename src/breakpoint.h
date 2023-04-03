#ifndef BREAKPOINT_H
#define BREAKPOINT_H

/*State of breakpoints*/
typedef enum BreakPointState {
    BREAKPOINT_UNUSED,
    BREAKPOINT_ENABLED,
    BREAKPOINT_DISABLED,
} BreakPointState;

typedef struct BreakPoint {
    void *addr;
    long data_original;
    long data_trap;
    BreakPointState state;
    int hits;
} BreakPoint;

void show_breakpoint_info(BreakPoint *bpt, int id);

#endif