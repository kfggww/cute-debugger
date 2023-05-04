#include <stdio.h>
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

static int command_continue(Command *cmd, void *privdata) {
    if (cmd->type != CMD_CONTINUE)
        return QDB_ERROR;

    Tracee *t = (Tracee *)privdata;
    if (t->state != TRACEE_TRAP && t->state != TRACEE_STOP)
        return QDB_ERROR;

    ptrace(PTRACE_CONT, t->pid, NULL, NULL);
    return tracee_wait(t);
}

void tracee_init(Tracee *t, char *elfpath, char **args) {
    t->pid = -1;
    t->elfpath = elfpath;
    t->args = args;
    t->state = TRACEE_READY;
    t->optset = 0;

    t->cmd_handlers = list_new();
    register_cmd_handler(CMD_CONTINUE, command_continue, t->cmd_handlers);
}

int tracee_start(Tracee *t) {
    int err = QDB_SUCCESS;

    pid_t pid = fork();
    if (pid == 0) {
        ptrace(PTRACE_TRACEME, 0, NULL, NULL);
        pid = getpid();
        kill(pid, SIGTRAP);
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
        if (t->optset == 0) {
            long data = PTRACE_O_TRACEEXEC | PTRACE_O_EXITKILL;
            ptrace(PTRACE_SETOPTIONS, t->pid, NULL, (void *)data);
            t->optset = 1;
            t->state = TRACEE_RUNNING;
            ptrace(PTRACE_CONT, t->pid, NULL, NULL);
            return tracee_wait(t);
        }
        return QDB_SUCCESS;
    } else {
        return QDB_ERROR;
    }
}

void tracee_accept_command(Tracee *t, Command *cmd) {
    list_foreach(t->cmd_handlers, entry) {
        CommandHandler *handler = (CommandHandler *)entry->data;
        if (handler && handler->func && cmd->type == handler->type) {
            handler->func(cmd, t);
            return;
        }
    }
}