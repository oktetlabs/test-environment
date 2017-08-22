/** @file
 * @brief Network namespaces configuration test API
 *
 * Implementation of test API for network namespaces configuration.
 *
 *
 * Copyright (C) 2017 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 *
 * @author Andrey Dmitrov <Andrey.Dmitrov@oktetlabs.ru>
 */

#define TE_LGR_USER     "NETNS TAPI"

#include "te_config.h"
#include "te_defs.h"
#include "te_errno.h"
#include "te_sockaddr.h"
#include "logger_api.h"
#include "conf_api.h"
#include "rcf_api.h"
#include "rcf_rpc.h"
#include "tapi_namespaces.h"
#include "tapi_cfg_base.h"
#include "tapi_cfg.h"
#include "tapi_cfg_iptables.h"
#include "tapi_rpc_stdio.h"

/* System tool 'ip' */
#define IP_TOOL "ip"

/* Template to excecute a command in a net namespace 'ip netns exec %s '. */
#define IP_NETNS_EXEC_IP_FMT IP_TOOL " netns exec %s " IP_TOOL

/* Maximum length of an agent configuration string. */
#define CONFSTR_LEN 1024

/* See description in tapi_namespaces.h */
te_errno
tapi_netns_add_rsrc(const char *ta, const char *ns_name)
{
    char ns_oid[CFG_OID_MAX];

    snprintf(ns_oid, CFG_OID_MAX, "/agent:%s/namespace:/net:%s", ta, ns_name);
    return cfg_add_instance_fmt(NULL, CVT_STRING, ns_oid,
                                "/agent:%s/rsrc:netns_%s", ta, ns_name);
}

/* See description in tapi_namespaces.h */
te_errno
tapi_netns_del_rsrc(const char *ta, const char *ns_name)
{
    return cfg_del_instance_fmt(FALSE, "/agent:%s/rsrc:netns_%s",
                                ta, ns_name);
}

/* See description in tapi_namespaces.h */
te_errno
tapi_netns_add(const char *ta, const char *ns_name)
{
    te_errno rc;

    rc = cfg_add_instance_fmt(NULL, CVT_NONE, NULL,
                              "/agent:%s/namespace:/net:%s", ta, ns_name);
    if (rc != 0)
        return rc;

    return tapi_netns_add_rsrc(ta, ns_name);
}

/* See description in tapi_namespaces.h */
te_errno
tapi_netns_del(const char *ta, const char *ns_name)
{
    te_errno rc;

    rc = cfg_del_instance_fmt(TRUE, "/agent:%s/namespace:/net:%s", ta,
                              ns_name);
    if (rc != 0)
        return rc;

    return tapi_netns_del_rsrc(ta, ns_name);
}

/* See description in tapi_namespaces.h */
te_errno
tapi_netns_if_set(const char *ta, const char *ns_name, const char *if_name)
{
    te_errno rc;

    rc = cfg_add_instance_fmt(NULL, CVT_NONE, NULL,
                              "/agent:%s/namespace:/net:%s/interface:%s",
                              ta, ns_name, if_name);
    if (rc != 0)
        return rc;

    rc = tapi_cfg_base_if_del_rsrc(ta, if_name);
    if (rc == TE_RC(TE_CS, TE_ENOENT))
        return 0;

    return rc;
}

/* See description in tapi_namespaces.h */
te_errno
tapi_netns_if_unset(const char *ta, const char *ns_name, const char *if_name)
{
    return cfg_del_instance_fmt(TRUE,
                                "/agent:%s/namespace:/net:%s/interface:%s",
                                ta, ns_name, if_name);
}

/**
 * Apply the minimum network configuration in new network namespace.
 *
 * @param ta            Test agent name
 * @param ns_name       The network namespace name
 * @param gw_addr_str   Gateway address
 * @param ctl_addr_str  Control channel IP address
 * @param prefix        The IP address prefix length
 * @param ctl_if        The control interface name
 *
 * @param Status code
 */
