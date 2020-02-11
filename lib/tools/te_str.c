/** @file
 * @brief API to deal with strings
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

#include "te_config.h"
#include "te_defs.h"
#include "te_errno.h"
#include <stdlib.h>
#ifdef HAVE_CTYPE_H
#include <ctype.h>
#endif
#ifdef HAVE_BSD_STRING_H
#include <bsd/string.h>
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
char *
te_strncpy(const char *id, char *dst, size_t size, const char *src)
{
    size_t n;

    if (size == 0)
    {
        ERROR("%s: destination buffer has zero size", id);
        return dst;
    }

    n = strlen(src) + 1;
    n = n < size ? n : size;
    memcpy(dst, src, n);
    if (dst[n - 1] != '\0')
    {
        dst[n - 1] = '\0';
        ERROR("%s: string \"%s\" is truncated", id, dst);
    }
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

/* See description in te_str.h */
te_errno
te_strtoul(const char *str, int base, unsigned long int *value)
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
    *value = strtoul(str, &endptr, base);
    new_errno = errno;
    errno = saved_errno;

    if ((new_errno == ERANGE && *value == ULONG_MAX) ||
        (new_errno != 0 && *value == 0))
    {
        rc = te_rc_os2te(new_errno);
        ERROR("%s(): strtoul() failed with errno %r", __FUNCTION__, rc);
        return rc;
    }

    if (endptr == NULL || *endptr != '\0' || endptr == str)
    {
        ERROR("%s(): failed to parse '%s'", __FUNCTION__, str);
        return TE_EINVAL;
    }

    return 0;
}

te_errno
te_strtoui(const char   *str,
           int           base,
           unsigned int *value)
{
    unsigned long int value_long;
    te_errno          rc;

    rc = te_strtoul(str, base, &value_long);
    if (rc != 0)
        return rc;

    if (value_long > UINT_MAX)
    {
        ERROR("%s(): the value '%s' is too big", __FUNCTION__, str);
        return TE_EOVERFLOW;
    }

    *value = value_long;

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

    if ((new_errno == ERANGE && *result == LONG_MAX) ||
        (new_errno != 0 && *result == 0))
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

    assert(data != NULL);
    assert(str != NULL);

    need_size = data_len * 3;

    if (str->ext_buf && str->size < need_size)
    {
        ERROR("%s(): buffer size is too small, needed %u, but provided %u",
               __FUNCTION__, need_size, str->size);
        return TE_EINVAL;
    }

    if (str->size < need_size)
    {
        char *new_ptr;

        /* Extend buffer */
        new_ptr = realloc(str->ptr, need_size);
        if (new_ptr == NULL)
        {
            ERROR("%s(): Memory allocation failure", __FUNCTION__);
            return TE_ENOMEM;
        }
        str->size = need_size;
        str->ptr = new_ptr;
    }

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
