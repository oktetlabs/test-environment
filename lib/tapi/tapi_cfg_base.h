/** @file
 * @brief Network Configuration Model TAPI
 *
 * Definition of test API for basic configuration model
 * (@path{storage/cm/cm_base.xml}).
 *
 *
 * Copyright (C) 2003-2019 OKTET Labs. All rights reserved.
 *
 * 
 *
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 * @author Oleg Kravtsov <Oleg.Kravtsov@oktetlabs.ru>
 *
 */

#ifndef __TE_TAPI_CFG_BASE_H__
#define __TE_TAPI_CFG_BASE_H__

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#include "te_defs.h"
#include "te_stdint.h"
#include "te_errno.h"
#include "conf_api.h"
#include "tapi_cfg_ip_rule.h"


#ifdef __cplusplus
extern "C" {
#endif

/** Kind of a TA directory */
typedef enum tapi_cfg_base_ta_dir {
    TAPI_CFG_BASE_TA_DIR_AGENT, /**< Agent directory */
    TAPI_CFG_BASE_TA_DIR_TMP,   /**< Directory of temporary files */
    TAPI_CFG_BASE_TA_DIR_KMOD,  /**< Kernel module directory */
    TAPI_CFG_BASE_TA_DIR_BIN,   /**< Library files directory */
} tapi_cfg_base_ta_dir;

/**
 * Get a TA directory as specified by @p kind.
 *
 * @param ta    Agent name
 * @param kind  Kind of a directory (see tapi_cfg_base_ta_dir type)
 *
 * @return Directory path (must be free()'d) or @c NULL on error
 */
extern char *tapi_cfg_base_get_ta_dir(const char *ta,
                                      tapi_cfg_base_ta_dir kind);

/**
 * @defgroup tapi_conf_base_net Network Base configuration
 * @ingroup tapi_conf
 * @{
 */

/** MAC VLAN interface mode: don't talk to other macvlans. */
#define TAPI_CFG_MACVLAN_MODE_PRIVATE   "private"

/** MAC VLAN interface mode: talk to other ports through ext bridge. */
#define TAPI_CFG_MACVLAN_MODE_VEPA      "vepa"

/** MAC VLAN interface mode: talk to bridge ports directly. */
#define TAPI_CFG_MACVLAN_MODE_BRIDGE    "bridge"

/** MAC VLAN interface mode: take over the underlying device. */
#define TAPI_CFG_MACVLAN_MODE_PASSTHRU  "passthru"

/** IP VLAN interface mode l2 */
#define TAPI_CFG_IPVLAN_MODE_L2   "l2"

/** IP VLAN interface mode l3 */
#define TAPI_CFG_IPVLAN_MODE_L3   "l3"

/** IP VLAN interface mode l3s */
#define TAPI_CFG_IPVLAN_MODE_L3S   "l3s"

/** IP VLAN default mode value */
#define TAPI_CFG_IPVLAN_MODE_DEFAULT    TAPI_CFG_IPVLAN_MODE_L2

/** IP VLAN interface flag bridge */
#define TAPI_CFG_IPVLAN_FLAG_BRIDGE   "bridge"

/** IP VLAN interface flag private */
#define TAPI_CFG_IPVLAN_FLAG_PRIVATE   "private"

/** IP VLAN interface flag vepa */
#define TAPI_CFG_IPVLAN_FLAG_VEPA   "vepa"

/** IP VLAN default flag value */
#define TAPI_CFG_IPVLAN_FLAG_DEFAULT    TAPI_CFG_IPVLAN_FLAG_BRIDGE

/**
 * Enable/disable IPv4 forwarding on a Test Agent.
 *
 * @param ta        TA name
 * @param enable    @c TRUE - enable, @c FALSE - disable
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_base_ipv4_fw(const char *ta, te_bool enable);

/**
 * Get IPv4 forwarding status on a Test Agent.
 *
 * @param ta            TA name
 * @param[out] enabled  @c TRUE - enabled, @c FALSE - disabled
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_base_ipv4_fw_enabled(const char *ta,
                                              te_bool *enabled);

/**
 * Enable/disable IPv4 forwarding on a specified network interface
 *
 * @param ta            TA name
 * @param ifname        Name of network interface
 * @param enable        @c TRUE - enable, @c FALSE - disable
 *
 * @return Status code
 */
extern te_errno tapi_cfg_ipv4_fw_set(const char *ta, const char *ifname,
                                     te_bool enable);

/**
 * Get IPv4 forwarding status of a specified network interface
 *
 * @param ta            TA name
 * @param ifname        Name of network interface
 * @param[out] enabled  @c TRUE - enabled, @c FALSE - disabled
 *
 * @return Status code
 */
extern te_errno tapi_cfg_ipv4_fw_get(const char *ta, const char *ifname,
                                     te_bool *enabled);

/**
 * It's a wrapper for tapi_cfg_ipv4_fw_set() to enable IPv4 forwarding
 * on a specified network interface
 */
static inline te_errno
tapi_cfg_ipv4_fw_enable(const char *ta, const char *ifname)
{
    return tapi_cfg_ipv4_fw_set(ta, ifname, TRUE);
}

/**
 * It's a wrapper for tapi_cfg_ipv4_fw_set() to disable IPv4 forwarding
 * on a specified network interface
 */
static inline te_errno
tapi_cfg_ipv4_fw_disable(const char *ta, const char *ifname)
{
    return tapi_cfg_ipv4_fw_set(ta, ifname, FALSE);
}

/**
 * Enable/disable IPv6 forwarding on a specified network interface
 *
 * @param ta            TA name
 * @param ifname        Name of network interface
 * @param enable        @c TRUE - enable, @c FALSE - disable
 *
 * @return Status code
 */
extern te_errno tapi_cfg_ipv6_fw_set(const char *ta, const char *ifname,
                                     te_bool enable);

/**
 * Get IPv6 forwarding status of a specified network interface
 *
 * @param ta            TA name
 * @param ifname        Name of network interface
 * @param[out] enabled  @c TRUE - enabled, @c FALSE - disabled
 *
 * @return Status code
 */
extern te_errno tapi_cfg_ipv6_fw_get(const char *ta, const char *ifname,
                                     te_bool *enabled);

/**
 * It's a wrapper for tapi_cfg_ipv6_fw_set() to enable IPv6 forwarding
 * on a specified network interface
 */
static inline te_errno
tapi_cfg_ipv6_fw_enable(const char *ta, const char *ifname)
{
    return tapi_cfg_ipv6_fw_set(ta, ifname, TRUE);
}

/**
 * It's a wrapper for tapi_cfg_ipv6_fw_set() to disable IPv6 forwarding
 * on a specified network interface
 */
static inline te_errno
tapi_cfg_ipv6_fw_disable(const char *ta, const char *ifname)
{
    return tapi_cfg_ipv6_fw_set(ta, ifname, FALSE);
}

/**
 * It's a wrapper for tapi_cfg_ipv6_fw_set() to enable/disable IPv6 forwarding
 * on a Test Agent. It sets forwarding to the interface "all".
 */
static inline te_errno
tapi_cfg_base_ipv6_fw(const char *ta, te_bool enable)
{
    return tapi_cfg_ipv6_fw_set(ta, "all", enable);
}

/**
 * It's a wrapper for tapi_cfg_ipv6_fw_get() to get IPv6 forwarding status
 * on a Test Agent, i.e. forwarding status of the interface "all".
 */
static inline te_errno
tapi_cfg_base_ipv6_fw_enabled(const char *ta, te_bool *enabled)
{
    return tapi_cfg_ipv6_fw_get(ta, "all", enabled);
}


/**@} <!-- END tapi_conf_base_net --> */

/**
 * @addtogroup tapi_conf_iface
 * @{
 */

/**
 * Get MAC address of TA interface.
 *
 * @param oid       instance OID of TA interface
 * @param mac       location for MAC address (at least ETHER_ADDR_LEN)
 *
 * @return Status code.
 */
extern int tapi_cfg_base_if_get_mac(const char *oid, uint8_t *mac);

/**
 * Set MAC address of TA interface.
 *
 * @param oid       instance OID of TA interface
 * @param mac       location of MAC address to be set
 *                  (at least ETHER_ADDR_LEN)
 *
 * @return Status code.
 */
extern int tapi_cfg_base_if_set_mac(const char *oid, const uint8_t *mac);

/**
 * Get broadcast MAC address of TA interface.
 *
 * @param oid       instance OID of TA interface
 * @param bcast_mac location for broadcast MAC address
 *                  (at least ETHER_ADDR_LEN)
 *
 * @return Status code.
 */
extern int tapi_cfg_base_if_get_bcast_mac(const char *oid,
                                          uint8_t *bcast_mac);

/**
 * Set broadcast MAC address of TA interface.
 *
 * @param oid       instance OID of TA interface
 * @param bcast_mac location of broadcast MAC address to be set
 *                  (at least ETHER_ADDR_LEN)
 *
 * @return Status code.
 */
extern int tapi_cfg_base_if_set_bcast_mac(const char *oid,
                                          const uint8_t *bcast_mac);


/**
 * Add multicast MAC address to a TA interface
 *
 * @param oid           instance OID of TA interface
 * @param mcast_mac     multicast MAC address to be added
 *                      (at least ETHER_ADDR_LEN)
 *
 * @return              Status code.
 */
extern int tapi_cfg_base_if_add_mcast_mac(const char *oid,
                                                const uint8_t *mcast_mac);


/**
 * Delete multicast MAC address from a TA interface
 *
 * @param oid           instance OID of TA interface
 * @param mcast_mac     multicast MAC address to be deleted
 *                      (at least ETHER_ADDR_LEN)
 *                      If NULL, then all multicast addresses are deleted
 *
 * @return              Status code.
 */

extern int tapi_cfg_base_if_del_mcast_mac(const char *oid,
                                          const uint8_t *mcast_mac);



/**
 * Get link address of TA interface.
 *
 * @param ta        Test Agent
 * @param dev       interface name
 * @param link_addr Location for link address
 *
 * @return Status code
 */
extern int tapi_cfg_base_if_get_link_addr(const char *ta, const char *dev,
                                          struct sockaddr *link_addr);
/**
 * Get MTU (layer 2 payload) of the Test Agent interface.
 *
 * @param oid       TA interface oid, e.g. /agent:A/interface:eth0
 * @param p_mtu     location for MTU
 *
 * @return Status code.
 */
extern int tapi_cfg_base_if_get_mtu(const char *oid, unsigned int *p_mtu);

/**
 * Add network address (/net_addr:).
 *
 * @param oid       TA interface oid, e.g. /agent:A/interface:eth0
 * @param addr      Address to add
 * @param prefix    Address prefix length (0 - default, -1 - do not set)
 * @param set_bcast Set broadcast address or not
 * @param cfg_hndl  Configurator handle of the new address
 *
 * @return Status code.
 * @retval TE_EAFNOSUPPORT     Address family is not supported
 * @retval TE_EEXIST           Address already exist
 */
extern int tapi_cfg_base_add_net_addr(const char            *oid,
                                      const struct sockaddr *addr,
                                      int                    prefix,
                                      te_bool                set_bcast,
                                      cfg_handle            *cfg_hndl);

/**
 * Wrapper over tapi_cfg_base_add_net_addr() function.
 *
 * @param ta        Test agent name
 * @param ifname    Interface name on the Agent
 * @param addr      Address to add
 * @param prefix    Address prefix length (0 - default, -1 - do not set)
 * @param set_bcast Set broadcast address or not
 * @param cfg_hndl  Configurator handle of the new address
 *
 * @return See return value of tapi_cfg_base_add_net_addr()
 */
static inline int
tapi_cfg_base_if_add_net_addr(const char *ta, const char *ifname,
                              const struct sockaddr *addr,
                              int prefix, te_bool set_bcast,
                              cfg_handle *cfg_hndl)
{
    char inst_name[CFG_OID_MAX];

    snprintf(inst_name, sizeof(inst_name),
             "/agent:%s/interface:%s", ta, ifname);
    return tapi_cfg_base_add_net_addr(inst_name, addr, prefix, set_bcast,
                                      cfg_hndl);
}

/**
 * Delete all IPv4 addresses on a given interface, except of
 * addr_to_save or the first address in acquired list.
 *
 * @param ta            Test Agent name
 * @param if_name       interface name
 * @param addr_to_save  address to save on interface.
 *                      If this parameter is NULL, then save
 *                      the first address in address list returned by
 *                      'ip addr list' output.
 *                      If one need to delete all addresses on interface,
 *                      he have to pass sockaddr with family 'AF_INET'
 *                      and  address 'INADDR_ANY'.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_del_if_ip4_addresses(const char *ta,
                                              const char *if_name,
                                              const struct sockaddr
                                                  *addr_to_save);

/**
 * Save and delete addresses of chosen address family on a given interface,
 * except for @p addr_to_save or the first address in acquired list.
 *
 * @param ta                Test Agent name
 * @param if_name           interface name
 * @param addr_to_save      address to save on the interface.
 *                          If this parameter is @c NULL, then save
 *                          the first address in address list returned by
 *                          `ip addr list` output.
 *                          If one needs to delete all addresses on
 *                          the interface, one should pass sockaddr with family
 *                          from @p addr_fam and address @c INADDR_ANY.
 * @param save_first        If @p addr_to_save is @c NULL but @p save_first is
 *                          @c TRUE, do not delete first address from acquired
 *                          list.
 * @param saved_addrs       Where address of array of deleted addresses
 *                          should be placed
 * @param saved_prefixes    Where address of array of deleted addresses'
 *                          prefixes should be placed
 * @param saved_broadcasts  Where address of array of deleted addresses'
 *                          broadcasts should be placed
 * @param saved_count       Where count of saved and deleted addresses
 *                          should be placed
 * @param addr_fam          Address family of deleted addresses
 *
 * @return Status code
 */
extern te_errno tapi_cfg_save_del_if_addresses(const char *ta,
                                      const char *if_name,
                                      const struct sockaddr *addr_to_save,
                                      te_bool save_first,
                                      struct sockaddr **saved_addrs,
                                      int **saved_prefixes,
                                      te_bool **saved_broadcasts,
                                      int *saved_count,
                                      int addr_fam);

/**
 * Save and delete all IPv4 addresses on a given interface, except of
 * addr_to_save or the first address in acquired list (if save_first
 * is TRUE).
 *
 * @param ta                Test Agent name
 * @param if_name           interface name
 * @param addr_to_save      address to save on interface.
 *                          If this parameter is NULL, then save
 *                          the first address in address list returned by
 *                          'ip addr list' output.
 *                          If one need to delete all addresses on
 *                          interface, he has to pass sockaddr with family
 *                          'AF_INET'
 *                          and address 'INADDR_ANY'.
 * @param save_first        If addr_to_save is NULL but save_first is
 *                          TRUE, do not delete first address from acquired
 *                          list.
 * @param saved_addrs       Where address of array of deleted addresses
 *                          should be placed
 * @param saved_prefixes    Where address of array of deleted addresses'
 *                          prefixes should be placed
 * @param saved_broadcasts  Where address of array of deleted addresses'
 *                          broadcasts should be placed
 * @param saved_count       Where count of saved and deleted addresses
 *                          should be placed
 *
 * @return Status code
 */
extern te_errno tapi_cfg_save_del_if_ip4_addresses(const char *ta,
                                                  const char *if_name,
                                                  const struct sockaddr
                                                    *addr_to_save,
                                                  te_bool save_first,
                                                  struct sockaddr
                                                    **saved_addrs,
                                                  int **saved_prefixes,
                                                  te_bool
                                                    **saved_broadcasts,
                                                  int *saved_count);

/**
 * Delete all IPv6 addresses on a given interface, except for
 * @p addr_to_save or the first address in acquired list.
 *
 * @param ta            Test Agent name
 * @param if_name       interface name
 * @param addr_to_save  address to save on interface.
 *                      If this parameter is NULL, then save
 *                      the first address in address list returned by
 *                      'ip addr list' output.
 *                      If one need to delete all addresses on interface,
 *                      he has to pass sockaddr with family @c AF_INET6
 *                      and address @c INADDR_ANY.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_del_if_ip6_addresses(
                    const char *ta,
                    const char *if_name,
                    const struct sockaddr *addr_to_save);

/**
 * Save and delete all IPv6 addresses on a given interface, except for
 * @p addr_to_save or the first address in acquired list (if save_first
 * is TRUE).
 *
 * @param ta                Test Agent name
 * @param if_name           interface name
 * @param addr_to_save      address to save on interface.
 *                          If this parameter is NULL, then save
 *                          the first address in address list returned by
 *                          'ip addr list' output.
 *                          If one need to delete all addresses on
 *                          interface, he has to pass sockaddr with family
 *                          @c AF_INET6 and address @c INADDR_ANY.
 * @param save_first        If addr_to_save is NULL but save_first is
 *                          TRUE, do not delete first address from acquired
 *                          list.
 * @param saved_addrs       Where address of array of deleted addresses
 *                          should be placed
 * @param saved_prefixes    Where address of array of deleted addresses'
 *                          prefixes should be placed
 * @param saved_broadcasts  Where address of array of deleted addresses'
 *                          broadcasts should be placed
 * @param saved_count       Where count of saved and deleted addresses
 *                          should be placed
 * @return Status code
 */
extern te_errno tapi_cfg_save_del_if_ip6_addresses(
                    const char *ta,
                    const char *if_name,
                    const struct sockaddr *addr_to_save,
                    te_bool save_first,
                    struct sockaddr **saved_addrs,
                    int **saved_prefixes,
                    te_bool **saved_broadcasts,
                    int *saved_count);

/**
 * Restore previously removed IPv4 addresses on a given interface
 *
 * @deprecated It is same as function tapi_cfg_restore_if_addresses
 *
 * @param ta                Test Agent name
 * @param if_name           interface name
 * @param saved_addrs       Where address of array of deleted addresses
 *                          should be placed
 * @param saved_prefixes    Where address of array of deleted addresses'
 *                          prefixes should be placed
 * @param saved_broadcasts  Where address of array of deleted addresses'
 *                          broadcasts should be placed
 * @param saved_count       Where count of saved and deleted addresses
 *                          should be placed
 *
 * @return Status code
 */
extern te_errno tapi_cfg_restore_if_ip4_addresses(const char *ta,
                                                  const char *if_name,
                                                  struct sockaddr
                                                    *saved_addrs,
                                                  int *saved_prefixes,
                                                  te_bool *saved_broadcasts,
                                                  int saved_count);

/**
 * Restore previously removed addresses on a given interface
 *
 * @param ta                Test Agent name
 * @param if_name           interface name
 * @param saved_addrs       Pointer to array of deleted addresses
 * @param saved_prefixes    Pointer to array of deleted addresses' prefixes
 * @param saved_broadcasts  Pointer to array of deleted addresses' broadcasts
 * @param saved_count       Count of saved and deleted addresses
 *
 * @return Status code
 */
extern te_errno tapi_cfg_restore_if_addresses(
                    const char *ta,
                    const char *if_name,
                    struct sockaddr *saved_addrs,
                    int *saved_prefixes,
                    te_bool *saved_broadcasts,
                    int saved_count);

static inline te_errno
tapi_cfg_base_if_up(const char *ta, const char *iface)
{
    int if_status = 1; /* enable */

    return cfg_set_instance_fmt(CFG_VAL(INTEGER, if_status),
                                "/agent:%s/interface:%s/status:",
                                ta, iface);
}

static inline te_errno
tapi_cfg_base_if_down(const char *ta, const char *iface)
{
    int if_status = 0; /* disable */

    return cfg_set_instance_fmt(CFG_VAL(INTEGER, if_status),
                                "/agent:%s/interface:%s/status:",
                                ta, iface);

}

static inline te_errno
tapi_cfg_base_if_arp_enable(const char *ta, const char * iface)
{
    int arp_use = 1; /* enable */

    return cfg_set_instance_fmt(CFG_VAL(INTEGER, arp_use),
                                "/agent:%s/interface:%s/arp:",
                                ta, iface);
}

static inline te_errno
tapi_cfg_base_if_arp_disable(const char *ta, const char * iface)
{
    int arp_use = 0; /* disable */

    return cfg_set_instance_fmt(CFG_VAL(INTEGER, arp_use),
                                "/agent:%s/interface:%s/arp:",
                                ta, iface);
}

/**
 * Set promiscuous mode for an interface.
 *
 * @param ta          Test Agent name.
 * @param ifname      Interface name.
 * @param enable      Whether to enable or to disable promiscuous mode.
 *
 * @return Status code.
 */
static inline te_errno
tapi_cfg_base_if_set_promisc(const char *ta,
                             const char *ifname,
                             te_bool enable)
{
    int val = (enable ? 1 : 0);

    return cfg_set_instance_fmt(CFG_VAL(INTEGER, val),
                                "/agent:%s/interface:%s/promisc:",
                                ta, ifname);
}

/**
 * Get promiscuous mode for an interface.
 *
 * @param ta          Test Agent name.
 * @param ifname      Interface name.
 * @param enabled     Will be set to @c TRUE if promiscuous mode is enabled
 *                    and to @c FALSE otherwise.
 *
 * @return Status code.
 */
static inline te_errno
tapi_cfg_base_if_get_promisc(const char *ta,
                             const char *ifname,
                             te_bool *enabled)
{
    te_errno rc;
    int val;

    rc = cfg_get_instance_int_fmt(&val, "/agent:%s/interface:%s/promisc:",
                                  ta, ifname);
    if (rc == 0)
        *enabled = (val != 0);

    return rc;
}

/**
 * Delete VLAN interface.
 *
 * @param ta            Test Agent name
 * @param if_name       interface name
 * @param vid           VLAN ID to create
 *
 * @return Status code
 */
extern te_errno tapi_cfg_base_if_del_vlan(const char *ta, const char *if_name,
                                          uint16_t vid);

/**
 * Add VLAN interface and get its name. The new interface is grabbed just
 * after creation.
 *
 * @param ta            Test Agent name
 * @param if_name       interface name
 * @param vid           VLAN ID to create
 * @param vlan_ifname   pointer to return the name of new interface
 *
 * @note MTU of the new VLAN interface is OS-dependent. For example, Linux
 * makes this MTU equal to the master interface MTU; Solaris creates VLAN
 * interface with maximum allowed MTU independently from the master
 * interface settings. Caller should care about MTU himself.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_base_if_add_vlan(const char *ta, const char *if_name,
                                          uint16_t vid, char **vlan_ifname);

/**
 * Delete VLAN if it exists and add VLAN interface and get its name if
 * possible.
 *
 * @param ta            Test Agent name
 * @param if_name       interface name
 * @param vid           VLAN ID to get the name
 * @param vlan_ifname   pointer to return the name of new interface
 *
 * @note MTU of the new VLAN interface is OS-dependent. For example, Linux
 * makes this MTU equal to the master interface MTU; Solaris creates VLAN
 * interface with maximum allowed MTU independently from the master
 * interface settings. Caller should care about MTU himself.
 *
 * @return Status code
 */
static inline te_errno
tapi_cfg_base_if_add_get_vlan(const char *ta, const char *if_name,
                              uint16_t vid, char **vlan_ifname)
{
    unsigned int    count = 0;
    cfg_handle      *handles = NULL;
    char             buf[128];
    int              vlan_not_found;

    vlan_not_found = cfg_get_instance_string_fmt(vlan_ifname,
                                     "/agent:%s/interface:%s/vlans:%d/ifname:",
                                     ta, if_name, vid);
    if (!vlan_not_found)
    {
        tapi_cfg_base_if_del_vlan(ta, if_name, vid);
        vlan_not_found = 1;
    }
    else
    {
        /* Try to search interface directly in case of windows VLANS */
        snprintf(buf, sizeof(buf), "/agent:%s/interface:%s.%d/",
                 ta, if_name, vid);
        cfg_find_pattern(buf, &count, &handles);
        if (count > 0)
        {
            tapi_cfg_base_if_del_vlan(ta, if_name, vid);
            free(handles);
        }
    }

    if (vlan_not_found &&
        /* Check if Priority only mode enabled */
        cfg_get_instance_string_fmt(vlan_ifname,
                                    "/agent:%s/interface:%s/vlans:%d/ifname:",
                                    ta, if_name, (vid | 0x1000)) &&
        /* Check if VLAN only vlan mode enabled */
        cfg_get_instance_string_fmt(vlan_ifname,
                                    "/agent:%s/interface:%s/vlans:%d/ifname:",
                                    ta, if_name, (vid | 0x2000)) )
    {
        return tapi_cfg_base_if_add_vlan(ta, if_name, vid, vlan_ifname);
    }

    return 0;
}

/**
 * Add interface @p ifname to the agent @p ta resources.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_base_if_add_rsrc(const char *ta,
                                          const char *ifname);

/**
 * Add interface @p ifname to the agent @p ta resources,
 * if it is not done already.
 *
 * @param ta      Test Agent name.
 * @param ifname  Interface name.
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_base_if_check_add_rsrc(const char *ta,
                                                const char *ifname);

/**
 * Delete interface @p ifname from the agent @p ta resources.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_base_if_del_rsrc(const char *ta,
                                          const char *ifname);

/**
 * Add MAC VLAN interface.
 *
 * @param ta        Test Agent name
 * @param link      Parent (link) interface name
 * @param ifname    MAC VLAN interface name
 * @param mode      MAC VLAN mode or @c NULL to use default
 *
 * @return Status code
 */
extern te_errno tapi_cfg_base_if_add_macvlan(const char *ta,
                                             const char *link,
                                             const char *ifname,
                                             const char *mode);

/**
 * Delete MAC VLAN interface.
 *
 * @param ta        Test Agent name
 * @param link      Parent (link) interface name
 * @param ifname    MAC VLAN interface name
 *
 * @return Status code
 */
extern te_errno tapi_cfg_base_if_del_macvlan(const char *ta,
                                             const char *link,
                                             const char *ifname);

/**
 * Get MAC VLAN interface.
 *
 * @param ta        Test Agent name
 * @param link      Parent (link) interface name
 * @param ifname    MAC VLAN interface name
 * @param mode      MAC VLAN mode location
 *
 * @return Status code
 */
extern te_errno tapi_cfg_base_if_get_macvlan_mode(const char *ta,
                                                  const char *link,
                                                  const char *ifname,
                                                  char **mode);

/**
 * Set MAC VLAN interface.
 *
 * @param ta        Test Agent name
 * @param link      Parent (link) interface name
 * @param ifname    MAC VLAN interface name
 * @param mode      MAC VLAN mode
 *
 * @return Status code
 */
extern te_errno tapi_cfg_base_if_set_macvlan_mode(const char *ta,
                                                  const char *link,
                                                  const char *ifname,
                                                  const char *mode);
/**
 * Add IP VLAN interface.
 *
 * @param ta        Test Agent name
 * @param link      Parent (link) interface name
 * @param ifname    IP VLAN interface name
 * @param mode      IP VLAN mode or @c NULL to use default mode value
 *                  @ref TAPI_CFG_IPVLAN_MODE_DEFAULT
 * @param flag      IP VLAN flag or @c NULL to use default flag value
 *                  @ref TAPI_CFG_IPVLAN_FLAG_DEFAULT
 *
 * @return Status code
 */
extern te_errno tapi_cfg_base_if_add_ipvlan(const char *ta,
                                            const char *link,
                                            const char *ifname,
                                            const char *mode,
                                            const char *flag);

/**
 * Delete IP VLAN interface.
 *
 * @param ta        Test Agent name
 * @param link      Parent (link) interface name
 * @param ifname    IP VLAN interface name
 *
 * @return Status code
 */
extern te_errno tapi_cfg_base_if_del_ipvlan(const char *ta,
                                            const char *link,
                                            const char *ifname);

/**
 * Get IP VLAN interface.
 *
 * @param[in] ta        Test Agent name
 * @param[in] link      Parent (link) interface name
 * @param[in] ifname    IP VLAN interface name
 * @param[out] mode     IP VLAN mode location
 * @param[out] flag     IP VLAN flag location
 *
 * @note: values @p mode and @p flag are allocated by this
 *        function and should be freed by user
 *
 * @return Status code
 */
extern te_errno tapi_cfg_base_if_get_ipvlan_mode(const char *ta,
                                                 const char *link,
                                                 const char *ifname,
                                                 char **mode,
                                                 char **flag);

/**
 * Set IP VLAN interface.
 *
 * @param ta        Test Agent name
 * @param link      Parent (link) interface name
 * @param ifname    IP VLAN interface name
 * @param mode      IP VLAN mode or @c NULL to use default mode value
 *                  @ref TAPI_CFG_IPVLAN_MODE_DEFAULT
 * @param flag      IP VLAN flag or @c NULL to use default flag value
 *                  @ref TAPI_CFG_IPVLAN_FLAG_DEFAULT
 *
 * @return Status code
 */
extern te_errno tapi_cfg_base_if_set_ipvlan_mode(const char *ta,
                                                 const char *link,
                                                 const char *ifname,
                                                 const char *mode,
                                                 const char *flag);

/**
 * Add veth interfaces pair.
 *
 * @param ta        Test Agent name
 * @param ifname    The interface name
 * @param peer      The peer interface name
 *
 * @return Status code
 */
extern te_errno tapi_cfg_base_if_add_veth(const char *ta, const char *ifname,
                                          const char *peer);

/**
 * Delete veth interfaces pair.
 *
 * @param ta        Test Agent name
 * @param ifname    The interface name
 *
 * @return Status code
 */
extern te_errno tapi_cfg_base_if_del_veth(const char *ta, const char *ifname);

/**
 * Get veth peer interface name.
 *
 * @param ta        Test Agent name
 * @param ifname    The interface name
 * @param peer      The peer interface name (allocated from the heap)
 *
 * @return Status code
 */
extern te_errno tapi_cfg_base_if_get_veth_peer(const char *ta,
                                               const char *ifname,
                                               char **peer);

/**
 * It is the same function as @p tapi_cfg_base_if_get_mtu, but it is more
 * user-friendly to use it in tests.
 *
 * @param agent     Agent name
 * @param interface Interface name
 * @param mtu       Location for MTU value
 *
 * @return Status code
 */
extern te_errno tapi_cfg_base_if_get_mtu_u(const char *agent,
                                           const char *interface, int *mtu);

/**
 * Set new MTU value.
 *
 * @param agent     Agent name
 * @param interface Interface name
 * @param mtu       MTU value
 * @param old_mtu   Location for old MTU value or @c NULL
 * @param fast      Don't sleep long time after interface restart if @c TRUE
 *
 * @note Consider using @c tapi_cfg_base_if_set_mtu_leastwise() instead.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_base_if_set_mtu_ext(const char *agent,
                                             const char *interface, int mtu,
                                             int *old_mtu, te_bool fast);

/**
 * Set new MTU value.
 *
 * @param agent     Agent name
 * @param interface Interface name
 * @param mtu       MTU value
 * @param old_mtu   Location for old MTU value or @c NULL
 *
 * @note Consider using @c tapi_cfg_base_if_set_mtu_leastwise() instead.
 *
 * @return Status code
 */
static inline te_errno
tapi_cfg_base_if_set_mtu(const char *agent, const char *interface, int mtu,
                         int *old_mtu)
{
    return tapi_cfg_base_if_set_mtu_ext(agent, interface, mtu, old_mtu,
                                        FALSE);
}

/**
 * Ensure that interface @a ifname on the host running TA @a ta
 * can receive frames up to MTU @a mtu . It increases the
 * interface MTU if required, but never decreases.
 *
 * @param ta     Agent name
 * @param ifname Interface name
 * @param mtu    MTU value
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_base_if_set_mtu_leastwise(const char   *ta,
                                                   const char   *ifname,
                                                   unsigned int  mtu);

/**
 * Down up interface.
 *
 * @param agent     Agent name
 * @param interface Interface name
 *
 * @note Caller should take care about wait for the interface to be raised.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_base_if_down_up(const char *agent,
                                         const char *interface);

/**@} <!-- END tapi_conf_iface --> */

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_CFG_BASE_H__ */
