/** @file
 * @brief Unix Test Agent
 *
 * Unix TA configuring support
 *
 *
 * Copyright (C) 2004,2005 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
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
 * @author Elena A. Vengerova <Elena.Vengerova@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER     "Unix Conf"

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#endif
#if HAVE_STRINGS_H
#include <strings.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_FCNTL_H
#include <fcntl.h>
#endif
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#if HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#if HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#if HAVE_NET_IF_H
#include <net/if.h>
#endif
#if HAVE_NET_IF_ARP_H
#include <net/if_arp.h>
#endif
#if HAVE_NET_ETHERNET_H
#include <net/ethernet.h>
#endif
#if HAVE_NET_IF_ETHER_H
#include <net/if_ether.h>
#endif
#if HAVE_NET_IF_DL_H
#include <net/if_dl.h>
#endif
#if HAVE_NET_ROUTE_H
#include <net/route.h>
#endif

#include "te_stdint.h"
#include "te_errno.h"
#include "te_defs.h"
#include "logger_api.h"
#include "comm_agent.h"
#include "rcf_ch_api.h"
#include "rcf_pch.h"
#include "rcf_pch_ta_cfg.h"
#include "logger_api.h"
#include "unix_internal.h"
#include "cs_common.h"

#ifdef CFG_UNIX_DAEMONS
#include "conf_daemons.h"
#endif

#ifdef USE_NETLINK
#include <sys/select.h>
#include <asm/types.h>
#include <linux/netlink.h>
#include <fnmatch.h>
#include <linux/sockios.h>
#include <iproute/libnetlink.h>
#include <iproute/rt_names.h>
#include <iproute/utils.h>
#include <iproute/ll_map.h>
#include <iproute/ip_common.h>
#endif

#ifndef IF_NAMESIZE
#define IF_NAMESIZE IFNAMSIZ
#endif

/**< MAC address in human notation */
#ifndef ETH_ADDRSTRLEN
#define ETH_ADDRSTRLEN (3 * ETHER_ADDR_LEN - 1)
#endif

#if !defined(__linux__) && defined(USE_NETLINK)
#error netlink can be used on Linux only
#endif

#undef NEIGH_USE_NETLINK

#ifdef ENABLE_8021X
extern te_errno ta_unix_conf_supplicant_init();
extern te_errno supplicant_grab(const char *name);
extern te_errno supplicant_release(const char *name);
#endif

#ifdef ENABLE_WIFI_SUPPORT
extern te_errno ta_unix_conf_wifi_init();
#endif

#ifdef WITH_ISCSI
extern te_errno ta_unix_iscsi_target_init();
extern te_errno ta_unix_iscsi_initiator_init();
#endif

#ifdef USE_NETLINK
struct nlmsg_list {
    struct nlmsg_list *next;
    struct nlmsghdr    h;
};
#endif

/** Check that interface is locked for using of this TA */
#if 1 /* Until dependencies in configurator are implemented */
#define INTERFACE_IS_MINE(ifname) \
    (strcmp(ifname, "lo") == 0 || \
     rcf_pch_rsrc_accessible("/agent:%s/interface:%s", ta_name, ifname))
#else
#define INTERFACE_IS_MINE(ifname)       TRUE
#endif

#define CHECK_INTERFACE(ifname)                     \
    ((ifname == NULL)? TE_EINVAL :                  \
     (strlen(ifname) > IFNAMSIZ)? TE_E2BIG :        \
     (strchr(ifname, ':') != NULL ||                \
      !INTERFACE_IS_MINE(ifname))? TE_ENODEV : 0)   \

/**
 * Type for both IPv4 and IPv6 address
 */
typedef union gen_ip_address {
    struct in_addr  ip4_addr;  /** IPv4 address */
    struct in6_addr ip6_addr;  /** IPv6 address */
} gen_ip_address;

const char *te_lockdir = "/tmp";

/* Auxiliary variables used for during configuration request processing */
static struct ifreq req;

static char buf[4096];
static char trash[128];
static te_errno  cfg_socket = -1;

/*
 * Access routines prototypes (comply to procedure types
 * specified in rcf_ch_api.h).
 */
static te_errno env_get(unsigned int, const char *, char *,
                   const char *);
static te_errno env_set(unsigned int, const char *, const char *,
                   const char *);
static te_errno env_add(unsigned int, const char *, const char *,
                   const char *);
static te_errno env_del(unsigned int, const char *,
                   const char *);
static te_errno env_list(unsigned int, const char *, char **);

/** Environment variables hidden in list operation */
static const char * const env_hidden[] = {
    "SSH_CLIENT",
    "SSH_CONNECTION",
    "SUDO_COMMAND",
    "TE_RPC_PORT",
    "TE_LOG_PORT",
    "TARPC_DL_NAME",
    "TCE_CONNECTION",
    "LD_PRELOAD"
};

static te_errno ip4_fw_get(unsigned int, const char *, char *);
static te_errno ip4_fw_set(unsigned int, const char *, const char *);

static te_errno interface_list(unsigned int, const char *, char **);
static te_errno interface_add(unsigned int, const char *, const char *,
                              const char *);
static te_errno interface_del(unsigned int, const char *, const char *);

static te_errno net_addr_add(unsigned int, const char *, const char *,
                             const char *, const char *);
static te_errno net_addr_del(unsigned int, const char *,
                             const char *, const char *);
static te_errno net_addr_list(unsigned int, const char *, char **,
                              const char *);

static te_errno prefix_get(unsigned int, const char *, char *,
                           const char *, const char *);
static te_errno prefix_set(unsigned int, const char *, const char *,
                           const char *, const char *);

static te_errno broadcast_get(unsigned int, const char *, char *,
                              const char *, const char *);
static te_errno broadcast_set(unsigned int, const char *, const char *,
                              const char *, const char *);

static te_errno link_addr_get(unsigned int, const char *, char *,
                              const char *);

static te_errno link_addr_set(unsigned int, const char *, const char *,
                              const char *);

static te_errno ifindex_get(unsigned int, const char *, char *,
                            const char *);

static te_errno status_get(unsigned int, const char *, char *,
                           const char *);
static te_errno status_set(unsigned int, const char *, const char *,
                           const char *);

static te_errno arp_get(unsigned int, const char *, char *,
                        const char *);
static te_errno arp_set(unsigned int, const char *, const char *,
                        const char *);

static te_errno mtu_get(unsigned int, const char *, char *,
                        const char *);
static te_errno mtu_set(unsigned int, const char *, const char *,
                        const char *);

static te_errno neigh_state_get(unsigned int, const char *, char *,
                                const char *, const char *);
static te_errno neigh_get(unsigned int, const char *, char *,
                          const char *, const char *);

static te_errno neigh_set(unsigned int, const char *, const char *,
                          const char *, const char *);
static te_errno neigh_add(unsigned int, const char *, const char *,
                          const char *, const char *);
static te_errno neigh_del(unsigned int, const char *,
                          const char *, const char *);
static te_errno neigh_list(unsigned int, const char *, char **,
                           const char *);

/* 
 * This is a bit of hack - there are same handlers for static and dynamic
 * branches, handler discovers dynamic subtree by presence of
 * "dynamic" in OID. But list method does not contain the last subid.
 */
static te_errno
neigh_dynamic_list(unsigned int gid, const char *oid, char **list, 
                   const char *ifname)
{
    UNUSED(oid);
    
    return neigh_list(gid, "dynamic", list, ifname);
}                   

static te_errno route_mtu_get(unsigned int, const char *, char *,
                              const char *);
static te_errno route_mtu_set(unsigned int, const char *, const char *,
                              const char *);
static te_errno route_win_get(unsigned int, const char *, char *,
                              const char *);
static te_errno route_win_set(unsigned int, const char *, const char *,
                              const char *);
static te_errno route_irtt_get(unsigned int, const char *, char *,
                               const char *);
static te_errno route_irtt_set(unsigned int, const char *, const char *,
                               const char *);
static te_errno route_dev_get(unsigned int, const char *, char *,
                              const char *);
static te_errno route_dev_set(unsigned int, const char *, const char *,
                              const char *);
static te_errno route_get(unsigned int, const char *, char *, const char *);
static te_errno route_set(unsigned int, const char *, const char *,
                          const char *);
static te_errno route_add(unsigned int, const char *, const char *,
                          const char *);
static te_errno route_del(unsigned int, const char *,
                          const char *);
static te_errno route_list(unsigned int, const char *, char **);

static te_errno route_commit(unsigned int gid, const cfg_oid *p_oid);

static te_errno nameserver_get(unsigned int, const char *, char *,
                               const char *, ...);

static te_errno user_list(unsigned int, const char *, char **);
static te_errno user_add(unsigned int, const char *, const char *,
                         const char *);
static te_errno user_del(unsigned int, const char *, const char *);

/* Unix Test Agent configuration tree */

static rcf_pch_cfg_object node_route;

RCF_PCH_CFG_NODE_RWC(node_route_irtt, "irtt", NULL, NULL,
                     route_irtt_get, route_irtt_set, &node_route);

RCF_PCH_CFG_NODE_RWC(node_route_win, "win", NULL, &node_route_irtt,
                     route_win_get, route_win_set, &node_route);

RCF_PCH_CFG_NODE_RWC(node_route_mtu, "mtu", NULL, &node_route_win,
                     route_mtu_get, route_mtu_set, &node_route);

RCF_PCH_CFG_NODE_RWC(node_route_dev, "dev", NULL, &node_route_mtu,
                     route_dev_get, route_dev_set, &node_route);

static rcf_pch_cfg_object node_route =
    {"route", 0, &node_route_dev, NULL,
     (rcf_ch_cfg_get)route_get, (rcf_ch_cfg_set)route_set,
     (rcf_ch_cfg_add)route_add, (rcf_ch_cfg_del)route_del,
     (rcf_ch_cfg_list)route_list, (rcf_ch_cfg_commit)route_commit, NULL};

RCF_PCH_CFG_NODE_RO(node_dns, "dns",
                    NULL, &node_route,
                    (rcf_ch_cfg_list)nameserver_get);

RCF_PCH_CFG_NODE_RO(node_neigh_state, "state",
                    NULL, NULL,
                    (rcf_ch_cfg_list)neigh_state_get);

static rcf_pch_cfg_object node_neigh_dynamic =
    { "neigh_dynamic", 0, &node_neigh_state, NULL,
      (rcf_ch_cfg_get)neigh_get, (rcf_ch_cfg_set)neigh_set,
      (rcf_ch_cfg_add)neigh_add, (rcf_ch_cfg_del)neigh_del,
      (rcf_ch_cfg_list)neigh_dynamic_list, NULL, NULL};
      
static rcf_pch_cfg_object node_neigh_static =
    { "neigh_static", 0, NULL, &node_neigh_dynamic,
      (rcf_ch_cfg_get)neigh_get, (rcf_ch_cfg_set)neigh_set,
      (rcf_ch_cfg_add)neigh_add, (rcf_ch_cfg_del)neigh_del,
      (rcf_ch_cfg_list)neigh_list, NULL, NULL};
      
RCF_PCH_CFG_NODE_RW(node_status, "status", NULL, &node_neigh_static,
                    status_get, status_set);

RCF_PCH_CFG_NODE_RW(node_mtu, "mtu", NULL, &node_status,
                    mtu_get, mtu_set);

RCF_PCH_CFG_NODE_RW(node_arp, "arp", NULL, &node_mtu,
                    arp_get, arp_set);

RCF_PCH_CFG_NODE_RW(node_link_addr, "link_addr", NULL, &node_arp,
                    link_addr_get, link_addr_set);

RCF_PCH_CFG_NODE_RW(node_broadcast, "broadcast", NULL, NULL,
                    broadcast_get, broadcast_set);

static rcf_pch_cfg_object node_net_addr =
    { "net_addr", 0, &node_broadcast, &node_link_addr,
      (rcf_ch_cfg_get)prefix_get, (rcf_ch_cfg_set)prefix_set,
      (rcf_ch_cfg_add)net_addr_add, (rcf_ch_cfg_del)net_addr_del,
      (rcf_ch_cfg_list)net_addr_list, NULL, NULL };

RCF_PCH_CFG_NODE_RO(node_ifindex, "index", NULL, &node_net_addr,
                    ifindex_get);

RCF_PCH_CFG_NODE_COLLECTION(node_interface, "interface",
                            &node_ifindex, &node_dns,
                            interface_add, interface_del,
                            interface_list, NULL);

RCF_PCH_CFG_NODE_RW(node_ip4_fw, "ip4_fw", NULL, &node_interface,
                    ip4_fw_get, ip4_fw_set);

static rcf_pch_cfg_object node_env =
    { "env", 0, NULL, &node_ip4_fw,
      (rcf_ch_cfg_get)env_get, (rcf_ch_cfg_set)env_set,
      (rcf_ch_cfg_add)env_add, (rcf_ch_cfg_del)env_del,
      (rcf_ch_cfg_list)env_list, NULL, NULL };

RCF_PCH_CFG_NODE_COLLECTION(node_user, "user",
                            NULL, &node_env,
                            user_add, user_del,
                            user_list, NULL);

RCF_PCH_CFG_NODE_AGENT(node_agent, &node_user);

static te_bool init = FALSE;

#ifdef ENABLE_8021X
/** Grab interface-specific resources */
static te_errno
interface_grab(const char *name)
{
    return supplicant_grab(name);
}

/** Release interface-specific resources */
static te_errno
interface_release(const char *name)
{
    return supplicant_release(name);
}
#endif

/**
 * Get root of the tree of supported objects.
 *
 * @return root pointer
 */
rcf_pch_cfg_object *
rcf_ch_conf_root(void)
{
    if (!init)
    {
#ifdef USE_NETLINK
        struct rtnl_handle rth;
        
        memset(&rth, 0, sizeof(rth));
        if (rtnl_open(&rth, 0) < 0)
        {
            ERROR("Failed to open a netlink socket");
            return NULL;
        }

        ll_init_map(&rth);
        rtnl_close(&rth);
#endif

        if ((cfg_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP)) < 0)
        {
            return NULL;
        }
        if (fcntl(cfg_socket, F_SETFD, FD_CLOEXEC) != 0)
        {
            ERROR("Failed to set close-on-exec flag on configuration "
                  "socket: %r", errno);
        }

        init = TRUE;

#ifdef ENABLE_8021X
        rcf_pch_rsrc_info("/agent/interface", 
                          interface_grab,
                          interface_release);
#else
        rcf_pch_rsrc_info("/agent/interface", 
                          rcf_pch_rsrc_grab_dummy,
                          rcf_pch_rsrc_release_dummy);
#endif

        rcf_pch_rsrc_info("/agent/ip4_fw", 
                          rcf_pch_rsrc_grab_dummy,
                          rcf_pch_rsrc_release_dummy);

#ifdef RCF_RPC
        /* Link RPC nodes */
        rcf_pch_rpc_init();
#endif

#ifdef CFG_UNIX_DAEMONS
        if (ta_unix_conf_daemons_init() != 0)
        {
            close(cfg_socket);
            return NULL;
        }
#endif
#ifdef WITH_ISCSI
        if (ta_unix_iscsi_target_init() != 0)
        {
            close(cfg_socket);
            return NULL;
        }

        if (ta_unix_iscsi_initiator_init() != 0)
        {
            close(cfg_socket);
            return NULL;
        }
#endif        
#ifdef ENABLE_WIFI_SUPPORT
        if (ta_unix_conf_wifi_init() != 0)
            return NULL;
#endif
#ifdef ENABLE_8021X
        if (ta_unix_conf_supplicant_init() != 0)
            return NULL;
#endif

        rcf_pch_rsrc_init();
    }

    return &node_agent;
}

/**
 * Get Test Agent name.
 *
 * @return name pointer
 */
const char *
rcf_ch_conf_agent()
{
    return ta_name;
}

/**
 * Release resources allocated for configuration support.
 */
void
rcf_ch_conf_release()
{
#ifdef CFG_UNIX_DAEMONS
    ta_unix_conf_daemons_release();
#endif
    if (cfg_socket >= 0)
        (void)close(cfg_socket);
}

/**
 * Obtain value of the IPv4 forwarding sustem variable.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         value location
 *
 * @return              Status code
 */
static te_errno
ip4_fw_get(unsigned int gid, const char *oid, char *value)
{
    char c = '0';

    UNUSED(gid);
    UNUSED(oid);

#if __linux__
    {
        int  fd;

        if ((fd = open("/proc/sys/net/ipv4/ip_forward", O_RDONLY)) < 0)
            return TE_OS_RC(TE_TA_UNIX, errno);

        if (read(fd, &c, 1) < 0)
        {
            close(fd);
            return TE_OS_RC(TE_TA_UNIX, errno);
        }

        close(fd);
    }
#endif

    sprintf(value, "%d", c == '0' ? 0 : 1);

    return 0;
}

/**
 * Enable/disable IPv4 forwarding.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         pointer to new value of IPv4 forwarding system
 *                      variable
 *
 * @return              Status code
 */
