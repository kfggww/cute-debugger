#include "debugger.h"
#include "linenoise.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <elf.h>

#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>

/*State of breakpoints*/
enum {
    BREAKPOINT_UNUSED,
    BREAKPOINT_ENABLED,
    BREAKPOINT_DISABLED,
};

enum {
    RET_SUCCESS,
    RET_ERROR,
    RET_TRACEE_EXIT,
    RET_TRACEE_STOP,
    RET_TRACEE_UNKNOW,
};

/*Debugger manipulates breakpoint*/
static int add_breakpoint1(Debugger *d, void *addr) { return 0; }
static int add_breakpoint2(Debugger *d, int lineno) { return 0; }
static int add_breakpoint3(Debugger *d, const char *fn_name) { return 0; }
static int remove_breakpoint(Debugger *d, int id) {}
static int enable_breakpoint(Debugger *d, int id) {}
static int disable_breakpoint(Debugger *d, int id) {}

/*Debugger manipulates tracee*/
int start_tracee(Debugger *d) {
    pid_t pid = fork();
    if (pid == 0) { // In tracee
        printf("starting tracee: %s\n", d->tracee_name);

        long ret = ptrace(PTRACE_TRACEME, 0, NULL, NULL);
        if (ret == -1) {
            printf("ptrace PTRACE_TRACEME fail\n");
            exit(EXIT_FAILURE);
        }

        execl(d->tracee_name, d->tracee_name, NULL);

        printf("You should NEVER see this...\n");
        exit(EXIT_FAILURE);
    } else if (pid < 0) {
        return RET_ERROR;
    }

    d->tracee_pid = pid;
    return RET_SUCCESS;
}

int wait_tracee(Debugger *d) {
    int status;
    pid_t pid = waitpid(d->tracee_pid, &status, 0);
    if (pid == -1 || pid != d->tracee_pid) {
        return RET_ERROR;
    }

    if (WIFEXITED(status))
        return RET_TRACEE_EXIT;
    else if (WIFSTOPPED(status))
        return RET_TRACEE_STOP;
    else
        return RET_TRACEE_UNKNOW;
}

/*Create and destroy debug info manager*/
static DebugInfoManager *create_info_manager(const char *tracee_name) {
    DebugInfoManager *im = NULL;

    int fd = open(tracee_name, O_RDONLY);
    if (fd == -1)
        return NULL;

    Elf64_Ehdr elf_hdr;
    ssize_t n = read(fd, &elf_hdr, sizeof(elf_hdr));
    if (n != sizeof(elf_hdr))
        goto end;

    im = malloc(sizeof(*im));
    im->elf_type = elf_hdr.e_type;

    Dwarf_Error err;
    if (dwarf_init(fd, DW_DLC_READ, NULL, NULL, &im->dbg, &err) != DW_DLV_OK) {
        free(im);
        im = NULL;
    }

end:
    close(fd);
    return im;
}

static void destroy_info_manager(DebugInfoManager *im) {
    Dwarf_Error err;
    dwarf_finish(im->dbg, &err);
    free(im);
}

static BreakPointOps default_breakpoint_ops = {
    .add_breakpoint_by_addr = add_breakpoint1,
    .add_breakpoint_by_lineno = add_breakpoint2,
    .add_breakpoint_by_fn = add_breakpoint3,
    .remove_breakpoint = remove_breakpoint,
    .enable_breakpoint = enable_breakpoint,
    .disable_breakpoint = disable_breakpoint,
};

static TraceeOps default_tracee_ops = {
    .start_tracee = start_tracee,
    .wait_tracee = wait_tracee,
};

/*Debugger APIs*/
Debugger *create_debugger() {
    Debugger *d = malloc(sizeof(*d));
    memset(d, 0, sizeof(*d));
    return d;
}

void init_debugger(Debugger *d, const char *tracee_name) {
    d->tracee_name = tracee_name;
    d->tracee_pid = -1;
    d->info_manager = create_info_manager(tracee_name);

    d->breakpoint_index = 0;
    d->breakpoint_ops = &default_breakpoint_ops;
    d->tracee_ops = &default_tracee_ops;

    for (int i = 0; i < DEBUGGER_NBREAKPOINTS; i++)
        d->breakpoints[i].state = BREAKPOINT_UNUSED;
}

void run_debugger(Debugger *d) {
    d->tracee_ops->start_tracee(d);
    int ret = d->tracee_ops->wait_tracee(d);

    if (ret == RET_TRACEE_EXIT || ret == RET_TRACEE_UNKNOW) {
        printf("tracee exit\n");
        return;
    }

    char *line = NULL;
    while ((line = linenoise("(qdb) ")) != NULL) {
        if (ret == RET_TRACEE_EXIT || ret == RET_TRACEE_UNKNOW) {
            printf("tracee exit\n");
            break;
        }

        /*Extract the command from line*/

        ret = d->tracee_ops->wait_tracee(d);
    }
}

void destroy_debugger(Debugger *d) {
    destroy_info_manager(d->info_manager);
    free(d);
}