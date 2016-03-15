/** @file
 * @brief Generic test API to storage routines
 *
 * Generic high level functions to work with storage.
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

#ifndef __TAPI_STORAGE_H__
#define __TAPI_STORAGE_H__

#include "tapi_storage_client.h"


#ifdef __cplusplus
extern "C" {
#endif

/**
 * Clean up the storage from content.
 *
 * @param client        Storage client handle.
 *
 * @return Status code.
 */
extern te_errno tapi_storage_bootstrap(tapi_storage_client *client);

/**
 * Remove all existent content from remote storage and fill by another one.
 * To update content it uses lazy-flag @b STORAGE_UPLOAD_LAZY obtained from
 * configurator tree. If @b STORAGE_UPLOAD_LAZY is @c TRUE then it will be
 * applied only to dissimilar files of source and remote storages.
 *
 * @param client        Storage client handle.
 *
 * @return Status code.
 */
extern te_errno tapi_storage_setup(tapi_storage_client *client);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TAPI_STORAGE_H__ */
