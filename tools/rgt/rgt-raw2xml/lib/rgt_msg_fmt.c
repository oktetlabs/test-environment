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

#include <arpa/inet.h>
#include "te_errno.h"
#include "rgt_msg_fmt.h"

te_bool
rgt_msg_fmt_out_obstack(void        *obstack,
                        const void  *ptr,
                        size_t       len)
{
    obstack_grow((struct obstack *)obstack, ptr, len);
    return TRUE;
}


#define OUT(_ptr, _len) \
    do {                                        \
        if (!(*out_fn)(out_data, _ptr, _len))   \
            return FALSE;                       \
    } while (0)


te_bool
rgt_msg_fmt_spec_plain(const char         **pspec,
                       size_t              *plen,
                       const rgt_msg_fld  **parg,
                       rgt_msg_fmt_out_fn  *out_fn,
                       void                *out_data)
{
    const char         *spec;
    size_t              len;
    const char         *p;
    char                c;
    const rgt_msg_fld  *arg;

    assert(pspec != NULL);
    assert(*pspec != NULL);
    assert(plen != NULL);
    assert(parg != NULL);
    assert(*parg != NULL);
    assert(out_fn != NULL);

    spec = *pspec;
    p = spec;
    len = *plen;
    if (len == 0 || *p != '%')
        return TRUE;
    p++;
    len--;

    if (len == 0)
    {
        OUT(spec, p - spec);
        *pspec = spec;
        *plen = len;
        return TRUE;
    }

    c = *p++;
    len--;
    arg = *parg;
    switch (c)
    {
        case 's':
            if (rgt_msg_fld_is_term(arg))
                OUT(spec, p - spec);
            else
            {
                OUT(arg->buf, arg->len);
                *parg = rgt_msg_fld_next(arg);
            }
            break;

        case 'r':
            {
                te_errno    rc;
                const char *src;
                const char *err;

                if (rgt_msg_fld_is_term(arg))
                {
                    OUT(spec, p - spec);
                    break;
                }

                rc = ntohl(*(const uint32_t *)arg->buf);
                src = te_rc_mod2str(rc);
                err = te_rc_err2str(rc);

                if (*src == '\0')
                    OUT(err, strlen(err));
                else
                {
                    size_t  src_len     = strlen(src);
                    size_t  err_len     = strlen(err);
                    size_t  len         = src_len + 1 + err_len;
                    char    buf[len];
                    char   *p           = buf;

                    memcpy(p, src, src_len);
                    p += src_len;
                    *p++ = '-';
                    memcpy(p, err, err_len);
                    OUT(buf, len);
                }

                *parg = rgt_msg_fld_next(arg);

                break;
            }

        case 'c':
        case 'd':
        case 'u':
        case 'o':
        case 'x':
        case 'X':
            {
                char        fmt[]       = {'%', c, '\0'};
                char        buf[16];

                if (rgt_msg_fld_is_term(arg))
                {
                    OUT(spec, p - spec);
                    break;
                }

                if (arg->len != 4)
                {
                    errno = EINVAL;
                    return FALSE;
                }

                OUT(buf, sprintf(buf, fmt,
                                 ntohl(*(const uint32_t *)arg->buf)));
                *parg = rgt_msg_fld_next(arg);
                break;
            }

        case 'p':
            if (rgt_msg_fld_is_term(arg))
            {
                OUT(spec, p - spec);
                break;
            }

            if (arg->len == 0 || (arg->len % sizeof(uint32_t)) != 0 ||
                /* For stack safety - 128 bit addresses should be enough */
                arg->len > (sizeof(uint64_t) * 2))
            {
                errno = EINVAL;
                return FALSE;
            }

            {
                static const char   xd[]  = "0123456789ABCDEF";

                size_t          al              = arg->len;
                char            buf[2 + al * 2];
                const uint8_t  *ap              = arg->buf;
                char           *bp              = buf;
                uint8_t         a;

                *bp++ = '0';
                *bp++ = 'x';

                /* Skip leading zeroes */
                for (; al > 0 && *(const uint32_t *)ap == 0;
                     al -= sizeof(uint32_t), ap += sizeof(uint32_t));

                /* Output hexadecimal digits */
                for (; al > 0; al--, ap++)
                {
                    a = *ap;
                    *bp++ = xd[a >> 4];
                    *bp++ = xd[a & 0xF];
                }

                /* Output result */
                OUT(buf, sizeof(buf));
            }

            *parg = rgt_msg_fld_next(arg);

            break;

        case '%':
            OUT(&c, 1);
            return TRUE;

        default:
            OUT(spec, p - spec);
            if (!rgt_msg_fld_is_term(arg))
                *parg = rgt_msg_fld_next(arg);
            break;
    }

    *pspec = p;
    *plen = len;

    return TRUE;
}


te_bool rgt_msg_fmt(const char             *fmt,
                    size_t                  len,
                    const rgt_msg_fld     **parg,
                    rgt_msg_fmt_spec_fn    *spec_fn,
                    rgt_msg_fmt_out_fn     *out_fn,
                    void                   *out_data)
{
    const char *prev_p;
    const char *p;
    size_t      l;

    assert(fmt != NULL);
    assert(parg != NULL);
    assert(*parg != NULL);
    assert(out_fn != NULL);

    prev_p = fmt;
    p = prev_p;
    l = len;

    while (TRUE)
    {
        for (; l > 0 && *p != '%'; p++, l--);

        if (p > prev_p)
            OUT(prev_p, p - prev_p);

        if (l == 0)
            return TRUE;

        if (!(*spec_fn)(&p, &l, parg, out_fn, out_data))
            return FALSE;

        prev_p = p;
    }
}


