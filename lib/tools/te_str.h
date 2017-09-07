/** @file
 * @brief API to deal with strings
 *
 * Function to operate the strings.
 *
 *
 * Copyright (C) 2017 Test Environment authors (see file AUTHORS
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

#ifndef __TE_STR_H__
#define __TE_STR_H__

#include "te_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Check for empty string.
 *
 * @param str           Byte string.
 *
 * @return @c TRUE if the string is @c NULL or zero-length; @c FALSE otherwise.
 */
static inline te_bool
te_str_is_null_or_empty(const char *str)
{
    return (str == NULL || *str == '\0');
}


/*
 * Convert lowercase letters of the @p string to uppercase. The function does
 * not work with multibyte/wide strings.
 *
 * @note Return value should be freed with free(3) when it is no longer needed.
 *
 * @param src               Source string.
 *
 * @return Uppercase letters string, or @c NULL if it fails to allocate memory.
 */
extern char *te_str_upper(const char *src);

/*
 * Convert uppercase letters of the @p string to lowercase. The function does
 * not work with multibyte/wide strings.
 *
 * @note Return value should be freed with free(3) when it is no longer needed.
 *
 * @param src               Source string.
 *
 * @return Lowercase letters string, or @c NULL if it fails to allocate memory.
 */
extern char *te_str_lower(const char *src);

/**
 * Concatenate two strings and put result to the newly allocated string.
 * The function does not work with multibyte/wide strings.
 *
 * @note Return value should be freed with free(3) when it is no longer needed.
 *
 * @param first     First string to concatenate. May be @c NULL, in this
 *                  case it will be interpreted as empty string, i.e. "".
 * @param second    Second string to concatenate. May be @c NULL, in this
 *                  case it will be interpreted as empty string, i.e. "".
 *
 * @return Concatenated string, or @c NULL if insufficient memory available
 * to allocate a resulting string.
 */
extern char *te_str_concat(const char *first, const char *second);

/**
 * Copy at most @p size bytes of the string pointed to by @p src to the buffer
 * pointed to by @p dst. It prints an error message if the @p size of destination
 * buffer is not big enough to store the whole source string, and terminates the
 * result string with '\0' forcibly.
 *
 * @param id        Prefix for error message, usually __func__.
 * @param dst       Pointer to destination buffer.
 * @param size      Size of destination buffer.
 * @param src       Source string buffer to copy from.
 *
 * @return A pointer to the destination string @p dst.
 */
extern char *te_strncpy(const char *id, char *dst, size_t size, const char *src);

/**
 * Copy one string to another. It is a wrapper for te_strncpy().
 *
 * @param _dst      Pointer to destination buffer.
 * @param _size     Size of destination buffer.
 * @param _src      Pointer to source string buffer to copy from.
 */
#define TE_STRNCPY(_dst, _size, _src) \
    te_strncpy(__func__, _dst, _size, _src)


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_STR_H__ */
