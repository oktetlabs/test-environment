/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Configurator
 *
 * RCF interaction auxiliary routines
 *
 * Copyright (C) 2019-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_CONF_RCF_H__
#define __TE_CONF_RCF_H__

#include "te_errno.h"

#include "conf_db.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Handle /rcf: subtree add requests.
 *
 * @return Status code.
 */
extern te_errno cfg_rcf_add(cfg_instance *inst);

/**
 * Handle /rcf: subtree delete requests.
 *
 * @return Status code.
 */
extern te_errno cfg_rcf_del(cfg_instance *inst);

/**
 * Handle /rcf: subtree set requests.
 *
 * @return Status code.
 */
extern te_errno cfg_rcf_set(cfg_instance *inst);

#ifdef __cplusplus
}
#endif
#endif /* __TE_CONF_RCF_H__ */
