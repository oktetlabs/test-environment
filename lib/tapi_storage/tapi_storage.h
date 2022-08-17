/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Generic test API to storage routines
 *
 * @defgroup tapi_storage_wrapper Test API to perform the generic operations over the storage
 * @ingroup tapi_storage
 * @{
 *
 * Generic high level functions to work with storage.
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
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
 * @param root          Root directory to bootstrap. May be @c NULL to use
 *                      default '/'.
 * @param remove_root   Enable delete a root directory as well (if @c TRUE).
 *
 * @return Status code.
 */
extern te_errno tapi_storage_bootstrap(tapi_storage_client *client,
                                       const char          *root,
                                       te_bool              remove_root);

/**
 * Remove all existent content from remote storage and fill by another one.
 * To update content it uses lazy-flag @b STORAGE_UPLOAD_LAZY obtained from
 * configurator tree. If @b STORAGE_UPLOAD_LAZY is @c TRUE then it will be
 * applied only to dissimilar files of source and remote storages.
 *
 * @param client        Storage client handle.
 * @param root          Root directory to setup. Will be created if needed.
 *                      May be @c NULL to use default '/'.
 *
 * @return Status code.
 */
extern te_errno tapi_storage_setup(tapi_storage_client *client,
                                   const char          *root);

/**@} <!-- END tapi_storage_wrapper --> */

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TAPI_STORAGE_H__ */
