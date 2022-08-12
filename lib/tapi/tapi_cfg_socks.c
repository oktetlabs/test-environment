/** @file
 * @brief Test API to control Socks configurator tree.
 *
 * Implementation of TAPI to configure Socks.
 *
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 * @author Svetlana Fishchuk <Svetlana.Fishchuk@oktetlabs.ru>
 */

#include "te_config.h"
#if HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

#include "tapi_cfg_socks.h"

#ifdef STDC_HEADERS
#include <stdlib.h>
#include <string.h>
#endif

#include "te_defs.h"
#include "logger_api.h"
#include "tapi_cfg.h"
#include "tapi_cfg_base.h"
#include "tapi_cfg_aggr.h"
#include "conf_api.h"
#include "te_sockaddr.h"
#include "tapi_mem.h"

/* Macros to ensure no typos in common part of addresses */
#define TE_CFG_TA_SOCKS_FMT             "/agent:%s/socks:%s"
#define TE_CFG_TA_SOCKS_ARGS(ta_, id_)  (ta_), (const char *)(id_)

/* See description in tapi_cfg_socks.h */
te_errno
tapi_cfg_socks_add(const char *ta, tapi_socks_id id)
{
    return cfg_add_instance_fmt(NULL, CVT_NONE,
                                NULL,
                                TE_CFG_TA_SOCKS_FMT,
                                TE_CFG_TA_SOCKS_ARGS(ta, id));
}

/* See description in tapi_cfg_socks.h */
te_errno
tapi_cfg_socks_del(const char *ta, tapi_socks_id id)
{
    return cfg_del_instance_fmt(FALSE,
                                TE_CFG_TA_SOCKS_FMT,
                                TE_CFG_TA_SOCKS_ARGS(ta, id));
}

/* See description in tapi_cfg_socks.h */
te_errno
tapi_cfg_socks_enable(const char *ta, tapi_socks_id id)
{
    return tapi_cfg_socks_status_set(ta, (const char *)id, TRUE);
}

/* See description in tapi_cfg_socks.h */
te_errno
tapi_cfg_socks_disable(const char *ta, tapi_socks_id id)
{
    return tapi_cfg_socks_status_set(ta, (const char *)id, FALSE);
}

/* See description in tapi_cfg_socks.h */
te_errno
tapi_cfg_socks_status_get(const char *ta, tapi_socks_id id, te_bool *value)
{
    te_errno        rc;
    int             val;

    rc = tapi_cfg_get_int_fmt(&val,
                              TE_CFG_TA_SOCKS_FMT "/status:",
                              TE_CFG_TA_SOCKS_ARGS(ta, id));
    if (rc == 0)
        *value = (val == 1);
    return rc;
}

/* See description in tapi_cfg_socks.h */
te_errno
tapi_cfg_socks_status_set(const char *ta, tapi_socks_id id,
                          te_bool status)
{
    return tapi_cfg_set_int_fmt(status ? 1 : 0, NULL,
                                TE_CFG_TA_SOCKS_FMT "/status:",
                                TE_CFG_TA_SOCKS_ARGS(ta, id));
}


/* See description in tapi_cfg_socks.h */
te_errno
tapi_cfg_socks_impl_get(const char *ta, tapi_socks_id id,
                        te_socks_impl *value)
{
    te_errno     rc;
    char        *proto;

    rc = cfg_get_instance_string_fmt(&proto, TE_CFG_TA_SOCKS_FMT "/impl:",
                                     TE_CFG_TA_SOCKS_ARGS(ta, id));
    if (rc != 0)
        return rc;

    if (strcmp(proto, "srelay") == 0)
        *value = TE_SOCKS_IMPL_SRELAY;
    else
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    return 0;
}

/* See description in tapi_cfg_socks.h */
te_errno
tapi_cfg_socks_impl_set(const char *ta, tapi_socks_id id,
                        te_socks_impl value)
{
    char *sval;

    if (value == TE_SOCKS_IMPL_SRELAY)
        sval = "srelay";
    else
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    return cfg_set_instance_fmt(CFG_VAL(STRING, sval),
                                TE_CFG_TA_SOCKS_FMT "/impl:",
                                TE_CFG_TA_SOCKS_ARGS(ta, id));
}

