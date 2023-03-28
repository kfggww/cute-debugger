#include <stdio.h>
#include <string.h>
#include "linenoise.h"

void completion(const char *buf, linenoiseCompletions *lc) {
    if (buf[0] == 'h') {
        linenoiseAddCompletion(lc, "hello");
        linenoiseAddCompletion(lc, "happy");
    }
}

int main(int argc, char const *argv[]) {

    linenoiseSetCompletionCallback(completion);

    char *line = NULL;
    while ((line = linenoise("(qdb) ")) != NULL) {
        printf("%s\n", line);
        if (strcmp(line, "quit") == 0)
            break;
    }

    return 0;
}
