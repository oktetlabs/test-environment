/** @file
 * @brief Testing Results Comparator: common
 *
 * Helper functions to work with dynamically strings.
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

#ifdef __cplusplus
extern "C" {
#endif

/** Initial length of the dynamically allocated string */
#define TE_STRING_INIT_LEN  1

/** Additional length when dynamically allocated string is reallocated */
#define TE_STRING_EXTRA_LEN 0

/**
 * Dynamically allocated string.
 */
typedef struct te_string {
    char   *ptr;    /**< Pointer to the buffer */
    size_t  size;   /**< Size of the buffer */
    size_t  len;    /**< Length of the string */
} te_string;

/** On-stack te_string initializer */
#define TE_STRING_INIT  { NULL, 0, 0 }

/**
 * Append to the string results of the sprintf(fmt, ...);
 *
 * @param str           Dynamic string
 * @param fmt           Format string
 * @param ...           Format string arguments
 *
 * @return Status code.
 */
extern te_errno te_string_append(te_string *str, const char *fmt, ...);

/**
 * Cut from the string specified number of characters.
 *
 * @param str           Dynamic string
 * @param len           Number of characters to cut
 */
extern void te_string_cut(te_string *str, size_t len); 

/**
 * Free dynamic string.
 *
 * @param str           Dynamic string
 */
extern void te_string_free(te_string *str);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_STRING_H__ */