/* See description in tapi_cfg_socks.h */
te_errno
tapi_cfg_socks_proto_add(const char         *ta,
                         tapi_socks_id       id,
                         tapi_socks_proto_id proto_id,
                         int                 proto)
{
    return cfg_add_instance_fmt(NULL, CFG_VAL(INTEGER, proto),
                                TE_CFG_TA_SOCKS_FMT "/proto:%s",
                                TE_CFG_TA_SOCKS_ARGS(ta, id),
                                (const char *)proto_id);
}

/* See description in tapi_cfg_socks.h */
te_errno
tapi_cfg_socks_proto_del(const char         *ta,
                         tapi_socks_id       id,
                         tapi_socks_proto_id proto_id)
{
    return cfg_del_instance_fmt(FALSE, TE_CFG_TA_SOCKS_FMT "/proto:%s",
                                TE_CFG_TA_SOCKS_ARGS(ta, id),
                                (const char *)proto_id);
}


/* See description in tapi_cfg_socks.h */
te_errno
tapi_cfg_socks_proto_get(const char            *ta,
                         tapi_socks_id          id,
                         tapi_socks_proto_id    proto_id,
                         int                   *value)
{
    te_errno     rc;
    int          proto;

    rc = tapi_cfg_get_int_fmt(&proto,
                              TE_CFG_TA_SOCKS_FMT "/proto:%s",
                              TE_CFG_TA_SOCKS_ARGS(ta, id),
                              (const char *)proto_id);
    if (rc != 0)
        return rc;

    *value = proto;
    return 0;
}

/* See description in tapi_cfg_socks.h */
te_errno
tapi_cfg_socks_proto_set(const char            *ta,
                         tapi_socks_id          id,
                         tapi_socks_proto_id    proto_id,
                         int                    value)
{
    return tapi_cfg_set_int_fmt(value, NULL,
                                TE_CFG_TA_SOCKS_FMT "/proto:%s",
                                TE_CFG_TA_SOCKS_ARGS(ta, id),
                                (const char *)proto_id);
}

/* See description in tapi_cfg_socks.h */
te_errno
tapi_cfg_socks_interface_add(const char                *ta,
                             tapi_socks_id              id,
                             tapi_socks_interface_id    interface_id,
                             const char                *value)
{
    return cfg_add_instance_fmt(NULL, CFG_VAL(STRING, value),
                                TE_CFG_TA_SOCKS_FMT "/interface:%s",
                                TE_CFG_TA_SOCKS_ARGS(ta, id),
                                (const char *)interface_id);
}

/* See description in tapi_cfg_socks.h */
te_errno
tapi_cfg_socks_interface_del(const char                *ta,
                             tapi_socks_id              id,
                             tapi_socks_interface_id    interface_id)
{
    return cfg_del_instance_fmt(FALSE, TE_CFG_TA_SOCKS_FMT "/interface:%s",
                                TE_CFG_TA_SOCKS_ARGS(ta, id),
                                (const char *)interface_id);
}

/* See description in tapi_cfg_socks.h */
te_errno
tapi_cfg_socks_interface_get(const char                *ta,
                             tapi_socks_id              id,
                             tapi_socks_interface_id    interface_id,
                             char                     **value)
{
    return cfg_get_instance_string_fmt(value,
                                       TE_CFG_TA_SOCKS_FMT "/interface:%s",
                                       TE_CFG_TA_SOCKS_ARGS(ta, id),
                                       (const char *)interface_id);
}

/* See description in tapi_cfg_socks.h */
te_errno
tapi_cfg_socks_interface_set(const char                *ta,
                             tapi_socks_id              id,
                             tapi_socks_interface_id    interface_id,
                             const char                *value)
{
    return cfg_set_instance_fmt(CVT_STRING, value,
                                TE_CFG_TA_SOCKS_FMT "/interface:%s",
                                TE_CFG_TA_SOCKS_ARGS(ta, id),
                                (const char *)interface_id);
}

