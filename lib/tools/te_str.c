/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief API to deal with strings
 *
 * Function to operate the strings.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#include "te_config.h"
#include "te_defs.h"
#include "te_alloc.h"
#include "te_errno.h"
#include <stdlib.h>
#ifdef HAVE_CTYPE_H
#include <ctype.h>
#endif
#ifdef HAVE_BSD_STRING_H
#include <bsd/string.h>
#endif
#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif

#include "logger_api.h"
#include "te_string.h"
#include "te_str.h"

/* See description in te_str.h */
char *
te_str_upper(const char *src)
{
    char *dst;
    char *res;

    res = malloc(strlen(src) + 1);
    if (res == NULL)
        return NULL;

    for (dst = res; *src != '\0'; src++, dst++)
        *dst = toupper(*src);
    *dst = '\0';

    return res;
}

/* See description in te_str.h */
char *
te_str_lower(const char *src)
{
    char *dst;
    char *res;

    res = malloc(strlen(src) + 1);
    if (res == NULL)
        return NULL;

    for (dst = res; *src != '\0'; src++, dst++)
        *dst = tolower(*src);
    *dst = '\0';

    return res;
}

/* See description in te_str.h */
te_bool
te_str_is_equal_nospace(const char *str1, const char *str2)
{
    te_bool has_space1 = TRUE;
    te_bool has_space2 = TRUE;

    while (*str1 != '\0' || *str2 != '\0')
    {
        for (; *str1 != '\0' && isspace(*str1); str1++)
            has_space1 = TRUE;
        for (; *str2 != '\0' && isspace(*str2); str2++)
            has_space2 = TRUE;

        if (*str1 != *str2)
            return FALSE;
        if (has_space1 != has_space2 && *str1 != '\0')
            return FALSE;

        has_space1 = has_space2 = FALSE;
        if (*str1 != '\0')
        {
            str1++;
            str2++;
        }
    }

    return TRUE;
}

/* See description in te_str.h */
char *
te_str_concat(const char *first, const char *second)
{
    char *str;
    size_t len1;
    size_t len2;

    if (first == NULL)
        first = "";
    if (second == NULL)
        second = "";

    len1 = strlen(first);
    len2 = strlen(second);

    str = malloc(len1 + len2 + 1);
    if (str == NULL)
        return NULL;

    memcpy(str, first, len1);
    memcpy(&str[len1], second, len2);
    str[len1 + len2] = '\0';

    return str;
}

/* See the description in te_str.h */
size_t
te_strlcpy(char *dst, const char *src, size_t size)
{
#ifdef HAVE_STRLCPY
    return strlcpy(dst, src, size);
#else
    size_t len = strlen(src);

    if (len < size)
    {
        memcpy(dst, src, len + 1);
    }
    else
    {
        memcpy(dst, src, size - 1);
        dst[size - 1] = '\0';
    }

    return len;
#endif
}

/* See the description in te_str.h */
size_t
te_strlcat(char *dst, const char *src, size_t size)
{
#ifdef HAVE_STRLCAT
    return strlcat(dst, src, size);
#else
    size_t len = strnlen(dst, size);

    if (len < size)
        return len + te_strlcpy(dst + len, src, size - len);
    else
        return len + strlen(src);
#endif
}

/* See description in te_str.h */
te_errno
te_strlcpy_safe(char *dst, const char *src, size_t size)
{
    if (size == 0)
        return TE_ESMALLBUF;

    if (te_strlcpy(dst, src, size) >= size)
        return TE_ESMALLBUF;

    return 0;
}

/* See description in te_str.h */
char *
te_strlcpy_verbose(const char *id, char *dst, const char *src, size_t size)
{
    if (te_strlcpy_safe(dst, src, size) != 0)
        ERROR("%s: string \"%s\" is truncated", id, dst);

    return dst;
}

/* See description in te_str.h */
te_errno
te_vsnprintf(char *dst, size_t size, const char *fmt, va_list ap)
{
    int n = vsnprintf(dst, size, fmt, ap);

    if (n < 0)
        return te_rc_os2te(errno);
    else if ((size_t)n >= size)
        return TE_ESMALLBUF;

    return 0;
}

/* See description in te_str.h */
te_errno
te_snprintf(char *dst, size_t size, const char *fmt, ...)
{
    te_errno rc;
    va_list ap;

    va_start(ap, fmt);
    rc = te_vsnprintf(dst, size, fmt, ap);
    va_end(ap);

    return rc;
}

