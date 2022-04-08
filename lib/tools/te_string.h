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
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
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
#define TE_STRING_INIT_LEN      (16)

/** String grow factor which is used during reallocation */
#define TE_STRING_GROW_FACTOR   (2)

/**
 * The grow factor exponent limit during a single relocation.
 * If a good size is not found in a range [size,
 * TE_STRING_GROW_FACTOR ^ TE_STRING_GROW_FACTOR_EXP_LIMIT], the fallback to
 * addendum-based grow is performed.
 *
 * Empirically, current grow factor exponent limit is enough for running
 * long tests without too frequent reallocation.
 */
#define TE_STRING_GROW_FACTOR_EXP_LIMIT (4)

/**< Type of function to be used for releasing resources */
struct te_string;
typedef void (te_string_free_func)(struct te_string *str);

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

    te_string_free_func *free_func; /**< If set, will be called
                                         from te_string_free()
                                         instead of default
                                         actions */
} te_string;

/* Internal function, to be used only in initializers. */
extern te_string_free_func te_string_free_heap;

/** On-stack te_string initializer */
#define TE_STRING_INIT TE_STRING_INIT_RESERVE(0)

/**
 * On-stack te_string initializer with a defined reserve
 */
#define TE_STRING_INIT_RESERVE(reserved_size_) \
    { .ptr = NULL, .size = (reserved_size_), .len = 0, .ext_buf = FALSE, \
      .free_func = &te_string_free_heap }

/**
 * On-stack te_string initializer with a defined reserve and
 * free function.
 */
#define TE_STRING_INIT_RESERVE_FREE(reserved_size_, free_func_) \
    { .ptr = NULL, .size = (reserved_size_), .len = 0, .ext_buf = FALSE, \
      .free_func = free_func_ }

/**
 * Initialize TE string assigning buffer to it.
 */
#define TE_STRING_BUF_INIT(buf_) \
    { .ptr = buf_, .size = sizeof(buf_), .len = 0, .ext_buf = TRUE, \
      .free_func = &te_string_reset }

/**
 * Initialize TE string assigning buffer and size to it.
 */
#define TE_STRING_EXT_BUF_INIT(buf_, size_) \
    { .ptr = buf_, .size = size_, .len = 0, .ext_buf = TRUE, \
      .free_func = &te_string_reset }

/**
 * Initialize TE string assigning statically allocated memory
 * to it. Dynamic memory allocation will not be used for such
 * string, so there will be no need in calling te_string_free().
 *
 * @param _size     Number of bytes reserved for storing the string.
 */
#define TE_STRING_INIT_STATIC(_size) TE_STRING_BUF_INIT((char[_size]){'\0'})

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
 * Reserve space for at least @p size elements in @p str string
 * (including null byte at the end).
 *
 * @param str           TE string.
 * @param size          Number of elements to to have a room for in a string
 *
 * @return Status code
 *
 * @remark If there is a room already for the number of elements specified
 *         by @p size, no action would be performed.
 */
extern te_errno te_string_reserve(te_string *str, size_t size);

/**
 * Get value of TE string.
 *
 * @param str     Pointer to TE string
 *
 * @return Pointer to null-terminated sequence of characters.
 *         If @p str is @c NULL or its internal buffer is not
 *         allocated, pointer to empty string is returned.
 */
