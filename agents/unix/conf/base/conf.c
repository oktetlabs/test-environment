/** @file
 * @brief Unix Test Agent
 *
 * Unix TA configuring support
 *
 *
 * Copyright (C) 2004-2006 Test Environment authors (see file AUTHORS
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

#include <limits.h>
#include <stdio.h>
#include <ctype.h>
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
#if HAVE_SYS_SOCKIO_H
#include <sys/sockio.h>
#endif
#if HAVE_SYS_FILIO_H
#include <sys/filio.h>
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
#if HAVE_NET_IF_DL_H
#include <net/if_dl.h>
#endif

#ifdef HAVE_LIBDLPI
#include <libdlpi.h>
#endif

#if HAVE_LINUX_IF_VLAN_H
#include <linux/if_vlan.h>
#define LINUX_VLAN_SUPPORT 1
#else
#define LINUX_VLAN_SUPPORT 0
#endif

/* { required for sysctl on netbsd */
#if HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
/* } required for sysctl on netbsd */
#if HAVE_SYS_SYSCTL_H
#include <sys/sysctl.h>
#endif
#if defined(HAVE_SYS_SYSCTL_H) && defined(HAVE_SYS_TYPES_H) && \
    !defined(__linux__) && !defined(BSD_IP_FW)
#define BSD_IP_FW 1
#endif

/* IP forwarding on Solaris: */
#ifdef HAVE_STROPTS_H
#include <stropts.h>
#endif
#ifdef HAVE_INET_ND_H
#include <inet/nd.h>
#endif
#if defined(HAVE_STROPTS_H) && defined(HAVE_INET_ND_H) && \
    !defined(SOLARIS_IP_FW)
#define SOLARIS_IP_FW 1
#endif

#include <pwd.h>

#ifdef HAVE_SYS_UTSNAME_H
#include <sys/utsname.h>
#endif

#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

/* PAM (Pluggable Authentication Modules) support */
#if defined(HAVE_SECURITY_PAM_APPL_H) && defined(HAVE_LIBPAM)
#include <security/pam_appl.h>

#define TA_USE_PAM  1

/** Data passed between 'set_change_passwd' and 'conv_fun' callback fun */
typedef struct {
    char const *passwd;                    /**< Password string pointer */
    char        err_msg[PAM_MAX_MSG_SIZE]; /**< Error message storage   */

} appdata_t;

typedef struct pam_response pam_response_t;

/** Avoid slight differences between UNIX'es over typedef */
#if defined __linux__
#define PAM_FLAGS 0
typedef struct pam_message const pam_message_t;
#elif defined __sun__
#define PAM_FLAGS (PAM_NO_AUTHTOK_CHECK | PAM_SILENT)
typedef struct pam_message pam_message_t;
#elif defined __FreeBSD__ || defined __NetBSD__
#define PAM_FLAGS PAM_SILENT
typedef struct pam_message const pam_message_t;
#endif

#else

#define TA_USE_PAM  0

#endif /* HAVE_SECURITY_PAM_APPL_H && HAVE_LIBPAM */

#include "te_stdint.h"
#include "te_errno.h"
#include "te_defs.h"
#include "te_queue.h"
#include "te_ethernet.h"
#include "te_sockaddr.h"
#include "cs_common.h"
#include "logger_api.h"
#include "comm_agent.h"
#include "rcf_ch_api.h"
#include "rcf_pch.h"
#include "rcf_pch_ta_cfg.h"
#include "logger_api.h"
#include "unix_internal.h"
#include "conf_dlpi.h"
#include "conf_route.h"
#include "te_shell_cmd.h"

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

#ifndef ENABLE_IFCONFIG_STATS
#define ENABLE_IFCONFIG_STATS
#endif

#ifndef ENABLE_NET_SNMP_STATS
#define ENABLE_NET_SNMP_STATS
#endif

#ifndef IF_NAMESIZE
#define IF_NAMESIZE IFNAMSIZ
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

#ifdef ENABLE_IFCONFIG_STATS
extern te_errno ta_unix_conf_net_if_stats_init();
#endif

#ifdef ENABLE_NET_SNMP_STATS
extern te_errno ta_unix_conf_net_snmp_stats_init();
#endif

#ifdef ENABLE_WIFI_SUPPORT
extern te_errno ta_unix_conf_wifi_init();
#endif

#ifdef WITH_ISCSI
extern te_errno ta_unix_iscsi_target_init();
extern te_errno iscsi_initiator_conf_init();
#endif

extern te_errno ta_unix_conf_sys_init();
extern te_errno ta_unix_conf_phy_init();

#ifdef USE_NETLINK
/** Netlink message storage */
typedef struct agt_nlmsg_entry {
    TAILQ_ENTRY(agt_nlmsg_entry) links;  /**< List links */
    struct nlmsghdr *hdr; /**< Pointer to netlink message */
} agt_nlmsg_entry;

/** Head of netlink messages list */
typedef TAILQ_HEAD(agt_nlmsg_list, agt_nlmsg_entry) agt_nlmsg_list;

static void free_nlmsg_list(agt_nlmsg_list *list);

#endif

/**
 * Determine family of the address in string representation.
 *
 * @param str_addr      Address in string representation
 *
 * @return Address family.
 * @retval AF_INET      IPv4
 * @retval AF_INET6     IPv6
 */
static inline sa_family_t
str_addr_family(const char *str_addr)
{
    return (strchr(str_addr, ':') == NULL) ? AF_INET : AF_INET6;
}

#define INTERFACE_IS_LOOPBACK(ifname) \
     (strncmp(ifname, "lo", strlen("lo")) == 0)

#define CHECK_INTERFACE(ifname) \
    ((ifname == NULL) ? TE_EINVAL :                 \
     (strlen(ifname) > IFNAMSIZ) ? TE_E2BIG :       \
     (strchr(ifname, ':') != NULL ||                \
      !ta_interface_is_mine(ifname)) ? TE_ENODEV : 0)

/**
 * Configuration IOCTL request.
 * On failure, ERROR is logged and return with 'te_errno' status is done.
 */
#define CFG_IOCTL(_s, _id, _req) \
    do {                                                    \
        if (ioctl((_s), (_id), (caddr_t)(_req)) != 0)       \
        {                                                   \
            te_errno rc = TE_OS_RC(TE_TA_UNIX, errno);      \
                                                            \
            ERROR("line %u: ioctl(" #_id ") failed: %r",    \
                  __LINE__, rc);                            \
            return rc;                                      \
        }                                                   \
    } while (0)

/**
 * Type for both IPv4 and IPv6 address
 */
typedef union gen_ip_address {
    struct in_addr  ip4_addr;  /** IPv4 address */
    struct in6_addr ip6_addr;  /** IPv6 address */
} gen_ip_address;

const char *te_lockdir = "/tmp";

/* Auxiliary variables used for during configuration request processing */
#if HAVE_STRUCT_LIFREQ
#define my_ifreq        lifreq
#define my_ifr_name     lifr_name
#define my_ifr_flags    lifr_flags
#define my_ifr_addr     lifr_addr
#define my_ifr_mtu      lifr_mtu
#define MY_SIOCGIFFLAGS     SIOCGLIFFLAGS
#define MY_SIOCSIFFLAGS     SIOCSLIFFLAGS
#define MY_SIOCGIFADDR      SIOCGLIFADDR
#define MY_SIOCSIFADDR      SIOCSLIFADDR
#define MY_SIOCGIFMTU       SIOCGLIFMTU
#define MY_SIOCSIFMTU       SIOCSLIFMTU
#define MY_SIOCGIFNETMASK   SIOCGLIFNETMASK
#define MY_SIOCSIFNETMASK   SIOCSLIFNETMASK
#define MY_SIOCGIFBRDADDR   SIOCGLIFBRDADDR
#define MY_SIOCSIFBRDADDR   SIOCSLIFBRDADDR
#else
#define my_ifreq        ifreq
#define my_ifr_name     ifr_name
#define my_ifr_flags    ifr_flags
#define my_ifr_addr     ifr_addr
#define my_ifr_mtu      ifr_mtu
#define my_ifr_hwaddr   ifr_hwaddr
#define MY_SIOCGIFFLAGS     SIOCGIFFLAGS
#define MY_SIOCSIFFLAGS     SIOCSIFFLAGS
#define MY_SIOCGIFADDR      SIOCGIFADDR
#define MY_SIOCSIFADDR      SIOCSIFADDR
#define MY_SIOCGIFMTU       SIOCGIFMTU
#define MY_SIOCSIFMTU       SIOCSIFMTU
#define MY_SIOCGIFNETMASK   SIOCGIFNETMASK
#define MY_SIOCSIFNETMASK   SIOCSIFNETMASK
#define MY_SIOCGIFBRDADDR   SIOCGIFBRDADDR
#define MY_SIOCSIFBRDADDR   SIOCSIFBRDADDR
#endif
static struct my_ifreq req;

static char buf[4096];
static char trash[128];

int cfg_socket = -1;
int cfg6_socket = -1;

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

static te_errno uname_get(unsigned int, const char *, char *);

static te_errno ip4_fw_get(unsigned int, const char *, char *);
static te_errno ip4_fw_set(unsigned int, const char *, const char *);

static te_errno ip6_fw_get(unsigned int, const char *, char *);
static te_errno ip6_fw_set(unsigned int, const char *, const char *);

static te_errno interface_list(unsigned int, const char *, char **);

static te_errno vlans_list(unsigned int, const char *, char **,
                           const char*);
static te_errno vlans_add(unsigned int, const char *, const char *,
                              const char *, const char *);
static te_errno vlans_del(unsigned int, const char *, const char *,
                          const char *);

static te_errno mcast_link_addr_add(unsigned int, const char *,
                                    const char *, const char *,
                                    const char *);
static te_errno mcast_link_addr_del(unsigned int, const char *,
                                    const char *, const char *);
static te_errno mcast_link_addr_list(unsigned int, const char *, char **,
                                     const char *);
#ifndef __linux__
typedef struct mma_list_el {
    char                value[ETHER_ADDR_LEN * 3];
    struct mma_list_el *next;
} mma_list_el;

typedef struct ifs_list_el {
    char                ifname[IFNAMSIZ];
#ifdef HAVE_LIBDLPI    
    dlpi_handle_t       fd;
#endif    
    struct mma_list_el *mcast_addresses;
    struct ifs_list_el *next;
} ifs_list_el;
static struct ifs_list_el *interface_stream_list = NULL;
#endif

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

static te_errno bcast_link_addr_set(unsigned int, const char *,
                                    const char *, const char *);

static te_errno bcast_link_addr_get(unsigned int, const char *,
                                    char *, const char *);

static te_errno vlan_ifname_get(unsigned int , const char *,
                                char *, const char *, const char *);

static te_errno vlan_ifname_get_internal(const char *ifname, int vlan_id, 
                                         char *v_ifname);


static te_errno ifindex_get(unsigned int, const char *, char *,
                            const char *);

static te_errno status_get(unsigned int, const char *, char *,
                           const char *);
static te_errno status_set(unsigned int, const char *, const char *,
                           const char *);

static te_errno promisc_get(unsigned int, const char *, char *,
                            const char *);