/* See description in te_str.h */
char *
te_snprintf_verbose(const char *id, char *dst, size_t size,
                    const char *fmt, ...)
{
    te_errno rc;
    va_list ap;

    va_start(ap, fmt);
    rc = te_vsnprintf(dst, size, fmt, ap);
    va_end(ap);

    if (rc == TE_ESMALLBUF)
        ERROR("%s: string \"%s\" is truncated by snprintf()", id, dst);
    else if (rc != 0)
        ERROR("%s: output error is encountered: %r", id, rc);

    return dst;
}

/* See description in te_str.h */
char *
te_str_strip_spaces(const char *str)
{
    size_t len;
    size_t offt;
    char  *res;

    len = strlen(str);
    while (len > 0 && isspace(str[len - 1]))
        len--;

    offt = strspn(str, " \f\n\r\t\v");
    if (offt <= len)
        len -= offt;

    res = malloc(len + 1);
    if (res == NULL)
        return NULL;

    memcpy(res, str + offt, len);
    res[len] = '\0';

    return res;
}

te_errno
te_strpbrk_balanced(const char *str, char opening, char closing,
                    char escape, const char *seps,
                    const char **result)
{
    unsigned int level = 0;

    if (result != NULL)
        *result = NULL;

    for (; *str != '\0'; str++)
    {
        if (escape != '\0' && *str == escape)
        {
            if (str[1] == '\0')
            {
                ERROR("Dangling '%c'", escape);
                return TE_EILSEQ;
            }
            if (level == 0 && seps == NULL)
            {
                if (result != NULL)
                    *result = str;
                return 0;
            }
            str++;
            continue;
        }

        if (*str == opening)
            level++;
        else if (*str == closing)
        {
            if (level == 0)
                ERROR("Unbalanced '%c'", closing);
            level--;
        }
        else if (level == 0 && (seps == NULL || strchr(seps, *str) != NULL))
        {
            if (result != NULL)
                *result = str;
            return 0;
        }
    }

    if (level != 0)
    {
        ERROR("Unbalanced '%c'", opening);
        return TE_EILSEQ;
    }

    if (seps == NULL)
    {
        if (result != NULL)
            *result = str;
        return 0;
    }

    return TE_ENOENT;
}

te_errno
te_strpbrk_rev_balanced(const char *str, char opening, char closing,
                        char escape, const char *seps,
                        const char **result)
{
    const char *ptr = str + strlen(str);
    unsigned int level = 0;

    if (result != NULL)
        *result = NULL;

    for (; ptr != str; ptr--)
    {
        te_bool is_escaped = FALSE;

        if (escape != '\0' && ptr > str + 1)
        {
            const char *check;

            for (check = ptr - 1; check != str && check[-1] == escape; check--)
                is_escaped = !is_escaped;
        }
        if (!is_escaped)
        {
            if (ptr[-1] == escape)
            {
                ERROR("Dangling '%c' at %s", escape, ptr - 1);
                return TE_EILSEQ;
            }

            if (ptr[-1] == closing)
            {
                level++;
                continue;
            }
            if (ptr[-1] == opening)
            {
                if (level == 0)
                    ERROR("Unbalanced '%c'", opening);
                level--;
                continue;
            }
        }
        if (level == 0 &&
            (seps == NULL ||
             (!is_escaped && strchr(seps, ptr[-1]) != NULL)))
        {
            if (result != NULL)
                *result = ptr - 1 - (int)is_escaped;
            return 0;
        }
        if (is_escaped)
            ptr--;
    }

    if (level != 0)
    {
        ERROR("Unbalanced '%c'", closing);
        return TE_EILSEQ;
    }

    return TE_ENOENT;
}

/* See description in te_str.h */
te_errno
te_strtoumax(const char *str, int base, uintmax_t *value)
{
    char     *endptr = NULL;
    te_errno  rc = 0;
    int       saved_errno = errno;
    int       new_errno = 0;

    if (str == NULL || value == NULL)
    {
        ERROR("%s(): invalid arguments", __FUNCTION__);
        return TE_EINVAL;
    }

    errno = 0;
    *value = strtoumax(str, &endptr, base);
    new_errno = errno;
    errno = saved_errno;

    if ((new_errno == ERANGE && *value == UINTMAX_MAX) ||
        (new_errno != 0 && *value == 0))
    {
        rc = te_rc_os2te(new_errno);
        ERROR("%s(): strtoumax() failed with errno %r", __FUNCTION__, rc);
        return rc;
    }

    if (endptr == NULL || *endptr != '\0' || endptr == str)
    {
        ERROR("%s(): failed to parse '%s'", __FUNCTION__, str);
        return TE_EINVAL;
    }

    return 0;
}

