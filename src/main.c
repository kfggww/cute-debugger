#include <stdio.h>
#include <stdlib.h>
#include "debugger.h"

int main(int argc, char const *argv[]) {

    if (argc <= 1) {
        printf("usage: qdb <tracee>\n");
        return EXIT_FAILURE;
    }

    Debugger *d = create_debugger();
    init_debugger(d, argv[1]);
    run_debugger(d);
    destroy_debugger(d);

    return 0;
}
