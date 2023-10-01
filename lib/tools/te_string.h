/* SPDX-License-Identifier: Apache-2.0 */
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
 * Copyright (C) 2004-2023 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_STRING_H__
#define __TE_STRING_H__

#include "te_errno.h"
#include "te_defs.h"
#include "te_vector.h"

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
 *
 * @note: You should probably not use it unless you really
 *        need to. Please use simple TE_STRING_INIT.
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
 * Safely move the string content to a pointer variable.
 *
 * te_strings are regularly used as temporary containers,
 * with the resulting data being passed upstream as a plain
 * character pointer. The function ensures that the pointer
 * won't be freed by an accidental te_string_free() etc.
 *
 * @param[out]      dest   Location of a destination pointer
 * @param[in,out]   src    Source string
 *
 * @note The function must not be used with an external-buffer
 *       te_strings --- it is a logic error: the function
 *       does transfer the ownership of the memory but
 *       an external-buffer te_string does *not* own the memory
 */
static inline void
te_string_move(char **dest, te_string *src)
{
    assert(!src->ext_buf);
    *dest = src->ptr;

    src->size = 0;
    src->len  = 0;
    src->ptr  = NULL;
}

/**
 * Reserve space for at least @p size elements in @p str string
 * (including null byte at the end).
 *
 * @param str           TE string.
 * @param size          Number of elements to to have a room for in a string
 *
 * @return 0
 *
 * @remark If there is a room already for the number of elements specified
 *         by @p size, no action would be performed.
 *
 * @note The function never returns an error. Its return type is not void
 *       for legacy reasons. New code should never check the return value.
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
 * Format arguments according to @p fmt and append the result to the string.
 *
 * @param str           TE string
 * @param fmt           Format string
 * @param ...           Format string arguments
 *
 * @return 0
 *
 * @note The function never returns an error. Its return type is not void
 *       for legacy reasons. New code should never check the return value.
 */
extern te_errno te_string_append(te_string *str, const char *fmt, ...);

/**
 * Format the varargs according to @p fmt and append the result to the string.
 *
 * @param str           TE string
 * @param fmt           Format string
 * @param ap            List of arguments
 *
 * @return 0 (see te_string_append() for explanation)
 */
extern te_errno te_string_append_va(te_string  *str,
                                    const char *fmt,
                                    va_list     ap);

/**
 * Format arguments according to @p fmt and append the result to the string.
 *
 * @param str           TE string
 * @param fmt           Format string
 * @param ...           Format string arguments
 *
 * @return Status code
 * @retval TE_ENOBUFS   The string has an external buffer and
 *                      it does not have enough space.
 *
 * @note This function is intended for special use cases, where
 *       a caller is ready to deal with static buffers of insufficient
 *       size in some sensible manner. Normally, te_string_append()
 *       should be used instead. Other than the possible error code,
 *       the two functions are identical.
 */
extern te_errno te_string_append_chk(te_string *str, const char *fmt, ...);

/**
 * Format the varargs according to @p fmt and append the result to the string.
 *
 * @param str           TE string
 * @param fmt           Format string
 * @param ap            List of arguments
 *
 * @return Status code
 * @retval TE_ENOBUFS   See te_string_append_chk() for explanation.
 */
