#ifndef MY_MEMORY_H
#define MY_MEMORY_H

#include <stddef.h>

/* largest number of bytes malloc() will allocate as one block of memory */
#define MAX_MALLOC_SIZE		131072

#define ZFREE(obj)                              \
do {                                            \
    free(obj);                                  \
    obj = NULL;                                 \
} while(0)

#endif
