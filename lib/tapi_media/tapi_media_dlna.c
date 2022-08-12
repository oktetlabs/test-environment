/** @file
 * @brief Test API to DLNA media routines
 *
 * Functions for convenient work with the DLNA files on remote storage.
 *
 *
 * Copyright (C) 2004-2018 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 * @author Ivan Melnikov <Ivan.Melnikov@oktetlabs.ru>
 *
 */

#define TE_LGR_USER     "TAPI Media DLNA"

#include "tapi_media_dlna.h"

#include "te_string.h"


/**
 * Build pathname of dlna file recursively.
 *
 * @param[in]    dlna_file      Remote DLNA file.
 * @param[in,out] pathname      Pathname which is recursively built.
 */
static void
build_dlna_pathname(const tapi_upnp_cd_container_node *dlna_file,
                    te_string                         *pathname)
{
    if (dlna_file->parent == NULL)
        return;     /* Root title should not be a part of pathname. */

    build_dlna_pathname(dlna_file->parent, pathname);
    te_string_append(pathname, "/%s", dlna_file->data.base.title);
}

/**
 * Build pathname of dlna file. Note, returned value should be freed with
 * @b free(3) when it is no longer needed.
 *
 * @param dlna_file     Remote DLNA file.
 *
 * @return Pathname of dlna file, or @c NULL on fail.
 */
static char *
get_dlna_pathname(const tapi_upnp_cd_container_node *dlna_file)
{
    te_string pathname = TE_STRING_INIT;

    build_dlna_pathname(dlna_file, &pathname);

    return pathname.ptr;
}

/**
 * Convert DLNA file to media local file. Note, returned value should be
 * released with @b release_media_file when it is no longer needed.
 *
 * @param dlna_file     Remote DLNA file.
 *
 * @return Media local file.
 *
 * @sa release_media_file
 */
static tapi_media_file *
dlna_file_to_local(const tapi_upnp_cd_container_node *dlna_file)
{
    static tapi_media_file      media_file;
    tapi_upnp_cd_resource_node *res;

    /* Type. */
    media_file.file.type = (dlna_file->data.type == TAPI_UPNP_CD_OBJECT_ITEM
                            ? TAPI_FILE_TYPE_FILE
                            : TAPI_FILE_TYPE_DIRECTORY);

    /* Size and date. */
    SLIST_FOREACH(res, &dlna_file->data.base.resources, next)
    {
        if (res->res.size != -1)
        {
            /*
             * Assume that only original file has a size.
             * For example, image has a scale images which is generated with
             * DLNA server from original one with different scale.
             */
            media_file.file.property.size = res->res.size;
            break;
        }
    }

    /* FIXME: DLNA date format YYYY-MM-DD. Does not support yet. */
    media_file.file.property.date = (struct timeval){ 0, 0 };

    /* Title. */
    media_file.metadata.title = dlna_file->data.base.title;

    /* Pathname. */
    media_file.file.pathname = get_dlna_pathname(dlna_file);

    return &media_file;
}

/**
 * Release media file which was got with @b dlna_file_to_local.
 *
 * @param media_file    Media file.
 *
 * @sa dlna_file_to_local
 */
static void
release_media_file(tapi_media_file *media_file)
{
    /* cast is required to remove const */
    free((char *)media_file->file.pathname);
}


/* See description in tapi_media_dlna.h. */
te_bool
tapi_media_dlna_cmp_with_local(const tapi_upnp_cd_container_node *dlna_file,
                               const tapi_media_file *local_file)
{
    const tapi_media_file *l = local_file;
    tapi_media_file       *r = dlna_file_to_local(dlna_file);
    te_bool                result;
    size_t                 l_dirname_len;
    size_t                 r_dirname_len;

    VERB("Compare media with dlna:\n"
         "media: \"%s\", \"%s\", %llu\n"
         "dlna : \"%s\", \"%s\", %llu",
         l->metadata.title, l->file.pathname, l->file.property.size,
         r->metadata.title, r->file.pathname, r->file.property.size);

    l_dirname_len = strrchr(l->file.pathname, '/') - l->file.pathname;
    r_dirname_len = strrchr(r->file.pathname, '/') - r->file.pathname;

    result = (
        /* Compare type */
        l->file.type == r->file.type &&
        /* Compare size */
        l->file.property.size == r->file.property.size &&
        /* Compare title */
        ((l->metadata.title == NULL && r->metadata.title == NULL) ||
         (l->metadata.title != NULL && r->metadata.title != NULL &&
          strcmp(l->metadata.title, r->metadata.title) == 0)) &&
        /* Compare path (i.e. only dirname) */
        l_dirname_len == r_dirname_len &&
        strncmp(l->file.pathname, r->file.pathname, r_dirname_len) == 0
    );

    release_media_file(r);

    return result;
}
