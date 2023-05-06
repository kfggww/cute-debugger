#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <regex.h>
#include <signal.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/user.h>
#include <unistd.h>

#include "tracee.h"

static char *tracee_states[] = {"Unused", "Ready", "Running", "Trap",
                                "Stop",   "Exit",  "Unknown"};

#define ASSERT_TRACEE_STATE(t, cond)                                           \
    if (!(cond)) {                                                             \
        printf("%s: tracee state error, current state [%d]\n", __func__,       \
               t->state);                                                      \
        return;                                                                \
    }

static void start_tracee(Tracee *t, int argc, char **argv) {
    ASSERT_TRACEE_STATE(t, t->state == kTraceeReady || t->state == kTraceeExit);

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
    ASSERT_TRACEE_STATE(t, t->state == kTraceeTrap || t->state == kTraceeStop);

    // TODO: also needed by step, next, stepi, ...etc
    if (t->hit_index != -1) {
        ptrace(PTRACE_SINGLESTEP, t->pid, NULL, NULL);
        /* wait is necessary after a single step ptrace */
        waitpid(t->pid, NULL, 0);
        BreakPoint *bpt_hit = &t->breakpoints[t->hit_index];
        enable_breakpoint(t->pid, bpt_hit);
    }

    t->state = kTraceeRunning;
    ptrace(PTRACE_CONT, t->pid, NULL, NULL);
}

static void update_breakpoint_hit(Tracee *t) {
    if (t->state != kTraceeTrap) {
        return;
    }

    struct user_regs_struct regs;
    ptrace(PTRACE_GETREGS, t->pid, NULL, (void *)&regs);

    for (int i = 0; i < 16; i++) {
        BreakPoint *bpt = &t->breakpoints[i];
        if (bpt->enabled && bpt->addr + 1 == (void *)regs.rip) {
            printf("breakpoint [%d] hit\n", i);

            disable_breakpoint(t->pid, bpt);
            regs.rip -= 1;
            ptrace(PTRACE_SETREGS, t->pid, NULL, &regs);

            if (!bpt->oneshot) {
                t->hit_index = i;
                bpt->hitcount += 1;
            } else {
                bpt->addr = 0;
                bpt->enabled = 0;
                bpt->hitcount = 0;
            }
            break;
        }
    }
}

static void wait_tracee(Tracee *t) {
    printf("enter wait: [%s]\n", tracee_states[t->state]);
    if (t->state != kTraceeRunning) {
        printf("exit wait: [%s]\n", tracee_states[t->state]);
        return;
    }

    int status = 0;
    pid_t wpid = waitpid(t->pid, &status, 0);

    if (WIFEXITED(status)) {
        t->state = kTraceeExit;
    } else if (WIFSTOPPED(status)) {
        if (WSTOPSIG(status) == SIGTRAP)
            t->state = kTraceeTrap;
        else {
            t->state = kTraceeStop;
            printf("in wait: stop signal [%d]\n", WSTOPSIG(status));
        }
    } else {
        t->state = kTraceeUnknown;
    }

    update_breakpoint_hit(t);
    printf("exit wait: [%s]\n", tracee_states[t->state]);
}

void create_breakpoint(Tracee *t, const char *loc, int oneshot) {
    ASSERT_TRACEE_STATE(t, t->state == kTraceeTrap || t->state == kTraceeStop);

    regex_t lineno_pattern;
    regex_t addr_pattern;
    regex_t func_pattern;

    regcomp(&lineno_pattern, "^[0-9]+$", REG_EXTENDED);
    regcomp(&addr_pattern, "^\\*0x[0-9a-fA-F]{4,16}$", REG_EXTENDED);
    regcomp(&func_pattern, "^[a-zA-Z_]+[a-zA-Z0-9_]+$", REG_EXTENDED);

    BreakPoint *bpt = NULL;
    for (int i = 0; i < 16; i++) {
        if (t->breakpoints[i].addr == NULL) {
            bpt = &t->breakpoints[i];
            break;
        }
    }

    if (regexec(&lineno_pattern, loc, 0, NULL, 0) == 0) {
        bpt->lineno = atoi(loc);
        bpt->func_name = NULL;
        // TODO: get addr by lineno
    } else if (regexec(&addr_pattern, loc, 0, NULL, 0) == 0) {
        bpt->addr = (void *)strtol(loc + 1, NULL, 16);
        bpt->lineno = -1;
        bpt->func_name = NULL;
    } else if (regexec(&func_pattern, loc, 0, NULL, 0) == 0) {
        if (bpt->func_name)
            free(bpt->func_name);
        int n = strlen(loc);
        bpt->func_name = malloc(n + 1);
        strncpy(bpt->func_name, loc, n);
        bpt->func_name[n] = '\0';
        bpt->lineno = -1;
        // TODO: get addr by func_name
    } else {
        printf("%s: breakpoint argument error [%s]\n", __func__, loc);
        return;
    }
    // TODO: handle duplicated breakpoints

    enable_breakpoint(t->pid, bpt);
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

    t->hit_index = -1;

    t->state = kTraceeReady;
    t->ops = &default_tracee_ops;
}

void tracee_destroy(Tracee *t) {
    if (t == NULL) {
        return;
    }

    if (t->program) {
        free(t->program);
    }

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