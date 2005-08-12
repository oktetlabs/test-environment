/** @file
 * @brief Linux Test Agent
 *
 * Linux TA configuring support
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

#define TE_LGR_USER     "Linux Conf"

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
#include "logger_ta.h"
#include "comm_agent.h"
#include "rcf_ch_api.h"
#include "rcf_pch.h"
#include "logger_api.h"
#include "linux_internal.h"

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


#ifdef CFG_LINUX_DAEMONS
extern int linuxconf_daemons_init(rcf_pch_cfg_object **last);
extern void linux_daemons_release();
#endif

#ifdef ENABLE_WIFI_SUPPORT
extern int linuxconf_wifi_init(rcf_pch_cfg_object **last);
#endif

#ifdef USE_NETLINK
struct nlmsg_list
{
    struct nlmsg_list *next;
    struct nlmsghdr      h;
};
#endif

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
    "TE_RPC_PORT"
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

static int arp_use_get(unsigned int, const char *, char *,
                       const char *);
static int arp_use_set(unsigned int, const char *, const char *,
                       const char *);

static int mtu_get(unsigned int, const char *, char *,
                   const char *);
static int mtu_set(unsigned int, const char *, const char *,
                   const char *);

static int arp_get(unsigned int, const char *, char *,
                   const char *, const char *);
static int arp_set(unsigned int, const char *, const char *,
                   const char *, const char *);
static int arp_add(unsigned int, const char *, const char *,
                   const char *, const char *);
static int arp_del(unsigned int, const char *,
                   const char *, const char *);
static int arp_list(unsigned int, const char *, char **);

static int route_get(unsigned int, const char *, char *,
                     const char *);
static int route_set(unsigned int, const char *, const char *,
                     const char *);
static int route_add(unsigned int, const char *, const char *,
                     const char *);
static int route_del(unsigned int, const char *,
                     const char *);
static int route_list(unsigned int, const char *, char **);

static int nameserver_get(unsigned int, const char *, char *,
                          const char *, ...);

static int user_list(unsigned int, const char *, char **);
static int user_add(unsigned int, const char *, const char *, const char *);
static int user_del(unsigned int, const char *, const char *);

/* Linux Test Agent configuration tree */

/* Volatile subtree */
static rcf_pch_cfg_object node_volatile_arp =
    { "arp", 0, NULL, NULL,
      (rcf_ch_cfg_get)arp_get, (rcf_ch_cfg_set)arp_set,
      (rcf_ch_cfg_add)arp_add, (rcf_ch_cfg_del)arp_del,
      (rcf_ch_cfg_list)arp_list, NULL, NULL};

RCF_PCH_CFG_NODE_NA(node_volatile, "volatile", &node_volatile_arp, NULL);

/* Non-volatile subtree */
static rcf_pch_cfg_object node_route =
    { "route", 0, NULL, &node_volatile,
      (rcf_ch_cfg_get)route_get, (rcf_ch_cfg_set)route_set,
      (rcf_ch_cfg_add)route_add, (rcf_ch_cfg_del)route_del,
      (rcf_ch_cfg_list)route_list, NULL, NULL};

static rcf_pch_cfg_object node_arp =
    { "arp", 0, NULL, &node_route,
      (rcf_ch_cfg_get)arp_get, (rcf_ch_cfg_set)arp_set,
      (rcf_ch_cfg_add)arp_add, (rcf_ch_cfg_del)arp_del,
      (rcf_ch_cfg_list)arp_list, NULL, NULL};

RCF_PCH_CFG_NODE_RO(node_dns, "dns",
                    NULL, &node_arp,
                    (rcf_ch_cfg_list)nameserver_get);

RCF_PCH_CFG_NODE_RW(node_status, "status", NULL, NULL,
                    status_get, status_set);

RCF_PCH_CFG_NODE_RW(node_mtu, "mtu", NULL, &node_status,
                    mtu_get, mtu_set);

RCF_PCH_CFG_NODE_RW(node_arp_use, "arp", NULL, &node_mtu,
                    arp_use_get, arp_use_set);

RCF_PCH_CFG_NODE_RO(node_link_addr, "link_addr", NULL, &node_arp_use,
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
#ifdef USE_NETLINK
    struct rtnl_handle rth;
#endif
#ifdef CFG_LINUX_DAEMONS
    rcf_pch_cfg_object *tail = &node_volatile;
    
    if (!init && tail->brother != NULL)
    {
        ERROR("The last element in configuration tree has brother, "
              "which is very strange - you must have forgotten to "
              "update 'tail' variable in %s:%d", __FILE__, __LINE__);
        return NULL;
    }

#endif

    if (!init)
    {
        init = TRUE;

#ifdef ENABLE_WIFI_SUPPORT
        rcf_pch_cfg_object *agt_if_tail = &node_status;

        if (agt_if_tail->brother != NULL)
        {
            ERROR("The last element in '/agent/interface' subtree "
                  "has brother, which is very strange - you must have "
                  "forgotten to replace '%s' variable in %s:%d",
                  agt_if_tail->sub_id, __FILE__, __LINE__);
            return NULL;
        }

        if (linuxconf_wifi_init(&agt_if_tail) != 0)
        {
            return NULL;
        }
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

#ifdef CFG_LINUX_DAEMONS
        if (linuxconf_daemons_init(&tail) != 0)
        {
            close(cfg_socket);
            return NULL;
        }
        assert(tail->brother == NULL);
#endif
#ifdef USE_NETLINK
        memset(&rth, 0, sizeof(rth));
        if (rtnl_open(&rth, 0) < 0)
        {
            ERROR("Failed to open a netlink socket");
            return NULL;
        }

        ll_init_map(&rth);
        rtnl_close(&rth);
#endif

#ifdef RCF_RPC
        /* Link RPC nodes */
        rcf_pch_rpc_init();
#endif
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
#ifdef CFG_LINUX_DAEMONS
    linux_daemons_release();
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
            return TE_OS_RC(TE_TA_LINUX, errno);

        if (read(fd, &c, 1) < 0)
        {
            close(fd);
            return TE_OS_RC(TE_TA_LINUX, errno);
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

    if ((*value != '0' && *value != '1') || *(value + 1) != 0)
        return TE_RC(TE_TA_LINUX, TE_EINVAL);

    fd = open("/proc/sys/net/ipv4/ip_forward",
              O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd < 0)
        return TE_OS_RC(TE_TA_LINUX, errno);

    if (write(fd, *value == '0' ? "0\n" : "1\n", 2) < 0)
    {
        close(fd);
        return TE_OS_RC(TE_TA_LINUX, errno);
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
 * @param family   AF_INET or AF_LOCAL
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
        return TE_OS_RC(TE_TA_LINUX, errno);
    }

    ll_init_map(&rth);
    type = (family == AF_INET) ? RTM_GETADDR : RTM_GETLINK;

    if (rtnl_wilddump_request(&rth, family, type) < 0)
    {
        ERROR("%s: Cannot send dump request, %s",
              __FUNCTION__, strerror(errno));
        rtnl_close(&rth);
        return TE_OS_RC(TE_TA_LINUX, errno);
    }

    if (rtnl_dump_filter(&rth, store_nlmsg, list, NULL, NULL) < 0)
    {
        ERROR("%s: Dump terminated, %s",
              __FUNCTION__, strerror(errno));
        rtnl_close(&rth);
        return TE_OS_RC(TE_TA_LINUX, errno);
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
                 uint32_t *addr, unsigned int *prefix, uint32_t *bcast)
{
    uint32_t           int_addr;
    struct nlmsg_list *ainfo = NULL;
    struct nlmsg_list *a = NULL;
    struct nlmsghdr   *n = NULL;
    struct ifaddrmsg  *ifa = NULL;
    struct rtattr     *rta_tb[IFA_MAX + 1];
    int                ifindex = 0;


    if (ifname != NULL && (strlen(ifname) >= IF_NAMESIZE))
    {
        ERROR("Interface name '%s' too long", ifname);
        return NULL;
    }

    if (inet_pton(AF_INET, str_addr, (void *)&int_addr) <= 0)
    {
        ERROR("%s(): inet_pton() failed for address '%s'",
              __FUNCTION__, str_addr);
        return NULL;
    }

    if (ip_addr_get(AF_INET, &ainfo) != 0)
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
            if (*(unsigned int *)(RTA_DATA(rta_tb[IFA_LOCAL])) ==
                int_addr)
            {
                if (ifname == NULL ||
                    (ll_name_to_index(ifname) == ifa->ifa_index))
                    break;

                WARN("Interfaces '%s' and '%s' have the same address '%s'",
                     ifname, ll_index_to_name(ifa->ifa_index), str_addr);
            }
        }
    }

    if (a != NULL)
    {
        ifindex = ifa->ifa_index;
        if (addr != NULL)
            *addr   = int_addr;
        if (prefix != NULL)
            *prefix = ifa->ifa_prefixlen;
        if (bcast != NULL)
            *bcast = (rta_tb[IFA_BROADCAST]) ?
                     *(uint32_t *)RTA_DATA(rta_tb[IFA_BROADCAST]) :
                     htonl(INADDR_BROADCAST);
    }

    free_nlmsg(ainfo);

    return (a == NULL) ? NULL :
           (ifname != NULL) ? ifname :
                              (const char *)ll_index_to_name(ifindex);
}


/**
 * Add/delete AF_INET address.
 *
 * @param cmd           Command (see enum net_addr_ops)
 * @param ifname        Interface name
 * @param addr          Address
 * @param prefix        Prefix to set or 0
 * @param bcast         Broadcast to set or 0
 *
 * @return error code
 */
static int
nl_ip4_addr_add_del(int cmd, const char *ifname, uint32_t addr,
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

    ENTRY("cmd=%d ifname=%s addr=0x%x prefix=%u bcast=0x%x",
          cmd, ifname, addr, prefix, bcast);

    memset(&req, 0, sizeof(req));
    memset(&lcl, 0, sizeof(lcl));
    memset(&brd, 0, sizeof(brd));
    memset(&rth, 0, sizeof(rth));

    lcl.family = AF_INET;
    lcl.bytelen = AF_INET_DEFAULT_BYTELEN;
    lcl.bitlen = (prefix != 0) ? prefix : AF_INET_DEFAULT_BITLEN;
    lcl.data[0] = addr;

    req.n.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifaddrmsg));
    req.n.nlmsg_flags = NLM_F_REQUEST;
    req.n.nlmsg_type = cmd;
    req.ifa.ifa_family = AF_INET;
    req.ifa.ifa_prefixlen = lcl.bitlen;

    addattr_l(&req.n, sizeof(req), IFA_LOCAL, &lcl.data, lcl.bytelen);

    if (bcast != 0)
    {
        brd.family = AF_INET;
        brd.bytelen = AF_INET_DEFAULT_BYTELEN;
        brd.bitlen = lcl.bitlen;
        brd.data[0] = bcast;
        addattr_l(&req.n, sizeof(req), IFA_BROADCAST,
                  &brd.data, brd.bytelen);
    }

    memset(&rth, 0, sizeof(rth));
    if (rtnl_open(&rth, 0) < 0)
    {
        rc = TE_OS_RC(TE_TA_LINUX, errno);
        ERROR("%s(): Cannot open netlink socket", __FUNCTION__);
        return rc;
    }

    ll_init_map(&rth);
    req.ifa.ifa_index = ll_name_to_index(ifname);

    if (rtnl_talk(&rth, &req.n, 0, 0, NULL, NULL, NULL) < 0)
    {
        rc = TE_OS_RC(TE_TA_LINUX, errno);
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
 * Modify AF_INET address.
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
nl_ip4_addr_modify(enum net_addr_ops cmd,
                   const char *ifname, const char *addr,
                   unsigned int *new_prefix, uint32_t *new_bcast)
{
    uint32_t        int_addr = 0;
    unsigned int    prefix = 0;
    uint32_t        bcast = 0;
    int             rc = 0;

    if (cmd == NET_ADDR_ADD)
    {
        if (inet_pton(AF_INET, addr, &int_addr) <= 0)
        {
            ERROR("Failed to convert addrss '%s' from string", addr);
            return TE_RC(TE_TA_LINUX, TE_EINVAL);
        }
    }
    else if (nl_find_net_addr(addr, ifname,
                              &int_addr, &prefix, &bcast) == NULL)
    {
        ERROR("Address '%s' on interface '%s' not found", addr, ifname);
        return TE_RC(TE_TA_LINUX, TE_ENOENT);
    }

    if (new_prefix != NULL)
        prefix = *new_prefix;
    if (new_bcast != NULL)
        bcast = *new_bcast;

    if (cmd != NET_ADDR_ADD)
        rc = nl_ip4_addr_add_del(RTM_DELADDR, ifname, int_addr, 0, 0);

    if (rc == 0 && cmd != NET_ADDR_DELETE)
        rc = nl_ip4_addr_add_del(RTM_NEWADDR, ifname, int_addr,
                                 prefix, bcast);

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
        /* It's not always called for correct arguments */
        VERB("ioctl(SIOCGIFADDR) for '%s' failed: %s",
              ifname, strerror(errno));
        return TE_OS_RC(TE_TA_LINUX, errno);
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
        ERROR("ioctl(SIOCSIFNETMASK) failed: %s", strerror(errno));
        return TE_OS_RC(TE_TA_LINUX, errno);
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
        return TE_OS_RC(TE_TA_LINUX, errno);
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
            return TE_OS_RC(TE_TA_LINUX, errno);
        }

        while (fgets(trash, sizeof(trash), f) != NULL)
        {
            char *s = strchr(trash, ':');

            if (s == NULL)
                continue;

            for (*s-- = 0; s != trash && *s != ' '; s--);

            if (*s == ' ')
                s++;

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
                off += snprintf(buf + off, sizeof(buf) - off,
                                "%s ", p->if_name);

            if_freenameindex(ifs);
        }
    }