static te_errno promisc_set(unsigned int, const char *, const char *,
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

static te_errno nameserver_get(unsigned int, const char *, char *,
                               const char *, ...);

static te_errno user_list(unsigned int, const char *, char **);
static te_errno user_add(unsigned int, const char *, const char *,
                         const char *);
static te_errno user_del(unsigned int, const char *, const char *);

te_errno ta_vlan_get_children(const char *, size_t *, int *);
te_errno ta_vlan_get_parent(const char *, char *);


/* XEN stuff interface */
static te_errno xen_path_get(unsigned int, char const *, char *);
static te_errno xen_path_set(unsigned int, char const *, char const *);

static te_errno dom_u_add(unsigned int, char const *, char const *,
                          char const *, char const *);
static te_errno dom_u_del(unsigned int, char const *, char const *,
                          char const *);
static te_errno dom_u_list(unsigned int, char const *, char **);
static te_errno dom_u_get(unsigned int, char const *, char *,
                          char const *, char const *);
static te_errno dom_u_set(unsigned int, char const *, char const *,
                          char const *, char const *);

static te_errno dom_u_status_get(unsigned int, char const *, char *,
                                 char const *, char const *);
static te_errno dom_u_status_set(unsigned int, char const *,
                                 char const *, char const *,
                                 char const *);

static te_errno dom_u_ip_addr_get(unsigned int, char const *, char *,
                                  char const *, char const *);
static te_errno dom_u_ip_addr_set(unsigned int, char const *,
                                  char const *, char const *,
                                  char const *);

static te_errno dom_u_mac_addr_get(unsigned int, char const *, char *,
                                   char const *, char const *);
static te_errno dom_u_mac_addr_set(unsigned int, char const *,
                                   char const *, char const *,
                                   char const *);

static te_errno dom_u_migrate_set(unsigned int, char const *,
                                  char const *, char const *,
                                  char const *);

static te_errno dom_u_migrate_kind_get(unsigned int, char const *, char *,
                                       char const *, char const *);
static te_errno dom_u_migrate_kind_set(unsigned int, char const *,
                                       char const *, char const *,
                                       char const *);

/*
 * Unix Test Agent basic configuration tree.
 */

RCF_PCH_CFG_NODE_RO(node_dns, "dns",
                    NULL, NULL,
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

      
RCF_PCH_CFG_NODE_RW(node_broadcast, "broadcast", NULL, NULL,
                    broadcast_get, broadcast_set);

static rcf_pch_cfg_object node_net_addr =
    { "net_addr", 0, &node_broadcast, &node_neigh_static,
      (rcf_ch_cfg_get)prefix_get, (rcf_ch_cfg_set)prefix_set,
      (rcf_ch_cfg_add)net_addr_add, (rcf_ch_cfg_del)net_addr_del,
      (rcf_ch_cfg_list)net_addr_list, NULL, NULL };

static rcf_pch_cfg_object node_mcast_link_addr = 
    { "mcast_link_addr", 0, NULL, &node_net_addr,
      NULL, NULL, (rcf_ch_cfg_add)mcast_link_addr_add,
      (rcf_ch_cfg_del)mcast_link_addr_del,
      (rcf_ch_cfg_list)mcast_link_addr_list, NULL, NULL };

RCF_PCH_CFG_NODE_RO(node_vl_ifname, "ifname", NULL, NULL,
                    vlan_ifname_get);

RCF_PCH_CFG_NODE_COLLECTION(node_vlans, "vlans",
                            &node_vl_ifname, &node_mcast_link_addr,
                            vlans_add, vlans_del,
                            vlans_list, NULL);

RCF_PCH_CFG_NODE_RW(node_promisc, "promisc", NULL, &node_vlans,
                    promisc_get, promisc_set);

RCF_PCH_CFG_NODE_RW(node_status, "status", NULL, &node_promisc,
                    status_get, status_set);

RCF_PCH_CFG_NODE_RW(node_mtu, "mtu", NULL, &node_status,
                    mtu_get, mtu_set);

RCF_PCH_CFG_NODE_RW(node_arp, "arp", NULL, &node_mtu,
                    arp_get, arp_set);

RCF_PCH_CFG_NODE_RW(node_link_addr, "link_addr", NULL, &node_arp,
                    link_addr_get, link_addr_set);

RCF_PCH_CFG_NODE_RW(node_bcast_link_addr, "bcast_link_addr", NULL,
                    &node_link_addr,
                    bcast_link_addr_get, bcast_link_addr_set);

RCF_PCH_CFG_NODE_RO(node_ifindex, "index", NULL, &node_bcast_link_addr,
                    ifindex_get);

RCF_PCH_CFG_NODE_COLLECTION(node_interface, "interface",
                            &node_ifindex, &node_dns,
                            NULL, NULL, interface_list, NULL);

RCF_PCH_CFG_NODE_RW(node_ip4_fw, "ip4_fw", NULL, &node_interface,
                    ip4_fw_get, ip4_fw_set);

RCF_PCH_CFG_NODE_RW(node_ip6_fw, "ip6_fw", NULL, &node_ip4_fw,
                    ip6_fw_get, ip6_fw_set);

static rcf_pch_cfg_object node_env =
    { "env", 0, NULL, &node_ip6_fw,
      (rcf_ch_cfg_get)env_get, (rcf_ch_cfg_set)env_set,
      (rcf_ch_cfg_add)env_add, (rcf_ch_cfg_del)env_del,
      (rcf_ch_cfg_list)env_list, NULL, NULL };

RCF_PCH_CFG_NODE_RO(node_uname, "uname", NULL, &node_env, uname_get);

RCF_PCH_CFG_NODE_COLLECTION(node_user, "user",
                            NULL, &node_uname,
                            user_add, user_del,
                            user_list, NULL);

/* XEN stuff tree */
RCF_PCH_CFG_NODE_RW(node_dom_u_migrate_kind, "kind",
                    NULL, NULL,
                    &dom_u_migrate_kind_get, &dom_u_migrate_kind_set);

RCF_PCH_CFG_NODE_RW(node_dom_u_migrate, "migrate",
                    &node_dom_u_migrate_kind, NULL,
                    NULL, &dom_u_migrate_set);

RCF_PCH_CFG_NODE_RW(node_dom_u_mac_addr, "mac_addr",
                    NULL, &node_dom_u_migrate,
                    &dom_u_mac_addr_get, &dom_u_mac_addr_set);

RCF_PCH_CFG_NODE_RW(node_dom_u_ip_addr, "ip_addr",
                    NULL, &node_dom_u_mac_addr,
                    &dom_u_ip_addr_get, &dom_u_ip_addr_set);

RCF_PCH_CFG_NODE_RW(node_dom_u_status, "status",
                    NULL, &node_dom_u_ip_addr,
                    &dom_u_status_get, &dom_u_status_set);

static rcf_pch_cfg_object node_dom_u =
    { "dom_u", 0, &node_dom_u_status, NULL,
      (rcf_ch_cfg_get)&dom_u_get, (rcf_ch_cfg_set)&dom_u_set,
      (rcf_ch_cfg_add)&dom_u_add, (rcf_ch_cfg_del)&dom_u_del,
      (rcf_ch_cfg_list)&dom_u_list, NULL, NULL };

RCF_PCH_CFG_NODE_RW(node_xen, "xen",
                    &node_dom_u, &node_user,
                    &xen_path_get, &xen_path_set);

RCF_PCH_CFG_NODE_AGENT(node_agent, &node_xen);

static te_bool init = FALSE;


#define MAX_VLANS 0xfff
static int vlans_buffer[MAX_VLANS];





te_bool
ta_interface_is_mine(const char *ifname)
{
    char parent[IFNAMSIZ] = "";

    if (INTERFACE_IS_LOOPBACK(ifname) || 
        rcf_pch_rsrc_accessible("/agent:%s/interface:%s",
                                ta_name, ifname))
        return TRUE;

    if (ta_vlan_get_parent(ifname, parent) != 0)
        return FALSE;

    if (*parent)
        return rcf_pch_rsrc_accessible("/agent:%s/interface:%s",
                                       ta_name, parent);
    return FALSE; 
}

/** Grab interface-specific resources */
static te_errno
interface_grab(const char *name)
{
    const char *ifname = strrchr(name, ':');
    te_errno    rc;
    char parent[IFNAMSIZ];


    if (ifname == NULL)
    {
        ERROR("%s(): Invalid interface instance name %s", __FUNCTION__,
              name);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }
    ifname++;

    rc = ta_vlan_get_parent(ifname, parent);
    if (rc != 0)
        return rc;

    if (*parent != 0)
    {
        rc = rcf_pch_rsrc_check_locks(parent);
        if (rc != 0)
            return rc;
    }
    else
    {
        /* Grab main interface with all its VLANs */
        unsigned int len = strlen(ifname);
        size_t n_vlans = MAX_VLANS, i;
        char         vlan_ifname[len + 10];
        

        rc = ta_vlan_get_children(ifname, &n_vlans, vlans_buffer);
        if (rc != 0)
            return rc;

        for (i = 0; i < n_vlans; i++)
        {
            vlan_ifname_get_internal(ifname, vlans_buffer[i], vlan_ifname);
            rc = rcf_pch_rsrc_check_locks(vlan_ifname);
            if (rc != 0)
                return rc;
        } 
    }

#ifdef ENABLE_8021X
    return supplicant_grab(name);
#else
    return 0;
#endif
}

/** Release interface-specific resources */
static te_errno
interface_release(const char *name)
{
#ifdef ENABLE_8021X
    return supplicant_release(name);
#else
    UNUSED(name);
    return 0;
#endif
}

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

        if ((cfg_socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        {
            return NULL;
        }
        if (fcntl(cfg_socket, F_SETFD, FD_CLOEXEC) != 0)
        {
            ERROR("Failed to set close-on-exec flag on configuration "
                  "socket: %r", errno);
        }
        /* Ignore IPv6 configuration socket creation failure */
        if ((cfg6_socket = socket(AF_INET6, SOCK_DGRAM, 0)) >= 0)
        {
            if (fcntl(cfg6_socket, F_SETFD, FD_CLOEXEC) != 0)
            {
                ERROR("Failed to set close-on-exec flag on IPv6 "
                      "configuration socket: %r", errno);
            }
        }

        init = TRUE;

        rcf_pch_rsrc_info("/agent/interface", 
                          interface_grab,
                          interface_release);

        rcf_pch_rsrc_info("/agent/ip4_fw", 
                          rcf_pch_rsrc_grab_dummy,
                          rcf_pch_rsrc_release_dummy);

        rcf_pch_rsrc_info("/agent/ip6_fw",
                          rcf_pch_rsrc_grab_dummy,
                          rcf_pch_rsrc_release_dummy);

        if (ta_unix_conf_route_init() != 0)
            goto fail;

#ifdef RCF_RPC
        /* Link RPC nodes */
        rcf_pch_rpc_init();
#endif

#ifdef CFG_UNIX_DAEMONS
        if (ta_unix_conf_daemons_init() != 0)
            goto fail;
#endif
#ifdef WITH_ISCSI
        if (ta_unix_iscsi_target_init() != 0)
            goto fail;
        if (iscsi_initiator_conf_init() != 0)
            goto fail;
#endif        
#ifdef ENABLE_WIFI_SUPPORT
        if (ta_unix_conf_wifi_init() != 0)
            goto fail;
#endif
#ifdef ENABLE_8021X
        if (ta_unix_conf_supplicant_init() != 0)
            goto fail;
#endif
#ifdef ENABLE_IFCONFIG_STATS
        if (ta_unix_conf_net_if_stats_init() != 0)
            goto fail;
#endif
#ifdef ENABLE_NET_SNMP_STATS
        if (ta_unix_conf_net_snmp_stats_init() != 0)
            goto fail;
#endif
        
        if (ta_unix_conf_sys_init() != 0)
            goto fail;
        
        /* Initialize configurator PHY support */
        if (ta_unix_conf_phy_init() != 0)
            goto fail;
        
        rcf_pch_rsrc_init();
    }

    return &node_agent;

fail:
    if (cfg_socket >= 0)
    {
        close(cfg_socket);
        cfg_socket = -1;
    }
    if (cfg6_socket >= 0)
    {
        close(cfg6_socket);
        cfg6_socket = -1;
    }
    return NULL;
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
    if (cfg6_socket >= 0)
        (void)close(cfg6_socket);
}

#if SOLARIS_IP_FW
/**
 * Set or obtain the value of IP forwarding variable on Solaris.
 *
 * @param ipfw_str      name: "ip_forwarding" or "ip6_forwarding"
 * @param p_val         location of the value: 0 or 1 to set the variable,
 *                      other - to read into the location (IN/OUT).
 *
 * @return              Status code.
 */
static te_errno
ipforward_solaris(char *ipfw_str, int *p_val)
{
    int            fd;
    char            xbuf[16 * 1024];
    struct strioctl si;
    int             rc;


    if ((fd = open("/dev/ip", O_RDWR)) < 0)
        return TE_OS_RC(TE_TA_UNIX, errno);
    
    strncpy(xbuf, ipfw_str, sizeof(xbuf));

    si.ic_cmd = ND_GET;
    if (*p_val == 0 || *p_val == 1)
    {
        si.ic_cmd = ND_SET;
        /* paramname\0value\0 */
        snprintf(xbuf + strlen(xbuf) + 1, 2, "%d", *p_val);
    }
    si.ic_timout = 0;  /* 0 means a default value of 15s */
    si.ic_len = sizeof(xbuf);
    si.ic_dp = xbuf;

    if ((rc = ioctl(fd, I_STR, &si)) < 0)
    {
        close(fd);
        return TE_OS_RC(TE_TA_UNIX, errno);
    }

    *p_val = atoi(xbuf);
    close(fd);
    return 0;
}
#endif

#if BSD_IP_FW
/**
 * Set or obtain the value of IP forwarding variable on BSD.
 *
 * @param ip6           FALSE for IPv4, TRUE for IPv6
 * @param p_val         location of the value: 0 or 1 to set the variable,
 *                      other - to read into the location (IN/OUT).
 *
 * @return              Status code.
 */
static te_errno
ipforward_bsd(te_bool ip6, int *p_val)
{
    int rc;
#define MIB_SZ 4
    int mib_v4[MIB_SZ] =
    {
        CTL_NET,
        PF_INET,
        IPPROTO_IP,
        IPCTL_FORWARDING
    };
    int mib_v6[MIB_SZ] =
    {
        CTL_NET,
        PF_INET6,
        IPPROTO_IPV6,
        IPV6CTL_FORWARDING
    };
    int *mib = mib_v4;
    size_t val_sz = sizeof(*p_val);


    if (ip6)
       mib = mib_v6;
    
    if (*p_val == 0 || *p_val == 1)
        rc = sysctl(mib, MIB_SZ, NULL, NULL, p_val, val_sz);
    else
        rc = sysctl(mib, MIB_SZ, p_val, &val_sz, NULL, 0);

    if (rc  < 0)
        return TE_OS_RC(TE_TA_UNIX, errno);

    return 0;
#undef MIB_SZ
}
#endif

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
#if __linux__
    char c = '0';
    int  fd;
#endif
#if defined(SOLARIS_IP_FW) || defined(BSD_IP_FW)
    te_errno    rc;
    int         ival;
#endif
    UNUSED(gid);
    UNUSED(oid);

    if (!rcf_pch_rsrc_accessible("/agent/ip4_fw"))
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

#if __linux__
    if ((fd = open("/proc/sys/net/ipv4/ip_forward", O_RDONLY)) < 0)
        return TE_OS_RC(TE_TA_UNIX, errno);

    if (read(fd, &c, 1) < 0)
    {
        close(fd);
        return TE_OS_RC(TE_TA_UNIX, errno);
    }
    close(fd);

    sprintf(value, "%d", c == '0' ? 0 : 1);
    
#elif SOLARIS_IP_FW
    ival = 2; /* anything except 0|1 is read */    
    rc = ipforward_solaris("ip_forwarding", &ival);
    if (rc != 0)
        return rc;
    sprintf(value, "%d", ival);

#elif BSD_IP_FW
    ival = 2;
    rc = ipforward_bsd(FALSE, &ival); /* FALSE if not ip6 */
    if (rc != 0)
        return rc;
    sprintf(value, "%d", ival);

#else
    /* Assume that forwarding is disabled */
    sprintf(value, "%d", 0); 
#endif

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
#if __linux__
    int fd;
#endif
#if defined(SOLARIS_IP_FW) || defined(BSD_IP_FW)
    te_errno rc;
    int ival;
#endif
    UNUSED(gid);
    UNUSED(oid);
    
    if (!rcf_pch_rsrc_accessible("/agent/ip4_fw"))
        return TE_RC(TE_TA_UNIX, TE_EPERM);

    if ((*value != '0' && *value != '1') || *(value + 1) != 0)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

#if __linux__
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
    
#elif SOLARIS_IP_FW
    ival = atoi(value);    
    rc = ipforward_solaris("ip_forwarding", &ival);
    if (rc != 0)
        return rc;

#elif BSD_IP_FW
    ival = atoi(value);
    rc = ipforward_bsd(FALSE, &ival); /* FALSE if not ip6 */
    if (rc != 0)
        return rc;

#else
    return TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif

    return 0;
}

/**
 * Obtain value of the IPv6 forwarding sustem variable.
 *
 * @param gid           group identifier (unused)
 * @param oid           full instance identifier (unused)
 * @param value         value location
 *
 * @return              Status code
 */
static te_errno
ip6_fw_get(unsigned int gid, const char *oid, char *value)
{
#if __linux__
    int  fd;
    char c = '0';
#endif
#if defined(SOLARIS_IP_FW) || defined(BSD_IP_FW)
    te_errno    rc;
    int         ival;
#endif
    UNUSED(gid);
    UNUSED(oid);

    if (!rcf_pch_rsrc_accessible("/agent/ip6_fw"))
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

#if __linux__
    if ((fd = open("/proc/sys/net/ipv6/conf/all/forwarding",
                   O_RDONLY)) < 0)
        return TE_OS_RC(TE_TA_UNIX, errno);

    if (read(fd, &c, 1) < 0)
    {
        close(fd);
        return TE_OS_RC(TE_TA_UNIX, errno);
    }
    close(fd);
    
    sprintf(value, "%d", c == '0' ? 0 : 1);

#elif SOLARIS_IP_FW
    ival = 2; /* anything except 0|1 is read */    
    rc = ipforward_solaris("ip6_forwarding", &ival);
    if (rc != 0)
        return rc;
    sprintf(value, "%d", ival);

#elif BSD_IP_FW
    ival = 2;
    rc = ipforward_bsd(TRUE, &ival); /* FALSE if not ip6 */
    if (rc != 0)
        return rc;
    sprintf(value, "%d", ival);

#else
    /* Assume that forwarding is disabled */
    sprintf(value, "%d", 0);
#endif

    return 0;
}   /* ip6_fw_get() */

/**
 * Enable/disable IPv6 forwarding.
 *
 * @param gid           group identifier (unused)
 * @param oid           full instance identifier (unused)
 * @param value         pointer to new value of IPv6 forwarding system
 *                      variable
 *
 * @return              Status code
 */
static te_errno
ip6_fw_set(unsigned int gid, const char *oid, const char *value)
{
#if __linux__
    int fd;
#endif
#if defined(SOLARIS_IP_FW) || defined(BSD_IP_FW)
    te_errno rc;
    int ival;
#endif    
    UNUSED(gid);
    UNUSED(oid);

    
    if (!rcf_pch_rsrc_accessible("/agent/ip6_fw"))
        return TE_RC(TE_TA_UNIX, TE_EPERM);

    if ((*value != '0' && *value != '1') || *(value + 1) != 0)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    
#if __linux__
    fd = open("/proc/sys/net/ipv6/conf/all/forwarding",
              O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd < 0)
        return TE_OS_RC(TE_TA_UNIX, errno);

    if (write(fd, *value == '0' ? "0\n" : "1\n", 2) < 0)
    {
        close(fd);
        return TE_OS_RC(TE_TA_UNIX, errno);
    }
    close(fd);
    
#elif SOLARIS_IP_FW
    ival = atoi(value);    
    rc = ipforward_solaris("ip6_forwarding", &ival);
    if (rc != 0)
        return rc;

#elif BSD_IP_FW
    ival = atoi(value);
    rc = ipforward_bsd(TRUE, &ival); /* FALSE if not ip6 */
    if (rc != 0)
        return rc;

#else
    return TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif

    return 0;
}   /* ip6_fw_set() */

/**
 * Convert and check address prefix value.
 *
 * @param value     Pointer to the new network mask in dotted notation
 * @param family    Address family
 *
 * @return Status code.
 */
static te_errno
prefix_check(const char *value, sa_family_t family, unsigned int *prefix)
{
    char *end;

    if (family != AF_INET && family != AF_INET6)
    {
        ERROR("%s(): unsupported address family %d", __FUNCTION__,
              (int)family);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    *prefix = strtoul(value, &end, 10);
    if (value == end)
    {
        ERROR("Invalid value '%s' of prefix length", value);
        return TE_RC(TE_TA_UNIX, TE_EFMT);
    }
    if (((family == AF_INET) &&
         (*prefix > (sizeof(struct in_addr) << 3))) ||
        ((family == AF_INET6) &&
         (*prefix > (sizeof(struct in6_addr) << 3))))
    {
        ERROR("Invalid prefix '%s' to be set", value);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    return 0;
}

#ifdef USE_NETLINK
/**
 * Store answer from RTM_GETXXX in netlink message list.
 *
 * @param who  address pointer
 * @param msg  address info to be stored
 * @param arg  list for netlink messages, which should be filled in
 *
 * @retval  0  success
 * @retval -1  failure
 */
static int
store_nlmsg(const struct sockaddr_nl *who,
            struct nlmsghdr *msg, void *arg)
{
    agt_nlmsg_list  *list = (agt_nlmsg_list *)arg;
    agt_nlmsg_entry *entry;

    /*
     * Allocate contiguous piece of memory for "list entry" and
     * netlink message:
     * [List Entry: <hdr> ] [netlink message]
     *                |      ^
     *                |------|
     */
    entry = (agt_nlmsg_entry *)malloc(sizeof(*entry) + msg->nlmsg_len);
    if (entry == NULL)
        return -1;

    entry->hdr = (struct nlmsghdr *)(entry + 1);
    memcpy(entry->hdr, msg, msg->nlmsg_len);

    TAILQ_INSERT_TAIL(list, entry, links);

    return ll_remember_index(who, msg, NULL);
}

/**
 * Free nlmsg list.
 *
 * @param linfo   nlmsg list to be freed
 */
static void
free_nlmsg_list(agt_nlmsg_list *list)
{
    agt_nlmsg_entry *entry;

    while ((entry = TAILQ_FIRST(list)) != NULL)
    {
        TAILQ_REMOVE(list, entry, links);
        free(entry);
    }
}

/**
 * Get link/protocol addresses information from all interfaces.
 *
 * @param family  AF_INET, AF_INET6
 * @param list    list to be filled in with address entries
 *
 * @return Status code
 */
static te_errno
ip_addr_get(int family, agt_nlmsg_list *list)
{
    struct rtnl_handle rth;

    if (family != AF_INET && family != AF_INET6)
    {
        ERROR("%s: invalid address family (%d)", __FUNCTION__, family);
        return TE_RC(TE_TA_UNIX, TE_EAFNOSUPPORT);
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
    agt_nlmsg_list     addr_list;
    agt_nlmsg_entry   *a;
    struct nlmsghdr   *n = NULL;
    struct ifaddrmsg  *ifa = NULL;
    struct rtattr     *rta_tb[IFA_MAX + 1];
    int                ifindex = 0;
    sa_family_t        family;
    int                rc;


    TAILQ_INIT(&addr_list);

    /* If address contains a colon, it is IPv6 address */
    family = str_addr_family(str_addr);

    if (bcast != NULL)
        memset(bcast, 0, sizeof(*bcast));
    
    if (ifname != NULL && (strlen(ifname) >= IF_NAMESIZE))
    {
        ERROR("Interface name '%s' too long", ifname);
        return NULL;
    }

    if ((rc = inet_pton(family, str_addr, &ip_addr)) <= 0)
    {
        ERROR("%s(): inet_pton() failed for address '%s': %s",
              __FUNCTION__, str_addr,
              (rc < 0) ? 
              "Address family not supported" : "Incorrect address");
        return NULL;
    }
       
    if (ip_addr_get(family, &addr_list) != 0)
    {
        ERROR("%s(): Cannot get addresses list", __FUNCTION__);
        return NULL;
    }

    TAILQ_FOREACH(a, &addr_list, links)
    {
        n = a->hdr;
        ifa = NLMSG_DATA(n);

        if (n->nlmsg_len < NLMSG_LENGTH(sizeof(ifa)))
        {
            ERROR("%s(): Bad netlink message header length", __FUNCTION__);
            free_nlmsg_list(&addr_list);
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
                    ((int)if_nametoindex(ifname) == ifa->ifa_index))
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

    free_nlmsg_list(&addr_list);

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
        lcl.bitlen = prefix;
    }
    else
    {
        assert(family == AF_INET6);
        lcl.bytelen = AF_INET6_DEFAULT_BYTELEN;
        lcl.bitlen = prefix;
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
    req.ifa.ifa_index = if_nametoindex(ifname);

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
    sa_family_t     family;
    gen_ip_address  ip_addr;

    /* If address contains ':', it is IPv6 address */
    family = str_addr_family(addr);

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
                                &ip_addr, prefix, 0);
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
te_errno
ta_unix_conf_get_addr(const char *ifname, sa_family_t af, void **addr)
{
    strncpy(req.my_ifr_name, ifname, sizeof(req.my_ifr_name));
    CFG_IOCTL((af == AF_INET6) ? cfg6_socket : cfg_socket,
              MY_SIOCGIFADDR, &req);
    if (af == AF_INET)
        *addr = &SIN(&req.my_ifr_addr)->sin_addr;
    else
        *addr = &SIN6(&req.my_ifr_addr)->sin6_addr;
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

    strcpy(req.my_ifr_name, ifname);

    SA(&req.my_ifr_addr)->sa_family = AF_INET;
    SIN(&(req.my_ifr_addr))->sin_addr.s_addr = htonl(mask);
    CFG_IOCTL(cfg_socket, MY_SIOCSIFNETMASK, &req);
    return 0;
}
#endif


#ifdef USE_IOCTL

/**
 * Get interfaces configuration via SIOCGIFCONF/SIOCGLIFCONF to 'buf'.
 * Be carefull, if HAVE_STRUCT_LIFREQ, 'buf' contains array of
 * 'struct lifreq', otherwise - array of 'struct ifreq'.
 *
 * @param buf       Location for pointer to the allocated buffer.
 * @param p_req     Location for pointer to the first interface data.
 * @param p_len     Location for length of returned data
 *
 * @return Status code.
 */
static te_errno
get_ifconf_to_buf(void **buf, void **p_req, size_t *p_len)
{
#if HAVE_STRUCT_LIFREQ
    {
        struct lifnum   ifnum;
        struct lifconf  conf;

        ifnum.lifn_family = conf.lifc_family = AF_UNSPEC;
        ifnum.lifn_flags = conf.lifc_flags = 0;
        CFG_IOCTL(cfg_socket, SIOCGLIFNUM, &ifnum);

        *buf = calloc(ifnum.lifn_count + 1, sizeof(struct lifreq));
        if (*buf == NULL)
            return TE_RC(TE_TA_UNIX, TE_ENOMEM);

        conf.lifc_len = sizeof(struct lifreq) * (ifnum.lifn_count + 1);
        conf.lifc_buf = (caddr_t)*buf;

        CFG_IOCTL(cfg_socket, SIOCGLIFCONF, &conf);

        *p_req = conf.lifc_req;
        *p_len = conf.lifc_len;
    }
#else /* !HAVE_STRUCT_LIFREQ */
    {
        struct ifconf   conf;

        *buf = calloc(32, sizeof(struct ifreq));
        if (*buf == NULL)
            return TE_RC(TE_TA_UNIX, TE_ENOMEM);

        conf.ifc_len = sizeof(struct ifreq) * 32;
        conf.ifc_buf = (caddr_t)*buf;

        CFG_IOCTL(cfg_socket, SIOCGIFCONF, &conf);

        *p_req = conf.ifc_req;
        *p_len = conf.ifc_len;
    }
#endif /* !HAVE_STRUCT_LIFREQ */
    return 0;
}

/**
 * Call function provided by user for each 'struct ifreq'-like entry
 * in 'struct ifconf'-like buffer.
 *
 * @param ifr           Pointer to 'struct ifreq'-like structure to start
 * @param length        Length of the 'struct ifconf'-like buffer
 * @param ifreq_cb      Function to be called for each entry
 * @param opaque        Opaque data to be passed in callback
 *
 * @return Status code.
 */
static te_errno
ifconf_foreach_ifreq(struct my_ifreq *ifr, size_t length,
                     te_errno (*ifreq_cb)(struct my_ifreq *, void *),
                     void *opaque)
{
    te_errno    rc = 0;
    size_t      step;

    assert(ifr != NULL);
    while (rc == 0 && length >= sizeof(struct my_ifreq))
    {
#ifdef _SIZEOF_ADDR_IFREQ
        /* 
         * The check may be done, iff avaialbe length is greater or
         * equal to minimum entry size which is checked by while
         * condition.
         */
        step = _SIZEOF_ADDR_IFREQ(*ifr);
#else
        step = sizeof(struct my_ifreq);
#endif
        /* Re-check step vs length once more */
        if (step > length)
            break;

        rc = ifreq_cb(ifr, opaque);

        ifr = (struct my_ifreq *)((caddr_t)ifr + step);
        length -= step;
    }

    return rc;
}

#endif /* USE_IOCTL */


#if !defined(__linux__) && defined(SIOCGIFCONF)

static te_errno
ifreq_ifname_search_cb(struct my_ifreq *ifr, void *opaque)
{
    if (ifr == opaque)
        return TE_ENOENT;
    else if (strcmp(ifr->my_ifr_name,
                    ((const struct my_ifreq *)opaque)->my_ifr_name) == 0)
        return TE_EEXIST;
    else
        return 0;
}

/** Opaque data for interface_list_ifreq_cb() */
struct interface_list_ifreq_cb_data {
    struct my_ifreq    *first;      /**< First entry to check dups */
    size_t              length;     /**< Total length to check dups */
    char               *buf;        /**< Buffer to print names */
    size_t              buf_len;    /**< Total length of the buffer */
    size_t              buf_off;    /**< Current offset in the buffer */
};

static te_errno 
interface_list_ifreq_cb(struct my_ifreq *ifr, void *opaque)
{
    struct interface_list_ifreq_cb_data *data = opaque;

    /* Aliases, logical and alien interfaces are skipped here */
    if (CHECK_INTERFACE(ifr->my_ifr_name) != 0)
        return 0;

    /* Skip duplicates */
    if (ifconf_foreach_ifreq(data->first, data->length,
                             ifreq_ifname_search_cb, ifr) == TE_EEXIST)
        return 0;

    data->buf_off += snprintf(data->buf + data->buf_off,
                              data->buf_len - data->buf_off,
                              "%s ", ifr->my_ifr_name);

    return 0;
}

#endif






/**
 * Get list of VLANs on particular physical device
 *
 * If there are no VLAN children under passed interface, 'n_vlans'
 * set to zero.
 *
 * @param devname       name of network device
 * @param n_vlans       number of vlans (IN/OUT)
 * @param vlans         location for vlan IDs (OUT)
 *
 * @return status code
 */
te_errno
ta_vlan_get_children(const char *devname, size_t *n_vlans, int *vlans)
{
    char f_buf[200];

    if (devname == NULL ||n_vlans == NULL || vlans == NULL)
        return TE_RC(TE_TA_UNIX, TE_EINVAL); 

    VERB("%s(): enter for device: <%s>", __FUNCTION__, devname);
    *n_vlans = 0;
#if defined __linux__
    {
        FILE   *proc_vlans = fopen("/proc/net/vlan/config", "r");
        int     vlan_id;

        if (proc_vlans == NULL)
        {
            if (errno == ENOENT)
            {
                /* 
                 * No vlan support module loaded, empty list. 
                 * Do not RING() here -- do not spam into the log.
                 */
                VERB("%s: no proc vlan file", __FUNCTION__);
                return 0;
            }

            ERROR("%s(): Failed to open /proc/net/vlan/config %s",
                  __FUNCTION__, strerror(errno));
            return TE_OS_RC(TE_TA_UNIX, errno);
        }
        while (fgets(f_buf, sizeof(f_buf), proc_vlans) != NULL)
        {
            char   *s = strchr(f_buf, '|');
            size_t  space_ofs;

            if (s == NULL)
                continue;
            s++;
            while (isspace(*s)) s++;
            if (!isdigit(*s))
                continue;
            vlan_id = atoi(s);

            s = strchr(s, '|');
            if (s == NULL)
                continue;
            s++;
            while (isspace(*s)) s++;

            space_ofs = strcspn(s, " \t\n\r");
            s[space_ofs] = 0; 

            if (strcmp(s, devname) == 0)
                vlans[(*n_vlans)++] = vlan_id;
        }
        (void)fclose(proc_vlans);
    }
#elif defined __sun__
    { 
        int   out_fd = -1;
        FILE *out;
        int   status;
        pid_t dladm_cmd_pid = te_shell_cmd("LANG=POSIX dladm show-link -p",
                                           -1, NULL, &out_fd, NULL);
        if (dladm_cmd_pid < 0)
        {
            ERROR("%s(): start of dladm failed", __FUNCTION__);
            return TE_RC(TE_TA_UNIX, TE_ESHCMD);
        }
        
        out = fdopen(out_fd, "r");
        while (fgets(f_buf, sizeof(f_buf), out) != NULL)
        { 
            size_t ofs; 
            char *s = f_buf;
            int vlan_id;

            VERB("%s(): read line: <%s>", __FUNCTION__, f_buf);
            /* skip "<ifname> type=" */
            s = strchr(s, ' '); 
            s++;
            s = strchr(s, '='); 
            s++;
            if (strncmp(s, "vlan", 4) != 0)
                continue;
            s += sizeof("vlan") - 1;
            vlan_id = atoi(s);

            s = strstr(s, "device=");
            if (s == NULL)
                continue;
            VERB("%s(): find vlan: %d, s: <%s>", __FUNCTION__, vlan_id, s);
            s += sizeof("device=") - 1; 
            ofs = strcspn(s," \n\r\t");
            s[ofs] = 0;
            if (strcmp(s, devname) != 0)
                continue;

            vlans[(*n_vlans)++] = vlan_id;
        }
        
        ta_waitpid(dladm_cmd_pid, &status, 0);
        if (status != 0)
        {
            ERROR("%s(): Non-zero status of dladm: %d",
                  __FUNCTION__, status);
            return TE_RC(TE_TA_UNIX, TE_ESHCMD);
        }
        fclose(out);
    }
#endif

    return 0;
}

/**
 * Get VLAN ifname
 *
 * @param devname       name of network device
 * @param vlan_id       VLAN id
 * @param v_ifname      location for VLAN ifname, 
 *
 * @return status
 */
static te_errno
vlan_ifname_get_internal(const char *ifname, int vlan_id, 
                         char *v_ifname)
{
#if defined __linux__
    sprintf(v_ifname, "%s.%d", ifname, vlan_id); 
#elif defined __sun__ 
    size_t offset = 0;
    while (!isdigit(ifname[offset])) offset++;

    memcpy(v_ifname, ifname, offset);
    sprintf(v_ifname + offset, "%d", vlan_id * 1000 + 
            atoi(ifname + offset));
#endif 
    return 0;
}

/**
 * Get VLAN ifname
 *
 * @param gid           request group identifier (unused)
 * @param oid           full object instence identifier
 * @param value         location for interface name
 * @param ifname        name of the interface
 * @param vid           name of the vlan (decimal integer)
 *
 * @return status
 */
static te_errno
vlan_ifname_get(unsigned int gid, const char *oid, char *value,
                const char *ifname, const char *vid)
{
    int vlan_id = atoi(vid);

    VERB("%s: gid=%u oid='%s', ifname = '%s', vid %d",
         __FUNCTION__, gid, oid, ifname,  vlan_id); 


    return vlan_ifname_get_internal(ifname, vlan_id, value);
}

/**
 * Get parent device name of VLAN interface.
 * If passed interface is not VLAN, method sets 'parent' to empty string
 * and return success.
 * 
 * @param ifname        interface name
 * @param parent        location of parent interface name,
 *                      IF_NAMESIZE buffer length(OUT)
 *
 * @return status
 */
te_errno
ta_vlan_get_parent(const char *ifname, char *parent)
{
    char f_buf[200];

    *parent = 0;
#if defined __linux__
    {
        FILE *proc_vlans = fopen("/proc/net/vlan/config", "r");

        if (proc_vlans == NULL)
        {
            if (errno == ENOENT)
            {
                VERB("%s: no proc vlan file ", __FUNCTION__);
                return 0; /* no vlan support module loaded, no parent */
            }

            ERROR("%s(): Failed to open /proc/net/vlan/config %s",
                  __FUNCTION__, strerror(errno));
            return TE_OS_RC(TE_TA_UNIX, errno);
        }
        while (fgets(f_buf, sizeof(f_buf), proc_vlans) != NULL)
        {
            size_t space_ofs;
            char *s = f_buf;
            char *p = parent;


            space_ofs = strcspn(s, " \t\n\r");
            s[space_ofs] = 0; 


            if (strcmp(s, ifname) != 0)
                continue;

            s += space_ofs + 1;

            s = strchr(s, '|');
            s++;
            s = strchr(s, '|');
            s++;

            while (isspace(*s)) s++;

            while (!isspace(*s)) 
                *p++ = *s++;
            *p = 0;
            break;
        }
        fclose(proc_vlans);
    }
#elif defined __sun__
    {
        int   out_fd = -1;
        FILE *out;
        int   status;
        pid_t dladm_cmd_pid = te_shell_cmd("LANG=POSIX dladm show-link -p",
                                           -1, NULL, &out_fd, NULL);
        VERB("%s(<%s>): cmd pid %d, out fd %d",
             __FUNCTION__, ifname, (int)dladm_cmd_pid, out_fd);
        if (dladm_cmd_pid < 0)
        {
            ERROR("%s(): start of dladm failed", __FUNCTION__);
            return TE_RC(TE_TA_UNIX, TE_ESHCMD);
        }
        
        out = fdopen(out_fd, "r");
        while (fgets(f_buf, sizeof(f_buf), out) != NULL)
        { 
            char *s = strchr(f_buf, ' ');
            char *p;
            *s = 0;
            s++;
            if (strcmp(ifname, f_buf) != 0)
                continue;
            if (strncmp(s, "type=vlan ", sizeof("type=vlan ") - 1) != 0)
                continue; 

            VERB("%s(): found parent <%s> for if <%s>", 
                 __FUNCTION__, s, ifname);

            s = strstr(s, "device");
            if (s == NULL)
                continue;
            s = strchr(s, '=');
            if (*s) s++; /* skip '=' */
            p = parent;
            while(*s != 0 && !isspace(*s))
                *(p++) = *(s++);
            *p = 0;
            break;
        }

        ta_waitpid(dladm_cmd_pid, &status, 0);
        if (status != 0)
        {
            ERROR("%s(): Non-zero status of dladm: %d",
                  __FUNCTION__, status);
            return TE_RC(TE_TA_UNIX, TE_ESHCMD);
        }
        fclose(out);
    }
#endif
    return 0;
}


/**
 * Get instance list for object "agent/interface/vlans".
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
vlans_list(unsigned int gid, const char *oid, char **list,
           const char *ifname)
{
    size_t n_vlans = MAX_VLANS;
    size_t i;
    te_errno rc;

    char *b;

    rc = ta_vlan_get_children(ifname, &n_vlans, vlans_buffer);
    if (rc != 0)
        return rc;

    VERB("%s: gid=%u oid='%s', ifname %s, num vlans %d",
         __FUNCTION__, gid, oid, ifname, n_vlans);

    if (n_vlans == 0) 
    {
        *list = NULL;
        return 0;
    }

    b = *list = malloc(n_vlans * 5 /* max digits in VLAN id + space */ + 1);
    if (*list == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM); 

    for (i = 0; i < n_vlans; i++)
        b += sprintf(b, "%d ", vlans_buffer[i]); 

    return 0;
}

/**
 * Add VLAN Ethernet device.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier 
 * @param value         value string 
 * @param ifname        device name, over it VLAN should be added 
 * @param vid_str       VLAN id string, decimal notation
 *
 * @return              Status code
 */
static te_errno
vlans_add(unsigned int gid, const char *oid, const char *value,
              const char *ifname, const char *vid_str)
{
#if LINUX_VLAN_SUPPORT
    struct vlan_ioctl_args if_request;
#endif
    int vid = atoi(vid_str);
    int l_errno = 0;
    te_errno rc;

    UNUSED(value);

    VERB("%s: gid=%u oid='%s', vid %s, ifname %s, errno %d",
         __FUNCTION__, gid, oid, vid_str, ifname, l_errno);


    if ((rc = CHECK_INTERFACE(ifname)) != 0)
    {
        return TE_RC(TE_TA_UNIX, rc);
    }

#if LINUX_VLAN_SUPPORT
    if (cfg_socket < 0)
    {
        ERROR("%s: non-init cfg socket", cfg_socket);
        return TE_RC(TE_TA_UNIX, TE_EFAULT); 
    }
    if_request.cmd = ADD_VLAN_CMD;
    strcpy(if_request.device1, ifname);
    if_request.u.VID = vid;

    if (ioctl(cfg_socket, SIOCSIFVLAN, &if_request) < 0)
        l_errno = errno; 

#elif defined __sun__
    {
        char vlan_if_name[IFNAMSIZ];
        vlan_ifname_get_internal(ifname, vid, vlan_if_name);

        sprintf(buf, "LANG=POSIX ifconfig %s plumb >/dev/null",
                vlan_if_name);
        return ta_system(buf) != 0 ? TE_RC(TE_TA_UNIX, TE_ESHCMD) : 0;
    }
#else
    ERROR("This test agent does not support VLANs");
    return TE_RC(TE_TA_UNIX, EINVAL);
#endif
    return TE_RC(TE_TA_UNIX, l_errno); 
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
vlans_del(unsigned int gid, const char *oid, const char *ifname,
          const char *vid_str)
{
#if LINUX_VLAN_SUPPORT
    struct vlan_ioctl_args if_request;
#endif
    int vid = atoi(vid_str);
    int l_errno = 0;

    te_errno rc;

    if ((rc = CHECK_INTERFACE(ifname)) != 0)
    {
        return TE_RC(TE_TA_UNIX, rc);
    }

#if LINUX_VLAN_SUPPORT
    if (cfg_socket < 0)
    {
        ERROR("%s: non-init cfg socket", cfg_socket);
        return TE_RC(TE_TA_UNIX, TE_EFAULT); 
    }
    if_request.cmd = DEL_VLAN_CMD;
    vlan_ifname_get_internal(ifname, vid, if_request.device1);
    if_request.u.VID = vid;

    if (ioctl(cfg_socket, SIOCSIFVLAN, &if_request) < 0)
        l_errno = errno;
#elif defined __sun__
    {
        char vlan_if_name[IFNAMSIZ];
        vlan_ifname_get_internal(ifname, vid, vlan_if_name);

        sprintf(buf, "LANG=POSIX ifconfig %s unplumb >/dev/null",
                vlan_if_name);
        return ta_system(buf) != 0 ? TE_RC(TE_TA_UNIX, TE_ESHCMD) : 0;
    }
#else
    ERROR("This test agent does not support VLANs");
    return TE_RC(TE_TA_UNIX, EINVAL);
#endif


    VERB("%s: gid=%u oid='%s', vid %s, ifname %s, errno %d",
         __FUNCTION__, gid, oid, vid_str, ifname, l_errno);

    return TE_RC(TE_TA_UNIX, l_errno); 
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
#elif defined(SIOCGIFCONF)
    {
        te_errno                                rc;
        void                                   *ifconf_buf = NULL;
        size_t                                  ifconf_len;
        struct my_ifreq                        *first_req;
        struct interface_list_ifreq_cb_data     data;

        rc = get_ifconf_to_buf(&ifconf_buf, (void **)&first_req,
                               &ifconf_len);
        if (rc != 0)
        {
            free(ifconf_buf);
            return rc;
        }

        data.first = first_req;
        data.length = ifconf_len;
        data.buf = buf;
        data.buf_len = sizeof(buf);
        data.buf_off = 0;
        rc = ifconf_foreach_ifreq(first_req, ifconf_len,
                                  interface_list_ifreq_cb, &data);
        free(ifconf_buf);
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
    te_errno            rc;
    void               *ifconf_buf = NULL;
    size_t              ifconf_len;
    struct my_ifreq    *req;

    te_bool     first = TRUE;
    char       *name = NULL;
    char       *ptr = buf;

    rc = get_ifconf_to_buf(&ifconf_buf, (void **)&req, &ifconf_len);
    if (rc != 0)
    {
        free(ifconf_buf);
        return rc;
    }

    for (; *(req->my_ifr_name) != 0; req++)
    {
        if (name != NULL && strcmp(req->my_ifr_name, name) == 0)
            continue;

        name = req->my_ifr_name;

        if (first)
        {
            buf[0] = 0;
            first = FALSE;
        }
        ptr += sprintf(ptr, "%s ", name);
    }
    free(ifconf_buf);

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
#ifdef HAVE_LIBDLPI
static te_errno
mcast_link_addr_change_dlpi(dlpi_handle_t hnd, const char *addr, int op)
{
    uint8_t     mac_addr[ETHER_ADDR_LEN];
    int         i;
    char       *p;
    te_errno    rc;
    
    for (i = 0, p = addr; i < ETHER_ADDR_LEN; i++, p++)
    {
        unsigned tmp = strtoul(p, (char **)&p, 16);

        if (tmp > UCHAR_MAX)
            return TE_RC(TE_TA_UNIX, TE_EINVAL);
        if (*p != ':' && (*p != '\0' || i < ETHER_ADDR_LEN - 1))
            return TE_RC(TE_TA_UNIX, TE_EINVAL);
        mac_addr[i] = tmp;
    }
    if (op == SIOCADDMULTI)
    {
        if ((rc = dlpi_enabmulti(hnd, mac_addr, ETHER_ADDR_LEN)) != 
            DLPI_SUCCESS)
        {
            ERROR("dlpi_enabmulti() failed, rc = %x", rc);
            return TE_EINVAL;
        }
    }
    else if (op == SIOCDELMULTI)
    {
        if ((rc = dlpi_disabmulti(hnd, mac_addr, ETHER_ADDR_LEN)) != 
            DLPI_SUCCESS)
        {
            ERROR("dlpi_disabmulti() failed, rc = %x", rc);
            return TE_EINVAL;
        }
    }
    else
    {
        ERROR("Invalid operation: %d", op);
        return TE_EINVAL;
    }
    return 0;
}
#endif
static te_errno
mcast_link_addr_change_ioctl(const char *ifname, const char *addr, int op)
{
    struct ifreq    request;
    int             i;
    const char     *p;
    uint8_t        *q;

    memset(&request, 0, sizeof(request));
    strncpy(request.ifr_name, ifname, IFNAMSIZ);
    /* Read MAC address */
#ifdef HAVE_STRUCT_IFREQ_IFR_HWADDR        
    q = (uint8_t *)request.ifr_hwaddr.sa_data;
#elif HAVE_STRUCT_IFREQ_IFR_ENADDR
    q = (uint8_t *)request.ifr_enaddr;
#else
    request.ifr_addr.sa_family = AF_LINK;
    {
        struct sockaddr_dl *sdl =
            (struct sockaddr_dl *)&(request.ifr_addr);

        sdl->sdl_alen = ETHER_ADDR_LEN;
        q = (uint8_t *)sdl->sdl_data;
    }
#endif            

    for (i = 0, p = addr; i < ETHER_ADDR_LEN; i++, p++)
    {
        unsigned tmp = strtoul(p, (char **)&p, 16);

        if (tmp > UCHAR_MAX)
            return TE_RC(TE_TA_UNIX, TE_EINVAL);
        if (*p != ':' && (*p != '\0' || i < ETHER_ADDR_LEN - 1))
            return TE_RC(TE_TA_UNIX, TE_EINVAL);
        q[i] = tmp;
    }
    if (ioctl(cfg_socket, op, &request) != 0)
    {
        te_errno rc = TE_OS_RC(TE_TA_UNIX, errno);
        ERROR("Changing multicast MAC address %s on %s failed: %r", 
              addr, ifname, rc);
        return rc;
    }
    
    return 0;
}

static te_errno
mcast_link_addr_add(unsigned int gid, const char *oid,
                    const char *value, const char *ifname, const char *addr)
{
    te_errno rc;
#ifndef __linux__    
    ifs_list_el *p = interface_stream_list;
    mma_list_el *q;
#endif    
    
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);
    
#ifdef __linux__    
    rc = mcast_link_addr_change_ioctl(ifname, addr, SIOCADDMULTI);
#else
    for (p = interface_stream_list;
         p != NULL && strcmp(p->ifname, ifname) != 0; p = p->next);
    if (p == NULL)
    {
        p = (ifs_list_el *)malloc(sizeof(ifs_list_el));
        strcpy(p->ifname, ifname);
        p->mcast_addresses = NULL;
        p->next = interface_stream_list;
#ifdef HAVE_LIBDLPI        
        dlpi_open(ifname, &p->fd, DLPI_NATIVE);
#endif        
        interface_stream_list = p;
    }

    for (q = p->mcast_addresses;
         q != NULL && strcmp(q->value, addr) != 0; q = q->next);
    if (q == NULL)
    {
        q = (mma_list_el *)malloc(sizeof(mma_list_el));
        /* Against setting too long value for MAC address */
        strncpy(q->value, addr, sizeof(q->value));
#ifdef HAVE_LIBDLPI        
        /* Adding the address via DLPI */
        rc = mcast_link_addr_change_dlpi(p->fd, addr, SIOCADDMULTI);
#endif
        q->next = p->mcast_addresses;
        p->mcast_addresses = q;
    }
#endif
    return rc;
}
 
static te_errno
mcast_link_addr_del(unsigned int gid, const char *oid, const char *ifname,
                    const char *addr)
{
    te_errno rc;
#ifndef __linux__
    ifs_list_el *p;
    mma_list_el *q;
#endif
    
    UNUSED(gid);
    UNUSED(oid);
#ifdef __linux__    
    rc = mcast_link_addr_change_ioctl(ifname, addr, SIOCDELMULTI);
/* there are problems with deleting neighbour discovery multicast addresses,
   when restoring configuration. 
   this is solely to shut up the configurator.
   yes, it's ugly, but there seems to be no other way...
*/
    if (rc == TE_RC(TE_TA_UNIX, TE_ENOENT) && 
        strncmp(addr, "33:33:", 6) == 0)
    {
        rc = 0;
    }
#else
    for (p = interface_stream_list;
         p != NULL && strcmp(p->ifname, ifname) != 0; p = p->next);
    if (p == NULL)
    {
        ERROR("No such interface: %s", ifname);
        return TE_RC(TE_TA_UNIX, TE_ENXIO);
    }
    if (strcmp(p->mcast_addresses->value, addr) == 0)
    {
#ifdef HAVE_LIBDLPI        
        /* Deleting address via DLPI */
        rc = mcast_link_addr_change_dlpi(p->fd, addr, SIOCDELMULTI);
#endif        
        q = p->mcast_addresses->next;
        free(p->mcast_addresses);
        p->mcast_addresses = q;
        if (q == NULL)
        {
#ifdef HAVE_LIBDLPI            
            dlpi_close(p->fd);
#endif            
            if (p == interface_stream_list)
            {
                interface_stream_list = p->next;
                free(p);
            }
            else
            {
                ifs_list_el *tmp;
                for (tmp = interface_stream_list;
                     tmp->next != p; tmp = tmp->next);
                tmp->next = p->next;
                free(p);
            }
        }
    }
    else
    {
        for (q = p->mcast_addresses;
             q->next != NULL && strcmp(addr, q->next->value) != 0;
             q = q->next);
        if (q->next == NULL)
        {
            ERROR("No such address: %s on interface %s", addr, ifname);
            return TE_RC(TE_TA_UNIX, TE_ENOENT);
        }
        else
        {
            mma_list_el *tmp = q->next->next;
#ifdef HAVE_LIBDLPI            
            /* Deleting address via DLPI */
            rc = mcast_link_addr_change_dlpi(p->fd, addr, SIOCDELMULTI);
#endif            
            free(q->next);
            q->next = tmp;
        }
    }

#endif            
    return rc;
}

static te_errno
mcast_link_addr_list(unsigned int gid, const char *oid, char **list,
                     const char *ifname)
{
    char       *s = NULL;
    int         sp = 0;         /* String Pointer */
    int         buf_segs = 1;

#define MMAC_ADDR_BUF_SIZE 16384 
#ifndef __linux__
    ifs_list_el *p = interface_stream_list;
    mma_list_el *tmp;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(ifname);

    s = (char *)malloc(MMAC_ADDR_BUF_SIZE);
    *s = '\0';
    for (p = interface_stream_list;
         (p != NULL) && (strcmp(p->ifname, ifname)) != 0; p = p->next);
    if (p != NULL)
    {
        for (tmp = p->mcast_addresses; tmp != NULL; tmp = tmp->next)
        {
            if (sp >= MMAC_ADDR_BUF_SIZE - ETHER_ADDR_LEN * 3)
            {
                s = realloc(s, (++buf_segs) * MMAC_ADDR_BUF_SIZE);
            }
            sp += sprintf(&s[sp], "%s ", tmp->value);
        }
    }
    else
    {
        return 0;
    }
#else
    FILE       *fd;
    char        ifn[IFNAMSIZ];
    char        addrstr[ETHER_ADDR_LEN * 3];

#define DEFAULT_MULTICAST_ETHER_ADDR_IPV4 "01005e000001"
#define DEFAULT_MULTICAST_ETHER_ADDR_IPV6 "333300000001"

    UNUSED(gid);
    UNUSED(oid);
    if ((fd = fopen("/proc/net/dev_mcast", "r")) == NULL)
        return TE_OS_RC(TE_TA_UNIX, errno);
    
    s = (char *)malloc(MMAC_ADDR_BUF_SIZE);
    *s = '\0';

    while (fscanf(fd, "%*d %s %*d %*d %s\n", ifn, addrstr) > 0)
    {
        /*
         * Read file and copy items with appropriate interface name
         * to the buffer, adding colons to MAC addresses.
         */
            
        if (strcmp(ifn, ifname) == 0)
        {
            int i;

            /* exclude `default' addresses */
            if (strcmp(addrstr, DEFAULT_MULTICAST_ETHER_ADDR_IPV4) == 0 ||
                strcmp(addrstr, DEFAULT_MULTICAST_ETHER_ADDR_IPV6) == 0)
                continue;

            for (i = 0; i < 6; i++)
            {
                if (sp >= MMAC_ADDR_BUF_SIZE - ETHER_ADDR_LEN * 3)
                {
                    s = realloc(s, (++buf_segs) * MMAC_ADDR_BUF_SIZE);
                }
                strncpy(&s[sp], &addrstr[i * 2], 2);
                sp += 2;
                s[sp++] = (i < 5) ? ':' : ' ';
                s[sp] = '\0';
            }
        }
    }
    fclose(fd);
#endif
    *list = s;
    return 0;
}

/**
 * Configure IPv4/IPv6 address for the interface.
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
static te_errno
net_addr_add(unsigned int gid, const char *oid, const char *value,
             const char *ifname, const char *addr)
{
    gen_ip_address  new_addr;
    te_errno        rc;
    sa_family_t     family = str_addr_family(addr);
    size_t          addrlen = (family == AF_INET) ?
                                  sizeof(struct in_addr) :
                                  sizeof(struct in6_addr);
    uint8_t         zeros[addrlen];
    unsigned int    prefix;
    char           *cur;
    char           *next;
#ifdef __linux__
    char            slots[32] = { 0, };
#endif

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);

    if (strlen(ifname) >= IF_NAMESIZE)
        return TE_RC(TE_TA_UNIX, TE_E2BIG);

    if ((rc = CHECK_INTERFACE(ifname)) != 0)
        return TE_RC(TE_TA_UNIX, rc);

    memset(zeros, 0, addrlen);

    if (((inet_pton(family, addr, &new_addr) <= 0) ||
        (memcmp(&new_addr, zeros, addrlen) == 0) ||
        ((family == AF_INET) &&
         (ntohl(new_addr.ip4_addr.s_addr) & 0xe0000000) == 0xe0000000)))
    {
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    rc = prefix_check(value, family, &prefix);
    if (rc != 0)
        return rc;

    if ((rc = aliases_list()) != 0)
        return rc;

    for (cur = buf; strlen(cur) > 0; cur = next)
    {
        void   *tmp_addr;

        next = strchr(cur, ' ');
        if (next != NULL)
        {
            *next++ = '\0';
            if (strlen(cur) == 0)
                continue;
        }

        rc = ta_unix_conf_get_addr(cur, family, &tmp_addr);
        if (rc == 0 && memcmp(tmp_addr, &new_addr, addrlen) == 0)
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

#ifdef __linux__
        slots[atoi(strchr(cur, ':') + 1)] = 1;
#endif
    }

#ifdef __linux__
    if (family != AF_INET)
    {
        ERROR("Only addition of IPv4 address is supported on Linux");
        return TE_RC(TE_TA_UNIX, TE_ENOSYS);
    }

    if (strlen(cur) != 0)
    {
        strncpy(req.my_ifr_name, cur, IFNAMSIZ);
    }
    else
    {
        unsigned int n;

        for (n = 0; n < sizeof(slots) && slots[n] != 0; n++);

        if (n == sizeof(slots))
            return TE_RC(TE_TA_UNIX, TE_EPERM);

        sprintf(trash, "%s:%d", ifname, n);
        strncpy(req.my_ifr_name, trash, IFNAMSIZ);
    }

    SIN(&req.my_ifr_addr)->sin_family = AF_INET;
    SIN(&req.my_ifr_addr)->sin_addr = new_addr.ip4_addr;
    CFG_IOCTL(cfg_socket, MY_SIOCSIFADDR, &req);

#elif defined(SIOCLIFADDIF)
    {
        /*
         * It is impossible to specify netmask/prefixlen when address
         * is added. We don't want to have address with incorrect mask,
         * since it can be fatal (unexpected routing, etc). Therefore,
         *  - if interface has specified address, add logical interface
         *    with unspecified address;
         *  - set required netmaks / prefix length;
         *  - set required address;
         *  - push interface up.
         */
        int             sock = (family == AF_INET6) ?
                               cfg6_socket : cfg_socket;
        struct lifreq   lreq;
        te_bool         logical_iface = FALSE;

        memset(&lreq, 0, sizeof(lreq));
        strncpy(lreq.lifr_name, ifname, sizeof(lreq.lifr_name));
        lreq.lifr_addr.ss_family = family;

        CFG_IOCTL(sock, SIOCGLIFADDR, &lreq);

        if (!te_sockaddr_is_wildcard(SA(&lreq.lifr_addr)))
        {
            logical_iface = TRUE;
            CFG_IOCTL(sock, SIOCLIFADDIF, &lreq);
            /* NOTE: Name of logical interface was set in 'lreq' */
        }

        te_sockaddr_mask_by_prefix(SA(&lreq.lifr_addr),
                                   sizeof(lreq.lifr_addr),
                                   family, prefix);
        CFG_IOCTL(sock, SIOCSLIFNETMASK, &lreq);

        lreq.lifr_addr.ss_family = family;
        memcpy((family == AF_INET) ?
                   (void *)&SIN(&lreq.lifr_addr)->sin_addr :
                   (void *)&SIN6(&lreq.lifr_addr)->sin6_addr,
               &new_addr, addrlen);
        CFG_IOCTL(sock, SIOCSLIFADDR, &lreq);

        if (logical_iface)
        {
            /* Push logical interface up */
            CFG_IOCTL(sock, SIOCGLIFFLAGS, &lreq);
            lreq.lifr_flags |= IFF_UP;
            CFG_IOCTL(sock, SIOCSLIFFLAGS, &lreq);
        }
    }
#elif defined(SIOCALIFADDR)
    {
        struct if_laddrreq lreq;

        memset(&lreq, 0, sizeof(lreq));
        strncpy(lreq.iflr_name, ifname, IFNAMSIZ);
        lreq.addr.ss_family = family;
        lreq.addr.ss_len =
            (family == AF_INET) ? sizeof(struct sockaddr_in) :
                                  sizeof(struct sockaddr_in6);
        if (inet_pton(family, addr,
                      (family == AF_INET) ?
                          (void *)&SIN(&lreq.addr)->sin_addr :
                          (void *)&SIN6(&lreq.addr)->sin6_addr) <= 0)
        {
            ERROR("inet_pton() failed");
            return TE_RC(TE_TA_UNIX, TE_EFMT);
        }
        CFG_IOCTL((family == AF_INET6) ? cfg6_socket : cfg_socket,
                  SIOCALIFADDR, &lreq);
    }
#else
    ERROR("%s(): %s", __FUNCTION__, strerror(EOPNOTSUPP));
    return TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP);
#endif

#if defined(__linux__) || (!defined(SIOCLIFADDIF) && defined(SIOCALIFADDR))
    /* SIOCLIFADDIF case sets prefix itself, so no need for this */
    if (*value != '\0')
    {
        if ((rc = prefix_set(gid, oid, value, ifname, addr)) != 0)
        {
            net_addr_del(gid, oid, ifname, addr);
            ERROR("prefix_set failure");
            return rc;
        }
    }
#endif

    return 0;
}
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

    sa_family_t     family;
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

    family = str_addr_family(addr);

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
    gen_ip_address  tgt_addr;
    te_errno        rc;
    sa_family_t     family = str_addr_family(addr);
    size_t          addrlen = (family == AF_INET) ?
                                  sizeof(struct in_addr) :
                                  sizeof(struct in6_addr);
    char           *cur;
    char           *next;

    if (CHECK_INTERFACE(ifname) != 0)
        return NULL;

    if (inet_pton(family, addr, &tgt_addr) <= 0)
    {
        ERROR("inet_pton() failed for address %s", addr);
        return NULL;
    }

    /* Get list of aliases/logical interfaces in 'buf' */
    if ((rc = aliases_list()) != 0)
        return NULL;

    for (cur = buf; strlen(cur) > 0; cur = next)
    {
        void   *tmp_addr;

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

        rc = ta_unix_conf_get_addr(cur, family, &tmp_addr);
        if (rc == 0 && memcmp(tmp_addr, &tgt_addr, addrlen) == 0)
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
 * @param oid           full object instance identifier (unused)
 * @param ifname        name of the interface (like "eth0")
 * @param addr          IPv4 address in dotted notation
 *
 * @return              Status code
 */
static te_errno
net_addr_del(unsigned int gid, const char *oid,
             const char *ifname, const char *addr)
{
    te_errno    rc;

    UNUSED(gid);
    UNUSED(oid);

    if ((rc = CHECK_INTERFACE(ifname)) != 0)
        return TE_RC(TE_TA_UNIX, rc);

#if defined(USE_NETLINK)
    return nl_ip_addr_modify(NET_ADDR_DELETE, ifname, addr, NULL, NULL);
#elif defined(USE_IOCTL)
    {
        sa_family_t  family = str_addr_family(addr);
        int          sock = (family == AF_INET6) ? cfg6_socket : cfg_socket;
        char        *name;

        if ((name = find_net_addr(ifname, addr)) == NULL)
        {
            ERROR("Address %s on interface %s not found", addr, ifname);
            return TE_RC(TE_TA_UNIX, TE_ENOENT);
        }

        memset(&req, 0, sizeof(req));
        strncpy(req.my_ifr_name, name, sizeof(req.my_ifr_name));

        if (strcmp(name, ifname) == 0)
        {
            /* It is a physical interface */
            /* Set unspecified address */
            SA(&req.my_ifr_addr)->sa_family = str_addr_family(addr);
            CFG_IOCTL(sock, MY_SIOCSIFADDR, &req);
        }
        else
        {
            /* It is a logical/alias interface */
            /* Push logical interface down */
            CFG_IOCTL(sock, MY_SIOCGIFFLAGS, &req);
            req.my_ifr_flags &= ~IFF_UP;
            CFG_IOCTL(sock, MY_SIOCSIFFLAGS, &req);
#if HAVE_STRUCT_LIFREQ
            /* On Solaris - remove logical interface directly */
            CFG_IOCTL(sock, SIOCLIFREMOVEIF, &req);
#else
            /* On Linux - nothing special to be done */
#endif
        }
        return 0;
    }
#else
#error Cannot delete network addresses from interfaces
#endif
}


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
    int               len = 0;
    te_errno          rc;
    agt_nlmsg_list    addr_list;
    agt_nlmsg_entry **first_inet6_addr;
    agt_nlmsg_entry  *a;
    sa_family_t       cur_family;
    char             *cur_ptr;
    int               ifindex;

    UNUSED(gid);
    UNUSED(oid);
    
    TAILQ_INIT(&addr_list);

    if ((rc = CHECK_INTERFACE(ifname)) != 0)
    {
        /* Alias does not exist from Configurator point of view */
        return TE_RC(TE_TA_UNIX, rc);
    }

    ifindex = if_nametoindex(ifname);
    if (ifindex <= 0)
    {
        ERROR("Device \"%s\" does not exist", ifname);
        return TE_RC(TE_TA_UNIX, TE_ENODEV);
    }

    if ((rc = ip_addr_get(AF_INET, &addr_list)) != 0)
    {
        ERROR("%s: ip_addr_get() for IPv4 failed", __FUNCTION__);
        return rc;
    }
    /* 
     * Keep in mind "next" field of the last entry 
     * in the list of IPv4 addresses.
     */
    first_inet6_addr = addr_list.tqh_last;

    if ((rc = ip_addr_get(AF_INET6, &addr_list)) != 0)
    {
        free_nlmsg_list(&addr_list);
        ERROR("%s: ip_addr_get() for IPv6 failed", __FUNCTION__);
        return rc;
    }

    if ((*list = strdup("")) == NULL)
    {
        free_nlmsg_list(&addr_list);
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }

    cur_family = AF_INET;
    cur_ptr = *list;
    TAILQ_FOREACH(a, &addr_list, links)
    {
        struct nlmsghdr  *hdr = a->hdr;
        struct ifaddrmsg *ifa = NLMSG_DATA(hdr);
        struct rtattr    *rta_tb[IFA_MAX + 1];

        /* IPv4 addresses are all printed, start printing IPv6 addresses */
        if (a == *first_inet6_addr)
            cur_family = AF_INET6;

        if (hdr->nlmsg_len < NLMSG_LENGTH(sizeof(ifa)))
        {
            ERROR("%s(): bad netlink message hdr length", __FUNCTION__);
            free(*list);
            free_nlmsg_list(&addr_list);
            return TE_RC(TE_TA_UNIX, TE_EINVAL);
        }

        if (ifa->ifa_index != ifindex)
            continue;

        /*
         * Sometimes netlink does not take into account family type
         * specified in request, so check it here explicitly.
         */
        if (ifa->ifa_family != cur_family)
            continue;

        memset(rta_tb, 0, sizeof(rta_tb));
        parse_rtattr(rta_tb, IFA_MAX, IFA_RTA(ifa),
                     hdr->nlmsg_len - NLMSG_LENGTH(sizeof(*ifa)));

        if (!rta_tb[IFA_LOCAL])
            rta_tb[IFA_LOCAL] = rta_tb[IFA_ADDRESS];
        if (!rta_tb[IFA_ADDRESS])
            rta_tb[IFA_ADDRESS] = rta_tb[IFA_LOCAL];

        assert(cur_ptr >= *list);
        assert(cur_ptr - (*list) <= len);
        /*
         * We need space at least for one IPv6 address and 
         * one space char. 
         */
        if ((*list + len - cur_ptr) <= (INET6_ADDRSTRLEN + 1))
        {
            char *tmp;
            uint32_t str_len = cur_ptr - *list;

            len += ADDR_LIST_BULK;
            if ((tmp = (char *)realloc(*list, len)) == NULL)
            {
                ERROR("%s(): realloc() failed", __FUNCTION__);
                free(*list);
                free_nlmsg_list(&addr_list);
                return TE_RC(TE_TA_UNIX, TE_ENOMEM);
            }
            *list = tmp;
            cur_ptr = *list + str_len;
        }

        if (inet_ntop(cur_family, RTA_DATA(rta_tb[IFA_LOCAL]),
                      cur_ptr, INET6_ADDRSTRLEN) == NULL)
        {
            ERROR("%s(): Cannot save network address", __FUNCTION__);
            free(*list);
            free_nlmsg_list(&addr_list);
            return TE_RC(TE_TA_UNIX, TE_EINVAL);
        }
        cur_ptr += strlen(cur_ptr);
        snprintf(cur_ptr, (*list + len - cur_ptr), " ");
        cur_ptr += strlen(cur_ptr);
    }

    free_nlmsg_list(&addr_list);
    return 0;
}

#elif USE_IOCTL

/** Opaque data for net_addr_list_ifreq_cb() */
struct net_addr_list_ifreq_cb_data {
    const char *ifname;     /**< Interface name */
    char       *buf;        /**< Buffer to print address to */
    size_t      buf_len;    /**< Size of the buffer */
    size_t      buf_off;    /**< Current offset in the buffer */
};

static te_errno
net_addr_list_ifreq_cb(struct my_ifreq *ifr, void *opaque)
{
    struct net_addr_list_ifreq_cb_data *data = opaque;

    size_t  str_addrlen;
    void   *net_addr;

    if (strcmp(ifr->my_ifr_name, data->ifname) != 0 &&
        !is_alias_of(ifr->my_ifr_name, data->ifname))
        return 0;

    if (SA(&ifr->my_ifr_addr)->sa_family == AF_INET)
    {
        str_addrlen = INET_ADDRSTRLEN;
        net_addr = &SIN(&ifr->my_ifr_addr)->sin_addr;
    }
    else if (SA(&ifr->my_ifr_addr)->sa_family == AF_INET6)
    {
        str_addrlen = INET6_ADDRSTRLEN;
        net_addr = &SIN6(&ifr->my_ifr_addr)->sin6_addr;
    }
    else
    {
        return 0;
    }

    while (data->buf_len - data->buf_off <= str_addrlen + 1)
    {
        data->buf_len += ADDR_LIST_BULK;
        data->buf = realloc(data->buf, data->buf_len);
        if (data->buf == NULL)
        {
            ERROR("realloc() failed");
            return TE_RC(TE_TA_UNIX, TE_ENOMEM);
        }
    }

    if (inet_ntop(SA(&ifr->my_ifr_addr)->sa_family, net_addr,
                  data->buf + data->buf_off, str_addrlen) == NULL)
    {
        ERROR("Failed to convert address to string");
        return TE_RC(TE_TA_UNIX, TE_EFAULT);
    }
    data->buf_off += strlen(data->buf + data->buf_off);
    data->buf[data->buf_off++] = ' ';
    data->buf[data->buf_off] = '\0';
    assert(data->buf_off < data->buf_len);

    return 0;
}

static te_errno
net_addr_list(unsigned int gid, const char *oid, char **list,
              const char *ifname)
{
    struct net_addr_list_ifreq_cb_data  cb_data;

    struct my_ifreq    *req;
    void               *ifconf_buf = NULL;
    size_t              ifconf_len;
    te_errno            rc;

    UNUSED(gid);
    UNUSED(oid);

    if ((rc = CHECK_INTERFACE(ifname)) != 0)
    {
        return TE_RC(TE_TA_UNIX, rc);
    }

    rc = get_ifconf_to_buf(&ifconf_buf, (void **)&req, &ifconf_len);
    if (rc != 0)
    {
        free(ifconf_buf);
        return rc;
    }

    cb_data.ifname = ifname;
    cb_data.buf_len = ADDR_LIST_BULK;
    cb_data.buf = malloc(cb_data.buf_len);
    if (cb_data.buf == NULL)
    {
        free(ifconf_buf);
        ERROR("calloc() failed");
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }
    cb_data.buf[0] = '\0';
    cb_data.buf_off = 0;

    rc = ifconf_foreach_ifreq(req, ifconf_len, net_addr_list_ifreq_cb,
                              &cb_data);
    if (rc != 0)
    {
        free(cb_data.buf);
    }
    else
    {
        *list = cb_data.buf;
    }

    free(ifconf_buf);

    return rc;
}
#endif

#ifdef USE_IOCTL
te_errno
ta_unix_conf_netaddr2ifname(const struct sockaddr *addr, char *ifname)
{
    size_t           addrlen = te_netaddr_get_size(addr->sa_family);
    void            *netaddr = te_sockaddr_get_netaddr(addr);
    te_errno         rc;
    void            *ifconf_buf = NULL;
    size_t           ifconf_len;
    struct my_ifreq *first_req;
    struct my_ifreq *p;

    rc = get_ifconf_to_buf(&ifconf_buf, (void **)&first_req, &ifconf_len);
    if (rc != 0)
    {
        free(ifconf_buf);
        return rc;
    }

    VERB("%s(): SEARCH %s", __FUNCTION__, te_sockaddr2str(addr));
    rc = TE_RC(TE_TA_UNIX, TE_ESRCH);
    for (p = first_req; *(p->my_ifr_name) != '\0'; p++)
    {
        VERB("%s(): CHECK name=%s addr=%s", __FUNCTION__, p->my_ifr_name,
             te_sockaddr2str(CONST_SA(&p->my_ifr_addr)));
        if (addr->sa_family == CONST_SA(&p->my_ifr_addr)->sa_family &&
            memcmp(netaddr,
                   te_sockaddr_get_netaddr(CONST_SA(&p->my_ifr_addr)),
                   addrlen) == 0)
        {
            strncpy(ifname, p->my_ifr_name, IF_NAMESIZE);
            rc = 0;
            break;
        }
    }
    free(ifconf_buf);
    return rc;
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
    strncpy(req.my_ifr_name, ifname, sizeof(req.my_ifr_name));
    if (strchr(addr, ':') == NULL)
    {
        SIN(&req.my_ifr_addr)->sin_family = AF_INET;
        if (inet_pton(AF_INET, addr, &SIN(&req.my_ifr_addr)->sin_addr) <= 0)
        {
            ERROR("inet_pton(AF_INET) failed for '%s'", addr);
            return TE_RC(TE_TA_UNIX, TE_EFMT);
        }
        CFG_IOCTL(cfg_socket, MY_SIOCGIFNETMASK, &req);
        MASK2PREFIX(ntohl(SIN(&req.my_ifr_addr)->sin_addr.s_addr), prefix);
    }
    else
    {
#ifdef SIOCGLIFSUBNET
        SIN6(&req.my_ifr_addr)->sin6_family = AF_INET6;
        if (inet_pton(AF_INET6, addr,
                      &SIN6(&req.my_ifr_addr)->sin6_addr) <= 0)
        {
            ERROR("inet_pton(AF_INET6) failed for '%s'", addr);
            return TE_RC(TE_TA_UNIX, TE_EFMT);
        }
        CFG_IOCTL(cfg6_socket, SIOCGLIFSUBNET, &req);
        prefix = req.lifr_addrlen;
#elif defined(SIOCGLIFADDR)
        struct if_laddrreq lreq;

        memset(&lreq, 0, sizeof(lreq));
        strncpy(lreq.iflr_name, ifname, sizeof(lreq.iflr_name));
        lreq.addr.ss_family = AF_INET6;
        lreq.addr.ss_len = 0;
        if (inet_pton(AF_INET6, addr, &SIN6(&lreq.addr)->sin6_addr) <= 0)
        {
            ERROR("inet_pton(AF_INET6) failed for '%s'", addr);
            return TE_RC(TE_TA_UNIX, TE_EFMT);
        }
        CFG_IOCTL(cfg6_socket, SIOCGLIFADDR, &lreq);
        prefix = lreq.prefixlen;
#else
        ERROR("Unable to get IPv6 address prefix");
        return TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
    }
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
    te_errno        rc;
    unsigned int    prefix;

    UNUSED(gid);
    UNUSED(oid);

    rc = prefix_check(value, str_addr_family(addr), &prefix);
    if (rc != 0)
        return rc;

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
    gen_ip_address  bcast;
    sa_family_t     family = str_addr_family(addr);

    UNUSED(gid);
    UNUSED(oid);

    if (family == AF_INET6)
    {
        /* No broadcast addresses in IPv6 */
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }
    assert(family == AF_INET);

    memset(&bcast, 0, sizeof(bcast));

#if defined(USE_NETLINK)
    if (nl_find_net_addr(addr, ifname, NULL, NULL, &bcast) == NULL)
    {
        ERROR("Address '%s' on interface '%s' to get broadcast address "
              "not found", addr, ifname);
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }
#elif defined(USE_IOCTL)
    strncpy(req.my_ifr_name, ifname, sizeof(req.my_ifr_name));
    if (inet_pton(AF_INET, addr, &SIN(&req.my_ifr_addr)->sin_addr) <= 0)
    {
        ERROR("inet_pton(AF_INET) failed for '%s'", addr);
        return TE_RC(TE_TA_UNIX, TE_EFMT);
    }
    if (ioctl(cfg_socket, MY_SIOCGIFBRDADDR, &req) < 0)
    {
        te_errno rc = TE_OS_RC(TE_TA_UNIX, errno);

        /* 
         * Solaris2 (SunOS 5.11) returns EADDRNOTAVAIL on request for
         * broadcast address on loopback.
         * FreeBSD6 returns EINVAL on request for broadcast address
         * on loopback.
         */
        if (INTERFACE_IS_LOOPBACK(ifname))
            return TE_RC(TE_TA_UNIX, TE_ENOENT);

        ERROR("ioctl(SIOCGIFBRDADDR) failed for if=%s addr=%s: %r",
              ifname, addr, rc);
        return rc;
    }
    else
    {
        bcast.ip4_addr.s_addr = SIN(&req.my_ifr_addr)->sin_addr.s_addr;
    }
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
    gen_ip_address  bcast;
    sa_family_t     family = str_addr_family(addr);

    UNUSED(gid);
    UNUSED(oid);

    if (family != AF_INET)
    {
        ERROR("Broadcast address can be set for IPv4 only");
        return TE_RC(TE_TA_UNIX, TE_ENOSYS);
    }

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

        strcpy(req.my_ifr_name, name);
        SA(&req.my_ifr_addr)->sa_family = AF_INET;
        SIN(&(req.my_ifr_addr))->sin_addr = bcast.ip4_addr;
        CFG_IOCTL(cfg_socket, MY_SIOCSIFBRDADDR, &req);
        return 0;
    }
#else
#error Way to work with network addresses is not defined.
#endif
}


#if defined(USE_NETLINK) || defined(HAVE_SYS_DLPI_H)
/*
 * Next functions are pulled out from iproute internals
 * to be accessible here and renamed.
 */
static const char *
link_addr_n2a(unsigned char *addr, int alen,
              char *buf, int blen)
{
    int i;
    int l;

    l = 0;
    for (i = 0; i < alen; i++)
    {
        if (i==0)
        {
            snprintf(buf + l, blen, "%02x", addr[i]);
            blen -= 2;
            l += 2;
        }
        else
        {
            snprintf(buf+l, blen, ":%02x", addr[i]);
            blen -= 3;
            l += 3;
        }
    }
    return buf;
}
#endif

#if defined(SIOCSIFHWADDR) || defined(SIOCSIFHWBROADCAST) || \
    defined(HAVE_SYS_DLPI_H)
static int
link_addr_a2n(uint8_t *lladdr, int len, const char *str)
{
    const char *arg = str;
    int         i;

    for (i = 0; i < len; i++)
    {
        unsigned int  temp;
        char         *cp = strchr(arg, ':');

        if (cp != NULL)
            *cp = '\0';

        if (sscanf(arg, "%x", &temp) != 1)
        {
            ERROR("%s: \"%s\" is invalid lladdr",
                  __FUNCTION__, arg);
            if (cp != NULL)
                *cp = ':';
            return -1;
        }

        if (cp != NULL)
            *cp = ':';

        if (temp > 255)
        {
            ERROR("%s:\"%s\" is invalid lladdr",
                  __FUNCTION__, arg);
            return -1;
        }

        lladdr[i] = (uint8_t)temp;

        if (cp == NULL)
            break;

        arg = ++cp;
    }
    return i + 1;
}
#endif

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
    te_errno        rc;
    const uint8_t  *ptr = NULL;

    UNUSED(gid);
    UNUSED(oid);

    if ((rc = CHECK_INTERFACE(ifname)) != 0)
        return TE_RC(TE_TA_UNIX, rc);

#ifdef SIOCGIFHWADDR
    memset(&req, 0, sizeof(req));
    strcpy(req.my_ifr_name, ifname);
    CFG_IOCTL(cfg_socket, SIOCGIFHWADDR, &req);

    ptr = (const uint8_t *)req.my_ifr_hwaddr.sa_data;

#elif HAVE_SYS_DLPI_H
    do {
        size_t  len = sizeof(buf);

        rc = ta_unix_conf_dlpi_phys_addr_get(ifname, buf, &len);
        if (TE_RC_GET_ERROR(rc) == TE_ENOENT)
        {
            /* No link-layer or address */
            break;
        }
        else if (rc != 0)
        {
            ERROR("Failed to get interface link-layer address using "
                  "DLPI: %r", rc);
            return TE_RC(TE_TA_UNIX, rc);
        }
        if (len != ETHER_ADDR_LEN)
        {
            ERROR("%s(): Unsupported link-layer address length %u",
                  __FUNCTION__, (unsigned)len);
            return TE_RC(TE_TA_UNIX, TE_ENOSYS);
        }
        ptr = (const uint8_t *)buf;
    } while (0);
#elif defined(__FreeBSD__)

    void           *ifconf_buf = NULL;
    size_t          ifconf_len;
    struct ifreq   *p;

    rc = get_ifconf_to_buf(&ifconf_buf, (void **)&p, &ifconf_len);
    if (rc != 0)
    {
        free(ifconf_buf);
        return rc;
    }

    for (; *(p->ifr_name) != '\0';
         p = (struct ifreq *)((caddr_t)p + _SIZEOF_ADDR_IFREQ(*p)))
    {
        if ((strcmp(p->ifr_name, ifname) == 0) &&
            (p->ifr_addr.sa_family == AF_LINK))
        {
            struct sockaddr_dl *sdl =
                (struct sockaddr_dl *)&(p->ifr_addr);

            if (sdl->sdl_alen == ETHER_ADDR_LEN)
            {
                ptr = (const uint8_t *)sdl->sdl_data + sdl->sdl_nlen;
            }
            else
            {
                /* ptr is NULL - no link-layer address */
            }
            break;
        }
    }
    free(ifconf_buf);
#endif

    if (ptr == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    snprintf(value, RCF_MAX_VAL, "%02x:%02x:%02x:%02x:%02x:%02x",
             ptr[0], ptr[1], ptr[2], ptr[3], ptr[4], ptr[5]);
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
    te_errno rc = 0;
    char     aux[65];

    UNUSED(gid);
    UNUSED(oid);

    memset(aux, 0, 65);

    if ((rc = CHECK_INTERFACE(ifname)) != 0)
        return TE_RC(TE_TA_UNIX, rc);

    if (value == NULL)
    {
       ERROR("A link layer address to set is not provided");
       return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    strncpy(aux, value, 64);

#ifdef SIOCSIFHWADDR
    strcpy(req.my_ifr_name, ifname);
    req.my_ifr_hwaddr.sa_family = AF_LOCAL;

    if ((rc = link_addr_a2n((uint8_t *)req.my_ifr_hwaddr.sa_data,
                            ETHER_ADDR_LEN, aux)) == -1)
    {
        ERROR("%s: Link layer address conversation issue", __FUNCTION__);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }
    rc = 0;

    CFG_IOCTL(cfg_socket, SIOCSIFHWADDR, &req);
#elif HAVE_SYS_DLPI_H
    if ((rc = link_addr_a2n((uint8_t *)buf, ETHER_ADDR_LEN, aux)) == -1)
    {
        ERROR("%s: Link layer address conversation issue", __FUNCTION__);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    rc = ta_unix_conf_dlpi_phys_addr_set(ifname, buf, ETHER_ADDR_LEN);
    if (rc != 0)
    {
        ERROR("Failed to set interface link-layer address using "
              "DLPI: %r", rc);
    }
#else
    ERROR("Set of link-layer address is not supported");
    rc = TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP);
#endif
    return rc;
}

/**
 * Set broadcast hardware address of the interface.
 * Only MAC addresses are supported now.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         broadcast hardware address (it should be
 *                      provided as XX:XX:XX:XX:XX:XX string)
 * @param ifname        name of the interface (like "eth0")
 *
 * @return              Status code
 */
static te_errno
bcast_link_addr_set(unsigned int gid, const char *oid,
                    const char *value, const char *ifname)
{
    te_errno rc = 0;

    UNUSED(gid);
    UNUSED(oid);

    if ((rc = CHECK_INTERFACE(ifname)) != 0)
        return TE_RC(TE_TA_UNIX, rc);

    if (value == NULL)
    {
       ERROR("A broadcast link layer address to set is not provided");
       return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

#ifdef SIOCSIFHWBROADCAST
    strcpy(req.my_ifr_name, ifname);
    req.my_ifr_hwaddr.sa_family = AF_LOCAL;

    if ((rc = link_addr_a2n((uint8_t *)req.my_ifr_hwaddr.sa_data, 6,
                            value)) == -1)
    {
        ERROR("%s: Link layer address conversation issue", __FUNCTION__);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }
    rc = 0;

    CFG_IOCTL(cfg_socket, SIOCSIFHWBROADCAST, &req);
#else
    ERROR("Set of broadcast link-layer address is not supported");
    rc = TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP);
#endif

    return rc;
}


/**
 * Set broadcast hardware address of the interface.
 * Only MAC addresses are supported now.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         broadcast hardware address (it should be
 *                      provided as XX:XX:XX:XX:XX:XX string)
 * @param ifname        name of the interface (like "eth0")
 *
 * @return              Status code
 */
static te_errno
bcast_link_addr_get(unsigned int gid, const char *oid,
                    char *value, const char *ifname)
{
    te_errno rc = 0;

    UNUSED(gid);
    UNUSED(oid);

    if ((rc = CHECK_INTERFACE(ifname)) != 0)
        return TE_RC(TE_TA_UNIX, rc);

    if (value == NULL)
    {
       ERROR("A buffer for broadcast link layer address is not provided");
       return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

#ifdef USE_NETLINK
{
    struct rtnl_handle  rth;
    int                 ifindex;
    agt_nlmsg_list      info_list;
    agt_nlmsg_entry    *a_aux;
    struct nlmsghdr    *n_aux = NULL;
    struct ifinfomsg   *ifi = NULL;
    struct rtattr      *tb[IFLA_MAX+1];
    int                 len;


    TAILQ_INIT(&info_list);

    memset(&rth, 0, sizeof(rth));
    if (rtnl_open(&rth, 0) < 0)
    {
        ERROR("%s: rtnl_open() failed", __FUNCTION__);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    ll_init_map(&rth);

    ifindex = if_nametoindex(ifname);
    if (ifindex <= 0)
    {
        ERROR("%s: Device \"%s\" does not exist.\n",
              __FUNCTION__, ifname);
        rc = TE_RC(TE_TA_UNIX, TE_ENODEV);
        goto exit;
    }

    if (rtnl_wilddump_request(&rth, AF_PACKET, RTM_GETLINK) < 0)
    {
        ERROR("%s: Cannot send dump request", __FUNCTION__);
        rc = TE_RC(TE_TA_UNIX, TE_EINVAL);
        goto exit;
    }

    if (rtnl_dump_filter(&rth, store_nlmsg, &info_list, NULL, NULL) < 0)
    {
        ERROR("%s: Dump terminated ", __FUNCTION__);
        rc = TE_RC(TE_TA_UNIX, TE_EINVAL);
        goto exit;
    }

    TAILQ_FOREACH(a_aux, &info_list, links)
    {
        n_aux = a_aux->hdr;
        len = n_aux->nlmsg_len;
        ifi = NLMSG_DATA(n_aux);

        if (n_aux->nlmsg_type != RTM_NEWLINK &&
            n_aux->nlmsg_type != RTM_DELLINK)
            continue;

        if (ifi->ifi_index != ifindex)
            continue;

        len -= NLMSG_LENGTH(sizeof(*ifi));
        if (len < 0)
            continue;

        if (ifi->ifi_index != ifindex)
            continue;

        memset(tb, 0, sizeof(tb));
        parse_rtattr(tb, IFLA_MAX, IFLA_RTA(ifi), len);
        if (tb[IFLA_IFNAME] == NULL)
        {
            ERROR("%s: BUG! For ifindex %d ifname is not "
                  "set into returned info", __FUNCTION__, ifindex);
            rc = TE_RC(TE_TA_UNIX, TE_EINVAL);
            goto exit;
        }

        if (tb[IFLA_BROADCAST])
        {
            link_addr_n2a(RTA_DATA(tb[IFLA_BROADCAST]),
                          RTA_PAYLOAD(tb[IFLA_BROADCAST]),
                          value, RCF_MAX_VAL);
#ifdef LOCAL_FUNC_DEBUGGING
            ERROR("IGORV: %s: tb[IFLA_BROADCAST] is TRUE - "
                  "%s: %s", __FUNCTION__, ifname, buf);
#endif
            /* Success */
            goto exit;
        }
        break;
    }
    rc = TE_RC(TE_TA_UNIX, TE_ENOENT);

exit:
    rtnl_close(&rth);
    free_nlmsg_list(&info_list);

    return rc;
}
#elif HAVE_SYS_DLPI_H
    do {
        size_t  len = sizeof(buf);

        rc = ta_unix_conf_dlpi_phys_bcast_addr_get(ifname, buf, &len);
        if (TE_RC_GET_ERROR(rc) == TE_ENOENT)
        {
            /* No link-layer or broadcast address */
            break;
        }
        else if (rc != 0)
        {
            ERROR("Failed to get interface link-layer broadcast address "
                  "using DLPI: %r", rc);
            break;
        }
        link_addr_n2a((unsigned char *)buf, len, value, RCF_MAX_VAL);
    } while (0);

    return rc;
#else
    return TE_RC(TE_TA_UNIX, TE_ENOENT);
#endif
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

#if defined(SIOCGIFMTU)  && defined(HAVE_STRUCT_IFREQ_IFR_MTU)   || \
    defined(SIOCGLIFMTU) && defined(HAVE_STRUCT_LIFREQ_LIFR_MTU)
    {
        struct my_ifreq req;

        strncpy(req.my_ifr_name, ifname, sizeof(req.my_ifr_name));
        CFG_IOCTL(cfg_socket, MY_SIOCGIFMTU, &req);
        sprintf(value, "%d", req.my_ifr_mtu);
    }
#endif
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
    long      mtu;

    UNUSED(gid);
    UNUSED(oid);

    if ((rc = CHECK_INTERFACE(ifname)) != 0)
        return TE_RC(TE_TA_UNIX, rc);

    mtu = strtol(value, &tmp, 10);
    if (tmp == value || *tmp != 0)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

#if (defined(SIOCGIFMTU)  && defined(HAVE_STRUCT_IFREQ_IFR_MTU))   || \
    (defined(SIOCGLIFMTU) && defined(HAVE_STRUCT_LIFREQ_LIFR_MTU))
    req.my_ifr_mtu = mtu;
    strcpy(req.my_ifr_name, ifname);
    if (ioctl(cfg_socket, MY_SIOCSIFMTU, (int)&req) != 0)
    {
        rc = TE_OS_RC(TE_TA_UNIX, errno);
        
        if (errno == EBUSY)
        {
            te_bool status;
            
            /* Try to down interface */
            if (ta_interface_status_get(ifname, &status) == 0 &&
                status && ta_interface_status_set(ifname, FALSE) == 0)
            {
                int rc1;

                WARN("Interface '%s' is pushed down/up to set a new MTU",
                     ifname);

                if (ioctl(cfg_socket, MY_SIOCSIFMTU, (int)&req) == 0)
                {
                    rc = 0;
                }
                
                if ((rc1 = ta_interface_status_set(ifname, TRUE)) != 0)
                {
                    ERROR("Failed to up interface after changing of mtu "
                          "error %r", rc1);
                    return rc1;
                }
            }
        }
    }
#else
    rc = TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif

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

    strcpy(req.my_ifr_name, ifname);
    CFG_IOCTL(cfg_socket, MY_SIOCGIFFLAGS, &req);

    sprintf(value, "%d", (req.my_ifr_flags & IFF_NOARP) != IFF_NOARP);

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

    strncpy(req.my_ifr_name, ifname, IFNAMSIZ);
    CFG_IOCTL(cfg_socket, MY_SIOCGIFFLAGS, &req);

    if (strcmp(value, "1") == 0)
        req.my_ifr_flags &= (~IFF_NOARP);
    else if (strcmp(value, "0") == 0)
        req.my_ifr_flags |= (IFF_NOARP);
    else
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    strncpy(req.my_ifr_name, ifname, IFNAMSIZ);
    CFG_IOCTL(cfg_socket, MY_SIOCSIFFLAGS, &req);

    return 0;
}

/**
 * Get status of the interface (FALSE - down or TRUE - up).
 *
 * @param ifname        name of the interface (like "eth0")
 * @param status        location to put status of the interface
 *
 * @return              Status code
 */
te_errno
ta_interface_status_get(const char *ifname, te_bool *status)
{
    te_errno rc;
    
    assert(status != NULL);

    if ((rc = CHECK_INTERFACE(ifname)) != 0)
        return TE_RC(TE_TA_UNIX, rc);

    strcpy(req.my_ifr_name, ifname);
    CFG_IOCTL(cfg_socket, MY_SIOCGIFFLAGS, &req);
    *status = !!(req.my_ifr_flags & IFF_UP);

#if defined(__sun__)
    rc = ioctl(cfg6_socket, MY_SIOCGIFFLAGS, &req);
    if (rc < 0)
        WARN("Failed to get staust of %s IPv6 interface", ifname);
    else if (*status != !!(req.my_ifr_flags & IFF_UP))
        WARN("Different statuses for %s IPv4 and IPv6 interfaces", ifname);
#endif

    return 0;
}

/**
 * Change status of the interface. If virtual interface is put to down
 * state,it is de-installed and information about it is stored in the list
 * of down interfaces.
 *
 * @param ifname        name of the interface (like "eth0")
 * @param status        TRUE to get interface up and FALSE to down
 *
 * @return              Status code
 */
te_errno
ta_interface_status_set(const char *ifname, te_bool status)
{
    te_errno rc;
    
    if ((rc = CHECK_INTERFACE(ifname)) != 0)
        return TE_RC(TE_TA_UNIX, rc);

    strncpy(req.my_ifr_name, ifname, IFNAMSIZ);
    CFG_IOCTL(cfg_socket, MY_SIOCGIFFLAGS, &req);

    if (status)
        req.my_ifr_flags |= (IFF_UP | IFF_RUNNING);
    else
        req.my_ifr_flags &= ~(IFF_UP | IFF_RUNNING);

    CFG_IOCTL(cfg_socket, MY_SIOCSIFFLAGS, &req);
#if defined(__sun__)
    rc = ioctl(cfg6_socket, MY_SIOCSIFFLAGS, &req);
    if (rc < 0)
        WARN("Failed to bring up %s IPv6 interface", ifname);
#endif
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
    te_bool  status;
    
    UNUSED(gid);
    UNUSED(oid);

    rc = ta_interface_status_get(ifname, &status);
    if (rc != 0)
        return rc;
    sprintf(value, "%d", status);

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
    te_bool  status;
    
    UNUSED(gid);
    UNUSED(oid);

    if (strcmp(value, "0") == 0)
        status = FALSE;
    else if (strcmp(value, "1") == 0)
        status = TRUE;
    else
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    return ta_interface_status_set(ifname, status);
}


/**
 * Get promiscuous mode of the interface ("0" - normal or "1" - promiscuous)
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         value location
 * @param ifname        name of the interface (like "eth0")
 *
 * @return              Status code
 */
static te_errno
promisc_get(unsigned int gid, const char *oid, char *value,
            const char *ifname)
{
    te_errno rc;
    
    UNUSED(gid);
    UNUSED(oid);

    if ((rc = CHECK_INTERFACE(ifname)) != 0)
        return TE_RC(TE_TA_UNIX, rc);

    strcpy(req.my_ifr_name, ifname);
    CFG_IOCTL(cfg_socket, MY_SIOCGIFFLAGS, &req);

    sprintf(value, "%d", (req.my_ifr_flags & IFF_PROMISC) != 0);

    return 0;
}

/**
 * Change the promiscuous mode of the interface 
 * ("1" - enable, "0" - disable)
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         new value pointer
 * @param ifname        name of the interface (like "eth0")
 *
 * @return              Status code
 */
static te_errno
promisc_set(unsigned int gid, const char *oid, const char *value,
           const char *ifname)
{
    te_errno rc;
    
    UNUSED(gid);
    UNUSED(oid);

    if ((rc = CHECK_INTERFACE(ifname)) != 0)
        return TE_RC(TE_TA_UNIX, rc);

    strncpy(req.my_ifr_name, ifname, IFNAMSIZ);
    CFG_IOCTL(cfg_socket, MY_SIOCGIFFLAGS, &req);

    if (strcmp(value, "0") == 0)
        req.my_ifr_flags &= ~IFF_PROMISC;
    else if (strcmp(value, "1") == 0)
        req.my_ifr_flags |= IFF_PROMISC;
    else
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    CFG_IOCTL(cfg_socket, MY_SIOCSIFFLAGS, &req);

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
neigh_find_cb(const struct sockaddr_nl *who, struct nlmsghdr *n,
              void *arg)
{    
    neigh_find_cb_param *p = (neigh_find_cb_param *)arg;
    sa_family_t          af = str_addr_family(p->addr);
    struct ndmsg        *r = NLMSG_DATA(n);
    struct rtattr       *tb[NDA_MAX+1] = {NULL,};
    uint8_t              addr_buf[sizeof(struct in6_addr)];    
    
    /* One item was already found, skip others */
    if (p->found)
    {
        return 0;
    }

    /* Neighbour from alien interface */
    if ((int)if_nametoindex(p->ifname) != r->ndm_ifindex)
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

    if (tb[NDA_LLADDR] == NULL)
        return 0;

    /* Save MAC address */
    if (p->mac_addr != NULL)
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

    rtnl_wilddump_request(&rth, str_addr_family(addr), RTM_GETNEIGH);
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
    {
        struct arpreq arp_req;
        sa_family_t   family;
        
        memset(&arp_req, 0, sizeof(arp_req));
        family = str_addr_family(addr);
        arp_req.arp_pa.sa_family = family;
        if (inet_pton(family, addr, &SIN(&(arp_req.arp_pa))->sin_addr) <= 0)
            return TE_RC(TE_TA_UNIX, TE_EINVAL);
#if HAVE_STRUCT_ARPREQ_ARP_DEV
        strncpy(arp_req.arp_dev, ifname, sizeof(arp_req.arp_dev));
#endif

#ifdef SIOCGARP
        if (ioctl(cfg_socket, SIOCGARP, (caddr_t)&arp_req) != 0)
        {
            te_errno rc = TE_OS_RC(TE_TA_UNIX, errno);
        
            if (TE_RC_GET_ERROR(rc) != TE_ENXIO)
            {
                /* Temporary hack to avoid failures */
                WARN("line %u: ioctl(SIOCGARP) failed: %r", __LINE__, rc);
            }
            return TE_RC(TE_TA_UNIX, TE_ENOENT);
        }
        if (mac_p != NULL)
        {
            int      i;
            char    *s = mac_p;

            for (i = 0; i < ETHER_ADDR_LEN; i++)
            {
                sprintf(s, "%02x:",
                        ((const uint8_t *)arp_req.arp_ha.sa_data)[i]);
                s += strlen(s);
            }
            *(s - 1) = '\0';
        }
        if (state_p != NULL)
        {
            if (arp_req.arp_flags & ATF_COM)
                *state_p = CS_NEIGH_REACHABLE;
            else
                *state_p = CS_NEIGH_INCOMPLETE;
        }

        return 0;
#else
        return TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP);
#endif
    }
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
    
    UNUSED(gid);
    UNUSED(oid);
    
    if ((rc = neigh_find("dynamic", ifname, addr, NULL, &state)) != 0)
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

    dst.family = str_addr_family(addr);
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

    if ((req.ndm.ndm_ifindex = if_nametoindex(ifname)) == 0)
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

    arp_req.arp_ha.sa_family = AF_UNIX; /* AF_LOCAL */
    for (i = 0; i < 6; i++)
        (arp_req.arp_ha.sa_data)[i] = (unsigned char)(int_addr[i]);

    arp_req.arp_flags = ATF_COM;
    if (strstr(oid, "dynamic") == NULL)
    {
        VERB("%s(): Add permanent ARP entry", __FUNCTION__);
        arp_req.arp_flags |= ATF_PERM;
    }
#if HAVE_STRUCT_ARPREQ_ARP_DEV
    strncpy(arp_req.arp_dev, ifname, sizeof(arp_req.arp_dev));
#endif

#ifdef SIOCSARP
    CFG_IOCTL(cfg_socket, SIOCSARP, &arp_req);

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
        sa_family_t   family;
        
        memset(&arp_req, 0, sizeof(arp_req));
        family = str_addr_family(addr);
        arp_req.arp_pa.sa_family = family;
        if (inet_pton(family, addr, &SIN(&(arp_req.arp_pa))->sin_addr) <= 0)
            return TE_RC(TE_TA_UNIX, TE_EINVAL);
#if HAVE_STRUCT_ARPREQ_ARP_DEV
        strncpy(arp_req.arp_dev, ifname, sizeof(arp_req.arp_dev));
#endif

#ifdef SIOCDARP

        if (ioctl(cfg_socket, SIOCDARP, (caddr_t)&arp_req) != 0)
        {
            te_errno rc = te_rc_os2te(errno);

            if ((rc != TE_ENXIO) || (strstr(oid, "dynamic") == NULL))
            {
                ERROR("line %u: ioctl(SIOCDARP) failed: %r", __LINE__, rc);
            }
            else
            {
                rc = TE_ENOENT;
            }
            return TE_RC(TE_TA_UNIX, rc);
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
neigh_print_cb(const struct sockaddr_nl *who, struct nlmsghdr *n,
               void *arg)
{
    neigh_print_cb_param *p = (neigh_print_cb_param *)arg;

    struct ndmsg   *r = NLMSG_DATA(n);
    struct rtattr  *tb[NDA_MAX+1] = {NULL,};
    char           *s = p->list + strlen(p->list);

    UNUSED(who);
 
    if ((int)if_nametoindex(p->ifname) != r->ndm_ifindex)
        return 0;

    if (r->ndm_state == NUD_NONE || (r->ndm_state & NUD_INCOMPLETE) != 0)
        return 0;
    
    if (!!(r->ndm_state & NUD_PERMANENT) == p->dynamic)
        return 0;

    parse_rtattr(tb, NDA_MAX, NDA_RTA(r), n->nlmsg_len -
                 NLMSG_LENGTH(sizeof(*r)));

    if (tb[NDA_LLADDR] == NULL)
        return 0;

    if (tb[NDA_DST] == NULL ||
        inet_ntop(r->ndm_family, RTA_DATA(tb[NDA_DST]), s,
                  INET6_ADDRSTRLEN) == NULL)
        return 0;
        
    s += strlen(s);
    sprintf(s, " ");
    
    return 0;
}

static te_errno
ta_unix_conf_neigh_list(const char *ifname, te_bool is_static,
                        char **list)
{
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
    user_data.dynamic = !is_static;
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

    if ((*list = strdup(buf)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    return 0;
}
#elif !HAVE_INET_MIB2_H
static te_errno
ta_unix_conf_neigh_list(const char *ifname, te_bool is_static,
                        char **list)
{
    UNUSED(ifname);
    UNUSED(is_static);
    *list = NULL;
    return 0;
}
#else /* defined as external in util/conf_getmsg.c */
te_errno
ta_unix_conf_neigh_list(const char *iface, te_bool is_static, char **list);
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
    UNUSED(gid);

    return ta_unix_conf_neigh_list(ifname,
                                   strstr(oid, "dynamic") == NULL,
                                   list);
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
 * Get agent uname value.
 *
 * @param gid       Request's group identifier (unused)
 * @param oid       Full object instance identifier (unused)
 * @param value     Location for the value (OUT)
 * @param name      Variable name
 *
 * @return Status code
 */
static te_errno
uname_get(unsigned int gid, const char *oid, char *value)
{
#ifdef HAVE_SYS_UTSNAME_H
    struct utsname val;

    UNUSED(gid);
    UNUSED(oid);

    if (uname(&val) >= 0)
    {
        if (strlen(val.sysname) >= RCF_MAX_VAL)
            ERROR("System uname '%s' truncated", val.sysname);
        snprintf(value, RCF_MAX_VAL, "%s", val.sysname);
        return 0;
    }
    else
    {
        ERROR("Failed to call uname()");
        return TE_OS_RC(TE_TA_UNIX, errno);
    }
#else
#warning "uname access method is not implemented"
    return TE_OS_RC(TE_TA_UNIX, TE_EINVAL);
#endif
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

/**
 * Check, if user with the specified name exists.
 *
 * @param user          user name
 *
 * @return              TRUE if user exists, FALSE if does not
 */
static te_bool
user_exists(const char *user)
{
    return getpwnam(user) != NULL ? TRUE : FALSE;
}

#if TA_USE_PAM
/**
 * Callback function provided by user and called from within PAM library.
 *
 * @param num_msg       number of messages
 * @param msg           array of 'num_msg' pointers to messages
 * @param resp          address of pointer to returned array of responses
 * @param data          pointer passed to PAM library pam_start function
 *
 * @return              Return code (PAM_SUCCESS on success,
 *                      PAM_BUF_ERR when it is insufficient memory)
 *
 * @sa                  PAM library expects that response array
 *                      itself and each its .resp member are allocated
 *                      by malloc (calloc, realloc).
 *                      PAM library is responsible for freeing them.
 */
static int
conv_fun(int num_msg, pam_message_t **msg, pam_response_t **resp,
         void *data)
{
    /* Try to allocate responses array to be returned */
    struct pam_response *resp_array = calloc(num_msg, sizeof(*resp));
    appdata_t           *appdata    = data;

    int      i;
    unsigned full_len = strlen(appdata->passwd) + 1; /**< Password
                                                       *  length + 1
                                                       */

    /** If responses array is allocated successfully */
    if (resp_array != NULL)
    {
        for (i = 0; i < num_msg; i++) /* Process each message */
        {
            /** PAM prompts for password */
            if (msg[i]->msg_style == PAM_PROMPT_ECHO_ON ||
                msg[i]->msg_style == PAM_PROMPT_ECHO_OFF)
            {
                /** Allocate memory for password and supply it to PAM */
                if ((resp_array[i].resp = malloc(full_len)) != NULL)
                    memcpy(resp_array[i].resp, appdata->passwd, full_len);
                else
                {
                   /* Rollback allocation already
                    * been done at the moment
                    */
                    while (i-- > 0)
                        free(resp_array[i].resp);

                    free(resp_array);
                    return PAM_BUF_ERR;
                }
            }
            else
                /** PAM assumes user should read this error message */
                if (msg[i]->msg_style == PAM_ERROR_MSG)
                {
                    WARN("%s", msg[i]->msg);

                   /* Save message in order to have opportunity
                    * to display it later by main execution flow
                    * (set_change_passwd) in case of a real error
                    */
                    strcpy(appdata->err_msg, msg[i]->msg);
                }
        }

        *resp = resp_array; /* Assign responses array pointer for PAM */
    }
    else
        return PAM_BUF_ERR;

    return PAM_SUCCESS;
}

/**
 * Set (change) user password over PAM (i. e. portably across UNIX'es).
 *
 * @param user          user name
 * @param passwd        user password
 *
 * @return              Return code (0 on success, -1 on error)
 */
static int
set_change_passwd(char const *user, char const *passwd)
{
    pam_handle_t       *handle;
    appdata_t           appdata;  /**< Data passed to callback and back */
    struct pam_conv     conv;     /**< Callback structure */

    int pam_rc;
    int rc = -1;

    appdata.passwd     = passwd;
    appdata.err_msg[0] = '\0';

    conv.conv        = &conv_fun; /**< callback function */
    conv.appdata_ptr = &appdata;  /**< data been passed to callback fun */

    /** Check user existence */
    if(getpwnam(user) != NULL)
    {
        /** Initialize PAM library */
        if ((pam_rc = pam_start("passwd", user, &conv, &handle))
            == PAM_SUCCESS)
        {
            uid_t euid = geteuid(); /**< Save current effective user id */

            if (setuid(0) == 0)     /**< Get 'root' */
            {
                /** Try to set/change password */
                if ((pam_rc = pam_chauthtok(handle, PAM_FLAGS))
                    == PAM_SUCCESS)
                    rc = 0;
                else
                {
                    ERROR("pam_chauthtok, user: '%s', passwd: '%s': %s",
                          user, passwd, pam_strerror(handle, pam_rc));

                   /* If callback function received error message string
                    * then type it too
                    */
                    if (appdata.err_msg[0]) 
                        ERROR("%s", appdata.err_msg);
                }

                setuid(euid);       /* Restore saved previously user id */
            }
            else
                ERROR("setuid: %s", strerror(errno));

            /** Terminate PAM library */
            if ((pam_rc = pam_end(handle, pam_rc)) != PAM_SUCCESS)
                ERROR("pam_end: %s", pam_strerror(handle, pam_rc));
        }
        else
            ERROR("pam_start, user: '%s', passwd: '%s': %s", user, passwd,
                 pam_strerror(handle, pam_rc));
    }
    else
        ERROR("getpwnam, user '%s': %s",
              user, errno ? strerror(errno) : "User does not exist");

    return rc;
}
#endif /* TA_USE_PAM */

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
#if TA_USE_PAM || defined(__linux__)
    char *tmp;
    char *tmp1;

    unsigned int uid;

    te_errno     rc;
#endif

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);

#if !TA_USE_PAM && !defined(__linux__)
    UNUSED(user);
    ERROR("user_add failed (no user management facilities available)");
    return TE_RC(TE_TA_UNIX, TE_ENOSYS);
#else
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

    /* Fedora has very aggressive nscd cache */
    /* https://bugzilla.redhat.com/bugzilla/show_bug.cgi?id=134323 */
    ta_system("/usr/sbin/nscd -i group && /usr/sbin/nscd -i passwd");

#if TA_USE_PAM
    /** Set (change) password for just added user */
    if (set_change_passwd(user, user) != 0)
#else
    sprintf(buf, "echo %s:%s | /usr/sbin/chpasswd", user, user);
    if ((rc = ta_system(buf)) != 0)
#endif
    {
        ERROR("change_passwd failed");
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
#endif /* !TA_USE_PAM */
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

/* XEN stuff implementation */

/** Maximal number of maintained domUs */
enum { MAX_DOM_U_NUM = 1024 };

/** DomU statuses */
typedef enum { DOM_U_STATUS_NON_RUNNING,
               DOM_U_STATUS_RUNNING,
               DOM_U_STATUS_SAVED,
               DOM_U_STATUS_MIGRATED,
               DOM_U_STATUS_ERROR } status_t;

/**
 * Path to accessible across network storage for
 * XEN kernel and templates of XEN config/VBD images.
 */
static char xen_path[PATH_MAX] = { '\0' };

/** Kernel, initial ramdisk and VBD image files */
static char const *const xen_kernel = "vmlinuz-2.6.18-4-xen-686";
static char const *const xen_ramdsk = "initrd.img-2.6.18-4-xen-686";
static char const *const xen_dsktpl = "disk-template.img";
static char const *const xen_tmpdir = "tmpdir";

/** Status name to status and vice versa translation array */
static struct {
    char const *name; /**< Status name */
    status_t status;  /**< Status      */
} const statuses[] = { { "non-running", DOM_U_STATUS_NON_RUNNING },
                       { "running",     DOM_U_STATUS_RUNNING },
                       { "saved",       DOM_U_STATUS_SAVED },
                       { "migrated",    DOM_U_STATUS_MIGRATED } };

/** DomU internal representation */
static struct {
    char const *name;         /**< DomU name (also serves as slot
                                   is empty sign if it is NULL)     */
    status_t    status;       /**< DomU state                       */
    char        ip_addr[16];  /**< DomU IP address                  */
    char        mac_addr[18]; /**< DomU MAC address                 */
    int         migrate_kind; /**< Migrate kind (non-live/live)     */
} dom_u_slot[MAX_DOM_U_NUM];

/**
 * Get the whole number of domU slots.
 *
 * @return              The whole number of domU slots
 */
static inline unsigned int
dom_u_limit(void)
{
    return sizeof(dom_u_slot) / sizeof(*dom_u_slot);
}

/**
 * Find domU.
 *
 * @param dom_u         The name of the domU to find
 *
 * @return              domU index (from 0 to DOM_U_MAX_NUM - 1) if
 *                      found, otherwise - 'sizeof(dom_u_list) /
 *                      sizeof(*dom_u_list)' (which is equivlent to
 *                      DOM_U_MAX_NUM)
 */
static unsigned
find_dom_u(char const *dom_u)
{
    unsigned int u;
    unsigned int limit = dom_u_limit();

    for (u = 0; u < limit; u++)
    {
        char const *name = dom_u_slot[u].name;

        if (name != NULL && strcmp(name, dom_u) == 0)
            break;
    }

    return u;
}

/* Try to find domU and initialize its index */
#define FIND_DOM_U(dom_u_name_, dom_u_index_) \
    do {                                                               \
        if ((dom_u_index_ = find_dom_u(dom_u_name_)) >= dom_u_limit()) \
        {                                                              \
            ERROR("DomU %s does NOT exist", dom_u_name_);              \
            return TE_RC(TE_TA_UNIX, TE_ENOENT);                       \
        }                                                              \
    } while(0)


/**
 * Converts status to its string representation.
 *
 * @param status        Status to be converted to string representation
 *
 * @return              String representation or NULL in case of an error
 */
static char const *
dom_u_status_to_string(status_t status)
{
    unsigned int u;

    for (u = 0; u < sizeof(statuses) / sizeof(*statuses); u++)
        if (statuses[u].status == status)
            return statuses[u].name;

    return NULL;
}

/**
 * Converts status string representation to status.
 *
 * @param status_string Status string representation
 *
 * @return              Converted status value or
 *                      DOM_U_STATUS_ERROR in case of an error
 */
static status_t
dom_u_status_string_to_status(char const *status_string)
{
    unsigned int u;

    for (u = 0; u < sizeof(statuses) / sizeof(*statuses); u++)
        if (strcmp(statuses[u].name, status_string) == 0)
            return statuses[u].status;

    return DOM_U_STATUS_ERROR;
}

/**
 * Checks whether the agent runs within dom0 or not
 *
 * @return              TRUE if the agent runs within dom0,
 *                      otherwise - FALSE
 */
static te_bool
is_within_dom0(void)
{
#ifdef HAVE_SYS_STAT_H
    struct stat st;

    /* Probably there is better mean do detect we are within dom0, eh? */
    return stat("/usr/sbin/xm", &st) == 0 &&
           (S_ISLNK(st.st_mode) || S_ISREG(st.st_mode));
#else
#warning "is_within_dom0 helper function for XEN is not implemented"
    return TE_OS_RC(TE_TA_UNIX, TE_EINVAL);
#endif
}

/**
 * Removes directory and all its subdirectories
 *
 * @param dir           Directory path
 *
 * @return              Status code
 */
static te_errno
xen_rmfr(char const *dir)
{
    /* FIXME: Non "ta_system" implementation is needed*/
    char const* const cmd = "rm -fr ";
    char *const cmdline = malloc(strlen(cmd) + strlen(dir) + 1);

    strcpy(cmdline, cmd);
    strcat(cmdline, dir);

    if (ta_system(cmdline) != 0)
    {
        free(cmdline);
        return TE_RC(TE_TA_UNIX, TE_EFAIL);
    }

    free(cmdline);
    return 0;
}

/**
 * (Re)creates file inside disk image and fills it with supplied data
 *
 * @param dom_u         domU
 * @param fname         File name inside disk image (path from root /)
 * @param fdata         Data string (zero ended)
 *
 * @return              Status code
 */
static te_errno
xen_fill_file_in_disk_image(char const *dom_u, char const *fname,
                            char const *fdata)
{
#ifdef HAVE_SYS_STAT_H
    char        buffer[PATH_MAX];
    struct stat st;
    te_errno    rc = 0;
    FILE       *f;
    int         sys;

    TE_SPRINTF(buffer, "%s/%s/%s", xen_path, dom_u, xen_tmpdir);

    if (stat(buffer, &st) == 0)
        goto cleanup2;

    if (mkdir(buffer, S_IRWXU | S_IRWXG | S_IRWXO) == -1)
    {
        ERROR("Failed to create temporary %s directory", buffer);
        rc = TE_RC(TE_TA_UNIX, TE_EFAIL);
        goto cleanup0;
    }

    if (chmod(buffer, S_IRWXU | S_IRWXG | S_IRWXO) == -1)
    {
        ERROR("Failed to chmod temporary %s directory", buffer);
        rc = TE_RC(TE_TA_UNIX, TE_EFAIL);
        goto cleanup1;
    }

    /* FIXME: Non "ta_system" implementation is needed*/
    TE_SPRINTF(buffer, "mount -o loop %s/%s/disk.img %s/%s/%s",
               xen_path, dom_u, xen_path, dom_u, xen_tmpdir);

    if ((sys = ta_system(buffer)) != 0 && !(sys == -1 && errno == ECHILD))
    {
        rc = TE_RC(TE_TA_UNIX, TE_EFAIL);
        goto cleanup1;
    }

    TE_SPRINTF(buffer, "%s/%s/%s%s", xen_path, dom_u, xen_tmpdir, fname);

    if ((f = fopen(buffer, "w")) == NULL)
    {
        ERROR("Failed to open %s file for writing", buffer);
        rc = TE_RC(TE_TA_UNIX, TE_EFAIL);
        goto cleanup2;
    }

    if ((size_t)fprintf(f, "%s", fdata) != strlen(fdata))
    {
        ERROR("Failed to write %s file with data:\n%s", buffer, fdata);
        rc = TE_RC(TE_TA_UNIX, TE_EFAIL);
    }

    if (fclose(f) != 0)
    {
        ERROR("Failed to close %s file after writing", buffer);

        if (rc == 0)
            rc = TE_RC(TE_TA_UNIX, TE_EFAIL);
    }

cleanup2:
    /* FIXME: Non "ta_system" implementation is needed*/
    TE_SPRINTF(buffer, "umount %s/%s/%s", xen_path, dom_u, xen_tmpdir);

    if ((sys = ta_system(buffer)) != 0 && !(sys == -1 && errno == ECHILD))
    {
        if (rc == 0)
            rc = TE_RC(TE_TA_UNIX, TE_EFAIL);
    }

cleanup1:
    TE_SPRINTF(buffer, "%s/%s/%s", xen_path, dom_u, xen_tmpdir);

    if (rmdir(buffer) == -1)
    {
        if (rc == 0)
            rc = TE_RC(TE_TA_UNIX, TE_EFAIL);
    }

cleanup0:
    return rc;
#else
#warning "xen_fill_file_in_disk_image helper for XEN is not implemented"
#endif
}

/**
 * Get path to accessible across network storage for
 * XEN kernel and templates of XEN config/VBD images.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         storage for path to be filled in
 *
 * @return              Status code
 */
static te_errno
xen_path_get(unsigned int gid, char const *oid, char *value)
{
#ifdef HAVE_SYS_STAT_H
    struct stat st;

    UNUSED(gid);
    UNUSED(oid);

    strcpy(value, xen_path);
    return 0;
#else
#warning "xen_path_get method for XEN is not implemented"
#endif
}

/**
 * Set path to accessible across network storage for
 * XEN kernel and templates of XEN config/VBD images.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         path to set
 *
 * @return              Status code
 */
static te_errno
xen_path_set(unsigned int gid, char const *oid, char const *value)
{
#ifdef HAVE_SYS_STAT_H
    unsigned int u;
    unsigned int limit = dom_u_limit();
    size_t       len   = strlen(value);

    UNUSED(gid);
    UNUSED(oid);

    /* If value is not empty string then the agent must run within dom0 */
    if (*value != '\0' && !is_within_dom0())
    {
        ERROR("Agent runs NOT within dom0");
        return TE_RC(TE_TA_UNIX, TE_EFAIL);
    }

    /* Check whether domUs exist */
    for (u = 0; u < limit; u++)
        if (dom_u_slot[u].name != NULL)
        {
            ERROR("Failed to change XEN path: domU(s) exist(s)");
            return TE_RC(TE_TA_UNIX, TE_EBUSY);
        }

    /* Check whether XEN path fits XEN path storage */
    if (len >= sizeof(xen_path))
    {
        ERROR("XEN path is too long");
        return TE_RC(TE_TA_UNIX, TE_E2BIG);
    }

    /* For non-empty XEN path perform all necessary checks */
    if (len > 0)
    {
        struct stat  st;

        if (*value != '/')
        {
            ERROR("XEN path must be absolute (starting from \"/\")");
            return TE_RC(TE_TA_UNIX, TE_EINVAL);
        }

        if (stat(value, &st) == -1)
        {
            ERROR("Path specified for XEN does NOT exist");
            return TE_RC(TE_TA_UNIX, TE_ENOENT);
        }

        if (!S_ISDIR(st.st_mode))
        {
            ERROR("Path specified for XEN is not a directory");
            return TE_RC(TE_TA_UNIX, TE_ENOTDIR);
        }

        TE_SPRINTF(buf, "%s/%s", value, xen_kernel);

        if (stat(buf, &st) == -1)
        {
            ERROR("XEN kernel does NOT exist on specified XEN path");
            return TE_RC(TE_TA_UNIX, TE_ENOENT);
        }

        if (!S_ISREG(st.st_mode))
        {
            ERROR("XEN kernel specified is NOT a file");
            return TE_RC(TE_TA_UNIX, TE_ENOENT);
        }

        TE_SPRINTF(buf, "%s/%s", value, xen_ramdsk);

        if (stat(buf, &st) == -1)
        {
            ERROR("XEN initial ramdisk does NOT exist "
                  "on specified XEN path");
            return TE_RC(TE_TA_UNIX, TE_ENOENT);
        }

        if (!S_ISREG(st.st_mode))
        {
            ERROR("XEN initial ramdisk specified is NOT a file");
            return TE_RC(TE_TA_UNIX, TE_ENOENT);
        }

        TE_SPRINTF(buf, "%s/%s", value, xen_dsktpl);

        if (stat(buf, &st) == -1)
        {
            ERROR("XEN disk image template does NOT exist "
                  "on specified XEN path");
            return TE_RC(TE_TA_UNIX, TE_ENOENT);
        }

        if (!S_ISREG(st.st_mode))
        {
            ERROR("XEN disk image template specified is NOT a file");
            return TE_RC(TE_TA_UNIX, TE_ENOENT);
        }
    }

    memcpy(xen_path, value, len + 1);
    return 0;
#else
#warning "xen_path_set method for XEN is not implemented"
#endif
}

/**
 * Get presence of directory/images state of domU.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         storage for status to be filled in
 * @param xen           name of the XEN node (empty, unused)
 * @param dom_u         name of the domU to get status of
 *
 * @return              Status code
 */
static te_errno
dom_u_get(unsigned int gid, char const *oid, char *value,
          char const *xen, char const *dom_u)
{
#ifdef HAVE_SYS_STAT_H
    unsigned int u;
    struct stat  st;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(xen);

    FIND_DOM_U(dom_u, u);

    TE_SPRINTF(buf, "%s/%s", xen_path, dom_u);
    strcpy(value, stat(buf, &st) == 0 ? "1" : "0");
    return 0;
#else
#warning "dom_u_get_set method for XEN is not implemented"
#endif
}

/**
 * Set (change) presence of directory/images state of domU.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         status to set
 * @param xen           name of the XEN node (empty, unused)
 * @param dom_u         name of the domU to set status of
 *
 * @return              Status code
 */
static te_errno
dom_u_set(unsigned int gid, char const *oid, char const *value,
           char const *xen, char const *dom_u)
{
#ifdef HAVE_SYS_STAT_H
    unsigned int u;
    struct stat  st;
    int          sys;
    te_bool      to_set = strcmp(value, "1") == 0;
    te_bool      is_set;
    te_errno     rc = 0;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(xen);

    FIND_DOM_U(dom_u, u);

    TE_SPRINTF(buf, "%s/%s", xen_path, dom_u);
    is_set = stat(buf, &st) == 0;

    /* If desired state is already exists, do nothing */
    if ((is_set && to_set) || (!is_set && !to_set))
        return 0;

    /* If not to set then remove domU directory and disk images */
    if (!to_set)
        goto cleanup1;

    /* Otherwise, create domU directory and all necessary images */
    if (mkdir(buf, S_IRWXU | S_IRWXG | S_IRWXO) == -1)
    {
        ERROR("Failed to create domU directory %s", buf);
        rc = TE_RC(TE_TA_UNIX, TE_EFAIL);
        goto cleanup0;
    }

    if (chmod(buf, S_IRWXU | S_IRWXG | S_IRWXO) == -1)
    {
        ERROR("Failed to chmod domU directory %s", buf);
        rc = TE_RC(TE_TA_UNIX, TE_EFAIL);
        goto cleanup1;
    }

    /* FIXME: Non "ta_system" implementation is needed*/
    TE_SPRINTF(buf, "set -x; cp --sparse=always %s/%s %s/%s/disk.img",
               xen_path, xen_dsktpl, xen_path, dom_u);

    if ((sys = ta_system(buf)) != 0 && !(sys == -1 && errno == ECHILD))
    {
        rc = TE_RC(TE_TA_UNIX, TE_EFAIL);
        goto cleanup1;
    }

    TE_SPRINTF(buf, "%s/%s/disk.img", xen_path, dom_u);

    if (chmod(buf, S_IRWXU | S_IRWXG | S_IRWXO) == -1)
    {
        ERROR("Failed to chmod domU disk image %s", buf);
        rc = TE_RC(TE_TA_UNIX, TE_EFAIL);
        goto cleanup1;
    }

    /* FIXME: Non "ta_system" implementation is needed*/
    TE_SPRINTF(buf, "set -x; dd if=/dev/zero of=%s/%s/swap.img "
               "bs=1K seek=131071 count=1", xen_path, dom_u);

    if ((sys = ta_system(buf)) != 0 && !(sys == -1 && errno == ECHILD))
    {
        rc = TE_RC(TE_TA_UNIX, TE_EFAIL);
        goto cleanup1;
    }

    TE_SPRINTF(buf, "%s/%s/swap.img", xen_path, dom_u);

    if (chmod(buf, S_IRWXU | S_IRWXG | S_IRWXO) == -1)
    {
        ERROR("Failed to chmod domU swap image %s", buf);
        rc = TE_RC(TE_TA_UNIX, TE_EFAIL);
        goto cleanup1;
    }

    if ((rc = xen_fill_file_in_disk_image(dom_u, "/etc/udev/rules.d/"
                                          "z25_persistent-net.rules",
                                          "")) != 0)
    {
        goto cleanup1;
    }

    if ((rc = xen_fill_file_in_disk_image(dom_u, "/etc/hostname",
                                          dom_u)) == 0)
    {
        goto cleanup0;
    }

cleanup1:
    /* Erase domU directory and disk images unconditionally */
    TE_SPRINTF(buf, "%s/%s", xen_path, dom_u);
    xen_rmfr(buf);

cleanup0:
    return rc;
#else
#warning "dom_u_set_set method for XEN is not implemented"
#endif
}

/**
 * Add new domU.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         initializing value (not used)
 * @param xen           name of the XEN node (empty, unused)
 * @param dom_u         name of the domU to add
 *
 * @return              Status code
 */
static te_errno
dom_u_add(unsigned int gid, char const *oid, char const *value,
          char const *xen, char const *dom_u)
{
    unsigned int u;
    unsigned int limit = dom_u_limit();
    te_errno     rc    = 0;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(xen);

    if (!is_within_dom0())
    {
        ERROR("Agent runs NOT within dom0");
        return TE_RC(TE_TA_UNIX, TE_EFAIL);
    }

    if (*xen_path == '\0')
    {
        ERROR("Failed to add domU %s since XEN path is not set", dom_u);
        return TE_RC(TE_TA_UNIX, TE_EFAIL);
    }

    if ((u = find_dom_u(dom_u)) < dom_u_limit())
    {
        ERROR("Failed to add DomU %s: it is already exists", dom_u);
        return TE_RC(TE_TA_UNIX, TE_EEXIST);
    }

    /* Find an empty slot */
    for (u = 0; u < limit; u++)
        if (dom_u_slot[u].name == NULL)
            break;

    /* If an empty slot is NOT found */
    if (u == limit)
    {
        ERROR("Failed to add domU %s: all domU slots are taken", dom_u);
        return TE_RC(TE_TA_UNIX, TE_E2BIG);
    }

    if ((dom_u_slot[u].name = strdup(dom_u)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    /* Try to set requested presence of directory/images state of domU */
    if ((rc = dom_u_set(gid, oid, value, xen, dom_u)) != 0)
    {
        /* Rollback */
        free((void *)dom_u_slot[u].name);
        dom_u_slot[u].name = NULL;
    }

    /* Assign here initial values (modified later from within TAPI) */
    dom_u_slot[u].status = DOM_U_STATUS_NON_RUNNING;
    strcpy(dom_u_slot[u].ip_addr, "0.0.0.0");
    strcpy(dom_u_slot[u].mac_addr, "00:00:00:00:00:00");
    dom_u_slot[u].migrate_kind = 0;

    return rc;
}

/**
 * Delete domU.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param xen           name of the XEN node (empty, unused)
 * @param dom_u         name of the domU to delete
 *
 * @return              Status code
 */
static te_errno
dom_u_del(unsigned int gid, char const *oid, char const *xen,
          char const *dom_u)
{
    unsigned int u;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(xen);

    FIND_DOM_U(dom_u, u);

    free((void *)dom_u_slot[u].name);
    dom_u_slot[u].name = NULL;
    return 0;
}

/**
 * List domUs.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param list          address of a pointer to storage allocated
 *                      for the list pointer is initialized with
 *
 * @return              Status code
 */
static te_errno
dom_u_list(unsigned int gid, char const *oid, char **list)
{
    unsigned int u;
    unsigned int limit = dom_u_limit();
    unsigned int len = 0;
    char        *ptr;

    UNUSED(gid);
    UNUSED(oid);

    /* Count the whole length of domU names plus one per name */
    for (u = 0; u < limit; u++)
        if (dom_u_slot[u].name != NULL)
            len += strlen(dom_u_slot[u].name) + 1;

    if ((ptr = malloc(len)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    *(*list = ptr) = '\0';

    /**
     * Fill in the list with existing domU names
     * separated with spaces except the last one
     */
    for (u = 0; u < limit; u++)
    {
        char const *name = dom_u_slot[u].name;

        if (name != NULL)
        {
            size_t len = strlen(name);

            if (ptr != *list)
                *ptr++ = ' ';

            memcpy(ptr, name, len);
            *(ptr += len) = '\0';
        }
    }

    return 0;
}

/**
 * Get domU status.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         storage for status to be filled in
 * @param xen           name of the XEN node (empty, unused)
 * @param dom_u         name of the domU to get status of
 *
 * @return              Status code
 */
static te_errno
dom_u_status_get(unsigned int gid, char const *oid, char *value,
                 char const *xen, char const *dom_u)
{
    char const  *s;
    unsigned int u;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(xen);

    FIND_DOM_U(dom_u, u);

    if ((s = dom_u_status_to_string(dom_u_slot[u].status)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    strcpy(value, s);
    return 0;
}

/**
 * Set (change) domU status; business logic is moved to TAPI.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         status to set
 * @param xen           name of the XEN node (empty, unused)
 * @param dom_u         name of the domU to set status of
 *
 * @return              Status code
 */
static te_errno
dom_u_status_set(unsigned int gid, char const *oid, char const *value,
                 char const *xen, char const *dom_u)
{
    unsigned int u;
    FILE        *f;
    status_t     status = dom_u_status_string_to_status(value);
    te_errno     rc     = 0;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(xen);

    if (status == DOM_U_STATUS_ERROR)
    {
        rc = TE_RC(TE_TA_UNIX, TE_EINVAL);
        goto cleanup0;
    }

    FIND_DOM_U(dom_u, u);

    /* If nothing to do */
    if ((dom_u_slot[u].status == status))
        goto cleanup0;

    /**
     * "Non-running" -> "migrated" pseudo transition: really the
     * status is either left in "non-running" or set as "running"
     */
    if (dom_u_slot[u].status == DOM_U_STATUS_NON_RUNNING &&
        status == DOM_U_STATUS_MIGRATED)
    {
        size_t len = strlen(dom_u);

        /* FIXME: Non "popen" implementation is needed*/
        TE_SPRINTF(buf, "xm list | awk '{print$1}' 2>/dev/null");

        if ((f = popen(buf, "r")) == NULL)
        {
            rc = TE_OS_RC(TE_TA_UNIX, errno);
            ERROR("popen(%s) failed with errno %d", buf, rc);
            goto cleanup0;
        }

        while (fgets(buf, sizeof(buf), f) != NULL)
        {
            if (strncmp(buf, dom_u, len) == 0)
            {
                status = DOM_U_STATUS_RUNNING;
                break;
            }
        }

        pclose(f);

        if (status != DOM_U_STATUS_RUNNING)
        {
            ERROR("Failed to accept migrated domU %s", dom_u);
            rc = TE_RC(TE_TA_UNIX, TE_EFAIL);
            goto cleanup0;
        }

        goto cleanup1; /** Imitation of 'break' statement */
    }

    /* "Non-running" -> "running" transition */
    if (dom_u_slot[u].status == DOM_U_STATUS_NON_RUNNING &&
        status == DOM_U_STATUS_RUNNING)
    {
        /* IP address must be set for domU */
        if (strcmp(dom_u_slot[u].ip_addr, "0.0.0.0") == 0)
        {
            ERROR("DomU %s IP address is not set", dom_u);
            rc = TE_RC(TE_TA_UNIX, TE_EFAIL);
            goto cleanup0;
        }

        /* Create XEN domU configuration file */
        TE_SPRINTF(buf, "%s/%s/conf.cfg", xen_path, dom_u);

        if ((f = fopen(buf, "w")) == NULL)
        {
            ERROR("Failed to (re)create domU %s configuration file %s",
                  dom_u, buf);
            rc = TE_RC(TE_TA_UNIX, TE_EFAIL);
            goto cleanup0;
        }

        if (fprintf(f, "kernel='%s/%s'\n", xen_path, xen_kernel)  < 0 ||
            fprintf(f, "ramdisk='%s/%s'\n", xen_path, xen_ramdsk) < 0 ||
            fprintf(f, "memory='128'\n")                          < 0 ||
            fprintf(f, "root='/dev/sda1 ro'\n")                   < 0 ||
            fprintf(f, "disk=[ 'file:%s/%s/disk.img,sda1,w', "
                       "'file:%s/%s/swap.img,sda2,w' ]\n",
                       xen_path, dom_u, xen_path, dom_u)          < 0 ||
            fprintf(f, "name='%s'\n", dom_u)                      < 0)
        {
            rc = TE_RC(TE_TA_UNIX, TE_EFAIL);
            goto cleanup2;
        }

        /* If MAC address is NOT specified it is set by XEN automatically */
        if ((strcmp(dom_u_slot[u].mac_addr, "00:00:00:00:00:00") == 0 ?
                        fprintf(f, "vif  = [ 'bridge=xenbr0' ]\n") :
                        fprintf(f, "vif  = [ 'bridge=xenbr0,mac=%s' ]\n",
                                dom_u_slot[u].mac_addr)) < 0)
        {
            rc = TE_RC(TE_TA_UNIX, TE_EFAIL);
            goto cleanup2;
        }

        if (fprintf(f, "on_poweroff = 'destroy'\n") < 0 ||
            fprintf(f, "on_reboot   = 'restart'\n") < 0 ||
            fprintf(f, "on_crash    = 'restart'\n") < 0)
        {
            rc = TE_RC(TE_TA_UNIX, TE_EFAIL);
            goto cleanup2;
        }

        if (fflush(f) != 0)
            rc = TE_RC(TE_TA_UNIX, TE_EFAIL);

cleanup2:
        if (fclose(f) != 0)
        {
            if (rc == 0)
                rc = TE_RC(TE_TA_UNIX, TE_EFAIL);
        }

        if (rc != 0)
            goto cleanup0;

        /* Clear list of interfaces */
        if ((rc = xen_fill_file_in_disk_image(dom_u, "/etc/udev/rules.d/"
                                              "z25_persistent-net.rules",
                                              "")) != 0)
        {
            goto cleanup0;
        }

        /* Creating domU "/etc/network/interfaces" file */
        TE_SPRINTF(buf,
                   "auto lo\n"
                   "iface lo inet loopback\n"
                   "\n"
                   "auto eth0\n"
                   "iface eth0 inet static\n"
                   "    address %s\n"
                   "    netmask 255.255.255.0\n", dom_u_slot[u].ip_addr);
        if ((rc = xen_fill_file_in_disk_image(dom_u,
                                              "/etc/network/interfaces",
                                              buf)) != 0)
        {
            goto cleanup0;
        }

        /* Starting domU */
        TE_SPRINTF(buf, "xm create %s/%s/conf.cfg", xen_path, dom_u);

        if (ta_system(buf) != 0)
        {
            rc = TE_RC(TE_TA_UNIX, TE_EFAIL);
            goto cleanup0;
        }

        /** 
         * FIXME: Here must be "domU is really started"
         * detection code rather than stupid 'sleep'
         */
        sleep(25);
        goto cleanup1; /** Imitation of 'break' statement */
    }

    /* "Running" -> "non-running" transition */
    if (dom_u_slot[u].status == DOM_U_STATUS_RUNNING &&
        status == DOM_U_STATUS_NON_RUNNING)
    {
        TE_SPRINTF(buf, "xm shutdown %s", dom_u);

        if (ta_system(buf) != 0)
        {
            rc = TE_RC(TE_TA_UNIX, TE_EFAIL);
            goto cleanup0;
        }

        /* FIXME: stupid sleep should be replaced with smarter code */
        sleep(15);
        goto cleanup1; /** Imitation of 'break' statement */
    }

    /* "Running" -> "saved" transition */
    if (dom_u_slot[u].status == DOM_U_STATUS_RUNNING &&
        status == DOM_U_STATUS_SAVED)
    {
        TE_SPRINTF(buf, "xm save %s %s/%s/saved.img",
                   dom_u, xen_path, dom_u);

        if (ta_system(buf) != 0)
        {
            rc = TE_RC(TE_TA_UNIX, TE_EFAIL);
            goto cleanup0;
        }

        /* FIXME: stupid sleep should be replaced with smarter code */
        sleep(10);
        goto cleanup1; /** Imitation of 'break' statement */
    }

    /* "Saved" -> "running" transition */
    if (dom_u_slot[u].status == DOM_U_STATUS_SAVED &&
        status == DOM_U_STATUS_RUNNING)
    {
        TE_SPRINTF(buf, "xm restore %s/%s/saved.img", xen_path, dom_u);

        if (ta_system(buf) != 0)
        {
            rc = TE_RC(TE_TA_UNIX, TE_EFAIL);
            goto cleanup0;
        }

        /* FIXME: stupid sleep should be replaced with smarter code */
        sleep(25);
        goto entry1; /** Imitation of absence of the 'break' statement */
    }

    /* "Saved" -> "non-running" transition */
    if (dom_u_slot[u].status == DOM_U_STATUS_SAVED &&
        status == DOM_U_STATUS_NON_RUNNING)
    {
entry1:
        TE_SPRINTF(buf, "%s/%s/saved.img", xen_path, dom_u);

        /* Error here is not critical */
        if (unlink(buf) == -1)
            ERROR("Failed to unlink %s/%s/saved.img", xen_path, dom_u);

        goto cleanup1; /** Imitation of 'break' statement */
    }

    /* All still unserviced transitions are erroneous */
    rc = TE_RC(TE_TA_UNIX, TE_EINVAL);
    goto cleanup0;

cleanup1: /** This label is used in case of success */
    dom_u_slot[u].status = status;

cleanup0: /** This label is used in case of an error */
    return rc;
}

/**
 * Get domU IP address.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         storage for IP address to be filled in
 * @param xen           name of the XEN node (empty, unused)
 * @param dom_u         name of the domU to get IP address of
 *
 * @return              Status code
 */
static te_errno
dom_u_ip_addr_get(unsigned int gid, char const *oid, char *value,
                  char const *xen, char const *dom_u)
{
    unsigned int u;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(xen);

    FIND_DOM_U(dom_u, u);

    strcpy(value, dom_u_slot[u].ip_addr);
    return 0;
}

/**
 * Set (change) domU IP address (possible only in non-running state).
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         status to set
 * @param xen           name of the XEN node (empty, unused)
 * @param dom_u         name of the domU to set status of
 *
 * @return              Status code
 */
static te_errno
dom_u_ip_addr_set(unsigned int gid, char const *oid, char const *value,
                  char const *xen, char const *dom_u)
{
    unsigned int u;
    size_t       len = strlen(value);

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(xen);

    FIND_DOM_U(dom_u, u);

    if (len >= sizeof(dom_u_slot[u].ip_addr))
    {
        ERROR("Too long IP address");
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    /**
     * IP address will be really changed in domU disk image
     * only when transition into "running" status is requested
     */
    strcpy(dom_u_slot[u].ip_addr, value);
    return 0;
}

/**
 * Get domU MAC address.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         storage for status to be filled in
 * @param xen           name of the XEN node (empty, unused)
 * @param dom_u         name of the domU to get status of
 *
 * @return              Status code
 */
static te_errno
dom_u_mac_addr_get(unsigned int gid, char const *oid, char *value,
                   char const *xen, char const *dom_u)
{
    unsigned int u;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(xen);

    FIND_DOM_U(dom_u, u);

    strcpy(value, dom_u_slot[u].mac_addr);
    return 0;
}

/**
 * Set (change) domU MAC address (possible only in non-running state).
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         status to set
 * @param xen           name of the XEN node (empty, unused)
 * @param dom_u         name of the domU to set status of
 *
 * @return              Status code
 */
static te_errno
dom_u_mac_addr_set(unsigned int gid, char const *oid, char const *value,
                   char const *xen, char const *dom_u)
{
    enum { ether_bytes = 6 };

    unsigned int u;
    size_t       len = strlen(value);

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(xen);

    FIND_DOM_U(dom_u, u);

    if (len >= sizeof(dom_u_slot[u].mac_addr))
    {
        ERROR("Too long MAC address");
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    /**
     * MAC address will be really changed in domU configuration
     * file only when transition into "running" status is requested
     */
    strcpy(dom_u_slot[u].mac_addr, value);
    return 0;
}

/**
 * Migrate to another XEN dom0.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         name of the agent running within XEN dom0
 * @param xen           name of the XEN node (empty, unused)
 * @param dom_u         name of the domU to migrate
 *
 * @return              Status code
 */
static te_errno
dom_u_migrate_set(unsigned int gid, char const *oid, char const *value,
                  char const *xen, char const *dom_u)
{
    unsigned int u;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(xen);

    FIND_DOM_U(dom_u, u);

    TE_SPRINTF(buf, "xm migrate %s %s %s",
               dom_u_slot[u].migrate_kind ? "--live" : "", dom_u, value);

    if (ta_system(buf) != 0)
    {
        ERROR("Failed to migrate domU %s", dom_u);
        return TE_RC(TE_TA_UNIX, TE_EFAIL);
    }

    return 0;
}

/**
 * Migrate to another XEN dom0.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         name of the agent running within XEN dom0
 * @param xen           name of the XEN node (empty, unused)
 * @param dom_u         name of the domU to migrate
 *
 * @return              Status code
 */
static te_errno
dom_u_migrate_kind_get(unsigned int gid, char const *oid, char *value,
                  char const *xen, char const *dom_u)
{
    unsigned u;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(xen);

    FIND_DOM_U(dom_u, u);

    strcpy(value, dom_u_slot[u].migrate_kind ? "1" : "0");
    return 0;
}

/**
 * Migrate to another XEN dom0.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         name of the agent running within XEN dom0
 * @param xen           name of the XEN node (empty, unused)
 * @param dom_u         name of the domU to migrate
 *
 * @return              Status code
 */
static te_errno
dom_u_migrate_kind_set(unsigned int gid, char const *oid, char const *value,
                  char const *xen, char const *dom_u)
{
    unsigned u;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(xen);

    FIND_DOM_U(dom_u, u);

    dom_u_slot[u].migrate_kind = (strcmp(value, "0") == 0 ? 0 : 1);
    return 0;
}
