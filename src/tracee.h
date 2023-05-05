#ifndef TRACEE_H
#define TRACEE_H

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
} Tracee;

typedef struct TraceeOps {
    void (*start_tracee)(Tracee *t, int argc, char **argv);
    void (*wait_tracee)(Tracee *t);
    void (*continue_tracee)(Tracee *t);
} TraceeOps;

Tracee *tracee_new();
void tracee_init(Tracee *t, int argc, char **argv);
void tracee_destroy(Tracee *t);

#endif