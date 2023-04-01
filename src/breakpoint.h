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
    long data;
    BreakPointState state;
    int hits;
} BreakPoint;

#endif