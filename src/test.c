#include <stdio.h>
#include <stdlib.h>

int add(int a, int b) { return a + b; }

int main(int argc, char const *argv[]) {
    int a = 10;
    int b = 20;

    int c = add(a, b);
    printf("c: %d\n", c);

    int d = add(b, c);
    printf("d: %d\n", d);

    return 0;
}
