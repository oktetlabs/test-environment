/** @file
 * @brief Format string parsing
 *
 * Some TE-specific features, such as memory dump, file content logging,
 * and additional length modifiers are supported.
 *
 *
 * Copyright (C) 2005 Test Environment authors (see file AUTHORS
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
 * @author Ivan Soloducha <Ivan.Soloducha@oktetlabs.ru>
 *
 * $Id: $
 */

#ifndef  __TE_TOOLS_FORMAT_H__
#define  __TE_TOOLS_FORMAT_H__

#include <stdio.h>
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#include "te_errno.h"


/** Parameters for te_log_vprintf() function */
struct te_log_out_params {
    FILE   *fp;     /**< Output file; if NULL, no file output */
    char   *buf;    /**< Output buffer; if NULL, no buffer output */
    size_t  buflen; /**< Buffer length; for NULL buffer must be 0 */
    size_t  offset; /**< Offset where output should begin */
};

/**
  * Preprocess and output message to log with special features parsing
  *
  * @param param        Output parameters
  * @param fmt          Format string
  * @param ap           Arguments for the format string
  *
  * @return             N/A
  */
extern te_errno te_log_vprintf(struct te_log_out_params *param,
                              const char *fmt, va_list ap);

#endif /* !__TE_TOOLS_FORMAT_H__ */
