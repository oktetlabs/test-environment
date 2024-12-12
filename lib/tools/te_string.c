/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Testing Results Comparator: common
 *
 * Routines to work with strings.
 *
 *
 * Copyright (C) 2004-2023 OKTET Labs Ltd. All rights reserved.
 */

#include "te_config.h"

#ifdef STDC_HEADERS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#endif

#include "logger_api.h"

#include "te_string.h"
#include "te_intset.h"
#include "te_defs.h"
#include "te_alloc.h"

void
te_string_free_heap(te_string *str)
{
    str->len = str->size = 0;
    free(str->ptr);
    str->ptr = NULL;
}

void
te_string_free(te_string *str)
{
    if (str == NULL)
        return;

    if (str->free_func == NULL)
    {
        ERROR("%s(): free_func must not be NULL. te_string_free_heap() will "
              "be used but you must fix your code initializing te_string "
              "properly!", __FUNCTION__);
        te_string_free_heap(str);
        return;
    }

    str->free_func(str);
}

te_errno
te_string_append(te_string *str, const char *fmt, ...)
{
    va_list  ap;
    te_errno rc;

    va_start(ap, fmt);
    rc = te_string_append_va(str, fmt, ap);
    va_end(ap);
    return rc;
}

te_errno
te_string_append_chk(te_string *str, const char *fmt, ...)
{
    va_list  ap;
    te_errno rc;

    va_start(ap, fmt);
    rc = te_string_append_va_chk(str, fmt, ap);
    va_end(ap);
    return rc;
}


/* See description in te_string.h */
te_errno
te_string_reserve(te_string *str, size_t size)
{
    size_t          pagesize = getpagesize();
    size_t          adj_size;
    size_t          malloc_header_size = 4 * sizeof(void *);
    size_t          grow_factor = 1;
    size_t          grow = 0;

    /*
     * Here we don't reinvent the wheel, we comply to the GCC C++ library
     * https://github.com/gcc-mirror/gcc/blob/master/libstdc++-v3/include/
     *  bits/basic_string.tcc
     * with the exception of grow factor exponent used.
     */
    if (size <= str->size)
        return 0;

    if (str->ext_buf)
        TE_FATAL_ERROR("cannot resize external buffer");

    /*
     * Apply grow factor ^ exp until predefined limit, and if size < newsize <
     * (factor ^ exp) * size, then use (factor ^ exp) * size as a resulting
     * size.
     *
     * Using factor ^ exp might be costly in terms of RAM used, so we fall back
     * to a regular addend-based expansion if we can't find it even after
     * applying @c TE_STRING_GROW_FACTOR_EXP_LIMIT exponent
     */
    for (grow = 0; grow < TE_STRING_GROW_FACTOR_EXP_LIMIT; ++grow)
    {
        grow_factor *= TE_STRING_GROW_FACTOR;
        if (size < grow_factor * str->size)
        {
            size = grow_factor * str->size;
            break;
        }
    }

    /*
     * Apply correction taking malloc overhead into account, it works for
     * allocations over page size. Based on GCC C++ basic_string implementation.
     */
    adj_size = size + malloc_header_size;
    if (adj_size > pagesize)
    {
        size_t extra;

        extra = pagesize - adj_size % pagesize;
        size += extra;
    }

    /* Actually reallocate data */
    TE_REALLOC(str->ptr, size);

    str->size = size;
    return 0;
}

te_errno
te_string_append_va_chk(te_string *str, const char *fmt, va_list ap)
{
    char    *s;
    size_t   rest;
    int      printed;
    bool again;
    va_list  ap_start;

    if (str->ptr == NULL)
    {
        size_t new_size;

        assert(!str->ext_buf);

        new_size = str->size != 0 ? str->size : TE_STRING_INIT_LEN;
        str->ptr = TE_ALLOC_UNINITIALIZED(new_size);

        str->size = new_size;
        str->len = 0;
    }

    assert(str->size > str->len);
    rest = str->size - str->len;
    do  {
        s = str->ptr + str->len;

        va_copy(ap_start, ap);
        printed = vsnprintf(s, rest, fmt, ap_start);
        va_end(ap_start);
        assert(printed >= 0);

        if ((size_t)printed >= rest)
        {
            if (str->ext_buf)
            {
                str->len = str->size - 1 /* '\0' */;
                return TE_ENOBUFS;
            }
            else
            {
                te_string_reserve(str, str->len + printed + 1 /* '\0' */);

                rest = str->size - str->len;
                /* Print again */
                again = true;
            }
        }
        else
        {
            str->len += printed;
            again = false;
        }
    } while (again);

    return 0;
}

