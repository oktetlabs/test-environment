/** @file
 * @brief Test API to DLNA media routines
 *
 * @defgroup tapi_media_dlna Test API to operate the DLNA media files
 * @ingroup tapi_media
 * @{
 *
 * Functions for convenient work with the DLNA files on remote storage.
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 * @author Ivan Melnikov <Ivan.Melnikov@oktetlabs.ru>
 *
 */

#ifndef __TAPI_MEDIA_DLNA_H__
#define __TAPI_MEDIA_DLNA_H__

#include "tapi_media_file.h"
#include "tapi_upnp_content_directory.h"


#ifdef __cplusplus
extern "C" {
#endif


/**
 * Compare DLNA file with media local file by type, title, dirname and size.
 *
 * @param dlna_file     Remote DLNA file.
 * @param local_file    Local media file.
 *
 * @return Result of comparison: @c TRUE if both DLNA and media local files
 *         are equal.
 */
extern te_bool tapi_media_dlna_cmp_with_local(
                            const tapi_upnp_cd_container_node *dlna_file,
                            const tapi_media_file             *local_file);

/**@} <!-- END tapi_media_dlna --> */

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TAPI_MEDIA_DLNA_H__ */
