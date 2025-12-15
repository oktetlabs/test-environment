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
    bool ext_buf;      /**< If @c true, buffer is supplied
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
    { .ptr = NULL, .size = (reserved_size_), .len = 0, .ext_buf = false, \
      .free_func = &te_string_free_heap }

/**
 * On-stack te_string initializer with a defined reserve and
 * free function.
 */
#define TE_STRING_INIT_RESERVE_FREE(reserved_size_, free_func_) \
    { .ptr = NULL, .size = (reserved_size_), .len = 0, .ext_buf = false, \
      .free_func = free_func_ }

/**
 * Initialize TE string assigning buffer to it.
 */
#define TE_STRING_BUF_INIT(buf_) \
    { .ptr = buf_, .size = sizeof(buf_), .len = 0, .ext_buf = true, \
      .free_func = &te_string_reset }

/**
 * Initialize TE string assigning buffer and size to it.
 */
#define TE_STRING_EXT_BUF_INIT(buf_, size_) \
    { .ptr = buf_, .size = size_, .len = 0, .ext_buf = true, \
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
 * Initialize a TE string with a pointer to a sequence of bytes.
 *
 * The produced TE string should only be used in a readonly manner,
 * so it is mostly useful in conjunction with #te_substring API.
 *
 * The reserved size is intentionally set to a zero so that any
 * attempt to extend the TE string would immediately result in
 * a failure.
 *
 * As the length of the string is explicitly specified by @p len_,
 * @p buf_ may contain embedded zero characters. However, it is
 * advised that there be a terminating zero right after @p buf_
 * content (this is neither checked nor enforced).
 *
 * @param buf_      A pointer to a sequence of bytes
 *                  (may contain embedded zeroes)
 */
#define TE_STRING_INIT_RO_PTR_BYTES(len_, buf_) \
    { .ptr = TE_CONST_PTR_CAST(char, buf_),     \
      .size = 0, .len = (len_),                 \
      .ext_buf = true,                          \
      .free_func = &te_string_reset }

/**
 * Initialize a TE string with a pointer to a plain C string.
 *
 * Like #TE_STRING_INIT_RO_PTR_BYTES, but it expects a zero-terminated
 * string with no embedded zeroes, so its length may be automatically
 * calculated.
 *
 * @param str_      A pointer to a plain C string (null-terminated).
 */
#define TE_STRING_INIT_RO_PTR(str_) \
    TE_STRING_INIT_RO_PTR_BYTES(strlen(str_), str_)

/**
 * Reset TE string (mark its empty).
 *
 * @param str           TE string.
 */
static inline void
te_string_reset(te_string *str)
{
    str->len = 0;
    /*
     * TODO: This check should be made simpler,
     * once TE_STRING_INIT_RESERVE() issues
     * are resolved
     */
    if (str->ptr != NULL && str->size > 0)
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
 */
extern void te_string_append(te_string *str, const char *fmt, ...);

/**
 * Format the varargs according to @p fmt and append the result to the string.
 *
 * @param str           TE string
 * @param fmt           Format string
 * @param ap            List of arguments
 */
extern void te_string_append_va(te_string  *str,
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
 * Used by functions like te_string_replace() to indicate that a starting
 * position should be calculated from other values, such as the length of
 * the string and/or the length of a segment.
 */
#define TE_STRING_POS_AUTO SIZE_MAX

/**
 * Format arguments according to @p fmt and replace a segment
 * of @p str with the result.
 *
 * Refer to te_string_replace_buf() for the description of
 * @p start and @p len handling.
 *
 * If @p fmt is @c NULL, the segment is deleted.
 *
 * @param str           TE string
 * @param seg_start     Start of the segment to replace
 * @param seg_len       Length of the segment to replace
 * @param fmt           Format string (may be @c NULL!)
 * @param ...           Format string arguments
 *
 * @return The length of the replacement string
 */
extern size_t te_string_replace(te_string *str,
                                size_t seg_start, size_t seg_len,
                                const char *fmt, ...) TE_LIKE_PRINTF(4, 5);

/**
 * Format the varargs according to @p fmt and replace a segment
 * of @p str with the result.
 *
 * The function is the same as te_string_replace() save for
 * the different varargs signature.
 *
 * @param str           TE string
 * @param fmt           Format string (may be @c NULL)
 * @param ap            List of arguments
 *
 * @return The length of the replacement string
 */
extern size_t te_string_replace_va(te_string *str,
                                   size_t seg_start, size_t seg_len,
                                   const char *fmt,
                                   va_list ap) TE_LIKE_VPRINTF(4);

/**
 * Replace a segment within a string.
 *
 * If @p seg_start is #TE_STRING_POS_AUTO, the starting point
 * is @p len bytes before the end of @p str.
 *
 * If @p seg_start is beyond the end of @p str,
 * the string is first padded by binary zeroes to end at
 * @p seg_start and then then contents of @p buf is appended to it,
 * irrespective of the value of @p seg_len. So if @p seg_start
 * points right after the end of string, this function behaves
 * exactly like te_string_append_buf().
 *
 * If @p seg_start + @p seg_len is beyond the end of @p str,
 * the whole suffix of @p str starting at @p seg_start is
 * replaced.
 *
 * If @p buf is @c NULL, a block of @p buf_len binary
 * zeroes is inserted.
 *
 * @param str         TE string
 * @param seg_start   Start of the segment to replace
 * @param seg_len     Length of the segment to replace
 * @param buf         Buffer (may be @c NULL)
 * @param buf_len     Length of the buffer
 */
extern void te_string_replace_buf(te_string *str,
                                  size_t seg_start, size_t seg_len,
                                  const char *buf, size_t buf_len);

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
                                                  TE_REQUIRE_SENTINEL;

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
 * If @p url_safe is @c true, the so called URL-safe variant of RFC 4648
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
                                    bool url_safe);

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
                           ...) TE_LIKE_PRINTF(1, 2);

/**
 * Cut from the string specified number of characters.
 *
 * @param str           TE string
 * @param len           Number of characters to cut
 */
static inline void
te_string_cut(te_string *str, size_t len)
{
    te_string_replace_buf(str, TE_STRING_POS_AUTO, len, NULL, 0);
}

/**
 * Cut specified number of characters from the beginning of the string.
 *
 * @param str           TE string
 * @param len           Number of characters to cut from the beginning
 */
static inline void
te_string_cut_beginning(te_string *str, size_t len)
{
    te_string_replace_buf(str, 0, len, NULL, 0);
}

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
 *     CHECK_RC(te_string_process_lines(&buffer, true,
 *                                      handle_line, data));
 * }
 * CHECK_RC(te_string_process_lines(&buffer, false, handle_line, &linecount));
 * @endcode
 *
 * @param[in,out] buffer          TE string.
 * @param[in]     complete_lines  Do not process incomplete lines
 *                                if @c true.
 * @param[in]     callback        Callback function.
 * @param[in]     user_data       User data.
 *
 * @return Status code.
 */
