/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Network Configuration Model TAPI
 *
 * Definition of test API for PHY configuration model
 * (storage/cm/cm_base.xml).
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_TAPI_CFG_PHY_H__
#define __TE_TAPI_CFG_PHY_H__

#include "te_errno.h"
#include "logger_api.h"
#include "conf_api.h"
#include "te_ethernet_phy.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup tapi_conf_phy Ethernet PHY configuration
 * @ingroup tapi_conf
 * @{
 */

/* Time to sleep after PHY properties has been changed */
#define TE_PHY_SLEEP_TIME   (10)

/**
 * Get PHY autonegotiation state.
 *
 * @param ta            Test Agent name
 * @param if_name       Interface name
 * @param state         Pointer to the returned autonegotiation state value:
 *                      TE_PHY_AUTONEG_OFF - autonegotiation OFF
 *                      TE_PHY_AUTONEG_ON  - autonegotiation ON
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_phy_autoneg_get(const char *ta,
                                         const char *if_name,
                                         int *state);

/**
 * Set PHY autonegotiation state.
 *
 * @param ta            Test Agent name
 * @param if_name       Interface name
 * @param state         Autonegotiation state value:
 *                      TE_PHY_AUTONEG_OFF - autonegotiation OFF
 *                      TE_PHY_AUTONEG_ON  - autonegotiation ON
 *
 * @return Status code
 */
extern te_errno tapi_cfg_phy_autoneg_set(const char *ta,
                                         const char *if_name,
                                         int state);

/**
 * Get PHY duplex oper state.
 *
 * NOTE: Output parameters are valid if function returns
 * positive result only.
 *
 * @param ta            Test Agent name
 * @param if_name       Interface name
 * @param state         Pointer to the returned duplex state value:
 *                      TE_PHY_DUPLEX_HALF - half duplex
 *                      TE_PHY_DUPLEX_FULL - full duplex
 *
 * @return Status code
 */
extern te_errno tapi_cfg_phy_duplex_oper_get(const char *ta,
                                             const char *if_name,
                                             int *state);

/**
 * Get PHY duplex admin state.
 *
 * NOTE: Output parameters are valid if function returns
 * positive result only.
 *
 * @param ta            Test Agent name
 * @param if_name       Interface name
 * @param state         Pointer to the returned duplex state value:
 *                      TE_PHY_DUPLEX_HALF - half duplex
 *                      TE_PHY_DUPLEX_FULL - full duplex
 *
 * @return Status code
 */
extern te_errno tapi_cfg_phy_duplex_admin_get(const char *ta,
                                              const char *if_name,
                                              int *state);

/**
 * Set PHY duplex state.
 *
 * @param ta            Test Agent name
 * @param if_name       Interface name
 * @param state         Duplex state value:
 *                      TE_PHY_DUPLEX_HALF - half duplex
 *                      TE_PHY_DUPLEX_FULL - full duplex
 *
 * @return Status code
 */
extern te_errno tapi_cfg_phy_duplex_admin_set(const char *ta,
                                              const char *if_name,
                                              int state);

/**
 * Get PHY speed oper value.
 *
 * @param ta            Test Agent name
 * @param if_name       Interface name
 * @param speed         Pointer to the returned speed value
 *                      (in Mbit/sec)
 *
 * @return Status code
 */
extern te_errno tapi_cfg_phy_speed_oper_get(const char *ta,
                                            const char *if_name,
                                            int *speed);
/**
 * Get PHY speed admin value.
 *
 * @param ta            Test Agent name
 * @param if_name       Interface name
 * @param speed         Pointer to the returned speed value
 *                      (in Mbit/sec)
 *
 * @return Status code
 */
extern te_errno tapi_cfg_phy_speed_admin_get(const char *ta,
                                             const char *if_name,
                                             int *speed);

/**
 * Set PHY speed.
 *
 * @param ta            Test Agent name
 * @param if_name       Interface name
 * @param speed         Speed value (Mbit/sec)
 *
 * @return Status code
 */
extern te_errno tapi_cfg_phy_speed_admin_set(const char *ta,
                                             const char *if_name,
                                             int speed);

/**
 * Get PHY interface mode: speed and duplex oper state.
 *
 * @param ta            Test Agent name
 * @param if_name       Interface name
 * @param speed         Speed value (Mbit/sec)
 * @param duplex        Duplex state value
 *                      TE_PHY_DUPLEX_HALF - half duplex
 *                      TE_PHY_DUPLEX_FULL - full duplex
 *
 * @return Status code
 */
extern te_errno tapi_cfg_phy_mode_oper_get(const char *ta,
                                           const char *if_name,
                                           int *speed, int *duplex);
/**
 * Get PHY interface mode: speed and duplex admin state.
 *
 * @param ta            Test Agent name
 * @param if_name       Interface name
 * @param speed         Speed value (Mbit/sec)
 * @param duplex        Duplex state value
 *                      TE_PHY_DUPLEX_HALF - half duplex
 *                      TE_PHY_DUPLEX_FULL - full duplex
 *
 * @return Status code
 */
extern te_errno tapi_cfg_phy_mode_admin_get(const char *ta,
                                            const char *if_name,
                                            int *speed, int *duplex);

/**
 * Set PHY interface mode: speed and duplex state.
 *
 * @param ta            Test Agent name
 * @param if_name       Interface name
 * @param speed         Speed value (Mbit/sec)
 * @param duplex        Duplex state value
 *                      TE_PHY_DUPLEX_HALF - half duplex
 *                      TE_PHY_DUPLEX_FULL - full duplex
 *
 * @return Status code
 */
extern te_errno tapi_cfg_phy_mode_admin_set(const char *ta,
                                            const char *if_name,
                                            int speed, int duplex);

/**
 * Get PHY link state.
 *
 * NOTE: Output parameters are valid if function returns
 * positive result only.
 *
 * @param ta            Test Agent name
 * @param if_name       Interface name
 * @param state         Pointer to the returned link state value:
 *                      TE_PHY_STATE_DOWN - link down
 *                      TE_PHY_STATE_UP   - link up
 *
 * @return Status code
 */
extern te_errno tapi_cfg_phy_state_get(const char *ta, const char *if_name,
                                       int *state);

/**
 * Wait until an interface is UP.
 *
 * @param ta            Test Agent name
 * @param if_name       Interface name
 * @param timeout       Timeout (in milliseconds)
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_phy_state_wait_up(const char *ta,
                                           const char *if_name,
                                           int timeout);

/**
 * Check that PHY mode is advertised.
 *
 * @note This function only checks link modes defining link speed
 *       and duplex. If any of the link modes with matching
 *       speed and duplex is enabled, this function reports
 *       advertised state.
 *       To check specific link mode or link mode not related to
 *       speed/duplex use tapi_cfg_phy_mode_get().
 *
 * @param ta            Test Agent name
 * @param if_name       Interface name
 * @param speed         Speed value (Mbit/sec, see linux/ethtool.h for
 *                      more details)
 * @param duplex        Duplex state value:
 *                      TE_PHY_DUPLEX_HALF - half duplex
 *                      TE_PHY_DUPLEX_FULL - full duplex
 * @param state         Pointer to mode state:
 *                      @c true - the mode is advertised
 *                      @c false - the mode is not advertised
 *
 * @return Status code
 */
extern te_errno tapi_cfg_phy_is_mode_advertised(const char *ta,
                                                const char *if_name,
                                                int speed, int duplex,
                                                bool *state);

/**
 * Set PHY mode to advertising state.
 *
 * @note This function will change advertising state for all
 *       link modes with matching speed and duplex.
 *       For more detailed control use tapi_cfg_phy_mode_set().
 *
 * @param ta            Test Agent name
 * @param if_name       Interface name
 * @param speed         Speed value (Mbit/sec, see linux/ethtool.h for
 *                      more details)
 * @param duplex        Duplex state value:
 *                      TE_PHY_DUPLEX_HALF - half duplex
 *                      TE_PHY_DUPLEX_FULL - full duplex
 * @param state         Mode state:
 *                      @c false - the mode is not advertised
 *                      @c true - the mode is advertised
 *
 * @return Status code
 */
extern te_errno tapi_cfg_phy_advertise_mode(const char *ta,
                                            const char *if_name,
                                            int speed, int duplex,
                                            bool state);

/**
 * Check whether a specific link mode is supported.
 *
 * @param ta            Test Agent name
 * @param if_name       Interface name
 * @param mode_name     Link mode name
 * @param supported     Will be set to @c true if the requested link
 *                      mode is supported, to @c false otherwise
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_phy_mode_supported(const char *ta,
                                            const char *if_name,
                                            const char *mode_name,
                                            bool *supported);

/**
 * Check whether a specific link mode is advertised.
 *
 * @param ta            Test Agent name
 * @param if_name       Interface name
 * @param mode_name     Link mode name
 * @param state         Will be set to @c true if the requested link
 *                      mode is advertised, to @c false otherwise
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_phy_mode_adv_get(const char *ta,
                                          const char *if_name,
                                          const char *mode_name,
                                          bool *state);

/**
 * Set advertising state for a link mode.
 *
 * @param ta            Test Agent name
 * @param if_name       Interface name
 * @param mode_name     Link mode name
 * @param state         If @c true, link mode should be advertised,
 *                      if @c false - it should not be advertised
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_phy_mode_adv_set(const char *ta,
                                          const char *if_name,
                                          const char *mode_name,
                                          bool state);

/**
 * Check whether link partner advertises a given link mode.
 *
 * @param ta            Test Agent name
 * @param if_name       Interface name
 * @param mode_name     Link mode name
 * @param advertised    Will be set to @c true if link partner advertises
 *                      the link mode and to @c false otherwise
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_phy_lp_advertised(const char *ta,
                                           const char *if_name,
                                           const char *mode_name,
                                           bool *advertised);

/**
 * Commit PHY interface changes to TAs.
 *
 * @param ta            Test Agent name
 * @param if_name       Interface name
 *
 * @return Status code
 */
extern te_errno tapi_cfg_phy_commit(const char *ta, const char *if_name);

/**
 * Get PHY duplex state by name string.
 *
 * @param name          Duplex state name string
 *
 * @return TE_PHY_DUPLEX_HALF - half duplex;
 *         TE_PHY_DUPLEX_FULL - full duplex;
 *         or -1 if name string does not recognized
 */
extern int tapi_cfg_phy_duplex_str2id(char *name);

/**
 * Get PHY duplex state by id.
 *
 * @param duplex        Duplex state id
 *
 * @return half - half duplex;
 *         full - full duplex;
 *         or NULL if id does not recognized
 */
extern char *tapi_cfg_phy_duplex_id2str(int duplex);

/**
 * Turn off all advertised modes and advertise only one.
 *
 * FIXME: this function calls tapi_cfg_phy_commit(), it may
 * be not convenient.
 *
 * @param ta            Test agent
 * @param if_name       Interface name
 * @param advert_speed  Speed to advertise (Mbit/sec)
 * @param advert_duplex Duplex to advertise
 *
 * @return Operation status code.
 */
extern te_errno tapi_cfg_phy_advertise_one(const char *ta,
                                           const char *if_name,
                                           int advert_speed,
                                           int advert_duplex);

/**
 * Get PHY link partner advertised pause frame use.
 *
 * NOTE: Output parameters are valid if function returns
 * positive result only.
 *
 * @param ta            Test Agent name
 * @param if_name       Interface name
 * @param state         Pointer to the returned pause frame use value:
 *                      TE_PHY_PAUSE_NONE               - no pause frame use
 *                      TE_PHY_PAUSE_TX_ONLY            - transmit only
 *                      TE_PHY_PAUSE_SYMMETRIC          - symmetric
 *                      TE_PHY_PAUSE_SYMMETRIC_RX_ONLY  - symmetric or receive only
 *
 * @return Status code
 */
extern te_errno tapi_cfg_phy_pause_lp_adv_get(const char *ta,
                                              const char *if_name,
                                              int *state);

/**
 * Get PHY link partner advertised autonegotiation state.
 *
 * NOTE: Output parameters are valid if function returns
 * positive result only.
 *
 * @param ta            Test Agent name
 * @param if_name       Interface name
 * @param state         Pointer to the returned autonegotiation state value:
 *                      TE_PHY_AUTONEG_OFF      - autonegotiation OFF
 *                      TE_PHY_AUTONEG_ON       - autonegotiation ON
 *
 * @return Status code
 */
extern te_errno tapi_cfg_phy_autoneg_lp_adv_get(const char *ta,
                                                const char *if_name,
                                                int *state);


/*
 * Deprecated functions which should no longer be used
 */

/**
 * Get PHY autonegotiation oper state.
 *
 * @note This function is outdated, use tapi_cfg_phy_autoneg_get().
 *
 * @param ta            Test Agent name
 * @param if_name       Interface name
 * @param state         Pointer to the returned autonegotiation state value:
 *                      TE_PHY_AUTONEG_OFF - autonegotiation OFF
 *                      TE_PHY_AUTONEG_ON  - autonegotiation ON
 *
 * @return Status code
 */
extern te_errno tapi_cfg_phy_autoneg_oper_get(const char *ta,
                                              const char *if_name,
                                              int *state);
/**
 * Get PHY autonegotiation admin state.
 *
 * @note This function is outdated, use tapi_cfg_phy_autoneg_get().
 *
 * @param ta            Test Agent name
 * @param if_name       Interface name
 * @param state         Pointer to the returned autonegotiation state value:
 *                      TE_PHY_AUTONEG_OFF - autonegotiation OFF
 *                      TE_PHY_AUTONEG_ON  - autonegotiation ON
 *
 * @return Status code
 */
extern te_errno tapi_cfg_phy_autoneg_admin_get(const char *ta,
                                               const char *if_name,
                                               int *state);

/**
 * Set PHY autonegotiation state.
 *
 * @note This function is outdated, use tapi_cfg_phy_autoneg_set().
 *
 * @param ta            Test Agent name
 * @param if_name       Interface name
 * @param state         Autonegotiation state value:
 *                      TE_PHY_AUTONEG_OFF - autonegotiation OFF
 *                      TE_PHY_AUTONEG_ON  - autonegotiation ON
 *
 * @return Status code
 */
extern te_errno tapi_cfg_phy_autoneg_admin_set(const char *ta,
                                               const char *if_name,
                                               int state);

/**@} <!-- END tapi_conf_phy --> */

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TE_TAPI_CFG_PHY_H__ */
