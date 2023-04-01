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
    RET_DEBUGGER_MORE_INPUT,
    RET_DEBUGGER_CONTINUE,
    RET_DEBUGGER_EXIT,
};

/*Command types that handled by debugger*/
enum {
    CMD_BREAK,
    CMD_BREAK_INFO,
    CMD_BREAK_ENABLE,
    CMD_BREAK_DISABLE,
    CMD_BREAK_DELETE,
    CMD_STEP,
    CMD_STEPI,
    CMD_CONTINUE,
    CMD_LIST_SOURCE,
    CMD_QUIT,
    CMD_UNKNOWN,
};

/*Breakpoint setting types*/
enum {
    SET_LINENO,
    SET_ADDR,
    SET_FNNAME,
};

/*Command argument*/
typedef struct CommandArgument {
    int type;
    union {
        /*Set breakpoint*/
        struct {
            int set_method;
            int lineno;
            void *addr;
            const char *fn_name;
        };
        /*Enable, disable or delete breakpoint*/
        union {
            int enable_id;
            int disable_id;
            int delete_id;
        };
    } args;
} CommandArgument;

typedef int (*CommandHandler)(Debugger *d, CommandArgument *arg);

/*Check the command type and argument from using input*/
int check_command(const char *line, CommandArgument *arg) {
    if (strstr(line, "break")) {
        return CMD_BREAK;
    } else if (strstr(line, "quit")) {
        return CMD_QUIT;
    }
}

/*Command handlers*/
int command_break(Debugger *d, CommandArgument *arg) {
    printf("set breakpoint done\n");
    return RET_DEBUGGER_MORE_INPUT;
}
int command_break_info(Debugger *d, CommandArgument *arg) {}
int command_break_enable(Debugger *d, CommandArgument *arg) {}
int command_break_disable(Debugger *d, CommandArgument *arg) {}
int command_break_delete(Debugger *d, CommandArgument *arg) {}
int command_step(Debugger *d, CommandArgument *arg) {}
int command_stepi(Debugger *d, CommandArgument *arg) {}
int command_continue(Debugger *d, CommandArgument *arg) {}
int command_list_source(Debugger *d, CommandArgument *arg) {}
int command_quit(Debugger *d, CommandArgument *arg) {
    printf("debugger exit\n");
    return RET_DEBUGGER_EXIT;
}
int command_unknown(Debugger *d, CommandArgument *arg) {}

static CommandHandler cmd_handlers[] = {
    command_break,         command_break_info,   command_break_enable,
    command_break_disable, command_break_delete, command_step,
    command_stepi,         command_continue,     command_list_source,
    command_quit,          command_unknown,
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

        // TODO: handle extra arguments of the tracee
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

void handle_tracee_hits(Debugger *d) {}

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
    .handle_tracee_hits = handle_tracee_hits,
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
            linenoiseFree(line);
            printf("tracee exit\n");
            break;
        }

        /*Handle user command*/
        CommandArgument arg;
        int cmd_type = check_command(line, &arg);
        int cmd_ret = cmd_handlers[cmd_type](d, &arg);
        linenoiseFree(line);

        if (cmd_ret == RET_DEBUGGER_MORE_INPUT)
            continue;
        else if (cmd_ret == RET_DEBUGGER_EXIT)
            break;

        ret = d->tracee_ops->wait_tracee(d);
        d->tracee_ops->handle_tracee_hits(d);
    }
}

void destroy_debugger(Debugger *d) {
    destroy_info_manager(d->info_manager);
    free(d);
}