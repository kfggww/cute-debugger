#ifndef BREAKPOINT_H
#define BREAKPOINT_H

typedef struct BreakPoint {
    void *addr;
    long original_data;
    int enabled;
    int oneshot;
    int hitcount;
    int lineno;
    char *func_name;
} BreakPoint;

#endif
