/** @file
 * @brief Configurator
 *
 * Primary types support
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 *
 *
 * @author Renata Sayakhova <Renata.Sayakhova@oktetlabs.ru>
 * @author Elena Vengerova  <Elena.Vengerova@oktetlabs.ru>
 *
 * $Id$
 */

#include "te_config.h"

#include <stdio.h>

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


#include "te_stdint.h"
#include "te_errno.h"
#include "te_defs.h"
#include "conf_api.h"
#include "conf_messages.h"
#include "conf_types.h"

#define MAC_ADDR_LEN          6

#define CFG_TP_MAX_BUF        255

#define SET_MSG_LEN(_msg) \
    _msg->len = (_msg->type == CFG_ADD) ? sizeof(cfg_add_msg) : \
                (_msg->type == CFG_SET) ? sizeof(cfg_set_msg) : \
                                          sizeof(cfg_get_msg)

/* Locals */

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

/* Convertion functions for type 'int' */
static int str2int(char *val_str, cfg_inst_val *val);
static int int2str(cfg_inst_val val, char **val_str);
static int int_def_val(cfg_inst_val *val);
static void int_free(cfg_inst_val val);
static int int_copy(cfg_inst_val src, cfg_inst_val *dst);
static int int_get(cfg_msg *msg, cfg_inst_val *val);
static void int_put(cfg_inst_val val, cfg_msg *msg);
static te_bool int_equal(cfg_inst_val first, cfg_inst_val second);
static size_t int_value_size(cfg_inst_val val);

/* Convertion functions for type 'char *' */
static int str2char(char *val_str, cfg_inst_val *val);
static int char2str(cfg_inst_val val, char **val_str);
static int str_def_val(cfg_inst_val *val);
static void str_free(cfg_inst_val val);
static int str_copy(cfg_inst_val src, cfg_inst_val *dst);
static int str_get(cfg_msg *msg, cfg_inst_val *val);
static void str_put(cfg_inst_val val, cfg_msg *msg);
static te_bool str_equal(cfg_inst_val first, cfg_inst_val second);
static size_t str_value_size(cfg_inst_val val);

/* Convertion functions for type 'sockaddr *' */
static int str2addr(char *val_str, cfg_inst_val *val);
static int addr2str(cfg_inst_val val, char **val_str);
static int addr_def_val(cfg_inst_val *val);
static void addr_free(cfg_inst_val val);
static int addr_copy(cfg_inst_val src, cfg_inst_val *dst);
static int addr_get(cfg_msg *msg, cfg_inst_val *val);
static void addr_put(cfg_inst_val val, cfg_msg *msg);
static te_bool addr_equal(cfg_inst_val first, cfg_inst_val second);
static size_t addr_value_size(cfg_inst_val val);

/* Convertion dummy functions for type 'none' */
static int str2none(char *val_str, cfg_inst_val *val);
static int none2str(cfg_inst_val val, char **val_str);
static int none_def_val(cfg_inst_val *val);
static void none_free(cfg_inst_val val);
static int none_copy(cfg_inst_val src, cfg_inst_val *dst);
static int none_get(cfg_msg *msg, cfg_inst_val *val);
static void none_put(cfg_inst_val val, cfg_msg *msg);
static te_bool none_equal(cfg_inst_val first, cfg_inst_val second);
static size_t none_value_size(cfg_inst_val val);

/* Primary types' convertion functions */
cfg_primary_type cfg_types[CFG_PRIMARY_TYPES_NUM] = {
    { str2int, int2str, int_def_val,
      int_free, int_copy, int_get, int_put, int_equal, int_value_size },
    { str2char, char2str, str_def_val,
      str_free, str_copy, str_get, str_put, str_equal, str_value_size },
    { str2addr, addr2str, addr_def_val,
      addr_free, addr_copy, addr_get, addr_put, addr_equal, addr_value_size },
    { str2none, none2str, none_def_val,
      none_free, none_copy, none_get, none_put, none_equal, none_value_size }
 };

/*----------------------- Integer type handlers -------------------------*/

static int
str2int(char *val_str, cfg_inst_val *val)
{
    char *invalid;
    int ret_val = 0;

    ret_val = strtol(val_str, &invalid, 0);
    if (*invalid != '\0')
    {
        return TE_EINVAL;
    }
    val->val_int = ret_val;
    return 0;
}

static int
int2str(cfg_inst_val val, char **val_str)
{
    char str[CFG_TP_MAX_BUF];
    int ret_val;

    ret_val = snprintf(str, CFG_TP_MAX_BUF, "%d", val.val_int);
    if (ret_val < 1)
    {
        return TE_EINVAL;
    }

    *val_str = (char *)calloc(strlen(str) + 1, 1);
    if (*val_str == NULL)
    {
        return TE_EINVAL;
    }
    memcpy((void *)(*val_str), str, strlen(str) + 1);
    return 0;
}

static int
int_def_val(cfg_inst_val *val)
{
    val->val_int = 0;
    return 0;
}

