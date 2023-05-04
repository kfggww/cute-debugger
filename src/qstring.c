#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "qstring.h"

static int ntokens(char *line) {
    if (line == NULL)
        return 0;

    int i = 0;
    int j = 0;
    int n = 0;

    while (line[j] != '\0') {
        while (line[i] != '\0' && isspace(line[i]))
            i++;
        j = i;

        while (line[j] != '\0' && !isspace(line[j]))
            j++;
        if (j > i)
            n++;
        i = j;
    }

    return n;
}

char **split(char *line) {
    char **tokens = NULL;

    int n = ntokens(line);
    if (n) {
        tokens = malloc(sizeof(char *) * (n + 1));
        tokens[n] = NULL;

        int i = 0;
        int j = 0;
        int k = 0;
        while (line[j] != '\0') {
            while (line[i] != '\0' && isspace(line[i]))
                i++;

            j = i;
            while (line[j] != '\0' && !isspace(line[j]))
                j++;
            if (j > i) {
                tokens[k] = malloc(sizeof(char) * (j - i + 1));
                strncpy(tokens[k], line + i, j - i);
                tokens[k][j - i] = '\0';
                k++;
            }

            i = j;
        }
    }

    return tokens;
}

int len(char **tokens) {
    if (tokens == NULL)
        return 0;

    int n = 0;
    while (tokens[n])
        n++;

    return n;
}

void free_all(char **arr) {
    if (arr == NULL)
        return;

    int i = len(arr);
    while (i > 0) {
        free(arr[i - 1]);
        i--;
    }
    free(arr);
}