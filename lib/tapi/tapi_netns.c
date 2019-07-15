/** @file
 * @brief Network namespaces configuration test API
 *
 * Implementation of test API for network namespaces configuration.
 *
 *
 * Copyright (C) 2003-2019 OKTET Labs. All rights reserved.
 *
 * 
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
#include "tapi_host_ns.h"

/* System tool 'ip' */
#define IP_TOOL "ip"

/* System tool 'dhclient' */
#define DHCLIENT_TOOL "dhclient"

/* Template to excecute a command in a net namespace 'ip netns exec %s '. */
#define IP_NETNS_EXEC_FMT IP_TOOL " netns exec %s "

/* Template to excecute a command in a net namespace 'ip netns exec %s ip'. */
#define IP_NETNS_EXEC_IP_FMT IP_NETNS_EXEC_FMT IP_TOOL

/* Specify dhclient PID file pathname.
 * It should contain namespace name, because each net namespace can have its
 * own instance of dhclient. */
#define DHCLIENT_PID_FILE_FMT "PATH_DHCLIENT_PID=/tmp/te_dhclient_%s.pid "

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

    CHRC(tapi_cfg_base_ipv4_fw(ta, TRUE));
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
                  const char *ta_type, int rcfport, const char *ta_conn)
{
    char     confstr[CONFSTR_LEN];
    int      res;
    te_errno rc;

    res = snprintf(confstr, sizeof(confstr),
                   "host=%s:port=%d:sudo:connect=%s:shell="IP_TOOL" netns exec %s:",
                   host, rcfport, ta_conn == NULL ? "" : ta_conn, ns_name);
    if (res >= (int)sizeof(confstr))
        return TE_RC(TE_TAPI, TE_ESMALLBUF);

    rc = rcf_add_ta(ta_name, ta_type, "rcfunix", confstr, 0);
    if (rc != 0)
        return rc;

    if (tapi_host_ns_enabled())
        rc = tapi_host_ns_agent_add(host, ta_name, ns_name);

    return rc;
}

/**
 * Extract issued IP address from dhclient output.
 *
 * @param buf       Buffer with dhclient output
 * @param addr      Buffer to keep extracted IP address
 * @param addr_len  Length of the buffer @p addr
 *
 * @return Status code
 */
static te_errno
dhclient_get_addr(char *buf, char *addr, size_t addr_len)
{
    const char *pattern = "bound to ";
    char       *ptr;
    size_t      len;

    if (buf == NULL)
        return TE_RC(TE_TAPI, TE_EINVAL);

    ptr = strstr(buf, pattern);
    if (ptr == NULL)
        return TE_RC(TE_TAPI, TE_ENOENT);
    ptr += strlen(pattern);
    ptr = strtok(ptr, " ");
    if (ptr == NULL)
        return TE_RC(TE_TAPI, TE_ENOENT);
    len = strlen(ptr) + 1;
    if (len > addr_len)
        return TE_RC(TE_TAPI, TE_ESMALLBUF);
    memcpy(addr, ptr, len);

    return 0;
}

/**
 * Apply the minimum network configuration in new network namespace, obtain
 * local address using dhclient.
 *
 * @param ta            Test agent name
 * @param ns_name       The network namespace name
 * @param ctl_if        The control interface name
 * @param[out] addr     Issued IP address
 * @param addr_len      Length of the buffer @p addr
 *
 * @param Status code
 */