/* See description in te_str.h */
te_errno
te_strtou_size(const char *str, int base, void *value, size_t size)
{
    uintmax_t max_val;
    uintmax_t parsed_val;
    te_errno rc;

    switch (size)
    {
        case sizeof(uint8_t):
            max_val = UINT8_MAX;
            break;

        case sizeof(uint16_t):
            max_val = UINT16_MAX;
            break;

        case sizeof(uint32_t):
            max_val = UINT32_MAX;
            break;

        case sizeof(uint64_t):
            max_val = UINT64_MAX;
            break;

        default:
            ERROR("%s(): not supported value size %zu", __FUNCTION__, size);
            return TE_EINVAL;
    }

    rc = te_strtoumax(str, base, &parsed_val);
    if (rc != 0)
        return rc;
    if (parsed_val > max_val)
    {
        ERROR("%s(): %s is too big for %zu bytes", __FUNCTION__, str, size);
        return TE_ERANGE;
    }

    switch (size)
    {
        case sizeof(uint8_t):
            *(uint8_t *)value = parsed_val;
            break;

        case sizeof(uint16_t):
            *(uint16_t *)value = parsed_val;
            break;

        case sizeof(uint32_t):
            *(uint32_t *)value = parsed_val;
            break;

        case sizeof(uint64_t):
            *(uint64_t *)value = parsed_val;
            break;
    }

    return 0;
}

/* See description in te_str.h */
te_errno
te_strtoi_size(const char *str, int base, void *value, size_t size)
{
    intmax_t max_val;
    intmax_t min_val;
    intmax_t parsed_val;
    te_errno rc;

    switch (size)
    {
        case sizeof(uint8_t):
            min_val = INT8_MIN;
            max_val = INT8_MAX;
            break;

        case sizeof(uint16_t):
            min_val = INT16_MIN;
            max_val = INT16_MAX;
            break;

        case sizeof(uint32_t):
            min_val = INT32_MIN;
            max_val = INT32_MAX;
            break;

        case sizeof(uint64_t):
            min_val = INT64_MIN;
            max_val = INT64_MAX;
            break;

        default:
            ERROR("%s(): not supported value size %zu", __FUNCTION__, size);
            return TE_EINVAL;
    }

    rc = te_strtoimax(str, base, &parsed_val);
    if (rc != 0)
        return rc;
    if (parsed_val > max_val || parsed_val < min_val)
    {
        ERROR("%s(): %s does not fit in %zu bytes",
              __FUNCTION__, str, size);
        return TE_ERANGE;
    }

    switch (size)
    {
        case sizeof(uint8_t):
            *(int8_t *)value = parsed_val;
            break;

        case sizeof(uint16_t):
            *(int16_t *)value = parsed_val;
            break;

        case sizeof(uint32_t):
            *(int32_t *)value = parsed_val;
            break;

        case sizeof(uint64_t):
            *(int64_t *)value = parsed_val;
            break;
    }

    return 0;
}

/* See description in te_str.h */
te_errno
te_strtoimax(const char *str, int base, intmax_t *value)
{
    char     *endptr = NULL;
    te_errno  rc = 0;
    int       saved_errno = errno;
    int       new_errno = 0;
    intmax_t  value_temp;


    if (str == NULL || value == NULL)
    {
        ERROR("%s(): invalid arguments", __FUNCTION__);
        return TE_EINVAL;
    }

/*
 * Man says that strtoimax() can legitimately return 0, INTMAX_MAX, or
 * INTMAX_MIN on both success and failure. And therefore the calling program
 * should set errno to 0 before the call. And then determine if an error
 * occurred by checking whether errno has a nonzero value after the call.
 *
 * As we explicitly set errno to 0, it would be easier to restore errno here
 * than to handle it somehow in all the callers.
 */
    errno = 0;
    value_temp = strtoimax(str, &endptr, base);
    new_errno = errno;
    errno = saved_errno;

    if (((value_temp == INTMAX_MAX || value_temp == INTMAX_MIN)
        && new_errno == ERANGE)
        || (new_errno != 0 && value_temp == 0))
    {
        rc = te_rc_os2te(new_errno);
        ERROR("%s(): strtoimax() failed with errno %r", __FUNCTION__, rc);
        return rc;
    }

    if (endptr == NULL || *endptr != '\0' || endptr == str)
    {
        ERROR("%s(): failed to parse '%s'", __FUNCTION__, str);
        return TE_EINVAL;
    }

    *value = value_temp;
    return 0;
}

