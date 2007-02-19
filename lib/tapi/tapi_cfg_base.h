/** @file
 * @brief Network Configuration Model TAPI
 *
 * Definition of test API for basic configuration model
 * (storage/cm/cm_base.xml).
 *
 *
 * Copyright (C) 2004 Test Environment authors (see file AUTHORS
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
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 * @author Oleg Kravtsov <Oleg.Kravtsov@oktetlabs.ru>
 *
 * $Id$
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


#ifdef __cplusplus
extern "C" {
#endif

/**
 * Enable/disable IPv4 forwarding on a Test Agent.
 *
 * @param ta        TA name
 * @param enabled   IN: TRUE - enable, FALSE - disable;
 *                  OUT: previous state of forwarding
 *
 * @return Status code.
 *
 * @note Allocates ip4_fw resource on the Test Agent (if it is not
 * allocated). The resource is not released, because it should be locked
 * while the forwarding state is important to the test. It is recommended
 * to use silent configuration tracking for tests using this function.
 */
extern int tapi_cfg_base_ipv4_fw(const char *ta, te_bool *enabled);

/**
 * Enable/disable IPv6 forwarding on a Test Agent.
 *
 * @param ta        TA name
 * @param enabled   IN: TRUE - enable, FALSE - disable;
 *                  OUT: previous state of forwarding
 *
 * @return Status code.
 *
 * @note Allocates ip6_fw resource on the Test Agent (if it is not
 * allocated). The resource is not released, because it should be locked
 * while the forwarding state is important to the test. It is recommended
 * to use silent configuration tracking for tests using this function.
 */
extern int tapi_cfg_base_ipv6_fw(const char *ta, te_bool *enabled);

/**
 * Enable/disable IPv4/IPv6 forwarding on a Test Agent
 * (a backend for ...ipv4_fw() and ...ipv6_fw() )
 *
 * @param ta        TA name
 * @param enabled   IN: TRUE - enable, FALSE - disable;
 *                  OUT: previous state of forwarding
 * @param vrsn      a string "4" or "6".
 *
 * @return Status code.
 *
 * @note Allocates ipN_fw resource on the Test Agent (if it is not
 * allocated). The resource is not released, because it should be locked
 * while the forwarding state is important to the test. It is recommended
 * to use silent configuration tracking for tests using this function.
 */
extern int tapi_cfg_base_ip_fw(const char *ta, te_bool *enabled,
                               const char *vrsn);

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
 * Add VLAN interface and get its name.
 *
 * @param ta            Test Agent name
 * @param if_name       interface name
 * @param vid           VLAN ID to create
 * @param vlan_ifname   pointer to return the name of new interface
 *
 * @return Status code
 */
te_errno tapi_cfg_base_if_add_vlan(const char *ta, const char *if_name,
                                   uint16_t vid, char **vlan_ifname);

/**
 * Add VLAN interface if necessary and get its name.
 *
 * @param ta            Test Agent name
 * @param if_name       interface name
 * @param vid           VLAN ID to get the name
 * @param vlan_ifname   pointer to return the name of new interface
 *
 * @return Status code
 */