extern te_errno te_string_process_lines(te_string *buffer,
                                        bool complete_lines,
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
 * @param base_ Pointer to the base string.
 *
 * @bug @p base_ may be a pointer to const #te_string without
 *      a warning from the compiler, but in this case a user
 *      shall not call te_substring_replace() on it and currenly
 *      this is not enforced --- but that may be fixed later.
 */
#define TE_SUBSTRING_INIT(base_) \
    (te_substring_t){                                                   \
        .base = TE_CONST_PTR_CAST(te_string, (base_)),                  \
        .len = 0,                                                       \
        .start = 0                                                      \
    }

/**
 * Invalidate the substring so that it would be
 * treated as pointing nowhere.
 *
 * @param substr Substring.
 */
static inline void
te_substring_invalidate(te_substring_t *substr)
{
    if (substr != NULL)
    {
        substr->start = SIZE_MAX;
        substr->len = 0;
    }
}

/**
 * Check that substring is valid.
 *
 * The substring is considered valid if it is completely
 * contained within its base string or if it has a length of
 * zero and points right after the end of its base string.
 *
 * @param substr Substring.
 *
 * @return @c true if the substring is valid.
 */
static inline bool
te_substring_is_valid(const te_substring_t *substr)
{
    return substr != NULL && substr->base != NULL &&
        substr->start <= substr->base->len &&
        substr->start + substr->len <= substr->base->len;
}

/**
 * Check whether a substring points past the end of its base string.
 *
 * If it is true, replacing a substring would effectively append data
 * to the base string.
 *
 * @param substr Substring.
 *
 * @return @c true if the substring is valid and points past
 *         the end of its base string.
 */
static inline bool
te_substring_past_end(const te_substring_t *substr)
{
    return te_substring_is_valid(substr) && substr->start == substr->base->len;
}

/**
 * Extend the length of the substring to reach the end of its base string.
 *
 * @param substr Substring.
 */
static inline void
te_substring_till_end(te_substring_t *substr)
{
    if (te_substring_is_valid(substr))
        substr->len = substr->base->len - substr->start;
}

/**
 * Copy the content of a substring into the C array.
 *
 * The terminating @c '\0' is added to the array, so it
 * should have enough space to hold @c src->len + 1 bytes.
 *
 * If the substring is not valid, a single @c '\0' is written
 * to the buffer.
 *
 * @param dst  Target buffer.
 * @param src  Substring.
 *
 * @return @c true if @p src is a valid substring.
 */
static inline bool
te_substring_extract_buf(char *dst, const te_substring_t *src)
{
    if (!te_substring_is_valid(src))
    {
        *dst = '\0';
        return false;
    }

    memcpy(dst, src->base->ptr + src->start, src->len);
    dst[src->len] = '\0';

    return true;
}

/**
 * Copy the content of a substring into the target string.
 *
 * If the substring is not valid, nothing happens.
 *
 * @param dst  Target string.
 * @param src  Substring.
 *
 * @return @c true if @p src is a valid substring.
 *
 * @note The function returns true if the substring is valid but empty,
 *       so nothing is actually copied.
 */
static inline bool
te_substring_extract(te_string *dst, const te_substring_t *src)
{
    if (!te_substring_is_valid(src))
        return false;

    te_string_append_buf(dst, src->base->ptr + src->start, src->len);
    return true;
}

/**
 * Compare two substrings like @c strcmp().
 *
 * An invalid substring always compares equal to another invalid substring
 * and is considered less than any valid substring.
 *
 * @param substr1  First substring.
 * @param substr2  Second substring.
 *
 * @return 0, -1 or 1 depending on whether @p substr1 is equal, less or
 *         greater than @p substr2.
 */
extern int te_substring_compare(const te_substring_t *substr1,
                                const te_substring_t *substr2);


/**
 * Compare a substring to a C string like @c strcmp().
 *
 * An invalid substring compares equal to @c NULL and is less than
 * any non-null string. A valid substring is always greater than
 * @c NULL.
 *
 * @param substr   Substring.
 * @param str      C string to compare (may be @c NULL).
 *
 * @return 0, -1 or 1 depending on whether @p substr is equal, less or
 *         greater than @p str.
 */
extern int te_substring_compare_str(const te_substring_t *substr,
                                    const char *str);

/**
 * Find a @p str starting at @p substr position and update it accordingly.
 *
 * @param substr Substring.
 * @param str    The string to find.
 *
 * @return @c true if @p str has been found.
 *
 * @note If @p str has not been found, te_substring_is_valid() will also
 *       return @c false after this call.
 *
 * @note If te_substring_find() is called the second time without calling
 *       te_substring_advance(), it will effectively do nothing.
 */
extern bool te_substring_find(te_substring_t *substr, const char *str);

/**
 * Make the substring cover the longset segment of characters entirely
 * from @p cset (or entirely *not* from @p cset if @p inverted is @c true).
 *
 * The starting point of the substring is not changed.
 *
 * @param substr    Substring.
 * @param cset      Set of characters.
 * @param inverted  If @c true, consider characters not in @p cset.
 *
 * @return The first character after the initial segment, may be @c '\0'.
 */
extern char te_substring_span(te_substring_t *substr, const char *cset,
                              bool inverted);


/**
 * Skip at most @p at_most characters @p skip in @p substr.
 *
 * Unlike te_substring_span(), this function does move the starting
 * point and the length is decreased if it's not zero.
 *
 * @return The number of characters actually skipped.
 */
extern size_t te_substring_skip(te_substring_t *substr, char skip,
                                size_t at_most);

/**
 * Strip a prefix from a substring.
 *
 * If @p substr starts with @p prefix, its starting point
 * is moved to skip that prefix.
 *
 * The underlying string is not modified.
 *
 * @param[in,out] substr  Substring.
 * @param[in]     prefix  Prefix to strip.
 *
 * @return @c true if @p prefix has been stripped.
 *
 * @sa te_str_strip_prefix()
 */
extern bool te_substring_strip_prefix(te_substring_t *substr,
                                      const char *prefix);

/**
 * Strip a suffix from a subsstring.
 *
 * If @p substr ends with @p suffix, the length of the suffix
 * is substracted from the length of the substring.
 *
 * The underlying string is not modified.
 *
 * @param[in,out] substr  Substring.
 * @param[in]     suffix  Suffix to strip.
 *
 * @return @c true if @p suffix has been stripped.
 */
extern bool te_substring_strip_suffix(te_substring_t *substr,
                                      const char *suffix);

/**
 * Strip a sequence of digits from the end of @p substring.
 *
 * If the sequence is not empty, the resulting number is
 * stored in @p suffix_val and the length of @p substr is
 * diminished to exclude the numeric suffix.
 *
 * Otherwise, @p substr is untouched and @c 0 is stored in @p suffix_val.
 *
 * If the numeric suffix represent a number that does not fit
 * into @c uintmax_t, it won't be stripped.
 *
 * The underlying string is never modified.
 *
 * @param[in,out] substr      Substring.
 * @param[out]    suffix_val  Place to store the numeric value of
 *                            a suffix or zero (may be @c NULL).
 *
 * @return @c true if any digits have been stripped off.
 */
extern bool te_substring_strip_uint_suffix(te_substring_t *str,
                                           uintmax_t *suffix_val);

/**
 * The mode of operation for substring modifications.
 */
typedef enum te_substring_mod_op {
    /** Prepend a new string to the content of a substring. */
    TE_SUBSTRING_MOD_OP_PREPEND,
    /** Append a new string to the content of a substring. */
    TE_SUBSTRING_MOD_OP_APPEND,
    /** Replace a substring with a new string. */
    TE_SUBSTRING_MOD_OP_REPLACE,
} te_substring_mod_op;

/**
 * Modify a substring at a given position, changing
 * the underlying #te_string in place. If the substring is not valid,
 * nothing happens. The exact behaviour depends on the value of @p op:
 * - #TE_SUBSTRING_MOD_OP_PREPEND: the replacement string is inserted
 *                                 at the beginning of the substring;
 * - #TE_SUBSTRING_MOD_OP_APPEND: the replacement string is inserted
 *                                after the end of the substring;
 * - #TE_SUBSTRING_MOD_OP_REPLACE: the replacement string is inserted
 *                                 in place of the whole substring.
 *
 * The replacement string is constructed by applying @c printf()
 * format to the arguments.
 *
 * If @p fmt is @c NULL, the content of the substring is deleted
 * (if @p op is #TE_SUBSTRING_MOD_OP_REPLACE, otherwise it's a no-op).
 * No variadic arguments shall be present in this case.
 *
 * The starting point of the substring remains the same in all cases
 * and the length is adjusted according to the operation and the length
 * of the replacement string (in contrast to te_substring_replace()).
 *
 * @param substr Substring.
 * @param op     Mode of operation.
 * @param fmt    Replacement format (may be @c NULL).
 * @param ...    Format arguments.
 *
 * @return The length of the replacement string.
 *         Note that this does *not* include the length of the old substring
 *         in case of appending/prepending.
 */

extern size_t te_substring_modify(te_substring_t *substr,
                                  te_substring_mod_op op,
                                  const char *fmt, ...) TE_LIKE_PRINTF(3, 4);

/**
 * Same as te_substring_modify() but accepts a variadic list argument.
 * If @p fmt is @c NULL, the content of the substring is deleted
 * (when @p op is #TE_SUBSTRING_MOD_OP_REPLACE).
 *
 * @param substr Substring.
 * @param op     Mode of operation.
 * @param fmt    Replacement format (may be @c NULL).
 * @param ...    Format arguments.
 *
 * @return The length of the replacement string.
 */
extern size_t te_substring_modify_va(te_substring_t *substr,
                                     te_substring_mod_op op,
                                     const char *fmt,
                                     va_list args) TE_LIKE_VPRINTF(3);

/**
 * Like te_substring_modify(), but the operation is always
 * #TE_SUBSTRING_MOD_OP_REPLACE.
 *
 * Unlike te_substring_modify(), this function moves the starting
 * point past the end of the replaced string and sets the substring
 * length to zero, so it's basically equivalent to calling
 * te_substring_advance() after te_substring_modify().
 *
 * @param substr Substring.
 * @param fmt    Replacement format (may be @c NULL).
 * @param ...    Format arguments.
 *
 * @return The length of the replacement string.
 *
 * @since 1.45.0. Before that version, it accepted a fixed replacement
 *        string, not a format. It also used to return a pretty
 *        uninformative status code instead of the length.
 */
extern size_t te_substring_replace(te_substring_t *substr,
                                   const char *fmt, ...) TE_LIKE_PRINTF(2, 3);

/**
 * Same as te_substring_replace() but accepts a variadic list argument.
 *
 * @param substr Substring.
 * @param fmt    Replacement format (may be @c NULL).
 * @param ...    Format arguments.
 *
 * @return The length of the replacement string.
 */
extern size_t te_substring_replace_va(te_substring_t *substr,
                                      const char *fmt,
                                      va_list args) TE_LIKE_VPRINTF(2);


/**
 * Inserts a separator at the start of the substring if there is no one
 * already.
 *
 * The function checks the character immediately preceding the substring.
 * If @p at_bol is true, the separator is also inserted if there is no
 * preceding character at all.
 *
 * @param substr  Substring.
 * @param sep     Separator.
 * @param at_bol  If true, insert at the start of the string.
 *
 * @return @c true if the separator has been
 */
extern bool te_substring_insert_sep(te_substring_t *substr,
                                    char sep, bool at_bol);


/**
 * Copy the content of a substring into another substring.
 *
 * If @p dst is invalid, no copying is done.
 * If @p src is invalid, the content of @p dst is deleted.
 *
 * @p dst and @p src must have *different* base strings.
 *
 * The mode of operation is the same as for te_substring_modify().
 *
 * @param dst  Target substring.
 * @param src  Source substring.
 * @param op   Modification mode.
 *
 * @return @c true if copying took place.
 *
 * @exception #TE_FATAL_ERROR if @p dst and @p src refer to
 *            the same string. This may change in the future.
 */
extern bool te_substring_copy(te_substring_t *dst,
                              const te_substring_t *src,
                              te_substring_mod_op op);

/**
 * Move the position of a substring by its length.
 *
 * In other words, after this call the substring will point
 * after the point where the substring ended initially.
 * If the string was not valid initially, nothing happens.
 *
 * @param substr Substring.
 *
 * @return @c true if the substring has been advanced.
 */
extern bool te_substring_advance(te_substring_t *substr);

/**
 * Limit the length of the @p substr so that it would end
 * right before the start of @p limit.
 *
 * If any of the substrings is not valid of if they have
 * different base strings or if @p limit starts before
 * @p substr, nothing happens.
 *
 * @param substr Substring.
 * @param limit  Limiting substring.
 *
 * @return @c true if the substring has been limited.
 */
extern bool te_substring_limit(te_substring_t *substr,
                               const te_substring_t *limit);

/**
 * Replace all occurrences of a substring in a string.
 *
 * @param str The string in which to replace.
 * @param new The replacement string (may be @c NULL).
 * @param old The substring to be replaced.
 *
 * @return The number of performed replacements.
 *
 * @since 1.45.0. Before that version the function returned
 *        a meaningless status code instead of the count.
 */
extern size_t te_string_replace_all_substrings(te_string *str,
                                               const char *new,
                                               const char *old);

/**
 * Replace the first occurrence of substring in a string.
 *
 * @param str The string in which to replace.
 * @param new The replacement (may be @c NULL).
 * @param old The substring to be replaced.
 *
 * @return @c true if a substring has been replaced.
 *
 * @since 1.45.0. Before that version the function returned
 *        a meaningless status code instead of the success
 *        flag.
 */
extern bool te_string_replace_substring(te_string *str,
                                        const char *new,
                                        const char *old);

/**@} <!-- END te_tools_te_substring --> */

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_STRING_H__ */
/**@} <!-- END te_tools_te_string --> */