te_errno
te_string_append_va(te_string *str, const char *fmt, va_list ap)
{
    te_errno rc = te_string_append_va_chk(str, fmt, ap);

    if (rc != 0)
        TE_FATAL_ERROR("Not enough space in supplied buffer");

    return 0;
}

te_errno
te_string_append_buf(te_string *str, const char *buf, size_t len)
{
    if (str == NULL || len == 0)
        return 0;

    te_string_reserve(str, str->len + len + 1);

    if (buf == NULL)
        memset(str->ptr + str->len, '\0', len);
    else
        memcpy(str->ptr + str->len, buf, len);
    str->len += len;
    str->ptr[str->len] = '\0';
    return 0;
}

size_t
te_string_replace(te_string *str,
                  size_t seg_start, size_t seg_len,
                  const char *fmt, ...)
{
    size_t rep_len;
    va_list args;

    va_start(args, fmt);
    rep_len = te_string_replace_va(str, seg_start, seg_len, fmt, args);
    va_end(args);

    return rep_len;
}

static void
te_string_replace_va_int(te_string *str,
                         size_t seg_start, size_t seg_len,
                         size_t fmt_len,
                         const char *fmt,
                         va_list ap)
{
    char buf[fmt_len + 1];

    vsnprintf(buf, sizeof(buf), fmt, ap);
    te_string_replace_buf(str, seg_start, seg_len,
                          buf, fmt_len);
}

size_t
te_string_replace_va(te_string *str,
                     size_t seg_start, size_t seg_len,
                     const char *fmt,
                     va_list ap)
{
    int fmt_size;
    va_list ap_copy;

    if (str == NULL)
        return 0;
    if (fmt == NULL)
    {
        te_string_replace_buf(str, seg_start, seg_len, NULL, 0);
        return 0;
    }
    va_copy(ap_copy, ap);
    fmt_size = vsnprintf(NULL, 0, fmt, ap_copy);
    va_end(ap_copy);
    assert(fmt_size >= 0);

    te_string_replace_va_int(str, seg_start, seg_len,
                             (size_t)fmt_size, fmt, ap);

    return (size_t)fmt_size;
}

void
te_string_replace_buf(te_string *str, size_t seg_start, size_t seg_len,
                      const char *buf, size_t buf_len)
{
    if (str == NULL)
        return;

    if (seg_start == TE_STRING_POS_AUTO)
        seg_start = seg_len < str->len ? str->len - seg_len : 0;

    if (seg_start >= str->len)
    {
        te_string_append_buf(str, NULL, seg_start - str->len);
        te_string_append_buf(str, buf, buf_len);
        return;
    }

    /*
     * The first check is to handle gracefully cases where
     * the sum of start and len may overflow (e.g len is SIZE_MAX)
     */
    if (seg_len > str->len || seg_start + seg_len > str->len)
        seg_len = str->len - seg_start;

    te_string_reserve(str, str->len + buf_len - seg_len + 1);
    memmove(str->ptr + seg_start + buf_len,
            str->ptr + seg_start + seg_len,
            str->len - seg_start - seg_len + 1);
    if (buf == NULL)
        memset(str->ptr + seg_start, '\0', buf_len);
    else
        memcpy(str->ptr + seg_start, buf, buf_len);

    str->len -= seg_len;
    str->len += buf_len;
}

te_errno
te_string_append_shell_arg_as_is(te_string *str, const char *arg)
{
    while (true)
    {
        const char *p;

        p = strchr(arg, '\'');
        if (p == NULL)
        {
            te_string_append(str, "'%s'", arg);
            break;
        }
        else
        {
            te_string_append(str, "'%.*s'\\\'",
                             (int)(p - arg), arg);
            arg = p + 1;
        }
    }

    return 0;
}

te_errno
te_string_append_shell_args_as_is(te_string *str, ...)
{
    va_list args;
    const char *arg;

    va_start(args, str);
    while ((arg = va_arg(args, const char *)) != NULL)
    {
        if (str->len != 0)
            te_string_append(str, " ");

        te_string_append_shell_arg_as_is(str, arg);
    }
    va_end(args);

    return 0;
}

