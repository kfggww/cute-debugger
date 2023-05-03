#include <stdlib.h>

#include <signal.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "types.h"
#include "tracee.h"

Tracee *tracee_new() {
    Tracee *t = malloc(sizeof(*t));
    return t;
}

void tracee_destroy(Tracee *t) {
    if (t->elfpath) {
        free(t->elfpath);
    }
    for (int i = 0; t->args[i] != NULL; i++) {
        free(t->args[i]);
    }
}

void tracee_init(Tracee *t, char *elfpath, char **args) {
    t->pid = -1;
    t->elfpath = elfpath;
    t->args = args;
    t->state = TRACEE_READY;
}

int tracee_start(Tracee *t) {
    int err = QDB_SUCCESS;

    pid_t pid = fork();
    if (pid == 0) {
        ptrace(PTRACE_TRACEME, 0, NULL, NULL);
        execv(t->elfpath, t->args);
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        t->pid = pid;
        t->state = TRACEE_RUNNING;
    } else {
        err = QDB_ERROR;
    }

    return err;
}

int tracee_wait(Tracee *t) {
    int status = 0;
    pid_t wpid = waitpid(t->pid, &status, 0);

    if (wpid != t->pid) {
        return QDB_ERROR;
    } else if (WIFEXITED(status)) {
        t->state = TRACEE_EXIT;
        return QDB_SUCCESS;
    } else if (WIFSTOPPED(status)) {
        int sig = WSTOPSIG(status);
        if (sig == SIGTRAP) {
            t->state = TRACEE_TRAP;
        } else {
            t->state = TRACEE_STOP;
        }
        return QDB_SUCCESS;
    } else {
        return QDB_ERROR;
    }
}

void tracee_handle_command(Tracee *t, Command *cmd) {

}