static te_errno
configure_netns_network_dhclient(const char *ta, const char *ns_name,
                                 const char *ctl_if, char *addr,
                                 size_t addr_len)
{
    rcf_rpc_server *rpcs   = NULL;
    te_errno        rc;
    te_errno        rc2;
    char           *pbuf[2] = {};
    rpc_wait_status st;

    rc = rcf_rpc_server_create(ta, "pco_ctl", &rpcs);
    if (rc != 0)
        return rc;

    RPC_AWAIT_IUT_ERROR(rpcs);
    st = rpc_system_ex(rpcs, IP_NETNS_EXEC_IP_FMT" li set dev lo up",
                       ns_name);
    if (st.flag != RPC_WAIT_STATUS_EXITED || st.value != 0)
    {
        ERROR("Failed to get up loopback interface");
        rcf_rpc_server_destroy(rpcs);
        return TE_RC(TE_TAPI, TE_EFAIL);
    }

    /* Stop previously run dhclient if it is, ignore execution result. */
    RPC_AWAIT_IUT_ERROR(rpcs);
    rpc_system_ex(rpcs, DHCLIENT_PID_FILE_FMT IP_NETNS_EXEC_FMT
                  DHCLIENT_TOOL " -x", ns_name, ns_name);

    RPC_AWAIT_IUT_ERROR(rpcs);
    st = rpc_shell_get_all3(rpcs, pbuf, DHCLIENT_PID_FILE_FMT
                            IP_NETNS_EXEC_FMT DHCLIENT_TOOL
                            " -4 -v %s", ns_name, ns_name, ctl_if);
    if (st.flag != RPC_WAIT_STATUS_EXITED || st.value != 0)
    {
        ERROR("Failed to get IP using dhclient");
        if (pbuf[0] != NULL)
            RING("dhclient: %s", pbuf[0]);
        if (pbuf[1] != NULL)
            RING("dhclient stderr: %s", pbuf[1]);
        free(pbuf[0]);
        free(pbuf[1]);
        rcf_rpc_server_destroy(rpcs);
        return TE_RC(TE_TAPI, TE_EFAIL);
    }

    rc = dhclient_get_addr(pbuf[1], addr, addr_len);
    if (rc != 0)
    {
        ERROR("Cannot extract obtained IP address from dhclient dump: %r",
              rc);
        RING("dhclient stderr: %s", pbuf[1]);
    }
    free(pbuf[0]);
    free(pbuf[1]);

    rc2 = rcf_rpc_server_destroy(rpcs);
    if (rc == 0)
        rc = rc2;

    return rc;
}

/**
 * Add or delete MAC vlan on @p ctl_if.
 *
 * @param ta            Test agent name
 * @param ctl_if        Control interface name on the test agent
 * @param macvlan_if    MAC VLAN interface name
 * @param add           Add if @c TRUE, else - remove the macvlan interface
 *
 * @return Status code
 */
static te_errno
add_del_macvlan(const char *ta, const char *ctl_if, const char *macvlan_if,
                te_bool add)
{
    cfg_val_type val_type = CVT_STRING;
    te_errno     rc = 0;
    te_errno     rc2 = 0;
    te_bool      grabbed = FALSE;

    if (cfg_get_instance_fmt(&val_type, NULL, "/agent:%s/rsrc:%s",
                             ta, ctl_if) == 0)
        grabbed = TRUE;

    if (!grabbed)
    {
        rc = tapi_cfg_base_if_add_rsrc(ta, ctl_if);
        if (rc != 0)
            return rc;
    }

    if (add)
    {
        rc = tapi_cfg_base_if_add_macvlan(ta, ctl_if, macvlan_if, NULL);

        /* Do not keep it in the configuration tree. */
        if (rc == 0 && tapi_host_ns_enabled())
        {
            rc = tapi_host_ns_if_del(ta, macvlan_if, TRUE);
            if (rc != 0)
                ERROR("Failed to remove interface %s/%s from /local/host: %r",
                      ta, macvlan_if, rc);
        }
    }
    else
    {
        rc = tapi_cfg_base_if_del_macvlan(ta, ctl_if, macvlan_if);
        /* Ignore ENOENT - macvlan interface may be not grabbed as the
         * resource, it is not important. */
        if (rc == TE_RC(TE_CS, TE_ENOENT))
            rc = 0;
    }

    if (!grabbed)
    {
        rc2 = tapi_cfg_base_if_del_rsrc(ta, ctl_if);
        if (rc == 0)
            rc = rc2;
    }

    return rc;
}

/* See description in tapi_namespaces.h */
te_errno
tapi_netns_create_ns_with_macvlan(const char *ta, const char *ns_name,
                                  const char *ctl_if, const char *macvlan_if,
                                  char *addr, size_t addr_len)
{
    te_errno rc = 0;

    rc = tapi_netns_add(ta, ns_name);
    if (rc != 0)
        return rc;

    rc = add_del_macvlan(ta, ctl_if, macvlan_if, TRUE);
    if (rc != 0)
        return rc;

    rc = tapi_netns_if_set(ta, ns_name, macvlan_if);
    if (rc != 0)
        return rc;

    return configure_netns_network_dhclient(ta, ns_name, macvlan_if, addr,
                                            addr_len);
}

/**
 * Stop dhclient running in the namespace.
 *
 * @param ta            Test agent name
 * @param ns_name       The network namespace name
 *
 * Status code
 */
