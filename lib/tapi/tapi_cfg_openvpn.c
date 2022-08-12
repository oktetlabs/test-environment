/** @file
 * @brief Test API to control OpenVPN configurator tree.
 *
 * Implementation of TAPI to configure OpenVPN.
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 */

#include "te_config.h"
#if HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

#include "tapi_cfg_openvpn.h"

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

/* See description in tapi_cfg_openvpn.h */
te_errno
tapi_cfg_openvpn_add(const char *ta, tapi_openvpn_id id)
{
    return cfg_add_instance_fmt(NULL, CVT_NONE,
                                NULL,
                                "/agent:%s/openvpn:%s",
                                ta, (const char *)id);
}

/* See description in tapi_cfg_openvpn.h */
te_errno
tapi_cfg_openvpn_del(const char *ta, tapi_openvpn_id id)
{
    return cfg_del_instance_fmt(FALSE, "/agent:%s/openvpn:%s",
                                ta, (const char *)id);
}

/* See description in tapi_cfg_openvpn.h */
te_errno
tapi_cfg_openvpn_prop_set(const char       *ta,
                          tapi_openvpn_id   id,
                          const char       *prop,
                          cfg_val_type      type,
                          const void       *val)
{
    return cfg_set_instance_fmt(type, val,
                                "/agent:%s/openvpn:%s/%s",
                                 ta, (const char *)id, prop);
}

/* See description in tapi_cfg_openvpn.h */
te_errno
tapi_cfg_openvpn_prop_get(const char       *ta,
                          tapi_openvpn_id   id,
                          const char       *prop,
                          cfg_val_type      type,
                          void            **val)
{
    cfg_val_type real_type = type;

    return cfg_get_instance_fmt(&real_type, val,
                                "/agent:%s/openvpn:%s/%s",
                                ta, (const char *)id, prop);
}

/* See description in tapi_cfg_openvpn.h */
te_errno
tapi_cfg_openvpn_enable(const char *ta, tapi_openvpn_id id)
{
    return tapi_cfg_openvpn_status_set(ta, (const char *)id, 1);
}

/* See description in tapi_cfg_openvpn.h */
te_errno
tapi_cfg_openvpn_disable(const char *ta, tapi_openvpn_id id)
{
    return tapi_cfg_openvpn_status_set(ta, (const char *)id, 0);
}

/* See description in tapi_cfg_openvpn.h */
te_errno
tapi_cfg_openvpn_endpoint_ip_get(const char *ta, tapi_openvpn_id id,
                                 struct sockaddr **addr)
{
    return cfg_get_instance_addr_fmt(addr, "/agent:%s/openvpn:%s/endpoint_ip:",
                                     ta, (const char *)id);
}


/* See description in tapi_cfg_openvpn.h */
te_errno
tapi_cfg_openvpn_peer_add(const char *ta,
                          tapi_openvpn_id id,
                          tapi_openvpn_prop peer)
{
    return cfg_add_instance_fmt(NULL, CVT_NONE,
                                NULL, "/agent:%s/openvpn:%s/client:/peer:%s",
                                ta, (const char *)id, (const char *)peer);
}

/* See description in tapi_cfg_openvpn.h */
te_errno
tapi_cfg_openvpn_peer_del(const char *ta,
                          tapi_openvpn_id id,
                          tapi_openvpn_prop peer)
{
    return cfg_del_instance_fmt(FALSE, "/agent:%s/openvpn:%s/client:/peer:%s",
                                ta, (const char *)id, (const char *)peer);
}

/* See description in tapi_cfg_openvpn.h */
te_errno
tapi_cfg_openvpn_peer_port_get(const char *ta, tapi_openvpn_id id,
                               tapi_openvpn_prop peer, uint16_t *val)
{
    te_errno  rc;
    int       port;

    rc = cfg_get_instance_int_fmt(&port,
                                  "/agent:%s/openvpn:%s/client:/peer:%s/port:",
                                  ta, (const char *)id, (const char *)peer);
    if (rc == 0)
    {
        /*
         * The type is supported by implementation, i.e. there can be no
         * value not directly casting to uint16_t.
         */
        *val = (uint16_t)port;
    }

    return rc;
}

/* See description in tapi_cfg_openvpn.h */
te_errno
tapi_cfg_openvpn_peer_port_set(const char *ta, tapi_openvpn_id id,
                               tapi_openvpn_prop peer, uint16_t val)
{
    int ival = (int)val;

    return cfg_set_instance_fmt(CFG_VAL(INTEGER, ival),
                                "/agent:%s/openvpn:%s/client:/peer:%s/port:",
                                ta, (const char *)id, (const char *)peer);
}

