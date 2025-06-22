/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief API to deal with files
 *
 * @defgroup te_tools_te_file File operations
 * @ingroup te_tools
 * @{
 *
 * Functions to operate the files.
 *
 * Copyright (C) 2018-2023 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_FILE_H__
#define __TE_FILE_H__

#ifdef HAVE_STDIO_H
#include "stdio.h"
#endif
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#include "te_vector.h"
#include "te_string.h"

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
                            TE_LIKE_PRINTF(1, 2);

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
 * Construct a filename from components.
 *
 * Unlike te_file_resolve_pathname(), the function does a purely
 * syntactic operation, so no checks are made that a pathname exists
 * or is accessible. Therefore it is suitable for constructing filenames
 * on one host that will be used on another.
 *
 * The constructing algorithm is as follows:
 * - if @p path is @c NULL, it is treated as an empty string;
 * - if @p dirname is @c NULL or @p path is an absolute path,
 *   @p path is used as is;
 * - otherwise, @p dirname and @p path are joined using @c / separator;
 * - then if @p suffix is not @c NULL, it is appended to the
 *   filename obtained at the previous step; if that name ends with
 *   a @c /, it is first stripped; i.e. a suffix never creates a new
 *   pathname component.
 *
 * If @p dest is @c NULL, a fresh string is allocated and returned.
 *
 * @param dest    the string to hold the name or @c NULL.
 * @param dirname directory name
 * @param path    pathname (relative or absolute)
 * @param suffix  suffix
 *
 * @note The name is appended to the string contents.
 *
 * @warning If @p dest is not empty upon entry and @p dirname is empty,
 *          an extra slash may be added before the resulting pathname.
 *
 * @return the pointer to the contents of @p dest or a heap-allocated buffer
 */
extern char *te_file_join_filename(te_string *dest, const char *dirname,
                                   const char *path, const char *suffix);

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
                                        TE_LIKE_PRINTF(2, 4);

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
                                        TE_LIKE_PRINTF(1, 3);

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
                                      TE_LIKE_PRINTF(2, 3);

/**
 * Read process identifier from PID file
 *
 * @param pid_path Path to PID file
 *
 * @return Process ID or @c -1 in case of error
 */
extern pid_t te_file_read_pid(const char *pid_path);

/**
 * Search a relative filename in a vector of directories.
 *
 * If the filename is absolute (i.e. starts with @c /), it is checked
 * and no search is performed.
 *
 * @param[in]  filename    Filename to check
 * @param[in]  pathvec     Vector of directories
 * @param[in]  mode        Access mode to check (as used by system access())
 * @param[out] resolved    A location for a resolved pathname
 *                         (which must be free()'d. If @c NULL,
 *                         the function just checks the presence
 *                         of @p filename somewhere in the @p path.
 * @return Status code (possible values match those reported by access())
 */
extern te_errno te_file_resolve_pathname_vec(const char *filename,
                                             const te_vec *pathvec,
                                             int mode,
                                             char **resolved);

/**
 * Search a relative filename in a colon-separated list of directories.
 *
 * The function is like te_file_resolve_pathname_vec(), only it takes
 * a colon-separated string for a path instead of a prepared vector.
 *
 * @param[in]  filename    Filename to check
 * @param[in]  path        Colon-separated list of directories
 * @param[in]  mode        Access mode to check (as used by system access())
 * @param[in]  basename    If not @c NULL, @p filename is first
 *                         looked up in a directory part of @p basename
 *                         (i.e. if @p basename points to a directory,
 *                         it is used, otherwise the result of dirname() is
 *                         used).
 * @param[out] resolved    A location for a resolved pathname
 *                         (which must be free()'d. If @c NULL,
 *                         the function just checks the presence
 *                         of @p filename somewhere in the @p path.
 * @return Status code (possible values match those reported by access())
 */
extern te_errno te_file_resolve_pathname(const char *filename,
                                         const char *path,
                                         int mode,
                                         const char *basename,
                                         char **resolved);

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
 * Check that a filename is accessible for a given @p mode.
 *
 * The filename is constructed from the format string and arguments.
 *
 * @param mode   access mode (see system access())
 * @param fmt    format string
 * @param ...    arguments
 *
 * @return status code
 * @retval TE_EACCESS  Access to the file is denied
 * @retval TE_ENOENT   The file or any of its parents does not exist
 */
extern te_errno te_access_fmt(int mode, const char *fmt, ...)
    TE_LIKE_PRINTF(2, 3);

/**
 * Delete a file.
 *
 * The filename is constructed from the format string and arguments.
 *
 * @param fmt    format string
 * @param ...    arguments
 *
 * @return status code
 * @retval TE_ENOENT   The file or any of its parents does not exist.
 */
extern te_errno te_unlink_fmt(const char *fmt, ...)
    TE_LIKE_PRINTF(1, 2);

/**
 * Read the contents of the file into a dynamic string @p dest.
 *
 * If @p binary is @c false, the function ensures that there is no
 * embedded zeroes in the file content and strips off trailing newlines
 * if there are any.
 *
 * If @p maxsize is not zero, the function ensures that the file size
 * is not greater than it. That allows to avoid crashes if @p dest
 * is a statically allocated TE string and in general to prevent memory
 * bombs if a file comes from an untrusted source.
 *
 * @note The function should be used with caution on non-regular files
 *       such as named FIFOs or character devices, as it may block
 *       indefinitely or return partial data. Using it with regular files
 *       on special filesystems such as @c /proc is completely reliable.
 *
 * @param dest     TE string (the content of the file will be appended to it)
 * @param binary   reading mode (binary or text mode)
 * @param maxsize  the maximum allowed file size, if not zero
 * @param path_fmt pathname format string
 * @param ...      arguments
 *
 * @return status code
 * @retval TE_EFBIG     The file size is larger than @p maxsize.
 * @retval TE_EILSEQ    The file contains embedded zeroes and
 *                      @p binary is @c false.
 */
extern te_errno te_file_read_string(te_string *dest, bool binary,
                                    size_t maxsize, const char *path_fmt, ...)
                                    TE_LIKE_PRINTF(4, 5);

/**
 * Write the contents of the file from a dynamic string @p src.
 *
 * The file is opened with POSIX @p flags ORed with @c O_WRONLY
 * and if the file is created, then access @p mode will be set on it.
 *
 * If @p fitlen is not zero, the resulting file will be exactly that long:
 * - if @p src is longer than @p fitlen, it will be truncated;
 * - if @p src is shorter, it will be repeated until the length is reached.
 *
 * If @p flags contains @p O_APPEND and @p fitlen is not zero, it defines
 * the length of the current segment being appended, not that of the whole
 * file.
 *
 * @param src      source TE string
 * @param fitlen   desired file length (ignored if zero)
 * @param flags    POSIX open flags, such as @c O_CREAT
 * @param mode     POSIX access mode for newly created files
 * @param path_fmt pathname format string
 * @param ...      arguments
 *
 * @return status code
 */
extern te_errno te_file_write_string(const te_string *src, size_t fitlen,
                                     int flags, mode_t mode,
                                     const char *path_fmt, ...)
                                     TE_LIKE_PRINTF(5, 6);

/**
 * Same as te_file_write_string(), but the pathname is constructed from
 * a variadc list argument.
 *
 * @param src      source TE string
 * @param fitlen   desired file length (ignored if zero)
 * @param flags    POSIX open flags, such as @c O_CREAT
 * @param mode     POSIX access mode for newly created files
 * @param path_fmt pathname format string
 * @param args     variadic list
 *
 * @return status code
 */
extern te_errno te_file_write_string_va(const te_string *src, size_t fitlen,
                                        int flags, mode_t mode,
                                        const char *path_fmt, va_list args)
                                        TE_LIKE_VPRINTF(5);

/**
 * Generate the contents of the file from a pattern as in te_make_spec_buf().
 *
 * @note If @p spec includes a terminating zero byte, it *will* be present
 *       in the file.
 *
 * @note Currently the content is generated in-memory and then
 *       written to disk, so the function may not work very well
 *       with large file sizes.
 *
 * @param[in]  minlen      minimum length of the generated content
 * @param[in]  maxlen      maximum length of the generated content
 * @param[out] p_len       actual size of the generared content
 *                         (may be @c NULL)
 * @param[in]  spec        pattern for the generated contents
 *                         (see te_compile_buf_pattern() for
 *                         the syntax description)
 * @param[in]  path_fmt    pathname format string
 * @param ...              arguments
 *
 * @return Status code.
 */
extern te_errno te_file_write_spec_buf(size_t minlen, size_t maxlen,
                                       size_t *p_len,
                                       const char *spec,
                                       int flags, mode_t mode,
                                       const char *path_fmt, ...)
                                       TE_LIKE_PRINTF(7, 8);

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
 * @retval TE_EILSEQ    The file contains embedded zeroes
 *
 * @deprecated Prefer te_file_read_string().
 */
extern te_errno te_file_read_text(const char *path, char *buffer,
                                  size_t bufsize);

/**
 * Function type for callbacks for te_file_scandir().
 *
 * @param pattern   pattern used to filter pathnames
 * @param pathname  full pathname of a current file
 * @param data      user data as passed to te_file_scandir().
 *
 * @return status code
 * @retval TE_EOK   The scanning would stop, but @c 0 would be returned
 *                  by te_file_scandir().
 */
typedef te_errno te_file_scandir_callback(const char *pattern,
                                          const char *pathname, void *data);

/**
 * Call @p callback for each file in @p dirname matching a pattern.
 *
 * Special entries @c . and @c .. are always skipped.
 *
 * The callback is passed a concatenation of a @p dirname and the basename
 * of the current file, not just the basename as scandir() would do.
 *
 * If the callback returns non-zero, the scanning stop and the result value
 * is passed upstream, but if it is @c TE_EOK, zero would be returned.
 *
 * @param dirname     directory to scan
 * @param callback    the callback
 * @param data        user data passed to @p callback
 * @param pattern_fmt glob-style pattern format
 * @param ...         arguments
 *
 * @note @p pattern_fmt may be @c NULL, in which case no arguments shall be
 *       be provided and all files in @p dirname will be processed.
 *
 * @return status code
 */
extern te_errno te_file_scandir(const char *dirname,
                                te_file_scandir_callback *callback,
                                void *data,
                                const char *pattern_fmt, ...)
    TE_LIKE_PRINTF(4, 5);

/**
 * Extract a varying part of a @p filename matching @p pattern.
 *
 * Only a limited subset of globs is supported, namely it must contain
 * exactly one @c * wildcard, otherwise a fatal error is reported.
 *
 * @param filename  filename
 * @param pattern   glob pattern with a single @c *
 * @param basename  if @c true, only the basename of @p filename is
 *                  used for matching.
 *
 * @return the varying part of the filename (must be free()'d) or
 *         @c NULL if @p filename does not match a @p pattern.
 */
extern char *te_file_extract_glob(const char *filename, const char *pattern,
                                  bool basename);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_FILE_H__ */
/**@} <!-- END te_tools_te_file --> */