/* See description in te_str.h */
te_errno
te_strtoul(const char *str, int base, unsigned long int *value)
{
    uintmax_t value_um;
    te_errno rc;

    rc = te_strtoumax(str, base, &value_um);
    if (rc != 0)
        return rc;

    if (value_um > ULONG_MAX)
    {
        ERROR("%s(): the value '%s' is too big", __FUNCTION__, str);
        return TE_EOVERFLOW;
    }

    *value = value_um;

    return 0;
}


/* See description in te_str.h */
te_errno
te_str_to_uint64(const char *str, int base, uint64_t *value)
{
    uintmax_t value_um;
    te_errno rc;

    rc = te_strtoumax(str, base, &value_um);
    if (rc != 0)
        return rc;

    if (value_um > UINT64_MAX)
    {
        ERROR("%s(): the value '%s' is too big", __FUNCTION__, str);
        return TE_EOVERFLOW;
    }

    *value = value_um;
    return 0;
}

te_errno
te_strtoi(const char *str, int base, int *value)
{
    long int value_long;
    te_errno rc;

    rc = te_strtol(str, base, &value_long);
    if (rc != 0)
        return rc;

    if (value_long < INT_MIN || value_long > INT_MAX)
    {
        ERROR("%s(): the value '%s' is too big", __FUNCTION__, str);
        return TE_EOVERFLOW;
    }

    *value = value_long;

    return 0;
}

te_errno
te_strtoui(const char   *str,
           int           base,
           unsigned int *value)
{
    uintmax_t value_um;
    te_errno rc;

    rc = te_strtoumax(str, base, &value_um);
    if (rc != 0)
        return rc;

    if (value_um > UINT_MAX)
    {
        ERROR("%s(): the value '%s' is too big", __FUNCTION__, str);
        return TE_EOVERFLOW;
    }

    *value = value_um;

    return 0;
}


static te_errno
te_strtol_raw_silent(const char *str, char **endptr,  int base,
                     long int *result)
{
    te_errno  rc = 0;
    int       saved_errno = errno;
    int       new_errno = 0;

    if (str == NULL || endptr == NULL || result == NULL)
    {
        ERROR("%s(): invalid arguments", __FUNCTION__);
        return TE_EINVAL;
    }

    errno = 0;
    *endptr = NULL;
    *result = strtol(str, endptr, base);
    new_errno = errno;
    errno = saved_errno;

    if (new_errno != 0)
    {
        rc = te_rc_os2te(new_errno);
        ERROR("%s(): strtol() failed with errno %r", __FUNCTION__, rc);
        return rc;
    }

    return 0;
}

/* See description in te_str.h */
te_errno
te_strtol_raw(const char *str, char **endptr,  int base, long int *result)
{
    te_errno rc;

    rc = te_strtol_raw_silent(str, endptr, base, result);
    if (rc != 0)
        return rc;

    if (*endptr == NULL || *endptr == str)
    {
        ERROR("%s(): failed to parse '%s'", __FUNCTION__, str);
        return TE_EINVAL;
    }

    return 0;
}

/* See description in te_str.h */
te_errno
te_strtol_silent(const char *str, int base, long int *result)
{
    char     *endptr;
    te_errno  rc;

    rc = te_strtol_raw_silent(str, &endptr, base, result);
    if (rc != 0)
        return rc;

    if (endptr == NULL || endptr == str || *endptr != '\0')
        return TE_EINVAL;

    return 0;
}

/* See description in te_str.h */
te_errno
te_strtol(const char *str, int base, long int *result)
{
    char     *endptr;
    te_errno  rc;

    rc = te_strtol_raw_silent(str, &endptr, base, result);
    if (rc != 0)
        return rc;

    if (endptr == NULL || endptr == str || *endptr != '\0')
    {
        ERROR("%s(): failed to parse '%s'", __FUNCTION__, str);
        rc = TE_EINVAL;
    }

    return rc;
}

