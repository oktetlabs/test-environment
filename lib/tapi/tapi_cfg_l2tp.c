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
#include "tapi_cfg_base.h"
#include "tapi_cfg_aggr.h"
#include "conf_api.h"

#define TE_CFG_TA_L2TP_SERVER "/agent:%s/l2tp:"
#define TE_CFG_TA_L2TP_SECRET "/agent:%s/l2tp:/lns:%s/auth:%s/client:%s"

/** Max length of the buffer for ip address */
#define L2TP_IP_STRING_LENGTH 15

/** Max length of the instance /ip_range: */
#define L2TP_IP_RANGE_INST 36

/** Max length of the secret instance  */
#define L2TP_SECRET_INST 128


/* All descriptions are in tapi_cfg_l2tp.h */

te_errno
tapi_cfg_l2tp_listen_ip_set(const char *ta, const char *lns,
                            struct sockaddr_in *local)
{
    UNUSED(lns);
    return cfg_set_instance_fmt(CFG_VAL(ADDRESS, local),
                                TE_CFG_TA_L2TP_SERVER "/listen:", ta);
}


te_errno
tapi_cfg_l2tp_listen_ip_get(const char *ta, const char *lns,
                     struct sockaddr_in *local)
{
    UNUSED(lns);
    return cfg_get_instance_fmt((cfg_val_type *) CVT_ADDRESS, local,
                                TE_CFG_TA_L2TP_SERVER "/listen:", ta);
}

te_errno
tapi_cfg_l2tp_tunnel_ip_set(const char *ta, const char *lns,
                            struct sockaddr_in *local)
{

    return cfg_set_instance_fmt(CFG_VAL(ADDRESS, local),
                                TE_CFG_TA_L2TP_SERVER "/lns:%s/local_ip:",
                                ta, lns);
}

te_errno
tapi_cfg_l2tp_tunnel_ip_get(const char *ta, const char *lns,
                            struct sockaddr_in *local)
{
    return cfg_get_instance_fmt((cfg_val_type *) CVT_ADDRESS, local,
                                TE_CFG_TA_L2TP_SERVER "/lns:%s/local_ip:",
                                ta, lns);
}

/**
 * Converts the l2tp_ipv4_range's fields to
 * the string like allowX.X.X.X-Y.Y.Y.Y
 *
 * @param iprange   testing ip
 *
 * @return range
 */
static char *
l2tp_get_range(const l2tp_ipv4_range *iprange)
{
    char    *range = malloc(L2TP_IP_RANGE_INST);
    char     end[L2TP_IP_STRING_LENGTH];
    strcpy(end, inet_ntoa(iprange->end->sin_addr));
    TE_SPRINTF(range, "%s%s-%s",
               iprange->type == L2TP_POLICY_ALLOW ?
               "allow" : "deny",
               inet_ntoa(iprange->start->sin_addr), end);
    return range;
}

te_errno
tapi_cfg_l2tp_lns_range_add(const char *ta, const char *lns,
                            const l2tp_ipv4_range *iprange,
                            enum l2tp_iprange_class kind)
{
    cfg_handle           handle;
    char                *range = l2tp_get_range(iprange);

    return cfg_add_instance_fmt(&handle, CFG_VAL(NONE, NULL),
                                TE_CFG_TA_L2TP_SERVER "/lns:%s/%s_range:%s",
                                ta, lns, kind == L2TP_IP_RANGE_CLASS_IP ?
                                "ip" : "lac", range);
}


static te_errno
tapi_cfg_l2tp_lns_range_del(const char *ta, const char *lns,
                            const l2tp_ipv4_range *iprange,
                            enum l2tp_iprange_class kind)
{
    char        *range = l2tp_get_range(iprange);

    return cfg_del_instance_fmt(FALSE, TE_CFG_TA_L2TP_SERVER
                                "/lns:%s/%s_range:%s", ta,
                                lns, kind == L2TP_IP_RANGE_CLASS_IP ?
                                "ip" : "lac", range);
}

te_errno
tapi_cfg_l2tp_lns_bit_set(const char *ta, const char *lns,
                          enum l2tp_bit bit, bool selector)
{
    return cfg_set_instance_fmt(CFG_VAL(INTEGER, selector),
                                TE_CFG_TA_L2TP_SERVER "/lns:%s/bit:%s",
                                ta, lns, bit == L2TP_BIT_HIDDEN ?
                                "hidden" :
                                bit == L2TP_BIT_LENGTH ?
                                "length" : "flow");
}

te_errno
tapi_cfg_l2tp_lns_bit_get(const char *ta, const char *lns,
                          enum l2tp_bit *bit, bool *selector)
{
    return cfg_get_instance_fmt((cfg_val_type *) CVT_INTEGER, selector,
                                TE_CFG_TA_L2TP_SERVER "/lns:%s/bit:%s",
                                ta, lns, *bit == L2TP_BIT_HIDDEN ?
                                "hidden" :
                                *bit == L2TP_BIT_LENGTH ?
                                "length" : "flow");
}