static te_errno
stop_dhclient(const char *ta, const char *ns_name)
{
    rcf_rpc_server *rpcs;
    te_errno        rc;
    rpc_wait_status st;

    rc = rcf_rpc_server_create(ta, "pco_ctl", &rpcs);
    if (rc != 0)
        return rc;

    RPC_AWAIT_IUT_ERROR(rpcs);
    st = rpc_system_ex(rpcs, DHCLIENT_PID_FILE_FMT IP_NETNS_EXEC_FMT
                       DHCLIENT_TOOL " -x", ns_name, ns_name);

    rc = rcf_rpc_server_destroy(rpcs);
    if (st.flag != RPC_WAIT_STATUS_EXITED || st.value != 0)
    {
        ERROR("Failed to kill dhclient");
        return TE_RC(TE_TAPI, TE_EFAIL);
    }

    return rc;
}

/* See description in tapi_namespaces.h */
te_errno
tapi_netns_destroy_ns_with_macvlan(const char *ta, const char *ns_name,
                                   const char *ctl_if, const char *macvlan_if)
{
    te_errno rc;
    te_errno rc2;

    rc = stop_dhclient(ta, ns_name);

    rc2 = tapi_netns_del(ta, ns_name);
    if (rc == 0)
        rc = rc2;

    rc2 = add_del_macvlan(ta, ctl_if, macvlan_if, FALSE);
    if (rc == 0)
        rc = rc2;

    return rc;
}

/**
 * Add or delete IP vlan on @p ctl_if.
 *
 * @param ta            Test agent name
 * @param ctl_if        Control interface name on the test agent
 * @param ipvlan_if     IP VLAN interface name
 * @param add           Add if @c TRUE, else - remove the ipvlan interface
 *
 * @return Status code
 */
static te_errno
add_del_ipvlan(const char *ta, const char *ctl_if, const char *ipvlan_if,
               te_bool add)
{
    cfg_val_type val_type = CVT_STRING;
    te_errno     rc = 0;
    te_errno     rc2 = 0;
    te_bool      grabbed = FALSE;

    if (cfg_get_instance_fmt(&val_type, NULL, "/agent:%s/rsrc:%s",
                             ta, ctl_if) == 0)
    {
        grabbed = TRUE;
    }

    if (!grabbed)
    {
        rc = tapi_cfg_base_if_add_rsrc(ta, ctl_if);
        if (rc != 0)
            return rc;
    }

    if (add)
    {
        rc = tapi_cfg_base_if_add_ipvlan(ta, ctl_if, ipvlan_if, NULL, NULL);

        /* Do not keep it in the configuration tree. */
        if (rc == 0 && tapi_host_ns_enabled())
        {
            rc = tapi_host_ns_if_del(ta, ipvlan_if, TRUE);
            if (rc != 0)
            {
                ERROR("Failed to remove interface %s/%s from /local/host: %r",
                      ta, ipvlan_if, rc);
            }
        }
    }
    else
    {
        rc = tapi_cfg_base_if_del_ipvlan(ta, ctl_if, ipvlan_if);
    }

    if (!grabbed)
    {
        rc2 = tapi_cfg_base_if_del_rsrc(ta, ctl_if);
        if (rc == 0)
            rc = rc2;
    }

    return rc;
}

/* See description in tapi_namespaces.h */
te_errno
tapi_netns_create_ns_with_ipvlan(const char *ta, const char *ns_name,
                                 const char *ctl_if, const char *ipvlan_if,
                                 char *addr, size_t addr_len)
{
    te_errno rc = 0;

    rc = tapi_netns_add(ta, ns_name);
    if (rc != 0)
        return rc;

    rc = add_del_ipvlan(ta, ctl_if, ipvlan_if, TRUE);
    if (rc != 0)
        return rc;

    rc = tapi_netns_if_set(ta, ns_name, ipvlan_if);
    if (rc != 0)
        return rc;

    return configure_netns_network_dhclient(ta, ns_name, ipvlan_if, addr,
                                            addr_len);
}

/* See description in tapi_namespaces.h */
te_errno
tapi_netns_destroy_ns_with_ipvlan(const char *ta, const char *ns_name,
                                  const char *ctl_if, const char *ipvlan_if)
{
    te_errno rc;
    te_errno rc2;

    rc = stop_dhclient(ta, ns_name);

    rc2 = tapi_netns_del(ta, ns_name);
    if (rc == 0)
        rc = rc2;

    rc2 = add_del_ipvlan(ta, ctl_if, ipvlan_if, FALSE);
    if (rc == 0)
        rc = rc2;

    return rc;
}
