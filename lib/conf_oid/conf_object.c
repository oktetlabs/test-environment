/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Common code for complex object in configuration tree
 *
 * For more detail see conf_object.h
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#define TE_LGR_USER "Configurator object"

#include "te_config.h"

#include "conf_object.h"
#include "logger_api.h"
#include "te_sockaddr.h"

#include <arpa/inet.h>

/**
 * Convert @b te_bool field to string.
 *
 * @param [in,out] str   String to append with converted value.
 * @param [in]    name   Field name.
 * @param [in]    arg    Pointer to a value.
 *
 * @return Status code.
 */
static te_errno
te_bool_to_str(te_string *str, const char *name,
               const void *arg)
{
    te_bool value = *(te_bool *)arg;

    return te_string_append(str, "%s=%s",
                            name, (value ? "true" : "false"));
}

/**
 * Obtain @b te_bool field value from string.
 *
 * @param [in]  value    String.
 * @param [out] arg      Pointer to the field value.
 * @param [in]  options  User options (not used).
 *
 * @return Status code.
 */
static te_errno
te_bool_from_str(const char *value, void *arg,
                 const void *options)
{
    UNUSED(options);

    if (strcmp(value, "true") == 0)
        *(te_bool *)arg = TRUE;
    else if (strcmp(value, "false") == 0)
        *(te_bool *)arg = FALSE;
    else
        return TE_EINVAL;

    return 0;
}

/**
 * Compare values of two @b te_bool fields.
 *
 * @param a     Pointer to the first field.
 * @param b     Pointer to the second field.
 *
 * @return          Result of the comparison
 * @retval ==0      Values are not equal
 * @retval !=0      Values are equal
 */
static int
te_bool_compare(const void *a, const void *b)
{
    return (!!(*(te_bool *)a) == !!(*(te_bool *)b));
}

/**
 * Numeric type field processing
 *
 * @param _type         Type of numeric field
 * @param _format       Format string to represent a number
 * @param _function     Function for conversion from a string to a number
 */
#define DECLARE_NUMERIC_TYPE(_type, _format, _function)               \
static te_errno                                                       \
TE_CONCAT(_type, _to_str)(te_string *str, const char *name,           \
                          const void *arg)                            \
{                                                                     \
    return te_string_append(str, "%s=" _format, name, *(_type*)arg);  \
}                                                                     \
                                                                      \
static te_errno                                                       \
TE_CONCAT(_type, _from_str)(const char *value, void *arg,             \
                            const void *options)                      \
{                                                                     \
    char   *end;                                                      \
    _type   num;                                                      \
                                                                      \
    UNUSED(options);                                                  \
                                                                      \
    num = (_type)_function(value, &end, 10);                          \
    if ((end == value) || (end[0] != 0))                              \
        return TE_EINVAL;                                             \
                                                                      \
    *(_type*)arg = num;                                               \
    return 0;                                                         \
}                                                                     \
                                                                      \
static int                                                            \
TE_CONCAT(_type, _compare)(const void *a, const void *b)              \
{                                                                     \
    return *(_type*)a == *(_type*)b;                                  \
}                                                                     \
                                                                      \
static int TE_CONCAT(_type, _compare)                                 \
(const void *a, const void *b)

DECLARE_NUMERIC_TYPE(uint8_t,  "%u", strtoul);
DECLARE_NUMERIC_TYPE(uint32_t, "%u", strtoul);


/**
 * Transform a structure @b sockaddr to a structure @b te_string
 *
 * @param [in,out]str   String with data
 * @param [in]   name   Field name
 * @param [in]   arg    Pointer to a value
 *
 * @return              Status code
 */
static te_errno
sockaddr_to_str(te_string *str, const char *name, const void *arg)
{
    te_errno    rc;
    char       *buf = NULL;

    rc = te_sockaddr_h2str(CONST_SA(arg), &buf);
    if (rc == 0)
    {
        rc = te_string_append(str, "%s=%s", name, buf);
        free(buf);
    }
    return rc;
}

/**
 * Transform a string @b char * to a structure @b sockaddr
 *
 * @param [in] value    Input value
 * @param [out]arg      Pointer to a value
 * @param [in] options  User options (unused)
 *
 * @return              Status code
 */
