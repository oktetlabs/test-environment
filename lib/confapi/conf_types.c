/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Configurator
 *
 * Primary types support
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#include <stdio.h>

#include "te_config.h"
#include "te_str.h"

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include "te_alloc.h"
#include "te_stdint.h"
#include "te_errno.h"
#include "te_defs.h"
#include "conf_api.h"
#include "conf_messages.h"
#include "conf_types.h"
#include "logger_api.h"

#define MAC_ADDR_LEN          6

#define CFG_TP_MAX_BUF        255

#define SET_MSG_LEN(_msg) \
    _msg->len = (_msg->type == CFG_ADD) ? sizeof(cfg_add_msg) : \
                (_msg->type == CFG_SET) ? sizeof(cfg_set_msg) : \
                                          sizeof(cfg_get_msg)

/* Zero of type double */
#define DBL_ZERO 0.0

const te_enum_map cfg_cvt_mapping[] = {
    {.name = "bool",     .value = CVT_BOOL},
    {.name = "int8",     .value = CVT_INT8},
    {.name = "uint8",    .value = CVT_UINT8},
    {.name = "int16",    .value = CVT_INT16},
    {.name = "uint16",   .value = CVT_UINT16},
    {.name = "int32",    .value = CVT_INT32},
    {.name = "integer",  .value = CVT_INT32},
    {.name = "uint32",   .value = CVT_UINT32},
    {.name = "int64",    .value = CVT_INT64},
    {.name = "uint64",   .value = CVT_UINT64},
    {.name = "double",   .value = CVT_DOUBLE},
    {.name = "string",   .value = CVT_STRING},
    {.name = "address",  .value = CVT_ADDRESS},
    {.name = "none",     .value = CVT_NONE},
    TE_ENUM_MAP_END
};

/* Locals */

static uintmax_t max_int_val[CFG_PRIMARY_TYPES_NUM + 1] = { [CVT_BOOL] = 1,
    [CVT_INT8] = INT8_MAX, [CVT_UINT8] = UINT8_MAX,
    [CVT_INT16] = INT16_MAX, [CVT_UINT16] = UINT16_MAX,
    [CVT_INT32] = INT32_MAX, [CVT_UINT32] = UINT32_MAX,
    [CVT_INT64] = INT64_MAX, [CVT_UINT64] = UINT64_MAX,
    [CVT_STRING] = 0, [CVT_ADDRESS] = 0, [CVT_NONE] = 0, [CVT_UNSPECIFIED] = 0};

static intmax_t min_int_val[CFG_PRIMARY_TYPES_NUM + 1] = { [CVT_BOOL] = 0,
    [CVT_INT8] = INT8_MIN, [CVT_UINT8] = 0,
    [CVT_INT16] = INT16_MIN, [CVT_UINT16] = 0,
    [CVT_INT32] = INT32_MIN, [CVT_UINT32] = 0,
    [CVT_INT64] = INT64_MIN, [CVT_UINT64] = 0,
    [CVT_STRING] = 0, [CVT_ADDRESS] = 0, [CVT_NONE] = 0, [CVT_UNSPECIFIED] = 0};

/* Check if given address family value is valid */
static bool
addr_valid_family(unsigned short af)
{
    switch (af)
    {
        case AF_INET:
        case AF_INET6:
        case AF_LOCAL:
        case AF_UNSPEC:
            return true;
        default:
            return false;
    }
}

/* Get size of sockaddr struct that corresponds to given address family */
static size_t
addr_size(unsigned short af)
{
    switch (af)
    {
        case AF_INET:
            return sizeof(struct sockaddr_in);
        case AF_INET6:
            return sizeof(struct sockaddr_in6);
        case AF_LOCAL:
        case AF_UNSPEC:
            return sizeof(struct sockaddr);
        default:
            return 0;
    }
}

#define DECLARE_CFG_TYPE_METHODS(variant_) \
static te_errno str_to_ ## variant_(char *val_str, cfg_inst_val *val);       \
static te_errno variant_ ## _to_str(cfg_inst_val val, char **val_str);       \
static te_errno variant_ ## _def_val(cfg_inst_val *val);                     \
static void variant_ ## _free(cfg_inst_val val);                             \
static te_errno variant_ ## _copy(cfg_inst_val src, cfg_inst_val *dst);      \
static te_errno variant_ ## _get(cfg_msg *msg, cfg_inst_val *val);           \
static void variant_ ## _put(cfg_inst_val val, cfg_msg *msg);                \
static bool variant_ ## _equal(cfg_inst_val first, cfg_inst_val second);  \
static size_t variant_ ## _value_size(cfg_inst_val val)