static te_errno
ip4_fw_set(unsigned int gid, const char *oid, const char *value)
{
    int fd;

    UNUSED(gid);
    UNUSED(oid);
    
    if (!rcf_pch_rsrc_accessible("/agent/ip4_fw"))
        return TE_RC(TE_TA_UNIX, TE_EPERM);

    if ((*value != '0' && *value != '1') || *(value + 1) != 0)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    fd = open("/proc/sys/net/ipv4/ip_forward",
              O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd < 0)
        return TE_OS_RC(TE_TA_UNIX, errno);

    if (write(fd, *value == '0' ? "0\n" : "1\n", 2) < 0)
    {
        close(fd);
        return TE_OS_RC(TE_TA_UNIX, errno);
    }

    close(fd);

    return 0;
}

#ifdef USE_NETLINK
/**
 * Store answer from RTM_GETXXX in nlmsg list.
 *
 * @param who           address pointer
 * @param msg           address info to be stored
 * @param arg           location for nlmsg list
 *
 * @retval 0            success
 * @retval -1           failure
 */
static int
store_nlmsg(const struct sockaddr_nl *who,
            const struct nlmsghdr *msg,
            void *arg)
{
    struct nlmsg_list **linfo = (struct nlmsg_list **)arg;
    struct nlmsg_list  *h;
    struct nlmsg_list **lp;

    h = malloc(msg->nlmsg_len + sizeof(void *));
    if (h == NULL)
        return -1;

    memcpy(&h->h, msg, msg->nlmsg_len);
    h->next = NULL;

    for (lp = linfo; *lp != NULL; lp = &(*lp)->next);

    *lp = h;

    ll_remember_index(who, msg, NULL);

    return 0;
}

/**
 * Free nlmsg list.
 *
 * @param linfo   nlmsg list to be freed
 */
static void
free_nlmsg(struct nlmsg_list *linfo)
{
    struct nlmsg_list *next;
    struct nlmsg_list *cur;

    for (cur = linfo; cur != NULL; cur = next)
    {
        next = cur->next;
        free(cur);
    }
    return;
}

/**
 * Get link/protocol addresses information
 *
 * @param family   AF_INET, AF_INET6
 * @param list     location for nlmsg list
 *                 containing address information
 *
 * @return         Status code
 */
static te_errno
ip_addr_get(int family, struct nlmsg_list **list)
{
    struct rtnl_handle rth;

    if (family != AF_INET && family != AF_INET6)
    {
        ERROR("%s: invalid address family (%d)", __FUNCTION__, family);
        return TE_OS_RC(TE_TA_UNIX, TE_EINVAL);
    }

    memset(&rth, 0, sizeof(rth));
    if (rtnl_open(&rth, 0) < 0)
    {
        ERROR("%s: rtnl_open() failed, %s",
              __FUNCTION__, strerror(errno));
        return TE_OS_RC(TE_TA_UNIX, errno);
    }

    ll_init_map(&rth);

    if (rtnl_wilddump_request(&rth, family, RTM_GETADDR) < 0)
    {
        ERROR("%s: Cannot send dump request, %s",
              __FUNCTION__, strerror(errno));
        rtnl_close(&rth);
        return TE_OS_RC(TE_TA_UNIX, errno);
    }

    if (rtnl_dump_filter(&rth, store_nlmsg, list, NULL, NULL) < 0)
    {
        ERROR("%s: Dump terminated, %s",
              __FUNCTION__, strerror(errno));
        rtnl_close(&rth);
        return TE_OS_RC(TE_TA_UNIX, errno);
    }
    rtnl_close(&rth);
    return 0;
}

/**
 * Find name of the interface with specified address and retrieve
 * attributes of the address.
 *
 * @param str_addr  Address in dotted notation
 * @param ifname    Name the interface or NULL
 * @param addr      Location for the address or NULL
 * @param prefix    Location for prefix or NULL
 * @param bcast     Location for broadcast address or NULL
 *
 * @return Pointer to interface name in ll_map static buffer or NULL.
 */
static const char *
nl_find_net_addr(const char *str_addr, const char *ifname,
                 gen_ip_address *addr, unsigned int *prefix,
                 gen_ip_address *bcast)
{
    gen_ip_address     ip_addr;
    
    struct nlmsg_list *ainfo = NULL;
    struct nlmsg_list *a = NULL;
    struct nlmsghdr   *n = NULL;
    struct ifaddrmsg  *ifa = NULL;
    struct rtattr     *rta_tb[IFA_MAX + 1];
    int                ifindex = 0;
    int                family;

    /* If address contains a colon, it is IPv6 address */
    family = (strchr(str_addr, ':') == NULL) ? AF_INET : AF_INET6;

    if (bcast != NULL)
        memset(bcast, 0, sizeof(*bcast));
    
    if (ifname != NULL && (strlen(ifname) >= IF_NAMESIZE))
    {
        ERROR("Interface name '%s' too long", ifname);
        return NULL;
    }

    if (inet_pton(family, str_addr, &ip_addr) <= 0)
    {
        ERROR("%s(): inet_pton() failed for address '%s'",
              __FUNCTION__, str_addr);
        return NULL;
    }
       
    if (ip_addr_get(family, &ainfo) != 0)
    {
        ERROR("%s(): Cannot get addresses list", __FUNCTION__);
        return NULL;
    }

    for (a = ainfo; a != NULL; a = a->next)
    {
        n = &a->h;
        ifa = NLMSG_DATA(n);

        if (n->nlmsg_len < NLMSG_LENGTH(sizeof(ifa)))
        {
            ERROR("%s(): Bad netlink message header length", __FUNCTION__);
            free_nlmsg(ainfo);
            return NULL;
        }

        memset(rta_tb, 0, sizeof(rta_tb));
        parse_rtattr(rta_tb, IFA_MAX, IFA_RTA(ifa),
                     n->nlmsg_len - NLMSG_LENGTH(sizeof(*ifa)));
        if (!rta_tb[IFA_LOCAL])
            rta_tb[IFA_LOCAL] = rta_tb[IFA_ADDRESS];
        if (!rta_tb[IFA_ADDRESS])
             rta_tb[IFA_ADDRESS] = rta_tb[IFA_LOCAL];
        if (rta_tb[IFA_LOCAL])
        {            
            if (((family == AF_INET) &&
                 (*(uint32_t *)(RTA_DATA(rta_tb[IFA_LOCAL])) ==
                 ip_addr.ip4_addr.s_addr)) || 
                 ((family == AF_INET6) &&
                  (memcmp(RTA_DATA(rta_tb[IFA_LOCAL]),
                         &ip_addr.ip6_addr, sizeof(struct in6_addr)) == 0)))
            {
                if (ifname == NULL ||
                    (ll_name_to_index(ifname) == ifa->ifa_index))
                    break;

                WARN("Interfaces '%s' and '%s' have the same address '%s'",
                     ifname, ll_index_to_name(ifa->ifa_index), str_addr);
            }
        }
    }
    /* If address was obtained, write it to the given locations */
    if (a != NULL)
    {
        if (prefix != NULL)
            *prefix = ifa->ifa_prefixlen;
        ifindex = ifa->ifa_index;
        if (family == AF_INET)
        {
            if (addr != NULL)
                addr->ip4_addr = ip_addr.ip4_addr;
            if (bcast != NULL)
            {
                bcast->ip4_addr.s_addr = (rta_tb[IFA_BROADCAST]) ?
                    *(uint32_t *)RTA_DATA(rta_tb[IFA_BROADCAST]) :
                    htonl(INADDR_BROADCAST);
            }
        }
        else
        {
            if (addr != NULL)
                memcpy(&addr->ip6_addr, &ip_addr.ip6_addr,
                       sizeof(struct in6_addr));
        }
    }

    free_nlmsg(ainfo);

    return (a == NULL) ? NULL :
           (ifname != NULL) ? ifname :
                              (const char *)ll_index_to_name(ifindex);
}


/**
 * Add/delete AF_INET/AF_INET6 address.
 *
 * @param cmd           Command (see enum net_addr_ops)
 * @param ifname        Interface name 
 * @param family        Address family (AF_INET or AF_INET6)
 * @param addr          Address
 * @param prefix        Prefix to set or 0
 * @param bcast         Broadcast to set or 0
 *
 * @return              Status code
 */
static te_errno
nl_ip_addr_add_del(int cmd, const char *ifname,
                   int family, gen_ip_address *addr,
                   unsigned int prefix, gen_ip_address *bcast)
{
    te_errno            rc;
    struct rtnl_handle  rth;
    struct {
        struct nlmsghdr  n;
        struct ifaddrmsg ifa;
        char             buf[256];
    } req;

    inet_prefix  lcl;
    inet_prefix  brd;

    char         addrstr[INET6_ADDRSTRLEN];

#define AF_INET_DEFAULT_BYTELEN  (sizeof(struct in_addr))
#define AF_INET_DEFAULT_BITLEN   (AF_INET_DEFAULT_BYTELEN << 3)
#define AF_INET6_DEFAULT_BYTELEN (sizeof(struct in6_addr))
#define AF_INET6_DEFAULT_BITLEN  (AF_INET6_DEFAULT_BYTELEN << 3)

    ENTRY("cmd=%d ifname=%s addr=0x%x prefix=%u bcast=%s",
          cmd, ifname, addr, prefix, bcast == NULL ? "<null>" :
          inet_ntop(family, bcast, addrstr, INET6_ADDRSTRLEN));

    memset(&req, 0, sizeof(req));
    memset(&lcl, 0, sizeof(lcl));
    memset(&brd, 0, sizeof(brd));
    memset(&rth, 0, sizeof(rth));

    lcl.family = family;
    if (family == AF_INET)
    {
        lcl.bytelen = AF_INET_DEFAULT_BYTELEN;
        lcl.bitlen = (prefix != 0) ? prefix : AF_INET_DEFAULT_BITLEN;
    }
    else
    {
        assert(family == AF_INET6);
        lcl.bytelen = AF_INET6_DEFAULT_BYTELEN;
        lcl.bitlen = (prefix != 0) ? prefix : AF_INET6_DEFAULT_BITLEN;
    }
    memcpy(lcl.data, addr, lcl.bytelen);

    req.n.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifaddrmsg));
    req.n.nlmsg_flags = NLM_F_REQUEST;
    req.n.nlmsg_type = cmd;
    req.ifa.ifa_family = family;
    req.ifa.ifa_prefixlen = lcl.bitlen;

    addattr_l(&req.n, sizeof(req), IFA_LOCAL, &lcl.data, lcl.bytelen);

    if (bcast != 0)
    {
        brd.family = family;
        brd.bytelen = lcl.bytelen;
        brd.bitlen = lcl.bitlen;
        memcpy(brd.data, bcast, brd.bytelen);
        addattr_l(&req.n, sizeof(req), IFA_BROADCAST,
                  &brd.data, brd.bytelen);
    }

    memset(&rth, 0, sizeof(rth));
    if (rtnl_open(&rth, 0) < 0)
    {
        rc = TE_OS_RC(TE_TA_UNIX, errno);
        ERROR("%s(): Cannot open netlink socket", __FUNCTION__);
        return rc;
    }

    ll_init_map(&rth);
    req.ifa.ifa_index = ll_name_to_index(ifname);

    if (rtnl_talk(&rth, &req.n, 0, 0, NULL, NULL, NULL) < 0)
    {
        rc = TE_OS_RC(TE_TA_UNIX, errno);
        ERROR("%s(): rtnl_talk() failed", __FUNCTION__);
        rtnl_close(&rth);
        return rc;
    }
    rtnl_close(&rth);

    EXIT("OK");

    return 0;
}


/** Operations over network addresses */
enum net_addr_ops {
    NET_ADDR_ADD,       /**< Add a new address */
    NET_ADDR_DELETE,    /**< Delete an existing address */
    NET_ADDR_MODIFY     /**< Modify an existing address */
};

/**
 * Modify AF_INET or AF_INET6 address.
 *
 * @param cmd           Command (see enum net_addr_ops)
 * @param ifname        Interface name
 * @param addr          Address as string
 * @param new_prefix    Pointer to the prefix to set or NULL
 * @param new_bcast     Pointer to the broadcast address to set or NULL
 *
 * @return              Status code
 */
static te_errno
nl_ip_addr_modify(enum net_addr_ops cmd,
                   const char *ifname, const char *addr,
                   unsigned int *new_prefix, gen_ip_address *new_bcast)
{
    unsigned int    prefix = 0;
    gen_ip_address  bcast;
    te_errno        rc = 0;
    int             family;
    gen_ip_address  ip_addr;

    /* If address contains ';', it is IPv6 address */
    family = (strchr(addr, ':') == NULL) ? AF_INET : AF_INET6;

    if (cmd == NET_ADDR_ADD)
    {
        if (inet_pton(family, addr, &ip_addr) <= 0)
        {
            ERROR("Failed to convert address '%s' from string", addr);
            return TE_RC(TE_TA_UNIX, TE_EINVAL);
        }
    }
    else
    {
        if (nl_find_net_addr(addr, ifname, &ip_addr,
                             &prefix, &bcast) == NULL)
        {
            ERROR("Address '%s' on interface '%s' not found", addr, ifname);
            return TE_RC(TE_TA_UNIX, TE_ENOENT);
        }
    }

    if (new_prefix != NULL)
        prefix = *new_prefix;
    /* Broadcast is supported in IPv4 only */    
    if ((family == AF_INET) && (new_bcast != NULL))
        bcast = *new_bcast;

    if (cmd != NET_ADDR_ADD)
    {
        rc = nl_ip_addr_add_del(RTM_DELADDR, ifname, family,
                                &ip_addr, 0, 0);
    }

    if (rc == 0 && cmd != NET_ADDR_DELETE)
    {
        rc = nl_ip_addr_add_del(RTM_NEWADDR, ifname, family,
                                &ip_addr, prefix, &bcast);
    }

    return rc;
}

#endif /* USE_NETLINK */


#ifdef USE_IOCTL
/**
 * Get IPv4 address of the network interface using ioctl.
 *
 *
 * @param ifname        interface name (like "eth0")
 * @param addr          location for the address (address is returned in
 *                      network byte order)
 *
 * @return OS errno
 */
static te_errno
get_addr(const char *ifname, struct in_addr *addr)
{
    strcpy(req.ifr_name, ifname);
    if (ioctl(cfg_socket, SIOCGIFADDR, (int)&req) < 0)
    {
        te_errno rc = TE_OS_RC(TE_TA_UNIX, errno);
        
        /* It's not always called for correct arguments */
        VERB("ioctl(SIOCGIFADDR) for '%s' failed: %r",
              ifname, rc);
        return rc;
    }
    *addr = SIN(&(req.ifr_addr))->sin_addr;
    return 0;
}


/* Check, if one interface is alias of other interface */
static te_bool
is_alias_of(const char *candidate, const char *master)
{
    char *tmp = strchr(candidate, ':');
    int   len = strlen(master);

    return !(tmp == NULL || tmp - candidate != len ||
             strncmp(master, candidate, len) != 0);
}

/**
 * Update IPv4 prefix length of the interface using ioctl.
 *
 * @param ifname        Interface name (like "eth0")
 * @param prefix        Prefix length
 *
 * @return OS errno
 */
static te_errno
set_prefix(const char *ifname, unsigned int prefix)
{
    in_addr_t mask = PREFIX2MASK(prefix);

    memset(&req, 0, sizeof(req));

    strcpy(req.ifr_name, ifname);

    req.ifr_addr.sa_family = AF_INET;
    SIN(&(req.ifr_addr))->sin_addr.s_addr = htonl(mask);
    if (ioctl(cfg_socket, SIOCSIFNETMASK, &req) < 0)
    {
        te_errno rc = TE_OS_RC(TE_TA_UNIX, errno);
        
        ERROR("ioctl(SIOCSIFNETMASK) failed: %r", rc);
        return rc;
    }
    return 0;
}
#endif

/**
 * Check, if the interface with specified name exists.
 *
 * @param name          interface name
 *
 * @return 0     if the interface exists,
 *         or error code otherwise
 */
static te_errno
interface_exists(const char *ifname)
{
    FILE *f;

    if ((f = fopen("/proc/net/dev", "r")) == NULL)
    {
        ERROR("%s(): Failed to open /proc/net/dev for reading: %s",
              __FUNCTION__, strerror(errno));
        return TE_OS_RC(TE_TA_UNIX, errno);
    }

    buf[0] = 0;

    while (fgets(trash, sizeof(trash), f) != NULL)
    {
        char *s = strchr(trash, ':');

        if (s == NULL)
            continue;

        for (*s-- = 0; s != trash && *s != ' '; s--);

        if (*s == ' ')
            s++;

        if (strcmp(s, ifname) == 0)
        {
            fclose(f);
            return 0;
        }
    }

    fclose(f);

    return TE_OS_RC(TE_TA_UNIX, TE_ENOENT);
}


/**
 * Get instance list for object "agent/interface".
 *
 * @param gid           group identifier (unused)
 * @param oid           full identifier of the father instance
 * @param list          location for the list pointer
 *
 * @return              Status code
 * @retval 0            success
 * @retval TE_ENOMEM       cannot allocate memory
 */
