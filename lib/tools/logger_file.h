/** @file
 * @brief Logger subsystem API
 *
 * Logger internal API to be used by standalone TE off-line applications.
 *
 *
 * Copyright (C) 2006 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
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
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_LIB_LOGGER_FILE_H__
#define __TE_LIB_LOGGER_FILE_H__

#if HAVE_STDIO_H
#include <stdio.h>
#endif

#include "logger_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/** File stream to write log messages. */
extern FILE *te_log_message_file_out;

/**
 * Log message to the file stream @e te_log_message_file_out.
 *
 * This function complies with te_log_message_f prototype.
 */
extern void te_log_message_file(const char *file, unsigned int line,
                                te_log_ts_sec sec, te_log_ts_usec usec,
                                unsigned int level,
                                const char *entity, const char *user,
                                const char *fmt, va_list ap); 

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_LIB_LOGGER_FILE_H__ */