DECLARE_CFG_TYPE_METHODS(bool);
DECLARE_CFG_TYPE_METHODS(int8);
DECLARE_CFG_TYPE_METHODS(uint8);
DECLARE_CFG_TYPE_METHODS(int16);
DECLARE_CFG_TYPE_METHODS(uint16);
DECLARE_CFG_TYPE_METHODS(int32);
DECLARE_CFG_TYPE_METHODS(uint32);
DECLARE_CFG_TYPE_METHODS(int64);
DECLARE_CFG_TYPE_METHODS(uint64);
DECLARE_CFG_TYPE_METHODS(double);
DECLARE_CFG_TYPE_METHODS(string);
DECLARE_CFG_TYPE_METHODS(addr);
DECLARE_CFG_TYPE_METHODS(none);

#undef DECLARE_CFG_TYPE_METHODS

/* Primary types' conversion functions */

#define CFG_TYPE_METHODS(variant_, cvt_type_) \
    [cvt_type_] =                                                            \
    { str_to_ ## variant_, variant_ ## _to_str, variant_ ## _def_val,        \
      variant_ ## _free, variant_ ## _copy, variant_ ## _get,                \
      variant_ ## _put, variant_ ## _equal, variant_ ## _value_size }

cfg_primary_type cfg_types[CFG_PRIMARY_TYPES_NUM] = {
    CFG_TYPE_METHODS(bool, CVT_BOOL),
    CFG_TYPE_METHODS(int8, CVT_INT8),
    CFG_TYPE_METHODS(uint8, CVT_UINT8),
    CFG_TYPE_METHODS(int16, CVT_INT16),
    CFG_TYPE_METHODS(uint16, CVT_UINT16),
    CFG_TYPE_METHODS(int32, CVT_INT32),
    CFG_TYPE_METHODS(uint32, CVT_UINT32),
    CFG_TYPE_METHODS(int64, CVT_INT64),
    CFG_TYPE_METHODS(uint64, CVT_UINT64),
    CFG_TYPE_METHODS(double, CVT_DOUBLE),
    CFG_TYPE_METHODS(string, CVT_STRING),
    CFG_TYPE_METHODS(addr, CVT_ADDRESS),
    CFG_TYPE_METHODS(none, CVT_NONE),
};

#undef CFG_TYPE_METHODS

/*-------------------------- integer types handlers ------------------------*/

/* Definitions of conversion-from-string methods for integral types */
static te_errno
str_to_u_integer(cfg_val_type cvt_type, char *val_str, uintmax_t *val)
{
    uintmax_t ret_val = 0;
    te_errno rc;

    if (*val_str == '\0')
    {
        *val = 0;
        return 0;
    }

    rc = te_strtoumax(val_str, 0, &ret_val);

    if (rc != 0)
        return rc;

    if (max_int_val[cvt_type] == 0)
        return TE_EINVAL;
    if (ret_val > max_int_val[cvt_type])
        return TE_EINVAL;

    *val = ret_val;
    return 0;
}

static te_errno
str_to_s_integer(cfg_val_type cvt_type, char *val_str, intmax_t *val)
{
    intmax_t ret_val = 0;
    te_errno rc;

    if (*val_str == '\0')
    {
        *val = 0;
        return 0;
    }

    rc = te_strtoimax(val_str, 0, &ret_val);

    if (rc != 0)
        return rc;

    if (min_int_val[cvt_type] == 0)
        return TE_EINVAL;
    if (ret_val < min_int_val[cvt_type] ||
        ret_val > (intmax_t) max_int_val[cvt_type])
        return TE_EINVAL;

    *val = ret_val;
    return 0;
}

#define DEFINE_STR_TO_INTEGER(variant_, cvt_type_, max_type_, \
                              convert_function_, type_)                     \
static te_errno                                                             \
str_to_ ## variant_(char *val_str, cfg_inst_val *val)                       \
{                                                                           \
    max_type_ val_tmp;                                                      \
    te_errno rc;                                                            \
                                                                            \
    rc = convert_function_(cvt_type_, val_str, &val_tmp);                   \
    if (rc == 0)                                                            \
        val->val_ ## variant_ = (type_)val_tmp;                             \
    return rc;                                                              \
}                                                                           \
struct noop