static void
int_free(cfg_inst_val val)
{
    UNUSED(val);
    return;
}

static int
int_copy(cfg_inst_val src, cfg_inst_val *dst)
{
    dst->val_int = src.val_int;
    return 0;
}

static int
int_get(cfg_msg *msg, cfg_inst_val *val)
{
    val->val_int = msg->type == CFG_GET ? ((cfg_get_msg *)msg)->val.val_int :
                   msg->type == CFG_SET ? ((cfg_set_msg *)msg)->val.val_int :
                   ((cfg_add_msg *)msg)->val.val_int;
    return 0;
}

static void
int_put(cfg_inst_val val, cfg_msg *msg)
{
    int *msg_val;

    if (msg == NULL)
        return;

    msg_val = (msg->type == CFG_ADD) ? &(((cfg_add_msg *)msg)->val.val_int) :
              (msg->type == CFG_SET) ? &(((cfg_set_msg *)msg)->val.val_int) :
              &(((cfg_get_msg *)msg)->val.val_int);
    *msg_val = val.val_int;

    SET_MSG_LEN(msg);
}

static te_bool
int_equal(cfg_inst_val first, cfg_inst_val second)
{
    return ((first.val_int - second.val_int) == 0) ? TRUE : FALSE;
}

static size_t
int_value_size(cfg_inst_val val)
{
    UNUSED(val);
    return sizeof(int);
}

/*----------------------- String type handlers --------------------------*/

static int
str2char(char *val_str, cfg_inst_val *val)
{
    if (val_str == NULL)
        return TE_EINVAL;

    return (*(char **)val = strdup(val_str)) == NULL ? TE_ENOMEM : 0;
}

static int
char2str(cfg_inst_val val, char **val_str)
{
    if (val.val_str == NULL)
        return TE_EINVAL;

    return (*val_str = strdup(val.val_str)) == NULL ? TE_ENOMEM : 0;
}

static int
str_def_val(cfg_inst_val *val)
{
    return (*(char **)val = strdup("")) == NULL ? TE_ENOMEM : 0;
}

static void
str_free(cfg_inst_val val)
{
    char *str = val.val_str;

    if (str != NULL)
        free(str);
    return;
}

static int
str_copy(cfg_inst_val src, cfg_inst_val *dst)
{
    if (src.val_str == NULL)
        return TE_EINVAL;

    return (*(char **)dst = strdup(src.val_str)) == NULL ? TE_ENOMEM : 0;
}

static int
str_get(cfg_msg *msg, cfg_inst_val *val)
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
str_put(cfg_inst_val val, cfg_msg *msg)
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


static te_bool
str_equal(cfg_inst_val first, cfg_inst_val second)
{
    return (strcmp(first.val_str, second.val_str) == 0) ? TRUE : FALSE;
}

static size_t
str_value_size(cfg_inst_val val)
{
    return strlen(val.val_str) + 1;
}

/*----------------------- Address type handlers --------------------------*/


static int
str2addr(char *val_str, cfg_inst_val *val)
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
        addr6 = (struct sockaddr_in6 *)calloc(1, sizeof(*addr6));
        if (addr6 == NULL)
            return TE_ENOMEM;

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

        if ((addr = (struct sockaddr *)calloc(1, sizeof(*addr))) == NULL)
            return TE_ENOMEM;

        memcpy((void *)(addr->sa_data), (void *)(mac_addr), MAC_ADDR_LEN);

        addr->sa_family = AF_LOCAL;
        val->val_addr = (struct sockaddr *)addr;
    }
    else if (strchr(val_str, '.') != NULL)
    {
        /* Probably IPv4 address */
        struct sockaddr_in *addr;

        addr = (struct sockaddr_in *)calloc(1, sizeof(*addr));
        if (addr == NULL)
            return TE_ENOMEM;

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
        addr = (struct sockaddr *)calloc(1, sizeof(*addr));
        if (addr == NULL)
            return TE_ENOMEM;

        addr->sa_family = AF_UNSPEC;
        val->val_addr = addr;
    }
    else
    {
        return TE_EINVAL;
    }

    return 0;
}

static int
addr2str(cfg_inst_val val, char **val_str)
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
    *val_str = (char *)calloc(len, 1);
    if (*val_str == NULL)
    {
        return TE_ENOMEM;
    }
    memcpy((void *)(*val_str), val_buf, len);
    return 0;
#undef CVT_ADDR
}

static int
addr_def_val(cfg_inst_val *val)
{
    return str2addr("", val);
}

static void
addr_free(cfg_inst_val val)
{
    struct sockaddr *addr = val.val_addr;
    free(addr);
    val.val_addr = NULL;
    return;
}

