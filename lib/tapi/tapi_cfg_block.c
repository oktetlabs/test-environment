/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2022 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief Block Device Configuration Model TAPI
 *
 * Definition of test API for block devices configuration model
 * (doc/cm/cm_block.yml).
 */

#define TE_LGR_USER "Configuration TAPI"

#include "te_config.h"

#include "tapi_cfg_modules.h"
#include "tapi_cfg_block.h"

#define CFG_BLOCK_DEVICE_FMT "/agent:%s/block:%s"

#define LOOP_BLOCK_KMOD "loop"

te_errno
tapi_cfg_block_initialize_loop(const char *ta)
{
    return tapi_cfg_module_add(ta, LOOP_BLOCK_KMOD, TRUE);
}