DEFINE_STR_TO_INTEGER(bool, CVT_BOOL, uintmax_t, str_to_u_integer, bool);
DEFINE_STR_TO_INTEGER(int8, CVT_INT8, intmax_t, str_to_s_integer, int8_t);
DEFINE_STR_TO_INTEGER(uint8, CVT_UINT8, uintmax_t, str_to_u_integer, uint8_t);
DEFINE_STR_TO_INTEGER(int16, CVT_INT16, intmax_t, str_to_s_integer, int16_t);
DEFINE_STR_TO_INTEGER(uint16, CVT_UINT16, uintmax_t, str_to_u_integer, uint16_t);
DEFINE_STR_TO_INTEGER(int32, CVT_INT32, intmax_t, str_to_s_integer, int32_t);
DEFINE_STR_TO_INTEGER(uint32, CVT_UINT32, uintmax_t, str_to_u_integer, uint32_t);
DEFINE_STR_TO_INTEGER(int64, CVT_INT64, intmax_t, str_to_s_integer, int64_t);
DEFINE_STR_TO_INTEGER(uint64, CVT_UINT64, uintmax_t, str_to_u_integer, uint64_t);

#undef DEFINE_STR_TO_INTEGER

/* Definitions of conversion-to-string methods for unsigned integral types */
static te_errno
u_integer_to_str(cfg_val_type cvt_type, uintmax_t val,
               char **val_str)
{
    char str[CFG_TP_MAX_BUF];
    te_errno ret_val;

    if (max_int_val[cvt_type] == 0)
        return TE_EINVAL;
    ret_val = snprintf(str, CFG_TP_MAX_BUF - 1, "%ju", val);
    if (ret_val < 1)
        return TE_EINVAL;
    else if (ret_val >= CFG_TP_MAX_BUF)
        return TE_ENOBUFS;

    *val_str = strdup(str);
    if (*val_str == NULL)
        TE_FATAL_ERROR("Not enough memory");
    return 0;
}

#define DEFINE_U_INTEGER_TO_STR(variant_, cvt_type_) \
static te_errno                                                           \
variant_ ## _to_str(cfg_inst_val val, char **val_str)                     \
{                                                                         \
    return u_integer_to_str(cvt_type_, (intmax_t)val.val_ ## variant_,    \
                            val_str);                                     \
}                                                                         \
struct noop

DEFINE_U_INTEGER_TO_STR(bool, CVT_BOOL);
DEFINE_U_INTEGER_TO_STR(uint8, CVT_UINT8);
DEFINE_U_INTEGER_TO_STR(uint16, CVT_UINT16);
DEFINE_U_INTEGER_TO_STR(uint32, CVT_UINT32);
DEFINE_U_INTEGER_TO_STR(uint64, CVT_UINT64);

#undef DEFINE_U_INTEGER_TO_STR

/* Definitions of conversion-to-string methods for signed integral types */
static te_errno
s_integer_to_str(cfg_val_type cvt_type, intmax_t val, char **val_str)
{
    char str[CFG_TP_MAX_BUF];
    te_errno ret_val;

    if (min_int_val[cvt_type] == 0)
        return TE_EINVAL;
    ret_val = snprintf(str, CFG_TP_MAX_BUF - 1, "%jd", val);
    if (ret_val < 1)
        return TE_EINVAL;
    else if (ret_val >= CFG_TP_MAX_BUF)
        return TE_ENOBUFS;

    *val_str = strdup(str);
    if (*val_str == NULL)
        TE_FATAL_ERROR("Not enough memory");
    return 0;
}


#define DEFINE_S_INTEGER_TO_STR(variant_, cvt_type_) \
static te_errno                                                         \
variant_ ## _to_str(cfg_inst_val val, char **val_str)                   \
{                                                                       \
    return s_integer_to_str(cvt_type_, (intmax_t)val.val_ ## variant_,  \
                            val_str);                                   \
}                                                                       \
struct noop

DEFINE_S_INTEGER_TO_STR(int8, CVT_INT8);
DEFINE_S_INTEGER_TO_STR(int16, CVT_INT16);
DEFINE_S_INTEGER_TO_STR(int32, CVT_INT32);
DEFINE_S_INTEGER_TO_STR(int64, CVT_INT64);

#undef DEFINE_S_INTEGER_TO_STR