static te_errno
sockaddr_from_str(const char *value, void *arg, const void *options)
{
    UNUSED(options);

    return te_sockaddr_str2h(value, SA(arg));
}

/**
 * Compare two structures @b sockaddr
 *
 * @param [in]a     First structure @b sockaddr
 * @param [in]b     Second structure @b sockaddr
 *
 * @return          Result of the comparison
 * @retval ==0      Values are not equal
 * @retval !=0      Values are equal
 */
static int
sockaddr_compare(const void *a, const void *b)
{
    const struct sockaddr *sa_a = a;
    const struct sockaddr *sa_b = b;
    if (sa_a->sa_family != sa_b->sa_family)
        return 0;

    switch (sa_a->sa_family)
    {
        case AF_INET:
            return memcmp(&SIN(sa_a)->sin_addr, &SIN(sa_b)->sin_addr,
                          sizeof(struct in_addr)) == 0;

        case AF_INET6:
            return memcmp(&SIN6(sa_a)->sin6_addr, &SIN6(sa_b)->sin6_addr,
                          sizeof(struct in6_addr)) == 0;
    }

    ERROR("%s:%d: Unknown address family (%d)",
          __FUNCTION__, __LINE__, sa_a->sa_family);
    return 0;
}


/**
 * Transform a string @b char * to a structure @b te_string
 *
 * @param [in,out]str   String with data
 * @param [in]name      Field name
 * @param [in]arg       Pointer to a value
 *
 * @return              Status code
 *
 * @note Input string (@p arg) should not contain symbol ","
 */
static te_errno
str_to_str(te_string *str, const char *name, const void *arg)
{
    return te_string_append(str, "%s=%s", name, (char*)arg);
}

/**
 * Transform a string @b char * to string @b char * and check length
 *
 * @param [in] value    Input value
 * @param [out]arg      Pointer to a value
 * @param [in] options  Pointer to a maximum length of an output string
 *
 * @return              Status code
 */
static te_errno
str_from_str(const char *value, void *arg, const void *options)
{
    const size_t max_size = *(const size_t*)options;
    size_t current_size = strlen(value);

    if (current_size >= max_size)
        return TE_ENOMEM;

    // string plus the terminating zero
    memcpy(arg, value, current_size+1);
    return 0;
}

/**
 * Compare two strings @b char*
 *
 * @param [in]a     First string
 * @param [in]b     Second string
 *
 * @return          Result of the comparison
 * @retval ==0      Strings are not equal
 * @retval !=0      Strings are equal
 */
int
str_compare(const void *a, const void *b)
{
    return strcmp((const char*)a, (const char*)b) == 0;
}

te_conf_obj_methods te_conf_obj_methods_te_bool = {
    .to_str   = te_bool_to_str,
    .from_str = te_bool_from_str,
    .compare  = te_bool_compare,
};
te_conf_obj_methods te_conf_obj_methods_uint8_t = {
    .to_str   = uint8_t_to_str,
    .from_str = uint8_t_from_str,
    .compare  = uint8_t_compare,
};
te_conf_obj_methods te_conf_obj_methods_uint32_t = {
    .to_str   = uint32_t_to_str,
    .from_str = uint32_t_from_str,
    .compare  = uint32_t_compare,
};
te_conf_obj_methods te_conf_obj_methods_sockaddr = {
    .to_str   = sockaddr_to_str,
    .from_str = sockaddr_from_str,
    .compare  = sockaddr_compare,
};
te_conf_obj_methods te_conf_obj_methods_str = {
    .to_str   = str_to_str,
    .from_str = str_from_str,
    .compare  = str_compare,
};

