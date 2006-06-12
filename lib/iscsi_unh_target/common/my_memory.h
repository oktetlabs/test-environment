#ifndef MY_MEMORY_H
#define MY_MEMORY_H

#include "te_defs.h"
#include "te_errno.h"
#include <stddef.h>
#include <string.h>

/* largest number of bytes malloc() will allocate as one block of memory */
#define MAX_MALLOC_SIZE		131072

#define ZFREE(obj)                              \
do {                                            \
    free(obj);                                  \
    obj = NULL;                                 \
} while(0)

#define SHARED volatile

extern te_bool is_shared_ptr(SHARED void *addr);

extern te_errno shared_mem_init(size_t size);
extern SHARED void *shalloc(size_t size);
extern SHARED void *shcalloc(size_t nmemb, size_t size);
extern te_errno shfree(SHARED void *addr);

#define ZSHFREE(obj)                            \
do {                                            \
    shfree(obj);                                \
    obj = NULL;                                 \
} while(0)

static inline size_t
shstrlen(SHARED const char *str)
{
    return strlen((const char *)str);
}

static inline SHARED void *
shmemcpy(SHARED void *dest, SHARED const void *src, size_t size)
{
    return (SHARED void *)memcpy((void *)dest, (const void *)src, size);
}

static inline SHARED char *
shstrdup(SHARED const char *src)
{
    size_t len = shstrlen(src) + 1;
    SHARED char *copy = shalloc(len);

    if (copy != NULL)
        shmemcpy(copy, src, len);
    return copy;
}

static inline SHARED char *
shstrchr(SHARED const char *str, int delim)
{
    return (SHARED void *)strchr((const char *)str, delim);
}

static inline int
shstrcmp(SHARED const char *str1, SHARED const char *str2)
{
    return strcmp((const char *)str1, (const char *)str2);
}

static inline int
shstrncmp(SHARED const char *str1, SHARED const char *str2, size_t maxlen)
{
    return strncmp((const char *)str1, (const char *)str2, maxlen);
}


static inline int
shmemcmp(SHARED const void *str1, SHARED const void *str2, size_t len)
{
    return memcmp((const void *)str1, (const void *)str2, len);
}

static inline SHARED void *
shmemset(SHARED void *ptr, int c, size_t len)
{
    return memset((void *)ptr, c, len);
}



#endif