/* Definitions of default-value methods for integral types */

#define DEFINE_INTEGER_DEF_VAL(variant_) \
static te_errno                                       \
variant_ ## _def_val(cfg_inst_val *val)               \
{                                                     \
    val->val_ ## variant_ = 0;                        \
    return 0;                                         \
}                                                     \
struct noop

DEFINE_INTEGER_DEF_VAL(bool);
DEFINE_INTEGER_DEF_VAL(int8);
DEFINE_INTEGER_DEF_VAL(uint8);
DEFINE_INTEGER_DEF_VAL(int16);
DEFINE_INTEGER_DEF_VAL(uint16);
DEFINE_INTEGER_DEF_VAL(int32);
DEFINE_INTEGER_DEF_VAL(uint32);
DEFINE_INTEGER_DEF_VAL(int64);
DEFINE_INTEGER_DEF_VAL(uint64);

#undef DEFINE_STR_TO_INTEGER

/* Definitions of free methods for integral types (they are dummy) */

#define DEFINE_INTEGER_FREE(variant_) \
static void                                  \
variant_ ## _free(cfg_inst_val val)          \
{                                            \
    UNUSED(val);                             \
    return;                                  \
}                                            \
struct noop

DEFINE_INTEGER_FREE(bool);
DEFINE_INTEGER_FREE(int8);
DEFINE_INTEGER_FREE(uint8);
DEFINE_INTEGER_FREE(int16);
DEFINE_INTEGER_FREE(uint16);
DEFINE_INTEGER_FREE(int32);
DEFINE_INTEGER_FREE(uint32);
DEFINE_INTEGER_FREE(int64);
DEFINE_INTEGER_FREE(uint64);

#undef DEFINE_INTEGER_FREE

/* Definitions of copy methods for integral types */

#define DEFINE_INTEGER_COPY(variant_) \
static te_errno                                          \
variant_ ## _copy(cfg_inst_val src, cfg_inst_val *dst)   \
{                                                        \
    dst->val_ ## variant_ = src.val_ ## variant_;        \
    return 0;                                            \
}                                                        \
struct noop

DEFINE_INTEGER_COPY(bool);
DEFINE_INTEGER_COPY(int8);
DEFINE_INTEGER_COPY(uint8);
DEFINE_INTEGER_COPY(int16);
DEFINE_INTEGER_COPY(uint16);
DEFINE_INTEGER_COPY(int32);
DEFINE_INTEGER_COPY(uint32);
DEFINE_INTEGER_COPY(int64);
DEFINE_INTEGER_COPY(uint64);

#undef DEFINE_INTEGER_COPY

/* Definitions of get-value methods for integral types */

#define DEFINE_INTEGER_GET(variant_) \
static te_errno                                                                  \
variant_ ## _get(cfg_msg *msg, cfg_inst_val *val)                                \
{                                                                                \
    switch (msg->type)                                                           \
    {                                                                            \
        case CFG_GET:                                                            \
            val->val_ ## variant_ = ((cfg_get_msg *)msg)->val.val_ ## variant_;  \
            break;                                                               \
                                                                                 \
        case CFG_SET:                                                            \
            val->val_ ## variant_ = ((cfg_set_msg *)msg)->val.val_ ## variant_;  \
            break;                                                               \
                                                                                 \
        case CFG_ADD:                                                            \
            val->val_ ## variant_ = ((cfg_add_msg *)msg)->val.val_ ## variant_;  \
            break;                                                               \
                                                                                 \
        default:                                                                 \
            assert(0);                                                           \
    }                                                                            \
    return 0;                                                                    \
}                                                                                \
struct noop

DEFINE_INTEGER_GET(bool);
DEFINE_INTEGER_GET(int8);
DEFINE_INTEGER_GET(uint8);
DEFINE_INTEGER_GET(int16);
DEFINE_INTEGER_GET(uint16);
DEFINE_INTEGER_GET(int32);
DEFINE_INTEGER_GET(uint32);
DEFINE_INTEGER_GET(int64);
DEFINE_INTEGER_GET(uint64);

#undef DEFINE_INTEGER_GET

/* Definitions of put-value methods for integral types */

