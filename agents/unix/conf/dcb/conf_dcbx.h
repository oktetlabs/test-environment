/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2026 Advanced Micro Devices, Inc. All rights reserved. */
/** @file
 * @brief Unix TA Traffic Control DCBX configuration support
 *
 * Interface of configuration nodes DCBX support.
 *
 * This API does not attempt to restrict the user to any specific standard
 * features. The user is free to specify any parameter values they deem
 * necessary.
 */

#ifndef __TE_AGENTS_UNIX_CONF_DCB_CONF_DCBX_H_
#define __TE_AGENTS_UNIX_CONF_DCB_CONF_DCBX_H_

#include "te_errno.h"

/**
 * Register the DCBX configuration subtree.
 *
 * @return Status code.
 */
extern te_errno ta_unix_conf_dcbx_init(void);

#endif /* __TE_AGENTS_UNIX_CONF_DCB_CONF_DCBX_H_ */
