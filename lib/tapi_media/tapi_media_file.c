/** @file
 * @brief Test API to media file routines
 *
 * Functions for convenient work with the media files on local storage.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 *
 */

#define TE_LGR_USER     "TAPI Media File"

#include "tapi_media_file.h"

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include "te_alloc.h"
#include "conf_api.h"
#include "tapi_local_fs.h"


/* See description in tapi_media_dlna.h. */
te_errno
tapi_media_file_get_from_local(const tapi_local_file  *local_file,
                               tapi_media_file       **media_file)
{
    tapi_media_file *media;
    te_errno         rc;

    media = TE_ALLOC(sizeof(*media));
    if (media == NULL)
        return TE_RC(TE_TAPI, TE_ENOMEM);
    media->file = *local_file;
    media->file.pathname = strdup(local_file->pathname);
    if (media->file.pathname == NULL)
    {
        tapi_media_file_free(media);
        return TE_RC(TE_TAPI, TE_ENOMEM);
    }
    rc = tapi_local_fs_get_file_metadata(media->file.pathname, "title",
                                         (char **)&media->metadata.title);
    if (rc == 0)
        *media_file = media;
    else
        tapi_media_file_free(media);
    return rc;
}

/* See description in tapi_media_file.h. */
void
tapi_media_file_free(tapi_media_file *media_file)
{
    if (media_file != NULL)
    {
        /* Discard const specifier by explicit type casting. */
        tapi_local_file_free_entry(&media_file->file);
        free((char *)media_file->metadata.title);
        free(media_file);
    }
}