#define DEFINE_INTEGER_PUT(variant_, type_) \
static void                                                          \
variant_ ## _put(cfg_inst_val val, cfg_msg *msg)                     \
{                                                                    \
    type_ *msg_val;                                                  \
                                                                     \
    if (msg == NULL)                                                 \
        return;                                                      \
                                                                     \
    switch (msg->type)                                               \
    {                                                                \
        case CFG_ADD:                                                \
            msg_val = &(((cfg_add_msg *)msg)->val.val_ ## variant_); \
            break;                                                   \
                                                                     \
        case CFG_SET:                                                \
            msg_val = &(((cfg_set_msg *)msg)->val.val_ ## variant_); \
            break;                                                   \
                                                                     \
        case CFG_GET:                                                \
            msg_val = &(((cfg_get_msg *)msg)->val.val_ ## variant_); \
            break;                                                   \
                                                                     \
        default:                                                     \
            assert(0);                                               \
    }                                                                \
    *msg_val = val.val_ ## variant_;                                 \
                                                                     \
    SET_MSG_LEN(msg);                                                \
}                                                                    \
struct noop

DEFINE_INTEGER_PUT(bool, bool);
DEFINE_INTEGER_PUT(int8, int8_t);
DEFINE_INTEGER_PUT(uint8, uint8_t);
DEFINE_INTEGER_PUT(int16, int16_t);
DEFINE_INTEGER_PUT(uint16, uint16_t);
DEFINE_INTEGER_PUT(int32, int32_t);
DEFINE_INTEGER_PUT(uint32, uint32_t);
DEFINE_INTEGER_PUT(int64, int64_t);
DEFINE_INTEGER_PUT(uint64, uint64_t);

#undef DEFINE_INTEGER_PUT

/* Definitions of equality methods for integral types */

#define DEFINE_INTEGER_EQUAL(variant_) \
static bool                                                                 \
variant_ ## _equal(cfg_inst_val first, cfg_inst_val second)                    \
{                                                                              \
    return first.val_ ## variant_ == second.val_ ## variant_;                  \
}                                                                              \
struct noop

DEFINE_INTEGER_EQUAL(bool);
DEFINE_INTEGER_EQUAL(int8);
DEFINE_INTEGER_EQUAL(uint8);
DEFINE_INTEGER_EQUAL(int16);
DEFINE_INTEGER_EQUAL(uint16);
DEFINE_INTEGER_EQUAL(int32);
DEFINE_INTEGER_EQUAL(uint32);
DEFINE_INTEGER_EQUAL(int64);
DEFINE_INTEGER_EQUAL(uint64);

#undef DEFINE_INTEGER_EQUAL

/* Definitions of get-value-size methods for integral types */

#define DEFINE_INTEGER_VALUE_SIZE(variant_, type_) \
static size_t                                                    \
variant_ ## _value_size(cfg_inst_val val)                        \
{                                                                \
    UNUSED(val);                                                 \
    return sizeof(type_);                                        \
}                                                                \
struct noop

DEFINE_INTEGER_VALUE_SIZE(bool, bool);
DEFINE_INTEGER_VALUE_SIZE(int8, int8_t);
DEFINE_INTEGER_VALUE_SIZE(uint8, uint8_t);
DEFINE_INTEGER_VALUE_SIZE(int16, int16_t);
DEFINE_INTEGER_VALUE_SIZE(uint16, uint16_t);
DEFINE_INTEGER_VALUE_SIZE(int32, int32_t);
DEFINE_INTEGER_VALUE_SIZE(uint32, uint32_t);
DEFINE_INTEGER_VALUE_SIZE(int64, int64_t);
DEFINE_INTEGER_VALUE_SIZE(uint64, uint64_t);

#undef DEFINE_INTEGER_VALUE_SIZE

/*----------------------- Double type handlers -------------------------*/

static te_errno
double_copy(cfg_inst_val src, cfg_inst_val *dst)
{
    dst->val_double = src.val_double;
    return 0;
}

static te_errno
str_to_double(char *val_str, cfg_inst_val *val)
{
    cfg_inst_val ret_val = {.val_double = DBL_ZERO};

    if (!te_str_is_null_or_empty(val_str))
        CHECK_NZ_RETURN(te_strtod(val_str, &ret_val.val_double));

    double_copy(ret_val, val);
    return 0;
}

static te_errno
double_to_str(cfg_inst_val val, char **val_str)
{
    char *out = NULL;

    out = te_string_fmt("%g", val.val_double);
    if (out == NULL)
        return TE_EFAIL;

    *val_str = out;
    return 0;
}

static te_errno
double_def_val(cfg_inst_val *val)
{
    val->val_double = DBL_ZERO;
    return 0;
}

static void
double_free(cfg_inst_val val)
{
    UNUSED(val);
    return;
}

static te_errno
double_get(cfg_msg *msg, cfg_inst_val *val)
{
    switch (msg->type)
    {
        case CFG_ADD:
            val->val_double = ((cfg_add_msg *)msg)->val.val_double;
            break;

        case CFG_GET:
            val->val_double = ((cfg_get_msg *)msg)->val.val_double;
            break;

        case CFG_SET:
            val->val_double = ((cfg_set_msg *)msg)->val.val_double;
            break;

        default:
            assert(0);
    }
    return 0;
}

static void
double_put(cfg_inst_val val, cfg_msg *msg)
{
    double *msg_val;

    if (msg == NULL)
        return;

    switch (msg->type)
    {
        case CFG_ADD:
            msg_val = &(((cfg_add_msg *)msg)->val.val_double);
            break;

        case CFG_SET:
            msg_val = &(((cfg_set_msg *)msg)->val.val_double);
            break;

        case CFG_GET:
            msg_val = &(((cfg_get_msg *)msg)->val.val_double);
            break;

        default:
            assert(0);
    }
    *msg_val = val.val_double;

    SET_MSG_LEN(msg);
}

static bool
double_equal(cfg_inst_val first, cfg_inst_val second)
{
    return first.val_double == second.val_double;
}

static size_t
double_value_size(cfg_inst_val val)
{
    UNUSED(val);
    return sizeof(double);
}

/*----------------------- String type handlers --------------------------*/

static te_errno
str_to_string(char *val_str, cfg_inst_val *val)
{
    if (val_str == NULL)
        return TE_EINVAL;

    return (*(char **)val = strdup(val_str)) == NULL ? TE_ENOMEM : 0;
}

static te_errno
string_to_str(cfg_inst_val val, char **val_str)
{
    if (val.val_str == NULL)
        return TE_EINVAL;

    return (*val_str = strdup(val.val_str)) == NULL ? TE_ENOMEM : 0;
}

static te_errno
string_def_val(cfg_inst_val *val)
{
    return (*(char **)val = strdup("")) == NULL ? TE_ENOMEM : 0;
}

static void
string_free(cfg_inst_val val)
{
    char *str = val.val_str;

    if (str != NULL)
        free(str);
    return;
}

static te_errno
string_copy(cfg_inst_val src, cfg_inst_val *dst)
{
    if (src.val_str == NULL)
        return TE_EINVAL;

    return (*(char **)dst = strdup(src.val_str)) == NULL ? TE_ENOMEM : 0;
}

static te_errno
string_get(cfg_msg *msg, cfg_inst_val *val)
{
    char *msg_str;

    if (msg == NULL)
         return TE_EINVAL;

    msg_str = (msg->type == CFG_ADD) ? ((cfg_add_msg *)msg)->val.val_str :
              (msg->type == CFG_SET) ? ((cfg_set_msg *)msg)->val.val_str :
              ((cfg_get_msg *)msg)->val.val_str;

    return ((val->val_str = strdup(msg_str)) == NULL) ? TE_ENOMEM : 0;
}

static void
string_put(cfg_inst_val val, cfg_msg *msg)
{
    char *msg_str;

    if (msg == NULL)
        return;

    msg_str = (msg->type == CFG_ADD) ? ((cfg_add_msg *)msg)->val.val_str :
              (msg->type == CFG_SET) ? ((cfg_set_msg *)msg)->val.val_str :
              ((cfg_get_msg *)msg)->val.val_str;

    SET_MSG_LEN(msg);

    if (val.val_str == NULL)
    {
        *msg_str = 0;
        msg->len++;
        return;
    }

    msg->len += strlen(val.val_str) + 1;
    strcpy(msg_str, val.val_str);

    return;
}


static bool
string_equal(cfg_inst_val first, cfg_inst_val second)
{
    return (strcmp(first.val_str, second.val_str) == 0) ? true : false;
}

static size_t
string_value_size(cfg_inst_val val)
{
    return strlen(val.val_str) + 1;
}

/*----------------------- Address type handlers --------------------------*/


static te_errno
str_to_addr(char *val_str, cfg_inst_val *val)
{
    if (val_str == NULL)
    {
        return TE_EINVAL;
    }

    /*
     * We should check for ':' first - IPv6 address
     * can too contain '.' if it is IPv6-mapped IPv4 address.
     */
    if (strchr(val_str, ':') != NULL)
    {
        struct sockaddr_in6 *addr6;
        struct sockaddr     *addr;
        uint8_t  mac_addr[MAC_ADDR_LEN];
        uint32_t mac_addr_bytes[MAC_ADDR_LEN];
        int      i;
        char     c;

        /* Probably IPv6 address */
        addr6 = TE_ALLOC(sizeof(*addr6));

        if (inet_pton(AF_INET6, val_str, &(addr6->sin6_addr)) > 0)
        {
            addr6->sin6_family = AF_INET6;
            val->val_addr = (struct sockaddr *)addr6;
            return 0;
        }
        free(addr6);

        /*
         * We can be sure that each element of this array fits into
         * uint8_t as we explicitly specify maximum field width "02"
         * between the '%'  and  'x' symbols.
         * [It's done to prevent compiler warnings]
         */
        if (sscanf(val_str, "%02x:%02x:%02x:%02x:%02x:%02x%c",
                   mac_addr_bytes,     mac_addr_bytes + 1,
                   mac_addr_bytes + 2, mac_addr_bytes + 3,
                   mac_addr_bytes + 4, mac_addr_bytes + 5, &c)
                != MAC_ADDR_LEN)
        {
            return TE_EINVAL;
        }
        for (i = 0; i < MAC_ADDR_LEN; i++)
        {
            mac_addr[i] = mac_addr_bytes[i];
        }

        addr = TE_ALLOC(sizeof(*addr));

        memcpy((void *)(addr->sa_data), (void *)(mac_addr), MAC_ADDR_LEN);

        addr->sa_family = AF_LOCAL;
        val->val_addr = (struct sockaddr *)addr;
    }
    else if (strchr(val_str, '.') != NULL)
    {
        /* Probably IPv4 address */
        struct sockaddr_in *addr;

        addr = TE_ALLOC(sizeof(*addr));

        if (inet_pton(AF_INET, val_str, &(addr->sin_addr)) <= 0)
        {
            free(addr);
            return TE_EINVAL;
        }
        addr->sin_family = AF_INET;
        val->val_addr = (struct sockaddr *)addr;
    }
    else if (strlen(val_str) == 0)
    {
        struct sockaddr    *addr;

        /* Unspecified address */
        addr = TE_ALLOC(sizeof(*addr));

        addr->sa_family = AF_UNSPEC;
        val->val_addr = addr;
    }
    else
    {
        return TE_EINVAL;
    }

    return 0;
}

static te_errno
addr_to_str(cfg_inst_val val, char **val_str)
{
    char val_buf[CFG_TP_MAX_BUF];
    const char *ret_val;
    int len;

    struct sockaddr *val_addr = val.val_addr;

    switch (val_addr->sa_family)
    {
#define CVT_ADDR(af, add) \
    do {                                                          \
        struct sockaddr_in##add *addr;                            \
        addr = (struct sockaddr_in##add *)val_addr;               \
        ret_val = inet_ntop(af, (void *)&(addr->sin##add##_addr), \
                            val_buf, CFG_TP_MAX_BUF);             \
        if (ret_val == NULL) {                                    \
            return TE_EINVAL;                                        \
        }                                                         \
    } while (0)
        case AF_INET:
        {
            CVT_ADDR(AF_INET,);
            break;
        }
        case AF_INET6:
        {
            CVT_ADDR(AF_INET6, 6);
            break;
        }
        case AF_LOCAL:
        {
#define MAC_ADDR_STR_LEN          17
            int size;
            unsigned char *mac = (unsigned char *)(val_addr->sa_data);

            size = snprintf(val_buf, CFG_TP_MAX_BUF,
                            "%02x:%02x:%02x:%02x:%02x:%02x",
                            (unsigned int)mac[0],
                            (unsigned int)mac[1],
                            (unsigned int)mac[2],
                            (unsigned int)mac[3],
                            (unsigned int)mac[4],
                            (unsigned int)mac[5]);
            if (size != MAC_ADDR_STR_LEN)
            {
                return TE_EINVAL;
            }
            break;
        }
        case AF_UNSPEC:
        {
            val_buf[0] = '\0';
            break;
        }
        default:
        {
            return TE_EINVAL;
        }
    }
    len = strlen(val_buf) + 1;
    *val_str = TE_ALLOC(len);
    memcpy((void *)(*val_str), val_buf, len);
    return 0;
#undef CVT_ADDR
}

static te_errno
addr_def_val(cfg_inst_val *val)
{
    return str_to_addr("", val);
}

static void
addr_free(cfg_inst_val val)
{
    struct sockaddr *addr = val.val_addr;
    free(addr);
    val.val_addr = NULL;
    return;
}

static te_errno
addr_copy(cfg_inst_val src, cfg_inst_val *dst)
{
    struct sockaddr *src_addr = src.val_addr;
    struct sockaddr *dst_addr;

    if (src_addr == NULL)
    {
        dst->val_addr = NULL;
        return 0;
    }

    if (!addr_valid_family(src_addr->sa_family))
        return TE_EINVAL;

    dst_addr = TE_ALLOC(addr_value_size(src));

    memcpy(dst_addr, src_addr, addr_value_size(src));
    dst->val_addr = dst_addr;

    return 0;
}

static te_errno
addr_get(cfg_msg *msg, cfg_inst_val *val)
{
    struct sockaddr *msg_addr;
    struct sockaddr *addr;

    if (msg == NULL)
    {
        return TE_EINVAL;
    }
    msg_addr = (msg->type == CFG_ADD) ? ((cfg_add_msg *)msg)->val.val_addr :
               (msg->type == CFG_SET) ? ((cfg_set_msg *)msg)->val.val_addr :
                                        ((cfg_get_msg *)msg)->val.val_addr;

    if (!addr_valid_family(msg_addr->sa_family))
        return TE_EINVAL;

    addr = TE_ALLOC(addr_size(msg_addr->sa_family));

    memcpy(addr, msg_addr, addr_size(msg_addr->sa_family));
    val->val_addr = addr;

    return 0;
}

static void
addr_put(cfg_inst_val val, cfg_msg *msg)
{
    struct sockaddr *msg_addr;
    struct sockaddr *addr = val.val_addr;

    if (msg == NULL)
    {
        return;
    }
    SET_MSG_LEN(msg);

    msg_addr = (msg->type == CFG_ADD) ? ((cfg_add_msg *)msg)->val.val_addr :
               (msg->type == CFG_SET) ? ((cfg_set_msg *)msg)->val.val_addr :
                                        ((cfg_get_msg *)msg)->val.val_addr;

    if (addr_valid_family(addr->sa_family))
    {
        memcpy(msg_addr, addr, addr_size(addr->sa_family));
        msg->len += addr_size(addr->sa_family);
    }

    return;
}

static bool
addr_equal(cfg_inst_val first, cfg_inst_val second)
{
    struct sockaddr *first_addr = first.val_addr;
    struct sockaddr *second_addr = second.val_addr;
    int ret_val;

    if (first_addr->sa_family != second_addr->sa_family)
        return false;

    if (!addr_valid_family(first_addr->sa_family))
        return false;

    ret_val = memcmp(first_addr, second_addr, addr_value_size(first));
    return (ret_val == 0) ? true : false;
}

static size_t
addr_value_size(cfg_inst_val val)
{
    return addr_size(val.val_addr->sa_family);
}

/*----------------------- None type handlers -------------------------*/

static te_errno
str_to_none(char *val_str, cfg_inst_val *val)
{
    UNUSED(val_str);
    UNUSED(val);
    return 0;
}

static te_errno
none_to_str(cfg_inst_val val, char **val_str)
{
    UNUSED(val);
    UNUSED(val_str);
    return 0;
}

static te_errno
none_def_val(cfg_inst_val *val)
{
    UNUSED(val);
    return TE_EINVAL;
}

static void
none_free(cfg_inst_val val)
{
    UNUSED(val);
    return;
}

static te_errno
none_copy(cfg_inst_val src, cfg_inst_val *dst)
{
    UNUSED(src);
    UNUSED(dst);
    return 0;
}

static te_errno
none_get(cfg_msg *msg, cfg_inst_val *val)
{
    UNUSED(msg);
    UNUSED(val);
    return 0;
}

static void
none_put(cfg_inst_val val, cfg_msg *msg)
{
    UNUSED(val);
    SET_MSG_LEN(msg);
    return;
}

static bool
none_equal(cfg_inst_val first, cfg_inst_val second)
{
    UNUSED(first);
    UNUSED(second);
    return true;
}

static size_t
none_value_size(cfg_inst_val val)
{
    UNUSED(val);
    return 0;
}