/* See description in tapi_cfg_openvpn.h */
te_errno
tapi_cfg_openvpn_user_add(const char *ta,
                          tapi_openvpn_id id,
                          tapi_openvpn_prop user)
{
    return cfg_add_instance_fmt(NULL, CVT_NONE,
                                NULL, "/agent:%s/openvpn:%s/server:/user:%s",
                                ta, (const char *)id, (const char *)user);
}

/* See description in tapi_cfg_openvpn.h */
te_errno
tapi_cfg_openvpn_user_del(const char *ta,
                          tapi_openvpn_id id,
                          tapi_openvpn_prop user)
{
    return cfg_del_instance_fmt(FALSE, "/agent:%s/openvpn:%s/server:/user:%s",
                                ta, (const char *)id, (const char *)user);
}

/* See description in tapi_cfg_openvpn.h */
te_errno
tapi_cfg_openvpn_user_username_get(const char *ta, tapi_openvpn_id id,
                                   tapi_openvpn_prop user, char **val)
{
    return cfg_get_instance_string_fmt(val,
        "/agent:%s/openvpn:%s/server:/user:%s/username:",
        ta, (const char *)id, (const char *)user);
}

/* See description in tapi_cfg_openvpn.h */
te_errno
tapi_cfg_openvpn_user_username_set(const char *ta, tapi_openvpn_id id,
                                   tapi_openvpn_prop user, const char *val)
{
    return cfg_set_instance_fmt(CVT_STRING, val,
        "/agent:%s/openvpn:%s/server:/user:%s/username:",
        ta, (const char *)id, (const char *)user);
}

/* See description in tapi_cfg_openvpn.h */
te_errno
tapi_cfg_openvpn_user_password_get(const char *ta, tapi_openvpn_id id,
                                   tapi_openvpn_prop user, char **val)
{
    return cfg_get_instance_string_fmt(val,
        "/agent:%s/openvpn:%s/server:/user:%s/password:",
        ta, (const char *)id, (const char *)user);
}

/* See description in tapi_cfg_openvpn.h */
te_errno
tapi_cfg_openvpn_user_password_set(const char *ta, tapi_openvpn_id id,
                                   tapi_openvpn_prop user, const char *val)
{
    return cfg_set_instance_fmt(CVT_STRING, val,
        "/agent:%s/openvpn:%s/server:/user:%s/password:",
        ta, (const char *)id, (const char *)user);
}

/* See description in tapi_cfg_openvpn.h */
te_errno
tapi_cfg_openvpn_user_certificate_get(const char *ta, tapi_openvpn_id id,
                                      tapi_openvpn_prop user, char **val)
{
    return cfg_get_instance_string_fmt(val,
        "/agent:%s/openvpn:%s/server:/user:%s/certificate:",
        ta, (const char *)id, (const char *)user);
}

/* See description in tapi_cfg_openvpn.h */
te_errno
tapi_cfg_openvpn_user_certificate_set(const char *ta, tapi_openvpn_id id,
                                      tapi_openvpn_prop user, const char *val)
{
    return cfg_set_instance_fmt(CVT_STRING, val,
        "/agent:%s/openvpn:%s/server:/user:%s/certificate:",
        ta, (const char *)id, (const char *)user);
}

/* See description in tapi_cfg_openvpn.h */
te_errno
tapi_cfg_openvpn_option_add(const char *ta, tapi_openvpn_id id,
                            tapi_openvpn_prop option, const char *value)
{
    te_errno    rc;

    rc = cfg_add_instance_fmt(NULL, CVT_NONE,
                              NULL,"/agent:%s/openvpn:%s/option:%s",
                              ta, (const char *)id, (const char *)option);
    if (rc != 0)
        return rc;

    return cfg_set_instance_fmt(CVT_STRING, value,
                                "/agent:%s/openvpn:%s/option:%s/value:",
                                ta, (const char *)id, (const char *)option);
}

/* See description in tapi_cfg_openvpn.h */
te_errno
tapi_cfg_openvpn_option_del(const char *ta,
                            tapi_openvpn_id id,
                            tapi_openvpn_prop option)
{
    return cfg_del_instance_fmt(FALSE, "/agent:%s/openvpn:%s/option:%s",
                                ta, (const char *)id, (const char *)option);
}
