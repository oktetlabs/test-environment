/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2015-2022 OKTET Labs Ltd. All rights reserved. */

#include "te_config.h"
#if HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

#include "tapi_cfg_l2tp.h"

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
#include "te_str.h"
#include "tapi_mem.h"


#define TE_CFG_TA_L2TP_SERVER "/agent:%s/l2tp:"


/* See descriptions in tapi_cfg_l2tp.h */
te_errno
tapi_cfg_l2tp_server_set(const char *ta, int status)
{
    return cfg_set_instance_fmt(CFG_VAL(INTEGER, status),
                                TE_CFG_TA_L2TP_SERVER, ta);
}

/* See descriptions in tapi_cfg_l2tp.h */
te_errno
tapi_cfg_l2tp_server_get(const char *ta, int *status)
{
    return tapi_cfg_get_int_fmt(status, TE_CFG_TA_L2TP_SERVER, ta);
}

/* See descriptions in tapi_cfg_l2tp.h */
te_errno
tapi_cfg_l2tp_lns_add(const char *ta, const char *lns)
{
    cfg_handle           handle;
    return cfg_add_instance_fmt(&handle, CFG_VAL(NONE, NULL),
                                TE_CFG_TA_L2TP_SERVER "/lns:%s",
                                ta, lns);
}

/* See descriptions in tapi_cfg_l2tp.h */
te_errno
tapi_cfg_l2tp_lns_del(const char *ta, const char *lns)
{
    return cfg_del_instance_fmt(FALSE, TE_CFG_TA_L2TP_SERVER
                                "/lns:%s", ta, lns);
}

/* See descriptions in tapi_cfg_l2tp.h */
te_errno
tapi_cfg_l2tp_listen_ip_set(const char *ta, const struct sockaddr *addr)
{
    return cfg_set_instance_fmt(CFG_VAL(ADDRESS, addr),
                                TE_CFG_TA_L2TP_SERVER "/listen:", ta);
}

/* See descriptions in tapi_cfg_l2tp.h */
te_errno
tapi_cfg_l2tp_listen_ip_get(const char *ta, struct sockaddr **addr)
{
    return cfg_get_instance_addr_fmt(addr,
                                     TE_CFG_TA_L2TP_SERVER "/listen:", ta);
}

/* See descriptions in tapi_cfg_l2tp.h */
te_errno
tapi_cfg_l2tp_port_set(const char *ta, int port)
{
    return cfg_set_instance_fmt(CFG_VAL(INTEGER, port),
                                TE_CFG_TA_L2TP_SERVER "/port:", ta);
}

/* See descriptions in tapi_cfg_l2tp.h */
te_errno
tapi_cfg_l2tp_port_get(const char *ta, int *port)
{
    return tapi_cfg_get_int_fmt(port, TE_CFG_TA_L2TP_SERVER "/port:", ta);
}

/* See descriptions in tapi_cfg_l2tp.h */
te_errno
tapi_cfg_l2tp_tunnel_ip_set(const char *ta, const char *lns,
                            const struct sockaddr *addr)
{

    return cfg_set_instance_fmt(CFG_VAL(ADDRESS, addr),
                                TE_CFG_TA_L2TP_SERVER "/lns:%s/local_ip:",
                                ta, lns);
}

/* See descriptions in tapi_cfg_l2tp.h */
te_errno
tapi_cfg_l2tp_tunnel_ip_get(const char *ta, const char *lns,
                            struct sockaddr **addr)
{
    return cfg_get_instance_addr_fmt(addr,
                                     TE_CFG_TA_L2TP_SERVER "/lns:%s/local_ip:",
                                     ta, lns);
}


/*
 * Get string representation of IP addresses range in format "ip_addr-ip_addr"
 *
 * @note Return value should be freed with free(3) when it is no longer needed.
 *
 * @param iprange       IP addresses range.
 *
 * @return String representation of addresses range.
 */
static char *
l2tp_get_range(const l2tp_ipaddr_range *iprange)
{
    char range[2 * INET6_ADDRSTRLEN + 1 + 1]; /* string format: "ip-ip" */

    TE_SPRINTF(range, "%s-%s", te_sockaddr_get_ipstr(iprange->start),
               te_sockaddr_get_ipstr(iprange->end));

    return tapi_strdup(range);
}

/* See descriptions in tapi_cfg_l2tp.h */
te_errno
tapi_cfg_l2tp_lns_range_add(const char *ta, const char *lns,
                            const l2tp_ipaddr_range *iprange,
                            enum l2tp_iprange_class kind)
{
    cfg_handle  handle;
    char       *range = l2tp_get_range(iprange);
    te_errno    rc;

