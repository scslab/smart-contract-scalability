#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

int cmain() {
    int* p = malloc(10);
    write(1, "hi\n", 3);
    *p = 10;
    printf("hello %p\n", p);
    return 0;
}
