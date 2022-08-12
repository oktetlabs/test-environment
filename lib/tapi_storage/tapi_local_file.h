/** @file
 * @brief Test API to local file routines
 *
 * Functions for convenient work with the files on the engine and TA.
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

#ifndef __TAPI_LOCAL_FILE_H__
#define __TAPI_LOCAL_FILE_H__

#include "te_config.h"

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#include "te_defs.h"
#include "te_stdint.h"


#ifdef __cplusplus
extern "C" {
#endif

/**
 * File types.
 */
typedef enum tapi_local_file_type {
    TAPI_FILE_TYPE_FILE,        /**< Regular file. */
    TAPI_FILE_TYPE_DIRECTORY,   /**< Directory. */
} tapi_local_file_type;

/**
 * File's property.
 */
typedef struct tapi_local_file_property {
    uint64_t        size;       /**< Size of file in bytes. */
    struct timeval  date;       /**< Date of last file modification. */
} tapi_local_file_property;

/**
 * A file representation.
 */
typedef struct tapi_local_file {
    tapi_local_file_type      type;         /**< Type of file. */
    const char               *pathname;     /**< File pathname. */
    tapi_local_file_property  property;     /**< File's property. */
} tapi_local_file;


/**
 * Check if @p file is a regular file or directory
 *
 * @param file      File.
 *
 * @return @c TRUE if @p file is a regular file.
 */
static inline te_bool
tapi_local_file_is_file(const tapi_local_file *file)
{
    return (file->type == TAPI_FILE_TYPE_FILE);
}

/**
 * Check if @p file is a regular file or directory
 *
 * @param file      File.
 *
 * @return @c TRUE if @p file is a directory.
 */
static inline te_bool
tapi_local_file_is_dir(const tapi_local_file *file)
{
    return (file->type == TAPI_FILE_TYPE_DIRECTORY);
}

/**
 * Get pathname of a file.
 *
 * @param file      File.
 *
 * @return File pathname.
 */
static inline const char *
tapi_local_file_get_pathname(const tapi_local_file *file)
{
    return file->pathname;
}


/**
 * Get file name. Extract name of file from @b pathname parameter.
 *
 * @param file      File.
 *
 * @return File name on success, or @c NULL on error.
 */
extern const char *tapi_local_file_get_name(const tapi_local_file *file);

/**
 * Compare two files by @b type, @b name and @b size parameters.
 *
 * @param file1     First file to compare.
 * @param file2     Second file to compare.
 *
 * @return @c TRUE if files are equal.
 */
extern te_bool tapi_local_file_cmp(const tapi_local_file *file1,
                                   const tapi_local_file *file2);

/**
 * Free an entry of local file.
 *
 * @param file      File.
 */
extern void tapi_local_file_free_entry(tapi_local_file *file);

/**
 * Free a local file.
 *
 * @param file      File.
 */
extern void tapi_local_file_free(tapi_local_file *file);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TAPI_LOCAL_FILE_H__ */
