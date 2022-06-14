/** @file
 * @brief API to deal with files
 *
 * @defgroup te_tools_te_file File operations
 * @ingroup te_tools
 * @{
 *
 * Functions to operate the files.
 *
 * Copyright (C) 2018 OKTET Labs. All rights reserved.
 *
 * @author Ivan Melnikov <Ivan.Melnikov@oktetlabs.ru>
 */

#ifndef __TE_FILE_H__
#define __TE_FILE_H__

#ifdef HAVE_STDIO_H
#include "stdio.h"
#endif
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Get a basename from @p pathname, and check if it is valid. Unlike system
 * basename() it does not modify the contents of @p pathname
 *
 * @param pathname      File pathname
 *
 * @return A heap-allocated string containing an extracted file name,
 *         or @c NULL if @p pathname is invalid, or allocation fails
 */
extern char *te_basename(const char *pathname);

/**
 * Get a resolved link using @p path.
 *
 * @param path_fmt      Path format string.
 * @param ...           Format string arguments.
 *
 * @return A heap-allocated string containing a resolved link,
 *         or @c NULL if an error has occurred.
 */
extern char *te_readlink_fmt(const char *path_fmt, ...)
                            __attribute__((format(printf, 1, 2)));

/**
 * Get a dirname from @p pathname, and check if it is valid. Unlike system
 * dirname() it does not modify the contents of @p pathname
 *
 * @param pathname      File pathname
 *
 * @return A heap-allocated string containing an extracted directory name,
 *         or @c NULL if @p pathname is invalid, or allocation fails
 */
extern char *te_dirname(const char *pathname);

/**
 * Create a file of unique name. It builds a template of file name in mkstemps
 * compatible format which has the form "prefixXXXXXXsuffix",
 * or "prefixXXXXXX" if @p suffix is @c NULL
 *
 * @param[out] filename         Name of created file. On success it should be
 *                              freed with free(3) when it is no longer needed
 * @param[in]  prefix_format    Format string of filename prefix
 * @param[in]  suffix           Filename suffix, can be @c NULL
 * @param[in]  ap               List of arguments
 *
 * @return File descriptor of the created file, or @c -1 in case of error
 *
 * @sa te_file_create_unique_fd, te_file_create_unique
 */
extern int te_file_create_unique_fd_va(char **filename,
                                       const char *prefix_format,
                                       const char *suffix,
                                       va_list ap);

/**
 * Create a file of unique name, see details in description of
 * te_file_create_unique_fd_va()
 *
 * @param[out] filename         Name of created file. On success it should be
 *                              freed with free(3) when it is no longer needed
 * @param[in]  prefix_format    Format string of filename prefix
 * @param[in]  suffix           Filename suffix, can be @c NULL
 * @param[in]  ...              Format string arguments
 *
 * @return File descriptor of the created file, or @c -1 in case of error
 *
 * @sa te_file_create_unique_fd_va, te_file_create_unique
 */
extern int te_file_create_unique_fd(char **filename, const char *prefix_format,
                                    const char *suffix, ...)
                                        __attribute__((format(printf, 2, 4)));

/**
 * Create a file of unique name, see details in description of
 * te_file_create_unique_fd_va()
 *
 * @note Return value should be freed with free(3) when it is no longer needed
 *
 * @param prefix_format     Format string of filename prefix
 * @param suffix            Filename suffix, can be @c NULL
 * @param ...               Format string arguments
 *
 * @return Name of created file, or @c NULL in case of error
 *
 * @sa te_file_create_unique_fd_va, te_file_create_unique_fd
 */
extern char *te_file_create_unique(const char *prefix_format,
                                   const char *suffix, ...)
                                        __attribute__((format(printf, 1, 3)));

/**
 * Call fopen() with a path specified by a format string and arguments.
 *
 * @param mode      Opening mode string.
 * @param path_fmt  File path format string.
 * @param ...       Format string arguments.
 *
 * @return FILE pointer on success, @c NULL on failure.
 */
extern FILE *te_fopen_fmt(const char *mode, const char *path_fmt, ...)
                                      __attribute__((format(printf, 2, 3)));

/**
 * Read process identifier from PID file
 *
 * @param pid_path Path to PID file
 *
 * @return Process ID or @c -1 in case of error
 */
extern pid_t te_file_read_pid(const char *pid_path);

/**
 * Check that the file is executable
 *
 * @note If @p path does't contains '/' then @p path is concatenated with
 *       the environment variable PATH. Otherwise, we are looking for a
 *       file from the current location
 *
 * @param path Path to the file
 *
 * @return Status code
 */
extern te_errno te_file_check_executable(const char *path);

/**
 * Read the contents of the file @p path into @p buffer.
 *
 * The function ensures that the contents of the file is no
 * larger than @p bufsize minus one and that the contents contain
 * no embedded zeroes. The resulting string will be zero-terminated.
 * If there are trailing newlines, they are stripped off.
 *
 * @note The file should be random-access, so the function cannot read
 * from sockets, named FIFOs and most kinds of character devices.
 *
 * @param[in]  path     Pathname to read
 * @param[out] buffer   Destination buffer
 * @param[in]  bufsize  The size of @p buffer including terminating zero.
 *
 * @return Status code
 * @retval TE_EFBIG     The file size is larger than @p bufsize
 * @retval TE_ESPIPE    The file is not random-access
 * @retval TE_EILSEQ    The file contains embedded zeroes
 */
extern te_errno te_file_read_text(const char *path, char *buffer,
                                  size_t bufsize);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_FILE_H__ */
/**@} <!-- END te_tools_te_file --> */
