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
#include "logger_api.h"


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

/* See description in te_str.h */
te_errno
te_strtol_raw(const char *str, char **endptr,  int base, long int *result)
{
    te_errno  rc = 0;
    int       saved_errno = errno;
    int       new_errno = 0;

    if (str == NULL || result == NULL)
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

    if (*endptr == NULL || *endptr == str)
    {
        ERROR("%s(): failed to parse '%s'", __FUNCTION__, str);
        return TE_EINVAL;
    }

    return 0;
}

te_errno
te_strtol(const char *str, int base, long int *result)
{
    char      *endptr;
    te_errno  rc;

    rc = te_strtol_raw(str, &endptr, base, result);
    if (!rc)
        return rc;

    if (*endptr != '\0')
    {
        ERROR("%s(): failed to parse '%s'", __FUNCTION__, str);
        return TE_EINVAL;
    }

    return 0;
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
