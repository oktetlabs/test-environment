/** @file
 * @brief Test API to storage share routines
 *
 * Functions for convenient work with storage share.
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

#ifndef __TAPI_STORAGE_SHARE_H__
#define __TAPI_STORAGE_SHARE_H__

#include "te_queue.h"


#ifdef __cplusplus
extern "C" {
#endif

/**
 * Shared directories list entry.
 */
typedef struct tapi_storage_share_le {
    char *storage;      /**< Storage device can be represented by name,
                             mount point, etc. depends on server
                             implementation. */
    char *path;         /**< Pathname of directory on the @p storage. */
    SLIST_ENTRY(tapi_storage_share_le) next;
} tapi_storage_share_le;

/**
 * Head of shared directories list.
 */
SLIST_HEAD(tapi_storage_share_list, tapi_storage_share_le);
typedef struct tapi_storage_share_list tapi_storage_share_list;


/**
 * Empty list of shared directories and free it entries.
 *
 * @param share         List of shared directories.
 */
extern void tapi_storage_share_list_free(tapi_storage_share_list *share);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TAPI_STORAGE_SHARE_H__ */
