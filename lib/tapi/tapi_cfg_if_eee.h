/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2025 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief Basic Configuration Model TAPI
 *
 * Definition of test API for Energy Efficient Ethernet
 * configuration (storage/cm/cm_base.xml).
 */

#ifndef __TE_TAPI_CFG_IF_EEE_H__
#define __TE_TAPI_CFG_IF_EEE_H__

#include "te_defs.h"
#include "te_errno.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Get EEE enable state.
 *
 * @param ta        Test Agent name.
 * @param if_name   Interface name.
 * @param enabled   Location for obtained value.
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_if_eee_enabled_get(const char *ta,
                                            const char *if_name,
                                            bool *enabled);

/**
 * Set EEE enable state.
 *
 * @param ta        Test Agent name.
 * @param if_name   Interface name.
 * @param enabled   Value to set.
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_if_eee_enabled_set(const char *ta,
                                            const char *if_name,
                                            bool enabled);

/**
 * Set EEE enable state locally to be committed later.
 *
 * @param ta        Test Agent name.
 * @param if_name   Interface name.
 * @param enabled   Value to set.
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_if_eee_enabled_set_local(const char *ta,
                                                  const char *if_name,
                                                  bool enabled);

/**
 * Get EEE active state.
 *
 * @param ta        Test Agent name.
 * @param if_name   Interface name.
 * @param active    Location for obtained value.
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_if_eee_active_get(const char *ta,
                                           const char *if_name,
                                           bool *active);

/**
 * Get EEE TX Low Power Idle (LPI) enable state.
 *
 * @param ta        Test Agent name.
 * @param if_name   Interface name.
 * @param enabled   Location for obtained value.
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_if_eee_tx_lpi_enabled_get(const char *ta,
                                                   const char *if_name,
                                                   bool *enabled);

/**
 * Set EEE TX Low Power Idle (LPI) enable state.
 *
 * @param ta        Test Agent name.
 * @param if_name   Interface name.
 * @param enabled   Value to set.
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_if_eee_tx_lpi_enabled_set(const char *ta,
                                                   const char *if_name,
                                                   bool enabled);

/**
 * Set EEE TX Low Power Idle (LPI) enable state locally to be committed
 * later.
 *
 * @param ta        Test Agent name.
 * @param if_name   Interface name.
 * @param enabled   Value to set.
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_if_eee_tx_lpi_enabled_set_local(
                                                      const char *ta,
                                                      const char *if_name,
                                                      bool enabled);

/**
 * Get EEE TX Low Power Idle (LPI) timer delay.
 *
 * @param ta        Test Agent name.
 * @param if_name   Interface name.
 * @param timer     Location for obtained value.
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_if_eee_tx_lpi_timer_get(const char *ta,
                                                 const char *if_name,
                                                 uint32_t *timer);

/**
 * Set EEE TX Low Power Idle (LPI) timer delay.
 *
 * @param ta        Test Agent name.
 * @param if_name   Interface name.
 * @param timer     Value to set.
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_if_eee_tx_lpi_timer_set(const char *ta,
                                                 const char *if_name,
                                                 uint32_t timer);

/**
 * Set EEE TX Low Power Idle (LPI) timer delay locally to be committed
 * later.
 *
 * @param ta        Test Agent name.
 * @param if_name   Interface name.
 * @param timer     Value to set.
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_if_eee_tx_lpi_timer_set_local(const char *ta,
                                                       const char *if_name,
                                                       uint32_t timer);

/**
 * Commit local changes made to EEE configuration.
 *
 * @param ta        Test Agent name.
 * @param if_name   Interface name.
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_if_eee_commit(const char *ta,
                                       const char *if_name);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __TE_TAPI_CFG_IF_EEE_H__ */
