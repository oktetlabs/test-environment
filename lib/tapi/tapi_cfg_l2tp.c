#include <arpa/inet.h>
#include <string.h>
#include "tapi_cfg_l2tp.h"
#include "te_config.h"

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

/** Max length of the instance /ip_range: */
#define L2TP_IP_RANGE_INST 35

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
tapi_cfg_l2tp_ip_get(const char *ta, const char *lns,
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

char *
l2tp_add_range(const l2tp_ipv4_range *iprange)
{
    char    *range = malloc(sizeof(char) * 32);
    char    *end = strdup(inet_ntoa(iprange->end->sin_addr));
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
    cfg_handle  handle;

    return cfg_add_instance_fmt(&handle, CFG_VAL(NONE, NULL), 
                                TE_CFG_TA_L2TP_SERVER "/lns:%s/%s_range:%s", 
                                ta, lns, kind == L2TP_IP_RANGE_CLASS_IP ? 
                                "ip" : "lac", l2tp_add_range(iprange));
}


te_errno
tapi_cfg_l2tp_lns_range_del(const char *ta, const char *lns,
                            const l2tp_ipv4_range *iprange,
                            enum l2tp_iprange_class kind)
{

    return cfg_del_instance_fmt(FALSE, TE_CFG_TA_L2TP_SERVER
                                "/lns:%s/%s_range:%s", ta, 
                                lns, kind == L2TP_IP_RANGE_CLASS_IP ? 
                                "ip" : "lac", l2tp_add_range(iprange));
}

te_errno
tapi_cfg_l2tp_lns_connected_get(const char *ta, const char *lns,
                                struct sockaddr_in ***connected)
{
    UNUSED(ta);
    UNUSED(lns);
    if (*connected != NULL)
    {
        return -1;
    }
    struct ifaddrs         *tmp;
    struct sockaddr_in    **connected_mem;
    struct sockaddr_in     *sin;
    int    counter = 0;
    if (getifaddrs(&tmp))
    {
        ERROR("getifaddrs error");
        return -1;
    }
    else
    {
        for (tmp; tmp != NULL; tmp = tmp->ifa_next)
        {

            if (tmp->ifa_addr && tmp->ifa_addr->sa_family == AF_INET
                && strstr(tmp->ifa_name, "ppp") != NULL
                && tmp->ifa_ifu.ifu_dstaddr->sa_family == AF_INET)
            {
                counter++;
                sin = (struct sockaddr_in *) tmp->ifa_ifu.ifu_dstaddr;
                connected_mem = (struct sockaddr_in **) realloc(*connected,
                                 counter * sizeof(struct sockaddr_in *));
                if (connected_mem != NULL)
                {
                    connected_mem[counter - 1] = sin;
                   *connected = connected_mem;

                }
            }
        }
    }
    return 0;
}

te_errno
tapi_cfg_l2tp_lns_bit_set(const char *ta, const char *lns,
                          enum l2tp_bit bit, bool selector)
{
    return cfg_set_instance_fmt(CFG_VAL(INTEGER, selector),
                                TE_CFG_TA_L2TP_SERVER "/lns:%s/bit:%s",
                                ta, lns, bit == L2TP_BIT_HIDDEN ?
                                "hidden" : bit == L2TP_BIT_LENGTH ?
                                "length" : "flow");
}

te_errno
tapi_cfg_l2tp_lns_bit_get(const char *ta, const char *lns,
                          enum l2tp_bit *bit, bool *selector)
{
    return cfg_get_instance_fmt((cfg_val_type *) CVT_INTEGER, selector,
                                TE_CFG_TA_L2TP_SERVER "/lns:%s/bit:%s",
                                ta, lns, *bit == L2TP_BIT_HIDDEN ?
                                "hidden" : *bit == L2TP_BIT_LENGTH ?
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
                                "chap" : param.protocol == L2TP_AUTH_PROT_PAP ?
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
                                "chap" : param.protocol == L2TP_AUTH_PROT_PAP ?
                                "pap" : "authentication",
                                param.type == L2TP_AUTH_POLICY_REQUIRE ?
                                "require" : "refuse");
}

/* It seems to be bad solution
 * BEGIN */
te_errno
l2tp_add_secret_part(char *client, char *part, int8_t selector)
{
    cfg_handle  handle;

    return cfg_add_instance_fmt(&handle, CFG_VAL(NONE, NULL),
                                strcat(client, "%s:%s"),
                                selector == L2TP_SECRET_PART_SERVER ?
                                "server" : selector ==
                                           L2TP_SECRET_PART_SECRET ?
                                "secret" : "ipv4", part);
};


te_errno
tapi_cfg_l2tp_lns_secret_add(const char *ta, const char *lns,
                             const l2tp_ppp_secret *new_secret)
{

    char buffer[L2TP_SECRET_INST] = {};

    TE_SPRINTF(buffer, TE_CFG_TA_L2TP_SECRET,
               ta, lns, new_secret->is_chap == L2TP_AUTH_PROT_CHAP ?
                        "chap" : "pap", new_secret->client);

    l2tp_add_secret_part(buffer, new_secret->server,
                         L2TP_SECRET_PART_SERVER);
    l2tp_add_secret_part(buffer, new_secret->secret,
                         L2TP_SECRET_PART_SECRET);
    l2tp_add_secret_part(buffer,
                         inet_ntoa(new_secret->sipv4.sin_addr),
                         L2TP_SECRET_PART_IPV4);

    return 0;

}

te_errno
l2tp_delete_secret_part(char *client, char *part, int8_t selector)
{

    return cfg_del_instance_fmt(FALSE, strcat(client, "%s:%s"),
                                selector == L2TP_SECRET_PART_SERVER ?
                                "server" : selector ==
                                           L2TP_SECRET_PART_SECRET ?
                                           "secret" : "ipv4", part);
};

te_errno
tapi_cfg_l2tp_lns_secret_delete(const char *ta, const char *lns,
                                const l2tp_ppp_secret *prev_secret)
{

    char buffer[L2TP_SECRET_INST] = {};

    TE_SPRINTF(buffer, TE_CFG_TA_L2TP_SECRET,
               ta, lns, prev_secret->is_chap == L2TP_AUTH_PROT_CHAP ?
                        "chap" : "pap", prev_secret->client);

    l2tp_delete_secret_part(buffer, prev_secret->server,
                            L2TP_SECRET_PART_SERVER);
    l2tp_delete_secret_part(buffer, prev_secret->secret,
                            L2TP_SECRET_PART_SECRET);
    l2tp_delete_secret_part(buffer,
                            inet_ntoa(prev_secret->sipv4.sin_addr),
                            L2TP_SECRET_PART_IPV4);

    return 0;

}
/* It seems to be bad solution
 * END */

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
                                       int value)
{
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


