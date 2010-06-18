/** @file
 * @brief Test Environment: RGT message formatting.
 *
 * Copyright (C) 2010 Test Environment authors (see file AUTHORS
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
 * @author Nikolai Kondrashov <Nikolai.Kondrashov@oktetlabs.ru>
 * 
 * $Id$
 */


#ifndef __RGT_MSG_FMT_H__
#define __RGT_MSG_FMT_H__

#include "rgt_msg.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Prototype for a formatting output function.
 *
 * @param data  Opaque data passed through the formatting function.
 * @param ptr   Output pointer.
 * @param len   Output length.
 *
 * @return TRUE if the output succeeded, FALSE otherwise.
 */
typedef te_bool rgt_msg_fmt_out_fn(void *data, const void *ptr, size_t len);

/**
 * Format a single specifier from a message format string.
 *
 * @param pspec     Location of/for a string pointer; initially at the
 *                  beginnging of the format specifier.
 * @param parg      Location of/for a message argument field pointer.
 * @param out_fn    Output function.
 * @param out_data  Output data.
 *
 * @return TRUE if output succeeded, FALSE otherwise.
 */
extern te_bool rgt_msg_fmt_spec(const char            **pspec,
                                const rgt_msg_fld     **parg,
                                rgt_msg_fmt_out_fn     *out_fn,
                                void                   *out_data);

/**
 * Format a message format string.
 *
 * @param fmt       Message format string.
 * @param arg       Message first argument field.
 * @param out_fn    Output function.
 * @param out_data  Output data.
 *
 * @return TRUE if output succeeded, FALSE otherwise.
 */
extern te_bool rgt_msg_fmt(const char          *spec,
                           const rgt_msg_fld   *arg,
                           rgt_msg_fmt_out_fn  *out_fn,
                           void                *out_data);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* !__RGT_MSG_FMT_H__ */