static void
make_uri_unescaped_charset(te_charset *cset, te_string_uri_escape_mode mode)
{
#define SUB_DELIMS "!$&'()*+,;="
    static const char *mode_specific[] = {
        [TE_STRING_URI_ESCAPE_BASE] = NULL,
        [TE_STRING_URI_ESCAPE_USER] = SUB_DELIMS ":",
        [TE_STRING_URI_ESCAPE_HOST] = SUB_DELIMS "[]:",
        [TE_STRING_URI_ESCAPE_PATH_SEGMENT] = SUB_DELIMS ":@",
        [TE_STRING_URI_ESCAPE_PATH] = SUB_DELIMS ":@/",
        [TE_STRING_URI_ESCAPE_QUERY] = SUB_DELIMS ":@/?" ,
        [TE_STRING_URI_ESCAPE_QUERY_VALUE] = "!$'()*,:@/?",
        [TE_STRING_URI_ESCAPE_FRAG] = SUB_DELIMS ":@/?",
    };
#undef SUB_DELIMS

    te_charset_clear(cset);

    /* RFC 3986 unreserved characters */
    te_charset_add_range(cset, '0', '9');
    te_charset_add_range(cset, 'a', 'z');
    te_charset_add_range(cset, 'A', 'Z');
    te_charset_add_from_string(cset, "_-.~");

    te_charset_add_from_string(cset, mode_specific[mode]);
}

void
te_string_append_escape_uri(te_string *str, te_string_uri_escape_mode mode,
                            const char *arg)
{
    te_charset unescaped;

    make_uri_unescaped_charset(&unescaped, mode);
    for (; *arg != '\0'; arg++)
    {
        if (te_charset_check(&unescaped, *arg))
            te_string_append(str, "%c", *arg);
        else
            te_string_append(str, "%%%2.2X", (unsigned int)(uint8_t)*arg);
    }
}

te_errno
te_string_join_vec(te_string *str, const te_vec *strvec,
                   const char *sep)
{
    bool need_sep = false;
    const char * const *item;

    TE_VEC_FOREACH(strvec, item)
    {
        /*
         * Seems meaningless to add "(null)" to TE string
         * for NULL pointers.
         */
        if (*item == NULL)
            continue;

        te_string_append(str, "%s%s", need_sep ? sep : "", *item);
        need_sep = true;
    }

    return 0;
}

void
te_string_join_uri_path(te_string *str, const te_vec *strvec)
{
    bool need_sep = false;
    const char * const *item;

    TE_VEC_FOREACH(strvec, item)
    {
        if (need_sep)
            te_string_append(str, "/");
        te_string_append_escape_uri(str, TE_STRING_URI_ESCAPE_PATH_SEGMENT,
                                    *item);
        need_sep = true;
    }
}

void
te_string_build_uri(te_string *str, const char *scheme,
                    const char *userinfo, const char *host, uint16_t port,
                    const char *path, const char *query, const char *frag)
{
    te_charset valid_chars = TE_CHARSET_INIT;

    if (scheme != NULL)
    {
        te_charset_add_range(&valid_chars, 'A', 'Z');
        te_charset_add_range(&valid_chars, 'a', 'z');
        te_charset_add_range(&valid_chars, '0', '9');
        te_charset_add_from_string(&valid_chars, "+-.");

        if (!te_charset_check_bytes(&valid_chars, strlen(scheme),
                                    (const uint8_t *)scheme))
            TE_FATAL_ERROR("Invalid URI scheme: %s", scheme);

        te_string_append(str, "%s:", scheme);
    }

    if (host != NULL)
    {
        te_string_append(str, "//");
        if (userinfo != NULL)
        {
            te_string_append_escape_uri(str, TE_STRING_URI_ESCAPE_USER,
                                        userinfo);
            te_string_append(str, "@");
        }
        te_string_append_escape_uri(str, TE_STRING_URI_ESCAPE_HOST, host);
        if (port != 0)
            te_string_append(str, ":%" PRIu16, port);
    }

    if (path != NULL)
    {
        make_uri_unescaped_charset(&valid_chars, TE_STRING_URI_ESCAPE_PATH);
        te_charset_add_range(&valid_chars, '%', '%');

        if (!te_charset_check_bytes(&valid_chars, strlen(path),
                                    (const uint8_t *)path))
            TE_FATAL_ERROR("Invalid URI path: %s", path);

        if (*path != '/' && host != NULL)
            te_string_append(str, "/");
        te_string_append(str, "%s", path);
    }

    if (query != NULL)
    {
        make_uri_unescaped_charset(&valid_chars, TE_STRING_URI_ESCAPE_QUERY);
        te_charset_add_range(&valid_chars, '%', '%');

        if (!te_charset_check_bytes(&valid_chars, strlen(query),
                                    (const uint8_t *)query))
            TE_FATAL_ERROR("Invalid URI query: %s", query);

        te_string_append(str, "?%s", query);
    }

    if (frag != NULL)
    {
        te_string_append(str, "#");
        te_string_append_escape_uri(str, TE_STRING_URI_ESCAPE_FRAG, frag);
    }
}