static int
addr_copy(cfg_inst_val src, cfg_inst_val *dst)
{
    struct sockaddr *src_addr = src.val_addr;
    struct sockaddr *dst_addr;

    if (src_addr == NULL)
    {
        dst->val_addr = NULL;
        return 0;
    }

    switch (src_addr->sa_family)
    {
#define CP_ADDR(in, version) \
    do {                                                     \
        dst_addr = (struct sockaddr *)                       \
            calloc(1, sizeof(struct sockaddr##in##version)); \
        if (dst_addr == NULL) {                              \
            return TE_ENOMEM;                                   \
        }                                                    \
        memcpy((void *)(dst_addr), (void *)src_addr,         \
               sizeof(struct sockaddr##in##version));        \
    } while (0)

        case AF_INET:
        {
            CP_ADDR(_in,);
            break;
        }
        case AF_INET6:
        {
            CP_ADDR(_in, 6);
            break;
        }
        case AF_LOCAL:
        case AF_UNSPEC:
        {
            CP_ADDR(,);
            break;
        }
        default:
        {
            return TE_EINVAL;
        }
    }
    dst->val_addr = dst_addr;

    return 0;
#undef CP_ADDR
}

static int
addr_get(cfg_msg *msg, cfg_inst_val *val)
{
    struct sockaddr *msg_addr;

    if (msg == NULL)
    {
        return TE_EINVAL;
    }
    msg_addr = (msg->type == CFG_ADD) ? ((cfg_add_msg *)msg)->val.val_addr :
               (msg->type == CFG_SET) ? ((cfg_set_msg *)msg)->val.val_addr :
                                        ((cfg_get_msg *)msg)->val.val_addr;

    switch (msg_addr->sa_family)
    {
#define CP_ADDR(in, version) \
    do {                                                     \
        struct sockaddr##in##version *addr;                  \
        addr = (struct sockaddr##in##version *)              \
            calloc(1, sizeof(struct sockaddr##in##version)); \
        if (addr == NULL) {                                  \
            return TE_ENOMEM;                                   \
        }                                                    \
        memcpy((void *)(addr), (void *)msg_addr,             \
               sizeof(struct sockaddr##in##version));        \
        val->val_addr = (struct sockaddr *)addr;             \
    } while (0)

        case AF_INET:
        {
            CP_ADDR(_in,);
            break;
        }
        case AF_INET6:
        {
            CP_ADDR(_in, 6);
            break;
        }
        case AF_LOCAL:
        case AF_UNSPEC:
        {
            CP_ADDR(,);
            break;
        }
        default:
        {
            return TE_EINVAL;
        }
    }
    return 0;
#undef CP_ADDR
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

    switch (addr->sa_family)
    {
#define CP_ADDR(in, version) \
    do {                                                     \
        memcpy((void *)(msg_addr), (void *)addr,             \
               sizeof(struct sockaddr##in##version));        \
        msg->len += sizeof(struct sockaddr##in##version);    \
    } while (0)
        case AF_INET:
        {
            CP_ADDR(_in,);
            break;
        }
        case AF_INET6:
        {
            CP_ADDR(_in, 6);
            break;
        }
        case AF_LOCAL:
        case AF_UNSPEC:
        {
            CP_ADDR(,);
            break;
        }
        default:
        {
            break;
        }
    }
    return;
}

static te_bool
addr_equal(cfg_inst_val first, cfg_inst_val second)
{
    struct sockaddr *first_addr = first.val_addr;
    struct sockaddr *second_addr = second.val_addr;
    int ret_val;

    if (first_addr->sa_family != second_addr->sa_family)
    {
        return FALSE;
    }
    switch (first_addr->sa_family)
    {
#define CMP_ADDR(in, version) \
    do {                                                          \
        ret_val = memcmp((void *)first_addr, (void *)second_addr, \
                         sizeof(struct sockaddr##in##version));   \
    } while (0)
        case AF_INET:
        {
            CMP_ADDR(_in,);
            break;
        }
        case AF_INET6:
        {
            CMP_ADDR(_in, 6);
            break;
        }
        case AF_LOCAL:
        case AF_UNSPEC:
        {
            CMP_ADDR(,);
            break;
        }
        default:
        {
            return FALSE;
        }
    }
    return (ret_val == 0) ? TRUE : FALSE;
#undef CMP_ADDR
}

static size_t
addr_value_size(cfg_inst_val val)
{
    return addr_size(val.val_addr->sa_family);
}

/*----------------------- None type handlers -------------------------*/

static int
str2none(char *val_str, cfg_inst_val *val)
{
    UNUSED(val_str);
    UNUSED(val);
    return 0;
}

static int
none2str(cfg_inst_val val, char **val_str)
{
    UNUSED(val);
    UNUSED(val_str);
    return 0;
}

static int
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

static int
none_copy(cfg_inst_val src, cfg_inst_val *dst)
{
    UNUSED(src);
    UNUSED(dst);
    return 0;
}

static int
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

static te_bool
none_equal(cfg_inst_val first, cfg_inst_val second)
{
    UNUSED(first);
    UNUSED(second);
    return TRUE;
}

static size_t
none_value_size(cfg_inst_val val)
{
    UNUSED(val);
    return 0;
}
