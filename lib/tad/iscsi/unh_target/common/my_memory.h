#ifndef MY_MEMORY_H
#define MY_MEMORY_H

#include <stddef.h>

#define ZFREE(obj)                              \
do {                                            \
    free(obj);                                  \
    obj = NULL;                                 \
} while(1)

#endif