static inline te_errno
tapi_cfg_base_if_add_get_vlan(const char *ta, const char *if_name,
                              uint16_t vid, char **vlan_ifname)
{
    cfg_val_type val = CVT_STRING;

    if (cfg_get_instance_fmt(&val, vlan_ifname,
                             "/agent:%s/interface:%s/vlans:%d/ifname:",
                             ta, if_name, vid))
    {
        return tapi_cfg_base_if_add_vlan(ta, if_name, vid, vlan_ifname);
    }

    return 0;
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
static inline te_errno
tapi_cfg_base_if_del_vlan(const char *ta, const char *if_name,
                          uint16_t vid)
{
    te_errno     rc = 0;

    if ((rc = cfg_del_instance_fmt(FALSE, "/agent:%s/interface:%s/vlans:%d",
                                   ta, if_name, vid)) != 0)
    {
        ERROR("Failed to delete VLAN with VID=%d from %s", vid, if_name);
        return rc;
    }

    return rc;
}


/*
 * Configurator PHY support
 */

#define CONFIGURATOR_PHY_SUPPORT 0


#if CONFIGURATOR_PHY_SUPPORT

#define HAVE_LINUX_ETHTOOL_H 0

#if HAVE_LINUX_ETHTOOL_H

/* PHY Autonegotiation states */
#define PHY_AUTONEG_OFF  (0)    /* OFF */
#define PHY_AUTONEG_ON   (1)    /* ON */

/* PHY Duplex states */
#define PHY_DUPLEX_HALF  (0)    /* Half duplex */
#define PHY_DUPLEX_FULL  (1)    /* Full duplex */

/* PHY link states */
#define PHY_STATE_DOWN   (0)    /* Link down */
#define PHY_STATE_UP     (1)    /* Link up */


/**
 * Get PHY autonegotiation state.
 *
 * @param ta            Test Agent name
 * @param if_name       Interface name
 * @param state         Pointer to returned autonegotiation state value:
 *                      PHY_AUTONEG_OFF - autonegatiation OFF
 *                      PHY_AUTONEG_ON  - autonegatiation ON
 *
 * @return Status code
 */
extern te_errno
tapi_cfg_base_phy_autoneg_get(const char *ta, const char *if_name,
    int *state);

/**
 * Set PHY autonegotiation state.
 *
 * @param ta            Test Agent name
 * @param if_name       Interface name
 * @param state         Autonegotiation state value:
 *                      PHY_AUTONEG_OFF - autonegatiation OFF
 *                      PHY_AUTONEG_ON  - autonegatiation ON
 *
 * @return Status code
 */
extern te_errno
tapi_cfg_base_phy_autoneg_set(const char *ta, const char *if_name,
    int state);

/**
 * Get PHY duplex state.
 *
 * @param ta            Test Agent name
 * @param if_name       Interface name
 * @param state         Pointer to returned duplex state value:
 *                      PHY_DUPLEX_HALF - half duplex
 *                      PHY_DUPLEX_FULL - full duplex
 *
 * @return Status code
 */
extern te_errno
tapi_cfg_base_phy_duplex_get(const char *ta, const char *if_name,
    int *state);

/**
 * Set PHY duplex state.
 *
 * @param ta            Test Agent name
 * @param if_name       Interface name
 * @param state         Duplex state value:
 *                      PHY_DUPLEX_HALF - half duplex
 *                      PHY_DUPLEX_FULL - full duplex
 *
 * @return Status code
 */
extern te_errno
tapi_cfg_base_phy_duplex_set(const char *ta, const char *if_name,
    int state);

/**
 * Get PHY speed value.
 *
 * @param ta            Test Agent name
 * @param if_name       Interface name
 * @param speed         Pointer to returned speed value
 *                      (see linux/ethtool.h for more details)
 *
 * @return Status code
 */
extern te_errno
tapi_cfg_base_phy_speed_get(const char *ta, const char *if_name,
    int *speed);

/**
 * Set PHY speed.
 *
 * @param ta            Test Agent name
 * @param if_name       Interface name
 * @param speed         Speed value (see linux/ethtool.h for more details)
 *
 * @return Status code
 */
extern te_errno
tapi_cfg_base_phy_speed_set(const char *ta, const char *if_name,
    int speed);

/**
 * Get PHY link state.
 *
 * @param ta            Test Agent name
 * @param if_name       Interface name
 * @param state         Pointer to returned link state value:
 *                      0 - link down
 *                      1 - link up
 *
 * @return Status code
 */
extern te_errno
tapi_cfg_base_phy_state_get(const char *ta, const char *if_name,
    int *state);

/**
 * Check that PHY mode is advertised.
 *
 * @param ta            Test Agent name
 * @param if_name       Interface name
 * @param speed         Speed value (see linux/ethtool.h for more details)
 * @param duplex        Duplex state value:
 *                      PHY_DUPLEX_HALF - half duplex
 *                      PHY_DUPLEX_FULL - full duplex
 * @param state         Pointer to mode state:
 *                      TRUE - mode is advertised
 *                      FALSE - mode is not advertised
 *
 * @return Status code
 */
extern te_errno
tapi_cfg_base_phy_is_mode_advertised(const char *ta, const char *if_name,
    int speed, int duplex, te_bool *state);

/**
 * Set PHY mode to advertising state.
 *
 * @param ta            Test Agent name
 * @param if_name       Interface name
 * @param speed         Speed value (see linux/ethtool.h for more details)
 * @param duplex        Duplex state value:
 *                      PHY_DUPLEX_HALF - half duplex
 *                      PHY_DUPLEX_FULL - full duplex
 * @param state         Mode state:
 *                      0 - mode is not advertised
 *                      1 - mode is advertised 
 *
 * @return Status code
 */
extern te_errno
tapi_cfg_base_phy_advertise_mode(const char *ta, const char *if_name,
    int speed, int duplex, int state);

/**
 * Restart autonegotiation.
 *
 * @param ta            Test Agent name
 * @param if_name       Interface name
 * @param unused        Unused parameter;
 *                      should be set to any value (just formality)
 *
 * @return Status code
 */
extern te_errno
tapi_cfg_base_phy_restart_autoneg(const char *ta, const char *if_name,
    int unused);

#endif /* HAVE_LINUX_ETHTOOL_H */

#endif /* CONFIGURATOR_PHY_SUPPORT */

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_CFG_BASE_H__ */
