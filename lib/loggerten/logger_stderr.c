/** @file
 * @brief Logger subsystem API
 *
 * Logger library to be used by standalone TE off-line applications.
 *
 *
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS
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
 * @author Igor B. Vasiliev <Igor.Vasiliev@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER     "Logger StdErr"

#include "te_config.h"

#include <stdio.h>
#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_STDARG_H
#include <stdarg.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#endif
#if HAVE_STRINGS_H
#include <strings.h>
#endif
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#include "te_defs.h"
#include "te_errno.h"
#include "logger_api.h"


static void stderr_log_message(const char *file, unsigned int line,
                               unsigned int level, const char *entity,
                               const char *user, const char *fmt, ...);

/** Logging backend */
te_log_message_f te_log_message = stderr_log_message;


/**
 * Log message to stderr stream.
 *
 * This function complies with te_log_message_f prototype.
 */
static void
stderr_log_message(const char *file, unsigned int line,
                   unsigned int level, const char *entity,
                   const char *user, const char *fmt, ...)
{
    UNUSED(file);
    UNUSED(line);
    UNUSED(level);
    UNUSED(entity);
    UNUSED(user);
    UNUSED(fmt);
}