    rc = cfg_add_instance_fmt(&handle, CFG_VAL(STRING,
                              iprange->type == L2TP_POLICY_ALLOW ?
                              "allow" : "deny"),
                              TE_CFG_TA_L2TP_SERVER "/lns:%s/%s_range:%s",
                              ta, lns, kind == L2TP_IP_RANGE_CLASS_IP ?
                              "ip" : "lac", range);

    free(range);
    return rc;
}

/* See descriptions in tapi_cfg_l2tp.h */
te_errno
tapi_cfg_l2tp_lns_range_del(const char *ta, const char *lns,
                            const l2tp_ipaddr_range *iprange,
                            enum l2tp_iprange_class kind)
{
    char     *range = l2tp_get_range(iprange);
    te_errno  rc;

    rc = cfg_del_instance_fmt(FALSE, TE_CFG_TA_L2TP_SERVER
                              "/lns:%s/%s_range:%s", ta,
                              lns, kind == L2TP_IP_RANGE_CLASS_IP ?
                              "ip" : "lac", range);

    free(range);
    return rc;
}

/* See descriptions in tapi_cfg_l2tp.h */
te_errno
tapi_cfg_l2tp_lns_connected_get(const char *ta, const char *lns,
                                struct sockaddr ***connected)
{
    unsigned int         ins_num;
    unsigned int         i;
    cfg_handle          *handle;
    cfg_val_type         type = CVT_ADDRESS;
    struct sockaddr    **connected_mem;
    int                  ret_val;

    ret_val = cfg_find_pattern_fmt(&ins_num, &handle,
                         TE_CFG_TA_L2TP_SERVER "/lns:%s/connected:*",
                         ta, lns);
    if (ret_val != 0)
        return ret_val;
    if (ins_num == 0)
        return 0;

    for (i = 0; i < ins_num; i++)
    {
        struct sockaddr *ip_client;

        ret_val = cfg_get_instance(handle[i], &type, &ip_client);
        if (ret_val != 0)
            return ret_val;
        connected_mem = (struct sockaddr **)realloc(*connected,
                         (i + 1) * sizeof(struct sockaddr *));
        if (connected_mem != NULL)
        {
            connected_mem[i] = ip_client;
            *connected = connected_mem;
        }

    }
    return 0;
}

/* See descriptions in tapi_cfg_l2tp.h */
te_errno
tapi_cfg_l2tp_lns_bit_add(const char *ta, const char *lns,
                          enum l2tp_bit bit, te_bool value)
{
    cfg_handle handle;

    return cfg_add_instance_fmt(&handle, CFG_VAL(INTEGER, value),
                                TE_CFG_TA_L2TP_SERVER "/lns:%s/bit:%s",
                                ta, lns, bit == L2TP_BIT_HIDDEN ?
                                "hidden" : "length");
}

/* See descriptions in tapi_cfg_l2tp.h */
te_errno
tapi_cfg_l2tp_lns_bit_del(const char *ta, const char *lns,
                          enum l2tp_bit *bit)
{
    return cfg_del_instance_fmt(FALSE, TE_CFG_TA_L2TP_SERVER "/lns:%s/bit:%s",
                                ta, lns, *bit == L2TP_BIT_HIDDEN ?
                                "hidden" :
                                *bit == L2TP_BIT_LENGTH ?
                                "length" : "flow");
}

/* See descriptions in tapi_cfg_l2tp.h */
te_errno
tapi_cfg_l2tp_lns_bit_get(const char *ta, const char *lns,
                          enum l2tp_bit *bit, char *selector)
{
    UNUSED(ta);
    UNUSED(lns);
    UNUSED(bit);
    UNUSED(selector);

    return TE_RC(TE_TAPI, TE_ENOSYS);
}

/* See descriptions in tapi_cfg_l2tp.h */
te_errno
tapi_cfg_l2tp_lns_add_auth(const char *ta, const char *lns,
                           l2tp_auth param, te_bool value)
{
    cfg_handle  handle;
    int         ret_val1;
    int         ret_val2;
    char        *protocol = param.protocol == L2TP_AUTH_PROT_CHAP ?
                    "chap" :
                    param.protocol == L2TP_AUTH_PROT_PAP ?
                    "pap" : "authentication";

    ret_val1 = cfg_add_instance_fmt(&handle, CFG_VAL(NONE, NULL),
                                    TE_CFG_TA_L2TP_SERVER
                                    "/lns:%s/auth:%s", ta, lns, protocol);
    if (ret_val1 != 0)
        return ret_val1;

    ret_val2 = cfg_set_instance_fmt(CFG_VAL(INTEGER, value),
                                TE_CFG_TA_L2TP_SERVER "/lns:%s/auth:%s/%s:", ta, lns,
                                protocol, param.type == L2TP_AUTH_POLICY_REQUIRE ?
                                "require" : "refuse");
    if (ret_val2 != 0)
        return ret_val2;

    return 0;
}

