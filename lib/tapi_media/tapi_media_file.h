/** @file
 * @brief Test API to media file routines
 *
 * Functions for convinient work with the media files on local storage.
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

#ifndef __TAPI_MEDIA_FILE_H__
#define __TAPI_MEDIA_FILE_H__

#include "tapi_local_file.h"


#ifdef __cplusplus
extern "C" {
#endif

/**
 * Metadata of media file.
 */
typedef struct tapi_media_file_metadata {
    const char *title;  /**< Title of media file. */
} tapi_media_file_metadata;

/**
 * Media file object.
 */
typedef struct tapi_media_file {
    tapi_local_file          file;      /**< File description. */
    tapi_media_file_metadata metadata;  /**< Media file metadata. */
} tapi_media_file;


/**
 * Get title from file metadata.
 *
 * @param metadata      File metadata.
 *
 * @return Title of media file.
 */
extern const char *tapi_media_file_get_metadata_title(
                                const tapi_media_file_metadata *metadata);

/**
 * Convert local file to media file and extract it metadata from /local/fs
 * configugator tree. Media file with it resources should be released with
 * @b tapi_media_file_release when it is no longer needed.
 *
 * @param[in]  local_file   Local file
 * @param[out] media_file   Media file.
 *
 * @return Status code.
 *
 * @sa tapi_media_file_release
 */
extern te_errno tapi_media_file_get_from_local(
                                        const tapi_local_file *local_file,
                                        tapi_media_file       *media_file);

/**
 * Release media file that was got with @b tapi_media_file_get_from_local.
 *
 * @param media_file    Media file.
 *
 * @sa tapi_media_file_get_from_local
 */
extern void tapi_media_file_release(tapi_media_file *media_file);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TAPI_MEDIA_FILE_H__ */
