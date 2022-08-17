/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Test API to local file system routines
 *
 * Functions for convenient work with the file system mapped on configurator
 * tree.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TAPI_LOCAL_FS_H__
#define __TAPI_LOCAL_FS_H__

#include "te_errno.h"
#include "te_queue.h"
#include "tapi_local_file.h"


#ifdef __cplusplus
extern "C" {
#endif


/**
 * Local file system root pathname representation in Configurator
 */
#define TAPI_LOCAL_FS_ROOT  ""


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
 *                  @p tapi_local_file structure.
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
 *                  @p tapi_local_file structure.
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
 *                  @p tapi_local_file structure.
 *
 * @sa tapi_local_file_le, tapi_local_file
 */
#define TAPI_LOCAL_FS_FOREACH_DIR(var_, head_, dir_) \
    for ((var_) = SLIST_FIRST((head_));             \
         (var_) && ((dir_) = &(var_)->file, 1);     \
         (var_) = SLIST_NEXT((var_), next))         \
        if (tapi_local_file_is_dir(dir_))

/**
 * Recursive loop 'for' to iterate through all local file system files.
 *
 * @param pathname_     Pathname to start traverse from.
 * @param cb_func_      Callback function which is called on each element of
 *                      local file system before traverse the next elements.
 * @param user_data_    Additional data to post to @p cb_func_ function,
 *                      or @c NULL.
 *
 * @sa tapi_local_fs_traverse, tapi_local_fs_traverse_cb
 */
#define TAPI_LOCAL_FS_FOREACH_RECURSIVE(pathname_, cb_func_, user_data_) \
    tapi_local_fs_traverse(pathname_, cb_func_, NULL, user_data_)


/**
 * Prototype of handler for using in @p tapi_local_fs_traverse.
 *
 * @param file         Local file which is represented by @p tapi_local_file
 *                     structure.
 * @param user_data    Opaque user data.
 *
 * @return Status code.
 *
 * @sa tapi_local_fs_traverse
 */
typedef te_errno (*tapi_local_fs_traverse_cb)(tapi_local_file *file,
                                              void            *user_data);

/**
 * Traverse the local file system. It calls @p tapi_local_fs_ls to obtain
 * subitems of @p files to recursive traverse of files tree and then
 * @p tapi_local_fs_list_free to free obtained items. This function is not
 * suitable to change the file system tree structure but only to manipulate
 * on it elements.
 *
 * @param pathname      Pathname to start traverse from.
 * @param cb_pre        Callback function which is called on each element of
 *                      local file system before traverse the next elements,
 *                      or @c NULL if it is @p cb_post only needed.
 * @param cb_post       Callback function which is called on each element of
 *                      local file system after traverse the next elements,
 *                      or @c NULL if it is @p cb_pre only needed.
 * @param user_data     Additional data to post to @p cb_pre and
 *                      @p cb_post handlers, or @c NULL.
 *
 * @return Status code.
 *
 * @sa tapi_local_fs_ls, tapi_local_fs_list_free
 */
extern te_errno tapi_local_fs_traverse(
                                    const char                *pathname,
                                    tapi_local_fs_traverse_cb  cb_pre,
                                    tapi_local_fs_traverse_cb  cb_post,
                                    void                      *user_data);

/**
 * Get root files list from /local/fs configurator tree.
 * It calls @p tapi_local_fs_ls to get the files. @p files should be
 * released with @p tapi_local_fs_list_free when they are no longer needed.
 *
 * @param[out] files        Files list.
 *
 * @return Status code
 *
 * @sa tapi_local_fs_ls, tapi_local_fs_list_free
 */
static inline te_errno
tapi_local_fs_ls_root(tapi_local_file_list *files)
{
    te_errno tapi_local_fs_ls(const char           *pathname,
                              tapi_local_file_list *files);

    return tapi_local_fs_ls(TAPI_LOCAL_FS_ROOT, files);
}


/**
 * Get files list from /local/fs configurator tree. @p files should be
 * released with @p tapi_local_fs_list_free when they are no longer needed.
 *
 * @param[in]  pathname     Pathname of file list.
 * @param[out] files        Files list.
 *
 * @return Status code
 *
 * @sa tapi_local_fs_list_free
 */
extern te_errno tapi_local_fs_ls(const char           *pathname,
                                 tapi_local_file_list *files);

/**
 * Free files list that was obtained with @p tapi_local_fs_ls.
 *
 * @param files     Files list.
 *
 * @sa tapi_local_fs_ls
 */
extern void tapi_local_fs_list_free(tapi_local_file_list *files);

/**
 * Get string representation of metadata of local file from configurator.
 * @p metadata should be freed with free when it is no longer needed.
 *
 * @param[in]  pathname     Pathname of file which metadata should be get.
 * @param[in]  metaname     Metadata name.
 * @param[out] metadata     Metadata value.
 *
 * @return Status code.
 */
extern te_errno tapi_local_fs_get_file_metadata(const char  *pathname,
                                                const char  *metaname,
                                                char       **metadata);

/**
 * Get real pathname of local file i.e. pathname mapped on real file system.
 * Returned value should be freed with @b free when it is no longer needed.
 *
 * @param file          Local file.
 * @param mapping_pfx   Prefix for @p file->pathname to map it on real file
 *                      system, i.e. it is real root directory for local
 *                      file. If @c NULL then configurator parameter
 *                      env:STORAGE_CONTENT_DIR value will be used instead
 *                      of @p mapping_pfx.
 *
 * @return Real pathname, or @c NULL in case of error.
 */
extern char *tapi_local_fs_get_file_real_pathname(
                                        const tapi_local_file *file,
                                        const char            *mapping_pfx);

/**
 * Print a local file system files info using RING function starting from
 * the @p pathname. This function should be used for debugging purpose.
 *
 * @param pathname      Path to start traverse from.
 */
extern void tapi_local_fs_ls_print(const char *pathname);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TAPI_LOCAL_FS_H__ */
