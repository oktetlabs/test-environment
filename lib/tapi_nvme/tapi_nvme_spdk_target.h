/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Spdk target
 *
 * @defgroup tapi_nvme_spdk_target Spdk target
 * @ingroup tapi_nvme
 * @{
 *
 * API for control Spdk target of NVMe Over Fabrics
 *
 * Copyright (C) 2025 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TAPI_NVME_SPDK_TARGET_H__
#define __TAPI_NVME_SPDK_TARGET_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "tapi_nvme.h"
#include "tapi_job.h"
#include "tapi_spdk_rpc.h"

/** Spdk target opts */
typedef struct tapi_nvme_spdk_target_opts {
    /** Path to SPDK rpc.py script */
    const char *rpc_path;
} tapi_nvme_spdk_target_opts;

/**
 * Init Spdk implementation of target
 *
 * @param target        Target for setup
 * @param opts          Spdk target options
 *
 * @return Status code
 */
extern te_errno tapi_nvme_spdk_target_init(tapi_nvme_target *target,
                                           void             *opts);

/**
 * Setup Spdk target
 *
 * @param target        Target for setup
 *
 * @return Status code
 */
extern te_errno tapi_nvme_spdk_target_setup(tapi_nvme_target *target);

/**
 * Cleanup Spdk target
 *
 * @param target        Target for cleanup
 *
 * @return Status code
 */
extern void tapi_nvme_spdk_target_cleanup(tapi_nvme_target *target);

/**
 * Deinit Spdk target
 *
 * @param target        Target for deinit
 *
 * @return Status code
 */
extern void tapi_nvme_spdk_target_fini(tapi_nvme_target *target);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __TAPI_NVME_SPDK_TARGET_H__ */

/**@} <!-- END tapi_nvme_spdk_target --> */