te_errno
te_strtol_bool(const char *input, te_bool *bresult)
{
    long int res;

    if (te_strtol(input, 10, &res) != 0)
        return TE_EINVAL;

    /* explicitly check for == cause bool is 0 or 1 only! */
    *bresult = (res == 0) ? FALSE : TRUE;

    return 0;
}

te_errno
te_strtoi_range_raw(const char *input, int minval, int maxval,
                    char **endptr, int base, int *result)
{
    long int parsed;
    te_errno rc = te_strtol_raw(input, endptr, base, &parsed);

    if (rc != 0)
        return rc;

    if (parsed < minval || parsed > maxval)
    {
        ERROR("%s(): %ld is not in range %d..%d", __func__,
              parsed, minval, maxval);
        return TE_ERANGE;
    }

    if (result != NULL)
        *result = (int)parsed;

    return 0;
}

te_errno
te_strtod_raw(const char *str, char **endptr, double *result)
{
    int saved_errno = errno;
    int new_errno = 0;

    if (str == NULL || endptr == NULL || result == NULL)
    {
        ERROR("%s(): invalid arguments", __FUNCTION__);
        return TE_EINVAL;
    }

    errno = 0;
    *endptr = NULL;

    *result = strtod(str, endptr);
    new_errno = errno;
    errno = saved_errno;

    if (new_errno != 0)
    {
        ERROR("%s(): strtod() failed to process '%s'", __FUNCTION__, str);
        return te_rc_os2te(new_errno);
    }

    if (*endptr == NULL || *endptr == str)
    {
        ERROR("%s(): failed to parse '%s'", __FUNCTION__, str);
        return TE_EINVAL;
    }

    return 0;
}

te_errno
te_strtod(const char *str, double *result)
{
    te_errno  rc;
    char     *endptr;

    rc = te_strtod_raw(str, &endptr, result);
    if (rc != 0)
        return rc;

    if (*endptr != '\0')
    {
        ERROR("%s(): failed to parse '%s'", __FUNCTION__, str);
        return TE_EINVAL;
    }

    return 0;
}

/* See description in te_str.h */
char **
te_str_make_array(const char *fst, ...)
{
    va_list ap;
    const char *elem = fst;
    char **new;
    size_t num;

    if (fst == NULL)
        return TE_ALLOC(sizeof(char *));

    va_start(ap, fst);
    for (num = 1; elem != NULL; num++)
        elem = va_arg(ap, char *);

    va_end(ap);
    va_start(ap, fst);
    new = TE_ALLOC(num * sizeof(char *));
    for (num = 0, elem = fst; elem != NULL; num++)
    {
        new[num] = strdup(elem);
        elem = va_arg(ap, char *);
    }

    va_end(ap);
    return new;
}

/* See description in te_str.h */
void
te_str_free_array(char **str)
{
    char **it;

    if (str == NULL)
        return;

    for (it = str; *it != NULL; it++)
        free(*it);

    free(str);
}

/* See description in te_str.h */
te_errno
te_str_hex_raw2str(const uint8_t *data, size_t data_len, te_string *str)
{
    const char     *hex = "0123456789ABCDEF";
    const uint8_t  *p_data;
    char           *p_buf;
    size_t          i;
    size_t          need_size;

    te_errno rc;

    assert(data != NULL);
    assert(str != NULL);

    need_size = data_len * 3;

    rc = te_string_reserve(str, need_size);
    if (rc != 0)
        return rc;

    p_data = data;
    p_buf = str->ptr;
    for(i = 0; i < data_len; i++)
    {
        if (i > 0)
            *p_buf++ = TE_STR_HEX_DELIMITER;
        *p_buf++ = hex[(*p_data >> 4) & 0xF];
        *p_buf++ = hex[(*p_data++) & 0xF];
    }
    *p_buf = '\0';

    str->len = need_size;
    return 0;
}

