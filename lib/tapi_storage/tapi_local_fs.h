/** @file
 * @brief Test API to local file system routines
 *
 * Functions for convinient work with the file system mapped on configurator
 * tree.
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

#ifndef __TAPI_LOCAL_FS_H__
#define __TAPI_LOCAL_FS_H__

#include "tapi_local_file.h"

#ifdef __cplusplus
extern "C" {
#endif


/**
 * Files list entry.
 */
typedef struct tapi_local_file_le {
    tapi_local_file file;
    SLIST_ENTRY(tapi_local_file_le) next;
} tapi_local_file_le;

/**
 * Head of files list.
 */
SLIST_HEAD(tapi_local_file_list, tapi_local_file_le);
typedef struct tapi_local_file_list tapi_local_file_list;


/**
 * Loop 'for' to iterate through all file list entries.
 *
 * @param var_      Pointer to the files list entry.
 * @param head_     Pointer to the head of the list.
 * @param file_     Pointer to the file which is represented by
 *                  @b tapi_local_file structure.
 *
 * @sa tapi_local_file_le, tapi_local_file
 */
#define TAPI_LOCAL_FS_FOREACH(var_, head_, file_) \
    for ((var_) = SLIST_FIRST((head_));             \
         (var_) && ((file_) = &(var_)->file, 1);    \
         (var_) = SLIST_NEXT((var_), next))

/**
 * Loop 'for' to iterate through all file list entries of regular file type.
 *
 * @param var_      Pointer to the files list entry.
 * @param head_     Pointer to the head of the list.
 * @param file_     Pointer to the file which is represented by
 *                  @b tapi_local_file structure.
 *
 * @sa tapi_local_file_le, tapi_local_file
 */
#define TAPI_LOCAL_FS_FOREACH_FILE(var_, head_, file_) \
    for ((var_) = SLIST_FIRST((head_));             \
         (var_) && ((file_) = &(var_)->file, 1);    \
         (var_) = SLIST_NEXT((var_), next))         \
        if (tapi_local_file_is_file(file_))

/**
 * Loop 'for' to iterate through all file list entries of directory type.
 *
 * @param var_      Pointer to the files list entry.
 * @param head_     Pointer to the head of the list.
 * @param dir_      Pointer to the directory which is represented by
 *                  @b tapi_local_file structure.
 *
 * @sa tapi_local_file_le, tapi_local_file
 */
#define TAPI_LOCAL_FS_FOREACH_DIR(var_, head_, dir_) \
    for ((var_) = SLIST_FIRST((head_));             \
         (var_) && ((dir_) = &(var_)->file, 1);     \
         (var_) = SLIST_NEXT((var_), next))         \
        if (tapi_local_file_is_dir(dir_))

/**
 * Recursive loop 'for' to iterate through all file list entries.
 *
 * @param var_      Pointer to the files list entry.
 * @param head_     Pointer to the head of the list.
 * @param file_     Pointer to the file which is represented by
 *                  @b tapi_local_file structure.
 *
 * @sa tapi_local_file_le, tapi_local_file
 *
 * @todo
 */
#define TAPI_LOCAL_FS_FOREACH_RECURSIVE(var_, head_, file_)

/**
 * Recursive loop 'for' to iterate through all file list entries of regular
 * file type.
 *
 * @param var_      Pointer to the files list entry.
 * @param head_     Pointer to the head of the list.
 * @param file_     Pointer to the file which is represented by
 *                  @b tapi_local_file structure.
 *
 * @sa tapi_local_file_le, tapi_local_file
 *
 * @todo
 */
#define TAPI_LOCAL_FS_FOREACH_FILE_RECURSIVE(var_, head_, file_)

/**
 * Recursive loop 'for' to iterate through all file list entries of
 * directory type.
 *
 * @param var_      Pointer to the files list entry.
 * @param head_     Pointer to the head of the list.
 * @param dir_      Pointer to the directory which is represented by
 *                  @b tapi_local_file structure.
 *
 * @sa tapi_local_file_le, tapi_local_file
 *
 * @todo
 */
#define TAPI_LOCAL_FS_FOREACH_DIR_RECURSIVE(var_, head_, dir_)


/**
 * Get files list from /local/fs configurator tree. Files should be released
 * with @sa tapi_local_fs_list_free when they are no longer needed.
 *
 * @param[in]  pathname     Pathname of file list.
 * @param[out] files        Files list.
 *
 * @return Status code
 */
extern te_errno tapi_local_fs_ls(const char           *pathname,
                                 tapi_local_file_list *files);

/**
 * Free files list that was obtained with @sa tapi_local_fs_ls.
 *
 * @param files     Files list.
 */
extern void tapi_local_fs_list_free(tapi_local_file_list *files);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TAPI_LOCAL_FS_H__ */
