#include <string.h>
#include "linenoise.h"

/*linenoise callbacks*/
void completion_callback(const char *buf, linenoiseCompletions *lc) {
    switch (buf[0]) {
    case 'b':
        linenoiseAddCompletion(lc, "break");
        break;
    case 'i':
        linenoiseAddCompletion(lc, "info");
        break;
    case 'e':
        linenoiseAddCompletion(lc, "enable");
        break;
    case 'd':
        linenoiseAddCompletion(lc, "disable");
        linenoiseAddCompletion(lc, "delete");
        break;
    case 's':
        linenoiseAddCompletion(lc, "stepi");
        linenoiseAddCompletion(lc, "step");
        break;
    case 'c':
        linenoiseAddCompletion(lc, "continue");
        break;
    case 'l':
        linenoiseAddCompletion(lc, "list");
        break;
    case 'q':
        linenoiseAddCompletion(lc, "quit");
        break;
    }
}

char *hint_callback(const char *buf, int *color, int *bold) {
    *color = 35;
    *bold = 0;
    if (strstr(buf, "break")) {
        return " Doc: create new breakpoint; Usage: break "
               "<address>/<lineno>/<function>";
    } else if (strstr(buf, "info")) {
        return " Doc: show information of breakpoints";
    } else if (strstr(buf, "stepi")) {
        return " Doc: step one instruction";
    } else if (strstr(buf, "step")) {
        return " Doc: step one line of code";
    } else if (strstr(buf, "list")) {
        return " Doc: show source code";
    } else if (strstr(buf, "cotinue")) {
        return " Doc: run tracee until hits a breakpoint or exit";
    } else if (strstr(buf, "enable")) {
        return " Doc: enable a breakpoint; Usage: enable <breakpoint id>";
    } else if (strstr(buf, "disable")) {
        return " Doc: disable a breakpoint; Usage: disable <breakpoint id>";
    } else if (strstr(buf, "dekete")) {
        return " Doc: delete a breakpoint; Usage: delete <breakpoint id>";
    }
}