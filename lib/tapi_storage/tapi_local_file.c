/** @file
 * @brief Test API to local file routines
 *
 * Functions for convenient work with the files on the engine and TA.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 *
 */

#define TE_LGR_USER     "TAPI Local File"

#include "tapi_local_file.h"

#ifdef HAVE_ASSERT_H
#include <assert.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif


/* See description in tapi_local_file.h. */
const char *
tapi_local_file_get_name(const tapi_local_file *file)
{
    const char *basename;

    assert(file != NULL);
    if (file->pathname == NULL)
        return NULL;

    basename = strrchr(file->pathname, '/');
    return (basename != NULL ? basename + 1 : file->pathname);
}

/* See description in tapi_local_file.h. */
te_bool
tapi_local_file_cmp(const tapi_local_file *file1,
                    const tapi_local_file *file2)
{
    assert(file1 != NULL && file2 != NULL);
    return (file1->type == file2->type &&
            file1->property.size == file2->property.size &&
            ((file1->pathname != NULL && file2->pathname != NULL &&
              strcmp(file1->pathname, file2->pathname) == 0) ||
             (file1->pathname == NULL && file2->pathname == NULL)));
}

/* See description in tapi_local_file.h. */
void
tapi_local_file_free_entry(tapi_local_file *file)
{
    free((char *)file->pathname);
}

/* See description in tapi_local_file.h. */
void
tapi_local_file_free(tapi_local_file *file)
{
    if (file != NULL)
    {
        tapi_local_file_free_entry(file);
        free(file);
    }
}
