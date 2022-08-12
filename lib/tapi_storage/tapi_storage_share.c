/** @file
 * @brief Test API to storage share routines
 *
 * Functions for convenient work with storage share.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 */

#define TE_LGR_USER     "TAPI Storage Share"

#include "te_defs.h"
#include "tapi_storage_share.h"


/* See description in tapi_storage_share.h. */
void
tapi_storage_share_list_free(tapi_storage_share_list *share)
{
    tapi_storage_share_le *s;

    while (!SLIST_EMPTY(share))
    {
        s = SLIST_FIRST(share);
        SLIST_REMOVE_HEAD(share, next);
        free(s->storage);
        free(s->path);
        free(s);
    }
}
