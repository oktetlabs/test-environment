/** @file
 * @brief Unix Test Agent
 *
 * Unix TA configuring support
 *
 *
 * Copyright (C) 2004 Test Environment authors (see file AUTHORS
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
#include "config.h"

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <sys/ioctl.h>
#if HAVE_NET_IF_DL_H
#include <net/if_dl.h>
#endif
#include <net/if_arp.h>
#include <net/route.h>
#if HAVE_NET_ETHERNET_H
#include <net/ethernet.h>
#endif
#include <arpa/inet.h>

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


#if !defined(__linux__) && \
    (defined(USE_NETLINK) || defined(USE_NETLINK_ROUTE))
#error netlink can be used on Linux only
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
    struct nlmsghdr      h;
};
#endif

/** Check that interface is locked for using of this TA */
#define INTERFACE_IS_MINE(ifname) \
    (strcmp(ifname, "lo") == 0 || \
     rcf_pch_rsrc_accessible("/agent:%s/interface:%s", ta_name, ifname))

/**
 * Type for both IPv4 and IPv6 address
 */
typedef union gen_ip_address {
    uint32_t        ip4_addr;  /** IPv4 address */
    struct in6_addr ip6_addr;  /** IPv6 address */
} gen_ip_address;

const char *te_lockdir = "/tmp";

/* Auxiliary variables used for during configuration request processing */
static struct ifreq req;

static char buf[4096];
static char trash[128];
static int  cfg_socket = -1;

/*
 * Access routines prototypes (comply to procedure types
 * specified in rcf_ch_api.h).
 */
static int env_get(unsigned int, const char *, char *,
                   const char *);
static int env_set(unsigned int, const char *, const char *,
                   const char *);
static int env_add(unsigned int, const char *, const char *,
                   const char *);
static int env_del(unsigned int, const char *,
                   const char *);
static int env_list(unsigned int, const char *, char **);

/** Environment variables hidden in list operation */
static const char * const env_hidden[] = {
    "SSH_CLIENT",
    "SSH_CONNECTION",
    "SUDO_COMMAND",
    "TE_RPC_PORT",
    "TE_LOG_PORT",
    "TARPC_DL_NAME",
    "TCE_CONNECTION"
};

static int ip4_fw_get(unsigned int, const char *, char *);
static int ip4_fw_set(unsigned int, const char *, const char *);

static int interface_list(unsigned int, const char *, char **);
static int interface_add(unsigned int, const char *, const char *,
                         const char *);
static int interface_del(unsigned int, const char *, const char *);

static int net_addr_add(unsigned int, const char *, const char *,
                        const char *, const char *);
static int net_addr_del(unsigned int, const char *,
                        const char *, const char *);
static int net_addr_list(unsigned int, const char *, char **,
                         const char *);

static int prefix_get(unsigned int, const char *, char *,
                       const char *, const char *);
static int prefix_set(unsigned int, const char *, const char *,
                       const char *, const char *);

static int broadcast_get(unsigned int, const char *, char *,
                         const char *, const char *);
static int broadcast_set(unsigned int, const char *, const char *,
                         const char *, const char *);

static int link_addr_get(unsigned int, const char *, char *,
                         const char *);

static int ifindex_get(unsigned int, const char *, char *,
                       const char *);

static int status_get(unsigned int, const char *, char *,
                      const char *);
static int status_set(unsigned int, const char *, const char *,
                      const char *);

static int arp_get(unsigned int, const char *, char *,
                   const char *);
static int arp_set(unsigned int, const char *, const char *,
                   const char *);

static int mtu_get(unsigned int, const char *, char *,
                   const char *);
static int mtu_set(unsigned int, const char *, const char *,
                   const char *);

static int neigh_state_get(unsigned int, const char *, char *,
                           const char *, const char *);
static int neigh_get(unsigned int, const char *, char *,
                     const char *, const char *);

static int neigh_set(unsigned int, const char *, const char *,
                     const char *, const char *);
static int neigh_add(unsigned int, const char *, const char *,
                     const char *, const char *);
static int neigh_del(unsigned int, const char *,
                     const char *, const char *);
static int neigh_list(unsigned int, const char *, char **, const char *);

/* 
 * This is a bit of hack - there are same handlers for static and dynamic
 * branches, handler discovers dynamic subtree by presence of
 * "dynamic" in OID. But list method does not contain the last subid.
 */
static int
neigh_dynamic_list(unsigned int gid, const char *oid, char **list, 
                   const char *ifname)
{
    UNUSED(oid);
    
    return neigh_list(gid, "dynamic", list, ifname);
}                   

static int route_mtu_get(unsigned int, const char *, char *,
                         const char *);
static int route_mtu_set(unsigned int, const char *, const char *,
                         const char *);
static int route_win_get(unsigned int, const char *, char *,
                         const char *);
static int route_win_set(unsigned int, const char *, const char *,
                         const char *);
static int route_irtt_get(unsigned int, const char *, char *,
                          const char *);
static int route_irtt_set(unsigned int, const char *, const char *,
                          const char *);
static int route_dev_get(unsigned int, const char *, char *,
                         const char *);
static int route_dev_set(unsigned int, const char *, const char *,
                         const char *);
static int route_get(unsigned int, const char *, char *, const char *);
static int route_set(unsigned int, const char *, const char *,
                     const char *);
static int route_add(unsigned int, const char *, const char *,
                     const char *);
static int route_del(unsigned int, const char *,
                     const char *);
static int route_list(unsigned int, const char *, char **);

static int route_commit(unsigned int gid, const cfg_oid *p_oid);

static int nameserver_get(unsigned int, const char *, char *,
                          const char *, ...);

static int user_list(unsigned int, const char *, char **);
static int user_add(unsigned int, const char *, const char *, const char *);
static int user_del(unsigned int, const char *, const char *);

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
        (rcf_ch_cfg_list)route_list, (rcf_ch_cfg_commit)route_commit,
        NULL};

RCF_PCH_CFG_NODE_RO(node_dns, "dns",
                    NULL, &node_route,
                    (rcf_ch_cfg_list)nameserver_get);

