/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Unix Test Agent
 *
 * Unix TA processes support
 *
 * Copyright (C) 2020-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_AGENTS_UNIX_CONF_PROCESS_H_
#define __TE_AGENTS_UNIX_CONF_PROCESS_H_

#include "te_errno.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize processes configuration.
 *
 * @return Status code.
 */
extern te_errno ta_unix_conf_ps_init(void);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_AGENTS_UNIX_CONF_PROCESS_H_ */
