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

/**
 * Converts the l2tp_ipv4_range's fields to
 * the string like allowX.X.X.X-Y.Y.Y.Y
 *
 * @param iprange   testing ip
 *
 * @return range
 */
char *
l2tp_get_range(const l2tp_ipv4_range *iprange)
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
                            enum l2tp_iprange_class kind, l2tp_range **ranges)
{
    if (*ranges == NULL)
        *ranges = (l2tp_range *)malloc(sizeof(l2tp_range));

    cfg_handle  handle;
    struct sockaddr_in *local;
    char       *range = l2tp_get_range(iprange);
    cfg_get_instance_fmt((cfg_val_type *) CVT_ADDRESS, local,
                                TE_CFG_TA_L2TP_SERVER "/lns:%s/local_ip:",
                                ta, lns);

    l2tp_range *element = (l2tp_range *)malloc(sizeof(l2tp_range));

    element->lns = strdup(lns);
    element->local_ip = strdup(inet_ntoa(local->sin_addr));
    element->range = range;

    SLIST_INSERT_HEAD(&(*ranges)->l2tp_range, element, list);

    return cfg_add_instance_fmt(&handle, CFG_VAL(NONE, NULL),
                                TE_CFG_TA_L2TP_SERVER "/lns:%s/%s_range:%s",
                                ta, lns, kind == L2TP_IP_RANGE_CLASS_IP ?
                                "ip" : "lac", range);
}


static te_errno
tapi_cfg_l2tp_lns_range_del(const char *ta, const char *lns,
                            const l2tp_ipv4_range *iprange,
                            enum l2tp_iprange_class kind, l2tp_range **ranges)
{
    char        *range = l2tp_get_range(iprange);
    l2tp_range  *temp;

    for (temp = SLIST_FIRST(&(*ranges)->l2tp_range);
         temp != NULL || strcmp(temp->lns, lns) != 0;
         temp = SLIST_NEXT(temp, list));

    if (temp != NULL)
    {
        SLIST_REMOVE(&(*ranges)->l2tp_range, temp, l2tp_range, list);

        return cfg_del_instance_fmt(FALSE, TE_CFG_TA_L2TP_SERVER
                                            "/lns:%s/%s_range:%s", ta,
                                    lns, kind == L2TP_IP_RANGE_CLASS_IP ?
                                         "ip" : "lac", range);
    }
    else
        return -1;

}

/**
 * Check the ip address is equal to any existing local ip
 *
 * @param local_ip   testing ip
 * @param ranges     array of ip ranges
 *
 * @return TRUE if local_ip is equal to
 *         local ip from ranges' array
 *         otherwise it returns FALSE
 */
static te_bool
l2tp_check_lns_ip(char *local_ip, l2tp_range **ranges)
{
    l2tp_range  *temp;

    for (temp = SLIST_FIRST(&(*ranges)->l2tp_range);
         temp != NULL || (strcmp(temp->local_ip, local_ip) != 0);
         temp = SLIST_NEXT(temp, list));

    if (temp == NULL)
        return FALSE;
    else
        return TRUE;
};

/**
 * Check the ip address belongs to any existing ip range
 *
 * @param tunnel_ip  testing ip
 * @param ranges     array of ip ranges
 *
 * @return TRUE if tunnel_ip belongs to any ip range
 *         otherwise it returns FALSE
 */
static te_bool
l2tp_check_lns_range(char *tunnel_ip, l2tp_range **ranges)
{
    l2tp_range  *temp;

    for (temp = SLIST_FIRST(&(*ranges)->l2tp_range);
         temp != NULL;
         temp = SLIST_NEXT(temp, list))
    {
        if (te_l2tp_ip_compare(temp->range, tunnel_ip))
            return TRUE;

    }
    return FALSE;
}

te_errno
tapi_cfg_l2tp_lns_connected_get(const char *ta, const char *lns,
                                struct sockaddr_in ***connected,
                                l2tp_range **ranges)
{

    UNUSED(ta);
    UNUSED(lns);
    if (*connected != NULL)
    {
        return -1;
    }
    struct ifaddrs         *tmp, *perm;
    struct sockaddr_in    **connected_mem;
    struct sockaddr_in     *cip;
    int    counter = 0;
    if (getifaddrs(&perm))
    {
        ERROR("getifaddrs error");
        return -1;
    }
    else
    {
        for (tmp = perm; tmp != NULL; tmp = tmp->ifa_next)
        {

            if (tmp->ifa_addr && tmp->ifa_addr->sa_family == AF_INET
                && l2tp_check_lns_ip(inet_ntoa(
                    ((struct sockaddr_in *) tmp->ifa_addr)->sin_addr), ranges)
                && tmp->ifa_ifu.ifu_dstaddr->sa_family == AF_INET)
            {
                cip = (struct sockaddr_in *) tmp->ifa_ifu.ifu_dstaddr;
                if (l2tp_check_lns_range(inet_ntoa(cip->sin_addr), ranges))
                {
                    counter++;
                    connected_mem = (struct sockaddr_in **) realloc(*connected,
                                     counter * sizeof(struct sockaddr_in *));
                    if (connected_mem != NULL)
                    {
                        connected_mem[counter - 1] = cip;
                        *connected = connected_mem;

                    }
                }

            }
        }
    }
    freeifaddrs(perm);
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


