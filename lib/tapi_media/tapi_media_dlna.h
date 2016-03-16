/** @file
 * @brief Test API to DLNA media routines
 *
 * Functions for convinient work with the DLNA files on remote storage.
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

#ifndef __TAPI_MEDIA_DLNA_H__
#define __TAPI_MEDIA_DLNA_H__

#include "tapi_media_file.h"
#include "tapi_upnp_content_directory.h"


#ifdef __cplusplus
extern "C" {
#endif


/**
 * Compare DLNA file with media local file.
 *
 * @param dlna_file     Remote DLNA file.
 * @param local_file    Local media file.
 *
 * @return Result of comparison: @c TRUE if both DLNA and media local files
 *         are equal.
 */
extern te_bool tapi_media_dlna_cmp_with_local(
                                const tapi_upnp_cd_container *dlna_file,
                                const tapi_media_file        *local_file);


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TAPI_MEDIA_DLNA_H__ */
