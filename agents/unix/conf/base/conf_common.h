/** @file
 * @brief Unix Test Agent
 *
 * Common declarations for Unix TA configuration
 *
 *
 * Copyright (C) 2017 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
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
 * @author Dmitry Izbitsky <Dmitry.Izbitsky@oktetlabs.ru>
 */

#ifndef __TE_AGENTS_UNIX_CONF_BASE_CONF_COMMON_H_
#define __TE_AGENTS_UNIX_CONF_BASE_CONF_COMMON_H_

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_LIMITS_H
#include <limits.h>
#endif

#include "te_defs.h"
#include "te_errno.h"

/* UNIX branching heritage: PATH_MAX can still be undefined here yet */
#if !defined(PATH_MAX)
#define PATH_MAX 108
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Write requested value to system file.
 *
 * @param value     Null-terminated string containing the value.
 * @param format    Format string for path to the system file.
 * @param ...       Arguments for the format string.
 *
 * @return Status code.
 */
extern te_errno write_sys_value(const char *value, const char *format, ...);

/**
 * Read requested value from system file.
 *
 * @param value           Where to save the value.
 * @param len             Expected length, including null byte.
 * @param ignore_eacces   If @c TRUE, return success saving
 *                        empty string in @p value if the file
 *                        cannot be opened due to @c EACCES error.
 * @param format          Format string for path to the system file.
 * @param ...             Arguments for the format string.
 *
 * @return Status code.
 */
extern te_errno read_sys_value(char *value, size_t len,
                               te_bool ignore_eaccess,
                               const char *format, ...);

/**
 * Type of callback which may be passed to get_dir_list().
 * It is used to check whether a given item should be included
 * in the list.
 */
typedef te_bool (*include_callback_func)(const char *name, void *data);

/**
 * Obtain list of files in a given directory.
 *
 * @param path              Filesystem path.
 * @param buffer            Where to save the list.
 * @param length            Available space in @p buffer.
 * @param ignore_abscence   If @c TRUE, return success and
 *                          save empty string to @p buffer
 *                          if @p path does not exist.
 * @param include_callback  If not @c NULL, will be called for
 *                          each file name before including it
 *                          in the list. The file name will be
 *                          included only if this callback
 *                          returns @c TRUE.
 * @param callback_data     Pointer which should be passed
 *                          to the callback as the second argument.
 *
 * @return Status code.
 */
extern te_errno get_dir_list(const char *path, char *buffer, size_t length,
                             te_bool ignore_absence,
                             include_callback_func include_callback,
                             void *callback_data);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* !__TE_AGENTS_UNIX_CONF_BASE_CONF_COMMON_H_ */