extern te_errno te_string_append_va_chk(te_string  *str,
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
 * @return 0 (see te_string_append() for explanation)
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
 * @return 0 (see te_string_append() for explanation)
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
 * @return 0 (see te_string_append() for explanation)
 */
extern te_errno te_string_append_shell_arg_as_is(te_string *str,
                                                 const char *arg);

/** URI escaping modes suitable for various parts of URI. */
typedef enum te_string_uri_escape_mode {
    /**
     * Basic escaping.
     *
     * Only RFC3986 "unreserved" characters are allowed.
     */
    TE_STRING_URI_ESCAPE_BASE,
    /** Escaping for the userinfo part. */
    TE_STRING_URI_ESCAPE_USER,
    /** Escaping for the host part. */
    TE_STRING_URI_ESCAPE_HOST,
    /**
     * Escaping for the path segment.
     *
     * This means that a path separator @c / would be escaped.
     */
    TE_STRING_URI_ESCAPE_PATH_SEGMENT,
    /** Escaping for the path component as a whole. */
    TE_STRING_URI_ESCAPE_PATH,
    /** Escaping for the query string as a whole. */
    TE_STRING_URI_ESCAPE_QUERY,
    /**
     * Escaping for the query keys and values.
     *
     * This means that @c =, ampersands and semicolons are escaped.
     */
    TE_STRING_URI_ESCAPE_QUERY_VALUE,
    /** Escaping for the fragment component. */
    TE_STRING_URI_ESCAPE_FRAG,
} te_string_uri_escape_mode;

/**
 * Append a part of an URI escaping all the characters that are not
 * acceptable in given @p mode.
 *
 * The unacceptable characters are percent-encoded as per RFC3986.
 *
 * @param str   TE string
 * @param mode  escaping mode
 * @param arg   source value
 *
 * @note The exact escaping rules of RFC3987 are a bit more subtle,
 *       so in theory it is possible to construct an invalid URI
 *       using this function, however, it is very unlikely for any
 *       practical usecase.
 */
extern void te_string_append_escape_uri(te_string *str,
                                        te_string_uri_escape_mode mode,
                                        const char *arg);

/**
 * Append the elements of @p vec (which must be C strings),
 * separated by @p sep. @c NULL elements are ignored.
 *
 * @param str     TE string
 * @param strvec  Vector of C strings
 * @param sep     Separator
 *
 * @return 0
 *
 * @note The function never returns an error. Its return type is not void
 *       for legacy reasons. New code should never check the return value.
 */
extern te_errno te_string_join_vec(te_string *str, const te_vec *strvec,
                                   const char *sep);

/**
 * Append the elements of @p vec (which must be C strings),
 * escaped as URI path segments, separated by @c /.
 *
 * @param str     TE string
 * @param strvec  vector of C strings
 *
 * @note The leading @c / is not added, to allow building relative URIs.
 *
 * @sa TE_STRING_URI_ESCAPE_PATH_SEGMENT
 */
extern void te_string_join_uri_path(te_string *str, const te_vec *strvec);

/**
 * Build an URI of parts and append it to @p str.
 *
 * If any of the components is @c NULL (including @p scheme), it is
 * omitted together with a corresponding separator.
 *
 * @param str      TE string
 * @param scheme   URI scheme
 * @param userinfo user info (escaped)
 * @param host     host (escaped)
 * @param port     port (@c 0 meaning no port)
 * @param path     path (not escaped)
 * @param query    query (not escaped)
 * @param frag     escaped
 *
 * @exception TE_FATAL_ERROR if @p scheme, @p path or @p query contains
 *            invalid characters.
 *
 * @note @p path and @p query are *not* automatically escaped, because
 *       the exact escaping rules depend on whether they are treated as
 *       monolithic strings or compound objects. Therefore the caller is
 *       responsible for providing correct escaping e.g by using
 *       te_string_append_escape_uri(), te_string_join_vec(),
 *       te_kvpair_to_uri_query() or in some other way.
 *
 * @note Leading @c / is added to @p path if @p host is present, and
 *       @p path does not already start with @c /.
 */
extern void te_string_build_uri(te_string *str, const char *scheme,
                                const char *userinfo, const char *host,
                                uint16_t port, const char *path,
                                const char *query, const char *frag);

/**
 * Type for character escaping functions.
 *
 * A function is expected to append some representation of @p c
 * to @p str. The representation is allowed to be empty, i.e.
 * the function may swallow some input characters.
 *
 * @param str TE string
 * @param c   input character
 */
typedef void te_string_generic_escape_fn(te_string *str, char c);

/**
 * Generic string escaping function.
 *
 * All characters from @p input are copied unchanged to @p str, except:
 * - if the character has a non-@c NULL entry in @p esctable,
 *   it is substituted;
 * - if @p ctrl_esc is not @c NULL and the character is a control character,
 *   @p ctrl_esc is used to write the representation of the character;
 * - if @p nonascii_esc is not @c NULL and the character is non-ASCII
 *   (i.e. its code is larger than @c 127), @p nonascii_esc is used to
 *   write the representation of the character.
 *
 * @param str           TE string
 * @param input         input string
 * @param esctable      table of escape sequences
 * @param ctrl_esc      control character escaping function
 *                      (may be @c NULL)
 * @param nonascii_esc  non-ASCII character escaping function
 *                      (may be @c NULL)
 */
extern void te_string_generic_escape(te_string *str, const char *input,
                                     const char *esctable[static UINT8_MAX + 1],
                                     te_string_generic_escape_fn *ctrl_esc,
                                     te_string_generic_escape_fn *nonascii_esc);

/**
 * Encode binary data with Base-64 encoding.
 *
 * The encoding is defined in RFC 4648. Lines are not split.
 *
 * If @p url_safe is @c TRUE, the so called URL-safe variant of RFC 4648
 * is used which produces strings that may be directed included into an URI
 * without additional escaping. Also they may be safely used as filenames.
 *
 * @param str       TE string
 * @param len       size of binary data
 * @param bytes     raw binary data
 * @param url_safe  a variant of RFC 4648
 */
extern void te_string_encode_base64(te_string *str, size_t len,
                                    const uint8_t bytes[len],
                                    te_bool url_safe);

/**
 * Decode a Base64-encoded string.
 *
 * Both variants of RFC 4648 encoding are accepted. Embedded newlines
 * are silently skipped.
 *
 * @param str         TE string
 * @param base64str   Base64-encoded string
 *
 * @return status code
 * @retval TE_EILSEQ  Invalid Base64 encoding
 *
 * @note @p str will always be zero-terminated, but it may contain
 *       embedded zeroes.
 */
extern te_errno te_string_decode_base64(te_string *str, const char *base64str);

/**
 * Return a char * that is a result of sprintf into allocated memory.
 *
 * @param fmt       Format string
 * @param ap        List of arguments
 *
 * @return Heap-allocated string.
 */
extern char *te_string_fmt_va(const char *fmt,
                              va_list     ap);
/**
 * Return a char * that is a result of sprintf into allocated memory.
 *
 * @param fmt       Format string
 * @param ...       Format string arguments
 *
 * @return Heap-allocated string.
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
 * Chop off trailing characters from @p str that belong to @p trail.
 *
 * For example, this function may be used to remove trailing newlines
 * from the contents of a file:
 * @code
 *     te_string_chop(dest, "\n");
 * @endcode
 *
 * @param str           TE string.
 * @param trail         Trailing characters to chop.
 */
extern void te_string_chop(te_string *str, const char *trail);

/**
 * Center the string @p src padding it to @p padlen with @p padchar and
 * append the result to @p str.
 *
 * @param str           TE string
 * @param src           Source C string
 * @param padlen        Desired length (if the original string is longer,
 *                      it will be truncated)
 * @param padchar       Padding character
 */
extern void te_string_add_centered(te_string *str, const char *src,
                                   size_t padlen, char padchar);

/**
 * Function type for handlers called by te_string_process_lines().
 *
 * The function may freely modify @p line, but it must treat it
 * as a pointer to a local buffer, that is it must not try to free()
 * or realloc() it, nor store it outside of the scope.
 *
 * @param line          Line buffer without a newline terminator.
 * @param user          User data.
 *
 * @return Status code.
 * @retval TE_EOK te_string_process_lines() will stop immediately and
 *                return success to the caller.
 */
typedef te_errno te_string_line_handler_fn(char *line, void *user);

/**
 * Call @p callback for every line in @p buffer.
 *
 * If @p complete_lines is true, the last incomplete line without
 * a newline terminator is not processed.
 *
 * The line terminator is @c '\n', however, if it is preceded by @c '\r',
 * it is removed as well.
 *
 * All processed lines are removed from @p buffer.
 *
 * Empty lines are never skipped, but if the @p buffer is empty,
 * @p callback is not called at all.
 *
 * If a @p callback returns a non-zero status, the processing stop,
 * however, the current line is still removed. TE_EOK is treated as
 * success.
 *
 * @par Example.
 *
 * The following snippet would read data from a POSIX fd in arbitrary
 * chunks, split the input into lines, count them, log any occurred error
 * messages and bail out early on fatal errors.
 *
 * @code
 * static te_errno
 * handle_line(char *line, void *data)
 * {
 *     unsigned int *linecount = data;
 *     const char *msg = te_str_strip_prefix(line, "FATAL: ");
 *
 *     if (msg != NULL)
 *     {
 *         ERROR("Fatal error at line %u: %s", *linecount, msg);
 *         return TE_EBADMSG;
 *     }
 *
 *     msg = te_str_strip_prefix(line, "ERROR: ");
 *     if (msg != NULL)
 *         ERROR("Error at line %u: %s", *linecount, msg);
 *
 *     (*linecount)++;
 *     return 0;
 * }
 *
 * ...
 *
 * te_string buffer = TE_STRING_INIT;
 * char buf[BUFSIZE];
 * ssize_t read_bytes;
 * unsigned int linecount = 1;
 *
 * while ((read_bytes = read(fd, buf, sizeof(buf))) > 0)
 * {
 *     te_string_append_buf(&buffer, buf, read_bytes);
 *     CHECK_RC(te_string_process_lines(&buffer, TRUE,
 *                                      handle_line, data));
 * }
 * CHECK_RC(te_string_process_lines(&buffer, FALSE, handle_line, &linecount));
 * @endcode
 *
 * @param[in,out] buffer          TE string.
 * @param[in]     complete_lines  Do not process incomplete lines
 *                                if @c TRUE.
 * @param[in]     callback        Callback function.
 * @param[in]     user_data       User data.
 *
 * @return Status code.
 */
extern te_errno te_string_process_lines(te_string *buffer,
                                        te_bool complete_lines,
                                        te_string_line_handler_fn *callback,
                                        void *user_data);

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
 * Get string representation of raw data.
 *
 * @param data  Buffer
 * @param size  Number of bytes
 *
 * @return String representation
 */
extern char *raw2string(const uint8_t *data, size_t size);

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
 * @return Status code.
 * @retval TE_EINVAL  Substring position is out of bounds.
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
 * @param str The string in which to replace.
 * @param new The new substring to replace.
 * @param old The substring to be replaced.
 *
 * @return Status code (always 0).
 */
extern te_errno te_string_replace_all_substrings(te_string *str,
                                                 const char *new,
                                                 const char *old);

/**
 * Replace the substring in a string
 *
 * @param str The string in which to replace.
 * @param new The new substring to replace.
 * @param old The substring to be replaced.
 *
 * @return Status code (always 0).
 */
extern te_errno te_string_replace_substring(te_string *str,
                                            const char *new,
                                            const char *old);
#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_STRING_H__ */
/**@} <!-- END te_tools_te_string --> */
