#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <signal.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "tracee.h"

static void start_tracee(Tracee *t) {}

static void wait_tracee(Tracee *t) {}

static TraceeOps default_tracee_ops = {
    .start_tracee = start_tracee,
    .wait_tracee = wait_tracee,
};

Tracee *tracee_new() {
    Tracee *t = calloc(1, sizeof(*t));
    return t;
}

void tracee_init(Tracee *t, int argc, char **argv) {
    t->pid = -1;

    int n = strlen(argv[0]) + 1;
    t->program = malloc(n);
    strncpy(t->program, argv[0], n);

    t->argv = malloc(sizeof(char *) * (argc + 1));
    for (int i = 0; i < argc; i++) {
        int n = strlen(argv[i]) + 1;
        t->argv[i] = malloc(n);
        strncpy(t->argv[i], argv[i], n);
    }
    t->argv[argc] = NULL;

    t->state = kTraceeReady;
    t->ops = &default_tracee_ops;
}

void tracee_destroy(Tracee *t) {
    if (t == NULL)
        return;

    if (t->program)
        free(t->program);

    if (t->argv) {
        int i = 0;
        char *parg = t->argv[i];
        while (parg) {
            free(parg);
            i++;
            parg = t->argv[i];
        }
        free(t->argv);
    }
}