RCF_PCH_CFG_NODE_RO(node_neigh_state, "state",
                    NULL, NULL,
                    (rcf_ch_cfg_list)neigh_state_get);

static rcf_pch_cfg_object node_neigh_dynamic =
    { "neigh_dynamic", 0, &node_neigh_state, &node_route,
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

RCF_PCH_CFG_NODE_RO(node_link_addr, "link_addr", NULL, &node_arp,
                    link_addr_get);

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

        rcf_pch_rsrc_info("/agent/interface", 
                          rcf_pch_rsrc_grab_dummy,
                          rcf_pch_rsrc_release_dummy);

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
 * @return error code
 */
static int
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
 * @return error code
 */
static int
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
 * @param who          address pointer
 * @param n            address info to be stored
 * @arg                location for nlmsg list
 *
 * @return error code
 */
static int
store_nlmsg(const struct sockaddr_nl *who,
            const struct nlmsghdr *n,
            void *arg)
{
    struct nlmsg_list **linfo = (struct nlmsg_list**)arg;
    struct nlmsg_list *h;
    struct nlmsg_list **lp;

    h = malloc(n->nlmsg_len+sizeof(void*));
    if (h == NULL)
        return -1;

    memcpy(&h->h, n, n->nlmsg_len);
    h->next = NULL;

    for (lp = linfo; *lp; lp = &(*lp)->next);
    *lp = h;

    ll_remember_index(who, n, NULL);
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

    for (cur = linfo; cur; cur = next)
    {
        next = cur->next;
        free (cur);
    }
    return;
}

/**
 * Get link/protocol addresses information
 *
 * @param family   AF_INET, AF_INET6 or AF_LOCAL
 * @param list     location for nlmsg list
 *                 containing address information
 *
 * @return error code
 */
static int
ip_addr_get(int family, struct nlmsg_list **list)
{
    struct rtnl_handle rth;
    int                type;

    memset(&rth, 0, sizeof(rth));
    if (rtnl_open(&rth, 0) < 0)
    {
        ERROR("%s: rtnl_open() failed, %s",
              __FUNCTION__, strerror(errno));
        return TE_OS_RC(TE_TA_UNIX, errno);
    }

    ll_init_map(&rth);
    type = (family == AF_INET) || (family == AF_INET6) ?
            RTM_GETADDR : RTM_GETLINK;

    if (rtnl_wilddump_request(&rth, family, type) < 0)
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
 * Find name of the interface with specified address.
 *
 * @param str_addr  Address in dotted notation
 * @param ifname    Name the interface or NULL
 * @param addr      Location for the address or NULL
 * @param prefix    Location for prefix or NULL
 * @param bcast     Location for broadcast address or NULL
 *
 * @return pointer to interface name in ll_map static buffer or NULL
 */
static const char *
nl_find_net_addr(const char *str_addr, const char *ifname,
                 gen_ip_address *addr, unsigned int *prefix,
                 uint32_t *bcast)
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

    for (a = ainfo;  a; a = a->next)
    {
        n = &a->h;
        ifa = NLMSG_DATA(n);

        if (n->nlmsg_len < NLMSG_LENGTH(sizeof(ifa)))
        {
            ERROR("%s(): Bad netlink mesg hdr length", __FUNCTION__);
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
                 ip_addr.ip4_addr)) || 
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
        ifindex = ifa->ifa_index;
        if (family == AF_INET)
        {
            if (addr != NULL)
                addr->ip4_addr = ip_addr.ip4_addr;
            if (prefix != NULL)
                *prefix = ifa->ifa_prefixlen;
            if (bcast != NULL)
            {
                *bcast = (rta_tb[IFA_BROADCAST]) ?
                         *(uint32_t *)RTA_DATA(rta_tb[IFA_BROADCAST]) :
                         htonl(INADDR_BROADCAST);
            }
        }
        else
        {
            if (addr != NULL)
                memcpy(&addr->ip6_addr, &ip_addr.ip6_addr,
                       sizeof(struct in6_addr));
            if (bcast != NULL)
                *bcast = 0;
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
 * @return error code
 */
static int
nl_ip_addr_add_del(int cmd, const char *ifname,
                   int family, gen_ip_address *addr,
                   unsigned int prefix, uint32_t bcast)
{
    int                 rc;
    struct rtnl_handle  rth;
    struct {
        struct nlmsghdr  n;
        struct ifaddrmsg ifa;
        char             buf[256];
    } req;

    inet_prefix  lcl;
    inet_prefix  brd;

#define AF_INET_DEFAULT_BITLEN   32
#define AF_INET_DEFAULT_BYTELEN  4
#define AF_INET6_DEFAULT_BITLEN 128
#define AF_INET6_DEFAULT_BYTELEN 16

    if (family == AF_INET6)
    {
        bcast = 0;
        prefix = 0;
    }

    ENTRY("cmd=%d ifname=%s addr=0x%x prefix=%u bcast=0x%x",
          cmd, ifname, addr, prefix, bcast);

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
        brd.data[0] = bcast;
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
 * @return error code
 */
static int
nl_ip_addr_modify(enum net_addr_ops cmd,
                   const char *ifname, const char *addr,
                   unsigned int *new_prefix, uint32_t *new_bcast)
{
    unsigned int    prefix = 0;
    uint32_t        bcast = 0;
    int             rc = 0;
    int             family;
    gen_ip_address  ip_addr;

    /* If address contains ';', it is IPv6 address */
    family = (strchr(addr, ':') == NULL) ? AF_INET : AF_INET6;

    if (cmd == NET_ADDR_ADD)
    {
        if (((family == AF_INET) &&
             (inet_pton(family, addr, &ip_addr.ip4_addr) <= 0))
          || ((family == AF_INET6) &&
              (inet_pton(family, addr, &ip_addr.ip6_addr) <= 0)))
        {
            ERROR("Failed to convert address '%s' from string", addr);
            return TE_RC(TE_TA_UNIX, TE_EINVAL);
        }
    }
    else
    {
        if (((family == AF_INET) && 
              (nl_find_net_addr(addr, ifname, &ip_addr,
                                &prefix, &bcast) == NULL)) ||
             ((family == AF_INET6) &&
              (nl_find_net_addr(addr, ifname, &ip_addr,
                                NULL, NULL) == NULL)))
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
                                &ip_addr, prefix, bcast);
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
static int
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
static int
set_prefix(const char *ifname, unsigned int prefix)
{
    uint32_t mask = PREFIX2MASK(prefix);

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
 * @return TRUE if the interface exists or FALSE otherwise
 */
static int
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
            return TRUE;
        }
    }

    fclose(f);

    return FALSE;
}


/**
 * Get instance list for object "agent/interface".
 *
 * @param gid           group identifier (unused)
 * @param oid           full identifier of the father instance
 * @param list          location for the list pointer
 *
 * @return error code
 * @retval 0            success
 * @retval TE_ENOMEM       cannot allocate memory
 */
static int
interface_list(unsigned int gid, const char *oid, char **list)
{
    size_t off = 0;

    UNUSED(gid);
    UNUSED(oid);

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
                
            if (!INTERFACE_IS_MINE(s))
                continue;

            off += snprintf(buf + off, sizeof(buf) - off, "%s ", s);
        }

        fclose(f);
    }
#else
    {
        struct if_nameindex *ifs = if_nameindex();
        struct if_nameindex *p;

        if (ifs != NULL)
        {
            for (p = ifs; (p->if_name != NULL) && (off < sizeof(buf)); ++p)
            {
                if (!INTERFACE_IS_MINE(p->if_name))
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
    else if (off > 0)
        buf[off - 1]  = '\0';

    if ((*list = strdup(buf)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

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
 * @return error code
 */
static int
interface_add(unsigned int gid, const char *oid, const char *value,
              const char *ifname)
{
    char    *devname;
    char    *vlan;
    char    *tmp;
    uint16_t vid;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);

    if (interface_exists(ifname))
        return TE_RC(TE_TA_UNIX, TE_EEXIST);

    if ((devname = strdup(ifname)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    if ((vlan = strchr(devname, '.')) == NULL)
    {
        free(devname);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }
    *vlan++ = 0;
    
    if (!INTERFACE_IS_MINE(devname))
    {
        free(devname);
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }

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
 * @return error code
 */
static int
interface_del(unsigned int gid, const char *oid, const char *ifname)
{
    char *devname;
    char *vlan;
    
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

    if (!INTERFACE_IS_MINE(devname))
    {
        free(devname);
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
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
 * @return error code
 */
static int
ifindex_get(unsigned int gid, const char *oid, char *value,
            const char *ifname)
{
    unsigned int ifindex = if_nametoindex(ifname);;
    
    UNUSED(gid);
    UNUSED(oid);

    if (!INTERFACE_IS_MINE(ifname))
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

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
 * @return error code
 */
#ifdef USE_IOCTL
#ifdef USE_IFCONFIG
static int
net_addr_add(unsigned int gid, const char *oid, const char *value,
             const char *ifname, const char *addr)
{
    unsigned int  new_addr;
    unsigned int  tmp_addr;
    int           rc;
    char         *cur;
    char         *next;
    char          slots[32] = { 0, };

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);

    if (strlen(ifname) >= IF_NAMESIZE)
        return TE_RC(TE_TA_UNIX, TE_E2BIG);

    if (!INTERFACE_IS_MINE(ifname))
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

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
static int
net_addr_add(unsigned int gid, const char *oid, const char *value,
             const char *ifname, const char *addr)
{
    uint32_t     new_addr;
    int           rc;
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

    if (!INTERFACE_IS_MINE(ifname))
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

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
static int
net_addr_add(unsigned int gid, const char *oid, const char *value,
             const char *ifname, const char *addr)
{
    const char     *name;
    unsigned int    prefix;
    char           *end;
    uint32_t        mask;
    uint32_t        broadcast = 0;

    int             family;
    gen_ip_address  ip_addr;

    UNUSED(gid);
    UNUSED(oid);

    if (!INTERFACE_IS_MINE(ifname))
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    /* Check that address has not been assigned to any interface yet*/
    name = nl_find_net_addr(addr, NULL, &ip_addr, NULL, NULL);
    if (name != NULL)
    {
        ERROR("%s(): Address '%s' already exists on interface '%s'",
              __FUNCTION__, addr, name);
        return TE_RC(TE_TA_UNIX, TE_EEXIST);
    }

    family = (strchr(addr, ':') == NULL) ? AF_INET : AF_INET6;

    /* Validate address to be added */    
    if ((family == AF_INET &&
        (inet_pton(family, addr, &ip_addr.ip4_addr) <= 0 ||
         ip_addr.ip4_addr == 0 ||
         ntohl(ip_addr.ip4_addr) & 0xe0000000) == 0xe0000000) ||
        (family == AF_INET6 &&
         inet_pton(family, addr, &ip_addr.ip6_addr) <= 0))
    {
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    /* Validate specified address prefix */
    prefix = strtol(value, &end, 10);
    if (value == end)
    {
        ERROR("Invalid value '%s' of prefix length", value);
        return TE_RC(TE_TA_UNIX, TE_EFMT);
    }
    if (((family == AF_INET) && (prefix > 32)) ||
       ((family == AF_INET6) && (prefix > 128)))
    {
        ERROR("Invalid prefix '%s' to be set", value);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }
    if (family == AF_INET)
    {
        if (prefix == 0)
        {
            /* Use default prefix in the cast of 0 */
            mask = ((ip_addr.ip4_addr) & htonl(0x80000000)) == 0 ?
                   htonl(0xFF000000) :
                   ((ip_addr.ip4_addr) & htonl(0xC0000000)) ==
                   htonl(0x80000000) ?
                   htonl(0xFFFF0000) : htonl(0xFFFFFF00);
            MASK2PREFIX(ntohl(mask), prefix);
        }
        else
        {
            mask = htonl(PREFIX2MASK(prefix));
        }
        /* Prepare broadcast address to be set */
        broadcast = (~mask) | ip_addr.ip4_addr;
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
    uint32_t      int_addr;
    unsigned int  tmp_addr;
    char         *cur;
    char         *next;
    int           rc;

    if (ifname != NULL &&
        (strlen(ifname) >= IF_NAMESIZE || strchr(ifname, ':') != NULL ||
         !INTERFACE_IS_MINE(ifname)))
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
 * @return error code
 */

#ifdef USE_IOCTL
#ifdef USE_IFCONFIG
static int
net_addr_del(unsigned int gid, const char *oid,
             const char *ifname, const char *addr)
{
    char *name;

    UNUSED(gid);
    UNUSED(oid);

    /* FIXME */
    if (strlen(ifname) >= IF_NAMESIZE || strchr(ifname, ':') != NULL ||
        !INTERFACE_IS_MINE(ifname))
    {
        /* Alias does not exist from Configurator point of view */
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
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
static int
net_addr_del(unsigned int gid, const char *oid,
             const char *ifname, const char *addr)
{
    char              *name;
    struct sockaddr_in sin;

    UNUSED(gid);
    UNUSED(oid);

    /* FIXME */
    if (strlen(ifname) >= IF_NAMESIZE || strchr(ifname, ':') != NULL ||
        !INTERFACE_IS_MINE(ifname))
    {
        /* Alias does not exist from Configurator point of view */
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
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
static int
net_addr_del(unsigned int gid, const char *oid,
             const char *ifname, const char *addr)
{
    UNUSED(gid);
    UNUSED(oid);

    if (!INTERFACE_IS_MINE(ifname))
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

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
 * @return error code
 * @retval 0                success
 * @retval TE_ENOENT        no such instance
 * @retval TE_ENOMEM        cannot allocate memory
 */


#ifdef USE_NETLINK
static int
net_addr_list(unsigned int gid, const char *oid, char **list,
              const char *ifname)
{
    int                len = ADDR_LIST_BULK;
    int                rc;
    struct nlmsg_list *ainfo = NULL;
    struct nlmsg_list *a6info = NULL;
    struct nlmsg_list *n = NULL;
    int                ifindex;
    int                inet6_addresses = 0;

    UNUSED(gid);
    UNUSED(oid);
    
    /* FIXME */
    if (strlen(ifname) >= IF_NAMESIZE || strchr(ifname, ':') != NULL ||
        !INTERFACE_IS_MINE(ifname))
    {
        /* Alias does not exist from Configurator point of view */
        return FALSE;
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
            return -1;
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
static int
net_addr_list(unsigned int gid, const char *oid, char **list,
              const char *ifname)
{
    struct ifconf conf;
    struct ifreq *req;
    char         *name = NULL;
    uint32_t      tmp_addr;
    int           len = ADDR_LIST_BULK;

    UNUSED(gid);
    UNUSED(oid);

    /* FIXME */
    if (strlen(ifname) >= IF_NAMESIZE || strchr(ifname, ':') != NULL ||
        !INTERFACE_IS_MINE(ifname))
    {
        /* Alias does not exist from Configurator point of view */
        return FALSE;
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
 * @return error code
 */
static int
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
 * @return error code
 */
static int
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
 * @param addr          IPv4 address in dotted notation
 *
 * @return error code
 */
static int
broadcast_get(unsigned int gid, const char *oid, char *value,
            const char *ifname, const char *addr)
{
    uint32_t    bcast = 0;

    UNUSED(gid);
    UNUSED(oid);

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
    bcast = SIN(&req.ifr_addr)->sin_addr.s_addr;
#else
#error Way to work with network addresses is not defined.
#endif

    if (inet_ntop(AF_INET, &bcast, value, RCF_MAX_VAL) == NULL)
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
 * @param addr          IPv4 address in dotted notation
 *
 * @return error code
 */
static int
broadcast_set(unsigned int gid, const char *oid, const char *value,
            const char *ifname, const char *addr)
{
    uint32_t bcast = 0;

    UNUSED(gid);
    UNUSED(oid);

    if (inet_pton(AF_INET, value, (void *)&bcast) <= 0 ||
        bcast == 0 ||
        (ntohl(bcast) & 0xe0000000) == 0xe0000000)
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
        SIN(&(req.ifr_addr))->sin_addr.s_addr = bcast;
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
 * @return error code
 */
static int
link_addr_get(unsigned int gid, const char *oid, char *value,
              const char *ifname)
{
    uint8_t *ptr = NULL;

    UNUSED(gid);
    UNUSED(oid);

    if (!INTERFACE_IS_MINE(ifname))
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

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
 * Get MTU of the interface.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         value location
 * @param ifname        name of the interface (like "eth0")
 *
 * @return error code
 */
static int
mtu_get(unsigned int gid, const char *oid, char *value,
        const char *ifname)
{
    UNUSED(gid);
    UNUSED(oid);

    if (!INTERFACE_IS_MINE(ifname))
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

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
 * @return error code
 */
static int
mtu_set(unsigned int gid, const char *oid, const char *value,
        const char *ifname)
{
    char    *tmp1;

    UNUSED(gid);
    UNUSED(oid);

    if (!INTERFACE_IS_MINE(ifname))
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    req.ifr_mtu = strtol(value, &tmp1, 10);
    if (tmp1 == value || *tmp1 != 0)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    strcpy(req.ifr_name, ifname);
    if (ioctl(cfg_socket, SIOCSIFMTU, (int)&req) != 0)
    {
        te_errno rc = TE_OS_RC(TE_TA_UNIX, errno);
        
        ERROR("ioctl(SIOCSIFMTU) failed: %r", rc);
        return rc;
    }

    return 0;
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
 * @return error code
 */
static int
arp_get(unsigned int gid, const char *oid, char *value, const char *ifname)
{

    UNUSED(gid);
    UNUSED(oid);

    if (!INTERFACE_IS_MINE(ifname))
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

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
 * @return error code
 */
static int
arp_set(unsigned int gid, const char *oid, const char *value,
        const char *ifname)
{
    UNUSED(gid);
    UNUSED(oid);

    if (!INTERFACE_IS_MINE(ifname))
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

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
 * @return error code
 */
static int
status_get(unsigned int gid, const char *oid, char *value,
           const char *ifname)
{
    UNUSED(gid);
    UNUSED(oid);

    if (!INTERFACE_IS_MINE(ifname))
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

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
 * @return error code
 */
static int
status_set(unsigned int gid, const char *oid, const char *value,
           const char *ifname)
{
    UNUSED(gid);
    UNUSED(oid);

    if (!INTERFACE_IS_MINE(ifname))
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

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

/** Find neighbour entry and return its parameters */
static te_errno
neigh_find(const char *oid, const char *ifname, const char *addr,
           char *mac_p, unsigned int *flags_p)
{
    te_bool  volatile_entry = strstr(oid, "dynamic") != NULL;
    FILE    *fp;
    char     device[32];
    char     mac[MAC_ADDR_LEN * 3];
    
    unsigned int flags;
    
    if (!INTERFACE_IS_MINE(ifname))
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if ((fp = fopen("/proc/net/arp", "r")) == NULL)
    {
        ERROR("Failed to open /proc/net/arp for reading: %s",
              strerror(errno));
        return TE_OS_RC(TE_TA_UNIX, errno);
    }

    while (fscanf(fp, "%s", buf) != EOF)
    {
        if (strcmp(buf, addr) == 0)
        {
            if (fscanf(fp, "%s %x %s %s %s", buf, &flags, mac, 
                       trash, device) != 5)
            {
                fclose(fp);
                ERROR("Failed to parse ARP entry values");
                return TE_RC(TE_TA_UNIX, TE_EFMT);
            }
            fclose(fp);
            
            if (strcmp(ifname, device) != 0)
                return TE_RC(TE_TA_UNIX, TE_ENOENT);

            if (flags == 0)
            {
                return TE_RC(TE_TA_UNIX, TE_ENOENT);
            }
            else
            {
                if (volatile_entry == !!(flags & ATF_PERM))
                {
                    ERROR("%s ARP entry %s ATF_PERM flag",
                          volatile_entry ? "Volatile" : "Non-volatile",
                          (flags & ATF_PERM) ? "has" : "does not have");
                    return TE_RC(TE_TA_UNIX, TE_EFAULT);
                }
                
                if (flags_p != NULL)
                    *flags_p = flags;
                    
                if (mac_p != NULL)
                    strcpy(mac_p, mac);

                return 0;
            }
        }
        fgets(buf, sizeof(buf), fp);
    }
    fclose(fp);

    return TE_RC(TE_TA_UNIX, TE_ENOENT);
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
    unsigned int flags;
    int          rc;
    char         mac[MAC_ADDR_LEN * 3];
    
    UNUSED(gid);
    UNUSED(oid);
    
    if ((rc = neigh_find("dynamic", ifname, addr, mac, &flags)) != 0)
        return rc;
        
    /* TODO: extract state via netlink */
    if (flags & ATF_COM)
        sprintf(value, "%u", CS_NEIGH_REACHABLE);
    else if (strcmp(mac, "00:00:00:00:00:00") == 0)
        sprintf(value, "%u", CS_NEIGH_INCOMPLETE);
    else
        sprintf(value, "%u", CS_NEIGH_STALE);
        
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
static int
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
static int
neigh_set(unsigned int gid, const char *oid, const char *value,
          const char *ifname, const char *addr)
{
    if (neigh_find(oid, ifname, addr, NULL, NULL) != 0)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    return neigh_add(gid, oid, value, ifname, addr);
}

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
static int
neigh_add(unsigned int gid, const char *oid, const char *value,
          const char *ifname, const char *addr)
{
    te_bool       volatile_entry = strstr(oid, "dynamic") != NULL;
    struct arpreq arp_req;
    int           int_addr[MAC_ADDR_LEN];
    int           res;
    int           i;

    UNUSED(gid);
    
    /* TODO: check that address corresponds to interface */
    UNUSED(ifname);

    res = sscanf(value, "%2x:%2x:%2x:%2x:%2x:%2x%s",
                 int_addr, int_addr + 1, int_addr + 2, int_addr + 3,
                 int_addr + 4, int_addr + 5, trash);

    if (res != 6)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    memset(&arp_req, 0, sizeof(arp_req));
    arp_req.arp_pa.sa_family = AF_INET;

    if (inet_pton(AF_INET, addr, &SIN(&(arp_req.arp_pa))->sin_addr) <= 0)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    arp_req.arp_ha.sa_family = AF_LOCAL;
    for (i = 0; i < 6; i++)
        (arp_req.arp_ha.sa_data)[i] = (unsigned char)(int_addr[i]);

    arp_req.arp_flags = ATF_COM;
    if (!volatile_entry)
        arp_req.arp_flags |= ATF_PERM;

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
static int
neigh_del(unsigned int gid, const char *oid, const char *ifname, 
          const char *addr)
{
    struct arpreq arp_req;
    int           rc;

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

    memset(&arp_req, 0, sizeof(arp_req));
    arp_req.arp_pa.sa_family = AF_INET;
    if (inet_pton(AF_INET, addr, &SIN(&(arp_req.arp_pa))->sin_addr) <= 0)
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
static int
neigh_list(unsigned int gid, const char *oid, char **list, 
           const char *ifname)
{
#ifdef __linux__
    te_bool  volatile_entry = strstr(oid, "dynamic") != NULL;
    char    *ptr = buf;
    FILE    *fp;
    
    if (!INTERFACE_IS_MINE(ifname))
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if ((fp = fopen("/proc/net/arp", "r")) == NULL)
    {
        ERROR("Failed to open /proc/net/arp for reading: %s",
              strerror(errno));
        return TE_OS_RC(TE_TA_UNIX, errno);
    }

    fgets(trash, sizeof(trash), fp);
    while (fscanf(fp, "%s", ptr) != EOF)
    {
        unsigned int flags = 0;
        char         device[32];

        fscanf(fp, "%s %x %s %s %s", trash, &flags, 
               trash, trash, device);
        fgets(trash, sizeof(trash), fp);
               
        if (strcmp(device, ifname) != 0)
        {
            *ptr = 0;
            continue;
        }
        
        if ((flags & ATF_COM) &&
            (volatile_entry != !!(flags & ATF_PERM)))
        {
            sprintf(ptr + strlen(ptr), " ");
            ptr += strlen(ptr);
        }
        else
            *ptr = 0;

    }
    fclose(fp);
#else
    UNUSED(oid);

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

#ifdef USE_NETLINK_ROUTE

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
        req->r.rtm_tos = rt_info->tos;
    
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
static int
route_change(ta_rt_info_t *rt_info, int action, unsigned flags)
{
    struct nl_request  req;
    struct rtnl_handle rth;
    int                rc;

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


#else /* !USE_NETLINK_ROUTE */

/** 
 * Convert system-independent route info data structure to
 * ioctl-specific rtentry data structure.
 *
 * @param rt_info TA portable route info
 * @Param rt      ioctl-specific data structure (OUT)
 */
static void
rt_info2rtentry(const ta_rt_info_t *rt_info,
#ifdef __linux__
                struct rtentry  *rt
#else
                struct ortentry *rt
#endif
)
{
    assert(rt_info != NULL && rt != NULL);

    memset(rt, 0, sizeof(*rt));
    memcpy(&(rt->rt_dst), &(rt_info->dst), sizeof(rt->rt_dst));

#ifdef __linux__
    rt->rt_genmask.sa_family = SIN(rt_info)->sin_family;
    ((struct sockaddr_in *)&(rt->rt_genmask))->sin_addr.s_addr =
        htonl(PREFIX2MASK(rt_info->prefix));
#endif
    if ((rt_info->dst.ss_family == AF_INET && rt_info->prefix == 32) ||
        (rt_info->dst.ss_family == AF_INET6 && rt_info->prefix == 128))
        rt->rt_flags |= RTF_HOST;

    if ((rt_info->flags & TA_RT_INFO_FLG_GW) != 0)
    {
        memcpy(&(rt->rt_gateway), &(rt_info->gw), sizeof(rt->rt_gateway));
        rt->rt_flags |= RTF_GATEWAY;
    }

#ifdef __linux__
    if ((rt_info->flags & TA_RT_INFO_FLG_IF) != 0)
    {
        rt->rt_dev = strdup(rt_info->ifname);
    }

    if ((rt_info->flags & TA_RT_INFO_FLG_METRIC) != 0)
        rt->rt_metric = rt_info->metric;

    if ((rt_info->flags & TA_RT_INFO_FLG_MTU) != 0)
    {
        rt->rt_mtu = rt_info->mtu;
        rt->rt_flags |= RTF_MSS;
    }

    if ((rt_info->flags & TA_RT_INFO_FLG_WIN) != 0)
    {
        rt->rt_window = rt_info->win;
        rt->rt_flags |= RTF_WINDOW;
    }

    if ((rt_info->flags & TA_RT_INFO_FLG_IRTT) != 0)
    {
        rt->rt_irtt = rt_info->irtt;
        rt->rt_flags |= RTF_IRTT;
    }

#endif /* !__linux__ */
}
#endif /* !USE_NETLINK_ROUTE */

#ifdef USE_NETLINK_ROUTE

/** Structure used for RTNL user callback */
typedef struct rtnl_cb_user_data {
    ta_rt_info_t *rt_info; /**< Routing entry information (IN/OUT)
                                On input it keeps route key,
                                On output it is augmented with route
                                attributes: mtu, win etc. */
    int if_index;          /**< Interface index in case of direct route.
                                This field has sense  only if 
                                TA_RT_INFO_FLG_IF flag is set */
    int rc;                /**< Return code */
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
    struct rtattr       *tb[RTA_MAX + 1] = {};
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
          user_data->rt_info->prefix == r->rtm_dst_len
#if 0
             &&
            /* Check that type of service is correct */
             ((user_data->rt_info->flags & TA_RT_INFO_FLG_TOS) != 0 &&
             user_data->rt_info->tos == r->rtm_tos)
#endif             
             ))            
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
            struct rtattr *mxrta[RTAX_MAX + 1] = {};

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
#endif /* USE_NETLINK_ROUTE */

/**
 * Find route and return its attributes.
 *
 * @param route    route instance name: see doc/cm_cm_base.xml
 *                 for the format
 * @param rt_info  route related information (OUT)
 *
 * @return error code
 */
static int
route_find(const char *route, ta_rt_info_t *rt_info)
{
#ifdef __linux__
    int       rc;
#ifndef USE_NETLINK_ROUTE
    FILE     *fp;
    char      ifname[IF_NAMESIZE];
    uint32_t  route_addr;
    uint32_t  route_mask;
    uint32_t  route_gw;
    char     *route_table;
    char      ip4_route_table[] = "/proc/net/route";
    char      ip6_route_table[] = "/proc/net/ipv6_route";
#endif /* !USE_NETLINK_ROUTE */

    ENTRY("%s", route);

    if ((rc = ta_rt_parse_inst_name(route, rt_info)) != 0)
    {
        ERROR("Error parsing instance name: %s", route);
        return rc;
    }

#ifdef USE_NETLINK_ROUTE

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
#if 0    
    if ((rt_info->flags & TA_RT_INFO_FLG_IF) != 0)
    {
        if ((user_data.if_index = ll_name_to_index(rt_info->ifname)) == 0)
        {
            rtnl_close(&rth);
            ERROR("Cannot find device %s", rt_info->ifname);
            return TE_OS_RC(TE_TA_UNIX, TE_ENOENT);
        }
    }
#endif
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

#else

    route_addr = ((struct sockaddr_in *)&(rt_info->dst))->sin_addr.s_addr;
    route_mask = htonl(PREFIX2MASK(rt_info->prefix));
    route_gw = ((struct sockaddr_in *)&(rt_info->gw))->sin_addr.s_addr;

    if (SIN(&rt_info->dst)->sin_family == AF_INET)
    {
        route_table = ip4_route_table;
    }
    else if (SIN(&rt_info->dst)->sin_family == AF_INET6)
    {
        ERROR("Retrieving IPv6 routing table is not yet supported");
        return TE_OS_RC(TE_TA_UNIX, TE_EINVAL);
#if 0        
        route_table = ip6_route_table;
#endif        
    }

    if ((fp = fopen(route_table, "r")) == NULL)
    {
        ERROR("Failed to open %s for reading: %s", route_table,
               strerror(errno));
        return TE_OS_RC(TE_TA_UNIX, errno);
    }

    fgets(trash, sizeof(trash), fp);
    while (fscanf(fp, "%s", ifname) != EOF)
    {
        uint32_t     addr;
        uint32_t     mask;
        uint32_t     gateway = 0;
        unsigned int flags = 0;
        unsigned int metric;
        int          mtu;
        int          win;
        int          irtt;

        fscanf(fp, "%x %x %x %*d %*d %d %x %d %d %d", &addr, &gateway,
               &flags, &metric, &mask, &mtu, &win, &irtt);
        VERB("%s: Route %s %x %x %x %d %d %d %x %d %d %d", __FUNCTION__,
             ifname, addr, gateway, flags, 0, 0, metric, mask,
             mtu, win, irtt);

        if (((rt_info->flags & TA_RT_INFO_FLG_IF) != 0 &&
             strcmp(rt_info->ifname, ifname) != 0) ||
            addr != route_addr ||
            gateway != route_gw || mask != route_mask)
        {
            fgets(trash, sizeof(trash), fp);
            VERB("Continue processing ...");
            continue;
        }

        if ((flags & RTF_UP) == 0)
            break;

        fclose(fp);

        rt_info->metric = metric;
        if (metric != 0)
            rt_info->flags |= TA_RT_INFO_FLG_METRIC;

        rt_info->mtu = mtu;
        if (mtu != 0)
            rt_info->flags |= TA_RT_INFO_FLG_MTU;

        rt_info->win = win;
        if (win != 0)
            rt_info->flags |= TA_RT_INFO_FLG_WIN;

        rt_info->irtt = irtt;
        if (irtt != 0)
            rt_info->flags |= TA_RT_INFO_FLG_IRTT;
        
        return 0;
    }

    fclose(fp);

    return TE_RC(TE_TA_UNIX, TE_ENOENT);
#endif /* !USE_NETLINK_ROUTE */

#else

    UNUSED(route);
    UNUSED(rt_info);

    return TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP);
#endif /* !__linux__ */
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
 * @return error code
 */
static int
route_get(unsigned int gid, const char *oid,
          char *value, const char *route_name)
{
    ta_rt_info_t  attr;
    int           rc;

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
 * @return      error code.
 */
static int
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
static int
route_load_attrs(ta_cfg_obj_t *obj)
{
    ta_rt_info_t rt_info;
    int          rc;
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
static int                                                  \
route_ ## field_ ## _get(unsigned int gid, const char *oid, \
                         char *value, const char *route)    \
{                                                           \
    int          rc;                                        \
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
static int                                                     \
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

static int                                       
route_dev_get(unsigned int gid, const char *oid,
              char *value, const char *route) 
{
    int          rc;
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
 * @return error code
 */
static int
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
 * @return error code
 */
static int
route_del(unsigned int gid, const char *oid, const char *route)
{
    UNUSED(gid);
    UNUSED(oid);

    return ta_obj_del(TA_OBJ_TYPE_ROUTE, route, NULL);
}

/**
 * Get instance list for object "agent/route".
 *
 * @param id            full identifier of the father instance
 * @param list          location for the list pointer
 *
 * @return error code
 * @retval 0                       success
 * @retval TE_ENOENT               no such instance
 * @retval TE_ENOMEM               cannot allocate memory
 */
static int
route_list(unsigned int gid, const char *oid, char **list)
{
    char *ptr = buf;
    char *end_ptr = buf + sizeof(buf);
    char  ifname[RCF_MAX_NAME];
    FILE *fp;

    UNUSED(gid);
    UNUSED(oid);

    ENTRY();

    buf[0] = '\0';

#ifdef __linux__
    if ((fp = fopen("/proc/net/route", "r")) == NULL)
    {
        ERROR("Failed to open /proc/net/route for reading: %s",
              strerror(errno));
        return TE_OS_RC(TE_TA_UNIX, errno);
    }

    fgets(trash, sizeof(trash), fp);
    while (fscanf(fp, "%s", ifname) != EOF)
    {
        unsigned int addr;
        unsigned int mask;
        uint32_t     gateway = 0;
        unsigned int flags = 0;
        unsigned int prefix = 0;
        int          metric;
        int          mtu;
        int          win;
        int          irtt;

        fscanf(fp, "%x %x %x %*d %*d %d %x %d %d %d", &addr, &gateway,
               &flags, &metric, &mask, &mtu, &win, &irtt);

        if (flags & RTF_UP)
        {
            MASK2PREFIX(ntohl(mask), prefix);

            snprintf(ptr, end_ptr - ptr, "%d.%d.%d.%d|%d",
                    ((uint8_t *)&addr)[0], ((uint8_t *)&addr)[1],
                    ((uint8_t *)&addr)[2], ((uint8_t *)&addr)[3], prefix);
            ptr += strlen(ptr);

            if (metric > 0)
            {
                snprintf(ptr, end_ptr - ptr, ",metric=%d", metric);
                ptr += strlen(ptr);
            }

            snprintf(ptr, end_ptr - ptr, " ");
            ptr += strlen(ptr);
        }
        fgets(trash, sizeof(trash), fp);
    }
    fclose(fp);

    VERB("Got IPv4 routes");

#if 1
    if ((fp = fopen("/proc/net/ipv6_route", "r")) == NULL)
    {
        ERROR("Failed to open /proc/net/ipv6_route for reading: %s",
              strerror(errno));
        return TE_OS_RC(TE_TA_UNIX, errno);
    }
    
#define IPV6_RAW_STR_LEN  33
#define IPV6_RAW_PRINT(addr, _ptr)                                    \
do {                                                                  \
    char *p;                                                          \
    int   i = 0;                                                      \
                                                                      \
    /* Print the address */                                           \
    for (p = addr; isxdigit(*p); p += 4 - i)                          \
    {                                                                 \
        /*                                                            \
         * If it is not the first quad of digits,                     \
         * print a colon before it                                    \
         */                                                           \
        if (p != addr && end_ptr >= _ptr)                             \
        {                                                             \
            snprintf(_ptr, end_ptr - _ptr, ":");                      \
            _ptr++;                                                   \
        }                                                             \
        for (i = 0; (i < 3) && (*p == '0'); i++, p++);                \
                                                                      \
        /* Do not print more than 4 digits at once */                 \
        if (end_ptr >= _ptr)                                          \
            snprintf(_ptr, MIN(5 - i, end_ptr - _ptr), p);            \
        _ptr += strlen(_ptr);                                         \
    }                                                                 \
} while(0)    
   
    {        
        char         mask[IPV6_RAW_STR_LEN];
        char         dst[IPV6_RAW_STR_LEN];
        char         gate[IPV6_RAW_STR_LEN];
        uint32_t     prefix = 0;
        uint32_t     flags = 0;
        int          metric;
        
        while (fscanf(fp, "%s %x %s %*x %s %x %*x %*x %x %s", dst, &prefix,
                      mask, gate, &metric, &flags, ifname) >0)
        {
            if (flags & RTF_UP)
            {
                IPV6_RAW_PRINT(dst, ptr);
                snprintf(ptr, end_ptr - ptr, "|%d", prefix);
                ptr += strlen(ptr);

                if (metric > 0)
                {
                    snprintf(ptr, end_ptr - ptr, ",metric=%d", metric);
                    ptr += strlen(ptr);
                }

                snprintf(ptr, end_ptr - ptr, " ");
                ptr += strlen(ptr);
            }
        }
        fgets(trash, sizeof(trash), fp);
        fclose(fp);
    }
#endif

#else
    UNUSED(ptr);
    UNUSED(end_ptr);
    UNUSED(ifname);
    UNUSED(fp);
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
 * @return error code
 */
static int
route_commit(unsigned int gid, const cfg_oid *p_oid)
{
    const char   *route;
    ta_cfg_obj_t *obj;
    ta_rt_info_t  rt_info_name_only;
    ta_rt_info_t  rt_info;
    int           rc = 0;
    
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

#ifdef USE_NETLINK_ROUTE
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
#else
    /* Use ioctl interface */
#ifdef __linux__
    {
        struct rtentry rt;

        rt_info2rtentry(&rt_info, &rt);

        switch (obj_action)
        {
            case TA_CFG_OBJ_DELETE:
            {
                if (ioctl(cfg_socket, SIOCDELRT, &rt) < 0)
                {
                    rc = TE_OS_RC(TE_TA_UNIX, errno);

                    ERROR("ioctl(SIOCDELRT) failed: %r", rc);
                    return rc;
                }
                return 0;
            }
            
            case TA_CFG_OBJ_SET:
            {
                struct rtentry rt_cur;

                /*
                 * In case of SET we first should delete an existing 
                 * route and then add a new one.
                 */
                rt_info2rtentry(&rt_info_name_only, &rt_cur);

                if (ioctl(cfg_socket, SIOCDELRT, &rt_cur) < 0)
                {
                    rc = TE_OS_RC(TE_TA_UNIX, errno);

                    ERROR("ioctl(SIOCDELRT) failed: %r", rc);
                    return rc;
                }
                 
                /* FALLTHROUGH */
            }

            case TA_CFG_OBJ_CREATE:
            {
                /* Add or set operation */
                if (rt.rt_metric != 0)
                {
                    /*
                     * Increment metric because ioctl substracts 
                     * one from the value ('route' command does 
                     * the same thing).
                     */
                    rt.rt_metric++;
                }

                rt.rt_flags |= (RTF_UP | RTF_STATIC);

                if (ioctl(cfg_socket, SIOCADDRT, &rt) < 0)
                {
                    rc = TE_OS_RC(TE_TA_UNIX, errno);

                    ERROR("ioctl(SIOCADDRT) failed: %r", rc);
                    return rc;
                }
                break;
            }
            
            default:
                ERROR("Unknown object action specified %d", obj_action);
                return TE_RC(TE_TA_UNIX, TE_EINVAL);
        }
    }
#else
    return TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP);
#endif

#endif /* !USE_NETLINK_ROUTE */
    
    return rc;
}



static int
nameserver_get(unsigned int gid, const char *oid, char *result,
               const char *instance, ...)
{
    FILE *resolver = NULL;
    char  buf[256];
    char *found = NULL, *endaddr = NULL;
    int   rc = TE_RC(TE_TA_UNIX, TE_ENOENT);

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
        if((found = strstr(buf, "nameserver")) != NULL)
        {
            found += strcspn(found, ip_symbols);
            if(*found != '\0')
            {
                endaddr = found + strspn(found, ip_symbols);
                *endaddr = '\0';
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
 * @return Error code
 */
static int
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
 * @return Error code
 */
static int
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
 * @return Error code
 */
static int
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
 * @return Error code
 */
static int
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
 * @return Error code
 */
static int
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
 * @return error code
 * @retval 0            success
 * @retval TE_ENOMEM       cannot allocate memory
 */
static int
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
 * @return error code
 */
static int
user_add(unsigned int gid, const char *oid, const char *value,
         const char *user)
{
    char *tmp;
    char *tmp1;

    unsigned int uid;

    int rc;

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
 * @return error code
 */
static int
user_del(unsigned int gid, const char *oid, const char *user)
{
    int rc;

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