te_errno
tapi_cfg_l2tp_lns_set_auth(const char *ta, const char *lns,
                           l2tp_auth param, bool value)
{
    return cfg_set_instance_fmt(CFG_VAL(INTEGER, value),
                                TE_CFG_TA_L2TP_SERVER
                                "/lns:%s/auth:%s/%s:", ta, lns,
                                param.protocol == L2TP_AUTH_PROT_CHAP ?
                                "chap" :
                                param.protocol == L2TP_AUTH_PROT_PAP ?
                                "pap" : "autrhentication",
                                param.type == L2TP_AUTH_POLICY_REQUIRE ?
                                "require" : "refuse");
}

te_errno
tapi_cfg_l2tp_lns_get_auth(const char *ta, const char *lns,
                           l2tp_auth param, bool *value)
{
    return cfg_get_instance_fmt((cfg_val_type *) CVT_INTEGER, value,
                                TE_CFG_TA_L2TP_SERVER
                                "/lns:%s/auth:%s/%s:", ta, lns,
                                param.protocol == L2TP_AUTH_PROT_CHAP ?
                                "chap" :
                                param.protocol == L2TP_AUTH_PROT_PAP ?
                                "pap" : "authentication",
                                param.type == L2TP_AUTH_POLICY_REQUIRE ?
                                "require" : "refuse");
}

te_errno
tapi_cfg_l2tp_lns_secret_add(const char *ta, const char *lns,
                             const l2tp_ppp_secret *new_secret)
{

    cfg_handle  handle;
    int         ret_val1;
    int         ret_val2;
    int         ret_val3;
    char       *prot = new_secret->is_chap == L2TP_AUTH_PROT_CHAP ?
                       "chap" :
                       new_secret->is_chap == L2TP_AUTH_PROT_PAP ?
                       "pap" : "authentication";

    ret_val1 = cfg_add_instance_fmt(&handle, CFG_VAL(NONE, NULL),
                                    TE_CFG_TA_L2TP_SERVER
                                            "/lns:%s/auth:%s/client:%s/server:%s",
                                    ta, lns, prot,
                                    new_secret->client);
    if (ret_val1 != 0)
        return ret_val1;

    ret_val2 = cfg_add_instance_fmt(&handle, CFG_VAL(NONE, NULL),
                                    TE_CFG_TA_L2TP_SERVER
                                            "/lns:%s/auth:%s/client:%/secret:%s",
                                    ta, lns, prot,
                                    new_secret->secret);
    if (ret_val2 != 0)
        return ret_val2;

    ret_val3 = cfg_add_instance_fmt(&handle, CFG_VAL(NONE, NULL),
                                    TE_CFG_TA_L2TP_SERVER
                                            "/lns:%s/auth:%s/client:%/ipv4:%s",
                                    ta, lns, prot,
                                    inet_ntoa(new_secret->sipv4.sin_addr),
                                    L2TP_SECRET_PART_IPV4);
    if (ret_val3 != 0)
        return ret_val3;

    return 0;
}

te_errno
tapi_cfg_l2tp_lns_secret_delete(const char *ta, const char *lns,
                                const l2tp_ppp_secret *prev_secret)
{

    int         ret_val1;
    int         ret_val2;
    int         ret_val3;
    char       *prot = prev_secret->is_chap == L2TP_AUTH_PROT_CHAP ?
                       "chap" :
                       prev_secret->is_chap == L2TP_AUTH_PROT_PAP ?
                       "pap" : "authentication";

    ret_val1 = cfg_del_instance_fmt(FALSE, TE_CFG_TA_L2TP_SERVER
                                    "/lns:%s/auth:%s/client:%s/server:%s",
                                    ta, lns, prot,
                                    prev_secret->client);
    if (ret_val1 != 0)
        return ret_val1;

    ret_val2 = cfg_del_instance_fmt(FALSE, TE_CFG_TA_L2TP_SERVER
                                    "/lns:%s/auth:%s/client:%s/server:%s",
                                    ta, lns, prot,
                                    prev_secret->secret);
    if (ret_val2 != 0)
        return ret_val2;

    ret_val3 = cfg_del_instance_fmt(FALSE, TE_CFG_TA_L2TP_SERVER
                                    "/lns:%s/auth:%s/client:%s/server:%s",
                                    ta, lns, prot,
                                    inet_ntoa(prev_secret->sipv4.sin_addr),
                                    L2TP_SECRET_PART_IPV4);
    if (ret_val3 != 0)
        return ret_val3;

    return 0;
}