#endif
    if (off >= sizeof(buf))
        return TE_RC(TE_TA_LINUX, TE_ESMALLBUF);
    else if (off > 0)
        buf[off - 1]  = '\0';

    if ((*list = strdup(buf)) == NULL)
        return TE_RC(TE_TA_LINUX, TE_ENOMEM);

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
        ERROR("ioctl(SIOCGIFCONF) failed: %s", strerror(errno));
        return TE_OS_RC(TE_TA_LINUX, errno);
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
            return TE_OS_RC(TE_TA_LINUX, errno);
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
        return TE_RC(TE_TA_LINUX, TE_EEXIST);

    if ((devname = strdup(ifname)) == NULL)
        return TE_RC(TE_TA_LINUX, TE_ENOMEM);

    if ((vlan = strchr(devname, '.')) == NULL)
    {
        free(devname);
        return TE_RC(TE_TA_LINUX, TE_EINVAL);
    }
    *vlan++ = 0;

    vid = strtol(vlan, &tmp, 10);
    if (tmp == vlan || *tmp != 0 || !interface_exists(devname))
    {
        free(devname);
        return TE_RC(TE_TA_LINUX, TE_EINVAL);
    }

    sprintf(buf, "/sbin/vconfig add %s %d", devname, vid);
    free(devname);

    return ta_system(buf) != 0 ? TE_RC(TE_TA_LINUX, TE_ESHCMD) : 0;
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
    UNUSED(gid);
    UNUSED(oid);

    if (!interface_exists(ifname))
        return TE_RC(TE_TA_LINUX, TE_ENOENT);

    sprintf(buf, "/sbin/vconfig rem %s", ifname);

    return (ta_system(buf) != 0) ? TE_RC(TE_TA_LINUX, TE_ESHCMD) : 0;
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

    if (ifindex == 0)
        return TE_RC(TE_TA_LINUX, TE_ENOENT);

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

    /* FIXME */
    if (strlen(ifname) >= IF_NAMESIZE || strchr(ifname, ':') != NULL)
    {
        /* Alias does not exist from Configurator point of view */
        return FALSE;
    }

    if (inet_pton(AF_INET, addr, (void *)&new_addr) <= 0 ||
        new_addr == 0 ||
        (new_addr & 0xe0000000) == 0xe0000000)
    {
        return TE_RC(TE_TA_LINUX, TE_EINVAL);
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
            return TE_RC(TE_TA_LINUX, TE_EEXIST);

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
            return TE_RC(TE_TA_LINUX, TE_EPERM);

        sprintf(trash, "/sbin/ifconfig %s:%d %s up", ifname, n, addr);
    }

    if (ta_system(trash) != 0)
        return TE_RC(TE_TA_LINUX, TE_ESHCMD);

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

    if (inet_pton(AF_INET, addr, (void *)&new_addr) <= 0 ||
        new_addr == 0 ||
        (ntohl(new_addr) & 0xe0000000) == 0xe0000000)
    {
        return TE_RC(TE_TA_LINUX, TE_EINVAL);
    }

#ifdef __linux__
    /* FIXME */
    if (strlen(ifname) >= IF_NAMESIZE || strchr(ifname, ':') != NULL)
    {
        /* Alias does not exist from Configurator point of view */
        return FALSE;
    }
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
            return TE_RC(TE_TA_LINUX, TE_EEXIST);

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
            return TE_RC(TE_TA_LINUX, TE_EPERM);

        sprintf(trash, "%s:%d", ifname, n);
        strncpy(req.ifr_name, trash, IFNAMSIZ);
    }

    memset(&sin, 0, sizeof(struct sockaddr));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = new_addr;
    memcpy(&req.ifr_addr, &sin, sizeof(struct sockaddr));
    if (ioctl(cfg_socket, SIOCSIFADDR, &req) < 0)
    {
        ERROR("ioctl(SIOCSIFADDR) failed: %s", strerror(errno));
        return TE_OS_RC(TE_TA_LINUX, errno);
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
            return TE_RC(TE_TA_LINUX, TE_EFMT);
        }
        if (ioctl(cfg_socket, SIOCALIFADDR, &lreq) < 0)
        {
            ERROR("ioctl(SIOCALIFADDR) failed: %s", strerror(errno));
            return TE_OS_RC(TE_TA_LINUX, errno);
        }
    }