/* See description in te_str.h */
te_errno te_str_hex_str2raw(const char *str, uint8_t *data, size_t data_len)
{
    const char *hex = str;
    uint32_t    val = 0;
    size_t      i = 0;

    while (*hex)
    {
        uint8_t byte = *hex++;

        if (byte >= '0' && byte <= '9')
        {
            byte = byte - '0';
        }
        else if (byte >= 'A' && byte <= 'F')
        {
            byte = byte - 'A' + 10;
        }
        else if (byte >= 'a' && byte <= 'f')
        {
            byte = byte - 'a' + 10;
        }
        else
        {
            ERROR("%s(): %d-st symbol is wrong in %s", __FUNCTION__,
                  (hex - str), str);
            return TE_EINVAL;
        }

        val = (val << 4) | (byte & 0xF);

        /* Check that val it not greater than the maximum byte size */
        if (val > 0xFF)
        {
            ERROR("%s(): %X is greater than 0xFF", __FUNCTION__, val);
            return TE_EOVERFLOW;
        }

        if (*hex == '\0' || *hex == TE_STR_HEX_DELIMITER)
        {
            if (i >= data_len)
            {
                ERROR("%s(): hex string too long", __FUNCTION__);
                return TE_EOVERFLOW;
            }
            if (*hex == TE_STR_HEX_DELIMITER)
                hex++;

            data[i] = (uint8_t)val;
            i++;
            val = 0;
        }
    }

    if (i == data_len)
        return 0;

    ERROR("%s(): hex string is too small.", __FUNCTION__);
    return TE_EINVAL;
}

/* See description in te_str.h */
te_errno
te_str_find_index(const char *str, const char **str_array,
                  unsigned int str_array_len, unsigned int *index)
{
    unsigned int i;

    if (str == NULL || *str == '\0')
        return TE_EINVAL;

    for (i = 0; i < str_array_len; i++)
    {
        if (strcmp(str, str_array[i]) == 0)
        {
            *index = i;
            return 0;
        }
    }

    return TE_EINVAL;
}

/** The symbol that separates parts in version format. */
#define TE_STR_VERSION_DELIMETER '.'

/**
 * Checks that version has appropriate format and supported symbols.
 *
 * @param v Version to validate.
 *
 */
static te_errno
check_version_consistency(const char* v)
{
    int i;

    if (v[0] == '\0')
        return 0;

    if (!isdigit(v[0]))
    {
        ERROR("version '%s' has bad syntax: version number "
              "does not start with digit", v);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    for (i = 0; i < strlen(v); i++)
    {
        if (!isdigit(v[i]) && v[i] != TE_STR_VERSION_DELIMETER)
        {
            ERROR("version '%s' has bad syntax: version number "
                  "contains unsupported symbol '%c'", v, v[i]);
            return TE_RC(TE_TAPI, TE_EINVAL);
        }

        if (v[i] == TE_STR_VERSION_DELIMETER &&
            v[i + 1] == TE_STR_VERSION_DELIMETER)
        {
            ERROR("version '%s' has bad syntax: version delimeters '%c' "
                  "are placed together", v, TE_STR_VERSION_DELIMETER);
            return TE_RC(TE_TAPI, TE_EINVAL);
        }
    }

    if (v[strlen(v) - 1] == TE_STR_VERSION_DELIMETER)
    {
        ERROR("version '%s' has bad syntax: version can't end by delimeter "
              "'%c'", v, TE_STR_VERSION_DELIMETER);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    return 0;
}

/* See description in te_str.h */
te_errno
te_str_compare_versions(const char *v1, const char *v2, int *res)
{
    int i;
    te_errno rc = 0;
    char *end_ptr = NULL;
    const char *versions[] = { v1, v2 };
    long int values[] = { 0, 0 };

    for (i = 0; i < TE_ARRAY_LEN(versions); i++)
    {
        rc = check_version_consistency(versions[i]);
        if (rc != 0)
            return rc;
    }

    while (*versions[0] != '\0' && *versions[1] != '\0')
    {
        for (i = 0; i < TE_ARRAY_LEN(versions); i++)
        {
            rc = te_strtol_raw(versions[i], &end_ptr, 10, &values[i]);
            if (rc != 0)
                return rc;

            if (*end_ptr != '\0')
                end_ptr++;

            versions[i] = end_ptr;
        }

        *res = values[0] - values[1];

        if (*res != 0)
            return rc;
    }

    if (*versions[0] != '\0')
        *res = 1;
    else if (*versions[1] != '\0')
        *res = -1;
    else
        *res = 0;

    return rc;
}