te_errno
tapi_cfg_l2tp_lns_set_use_challenge(const char *ta, const char *lns,
                                    bool value)
{
    return cfg_set_instance_fmt(CFG_VAL(INTEGER, value),
                                TE_CFG_TA_L2TP_SERVER "/lns:%s/use_challenge:",
                                ta, lns);
}

te_errno
tapi_cfg_l2tp_lns_get_use_challenge(const char *ta, const char *lns,
                                    bool value)
{
    return cfg_get_instance_fmt((cfg_val_type *) CVT_INTEGER, &value,
                                TE_CFG_TA_L2TP_SERVER "/lns:%s/use_challenge:",
                                ta, lns);
}

te_errno
tapi_cfg_l2tp_lns_set_unix_auth(const char *ta, const char *lns,
                                bool value)
{
    return cfg_set_instance_fmt(CFG_VAL(INTEGER, value),
                                TE_CFG_TA_L2TP_SERVER "/lns:%s/unix_auth:",
                                ta, lns);
}

te_errno
tapi_cfg_l2tp_lns_get_unix_auth(const char *ta, const char *lns,
                                bool value)
{
    return cfg_get_instance_fmt((cfg_val_type *) CVT_INTEGER, &value,
                                TE_CFG_TA_L2TP_SERVER "/lns:%s/unix_auth:",
                                ta, lns);
}

te_errno
tapi_cfg_l2tp_lns_mtu_set(const char *ta, const char *lns, int value)
{
    return cfg_set_instance_fmt(CFG_VAL(INTEGER, value),
                                TE_CFG_TA_L2TP_SERVER "/lns:%s/pppopt:%s/mtu:",
                                ta, lns);
}

te_errno
tapi_cfg_l2tp_lns_mtu_get(const char *ta, const char *lns, int *value)
{
    return cfg_get_instance_fmt((cfg_val_type *) CVT_INTEGER, value,
                                TE_CFG_TA_L2TP_SERVER "/lns:%s/pppopt:%s/mtu:",
                                ta, lns);
}

te_errno
tapi_cfg_l2tp_lns_mru_set(const char *ta, const char *lns, int value)
{
    return cfg_set_instance_fmt(CFG_VAL(INTEGER, value),
                                TE_CFG_TA_L2TP_SERVER "/lns:%s/pppopt:%s/mru:",
                                ta, lns);
}

te_errno
tapi_cfg_l2tp_lns_mru_get(const char *ta, const char *lns, int *value)
{
    return cfg_get_instance_fmt((cfg_val_type *) CVT_INTEGER, value,
                                TE_CFG_TA_L2TP_SERVER "/lns:%s/pppopt:%s/mru:",
                                ta, lns);
}

te_errno
tapi_cfg_l2tp_lns_lcp_echo_failure_set(const char *ta, const char *lns,
                                       int value) {
    return cfg_set_instance_fmt(CFG_VAL(INTEGER, value),
                                TE_CFG_TA_L2TP_SERVER
                                        "/lns:%s/pppopt:%s/lcp-echo-failure:",
                                ta, lns);
}

te_errno
tapi_cfg_l2tp_lns_lcp_echo_failure_get(const char *ta, const char *lns,
                                       int *value)
{
        return cfg_get_instance_fmt((cfg_val_type *) CVT_INTEGER, value,
                                TE_CFG_TA_L2TP_SERVER
                                "/lns:%s/pppopt:%s/lcp-echo-failure:",
                                ta, lns);
}

te_errno
tapi_cfg_l2tp_lns_lcp_echo_interval_set(const char *ta, const char *lns,
                                        int value)
{
    return cfg_set_instance_fmt(CFG_VAL(INTEGER, value),
                                TE_CFG_TA_L2TP_SERVER
                                "/lns:%s/pppopt:%s/lcp-echo-interval:",
                                ta, lns);
}

te_errno
tapi_cfg_l2tp_lns_lcp_echo_interval_get(const char *ta, const char *lns,
                                        int *value)
{
        return cfg_get_instance_fmt((cfg_val_type *) CVT_INTEGER, value,
                                TE_CFG_TA_L2TP_SERVER
                                "/lns:%s/pppopt:%s/lcp-echo-interval:",
                                ta, lns);
}

te_errno
tapi_cfg_l2tp_lns_pppopt_add(const char *ta, const char *lns,
                             const char *pparam)
{
    cfg_handle  handle;

    return cfg_add_instance_fmt(&handle, CFG_VAL(NONE, NULL),
                                TE_CFG_TA_L2TP_SERVER "/pppopt/option:%s",
                                ta, lns, pparam);
}

te_errno
tapi_cfg_l2tp_lns_pppopt_del(const char *ta, const char *lns,
                             const char *pparam)
{
    return cfg_del_instance_fmt(FALSE, TE_CFG_TA_L2TP_SERVER
                                "/pppopt/option:%s",
                                ta, lns, pparam);
}


