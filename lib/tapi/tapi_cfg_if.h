/** @file
 * @brief Network interface configuration
 *
 * @defgroup tapi_conf_if Network interface configuration TAPI
 * @ingroup tapi_conf
 * @{
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 */

#ifndef __TE_TAPI_CFG_IF_H__
#define __TE_TAPI_CFG_IF_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Check whether a given interface feature is read-only
 *
 * @param ta                Test agent name
 * @param ifname            Interface name
 * @param feature_name      Feature name
 * @param readonly          Will be set to @c TRUE if the feature
 *                          is read-only
 *
 * @return Status code
 */
extern te_errno tapi_cfg_if_feature_is_readonly(const char *ta,
                                                const char *ifname,
                                                const char *feature_name,
                                                te_bool *readonly);

/**
 * Check whether a given interface feature is present
 *
 * @param ta                Test agent name
 * @param ifname            Interface name
 * @param feature_name      Feature name
 * @param present           Will be set to @c TRUE if the feature
 *                          is present
 *
 * @return Status code
 */
extern te_errno tapi_cfg_if_feature_is_present(const char *ta,
                                               const char *ifname,
                                               const char *feature_name,
                                               te_bool *present);

/**
 * Get feature value of an ethernet interface
 *
 * @param ta                Test agent name
 * @param ifname            Interface name
 * @param feature_name      Feature name
 * @param feature_value_out Location for the value
 *
 * @return Status code
 */
extern te_errno tapi_cfg_if_feature_get(const char *ta,
                                        const char *ifname,
                                        const char *feature_name,
                                        int        *feature_value_out);

/**
 * Set feature value of an ethernet interface
 *
 * @param ta            Test agent name
 * @param ifname        Interface name
 * @param feature_name  Feature name
 * @param feature_value Location for the value
 *
 * @return Status code
 */
extern te_errno tapi_cfg_if_feature_set(const char *ta,
                                        const char *ifname,
                                        const char *feature_name,
                                        int         feature_value);

/**
 * Set feature value of an ethernet interface and all its parents if they are.
 * Ignore @c EOPNOTSUPP failures and failures to change read-only feature if
 * requested changing was successful at least for one interface.
 *
 * @param ta            Test agent name
 * @param ifname        Interface name
 * @param feature_name  Feature name
 * @param feature_value Location for the value
 *
 * @return Status code
 */
extern te_errno tapi_cfg_if_feature_set_all_parents(const char *ta,
                                                    const char *ifname,
                                                    const char *feature_name,
                                                    int feature_value);

/**
 * Get driver name of a network interface.
 *
 * @param ta            Test agent name
 * @param ifname        Interface name
 * @param drivername    Driver name, obtained value should be released if
 *                      not required anymore.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_if_deviceinfo_drivername_get(const char *ta,
                                                      const char *ifname,
                                                      char **drivername);

/**
 * Get driver version of a network interface.
 *
 * @param ta            Test agent name
 * @param ifname        Interface name
 * @param driverversion Driver version, obtained value should be released if
 *                      not required anymore.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_if_deviceinfo_driverversion_get(const char *ta,
                                                         const char *ifname,
                                                         char **driverversion);

/**
 * Get firmware version of a network interface.
 *
 * @param ta                Test agent name
 * @param ifname            Interface name
 * @param firmwareversion   Firmware version, obtained value should be
 *                          released if not required anymore.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_if_deviceinfo_firmwareversion_get(
                                                    const char *ta,
                                                    const char *ifname,
                                                    char **firmwareversion);

/**
 * Get GRO value of an ethernet interface
 *
 * @param ta        Test agent name
 * @param ifname    Interface name
 * @param gro       Location for the GRO value
 *
 * @return Status code
 */
extern te_errno tapi_cfg_if_gro_get(const char *ta, const char *ifname,
                                    int *gro);

/**
 * Get GSO value of an ethernet interface
 *
 * @param ta        Test agent name
 * @param ifname    Interface name
 * @param gso       Location for the GSO value
 *
 * @return Status code
 */
extern te_errno tapi_cfg_if_gso_get(const char *ta, const char *ifname,
                                    int *gso);

/**
 * Get TSO value of an ethernet interface
 *
 * @param ta        Test agent name
 * @param ifname    Interface name
 * @param tso       The TSO value location
 *
 * @return Status code
 */
extern te_errno tapi_cfg_if_tso_get(const char *ta, const char *ifname,
                                    int *tso);

/**
 * Get flags value of an ethernet interface
 *
 * @param ta        Test agent name
 * @param ifname    Interface name
 * @param flags     Location for the eth flags
 *
 * @return Status code
 */
extern te_errno tapi_cfg_if_flags_get(const char *ta, const char *ifname,
                                      int *flags);

/**
 * Get network interface Rx or Tx ring size
 *
 * @param is_rx         Rx or Tx option
 * @param ta            Test agent name
 * @param ifname        Interface name
 * @param ring_size     Location for the ring size
 *
 * @return Status code
 */
extern te_errno tapi_cfg_if_get_ring_size(const char *ta,
                                          const char *ifname,
                                          bool is_rx,
                                          int *ring_size);

