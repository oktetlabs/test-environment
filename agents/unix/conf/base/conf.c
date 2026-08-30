/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Unix Test Agent
 *
 * Unix TA configuring support
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
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

#if HAVE_DIRENT_H
#include <dirent.h>
#endif

#if HAVE_LINUX_NET_TSTAMP_H
#include <linux/net_tstamp.h>
#endif

#if HAVE_LINUX_IF_VLAN_H
#include <linux/if_vlan.h>
#define LINUX_VLAN_SUPPORT 1
#else
#define LINUX_VLAN_SUPPORT 0
#endif

#if defined(HAVE_SYS_SYSCTL_H) && defined(HAVE_SYS_TYPES_H) && \
    !defined(__linux__) && !defined(BSD_IP_FW)
#define BSD_IP_FW 1
#endif

#if defined(HAVE_SYS_PARAM_H) && defined(BSD_IP_FW)
/* { required for sysctl on netbsd */
#include <sys/param.h>
/* } required for sysctl on netbsd */
#endif

#if defined(HAVE_SYS_SYSCTL_H) && defined(BSD_IP_FW)
#include <sys/sysctl.h>
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

#include "te_alloc.h"
#include "te_stdint.h"
#include "te_errno.h"
#include "te_defs.h"
#include "te_enum.h"
#include "te_queue.h"
#include "te_ethernet.h"
#include "te_sockaddr.h"
#include "te_str.h"
#include "cs_common.h"
#include "logger_api.h"
#include "comm_agent.h"
#include "rcf_ch_api.h"
#include "rcf_pch.h"
#include "rcf_pch_ta_cfg.h"
#include "rcf_pch_tree.h"
#include "logger_api.h"
#include "unix_internal.h"
#include "conf_ovs.h"
#include "conf_route.h"
#include "conf_rule.h"
#include "conf_vm.h"
#include "conf_process.h"
#include "conf_getmsg.h"
#include "conf_common.h"
#include "te_shell_cmd.h"
#include "te_string.h"
#include "te_alloc.h"
#include "te_vector.h"

#include "conf_daemons.h"

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

#ifdef ENABLE_8021X
extern te_errno ta_unix_conf_supplicant_init(void);
extern te_errno supplicant_grab(const char *name);
extern te_errno supplicant_release(const char *name);
#endif

#ifdef ENABLE_IFCONFIG_STATS
extern te_errno ta_unix_conf_net_if_stats_init(void);
#endif

#ifdef ENABLE_NET_SNMP_STATS
extern te_errno ta_unix_conf_net_snmp_stats_init(void);
#endif

#ifdef ENABLE_VCM_SUPPORT
extern te_errno ta_unix_conf_vcm_init(void);
#endif

#ifdef WITH_ISCSI
extern te_errno ta_unix_iscsi_target_init(void);
extern te_errno iscsi_initiator_conf_init(void);
#endif

#ifdef WITH_IPTABLES
extern te_errno ta_unix_conf_iptables_init(void);
#endif

#ifdef WITH_SERIALPARSE
extern te_errno ta_unix_serial_parser_init(void);
extern te_errno ta_unix_serial_parser_cleanup(void);
#endif

#ifdef WITH_SERIAL
extern te_errno ta_unix_serial_console_init(void);
extern te_errno ta_unix_serial_console_cleanup(void);
#endif

extern te_errno ta_unix_conf_configfs_init(void);
extern te_errno ta_unix_conf_netconsole_init(void);
extern te_errno ta_unix_conf_sys_init(void);
extern te_errno ta_unix_conf_sys_tree_init(void);
extern te_errno ta_unix_conf_sys_tree_fini(void);
extern te_errno ta_unix_conf_if_phy_init(void);
extern te_errno ta_unix_conf_if_coalesce_init(void);
extern te_errno ta_unix_conf_if_flow_ctrl_init(void);
extern te_errno ta_unix_conf_if_rss_init(void);
extern te_errno ta_unix_conf_if_rx_rules_init(void);
extern te_errno ta_unix_conf_if_eee_init(void);
extern te_errno ta_unix_conf_eth_init(void);
extern te_errno ta_unix_conf_iommu_init(void);
extern te_errno ta_unix_conf_macvlan_init(void);
extern te_errno ta_unix_conf_ipvlan_init(void);
extern te_errno ta_unix_conf_module_init(void);
extern te_errno ta_unix_conf_ns_net_init(void);
extern te_errno ta_unix_conf_veth_init(void);
extern te_errno ta_unix_conf_tap_init(void);
extern te_errno ta_unix_conf_udp_tunnel_init(void);
extern te_errno ta_unix_conf_bridge_init(void);
extern te_errno ta_unix_conf_block_dev_init(void);
extern te_errno ta_unix_conf_l4_port_init(void);
extern te_errno ta_unix_conf_eth_xstats_init(void);
extern te_errno ta_unix_conf_irq_stats_init(void);

extern te_errno ta_unix_conf_key_init(void);
extern void ta_unix_conf_key_fini(void);

#ifdef WITH_OPENVPN
extern te_errno ta_unix_conf_openvpn_init(void);
#endif

#ifdef WITH_TC
extern te_errno ta_unix_conf_tc_init(void);
extern te_errno ta_unix_conf_tc_fini(void);
#endif

#ifdef USE_LIBNETCONF
netconf_handle nh = NETCONF_HANDLE_INVALID;
#endif

#ifdef WITH_AGGREGATION
extern te_errno ta_unix_conf_aggr_init(void);
#endif

#ifdef ENABLE_PCI_SUPPORT
extern te_errno ta_unix_conf_pci_init(void);
extern te_errno ta_unix_conf_pci_cleanup(void);
#endif

extern te_errno ta_unix_conf_memory_init(void);
extern te_errno ta_unix_conf_memory_cleanup(void);

extern te_errno ta_unix_conf_cpu_init(void);

#ifdef WITH_SOCKS
extern te_errno ta_unix_conf_socks_init(void);
#endif

#ifdef WITH_SNIFFERS
extern te_errno ta_unix_conf_sniffer_init(void);
extern te_errno ta_unix_conf_sniffer_cleanup(void);
#endif

extern te_errno ta_unix_conf_cmd_monitor_init(void);
extern te_errno ta_unix_conf_cmd_monitor_cleanup(void);

extern te_errno ta_unix_conf_rlimits_init(void);

#ifdef WITH_BPF
extern te_errno ta_unix_conf_bpf_init(void);
extern te_errno ta_unix_conf_if_xdp_init(void);
extern te_errno ta_unix_conf_bpf_cleanup(void);
extern te_errno ta_unix_conf_if_xdp_cleanup(void);
#endif

#ifdef WITH_NGINX
extern te_errno ta_unix_conf_nginx_init(void);
#endif

extern te_errno ta_unix_conf_loadavg_init(void);

