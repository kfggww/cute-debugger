#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <regex.h>
#include <signal.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "tracee.h"

static void start_tracee(Tracee *t, int argc, char **argv) {
    if (t == NULL || t->state == kTraceeRunning || t->state == kTraceeTrap ||
        t->state == kTraceeStop || t->state == kTraceeUnknown)
        return;

    /* Reset the tracee arguments */
    if (argc > 0) {
    }

    pid_t pid = fork();
    if (pid == 0) {
        ptrace(PTRACE_TRACEME, 0, NULL, NULL);
        kill(getpid(), SIGTRAP);
        execv(t->program, t->argv);
        exit(EXIT_FAILURE);
    }

    if (pid < 0)
        return;

    /**
     * Setup ptrace options, so that:
     *  1. tracee will stop at next exec syscall;
     *  2. tracee will be killed if debugger exit;
     */
    int status = 0;
    pid_t wpid = waitpid(pid, &status, 0);
    if (wpid != pid || !WIFSTOPPED(status) || WSTOPSIG(status) != SIGTRAP) {
        kill(SIGKILL, pid);
        return;
    }

    t->pid = pid;
    ptrace(PTRACE_SETOPTIONS, pid, NULL,
           PTRACE_O_EXITKILL | PTRACE_O_TRACEEXEC);

    /* Continue running tracee, it will be stoped at exec syscall */
    t->state = kTraceeRunning;
    ptrace(PTRACE_CONT, pid, NULL, NULL);
}

static void continue_tracee(Tracee *t) {
    if (t == NULL || t->state == kTraceeExit || t->state == kTraceeRunning ||
        t->state == kTraceeUnknown)
        return;

    t->state = kTraceeRunning;
    ptrace(PTRACE_CONT, t->pid, NULL, NULL);
}

static void wait_tracee(Tracee *t) {
    if (t == NULL || t->state != kTraceeRunning)
        return;

    int status = 0;
    pid_t wpid = waitpid(t->pid, &status, 0);

    if (WIFEXITED(status))
        t->state = kTraceeExit;
    else if (WIFSTOPPED(status)) {
        if (WSTOPSIG(status) == SIGTRAP)
            t->state = kTraceeTrap;
        else
            t->state = kTraceeStop;
    } else
        t->state = kTraceeUnknown;
}

void create_breakpoint(Tracee *t, const char *loc, int oneshot) {
    if (loc ==
        NULL /* || (t->state != kTraceeTrap && t->state != kTraceeStop) */)
        return;

    regex_t lineno_pattern;
    regex_t addr_pattern;

    regcomp(&lineno_pattern, "^[0-9]+$", REG_EXTENDED);
    regcomp(&addr_pattern, "^\\*0x[0-9a-fA-F]{4,16}$", REG_EXTENDED);

    BreakPoint *bpt = NULL;
    for (int i = 0; i < 16; i++) {
        if (t->breakpoints[i].addr == NULL) {
            bpt = &t->breakpoints[i];
            break;
        }
    }

    if (regexec(&lineno_pattern, loc, 0, NULL, 0) == 0)
        bpt->lineno = atoi(loc);
    // TODO: get addr by lineno
    else if (regexec(&addr_pattern, loc, 0, NULL, 0) == 0)
        bpt->addr = (void *)strtol(loc + 1, NULL, 16);
    else {
        if (bpt->func_name)
            free(bpt->func_name);
        int n = strlen(loc);
        bpt->func_name = malloc(n + 1);
        strncpy(bpt->func_name, loc, n);
        bpt->func_name[n] = '\0';
        // TODO: get addr by func_name
    }

    bpt->original_data = ptrace(PTRACE_PEEKDATA, t->pid, bpt->addr, NULL);
    long modified = (bpt->original_data & (~0xff)) | 0xcc;
    ptrace(PTRACE_POKEDATA, t->pid, bpt->addr, (void *)modified);

    bpt->enabled = 1;
    bpt->oneshot = oneshot;
    bpt->hitcount = 0;
}

static TraceeOps default_tracee_ops = {
    .start_tracee = start_tracee,
    .wait_tracee = wait_tracee,
    .continue_tracee = continue_tracee,
    .create_breakpoint = create_breakpoint,
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