void
te_string_generic_escape(te_string *str, const char *input,
                         const char *esctable[static UINT8_MAX + 1],
                         te_string_generic_escape_fn *ctrl_esc,
                         te_string_generic_escape_fn *nonascii_esc)
{
    for (; *input != '\0'; input++)
    {
        if (esctable[(uint8_t)*input] != NULL)
            te_string_append(str, "%s", esctable[(uint8_t)*input]);
        else if (ctrl_esc != NULL && iscntrl(*input))
            ctrl_esc(str, *input);
        else if (nonascii_esc != NULL && !isascii(*input))
            nonascii_esc(str, *input);
        else
            te_string_append(str, "%c", *input);
    }
}

static char
encode_base64_bits(uint8_t sextet, bool url_safe)
{
    char bc;

    if (sextet < 26)
        bc = 'A' + sextet;
    else if (sextet < 52)
        bc = 'a' + sextet - 26;
    else if (sextet < 62)
        bc = '0' + sextet - 52;
    else if (sextet == 62)
        bc = url_safe ? '-' : '+';
    else
        bc = url_safe ? '_' : '/';

    return bc;
}

#define BASE64_ENC_BITS 6

void
te_string_encode_base64(te_string *str, size_t len, const uint8_t bytes[len],
                        bool url_safe)
{
    size_t i;
    uint32_t latch = 0;
    unsigned int bits = 0;
    uint8_t sextet;

    /* Base64 encodes each three bytes as four characters */
    te_string_reserve(str, str->size + len * 4 / 3);
    for (i = 0; i < len; i++)
    {
        latch <<= CHAR_BIT;
        latch |= bytes[i];
        bits += CHAR_BIT;

        while (bits >= BASE64_ENC_BITS)
        {
            sextet = TE_EXTRACT_BITS(latch, bits - BASE64_ENC_BITS,
                                     BASE64_ENC_BITS);
            bits -= BASE64_ENC_BITS;

            te_string_append(str, "%c", encode_base64_bits(sextet, url_safe));
        }
    }

    if (bits != 0)
    {
        sextet = TE_EXTRACT_BITS(latch, 0, bits) << (BASE64_ENC_BITS - bits);
        te_string_append(str, "%c%s", encode_base64_bits(sextet, url_safe),
                         bits == 2 ? "==" : "=");
    }
}

te_errno
te_string_decode_base64(te_string *str, const char *base64str)
{
    const char *src;
    uint32_t latch = 0;
    unsigned int bits = 0;
    unsigned int padding = 0;

    for (src = base64str; *src != '\0'; src++)
    {
        uint8_t sextet;

        if (isspace(*src))
            continue;
        if (*src == '=')
        {
            padding++;
            if (padding > 2)
            {
                ERROR("Too many padding characters");
                return TE_EILSEQ;
            }
            continue;
        }

        if (padding > 0)
        {
            ERROR("Significant characters after padding");
            return TE_EILSEQ;
        }
        if (*src >= 'A' && *src <= 'Z')
            sextet = *src - 'A';
        else if (*src >= 'a' && *src <= 'z')
            sextet = *src - 'a' + 26;
        else if (*src >= '0' && *src <= '9')
            sextet = *src - '0' + 52;
        else if (*src == '+' || *src == '-')
            sextet = 62;
        else if (*src == '/' || *src == '_')
            sextet = 63;
        else
        {
            ERROR("Invalid Base64 character: %#x", (unsigned int)*src);
            return TE_EILSEQ;
        }

        latch <<= BASE64_ENC_BITS;
        latch |= sextet;
        bits += BASE64_ENC_BITS;
        if (bits >= CHAR_BIT)
        {
            te_string_append_buf(str,
                                 (const char[]){TE_EXTRACT_BITS(latch,
                                                                bits - CHAR_BIT,
                                                                CHAR_BIT)}, 1);
            bits -= CHAR_BIT;
        }
    }
    if (bits == BASE64_ENC_BITS)
    {
        ERROR("Insufficient number of Base64 characters");
        return TE_EILSEQ;
    }

    return 0;
}

