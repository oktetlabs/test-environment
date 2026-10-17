/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2026 Advanced Micro Devices, Inc. All rights reserved. */
/** @file
 * @brief Unix TA Traffic Control ETS configuration support
 *
 * Interface of configuration nodes ETS support.
 *
 * This API does not restrict the writable parameters to any specific
 * standard features: the user may set any values they deem necessary.
 * The capabilities (ets_cap, cbs) are read-only.
 */

#ifndef __TE_AGENTS_UNIX_CONF_DCB_CONF_ETS_H_
#define __TE_AGENTS_UNIX_CONF_DCB_CONF_ETS_H_

#include "te_errno.h"

/**
 * Register the ETS configuration subtree.
 *
 * @return Status code.
 */
extern te_errno ta_unix_conf_ets_init(void);

#endif /* __TE_AGENTS_UNIX_CONF_DCB_CONF_ETS_H_ */