#ifdef WITH_UPNP_CP
# include "conf_upnp_cp.h"
#endif /* WITH_UPNP_CP */

#define INTERFACE_IS_PPP(ifname) \
     (strncmp(ifname, "ppp", strlen("ppp")) == 0)

const char *te_lockdir = "/tmp";

/* Auxiliary variables used for during configuration request processing */

static char buf[4096];
static char trash[128];

int cfg_socket = -1;
int cfg6_socket = -1;

/*
 * Access routines prototypes (comply to procedure types
 * specified in rcf_ch_api.h).
 */
static te_errno env_get(const ta_conf_ctx *ctx, te_string *val);
static te_errno env_set(const ta_conf_ctx *ctx, const char *val);
static te_errno env_add(const ta_conf_ctx *ctx, const char *val);
static te_errno env_del(const ta_conf_ctx *ctx);
static te_errno env_list(const ta_conf_ctx *ctx, te_vec *names);

/* routines to make substitutions */
static te_errno env_subst_path_process(te_string *value, const char *subst,
                                       const char *replaced_value);
static te_errno env_subst_ld_lib_path_process(te_string *value,
                                              const char *subst,
                                              const char *replaced_value);
static te_errno env_subst_underscore_process(te_string *value,
                                             const char *subst,
                                             const char *replaced_value);

/** Environment variables hidden in list operation */
static const char * const env_hidden[] = {
    "SSH_CLIENT",
    "SSH_CONNECTION",
    "SUDO_COMMAND",
    "TE_RPC_PORT",
    "TE_LOG_PORT",
    "TARPC_DL_NAME",
    "TCE_CONNECTION",
    "LD_PRELOAD",
    "LS_COLORS",
    "XDG_RUNTIME_DIR",
    "XDG_SESSION_ID",
};

/** Environment variables with specified prefix hidden in list operation */
static const char * const env_prefix_hidden[] = {
    "BASH_FUNC_",
};

static te_errno uname_get(const ta_conf_ctx *ctx, te_string *val);
static te_errno uname_version_get(const ta_conf_ctx *ctx, te_string *val);
static te_errno uname_release_get(const ta_conf_ctx *ctx, te_string *val);
static te_errno uname_machine_get(const ta_conf_ctx *ctx, te_string *val);

static te_errno ip4_fw_get(const ta_conf_ctx *ctx, bool *val);
static te_errno ip4_fw_set(const ta_conf_ctx *ctx, bool val);

static te_errno ip6_fw_get(const ta_conf_ctx *ctx, bool *val);
static te_errno ip6_fw_set(const ta_conf_ctx *ctx, bool val);

static te_errno switchdev_name_get(const ta_conf_ctx *ctx, te_string *val);
static te_errno switchdev_name_list(const ta_conf_ctx *ctx, te_vec *names);

static te_errno rp_filter_all_get(const ta_conf_ctx *ctx, te_string *val);
static te_errno rp_filter_all_set(const ta_conf_ctx *ctx, const char *val);

static te_errno arp_ignore_all_get(const ta_conf_ctx *ctx, te_string *val);
static te_errno arp_ignore_all_set(const ta_conf_ctx *ctx, const char *val);

static te_errno agent_platform_get(const ta_conf_ctx *ctx, te_string *val);

static te_errno agent_dir_get(const ta_conf_ctx *ctx, te_string *val);

static te_errno agent_tmp_dir_get(const ta_conf_ctx *ctx, te_string *val);

static te_errno agent_lib_mod_dir_get(const ta_conf_ctx *ctx, te_string *val);

static te_errno agent_lib_bin_dir_get(const ta_conf_ctx *ctx, te_string *val);

static te_errno nameserver_get(const ta_conf_ctx *ctx, te_string *val);

static te_errno user_list(const ta_conf_ctx *ctx, te_vec *names);
static te_errno user_add(const ta_conf_ctx *ctx);
static te_errno user_del(const ta_conf_ctx *ctx);

/*
 * Unix Test Agent basic configuration tree.
 */

static const ta_conf_node *const node_platform =
    TA_CONF_RO_STR("platform", agent_platform_get);

static const ta_conf_node *const node_dir =
    TA_CONF_RO_STR("dir", agent_dir_get);

static const ta_conf_node *const node_tmp_dir =
    TA_CONF_RO_STR("tmp_dir", agent_tmp_dir_get);

static const ta_conf_node *const node_lib_mod_dir =
    TA_CONF_RO_STR("lib_mod_dir", agent_lib_mod_dir_get);

static const ta_conf_node *const node_lib_bin_dir =
    TA_CONF_RO_STR("lib_bin_dir", agent_lib_bin_dir_get);

static const ta_conf_node *const node_dns =
    TA_CONF_RO_STR("dns", nameserver_get);

static const ta_conf_node *const node_rp_filter_all =
    TA_CONF_RW_STR("rp_filter_all", rp_filter_all_get, rp_filter_all_set);

static const ta_conf_node *const node_arp_ignore_all =
    TA_CONF_RW_STR("arp_ignore_all", arp_ignore_all_get, arp_ignore_all_set);

static const ta_conf_node *const node_switchdev_name =
    TA_CONF_RO_COLL("switchdev_name", switchdev_name_get,
                    switchdev_name_list);

static const ta_conf_node *const node_ip4_fw =
    TA_CONF_RW_BOOL("ip4_fw", ip4_fw_get, ip4_fw_set);

static const ta_conf_node *const node_ip6_fw =
    TA_CONF_RW_BOOL("ip6_fw", ip6_fw_get, ip6_fw_set);

static const rcf_pch_cfg_substitution env_subst[] = RCF_PCH_CFG_SUBST_SET(
    { "PATH", "/agent/dir", env_subst_path_process },
    { "LD_LIBRARY_PATH", "/agent/dir", env_subst_ld_lib_path_process },
    { "_", "/agent/dir", env_subst_underscore_process}
);

static const ta_conf_node *const node_env =
    TA_CONF_NODE((.name = "env", .type = CVT_STRING,
                  .get = { .as_str = env_get },
                  .set = { .as_str = env_set },
                  .add = { .as_str = env_add },
                  .del = env_del,
                  .list = env_list,
                  .subst = env_subst));

static const ta_conf_node *const node_uname =
    TA_CONF_RO_STR("uname", uname_get,
        TA_CONF_RO_STR("version", uname_version_get),
        TA_CONF_RO_STR("release", uname_release_get),
        TA_CONF_RO_STR("machine", uname_machine_get));

static const ta_conf_node *const node_user =
    TA_CONF_COLL("user", user_add, user_del, user_list);

static const ta_conf_node *const node_namespace =
    TA_CONF_NA("namespace");

static const ta_conf_node *const node_hardware =
    TA_CONF_NA("hardware");

