/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2023 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief Basic Configuration Model TAPI
 *
 * Definition of TAPI for network interface channels configuration
 * (doc/cm/cm_base.xml).
 */

#ifndef __TE_TAPI_CFG_IF_CHAN_H__
#define __TE_TAPI_CFG_IF_CHAN_H__

#include "te_errno.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Supported types of channels */
typedef enum tapi_cfg_if_chan {
    /** Channels with only receive queues */
    TAPI_CFG_IF_CHAN_RX,
    /** Channels with only transmit queues */
    TAPI_CFG_IF_CHAN_TX,
    /** Channels used only for other purposes (link interrupts, etc) */
    TAPI_CFG_IF_CHAN_OTHER,
    /** Multi-purpose channels */
    TAPI_CFG_IF_CHAN_COMBINED,
} tapi_cfg_if_chan;

/**
 * Get current number of network channels of a given type.
 *
 * @param ta          Test Agent name
 * @param if_name     Interface name
 * @param chan_type   Network channels type
 * @param num         Location for the number of channels
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_if_chan_cur_get(const char *ta,
                                         const char *if_name,
                                         tapi_cfg_if_chan chan_type,
                                         int *num);

/**
 * Get maximum number of network channels of a given type.
 *
 * @param ta          Test Agent name
 * @param if_name     Interface name
 * @param chan_type   Network channels type
 * @param num         Location for the number of channels
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_if_chan_max_get(const char *ta,
                                         const char *if_name,
                                         tapi_cfg_if_chan chan_type,
                                         int *num);

/**
 * Set current number of network channels of a given type.
 *
 * @param ta          Test Agent name
 * @param if_name     Interface name
 * @param chan_type   Network channels type
 * @param num         Number of channels to set
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_if_chan_cur_set(const char *ta,
                                         const char *if_name,
                                         tapi_cfg_if_chan chan_type,
                                         int num);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __TE_TAPI_CFG_IF_CHAN_H__ */
