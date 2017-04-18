/** @file
 * @brief API to deal with strings concatenation
 *
 * Function to concatenate the strings.
 *
 *
 * Copyright (C) 2016-2017 Test Environment authors (see file AUTHORS
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
 * @author Ivan Melnikov <Ivan.Melnikov@oktetlabs.ru>
 */

#ifndef __TE_CONCAT_H__
#define __TE_CONCAT_H__

#include "te_str.h"

#ifdef __cplusplus
extern "C" {
#endif


/**
 * Concatenate two strings and put result to the newly allocated string.
 * Returned string should be freed with @p free(3) when it is no longer
 * needed.
 *
 * @param first     First string to concatenate. May be @c NULL, in this
 *                  case it will be interpreted as empty string, i.e. "".
 * @param second    Second string to concatenate. May be @c NULL, in this
 *                  case it will be interpreted as empty string, i.e. "".
 *
 * @return Concatenated string, or @c NULL if insufficient memory available
 *         to allocate a resulting string.
 *
 * @deprecated Use @ref te_str_concat instead.
 */
static inline char *
te_concat(const char *first, const char *second)
{
    return te_str_concat(first, second);
}


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_CONCAT_H__ */
