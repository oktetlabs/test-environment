/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2022 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief Basic Configuration Model TAPI
 *
 * Definition of test API for Network Interface flow control configuration
 * (doc/cm/cm_base.xml).
 */

#ifndef __TE_TAPI_CFG_IF_FLOW_CONTROL_H__
#define __TE_TAPI_CFG_IF_FLOW_CONTROL_H__

#include "te_errno.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Get flow control autonegotiation state.
 *
 * @param ta        Test agent name
 * @param ifname    Interface name
 * @param autoneg   Where to save flow control autonegotiation state
 *                  (@c 0 - disabled, @c 1 - enabled)
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_if_fc_autoneg_get(const char *ta,
                                           const char *ifname,
                                           int *autoneg);

/**
 * Set flow control autonegotiation state.
 *
 * @param ta        Test agent name
 * @param ifname    Interface name
 * @param autoneg   State to set (@c 0 - disabled, @c 1 - enabled)
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_if_fc_autoneg_set(const char *ta,
                                           const char *ifname,
                                           int autoneg);

/**
 * Set flow control autonegotiation state locally, to commit it later.
 *
 * @param ta        Test agent name
 * @param ifname    Interface name
 * @param autoneg   State to set (@c 0 - disabled, @c 1 - enabled)
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_if_fc_autoneg_set_local(const char *ta,
                                                 const char *ifname,
                                                 int autoneg);

/**
 * Get flow control Rx state.
 *
 * @param ta        Test agent name
 * @param ifname    Interface name
 * @param rx        Where to save flow control Rx state
 *                  (@c 0 - disabled, @c 1 - enabled)
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_if_fc_rx_get(const char *ta,
                                      const char *ifname,
                                      int *rx);

/**
 * Set flow control Rx state.
 *
 * @param ta        Test agent name
 * @param ifname    Interface name
 * @param rx        State to set (@c 0 - disabled, @c 1 - enabled)
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_if_fc_rx_set(const char *ta,
                                      const char *ifname,
                                      int rx);

/**
 * Set flow control Rx state locally, to commit it later.
 *
 * @param ta        Test agent name
 * @param ifname    Interface name
 * @param rx        State to set (@c 0 - disabled, @c 1 - enabled)
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_if_fc_rx_set_local(const char *ta,
                                            const char *ifname,
                                            int rx);

/**
 * Get flow control Tx state.
 *
 * @param ta        Test agent name
 * @param ifname    Interface name
 * @param tx        Where to save flow control Tx state
 *                  (@c 0 - disabled, @c 1 - enabled)
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_if_fc_tx_get(const char *ta,
                                      const char *ifname,
                                      int *tx);

/**
 * Set flow control Tx state.
 *
 * @param ta        Test agent name
 * @param ifname    Interface name
 * @param tx        State to set (@c 0 - disabled, @c 1 - enabled)
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_if_fc_tx_set(const char *ta,
                                      const char *ifname,
                                      int tx);

/**
 * Set flow control Tx state locally, to commit it later.
 *
 * @param ta        Test agent name
 * @param ifname    Interface name
 * @param tx        State to set (@c 0 - disabled, @c 1 - enabled)
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_if_fc_tx_set_local(const char *ta,
                                            const char *ifname,
                                            int tx);

/**
 * Commit all local flow control parameter changes.
 *
 * @param ta        Test agent name
 * @param if_name   Interface name
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_if_fc_commit(const char *ta,
                                      const char *if_name);

/**
 * Interface flow control parameters.
 * Negative value means "do not set", @c 0 - disable, @c 1 - enable.
 */
typedef struct tapi_cfg_if_fc {
    int autoneg;  /**< Flow control autonegotiation */
    int rx;       /**< Rx flow control */
    int tx;       /**< Tx flow control */
} tapi_cfg_if_fc;

/** Initializer for tapi_cfg_if_fc */
#define TAPI_CFG_IF_FC_INIT { .autoneg = -1, .rx = -1, .tx = -1 }

/**
 * Get all flow control parameter values for a given interface.
 *
 * @param ta        Test agent name
 * @param ifname    Interface name
 * @param params    Where to save parameter values
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_if_fc_get(const char *ta,
                                   const char *ifname,
                                   tapi_cfg_if_fc *params);

/**
 * Set flow control parameter values for a given interface.
 * All non-negative parameter values will be set simultaneously
 * by committing all changes at once.
 *
 * @param ta        Test agent name
 * @param ifname    Interface name
 * @param params    Flow control parameter values
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_if_fc_set(const char *ta,
                                   const char *ifname,
                                   tapi_cfg_if_fc *params);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __TE_TAPI_CFG_IF_FLOW_CONTROL_H__ */