static te_errno
interface_list(unsigned int gid, const char *oid, char **list)
{
    size_t off = 0;

    UNUSED(gid);
    UNUSED(oid);

    ENTRY("gid=%u oid='%s'", gid, oid);

    buf[0] = '\0';

#ifdef __linux__
    {
        FILE *f;

        if ((f = fopen("/proc/net/dev", "r")) == NULL)
        {
            ERROR("%s(): Failed to open /proc/net/dev for reading: %s",
                  __FUNCTION__, strerror(errno));
            return TE_OS_RC(TE_TA_UNIX, errno);
        }

        while (fgets(trash, sizeof(trash), f) != NULL)
        {
            char *s = strchr(trash, ':');

            if (s == NULL)
                continue;

            for (*s-- = 0; s != trash && *s != ' '; s--);

            if (*s == ' ')
                s++;
                
            if (CHECK_INTERFACE(s) != 0)
                continue;

            off += snprintf(buf + off, sizeof(buf) - off, "%s ", s);
        }

        fclose(f);
    }
#else
    {
        /*
         * This branch does not show interfaces in down state, be
         * carefull.
         */
        struct if_nameindex *ifs = if_nameindex();
        struct if_nameindex *p;

        if (ifs != NULL)
        {
            for (p = ifs; (p->if_name != NULL) && (off < sizeof(buf)); ++p)
            {
                if (CHECK_INTERFACE(p->if_name) != 0)
                    continue;

                off += snprintf(buf + off, sizeof(buf) - off,
                                "%s ", p->if_name);
            }

            if_freenameindex(ifs);
        }
    }
#endif
    if (off >= sizeof(buf))
        return TE_RC(TE_TA_UNIX, TE_ESMALLBUF);

    if ((*list = strdup(buf)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    EXIT("list='%s'", *list);

    return 0;
}

#ifdef USE_IOCTL
/** List both devices and interfaces */
static int
aliases_list()
{
    struct ifconf conf;
    struct ifreq *req;
    te_bool       first = TRUE;
    char         *name = NULL;
    char         *ptr = buf;

    conf.ifc_len = sizeof(buf);
    conf.ifc_buf = buf;

    memset(buf, 0, sizeof(buf));
    if (ioctl(cfg_socket, SIOCGIFCONF, &conf) < 0)
    {
        te_errno rc = TE_OS_RC(TE_TA_UNIX, errno);
        
        ERROR("ioctl(SIOCGIFCONF) failed: %r", rc);
        return rc;
    }

    for (req = conf.ifc_req; *(req->ifr_name) != 0; req++)
    {
        if (name != NULL && strcmp(req->ifr_name, name) == 0)
            continue;

        name = req->ifr_name;

        if (first)
        {
            buf[0] = 0;
            first = FALSE;
        }
        ptr += sprintf(ptr, "%s ", name);
    }

#ifdef __linux__
    {
        FILE         *f;

        if ((f = fopen("/proc/net/dev", "r")) == NULL)
        {
            ERROR("%s(): Failed to open /proc/net/dev for reading: %s",
                  __FUNCTION__, strerror(errno));
            return TE_OS_RC(TE_TA_UNIX, errno);
        }

        while (fgets(trash, sizeof(trash), f) != NULL)
        {
            char *name = strchr(trash, ':');
            char *tmp;
            int   n;

            if (name == NULL)
                continue;

            for (*name-- = 0; name != trash && *name != ' '; name--);

            if (*name == ' ')
                name++;

            n = strlen(name);
            for (tmp = strstr(buf, name);
                 tmp != NULL;
                 tmp = strstr(tmp, name))
            {
                tmp += n;
                if (*tmp == ' ')
                    break;
            }

            if (tmp == NULL)
                ptr += sprintf(ptr, "%s ", name);
        }

        fclose(f);
    }
#endif

    return 0;
}
#endif

/**
 * Add VLAN Ethernet device.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         value string (unused)
 * @param ifname        VLAN device name: ethX.VID
 *
 * @return              Status code
 */
static te_errno
interface_add(unsigned int gid, const char *oid, const char *value,
              const char *ifname)
{
    char    *devname;
    char    *vlan;
    char    *tmp;
    uint16_t vid;
    te_errno rc;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);

    if ((devname = strdup(ifname)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    if ((rc = CHECK_INTERFACE(devname)) != 0)
    {
        return TE_RC(TE_TA_UNIX, rc);
    }

    if (interface_exists(ifname))
        return TE_RC(TE_TA_UNIX, TE_EEXIST);

    if ((vlan = strchr(devname, '.')) == NULL)
    {
        free(devname);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }
    *vlan++ = 0;
    
    vid = strtol(vlan, &tmp, 10);
    if (tmp == vlan || *tmp != 0 || !interface_exists(devname))
    {
        free(devname);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    sprintf(buf, "/sbin/vconfig add %s %d", devname, vid);
    free(devname);

    return ta_system(buf) != 0 ? TE_RC(TE_TA_UNIX, TE_ESHCMD) : 0;
}

/**
 * Delete VLAN Ethernet device.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param ifname        VLAN device name: ethX.VID
 *
 * @return              Status code
 */
static te_errno
interface_del(unsigned int gid, const char *oid, const char *ifname)
{
    char *devname;
    char *vlan;
    
    te_errno    rc;
    
    UNUSED(gid);
    UNUSED(oid);

    if (!interface_exists(ifname))
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if ((devname = strdup(ifname)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    if ((vlan = strchr(devname, '.')) == NULL)
    {
        free(devname);
        return TE_RC(TE_TA_UNIX, TE_EPERM);
    }
    *vlan = 0;

    if ((rc = CHECK_INTERFACE(devname)) != 0)
    {
        free(devname);
        return TE_RC(TE_TA_UNIX, rc);
    }
    free(devname);

    sprintf(buf, "/sbin/vconfig rem %s", ifname);

    return (ta_system(buf) != 0) ? TE_RC(TE_TA_UNIX, TE_ESHCMD) : 0;
}


/**
 * Get index of the interface.
 *
 * @param gid           request group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         location for interface index
 * @param ifname        name of the interface (like "eth0")
 *
 * @return              Status code
 */
static te_errno
ifindex_get(unsigned int gid, const char *oid, char *value,
            const char *ifname)
{
    unsigned int ifindex = if_nametoindex(ifname);;
    te_errno     rc;
    
    UNUSED(gid);
    UNUSED(oid);

    if ((rc = CHECK_INTERFACE(ifname)) != 0)
        return TE_RC(TE_TA_UNIX, rc);

    if (ifindex == 0)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    sprintf(value, "%u", ifindex);

    return 0;
}

/**
 * Configure IPv4 address for the interface.
 * If the address does not exist, alias interface is created.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         value string (unused)
 * @param ifname        name of the interface (like "eth0")
 * @param addr          IPv4 address in dotted notation
 *
 * @return              Status code
 */
#ifdef USE_IOCTL
#ifdef USE_IFCONFIG
static te_errno
net_addr_add(unsigned int gid, const char *oid, const char *value,
             const char *ifname, const char *addr)
{
    unsigned int  new_addr;
    unsigned int  tmp_addr;
    te_errno      rc;
    char         *cur;
    char         *next;
    char          slots[32] = { 0, };

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);

    if (strlen(ifname) >= IF_NAMESIZE)
        return TE_RC(TE_TA_UNIX, TE_E2BIG);

    if ((rc = CHECK_INTERFACE(ifname)) != 0)
        return TE_RC(TE_TA_UNIX, rc);

    if (inet_pton(AF_INET, addr, (void *)&new_addr) <= 0 ||
        new_addr == 0 ||
        (new_addr & 0xe0000000) == 0xe0000000)
    {
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    if ((rc = aliases_list()) != 0)
        return rc;

    cur = buf;
    while (cur != NULL)
    {
        next = strchr(cur, ' ');
        if (next != NULL)
            *next++ = 0;

        rc = get_addr(cur, (struct in_addr *)&tmp_addr);

        if (rc == 0 && tmp_addr == new_addr)
            return TE_RC(TE_TA_UNIX, TE_EEXIST);

        if (strcmp(cur, ifname) == 0)
        {
            if (rc != 0)
                break;
            else
                goto next;

        }

        if (!is_alias_of(cur, ifname))
            goto next;

        if (rc != 0)
            break;

        slots[atoi(strchr(cur, ':') + 1)] = 1;

        next:
        cur = next;
    }

    if (cur != NULL)
    {
        sprintf(trash, "/sbin/ifconfig %s %s up", cur, addr);
    }
    else
    {
        unsigned int n;

        for (n = 0; n < sizeof(slots) && slots[n] != 0; n++);

        if (n == sizeof(slots))
            return TE_RC(TE_TA_UNIX, TE_EPERM);

        sprintf(trash, "/sbin/ifconfig %s:%d %s up", ifname, n, addr);
    }

    if (ta_system(trash) != 0)
        return TE_RC(TE_TA_UNIX, TE_ESHCMD);

    if (*value != 0)
    {
        if ((rc = prefix_set(gid, oid, value, ifname, addr)) != 0)
        {
            net_addr_del(gid, oid, ifname, addr);
            return rc;
        }
    }

    return 0;
}
#else
static te_errno
net_addr_add(unsigned int gid, const char *oid, const char *value,
             const char *ifname, const char *addr)
{
    in_addr_t    new_addr;
    te_errno     rc;
#ifdef __linux__
    char         *cur;
    char         *next;
    char          slots[32] = { 0, };
    struct        sockaddr_in sin;
#endif

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);

    if (strlen(ifname) >= IF_NAMESIZE)
        return TE_RC(TE_TA_UNIX, TE_E2BIG);

    if ((rc = CHECK_INTERFACE(ifname)) != 0)
        return TE_RC(TE_TA_UNIX, rc);

    if (inet_pton(AF_INET, addr, (void *)&new_addr) <= 0 ||
        new_addr == 0 ||
        (ntohl(new_addr) & 0xe0000000) == 0xe0000000)
    {
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

#ifdef __linux__
    if ((rc = aliases_list()) != 0)
        return rc;

    for (cur = buf; strlen(cur) > 0; cur = next)
    {
        unsigned int  tmp_addr;

        next = strchr(cur, ' ');
        if (next != NULL)
        {
            *next++ = '\0';
            if (strlen(cur) == 0)
                continue;
        }

        rc = get_addr(cur, (struct in_addr *)&tmp_addr);
        if (rc == 0 && tmp_addr == new_addr)
            return TE_RC(TE_TA_UNIX, TE_EEXIST);

        if (strcmp(cur, ifname) == 0)
        {
            if (rc != 0)
                break;
            else
                continue;
        }

        if (!is_alias_of(cur, ifname))
            continue;

        if (rc != 0)
            break;

        slots[atoi(strchr(cur, ':') + 1)] = 1;
    }

    if (strlen(cur) != 0)
    {
        strncpy(req.ifr_name, cur, IFNAMSIZ);
    }
    else
    {
        unsigned int n;

        for (n = 0; n < sizeof(slots) && slots[n] != 0; n++);

        if (n == sizeof(slots))
            return TE_RC(TE_TA_UNIX, TE_EPERM);

        sprintf(trash, "%s:%d", ifname, n);
        strncpy(req.ifr_name, trash, IFNAMSIZ);
    }

    memset(&sin, 0, sizeof(struct sockaddr));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = new_addr;
    memcpy(&req.ifr_addr, &sin, sizeof(struct sockaddr));
    if (ioctl(cfg_socket, SIOCSIFADDR, &req) < 0)
    {
        te_errno rc = TE_OS_RC(TE_TA_UNIX, errno);

        ERROR("ioctl(SIOCSIFADDR) failed: %r", rc);
        return rc;
    }
#elif defined(SIOCALIFADDR)
    {
        struct if_laddrreq lreq;

        memset(&lreq, 0, sizeof(lreq));
        strncpy(lreq.iflr_name, ifname, IFNAMSIZ);
        lreq.addr.ss_family = AF_INET;
        lreq.addr.ss_len = sizeof(struct sockaddr_in);
        if (inet_pton(AF_INET, addr, &SIN(&lreq.addr)->sin_addr) <= 0)
        {
            ERROR("inet_pton() failed");
            return TE_RC(TE_TA_UNIX, TE_EFMT);
        }
        if (ioctl(cfg_socket, SIOCALIFADDR, &lreq) < 0)
        {
            te_errno rc = TE_OS_RC(TE_TA_UNIX, errno);

            ERROR("ioctl(SIOCALIFADDR) failed: %r", rc);
            return rc;
        }
    }
#else
    ERROR("%s(): %s", __FUNCTION__, strerror(EOPNOTSUPP));
    return TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP);
#endif

    if (*value != 0)
    {
        if ((rc = prefix_set(gid, oid, value, ifname, addr)) != 0)
        {
            net_addr_del(gid, oid, ifname, addr);
            return rc;
        }
    }

    return 0;
}
#endif
#endif
#ifdef USE_NETLINK
static te_errno
net_addr_add(unsigned int gid, const char *oid, const char *value,
             const char *ifname, const char *addr)
{
    const char     *name;
    unsigned int    prefix;
    char           *end;
    in_addr_t       mask;
    gen_ip_address  broadcast;

    int             family;
    gen_ip_address  ip_addr;
    te_errno        rc;

    UNUSED(gid);
    UNUSED(oid);

    if ((rc = CHECK_INTERFACE(ifname)) != 0)
        return TE_RC(TE_TA_UNIX, rc);

    /* Check that address has not been assigned to any interface yet */
    name = nl_find_net_addr(addr, NULL, &ip_addr, NULL, NULL);
    if (name != NULL)
    {
        ERROR("%s(): Address '%s' already exists on interface '%s'",
              __FUNCTION__, addr, name);
        return TE_RC(TE_TA_UNIX, TE_EEXIST);
    }

    family = (strchr(addr, ':') == NULL) ? AF_INET : AF_INET6;

    /* Validate address to be added */    
    if (inet_pton(family, addr, &ip_addr) <= 0 ||
        ip_addr.ip4_addr.s_addr == 0 ||
        IN_CLASSD(ip_addr.ip4_addr.s_addr) ||
        IN_EXPERIMENTAL(ip_addr.ip4_addr.s_addr))
    {
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    /* Validate specified address prefix */
    prefix = strtol(value, &end, 10);
    if (end == '\0')
    {
        ERROR("Invalid value '%s' of prefix length", value);
        return TE_RC(TE_TA_UNIX, TE_EFMT);
    }
    if (((family == AF_INET) && (prefix > AF_INET_DEFAULT_BITLEN)) ||
       ((family == AF_INET6) && (prefix > AF_INET6_DEFAULT_BITLEN)))
    {
        ERROR("Invalid prefix '%s' to be set", value);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }
    if (family == AF_INET)
    {
        if (prefix == 0)
        {
            /* Use default prefix in the cast of 0 */
            mask = ((ip_addr.ip4_addr.s_addr) & htonl(0x80000000)) == 0 ?
                   htonl(0xFF000000) :
                   ((ip_addr.ip4_addr.s_addr) & htonl(0xC0000000)) ==
                   htonl(0x80000000) ?
                   htonl(0xFFFF0000) : htonl(0xFFFFFF00);
            MASK2PREFIX(ntohl(mask), prefix);
        }
        else
        {
            mask = htonl(PREFIX2MASK(prefix));
        }
        /* Prepare broadcast address to be set */
        broadcast.ip4_addr.s_addr = (~mask) | ip_addr.ip4_addr.s_addr;
    }

    return nl_ip_addr_modify(NET_ADDR_ADD, ifname, addr,
                              &prefix, &broadcast);
}
#endif

#ifdef USE_IOCTL
/**
 * Find name of the interface with specified address.
 *
 * @param ifname    name of "master" (non-alias) interface
 * @param addr      address in dotted notation
 *
 * @return pointer to interface name in buf or NULL
 */
static char *
find_net_addr(const char *ifname, const char *addr)
{
    in_addr_t     int_addr;
    in_addr_t     tmp_addr;
    char         *cur;
    char         *next;
    te_errno      rc;

    if (CHECK_INTERFACE(ifname) != 0)
        return NULL;

    if (inet_pton(AF_INET, addr, (void *)&int_addr) <= 0)
    {
        ERROR("inet_pton() failed for address %s", addr);
        return NULL;
    }
    if ((rc = aliases_list()) != 0)
        return NULL;

    for (cur = buf; strlen(cur) > 0; cur = next)
    {
        next = strchr(cur, ' ');
        if (next != NULL)
        {
            *next++ = 0;
            if (strlen(cur) == 0)
                continue;
        }

        if (strcmp(cur, ifname) != 0 && !is_alias_of(cur, ifname))
        {
            continue;
        }

        if ((get_addr(cur, (struct in_addr *)&tmp_addr) == 0) &&
            (tmp_addr == int_addr))
        {
            return cur;
        }
    }
    return NULL;
}
#endif

/**
 * Clear interface address of the down interface.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param ifname        name of the interface (like "eth0")
 * @param addr          IPv4 address in dotted notation
 *
 * @return              Status code
 */

#ifdef USE_IOCTL
#ifdef USE_IFCONFIG
static te_errno
net_addr_del(unsigned int gid, const char *oid,
             const char *ifname, const char *addr)
{
    char    *name;
    te_errno rc;
  
    UNUSED(gid);
    UNUSED(oid);

    if ((rc = CHECK_INTERFACE(ifname)) != 0)
    {
        /* Alias does not exist from Configurator point of view */
        return TE_RC(TE_TA_UNIX, rc);
    }

    if ((name = find_net_addr(ifname, addr)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (strcmp(name, ifname) == 0)
        sprintf(trash, "/sbin/ifconfig %s 0.0.0.0", ifname);
    else
        sprintf(trash, "/sbin/ifconfig %s down", name);

    return ta_system(trash) != 0 ? TE_RC(TE_TA_UNIX, TE_ESHCMD) : 0;
}
#else
static te_errno
net_addr_del(unsigned int gid, const char *oid,
             const char *ifname, const char *addr)
{
    char              *name;
    struct sockaddr_in sin;
    te_errno           rc;

    UNUSED(gid);
    UNUSED(oid);

    if ((rc = CHECK_INTERFACE(ifname)) != 0)
    {
        /* Alias does not exist from Configurator point of view */
        return TE_RC(TE_TA_UNIX, rc);
    }

    if ((name = find_net_addr(ifname, addr)) == NULL)
    {
        ERROR("Address %s on interface %s not found", addr, ifname);
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }
    if (strcmp(name, ifname) == 0)
    {
        strncpy(req.ifr_name, ifname, IFNAMSIZ);

        memset(&sin, 0, sizeof(struct sockaddr));
        sin.sin_family = AF_INET;
        sin.sin_addr.s_addr = INADDR_ANY;
        memcpy(&req.ifr_addr, &sin, sizeof(struct sockaddr));

        if (ioctl(cfg_socket, SIOCSIFADDR, (int)&req) < 0)
        {
            te_errno rc = TE_OS_RC(TE_TA_UNIX, errno);

            ERROR("ioctl(SIOCSIFADDR) failed: %r", rc);
            return rc;
        }
    }
    else
    {
        strncpy(req.ifr_name, name, IFNAMSIZ);
        if (ioctl(cfg_socket, SIOCGIFFLAGS, &req) < 0)
        {
            te_errno rc = TE_OS_RC(TE_TA_UNIX, errno);

            ERROR("ioctl(SIOCGIFFLAGS) failed: %r", rc);
            return rc;
        }

        strncpy(req.ifr_name, name, IFNAMSIZ);
        req.ifr_flags &= ~(IFF_UP | IFF_RUNNING);
        if (ioctl(cfg_socket, SIOCSIFFLAGS, &req) < 0)
        {
            te_errno rc = TE_OS_RC(TE_TA_UNIX, errno);

            ERROR("ioctl(SIOCSIFFLAGS) failed: %r", rc);
            return rc;
        }
    }
    return 0;
}
#endif
#endif
#ifdef USE_NETLINK
static te_errno
net_addr_del(unsigned int gid, const char *oid,
             const char *ifname, const char *addr)
{
    te_errno rc;
    
    UNUSED(gid);
    UNUSED(oid);

    if ((rc = CHECK_INTERFACE(ifname)) != 0)
        return TE_RC(TE_TA_UNIX, rc);

    return nl_ip_addr_modify(NET_ADDR_DELETE, ifname, addr, NULL, NULL);
}
#endif


#define ADDR_LIST_BULK      (INET6_ADDRSTRLEN * 4)

/**
 * Get instance list for object "agent/interface/net_addr".
 *
 * @param id            full identifier of the father instance
 * @param list          location for the list pointer
 * @param ifname        interface name
 *
 * @return              Status code:
 * @retval 0                success
 * @retval TE_ENOENT        no such instance
 * @retval TE_ENOMEM        cannot allocate memory
 */


#ifdef USE_NETLINK
static te_errno
net_addr_list(unsigned int gid, const char *oid, char **list,
              const char *ifname)
{
    int                len = ADDR_LIST_BULK;
    te_errno           rc;
    struct nlmsg_list *ainfo = NULL;
    struct nlmsg_list *a6info = NULL;
    struct nlmsg_list *n = NULL;
    int                ifindex;
    int                inet6_addresses = 0;

    UNUSED(gid);
    UNUSED(oid);
    
    if ((rc = CHECK_INTERFACE(ifname)) != 0)
    {
        /* Alias does not exist from Configurator point of view */
        return TE_RC(TE_TA_UNIX, rc);
    }

    *list = (char *)calloc(ADDR_LIST_BULK, 1);
    if (*list == NULL)
    {
        ERROR("calloc() failed");
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }
    if ((rc = ip_addr_get(AF_INET, &ainfo)) != 0)
    {
        ERROR("%s: ip_addr_get() for IPv4 failed", __FUNCTION__);
        return rc;
    }

    if ((rc = ip_addr_get(AF_INET6, &a6info)) != 0)
    {
        ERROR("%s: ip_addr_get() for IPv6 failed", __FUNCTION__);
        return rc;
    }

    /* Join lists of IPv4 and IPv6 addresses */
    if (ainfo == NULL)
    {
        ainfo = a6info;
    }
    else
    {
        for (n = ainfo; n->next != NULL; n = n->next);

        n->next = a6info;
    }
    
    ifindex = ll_name_to_index(ifname);
    if (ifindex <= 0)
    {
        ERROR("Device \"%s\" does not exist", ifname);
        return TE_RC(TE_TA_UNIX, TE_ENODEV);
    }

    for (n = ainfo; n; n = n->next)
    {
        struct nlmsghdr *hdr = &n->h;
        struct ifaddrmsg *ifa = NLMSG_DATA(hdr);
        struct rtattr * rta_tb[IFA_MAX+1];

        /* IPv4 addresses are all printed, start printing IPv6 addresses */
        if (n == a6info)
        {
            inet6_addresses = 1;
        }            

        if (hdr->nlmsg_len < NLMSG_LENGTH(sizeof(ifa)))
        {
            ERROR("%s: bad netlink message hdr length");
            return TE_RC(TE_TA_UNIX, TE_EINVAL);
        }
        if (ifa->ifa_index != ifindex)
            continue;

        memset(rta_tb, 0, sizeof(rta_tb));
        parse_rtattr(rta_tb, IFA_MAX, IFA_RTA(ifa),
                     hdr->nlmsg_len - NLMSG_LENGTH(sizeof(*ifa)));

        if (!rta_tb[IFA_LOCAL])
            rta_tb[IFA_LOCAL] = rta_tb[IFA_ADDRESS];
        if (!rta_tb[IFA_ADDRESS])
            rta_tb[IFA_ADDRESS] = rta_tb[IFA_LOCAL];

        if (len - strlen(*list) <= INET6_ADDRSTRLEN)
        {
            char *tmp;

            len += ADDR_LIST_BULK;
            tmp = (char *)realloc(*list, len);
            if (tmp == NULL)
            {
                free(*list);
                ERROR("realloc() failed");
                return TE_RC(TE_TA_UNIX, TE_ENOMEM);
            }
            *list = tmp;
        }

        if (!inet6_addresses)
        {
             sprintf(*list + strlen(*list), "%d.%d.%d.%d ",
                     ((unsigned char *)RTA_DATA(rta_tb[IFA_LOCAL]))[0],
                     ((unsigned char *)RTA_DATA(rta_tb[IFA_LOCAL]))[1],
                     ((unsigned char *)RTA_DATA(rta_tb[IFA_LOCAL]))[2],
                     ((unsigned char *)RTA_DATA(rta_tb[IFA_LOCAL]))[3]);
        }
        else
        {
            int i;
            int zeroes_printed = 0;

           /* Print IPv6 address */
            for (i = 0; i < 8; i++)
            {
                /*
                 * zeroes_printed is equal to 1 while zeroes are skipped
                 * and replaced with '::'
                 */
                if (((uint16_t *)RTA_DATA(rta_tb[IFA_LOCAL]))[i] == 0)
                {
                    if (zeroes_printed != 2)
                    {
                        zeroes_printed = 1;
                        /*
                         * If zero sequence starts from the beginning,
                         * print a colon 
                         */
                        if (i == 0)
                        {
                            sprintf(*list + strlen(*list), ":");
                        }                            
                        continue;
                    }
                }
                else if (zeroes_printed == 1)
                {
                    zeroes_printed = 2;
                    sprintf(*list + strlen(*list), ":");
                }       

                sprintf(*list + strlen(*list), "%x",
                        ntohs(((uint16_t *)
                        RTA_DATA(rta_tb[IFA_LOCAL]))[i]));
                /* 
                 * Print a colon after each 4 hexadecimal digits
                 * except the last ones 
                 */
                if (i < 7)
                {
                    sprintf(*list + strlen(*list), ":");
                }
            }
            sprintf(*list + strlen(*list), " ");
        }
    }
    free_nlmsg(ainfo);
    return 0;
}
#endif
#ifdef USE_IOCTL
static te_errno
net_addr_list(unsigned int gid, const char *oid, char **list,
              const char *ifname)
{
    struct ifconf conf;
    struct ifreq *req;
    char         *name = NULL;
    in_addr_t     tmp_addr;
    int           len = ADDR_LIST_BULK;
    te_errno      rc;

    UNUSED(gid);
    UNUSED(oid);

    if ((rc = CHECK_INTERFACE(ifname)) != 0)
    {
        return TE_RC(TE_TA_UNIX, rc);
    }
    conf.ifc_len = sizeof(buf);
    conf.ifc_buf = buf;

    memset(buf, 0, sizeof(buf));
    if (ioctl(cfg_socket, SIOCGIFCONF, &conf) < 0)
    {
        te_errno rc = TE_OS_RC(TE_TA_UNIX, errno);

        ERROR("ioctl(SIOCGIFCONF) failed: %r", rc);
        return rc;
    }
    *list = (char *)calloc(ADDR_LIST_BULK, 1);
    if (*list == NULL)
    {
        ERROR("calloc() failed");
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }
    for (req = conf.ifc_req; strlen(req->ifr_name) != 0; req++)
    {
        if (name != NULL && strcmp(req->ifr_name, name) == 0)
            continue;

        name = req->ifr_name;

        if (strcmp(name, ifname) != 0 && !is_alias_of(name, ifname))
            continue;

        if (get_addr(name, (struct in_addr *)&tmp_addr) != 0)
            continue;

        if (len - strlen(*list) <= INET_ADDRSTRLEN)
        {
            char *tmp;

            len += ADDR_LIST_BULK;
            tmp = (char *)realloc(*list, len);
            if (tmp == NULL)
            {
                free(*list);
                ERROR("realloc() failed");
                return TE_RC(TE_TA_UNIX, TE_ENOMEM);
            }
            *list = tmp;
        }
        sprintf(*list + strlen(*list), "%d.%d.%d.%d ",
                ((unsigned char *)&tmp_addr)[0],
                ((unsigned char *)&tmp_addr)[1],
                ((unsigned char *)&tmp_addr)[2],
                ((unsigned char *)&tmp_addr)[3]);
   }
    return 0;
}
#endif


/**
 * Get prefix of the interface.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         prefix location (prefix is presented in dotted
 *                      notation)
 * @param ifname        name of the interface (like "eth0")
 * @param addr          IPv4 address in dotted notation
 *
 * @return              Status code
 */
static te_errno
prefix_get(unsigned int gid, const char *oid, char *value,
            const char *ifname, const char *addr)
{
    unsigned int prefix = 0;

    UNUSED(gid);
    UNUSED(oid);

#if defined(USE_NETLINK)
    if (nl_find_net_addr(addr, ifname, NULL, &prefix, NULL) == NULL)
    {
        ERROR("Address '%s' on interface '%s' to get prefix not found",
              addr, ifname);
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }
#elif defined(USE_IOCTL)
    strncpy(req.ifr_name, ifname, sizeof(req.ifr_name));
    if (inet_pton(AF_INET, addr, &SIN(&req.ifr_addr)->sin_addr) <= 0)
    {
        ERROR("inet_pton() failed");
        return TE_RC(TE_TA_UNIX, TE_EFMT);
    }
    if (ioctl(cfg_socket, SIOCGIFNETMASK, &req) < 0)
    {
        te_errno rc = TE_OS_RC(TE_TA_UNIX, errno);

        ERROR("ioctl(SIOCGIFNETMASK) failed for if=%s addr=%s: %r",
              ifname, addr, rc);
        return rc;
    }
    MASK2PREFIX(ntohl(SIN(&req.ifr_addr)->sin_addr.s_addr), prefix);
#else
#error Way to work with network addresses is not defined.
#endif

    sprintf(value, "%u", prefix);

    return 0;
}

/**
 * Change prefix of the interface.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         pointer to the new network mask in dotted notation
 * @param ifname        name of the interface (like "eth0")
 * @param addr          IPv4 address in dotted notation
 *
 * @return              Status code
 */
static te_errno
prefix_set(unsigned int gid, const char *oid, const char *value,
           const char *ifname, const char *addr)
{
    unsigned int    prefix;
    char           *end;

    UNUSED(gid);
    UNUSED(oid);

    prefix = strtoul(value, &end, 10);
    if (value == end)
    {
        ERROR("Invalid value '%s' of prefix length", value);
        return TE_RC(TE_TA_UNIX, TE_EFMT);
    }
    if ((strchr(addr, ':') == NULL && prefix > 32) || prefix > 128)
    {
        ERROR("Invalid prefix '%s' to be set", value);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

#if defined(USE_NETLINK)
    return nl_ip_addr_modify(NET_ADDR_MODIFY, ifname, addr, &prefix, NULL);
#elif defined(USE_IOCTL)
    {
        const char *name;

        if ((name = find_net_addr(ifname, addr)) == NULL)
        {
            ERROR("Address '%s' on interface '%s' to set prefix not found",
                  addr, ifname);
            return TE_RC(TE_TA_UNIX, TE_ENOENT);
        }
        return set_prefix(name, prefix);
    }
#else
#error Way to work with network addresses is not defined.
#endif
}


/**
 * Get broadcast of the interface.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         broadcast address location (in dotted notation)
 * @param ifname        name of the interface (like "eth0")
 * @param addr          IP address in human notation
 *
 * @return              Status code
 */
static te_errno
broadcast_get(unsigned int gid, const char *oid, char *value,
            const char *ifname, const char *addr)
{
    gen_ip_address bcast;
    
    int family = (strchr(addr, ':') == NULL) ? AF_INET : AF_INET6;

    UNUSED(gid);
    UNUSED(oid);

    memset(&bcast, 0, sizeof(bcast));

#if defined(USE_NETLINK)
    if (nl_find_net_addr(addr, ifname, NULL, NULL, &bcast) == NULL)
    {
        ERROR("Address '%s' on interface '%s' to get broadcast address "
              "not found", addr, ifname);
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }
#elif defined(USE_IOCTL)
    strncpy(req.ifr_name, ifname, sizeof(req.ifr_name));
    if (inet_pton(AF_INET, addr, &SIN(&req.ifr_addr)->sin_addr) <= 0)
    {
        ERROR("inet_pton() failed");
        return TE_RC(TE_TA_UNIX, TE_EFMT);
    }
    if (ioctl(cfg_socket, SIOCGIFBRDADDR, &req) < 0)
    {
        te_errno rc = TE_OS_RC(TE_TA_UNIX, errno);

        ERROR("ioctl(SIOCGIFBRDADDR) failed for if=%s addr=%s: %r",
              ifname, addr, rc);
        return rc;
    }
    bcast.ip4_addr.s_addr = SIN(&req.ifr_addr)->sin_addr.s_addr;
#else
#error Way to work with network addresses is not defined.
#endif

    if (inet_ntop(family, &bcast, value, RCF_MAX_VAL) == NULL)
    {
        ERROR("inet_ntop() failed");
        return TE_OS_RC(TE_TA_UNIX, errno);
    }

    return 0;
}

/**
 * Change broadcast of the interface.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         pointer to the new broadcast address in dotted
 *                      notation
 * @param ifname        name of the interface (like "eth0")
 * @param addr          IP address in human notation
 *
 * @return              Status code
 */
static te_errno
broadcast_set(unsigned int gid, const char *oid, const char *value,
            const char *ifname, const char *addr)
{
    gen_ip_address bcast;
    
    int family = (strchr(addr, ':') == NULL) ? AF_INET : AF_INET6;

    UNUSED(gid);
    UNUSED(oid);

    if (inet_pton(family, value, &bcast) <= 0 ||
        ((family == AF_INET) && (bcast.ip4_addr.s_addr == 0 ||
        (ntohl(bcast.ip4_addr.s_addr) & 0xe0000000) == 0xe0000000)))
    {
        ERROR("%s(): Invalid broadcast %s", __FUNCTION__, value);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

#if defined(USE_NETLINK)
    return nl_ip_addr_modify(NET_ADDR_MODIFY, ifname, addr, NULL, &bcast);
#elif defined(USE_IOCTL)
    {
        const char *name;

        if ((name = find_net_addr(ifname, addr)) == NULL)
        {
            ERROR("Address '%s' on interface '%s' to set broadcast "
                  "not found", addr, ifname);
            return TE_RC(TE_TA_UNIX, TE_ENOENT);
        }

        strcpy(req.ifr_name, name);
        req.ifr_addr.sa_family = AF_INET;
        SIN(&(req.ifr_addr))->sin_addr = bcast.ip4_addr;
        if (ioctl(cfg_socket, SIOCSIFBRDADDR, (int)&req) < 0)
        {
            te_errno rc = TE_OS_RC(TE_TA_UNIX, errno);

            ERROR("ioctl(SIOCSIFBRDADDR) failed: %s", rc);
            return rc;
        }
        return 0;
    }
#else
#error Way to work with network addresses is not defined.
#endif
}


/**
 * Get hardware address of the interface.
 * Only MAC addresses are supported now.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         location for hardware address (address is returned
 *                      as XX:XX:XX:XX:XX:XX)
 * @param ifname        name of the interface (like "eth0")
 *
 * @return              Status code
 */
static te_errno
link_addr_get(unsigned int gid, const char *oid, char *value,
              const char *ifname)
{
    uint8_t *ptr = NULL;
    te_errno rc;

    UNUSED(gid);
    UNUSED(oid);

    if ((rc = CHECK_INTERFACE(ifname)) != 0)
        return TE_RC(TE_TA_UNIX, rc);

#ifdef SIOCGIFHWADDR
    strcpy(req.ifr_name, ifname);
    if (ioctl(cfg_socket, SIOCGIFHWADDR, (int)&req) < 0)
    {
        te_errno rc = TE_OS_RC(TE_TA_UNIX, errno);
    
        ERROR("ioctl(SIOCGIFHWADDR) failed: %r", rc);
        return rc;
    }

    ptr = req.ifr_hwaddr.sa_data;

#elif defined(__FreeBSD__)

    struct ifconf  ifc;
    struct ifreq  *p;

    memset(&ifc, 0, sizeof(ifc));
    ifc.ifc_len = sizeof(buf);
    ifc.ifc_buf = (caddr_t)buf;
    memset(buf, 0, sizeof(buf));
    if (ioctl(cfg_socket, SIOCGIFCONF, &ifc) < 0)
    {
        te_errno rc = TE_OS_RC(TE_TA_UNIX, errno);

        ERROR("ioctl(SIOCGIFCONF) failed: %r", rc);
        return rc;
    }
    for (p = (struct ifreq *)ifc.ifc_buf;
         ifc.ifc_len >= (int)sizeof(*p);
         p = (struct ifreq *)((caddr_t)p + _SIZEOF_ADDR_IFREQ(*p)))
    {
        if ((strcmp(p->ifr_name, ifname) == 0) &&
            (p->ifr_addr.sa_family == AF_LINK))
        {
            struct sockaddr_dl *sdl = (struct sockaddr_dl *)&(p->ifr_addr);

            if (sdl->sdl_alen == ETHER_ADDR_LEN)
            {
                ptr = sdl->sdl_data + sdl->sdl_nlen;
            }
            else
            {
                /* FIXME */
                memset(buf, 0, ETHER_ADDR_LEN);
                ptr = buf;
            }
            break;
        }
    }
#else
    ERROR("%s(): %s", __FUNCTION__, strerror(EOPNOTSUPP));
    return TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP);
#endif
    if (ptr != NULL)
    {
        snprintf(value, RCF_MAX_VAL, "%02x:%02x:%02x:%02x:%02x:%02x",
                 ptr[0], ptr[1], ptr[2], ptr[3], ptr[4], ptr[5]);
    }
    else
    {
        ERROR("Not found link layer address of the interface %s", ifname);
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }
    return 0;
}

/**
 * Set hardware address of the interface.
 * Only MAC addresses are supported now.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         hardware address (address should be
 *                      provided as XX:XX:XX:XX:XX:XX)
 * @param ifname        name of the interface (like "eth0")
 *
 * @return              Status code
 */
static te_errno
link_addr_set(unsigned int gid, const char *oid, const char *value,
              const char *ifname)
{
    uint8_t *ll_addr = NULL;
    te_errno rc = 0;

    UNUSED(gid);
    UNUSED(oid);

    if ((rc = CHECK_INTERFACE(ifname)) != 0)
        return TE_RC(TE_TA_UNIX, rc);

    if (value == NULL)
    {
       ERROR("A link layer address is not provided to "
             "ioctl(SIOCSIFHWADDR)");
       return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    strcpy(req.ifr_name, ifname);
    req.ifr_hwaddr.sa_family = AF_LOCAL;
    ll_addr = req.ifr_hwaddr.sa_data;

    {
        /* Conversion MAC address to binary value */
        const char *ptr = value;
        const char *aux_ptr;
        int         i;

        for (i = 0; i < 6; i++)
        {
            aux_ptr = ptr;
            if (!isxdigit(*aux_ptr))
            {
                ERROR("Invalid MAC address (unexpected symbol %c)",
                      *aux_ptr);
                return TE_RC(TE_TA_UNIX, TE_EINVAL);
            }
            aux_ptr++;
            if (isxdigit(*aux_ptr))
                aux_ptr++;
            if ((*aux_ptr == ':') || (*aux_ptr == '\0'))
            {
                aux_ptr = ptr;
                ll_addr[i] = strtol(ptr, (char **)&aux_ptr, 16);
                ptr = aux_ptr + 1;
                if (i == 5)
                {
                    if ((*aux_ptr == '\0'))
                    {
                        break;
                    }
                    else
                    {
                        ERROR("Invalid MAC address");
                        return TE_RC(TE_TA_UNIX, TE_EINVAL);
                    }
                }
            }
            else
            {
                ERROR("Invalid MAC address (unexp. symbol %c)", *aux_ptr);
                return TE_RC(TE_TA_UNIX, TE_EINVAL);
            }
        }
    }

    if (ioctl(cfg_socket, SIOCSIFHWADDR, &req) < 0)
    {
        te_errno rc = TE_OS_RC(TE_TA_UNIX, errno);
        ERROR("ioctl(SIOCSIFHWADDR) failed to set : %r", rc);
    }

    return rc;
}

/**
 * Get MTU of the interface.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         value location
 * @param ifname        name of the interface (like "eth0")
 *
 * @return              Status code
 */
static te_errno
mtu_get(unsigned int gid, const char *oid, char *value,
        const char *ifname)
{
    te_errno    rc;
    
    UNUSED(gid);
    UNUSED(oid);

    if ((rc = CHECK_INTERFACE(ifname)) != 0)
        return TE_RC(TE_TA_UNIX, rc);

    strcpy(req.ifr_name, ifname);
    if (ioctl(cfg_socket, SIOCGIFMTU, (int)&req) != 0)
    {
        te_errno rc = TE_OS_RC(TE_TA_UNIX, errno);
        
        ERROR("ioctl(SIOCGIFMTU) failed: %r", rc);
        return rc;
    }
    sprintf(value, "%d", req.ifr_mtu);
    return 0;
}

/**
 * Change MTU of the interface.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         new value pointer
 * @param ifname        name of the interface (like "eth0")
 *
 * @return              Status code
 */
static te_errno
mtu_set(unsigned int gid, const char *oid, const char *value,
        const char *ifname)
{
    char     *tmp;
    te_errno  rc = 0;

    UNUSED(gid);
    UNUSED(oid);

    if ((rc = CHECK_INTERFACE(ifname)) != 0)
        return TE_RC(TE_TA_UNIX, rc);

    req.ifr_mtu = strtol(value, &tmp, 10);
    if (tmp == value || *tmp != 0)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    strcpy(req.ifr_name, ifname);
    if (ioctl(cfg_socket, SIOCSIFMTU, (int)&req) != 0)
    {
        rc = TE_OS_RC(TE_TA_UNIX, errno);
        
        if (errno == EBUSY)
        {
            char status[2];
            
            /* Try to down interface */
            if (status_get(0, NULL, status, ifname) == 0 &&
                *status == '1' && status_set(0, NULL, "0", ifname) == 0)
            {
                int rc1;
                
                if (ioctl(cfg_socket, SIOCSIFMTU, (int)&req) == 0)
                {
                    rc = 0;
                }
                
                if ((rc1 = status_set(0, NULL, "1", ifname)) != 0)
                {
                    ERROR("Failed to up interface after changing of mtu "
                          "error %r", rc1);
                    return rc1;
                }
            }
        }
    }
    
    if (rc != 0)
        ERROR("ioctl(SIOCSIFMTU) failed: %r", rc);

    return rc;
}

/**
 * Check if ARP is enabled on the interface 
 * ("0" - arp disable, "1" - arp enable).
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         value location
 * @param ifname        name of the interface (like "eth0")
 *
 * @return              Status code
 */
static te_errno
arp_get(unsigned int gid, const char *oid, char *value, const char *ifname)
{
    te_errno rc;

    UNUSED(gid);
    UNUSED(oid);

    if ((rc = CHECK_INTERFACE(ifname)) != 0)
        return TE_RC(TE_TA_UNIX, rc);

    strcpy(req.ifr_name, ifname);
    if (ioctl(cfg_socket, SIOCGIFFLAGS, (int)&req) != 0)
    {
        te_errno rc = TE_OS_RC(TE_TA_UNIX, errno);

        ERROR("ioctl(SIOCGIFFLAGS) failed: %r", rc);
        return rc;
    }

    sprintf(value, "%d", (req.ifr_flags & IFF_NOARP) != IFF_NOARP);

    return 0;
}

/**
 * Enable/disable ARP on the interface 
 * ("0" - arp disable, "1" - arp enable).
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         value location
 * @param ifname        name of the interface (like "eth0")
 *
 * @return              Status code
 */
static te_errno
arp_set(unsigned int gid, const char *oid, const char *value,
        const char *ifname)
{
    te_errno rc;
    
    UNUSED(gid);
    UNUSED(oid);

    if ((rc = CHECK_INTERFACE(ifname)) != 0)
        return TE_RC(TE_TA_UNIX, rc);

    strncpy(req.ifr_name, ifname, IFNAMSIZ);
    if (ioctl(cfg_socket, SIOCGIFFLAGS, &req) < 0)
    {
        te_errno rc = TE_OS_RC(TE_TA_UNIX, errno);

        ERROR("ioctl(SIOCGIFFLAGS) failed: %r", rc);
        return rc;
    }

    if (strcmp(value, "1") == 0)
        req.ifr_flags &= (~IFF_NOARP);
    else if (strcmp(value, "0") == 0)
        req.ifr_flags |= (IFF_NOARP);
    else
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    strncpy(req.ifr_name, ifname, IFNAMSIZ);
    if (ioctl(cfg_socket, SIOCSIFFLAGS, &req) < 0)
    {
        te_errno rc = TE_OS_RC(TE_TA_UNIX, errno);

        ERROR("ioctl(SIOCSIFFLAGS) failed: %r", rc);
        return rc;
    }
    return 0;
}

/**
 * Get status of the interface ("0" - down or "1" - up).
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         value location
 * @param ifname        name of the interface (like "eth0")
 *
 * @return              Status code
 */
static te_errno
status_get(unsigned int gid, const char *oid, char *value,
           const char *ifname)
{
    te_errno rc;
    
    UNUSED(gid);
    UNUSED(oid);

    if ((rc = CHECK_INTERFACE(ifname)) != 0)
        return TE_RC(TE_TA_UNIX, rc);

    strcpy(req.ifr_name, ifname);
    if (ioctl(cfg_socket, SIOCGIFFLAGS, (int)&req) != 0)
    {
        te_errno rc = TE_OS_RC(TE_TA_UNIX, errno);

        ERROR("ioctl(SIOCGIFFLAGS) failed: %r", rc);
        return rc;
    }

    sprintf(value, "%d", (req.ifr_flags & IFF_UP) != 0);

    return 0;
}

/**
 * Change status of the interface. If virtual interface is put to down
 * state,it is de-installed and information about it is stored in the list
 * of down interfaces.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         new value pointer
 * @param ifname        name of the interface (like "eth0")
 *
 * @return              Status code
 */
static te_errno
status_set(unsigned int gid, const char *oid, const char *value,
           const char *ifname)
{
    te_errno rc;
    
    UNUSED(gid);
    UNUSED(oid);

    if ((rc = CHECK_INTERFACE(ifname)) != 0)
        return TE_RC(TE_TA_UNIX, rc);

    strncpy(req.ifr_name, ifname, IFNAMSIZ);
    if (ioctl(cfg_socket, SIOCGIFFLAGS, &req) < 0)
    {
        te_errno rc = TE_OS_RC(TE_TA_UNIX, errno);

        ERROR("ioctl(SIOCGIFFLAGS) failed: %r", rc);
        return rc;
    }

    if (strcmp(value, "0") == 0)
        req.ifr_flags &= ~(IFF_UP | IFF_RUNNING);
    else if (strcmp(value, "1") == 0)
        req.ifr_flags |= (IFF_UP | IFF_RUNNING);
    else
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    strncpy(req.ifr_name, ifname, IFNAMSIZ);
    if (ioctl(cfg_socket, SIOCSIFFLAGS, &req) < 0)
    {
        te_errno rc = TE_OS_RC(TE_TA_UNIX, errno);

        ERROR("ioctl(SIOCSIFFLAGS) failed: %r", rc);
        return rc;
    }

    return 0;
}

#ifdef USE_NETLINK /* NEIGH_USE_NETLINK */
/** Find neighbour entry and return its parameters */
/**< User data for neigh_find_cb() callback function */
typedef struct {
    char      ifname[IFNAMSIZ];     /**< Interface name */
    char     *addr;                 /**< IP address in human notation */
    te_bool   dynamic;              /**< Dynamic or static neighbour */
    char     *mac_addr;             /**< MAC address (OUT) */
    uint16_t  state;                /**< Neighbour state (OUT) */
    te_bool   found;                /**< Neighbour already found */
} neigh_find_cb_param; 
    
int
neigh_find_cb(const struct sockaddr_nl *who, const struct nlmsghdr *n,
              void *arg)
{    
    neigh_find_cb_param *p = (neigh_find_cb_param *)arg;
    int                  af = (strchr(p->addr, ':') == NULL)?
                              AF_INET : AF_INET6;
    struct ndmsg        *r = NLMSG_DATA(n);
    struct rtattr       *tb[NDA_MAX+1] = {NULL,};
    uint8_t              addr_buf[sizeof(struct in6_addr)];    
    
    /* One item was already found, skip others */
    if (p->found)
    {
        return 0;
    }

    /* Neighbour from alien interface */
    if (ll_name_to_index(p->ifname) != r->ndm_ifindex)
    {
        return 0;
    }
    
    parse_rtattr(tb, NDA_MAX, NDA_RTA(r), n->nlmsg_len -
                 NLMSG_LENGTH(sizeof(*r)));

    if (inet_pton(af, p->addr, addr_buf) < 0)
    {
        return 0;
    }

    /* Check that destination is one we look for */    
    if (tb[NDA_DST] == NULL ||
        memcmp(RTA_DATA(tb[NDA_DST]), addr_buf, (af == AF_INET)?
               sizeof(struct in_addr) : sizeof(struct in6_addr)) != 0)
        return 0;

    /* Check neighbour state */
    if (r->ndm_state == NUD_NONE || r->ndm_state & NUD_FAILED)
        return 0;

    if (p->dynamic == !!(r->ndm_state & NUD_PERMANENT))
        return 0;

    /* Save MAC address */
    if (p->mac_addr != NULL && tb[NDA_LLADDR] != NULL)
    {
        int      i;
        char    *s = p->mac_addr;

        for (i = 0; i < 6; i++)
        {
            sprintf(s, "%02x:", ((uint8_t *)RTA_DATA(tb[NDA_LLADDR]))[i]);
            s += strlen(s);
        }
        *(s - 1) = '\0';
    }

    /* Save neighbour state */
    p->state = r->ndm_state;
    p->found = TRUE;

    UNUSED(who);
    
    return 0;
}
#endif

static te_errno
neigh_find(const char *oid, const char *ifname, const char *addr,
           char *mac_p, unsigned int *state_p)
{
#ifdef USE_NETLINK /* NEIGH_USE_NETLINK */
    struct rtnl_handle   rth;    
    neigh_find_cb_param  user_data;
    te_errno             rc;
    
    if ((rc = CHECK_INTERFACE(ifname)) != 0)
        return TE_RC(TE_TA_UNIX, rc);
    
    memset(&rth, 0, sizeof(rth));
    if (rtnl_open(&rth, 0) < 0)
    {
        ERROR("Failed to open a netlink socket");
        return TE_OS_RC(TE_TA_UNIX, errno);
    }
    ll_init_map(&rth);

    memset(&user_data, 0, sizeof(user_data));

    strcpy(user_data.ifname, ifname);
    user_data.addr = (char *)addr;
    user_data.dynamic = strstr(oid, "dynamic") != NULL;
    user_data.mac_addr = mac_p;
    user_data.found = FALSE;

    rtnl_wilddump_request(&rth, strchr(addr, ':') == NULL?
                          AF_INET : AF_INET6, RTM_GETNEIGH);
    rtnl_dump_filter(&rth, neigh_find_cb, &user_data, NULL, NULL);
    
    if (!user_data.found)
    {
        rtnl_close(&rth);
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }

    if (state_p != NULL)
    {
        *state_p = user_data.state;
    }

    rtnl_close(&rth);
    return 0;
    
#else
    UNUSED(oid);
    UNUSED(ifname);
    UNUSED(addr);
    UNUSED(mac_p);
    UNUSED(state_p);

    return TE_RC(TE_TA_UNIX, TE_ENOENT);
#endif
}



/**
 * Get neighbour entry state.
 *
 * @param gid            group identifier (unused)
 * @param oid            full object instence identifier (unused)
 * @param value          location for the value
 *                       (XX:XX:XX:XX:XX:XX is returned)
 * @param ifname         interface name
 * @param addr           IP address in human notation
 *
 * @return Status code
 */
int 
neigh_state_get(unsigned int gid, const char *oid, char *value,
                const char *ifname, const char *addr)
{
    unsigned int state;
    te_errno     rc;
    char         mac[ETHER_ADDR_LEN * 3];
    
    UNUSED(gid);
    UNUSED(oid);
    
    if ((rc = neigh_find("dynamic", ifname, addr, mac, &state)) != 0)
        return rc;

    sprintf(value, "%u", state);
        
    return 0;
}

/**
 * Get neighbour entry value (hardware address corresponding to IP).
 *
 * @param gid            group identifier (unused)
 * @param oid            full object instance identifier or NULL for
 *                       dynamic entries (hack)
 * @param value          location for the value
 *                       (XX:XX:XX:XX:XX:XX is returned)
 * @param ifname         interface name
 * @param addr           IP address in human notation
 *
 * @return Status code
 */
static te_errno
neigh_get(unsigned int gid, const char *oid, char *value, 
          const char *ifname, const char *addr)
{
    UNUSED(gid);
    
    return neigh_find(oid, ifname, addr, value, NULL);
}

/**
 * Change already existing neighbour entry.
 *
 * @param gid            group identifier
 * @param oid            full object instance identifier (unused)
 * @param value          new value pointer ("XX:XX:XX:XX:XX:XX")
 * @param ifname         interface name
 * @param addr           IP address in human notation
 *
 * @return Status code
 */
static te_errno
neigh_set(unsigned int gid, const char *oid, const char *value,
          const char *ifname, const char *addr)
{
    if (neigh_find(oid, ifname, addr, NULL, NULL) != 0)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    return neigh_add(gid, oid, value, ifname, addr);
}

#ifdef NEIGH_USE_NETLINK
/** Add or delete a neighbour entry.
 *
 * @param oid           Object instance identifier
 * @param addr          Destination IP address in human notation.
 * @param ifname        Interface name
 * @param value         Entry value (raw MAC address)
 * @param cmd           RTM_NEWNEIGH to add a neighbour,
 *                      RTM_DELNEIGH to delete it.
 *
 * @return Status code
 */
static te_errno
neigh_change(const char *oid, const char *addr, const char *ifname,
             uint8_t *value, int cmd)
{
    struct rtnl_handle rth;
    struct {
        struct nlmsghdr         n;
        struct ndmsg            ndm;
        char                    buf[256];
    } req;    
    inet_prefix dst;   
    
    ENTRY("oid=%s addr=%s ifname=%s value=%x:%x:%x:%x:%x:%x cmd=%d",
          oid, addr, ifname, value[0], value[1], value[2], value[3],
          value[4], value[5], cmd);

    /* TODO: check that address corresponds to interface */

    memset(&req, 0, sizeof(req));
    req.n.nlmsg_len = NLMSG_LENGTH(sizeof(struct ndmsg));
    req.n.nlmsg_flags = NLM_F_REQUEST;

    if (cmd == RTM_NEWNEIGH)
    {
        req.n.nlmsg_flags |= (NLM_F_CREATE | NLM_F_REPLACE);
    }
        
    req.n.nlmsg_type = cmd;

    dst.family = (strchr(addr, ':') != NULL) ? AF_INET6 : AF_INET;
    dst.bytelen = (dst.family == AF_INET) ?
                  sizeof(struct in_addr) : sizeof(struct in6_addr);
    dst.bitlen = dst.bytelen << 3;
    if (inet_pton(dst.family, addr, dst.data) < 0)
    {
        ERROR("Invalid neighbour address (%s)", addr);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }
    
    req.ndm.ndm_family = dst.family;
    
    if (cmd == RTM_NEWNEIGH)
    {
        if (strstr(oid, "dynamic") == NULL)
            req.ndm.ndm_state = NUD_PERMANENT;
        else
            req.ndm.ndm_state = NUD_REACHABLE;
    }
    else
    {
        req.ndm.ndm_state = NUD_NONE;
    }
    
    addattr_l(&req.n, sizeof(req), NDA_DST, &dst.data, dst.bytelen);
    
    if (value != NULL)
    {        
        addattr_l(&req.n, sizeof(req), NDA_LLADDR, value, ETHER_ADDR_LEN);
    }

    if (rtnl_open(&rth, 0) < 0)
    {
        ERROR("Failed to open Netlink socket");
        return TE_RC(TE_TA_UNIX, errno);
    }

    ll_init_map(&rth);

    if ((req.ndm.ndm_ifindex = ll_name_to_index(ifname)) == 0)
    {
        rtnl_close(&rth);
        ERROR("No device (%s) found", ifname);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    if (rtnl_talk(&rth, &req.n, 0, 0, NULL, NULL, NULL) < 0)
    {
        rtnl_close(&rth);
        ERROR("Failed to send the Netlink message");
        return TE_OS_RC(TE_TA_UNIX, errno);
    }

    rtnl_close(&rth);

    return 0;
}
#endif

/**
 * Add a new neighbour entry.
 *
 * @param gid            group identifier (unused)
 * @param oid            full object instance identifier (unused)
 * @param value          new entry value pointer ("XX:XX:XX:XX:XX:XX")
 * @param ifname         interface name
 * @param addr           IP address in human notation
 *
 * @return Status code
 */
static te_errno
neigh_add(unsigned int gid, const char *oid, const char *value,
          const char *ifname, const char *addr)
{
#ifndef NEIGH_USE_NETLINK
    struct arpreq arp_req;
    int           i;
#endif    
 
    int           int_addr[ETHER_ADDR_LEN]; 
    int           res;
    
    UNUSED(gid);
    
    res = sscanf(value, "%2x:%2x:%2x:%2x:%2x:%2x%*s",
                 int_addr, int_addr + 1, int_addr + 2, int_addr + 3,
                 int_addr + 4, int_addr + 5);

    if (res != 6)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

#ifdef NEIGH_USE_NETLINK
    if (value != NULL)
    {        
        unsigned int    i;
        uint8_t         raw_addr[ETHER_ADDR_LEN];

        for (i = 0; i < ETHER_ADDR_LEN; i++)
        {
            raw_addr[i] = (uint8_t)int_addr[i];
        }
        return neigh_change(oid, addr, ifname, raw_addr, RTM_NEWNEIGH);
    }
    else
    {
        return neigh_change(oid, addr, ifname, NULL, RTM_NEWNEIGH);
    }
#else /* USE_IOCTL */
    /* TODO: check that address corresponds to interface */
    UNUSED(ifname);

    memset(&arp_req, 0, sizeof(arp_req));
    arp_req.arp_pa.sa_family = AF_INET;

    if (inet_pton(AF_INET, addr, &SIN(&(arp_req.arp_pa))->sin_addr) <= 0)
    {
        ERROR("%s(): Failed to convert IPv4 address from string '%s'",
              __FUNCTION__, addr);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    arp_req.arp_ha.sa_family = AF_LOCAL;
    for (i = 0; i < 6; i++)
        (arp_req.arp_ha.sa_data)[i] = (unsigned char)(int_addr[i]);

    arp_req.arp_flags = ATF_COM;
    if (strstr(oid, "dynamic") == NULL)
    {
        VERB("%s(): Add permanent ARP entry", __FUNCTION__);
        arp_req.arp_flags |= ATF_PERM;
    }

#ifdef SIOCSARP
    if (ioctl(cfg_socket, SIOCSARP, &arp_req) < 0)
    {
        te_errno rc = TE_OS_RC(TE_TA_UNIX, errno);

        ERROR("ioctl(SIOCSARP) failed: %r", rc);
        return rc;
    }

    return 0;
#else
    return TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP);
#endif
#endif    
}

/**
 * Delete neighbour entry.
 *
 * @param gid            group identifier (unused)
 * @param oid            full object instance identifier (unused)
 * @param ifname         interface name
 * @param addr           IP address in human notation
 *
 * @return Status code
 */
static te_errno
neigh_del(unsigned int gid, const char *oid, const char *ifname, 
          const char *addr)
{
    te_errno      rc;    
    UNUSED(gid);

    if ((rc = neigh_find(oid, ifname, addr, NULL, NULL)) != 0)
    {
        if (TE_RC_GET_ERROR(rc) == TE_ENOENT)
        {
            WARN("Cannot delete ARP entry: it disappeared");
            rc = 0;
        }
        return rc;
    }
#ifdef NEIGH_USE_NETLINK
    return neigh_change(oid, addr, ifname, NULL, RTM_DELNEIGH); 
#else /* USE_IOCTL */
    {
        struct arpreq arp_req;
        int           family;
        
        memset(&arp_req, 0, sizeof(arp_req));
        family = (strchr(addr, ':') != NULL)? AF_INET6 : AF_INET;
        arp_req.arp_pa.sa_family = family;
        if (inet_pton(family, addr, &SIN(&(arp_req.arp_pa))->sin_addr) <= 0)
            return TE_RC(TE_TA_UNIX, TE_EINVAL);

#ifdef SIOCDARP

        if (ioctl(cfg_socket, SIOCDARP, &arp_req) < 0)
        {
            if (errno == ENXIO || errno == ENETDOWN || errno == ENETUNREACH)
                return 0;
            else
                return TE_OS_RC(TE_TA_UNIX, errno);
        }

        return 0;
#else
        return TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP);
#endif
    }
#endif    
}

#ifdef USE_NETLINK /* NEIGH_USE_NETLINK */
typedef struct {
    te_bool dynamic;
    char    ifname[IFNAMSIZ];
    char   *list;
} neigh_print_cb_param;

int
neigh_print_cb(const struct sockaddr_nl *who, const struct nlmsghdr *n,
               void *arg)
{
    neigh_print_cb_param *p = (neigh_print_cb_param *)arg;

    struct ndmsg   *r = NLMSG_DATA(n);
    struct rtattr  *tb[NDA_MAX+1] = {NULL,};
    char           *s = p->list + strlen(p->list);
 
    if (ll_name_to_index(p->ifname) != r->ndm_ifindex)
        return 0;
    
    if (r->ndm_state == NUD_NONE || (r->ndm_state & NUD_INCOMPLETE) != 0)
        return 0;
    
    if (!!(r->ndm_state & NUD_PERMANENT) == p->dynamic)
        return 0;

    parse_rtattr(tb, NDA_MAX, NDA_RTA(r), n->nlmsg_len -
                 NLMSG_LENGTH(sizeof(*r)));
    if (tb[NDA_DST] == NULL ||
        inet_ntop(r->ndm_family, RTA_DATA(tb[NDA_DST]), s,
                  INET6_ADDRSTRLEN) == NULL)
        return 0;
        
    s += strlen(s);
    sprintf(s, " ");

    UNUSED(who);
    
    return 0;
}
#endif
 
/**
 * Get instance list for object "agent/arp" and "agent/volatile/arp".
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier
 * @param list          location for the list pointer
 * @param ifname        interface name
 *
 * @return Status code
 */
static te_errno
neigh_list(unsigned int gid, const char *oid, char **list, 
           const char *ifname)
{
#ifdef USE_NETLINK /* NEIGH_USE_NETLINK */
    struct rtnl_handle   rth;    
    neigh_print_cb_param user_data;
    
    buf[0] = '\0';

    if (strcmp(ifname, "lo") == 0)
        return 0;
        
    memset(&rth, 0, sizeof(rth));
    if (rtnl_open(&rth, 0) < 0)
    {
        ERROR("Failed to open a netlink socket");
        return TE_OS_RC(TE_TA_UNIX, errno);
    }
    ll_init_map(&rth);

    strcpy(user_data.ifname, ifname);
    user_data.dynamic = strstr(oid, "dynamic") != NULL;
    user_data.list = buf;

    rtnl_wilddump_request(&rth, AF_INET, RTM_GETNEIGH);
    rtnl_dump_filter(&rth, neigh_print_cb, &user_data, NULL, NULL);    
 
#ifdef NEIGH_USE_NETLINK
    /* 
     * We cannot list IPv6 entries because we'll not be able 
     * to delete them 
     */
    user_data.list = buf + strlen(buf);

    rtnl_wilddump_request(&rth, AF_INET6, RTM_GETNEIGH);
    rtnl_dump_filter(&rth, neigh_print_cb, &user_data, NULL, NULL);
#endif

    rtnl_close(&rth);

#else
    UNUSED(oid);
    UNUSED(ifname);

    *buf = '\0';
#endif

    UNUSED(gid);

    if ((*list = strdup(buf)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    return 0;
}

/******************************************************************/
/* Implementation of /agent/route subtree                         */
/******************************************************************/

#ifdef USE_NETLINK
/*
 * The following code is based on iproute2-050816 package.
 * There is no clear specification of netlink interface.
 */

struct nl_request {
    struct nlmsghdr n;
    struct rtmsg    r;
    char            buf[1024];
};

/** 
 * Convert system-independent route info data structure to
 * netlink-specific data structure.
 *
 * @param rt_info TA portable route info
 * @Param req     netlink-specific data structure (OUT)
 */
static int
rt_info2nl_req(const ta_rt_info_t *rt_info,
               struct nl_request *req)
{
    char                 mxbuf[256];
    struct rtattr       *mxrta = (void*)mxbuf;
    unsigned char        family;  

    assert(rt_info != NULL && req != NULL);

    mxrta->rta_type = RTA_METRICS;
    mxrta->rta_len = RTA_LENGTH(0);

    req->r.rtm_dst_len = rt_info->prefix;
    family = req->r.rtm_family = rt_info->dst.ss_family;
    
    if ((family == AF_INET && 
         addattr_l(&req->n, sizeof(*req), RTA_DST,
                   &SIN(&rt_info->dst)->sin_addr,
                   sizeof(struct in_addr)) != 0) ||
        (family == AF_INET6 &&
         addattr_l(&req->n, sizeof(*req), RTA_DST,
                   &SIN6(&rt_info->dst)->sin6_addr,
                   sizeof(struct in6_addr)) != 0))
    {
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    if ((rt_info->flags & TA_RT_INFO_FLG_GW) != 0)
    {
        if (addattr_l(&req->n, sizeof(*req), RTA_GATEWAY,
                      &(SIN(&(rt_info->gw))->sin_addr),
                      sizeof(SIN(&(rt_info->gw))->sin_addr)) != 0)
        {
            return TE_RC(TE_TA_UNIX, TE_EINVAL);
        }
    }
    if ((rt_info->flags & TA_RT_INFO_FLG_IF) != 0)
    {
        int idx;

        if ((idx = ll_name_to_index(rt_info->ifname)) == 0)
        {
            ERROR("Cannot find interface %s", rt_info->ifname);
            return TE_RC(TE_TA_UNIX, TE_EINVAL);
        }
        addattr32(&req->n, sizeof(*req), RTA_OIF, idx);
    }

    if ((rt_info->flags & TA_RT_INFO_FLG_METRIC) != 0)
        addattr32(&req->n, sizeof(*req), RTA_PRIORITY, rt_info->metric);

    if ((rt_info->flags & TA_RT_INFO_FLG_MTU) != 0)
        rta_addattr32(mxrta, sizeof(mxbuf), RTAX_MTU, rt_info->mtu);

    if ((rt_info->flags & TA_RT_INFO_FLG_WIN) != 0)
        rta_addattr32(mxrta, sizeof(mxbuf), RTAX_WINDOW, rt_info->win);

    if ((rt_info->flags & TA_RT_INFO_FLG_IRTT) != 0)
        rta_addattr32(mxrta, sizeof(mxbuf), RTAX_RTT, rt_info->irtt);

    if ((rt_info->flags & TA_RT_INFO_FLG_TOS) != 0)
    {
        req->r.rtm_tos = rt_info->tos;
    }
    
    if (mxrta->rta_len > RTA_LENGTH(0))
    {
        addattr_l(&req->n, sizeof(*req), RTA_METRICS, RTA_DATA(mxrta),
                  RTA_PAYLOAD(mxrta));
    }

    return 0;
}

/** 
 * The code of this function is based on iproute2 GPL package.
 *
 * @param rt_info  Route-related information
 * @param action   What to do with this route
 * @param flags    netlink-specific flags
 */
static te_errno
route_change(ta_rt_info_t *rt_info, int action, unsigned flags)
{
    struct nl_request  req;
    struct rtnl_handle rth;
    te_errno           rc;

    if (rt_info == NULL)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    memset(&req, 0, sizeof(req));

    req.n.nlmsg_len = NLMSG_LENGTH(sizeof(struct rtmsg));
    req.n.nlmsg_flags = NLM_F_REQUEST | flags;
    req.n.nlmsg_type = action;

    req.r.rtm_family = SA(&rt_info->dst)->sa_family;
    req.r.rtm_table = RT_TABLE_MAIN;
    req.r.rtm_scope = RT_SCOPE_NOWHERE;

    if (action != RTM_DELROUTE)
    {
        req.r.rtm_protocol = RTPROT_BOOT;
        req.r.rtm_scope = RT_SCOPE_UNIVERSE;
        req.r.rtm_type = RTN_UNICAST;
    }

    /* Sending the netlink message */
    if (rtnl_open(&rth, 0) < 0)
    {
        ERROR("Failed to open the netlink socket");
        return TE_OS_RC(TE_TA_UNIX, errno);
    }

    if ((rt_info->flags & TA_RT_INFO_FLG_IF) != 0)
    {
        /*
         * We need to call this function as rt_info2nl_req() 
         * will need to convert interface name to index.
         */
        ll_init_map(&rth);
    }
    
    if ((rc = rt_info2nl_req(rt_info, &req)) != 0)
    {
        rtnl_close(&rth);
        return rc;
    }
    
    {
        if (req.r.rtm_type == RTN_LOCAL ||
            req.r.rtm_type == RTN_NAT)
            req.r.rtm_scope = RT_SCOPE_HOST;
        else if (req.r.rtm_type == RTN_BROADCAST ||
                 req.r.rtm_type == RTN_MULTICAST ||
                 req.r.rtm_type == RTN_ANYCAST)
            req.r.rtm_scope = RT_SCOPE_LINK;
        else if (req.r.rtm_type == RTN_UNICAST ||
                 req.r.rtm_type == RTN_UNSPEC)
        {
            if (action == RTM_DELROUTE)
                req.r.rtm_scope = RT_SCOPE_NOWHERE;
            else if ((rt_info->flags & TA_RT_INFO_FLG_GW) == 0)
                req.r.rtm_scope = RT_SCOPE_LINK;
        }
    }

    if (rtnl_talk(&rth, &req.n, 0, 0, NULL, NULL, NULL) < 0)
    {
        ERROR("Failed to send the netlink message");
        rtnl_close(&rth);
        return TE_OS_RC(TE_TA_UNIX, errno);
    }

    rtnl_close(&rth);
    return 0;
}

/** Structure used for RTNL user callback */
typedef struct rtnl_cb_user_data {
    ta_rt_info_t *rt_info; /**< Routing entry information (IN/OUT)
                                On input it keeps route key,
                                On output it is augmented with route
                                attributes: mtu, win etc. */
    int if_index;          /**< Interface index in case of direct route.
                                This field has sense  only if 
                                TA_RT_INFO_FLG_IF flag is set */
    te_errno rc;                /**< Return code */
    te_bool filled;        /**< Whether this structure is filled or not */
} rtnl_cb_user_data_t;

/**
 * Callback for rtnl_dump_filter() function.
 */
static int
rtnl_get_route_cb(const struct sockaddr_nl *who,
                  const struct nlmsghdr *n, void *arg)
{
    struct rtmsg        *r = NLMSG_DATA(n);
    int                  len = n->nlmsg_len;
    rtnl_cb_user_data_t *user_data = (rtnl_cb_user_data_t *)arg;
    struct rtattr       *tb[RTA_MAX + 1] = {NULL,};
    int                  family;
    struct in6_addr      addr_any = IN6ADDR_ANY_INIT;

    UNUSED(who);

    if (user_data->filled)
        return 0;

    if (n->nlmsg_type != RTM_NEWROUTE && n->nlmsg_type != RTM_DELROUTE)
        return 0;
    
    if (r->rtm_family != AF_INET && r->rtm_family != AF_INET6)
    {
       return 0;        
    }

    len -= NLMSG_LENGTH(sizeof(*r));

    parse_rtattr(tb, RTA_MAX, RTM_RTA(r), len);

    family = r->rtm_family;
    /* Check the case of INADDR_ANY address */
    if ((tb[RTA_DST] == NULL &&
         ((family == AF_INET &&
           SIN(&(user_data->rt_info->dst))->sin_addr.s_addr == INADDR_ANY)
       || (family == AF_INET6 &&
           memcmp(&SIN6(&(user_data->rt_info->dst))->sin6_addr.in6_u,
                  &addr_any, sizeof(struct in6_addr)) == 0))) ||
    /* Check that destination address is equal to requested one */
        (tb[RTA_DST] != NULL &&
         ((family == AF_INET &&
           memcmp(RTA_DATA(tb[RTA_DST]),
                  &(SIN(&(user_data->rt_info->dst))->sin_addr),
                  sizeof(struct in_addr)) == 0) ||
          (family == AF_INET6 &&
          memcmp(RTA_DATA(tb[RTA_DST]),
                  &(SIN6(&(user_data->rt_info->dst))->sin6_addr),
                  sizeof(struct in6_addr)) == 0)) &&
          /* Check that prefix is correct */
          user_data->rt_info->prefix == r->rtm_dst_len &&
          ((user_data->rt_info->flags & TA_RT_INFO_FLG_METRIC) == 0 ||
           user_data->rt_info->metric ==
               *(uint32_t *)RTA_DATA(tb[RTA_PRIORITY])) &&
          ((user_data->rt_info->flags & TA_RT_INFO_FLG_TOS) == 0 ||
           user_data->rt_info->tos == r->rtm_tos)))
    {
        if (tb[RTA_OIF] != NULL)
        {            
            user_data->rt_info->flags |= TA_RT_INFO_FLG_IF;
            user_data->if_index = *(int *)RTA_DATA(tb[RTA_OIF]);
            memcpy(user_data->rt_info->ifname,
                   ll_index_to_name(user_data->if_index),
                   IFNAMSIZ);
        }
        
        if (tb[RTA_GATEWAY] != NULL)
        {
            user_data->rt_info->flags |= TA_RT_INFO_FLG_GW;
            user_data->rt_info->gw.ss_family = family;
            if (family == AF_INET)
            {
                memcpy(&SIN(&user_data->rt_info->gw)->sin_addr,
                       RTA_DATA(tb[RTA_GATEWAY]), sizeof(struct in_addr));
            }
            else if (family == AF_INET6)
            {
                memcpy(&SIN6(&user_data->rt_info->gw)->sin6_addr,
                       RTA_DATA(tb[RTA_GATEWAY]), sizeof(struct in6_addr));
            }
        }
 
        if (tb[RTA_PRIORITY] != NULL)
        {
            user_data->rt_info->flags |= TA_RT_INFO_FLG_METRIC;
            user_data->rt_info->metric = *(uint32_t *)
                                         RTA_DATA(tb[RTA_PRIORITY]);
        }
        
        if (tb[RTA_METRICS] != NULL)
        {
            struct rtattr *mxrta[RTAX_MAX + 1] = {NULL,};

            parse_rtattr(mxrta, RTAX_MAX, RTA_DATA(tb[RTA_METRICS]),
                         RTA_PAYLOAD(tb[RTA_METRICS]));
            
            if (mxrta[RTAX_MTU] != NULL)
            {
                user_data->rt_info->flags |= TA_RT_INFO_FLG_MTU;
                user_data->rt_info->mtu = 
                    *(unsigned *)RTA_DATA(mxrta[RTAX_MTU]);
            }
            if (mxrta[RTAX_WINDOW] != NULL)
            {
                user_data->rt_info->flags |= TA_RT_INFO_FLG_WIN;
                user_data->rt_info->win = 
                    *(unsigned *)RTA_DATA(mxrta[RTAX_WINDOW]);
            }
            if (mxrta[RTAX_RTT] != NULL)
            {
                user_data->rt_info->flags |= TA_RT_INFO_FLG_IRTT;
                user_data->rt_info->irtt = 
                    *(unsigned *)RTA_DATA(mxrta[RTAX_RTT]);
            }
        }        

        user_data->filled = TRUE;
        return 0;
    }
    return 0;
}
#endif /* USE_NETLINK */

/**
 * Find route and return its attributes.
 *
 * @param route    route instance name: see doc/cm_cm_base.xml
 *                 for the format
 * @param rt_info  route related information (OUT)
 *
 * @return         Status code
 */
static te_errno
route_find(const char *route, ta_rt_info_t *rt_info)
{
#ifdef USE_NETLINK
    te_errno  rc;

    ENTRY("%s", route);

    if ((rc = ta_rt_parse_inst_name(route, rt_info)) != 0)
    {
        ERROR("Error parsing instance name: %s", route);
        return TE_OS_RC(TE_TA_UNIX, rc);
    }

    struct rtnl_handle  rth;
    rtnl_cb_user_data_t user_data;

    memset(&user_data, 0, sizeof(user_data));

    memset(&rth, 0, sizeof(rth));
    if (rtnl_open(&rth, 0) < 0)
    {
        ERROR("Failed to open a netlink socket");
        return TE_OS_RC(TE_TA_UNIX, errno);
    }
    ll_init_map(&rth);

    if (rtnl_wilddump_request(&rth, rt_info->dst.ss_family,
                              RTM_GETROUTE) < 0)
    {
        rtnl_close(&rth);
        ERROR("Cannot send dump request to netlink");
        return TE_OS_RC(TE_TA_UNIX, errno);
    }

    /* Fill in user_data, which will be passed to callback function */
    user_data.rt_info = rt_info;
    user_data.filled = FALSE;

    if (rtnl_dump_filter(&rth, rtnl_get_route_cb,
                         &user_data, NULL, NULL) < 0)
    {
        rtnl_close(&rth);
        ERROR("Dump terminated");
        return TE_OS_RC(TE_TA_UNIX, errno);
    }
    rtnl_close(&rth);

    if (!user_data.filled)
    {
        ERROR("Cannot find route %s", route);
        return TE_OS_RC(TE_TA_UNIX, TE_ENOENT);
    }

    return 0;

#else /* USE_NETLINK */

    UNUSED(route);
    UNUSED(rt_info);

    return TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP);

#endif /* !USE_NETLINK */
}

/**
 * Get route value.
 *
 * @param  gid          Group identifier (unused)
 * @param  oid          Object identifier (unused)
 * @param  value        Place for route value (gateway address
 *                      or zero if it is a direct route)
 * @param route_name    Name of the route
 *
 * @return              Status code
 */
static te_errno
route_get(unsigned int gid, const char *oid,
          char *value, const char *route_name)
{
    ta_rt_info_t  attr;
    te_errno      rc;

    UNUSED(gid);
    UNUSED(oid);
    
    if ((rc = route_find(route_name, &attr)) != 0)
    {
        ERROR("Route %s cannot be found", route_name);
        return rc;
    }

    switch (attr.dst.ss_family)
    {
        case AF_INET:
        {
            inet_ntop(AF_INET, &SIN(&attr.gw)->sin_addr,
                      value, INET_ADDRSTRLEN);
            break;
        }

        case AF_INET6:
        {
            inet_ntop(AF_INET6, &SIN6(&attr.gw)->sin6_addr,
                      value, INET6_ADDRSTRLEN);
            break;
        }

        default:
        {
            ERROR("Unexpected destination address family: %d",
                  attr.dst.ss_family);
            return TE_RC(TE_TA_UNIX, TE_EINVAL);
        }
    }

    return 0;
}

/**
 * Set route value.
 *
 * @param       gid        Group identifier (unused)
 * @param       oid        Object identifier
 * @param       value      New value for the route
 * @param       route_name Name of the route
 *
 * @return      Status code.
 */
static te_errno
route_set(unsigned int gid, const char *oid, const char *value,
          const char *route_name)
{   
    UNUSED(gid);
    UNUSED(oid);

    return ta_obj_value_set(TA_OBJ_TYPE_ROUTE ,route_name, value);
}

/**
 * Load all route-specific attributes into route object.
 *
 * @param obj  Object to be uploaded
 *
 * @return 0 in case of success, or error code on failure.
 */
static te_errno
route_load_attrs(ta_cfg_obj_t *obj)
{
    ta_rt_info_t rt_info;
    te_errno     rc;
    char         val[128];

    if ((rc = route_find(obj->name, &rt_info)) != 0)
        return rc;

#define ROUTE_LOAD_ATTR(field_flg_, field_) \
    do {                                                      \
        snprintf(val, sizeof(val), "%d", rt_info.field_);     \
        if (rt_info.flags & TA_RT_INFO_FLG_ ## field_flg_ &&  \
            (rc = ta_obj_set(TA_OBJ_TYPE_ROUTE,               \
                             obj->name, #field_,              \
                             val, NULL)) != 0)                \
        {                                                     \
            return rc;                                        \
        }                                                     \
    } while (0)

    ROUTE_LOAD_ATTR(MTU, mtu);
    ROUTE_LOAD_ATTR(WIN, win);
    ROUTE_LOAD_ATTR(IRTT, irtt);

    snprintf(val, sizeof(val), "%s", rt_info.ifname);
    if (rt_info.flags & TA_RT_INFO_FLG_IF &&
        (rc = ta_obj_set(TA_OBJ_TYPE_ROUTE,
                        obj->name, "dev",
                        val, NULL) != 0))
    {
        ERROR("Invalid interface");
        return rc;
    }

    if (rt_info.gw.ss_family == AF_INET)
    {
        inet_ntop(AF_INET, &(SIN(&rt_info.gw)->sin_addr),
                  val, INET_ADDRSTRLEN);
    }
    else if (rt_info.gw.ss_family == AF_INET6)
    {
        inet_ntop(AF_INET6, &(SIN6(&rt_info.gw)->sin6_addr),
                  val, INET6_ADDRSTRLEN);
    }    

#undef ROUTE_LOAD_ATTR

    return 0;
}

#define DEF_ROUTE_GET_FUNC(field_) \
static te_errno                                             \
route_ ## field_ ## _get(unsigned int gid, const char *oid, \
                         char *value, const char *route)    \
{                                                           \
    te_errno     rc;                                        \
    ta_rt_info_t rt_info;                                   \
                                                            \
    UNUSED(gid);                                            \
    UNUSED(oid);                                            \
                                                            \
    if ((rc = route_find(route, &rt_info)) != 0)             \
        return rc;                                          \
                                                            \
    sprintf(value, "%d", rt_info.field_);                   \
    return 0;                                               \
}

#define DEF_ROUTE_SET_FUNC(field_) \
static te_errno                                                \
route_ ## field_ ## _set(unsigned int gid, const char *oid,    \
                         const char *value, const char *route) \
{                                                              \
    UNUSED(gid);                                               \
    UNUSED(oid);                                               \
                                                               \
    return ta_obj_set(TA_OBJ_TYPE_ROUTE, route, #field_,       \
                      value, route_load_attrs);                \
}

DEF_ROUTE_GET_FUNC(mtu);
DEF_ROUTE_SET_FUNC(mtu);
DEF_ROUTE_GET_FUNC(win);
DEF_ROUTE_SET_FUNC(win);
DEF_ROUTE_GET_FUNC(irtt);
DEF_ROUTE_SET_FUNC(irtt);
DEF_ROUTE_SET_FUNC(dev);

static te_errno
route_dev_get(unsigned int gid, const char *oid,
              char *value, const char *route) 
{
    te_errno     rc;
    ta_rt_info_t rt_info;

    UNUSED(gid);
    UNUSED(oid);
    
    if ((rc = route_find(route, &rt_info)) != 0)
        return rc;

    sprintf(value, "%s", rt_info.ifname);
    return 0;
}

#undef DEF_ROUTE_GET_FUNC
#undef DEF_ROUTE_SET_FUNC

/**
 * Add a new route.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         value string
 * @param route         route instance name: see doc/cm_cm_base.xml
 *                      for the format
 *
 * @return              Status code
 */
static te_errno
route_add(unsigned int gid, const char *oid, const char *value,
          const char *route)
{
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);

    return ta_obj_add(TA_OBJ_TYPE_ROUTE, route, value, NULL, NULL);
}

/**
 * Delete a route.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param route         route instance name: see doc/cm_cm_base.xml
 *                      for the format
 *
 * @return              Status code
 */
static te_errno
route_del(unsigned int gid, const char *oid, const char *route)
{
    UNUSED(gid);
    UNUSED(oid);

    return ta_obj_del(TA_OBJ_TYPE_ROUTE, route, NULL);
}


#ifdef USE_NETLINK

typedef struct{
    int           family;        /**< Route address family */
    char         *buf;           /**< Where to print the route */
}rtnl_print_route_cb_user_data_t;

static int
rtnl_print_route_cb(const struct sockaddr_nl *who,
                    const struct nlmsghdr *n, void *arg)
{
    struct rtmsg        *r = NLMSG_DATA(n);
    int                  len = n->nlmsg_len;
    char                *p;
    const char          *ifname;
    
    rtnl_print_route_cb_user_data_t *user_data = 
                         (rtnl_print_route_cb_user_data_t *)arg;
    
    struct rtattr       *tb[RTA_MAX + 1] = {NULL,};
    int                  family;

    UNUSED(who);

      
    family = r->rtm_family;
    
    if (family != user_data->family)
    {
        return 0;
    }

    if (family != AF_INET && family != AF_INET6)
    {
        return 0;
    }
    
    if (family == AF_INET6)
    {
#if 1
        /* Route destination is unreachable */
        if (tb[RTA_PRIORITY] && *(int*)RTA_DATA(tb[RTA_PRIORITY]) == -1)
            return 0;

        if ((r->rtm_flags & RTM_F_CLONED) != 0)
            return 0;

        if (r->rtm_type != RTN_UNICAST)
            return 0;

        if (n->nlmsg_type != RTM_NEWROUTE)
            return 0;
#endif        
    }
    else
    {
        if (r->rtm_table != RT_TABLE_MAIN)
            return 0;
    }

    len -= NLMSG_LENGTH(sizeof(*r));

    parse_rtattr(tb, RTA_MAX, RTM_RTA(r), len);

    ifname = ll_index_to_name(*(int *)RTA_DATA(tb[RTA_OIF]));
    if (!INTERFACE_IS_MINE(ifname))
        return 0;
   
    p = user_data->buf;
    
    if (tb[RTA_DST] == NULL)
    {
        if (r->rtm_dst_len != 0)
        {
            ERROR("NULL destination with non-zero prefix");
            return 0;
        }
        else
        {
            if (family == AF_INET)
            {
                p += sprintf(p, "0.0.0.0|0");
            }
            else
            {    
                p += sprintf(p, "::|0");
            }
        }
    }
    else
    {
        if (tb[RTA_GATEWAY] != NULL &&
            memcmp(RTA_DATA(tb[RTA_DST]), RTA_DATA(tb[RTA_GATEWAY]),
            r->rtm_dst_len) == 0)
        {
            /* Gateway = destination, skip route */
            return 0;
        }
            
        inet_ntop(family, RTA_DATA(tb[RTA_DST]), p, INET6_ADDRSTRLEN);
        p += strlen(p);
        p += sprintf(p, "|%d", r->rtm_dst_len);        
    }

    if (tb[RTA_PRIORITY] != NULL &&
        (*(uint32_t *)RTA_DATA(tb[RTA_PRIORITY]) != 0))
    {
        p += sprintf(p, ",metric=%d", 
                     *(uint32_t *)RTA_DATA(tb[RTA_PRIORITY]));
    }

    if (r->rtm_tos != 0)
    {
        p += sprintf(p, ",tos=%d", r->rtm_tos);
    }

    p += sprintf(p, " ");
    user_data->buf = p;

    return 0;
}

#endif

/**
 * Get instance list for object "agent/route".
 *
 * @param id            full identifier of the father instance
 * @param list          location for the list pointer
 *
 * @return              Status code
 * @retval 0                       success
 * @retval TE_ENOENT               no such instance
 * @retval TE_ENOMEM               cannot allocate memory
 */
static te_errno
route_list(unsigned int gid, const char *oid, char **list)
{
    UNUSED(gid);
    UNUSED(oid);

    ENTRY();

    buf[0] = '\0';

#ifdef USE_NETLINK

    struct rtnl_handle               rth;
    rtnl_print_route_cb_user_data_t  user_data;    

    memset(&user_data, 0, sizeof(user_data));

    user_data.buf = buf;

    memset(&rth, 0, sizeof(rth));
    if (rtnl_open(&rth, 0) < 0)
    {
        ERROR("Failed to open a netlink socket");
        return TE_OS_RC(TE_TA_UNIX, errno);
    }
    ll_init_map(&rth);
    
#define GET_ALL_ROUTES_OF_FAMILY(__family)                      \
do {                                                            \
    if (rtnl_wilddump_request(&rth, __family,                   \
                              RTM_GETROUTE) < 0)                \
    {                                                           \
        rtnl_close(&rth);                                       \
        ERROR("Cannot send dump request to netlink");           \
        return TE_OS_RC(TE_TA_UNIX, errno);                     \
    }                                                           \
                                                                \
    /* Fill in user_data, which will be passed to callback function */  \
    user_data.family = __family;                                \
    if (rtnl_dump_filter(&rth, rtnl_print_route_cb,             \
                         &user_data, NULL, NULL) < 0)           \
    {                                                           \
        rtnl_close(&rth);                                       \
        ERROR("Dump terminated");                               \
        return TE_OS_RC(TE_TA_UNIX, errno);                     \
    }                                                           \
} while (0)

    GET_ALL_ROUTES_OF_FAMILY(AF_INET);
#if 0
    GET_ALL_ROUTES_OF_FAMILY(AF_INET6);
#endif

#undef GET_ALL_ROUTES_OF_FAMILY    
    
    rtnl_close(&rth);
#else
    ERROR("Only routes via Netlink are currently supported");
#endif

    INFO("%s: Routes: %s", __FUNCTION__, buf);
    if ((*list = strdup(buf)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    return 0;
}

/**
 * Commit changes made for the route.
 *
 * @param gid           group identifier (unused)
 * @param p_oid         object instance data structure
 *
 * @return              Status code
 */
static te_errno
route_commit(unsigned int gid, const cfg_oid *p_oid)
{
#ifdef USE_NETLINK
    const char   *route;
    ta_cfg_obj_t *obj;
    ta_rt_info_t  rt_info_name_only;
    ta_rt_info_t  rt_info;
    te_errno      rc = 0;
    
    ta_cfg_obj_action_e obj_action;

    UNUSED(gid);
    ENTRY("%s", route);

    route = ((cfg_inst_subid *)(p_oid->ids))[p_oid->len - 1].name;
    
    if ((obj = ta_obj_find(TA_OBJ_TYPE_ROUTE, route)) == NULL)
    {
        WARN("Commit for %s route which has not been updated", route);
        return 0;
    }

    memset(&rt_info, 0, sizeof(rt_info));

    if ((rc = ta_rt_parse_obj(obj, &rt_info)) != 0)
    {
        ta_obj_free(obj);
        return rc;
    }
   
    obj_action = obj->action;
    ta_rt_parse_inst_name(obj->name, &rt_info_name_only);

    ta_obj_free(obj);

    {
        int nlm_flags;
        int nlm_action;

        switch (obj_action)
        {
#define TA_CFG_OBJ_ACTION_CASE(case_label_, action_, flg_) \
            case TA_CFG_OBJ_ ## case_label_:               \
                nlm_action = (action_);                    \
                nlm_flags = (flg_);                        \
                break
            
            TA_CFG_OBJ_ACTION_CASE(CREATE, RTM_NEWROUTE,
                                   NLM_F_CREATE | NLM_F_EXCL);
            TA_CFG_OBJ_ACTION_CASE(DELETE, RTM_DELROUTE, 0);
            TA_CFG_OBJ_ACTION_CASE(SET, RTM_NEWROUTE,
                                   NLM_F_REPLACE);

#undef TA_CFG_OBJ_ACTION_CASE

            default:
                ERROR("Unknown object action specified %d", obj_action);
                return TE_RC(TE_TA_UNIX, TE_EINVAL);
        }
        
    rc = route_change(&rt_info, nlm_action, nlm_flags);
    }
    
    return rc;

#else /* !USE_NETLINK */
    UNUSED(gid);
    UNUSED(p_oid);
    return TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP);
#endif /* USE_NETLINK */
}

static te_errno
nameserver_get(unsigned int gid, const char *oid, char *result,
               const char *instance, ...)
{
    FILE    *resolver = NULL;
    char     buf[256] = { 0, };
    char    *found = NULL, *endaddr = NULL;
    te_errno rc = TE_RC(TE_TA_UNIX, TE_ENOENT);

    static const char ip_symbols[] = "0123456789.";

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(instance);
    
    *result = '\0';
    resolver = fopen("/etc/resolv.conf", "r");
    if (!resolver)
    {
        rc = errno;
        ERROR("Unable to open '/etc/resolv.conf'");
        return TE_OS_RC(TE_TA_UNIX, rc);
    }
    while ((fgets(buf, sizeof(buf), resolver)) != NULL)
    {
        if ((found = strstr(buf, "nameserver")) != NULL)
        {
            found += strcspn(found, ip_symbols);
            if (*found != '\0')
            {
                endaddr = found + strspn(found, ip_symbols);
                *endaddr = '\0';

                if (inet_addr(found) == INADDR_NONE)
                    continue;
                if(endaddr - found > RCF_MAX_VAL)
                    rc = TE_RC(TE_TA_UNIX, TE_ENAMETOOLONG);
                else
                {
                    rc = 0;
                    memcpy(result, found, endaddr - found);
                }
                break;
            }
        }
    }
    fclose(resolver);
    return rc;
}


/**
 * Is Environment variable with such name hidden?
 *
 * @param name      Variable name
 * @param name_len  -1, if @a name is a NUL-terminated string;
 *                  >= 0, if length of the @a name is @a name_len
 */
static te_bool
env_is_hidden(const char *name, int name_len)
{
    unsigned int    i;

    for (i = 0; i < sizeof(env_hidden) / sizeof(env_hidden[0]); ++i)
    {
        if (memcmp(env_hidden[i], name,
                   (name_len < 0) ? strlen(name) : (size_t)name_len) == 0)
            return TRUE;
    }
    return FALSE;
}

/**
 * Get Environment variable value.
 *
 * @param gid       Request's group identifier (unused)
 * @param oid       Full object instance identifier (unused)
 * @param value     Location for the value (OUT)
 * @param name      Variable name
 *
 * @return Status code
 */
static te_errno
env_get(unsigned int gid, const char *oid, char *value,
        const char *name)
{
    const char *tmp = getenv(name);

    UNUSED(gid);
    UNUSED(oid);

    if (!env_is_hidden(name, -1) && (tmp != NULL))
    {
        if (strlen(tmp) >= RCF_MAX_VAL)
            WARN("Environment variable '%s' value truncated", name);
        snprintf(value, RCF_MAX_VAL, "%s", tmp);
        return 0;
    }
    else
    {
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }
}

/**
 * Change already existing Environment variable.
 *
 * @param gid       Request's group identifier (unused)
 * @param oid       Full object instance identifier (unused)
 * @param value     New value to set
 * @param name      Variable name
 *
 * @return Status code
 */
static te_errno
env_set(unsigned int gid, const char *oid, const char *value,
        const char *name)
{
    UNUSED(gid);
    UNUSED(oid);

    if (env_is_hidden(name, -1))
        return TE_RC(TE_TA_UNIX, TE_EPERM);

    if (setenv(name, value, TRUE) == 0)
    {
        return 0;
    }
    else
    {
        te_errno rc = TE_OS_RC(TE_TA_UNIX, errno);

        ERROR("Failed to set Environment variable '%s' to '%s'; errno %r",
              name, value, rc);
        return rc;
    }
}

/**
 * Add a new Environment variable.
 *
 * @param gid       Request's group identifier (unused)
 * @param oid       Full object instance identifier (unused)
 * @param value     Value
 * @param name      Variable name
 *
 * @return Status code
 */
static te_errno
env_add(unsigned int gid, const char *oid, const char *value,
        const char *name)
{
    UNUSED(gid);
    UNUSED(oid);

    if (env_is_hidden(name, -1))
        return TE_RC(TE_TA_UNIX, TE_EPERM);

    if (getenv(name) == NULL)
    {
        if (setenv(name, value, FALSE) == 0)
        {
            return 0;
        }
        else
        {
            te_errno rc = TE_OS_RC(TE_TA_UNIX, errno);

            ERROR("Failed to add Environment variable '%s=%s'",
                  name, value);
            return rc;
        }
    }
    else
    {
        return TE_RC(TE_TA_UNIX, TE_EEXIST);
    }
}

/**
 * Delete Environment variable.
 *
 * @param gid       Request's group identifier (unused)
 * @param oid       Full object instance identifier (unused)
 * @param name      Variable name
 *
 * @return Status code
 */
static te_errno
env_del(unsigned int gid, const char *oid, const char *name)
{
    UNUSED(gid);
    UNUSED(oid);

    if (env_is_hidden(name, -1))
        return TE_RC(TE_TA_UNIX, TE_EPERM);

    if (getenv(name) != NULL)
    {
        unsetenv(name);
        return 0;
    }
    else
    {
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }
}

/**
 * Get instance list for object "/agent/env".
 *
 * @param gid       Request's group identifier (unused)
 * @param oid       Full object instance identifier (unused)
 * @param list      Location for the list pointer
 *
 * @return Status code
 */
static te_errno
env_list(unsigned int gid, const char *oid, char **list)
{
    char const * const *env;

    char   *ptr = buf;
    char   *buf_end = buf + sizeof(buf);

    UNUSED(gid);
    UNUSED(oid);

    if (environ == NULL)
        return 0;

    *ptr = '\0';
    for (env = environ; *env != NULL; ++env)
    {
        char    *s = strchr(*env, '=');
        ssize_t  name_len;

        if (s == NULL)
        {
            ERROR("Invalid Environment entry format: %s", *env);
            return TE_RC(TE_TA_UNIX, TE_EFMT);
        }
        name_len = s - *env;
        if (env_is_hidden(*env, name_len))
            continue;

        if (ptr != buf)
            *ptr++ = ' ';
        if ((buf_end - ptr) <= name_len)
        {
            ERROR("Too small buffer for the list of Environment "
                  "variables");
            return TE_RC(TE_TA_UNIX, TE_ESMALLBUF);
        }
        memcpy(ptr, *env, name_len);
        ptr += name_len;
        *ptr = '\0';
    }

    if ((*list = strdup(buf)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    return 0;
}

/**
 * Get instance list for object "agent/user".
 *
 * @param id            full identifier of the father instance
 * @param list          location for the list pointer
 *
 * @return              Status code:
 * @retval 0                success
 * @retval TE_ENOMEM        cannot allocate memory
 */
static te_errno
user_list(unsigned int gid, const char *oid, char **list)
{
    FILE *f;
    char *s = buf;

    UNUSED(gid);
    UNUSED(oid);

    if ((f = fopen("/etc/passwd", "r")) == NULL)
    {
        te_errno rc = TE_OS_RC(TE_TA_UNIX, errno);

        ERROR("Failed to open file /etc/passwd; errno %r", rc);
        return rc;
    }

    buf[0] = 0;

    while (fgets(trash, sizeof(trash), f) != NULL)
    {
        char *tmp = strstr(trash, TE_USER_PREFIX);
        char *tmp1;

        unsigned int uid;

        if (tmp == NULL)
            continue;

        tmp += strlen(TE_USER_PREFIX);
        uid = strtol(tmp, &tmp1, 10);
        if (tmp1 == tmp || *tmp1 != ':')
            continue;
        s += sprintf(s, TE_USER_PREFIX "%u", uid);
    }
    fclose(f);

    if ((*list = strdup(buf)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    return 0;
}

/** Check, if user with the specified name exists */
static te_bool
user_exists(const char *user)
{
    FILE *f;

    if ((f = fopen("/etc/passwd", "r")) == NULL)
    {
        ERROR("Failed to open file /etc/passwd; errno %d", errno);
        return FALSE;
    }

    while (fgets(trash, sizeof(trash), f) != NULL)
    {
        char *tmp = strstr(trash, user);

        if (tmp == NULL)
            continue;

        if (*(tmp + strlen(user)) == ':')
        {
            fclose(f);
            return TRUE;
        }
    }
    fclose(f);

    return FALSE;
}

/**
 * Add tester user.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         value string (unused)
 * @param user          user name: te_tester_<uid>
 *
 * @return              Status code
 */
static te_errno
user_add(unsigned int gid, const char *oid, const char *value,
         const char *user)
{
    char *tmp;
    char *tmp1;

    unsigned int uid;

    te_errno     rc;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);

    if (user_exists(user))
        return TE_RC(TE_TA_UNIX, TE_EEXIST);

    if (strncmp(user, TE_USER_PREFIX, strlen(TE_USER_PREFIX)) != 0)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    tmp = (char *)user + strlen(TE_USER_PREFIX);
    uid = strtol(tmp, &tmp1, 10);
    if (tmp == tmp1 || *tmp1 != 0)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    /* 
     * We manually add group to be independent from system settings
     * (one group for all users / each user with its group)
     */
    sprintf(buf, "/usr/sbin/groupadd -g %u %s ", uid, user);
    if ((rc = ta_system(buf)) != 0)
    {
        ERROR("\"%s\" command failed with %d", buf, rc);
        return TE_RC(TE_TA_UNIX, TE_ESHCMD);
    }
    sprintf(buf, "/usr/sbin/useradd -d /tmp/%s -g %u -u %u -m %s ",
            user, uid, uid, user);
    if ((rc = ta_system(buf)) != 0)
    {
        ERROR("\"%s\" command failed with %d", buf, rc);
        return TE_RC(TE_TA_UNIX, TE_ESHCMD);
    }

    sprintf(buf, "echo %s:%s | /usr/sbin/chpasswd", user, user);
    if ((rc = ta_system(buf)) != 0)
    {
        ERROR("\"%s\" command failed with %d", buf, rc);
        user_del(gid, oid, user);
        return TE_RC(TE_TA_UNIX, TE_ESHCMD);
    }

    /* Fedora has very aggressive nscd cache */
    /* https://bugzilla.redhat.com/bugzilla/show_bug.cgi?id=134323 */
    ta_system("/usr/sbin/nscd -i group && /usr/sbin/nscd -i passwd");

    sprintf(buf, "su - %s -c 'ssh-keygen -t dsa -N \"\" "
                 "-f /tmp/%s/.ssh/id_dsa' >/dev/null 2>&1", user, user);

    if ((rc = ta_system(buf)) != 0)
    {
        ERROR("\"%s\" command failed with %d", buf, rc);
        user_del(gid, oid, user);
        return TE_RC(TE_TA_UNIX, TE_ESHCMD);
    }

    return 0;
}

/**
 * Delete tester user.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param user          user name
 *
 * @return              Status code
 */
static te_errno
user_del(unsigned int gid, const char *oid, const char *user)
{
    te_errno rc;

    UNUSED(gid);
    UNUSED(oid);

    if (!user_exists(user))
        return TE_RC(TE_TA_UNIX, TE_EEXIST);

    sprintf(buf, "/usr/sbin/userdel -r %s", user);
    if ((rc = ta_system(buf)) != 0)
    {
        ERROR("\"%s\" command failed with %d", buf, rc);
        return TE_RC(TE_TA_UNIX, TE_ESHCMD);
    }
    sprintf(buf, "/usr/sbin/groupdel %s", user);
    if ((rc = ta_system(buf)) != 0)
    {
        /* Yes, we ignore rc, as group may be deleted by userdel */
        VERB("\"%s\" command failed with %d", buf, rc);
    }

    /* Fedora has very aggressive nscd cache */
    /* https://bugzilla.redhat.com/bugzilla/show_bug.cgi?id=134323 */
    ta_system("/usr/sbin/nscd -i group && /usr/sbin/nscd -i passwd");

    return 0;
}
