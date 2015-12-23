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

#ifdef _NET_TIMESTAMPING_H
#include <linux/net_tstamp.h>
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

#ifdef HAVE_SYS_UTSNAME_H
#include <sys/utsname.h>
#endif

/* XEN support */
#if defined(HAVE_SYS_STAT_H)
#include <sys/stat.h> /** For 'struct stat' */

/* UNIX branching heritage: PATH_MAX can still be undefined here yet */
#if !defined(PATH_MAX)
#define PATH_MAX 108
#endif

#define XEN_SUPPORT 1
#else
#define XEN_SUPPORT 0
#endif /* HAVE_SYS_STAT_H */

#if defined(HAVE_PWD_H)
#include <pwd.h>
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
#include "conf_rule.h"
#include "te_shell_cmd.h"

#ifdef CFG_UNIX_DAEMONS
#include "conf_daemons.h"
#endif

#if defined(__linux__)
#include <linux/sockios.h>
#endif

#ifdef USE_LIBNETCONF
#include "netconf.h"
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

#if ((!defined(__linux__)) && (defined(USE_LIBNETCONF)))
#error netlink can be used on Linux only
#endif

/** User environment */
extern char **environ;

extern int link_addr_a2n(uint8_t *lladdr, int len, const char *str);

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

#ifdef ENABLE_VCM_SUPPORT
extern te_errno ta_unix_conf_vcm_init();
#endif
#ifdef ENABLE_WIFI_SUPPORT
extern te_errno ta_unix_conf_wifi_init();
#endif

#ifdef WITH_ISCSI
extern te_errno ta_unix_iscsi_target_init();
extern te_errno iscsi_initiator_conf_init();
#endif

#ifdef WITH_IPTABLES
extern te_errno ta_unix_conf_iptables_init();
#endif

#ifdef WITH_TR069_SUPPORT
extern te_errno ta_unix_conf_acse_init();
#endif

#ifdef WITH_SERIALPARSE
extern te_errno ta_unix_serial_parser_init();
extern te_errno ta_unix_serial_parser_cleanup();
#endif

extern te_errno ta_unix_conf_configfs_init();
extern te_errno ta_unix_conf_netconsole_init();
extern te_errno ta_unix_conf_sys_init();
extern te_errno ta_unix_conf_phy_init();
extern te_errno ta_unix_conf_eth_init(void);

#ifdef USE_LIBNETCONF
netconf_handle nh = NETCONF_HANDLE_INVALID;
#endif

#ifdef WITH_AGGREGATION
extern te_errno ta_unix_conf_aggr_init();
#endif

#ifdef WITH_SNIFFERS
extern te_errno ta_unix_conf_sniffer_init();
extern te_errno ta_unix_conf_sniffer_cleanup();
#endif

extern te_errno ta_unix_conf_cmd_monitor_init(void);
extern te_errno ta_unix_conf_cmd_monitor_cleanup(void);

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

#define INTERFACE_IS_PPP(ifname) \
     (strncmp(ifname, "ppp", strlen("ppp")) == 0)

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

/*
 * To access attributes of a network interface we can use
 * ioctl based interface available on most UNIX systems.
 * Widely used data structure in interface-related ioctl calls
 * is "struct ifreq", but on some systems (for example Solaris)
 * this structure is obsoleted and "struct lifreq" is used
 * instead.
 *
 * We try to avoid code duplication that only differs in
 * structure and field names, which is why we try to use
 * system independent names (prefixed with "my_").
 */
#ifdef HAVE_STRUCT_LIFREQ
#define my_ifreq        lifreq
#define my_ifr_name     lifr_name
#define my_ifr_flags    lifr_flags
#define my_ifr_addr     lifr_addr
#define my_ifr_mtu      lifr_mtu
#define my_ifr_hwaddr_data(req_)   \
        ((struct sockaddr_dl *)&(req_).lifr_addr)->sdl_data
#define my_ifr_hwaddr_family(req_) \
        ((struct sockaddr_dl *)&(req_).lifr_addr)->sdl_family
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
#define MY_SIOCGIFHWADDR    SIOCGLIFHWADDR
#else
#define my_ifreq        ifreq
#define my_ifr_name     ifr_name
#define my_ifr_flags    ifr_flags
#define my_ifr_addr     ifr_addr
#define my_ifr_mtu      ifr_mtu
#define my_ifr_hwaddr_data(req_) (req_).ifr_hwaddr.sa_data
#define my_ifr_hwaddr_family(req_) (req_).ifr_hwaddr.sa_family
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

#if defined SIOCGIFHWADDR
#define MY_SIOCGIFHWADDR    SIOCGIFHWADDR
#endif

#endif /* !HAVE_STRUCT_LIFREQ */

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

static te_errno iface_ip4_fw_get(unsigned int, const char *, char *,
                                 const char *);
static te_errno iface_ip4_fw_set(unsigned int, const char *, const char *,
                                 const char *);

static te_errno iface_ip6_fw_get(unsigned int, const char *, char *,
                                 const char *);
static te_errno iface_ip6_fw_set(unsigned int, const char *, const char *,
                                 const char *);

static te_errno iface_ip6_accept_ra_get(unsigned int, const char *, char *,
                                const char *);
static te_errno iface_ip6_accept_ra_set(unsigned int, const char *,
                                const char *, const char *);

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

static te_errno rp_filter_get(unsigned int, const char *, char *,
                              const char *);