/* See description in tapi_cfg_socks.h */
te_errno
tapi_cfg_socks_interface_port_get(const char               *ta,
                                  tapi_socks_id             id,
                                  tapi_socks_interface_id   interface_id,
                                  uint16_t                 *value)
{
    te_errno        rc;
    int             port;

    rc = tapi_cfg_get_int_fmt(&port,
                              TE_CFG_TA_SOCKS_FMT "/interface:%s/port:",
                              TE_CFG_TA_SOCKS_ARGS(ta, id),
                              (const char *)interface_id);
    if (rc == 0)
    {
        /*
         * Casting is harmless - port is checked for sanity on set.
         */
        *value = (uint16_t)port;
    }

    return rc;
}

/* See description in tapi_cfg_socks.h */
te_errno
tapi_cfg_socks_interface_port_set(const char               *ta,
                                  tapi_socks_id             id,
                                  tapi_socks_interface_id   interface_id,
                                  uint16_t                  value)
{
    int ival = (int)value;

    return tapi_cfg_set_int_fmt(ival, NULL,
                                TE_CFG_TA_SOCKS_FMT "/interface:%s/port:",
                                TE_CFG_TA_SOCKS_ARGS(ta, id),
                                (const char *)interface_id);
}

/* See description in tapi_cfg_socks.h */
te_errno
tapi_cfg_socks_interface_addr_family_get(const char               *ta,
                                         tapi_socks_id             id,
                                         tapi_socks_interface_id   interface_id,
                                         int                      *value)
{
    te_errno        rc;
    int             af;

    rc = tapi_cfg_get_int_fmt(&af,
                              TE_CFG_TA_SOCKS_FMT "/interface:%s/addr_family:",
                              TE_CFG_TA_SOCKS_ARGS(ta, id),
                              (const char *)interface_id);
    if (rc == 0)
        *value = af;

    return rc;
}

/* See description in tapi_cfg_socks.h */
te_errno
tapi_cfg_socks_interface_addr_family_set(const char               *ta,
                                         tapi_socks_id             id,
                                         tapi_socks_interface_id   interface_id,
                                         int                       value)
{
    return tapi_cfg_set_int_fmt(value, NULL,
        TE_CFG_TA_SOCKS_FMT "/interface:%s/addr_family:",
        TE_CFG_TA_SOCKS_ARGS(ta, id),
        (const char *)interface_id);
}

/* See description in tapi_cfg_socks.h */
te_errno
tapi_cfg_socks_outbound_interface_get(const char *ta, tapi_socks_id id,
                                      char **value)
{
    return cfg_get_instance_string_fmt(value,
                                   TE_CFG_TA_SOCKS_FMT "/outbound_interface:",
                                   TE_CFG_TA_SOCKS_ARGS(ta, id));
}

/* See description in tapi_cfg_socks.h */
te_errno
tapi_cfg_socks_outbound_interface_set(const char *ta, tapi_socks_id id,
                                      const char *value)
{
    return cfg_set_instance_fmt(CVT_STRING, value,
                                TE_CFG_TA_SOCKS_FMT "/outbound_interface:",
                                TE_CFG_TA_SOCKS_ARGS(ta, id));
}

/* See description in tapi_cfg_socks.h */
te_errno
tapi_cfg_socks_cipher_get(const char *ta, tapi_socks_id id, char **value)
{
    return cfg_get_instance_string_fmt(value,
                                       TE_CFG_TA_SOCKS_FMT "/cipher:",
                                       TE_CFG_TA_SOCKS_ARGS(ta, id));
}

/* See description in tapi_cfg_socks.h */
te_errno
tapi_cfg_socks_cipher_set(const char *ta, tapi_socks_id id, const char *value)
{
    return cfg_set_instance_fmt(CVT_STRING, value,
                                TE_CFG_TA_SOCKS_FMT "/cipher:",
                                TE_CFG_TA_SOCKS_ARGS(ta, id));
}

/* See description in tapi_cfg_socks.h */
te_errno
tapi_cfg_socks_auth_get(const char *ta, tapi_socks_id id,
                        char **value)
{
    return cfg_get_instance_string_fmt(value,
                                       TE_CFG_TA_SOCKS_FMT "/auth:",
                                       TE_CFG_TA_SOCKS_ARGS(ta, id));
}

