/** @file
 * @brief Test API to storage share routines
 *
 * Functions for convenient work with storage share.
 *
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 * @author Ivan Melnikov <Ivan.Melnikov@oktetlabs.ru>
 *
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
