/** @file
 * @brief API to deal with dynamic strings
 *
 * @defgroup te_tools_te_string Dynamic strings
 * @ingroup te_tools
 * @{
 *
 * Helper functions to work with strings.
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

#ifndef __TE_STRING_H__
#define __TE_STRING_H__

#include "te_errno.h"
#include "te_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Initial length of the dynamically allocated string */
#define TE_STRING_INIT_LEN  1

/** Additional length when dynamically allocated string is reallocated */
#define TE_STRING_EXTRA_LEN 0

/**
 * TE string type.
 */
typedef struct te_string {
    char   *ptr;          /**< Pointer to the buffer */
    size_t  size;         /**< Size of the buffer */
    size_t  len;          /**< Length of the string */
    te_bool ext_buf;      /**< If TRUE, buffer is supplied
                               by user and should not be
                               reallocated or freed. */
} te_string;

/** On-stack te_string initializer */
#define TE_STRING_INIT  { NULL, 0, 0, FALSE }

/**
 * Initialize TE string assigning buffer to it.
 */
#define TE_STRING_BUF_INIT(buf_)  { buf_, sizeof(buf_), 0, TRUE }

/**
 * Reset TE string (mark its empty).
 *
 * @param str           TE string.
 */
static inline void
te_string_reset(te_string *str)
{
    str->len = 0;
    if (str->ptr != NULL)
        *str->ptr = '\0';
}


/**
 * Append to the string results of the sprintf(fmt, ...);
 *
 * @param str           TE string
 * @param fmt           Format string
 * @param ...           Format string arguments
 *
 * @return Status code.
 */
extern te_errno te_string_append(te_string *str, const char *fmt, ...);

/**
 * Append to the string results of the @b vsnprintf.
 *
 * @param str           TE string
 * @param fmt           Format string
 * @param ap            List of arguments
 *
 * @return Status code.
 */
extern te_errno te_string_append_va(te_string  *str,
                                    const char *fmt,
                                    va_list     ap);

/**
 * Cut from the string specified number of characters.
 *
 * @param str           TE string
 * @param len           Number of characters to cut
 */
extern void te_string_cut(te_string *str, size_t len);

/**
 * Free TE string.
 *
 * @note It will not release buffer supplied by user with
 *       te_string_set_buf().
 *
 * @param str           TE string
 */
extern void te_string_free(te_string *str);

/**
 * Specify buffer to use in TE string instead
 * of dynamically allocated one used by default
 * (buffer may already contain some string).
 *
 * @param str       TE string.
 * @param buf       Buffer.
 * @param size      Number of bytes in buffer.
 * @param len       Length of string already in buffer.
 */
extern void te_string_set_buf(te_string *str,
                              char *buf, size_t size,
                              size_t len);

/**
 * Specify buffer to use in TE string instead
 * of dynamically allocated one used by default.
 *
 * @param str       TE string.
 * @param buf       Buffer.
 * @param size      Number of bytes in buffer.
 */
static inline void
te_string_assign_buf(te_string *str, char *buf, size_t size)
{
    te_string_set_buf(str, buf, size, 0);
}

/**
 * Get string representation of raw data.
 *
 * @param data  Buffer
 * @param size  Number of bytes
 *
 * @return String representation
 */
static inline
char *raw2string(uint8_t *data, int size)
{
    te_string  str = TE_STRING_INIT;
    int        i = 0;

    if (te_string_append(&str, "[ ") != 0)
    {
        te_string_free(&str);
        return NULL;
    }

    for (i = 0; i < size; i++)
    {
        if (te_string_append(&str, "%#02x ", data[i]) != 0)
        {
            te_string_free(&str);
            return NULL;
        }
    }

    if (te_string_append(&str, "]") != 0)
    {
        te_string_free(&str);
        return NULL;
    }

    return str.ptr;
}

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_STRING_H__ */
/**@} <!-- END te_tools_te_string --> */