#else
    ERROR("%s(): %s", __FUNCTION__, strerror(EOPNOTSUPP));
    return TE_RC(TE_TA_LINUX, TE_EOPNOTSUPP);
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
    uint32_t        new_addr;
    unsigned int    prefix;
    char           *end;
    uint32_t        mask;
    uint32_t        broadcast = 0;

    UNUSED(gid);
    UNUSED(oid);

    /* Check that address has not already been assigned to any interface */
    name = nl_find_net_addr(addr, NULL, &new_addr, NULL, NULL);
    if (name != NULL)
    {
        ERROR("%s(): Address '%s' already exists on interface '%s'",
              __FUNCTION__, addr, name);
        return TE_RC(TE_TA_LINUX, TE_EEXIST);
    }

    /* Validate address to be added */
    if (inet_pton(AF_INET, addr, (void *)&new_addr) <= 0 ||
        new_addr == 0 ||
        (ntohl(new_addr) & 0xe0000000) == 0xe0000000)
    {
        return TE_RC(TE_TA_LINUX, TE_EINVAL);
    }

    /* Validate specified address prefix */
    prefix = strtol(value, &end, 10);
    if (value == end)
    {
        ERROR("Invalid value '%s' of prefix length", value);
        return TE_RC(TE_TA_LINUX, TE_EFMT);
    }
    if (prefix > 32)
    {
        ERROR("Invalid prefix '%s' to be set", value);
        return TE_RC(TE_TA_LINUX, TE_EINVAL);
    }
    if (prefix == 0)
    {
        /* Use default prefix in the cast of 0 */
        mask = ((new_addr) & htonl(0x80000000)) == 0 ? htonl(0xFF000000) :
               ((new_addr) & htonl(0xC0000000)) == htonl(0x80000000) ?
               htonl(0xFFFF0000) : htonl(0xFFFFFF00);
        MASK2PREFIX(ntohl(mask), prefix);
    }
    else
    {
        mask = htonl(PREFIX2MASK(prefix));
    }
    /* Prepare broadcast address to be set */
    broadcast = (~mask) | new_addr;

    return nl_ip4_addr_modify(NET_ADDR_ADD, ifname, addr,
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
        (strlen(ifname) >= IF_NAMESIZE || strchr(ifname, ':') != NULL))
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
    if (strlen(ifname) >= IF_NAMESIZE || strchr(ifname, ':') != NULL)
    {
        /* Alias does not exist from Configurator point of view */
        return FALSE;
    }

    if ((name = find_net_addr(ifname, addr)) == NULL)
        return TE_RC(TE_TA_LINUX, TE_ENOENT);

    if (strcmp(name, ifname) == 0)
        sprintf(trash, "/sbin/ifconfig %s 0.0.0.0", ifname);
    else
        sprintf(trash, "/sbin/ifconfig %s down", name);

    return ta_system(trash) != 0 ? TE_RC(TE_TA_LINUX, TE_ESHCMD) : 0;
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
    if (strlen(ifname) >= IF_NAMESIZE || strchr(ifname, ':') != NULL)
    {
        /* Alias does not exist from Configurator point of view */
        return FALSE;
    }

    if ((name = find_net_addr(ifname, addr)) == NULL)
    {
        ERROR("Address %s on interface %s not found", addr, ifname);
        return TE_RC(TE_TA_LINUX, TE_ENOENT);
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
            ERROR("ioctl(SIOCSIFADDR) failed: %s", strerror(errno));
            return TE_OS_RC(TE_TA_LINUX, errno);
        }
    }
    else
    {
        strncpy(req.ifr_name, name, IFNAMSIZ);
        if (ioctl(cfg_socket, SIOCGIFFLAGS, &req) < 0)
        {
            ERROR("ioctl(SIOCGIFFLAGS) failed: %s", strerror(errno));
            return TE_OS_RC(TE_TA_LINUX, errno);
        }

        strncpy(req.ifr_name, name, IFNAMSIZ);
        req.ifr_flags &= ~(IFF_UP | IFF_RUNNING);
        if (ioctl(cfg_socket, SIOCSIFFLAGS, &req) < 0)
        {
            ERROR("ioctl(SIOCSIFFLAGS) failed: %s", strerror(errno));
            return TE_OS_RC(TE_TA_LINUX, errno);
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

    return nl_ip4_addr_modify(NET_ADDR_DELETE, ifname, addr, NULL, NULL);
}
#endif


#define ADDR_LIST_BULK      (INET_ADDRSTRLEN * 4)

/**
 * Get instance list for object "agent/interface/net_addr".
 *
 * @param id            full identifier of the father instance
 * @param list          location for the list pointer
 * @param ifname        interface name
 *
 * @return error code
 * @retval 0                    success
 * @retval TE_ENOENT        no such instance
 * @retval TE_ENOMEM               cannot allocate memory
 */
#ifdef USE_NETLINK
static int
net_addr_list(unsigned int gid, const char *oid, char **list,
              const char *ifname)
{
    int           len = ADDR_LIST_BULK;
    int                rc;
    struct nlmsg_list *ainfo = NULL;
    struct nlmsg_list *n = NULL;
    int                ifindex;

    UNUSED(gid);
    UNUSED(oid);

    /* FIXME */
    if (strlen(ifname) >= IF_NAMESIZE || strchr(ifname, ':') != NULL)
    {
        /* Alias does not exist from Configurator point of view */
        return FALSE;
    }
    *list = (char *)calloc(ADDR_LIST_BULK, 1);
    if (*list == NULL)
    {
        ERROR("calloc() failed");
        return TE_RC(TE_TA_LINUX, TE_ENOMEM);
    }
    rc = ip_addr_get(AF_INET, &ainfo);
    if (rc != 0)
    {
        ERROR("%s: ip_addr_get() failed", __FUNCTION__);
        return rc;
    }

    ifindex = ll_name_to_index(ifname);
    if (ifindex <= 0)
    {
        ERROR("Device \"%s\" does not exist", ifname);
        return TE_RC(TE_TA_LINUX, TE_ENODEV);
    }

    for (n = ainfo; n; n = n->next)
    {
        struct nlmsghdr *hdr = &n->h;
        struct ifaddrmsg *ifa = NLMSG_DATA(hdr);
        struct rtattr * rta_tb[IFA_MAX+1];

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

        if (len - strlen(*list) <= INET_ADDRSTRLEN)
        {
            char *tmp;

            len += ADDR_LIST_BULK;
            tmp = (char *)realloc(*list, len);
            if (tmp == NULL)
            {
                free(*list);
                ERROR("realloc() failed");
                return TE_RC(TE_TA_LINUX, TE_ENOMEM);
            }
            *list = tmp;
        }
        sprintf(*list + strlen(*list), "%d.%d.%d.%d ",
                ((unsigned char *)RTA_DATA(rta_tb[IFA_LOCAL]))[0],
                ((unsigned char *)RTA_DATA(rta_tb[IFA_LOCAL]))[1],
                ((unsigned char *)RTA_DATA(rta_tb[IFA_LOCAL]))[2],
                ((unsigned char *)RTA_DATA(rta_tb[IFA_LOCAL]))[3]);
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
    if (strlen(ifname) >= IF_NAMESIZE || strchr(ifname, ':') != NULL)
    {
        /* Alias does not exist from Configurator point of view */
        return FALSE;
    }
    conf.ifc_len = sizeof(buf);
    conf.ifc_buf = buf;

    memset(buf, 0, sizeof(buf));
    if (ioctl(cfg_socket, SIOCGIFCONF, &conf) < 0)
    {
        ERROR("ioctl(SIOCGIFCONF) failed: %d", errno);
        return TE_OS_RC(TE_TA_LINUX, errno);
    }
    *list = (char *)calloc(ADDR_LIST_BULK, 1);
    if (*list == NULL)
    {
        ERROR("calloc() failed");
        return TE_RC(TE_TA_LINUX, TE_ENOMEM);
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
                return TE_RC(TE_TA_LINUX, TE_ENOMEM);
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
        return TE_RC(TE_TA_LINUX, TE_ENOENT);
    }
#elif defined(USE_IOCTL)
    strncpy(req.ifr_name, ifname, sizeof(req.ifr_name));
    if (inet_pton(AF_INET, addr, &SIN(&req.ifr_addr)->sin_addr) <= 0)
    {
        ERROR("inet_pton() failed");
        return TE_RC(TE_TA_LINUX, TE_EFMT);
    }
    if (ioctl(cfg_socket, SIOCGIFNETMASK, &req) < 0)
    {
        ERROR("ioctl(SIOCGIFNETMASK) failed for if=%s addr=%s: %s",
              ifname, addr, strerror(errno));
        return TE_OS_RC(TE_TA_LINUX, errno);
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
        return TE_RC(TE_TA_LINUX, TE_EFMT);
    }
    if (prefix > 32)
    {
        ERROR("Invalid prefix '%s' to be set", value);
        return TE_RC(TE_TA_LINUX, TE_EINVAL);
    }

#if defined(USE_NETLINK)
    return nl_ip4_addr_modify(NET_ADDR_MODIFY, ifname, addr, &prefix, NULL);
#elif defined(USE_IOCTL)
    {
        const char *name;

        if ((name = find_net_addr(ifname, addr)) == NULL)
        {
            ERROR("Address '%s' on interface '%s' to set prefix not found",
                  addr, ifname);
            return TE_RC(TE_TA_LINUX, TE_ENOENT);
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
    uint32_t    bcast;

    UNUSED(gid);
    UNUSED(oid);

#if defined(USE_NETLINK)
    if (nl_find_net_addr(addr, ifname, NULL, NULL, &bcast) == NULL)
    {
        ERROR("Address '%s' on interface '%s' to get broadcast address "
              "not found", addr, ifname);
        return TE_RC(TE_TA_LINUX, TE_ENOENT);
    }
#elif defined(USE_IOCTL)
    strncpy(req.ifr_name, ifname, sizeof(req.ifr_name));
    if (inet_pton(AF_INET, addr, &SIN(&req.ifr_addr)->sin_addr) <= 0)
    {
        ERROR("inet_pton() failed");
        return TE_RC(TE_TA_LINUX, TE_EFMT);
    }
    if (ioctl(cfg_socket, SIOCGIFBRDADDR, &req) < 0)
    {
        ERROR("ioctl(SIOCGIFBRDADDR) failed for if=%s addr=%s: %s",
              ifname, addr, strerror(errno));
        return TE_OS_RC(TE_TA_LINUX, errno);
    }
    bcast = SIN(&req.ifr_addr)->sin_addr.s_addr;
#else
#error Way to work with network addresses is not defined.
#endif

    if (inet_ntop(AF_INET, &bcast, value, RCF_MAX_VAL) == NULL)
    {
        ERROR("inet_ntop() failed");
        return TE_OS_RC(TE_TA_LINUX, errno);
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
        return TE_RC(TE_TA_LINUX, TE_EINVAL);
    }

#if defined(USE_NETLINK)
    return nl_ip4_addr_modify(NET_ADDR_MODIFY, ifname, addr, NULL, &bcast);
#elif defined(USE_IOCTL)
    {
        const char *name;

        if ((name = find_net_addr(ifname, addr)) == NULL)
        {
            ERROR("Address '%s' on interface '%s' to set broadcast "
                  "not found", addr, ifname);
            return TE_RC(TE_TA_LINUX, TE_ENOENT);
        }

        strcpy(req.ifr_name, name);
        req.ifr_addr.sa_family = AF_INET;
        SIN(&(req.ifr_addr))->sin_addr.s_addr = bcast;
        if (ioctl(cfg_socket, SIOCSIFBRDADDR, (int)&req) < 0)
        {
            ERROR("ioctl(SIOCSIFBRDADDR) failed: %s", strerror(errno));
            return TE_OS_RC(TE_TA_LINUX, errno);
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

#ifdef SIOCGIFHWADDR
    strcpy(req.ifr_name, ifname);
    if (ioctl(cfg_socket, SIOCGIFHWADDR, (int)&req) < 0)
    {
        ERROR("ioctl(SIOCGIFHWADDR) failed: %s", strerror(errno));
        return TE_OS_RC(TE_TA_LINUX, errno);
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
        ERROR("ioctl(SIOCGIFCONF) failed: %s", strerror(errno));
        return TE_OS_RC(TE_TA_LINUX, errno);
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
    return TE_RC(TE_TA_LINUX, TE_EOPNOTSUPP);
#endif
    if (ptr != NULL)
    {
        snprintf(value, RCF_MAX_VAL, "%02x:%02x:%02x:%02x:%02x:%02x",
                 ptr[0], ptr[1], ptr[2], ptr[3], ptr[4], ptr[5]);
    }
    else
    {
        ERROR("Not found link layer address of the interface %s", ifname);
        return TE_RC(TE_TA_LINUX, TE_ENOENT);
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

    strcpy(req.ifr_name, ifname);
    if (ioctl(cfg_socket, SIOCGIFMTU, (int)&req) != 0)
    {
        ERROR("ioctl(SIOCGIFMTU) failed: %s", strerror(errno));
        return TE_OS_RC(TE_TA_LINUX, errno);
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

    req.ifr_mtu = strtol(value, &tmp1, 10);
    if (tmp1 == value || *tmp1 != 0)
        return TE_RC(TE_TA_LINUX, TE_EINVAL);

    strcpy(req.ifr_name, ifname);
    if (ioctl(cfg_socket, SIOCSIFMTU, (int)&req) != 0)
    {
        ERROR("ioctl(SIOCSIFMTU) failed: %s", strerror(errno));
        return TE_OS_RC(TE_TA_LINUX, errno);
    }

    return 0;
}

/**
 * Get ARP use on the interface ("0" - arp disable, "1" - arp enable)
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         value location
 * @param ifname        name of the interface (like "eth0")
 *
 * @return error code
 */
static int
arp_use_get(unsigned int gid, const char *oid, char *value,
            const char *ifname)
{

    UNUSED(gid);
    UNUSED(oid);

    strcpy(req.ifr_name, ifname);
    if (ioctl(cfg_socket, SIOCGIFFLAGS, (int)&req) != 0)
    {
        ERROR("ioctl(SIOCGIFFLAGS) failed: %s", strerror(errno));
        return TE_OS_RC(TE_TA_LINUX, errno);
    }

    sprintf(value, "%d", (req.ifr_flags & IFF_NOARP) != IFF_NOARP);

    return 0;
}

/**
 * Change ARP use on the interface ("0" - arp disable, "1" - arp enable)
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         value location
 * @param ifname        name of the interface (like "eth0")
 *
 * @return error code
 */
static int
arp_use_set(unsigned int gid, const char *oid, const char *value,
            const char *ifname)
{
    UNUSED(gid);
    UNUSED(oid);

    strncpy(req.ifr_name, ifname, IFNAMSIZ);
    if (ioctl(cfg_socket, SIOCGIFFLAGS, &req) < 0)
    {
        ERROR("ioctl(SIOCGIFFLAGS) failed: %s", strerror(errno));
        return TE_OS_RC(TE_TA_LINUX, errno);
    }

    if (strcmp(value, "1") == 0)
        req.ifr_flags &= (~IFF_NOARP);
    else if (strcmp(value, "0") == 0)
        req.ifr_flags |= (IFF_NOARP);
    else
        return TE_RC(TE_TA_LINUX, TE_EINVAL);

    strncpy(req.ifr_name, ifname, IFNAMSIZ);
    if (ioctl(cfg_socket, SIOCSIFFLAGS, &req) < 0)
    {
        ERROR("ioctl(SIOCSIFFLAGS) failed: %s", strerror(errno));
        return TE_OS_RC(TE_TA_LINUX, errno);
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

    strcpy(req.ifr_name, ifname);
    if (ioctl(cfg_socket, SIOCGIFFLAGS, (int)&req) != 0)
    {
        ERROR("ioctl(SIOCGIFFLAGS) failed: %s", strerror(errno));
        return TE_OS_RC(TE_TA_LINUX, errno);
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
#if 0
static int
status_set(unsigned int gid, const char *oid, const char *value,
           const char *ifname)
{
    int status;

    UNUSED(gid);
    UNUSED(oid);

    if (!interface_exists(ifname))
        return TE_RC(TE_TA_LINUX, TE_ENOENT);

    if (strcmp(value, "0") == 0)
        status = 0;
    else if (strcmp(value, "1") == 0)
        status = 1;
    else
        return TE_RC(TE_TA_LINUX, TE_EINVAL);

    sprintf(buf, "/sbin/ifconfig %s %s",
            ifname, status == 1 ? "up" : "down");

    if (ta_system(buf) != 0)
        return TE_RC(TE_TA_LINUX, TE_ESHCMD);

    return 0;
}
#else
static int
status_set(unsigned int gid, const char *oid, const char *value,
           const char *ifname)
{
    UNUSED(gid);
    UNUSED(oid);

    strncpy(req.ifr_name, ifname, IFNAMSIZ);
    if (ioctl(cfg_socket, SIOCGIFFLAGS, &req) < 0)
    {
        ERROR("ioctl(SIOCGIFFLAGS) failed: %s", strerror(errno));
        return TE_OS_RC(TE_TA_LINUX, errno);
    }

    if (strcmp(value, "0") == 0)
        req.ifr_flags &= ~(IFF_UP | IFF_RUNNING);
    else if (strcmp(value, "1") == 0)
        req.ifr_flags |= (IFF_UP | IFF_RUNNING);
    else
        return TE_RC(TE_TA_LINUX, TE_EINVAL);

    strncpy(req.ifr_name, ifname, IFNAMSIZ);
    if (ioctl(cfg_socket, SIOCSIFFLAGS, &req) < 0)
    {
        ERROR("ioctl(SIOCSIFFLAGS) failed: %s", strerror(errno));
        return TE_OS_RC(TE_TA_LINUX, errno);
    }

    return 0;
}
#endif

/**
 * Get ARP entry value (hardware address corresponding to IPv4).
 *
 * @param gid            group identifier (unused)
 * @param oid            full object instence identifier (unused)
 * @param value          location for the value
 *                       (XX:XX:XX:XX:XX:XX is returned)
 * @param addr           IPv4 address in the dotted notation
 * @param addr_volatile  IPv4 address in case of volatile ARP subtree,
 *                       in this case @p addr parameter points to zero
 *                       length string
 *
 * @return error code
 */
static int
arp_get(unsigned int gid, const char *oid, char *value,
        const char *addr, const char *addr_volatile)
{
    te_bool  volatile_entry = FALSE;
    FILE    *fp;

    UNUSED(gid);

    /*
     * Determine which subtree we are working with
     * (volatile or non-volatile).
     */
    if (strstr(oid, node_volatile.sub_id) != NULL)
    {
        /*
         * Volatile subtree, as soon as its instance names are
         * /agent:NAME/volatile:/arp:ADDR,
         * in which case we have:
         * + addr          - "" (empty string 'volatile' instance name);
         * + addr_volatile - "ADDR" (dynamic ARP entry name).
         */
        volatile_entry = TRUE;
        addr = addr_volatile;

        assert(strlen(addr_volatile) > 0);
    }

    if ((fp = fopen("/proc/net/arp", "r")) == NULL)
    {
        ERROR("Failed to open /proc/net/arp for reading: %s",
              strerror(errno));
        return TE_OS_RC(TE_TA_LINUX, errno);
    }

    while (fscanf(fp, "%s", buf) != EOF)
    {
        if (strcmp(buf, addr) == 0)
        {
            unsigned int flags = 0;

            if (fscanf(fp, "%s %x %s", buf, &flags, value) != 3)
            {
                fclose(fp);
                ERROR("Failed to parse ARP entry values");
                return TE_RC(TE_TA_LINUX, TE_EFMT);
            }
            fclose(fp);

            if (flags == 0)
            {
                return TE_RC(TE_TA_LINUX, TE_ENOENT);
            }
            else
            {
                if (!(volatile_entry ^ (flags & ATF_PERM)))
                {
                    ERROR("%s ARP entry %s ATF_PERM flag",
                          volatile_entry ? "Volatile" : "Non-volatile",
                          (flags & ATF_PERM) ? "has" : "does not have");
                    return TE_RC(TE_TA_LINUX, TE_EFAULT);
                }

                return 0;
            }
        }
        fgets(buf, sizeof(buf), fp);
    }

    fclose(fp);

    return TE_RC(TE_TA_LINUX, TE_ENOENT);
}

/**
 * Change already existing ARP entry.
 *
 * @param gid            group identifier
 * @param oid            full object instence identifier (unused)
 * @param value          new value pointer ("XX:XX:XX:XX:XX:XX")
 * @param addr           IPv4 address in the dotted notation
 * @param addr_volatile  IPv4 address in case of volatile ARP subtree,
 *                       in this case @p addr parameter points to zero
 *                       length string
 *
 * @return error code
 */
static int
arp_set(unsigned int gid, const char *oid, const char *value,
        const char *addr, const char *addr_volatile)
{
    char val[RCF_MAX_VAL];

    if (arp_get(gid, oid, val, addr, addr_volatile) != 0)
        return TE_RC(TE_TA_LINUX, TE_ENOENT);

    return arp_add(gid, oid, value, addr, addr_volatile);
}

/**
 * Add a new ARP entry.
 *
 * @param gid            group identifier (unused)
 * @param oid            full object instence identifier (unused)
 * @param value          new entry value pointer ("XX:XX:XX:XX:XX:XX")
 * @param addr           IPv4 address in the dotted notation
 * @param addr_volatile  IPv4 address in case of volatile ARP subtree,
 *                       in this case @p addr parameter points to zero
 *                       length string
 *
 * @return error code
 */
static int
arp_add(unsigned int gid, const char *oid, const char *value,
        const char *addr, const char *addr_volatile)
{
    te_bool       volatile_entry = FALSE;
    struct arpreq arp_req;
    int           int_addr[6];
    int           res;
    int           i;

    UNUSED(gid);

    if (strstr(oid, node_volatile.sub_id) != NULL)
    {
        volatile_entry = TRUE;
        addr = addr_volatile;
    }

    res = sscanf(value, "%2x:%2x:%2x:%2x:%2x:%2x%s",
                 int_addr, int_addr + 1, int_addr + 2, int_addr + 3,
                 int_addr + 4, int_addr + 5, trash);

    if (res != 6)
        return TE_RC(TE_TA_LINUX, TE_EINVAL);

    memset (&arp_req, 0, sizeof(arp_req));
    arp_req.arp_pa.sa_family = AF_INET;

    if (inet_pton(AF_INET, addr, &SIN(&(arp_req.arp_pa))->sin_addr) <= 0)
        return TE_RC(TE_TA_LINUX, TE_EINVAL);

    arp_req.arp_ha.sa_family = AF_LOCAL;
    for (i = 0; i < 6; i++)
        (arp_req.arp_ha.sa_data)[i] = (unsigned char)(int_addr[i]);

    arp_req.arp_flags = ATF_COM;
    if (!volatile_entry)
        arp_req.arp_flags |= ATF_PERM;

#ifdef SIOCSARP
    if (ioctl(cfg_socket, SIOCSARP, &arp_req) < 0)
    {
        ERROR("ioctl(SIOCSARP) failed: %s", strerror(errno));
        return TE_OS_RC(TE_TA_LINUX, errno);
    }

    return 0;
#else
    return TE_RC(TE_TA_LINUX, TE_EOPNOTSUPP);
#endif
}

/**
 * Delete ARP entry.
 *
 * @param gid            group identifier (unused)
 * @param oid            full object instence identifier (unused)
 * @param value          value string (unused)
 * @param addr           IPv4 address in the dotted notation
 * @param addr_volatile  IPv4 address in case of volatile ARP subtree,
 *                       in this case @p addr parameter points to zero
 *                       length string
 *
 * @return error code
 */
static int
arp_del(unsigned int gid, const char *oid,
        const char *addr, const char *addr_volatile)
{
    struct arpreq arp_req;
    char          val[32];
    int           rc;

    UNUSED(gid);

    if ((rc = arp_get(gid, oid, val, addr, addr_volatile)) != 0)
    {
        if (TE_RC_GET_ERROR(rc) == TE_ENOENT)
        {
            WARN("Cannot delete ARP entry: it disappeared");
            rc = 0;
        }
        return rc;
    }

    if (strstr(oid, node_volatile.sub_id) != NULL)
        addr = addr_volatile;

    memset(&arp_req, 0, sizeof(arp_req));
    arp_req.arp_pa.sa_family = AF_INET;
    if (inet_pton(AF_INET, addr, &SIN(&(arp_req.arp_pa))->sin_addr) <= 0)
        return TE_RC(TE_TA_LINUX, TE_EINVAL);

#ifdef SIOCDARP

    if (ioctl(cfg_socket, SIOCDARP, &arp_req) < 0)
    {
        if (errno == ENXIO || errno == ENETDOWN || errno == ENETUNREACH)
            return 0;
        else
            return TE_OS_RC(TE_TA_LINUX, errno);
    }

    return 0;
#else
    return TE_RC(TE_TA_LINUX, TE_EOPNOTSUPP);
#endif
}

/**
 * Get instance list for object "agent/arp" and "agent/volatile/arp".
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier
 * @param list          location for the list pointer
 *
 * @return error code
 */
static int
arp_list(unsigned int gid, const char *oid, char **list)
{
#ifdef __linux__
    te_bool  volatile_entry = FALSE;
    char    *ptr = buf;
    FILE    *fp;

    /*
     * Determine which subtree we are working with
     * (volatile or non-volatile).
     */
    if (strstr(oid, node_volatile.sub_id) != NULL)
        volatile_entry = TRUE;

    if ((fp = fopen("/proc/net/arp", "r")) == NULL)
    {
        ERROR("Failed to open /proc/net/arp for reading: %s",
              strerror(errno));
        return TE_OS_RC(TE_TA_LINUX, errno);
    }

    fgets(trash, sizeof(trash), fp);
    while (fscanf(fp, "%s", ptr) != EOF)
    {
        unsigned int flags = 0;

        fscanf(fp, "%s %x", trash, &flags);
        if ((flags & ATF_COM) &&
            (volatile_entry ^ (flags & ATF_PERM)))
        {
            sprintf(ptr + strlen(ptr), " ");
            ptr += strlen(ptr);
        }
        else
            *ptr = '\0';

        fgets(trash, sizeof(trash), fp);
    }
    fclose(fp);
#else
    UNUSED(oid);

    *buf = '\0';
#endif

    UNUSED(gid);

    if ((*list = strdup(buf)) == NULL)
        return TE_RC(TE_TA_LINUX, TE_ENOMEM);

    return 0;
}

/**
 * Parses instance name and converts its value into routing table entry
 * data structure.
 *
 * @param inst_name  Instance name that keeps route information
 * @param rt         Routing entry location to be filled in (OUT)
 *
 * @note The function is not thread safe - it uses static memory for
 * 'rt_dev' field in 'rt' structure.
 *
 * ATENTION - read the text below!
 * @todo This function is used in both places agent/linux/linuxconf.c
 * and lib/tapi/tapi_cfg.c, which is very ugly!
 * We couldn't find the right place to put it in so that it
 * is accessible from both places. If you modify it you should modify
 * the same function in the second place!
 */
static int
route_parse_inst_name(const char *inst_name,
#ifdef __linux__
                      struct rtentry  *rt
#else
                      struct ortentry *rt
#endif
)
{
    char        *tmp, *tmp1;
    int          prefix;
#ifdef __linux__
    static char  ifname[IF_NAMESIZE];
#endif
    char        *ptr;
    char        *end_ptr;
    char        *term_byte; /* Pointer to the trailing zero byte
                               in 'inst_name' */
    static char  inst_copy[RCF_MAX_VAL];
#ifdef __linux__
    int          int_val;
#endif

    memset(rt, 0, sizeof(*rt));
    strncpy(inst_copy, inst_name, sizeof(inst_copy));
    inst_copy[sizeof(inst_copy) - 1] = '\0';

    if ((tmp = strchr(inst_copy, '|')) == NULL)
        return TE_RC(TE_TA_LINUX, TE_ENOENT);

    *tmp = 0;
    rt->rt_dst.sa_family = AF_INET;
    if (inet_pton(AF_INET, inst_copy,
                  &(((struct sockaddr_in *)&(rt->rt_dst))->sin_addr)) <= 0)
    {
        ERROR("Incorrect 'destination address' value in route %s",
              inst_name);
        return TE_RC(TE_TA_LINUX, TE_ENOENT);
    }
    tmp++;
    if (*tmp == '-' ||
        (prefix = strtol(tmp, &tmp1, 10), tmp == tmp1 || prefix > 32))
    {
        ERROR("Incorrect 'prefix length' value in route %s", inst_name);
        return TE_RC(TE_TA_LINUX, TE_ENOENT);
    }
    tmp = tmp1;

#ifdef __linux__
    rt->rt_genmask.sa_family = AF_INET;
    ((struct sockaddr_in *)&(rt->rt_genmask))->sin_addr.s_addr =
        htonl(PREFIX2MASK(prefix));
#endif
    if (prefix == 32)
        rt->rt_flags |= RTF_HOST;

    term_byte = (char *)(tmp + strlen(tmp));

    /* @todo Make a macro to wrap around similar code below */
    if ((ptr = strstr(tmp, "gw=")) != NULL)
    {
        int rc;

        end_ptr = ptr += strlen("gw=");
        while (*end_ptr != ',' && *end_ptr != '\0')
            end_ptr++;
        *end_ptr = '\0';

        rc = inet_pton(AF_INET, ptr,
                       &(((struct sockaddr_in *)
                               &(rt->rt_gateway))->sin_addr));
        if (rc <= 0)
        {
            ERROR("Incorrect format of 'gateway address' value in route %s",
                  inst_name);
            return TE_RC(TE_TA_LINUX, TE_ENOENT);
        }
        if (term_byte != end_ptr)
            *end_ptr = ',';
        rt->rt_gateway.sa_family = AF_INET;
        rt->rt_flags |= RTF_GATEWAY;
    }

#ifdef __linux__
    if ((ptr = strstr(tmp, "dev=")) != NULL)
    {
        end_ptr = ptr += strlen("dev=");
        while (*end_ptr != ',' && *end_ptr != '\0')
            end_ptr++;
        *end_ptr = '\0';

        if (strlen(ptr) >= sizeof(ifname))
        {
            ERROR("Interface name is too long: %s in route %s",
                  ptr, inst_name);
            return TE_RC(TE_TA_LINUX, TE_EINVAL);
        }
        strcpy(ifname, ptr);

        if (term_byte != end_ptr)
            *end_ptr = ',';
        rt->rt_dev = ifname;
    }

    if ((ptr = strstr(tmp, "metric=")) != NULL)
    {
        end_ptr = ptr += strlen("metric=");
        while (*end_ptr != ',' && *end_ptr != '\0')
            end_ptr++;
        *end_ptr = '\0';

        if (*ptr == '\0' || *ptr == '-' ||
            (int_val = strtol(ptr, &end_ptr, 10), *end_ptr != '\0'))
        {
            ERROR("Incorrect 'route metric' value in route %s",
                  inst_name);
            return TE_RC(TE_TA_LINUX, TE_EINVAL);
        }
        if (term_byte != end_ptr)
            *end_ptr = ',';
        rt->rt_metric = int_val;
    }

    if ((ptr = strstr(tmp, "mtu=")) != NULL)
    {
        end_ptr = ptr += strlen("mtu=");
        while (*end_ptr != ',' && *end_ptr != '\0')
            end_ptr++;
        *end_ptr = '\0';

        if (*ptr == '\0' || *ptr == '-' ||
            (int_val = strtol(ptr, &end_ptr, 10), *end_ptr != '\0'))
        {
            ERROR("Incorrect 'route mtu' value in route %s", inst_name);
            return TE_RC(TE_TA_LINUX, TE_EINVAL);
        }
        if (term_byte != end_ptr)
            *end_ptr = ',';

        /* Don't be confused the structure does not have mtu field */
        rt->rt_mtu = int_val;
        rt->rt_flags |= RTF_MSS;
    }

    if ((ptr = strstr(tmp, "window=")) != NULL)
    {
        end_ptr = ptr += strlen("window=");
        while (*end_ptr != ',' && *end_ptr != '\0')
            end_ptr++;
        *end_ptr = '\0';

        if (*ptr == '\0' ||  *ptr == '-' ||
            (int_val = strtol(ptr, &end_ptr, 10), *end_ptr != '\0'))
        {
            ERROR("Incorrect 'route window' value in route %s", inst_name);
            return TE_RC(TE_TA_LINUX, TE_EINVAL);
        }
        if (term_byte != end_ptr)
            *end_ptr = ',';
        rt->rt_window = int_val;
        rt->rt_flags |= RTF_WINDOW;
    }

    if ((ptr = strstr(tmp, "irtt=")) != NULL)
    {
        end_ptr = ptr += strlen("irtt=");
        while (*end_ptr != ',' && *end_ptr != '\0')
            end_ptr++;
        *end_ptr = '\0';

        if (*ptr == '\0' || *ptr == '-' ||
            (int_val = strtol(ptr, &end_ptr, 10), *end_ptr != '\0'))
        {
            ERROR("Incorrect 'route irtt' value in route %s", inst_name);
            return TE_RC(TE_TA_LINUX, TE_EINVAL);
        }
        if (term_byte != end_ptr)
            *end_ptr = ',';
        rt->rt_irtt = int_val;
        rt->rt_flags |= RTF_IRTT;
    }
#endif /* !__linux__ */

    if (strstr(tmp, "reject") != NULL)
        rt->rt_flags |= RTF_REJECT;

    return 0;
}

#ifdef USE_NETLINK_ROUTE
struct nl_request {
    struct nlmsghdr n;
    struct rtmsg    r;
    char            buf[1024];
};

static int
nl_get_unsigned(unsigned *val, const char *arg, int base)
{
    unsigned long res;
    char *ptr;

    if (!arg || !*arg)
        return -1;
    res = strtoul(arg, &ptr, base);
    if (!ptr || ptr == arg || *ptr || res > UINT_MAX)
        return -1;
    *val = res;
    return 0;
}

static int
nl_get_addr(inet_prefix *addr, const char *name, int family)
{
    const char *cp;
    unsigned char *ap = (unsigned char*)addr->data;
    int i;

    UNUSED(family);

    memset(addr, 0, sizeof(*addr));

    addr->bytelen = 4;
    addr->bitlen = -1;

    addr->family = AF_INET;
    for (cp=name, i=0; *cp; cp++) {
        if (*cp <= '9' && *cp >= '0') {
            ap[i] = 10*ap[i] + (*cp-'0');
            continue;
        }
        if (*cp == '.' && ++i <= 3)
            continue;
        return -1;
    }
    return 0;
}

static int
nl_get_integer(int *val, const char *arg, int base)
{
    long res;
    char *ptr;

    if (!arg || !*arg)
        return -1;
    res = strtol(arg, &ptr, base);
    if (!ptr || ptr == arg || *ptr || res > INT_MAX || res < INT_MIN)
        return -1;
    *val = res;
    return 0;
}

static int
nl_get_prefix(inet_prefix *dst, const char *arg, int family)
{
    int         err;
    unsigned    plen;
    char       *slash;
    char       *tmp = strdup(arg);

    if (tmp == NULL)
        return TE_ENOMEM;

    memset(dst, 0, sizeof(*dst));

    slash = strchr(tmp, '|');
    if (slash)
        *slash = 0;

    err = nl_get_addr(dst, tmp, family);
    if (err == 0) {
        dst->bitlen = 32;
        if (slash) {
            if (nl_get_integer(&plen, slash + 1, 0) ||
                plen > (unsigned)dst->bitlen) {
                err = -1;
                goto done;
            }
            dst->bitlen = plen;
        }
    }
done:
    if (slash)
        *slash = '|';

    free(tmp);
    return err;
}

static int
route_change(unsigned int gid, const char *oid, const char *value,
             const char *route, int action, unsigned flags)
{
    struct nl_request    req;
    inet_prefix          dst;
    struct rtnl_handle   rth;
    char                 mxbuf[256];
    struct rtattr       *mxrta = (void*)mxbuf;
    inet_prefix          addr;

    char                *c;
    char                 dev[10];
    int                  gw_ok = 0;

    UNUSED(gid);
    UNUSED(oid);

    ENTRY("%s", route);


    memset(&req, 0, sizeof(req));
    memset(&dev, 0, sizeof(dev));

    req.n.nlmsg_len = NLMSG_LENGTH(sizeof(struct rtmsg));
    req.n.nlmsg_flags = NLM_F_REQUEST | flags;
    req.n.nlmsg_type = action;

    req.r.rtm_family = AF_INET;
    req.r.rtm_table = RT_TABLE_MAIN;

    if (action != RTM_DELROUTE)
    {
        req.r.rtm_protocol = RTPROT_BOOT;
        req.r.rtm_scope = RT_SCOPE_UNIVERSE;
        req.r.rtm_type = RTN_UNICAST;
    }

    mxrta->rta_type = RTA_METRICS;
    mxrta->rta_len = RTA_LENGTH(0);

    {
        c = strchr(route, ',');
        if (c != NULL)
            *c = 0;

        if (nl_get_prefix(&dst, route, req.r.rtm_family) != 0)
            return TE_RC(TE_TA_LINUX, TE_EINVAL);

        req.r.rtm_dst_len = dst.bitlen;
        if (dst.bytelen)
        {
            if (addattr_l(&req.n, sizeof(req), RTA_DST, &dst.data,
                          dst.bytelen) != 0)
                return TE_RC(TE_TA_LINUX, TE_EINVAL);
        }
        if (c != NULL)
            *c = ',';
    }

    if ((c = strstr(route, "gw=")) != NULL)
    {
        char *d = strchr(c, ',');

        if (d != NULL)
            *d = 0;

        if (nl_get_addr(&addr, c + strlen("gw="), req.r.rtm_family) != 0)
            return TE_RC(TE_TA_LINUX, TE_EINVAL);

        if (addattr_l(&req.n, sizeof(req), RTA_GATEWAY, &addr.data,
                      addr.bytelen) != 0)
            return TE_RC(TE_TA_LINUX, TE_EINVAL);

        gw_ok = 1;
        if (d != NULL)
            *d = ',';
    }

    if ((c = strstr(route, "dev=")) != NULL)
    {
        char     *d = strchr(c, ',');

        if (d != NULL)
            *d = 0;

        strcpy(dev, c + strlen("dev="));
        if (dev == NULL)
            return TE_RC(TE_TA_LINUX, TE_EINVAL);

        if (d != NULL)
            *d = ',';
    }

    if ((c = strstr(route, "metric=")) != NULL)
    {
        unsigned  metric= 1500;
        char     *d = strchr(c, ',');

        if (d != NULL)
            *d = 0;

        if (nl_get_unsigned(&metric, c + strlen("metric="), 0))
            return TE_RC(TE_TA_LINUX, TE_EINVAL);

        addattr32(&req.n, sizeof(req), RTA_PRIORITY, metric);

        if (d != NULL)
            *d = ',';
    }

    if ((c = strstr(value, "mtu=")) != NULL)
    {
        unsigned  mtu = 1500;
        char     *d = strchr(c, ',');

        if (d != NULL)
            *d = 0;

        if (nl_get_unsigned(&mtu, c + strlen("mtu="), 0))
            return TE_RC(TE_TA_LINUX, TE_EINVAL);

        rta_addattr32(mxrta, sizeof(mxbuf), RTAX_MTU, mtu);

        if (d != NULL)
            *d = ',';
    }

    if ((c = strstr(value, "window=")) != NULL)
    {
        unsigned  window = 1500;
        char     *d = strchr(c, ',');

        if (d != NULL)
            *d = 0;

        if (nl_get_unsigned(&window, c + strlen("window="), 0))
            return TE_RC(TE_TA_LINUX, TE_EINVAL);

        rta_addattr32(mxrta, sizeof(mxbuf), RTAX_WINDOW, window);

        if (d != NULL)
            *d = ',';
    }

    if ((c = strstr(value, "irtt=")) != NULL)
    {
        unsigned  rtt = 1500;
        char     *d = strchr(c, ',');

        if (d != NULL)
            *d = 0;

        if (nl_get_unsigned(&rtt, c + strlen("irtt="), 0))
            return TE_RC(TE_TA_LINUX, TE_EINVAL);

        rta_addattr32(mxrta, sizeof(mxbuf), RTAX_RTT, rtt);

        if (d != NULL)
            *d = ',';
    }



    /* Sending the netlink message */
    if (rtnl_open(&rth, 0) < 0)
    {
        ERROR("Failed to open the netlink socket");
        return TE_OS_RC(TE_TA_LINUX, errno);
    }

    if (*dev != '\0')
    {
        int idx;

        ll_init_map(&rth);


        if ((idx = ll_name_to_index(dev)) == 0)
        {
            ERROR("Cannot find device");
            rtnl_close(&rth);
            return TE_RC(TE_TA_LINUX, TE_EINVAL);
        }
        addattr32(&req.n, sizeof(req), RTA_OIF, idx);
    }

    if (mxrta->rta_len > RTA_LENGTH(0))
    {
        addattr_l(&req.n, sizeof(req), RTA_METRICS, RTA_DATA(mxrta),
                  RTA_PAYLOAD(mxrta));
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
            else if (gw_ok == 0)
                req.r.rtm_scope = RT_SCOPE_LINK;
        }
    }

    if (rtnl_talk(&rth, &req.n, 0, 0, NULL, NULL, NULL) < 0)
    {
        ERROR("Failed to send the netlink message");
        rtnl_close(&rth);
        return TE_OS_RC(TE_TA_LINUX, errno);
    }

    rtnl_close(&rth);
    return 0;
}

/********************************************************************/

/**
 * Add a new route.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         value string (unused)
 * @param route         route instance name: see doc/cm_cm_base.xml
 *                      for the format
 *
 * @return error code
 */
static int
route_add(unsigned int gid, const char *oid, const char *value,
          const char *route)
{
    ENTRY("%s", route);
    fprintf(stderr, "\nadding route: %s = %s\n", route, value);

    return route_change(gid, oid, value, route,
                        RTM_NEWROUTE, NLM_F_CREATE|NLM_F_EXCL);
}

/**
 * Change already existing route.
 *
 * @param gid           group identifier
 * @param oid           full object instence identifier (unused)
 * @param value         value string (unused)
 * @param route         route instance name: see doc/cm_cm_base.xml
 *                      for the format
 *
 * @return error code
 */
static int
route_set(unsigned int gid, const char *oid, const char *value,
          const char *route)
{
    ENTRY("%s", route);
    UNUSED(oid);
    UNUSED(gid);
    fprintf(stderr, "\nchanging route: %s = %s\n", route, value);

    return route_change(gid, oid, value, route, RTM_NEWROUTE,
                        NLM_F_REPLACE);
}

/**
 * Delete a route.
 *
 * @param gid           group identifier
 * @param oid           full object instence identifier (unused)
 * @param route         route instance name: see doc/cm_cm_base.xml
 *                      for the format
 *
 * @return error code
 */
static int
route_del(unsigned int gid, const char *oid, const char *route)
{
    char            value[RCF_MAX_VAL];

    ENTRY("%s", route);
    UNUSED(oid);
    UNUSED(gid);

    fprintf(stderr, "deleting route: %s\n", route);

    return route_change(gid, oid, value, route,
                        RTM_DELROUTE, 0);

}

#else

/**
 * Change already existing route.
 *
 * @param gid           group identifier
 * @param oid           full object instence identifier (unused)
 * @param value         value string (unused)
 * @param route         route instance name: see doc/cm_cm_base.xml
 *                      for the format
 *
 * @return error code
 */
static int
route_set(unsigned int gid, const char *oid, const char *value,
          const char *route)
{
    char val[RCF_MAX_VAL];

    ENTRY("%s", route);

    if (route_get(gid, oid, val, route) != 0)
        return TE_RC(TE_TA_LINUX, TE_ENOENT);

    return route_add(gid, oid, value, route);
}

/**
 * Add a new route.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         value string (unused)
 * @param route         route instance name: see doc/cm_cm_base.xml
 *                      for the format
 *
 * @return error code
 */
static int
route_add(unsigned int gid, const char *oid, const char *value,
          const char *route)
{
#ifdef __linux__
    int             rc;
    struct rtentry  rt;

    UNUSED(gid);
    UNUSED(oid);

    ENTRY("%s", route);

    if ((rc = route_parse_inst_name(route, &rt)) != 0)
        return 0;

    if (strstr(value, "mod") != NULL)
        rt.rt_flags |= RTF_MODIFIED;
    if (strstr(value, "dyn") != NULL)
        rt.rt_flags |= RTF_DYNAMIC;
    if (strstr(value, "reinstate") != NULL)
        rt.rt_flags |= RTF_REINSTATE;

    if (rt.rt_metric != 0)
    {
        /*
         * Increment metric because ioctl substracts one from the value,
         * 'route' command does the same thing.
         */
        rt.rt_metric++;
    }

    rt.rt_flags |= (RTF_UP | RTF_STATIC);

    if (ioctl(cfg_socket, SIOCADDRT, &rt) < 0)
    {
        ERROR("ioctl(SIOCADDRT) failed: %s", strerror(errno));
        return TE_OS_RC(TE_TA_LINUX, errno);
    }

    return 0;
#else
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);
    UNUSED(route);

    return TE_RC(TE_TA_LINUX, TE_EOPNOTSUPP);
#endif
}


/**
 * Delete a route.
 *
 * @param gid           group identifier
 * @param oid           full object instence identifier (unused)
 * @param route         route instance name: see doc/cm_cm_base.xml
 *                      for the format
 *
 * @return error code
 */
static int
route_del(unsigned int gid, const char *oid, const char *route)
{
#ifdef __linux__
    int             rc;
    char            value[RCF_MAX_VAL];
    struct rtentry  rt;

    ENTRY("%s", route);

    if (route_get(gid, oid, value, route) != 0)
    {
        ERROR("NOT FOUND");
        return TE_RC(TE_TA_LINUX, TE_ENOENT);
    }

    if ((rc = route_parse_inst_name(route, &rt)) != 0)
        return 0;

    if (rt.rt_metric != 0)
    {
        /*
         * Increment metric because ioctl substracts one from the value,
         * 'route' command does the same thing.
         */
        rt.rt_metric++;
    }

    if (ioctl(cfg_socket, SIOCDELRT, &rt) < 0)
    {
        ERROR("ioctl(SIOCDELRT) failed: %s", strerror(errno));
        return TE_OS_RC(TE_TA_LINUX, errno);
    }

    return 0;
#else
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(route);

    return TE_RC(TE_TA_LINUX, TE_EOPNOTSUPP);
#endif
}
#endif /* USE_NETLINK_ROUTE */



/**
 * Get route value (gateway IP address).
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         value location (IPv4 address is returned in
 *                      dotted notation)
 * @param route         route instance name: see doc/cm_cm_base.xml
 *                      for the format
 *
 * @return error code
 */
static int
route_get(unsigned int gid, const char *oid, char *value,
          const char *route)
{
#ifdef __linux__
    int       rc;
    FILE     *fp;
    char      ifname[IF_NAMESIZE];
    uint32_t  route_addr;
    uint32_t  route_mask;
    uint32_t  route_gw;

    struct rtentry  rt;

    UNUSED(gid);
    UNUSED(oid);

    ENTRY("%s", route);

    if ((rc = route_parse_inst_name(route, &rt)) != 0)
        return 0;

    memcpy(&route_addr, &(((struct sockaddr_in *)&(rt.rt_dst))->sin_addr),
           sizeof(route_addr));

    route_mask = ((struct sockaddr_in *)&(rt.rt_genmask))->sin_addr.s_addr;
    route_gw = ((struct sockaddr_in *)&(rt.rt_gateway))->sin_addr.s_addr;

    if ((fp = fopen("/proc/net/route", "r")) == NULL)
    {
        ERROR("Failed to open /proc/net/route for reading: %s",
              strerror(errno));
        return TE_OS_RC(TE_TA_LINUX, errno);
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

        fscanf(fp, "%x %x %x %d %d %d %x %d %d %d", &addr, &gateway,
               &flags, (int *)trash, (int *)trash, &metric, &mask,
               &mtu, &win, &irtt);
        VERB("%s: Route %s %x %x %x %d %d %d %x %d %d %d", __FUNCTION__,
             ifname, addr, gateway, flags, 0, 0, metric, mask,
             mtu, win, irtt);

        if ((rt.rt_dev != NULL && strcmp(rt.rt_dev, ifname) != 0) ||
            addr != route_addr ||
            gateway != route_gw ||
            (unsigned int)rt.rt_metric != metric ||
            mask != route_mask ||
            rt.rt_mtu != (unsigned long int)mtu ||
            rt.rt_window != (unsigned long int)win ||
            rt.rt_irtt != irtt ||
            ((rt.rt_flags & RTF_REJECT) ^ (flags & RTF_REJECT)))
        {
            fgets(trash, sizeof(trash), fp);
            VERB("Continue processing ...");
            continue;
        }

        if ((flags & RTF_UP) == 0)
            break;

        fclose(fp);

        value[0] = '\0';

#define TE_LC_RTF_SET_FLAG(flg_, name_) \
        do {                                           \
            if (flags & flg_)                          \
            {                                          \
                snprintf(value + strlen(value),        \
                         RCF_MAX_VAL - strlen(value),  \
                         " " name_);                   \
            }                                          \
        } while (0)

        TE_LC_RTF_SET_FLAG(RTF_MODIFIED, "mod");
        TE_LC_RTF_SET_FLAG(RTF_DYNAMIC, "dyn");
        TE_LC_RTF_SET_FLAG(RTF_REINSTATE, "reinstate");

#undef TE_LC_RTF_SET_FLAG

        return 0;
    }

    fclose(fp);

    return TE_RC(TE_TA_LINUX, TE_ENOENT);
#else
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);
    UNUSED(route);

    return TE_RC(TE_TA_LINUX, TE_EOPNOTSUPP);
#endif
}

/**
 * Get instance list for object "agent/route".
 *
 * @param id            full identifier of the father instance
 * @param list          location for the list pointer
 *
 * @return error code
 * @retval 0                    success
 * @retval TE_ENOENT      no such instance
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
    if ((fp = fopen("/proc/net/route", "r")) == NULL)
    {
        ERROR("Failed to open /proc/net/route for reading: %s",
              strerror(errno));
        return TE_OS_RC(TE_TA_LINUX, errno);
    }

    buf[0] = 0;

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

        fscanf(fp, "%x %x %x %d %d %d %x %d %d %d", &addr, &gateway,
               &flags, (int *)trash, (int *)trash, &metric, &mask,
               &mtu, &win, &irtt);

        if (flags & RTF_UP)
        {
            MASK2PREFIX(ntohl(mask), prefix);

            snprintf(ptr, end_ptr - ptr, "%d.%d.%d.%d|%d",
                    ((uint8_t *)&addr)[0], ((uint8_t *)&addr)[1],
                    ((uint8_t *)&addr)[2], ((uint8_t *)&addr)[3], prefix);
            ptr += strlen(ptr);

            if (flags & RTF_GATEWAY)
            {
                snprintf(ptr, end_ptr - ptr, ",gw=%d.%d.%d.%d",
                        ((uint8_t *)&gateway)[0], ((uint8_t *)&gateway)[1],
                        ((uint8_t *)&gateway)[2], ((uint8_t *)&gateway)[3]);
            }
            else
            {
                snprintf(ptr, end_ptr - ptr, ",dev=%s", ifname);
            }
            ptr += strlen(ptr);

            if (metric != 0)
            {
                snprintf(ptr, end_ptr - ptr, ",metric=%d", metric);
                ptr += strlen(ptr);
            }
#ifndef USE_NETLINK_ROUTE
            if (mtu != 0)
            {
                snprintf(ptr, end_ptr - ptr, ",mtu=%d", mtu);
                ptr += strlen(ptr);
            }
            if (win != 0)
            {
                snprintf(ptr, end_ptr - ptr, ",window=%d", win);
                ptr += strlen(ptr);
            }
            if (irtt != 0)
            {
                snprintf(ptr, end_ptr - ptr, ",irtt=%d", irtt);
                ptr += strlen(ptr);
            }
#endif
            if (flags & RTF_REJECT)
            {
                snprintf(ptr, end_ptr - ptr, "rejected");
                ptr += strlen(ptr);
            }
            snprintf(ptr, end_ptr - ptr, " ");
            ptr += strlen(ptr);
        }
        fgets(trash, sizeof(trash), fp);
    }
    fclose(fp);

    INFO("%s: Routes: %s", __FUNCTION__, buf);
    if ((*list = strdup(buf)) == NULL)
        return TE_RC(TE_TA_LINUX, TE_ENOMEM);

    return 0;
}

static int
nameserver_get(unsigned int gid, const char *oid, char *result,
               const char *instance, ...)
{
    FILE *resolver = NULL;
    char  buf[256];
    char *found = NULL, *endaddr = NULL;
    int   rc = TE_RC(TE_TA_LINUX, TE_ENOENT);

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
        return TE_OS_RC(TE_TA_LINUX, rc);
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
                    rc = TE_RC(TE_TA_LINUX, TE_ENAMETOOLONG);
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
 * @param oid       Full object instence identifier (unused)
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
        return TE_RC(TE_TA_LINUX, TE_ENOENT);
    }
}

/**
 * Change already existing Environment variable.
 *
 * @param gid       Request's group identifier (unused)
 * @param oid       Full object instence identifier (unused)
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
        return TE_RC(TE_TA_LINUX, TE_EPERM);

    if (setenv(name, value, TRUE) == 0)
    {
        return 0;
    }
    else
    {
        int rc = errno;

        ERROR("Failed to set Environment variable '%s' to '%s'",
              name, value);
        return TE_OS_RC(TE_TA_LINUX, rc);
    }
}

/**
 * Add a new Environment variable.
 *
 * @param gid       Request's group identifier (unused)
 * @param oid       Full object instence identifier (unused)
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
        return TE_RC(TE_TA_LINUX, TE_EPERM);

    if (getenv(name) == NULL)
    {
        if (setenv(name, value, FALSE) == 0)
        {
            return 0;
        }
        else
        {
            int rc = errno;

            ERROR("Failed to add Environment variable '%s=%s'",
                  name, value);
            return TE_OS_RC(TE_TA_LINUX, rc);
        }
    }
    else
    {
        return TE_RC(TE_TA_LINUX, TE_EEXIST);
    }
}

/**
 * Delete Environment variable.
 *
 * @param gid       Request's group identifier (unused)
 * @param oid       Full object instence identifier (unused)
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
        return TE_RC(TE_TA_LINUX, TE_EPERM);

    if (getenv(name) != NULL)
    {
        unsetenv(name);
        return 0;
    }
    else
    {
        return TE_RC(TE_TA_LINUX, TE_ENOENT);
    }
}

/**
 * Get instance list for object "/agent/env".
 *
 * @param gid       Request's group identifier (unused)
 * @param oid       Full object instence identifier (unused)
 * @param list      Location for the list pointer
 *
 * @return Error code
 */
static int
env_list(unsigned int gid, const char *oid, char **list)
{
    extern char const * const *environ;

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
            return TE_RC(TE_TA_LINUX, TE_EFMT);
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
            return TE_RC(TE_TA_LINUX, TE_ESMALLBUF);
        }
        memcpy(ptr, *env, name_len);
        ptr += name_len;
        *ptr = '\0';
    }

    if ((*list = strdup(buf)) == NULL)
        return TE_RC(TE_TA_LINUX, TE_ENOMEM);

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
        int rc = errno;

        ERROR("Failed to open file /etc/passwd; errno %d", rc);
        return TE_OS_RC(TE_TA_LINUX, rc);
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
        return TE_RC(TE_TA_LINUX, TE_ENOMEM);

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
 * @param oid           full object instence identifier (unused)
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

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);

    if (user_exists(user))
        return TE_RC(TE_TA_LINUX, TE_EEXIST);

    if (strncmp(user, TE_USER_PREFIX, strlen(TE_USER_PREFIX)) != 0)
        return TE_RC(TE_TA_LINUX, TE_EINVAL);

    tmp = (char *)user + strlen(TE_USER_PREFIX);
    uid = strtol(tmp, &tmp1, 10);
    if (tmp == tmp1 || *tmp1 != 0)
        return TE_RC(TE_TA_LINUX, TE_EINVAL);

    if (ta_system("adduser --help >/dev/null 2>&1") != 0)
    {
        /* Red Hat/Fedora */
        sprintf(buf, "/usr/sbin/adduser -d /tmp/%s -u %u -m %s ",
                user, uid, user);
        if (ta_system(buf) != 0)
            return TE_RC(TE_TA_LINUX, TE_ESHCMD);
    }
    else
    {
        /* Debian */
        sprintf(buf, "/usr/sbin/adduser --home /tmp/%s --force-badname "
                     "--disabled-password --gecos \"\" "
                     "--uid %u %s >/dev/null 2>&1", user, uid, user);
        if (ta_system(buf) != 0)
            return TE_RC(TE_TA_LINUX, TE_ESHCMD);
    }
    sprintf(buf, "echo %s:%s | /usr/sbin/chpasswd", user, user);
    if (ta_system(buf) != 0)
    {
        user_del(gid, oid, user);
        return TE_RC(TE_TA_LINUX, TE_ESHCMD);
    }
    ta_system("sync");
    sleep(1);
    ta_system("sync");

    sprintf(buf, "su - %s -c 'ssh-keygen -t dsa -N \"\" "
                 "-f /tmp/%s/.ssh/id_dsa' >/dev/null 2>&1", user, user);

    if (ta_system(buf) != 0)
    {
        user_del(gid, oid, user);
        return TE_RC(TE_TA_LINUX, TE_ESHCMD);
    }

    return 0;
}

/**
 * Delete tester user.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param user          user name
 *
 * @return error code
 */
static int
user_del(unsigned int gid, const char *oid, const char *user)
{
    UNUSED(gid);
    UNUSED(oid);

    if (!user_exists(user))
        return TE_RC(TE_TA_LINUX, TE_EEXIST);

    sprintf(buf, "/usr/sbin/userdel -r %s", user);
    ta_system(buf);

    return 0;
}
