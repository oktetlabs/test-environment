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

#ifdef __cplusplus
extern "C" {
#endif

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

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_FILE_H__ */
/**@} <!-- END te_tools_te_file --> */