static te_errno
configure_netns_base_network(const char *ta, const char *ns_name,
                             const char *gw_addr_str,
                             const char *ctl_addr_str,
                             int prefix, const char *ctl_if)
{
    rcf_rpc_server *rpcs   = NULL;
    te_errno rc;

    rc = rcf_rpc_server_create(ta, "pco_ctl", &rpcs);
    if (rc != 0)
        return rc;

#define EXEC_SH_CMD(_rpcs, _cmd, _args...) \
    do {                                                            \
        rpc_wait_status st;                                         \
        RPC_AWAIT_IUT_ERROR(_rpcs);                                 \
        st = rpc_system_ex(_rpcs, _cmd, _args);                     \
        if (st.flag != RPC_WAIT_STATUS_EXITED || st.value != 0)     \
        {                                                           \
            ERROR("%s:%d: shell command execution failed: " _cmd,   \
                  __FUNCTION__, __LINE__, _args);                   \
            rcf_rpc_server_destroy(rpcs);                           \
            return TE_RC(TE_TAPI, TE_EFAIL);                        \
        }                                                           \
    } while (0)

    EXEC_SH_CMD(rpcs, IP_NETNS_EXEC_IP_FMT" li set dev lo up",
                ns_name);
    EXEC_SH_CMD(rpcs, IP_NETNS_EXEC_IP_FMT" li set dev %s up",
                ns_name, ctl_if);
    EXEC_SH_CMD(rpcs, IP_NETNS_EXEC_IP_FMT" addr add %s/%d dev %s",
                ns_name, ctl_addr_str, prefix, ctl_if);
    EXEC_SH_CMD(rpcs, IP_NETNS_EXEC_IP_FMT" route add default "
                "dev %s via %s", ns_name, ctl_if, gw_addr_str);

#undef CALL_SH_CMD

    return rcf_rpc_server_destroy(rpcs);
}

/* See description in tapi_namespaces.h */
te_errno
tapi_netns_create_ns_with_net_channel(const char *ta, const char *ns_name,
                                      const char *veth1, const char *veth2,
                                      const char *ctl_if, int rcfport)
{
    struct sockaddr *addr1 = NULL;
    struct sockaddr *addr2 = NULL;
    char            *addr_str1 = NULL;
    char            *addr_str2 = NULL;

    te_bool  enabled = TRUE;
    int      prefix;
    te_errno rc;

#define CHRC(_x)    \
    do {                                                \
        rc = _x;                                        \
        if (rc != 0)                                    \
        {                                               \
            ERROR("%s:%d: %s -> %r",                    \
                  __FUNCTION__, __LINE__,  #_x, rc);    \
            goto tapi_netns_create_cleanup;             \
        }                                               \
    } while (0)

    CHRC(tapi_cfg_alloc_net_addr_pair(&addr1, &addr2, &prefix));
    CHRC(te_sockaddr_h2str(addr1, &addr_str1));
    CHRC(te_sockaddr_h2str(addr2, &addr_str2));

    CHRC(tapi_cfg_base_ipv4_fw(ta, &enabled));
    CHRC(tapi_netns_add(ta, ns_name));
    CHRC(tapi_cfg_base_if_add_veth(ta, veth1, veth2));
    CHRC(tapi_netns_if_set(ta, ns_name, veth2));

    CHRC(configure_netns_base_network(ta, ns_name, addr_str1, addr_str2,
                                      prefix, veth2));

    CHRC(tapi_cfg_base_if_add_net_addr(ta, veth1, addr1,
                                       prefix, FALSE, NULL));

    /* iptables -t nat -A POSTROUTING -o ctl_if -j MASQUERADE");*/
    CHRC(tapi_cfg_iptables_chain_add(ta, veth1, "nat", "NS_MASQUERADE",
                                     FALSE));
    CHRC(tapi_cfg_iptables_cmd_fmt(ta, veth1, "nat", "NS_MASQUERADE",
                                       "-A POSTROUTING -o %s -j", ctl_if));
    CHRC(tapi_cfg_iptables_cmd(ta, veth1, "nat",
                                   "NS_MASQUERADE", "-A -j MASQUERADE"));

    /* iptables -t nat -A PREROUTING -p tcp --dport rcfport -j DNAT
     *          --to-destination addr_str2:rcfport  */
    CHRC(tapi_cfg_iptables_chain_add(ta, veth1, "nat", "NS_PORT_FW", FALSE));
    CHRC(tapi_cfg_iptables_cmd_fmt(ta, veth1, "nat", "NS_PORT_FW",
                                   "-A PREROUTING -p tcp --dport %d -j",
                                   rcfport));
    CHRC(tapi_cfg_iptables_cmd_fmt(ta, veth1, "nat", "NS_PORT_FW",
                                   "-A -p tcp -j DNAT --to-destination %s:%d",
                                   addr_str2, rcfport));

tapi_netns_create_cleanup:
    free(addr1);
    free(addr2);
    free(addr_str1);
    free(addr_str2);

    return rc;
#undef CHRC
}

/* See description in tapi_namespaces.h */
te_errno
tapi_netns_add_ta(const char *host, const char *ns_name, const char *ta_name,
                  const char *ta_type, int rcfport)
{
    char    confstr[CONFSTR_LEN];
    int     res;

    res = snprintf(confstr, sizeof(confstr),
                   "%s:%d:sudo:"IP_TOOL" netns exec %s:",
                   host, rcfport, ns_name);
    if (res >= (int)sizeof(confstr))
        return TE_RC(TE_TAPI, TE_ESMALLBUF);

    return rcf_add_ta(ta_name, ta_type, "rcfunix", confstr, 0);
}