/**
 * Get network interface Rx or Tx preset maximum ring size
 *
 * @param is_rx             Rx or Tx option
 * @param ta                Test agent name
 * @param ifname            Interface name
 * @param max_ring_size     Location for the ring size
 *
 * @return Status code
 */
extern te_errno tapi_cfg_if_get_max_ring_size(const char *ta,
                                              const char *ifname,
                                              bool is_rx,
                                              int *max_ring_size);

/**
 * Set GRO value of an ethernet interface
 *
 * @param ta        Test agent name
 * @param ifname    Interface name
 * @param gro       A new GRO value
 *
 * @return Status code
 */
extern te_errno tapi_cfg_if_gro_set(const char *ta, const char *ifname,
                                    int gro);

/**
 * Set GSO value of an ethernet interface
 *
 * @param ta        Test agent name
 * @param ifname    Interface name
 * @param gso       A new GSO value
 *
 * @return Status code
 */
extern te_errno tapi_cfg_if_gso_set(const char *ta, const char *ifname,
                                 int gso);

/**
 * Set TSO value of an ethernet interface
 *
 * @param ta        Test agent name
 * @param ifname    Interface name
 * @param tso       The TSO value
 *
 * @return Status code
 */
extern te_errno tapi_cfg_if_tso_set(const char *ta, const char *ifname,
                                    int tso);

/**
 * Set flags value of an ethernet interface
 *
 * @param ta        Test agent name
 * @param ifname    Interface name
 * @param flags     A new eth flags value
 *
 * @return Status code
 */
extern te_errno tapi_cfg_if_flags_set(const char *ta, const char *ifname,
                                      int flags);

/**
 * Set network interface Rx or Tx ring size
 *
 * @param is_rx         Rx or Tx option
 * @param ta            Test agent name
 * @param ifname        Interface name
 * @param ring_size     New ring size to set
 *
 * @return Status code
 */
extern te_errno tapi_cfg_if_set_ring_size(const char *ta, const char *ifname,
                                          bool is_rx, int ring_size);

/**
 * Reset an ethernet interface
 *
 * @param ta        Test agent name
 * @param ifname    Interface name
 *
 * @return Status code
 */
extern te_errno tapi_cfg_if_reset(const char *ta, const char *ifname);

/**
 * Network interface driver message level flags. Values should be the same
 * as values of corresponding NETIF_MSG* flags from
 * include/linux/netdevice.h in linux kernel sources.
 */
typedef enum {
    TAPI_NETIF_MSG_DRV = 0x0001,
    TAPI_NETIF_MSG_PROBE = 0x0002,
    TAPI_NETIF_MSG_LINK = 0x0004,
    TAPI_NETIF_MSG_TIMER = 0x0008,
    TAPI_NETIF_MSG_IFDOWN = 0x0010,
    TAPI_NETIF_MSG_IFUP = 0x0020,
    TAPI_NETIF_MSG_RX_ERR = 0x0040,
    TAPI_NETIF_MSG_TX_ERR = 0x0080,
    TAPI_NETIF_MSG_TX_QUEUED = 0x0100,
    TAPI_NETIF_MSG_INTR = 0x0200,
    TAPI_NETIF_MSG_TX_DONE = 0x0400,
    TAPI_NETIF_MSG_RX_STATUS = 0x0800,
    TAPI_NETIF_MSG_PKTDATA = 0x1000,
    TAPI_NETIF_MSG_HW = 0x2000,
    TAPI_NETIF_MSG_WOL = 0x4000,
} tapi_netif_msg_level;

/** All flags from te_netif_msg_level */
#define TAPI_NETIF_MSG_ALL \
    (TAPI_NETIF_MSG_DRV | TAPI_NETIF_MSG_PROBE |\
     TAPI_NETIF_MSG_LINK | TAPI_NETIF_MSG_TIMER |\
     TAPI_NETIF_MSG_IFDOWN | TAPI_NETIF_MSG_IFUP |\
     TAPI_NETIF_MSG_RX_ERR | TAPI_NETIF_MSG_TX_ERR |\
     TAPI_NETIF_MSG_TX_QUEUED | TAPI_NETIF_MSG_INTR |\
     TAPI_NETIF_MSG_TX_DONE | TAPI_NETIF_MSG_RX_STATUS |\
     TAPI_NETIF_MSG_PKTDATA | TAPI_NETIF_MSG_HW |\
     TAPI_NETIF_MSG_WOL)

/**
 * Get current network interface driver message level.
 *
 * @param ta        Test Agent name
 * @param ifname    Interface name
 * @param msglvl    Where to save driver message level
 *                  (see tapi_netif_msg_level)
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_if_msglvl_get(const char *ta, const char *ifname,
                                       uint64_t *msglvl);

/**
 * Set current network interface driver message level.
 *
 * @param ta        Test Agent name
 * @param ifname    Interface name
 * @param msglvl    Driver message level to set (see te_netif_msg_level)
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_if_msglvl_set(const char *ta, const char *ifname,
                                       uint64_t msglvl);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_CFG_IF_H__ */

/**@} <!-- END tapi_conf_if --> */
