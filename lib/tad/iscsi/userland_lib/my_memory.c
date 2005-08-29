#include <stdlib.h>
#include "my_memory.h"

void my_free(void **ptr)
{
    free(*ptr);

    *ptr = NULL;
}
