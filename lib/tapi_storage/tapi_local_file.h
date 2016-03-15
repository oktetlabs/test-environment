/** @file
 * @brief Test API to local file routines
 *
 * Functions for convinient work with the files on the engine and TA.
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

#ifndef __TAPI_LOCAL_FILE_H__
#define __TAPI_LOCAL_FILE_H__


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

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TAPI_LOCAL_FILE_H__ */