#undef BASE64_ENC_BITS

char *
te_string_fmt_va(const char *fmt,
                 va_list     ap)
{
    te_string str = TE_STRING_INIT;

    te_string_append_va(&str, fmt, ap);

    return str.ptr;
}

char *
te_string_fmt(const char *fmt, ...)
{
    va_list  ap;
    char *result;

    va_start(ap, fmt);
    result = te_string_fmt_va(fmt, ap);
    va_end(ap);

    return result;
}

void
te_string_chop(te_string *str, const char *trail)
{
    while (str->len > 0 && strchr(trail, str->ptr[str->len - 1]) != NULL)
    {
        str->len--;
        str->ptr[str->len] = '\0';
    }
}

void
te_string_add_centered(te_string *str, const char *src,
                       size_t padlen, char padchar)
{
    char *dest;
    size_t src_len = strlen(src);

    te_string_reserve(str, str->len + padlen + 1);

    if (src_len > padlen)
        src_len = padlen;

    dest = str->ptr + str->len;
    /* If we cannot center exactly, prefer shifting to the right */
    memset(dest, padchar, (padlen - src_len + 1) / 2);
    dest += (padlen - src_len + 1) / 2;
    memcpy(dest, src, src_len);
    dest += src_len;
    memset(dest, padchar, (padlen - src_len) / 2);
    str->len += padlen;
    str->ptr[str->len] = '\0';
}

te_errno
te_string_process_lines(te_string *buffer, bool complete_lines,
                        te_string_line_handler_fn *callback, void *user_data)
{
    te_errno rc = 0;

    while (rc == 0 && buffer->len > 0)
    {
        size_t line_len = strcspn(buffer->ptr, "\n");
        if (buffer->ptr[line_len] == '\0' && complete_lines)
            return 0;

        buffer->ptr[line_len] = '\0';
        if (line_len > 0 && buffer->ptr[line_len - 1] == '\r')
            buffer->ptr[line_len - 1] = '\0';

        rc = callback(buffer->ptr, user_data);

        te_string_cut_beginning(buffer, line_len + 1);
    }

    return rc == TE_EOK ? 0 : rc;
}

char *
raw2string(const uint8_t *data, size_t size)
{
    te_string  str = TE_STRING_INIT;
    size_t i = 0;

    te_string_append(&str, "[ ");

    for (i = 0; i < size; i++)
        te_string_append(&str, "%#02x ", data[i]);

    te_string_append(&str, "]");

    return str.ptr;
}

void
te_substring_find(te_substring_t *substr, const char *str)
{
    char *ch;

    if (!te_substring_is_valid(substr))
        return;

    ch = strstr(substr->base->ptr + substr->start, str);
    if (ch == NULL)
    {
        substr->start = SIZE_MAX;
        substr->len = 0;
    }
    else
    {
       substr->start = ch - substr->base->ptr;
       substr->len = strlen(str);
    }

    return;
}

te_errno
te_substring_replace(te_substring_t *substr, const char *str)
{
    size_t rep_len = str == NULL ? 0 : strlen(str);
    if (substr->start + substr->len > substr->base->len)
    {
        ERROR("Substring position out of bounds");
        return TE_EINVAL;
    }

    te_string_replace_buf(substr->base, substr->start, substr->len,
                          str, rep_len);

    substr->start += rep_len;
    substr->len = 0;

    return 0;
}

void
te_substring_advance(te_substring_t *substr)
{
    substr->start += substr->len;
    substr->len = 0;
}

void
te_substring_limit(te_substring_t *substr, const te_substring_t *limit)
{
    substr->len = limit->start - substr->start;
}

static void
replace_substring(te_substring_t *substr, const char *new,
                  const char *old)
{
    te_substring_find(substr, old);

    if (!te_substring_is_valid(substr))
        return;

    /*
     * No need to check for return code:
     * substr is known to be valid.
     */
    te_substring_replace(substr, new);
}

te_errno
te_string_replace_substring(te_string *str, const char *new,
                            const char *old)
{
    te_substring_t iter = TE_SUBSTRING_INIT(str);

    replace_substring(&iter, new, old);
    return 0;
}

te_errno
te_string_replace_all_substrings(te_string *str, const char *new,
                                 const char *old)
{
    te_substring_t iter = TE_SUBSTRING_INIT(str);

    while (1)
    {
        replace_substring(&iter, new, old);

        if (!te_substring_is_valid(&iter))
            break;
    }

    return 0;
}
