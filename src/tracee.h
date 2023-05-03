#ifndef TRACEE_H
#define TRACEE_H

#include "list.h"
#include "command.h"

enum {
    TRACEE_UNUSED,
    TRACEE_READY,
    TRACEE_RUNNING,
    TRACEE_TRAP,
    TRACEE_STOP,
    TRACEE_EXIT,
    TRACEE_UNKNOWN,
};

typedef struct Tracee {
    int pid;
    char *elfpath;
    char **args;
    int state;

    List *cmd_handlers;
} Tracee;

/* Tracee APIs */
Tracee *tracee_new();
void tracee_destroy(Tracee *t);

void tracee_init(Tracee *t, char *elfpath, char **args);
int tracee_start(Tracee *t);
int tracee_wait(Tracee *t);

void tracee_handle_command(Tracee *t, Command *cmd);

#endif