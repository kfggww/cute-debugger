#ifndef TRACEE_H
#define TRACEE_H

#include "breakpoint.h"

enum TraceeState {
    kTraceeUnused,
    kTraceeReady,
    kTraceeRunning,
    kTraceeTrap,
    kTraceeStop,
    kTraceeExit,
    kTraceeUnknown,
};

struct TraceeOps;

typedef struct Tracee {
    int pid;
    char *program;
    char **argv;
    int state;
    struct TraceeOps *ops;
    BreakPoint breakpoints[16];
    int hit_index;
} Tracee;

typedef struct TraceeOps {
    void (*start_tracee)(Tracee *t, int argc, char **argv);
    void (*wait_tracee)(Tracee *t);
    void (*continue_tracee)(Tracee *t);
    void (*create_breakpoint)(Tracee *t, const char *loc, int oneshot);
} TraceeOps;

Tracee *tracee_new();
void tracee_init(Tracee *t, int argc, char **argv);
void tracee_destroy(Tracee *t);

#endif