/* See the description in conf_object.h */
te_errno
te_conf_obj_to_str(const te_conf_obj *fields, size_t fields_number,
                   const void *base, uint32_t mask, char **str)
{
    size_t      i;
    const void *arg;

    te_errno    rc = 0;
    te_string   string = TE_STRING_INIT;

/**
 * The short form of the error message
 *
 * @param _fmt      Format string
 * @param _args     Arguments for @p _fmt
 */
#define ERROR_MSG(_fmt, _args...)                           \
    ERROR("%s: Failed transform object to string, " _fmt,   \
          __FUNCTION__, _args)

    for (i = 0; i < fields_number; i++)
        if ((mask & fields[i].flag) != 0)
        {
            if (string.ptr && (rc = te_string_append(&string, ",")) != 0)
            {
                ERROR_MSG("%s", "not enough memory");
                break;
            }

            arg = base + fields[i].offset;
            rc = fields[i].methods->to_str(&string, fields[i].name, arg);
            if (rc != 0)
            {
                ERROR_MSG("for field '%s' (%r)", fields[i].name, rc);
                break;
            }
        }

    if (rc == 0)
    {
        *str = strdup(string.ptr);
        if (*str == NULL)
        {
            ERROR_MSG("%s", "not enough memory");
            rc = TE_ENOMEM;
        }
    }

#undef ERROR_MSG

    te_string_free(&string);
    return rc;
}

/* See the description in conf_object.h */
te_errno
te_conf_obj_from_str(const te_conf_obj *fields, size_t fields_number,
                     const char *str, uint32_t *required,
                     void *base, uint32_t *mask)
{
    void      *arg;
    char      *name;
    char      *value;
    char      *str_buf;

    size_t     i = 0;
    te_errno   rc = 0;

/**
 * The short form of the error message
 *
 * @param _fmt      Format string
 * @param _args     Arguments for @p _fmt
 */
#define ERROR_MSG(_fmt, _args...)                               \
    ERROR("%s: Failed to transform string to object, " _fmt,    \
          __FUNCTION__, _args)

    str_buf = strdup(str);
    if (str_buf == NULL)
    {
        ERROR_MSG("%s", "not enough memory");
        return TE_ENOMEM;
    }

    name = strtok(str_buf, ",");
    while (name)
    {
        rc = TE_EINVAL;

        value = strchr(name, '=');
        if (value != NULL)
        {
            *value++ = 0;

            for (i = 0; i < fields_number; i++)
            {
                if (strcmp(name, fields[i].name) != 0)
                    continue;

                arg = base + fields[i].offset;
                rc = fields[i].methods->from_str(value, arg,
                                                 fields[i].options);
                if (rc == 0)
                    *mask |= fields[i].flag;
                break;
            }
        }
        else if (name[0] == '!')
        {
            for (i = 0; i < fields_number; i++)
            {
                if (strcmp(name, fields[i].name) != 0)
                    continue;

                rc = 0;
                *required |= fields[i].flag;
                break;
            }
        }
        else
        {
            ERROR_MSG("invalid field '%s'", name);
            rc = TE_EINVAL;
            break;
        }

        if (rc != 0)
        {
            if (i == fields_number)
                ERROR_MSG("cannot find the field '%s'", name);
            else
            {
                ERROR_MSG("cannot parse the field value '%s'='%s'",
                          name, value);
            }
            break;
        }

        name = strtok(NULL, ",");
    }

#undef ERROR_MSG

    *required |= *mask;
    return rc;
}

/* See the description in conf_object.h */
te_conf_obj_compare_result
te_conf_obj_compare(const te_conf_obj *fields, size_t fields_number,
                    uint32_t required, const void *base_a, uint32_t mask_a,
                    const void *base_b, uint32_t mask_b)
{
    size_t          i;
    const void     *a;
    const void     *b;

    te_conf_obj_compare_result  result = TE_CONF_OBJ_COMPARE_EQUAL;

    if ((~required & mask_a) != 0)
        return TE_CONF_OBJ_COMPARE_ERROR;

    for (i = 0; i < fields_number; i++)
    {
        const te_bool required_for_a = (required & fields[i].flag) != 0;
        const te_bool filled_for_a = (mask_a  & fields[i].flag) != 0;
        const te_bool filled_for_b = (mask_b  & fields[i].flag) != 0;

        if (!required_for_a)
        {
            if (filled_for_b)
                result = TE_CONF_OBJ_COMPARE_CONTAINS;
        }
        else
        {
            if (filled_for_a != filled_for_b)
                return TE_CONF_OBJ_COMPARE_DIFFERENT;
            else if(filled_for_a)
            {
                a = base_a + fields[i].offset;
                b = base_b + fields[i].offset;
                if (!fields[i].methods->compare(a, b))
                    return TE_CONF_OBJ_COMPARE_DIFFERENT;
            }
        }
    }

    return result;
}