bool
ta_interface_is_mine(const char *ifname)
{
    if (rcf_pch_rsrc_accessible("/agent:%s/interface:%s",
                                ta_name, ifname))
        return true;

#if 0
    if (INTERFACE_IS_PPP(ifname) ||
        rcf_pch_rsrc_accessible("/agent:%s/pppoeserver:", ta_name))
        return true;
#endif

    return false;
}

#ifndef DISABLE_NETWORKMANAGER_CHECK
/**
 * Check if NetworkManager controls this interface. If there is no
 * NetworkManager in the system, it is considered that NetworkManager does not
 * control the interface.
 *
 * @param ifname     network interface name
 *
 * @return           Status code.
 * @retval TE_EBUSY  Interface is controlled by NetworkManager
 * @retval 0         Interface is not controlled by NetworkManager
 */
static te_errno
check_networkmanager_on_interface(const char *ifname)
{
    int result;

/**
 * Check a result of a shell command execution and terminate with error if
 * the execution failed. This implies that error is not produced if the command
 * returned non-zero status, it occurs only if it was impossible to execute
 * the command.
 */
#define CHECK_NM_CMD_RESULT(_result)                                         \
    do {                                                                     \
        if (_result < 0 || !WIFEXITED(_result))                              \
        {                                                                    \
            ERROR("%s(): "                                                   \
                  "Failed to execute shell command to check NetworkManager " \
                  "presence, returned status is %d", __FUNCTION__, _result); \
            return TE_RC(TE_TA_UNIX, TE_ESHCMD);                             \
        }                                                                    \
    } while(0)

    /**
     * Check if NetworkManager daemon is running on the system. If it is not,
     * return zero since in this case the interface cannot be controlled by it.
     * Note that the check works fine if the system does not support
     * NetworkManager at all.
     */
    result = ta_system("ps aux | grep NetworkManager | grep -vq grep");
    CHECK_NM_CMD_RESULT(result);
    if (WEXITSTATUS(result) == 0)
    {
        /**
         * At this point it is known that NetworkManager is running.
         * Check if the particular interface is managed by it.
         * Exit with error if it is and with zero if it is not.
         * If nmcli was not found under the standard location, also error out
         * since something is definitely wrong with NetworkManager installation,
         * and it is impossible to find out whether the interface is controlled
         * by NetworkManager.
         */
        result = ta_system_fmt(
            "nmcli -g GENERAL.STATE dev show %s 2>&1 | "
            "grep -Eq 'unmanaged|not found'", ifname);

        CHECK_NM_CMD_RESULT(result);
        if (WEXITSTATUS(result) != 0)
        {
            ERROR("%s(): Interface %s is managed by NetworkManager which can "
                  "cause unreliable behaviour", __FUNCTION__, ifname);
            return TE_RC(TE_TA_UNIX, TE_EBUSY);
        }
    }

    return 0;
#undef CHECK_NM_CMD_RESULT
}
#endif

/** Grab interface-specific resources */
static te_errno
interface_grab(const char *name)
{
#ifndef DISABLE_NETWORKMANAGER_CHECK
    const char *ifname;
    te_errno rc;

    /* Get resource instance name */
    ifname = strrchr(name, ':');
    if (ifname == NULL)
    {
        ERROR("Invalid resource instance name");
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }
    ifname++;

    /* Check if NetworkManager controls this interface and abort if it does */
    rc = check_networkmanager_on_interface(ifname);
    if (rc != 0)
        return rc;
#endif

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
    te_errno rc;

    if ((rc = ta_conf_register("/agent", node_platform)) != 0)
        return rc;
    if ((rc = ta_conf_register("/agent", node_dir)) != 0)
        return rc;
    if ((rc = ta_conf_register("/agent", node_tmp_dir)) != 0)
        return rc;
    if ((rc = ta_conf_register("/agent", node_lib_mod_dir)) != 0)
        return rc;
    if ((rc = ta_conf_register("/agent", node_lib_bin_dir)) != 0)
        return rc;
    if ((rc = ta_conf_register("/agent", node_dns)) != 0)
        return rc;
    if ((rc = ta_conf_register("/agent", node_rp_filter_all)) != 0)
        return rc;
    if ((rc = ta_conf_register("/agent", node_arp_ignore_all)) != 0)
        return rc;

    if ((rc = ta_unix_conf_interface_init()) != 0)
        return rc;

    if ((rc = ta_unix_conf_xen_init()) != 0)
        return rc;

    if ((rc = ta_conf_register("/agent", node_switchdev_name)) != 0)
        return rc;
    if ((rc = ta_conf_register("/agent", node_ip4_fw)) != 0)
        return rc;
    if ((rc = ta_conf_register("/agent", node_ip6_fw)) != 0)
        return rc;
    if ((rc = ta_conf_register("/agent", node_env)) != 0)
        return rc;
    if ((rc = ta_conf_register("/agent", node_uname)) != 0)
        return rc;
    if ((rc = ta_conf_register("/agent", node_user)) != 0)
        return rc;
    if ((rc = ta_conf_register("/agent", node_namespace)) != 0)
        return rc;

    return ta_conf_register("/agent", node_hardware);
}

