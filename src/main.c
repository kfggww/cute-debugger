#include "debugger.h"

int main(int argc, char **argv) {
    qdb_init(argc, argv);
    qdb_run();
    qdb_exit();
    return 0;
}