/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) OKTET Ltd. All rights reserved. */
/** @file
 * @brief Test API to get info about RAM.
 *
 * Definition of API to get info about RAM.
 */

#ifndef __TE_TAPI_CFG_MEMORY_H__
#define __TE_TAPI_CFG_MEMORY_H__

#include "conf_api.h"
#include "te_defs.h"
#include "te_errno.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup tapi_conf_memory RAM configuration of Test Agents
 * @ingroup tapi_conf
 * @{
 */

/**
 * Get amount of memory in bytes on a test agent.
 *
 * @param[in]  ta               test agent
 * @param[in]  node_id          node ID
 * @param[out] memory           amount of system memory in bytes
 *
 * @return status code
 */
extern te_errno tapi_cfg_get_memory(const char *ta, int node_id,
                                    uint64_t *memory);

/**
 * Get amount of free memory in bytes on a test agent.
 *
 * @param[in]  ta               test agent
 * @param[in]  node_id          node ID
 * @param[out] avail_mem        amount of available memory in bytes
 *
 * @return status code
 */
extern te_errno tapi_cfg_get_free_memory(const char *ta, int node_id,
                                         uint64_t *avail_mem);

/**@} <!-- END tapi_conf_memory --> */

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_CFG_MEMORY_H__ */