/* See the description in lib/rcfpch/rcf_ch_api.h */
int
rcf_ch_conf_init(void)
{
    static bool init = false;

    if (!init)
    {
#ifdef USE_LIBNETCONF
        if (netconf_open(&nh, NETLINK_ROUTE) != 0)
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

        if (ta_unix_conf_base_init() != 0)
            goto fail;

        if (ta_unix_conf_route_init() != 0)
            goto fail;

        if (ta_unix_conf_rule_init() != 0)
            goto fail;

        if (ta_unix_conf_vm_init() != 0)
            goto fail;

        if (ta_unix_conf_ps_init() != 0)
            goto fail;

#ifdef WITH_OVS
        if (ta_unix_conf_ovs_init() != 0)
            goto fail;
#endif /* WITH_OVS */

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
#ifdef ENABLE_PCI_SUPPORT
        if (ta_unix_conf_pci_init() != 0)
            goto fail;
#endif

        if (ta_unix_conf_cpu_init() != 0)
            goto fail;

#ifdef ENABLE_VCM_SUPPORT
        if (ta_unix_conf_vcm_init() != 0)
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

        if (ta_unix_conf_memory_init() != 0)
            goto fail;

        if (ta_unix_conf_sys_init() != 0)
            goto fail;

        if (ta_unix_conf_sys_tree_init() != 0)
            goto fail;

        /* Initialize configurator PHY support */
        if (ta_unix_conf_if_phy_init() != 0)
            goto fail;

        if (ta_unix_conf_if_coalesce_init() != 0)
            goto fail;

        if (ta_unix_conf_if_flow_ctrl_init() != 0)
            goto fail;

        if (ta_unix_conf_if_rss_init() != 0)
            goto fail;

        if (ta_unix_conf_if_rx_rules_init() != 0)
            goto fail;

        if (ta_unix_conf_if_eee_init() != 0)
            goto fail;

        if (ta_unix_conf_configfs_init() != 0)
            goto fail;

        if (ta_unix_conf_netconsole_init() != 0)
            goto fail;

        if (ta_unix_conf_eth_init() != 0)
            goto fail;

        if (ta_unix_conf_eth_xstats_init() != 0)
            goto fail;

        if (ta_unix_conf_irq_stats_init() != 0)
            goto fail;

        if (ta_unix_conf_iommu_init() != 0)
            goto fail;

        if (ta_unix_conf_loadavg_init() != 0)
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
#ifdef WITH_SERIAL
        ta_unix_serial_console_init();
#endif
#ifdef WITH_SNIFFERS
        if (ta_unix_conf_sniffer_init() != 0)
            ERROR("Failed to add sniffer configuration tree");
#endif

        ta_unix_conf_cmd_monitor_init();

#ifdef WITH_UPNP_CP
        if (ta_unix_conf_upnp_cp_init() != 0)
        {
            ERROR("Failed to add UPnP Control Point configuration subtree");
            goto fail;
        }
#endif /* WITH_UPNP_CP */

#ifdef WITH_SOCKS
        if (ta_unix_conf_socks_init() != 0)
        {
            ERROR("Failed to add SOCKS configuration subtree");
            goto fail;
        }
#endif /* WITH_SOCKS */

        if (ta_unix_conf_macvlan_init() != 0)
        {
            ERROR("Failed to add macvlan interface configuration subtree");
            goto fail;
        }

#ifdef IFLA_IPVLAN_MAX
        if (ta_unix_conf_ipvlan_init() != 0)
        {
            ERROR("Failed to add ipvlan interface configuration subtree");
            goto fail;
        }
#endif /* IFLA_IPVLAN_MAX */

        if (ta_unix_conf_module_init() != 0)
        {
            ERROR("Failed to add system module configuration subtree");
            goto fail;
        }

        if (ta_unix_conf_ns_net_init() != 0)
        {
            ERROR("Failed to add network namespaces configuration subtree");
            goto fail;
        }

        if (ta_unix_conf_veth_init() != 0)
        {
            ERROR("Failed to add veth interfaces configuration subtree");
            goto fail;
        }

        if (ta_unix_conf_tap_init() != 0)
        {
            ERROR("Failed to add tap interfaces configuration subtree");
            goto fail;
        }

        if (ta_unix_conf_udp_tunnel_init() != 0)
        {
            ERROR("Failed to add udp tunnel interfaces configuration subtree");
            goto fail;
        }

        if (ta_unix_conf_bridge_init() != 0)
        {
            ERROR("Failed to add bridge interfaces configuration subtree");
            goto fail;
        }

        if (ta_unix_conf_rlimits_init() != 0)
        {
            ERROR("Failed to add resource limits configuration subtree");
            goto fail;
        }

#ifdef WITH_OPENVPN
        if (ta_unix_conf_openvpn_init() != 0)
        {
            ERROR("Failed to add OpenVPN configuration subtree");
            goto fail;
        }
#endif /* WITH_OPENVPN */

#ifdef WITH_TC
        if (ta_unix_conf_tc_init() != 0)
        {
            ERROR("Failed to add resource tc configuration subtree");
            goto fail;
        }
#endif /* WITH_TC */

        if (ta_unix_conf_block_dev_init() != 0)
        {
            ERROR("Failed to add block devices subtree");
            goto fail;
        }

#ifdef WITH_BPF
        if (ta_unix_conf_bpf_init() != 0)
            goto fail;

        if (ta_unix_conf_if_xdp_init() != 0)
            goto fail;
#endif

#ifdef WITH_NGINX
        if (ta_unix_conf_nginx_init() != 0)
            goto fail;
#endif

        if (ta_unix_conf_l4_port_init() != 0)
            goto fail;

        if (ta_unix_conf_key_init() != 0)
            goto fail;

        if (ta_unix_conf_selftest_init() != 0)
            goto fail;

        init = true;

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
rcf_ch_conf_agent(void)
{
    return ta_name;
}

/**
 * Release resources allocated for configuration support.
 */
void
rcf_ch_conf_fini(void)
{
#ifdef WITH_SERIALPARSE
    ta_unix_serial_parser_cleanup();
#endif
#ifdef WITH_SERIAL
    ta_unix_serial_console_cleanup();
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
#ifdef WITH_UPNP_CP
    ta_unix_conf_upnp_cp_release();
#endif /* WITH_UPNP_CP */
#ifdef WITH_BPF
    ta_unix_conf_if_xdp_cleanup();
    ta_unix_conf_bpf_cleanup();
#endif

#ifdef ENABLE_PCI_SUPPORT
    ta_unix_conf_pci_cleanup();
#endif

    ta_unix_conf_memory_cleanup();

    ta_unix_conf_key_fini();

   (void)ta_unix_conf_sys_tree_fini();

    ta_unix_conf_cmd_monitor_cleanup();
    if (cfg_socket >= 0)
        (void)close(cfg_socket);
    if (cfg6_socket >= 0)
        (void)close(cfg6_socket);
}

/* See the description in conf_common.h */
te_errno
write_sys_value(const char *value, const char *format, ...)
{
    va_list   valist;
    char      path[PATH_MAX];
    int       rc = 0;
    FILE     *f = NULL;
    te_errno  result = 0;

    va_start(valist, format);
    rc = vsnprintf(path, PATH_MAX, format, valist);
    va_end(valist);

    if (rc >= PATH_MAX || rc < 0)
    {
        ERROR("%s(): failed to fill path", __FUNCTION__);
        return TE_RC(TE_TA_UNIX, TE_EFAIL);
    }

    if ((f = fopen(path, "w")) == NULL)
    {
        ERROR("%s: failed to open %s for writing",
              __FUNCTION__, path);
        return TE_OS_RC(TE_TA_UNIX, errno);
    }

    rc = fprintf(f, "%s\n", value);
    if (rc < 0)
    {
        ERROR("%s: failed to write '%s' in %s",
              __FUNCTION__, value, path);
        result = TE_OS_RC(TE_TA_UNIX, errno);
    }
    else if (rc != (int)strlen(value) + 1)
    {
        ERROR("%s: wrong length was returned by fprintf()",
              __FUNCTION__);
        result = TE_RC(TE_TA_UNIX, TE_EFAIL);
    }

    if (fclose(f) < 0)
    {
        ERROR("%s: failed to close %s after writing",
              __FUNCTION__, path);
        result = TE_OS_RC(TE_TA_UNIX, errno);
    }

    return result;
}

/* See the description in conf_common.h */
te_errno
read_sys_value(char *value, size_t len, bool ignore_eaccess,
               const char *format, ...)
{
    va_list   valist;
    char      path[PATH_MAX];
    int       rc = 0;
    FILE     *f = NULL;
    te_errno  result = 0;
    int       i = 0;

    value[0] = '\0';

    if (len < 1)
    {
        ERROR("%s(): length must be positive", __FUNCTION__);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    va_start(valist, format);
    rc = vsnprintf(path, PATH_MAX, format, valist);
    va_end(valist);

    if (rc >= PATH_MAX || rc < 0)
    {
        ERROR("%s(): failed to fill path", __FUNCTION__);
        return TE_RC(TE_TA_UNIX, TE_EFAIL);
    }

    if ((f = fopen(path, "r")) == NULL)
    {
        if (ignore_eaccess && errno == EACCES)
            return 0;

        /*
         * Do not print any logs here to avoid a lot of error
         * messages during configuration synchronization if
         * some sysfs file is simply not present.
         */
        if (errno == ENOENT)
            return TE_RC(TE_TA_UNIX, TE_ENOENT);

        ERROR("%s: failed to open %s for reading, errno=%d ('%s')",
              __FUNCTION__, path, errno, strerror(errno));
        return TE_OS_RC(TE_TA_UNIX, errno);
    }

    rc = fread(value, 1, len - 1, f);
    if (rc < 0)
    {
        ERROR("%s: failed to read data from %s",
              __FUNCTION__, path);
        result = TE_OS_RC(TE_TA_UNIX, errno);
    }
    else if (rc > (int)len - 1)
    {
        ERROR("%s: too much data was read from %s",
              __FUNCTION__, path);
        result = TE_RC(TE_TA_UNIX, TE_EFAIL);
    }
    else
    {
        value[rc] = '\0';
        for (i = rc - 1; i >= 0; i--)
        {
            if (value[i] == '\n' || value[i] == '\r')
                value[i] = '\0';
            else
                break;
        }
    }

    if (fclose(f) < 0)
    {
        ERROR("%s: failed to close %s after writing",
              __FUNCTION__, path);
        result = TE_OS_RC(TE_TA_UNIX, errno);
    }

    return result;
}

/* See the description in conf_common.h */
te_errno
get_dir_list_vec(const char *path, te_vec *names,
                 bool ignore_absence,
                 include_callback_func include_callback,
                 void *callback_data,
                 int (*compar)(const struct dirent **,
                               const struct dirent **))
{
    struct dirent  **namelist = NULL;
    struct dirent   *de = NULL;
    int              n;
    int              i;

    n = scandir(path, &namelist, NULL, compar);
    if (n < 0)
    {
        if (errno == ENOENT && ignore_absence)
            return 0;

        ERROR("%s: failed to scan %s directory",
              __FUNCTION__, path);
        return TE_OS_RC(TE_TA_UNIX, errno);
    }

    for (i = 0; i < n; i++)
    {
        de = namelist[i];

        if (strcmp(de->d_name, ".") == 0 ||
            strcmp(de->d_name, "..") == 0)
            continue;

        if (include_callback != NULL &&
            !include_callback(de->d_name, callback_data))
            continue;

        {
            char *name = TE_STRDUP(de->d_name);

            TE_VEC_APPEND(names, name);
        }
    }

    for (i = 0; i < n; i++)
    {
        free(namelist[i]);
    }
    free(namelist);

    return 0;
}

/* See the description in conf_common.h */
te_errno
get_dir_list(const char *path, char *buffer, size_t length,
             bool ignore_absence,
             include_callback_func include_callback,
             void *callback_data,
             int (*compar)(const struct dirent **,
                           const struct dirent **))
{
    te_vec    names = TE_VEC_INIT_AUTOPTR(char *);
    te_string str = TE_STRING_INIT;
    te_errno  ret;
    int       rc;

    buffer[0] = '\0';

    ret = get_dir_list_vec(path, &names, ignore_absence, include_callback,
                           callback_data, compar);
    if (ret != 0)
    {
        te_vec_free(&names);
        return ret;
    }

    /*
     * Every name is followed by a single space, including the last one:
     * that is the encoding the legacy rcf_pch list callbacks and their
     * callers expect.
     */
    te_string_join_vec(&str, &names, " ");
    if (str.len > 0)
        te_string_append(&str, " ");

    te_vec_free(&names);

    /*
     * snprintf() truncates exactly like the previous per-name loop did,
     * so a caller inspecting the buffer after TE_ESMALLBUF still sees
     * the same bytes.
     */
    rc = snprintf(buffer, length, "%s", te_string_value(&str));
    if (rc < 0)
    {
        ret = TE_OS_RC(TE_TA_UNIX, errno);
        ERROR("%s(): snprintf() failed", __FUNCTION__);
    }
    else if ((size_t)rc >= length)
    {
        ret = TE_RC(TE_TA_UNIX, TE_ESMALLBUF);
        ERROR("%s(): not enough space for all names from %s",
              __FUNCTION__, path);
    }

    te_string_free(&str);

    return ret;
}

/* See the description in conf_common.h */
te_errno
string_replace(char **dst, const char *src)
{
    char *new_dst = NULL;

    if (src != NULL && *src != '\0')
    {
        new_dst = strdup(src);
        if (new_dst == NULL)
            return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }

    free(*dst);
    *dst = new_dst;

    return 0;
}

/* See the description in conf_common.h */
te_errno
string_empty_list(char **list)
{
    char *l = strdup("");

    if (l == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    *list = l;
    return 0;
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

    te_strlcpy(xbuf, ipfw_str, sizeof(xbuf));

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
 * @param ip6           @c false for IPv4, @c true for IPv6
 * @param p_val         location of the value: 0 or 1 to set the variable,
 *                      other - to read into the location (IN/OUT).
 *
 * @return              Status code.
 */
static te_errno
ipforward_bsd(bool ip6, int *p_val)
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
 * @param ctx           request context (unused)
 * @param val           value location
 *
 * @return              Status code
 */
static te_errno
ip4_fw_get(const ta_conf_ctx *ctx, bool *val)
{
#if __linux__
    char c = '0';
    int  fd;
#endif
#if defined(SOLARIS_IP_FW) || defined(BSD_IP_FW)
    te_errno    rc;
    int         ival;
#endif
    UNUSED(ctx);

#if __linux__
    if ((fd = open("/proc/sys/net/ipv4/ip_forward", O_RDONLY)) < 0)
        return TE_OS_RC(TE_TA_UNIX, errno);

    if (read(fd, &c, 1) < 0)
    {
        close(fd);
        return TE_OS_RC(TE_TA_UNIX, errno);
    }
    close(fd);

    *val = (c != '0');

#elif SOLARIS_IP_FW
    ival = 2; /* anything except 0|1 is read */
    rc = ipforward_solaris("ip_forwarding", &ival);
    if (rc != 0)
        return rc;
    *val = (ival != 0);

#elif BSD_IP_FW
    ival = 2;
    rc = ipforward_bsd(false, &ival); /* @c false if not ip6 */
    if (rc != 0)
        return rc;
    *val = (ival != 0);

#else
    /* Assume that forwarding is disabled */
    *val = false;
#endif

    return 0;
}

/**
 * Enable/disable IPv4 forwarding.
 *
 * @param ctx           request context (unused)
 * @param val           new value of IPv4 forwarding system variable
 *
 * @return              Status code
 */
static te_errno
ip4_fw_set(const ta_conf_ctx *ctx, bool val)
{
#if __linux__
    int fd;
#endif
#if defined(SOLARIS_IP_FW) || defined(BSD_IP_FW)
    te_errno rc;
    int ival;
#endif
    UNUSED(ctx);
#if !defined(__linux__) && !defined(SOLARIS_IP_FW) && !defined(BSD_IP_FW)
    UNUSED(val);
#endif

#if __linux__
    fd = open("/proc/sys/net/ipv4/ip_forward",
              O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd < 0)
        return TE_OS_RC(TE_TA_UNIX, errno);

    if (write(fd, val ? "1\n" : "0\n", 2) < 0)
    {
        close(fd);
        return TE_OS_RC(TE_TA_UNIX, errno);
    }
    close(fd);

#elif SOLARIS_IP_FW
    ival = val ? 1 : 0;
    rc = ipforward_solaris("ip_forwarding", &ival);
    if (rc != 0)
        return rc;

#elif BSD_IP_FW
    ival = val ? 1 : 0;
    rc = ipforward_bsd(false, &ival); /* @c false if not ip6 */
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
 * @param ctx           request context (unused)
 * @param val           value location
 *
 * @return              Status code
 */
static te_errno
ip6_fw_get(const ta_conf_ctx *ctx, bool *val)
{
#if __linux__
    int  fd;
    char c = '0';
#endif
#if defined(SOLARIS_IP_FW) || defined(BSD_IP_FW)
    te_errno    rc;
    int         ival;
#endif
    UNUSED(ctx);

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

    *val = (c != '0');

#elif SOLARIS_IP_FW
    ival = 2; /* anything except 0|1 is read */
    rc = ipforward_solaris("ip6_forwarding", &ival);
    if (rc != 0)
        return rc;
    *val = (ival != 0);

#elif BSD_IP_FW
    ival = 2;
    rc = ipforward_bsd(true, &ival); /* @c false if not ip6 */
    if (rc != 0)
        return rc;
    *val = (ival != 0);

#else
    /* Assume that forwarding is disabled */
    *val = false;
#endif

    return 0;
}   /* ip6_fw_get() */

/**
 * Enable/disable IPv6 forwarding.
 *
 * @param ctx           request context (unused)
 * @param val           new value of IPv6 forwarding system variable
 *
 * @return              Status code
 */
static te_errno
ip6_fw_set(const ta_conf_ctx *ctx, bool val)
{
#if __linux__
    int fd;
#endif
#if defined(SOLARIS_IP_FW) || defined(BSD_IP_FW)
    te_errno rc;
    int ival;
#endif
    UNUSED(ctx);
#if !defined(__linux__) && !defined(SOLARIS_IP_FW) && !defined(BSD_IP_FW)
    UNUSED(val);
#endif

#if __linux__
    fd = open("/proc/sys/net/ipv6/conf/all/forwarding",
              O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd < 0)
        return TE_OS_RC(TE_TA_UNIX, errno);

    if (write(fd, val ? "1\n" : "0\n", 2) < 0)
    {
        close(fd);
        return TE_OS_RC(TE_TA_UNIX, errno);
    }
    close(fd);

#elif SOLARIS_IP_FW
    ival = val ? 1 : 0;
    rc = ipforward_solaris("ip6_forwarding", &ival);
    if (rc != 0)
        return rc;

#elif BSD_IP_FW
    ival = val ? 1 : 0;
    rc = ipforward_bsd(true, &ival); /* @c false if not ip6 */
    if (rc != 0)
        return rc;

#else
    return TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif

    return 0;
}   /* ip6_fw_set() */

/**
 * Get instance value for object "agent/switchdev_name".
 *
 * @param ctx           Request context; its instance name is the
 *                       (switch ID, port name) pair.
 * @param val           Location for the interface name.
 *
 * @return              Status code.
 */
static te_errno
switchdev_name_get(const ta_conf_ctx *ctx, te_string *val)
{
    const char *id = ta_conf_ctx_inst(ctx, "switchdev_name");
    const char *sep;
    char *switch_id;
    char *port_name;

    if (*id == '\0')
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    sep = strchr(id, ':');
    if (sep == NULL)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    switch_id = strndup(id, sep - id);
    port_name = strdup(sep + 1);

#ifdef USE_LIBNETCONF
    {
        netconf_list       *links;
        const netconf_node *node;

        links = netconf_link_dump(nh);
        if (links == NULL)
        {
            free(switch_id);
            free(port_name);
            return TE_OS_RC(TE_TA_UNIX, errno);
        }

        for (node = links->head; node != NULL; node = node->next)
        {
            const netconf_link *link = &(node->data.link);

            if (link->switch_id != NULL && link->port_name != NULL &&
                strcmp(link->switch_id, switch_id) == 0 &&
                strcmp(link->port_name, port_name) == 0)
            {
                te_string_append(val, "%s", link->ifname);

                free(switch_id);
                free(port_name);
                netconf_list_free(links);
                return 0;
            }
        }
        netconf_list_free(links);
    }

    ERROR("Failed to find rep for '%s/%s'", switch_id, port_name);
    free(switch_id);
    free(port_name);

    return TE_RC(TE_TA_UNIX, TE_ENOENT);
#else
    free(switch_id);
    free(port_name);

    return 0;
#endif
}

/**
 * Get instance list for object "agent/switchdev_name".
 *
 * @param ctx           Request context (unused).
 * @param names         Vector of heap-allocated names to append to.
 *
 * @return              Status code.
 */
static te_errno
switchdev_name_list(const ta_conf_ctx *ctx, te_vec *names)
{
    UNUSED(ctx);

#ifdef USE_LIBNETCONF
    {
        netconf_list       *links;
        const netconf_node *node;

        links = netconf_link_dump(nh);
        if (links == NULL)
            return TE_OS_RC(TE_TA_UNIX, errno);

        for (node = links->head; node != NULL; node = node->next)
        {
            const netconf_link *link = &(node->data.link);

            if (link->switch_id != NULL && link->port_name != NULL)
            {
                char *name = te_string_fmt("%s:%s", link->switch_id,
                                           link->port_name);

                TE_VEC_APPEND(names, name);
            }
        }
        netconf_list_free(links);
    }
#endif

    return 0;
}

/**
 * Get RPF filtering value for interface "all"
 *
 * @param ctx           request context (unused)
 * @param val           value location
 *
 * @return              Status code
 */
static te_errno
rp_filter_all_get(const ta_conf_ctx *ctx, te_string *val)
{
    UNUSED(ctx);
    return rp_filter_get_core("all", val);
}

/**
 * Set RPF filtering value for interface "all"
 *
 * @param ctx           request context (unused)
 * @param val           new value
 *
 * @return              Status code
 */
static te_errno
rp_filter_all_set(const ta_conf_ctx *ctx, const char *val)
{
    UNUSED(ctx);
    return rp_filter_set_core("all", val);
}

/**
 * Get arp_ignore value for interface "all"
 *
 * @param ctx           request context (unused)
 * @param val           value location
 *
 * @return              Status code
 */
static te_errno
arp_ignore_all_get(const ta_conf_ctx *ctx, te_string *val)
{
    UNUSED(ctx);
    return arp_ignore_get_core("all", val);
}

/**
 * Set arp_ignore value for interface "all"
 *
 * @param ctx           request context (unused)
 * @param val           new value
 *
 * @return              Status code
 */
static te_errno
arp_ignore_all_set(const ta_conf_ctx *ctx, const char *val)
{
    UNUSED(ctx);
    return arp_ignore_set_core("all", val);
}

static te_errno
agent_platform_get(const ta_conf_ctx *ctx, te_string *val)
{
    UNUSED(ctx);
#ifdef TE_AGT_PLATFORM
    te_string_append(val, "%s", TE_AGT_PLATFORM);
#else
    te_string_append(val, "%s", "default");
#endif
    return 0;
}

static te_errno
agent_dir_get(const ta_conf_ctx *ctx, te_string *val)
{
    UNUSED(ctx);
    te_string_append(val, "%s", ta_dir);
    return 0;
}

static te_errno
agent_tmp_dir_get(const ta_conf_ctx *ctx, te_string *val)
{
    UNUSED(ctx);
    te_string_append(val, "%s", ta_tmp_dir);
    return 0;
}

static te_errno
agent_lib_mod_dir_get(const ta_conf_ctx *ctx, te_string *val)
{
    UNUSED(ctx);
    te_string_append(val, "%s", ta_lib_mod_dir);
    return 0;
}

static te_errno
agent_lib_bin_dir_get(const ta_conf_ctx *ctx, te_string *val)
{
    UNUSED(ctx);
    te_string_append(val, "%s", ta_lib_bin_dir);
    return 0;
}

static te_errno
nameserver_get(const ta_conf_ctx *ctx, te_string *val)
{
    FILE    *resolver = NULL;
    char     buf[256] = { 0, };
    char    *found = NULL, *endaddr = NULL;
    te_errno rc = TE_RC(TE_TA_UNIX, TE_ENOENT);

    static const char ip_symbols[] = "0123456789.";

    UNUSED(ctx);

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
                    te_string_append(val, "%s", found);
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
static bool
env_is_hidden(const char *name, int name_len)
{
    unsigned int    i;

    for (i = 0; i < sizeof(env_hidden) / sizeof(env_hidden[0]); ++i)
    {
        if (strncmp(env_hidden[i], name,
                    (name_len < 0) ? strlen(name) : (size_t)name_len) == 0)
            return true;
    }
    for (i = 0; i < sizeof(env_prefix_hidden) / sizeof(env_prefix_hidden[0]);
         ++i)
    {
        if ((name_len < 0 || name_len >= strlen(env_prefix_hidden[i])) &&
            strncmp(env_prefix_hidden[i], name,
                    strlen(env_prefix_hidden[i])) == 0)
            return true;
    }
    return false;
}

/**
 * Get Environment variable value.
 *
 * @param ctx       Request context; its instance name is the
 *                  variable name
 * @param val       Location for the value (OUT)
 *
 * @return Status code
 */
static te_errno
env_get(const ta_conf_ctx *ctx, te_string *val)
{
    const char *name = ta_conf_ctx_inst(ctx, "env");
    const char *tmp = getenv(name);

    if (env_is_hidden(name, -1) || tmp == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (strlen(tmp) >= RCF_MAX_VAL)
    {
        WARN("Environment variable '%s' value truncated", name);
        te_string_append(val, "%.*s", RCF_MAX_VAL - 1, tmp);
    }
    else
    {
        te_string_append(val, "%s", tmp);
    }

    return 0;
}

/**
 * Change already existing Environment variable.
 *
 * @param ctx       Request context; its instance name is the
 *                  variable name
 * @param val       New value to set
 *
 * @return Status code
 */
static te_errno
env_set(const ta_conf_ctx *ctx, const char *val)
{
    const char *name = ta_conf_ctx_inst(ctx, "env");

    if (env_is_hidden(name, -1))
        return TE_RC(TE_TA_UNIX, TE_EPERM);

    if (setenv(name, val, true) == 0)
    {
        return 0;
    }
    else
    {
        te_errno rc = TE_OS_RC(TE_TA_UNIX, errno);

        ERROR("Failed to set Environment variable '%s' to '%s'; errno %r",
              name, val, rc);
        return rc;
    }
}

/**
 * Add a new Environment variable.
 *
 * @param ctx       Request context; its instance name is the
 *                  variable name
 * @param val       Value
 *
 * @return Status code
 */
static te_errno
env_add(const ta_conf_ctx *ctx, const char *val)
{
    const char *name = ta_conf_ctx_inst(ctx, "env");

    if (env_is_hidden(name, -1))
        return TE_RC(TE_TA_UNIX, TE_EPERM);

    if (getenv(name) == NULL)
    {
        if (setenv(name, val, false) == 0)
        {
            return 0;
        }
        else
        {
            te_errno rc = TE_OS_RC(TE_TA_UNIX, errno);

            ERROR("Failed to add Environment variable '%s=%s'",
                  name, val);
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
 * @param ctx       Request context; its instance name is the
 *                  variable name
 *
 * @return Status code
 */
static te_errno
env_del(const ta_conf_ctx *ctx)
{
    const char *name = ta_conf_ctx_inst(ctx, "env");

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
 * @param ctx       Request context (unused)
 * @param names     Vector of heap-allocated names to append to
 *
 * @return Status code
 */
static te_errno
env_list(const ta_conf_ctx *ctx, te_vec *names)
{
    char * const *env;

    UNUSED(ctx);

    if (environ == NULL)
        return 0;

    for (env = environ; *env != NULL; ++env)
    {
        char    *s = strchr(*env, '=');
        ssize_t  name_len;
        char    *name;

        if (s == NULL)
        {
            ERROR("Invalid Environment entry format: %s", *env);
            return TE_RC(TE_TA_UNIX, TE_EFMT);
        }
        name_len = s - *env;
        if (env_is_hidden(*env, name_len))
            continue;

        name = TE_ALLOC(name_len + 1);
        memcpy(name, *env, name_len);
        name[name_len] = '\0';
        TE_VEC_APPEND(names, name);
    }

    return 0;
}

static void
path_substitution_process(te_string *value, const char *subst,
                          const char *replaced_value,
                          const char *env_var)
{
    te_substring_t iter = TE_SUBSTRING_INIT(value);
    char *cur_pos = value->ptr;
    char *end_pos;
    char *ch;

    while (1)
    {
        if (strcmp_start(replaced_value, cur_pos) == 0)
        {
            end_pos = cur_pos + strlen(replaced_value);
            if (*end_pos == '/' || *end_pos == '\0' || *end_pos == ':')
            {
                iter.start = cur_pos - value->ptr;
                iter.len = strlen(replaced_value);
                if (!te_substring_replace(&iter, "%s", subst))
                    break;
            }
        }

        ch = strchr(cur_pos, ':');
        if (ch == NULL)
            break;

        cur_pos = ch + 1;
    }
}

static te_errno
env_subst_path_process(te_string *value, const char *subst,
                       const char *replaced_value)
{
    path_substitution_process(value, subst, replaced_value, "PATH");
    return 0;
}

static te_errno
env_subst_ld_lib_path_process(te_string *value, const char *subst,
                              const char *replaced_value)
{
    path_substitution_process(value, subst, replaced_value, "LD_LIBRARY_PATH");
    return 0;
}

static te_errno
env_subst_underscore_process(te_string *value, const char *subst,
                             const char *replaced_value)
{
    if (strcmp_start(replaced_value, value->ptr) == 0)
        te_string_replace(value, 0, strlen(replaced_value), "%s", subst);

    return 0;
}

/**
 * Retrieve an uname string.
 *
 * @param[out] val    resulting value
 * @param[in]  field  the offset of a field in `struct utsname`
 *                    (expected to be a string pointer field)
 *
 * @return status code
 */
static te_errno
uname_string_get(te_string *val, size_t field)
{
#if HAVE_SYS_UTSNAME_H
    struct utsname uts;

    if (uname(&uts) < 0)
    {
        te_errno rc = TE_OS_RC(TE_TA_UNIX, errno);

        ERROR("cannot call uname(): %r", rc);
        return rc;
    }

    te_string_append(val, "%s", (char *)&uts + field);

    return 0;
#else
    /* In this extremely unlikely case just return empty string */
    UNUSED(field);
    UNUSED(val);

    return 0;
#endif /* HAVE_SYS_UTSNAME_H */
}

static te_errno
uname_get(const ta_conf_ctx *ctx, te_string *val)
{
    UNUSED(ctx);

    return uname_string_get(val, offsetof(struct utsname, sysname));
}

static te_errno
uname_version_get(const ta_conf_ctx *ctx, te_string *val)
{
    UNUSED(ctx);

    return uname_string_get(val, offsetof(struct utsname, version));
}

static te_errno
uname_release_get(const ta_conf_ctx *ctx, te_string *val)
{
    UNUSED(ctx);

    return uname_string_get(val, offsetof(struct utsname, release));
}

static te_errno
uname_machine_get(const ta_conf_ctx *ctx, te_string *val)
{
    UNUSED(ctx);

    return uname_string_get(val, offsetof(struct utsname, machine));
}

/**
 * Get instance list for object "agent/user".
 *
 * @param ctx           request context (unused)
 * @param names         vector of heap-allocated names to append to
 *
 * @return              Status code:
 * @retval 0                success
 */
static te_errno
user_list(const ta_conf_ctx *ctx, te_vec *names)
{
    FILE *f;

    UNUSED(ctx);

    if ((f = fopen("/etc/passwd", "r")) == NULL)
    {
        te_errno rc = TE_OS_RC(TE_TA_UNIX, errno);

        ERROR("Failed to open file /etc/passwd; errno %r", rc);
        return rc;
    }

    while (fgets(trash, sizeof(trash), f) != NULL)
    {
        char *tmp = strstr(trash, TE_USER_PREFIX);
        char *tmp1;
        char *name;

        unsigned int uid;

        if (tmp == NULL)
            continue;

        tmp += strlen(TE_USER_PREFIX);
        uid = strtol(tmp, &tmp1, 10);
        if (tmp1 == tmp || *tmp1 != ':')
            continue;

        name = te_string_fmt(TE_USER_PREFIX "%u", uid);
        TE_VEC_APPEND(names, name);
    }
    fclose(f);

    return 0;
}

/**
 * Check, if user with the specified name exists.
 *
 * @param user          user name
 *
 * @return              @c true if user exists, @c false if does not
 */
static bool
user_exists(const char *user)
{
    return getpwnam(user) != NULL ? true : false;
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
    struct pam_response *resp_array = TE_ALLOC(num_msg * sizeof(*resp));
    appdata_t           *appdata    = data;

    int      i;
    unsigned full_len = strlen(appdata->passwd) + 1; /**< Password
                                                       *  length + 1
                                                       */

    for (i = 0; i < num_msg; i++) /* Process each message */
    {
        /** PAM prompts for password */
        if (msg[i]->msg_style == PAM_PROMPT_ECHO_ON ||
                msg[i]->msg_style == PAM_PROMPT_ECHO_OFF)
        {
            /** Allocate memory for password and supply it to PAM */
            resp_array[i].resp = TE_ALLOC(full_len);
            memcpy(resp_array[i].resp, appdata->passwd, full_len);
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
 * Delete tester user.
 *
 * @param user          user name
 *
 * @return              Status code
 */
static te_errno
user_del_core(const char *user)
{
    te_errno rc;

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

/**
 * Add tester user.
 *
 * @param ctx           request context
 *
 * @return              Status code
 */
static te_errno
user_add(const ta_conf_ctx *ctx)
{
    const char *user = ta_conf_ctx_inst(ctx, "user");
#if TA_USE_PAM || defined(__linux__)
    char *tmp;
    char *tmp1;

    unsigned int uid;

    te_errno     rc;
#endif

#if !TA_USE_PAM && !defined(__linux__)
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
     * "-f" is used in order not to fail if such group already exists (bug 11813)
     */
    sprintf(buf, "/usr/sbin/groupadd -f -g %u %s ", uid, user);
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
        user_del_core(user);
        return TE_RC(TE_TA_UNIX, TE_ESHCMD);
    }

#if 0
    /* Fedora has very aggressive nscd cache */
    /* https://bugzilla.redhat.com/bugzilla/show_bug.cgi?id=134323 */
    ta_system("/usr/sbin/nscd -i group && /usr/sbin/nscd -i passwd");
#endif


    TE_SPRINTF(buf, "/tmp/%s/.ssh/id_ed25519", user);
    rc = agent_key_generate(AGENT_KEY_MANAGER_SSH, "ed25519", 1024, user, buf);
    if (rc != 0)
    {
        ERROR("Cannot create ssh key: %r", rc);
        user_del_core(user);
        return rc;
    }

    return 0;
#endif /* !TA_USE_PAM */
}

/**
 * Delete tester user.
 *
 * @param ctx           request context
 *
 * @return              Status code
 */
static te_errno
user_del(const ta_conf_ctx *ctx)
{
    return user_del_core(ta_conf_ctx_inst(ctx, "user"));
}
