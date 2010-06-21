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

#include <obstack.h>
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
 * Output formatted string chunks to an obstack.
 *
 * @param obstack   Obstack to output to.
 * @param ptr       Output pointer.
 * @param len       Output length.
 */
extern te_bool rgt_msg_fmt_out_obstack(void        *obstack,
                                       const void  *ptr,
                                       size_t       len);

/**
 * Prototype for a function used format a single specifier from a message
 * format string.
 *
 * @param pspec     Location of/for a format string pointer; initially at
 *                  the beginnging of the format specifier.
 * @param plen      Location of/for a format string length.
 * @param parg      Location of/for a message argument field pointer.
 * @param out_fn    Output function.
 * @param out_data  Output data.
 *
 * @return TRUE if output succeeded, FALSE otherwise.
 */
typedef te_bool rgt_msg_fmt_spec_fn(const char            **pspec,
                                    size_t                 *plen,
                                    const rgt_msg_fld     **parg,
                                    rgt_msg_fmt_out_fn     *out_fn,
                                    void                   *out_data);

/**
 * Format a single specifier from a message format string as plain text.
 *
 * @param pspec     Location of/for a format string pointer; initially at
 *                  the beginnging of the format specifier.
 * @param plen      Location of/for a format string length.
 * @param parg      Location of/for a message argument field pointer.
 * @param out_fn    Output function.
 * @param out_data  Output data.
 *
 * @return TRUE if output succeeded, FALSE otherwise.
 */
extern te_bool rgt_msg_fmt_spec_plain(const char          **pspec,
                                      size_t               *plen,
                                      const rgt_msg_fld   **parg,
                                      rgt_msg_fmt_out_fn   *out_fn,
                                      void                 *out_data);

/**
 * Format a message format string.
 *
 * @param fmt       Message format string.
 * @param len       Format string length.
 * @param parg      Location of/for a message argument field pointer.
 * @param fmt_fn    Single format specifier formatting function.
 * @param out_fn    Output function.
 * @param out_data  Output data.
 *
 * @return TRUE if output succeeded, FALSE otherwise.
 */
extern te_bool rgt_msg_fmt(const char          *fmt,
                           size_t               len,
                           const rgt_msg_fld  **parg,
                           rgt_msg_fmt_spec_fn *spec_fn,
                           rgt_msg_fmt_out_fn  *out_fn,
                           void                *out_data);

/**
 * Format a message format string as plain text.
 *
 * @param fmt       Message format string.
 * @param len       Format string length.
 * @param parg      Location of/for a message argument field pointer.
 * @param out_fn    Output function.
 * @param out_data  Output data.
 *
 * @return TRUE if output succeeded, FALSE otherwise.
 */
static inline te_bool
rgt_msg_fmt_plain(const char           *fmt,
                  size_t                len,
                  const rgt_msg_fld   **parg,
                  rgt_msg_fmt_out_fn   *out_fn,
                  void                 *out_data)
{
    return rgt_msg_fmt(fmt, len, parg,
                       rgt_msg_fmt_spec_plain, out_fn, out_data);
}


/**
 * Format a message format string as plain text, outputting to an obstack.
 *
 * @param obs       Obstack to output to.
 * @param fmt       Message format string.
 * @param len       Format string length.
 * @param parg      Location of/for a message argument field pointer.
 *
 * @return TRUE if output succeeded, FALSE otherwise.
 */
static inline te_bool
rgt_msg_fmt_plain_obstack(struct obstack       *obs,
                          const char           *fmt,
                          size_t                len,
                          const rgt_msg_fld   **parg)
{
    return rgt_msg_fmt_plain(fmt, len, parg,
                             rgt_msg_fmt_out_obstack, obs);
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* !__RGT_MSG_FMT_H__ */