/* See descriptions in tapi_cfg_l2tp.h */
te_errno
tapi_cfg_l2tp_lns_del_auth(const char *ta, const char *lns,
                           l2tp_auth param)
{
    return cfg_del_instance_fmt(FALSE, TE_CFG_TA_L2TP_SERVER
                                "/lns:%s/auth:%s", ta, lns,
                                param.protocol == L2TP_AUTH_PROT_CHAP ?
                                "chap" :
                                param.protocol == L2TP_AUTH_PROT_PAP ?
                                "pap" : "authentication");

}

/* See descriptions in tapi_cfg_l2tp.h */
te_errno
tapi_cfg_l2tp_lns_secret_add(const char *ta, const char *lns,
                             const l2tp_ppp_secret *secret)
{

    cfg_handle  handle;
    int         ret_val1;
    int         ret_val2;
    int         ret_val3;
    int         ret_val4;

    char       *prot = secret->is_chap == L2TP_AUTH_PROT_CHAP ?
                       "chap" :
                       secret->is_chap == L2TP_AUTH_PROT_PAP ?
                       "pap" : "authentication";

    ret_val1 = cfg_add_instance_fmt(&handle, CFG_VAL(NONE, NULL),
                                    TE_CFG_TA_L2TP_SERVER
                                            "/lns:%s/auth:%s/client:%s",
                                    ta, lns, prot,
                                    secret->client);
    if (ret_val1 != 0)
        return ret_val1;

    ret_val2 = cfg_set_instance_fmt(CFG_VAL(STRING, secret->secret),
                                    TE_CFG_TA_L2TP_SERVER
                                            "/lns:%s/auth:%s/client:%s/secret:",
                                    ta, lns, prot,
                                    secret->client);
    if (ret_val2 != 0)
        return ret_val2;

    ret_val3 = cfg_set_instance_fmt(CFG_VAL(STRING, secret->server),
                                    TE_CFG_TA_L2TP_SERVER
                                            "/lns:%s/auth:%s/client:%s/server:",
                                    ta, lns, prot, secret->client);
    if (ret_val3 != 0)
        return ret_val3;

    ret_val4 = cfg_set_instance_fmt(CFG_VAL(STRING, secret->sipv4),
                                    TE_CFG_TA_L2TP_SERVER
                                            "/lns:%s/auth:%s/client:%s/ipv4:",
                                    ta, lns, prot, secret->client);
    if (ret_val4 != 0)
        return ret_val4;

    return 0;
}

/* See descriptions in tapi_cfg_l2tp.h */
te_errno
tapi_cfg_l2tp_lns_secret_delete(const char *ta, const char *lns,
                                const l2tp_ppp_secret *secret)
{

    char    *prot = secret->is_chap == L2TP_AUTH_PROT_CHAP ?
                       "chap" :
                       secret->is_chap == L2TP_AUTH_PROT_PAP ?
                       "pap" : "authentication";

    return cfg_del_instance_fmt(FALSE, TE_CFG_TA_L2TP_SERVER
                                "/lns:%s/auth:%s/client:%s",
                                ta, lns, prot,
                                secret->client);
}

/* See descriptions in tapi_cfg_l2tp.h */
te_errno
tapi_cfg_l2tp_lns_set_use_challenge(const char *ta, const char *lns,
                                    te_bool value)
{
    return cfg_set_instance_fmt(CFG_VAL(INTEGER, value),
                                TE_CFG_TA_L2TP_SERVER "/lns:%s/use_challenge:",
                                ta, lns);
}

/* See descriptions in tapi_cfg_l2tp.h */
te_errno
tapi_cfg_l2tp_lns_get_use_challenge(const char *ta, const char *lns,
                                    te_bool *value)
{
    int      val;
    te_errno rc;

    rc = tapi_cfg_get_int_fmt(&val,
                              TE_CFG_TA_L2TP_SERVER "/lns:%s/use_challenge:",
                              ta, lns);

    *value = (val != 0);
    return rc;
}

/* See descriptions in tapi_cfg_l2tp.h */
te_errno
tapi_cfg_l2tp_lns_set_unix_auth(const char *ta, const char *lns,
                                te_bool value)
{
    return cfg_set_instance_fmt(CFG_VAL(INTEGER, value),
                                TE_CFG_TA_L2TP_SERVER "/lns:%s/unix_auth:",
                                ta, lns);
}

