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

#ifdef __cplusplus
extern "C" {
#endif

/** Initialize ta_events agent configuration subtree. */
extern te_errno rcf_pch_ta_events_conf_init(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* !__TE_RCF_PCH_TA_EVENTS_H__ */
