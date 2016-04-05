/** @file
 * @brief Test API to media file routines
 *
 * Functions for convenient work with the media files on local storage.
 *
 *
 * Copyright (C) 2016 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 *
 * @author Ivan Melnikov <Ivan.Melnikov@oktetlabs.ru>
 *
 * $Id$
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
                                         &media->metadata.title);
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
        free((char *)media_file->file.pathname);
        free((char *)media_file->metadata.title);
        free(media_file);
    }
}
