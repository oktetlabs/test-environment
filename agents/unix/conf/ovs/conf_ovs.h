/** @file
 * @brief Unix Test Agent
 *
 * Unix TA Open vSwitch deployment.
 *
 * Copyright (C) 2019-2022 OKTET Labs. All rights reserved.
 *
 */

#ifndef __TE_AGENTS_UNIX_CONF_OVS_H_
#define __TE_AGENTS_UNIX_CONF_OVS_H_

#include "te_errno.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * Initialise Open vSwitch deployment subtree.
 *
 * @return Status code.
 */
extern te_errno ta_unix_conf_ovs_init(void);

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */
#endif /* !__TE_AGENTS_UNIX_CONF_OVS_H_ */
