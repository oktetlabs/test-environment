/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2022 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief Block Device Configuration Model TAPI
 *
 * Definition of test API for block devices configuration model
 * (doc/cm/cm_block.yml).
 */

#ifndef __TE_TAPI_CFG_BLOCK_H__
#define __TE_TAPI_CFG_BLOCK_H__

#include "te_errno.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize loop block devices subsystem on the agent @p ta.
 * In particular, it implies loading the required kernel modules.
 *
 * @param agent  agent name
 *
 * @return status code
 */
extern te_errno tapi_cfg_block_initialize_loop(const char *ta);

/**@} <!-- END tapi_conf_block --> */

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_CFG_BLOCK_H__ */
