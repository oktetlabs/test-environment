/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Test Agent events functions.
 *
 * TA events configuration tree support.
 *
 * Copyright (C) 2024-2024 ARK NETWORKS LLC. All rights reserved.
 */

#ifndef __TE_RCF_PCH_TA_EVENTS_H__
#define __TE_RCF_PCH_TA_EVENTS_H__

#include "te_errno.h"
#include "te_string.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Initialize ta_events agent configuration subtree. */
extern te_errno rcf_pch_ta_events_conf_init(void);

/**
 * Collect list of RCF clients by given @ref event
 *
 * @param event         TA event name to find
 * @param rcf_clients   List RCF clients subscribed to this TA event
 *
 * @return Number of RCF clients
 */
extern int rcf_pch_ta_events_collect_rcf_clients(const char *event,
                                                 te_string  *rcf_clients);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* !__TE_RCF_PCH_TA_EVENTS_H__ */
