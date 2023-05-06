#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/epoll.h>
#include <unistd.h>

#include "command.h"
#include "tracee.h"
#include "debugger.h"

typedef struct Debugger {
    Tracee *current;
    int initialized;
    int stoped;
    int epfd;
    int fd_in;
    int fd_out;
    struct epoll_event events[16];
} Debugger;

static Debugger *qdb = NULL;

static void command_quit_handler(Command *cmd) {
    if (cmd == NULL || cmd->type != kCommandQuit)
        return;

    if (cmd->nparam != 0) {
        printf("error: <quit> accept no arguments\n");
        return;
    }

    qdb->stoped = 1;
}

static void command_start_handler(Command *cmd) {
    Tracee *t = qdb->current;
    if (t == NULL || cmd == NULL)
        return;

    t->ops->start_tracee(t, cmd->nparam, cmd->params);
}

static void command_continue_handler(Command *cmd) {
    Tracee *t = qdb->current;
    if (t == NULL || cmd == NULL)
        return;

    if (cmd->nparam != 0) {
        printf("error: <continue> accept no arguments\n");
        return;
    }

    t->ops->continue_tracee(t);
}

static void command_break_handler(Command *cmd) {
    Tracee *t = qdb->current;
    if (t == NULL || cmd == NULL)
        return;

    if (cmd->nparam != 1) {
        printf("error: <break> accept only 1 argument but got %d\n",
               cmd->nparam);
        return;
    }

    t->ops->create_breakpoint(t, cmd->params[0], 0);
}

static struct CommandHandler {
    int type;
    void (*handler_fn)(Command *cmd);
} command_dispatch_table[] = {
    {
        .type = kCommandQuit,
        .handler_fn = command_quit_handler,
    },
    {
        .type = kCommandStart,
        .handler_fn = command_start_handler,
    },
    {
        .type = kCommandRun,
        .handler_fn = NULL,
    },
    {
        .type = kCommandContinue,
        .handler_fn = command_continue_handler,
    },
    {
        .type = kCommandBreak,
        .handler_fn = command_break_handler,
    },
    {
        .type = kCommandTBreak,
        .handler_fn = NULL,
    },
    {
        .type = kCommandStep,
        .handler_fn = NULL,
    },
    {
        .type = kCommandNext,
        .handler_fn = NULL,
    },
    {
        .type = kCommandStepi,
        .handler_fn = NULL,
    },

    {
        .type = kCommandNexti,
        .handler_fn = NULL,
    },
    {
        .type = kCommandPrint,
        .handler_fn = NULL,
    },
    {
        .type = kCommandSet,
        .handler_fn = NULL,
    },
    {
        .type = kCommandInfo,
        .handler_fn = NULL,
    },
    {
        .type = kCommandList,
        .handler_fn = NULL,
    },
    {.type = kCommandUnknown, .handler_fn = NULL},
};

static void dispatch_command(Command *cmd) {
    if (cmd == NULL)
        return;

    struct CommandHandler *handler = command_dispatch_table;
    while (handler->type <= kCommandUnknown) {
        if (handler->type == cmd->type) {
            if (handler->handler_fn != NULL) {
                handler->handler_fn(cmd);
                break;
            }
        }
        handler++;
    }
}

static void update_tracee(Tracee *t) {
    if (t == NULL)
        return;
    t->ops->wait_tracee(t);
}

static void accept_command() {
    int nfd = epoll_wait(qdb->epfd, qdb->events, 16, -1);
    if (nfd <= 0)
        return;
    for (int i = 0; i < nfd; i++) {
        struct epoll_event *ev = qdb->events + i;
        if ((ev->events & EPOLLIN) && ev->data.fd == qdb->fd_in) {
            char line[128];
            ssize_t n = read(qdb->fd_in, line, 127);
            if (n <= 0)
                return;
            line[n] = '\0';
            Command *cmd = parse_command(line);
            dispatch_command(cmd);
        }
    }
}

void qdb_init(int argc, char **argv) {
    /* If no tracee to debug, just return. */
    if (argc <= 1)
        return;

    if (qdb == NULL)
        qdb = calloc(1, sizeof(*qdb));

    if (qdb->initialized)
        return;

    /* Initialize epoll to accept user input command */
    int fd = epoll_create(16);
    if (fd < 0)
        return;
    qdb->epfd = fd;
    qdb->fd_in = STDIN_FILENO;
    qdb->fd_out = STDOUT_FILENO;

    struct epoll_event ev = {
        .events = EPOLLIN,
        .data.fd = qdb->fd_in,
    };
    epoll_ctl(qdb->epfd, EPOLL_CTL_ADD, qdb->fd_in, &ev);

    /* Initialize current tracee */
    qdb->current = tracee_new();
    tracee_init(qdb->current, argc - 1, argv + 1);

    /* Initialize command parser */
    command_parser_init();

    qdb->initialized = 1;
}

void qdb_run() {
    if (qdb->initialized == 0 || qdb->current == NULL)
        return;

    while (!qdb->stoped) {
        update_tracee(qdb->current);
        accept_command();
    }
}

void qdb_exit() {
    if (qdb == NULL)
        return;

    if (qdb->current) {
        tracee_destroy(qdb->current);
    }

    free(qdb);
}