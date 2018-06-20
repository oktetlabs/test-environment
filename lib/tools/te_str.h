/** @file
 * @brief API to deal with strings
 *
 * @defgroup te_tools_te_str Regular strings
 * @ingroup te_tools
 * @{
 *
 * Function to operate the strings.
 *
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
 *
 *
 * @author Ivan Melnikov <Ivan.Melnikov@oktetlabs.ru>
 */

#ifndef __TE_STR_H__
#define __TE_STR_H__

#include "te_defs.h"
#include "te_errno.h"

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


/**
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

/**
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

/**
 * Take off heading and trailing spaces (all the symbols " \f\n\r\t\v").
 *
 * @param str   String to strip spaces.
 *
 * @return Pointer to the duplicated (heap) string without surrounding spaces,
 *         or @c NULL in case of memory allocation failure.
 */
extern char *te_str_strip_spaces(const char *str);

/**
 * Wrapper over strtoul().
 *
 * @param str       String to convert.
 * @param base      Base of a numeral system.
 * @param value     Where to save conversion result.
 *
 * @return Status code.
 */
extern te_errno te_strtoul(const char *str, int base,
                           unsigned long int *value);


/**
 * Convert string to long int. Should be used to avoid
 * creating extra vars for 'end' parameter in the code.
 *
 * @param input        String to convert
 * @param base         Base to be used
 * @param [out]result  Storage for result
 *
 * @return 0 or error
 */
extern te_errno te_strtol(const char *input, int base, long int* result);

/**
 * Convert string to bool meaning 0 - FALSE, not 0 - TRUE
 *
 * @param input        String to convert
 * @param [out]bresult Storage for result
 *
 * @return 0 or error
 */
extern te_errno te_strtol_bool(const char *input, te_bool *bresult);


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_STR_H__ */
/**@} <!-- END te_tools_te_str --> */