static inline const char *te_string_value(const te_string *str)
{
    if (str == NULL || str->ptr == NULL)
        return "";

    return str->ptr;
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
 * Append contents of a buffer to TE string. Buffer may be not
 * null-terminated.
 *
 * @param str     TE string
 * @param buf     Buffer
 * @param len     Number of bytes in buffer
 *
 * @return Status code.
 */
extern te_errno te_string_append_buf(te_string *str, const char *buf,
                                     size_t len);

/**
 * Append arguments separated by space with required shell escaping
 * to avoid expansion and variable substitution.
 *
 * @param str           TE string
 * @param ...           String arguments terminated by @c NULL
 *
 * @return Status code.
 */
extern te_errno te_string_append_shell_args_as_is(te_string *str, ...)
                    __attribute__((sentinel));

/**
 * Append a single argument with required shell escaping to avoid
 * expansion and variable substitution.
 *
 * @param str           TE string
 * @param arg           String argument
 *
 * @return Status code
 */
extern te_errno te_string_append_shell_arg_as_is(te_string *str,
                                                 const char *arg);
/**
 * Return a char * that is a result of sprintf into allocated memory.
 *
 * @param fmt       Format string
 * @param ap        List of arguments
 *
 * @return string on which you can call free(), or @c NULL in case of error
 */
extern char *te_string_fmt_va(const char *fmt,
                              va_list     ap);
/**
 * Return a char * that is a result of sprintf into allocated memory.
 *
 * @param fmt       Format string
 * @param ...       Format string arguments
 *
 * @return string on which you can call free(), or @c NULL in case of error
 */
extern char *te_string_fmt(const char *fmt,
                           ...) __attribute__((format(printf, 1, 2)));

/**
 * Cut from the string specified number of characters.
 *
 * @param str           TE string
 * @param len           Number of characters to cut
 */
extern void te_string_cut(te_string *str, size_t len);

/**
 * Cut specified number of characters from the beginning of the string.
 *
 * @param str           TE string
 * @param len           Number of characters to cut from the beginning
 */
extern void te_string_cut_beginning(te_string *str, size_t len);

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

/**
 * @defgroup te_tools_te_substring Substring manipulation API
 * @ingroup te_tools_te_string
 * @{
 */

/** Structure for describing a piece of string */
typedef struct te_substring_t {
   /** The underlying string */
   te_string *base;
   /** The position of the beginning of the substring */
   size_t start;
   /** The length of the substring */
   size_t len;
} te_substring_t;

/**
 * Substring initializer
 *
 * @param _base Pointer to the base string
 */
#define TE_SUBSTRING_INIT(_base) \
    (te_substring_t){ .base = _base, .len = 0, .start = 0 }

/**
 * Check that substring is valid
 *
 * @param substr Substring
 *
 * @return @c TRUE or @c FALSE
 */
static inline te_bool
te_substring_is_valid(const te_substring_t *substr)
{
    return substr->start < substr->base->len;
}

/**
 * Find a @p str starting at @p substr position and update it accordingly
 *
 * @param substr Substring
 * @param str    The string to find
 */
extern void te_substring_find(te_substring_t *substr, const char *str);

/**
 * Replace a substring at a given position, modifying
 * the underlying `te_string`
 *
 * @param substr Substring
 * @param str    Replacement string
 *
 * @return Status code
 */
extern te_errno te_substring_replace(te_substring_t *substr,
                                     const char *str);

/**
 * Move the position by the length of the previously substring
 *
 * @param substr Substring
 */
extern void te_substring_advance(te_substring_t *substr);

/**
 * Limit the length of the @p substr to position @p limit
 * so that it ended right before @p limit
 *
 * @param substr Substring
 * @param limit Limiting substring
 */
extern void te_substring_limit(te_substring_t *substr,
                               const te_substring_t *limit);

/**@} <!-- END te_tools_te_substring --> */

/**
 * Replace all the substrings in a string
 *
 * @param src The string in which to replace.
 * @param new The new substring to replace.
 * @param old The substring to be replaced.
 *
 * @return Status code
 */
extern te_errno te_string_replace_all_substrings(te_string *str,
                                                 const char *new,
                                                 const char *old);

/**
 * Replace the substring in a string
 *
 * @param src The string in which to replace.
 * @param new The new substring to replace.
 * @param old The substring to be replaced.
 *
 * @return Status code
 */
extern te_errno te_string_replace_substring(te_string *str,
                                            const char *new,
                                            const char *old);
#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_STRING_H__ */
/**@} <!-- END te_tools_te_string --> */
