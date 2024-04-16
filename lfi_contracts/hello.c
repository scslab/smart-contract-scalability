#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

int cmain() {
    int* p = malloc(10);
    *p = 10;
    printf("hello %p\n", p);
    return 0;
}
