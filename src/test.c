#include <stdio.h>
#include <stdlib.h>

int add(int a, int b) { return a + b; }

int main(int argc, char const *argv[]) {
    int a = 10;
    int b = 20;
    int c = add(a, b);
    int d = add(b, c);
    printf("%d, %d\n", c, d);

    return 0;
}
