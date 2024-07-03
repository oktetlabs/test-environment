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
#include "te_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup tapi_conf_block Block devices subtree
 * @ingroup tapi_conf
 * @{
 */

/**
 * Initialize loop block devices subsystem on the agent @p ta.
 * In particular, it implies loading the required kernel modules.
 *
 * @param ta  agent name
 *
 * @return status code
 */
extern te_errno tapi_cfg_block_initialize_loop(const char *ta);

/**
 * Grab a block device as a resource.
 *
 * @param ta         agent name
 * @param block_dev  block device name
 *
 * @return status code
 */
extern te_errno tapi_cfg_block_grab(const char *ta, const char *block_dev);

/**
 * Check whether a block device is a loop device.
 *
 * @param ta         Agent name
 * @param block_dev  Block device name
 *
 * @return @c true iff @p block_dev refers to a loop block device on @p ta
 */
extern bool tapi_cfg_block_is_loop(const char *ta, const char *block_dev);

/**
 * Get the name of a backing file for a loop device @p block_dev.
 *
 * If there is no backing file, the value will be @c NULL.
 *
 * @param[in]  ta        agent name
 * @param[in]  block_dev block device name
 * @param[out] filename  the name of the backing file or @c NULL
 *                       (must be free()'d)
 *
 * @return status code
 */
extern te_errno tapi_cfg_block_loop_get_backing_file(const char *ta,
                                                     const char *block_dev,
                                                     char **filename);

/**
 * Set the name of a backing file for a loop device @p block_dev.
 *
 * If the name is @c NULL or empty, the loop device is detached from
 * any backing file.
 *
 * @param  ta        agent name
 * @param  block_dev block device name
 * @param filename   the name of the backing file (may be @c NULL)
 *
 * @return status code
 */
extern te_errno tapi_cfg_block_loop_set_backing_file(const char *ta,
                                                     const char *block_dev,
                                                     const char *filename);

/**@} <!-- END tapi_conf_block --> */

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_CFG_BLOCK_H__ */