/* See description in tapi_cfg_socks.h */
te_errno
tapi_cfg_socks_auth_set(const char *ta, tapi_socks_id id,
                        const char *value)
{
    return cfg_set_instance_fmt(CFG_VAL(STRING, value),
                                TE_CFG_TA_SOCKS_FMT "/auth:",
                                TE_CFG_TA_SOCKS_ARGS(ta, id));
}

/* See description in tapi_cfg_socks.h */
te_errno
tapi_cfg_socks_user_add(const char         *ta,
                        tapi_socks_id       id,
                        tapi_socks_user_id  user)
{
    return cfg_add_instance_fmt(NULL, CVT_NONE,
                                NULL, TE_CFG_TA_SOCKS_FMT "/user:%s",
                                TE_CFG_TA_SOCKS_ARGS(ta, id),
                                (const char *)user);
}

/* See description in tapi_cfg_socks.h */
te_errno
tapi_cfg_socks_user_del(const char         *ta,
                        tapi_socks_id       id,
                        tapi_socks_user_id  user)
{
    return cfg_del_instance_fmt(FALSE, TE_CFG_TA_SOCKS_FMT "/user:%s",
                                TE_CFG_TA_SOCKS_ARGS(ta, id),
                                (const char *)user);
}

/* See description in tapi_cfg_socks.h */
te_errno
tapi_cfg_socks_user_next_hop_get(const char *ta, tapi_socks_id id,
                                 tapi_socks_user_id user, char **value)
{
    return cfg_get_instance_string_fmt(value,
                                       TE_CFG_TA_SOCKS_FMT "/user:%s/next_hop:",
                                       TE_CFG_TA_SOCKS_ARGS(ta, id),
                                       (const char *)user);
}

/* See description in tapi_cfg_socks.h */
te_errno
tapi_cfg_socks_user_next_hop_set(const char *ta, tapi_socks_id id,
                                 tapi_socks_user_id user, const char *value)
{
    return cfg_set_instance_fmt(CVT_STRING, value,
        TE_CFG_TA_SOCKS_FMT "/user:%s/next_hop:",
        TE_CFG_TA_SOCKS_ARGS(ta, id), (const char *)user);
}

/* See description in tapi_cfg_socks.h */
te_errno
tapi_cfg_socks_user_username_get(const char *ta, tapi_socks_id id,
                                 tapi_socks_user_id user, char **value)
{
    return cfg_get_instance_string_fmt(value,
                                       TE_CFG_TA_SOCKS_FMT "/user:%s/username:",
                                       TE_CFG_TA_SOCKS_ARGS(ta, id),
                                       (const char *)user);
}

/* See description in tapi_cfg_socks.h */
te_errno
tapi_cfg_socks_user_username_set(const char *ta, tapi_socks_id id,
                                 tapi_socks_user_id user, const char *value)
{
    return cfg_set_instance_fmt(CVT_STRING, value,
        TE_CFG_TA_SOCKS_FMT "/user:%s/username:",
        TE_CFG_TA_SOCKS_ARGS(ta, id), (const char *)user);
}

/* See description in tapi_cfg_socks.h */
te_errno
tapi_cfg_socks_user_password_get(const char *ta, tapi_socks_id id,
                                 tapi_socks_user_id user, char **value)
{
    return cfg_get_instance_string_fmt(value,
                                       TE_CFG_TA_SOCKS_FMT "/user:%s/password:",
                                       TE_CFG_TA_SOCKS_ARGS(ta, id),
                                       (const char *)user);
}

/* See description in tapi_cfg_socks.h */
te_errno
tapi_cfg_socks_user_password_set(const char *ta, tapi_socks_id id,
                                 tapi_socks_user_id user, const char *value)
{
    return cfg_set_instance_fmt(CVT_STRING, value,
        TE_CFG_TA_SOCKS_FMT "/user:%s/password:",
        TE_CFG_TA_SOCKS_ARGS(ta, id), (const char *)user);
}