/* See descriptions in tapi_cfg_l2tp.h */
te_errno
tapi_cfg_l2tp_lns_get_unix_auth(const char *ta, const char *lns,
                                te_bool *value)
{
    int      val;
    te_errno rc;

    rc = tapi_cfg_get_int_fmt(&val,
                              TE_CFG_TA_L2TP_SERVER "/lns:%s/unix_auth:",
                              ta, lns);

    *value = (val != 0);
    return rc;
}

/* See descriptions in tapi_cfg_l2tp.h */
te_errno
tapi_cfg_l2tp_lns_mtu_set(const char *ta, const char *lns, int value)
{
    return cfg_set_instance_fmt(CFG_VAL(INTEGER, value),
                                TE_CFG_TA_L2TP_SERVER "/lns:%s/pppopt:/mtu:",
                                ta, lns);
}

/* See descriptions in tapi_cfg_l2tp.h */
te_errno
tapi_cfg_l2tp_lns_mtu_get(const char *ta, const char *lns, int *value)
{
    return tapi_cfg_get_int_fmt(value,
                                TE_CFG_TA_L2TP_SERVER "/lns:%s/pppopt:/mtu:",
                                ta, lns);
}

/* See descriptions in tapi_cfg_l2tp.h */
te_errno
tapi_cfg_l2tp_lns_mru_set(const char *ta, const char *lns, int value)
{
    return cfg_set_instance_fmt(CFG_VAL(INTEGER, value),
                                TE_CFG_TA_L2TP_SERVER "/lns:%s/pppopt:/mru:",
                                ta, lns);
}

/* See descriptions in tapi_cfg_l2tp.h */
te_errno
tapi_cfg_l2tp_lns_mru_get(const char *ta, const char *lns, int *value)
{
    return tapi_cfg_get_int_fmt(value,
                                TE_CFG_TA_L2TP_SERVER "/lns:%s/pppopt:/mru:",
                                ta, lns);
}

/* See descriptions in tapi_cfg_l2tp.h */
te_errno
tapi_cfg_l2tp_lns_lcp_echo_failure_set(const char *ta, const char *lns,
                                       int value) {
    return cfg_set_instance_fmt(CFG_VAL(INTEGER, value),
                                TE_CFG_TA_L2TP_SERVER
                                "/lns:%s/pppopt:/lcp-echo-failure:",
                                ta, lns);
}

/* See descriptions in tapi_cfg_l2tp.h */
te_errno
tapi_cfg_l2tp_lns_lcp_echo_failure_get(const char *ta, const char *lns,
                                       int *value)
{
    return tapi_cfg_get_int_fmt(value,
                                TE_CFG_TA_L2TP_SERVER
                                "/lns:%s/pppopt:/lcp-echo-failure:",
                                ta, lns);
}

/* See descriptions in tapi_cfg_l2tp.h */
te_errno
tapi_cfg_l2tp_lns_lcp_echo_interval_set(const char *ta, const char *lns,
                                        int value)
{
    return cfg_set_instance_fmt(CFG_VAL(INTEGER, value),
                                TE_CFG_TA_L2TP_SERVER
                                "/lns:%s/pppopt:/lcp-echo-interval:",
                                ta, lns);
}

/* See descriptions in tapi_cfg_l2tp.h */
te_errno
tapi_cfg_l2tp_lns_lcp_echo_interval_get(const char *ta, const char *lns,
                                        int *value)
{
    return tapi_cfg_get_int_fmt(value,
                                TE_CFG_TA_L2TP_SERVER
                                "/lns:%s/pppopt:/lcp-echo-interval:",
                                ta, lns);
}

/* See descriptions in tapi_cfg_l2tp.h */
te_errno
tapi_cfg_l2tp_lns_pppopt_add(const char *ta, const char *lns,
                             const char *opt)
{
    cfg_handle  handle;

    return cfg_add_instance_fmt(&handle, CFG_VAL(NONE, NULL),
                                TE_CFG_TA_L2TP_SERVER "/lns:%s/pppopt:/option:%s",
                                ta, lns, opt);
}

/* See descriptions in tapi_cfg_l2tp.h */
te_errno
tapi_cfg_l2tp_lns_pppopt_del(const char *ta, const char *lns,
                             const char *opt)
{
    return cfg_del_instance_fmt(FALSE, TE_CFG_TA_L2TP_SERVER
                                "/lns:%s/pppopt:/option:%s",
                                ta, lns, opt);
}
