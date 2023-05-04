#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/epoll.h>
#include <unistd.h>

#include "list.h"
#include "command.h"
#include "types.h"
#include "tracee.h"
#include "qstring.h"
#include "debugger.h"

typedef struct Debugger {
    Tracee *current;
    List *tracees;
    int run;
    int epfd;
    struct epoll_event events[16];
    List *cmd_handlers;
} Debugger;

static Debugger qdb;

static int command_quit(Command *cmd, void *privdata) {
    if (cmd->type != CMD_QUIT) {
        return QDB_ERROR;
    }

    qdb.run = 0;
    return QDB_SUCCESS;
}

static int command_start(Command *cmd, void *privdata) {
    if (qdb.current == NULL || qdb.current->state == TRACEE_RUNNING ||
        qdb.current->state == TRACEE_TRAP || qdb.current->state == TRACEE_STOP)
        return QDB_ERROR;

    Tracee *t = qdb.current;
    if (cmd->args.start_args != NULL) {
        free_all(t->args);

        int n = len(cmd->args.start_args);
        t->args = malloc(sizeof(char *) * (n + 2));
        t->args[0] = malloc(strlen(t->elfpath));
        strcpy(t->args[0], t->elfpath);

        for (int i = 1; i <= n; i++) {
            t->args[i] = malloc(strlen(cmd->args.start_args[i - 1]));
            strcpy(t->args[i], cmd->args.start_args[i - 1]);
        }

        t->args[n + 1] = NULL;
    }

    return tracee_start(t);
}

static void qdb_accept_command(Command *cmd) {
    list_foreach((qdb.cmd_handlers), node) {
        CommandHandler *handler = (CommandHandler *)node->data;
        if (handler && handler->type == cmd->type) {
            handler->func(cmd, NULL);
        }
    }

    if (qdb.current) {
        tracee_accept_command(qdb.current, cmd);
    }
}

static void event_user_input() {
    char line[128];
    ssize_t n = read(0, line, 128);
    if (n > 0) {
        line[n - 1] = '\0';
        Command *cmd = parse_command(line);
        qdb_accept_command(cmd);
        if (cmd) {
            free(cmd);
            cmd = NULL;
        }
    }
}

static void qdb_wait_events() {
    int nevents = epoll_wait(qdb.epfd, qdb.events, 16, -1);
    for (int i = 0; i < nevents; i++) {
        struct epoll_event *ev = &qdb.events[i];
        if (ev->data.fd == 0) {
            event_user_input();
        }
    }
}

void qdb_init(int argc, char **argv) {
    if (argc <= 1)
        return;

    memset(&qdb, 0, sizeof(qdb));

    int fd = epoll_create(16);
    if (fd == -1) {
        return;
    }
    qdb.epfd = fd;

    /* Register events */
    struct epoll_event ev = {
        .events = EPOLLIN,
        .data.fd = 0,
    };

    int err = epoll_ctl(qdb.epfd, EPOLL_CTL_ADD, 0, &ev);
    if (err) {
        return;
    }

    /* Register commands that can be handled by debugger */
    qdb.cmd_handlers = list_new();
    register_cmd_handler(CMD_QUIT, command_quit, qdb.cmd_handlers);
    register_cmd_handler(CMD_START, command_start, qdb.cmd_handlers);

    /* TODO: tracee related stuff */
    qdb.current = tracee_new();
    Tracee *t = qdb.current;

    char *elfpath = malloc(strlen(argv[1]));
    strcpy(elfpath, argv[1]);

    char **args = malloc(sizeof(char *) * argc);
    args[argc - 1] = NULL;
    for (int i = 0; i < argc - 1; i++) {
        args[i] = malloc(strlen(argv[i + 1]));
        strcpy(args[i], argv[i + 1]);
    }
    tracee_init(t, elfpath, args);

    qdb.run = 1;
}

void qdb_run() {
    int err = QDB_ERROR;

    while (qdb.run) {
        if (qdb.current && qdb.current->state == TRACEE_RUNNING) {
            err = tracee_wait(qdb.current);
            if (err != QDB_SUCCESS) {
            }
        }
        qdb_wait_events();
    }
}

void qdb_exit() {
    /* TODO: free tracees */

    /* free cmd handler list */
    list_destroy(qdb.cmd_handlers);
}