static te_errno rp_filter_set(unsigned int, const char *, const char *,
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

static te_errno agent_platform_get(unsigned int, const char *, char *,
                                   const char *, ...);

static te_errno agent_dir_get(unsigned int, const char *, char *,
                              const char *, ...);

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

static te_errno xen_subpath_get(unsigned int, char const *, char *);
static te_errno xen_subpath_set(unsigned int, char const *, char const *);

static te_errno xen_kernel_get(unsigned int, char const *, char *);
static te_errno xen_kernel_set(unsigned int, char const *, char const *);

static te_errno xen_initrd_get(unsigned int, char const *, char *);
static te_errno xen_initrd_set(unsigned int, char const *, char const *);

static te_errno xen_dsktpl_get(unsigned int, char const *, char *);
static te_errno xen_dsktpl_set(unsigned int, char const *, char const *);

static te_errno xen_rcf_port_get(unsigned int, char const *, char *);
static te_errno xen_rcf_port_set(unsigned int, char const *,
                                 char const *);

static te_errno xen_rpc_br_get(unsigned int, char const *, char *);
static te_errno xen_rpc_br_set(unsigned int, char const *, char const *);

static te_errno xen_rpc_if_get(unsigned int, char const *, char *);
static te_errno xen_rpc_if_set(unsigned int, char const *, char const *);

static te_errno xen_base_mac_addr_get(unsigned int, char const *, char *);
static te_errno xen_base_mac_addr_set(unsigned int, char const *,
                                      char const *);

static te_errno xen_accel_get(unsigned int, char const *, char *);
static te_errno xen_accel_set(unsigned int, char const *, char const *);

static te_errno xen_init_set(unsigned int, char const *, char const *);

static te_errno xen_interface_add(unsigned int, char const *, char const *,
                                  char const *, char const *);
static te_errno xen_interface_del(unsigned int, char const *, char const *,
                                  char const *);
static te_errno xen_interface_list(unsigned int, char const *, char **);
static te_errno xen_interface_get(unsigned int, char const *, char *,
                                  char const *, char const *);
static te_errno xen_interface_set(unsigned int, char const *, char const *,
                                  char const *, char const *);

static te_errno xen_interface_bridge_get(unsigned int, char const *,
                                         char *, char const *,
                                         char const *);
static te_errno xen_interface_bridge_set(unsigned int, char const *,
                                         char const *, char const *,
                                         char const *);

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

static te_errno dom_u_memory_get(unsigned int, char const *, char *,
                                 char const *, char const *);
static te_errno dom_u_memory_set(unsigned int, char const *,
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

static te_errno dom_u_bridge_add(unsigned int, char const *, char const *,
                                 char const *, char const *, char const *);
static te_errno dom_u_bridge_del(unsigned int, char const *, char const *,
                                 char const *, char const *);
static te_errno dom_u_bridge_list(unsigned int, char const *, char **,
                                  char const *, char const *);
static te_errno dom_u_bridge_get(unsigned int, char const *, char *,
                                 char const *, char const *, char const *);
static te_errno dom_u_bridge_set(unsigned int, char const *, char const *,
                                 char const *, char const *, char const *);

static te_errno dom_u_bridge_ip_addr_get(unsigned int, char const *,
                                         char *, char const *,
                                         char const *, char const *);
static te_errno dom_u_bridge_ip_addr_set(unsigned int, char const *,
                                         char const *, char const *,
                                         char const *, char const *);

static te_errno dom_u_bridge_mac_addr_get(unsigned int, char const *,
                                          char *, char const *,
                                          char const *, char const *);
static te_errno dom_u_bridge_mac_addr_set(unsigned int, char const *,
                                          char const *, char const *,
                                          char const *, char const *);

static te_errno dom_u_bridge_accel_get(unsigned int, char const *,
                                       char *, char const *,
                                       char const *, char const *);
static te_errno dom_u_bridge_accel_set(unsigned int, char const *,
                                       char const *, char const *,
                                       char const *, char const *);

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

RCF_PCH_CFG_NODE_RO(node_platform, "platform",
                    NULL, NULL,
                    (rcf_ch_cfg_list)agent_platform_get);

RCF_PCH_CFG_NODE_RO(node_dir, "dir",
                    NULL, &node_platform,
                    (rcf_ch_cfg_list)agent_dir_get);

RCF_PCH_CFG_NODE_RO(node_dns, "dns",
                    NULL, &node_dir,
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

RCF_PCH_CFG_NODE_RW(node_rp_filter, "rp_filter", NULL, &node_vlans,
                    rp_filter_get, rp_filter_set);

RCF_PCH_CFG_NODE_RW(node_promisc, "promisc", NULL, &node_rp_filter,
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

RCF_PCH_CFG_NODE_RW(node_iface_ip4_fw, "iface_ip4_fw", NULL,
                    &node_bcast_link_addr, iface_ip4_fw_get,
                    iface_ip4_fw_set);

RCF_PCH_CFG_NODE_RW(node_iface_ip6_fw, "iface_ip6_fw", NULL,
                    &node_iface_ip4_fw, iface_ip6_fw_get, iface_ip6_fw_set);

RCF_PCH_CFG_NODE_RW(node_iface_ip6_accept_ra, "iface_ip6_accept_ra", NULL,
                    &node_iface_ip6_fw,
                    iface_ip6_accept_ra_get, iface_ip6_accept_ra_set);

RCF_PCH_CFG_NODE_RO(node_ifindex, "index", NULL, &node_iface_ip6_accept_ra,
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

RCF_PCH_CFG_NODE_RW(node_dom_u_bridge_accel, "accel",
                    NULL, NULL,
                    &dom_u_bridge_accel_get,
                    &dom_u_bridge_accel_set);

RCF_PCH_CFG_NODE_RW(node_dom_u_bridge_mac_addr, "mac_addr",
                    NULL, &node_dom_u_bridge_accel,
                    &dom_u_bridge_mac_addr_get,
                    &dom_u_bridge_mac_addr_set);

RCF_PCH_CFG_NODE_RW(node_dom_u_bridge_ip_addr, "ip_addr",
                    NULL, &node_dom_u_bridge_mac_addr,
                    &dom_u_bridge_ip_addr_get,
                    &dom_u_bridge_ip_addr_set);

static rcf_pch_cfg_object node_dom_u_bridge =
    { "bridge", 0, &node_dom_u_bridge_ip_addr, &node_dom_u_migrate,
      (rcf_ch_cfg_get)&dom_u_bridge_get,
      (rcf_ch_cfg_set)&dom_u_bridge_set,
      (rcf_ch_cfg_add)&dom_u_bridge_add,
      (rcf_ch_cfg_del)&dom_u_bridge_del,
      (rcf_ch_cfg_list)&dom_u_bridge_list, NULL, NULL };

RCF_PCH_CFG_NODE_RW(node_dom_u_mac_addr, "mac_addr",
                    NULL, &node_dom_u_bridge,
                    &dom_u_mac_addr_get, &dom_u_mac_addr_set);

RCF_PCH_CFG_NODE_RW(node_dom_u_ip_addr, "ip_addr",
                    NULL, &node_dom_u_mac_addr,
                    &dom_u_ip_addr_get, &dom_u_ip_addr_set);

RCF_PCH_CFG_NODE_RW(node_dom_u_memory, "memory",
                    NULL, &node_dom_u_ip_addr,
                    &dom_u_memory_get, &dom_u_memory_set);

RCF_PCH_CFG_NODE_RW(node_dom_u_status, "status",
                    NULL, &node_dom_u_memory,
                    &dom_u_status_get, &dom_u_status_set);

static rcf_pch_cfg_object node_dom_u =
    { "dom_u", 0, &node_dom_u_status, NULL,
      (rcf_ch_cfg_get)&dom_u_get, (rcf_ch_cfg_set)&dom_u_set,
      (rcf_ch_cfg_add)&dom_u_add, (rcf_ch_cfg_del)&dom_u_del,
      (rcf_ch_cfg_list)&dom_u_list, NULL, NULL };

RCF_PCH_CFG_NODE_RW(node_xen_interface_bridge, "bridge",
                    NULL, NULL,
                    &xen_interface_bridge_get, &xen_interface_bridge_set);

static rcf_pch_cfg_object node_xen_interface =
    { "interface", 0, &node_xen_interface_bridge, &node_dom_u,
      (rcf_ch_cfg_get)&xen_interface_get,
      (rcf_ch_cfg_set)&xen_interface_set,
      (rcf_ch_cfg_add)&xen_interface_add,
      (rcf_ch_cfg_del)&xen_interface_del,
      (rcf_ch_cfg_list)&xen_interface_list, NULL, NULL };

RCF_PCH_CFG_NODE_RW(node_xen_init, "init",
                    NULL, &node_xen_interface,
                    NULL, &xen_init_set);

RCF_PCH_CFG_NODE_RW(node_xen_accel, "accel",
                    NULL, &node_xen_init,
                    &xen_accel_get, &xen_accel_set);

RCF_PCH_CFG_NODE_RW(node_base_mac_addr, "base_mac_addr",
                    NULL, &node_xen_accel,
                    &xen_base_mac_addr_get, &xen_base_mac_addr_set);

RCF_PCH_CFG_NODE_RW(node_rpc_if, "rpc_if",
                    NULL, &node_base_mac_addr,
                    &xen_rpc_if_get, &xen_rpc_if_set);

RCF_PCH_CFG_NODE_RW(node_rpc_br, "rpc_br",
                    NULL, &node_rpc_if,
                    &xen_rpc_br_get, &xen_rpc_br_set);

RCF_PCH_CFG_NODE_RW(node_rcf_port, "rcf_port",
                    NULL, &node_rpc_br,
                    &xen_rcf_port_get, &xen_rcf_port_set);

RCF_PCH_CFG_NODE_RW(node_dsktpl, "dsktpl",
                    NULL, &node_rcf_port,
                    &xen_dsktpl_get, &xen_dsktpl_set);

RCF_PCH_CFG_NODE_RW(node_initrd, "initrd",
                    NULL, &node_dsktpl,
                    &xen_initrd_get, &xen_initrd_set);

RCF_PCH_CFG_NODE_RW(node_kernel, "kernel",
                    NULL, &node_initrd,
                    &xen_kernel_get, &xen_kernel_set);

RCF_PCH_CFG_NODE_RW(node_subpath, "subpath",
                    NULL, &node_kernel,
                    &xen_subpath_get, &xen_subpath_set);

RCF_PCH_CFG_NODE_RW(node_xen, "xen",
                    &node_subpath, &node_user,
                    &xen_path_get, &xen_path_set);


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

#if 0
    if (INTERFACE_IS_PPP(ifname) ||
        rcf_pch_rsrc_accessible("/agent:%s/pppoeserver:", ta_name))
        return TRUE;
#endif

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
 * Initialize base configuration.
 *
 * @return Status code.
 */
static inline te_errno
ta_unix_conf_base_init(void)
{
    return rcf_pch_add_node("/agent", &node_xen);
}

/* See the description in lib/rcfpch/rcf_ch_api.h */
int
rcf_ch_conf_init()
{
    static te_bool init = FALSE;

    if (!init)
    {
#ifdef USE_LIBNETCONF
        if (netconf_open(&nh) != 0)
        {
            ERROR("Failed to open netconf session");
            return -1;
        }
#endif

        if ((cfg_socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        {
            return -1;
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

        rcf_pch_rsrc_info("/agent/interface",
                          interface_grab,
                          interface_release);

        rcf_pch_rsrc_info("/agent/ip4_fw",
                          rcf_pch_rsrc_grab_dummy,
                          rcf_pch_rsrc_release_dummy);

        rcf_pch_rsrc_info("/agent/ip6_fw",
                          rcf_pch_rsrc_grab_dummy,
                          rcf_pch_rsrc_release_dummy);

        rcf_pch_rsrc_info("/agent/telephony_port",
                          interface_grab,
                          interface_release);

        if (ta_unix_conf_base_init() != 0)
            goto fail;

        if (ta_unix_conf_route_init() != 0)
            goto fail;

        if (ta_unix_conf_rule_init() != 0)
            goto fail;

#ifdef RCF_RPC
        /* Link RPC nodes */
        rcf_pch_rpc_init(ta_dir);
#endif

#ifdef WITH_NTPD
        if (ta_unix_conf_ntpd_init() != 0)
            goto fail;
#endif

#ifdef WITH_SFPTPD
        if (ta_unix_conf_sfptpd_init() != 0)
            goto fail;
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
#ifdef ENABLE_VCM_SUPPORT
        if (ta_unix_conf_vcm_init() != 0)
            goto fail;
#endif
#ifdef WITH_TR069_SUPPORT
        if (ta_unix_conf_acse_init() != 0)
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
#ifdef WITH_IPTABLES
        if (ta_unix_conf_iptables_init() != 0)
            goto fail;
#endif

        if (ta_unix_conf_sys_init() != 0)
            goto fail;

        /* Initialize configurator PHY support */
        if (ta_unix_conf_phy_init() != 0)
            goto fail;

        if (ta_unix_conf_configfs_init() != 0)
            goto fail;

        if (ta_unix_conf_netconsole_init() != 0)
            goto fail;

        if (ta_unix_conf_eth_init() != 0)
            goto fail;

        rcf_pch_rsrc_init();
        
#ifdef WITH_AGGREGATION
        if (ta_unix_conf_aggr_init() != 0)
        {
            ERROR("Failed to add aggregation configuration tree");
            goto fail;
        }
#endif

#ifdef WITH_SERIALPARSE
        ta_unix_serial_parser_init();
#endif
#ifdef WITH_SNIFFERS
        if (ta_unix_conf_sniffer_init() != 0)
            ERROR("Failed to add sniffer configuration tree");
#endif

        ta_unix_conf_cmd_monitor_init();

        init = TRUE;

    }
    return 0;

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
    return -1;
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
rcf_ch_conf_fini()
{
#ifdef WITH_SERIALPARSE
    ta_unix_serial_parser_cleanup();
#endif
#ifdef WITH_SNIFFERS
    ta_unix_conf_sniffer_cleanup();
#endif
#ifdef CFG_UNIX_DAEMONS
    ta_unix_conf_daemons_release();
#endif
#ifdef WITH_SFPTPD
    ta_unix_conf_sfptpd_release();
#endif
    ta_unix_conf_cmd_monitor_cleanup();
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

#if defined __sun__
/**
 * Callback function used in VLAN iteration procedure.
 *
 * @param ifname       Parent interface name
 * @param vlan_id      VID value
 * @param vlan_ifname  VLAN network interface name
 * @param user_data    Opaque data pointer passed as the value
 *                     of an argument to sun_iterate_vlans() function
 *
 * @return Directive to continue or to interrupt traveral
 * @retval 0 - Stop VLAN traversal
 * @retval 1 - Continue VLAN traversal
 */
typedef int (* sun_iterate_vlan_cb_f)(const char *ifname,
                                      int vlan_id,
                                      const char *vlan_ifname,
                                      void *user_data);

/**
 * Iterate VLANs of the particular interface
 *
 * @param ifname     network interface name whose VLANs to iterate
 * @param cb         callback function to use for each VLAN
 * @param user_data  opaque data pointer passed to @p cb function
 *
 * @return Status of the operation
 */
static te_errno
sun_iterate_vlans(const char *ifname,
                  sun_iterate_vlan_cb_f cb, void *user_data)
{
    te_errno  rc = 0;
    int       out_fd = -1;
    FILE     *out_fp;
    int       status;
    char      f_buf[200];
    pid_t     dladm_cmd_pid;

    /*
     * The name of VLAN interface can be an arbitrary string,
     * so we need to use 'dladm' to detect it.
     */
    dladm_cmd_pid = te_shell_cmd(
            "LANG=POSIX /usr/sbin/dladm show-vlan -p -o LINK,VID,OVER",
            -1, NULL, &out_fd, NULL);

    if (dladm_cmd_pid < 0)
    {
        ERROR("%s(): start of dladm failed", __FUNCTION__);
        return TE_RC(TE_TA_UNIX, TE_ESHCMD);
    }

    if ((out_fp = fdopen(out_fd, "r")) == NULL)
    {
        ERROR("Failed to obtain file pointer for shell command output");
        rc = TE_OS_RC(TE_TA_UNIX, te_rc_os2te(errno));
        goto cleanup;
    }

    while (fgets(f_buf, sizeof(f_buf), out_fp) != NULL)
    {
        size_t  ofs;
        char   *s = f_buf;
        char   *vlan_str;
        int     vlan_id;

        VERB("%s(): read line: <%s>", __FUNCTION__, f_buf);
        /* Find delimeters between LINK and VID fields */
        s = strchr(s, ':');
        if (s == NULL)
        {
            ERROR("%s() Unexpected format 'dladm' output: '%s'",
                  __FUNCTION__, f_buf);
            rc = TE_OS_RC(TE_TA_UNIX, TE_EINVAL);
            break;
        }
        *s++ = '\0';

        /* Find delimeters between VID and OVER fields */
        vlan_str = s;
        s = strchr(vlan_str, ':');
        if (s == NULL)
        {   
            ERROR("%s() Unexpected format 'dladm' output: '%s'", 
                  __FUNCTION__, f_buf);
            rc = TE_OS_RC(TE_TA_UNIX, TE_EINVAL);
            break;
        }
        *s++ = '\0';

        /* Check if VLAN is OVER specified network interface */
        ofs = strcspn(s," \n\r\t");
        s[ofs] = '\0';

        if (strcmp(s, ifname) != 0)
            continue;

        vlan_id = atoi(vlan_str);

        /* Call user callback and check if we need to continue */
        if (cb(ifname, vlan_id, f_buf, user_data) == 0)
            break;
    }

    /*
     * Read out all the command output, because otherwise we can have
     * program killed by SIGPIPE signal, which will cause ta_waitpid
     * return non-zero status.
     */
    while (fgets(f_buf, sizeof(f_buf), out_fp) != NULL)
        ;

cleanup:
    if (out_fp != NULL)
        fclose(out_fp);
    close(out_fd);

    ta_waitpid(dladm_cmd_pid, &status, 0);
    if (status != 0)
    {
        ERROR("%s(): Non-zero status of dladm: %d",
              __FUNCTION__, status);
        return TE_RC(TE_TA_UNIX, TE_ESHCMD);
    }

    return rc;
}

/** Structure to save information about VLAN IDs */
struct sun_vlan_list {
    size_t  vlans_size; /**< The size of @p vlans array */
    int    *vlans; /**< Array to fill in VID values */
    size_t  vlans_num; /**< Number of filled entries in @p vlans array */
    te_errno rc; /**< Processing result */
};

/** Callback function to register VID values. */
static int
sun_vlan_list(const char *ifname,
              int vlan_id, const char *vlan_ifname, void *user_data)
{
    struct sun_vlan_list *vlan_info = (struct sun_vlan_list *)user_data;

    UNUSED(vlan_ifname);

    if (vlan_info->vlans_size <= vlan_info->vlans_num)
    {
        ERROR("Too many VLANs for %s interface", ifname);
        vlan_info->rc = TE_OS_RC(TE_TA_UNIX, TE_ENOSPC);
        return 0; /* Interrupt VLAN traversal */
    }

    vlan_info->vlans[vlan_info->vlans_num] = vlan_id;
    vlan_info->vlans_num++;

    return 1; /* Continue VLAN traversal */
}

/** Structure to save VLAN interface name */
struct sun_vlan_name {
    int vlan_id; /**< VLAN ID whose name to get */
    size_t vlan_ifname_size; /**< The size of @p vlan_ifname array */
    char *vlan_ifname; /**< Array to fill in with interface name */
};

/** Callback function to get VLAN interface name */
static int
sun_vlan_find_name(const char *ifname,
                   int vlan_id, const char *vlan_ifname, void *user_data)
{
    struct sun_vlan_name *vlan_name = (struct sun_vlan_name *)user_data;

    UNUSED(ifname);

    if (vlan_name->vlan_id == vlan_id)
    {
        snprintf(vlan_name->vlan_ifname, vlan_name->vlan_ifname_size,
                 "%s", vlan_ifname);
        return 0; /* Interrupt VLAN traversal */
    }
    return 1; /* Continue VLAN traversal */
}
#endif /* __sun__ */



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
    size_t   n_vlans_size;
    te_errno rc = 0;

    if (devname == NULL ||n_vlans == NULL || vlans == NULL)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    n_vlans_size = *n_vlans;

    VERB("%s(): enter for device: <%s>", __FUNCTION__, devname);
    *n_vlans = 0;
#if defined __linux__
    {
        FILE *proc_vlans = fopen("/proc/net/vlan/config", "r");
        int   vlan_id;
        char  f_buf[200];

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

            if (n_vlans_size <= *n_vlans)
            {
                ERROR("Too many VLANs for %s interface", devname);
                rc = TE_OS_RC(TE_TA_UNIX, TE_ENOSPC);
                break;
            }

            if (strcmp(s, devname) == 0)
                vlans[(*n_vlans)++] = vlan_id;
        }
        (void)fclose(proc_vlans);
    }
#elif defined __sun__
    {
        struct sun_vlan_list vlan_info = { n_vlans_size, vlans, 0, 0 };

        rc = sun_iterate_vlans(devname, sun_vlan_list, &vlan_info);
        if (rc == 0 && vlan_info.rc != 0)
            rc = vlan_info.rc;
        else
            *n_vlans = vlan_info.vlans_num;
    }
#endif

    return rc;
}

/**
 * Get VLAN ifname
 *
 * @param devname       name of network device
 * @param vlan_id       VLAN id
 * @param v_ifname      location for VLAN ifname
 *                      (for Solaris port this field will be set to
 *                      an empty string in case there is no such VLAN
 *                      interface configured in the system)
 *
 * @return status
 */
static te_errno
vlan_ifname_get_internal(const char *ifname, int vlan_id,
                         char *v_ifname)
{
    te_errno rc = 0;

#if defined __linux__
    sprintf(v_ifname, "%s.%d", ifname, vlan_id);
#elif defined __sun__
    {
        struct sun_vlan_name vlan_name = { vlan_id, IF_NAMESIZE, v_ifname };

        v_ifname[0] = '\0';
        rc = sun_iterate_vlans(ifname, sun_vlan_find_name, &vlan_name);
    }
#else
    ERROR("%s() Not supported", __FUNCTION__);
    rc = TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP);
#endif
    return rc;
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

    VERB("VLAN list: '%s'", *list);
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
    te_errno rc;
    int      vid = atoi(vid_str);

    UNUSED(value);

    VERB("%s: gid=%u oid='%s', vid %s, ifname %s",
         __FUNCTION__, gid, oid, vid_str, ifname);

    if ((rc = CHECK_INTERFACE(ifname)) != 0)
        return TE_RC(TE_TA_UNIX, rc);

#if LINUX_VLAN_SUPPORT
    {
        struct vlan_ioctl_args  if_request;
        struct ifreq            ifr;
        te_bool                 try_restore_ip_addr = TRUE;

        if (cfg_socket < 0)
        {
            ERROR("%s: non-init cfg socket", cfg_socket);
            return TE_RC(TE_TA_UNIX, TE_EFAULT);
        }

        /*
         * On old CentOS kernels existing IP address
         * is removed from parent interface when VLAN
         * is created - so we try to save it and restore
         * after creating VLAN.
         */
        ifr.ifr_addr.sa_family = AF_INET;
        strncpy(ifr.ifr_name, ifname, IFNAMSIZ - 1); 
        if (ioctl(cfg_socket, SIOCGIFADDR, &ifr) != 0)
            try_restore_ip_addr = FALSE;

        if_request.cmd = ADD_VLAN_CMD;
        strcpy(if_request.device1, ifname);
        if_request.u.VID = vid;

        /*
         * Creating VLAN.
         */
        if (ioctl(cfg_socket, SIOCSIFVLAN, &if_request) < 0)
            rc = te_rc_os2te(errno);

        /*
         * Restoring IP address on parent interface.
         */
        if (try_restore_ip_addr &&
            ((struct sockaddr_in *)
                        &ifr.ifr_addr)->sin_addr.s_addr != 0)
        {
            struct ifreq    ifr_aux;
            int             rc_aux;

            /*
             * IP address disappears on parent interface not
             * instantly.
             */
            usleep(500000);

            memcpy(&ifr_aux, &ifr, sizeof(ifr));
            if (ioctl(cfg_socket, SIOCGIFADDR, &ifr_aux) != 0 ||
                ((struct sockaddr_in *)
                        &ifr_aux.ifr_addr)->sin_addr.s_addr !=
                ((struct sockaddr_in *)
                        &ifr.ifr_addr)->sin_addr.s_addr)
            {
                rc_aux = ioctl(cfg_socket, SIOCSIFADDR, &ifr);
                if (rc_aux == 0)
                    RING("IP address %s was restored on "
                         "parent interface %s",
                         inet_ntoa(((struct sockaddr_in *)
                                      &ifr.ifr_addr)->sin_addr),
                         ifname);
                else
                    ERROR("Failed to restore IP address on "
                          "parent interface: %s",
                          strerror(errno));
            }
        }
#if 0
    {
        char vlan_if_name[IFNAMSIZ];

        vlan_ifname_get_internal(ifname, vid, vlan_if_name);

        sprintf(buf, "ifconfig %s up > /dev/null",
                vlan_if_name);

        if (ta_system(buf) != 0)
            return TE_RC(TE_TA_UNIX, TE_ESHCMD);
    }
#endif
        return TE_RC(TE_TA_UNIX, rc);
    }
#elif defined __sun__
    {
        char vlan_if_name[IFNAMSIZ];
        
        rc = vlan_ifname_get_internal(ifname, vid, vlan_if_name);
        if (rc != 0)
            return rc;

        /* Check if there is no VLAN with the same VID in the system */
        if (vlan_if_name[0] != '\0')
            return TE_RC(TE_TA_UNIX, TE_EEXIST);

        snprintf(buf, sizeof(buf),
                 "LANG=POSIX /usr/sbin/dladm create-vlan -l %s -v %d",
                 ifname, vid);
        if (ta_system(buf) != 0)
            return TE_RC(TE_TA_UNIX, TE_ESHCMD);

        /* Get VLAN interface name assigned by the system */
        rc = vlan_ifname_get_internal(ifname, vid, vlan_if_name);
        if (rc != 0)
            return rc;
        if (vlan_if_name[0] == '\0')
        {
            ERROR("Unexpected error happened while adding VLAN interface "
                  "OVER '%s' with VID '%d'", ifname, vid);
            return TE_RC(TE_TA_UNIX, TE_EFAULT);
        }

        /* Now we need to create a network interface associated with VLAN */
        snprintf(buf, sizeof(buf),
                 "LANG=POSIX /usr/sbin/ipadm create-ip %s", vlan_if_name);
        if (ta_system(buf) != 0)
        {
            ERROR("Failed to create a network interface associated with "
                  "VLAN interface '%s'", vlan_if_name);
            
            snprintf(buf, sizeof(buf),
                     "LANG=POSIX /usr/sbin/dladm delete-vlan %s",
                     vlan_if_name);
            ta_system(buf);

            return TE_RC(TE_TA_UNIX, TE_ESHCMD);
        }

        RING("VLAN interface '%s' added: VID '%d' OVER '%s'",
             vlan_if_name, vid, ifname);

        return 0;
    }
#else
    ERROR("This test agent does not support VLANs");
    return TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP);
#endif
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
    te_errno rc;
    int      vid = atoi(vid_str);

    UNUSED(gid);
    UNUSED(oid);

    if ((rc = CHECK_INTERFACE(ifname)) != 0)
        return TE_RC(TE_TA_UNIX, rc);

#if LINUX_VLAN_SUPPORT
    {
        struct vlan_ioctl_args if_request;

        if (cfg_socket < 0)
        {
            ERROR("%s: non-init cfg socket", cfg_socket);
            return TE_RC(TE_TA_UNIX, TE_EFAULT);
        }
        if_request.cmd = DEL_VLAN_CMD;
        vlan_ifname_get_internal(ifname, vid, if_request.device1);
        if_request.u.VID = vid;

        if (ioctl(cfg_socket, SIOCSIFVLAN, &if_request) < 0)
            rc = te_rc_os2te(errno);
    
        return TE_RC(TE_TA_UNIX, rc);
    }
#elif defined __sun__
    {
        char vlan_if_name[IFNAMSIZ];

        /*
         * Check if VLAN with specific VID and
         * OVER specified interface exists.
         */
        rc = vlan_ifname_get_internal(ifname, vid, vlan_if_name);
        if (rc != 0)
            return rc;

        if (vlan_if_name[0] == '\0')
        {
            ERROR("Can't find VLAN OVER '%s' with VID '%d'",
                  ifname, vid);
            return TE_RC(TE_TA_UNIX, TE_ENOENT);
        }

        /* First we need to delete an interface */
        snprintf(buf, sizeof(buf),
                "LANG=POSIX /usr/sbin/ipadm delete-ip %s", vlan_if_name);
        if (ta_system(buf) != 0)
            WARN("Failed to delete network interface '%s'", vlan_if_name);

        /* Now delete VLAN link */
        snprintf(buf, sizeof(buf),
                 "LANG=POSIX /usr/sbin/dladm delete-vlan %s", vlan_if_name);
        if (ta_system(buf) != 0)
        {
            rc = TE_ESHCMD;
            ERROR("Failed to delete VLAN link '%s'", vlan_if_name);
        }
        else
            RING("VLAN interface '%s' deleted: VID '%d' OVER '%s'",
                 vlan_if_name, vid, ifname);

        return TE_RC(TE_TA_UNIX, rc);
    }
#else
    ERROR("This test agent does not support VLANs");
    return TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP);
#endif
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
    te_errno rc = 0;
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

#ifdef USE_LIBNETCONF
#define AF_INET_DEFAULT_BYTELEN  (sizeof(struct in_addr))
#define AF_INET_DEFAULT_BITLEN   (AF_INET_DEFAULT_BYTELEN << 3)
#define AF_INET6_DEFAULT_BYTELEN (sizeof(struct in6_addr))
#define AF_INET6_DEFAULT_BITLEN  (AF_INET6_DEFAULT_BYTELEN << 3)

static te_errno
net_addr_add(unsigned int gid, const char *oid, const char *value,
             const char *ifname, const char *addr)
{
    unsigned int    prefix;
    char           *end;
    in_addr_t       mask;
    gen_ip_address  broadcast;

    sa_family_t     family;
    gen_ip_address  ip_addr;
    struct in6_addr zero_ip6_addr;
    te_errno        rc;

    UNUSED(gid);
    UNUSED(oid);

    if ((rc = CHECK_INTERFACE(ifname)) != 0)
        return TE_RC(TE_TA_UNIX, rc);

    family = str_addr_family(addr);

    memset(&zero_ip6_addr, 0, sizeof(zero_ip6_addr));

    /* Validate address to be added */
    if (inet_pton(family, addr, &ip_addr) <= 0 ||
        (family == AF_INET && ip_addr.ip4_addr.s_addr == 0) ||
        (family == AF_INET6 && memcmp(&ip_addr.ip6_addr,
                                      &zero_ip6_addr,
                                      sizeof(zero_ip6_addr)) == 0) ||
        (family == AF_INET &&
         (IN_CLASSD(ntohl(ip_addr.ip4_addr.s_addr)) ||
          IN_EXPERIMENTAL(ntohl(ip_addr.ip4_addr.s_addr)))))
    {
        ERROR("%s(): Trying to add incorrect address %s",
              __FUNCTION__, addr);
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

    {
        unsigned int      ifindex;
        unsigned int      addrlen;
        netconf_list     *list;
        netconf_node     *t;
        netconf_net_addr  net_addr;

        if ((ifindex = if_nametoindex(ifname)) == 0)
        {
            ERROR("%s(): Device '%s' does not exist",
                  __FUNCTION__, ifname);
            return TE_RC(TE_TA_UNIX, TE_ENODEV);
        }

        addrlen = (family == AF_INET) ?
                  sizeof(struct in_addr) : sizeof(struct in6_addr);

        /*
         * Check that address has not been assigned to any
         * interface yet.
         */

        list = netconf_net_addr_dump_iface(nh, (unsigned char)family,
                                           ifindex);
        if (list == NULL)
        {
            ERROR("%s(): Cannot get list of addresses", __FUNCTION__);
            return TE_OS_RC(TE_TA_UNIX, errno);
        }

        for (t = list->head; t != NULL; t = t->next)
        {
            const netconf_net_addr *naddr = &(t->data.net_addr);

            if (memcmp(&ip_addr, naddr->address, addrlen) == 0)
            {
                static char tmp[IF_NAMESIZE];

                if (if_indextoname(naddr->ifindex, tmp) == NULL)
                    strcpy(tmp, "unknown");
                /**
                 * Exit without error if the address exists on needed
                 * interface.
                 */
                netconf_list_free(list);
                VERB("%s(): Address '%s' already exists "
                     "on interface '%s'", __FUNCTION__, addr, tmp);
                return 0;
            }
        }

        netconf_list_free(list);

        netconf_net_addr_init(&net_addr);
        net_addr.family = family;
        net_addr.prefix = prefix;
        net_addr.ifindex = ifindex;
        net_addr.address = (uint8_t *)&ip_addr;
        net_addr.broadcast = (uint8_t *)&broadcast;

        if (netconf_net_addr_modify(nh, NETCONF_CMD_ADD, &net_addr) < 0)
        {
            ERROR("%s(): Cannot add address '%s' on interface '%s'",
                  __FUNCTION__, addr, ifname);
            return TE_OS_RC(TE_TA_UNIX, errno);
        }

        return 0;
    }
}
#endif /* USE_LIBNETCONF */

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

#if defined(USE_LIBNETCONF)
    {
        sa_family_t             family;
        unsigned int            addrlen;
        unsigned int            ifindex;
        netconf_net_addr        net_addr;
        gen_ip_address          ip_addr;
        netconf_list           *list;
        netconf_node           *t;
        te_bool                 found;
        unsigned char           prefix = 0;

        family = str_addr_family(addr);
        if ((ifindex = if_nametoindex(ifname)) == 0)
        {
            ERROR("%s(): Device '%s' does not exist",
                  __FUNCTION__, ifname);
            return TE_RC(TE_TA_UNIX, TE_ENODEV);
        }

        addrlen = (family == AF_INET) ?
                  sizeof(struct in_addr) : sizeof(struct in6_addr);

        if (inet_pton(family, addr, &ip_addr) <= 0)
        {
            ERROR("Failed to convert address '%s' from string", addr);
            return TE_RC(TE_TA_UNIX, TE_EINVAL);
        }

        /* Check address existence */
        if ((list = netconf_net_addr_dump_iface(nh,
                                                (unsigned char)family,
                                                ifindex)) == NULL)
        {
            ERROR("%s(): Cannot get list of addresses", __FUNCTION__);
            return TE_OS_RC(TE_TA_UNIX, errno);
        }

        found = FALSE;
        for (t = list->head; t != NULL; t = t->next)
        {
            const netconf_net_addr *naddr = &(t->data.net_addr);

            if (memcmp(&ip_addr, naddr->address, addrlen) == 0)
            {
                found = TRUE;
                prefix = naddr->prefix;
                break;
            }
        }

        netconf_list_free(list);

        if (!found)
        {
            ERROR("Address '%s' on interface '%s' not found",
                  addr, ifname);
            return TE_RC(TE_TA_UNIX, TE_ENOENT);
        }

        netconf_net_addr_init(&net_addr);
        net_addr.family = family;
        net_addr.prefix = prefix;
        net_addr.ifindex = ifindex;
        net_addr.address = (uint8_t *)&ip_addr;

        if (netconf_net_addr_modify(nh, NETCONF_CMD_DEL, &net_addr) < 0)
        {
            ERROR("%s(): Cannot delete address '%s' from "
                  "interface '%s'", __FUNCTION__, addr, ifname);
            return TE_OS_RC(TE_TA_UNIX, errno);
        }

        return 0;
    }
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

#ifdef USE_LIBNETCONF
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
static te_errno
net_addr_list(unsigned int gid, const char *oid, char **list,
              const char *ifname)
{
    te_errno            rc;
    unsigned int        ifindex;
    netconf_list       *nlist;
    netconf_node       *t;
    unsigned int        len;
    char               *cur_ptr;

    UNUSED(gid);
    UNUSED(oid);

    if (list == NULL)
    {
        ERROR("%s(): Invalid value for 'list' argument", __FUNCTION__);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    if ((rc = CHECK_INTERFACE(ifname)) != 0)
    {
        ERROR("%s(): Bad device name '%s'", __FUNCTION__, ifname);
        return TE_RC(TE_TA_UNIX, rc);
    }

    if ((ifindex = if_nametoindex(ifname)) == 0)
    {
        ERROR("%s(): Device '%s' does not exist", __FUNCTION__, ifname);
        return TE_RC(TE_TA_UNIX, TE_ENODEV);
    }

    /* Get addresses of both families, IPv4 and IPv6 */
    if ((nlist = netconf_net_addr_dump_iface(nh, AF_UNSPEC,
                                            ifindex)) == NULL)
    {
        ERROR("%s(): Cannot get list of addresses", __FUNCTION__);
        return TE_OS_RC(TE_TA_UNIX, errno);
    }

    /* Calculate maximum space needed by list */
    len = nlist->length * (INET6_ADDRSTRLEN + 1);

    if (len == 0)
    {
        *list = NULL;
        return 0;
    }

    if ((*list = malloc(len)) == NULL)
    {
        netconf_list_free(nlist);
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }

    memset(*list, 0, len);

    cur_ptr = *list;
    for (t = nlist->head; t != NULL; t = t->next)
    {
        const netconf_net_addr *net_addr = &(t->data.net_addr);

        assert(cur_ptr >= *list);
        assert((unsigned int)(cur_ptr - *list) <= len);

        if (cur_ptr != *list)
        {
            snprintf(cur_ptr, len - (cur_ptr - *list), " ");
            cur_ptr += strlen(cur_ptr);
        }

        if (inet_ntop(net_addr->family, net_addr->address, cur_ptr,
                      *list + len - cur_ptr) == NULL)
        {
            ERROR("%s(): Cannot save network address", __FUNCTION__);
            free(*list);
            netconf_list_free(nlist);
            return TE_RC(TE_TA_UNIX, TE_EINVAL);
        }

        cur_ptr += strlen(cur_ptr);
    }

    netconf_list_free(nlist);

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

#if defined(USE_LIBNETCONF)
    {
        te_errno                rc;
        sa_family_t             family;
        unsigned int            addrlen;
        unsigned int            ifindex;
        gen_ip_address          ip_addr;
        netconf_list           *list;
        netconf_node           *t;
        te_bool                 found;

        if ((rc = CHECK_INTERFACE(ifname)) != 0)
        {
            ERROR("%s(): Bad device name '%s'", __FUNCTION__, ifname);
            return TE_RC(TE_TA_UNIX, rc);
        }

        family = str_addr_family(addr);

        if ((ifindex = if_nametoindex(ifname)) == 0)
        {
            ERROR("%s(): Device '%s' does not exist",
                  __FUNCTION__, ifname);
            return TE_RC(TE_TA_UNIX, TE_ENODEV);
        }

        addrlen = (family == AF_INET) ?
                  sizeof(struct in_addr) : sizeof(struct in6_addr);

        if (inet_pton(family, addr, &ip_addr) <= 0)
        {
            ERROR("Failed to covnert address '%s' from string", addr);
            return TE_RC(TE_TA_UNIX, TE_EINVAL);
        }

        if ((list = netconf_net_addr_dump_iface(nh,
                                                (unsigned char)family,
                                                ifindex)) == NULL)
        {
            ERROR("%s(): Cannot get list of addresses", __FUNCTION__);
            return TE_OS_RC(TE_TA_UNIX, errno);
        }

        found = FALSE;
        for (t = list->head; t != NULL; t = t->next)
        {
            const netconf_net_addr *net_addr = &(t->data.net_addr);

            if (memcmp(&ip_addr, net_addr->address, addrlen) == 0)
            {
                found = TRUE;
                prefix = net_addr->prefix;
                break;
            }
        }

        netconf_list_free(list);

        if (!found)
        {
            ERROR("Address '%s' on interface '%s' to get prefix "
                  "not found", addr, ifname);
            return TE_RC(TE_TA_UNIX, TE_ENOENT);
        }
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

#if defined(USE_LIBNETCONF)
    {
        te_errno                rc;
        sa_family_t             family;
        unsigned int            addrlen;
        unsigned int            ifindex;
        netconf_net_addr        net_addr;
        gen_ip_address          ip_addr;
        netconf_list           *list;
        netconf_node           *t;
        unsigned char           oldprefix = 0;
        te_bool                 found;

        if ((rc = CHECK_INTERFACE(ifname)) != 0)
        {
            ERROR("%s(): Bad device name '%s'", __FUNCTION__, ifname);
            return TE_RC(TE_TA_UNIX, rc);
        }

        family = str_addr_family(addr);

        if ((ifindex = if_nametoindex(ifname)) == 0)
        {
            ERROR("%s(): Device '%s' does not exist",
                  __FUNCTION__, ifname);
            return TE_RC(TE_TA_UNIX, TE_ENODEV);
        }

        addrlen = (family == AF_INET) ?
                  sizeof(struct in_addr) : sizeof(struct in6_addr);

        if (inet_pton(family, addr, &ip_addr) <= 0)
        {
            ERROR("Failed to convert address '%s' from string", addr);
            return TE_RC(TE_TA_UNIX, TE_EINVAL);
        }

        if ((list = netconf_net_addr_dump_iface(nh,
                                                (unsigned char)family,
                                                ifindex)) == NULL)
        {
            ERROR("%s(): Cannot get list of addresses", __FUNCTION__);
            return TE_OS_RC(TE_TA_UNIX, errno);
        }

        found = FALSE;
        for (t = list->head; t != NULL; t = t->next)
        {
            const netconf_net_addr *naddr = &(t->data.net_addr);

            if (memcmp(&ip_addr, naddr->address, addrlen) == 0)
            {
                found = TRUE;
                oldprefix = naddr->prefix;
                break;
            }
        }

        netconf_list_free(list);

        if (!found)
        {
            ERROR("Address '%s' on interface '%s' to set prefix "
                  "not found", addr, ifname);
            return TE_RC(TE_TA_UNIX, TE_ENOENT);
        }

        netconf_net_addr_init(&net_addr);
        net_addr.family = family;
        net_addr.prefix = oldprefix;
        net_addr.ifindex = ifindex;
        net_addr.address = (uint8_t *)&ip_addr;

        if (netconf_net_addr_modify(nh, NETCONF_CMD_DEL,
                                    &net_addr) < 0)
        {
            ERROR("%s(): Cannot delete address '%s' from "
                  "interface '%s'", __FUNCTION__, addr, ifname);
            return TE_OS_RC(TE_TA_UNIX, errno);
        }

        net_addr.prefix = prefix;

        if (netconf_net_addr_modify(nh, NETCONF_CMD_ADD,
                                    &net_addr) < 0)
        {
            ERROR("%s(): Cannot add address '%s' to interface '%s'",
                  __FUNCTION__, addr, ifname);
            return TE_OS_RC(TE_TA_UNIX, errno);
        }

        return 0;
    }
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

#if defined(USE_LIBNETCONF)
    {
        te_errno                rc;
        unsigned int            ifindex;
        gen_ip_address          ip_addr;
        netconf_list           *list;
        netconf_node           *t;
        te_bool                 found;

        if ((rc = CHECK_INTERFACE(ifname)) != 0)
        {
            ERROR("%s(): Bad device name '%s'", __FUNCTION__, ifname);
            return TE_RC(TE_TA_UNIX, rc);
        }

        if ((ifindex = if_nametoindex(ifname)) == 0)
        {
            ERROR("%s(): Device '%s' does not exist",
                  __FUNCTION__, ifname);
            return TE_RC(TE_TA_UNIX, TE_ENODEV);
        }

        if (inet_pton(AF_INET, addr, &ip_addr) <= 0)
        {
            ERROR("Failed to convert address '%s' from string", addr);
            return TE_RC(TE_TA_UNIX, TE_EINVAL);
        }

        if ((list = netconf_net_addr_dump_iface(nh, AF_INET,
                                                ifindex)) == NULL)
        {
            ERROR("%s(): Cannot get list of addresses", __FUNCTION__);
            return TE_OS_RC(TE_TA_UNIX, errno);
        }

        found = FALSE;
        for (t = list->head; t != NULL; t = t->next)
        {
            const netconf_net_addr *net_addr = &(t->data.net_addr);

            if (memcmp(&ip_addr, net_addr->address,
                       sizeof(struct in_addr)) == 0)
            {
                found = TRUE;

                if (net_addr->broadcast != NULL)
                {
                    memcpy(&bcast.ip4_addr.s_addr, net_addr->broadcast,
                           sizeof(struct in_addr));
                }
                else
                {
                    bcast.ip4_addr.s_addr = htonl(INADDR_BROADCAST);
                }

                break;
            }
        }

        netconf_list_free(list);

        if (!found)
        {
            ERROR("Address '%s' on interface '%s' to get broadcast "
                  "address not found", addr, ifname);
            return TE_RC(TE_TA_UNIX, TE_ENOENT);
        }
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
        ((family == AF_INET) &&
         ((bcast.ip4_addr.s_addr == 0) ||
          (((ntohl(bcast.ip4_addr.s_addr) & 0xe0000000) == 0xe0000000) &&
           (ntohl(bcast.ip4_addr.s_addr) != 0xffffffff)))))
    {
        ERROR("%s(): Invalid broadcast %s", __FUNCTION__, value);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

#if defined(USE_LIBNETCONF)
    {
        te_errno                rc;
        unsigned int            ifindex;
        netconf_net_addr        net_addr;
        gen_ip_address          ip_addr;
        netconf_list           *list;
        netconf_node           *t;
        te_bool                 found;
        unsigned char           prefix = 0;

        if ((rc = CHECK_INTERFACE(ifname)) != 0)
        {
            ERROR("%s(): Bad device name '%s'", __FUNCTION__, ifname);
            return TE_RC(TE_TA_UNIX, rc);
        }

        if ((ifindex = if_nametoindex(ifname)) == 0)
        {
            ERROR("%s(): Device '%s' does not exist",
                  __FUNCTION__, ifname);
            return TE_RC(TE_TA_UNIX, TE_ENODEV);
        }

        if (inet_pton(AF_INET, addr, &ip_addr) <= 0)
        {
            ERROR("Failed to convert address '%s' from string", addr);
            return TE_RC(TE_TA_UNIX, TE_EINVAL);
        }

        if ((list = netconf_net_addr_dump_iface(nh,
                                                AF_INET,
                                                ifindex)) == NULL)
        {
            ERROR("%s(): Cannot get list of addresses", __FUNCTION__);
            return TE_OS_RC(TE_TA_UNIX, errno);
        }

        found = FALSE;
        for (t = list->head; t != NULL; t = t->next)
        {
            const netconf_net_addr *naddr = &(t->data.net_addr);

            if (memcmp(&ip_addr, naddr->address,
                       sizeof(struct in_addr)) == 0)
            {
                found = TRUE;
                prefix = naddr->prefix;
                break;
            }
        }

        netconf_list_free(list);

        if (!found)
        {
            ERROR("Address '%s' on interface '%s' to set broadcast "
                  "not found", addr, ifname);
            return TE_RC(TE_TA_UNIX, TE_ENOENT);
        }

        netconf_net_addr_init(&net_addr);
        net_addr.family = AF_INET;
        net_addr.prefix = prefix;
        net_addr.ifindex = ifindex;
        net_addr.address = (uint8_t *)&ip_addr;

        if (netconf_net_addr_modify(nh, NETCONF_CMD_DEL,
                                    &net_addr) < 0)
        {
            ERROR("%s(): Cannot delete address '%s' from "
                  "interface '%s'",
                  __FUNCTION__, addr, ifname);
            return TE_OS_RC(TE_TA_UNIX, errno);
        }

        net_addr.broadcast = (uint8_t *)&bcast;

        if (netconf_net_addr_modify(nh, NETCONF_CMD_ADD,
                                    &net_addr) < 0)
        {
            ERROR("%s(): Cannot add address '%s' to interface '%s'",
                  __FUNCTION__, addr, ifname);
            return TE_OS_RC(TE_TA_UNIX, errno);
        }

        return 0;
    }
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


#if ((defined(USE_LIBNETCONF)) || (defined(HAVE_SYS_DLPI_H)))
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

int
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

#ifdef MY_SIOCGIFHWADDR
    memset(&req, 0, sizeof(req));
    strcpy(req.my_ifr_name, ifname);

    if (ioctl(cfg_socket, MY_SIOCGIFHWADDR, (caddr_t)&req) != 0)
    {
        static const uint8_t zero_mac[ETHER_ADDR_LEN] = {};

        rc = TE_OS_RC(TE_TA_UNIX, errno);

        /*
         * For the case of loopback interface SIOCGIFHWADDR
         * can return an error with errno code set to EADDRNOTAVAIL.
         * There is nothing wrong here and we need to check that
         * we really deal with loopback interface.
         */
        if (errno != EADDRNOTAVAIL)
        {
            ERROR("line %u: ioctl(MY_SIOCGIFHWADDR) failed: %r",
                  __LINE__, rc);
            return rc;
        }
        CFG_IOCTL(cfg_socket, MY_SIOCGIFFLAGS, &req);
        if (!(req.my_ifr_flags & IFF_LOOPBACK))
        {
            ERROR("line %u: ioctl(MY_SIOCGIFHWADDR) failed: %r "
                  "for non loopback interface", __LINE__, rc);
            return rc;
        }
        ptr = zero_mac;
    }
    else
        ptr = (const uint8_t *)my_ifr_hwaddr_data(req);

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
    uint8_t  link_addr[ETHER_ADDR_LEN];

    UNUSED(gid);
    UNUSED(oid);

    if ((rc = CHECK_INTERFACE(ifname)) != 0)
        return TE_RC(TE_TA_UNIX, rc);

    if (value == NULL)
    {
       ERROR("A link layer address to set is not provided");
       return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    if (link_addr_a2n(link_addr, sizeof(link_addr), value) == -1)
    {
        ERROR("%s: Link layer address conversation issue", __FUNCTION__);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

#ifdef SIOCSIFHWADDR
    strcpy(req.my_ifr_name, ifname);
    my_ifr_hwaddr_family(req) = AF_LOCAL;
    memcpy(my_ifr_hwaddr_data(req), link_addr, sizeof(link_addr));

    CFG_IOCTL(cfg_socket, SIOCSIFHWADDR, &req);
#elif HAVE_SYS_DLPI_H
    rc = ta_unix_conf_dlpi_phys_addr_set(ifname,
                                         link_addr, sizeof(link_addr));
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
    my_ifr_hwaddr_family(req) = AF_LOCAL;

    if ((rc = link_addr_a2n((uint8_t *)my_ifr_hwaddr_data(req), 6,
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

    /*
     * In case of point-to-point protocol there is no broadcast
     * hardware address, return zero-address.
     */
    if (strstr(ifname, "ppp") != NULL)
    {
        strcpy(value, "00:00:00:00:00:00");
        return 0;
    }

#if defined(USE_LIBNETCONF)
    {
        unsigned int            ifindex;
        netconf_list           *list;
        netconf_node           *t;
        te_bool                 found;

        if ((ifindex = if_nametoindex(ifname)) == 0)
        {
            ERROR("%s(): Device '%s' does not exist",
                  __FUNCTION__, ifname);
            return TE_RC(TE_TA_UNIX, TE_ENODEV);
        }

        if ((list = netconf_link_dump(nh)) == NULL)
        {
            ERROR("%s(): Cannot get list of interfaces", __FUNCTION__);
            return TE_OS_RC(TE_TA_UNIX, errno);
        }

        found = FALSE;
        for (t = list->head; t != NULL; t = t->next)
        {
            const netconf_link *link = &(t->data.link);

            if (ifindex == (unsigned int)(link->ifindex))
            {
                link_addr_n2a(link->broadcast, link->addrlen,
                              value, RCF_MAX_VAL);
                found = TRUE;
                break;
            }
        }

        netconf_list_free(list);

        if (!found)
        {
            ERROR("Cannot find interface '%s'", ifname);
            return TE_RC(TE_TA_UNIX, TE_ENOENT);
        }

        return 0;
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
 * Change MTU for the specified interface.
 * 
 * @param ifname  Interface name
 * @param mtu     MTU value
 * 
 * @return Error code.
 */
static te_errno
change_mtu(const char *ifname, int mtu)
{
    te_errno  rc = 0;
    te_bool   status;

    req.my_ifr_mtu = mtu;
    strcpy(req.my_ifr_name, ifname);
    if (ioctl(cfg_socket, MY_SIOCSIFMTU, (intptr_t)&req) != 0)
    {
        rc = TE_OS_RC(TE_TA_UNIX, errno);
        if (errno != EBUSY)
        {
            ERROR("Failed to change MTU to %d on interface %s: %r",
                  mtu, ifname, TE_OS_RC(TE_TA_UNIX, errno));
            return rc;
        }

        /* Try to down interface */
        if (ta_interface_status_get(ifname, &status) == 0 &&
            status && ta_interface_status_set(ifname, FALSE) == 0)
        {
            te_errno  rc1;

            RING("Interface '%s' is pushed down/up to set a new MTU",
                 ifname);

            if (ioctl(cfg_socket, MY_SIOCSIFMTU, (intptr_t)&req) == 0)
                rc = 0;
            else
                ERROR("Failed to change MTU to %d on interface %s: %r",
                      mtu, ifname, TE_OS_RC(TE_TA_UNIX, errno));

            if ((rc1 = ta_interface_status_set(ifname, TRUE)) != 0)
            {
                ERROR("Failed to up interface after mtu changing "
                      "error %r", rc1);
                return rc1;
            }
        }
    }

    return rc;
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
    te_errno  rc = 0;

    UNUSED(gid);
    UNUSED(oid);

    if ((rc = CHECK_INTERFACE(ifname)) != 0)
        return TE_RC(TE_TA_UNIX, rc);

#if (defined(SIOCGIFMTU)  && defined(HAVE_STRUCT_IFREQ_IFR_MTU))   || \
    (defined(SIOCGLIFMTU) && defined(HAVE_STRUCT_LIFREQ_LIFR_MTU))
    rc = change_mtu(ifname, strtol(value, NULL, 10));
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
 * Get IP4 forwarding state of the interface.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         value location
 * @param ifname        name of the interface (like "eth0")
 *
 * @return              Status code
 */
static te_errno
iface_ip4_fw_get(unsigned int gid, const char *oid, char *value,
                 const char *ifname)
{
#if __linux__
    char    c = '0';
    int     fd;
    char    filename[128];
#endif

    UNUSED(gid);
    UNUSED(oid);

#if __linux__
    sprintf(filename, "/proc/sys/net/ipv4/conf/%s/forwarding", ifname);
    if ((fd = open(filename, O_RDONLY)) < 0)
        return TE_OS_RC(TE_TA_UNIX, errno);

    if (read(fd, &c, 1) < 0)
    {
        close(fd);
        return TE_OS_RC(TE_TA_UNIX, errno);
    }
    close(fd);

    sprintf(value, "%d", c == '0' ? 0 : 1);
#else
    /* FIXME Add implementation in SOLARIS and(or) BSD if necessary */
    sprintf(value, "%d", 0);
#endif

    return 0;
}

/**
 * Change IP4 forwarding state of the interface.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         new value pointer
 * @param ifname        name of the interface (like "eth0")
 *
 * @return              Status code
 */
static te_errno
iface_ip4_fw_set(unsigned int gid, const char *oid, const char *value,
                 const char *ifname)
{
#if __linux__
    int     fd;
    char    filename[128];
#endif

    UNUSED(gid);
    UNUSED(oid);

#if __linux__
    if ((*value != '0' && *value != '1') || *(value + 1) != 0)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    sprintf(filename, "/proc/sys/net/ipv4/conf/%s/forwarding", ifname);
    if ((fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0666)) < 0)
        return TE_OS_RC(TE_TA_UNIX, errno);

    if (write(fd, *value == '0' ? "0\n" : "1\n", 2) < 0)
    {
        close(fd);
        return TE_OS_RC(TE_TA_UNIX, errno);
    }

    close(fd);
#else
    /* FIXME Add implementation in SOLARIS and(or) BSD if necessary */
    return TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif

    return 0;
}

/**
 * Get IP6 forwarding state of the interface.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         value location
 * @param ifname        name of the interface (like "eth0")
 *
 * @return              Status code
 */
static te_errno
iface_ip6_fw_get(unsigned int gid, const char *oid, char *value,
                 const char *ifname)
{
#if __linux__
    char    c = '0';
    int     fd;
    char    filename[128];
#endif

    UNUSED(gid);
    UNUSED(oid);

#if __linux__
    sprintf(filename, "/proc/sys/net/ipv6/conf/%s/forwarding", ifname);
    if ((fd = open(filename, O_RDONLY)) < 0)
        return TE_OS_RC(TE_TA_UNIX, errno);

    if (read(fd, &c, 1) < 0)
    {
        close(fd);
        return TE_OS_RC(TE_TA_UNIX, errno);
    }
    close(fd);

    sprintf(value, "%d", c == '0' ? 0 : 1);
#else
    /* FIXME Add implementation in SOLARIS and(or) BSD if necessary */
    sprintf(value, "%d", 0);
#endif

    return 0;
}

/**
 * Change IP6 forwarding state of the interface.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         new value pointer
 * @param ifname        name of the interface (like "eth0")
 *
 * @return              Status code
 */
static te_errno
iface_ip6_fw_set(unsigned int gid, const char *oid, const char *value,
                 const char *ifname)
{
#if __linux__
    int     fd;
    char    filename[128];
#endif

    UNUSED(gid);
    UNUSED(oid);

#if __linux__
    if ((*value != '0' && *value != '1') || *(value + 1) != 0)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    sprintf(filename, "/proc/sys/net/ipv6/conf/%s/forwarding", ifname);
    if ((fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0666)) < 0)
        return TE_OS_RC(TE_TA_UNIX, errno);

    if (write(fd, *value == '0' ? "0\n" : "1\n", 2) < 0)
    {
        close(fd);
        return TE_OS_RC(TE_TA_UNIX, errno);
    }

    close(fd);
#else
    /* FIXME Add implementation in SOLARIS and(or) BSD if necessary */
    return TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif

    return 0;
}

/**
 * Get IP6 'accept_ra' state of the interface.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         value location
 * @param ifname        name of the interface (like "eth0")
 *
 * @return              Status code
 */
static te_errno
iface_ip6_accept_ra_get(unsigned int gid, const char *oid, char *value,
                 const char *ifname)
{
#if __linux__
    char    c[2];
    int     fd;
    char    filename[128];

    UNUSED(gid);
    UNUSED(oid);

    sprintf(filename, "/proc/sys/net/ipv6/conf/%s/accept_ra", ifname);
    if ((fd = open(filename, O_RDONLY)) < 0)
        return TE_OS_RC(TE_TA_UNIX, errno);

    if (read(fd, c, 1) < 0)
    {
        close(fd);
        return TE_OS_RC(TE_TA_UNIX, errno);
    }
    close(fd);

    c[1] = '\0';
    sprintf(value, "%s", c);
#else
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(ifname);

    /* FIXME Add implementation in SOLARIS and(or) BSD if necessary */
    sprintf(value, "%d", 0);
#endif

    return 0;
}

/**
 * Change IP6 'accept_ra' state of the interface.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         new value pointer
 * @param ifname        name of the interface (like "eth0")
 *
 * @return              Status code
 */
static te_errno
iface_ip6_accept_ra_set(unsigned int gid, const char *oid,
                const char *value, const char *ifname)
{
#if __linux__
    int     fd;
    char    filename[128];
    int     accept_ra_val = -1; /* Invalid value */

    UNUSED(gid);
    UNUSED(oid);

    if (sscanf(value, "%d", &accept_ra_val) != 1 ||
        accept_ra_val < 0 ||
        accept_ra_val > 2 /* Allowed values are 0, 1, 2 */)
    {
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    sprintf(filename, "/proc/sys/net/ipv6/conf/%s/accept_ra", ifname);
    if ((fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0666)) < 0)
        return TE_OS_RC(TE_TA_UNIX, errno);

    if (write(fd,
              accept_ra_val == 0 ?
                "0\n" :
                    accept_ra_val == 1 ?
                        "1\n" :
                            "2\n", 2) < 0)
    {
        close(fd);
        return TE_OS_RC(TE_TA_UNIX, errno);
    }

    close(fd);
#else
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);
    UNUSED(ifname);

    /* FIXME Add implementation in SOLARIS and(or) BSD if necessary */
    return TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif

    return 0;
}

/**
 * Get RPF filtering value
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         value location
 * @param ifname        name of the interface (like "eth0")
 *
 * @return              Status code
 */
static te_errno
rp_filter_get(unsigned int gid, const char *oid, char *value,
              const char *ifname)
{
    UNUSED(gid);
    UNUSED(oid);

#if __linux__
    FILE *fd;
    int   res;
    char  path[128];

    if ((res = snprintf(path, sizeof(path),
                        "/proc/sys/net/ipv4/conf/%s/rp_filter",
                        ifname)) < 0 || res == sizeof(path))
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    if ((fd = fopen(path, "r")) == NULL)
        return TE_OS_RC(TE_TA_UNIX, errno);

    if (fread(value, 1, 1, fd) != 2)
    {
        fclose(fd);
        return TE_OS_RC(TE_TA_UNIX, errno);
    }
    fclose(fd);
    value[1] = 0;
#else
    UNUSED(value);
    UNUSED(ifname);

    return TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif

    return 0;
}

/**
 * Set RPF filtering value
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         new value pointer
 * @param ifname        name of the interface (like "eth0")
 *
 * @return              Status code
 */
static te_errno
rp_filter_set(unsigned int gid, const char *oid, const char *value,
              const char *ifname)
{
    UNUSED(gid);
    UNUSED(oid);

#if __linux__
    FILE *fd;
    int   res;
    char  path[128];

    if (*value < '0' || *value > '2' || *(value + 1) != 0)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    if ((res = snprintf(path, sizeof(path),
                        "/proc/sys/net/ipv4/conf/%s/rp_filter",
                        ifname)) < 0 || res == sizeof(path))
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    if ((fd = fopen(path, "w")) == NULL)
        return TE_OS_RC(TE_TA_UNIX, errno);

    
    if (fputc(*value, fd) != *value ||
        fputc('\n', fd) != '\n')
    {
        fclose(fd);
        return TE_OS_RC(TE_TA_UNIX, errno);
    }

    fclose(fd);

#else
    UNUSED(value);
    UNUSED(ifname);

    return TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif

    return 0;
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

static te_errno
neigh_find(const char *oid, const char *ifname, const char *addr,
           char *mac_p, unsigned int *state_p)
{
#if defined(USE_LIBNETCONF)
    te_errno            rc;
    sa_family_t         family;
    unsigned int        ifindex;
    gen_ip_address      ip_addr;
    unsigned int        addrlen;
    netconf_list       *list;
    netconf_node       *t;
    te_bool             dynamic;
    te_bool             found;

    family = str_addr_family(addr);
    dynamic = (strstr(oid, "dynamic") != NULL);

    if ((rc = CHECK_INTERFACE(ifname)) != 0)
    {
        ERROR("%s(): Bad device name '%s'", __FUNCTION__, ifname);
        return TE_RC(TE_TA_UNIX, rc);
    }

    if ((ifindex = if_nametoindex(ifname)) == 0)
    {
        ERROR("%s(): Device '%s' does not exist", __FUNCTION__, ifname);
        return TE_RC(TE_TA_UNIX, TE_ENODEV);
    }

    if (inet_pton(family, addr, &ip_addr) <= 0)
    {
        ERROR("Failed to convert address '%s' from string", addr);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    addrlen = (family == AF_INET) ?
              sizeof(struct in_addr) : sizeof(struct in6_addr);

    if ((list = netconf_neigh_dump(nh, family)) == NULL)
    {
        ERROR("%s(): Cannot get list of neighbours", __FUNCTION__);
        return TE_OS_RC(TE_TA_UNIX, errno);
    }

    found = FALSE;
    for (t = list->head; t != NULL; t = t->next)
    {
        const netconf_neigh *neigh = &(t->data.neigh);

        if ((unsigned int)(neigh->ifindex) != ifindex)
            continue;

        if (memcmp(neigh->dst, &ip_addr, addrlen) != 0)
            continue;

        if ((neigh->state == NETCONF_NUD_UNSPEC) ||
            (neigh->state == NETCONF_NUD_FAILED) ||
            (dynamic == !!(neigh->state & NETCONF_NUD_PERMANENT)))
        {
            continue;
        }

        found = TRUE;

        if (mac_p != NULL)
        {
            link_addr_n2a(neigh->lladdr, neigh->addrlen,
                          mac_p, RCF_MAX_VAL);
        }

        if (state_p != NULL)
            *state_p = neigh->state;

        /* Find the first one */
        break;
    }

    netconf_list_free(list);

    if (!found)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

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
#if defined(USE_LIBNETCONF)
    te_errno            rc;
    te_bool             dynamic;
    sa_family_t         family;
    unsigned int        ifindex;
    gen_ip_address      ip_addr;
    netconf_neigh       neigh;
    uint8_t             raw_addr[ETHER_ADDR_LEN];

    UNUSED(gid);

    family = str_addr_family(addr);
    dynamic = (strstr(oid, "dynamic") != NULL);

    if ((rc = CHECK_INTERFACE(ifname)) != 0)
    {
        ERROR("%s(): Bad device name '%s'", __FUNCTION__, ifname);
        return TE_RC(TE_TA_UNIX, rc);
    }

    if ((ifindex = if_nametoindex(ifname)) == 0)
    {
        ERROR("%s(): Device '%s' does not exist", __FUNCTION__, ifname);
        return TE_RC(TE_TA_UNIX, TE_ENODEV);
    }

    if (inet_pton(family, addr, &ip_addr) <= 0)
    {
        ERROR("Failed to convert address '%s' from string", addr);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    netconf_neigh_init(&neigh);
    neigh.family = family;
    neigh.ifindex = ifindex;

    neigh.state = (dynamic) ?
                  NETCONF_NUD_REACHABLE : NETCONF_NUD_PERMANENT;

    neigh.dst = (uint8_t *)&ip_addr;

    if (value != NULL)
    {
        if (link_addr_a2n(raw_addr, sizeof(raw_addr),
                          value) != ETHER_ADDR_LEN)
        {
            ERROR("Bad hardware address '%s'", value);
            return TE_RC(TE_TA_UNIX, TE_EINVAL);
        }

        neigh.addrlen = ETHER_ADDR_LEN;
        neigh.lladdr = raw_addr;
    }

    if (netconf_neigh_modify(nh, NETCONF_CMD_REPLACE, &neigh) < 0)
    {
        ERROR("%s(): Cannot add neighbour '%s' on interface '%s'",
              __FUNCTION__, addr, ifname);
        return TE_OS_RC(TE_TA_UNIX, errno);
    }

    return 0;
#else
    struct arpreq arp_req;
    int           i;

    int           int_addr[ETHER_ADDR_LEN];
    int           res;

    UNUSED(gid);

    res = sscanf(value, "%2x:%2x:%2x:%2x:%2x:%2x%*s",
                 int_addr, int_addr + 1, int_addr + 2, int_addr + 3,
                 int_addr + 4, int_addr + 5);

    if (res != 6)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

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
#if defined(USE_LIBNETCONF)
    {
        sa_family_t         family;
        unsigned int        ifindex;
        gen_ip_address      ip_addr;
        netconf_neigh       neigh;

        family = str_addr_family(addr);

        if ((rc = CHECK_INTERFACE(ifname)) != 0)
        {
            ERROR("%s(): Bad device name '%s'", __FUNCTION__, ifname);
            return TE_RC(TE_TA_UNIX, rc);
        }

        if ((ifindex = if_nametoindex(ifname)) == 0)
        {
            ERROR("%s(): Device '%s' does not exist",
                  __FUNCTION__, ifname);
            return TE_RC(TE_TA_UNIX, TE_ENODEV);
        }

        if (inet_pton(family, addr, &ip_addr) <= 0)
        {
            ERROR("Failed to convert address '%s' from string", addr);
            return TE_RC(TE_TA_UNIX, TE_EINVAL);
        }

        netconf_neigh_init(&neigh);
        neigh.family = family;
        neigh.ifindex = ifindex;
        neigh.dst = (uint8_t *)&ip_addr;

        if (netconf_neigh_modify(nh, NETCONF_CMD_DEL, &neigh) < 0)
        {
            ERROR("%s(): Cannot delete neighbour '%s' from "
                  "interface '%s'",
                  __FUNCTION__, addr, ifname);
            return TE_OS_RC(TE_TA_UNIX, errno);
        }

        return 0;
    }
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

#if defined(USE_LIBNETCONF)
static te_errno
ta_unix_conf_neigh_list(const char *ifname, te_bool is_static,
                        char **list)
{
    te_errno            rc;
    unsigned int        ifindex;
    netconf_list       *nlist;
    netconf_node       *t;
    unsigned int        len;
    char               *cur_ptr;

    if (list == NULL)
    {
        ERROR("%s(): Invalid value for 'list' argument", __FUNCTION__);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    if ((rc = CHECK_INTERFACE(ifname)) != 0)
    {
        ERROR("%s(): Bad device name '%s'", __FUNCTION__, ifname);
        return TE_RC(TE_TA_UNIX, rc);
    }

    if ((ifindex = if_nametoindex(ifname)) == 0)
    {
        ERROR("%s(): Device '%s' does not exist", __FUNCTION__, ifname);
        return TE_RC(TE_TA_UNIX, TE_ENODEV);
    }

    /* There are no neighbours for loopback interface */
    if (strcmp(ifname, "lo") == 0)
        return 0;

    /* Get neighbours of both families: IPv4 and IPv6 */
    if ((nlist = netconf_neigh_dump(nh, AF_UNSPEC)) == NULL)
    {
        ERROR("%s(): Cannot get list of neighbours", __FUNCTION__);
        return TE_OS_RC(TE_TA_UNIX, errno);
    }

    /* Calculate maximum space needed by the list */
    len = nlist->length * (INET6_ADDRSTRLEN + 1);

    if (len == 0)
    {
        *list = NULL;
        return 0;
    }

    if ((*list = malloc(len)) == NULL)
    {
        netconf_list_free(nlist);
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }

    if (len == 0)
    {
        *list = NULL;
        return 0;
    }

    memset(*list, 0, len);

    cur_ptr = *list;
    for (t = nlist->head; t != NULL; t = t->next)
    {
        const netconf_neigh *neigh = &(t->data.neigh);

        assert(cur_ptr >= *list);
        assert((unsigned int)(cur_ptr - *list) <= len);

        if ((unsigned int)(neigh->ifindex) != ifindex)
            continue;

        if (((neigh->state & NETCONF_NUD_UNSPEC) != 0) ||
            ((neigh->state & NETCONF_NUD_INCOMPLETE) != 0) ||
            (!(neigh->state & NETCONF_NUD_PERMANENT) == is_static))
        {
            continue;
        }

        if ((neigh->lladdr == NULL) || (neigh->dst == NULL))
            continue;

        /* Neighbour is ok, save it to the list */

        if (cur_ptr != *list)
        {
            snprintf(cur_ptr, len - (cur_ptr - *list), " ");
            cur_ptr += strlen(cur_ptr);
        }

        if (inet_ntop(neigh->family, neigh->dst, cur_ptr,
                      len - (cur_ptr - *list)) == NULL)
        {
            ERROR("%s(): Cannot save destination address",
                  __FUNCTION__);
            free(*list);
            netconf_list_free(nlist);
            return TE_RC(TE_TA_UNIX, TE_EINVAL);
        }

        cur_ptr += strlen(cur_ptr);
    }

    netconf_list_free(nlist);

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
agent_platform_get(unsigned int gid, const char *oid, char *result,
                   const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(instance);
#ifdef TE_AGT_PLATFORM
    memcpy(result, TE_AGT_PLATFORM, sizeof(TE_AGT_PLATFORM));
#else
    memcpy(result, "default", sizeof("default"));
#endif
    return 0;
}

static te_errno
agent_dir_get(unsigned int gid, const char *oid, char *result,
              const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(instance);
    memcpy(result, ta_dir, strlen(ta_dir) + 1);
    return 0;
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
    char * const *env;

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

            if (euid == 0 || setuid(0) == 0)     /**< Get 'root' */
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

                if (euid != 0)
                    setuid(euid);   /* Restore saved previously user id */
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

#if 0
    /* Fedora has very aggressive nscd cache */
    /* https://bugzilla.redhat.com/bugzilla/show_bug.cgi?id=134323 */
    ta_system("/usr/sbin/nscd -i group && /usr/sbin/nscd -i passwd");
#endif

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

#if 0
    /* Fedora has very aggressive nscd cache */
    /* https://bugzilla.redhat.com/bugzilla/show_bug.cgi?id=134323 */
    ta_system("/usr/sbin/nscd -i group && /usr/sbin/nscd -i passwd");
#endif

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

    sprintf(buf, "/usr/bin/killall -u %s", user);
    ta_system(buf); /* Ignore rc */
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

#if XEN_SUPPORT

/** Maximal number of maintained domUs and bridges in every domU */
enum { MAX_DOM_U_NUM = 256, MAX_BRIDGE_NUM = 16, MAX_INTERFACE_NUM = 16 };

/** DomU statuses */
typedef enum { DOM_U_STATUS_NON_RUNNING,
               DOM_U_STATUS_RUNNING,
               DOM_U_STATUS_SAVED,
               DOM_U_STATUS_MIGRATED_RUNNING,
               DOM_U_STATUS_MIGRATED_SAVED,
               DOM_U_STATUS_ERROR } status_t;

/**
 * Path to accessible across network storage for
 * XEN kernel and templates of XEN config/VBD images.
 */
static char xen_path[PATH_MAX]    = { '\0' };

/** Subpath to XEN storage for dynamically created/destroyed domUs */
static char xen_subpath[PATH_MAX] = { '\0' };

/** Kernel, initial ramdisk and VBD image files */
static char xen_kernel[PATH_MAX]  = { '\0' };
static char xen_initrd[PATH_MAX]  = { '\0' };
static char xen_dsktpl[PATH_MAX]  = { '\0' };

/** RCF port number */
static unsigned int xen_rcf_port  = 0;

/** XEN dom0 RPC bridge and interface */
static char xen_rpc_br[PATH_MAX]  = { '\0' };
static char xen_rpc_if[PATH_MAX]  = { '\0' };

/** XEN domU base MAC address */
static char xen_base_mac_addr[] = "00:00:00:00:00:00";

/** Values that are used to initialize addresses */
static char const init_ip_addr[]  = "0.0.0.0";
static char const init_mac_addr[] = "00:00:00:00:00:00";

/* Names of the cloned disk image, swap image and temporary directory */
static char const *const xen_dskimg = "disk.img";
static char const *const xen_swpimg = "swap.img";
static char const *const xen_tmpdir = "tmpdir";

/** Status name to status and vice versa translation array */
static struct {
    char const *name; /**< Status name */
    status_t status;  /**< Status      */
} const statuses[] = {
    { "non-running",      DOM_U_STATUS_NON_RUNNING },
    { "running",          DOM_U_STATUS_RUNNING },
    { "saved",            DOM_U_STATUS_SAVED },
    { "migrated-running", DOM_U_STATUS_MIGRATED_RUNNING },
    { "migrated-saved",   DOM_U_STATUS_MIGRATED_SAVED } };

/** XEN vertual tested interface internal representation */
static struct {
    char const *if_name; /**< XEN virtual tested interface name       */
    char const *ph_name; /**< XEN realp hysical tested interface name */
    char const *br_name; /**< XEN bridge, which both interfaces
                              are connected to*/
} interface_slot[MAX_INTERFACE_NUM];

/** DomU internal representation */
static struct {
    char const   *name;          /**< DomU name (also serves as slot
                                      is empty sign if it is NULL)       */
    status_t     status;         /**< DomU state                         */
    unsigned int memory;         /**< DomU state                         */
    char         ip_addr[16];    /**< DomU IP address                    */
    char         mac_addr[18];   /**< DomU MAC address                   */

    struct {
       char const *br_name;      /**< Name of the bridge where dom0
                                      tested interface is added to       */
       char const *if_name;      /**< DomU testing interface name        */
       char        ip_addr[16];  /**< DomU testing interface IP address  */
       char        mac_addr[18]; /**< DomU testing interface MAC address */
       te_bool     accel;        /**< Accelerated spec-tion in config    */
    }
    bridge_slot[MAX_BRIDGE_NUM]; /**< DomU bridges where dom0 tested
                                      interfaces are added to            */
    int          migrate_kind;   /**< Migrate kind (non-live/live)       */
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
 *                      MAX_DOM_U_NUM)
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
        if (((dom_u_index_) =                                          \
                  find_dom_u(dom_u_name_)) >= dom_u_limit())           \
        {                                                              \
            ERROR("DomU '%s' does NOT exist", (dom_u_name_));          \
            return TE_RC(TE_TA_UNIX, TE_ENOENT);                       \
        }                                                              \
    } while(0)


/**
 * Get the whole number of bridge slots.
 *
 * @return              The whole number of bridge slots
 */
static inline unsigned int
bridge_limit(void)
{
    return sizeof(dom_u_slot[0].bridge_slot) /
               sizeof(*dom_u_slot[0].bridge_slot);
}

/**
 * Find bridge.
 *
 * @param bridge        The name of the bridge to find
 *
 * @return              domU index (from 0 to MAX_BRIDGE_NUM - 1) if
 *                      found, otherwise - 'sizeof(bridge_slot) /
 *                      sizeof(*bridge_slot)' (which is equivlent to
 *                      MAX_BRIDGE_NUM)
 */
static unsigned
find_bridge(char const *bridge, unsigned int u)
{
    unsigned int v;
    unsigned int limit = bridge_limit();

    for (v = 0; v < limit; v++)
    {
        char const *name = dom_u_slot[u].bridge_slot[v].br_name;

        if (name != NULL && strcmp(name, bridge) == 0)
            break;
    }

    return v;
}

/* Try to find bridge and initialize its index */
#define FIND_BRIDGE(bridge_name_, dom_u_index_, bridge_index_) \
    do {                                                               \
        if (((bridge_index_) =                                         \
                 find_bridge((bridge_name_),                           \
                             (dom_u_index_))) >= bridge_limit())       \
        {                                                              \
            ERROR("Bridge '%s' in DomU '%s' does NOT exist",           \
                  (bridge_name_), dom_u_slot[dom_u_index_].name);      \
            return TE_RC(TE_TA_UNIX, TE_ENOENT);                       \
        }                                                              \
    } while(0)


/**
 * Get the whole number of interface slots.
 *
 * @return              The whole number of domU slots
 */
static inline unsigned int
interface_limit(void)
{
    return sizeof(interface_slot) / sizeof(*interface_slot);
}

/**
 * Find interface.
 *
 * @param interface     The name of the interface to find
 *
 * @return              interface index (from 0 to MAX_INTERFACE_NUM - 1)
 *                      if found, otherwise - 'sizeof(interface_slot) /
 *                      sizeof(*interface_slot)' (which is equivlent to
 *                      MAX_INTERFACE_NUM)
 */
static unsigned
find_interface(char const *interface)
{
    unsigned int u;
    unsigned int limit = interface_limit();

    for (u = 0; u < limit; u++)
    {
        char const *name = interface_slot[u].if_name;

        if (name != NULL && strcmp(name, interface) == 0)
            break;
    }

    return u;
}

/**
 * Find interface.
 *
 * @param bridge        The name of the bridge physical interface
 *                      is connected to
 *
 * @return              Physical interface name or NULL if not found
 */
static char const *
find_physical_interface(char const *bridge)
{
    unsigned int u;
    unsigned int limit = interface_limit();

    if (bridge == NULL)
        return NULL;

    for (u = 0; u < limit; u++)
    {
        if (interface_slot[u].if_name != NULL &&
            strcmp(interface_slot[u].br_name, bridge) == 0)
        {
            return interface_slot[u].ph_name;
        }
    }

    return NULL;
}

/* Try to find interface and initialize its index */
#define FIND_INTERFACE(interface_name_, interface_index_) \
    do {                                                                 \
        if (((interface_index_) =                                        \
                  find_interface(interface_name_)) >= interface_limit()) \
        {                                                                \
            ERROR("Interface '%s' does NOT exist", (interface_name_));   \
            return TE_RC(TE_TA_UNIX, TE_ENOENT);                         \
        }                                                                \
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
    struct stat st;

    /* Probably there is better mean do detect we are within dom0, eh? */
    return stat("/usr/sbin/xm", &st) == 0 &&
           (S_ISLNK(st.st_mode) || S_ISREG(st.st_mode));
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
 * Forms full path to domU disk image dynamic storage
 *
 * @param dom_u         domU
 * @param fname         File name inside disk image (path from root /)
 * @param fdata         Data string (zero ended)
 *
 * @return              Status code
 */
static char const *
get_dom_u_path(char const *dom_u)
{
    size_t      xen_path_len    = strlen(xen_path);
    size_t      xen_subpath_len = strlen(xen_subpath);
    size_t      dom_u_len       = strlen(dom_u);
    static char dom_u_path[PATH_MAX];
    char       *ptr             = dom_u_path;

    if (xen_path_len + 1 +
            xen_subpath_len + (xen_subpath_len > 0 ? 1 : 0) +
            dom_u_len + 1 > sizeof(dom_u_path))
    {
        *dom_u_path = '\0';
    }
    else
    {
        memcpy(ptr, xen_path, xen_path_len);
        ptr   += xen_path_len;
        *ptr++ = '/';

        if (xen_subpath_len > 0)
        {
            memcpy(ptr, xen_subpath, xen_subpath_len);
            ptr   += xen_subpath_len;
            *ptr++ = '/';
        }

        strcpy(ptr, dom_u);
    }

    return dom_u_path;
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
    char              buffer[PATH_MAX];
    char const *const dom_u_path = get_dom_u_path(dom_u);
    struct stat       st;
    te_errno          rc = 0;
    FILE             *f;
    int               sys;

    TE_SPRINTF(buffer, "%s/%s", dom_u_path, xen_tmpdir);

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
    TE_SPRINTF(buffer, "mount -o loop %s/%s %s/%s",
               dom_u_path, xen_dskimg, dom_u_path, xen_tmpdir);

    if ((sys = ta_system(buffer)) != 0 && !(sys == -1 && errno == ECHILD))
    {
        rc = TE_RC(TE_TA_UNIX, TE_EFAIL);
        goto cleanup1;
    }

    TE_SPRINTF(buffer, "%s/%s%s", dom_u_path, xen_tmpdir, fname);

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
    TE_SPRINTF(buffer, "umount %s/%s", dom_u_path, xen_tmpdir);

    if ((sys = ta_system(buffer)) != 0 && !(sys == -1 && errno == ECHILD))
    {
        if (rc == 0)
            rc = TE_RC(TE_TA_UNIX, TE_EFAIL);
    }

cleanup1:
    TE_SPRINTF(buffer, "%s/%s", dom_u_path, xen_tmpdir);

    if (rmdir(buffer) == -1)
    {
        if (rc == 0)
            rc = TE_RC(TE_TA_UNIX, TE_EFAIL);
    }

cleanup0:
    return rc;
}

/**
 * Checks that all attributes of initialized
 * domU interfaces are also initialized properly
 *
 * @param u             DomU slot number
 *
 * @return              Status code
 */
static te_errno
check_dom_u_is_initialized_properly(unsigned int u)
{
    unsigned int v;
    unsigned int limit = bridge_limit();

    if (dom_u_slot[u].memory == 0)
    {
        ERROR("Memory amount for '%s' domU is UNspecified",
              dom_u_slot[u].name);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    if (*xen_rpc_br == '\0')
    {
        ERROR("The name of the bridge that is used for RCF/RPC "
              "communication ('/agent/xen/rpc_br') is NOT initialized");
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    if (*xen_rpc_if == '\0')
    {
        ERROR("The name of the interface that is used for RCF/RPC "
              "communication ('/agent/xen/rpc_if') is NOT initialized");
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    if (strcmp(dom_u_slot[u].ip_addr, init_ip_addr) == 0)
    {
        ERROR("The IP address of the interface that is used for "
              "RCF/RPC communication ('/agent/xen/dom_u/ip_addr') "
              "is NOT initialized for '%s' domU", dom_u_slot[u].name);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    if (strcmp(dom_u_slot[u].mac_addr, init_mac_addr) == 0)
    {
        ERROR("The MAC address of the interface that is used for "
              "RCF/RPC communication ('/agent/xen/dom_u/mac_addr') "
              "is NOT initialized for '%s' domU", dom_u_slot[u].name);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    for (v = 0; v < limit; v++)
    {
        char const *br_name = dom_u_slot[u].bridge_slot[v].br_name;

        if (br_name != NULL)
        {
            char const *if_name = dom_u_slot[u].bridge_slot[v].if_name;

            if (*if_name == '\0')
            {
                ERROR("The name of the interface that is used for "
                      "testing communication over '%s' bridge (the "
                      "value of '/agent/xen/dom_u/bridge') is NOT "
                      "initialized for '%s' domU", br_name,
                      dom_u_slot[u].name);
                return TE_RC(TE_TA_UNIX, TE_EINVAL);
            }

            if (strcmp(dom_u_slot[u].bridge_slot[v].ip_addr,
                       init_ip_addr) == 0)
            {
                ERROR("The IP address of the '%s' interface that is "
                      "used for testing communication over '%s' "
                      "bridge ('/agent/xen/dom_u/bridge/ip_addr') "
                      "is NOT initialized for '%s' domU", if_name,
                      br_name, dom_u_slot[u].name);
                return TE_RC(TE_TA_UNIX, TE_EINVAL);
            }

            if (strcmp(dom_u_slot[u].bridge_slot[v].mac_addr,
                       init_mac_addr) == 0)
            {
                ERROR("The MAC address of the '%s' interface that is "
                      "used for testing communication over '%s' "
                      "bridge ('/agent/xen/dom_u/bridge/mac_addr') "
                      "is NOT initialized for '%s' domU", if_name,
                      br_name, dom_u_slot[u].name);
                return TE_RC(TE_TA_UNIX, TE_EINVAL);
            }
        }
    }

    return 0;
}

/**
 * Fills the next part of the 'buf' or resets buffer pointer
 *
 * @param fmt           Format (resets buffer pointer if NULL)
 *
 * @return              Status code
 */
static te_errno
update_buf(char const *fmt, ...)
{
    int     num;
    va_list ap;

    static char *ptr = buf;

    if (fmt == NULL)
    {
        *(ptr = buf) = '\0';
        return 0;
    }

    va_start(ap, fmt);
    num = vsnprintf(ptr, buf + sizeof(buf) - ptr, fmt, ap);
    va_end(ap);

    if (num < 0)
    {
        *(ptr = buf) = '\0';
        return TE_OS_RC(TE_TA_UNIX, errno);
    }

    /* Check whether truncation has occured */
    if (num >= buf + sizeof(buf) - ptr)
    {
        ERROR("Buffer size (%u) is too small", sizeof(buf));
        *(ptr = buf) = '\0';
        return TE_RC(TE_TA_UNIX, TE_EFAIL);
    }

    ptr += num;

    return 0;
}

/**
 * Fills 'buf' with the network interfaces data for domU config file
 *
 * @param u             DomU slot number
 * @param i             DomU bridge slot number
 *
 * @return              Status code
 */
static te_errno
add_dom_u_interfaces_config(unsigned int u, int i)
{
    if (i < 0)
    {
        return update_buf("vif  = [ 'bridge=%s,mac=%s'",
                          xen_rpc_br, dom_u_slot[u].mac_addr);
    }

    if (i >= (int) bridge_limit())
        return update_buf(" ]\n");

    {
        char const *brd = dom_u_slot[u].bridge_slot[i].br_name;
        char const *mac = dom_u_slot[u].bridge_slot[i].mac_addr;
        char const *phy = find_physical_interface(brd);

        if (brd != NULL)
        {
            if (phy == NULL)
            {
                ERROR("Internal error: cannot find "
                      "physical interface by bridge name");
                return TE_RC(TE_TA_UNIX, TE_EFAIL);
            }

            return dom_u_slot[u].bridge_slot[i].accel ?
                       update_buf(",'bridge=%s,accel=%s,mac=%s'",
                                  brd, phy, mac) :
                       update_buf(",'bridge=%s,mac=%s'", brd, mac);
        }
    }

    return 0;
}

/**
 * Fills 'buf' with the network interfaces data for domU config file
 *
 * @param u             DomU slot number
 *
 * @return              Status code
 */
static te_errno
prepare_dom_u_interfaces_config(unsigned int u)
{
    int i;
    int limit = (int)bridge_limit();

    update_buf(NULL); /** Restart buffer pointer (see update_conf) */

    /* Prepare interfaces */
    for (i = -1; i <= limit; i++)
    {
        te_errno rc = add_dom_u_interfaces_config(u, i);

        if (rc != 0)
            return rc;
    }

    return 0;
}

/**
 * Fills 'buf' with the data for domU
 * '/etc/udev/rules.d/z25_persistent-net.rules'
 *
 * @param u             DomU slot number
 *
 * @return              Status code
 */
static te_errno
prepare_persistent_net_rules(unsigned int u)
{
    int   i;
    int   limit = (int) bridge_limit();

    update_buf(NULL); /** Restart buffer pointer (see update_conf) */

    /* Prepare interfaces */
    for (i = -1; i < limit; i++)
    {
        if (i < 0 || dom_u_slot[u].bridge_slot[i].br_name != NULL)
        {
            char const *mac = i < 0 ?
                           dom_u_slot[u].mac_addr :
                           dom_u_slot[u].bridge_slot[i].mac_addr;
            char const *ifn = i < 0 ?
                           xen_rpc_if :
                           dom_u_slot[u].bridge_slot[i].if_name;

            te_errno    rc = update_buf("\n"
                                        "# Xen virtual device (vif)\n"
                                        "SUBSYSTEM==\"net\", "
                                        "DRIVERS==\"?*\", "
                                        "ATTRS{address}==\"%s\", "
                                        "NAME=\"%s\"\n", mac, ifn);

            if (rc != 0)
                return rc;
        }
    }

    return 0;
}

/**
 * Fills 'buf' with the data for domU '/etc/network/interfaces'
 *
 * @param u             DomU slot number
 *
 * @return              Status code
 */
static te_errno
prepare_network_interfaces_config(unsigned int u)
{
    int   i;
    int   limit = (int) bridge_limit();

    update_buf(NULL); /** Restart buffer pointer (see update_conf) */

    /* Prepare interfaces */
    for (i = -1; i < limit; i++)
    {
        if (i < 0 || dom_u_slot[u].bridge_slot[i].br_name != NULL)
        {
            char const *hdr = i < 0 ?
                           "auto lo\niface lo inet loopback\n" :
                           "";
            char const *ifn = i < 0 ?
                           xen_rpc_if :
                           dom_u_slot[u].bridge_slot[i].if_name;
            char const *ipa = i < 0 ?
                           dom_u_slot[u].ip_addr :
                           dom_u_slot[u].bridge_slot[i].ip_addr;

            te_errno    rc = update_buf("%s\nauto %s\n"
                                        "iface %s inet static\n"
                                        "    address %s\n"
                                        "    netmask 255.255.255.0\n",
                                        hdr, ifn, ifn, ipa);
            if (rc != 0)
                return rc;
        }
    }

    return 0;
}
#endif /* XEN_SUPPORT */

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
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);

#if XEN_SUPPORT
    strcpy(value, xen_path);
    return 0;
#else
#warning '/agent/xen' 'get' access method is not implemented
    ERROR("'/agent/xen' 'get' access method is not implemented");
    return TE_OS_RC(TE_TA_UNIX, TE_ENOSYS);
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
#if XEN_SUPPORT
    unsigned int u;
    unsigned int limit = dom_u_limit();
    size_t       len   = strlen(value);
#endif

    UNUSED(gid);
    UNUSED(oid);

#if XEN_SUPPORT
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
    }

    memcpy(xen_path, value, len + 1);
    return 0;
#else
#warning '/agent/xen' 'set' access method is not implemented
    UNUSED(value);

    ERROR("'/agent/xen' 'set' access method is not implemented");
    return TE_OS_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
}

/**
 * Get subpath to accessible across network storage for
 * XEN config/VBD images.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         storage for path to be filled in
 *
 * @return              Status code
 */
static te_errno
xen_subpath_get(unsigned int gid, char const *oid, char *value)
{
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);

#if XEN_SUPPORT
    strcpy(value, xen_subpath);
    return 0;
#else
#warning '/agent/xen/subpath' 'get' access method is not implemented
    ERROR("'/agent/xen/subpath' 'get' access method is not implemented");
    return TE_OS_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
}

/**
 * Set subpath to accessible across network storage for
 * XEN config/VBD images.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         path to set
 *
 * @return              Status code
 */
static te_errno
xen_subpath_set(unsigned int gid, char const *oid, char const *value)
{
#if XEN_SUPPORT
    size_t len = strlen(value);
#endif

    UNUSED(gid);
    UNUSED(oid);

#if XEN_SUPPORT
    /* Check whether XEN subpath fits XEN subpath storage */
    if (len >= sizeof(xen_subpath))
    {
        ERROR("XEN subpath is too long");
        return TE_RC(TE_TA_UNIX, TE_E2BIG);
    }

    memcpy(xen_subpath, value, len + 1);
    return 0;
#else
#warning '/agent/xen/subpath' 'set' access method is not implemented
    UNUSED(value);

    ERROR("'/agent/xen/subpath' 'set' access method is not implemented");
    return TE_OS_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
}

/**
 * Get XEN kernel file name.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         storage for kernel file name to be filled in
 *
 * @return              Status code
 */
static te_errno
xen_kernel_get(unsigned int gid, char const *oid, char *value)
{
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);

#if XEN_SUPPORT
    strcpy(value, xen_kernel);
    return 0;
#else
#warning '/agent/xen/kernel' 'get' access method is not implemented
    ERROR("'/agent/xen/kernel' 'get' access method is not implemented");
    return TE_OS_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
}

/**
 * Set XEN kernel file name (XEN path must be set properly previously).
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         kernel file name to set
 *
 * @return              Status code
 */
static te_errno
xen_kernel_set(unsigned int gid, char const *oid, char const *value)
{
#if XEN_SUPPORT
    unsigned int u;
    unsigned int limit = dom_u_limit();
    size_t       len   = strlen(value);
#endif

    UNUSED(gid);
    UNUSED(oid);

#if XEN_SUPPORT
    /* If value is not empty string then the agent must run within dom0 */
    if (*value != '\0' && !is_within_dom0())
    {
        ERROR("Agent runs NOT within dom0");
        return TE_RC(TE_TA_UNIX, TE_EFAIL);
    }

    /* XEN path must be previously properly set */
    if (*xen_path == '\0')
    {
        ERROR("Failed to set XEN kernel file name because "
              "XEN path is NOT set properly yet");
        return TE_RC(TE_TA_UNIX, TE_EFAIL);
    }

    /* Check whether domUs exist */
    for (u = 0; u < limit; u++)
        if (dom_u_slot[u].name != NULL)
        {
            ERROR("Failed to change XEN kernel file name: "
                  "domU(s) exist(s)");
            return TE_RC(TE_TA_UNIX, TE_EBUSY);
        }

    /* Check whether XEN kernel file name fits XEN path storage */
    if (len >= sizeof(xen_kernel))
    {
        ERROR("XEN kernel file name is too long");
        return TE_RC(TE_TA_UNIX, TE_E2BIG);
    }

    /* For non-empty XEN kernel file name perform all necessary checks */
    if (len > 0)
    {
        struct stat  st;

        TE_SPRINTF(buf, "%s/%s", xen_path, value);

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
    }

    memcpy(xen_kernel, value, len + 1);
    return 0;
#else
#warning '/agent/xen/kernel' 'set' access method is not implemented
    UNUSED(value);
    ERROR("'/agent/xen/kernel' 'set' access method is not implemented");
    return TE_OS_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
}

/**
 * Get XEN initial ramdisk file name.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         storage for initrd file name to be filled in
 *
 * @return              Status code
 */
static te_errno
xen_initrd_get(unsigned int gid, char const *oid, char *value)
{
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);

#if XEN_SUPPORT
    strcpy(value, xen_initrd);
    return 0;
#else
#warning '/agent/xen/initrd' 'get' access method is not implemented
    ERROR("'/agent/xen/initrd' 'get' access method is not implemented");
    return TE_OS_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
}

/**
 * Set XEN initrd file name (XEN path must be set properly previously).
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         initrd file name to set
 *
 * @return              Status code
 */
static te_errno
xen_initrd_set(unsigned int gid, char const *oid, char const *value)
{
#if XEN_SUPPORT
    unsigned int u;
    unsigned int limit = dom_u_limit();
    size_t       len   = strlen(value);
#endif

    UNUSED(gid);
    UNUSED(oid);

#if XEN_SUPPORT
    /* If value is not empty string then the agent must run within dom0 */
    if (*value != '\0' && !is_within_dom0())
    {
        ERROR("Agent runs NOT within dom0");
        return TE_RC(TE_TA_UNIX, TE_EFAIL);
    }

    /* XEN path must be previously properly set */
    if (*xen_path == '\0')
    {
        ERROR("Failed to set XEN initrd file name because "
              "XEN path is NOT set properly yet");
        return TE_RC(TE_TA_UNIX, TE_EFAIL);
    }

    /* Check whether domUs exist */
    for (u = 0; u < limit; u++)
        if (dom_u_slot[u].name != NULL)
        {
            ERROR("Failed to change XEN initrd file name: "
                  "domU(s) exist(s)");
            return TE_RC(TE_TA_UNIX, TE_EBUSY);
        }

    /* Check whether XEN initrd file name fits XEN path storage */
    if (len >= sizeof(xen_initrd))
    {
        ERROR("XEN initrd file name is too long");
        return TE_RC(TE_TA_UNIX, TE_E2BIG);
    }

    /* For non-empty XEN initrd file name perform all necessary checks */
    if (len > 0)
    {
        struct stat  st;

        TE_SPRINTF(buf, "%s/%s", xen_path, value);

        if (stat(buf, &st) == -1)
        {
            ERROR("XEN initrd does NOT exist on specified XEN path");
            return TE_RC(TE_TA_UNIX, TE_ENOENT);
        }

        if (!S_ISREG(st.st_mode))
        {
            ERROR("XEN initrd specified is NOT a file");
            return TE_RC(TE_TA_UNIX, TE_ENOENT);
        }
    }

    memcpy(xen_initrd, value, len + 1);
    return 0;
#else
#warning '/agent/xen/initrd' 'set' access method is not implemented
    UNUSED(value);
    ERROR("'/agent/xen/initrd' 'set' access method is not implemented");
    return TE_OS_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
}

/**
 * Get XEN dsktpl file name.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         storage for dsktpl file name to be filled in
 *
 * @return              Status code
 */
static te_errno
xen_dsktpl_get(unsigned int gid, char const *oid, char *value)
{
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);

#if XEN_SUPPORT
    strcpy(value, xen_dsktpl);
    return 0;
#else
#warning '/agent/xen/dsktpl' 'get' access method is not implemented
    ERROR("'/agent/xen/dsktpl' 'get' access method is not implemented");
    return TE_OS_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
}

/**
 * Set XEN dsktpl file name (XEN path must be set properly previously).
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         dsktpl file name to set
 *
 * @return              Status code
 */
static te_errno
xen_dsktpl_set(unsigned int gid, char const *oid, char const *value)
{
#if XEN_SUPPORT
    unsigned int u;
    unsigned int limit = dom_u_limit();
    size_t       len   = strlen(value);
#endif

    UNUSED(gid);
    UNUSED(oid);

#if XEN_SUPPORT
    /* If value is not empty string then the agent must run within dom0 */
    if (*value != '\0' && !is_within_dom0())
    {
        ERROR("Agent runs NOT within dom0");
        return TE_RC(TE_TA_UNIX, TE_EFAIL);
    }

    /* XEN path must be previously properly set */
    if (*xen_path == '\0')
    {
        ERROR("Failed to set XEN dsktpl file name because "
              "XEN path is NOT set properly yet");
        return TE_RC(TE_TA_UNIX, TE_EFAIL);
    }

    /* Check whether domUs exist */
    for (u = 0; u < limit; u++)
        if (dom_u_slot[u].name != NULL)
        {
            ERROR("Failed to change XEN dsktpl file name: "
                  "domU(s) exist(s)");
            return TE_RC(TE_TA_UNIX, TE_EBUSY);
        }

    /* Check whether XEN dsktpl file name fits XEN path storage */
    if (len >= sizeof(xen_dsktpl))
    {
        ERROR("XEN dsktpl file name is too long");
        return TE_RC(TE_TA_UNIX, TE_E2BIG);
    }

    /* For non-empty XEN dsktpl file name perform all necessary checks */
    if (len > 0)
    {
        struct stat  st;

        TE_SPRINTF(buf, "%s/%s", xen_path, value);

        if (stat(buf, &st) == -1)
        {
            ERROR("XEN dsktpl does NOT exist on specified XEN path");
            return TE_RC(TE_TA_UNIX, TE_ENOENT);
        }

        if (!S_ISREG(st.st_mode))
        {
            ERROR("XEN dsktpl specified is NOT a file");
            return TE_RC(TE_TA_UNIX, TE_ENOENT);
        }
    }

    memcpy(xen_dsktpl, value, len + 1);
    return 0;
#else
#warning '/agent/xen/dsktpl' 'set' access method is not implemented
    UNUSED(value);
    ERROR("'/agent/xen/dsktpl' 'set' access method is not implemented");
    return TE_OS_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
}

/**
 * Get RCF port number.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         storage for RCF port number to be filled in
 *
 * @return              Status code
 */
static te_errno
xen_rcf_port_get(unsigned int gid, char const *oid, char *value)
{
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);

#if XEN_SUPPORT
    sprintf(value, "%u", xen_rcf_port);
    return 0;
#else
#warning '/agent/xen/rcf_port' 'get' access method is not implemented
    ERROR("'/agent/xen/rcf_port' 'get' access method is not implemented");
    return TE_OS_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
}

/**
 * Set RCF port numer (restrictions are applied).
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         RCF port number to set
 *
 * @return              Status code
 */
static te_errno
xen_rcf_port_set(unsigned int gid, char const *oid, char const *value)
{
#if XEN_SUPPORT
    unsigned int u;
    unsigned int limit = dom_u_limit();
    int          port  = atoi(value); /** Relying on value validity */
#endif

    UNUSED(gid);
    UNUSED(oid);

#if XEN_SUPPORT
    /* If value is not 0 then the agent must run within dom0 */
    if (port != 0 && !is_within_dom0())
    {
        ERROR("Agent runs NOT within dom0");
        return TE_RC(TE_TA_UNIX, TE_EFAIL);
    }

    /* Check whether domUs exist */
    for (u = 0; u < limit; u++)
        if (dom_u_slot[u].name != NULL)
        {
            ERROR("Failed to change RCF port number: domU(s) exist(s)");
            return TE_RC(TE_TA_UNIX, TE_EBUSY);
        }

    /* For non-0 RCF port number perform all necessary checks */
    if (port != 0 && port < 1024 && port > 65535)
    {
        ERROR("RCF port number is neither 0 "
              "nor in the range from 1024 to 65535");
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    xen_rcf_port = port;
    return 0;
#else
#warning '/agent/xen/rcf_port' 'set' access method is not implemented
    UNUSED(value);
    ERROR("'/agent/xen/rcf_port' 'set' access method is not implemented");
    return TE_OS_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
}

/**
 * Get XEN RPC bridge name.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         storage for RPC bridge name to be filled in
 *
 * @return              Status code
 */
static te_errno
xen_rpc_br_get(unsigned int gid, char const *oid, char *value)
{
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);

#if XEN_SUPPORT
    strcpy(value, xen_rpc_br);
    return 0;
#else
#warning '/agent/xen/rpc_br' 'get' access method is not implemented
    ERROR("'/agent/xen/rpc_br' 'get' access method is not implemented");
    return TE_OS_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
}

/**
 * Set XEN RPC bridge name.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         RPC bridge name to set
 *
 * @return              Status code
 */
static te_errno
xen_rpc_br_set(unsigned int gid, char const *oid, char const *value)
{
#if XEN_SUPPORT
    unsigned int u;
    unsigned int limit = dom_u_limit();
    size_t       len   = strlen(value);
#endif

    UNUSED(gid);
    UNUSED(oid);

#if XEN_SUPPORT
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
            ERROR("Failed to change XEN RPC bridge name: "
                  "domU(s) exist(s)");
            return TE_RC(TE_TA_UNIX, TE_EBUSY);
        }

    /* Check whether XEN RPC bridge name fits XEN path storage */
    if (len >= sizeof(xen_rpc_br))
    {
        ERROR("XEN RPC bridge name is too long");
        return TE_RC(TE_TA_UNIX, TE_E2BIG);
    }

    memcpy(xen_rpc_br, value, len + 1);
    return 0;
#else
#warning '/agent/xen/rpc_br' 'set' access method is not implemented
    UNUSED(value);
    ERROR("'/agent/xen/rpc_br' 'set' access method is not implemented");
    return TE_OS_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
}

/**
 * Get XEN RPC interface name.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         storage for RPC interface name to be filled in
 *
 * @return              Status code
 */
static te_errno
xen_rpc_if_get(unsigned int gid, char const *oid, char *value)
{
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);

#if XEN_SUPPORT
    strcpy(value, xen_rpc_if);
    return 0;
#else
#warning '/agent/xen/rpc_if' 'get' access method is not implemented
    ERROR("'/agent/xen/rpc_if' 'get' access method is not implemented");
    return TE_OS_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
}

/**
 * Set XEN RPC interface name.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         RPC interface name to set
 *
 * @return              Status code
 */
static te_errno
xen_rpc_if_set(unsigned int gid, char const *oid, char const *value)
{
#if XEN_SUPPORT
    unsigned int u;
    unsigned int limit = dom_u_limit();
    size_t       len   = strlen(value);
#endif

    UNUSED(gid);
    UNUSED(oid);

#if XEN_SUPPORT
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
            ERROR("Failed to change XEN RPC interface name: "
                  "domU(s) exist(s)");
            return TE_RC(TE_TA_UNIX, TE_EBUSY);
        }

    /* Check whether XEN RPC interface name fits XEN path storage */
    if (len >= sizeof(xen_rpc_if))
    {
        ERROR("XEN RPC interface name is too long");
        return TE_RC(TE_TA_UNIX, TE_E2BIG);
    }

    memcpy(xen_rpc_if, value, len + 1);
    return 0;
#else
#warning '/agent/xen/rpc_if' 'set' access method is not implemented
    UNUSED(value);
    ERROR("'/agent/xen/rpc_if' 'set' access method is not implemented");
    return TE_OS_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
}

/**
 * Get XEN domU base MAC address template.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         storage for base MAC address to be filled in
 *
 * @return              Status code
 */
static te_errno
xen_base_mac_addr_get(unsigned int gid, char const *oid, char *value)
{
    UNUSED(gid);
    UNUSED(oid);

#if XEN_SUPPORT
    strcpy(value, xen_base_mac_addr);
    return 0;
#else
    UNUSED(value);
#warning '/agent/xen/base_mac_addr' 'get' \
access method is not implemented
    ERROR("'/agent/xen/base_mac_addr' 'get' "
          "access method is not implemented");
    return TE_OS_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
}

/**
 * Set XEN domU base MAC address template.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         Base MAC address to set
 *
 * @return              Status code
 */
static te_errno
xen_base_mac_addr_set(unsigned int gid, char const *oid, char const *value)
{
#if XEN_SUPPORT
    unsigned int u;
    unsigned int limit = dom_u_limit();
    size_t       len   = strlen(value);
#endif

    UNUSED(gid);
    UNUSED(oid);

#if XEN_SUPPORT
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
            ERROR("Failed to change XEN base MAC address template: "
                  "domU(s) exist(s)");
            return TE_RC(TE_TA_UNIX, TE_EBUSY);
        }

    /* Check whether XEN base MAC address fits XEN path storage */
    if (len >= sizeof(xen_rpc_if))
    {
        ERROR("XEN base MAC address template is too long");
        return TE_RC(TE_TA_UNIX, TE_E2BIG);
    }

    memcpy(xen_base_mac_addr, value, len + 1);
    return 0;
#else
#warning '/agent/xen/base_mac_addr' 'set' \
access method is not implemented
    UNUSED(value);
    ERROR("'/agent/xen/base_mac_addr' 'set' "
          "access method is not implemented");
    return TE_OS_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
}

#if XEN_SUPPORT
static te_errno
xen_executive(char const *cmd)
{
    int        fd;
    int        st;
    ssize_t    rd  = 0;
    pid_t      pid = te_shell_cmd(cmd, -1, NULL, &fd, NULL);
    te_errno   rc  = 0;

    if (pid == -1)
        return TE_OS_RC(TE_TA_UNIX, errno);

    ta_waitpid(pid, &st, 0);

    if (st != 0 || (rd = read(fd, buf, sizeof(buf) - 1)) < 0)
        rc = TE_OS_RC(TE_TA_UNIX, errno);

    close(fd);

    while (rd > 0 && buf[rd - 1] == '\n')
        rd--;

    buf[rd] = '\0';
    return rc;
}

static te_errno
xen_accel_get_executive(te_bool *status)
{
    char const pt[] = "sfc_netback";
    te_errno   rc   = xen_executive("lsmod | grep -w ^sfc_netback "
                                    "2> /dev/null | awk '{print$1}'");

    if (rc == 0)
        *status = strncmp(buf, pt, sizeof(pt) - 1) == 0 ? TRUE : FALSE;

    return rc;
}
#endif

/**
 * Get XEN dom0 acceleration status
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         storage for acceleration status
 *
 * @return              Status code
 */
static te_errno
xen_accel_get(unsigned int gid, char const *oid, char *value)
{
#if XEN_SUPPORT
    te_bool  status;
    te_errno rc;
#endif

    UNUSED(gid);
    UNUSED(oid);

#if XEN_SUPPORT
    /* The agent must run within dom0 */
    if (!is_within_dom0())
    {
        value = "0";
        return 0;
    }

    if ((rc = xen_accel_get_executive(&status)) == 0)
        strcpy(value, status ? "1" : "0");

    return rc;
#else
    UNUSED(value);
#warning '/agent/xen/accel' 'get' \
access method is not implemented
    ERROR("'/agent/xen/accel' 'get' "
          "access method is not implemented");
    return TE_OS_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
}

/**
 * Set XEN dom0 acceleration status
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         acceleration status
 *
 * @return              Status code
 */
static te_errno
xen_accel_set(unsigned int gid, char const *oid, char const *value)
{
#if XEN_SUPPORT
    te_bool     status;
    te_bool     needed_status = strcmp(value, "0") == 0 ? FALSE : TRUE;
    te_errno    rc;
    char const *cmd = NULL;
#endif

    UNUSED(gid);
    UNUSED(oid);

#if XEN_SUPPORT
    /* The agent must run within dom0 */
    if (!is_within_dom0())
    {
        ERROR("Agent runs NOT within dom0");
        return TE_RC(TE_TA_UNIX, TE_EFAIL);
    }

    if ((rc = xen_accel_get_executive(&status)) == 0)
    {
        switch (status)
        {
            case FALSE:
                if (needed_status)
                    cmd = "/sbin/modprobe sfc_netback";

                break;

            default:
                if (!needed_status)
                    cmd = "/sbin/rmmod sfc_netback";

                break;
        }

        if (cmd != NULL)
        {
            if (ta_system(cmd) != 0)
            {
                rc = TE_OS_RC(TE_TA_UNIX, errno);
            }
            else if ((rc = xen_accel_get_executive(&status)) == 0)
            {
                if ((needed_status && !status) ||
                    (!needed_status && status))
                {
                    ERROR("Failed to set acceleration %s",
                          status ? "ON" : "OFF");
                    rc = TE_RC(TE_TA_UNIX, TE_EFAIL);
                }
            }
        }
    }

    return rc;
#else
#warning '/agent/xen/accel' 'set' \
access method is not implemented
    UNUSED(value);
    ERROR("'/agent/xen/accel' 'set' "
          "access method is not implemented");
    return TE_OS_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
}

/**
 * Perform XEN dom0 initialization/cleanup
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         init "command"
 *
 * @return              Status code
 */
static te_errno
xen_init_set(unsigned int gid, char const *oid, char const *value)
{
#if XEN_SUPPORT
    char const  *cmd_list = "/usr/sbin/xm list | awk '{print$1}' | "
                            "grep -v 'Name' | grep -v 'Domain-0'";
    char const  *cmd_shut = "for dom_u in "
                            "`/usr/sbin/xm list | awk '{print $1}' | "
                            "grep -v 'Name' | grep -v 'Domain-0'`; "
                            "do /usr/sbin/xm shutdown $dom_u; done";
    char const  *cmd_dest = "for dom_u in "
                            "`/usr/sbin/xm list | awk '{print $1}' | "
                            "grep -v 'Name' | grep -v 'Domain-0'`; "
                            "do /usr/sbin/xm destroy $dom_u; done";

    te_errno     rc;
    unsigned int u;
#endif

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);

#if XEN_SUPPORT
    /* The agent must run within dom0 */
    if (!is_within_dom0())
    {
        ERROR("Agent runs NOT within dom0");
        return TE_RC(TE_TA_UNIX, TE_EFAIL);
    }

    if (*xen_path == '\0')
    {
        ERROR("XEN path is NOT set");
        return TE_RC(TE_TA_UNIX, TE_EFAIL);
    }

    if ((rc =  xen_executive(cmd_list)) != 0)
        return rc;

    if (*buf == '\0')
        goto clear_xen_sub_path;

    RING("Shutting down domUs:\n%s", buf);

    if ((rc =  xen_executive(cmd_shut)) != 0)
        return rc;

    for (u = 0; u < 9; u++)
    {
        if ((rc =  xen_executive(cmd_list)) != 0)
            return rc;

        if (*buf == '\0')
            goto clear_xen_sub_path;

        sleep(3);
    }

    RING("Destroying domUs:\n%s", buf);

    if ((rc =  xen_executive(cmd_dest)) != 0)
        return rc;

    for (u = 0; u < 9; u++)
    {
        if ((rc =  xen_executive(cmd_list)) != 0)
            return rc;

        if (*buf == '\0')
            goto clear_xen_sub_path;

        sleep(3);
    }

    ERROR("Failed to shutdown and then destroy all domUs");
    return TE_RC(TE_TA_UNIX, TE_EFAIL);

clear_xen_sub_path:

    TE_SPRINTF(buf, "%s/%s/*", xen_path, xen_subpath);

    if ((rc = xen_rmfr(buf)) != 0)
        ERROR("Failed to clear XEN subpath '%s'", buf);

    return rc;
#else
#warning '/agent/xen/init' 'set' \
init method is not implemented
    UNUSED(value);
    ERROR("'/agent/xen/init' 'set' "
          "init method is not implemented");
    return TE_OS_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
}

/**
 * Get real physical interface name by the name of the virtual one.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         storage for real interface name to be filled in
 * @param xen           name of the XEN node (empty, unused)
 * @param interface     name of the virtual interface
 *
 * @return              Status code
 */
static te_errno
xen_interface_get(unsigned int gid, char const *oid, char *value,
                  char const *xen, char const *interface)
{
#if XEN_SUPPORT
    unsigned int u;
#endif

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(xen);

#if XEN_SUPPORT
    FIND_INTERFACE(interface, u);

    strcpy(value, interface_slot[u].ph_name);
    return 0;
#else
#warning '/agent/xen/interface' 'get' \
access method is not implemented
    ERROR("'/agent/xen/interface' 'get' "
          "access method is not implemented");
    return TE_OS_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
}

/**
 * Set real physical interface name by the name of the virtual one.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         real interface name to set
 * @param xen           name of the XEN node (empty, unused)
 * @param interface     name of the virtual interface
 *
 * @return              Status code
 */
static te_errno
xen_interface_set(unsigned int gid, char const *oid, char const *value,
                  char const *xen, char const *interface)
{
#if XEN_SUPPORT
    char const  *ph_name;
    unsigned int u;
    unsigned int limit = dom_u_limit();
#endif

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(xen);

#if XEN_SUPPORT
    /* Check whether domUs exist */
    for (u = 0; u < limit; u++)
        if (dom_u_slot[u].name != NULL)
        {
            ERROR("Failed to change XEN bridge name: "
                  "domU(s) exist(s)");
            return TE_RC(TE_TA_UNIX, TE_EBUSY);
        }

    FIND_INTERFACE(interface, u);

    if ((ph_name = strdup(value)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    free((void *)interface_slot[u].ph_name);
    interface_slot[u].ph_name = ph_name;
    return 0;
#else
#warning '/agent/xen/interface' 'set' \
access method is not implemented
    ERROR("'/agent/xen/interface' 'set' "
          "access method is not implemented");
    return TE_OS_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
}

/**
 * Add new XEN virtual tested interface.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         initializing value (not used)
 * @param xen           name of the XEN node (empty, unused)
 * @param interface     name of the XEN virtual tested interface to add
 *
 * @return              Status code
 */
static te_errno
xen_interface_add(unsigned int gid, char const *oid, char const *value,
                  char const *xen, char const *interface)
{
#if XEN_SUPPORT
    unsigned int u;
    unsigned int limit = dom_u_limit();
#endif

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(xen);

#if XEN_SUPPORT
    if (!is_within_dom0())
    {
        ERROR("Agent runs NOT within dom0");
        return TE_RC(TE_TA_UNIX, TE_EFAIL);
    }

    /* Check whether domUs exist */
    for (u = 0; u < limit; u++)
        if (dom_u_slot[u].name != NULL)
        {
            ERROR("Failed to delete XEN virtual tested interface: "
                  "domU(s) exist(s)");
            return TE_RC(TE_TA_UNIX, TE_EBUSY);
        }

    if ((u = find_interface(interface)) < interface_limit())
    {
        ERROR("Failed to add interface %s: it already exists", interface);
        return TE_RC(TE_TA_UNIX, TE_EEXIST);
    }

    /* Find an empty slot */
    for (u = 0, limit = interface_limit(); u < limit; u++)
        if (interface_slot[u].if_name == NULL)
            break;

    /* If an empty slot is NOT found */
    if (u == limit)
    {
        ERROR("Failed to add interface %s: all interface slots are taken",
              interface);
        return TE_RC(TE_TA_UNIX, TE_E2BIG);
    }

    if ((interface_slot[u].br_name = strdup("")) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    if ((interface_slot[u].ph_name = strdup(value)) == NULL)
    {
        free((void *)interface_slot[u].br_name);
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }

    if ((interface_slot[u].if_name = strdup(interface)) == NULL)
    {
        free((void *)interface_slot[u].ph_name);
        free((void *)interface_slot[u].br_name);
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }

    return 0;
#else
#warning '/agent/xen/interface' 'add' \
access method is not implemented
    ERROR("'/agent/xen/interface' 'add' "
          "access method is not implemented");
    return TE_OS_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
}

/**
 * Delete XEN virtual tested interface.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param xen           name of the XEN node (empty, unused)
 * @param interface     name of the XEN virtual tested interface to delete
 *
 * @return              Status code
 */
static te_errno
xen_interface_del(unsigned int gid, char const *oid, char const *xen,
                  char const *interface)
{
#if XEN_SUPPORT
    unsigned int u;
    unsigned int limit = dom_u_limit();
#endif

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(xen);

#if XEN_SUPPORT
    /* Check whether domUs exist */
    for (u = 0; u < limit; u++)
        if (dom_u_slot[u].name != NULL)
        {
            ERROR("Failed to delete XEN virtual tested interface: "
                  "domU(s) exist(s)");
            return TE_RC(TE_TA_UNIX, TE_EBUSY);
        }

    FIND_INTERFACE(interface, u);

    free((void *)interface_slot[u].br_name);
    free((void *)interface_slot[u].ph_name);
    free((void *)interface_slot[u].if_name);
    interface_slot[u].if_name = NULL;
    return 0;
#else
#warning '/agent/xen/interface' 'del' i\
access method is not implemented
    ERROR("'/agent/xen/interface' 'del' "
          "access method is not implemented");
    return TE_OS_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
}

/**
 * List XEN virtual tested interfaces.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param list          address of a pointer to storage allocated
 *                      for the list pointer is initialized with
 *
 * @return              Status code
 */
static te_errno
xen_interface_list(unsigned int gid, char const *oid, char **list)
{
#if XEN_SUPPORT
    unsigned int u;
    unsigned int limit = interface_limit();
    unsigned int len = 0;
    char        *ptr;
#endif

    UNUSED(gid);
    UNUSED(oid);

#if XEN_SUPPORT
    /* Count the whole length of interface names plus one per name */
    for (u = 0; u < limit; u++)
        if (interface_slot[u].if_name != NULL)
            len += strlen(interface_slot[u].if_name) + 1;

    if (len == 0)
    {
        *list = NULL;
        return 0;
    }

    if ((ptr = malloc(len)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    if (list != NULL)
        *(*list = ptr) = '\0';

    /**
     * Fill in the list with existing domU names
     * separated with spaces except the last one
     */
    for (u = 0; u < limit; u++)
    {
        char const *name = interface_slot[u].if_name;

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
#else
#warning '/agent/xen/interface' 'list' \
access method is not implemented
    ERROR("'/agent/xen/interface' 'list' "
          "access method is not implemented");
    return TE_OS_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
}

/**
 * Get the name of the XEN bridge, which
 * virtual tested interface is connected to.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         storage for XEN bridge name to be filled in
 * @param interface     virtual tested interface name
 *
 * @return              Status code
 */
static te_errno
xen_interface_bridge_get(unsigned int gid, char const *oid, char *value,
                         char const *xen, char const *interface)
{
#if XEN_SUPPORT
    unsigned int u;
#endif

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);
    UNUSED(xen);

#if XEN_SUPPORT
    FIND_INTERFACE(interface, u);

    strcpy(value, interface_slot[u].br_name);
    return 0;
#else
#warning '/agent/xen/interface/bridge' 'get' \
access method is not implemented
    ERROR("'/agent/xen/interface/bridge' 'get' "
          "access method is not implemented");
    return TE_OS_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
}

/**
 * Set the name of the XEN bridge, which
 * virtual tested interface is connected to.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         XEN bridge name to set
 * @param interface     virtual tested interface name
 *
 * @return              Status code
 */
static te_errno
xen_interface_bridge_set(unsigned int gid, char const *oid,
                         char const *value, char const *xen,
                         char const *interface)
{
#if XEN_SUPPORT
    char const  *br_name;
    unsigned int u;
    unsigned int limit = dom_u_limit();
#endif

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(xen);

#if XEN_SUPPORT
    /* Check whether domUs exist */
    for (u = 0; u < limit; u++)
        if (dom_u_slot[u].name != NULL)
        {
            ERROR("Failed to change XEN bridge name: "
                  "domU(s) exist(s)");
            return TE_RC(TE_TA_UNIX, TE_EBUSY);
        }

    FIND_INTERFACE(interface, u);

    if ((br_name = strdup(value)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    free((void *)interface_slot[u].br_name);
    interface_slot[u].br_name = br_name;
    return 0;
#else
#warning '/agent/xen/interface/bridge' 'set' \
access method is not implemented
    UNUSED(value);
    UNUSED(interface);
    ERROR("'/agent/xen/interface/bridge' 'set' "
          "access method is not implemented");
    return TE_OS_RC(TE_TA_UNIX, TE_ENOSYS);
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
#if XEN_SUPPORT
    unsigned int u;
    struct stat  st;
#endif

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(xen);

#if XEN_SUPPORT
    FIND_DOM_U(dom_u, u);

    strcpy(value, stat(get_dom_u_path(dom_u), &st) == 0 ? "1" : "0");
    return 0;
#else
#warning '/agent/xen/dom_u' 'get' access method is not implemented
    ERROR("'/agent/xen/dom_u' 'get' access method is not implemented");
    return TE_OS_RC(TE_TA_UNIX, TE_ENOSYS);
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
#if XEN_SUPPORT
    unsigned int u;
    struct stat  st;
    int          sys;
    te_bool      to_set = strcmp(value, "1") == 0;
    te_bool      is_set;
    te_errno     rc = 0;

    char const *const dom_u_path = get_dom_u_path(dom_u);
#endif

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(xen);

#if XEN_SUPPORT
    FIND_DOM_U(dom_u, u);

    is_set = stat(dom_u_path, &st) == 0;

    /* If desired state is already exists, do nothing */
    if ((is_set && to_set) || (!is_set && !to_set))
        return 0;

    /* If not to set then remove domU directory and disk images */
    if (!to_set)
        goto cleanup1;

    /* Otherwise, create domU directory and all necessary images */
    if (mkdir(dom_u_path, S_IRWXU | S_IRWXG | S_IRWXO) == -1)
    {
        ERROR("Failed to create domU directory %s", dom_u_path);
        rc = TE_RC(TE_TA_UNIX, TE_EFAIL);
        goto cleanup0;
    }

    if (chmod(dom_u_path, S_IRWXU | S_IRWXG | S_IRWXO) == -1)
    {
        ERROR("Failed to chmod domU directory %s", dom_u_path);
        rc = TE_RC(TE_TA_UNIX, TE_EFAIL);
        goto cleanup1;
    }

    /* FIXME: Non "ta_system" implementation is needed*/
    TE_SPRINTF(buf, "cp --sparse=always %s/%s %s/%s",
               xen_path, xen_dsktpl, dom_u_path, xen_dskimg);

    if ((sys = ta_system(buf)) != 0 && !(sys == -1 && errno == ECHILD))
    {
        rc = TE_RC(TE_TA_UNIX, TE_EFAIL);
        goto cleanup1;
    }

    TE_SPRINTF(buf, "%s/%s", dom_u_path, xen_dskimg);

    if (chmod(buf, S_IRUSR | S_IWUSR |
                   S_IRGRP | S_IWGRP |
                   S_IROTH | S_IWOTH) == -1)
    {
        ERROR("Failed to chmod domU disk image %s", buf);
        rc = TE_RC(TE_TA_UNIX, TE_EFAIL);
        goto cleanup1;
    }

    /* FIXME: Non "ta_system" implementation is needed*/
    TE_SPRINTF(buf, "dd if=/dev/zero of=%s/%s bs=1k seek=131071 "
               "count=1 2>/dev/null", dom_u_path, xen_swpimg);

    if ((sys = ta_system(buf)) != 0 && !(sys == -1 && errno == ECHILD))
    {
        rc = TE_RC(TE_TA_UNIX, TE_EFAIL);
        goto cleanup1;
    }

    TE_SPRINTF(buf, "%s/%s", dom_u_path, xen_swpimg);

    if (chmod(buf, S_IRUSR | S_IWUSR |
                   S_IRGRP | S_IWGRP |
                   S_IROTH | S_IWOTH) == -1)
    {
        ERROR("Failed to chmod domU swap image %s", buf);
        rc = TE_RC(TE_TA_UNIX, TE_EFAIL);
        goto cleanup1;
    }

    /* FIXME: Non "ta_system" implementation is needed*/
    TE_SPRINTF(buf, "/sbin/mkswap %s/%s > /dev/null",
               dom_u_path, xen_swpimg);

    if ((sys = ta_system(buf)) != 0 && !(sys == -1 && errno == ECHILD))
    {
        rc = TE_RC(TE_TA_UNIX, TE_EFAIL);
        goto cleanup1;
    }

    if ((rc = xen_fill_file_in_disk_image(dom_u, "/etc/udev/rules.d/"
                                          "z25_persistent-net.rules",
                                          "")) == 0)
    {
        goto cleanup0;
    }

cleanup1:
    /* Erase domU directory and disk images unconditionally */
    xen_rmfr(dom_u_path);

cleanup0:
    return rc;
#else
#warning '/agent/xen/dom_u' 'set' access method is not implemented
    ERROR("'/agent/xen/dom_u' 'set' access method is not implemented");
    return TE_OS_RC(TE_TA_UNIX, TE_ENOSYS);
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
#if XEN_SUPPORT
    unsigned int u;
    unsigned int v;
    unsigned int limit = dom_u_limit();
    te_errno     rc    = 0;
#endif

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(xen);

#if XEN_SUPPORT
    if (!is_within_dom0())
    {
        ERROR("Agent runs NOT within dom0");
        return TE_RC(TE_TA_UNIX, TE_EFAIL);
    }

    if (*xen_path == '\0')
    {
        ERROR("Failed to add '%s' domU since XEN path is not set", dom_u);
        return TE_RC(TE_TA_UNIX, TE_EFAIL);
    }

    if (*dom_u == '\0')
    {
        ERROR("Failed to add '%s' domU: domU name is empty", dom_u);
        return TE_RC(TE_TA_UNIX, TE_EEXIST);
    }

    if ((u = find_dom_u(dom_u)) < dom_u_limit())
    {
        ERROR("Failed to add domU %s: it already exists", dom_u);
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

    /* Assign here initial values (modified later from within TAPI) */
    dom_u_slot[u].status = DOM_U_STATUS_NON_RUNNING;
    dom_u_slot[u].memory = 0;

    rc = TE_RC(TE_TA_UNIX, TE_ENOMEM);

    strcpy(dom_u_slot[u].ip_addr, init_ip_addr);
    strcpy(dom_u_slot[u].mac_addr, init_mac_addr);

    for (v = 0, limit = bridge_limit(); v < limit; v++)
        dom_u_slot[u].bridge_slot[v].br_name = NULL;

    dom_u_slot[u].migrate_kind = 0;

    /* Try to set requested presence of directory/images state of domU */
    if ((rc = dom_u_set(gid, oid, value, xen, dom_u)) == 0)
        return 0;

    free((void *)dom_u_slot[u].name);
    dom_u_slot[u].name = NULL;
    return rc;
#else
#warning '/agent/xen/dom_u' 'add' access method is not implemented
    ERROR("'/agent/xen/dom_u' 'add' access method is not implemented");
    return TE_OS_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
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
#if XEN_SUPPORT
    unsigned int u;
    unsigned int v;
    unsigned int limit = bridge_limit();
#endif

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(xen);

#if XEN_SUPPORT
    FIND_DOM_U(dom_u, u);

    for (v = 0; v < limit; v++)
    {
        if (dom_u_slot[u].bridge_slot[v].br_name != NULL)
        {
            free((void *)dom_u_slot[u].bridge_slot[v].if_name);
            free((void *)dom_u_slot[u].bridge_slot[v].br_name);
        }
    }

    free((void *)dom_u_slot[u].name);
    dom_u_slot[u].name = NULL;
    return 0;
#else
#warning '/agent/xen/dom_u' 'del' access method is not implemented
    ERROR("'/agent/xen/dom_u' 'del' access method is not implemented");
    return TE_OS_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
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
#if XEN_SUPPORT
    unsigned int u;
    unsigned int limit = dom_u_limit();
    unsigned int len = 0;
    char        *ptr;
#endif

    UNUSED(gid);
    UNUSED(oid);

#if XEN_SUPPORT
    /* Count the whole length of domU names plus one per name */
    for (u = 0; u < limit; u++)
        if (dom_u_slot[u].name != NULL)
            len += strlen(dom_u_slot[u].name) + 1;

    if (len == 0)
    {
        *list = NULL;
        return 0;
    }

    if ((ptr = malloc(len)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    if (list != NULL)
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
#else
#warning '/agent/xen/dom_u' 'list' access method is not implemented
    ERROR("'/agent/xen/dom_u' 'list' access method is not implemented");
    return TE_OS_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
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
#if XEN_SUPPORT
    char const  *s;
    unsigned int u;
#endif

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(xen);

#if XEN_SUPPORT
    FIND_DOM_U(dom_u, u);

    if ((s = dom_u_status_to_string(dom_u_slot[u].status)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    strcpy(value, s);
    return 0;
#else
#warning '/agent/xen/dom_u/status' 'get' access method is not implemented
     ERROR("'/agent/xen/dom_u/status' 'get' access method is not " \
           "implemented");
    return TE_OS_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
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
#if XEN_SUPPORT
    unsigned int u;
    FILE        *f;
    status_t     status = dom_u_status_string_to_status(value);
    te_errno     rc     = 0;

    char const *const dom_u_path = get_dom_u_path(dom_u);
#endif

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(xen);

#if XEN_SUPPORT
    if (status == DOM_U_STATUS_ERROR)
    {
        rc = TE_RC(TE_TA_UNIX, TE_EINVAL);
        goto cleanup0;
    }

    FIND_DOM_U(dom_u, u);

    /* If nothing to do */
    if ((dom_u_slot[u].status == status))
        goto cleanup0;

    /* "Non-running" -> "<another status>" transition */
    if (dom_u_slot[u].status == DOM_U_STATUS_NON_RUNNING &&
        (rc = check_dom_u_is_initialized_properly(u)) != 0)
    {
        goto cleanup0;
    }

    /* "Non-running" -> "migrated-saved" transition */
    if (dom_u_slot[u].status == DOM_U_STATUS_NON_RUNNING &&
        status == DOM_U_STATUS_MIGRATED_SAVED)
    {
        struct stat st;

        TE_SPRINTF(buf, "%s/%s", dom_u_path, xen_dskimg);

        if (stat(buf, &st) != 0 || !S_ISREG(st.st_mode))
        {
            ERROR("Failed to accept migrated saved '%s' domU", dom_u);
            rc = TE_RC(TE_TA_UNIX, TE_EFAIL);
            goto cleanup0;
        }

        goto cleanup1; /** Imitation of 'break' statement */
    }

    /* "Non-running" -> "migrated-running" transition */
    if (dom_u_slot[u].status == DOM_U_STATUS_NON_RUNNING &&
        status == DOM_U_STATUS_MIGRATED_RUNNING)
    {
        size_t  len = strlen(dom_u);

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
                dom_u_slot[u].status = DOM_U_STATUS_MIGRATED_RUNNING;
                break;
            }
        }

        pclose(f);

        if (dom_u_slot[u].status != DOM_U_STATUS_MIGRATED_RUNNING)
        {
            ERROR("Failed to accept migrated running '%s' domU", dom_u);
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

        /* Memory size must be set for domU */
        if (dom_u_slot[u].memory == 0)
        {
            ERROR("DomU %s memory size is not set", dom_u);
            rc = TE_RC(TE_TA_UNIX, TE_EFAIL);
            goto cleanup0;
        }

        /* Create XEN domU configuration file */
        TE_SPRINTF(buf, "%s/conf.cfg", dom_u_path);

        if ((f = fopen(buf, "w")) == NULL)
        {
            ERROR("Failed to (re)create domU %s configuration file %s",
                  dom_u, buf);
            rc = TE_RC(TE_TA_UNIX, TE_EFAIL);
            goto cleanup0;
        }

        if (fprintf(f, "kernel='%s/%s'\n", xen_path, xen_kernel)  < 0 ||
            fprintf(f, "ramdisk='%s/%s'\n", xen_path, xen_initrd) < 0 ||
            fprintf(f, "memory='%u'\n", dom_u_slot[u].memory)     < 0 ||
            fprintf(f, "root='/dev/sda1 ro'\n")                   < 0 ||
            fprintf(f, "disk=[ 'file:%s/%s,sda1,w', "
                       "'file:%s/%s,sda2,w' ]\n",
                       dom_u_path, xen_dskimg,
                       dom_u_path, xen_swpimg)               < 0 ||
            fprintf(f, "name='%s'\n", dom_u)                      < 0 ||
            (rc = prepare_dom_u_interfaces_config(u)) != 0 ||
            fprintf(f, "%s", buf) < 0 ||
            fprintf(f, "on_poweroff = 'destroy'\n") < 0 ||
            fprintf(f, "on_reboot   = 'restart'\n") < 0 ||
            fprintf(f, "on_crash    = 'restart'\n") < 0 ||
            fflush(f) != 0)
        {
            if (rc == 0)
                rc = TE_RC(TE_TA_UNIX, TE_EFAIL);

            goto cleanup2;
        }

cleanup2:
        if (fclose(f) != 0)
        {
            if (rc == 0)
                rc = TE_RC(TE_TA_UNIX, TE_EFAIL);
        }

        if (rc != 0)
            goto cleanup0;

        /* Create list of interfaces */
        if ((rc = prepare_persistent_net_rules(u)) != 0 ||
            (rc = xen_fill_file_in_disk_image(dom_u, "/etc/udev/rules.d/"
                                              "z25_persistent-net.rules",
                                              buf)) != 0)
        {
            goto cleanup0;
        }

        /* Create domU "/etc/network/interfaces" file */
        if ((rc = prepare_network_interfaces_config(u)) != 0 ||
            (rc = xen_fill_file_in_disk_image(dom_u,
                                              "/etc/network/interfaces",
                                              buf)) != 0)
        {
            goto cleanup0;
        }

        /* Starting domU */
        TE_SPRINTF(buf, "xm create %s/conf.cfg", dom_u_path);

        if (ta_system(buf) != 0)
        {
            rc = TE_RC(TE_TA_UNIX, TE_EFAIL);
            goto cleanup0;
        }

        /* Really domU is stil booting here */
        goto cleanup1; /** Imitation of 'break' statement */
    }

    /* "Running/migrated-running" -> "non-running" transition */
    if ((dom_u_slot[u].status == DOM_U_STATUS_RUNNING ||
         dom_u_slot[u].status == DOM_U_STATUS_MIGRATED_RUNNING) &&
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

    /* "Running/migrated-running" -> "saved" transition */
    if ((dom_u_slot[u].status == DOM_U_STATUS_RUNNING ||
         dom_u_slot[u].status == DOM_U_STATUS_MIGRATED_RUNNING) &&
        status == DOM_U_STATUS_SAVED)
    {
        TE_SPRINTF(buf, "xm save %s %s/saved.img", dom_u, dom_u_path);

        if (ta_system(buf) != 0)
        {
            rc = TE_RC(TE_TA_UNIX, TE_EFAIL);
            goto cleanup0;
        }

        /* FIXME: stupid sleep should be replaced with smarter code */
        sleep(10);
        goto cleanup1; /** Imitation of 'break' statement */
    }

    /* "Saved/migrated-saved" -> "running" transition */
    if ((dom_u_slot[u].status == DOM_U_STATUS_SAVED ||
         dom_u_slot[u].status == DOM_U_STATUS_MIGRATED_SAVED) &&
        status == DOM_U_STATUS_RUNNING)
    {
        TE_SPRINTF(buf, "xm restore %s/saved.img", dom_u_path);

        if (ta_system(buf) != 0)
        {
            rc = TE_RC(TE_TA_UNIX, TE_EFAIL);
            goto cleanup0;
        }

        /* FIXME: stupid sleep should be replaced with smarter code */
        sleep(25);
        goto entry1; /** Imitation of absence of the 'break' statement */
    }

    /* "Saved/migrated-saved" -> "non-running" transition */
    if ((dom_u_slot[u].status == DOM_U_STATUS_SAVED ||
         dom_u_slot[u].status == DOM_U_STATUS_MIGRATED_SAVED) &&
        status == DOM_U_STATUS_NON_RUNNING)
    {
entry1:
        TE_SPRINTF(buf, "%s/saved.img", dom_u_path);

        /* Error here is not critical */
        if (unlink(buf) == -1)
            ERROR("Failed to unlink %s/saved.img", dom_u_path);

        goto cleanup1; /** Imitation of 'break' statement */
    }

    /* All still unserviced transitions are erroneous */
    rc = TE_RC(TE_TA_UNIX, TE_EINVAL);
    goto cleanup0;

cleanup1: /** This label is used in case of success */
    dom_u_slot[u].status = status;

cleanup0: /** This label is used in case of an error */
    return rc;
#else
#warning '/agent/xen/dom_u_status' 'set' access method is not implemented
    ERROR("'/agent/xen/dom_u_status' 'set' access method is not " \
          "implemented");
    return TE_OS_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
}

/**
 * Get domU memory size.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         storage for memory size to be filled in
 * @param xen           name of the XEN node (empty, unused)
 * @param dom_u         name of the domU to get memory size of
 *
 * @return              Status code
 */
static te_errno
dom_u_memory_get(unsigned int gid, char const *oid, char *value,
                 char const *xen, char const *dom_u)
{
#if XEN_SUPPORT
    unsigned int u;
#endif

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(xen);

#if XEN_SUPPORT
    FIND_DOM_U(dom_u, u);

    sprintf(value, "%u", dom_u_slot[u].memory);
    return 0;
#else
#warning '/agent/xen/dom_u/memory' 'get' access method is not implemented
     ERROR("'/agent/xen/dom_u/memory' 'get' access method is not " \
           "implemented");
    return TE_OS_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
}

/**
 * Set (change) domU memory size.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         memory size to set
 * @param xen           name of the XEN node (empty, unused)
 * @param dom_u         name of the domU to set memory size of
 *
 * @return              Status code
 */
static te_errno
dom_u_memory_set(unsigned int gid, char const *oid, char const *value,
                 char const *xen, char const *dom_u)
{
#if XEN_SUPPORT
    int          mem = atoi(value);
    unsigned int u;
#endif

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(xen);

#if XEN_SUPPORT
    FIND_DOM_U(dom_u, u);

    if (mem < 0)
    {
        ERROR("Invalid memory size value = %d", mem);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    dom_u_slot[u].memory = mem;
    return 0;
#else
#warning '/agent/xen/dom_u/memory' 'get' access method is not implemented
     ERROR("'/agent/xen/dom_u/memory' 'get' access method is not " \
           "implemented");
    return TE_OS_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
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
#if XEN_SUPPORT
    unsigned int u;
#endif

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(xen);

#if XEN_SUPPORT
    FIND_DOM_U(dom_u, u);

    strcpy(value, dom_u_slot[u].ip_addr);
    return 0;
#else
#warning '/agent/xen/dom_u/ip_addr' 'get' access method is not implemented
    ERROR("'/agent/xen/dom_u/ip_addr' 'get' access method is not " \
          "implemented");
    return TE_OS_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
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
#if XEN_SUPPORT
    unsigned int u;
    size_t       len = strlen(value);
#endif

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(xen);

#if XEN_SUPPORT
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
#else
#warning '/agent/xen/dom_u/ip_addr' 'set' access method is not implemented
    ERROR("'/agent/xen/dom_u/ip_addr' 'set' access method is not " \
          "implemented");
    return TE_OS_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
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
#if XEN_SUPPORT
    unsigned int u;
#endif

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(xen);

#if XEN_SUPPORT
    FIND_DOM_U(dom_u, u);

    strcpy(value, dom_u_slot[u].mac_addr);
    return 0;
#else
#warning '/agent/xen/dom_u/mac_addr' 'get' access method is not implemented
    ERROR("'/agent/xen/dom_u/mac_addr' 'get' access method is not " \
          "implemented");
    return TE_OS_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
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
#if XEN_SUPPORT
    enum { ether_bytes = 6 };

    unsigned int u;
    size_t       len = strlen(value);
#endif

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(xen);

#if XEN_SUPPORT
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
#else
#warning '/agent/xen/dom_u/mac_addr' 'set' access method is not implemented
    ERROR("'/agent/xen/dom_u/mac_addr' 'set' access method is not " \
          "implemented");
    return TE_OS_RC(TE_TA_UNIX, TE_ENOSYS);
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
dom_u_bridge_get(unsigned int gid, char const *oid,
                 char *value, char const *xen,
                 char const *dom_u, char const *bridge)
{
#if XEN_SUPPORT
    unsigned int u;
    unsigned int v;
#endif

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(xen);

#if XEN_SUPPORT
    FIND_DOM_U(dom_u, u);
    FIND_BRIDGE(bridge, u, v);

    strcpy(value, dom_u_slot[u].bridge_slot[v].if_name);
    return 0;
#else
#warning '/agent/xen/dom_u/bridge' 'get' access method is not implemented
    ERROR("'/agent/xen/dom_u/bridge' 'get' "
          "access method is not implemented");
    return TE_OS_RC(TE_TA_UNIX, TE_ENOSYS);
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
dom_u_bridge_set(unsigned int gid, char const *oid,
                 char const *value, char const *xen,
                 char const *dom_u, char const *bridge)
{
#if XEN_SUPPORT
    char const  *if_name;
    unsigned int u;
    unsigned int v;
#endif

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(xen);

#if XEN_SUPPORT
    FIND_DOM_U(dom_u, u);
    FIND_BRIDGE(bridge, u, v);

    if ((if_name = strdup(value)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    free((void *)dom_u_slot[u].bridge_slot[v].if_name);
    dom_u_slot[u].bridge_slot[v].if_name = if_name;
    return 0;
#else
#warning '/agent/xen/dom_u/bridge' 'set' access method is not implemented
    ERROR("'/agent/xen/dom_u/bridge' 'set' "
          "access method is not implemented");
    return TE_OS_RC(TE_TA_UNIX, TE_ENOSYS);
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
dom_u_bridge_add(unsigned int gid, char const *oid,
                 char const *value, char const *xen,
                 char const *dom_u, char const *bridge)
{
#if XEN_SUPPORT
    unsigned int u;
    unsigned int v;
    unsigned int limit = bridge_limit();
#endif

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(xen);

#if XEN_SUPPORT
    FIND_DOM_U(dom_u, u);

    if ((v = find_bridge(bridge, u)) < limit)
    {
        ERROR("Failed to add '%s' bridge on '%s' domU: it already "
              "exists", bridge, dom_u);
        return TE_RC(TE_TA_UNIX, TE_EEXIST);
    }

    /* Find an empty slot */
    for (v = 0; v < limit; v++)
        if (dom_u_slot[u].bridge_slot[v].br_name == NULL)
            break;

    /* If an empty slot is NOT found */
    if (v == limit)
    {
        ERROR("Failed to add '%s' bridge on '%s' domU: all bridge "
              "slots are taken", bridge, dom_u);
        return TE_RC(TE_TA_UNIX, TE_E2BIG);
    }

    if ((dom_u_slot[u].bridge_slot[v].if_name = strdup(value)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    if ((dom_u_slot[u].bridge_slot[v].br_name = strdup(bridge)) == NULL)
    {
        free((void *)dom_u_slot[u].bridge_slot[v].if_name);
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }

    strcpy(dom_u_slot[u].bridge_slot[v].ip_addr, init_ip_addr);
    strcpy(dom_u_slot[u].bridge_slot[v].mac_addr, init_mac_addr);
    dom_u_slot[u].bridge_slot[v].accel = FALSE;
    return 0;
#else
#warning '/agent/xen/dom_u/bridge' 'add' access method is not implemented
    ERROR("'/agent/xen/dom_u/bridge' 'add' "
          "access method is not implemented");
    return TE_OS_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
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
dom_u_bridge_del(unsigned int gid, char const *oid,
                 char const *xen, char const *dom_u,
                 char const *bridge)
{
#if XEN_SUPPORT
    unsigned int u;
    unsigned int v;
#endif

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(xen);

#if XEN_SUPPORT
    FIND_DOM_U(dom_u, u);
    FIND_BRIDGE(bridge, u, v);

    free((void *)dom_u_slot[u].bridge_slot[v].if_name);
    free((void *)dom_u_slot[u].bridge_slot[v].br_name);
    dom_u_slot[u].bridge_slot[v].br_name = NULL;
    return 0;
#else
#warning '/agent/xen/dom_u/bridge' 'del' access method is not implemented
    ERROR("'/agent/xen/dom_u/bridge' 'del' "
          "access method is not implemented");
    return TE_OS_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
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
dom_u_bridge_list(unsigned int gid, char const *oid, char **list,
                  char const *xen, char const *dom_u)
{
#if XEN_SUPPORT
    unsigned int u;
    unsigned int v;
    unsigned int limit = bridge_limit();
    unsigned int len = 0;
    char        *ptr;
#endif

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(xen);

#if XEN_SUPPORT
    FIND_DOM_U(dom_u, u);

    /* Count the whole length of domU names plus one per name */
    for (v = 0; v < limit; v++)
        if (dom_u_slot[u].bridge_slot[v].br_name != NULL)
            len += strlen(dom_u_slot[u].bridge_slot[v].br_name) + 1;

    if (len == 0)
    {
        *list = NULL;
        return 0;
    }

    if ((ptr = malloc(len)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    if (list != NULL)
        *(*list = ptr) = '\0';

    /**
     * Fill in the list with existing domU names
     * separated with spaces except the last one
     */
    for (v = 0; v < limit; v++)
    {
        char const *br_name = dom_u_slot[u].bridge_slot[v].br_name;

        if (br_name != NULL)
        {
            size_t len = strlen(br_name);

            if (ptr != *list)
                *ptr++ = ' ';

            memcpy(ptr, br_name, len);
            *(ptr += len) = '\0';
        }
    }

    return 0;
#else
#warning '/agent/xen/dom_u/bridge' 'list' access method is not implemented
    ERROR("'/agent/xen/dom_u/bridge' 'list' "
          "access method is not implemented");
    return TE_OS_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
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
dom_u_bridge_ip_addr_get(unsigned int gid, char const *oid,
                         char *value, char const *xen,
                         char const *dom_u, char const *bridge)
{
#if XEN_SUPPORT
    unsigned int u;
    unsigned int v;
#endif

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(xen);

#if XEN_SUPPORT
    FIND_DOM_U(dom_u, u);
    FIND_BRIDGE(bridge, u, v);

    strcpy(value, dom_u_slot[u].bridge_slot[v].ip_addr);
    return 0;
#else
#warning '/agent/xen/dom_u/bridge/ip_addr' 'get' \
access method is not implemented
    ERROR("'/agent/xen/dom_u/bridge/ip_addr' 'get' "
          "access method is not implemented");
    return TE_OS_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
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
dom_u_bridge_ip_addr_set(unsigned int gid, char const *oid,
                         char const *value, char const *xen,
                         char const *dom_u, char const *bridge)
{
#if XEN_SUPPORT
    unsigned int u;
    unsigned int v;
    size_t       len = strlen(value);
#endif

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(xen);

#if XEN_SUPPORT
    FIND_DOM_U(dom_u, u);
    FIND_BRIDGE(bridge, u, v);

    if (len >= sizeof(dom_u_slot[u].bridge_slot[v].ip_addr))
    {
        ERROR("Too long IP address");
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    /**
     * IP address will be really changed in domU disk image
     * only when transition into "running" status is requested
     */
    strcpy(dom_u_slot[u].bridge_slot[v].ip_addr, value);
    return 0;
#else
#warning '/agent/xen/dom_u/bridge/ip_addr' 'set' \
access method is not implemented
    ERROR("'/agent/xen/dom_u/bridge/ip_addr' 'set' "
          "access method is not implemented");
    return TE_OS_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
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
dom_u_bridge_mac_addr_get(unsigned int gid, char const *oid,
                          char *value, char const *xen,
                          char const *dom_u, char const *bridge)
{
#if XEN_SUPPORT
    unsigned int u;
    unsigned int v;
#endif

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(xen);

#if XEN_SUPPORT
    FIND_DOM_U(dom_u, u);
    FIND_BRIDGE(bridge, u, v);

    strcpy(value, dom_u_slot[u].bridge_slot[v].mac_addr);
    return 0;
#else
#warning '/agent/xen/dom_u/bridge/mac_addr' 'get' \
access method is not implemented
    ERROR("'/agent/xen/dom_u/bridge/mac_addr' 'get' "
          "access method is not implemented");
    return TE_OS_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
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
dom_u_bridge_mac_addr_set(unsigned int gid, char const *oid,
                          char const *value, char const *xen,
                          char const *dom_u, char const *bridge)
{
#if XEN_SUPPORT
    unsigned int u;
    unsigned int v;
    size_t       len = strlen(value);
#endif

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(xen);

#if XEN_SUPPORT
    FIND_DOM_U(dom_u, u);
    FIND_BRIDGE(bridge, u, v);

    if (len >= sizeof(dom_u_slot[u].bridge_slot[v].mac_addr))
    {
        ERROR("Too long MAC address");
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    /**
     * MAC address will be really changed in domU configuration
     * file only when transition into "running" status is requested
     */
    strcpy(dom_u_slot[u].bridge_slot[v].mac_addr, value);
    return 0;
#else
#warning '/agent/xen/dom_u/bridge/mac_addr' 'set' \
access method is not implemented
    ERROR("'/agent/xen/dom_u/bridge/mac_addr' 'set' "
          "access method is not implemented");
    return TE_OS_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
}

/**
 * Get domU acceleration sign.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         storage for acceleration sign to be filled in
 * @param xen           name of the XEN node (empty, unused)
 * @param dom_u         name of the domU to get status of
 *
 * @return              Status code
 */
static te_errno
dom_u_bridge_accel_get(unsigned int gid, char const *oid,
                       char *value, char const *xen,
                       char const *dom_u, char const *bridge)
{
#if XEN_SUPPORT
    unsigned int u;
    unsigned int v;
#endif

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(xen);

#if XEN_SUPPORT
    FIND_DOM_U(dom_u, u);
    FIND_BRIDGE(bridge, u, v);

    strcpy(value, dom_u_slot[u].bridge_slot[v].accel ? "1" : "0");
    return 0;
#else
#warning '/agent/xen/dom_u/bridge/accel' 'get' \
access method is not implemented
    ERROR("'/agent/xen/dom_u/bridge/accel' 'get' "
          "access method is not implemented");
    return TE_OS_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
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
dom_u_bridge_accel_set(unsigned int gid, char const *oid,
                       char const *value, char const *xen,
                       char const *dom_u, char const *bridge)
{
#if XEN_SUPPORT
    unsigned int u;
    unsigned int v;
#endif

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(xen);

#if XEN_SUPPORT
    FIND_DOM_U(dom_u, u);
    FIND_BRIDGE(bridge, u, v);

    dom_u_slot[u].bridge_slot[v].accel = *value != '0' ? TRUE : FALSE;
    return 0;
#else
#warning '/agent/xen/dom_u/bridge/accel' 'set' \
access method is not implemented
    ERROR("'/agent/xen/dom_u/bridge/accel' 'set' "
          "access method is not implemented");
    return TE_OS_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
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
#if XEN_SUPPORT
    unsigned int u;
#endif

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(xen);

#if XEN_SUPPORT
    FIND_DOM_U(dom_u, u);

    TE_SPRINTF(buf, "xm migrate %s %s %s",
               dom_u_slot[u].migrate_kind ? "--live" : "", dom_u, value);

    if (ta_system(buf) != 0)
    {
        ERROR("Failed to migrate domU %s", dom_u);
        return TE_RC(TE_TA_UNIX, TE_EFAIL);
    }

    return 0;
#else
#warning '/agent/xen/dom_u/migrate' 'set' access method is not implemented
    ERROR("'/agent/xen/dom_u/migrate' 'set' access method is not " \
          "implemented");
    return TE_OS_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
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
#if XEN_SUPPORT
    unsigned u;
#endif

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(xen);

#if XEN_SUPPORT
    FIND_DOM_U(dom_u, u);

    strcpy(value, dom_u_slot[u].migrate_kind ? "1" : "0");
    return 0;
#else
#warning '/agent/xen/dom_u/migrate/kind' 'get' access method is not \
         implemented
    ERROR("'/agent/xen/dom_u/migrate/kind' 'get' access method is not " \
          "implemented");
    return TE_OS_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
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
dom_u_migrate_kind_set(unsigned int gid, char const *oid,
                       char const *value, char const *xen,
                       char const *dom_u)
{
#if XEN_SUPPORT
    unsigned u;
#endif

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(xen);

#if XEN_SUPPORT
    FIND_DOM_U(dom_u, u);

    dom_u_slot[u].migrate_kind = (strcmp(value, "0") == 0 ? 0 : 1);
    return 0;
#else
#warning '/agent/xen/dom_u/migrate/kind' 'set' access method is not \
         implemented
    ERROR("'/agent/xen/dom_u/migrate/kind' 'set' access method is not " \
          "implemented");
    return TE_OS_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
}

