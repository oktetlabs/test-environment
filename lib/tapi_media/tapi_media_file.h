/** @file
 * @brief Test API to media file routines
 *
 * @defgroup tapi_media_file Test API to operate the media files
 * @ingroup tapi_media
 * @{
 *
 * Functions for convenient work with the media files on local storage.
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 * @author Ivan Melnikov <Ivan.Melnikov@oktetlabs.ru>
 *
 */

#ifndef __TAPI_MEDIA_FILE_H__
#define __TAPI_MEDIA_FILE_H__

#include "te_errno.h"
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
static inline const char *
tapi_media_file_get_metadata_title(
                            const tapi_media_file_metadata *metadata)
{
    return metadata->title;
}

/**
 * Convert local file to media file and extract it metadata from /local/fs
 * configugator tree. Media file with it resources should be released with
 * @p tapi_media_file_release when it is no longer needed.
 *
 * @param[in]  local_file   Local file
 * @param[out] media_file   Media file.
 *
 * @return Status code.
 *
 * @sa tapi_media_file_release
 */
extern te_errno tapi_media_file_get_from_local(
                                        const tapi_local_file  *local_file,
                                        tapi_media_file       **media_file);

/**
 * Release media file that was got with @p tapi_media_file_get_from_local.
 *
 * @param media_file    Media file.
 *
 * @sa tapi_media_file_get_from_local
 */
extern void tapi_media_file_free(tapi_media_file *media_file);

/**@} <!-- END tapi_media_file --> */

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TAPI_MEDIA_FILE_H__ */
