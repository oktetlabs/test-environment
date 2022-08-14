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
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_STR_H__
#define __TE_STR_H__

#include "te_defs.h"
#include "te_errno.h"
#include "te_string.h"

#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Delimiter for hex-string representaion, i.e. "00:01:02:04" */
#define TE_STR_HEX_DELIMITER ':'

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
 * Return pointer to an empty string if the string is @c NULL or
 * pointer to the string itself.
 *
 * @param str           Byte string.
 *
 * @return @p str or static empty string if @p str is @c NULL
 */
static inline const char *
te_str_empty_if_null(const char *str)
{
    return (str == NULL ? "" : str);
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
 * Copy a string to provided buffer with NUL-terminated result guarantee.
 *
 * @param dst       Destination buffer
 * @param src       String to copy
 * @param size      Size of destination buffer
 *
 * @return Total length even if it is larger than buffer size.
 */
extern size_t te_strlcpy(char *dst, const char *src, size_t size);

/**
 * Concatenate with NUL-terminated result guarantee a string to provided
 * string which is located in buffer of specified size.
 *
 * @param dst       Buffer with a string to concatenate to
 * @param src       String to append
 * @param size      Size of destination buffer
 *
 * @return Total length even if it is larger than buffer size.
 */
extern size_t te_strlcat(char *dst, const char *src, size_t size);

/**
 * Same as te_strlcpy() except it checks the result itself and returns
 * a status code
 */
extern te_errno te_strlcpy_safe(char *dst, const char *src, size_t size);

/**
 * Call te_strlcpy_safe() and print an error message if there is no enough
 * space in the buffer to allocate the whole input string.
 *
 * @param id        Prefix for error message, usually __func__
 * @param dst       Destination buffer
 * @param src       String to copy
 * @param size      Size of destination buffer
 *
 * @return A pointer to the destination buffer @p dst.
 */
extern char *te_strlcpy_verbose(const char *id, char *dst, const char *src,
                                size_t size);

/**
 * Call te_strlcpy_verbose() with id "__func__".
 *
 * @param _dst      Destination buffer
 * @param _src      String to copy
 * @param _size     Size of destination buffer
 */
#define TE_STRLCPY(_dst, _src, _size) \
    te_strlcpy_verbose(__func__, _dst, _src, _size)

/**
 * The function is equivalent to the function te_snprintf(), except that
 * it just prints error messages instead of returning error code
 *
 * @param id        Prefix for error message, usually __func__
 * @param dst       Pointer to destination buffer
 * @param size      Size of destination buffer
 * @param fmt       Format string
 * @param ...       Format string arguments
 *
 * @return A pointer to the destination string @p dst
 */
extern char *te_snprintf_verbose(const char *id, char *dst, size_t size,
                                 const char *fmt, ...)
                                    __attribute__((format(printf, 4, 5)));

/**
 * Fill a destination buffer according to a format string. It is a wrapper
 * for te_snprintf_verbose()
 *
 * @param _dst      Pointer to destination buffer
 * @param _size     Size of destination buffer
 * @param _fmt      Format string with arguments
 *
 * @sa TE_SPRINTF
 */
#define TE_SNPRINTF(_dst, _size, _fmt...) \
    te_snprintf_verbose(__func__, _dst, _size, _fmt)

/**
 * Fill a destination buffer according to a format string. It should only
 * be used if the buffer is a real array, not a pointer
 *
 * @sa TE_SNPRINTF
 */
#define TE_SPRINTF(_buf, _fmt...) \
    TE_SNPRINTF(_buf, sizeof(_buf), _fmt)

/**
 * Write at most @p size bytes (including the terminating null byte ('\0')) to
 * the buffer pointed to by @p dst according to a format string.
 *
 * @note The function does not call the va_end macro. Because it invokes
 *       the va_arg macro, the value of ap is undefined after the call
 *
 * @param dst       Pointer to destination buffer
 * @param size      Size of destination buffer
 * @param fmt       Format string
 * @param ap        List of arguments
 *
 * @return Status code
 * @retval TE_ESMALLBUF     Size of destination buffer is not big enough
 *                          to store the whole source string.
 *                          If @p size = @c 0, @p dst is not modified
 * @retval errno            An output error is encountered
 *
 * @sa te_snprintf, te_strnprintf
 */
extern te_errno te_vsnprintf(char *dst, size_t size,
                             const char *fmt, va_list ap);

/**
 * The function is equivalent to the function te_vsnprintf(), except that
 * it is called with a variable number of arguments instead of a va_list
 */
extern te_errno te_snprintf(char *dst, size_t size, const char *fmt, ...)
                                    __attribute__((format(printf, 3, 4)));

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
 * Wrapper over strtoumax().
 *
 * @param str       String to convert.
 * @param base      Base of a numeral system.
 * @param value     Where to save conversion result.
 *
 * @return Status code.
 */
extern te_errno te_strtoumax(const char *str, int base,
                             uintmax_t *value);

/**
 * Wrapper over te_strtoumax() with overflow check.
 *
 * @param str       String to convert
 * @param base      Base of a numeral system
 * @param value     Where to save conversion result
 *
 * @return Status code.
 */
extern te_errno te_str_to_uint64(const char *str, int base,
                                 uint64_t *value);

/**
 * Wrapper over te_strtoumax() with overflow check.
 *
 * @param str       String to convert
 * @param base      Base of a numeral system
 * @param value     Where to save conversion result
 *
 * @return Status code.
 */
extern te_errno te_strtoul(const char *str, int base,
                           unsigned long int *value);

/**
 * Wrapper for te_strtol() with overflow check
 *
 * @param str   String to convert
 * @param base  Base of the numeral system
 * @param value Location for the resulting value
 *
 * @return Status code
 */
extern te_errno te_strtoi(const char *str, int base, int *value);

/**
 * Wrapper for te_strtoumax() with overflow check.
 *
 * @param str   String to convert
 * @param base  Base of the numeral system
 * @param value Location for the resulting value
 *
 * @return Status code
 */
extern te_errno te_strtoui(const char   *str,
                           int           base,
                           unsigned int *value);

/**
 * Convert string to long int. Following characters are
 * allowed.
 *
 * @param input        String to convert
 * @param [out]endptr  Pointer to the end of parsed int
 * @param base         Base to be used
 * @param [out]result  Storage for result
 *
 * @return 0 or error
 */
extern te_errno te_strtol_raw(const char *input, char **endptr, int base,
                              long int *result);

/**
 * Convert string to long int without error logs input is not a number.
 *
 * Should be used to avoid creating extra vars for 'end' parameter in the code.
 *
 * @param input        String to convert
 * @param base         Base to be used
 * @param [out]result  Storage for result
 *
 * @return 0 or error
 */
extern te_errno te_strtol_silent(const char *input, int base, long int *result);

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
extern te_errno te_strtol(const char *input, int base, long int *result);

/**
 * Convert string to bool meaning 0 - FALSE, not 0 - TRUE
 *
 * @param input        String to convert
 * @param [out]bresult Storage for result
 *
 * @return 0 or error
 */
extern te_errno te_strtol_bool(const char *input, te_bool *bresult);

/**
 * Convert string to double. Following characters are allowed.
 *
 * @param str          String to convert
 * @param endptr       Pointer to the end of parsed double
 * @param [out]result  Storage for result
 *
 * @return Status code.
 */
extern te_errno te_strtod_raw(const char *str, char **endptr, double *result);

/**
 * Convert string to double. Should be used to avoid creating extra vars
 * for 'end' parameter in the code.
 *
 * @param str          String to convert
 * @param [out]result  Storage for result
 *
 * @return Status code.
 */
extern te_errno te_strtod(const char *str, double *result);

/**
 * Convert raw data to hex-string representation
 *
 * @param data          Raw data to be converted
 * @param data_len      Length of the @p data
 * @param str           Pointer to @b te_string
 *
 * @return              Status code
 */
extern te_errno te_str_hex_raw2str(const uint8_t *data, size_t data_len,
                                   te_string *str);

/**
 * Convert hex-string to raw data
 *
 * @param str           Null-terminated hex-string
 * @param data          Storage to raw data
 * @param data_len      The length of the @p data
 *
 * @return              Status code
 */
extern te_errno te_str_hex_str2raw(const char *str, uint8_t *data,
                                   size_t data_len);

/**
 * Find index of the string in the array of strings.
 *
 * @param str           Null-terminated string
 * @param str_array     Array of strings
 * @param str_array_len The length of the @p str_array
 * @param[out] index    The index of the @p srt in the @p str_array
 *
 * @return              Status code
 */
extern te_errno te_str_find_index(const char *str, const char **str_array,
                                  unsigned int str_array_len,
                                  unsigned int *index);

/**
 * Free array of strings
 *
 * @param str   The array of strings with @c NULL item at the end
 */
extern void te_str_free_array(char **str);


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_STR_H__ */
/**@} <!-- END te_tools_te_str --> */
