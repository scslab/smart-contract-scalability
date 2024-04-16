#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <syscall.h>


int cmain() {
    int* p = malloc(10);
    *p = 10;
    printf("hello %p\n", p);

    uint64_t clen = calldata_len();

    printf("calldata_len = %lu\n", clen);

    uint8_t bytes[clen+1];

    if (calldata(bytes) != 0)
    {
	    printf("error in calldata\n");

	}
    printf("calldata %s\n", bytes);

    return 0;
}
