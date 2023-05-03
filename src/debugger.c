#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/epoll.h>
#include <unistd.h>

#include "list.h"
#include "command.h"
#include "types.h"
#include "tracee.h"
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

static int command_quit(Command *cmd) {
    if (cmd->type != CMD_QUIT) {
        return QDB_ERROR;
    }

    qdb.run = 0;
    return QDB_SUCCESS;
}

static void qdb_handle_command(Command *cmd) {
    list_foreach((qdb.cmd_handlers), node) {
        CommandHandler *handler = (CommandHandler *)node->data;
        if (handler && handler->type == cmd->type) {
            (*handler->func)(cmd);
        }
    }

    if (qdb.current) {
        tracee_handle_command(qdb.current, cmd);
    }
}

static void event_user_input() {
    char line[128];
    ssize_t n = read(0, line, 128);
    if (n > 0) {
        line[n - 1] = '\0';
        Command *cmd = parse_command(line);
        qdb_handle_command(cmd);
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

    /* TODO: tracee related stuff */

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