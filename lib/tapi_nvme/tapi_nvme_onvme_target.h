/** @file
 * @brief ONVMe target
 *
 * @defgroup tapi_nvme_onvme_target ONVMe target
 * @ingroup tapi_nvme
 * @{
 *
 * API for control ONVMe target of NVMe Over Fabrics
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 */

#ifndef __TAPI_NVME_ONVME_TARGET_H__
#define __TAPI_NVME_ONVME_TARGET_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "tapi_nvme.h"
#include "tapi_job.h"

/** ONVMe target opts */
typedef struct tapi_nvme_onvme_target_opts {
    te_bool is_nullblock;       /**< Target use nullblock instend nvme dev */
    te_string cores;            /**< Target will use the specified cpu */
    unsigned max_worker_conn;   /**< Maximum number of connections to accept
                                     per worker */
    int log_level;              /**< Log level of ONVMe proccess */
} tapi_nvme_onvme_target_opts;

/** ONVMe target opts defaults */
#define TAPI_NVME_ONVME_TARGET_OPTS_DEFAULTS (tapi_nvme_onvme_target_opts) { \
    .is_nullblock = FALSE,                                                   \
    .cores = TE_STRING_INIT,                                                 \
    .max_worker_conn = 0,                                                    \
    .log_level = -1,                                                         \
}

/** ONVMe target context */
typedef struct tapi_nvme_onvme_target_proc {
    tapi_job_t *onvme_job;              /**< ONVMe proccess of target */
    tapi_job_channel_t *out_chs[2];     /**< stdout/sterr channels */
    tapi_nvme_onvme_target_opts opts;   /**< Options for ONVMe target process */
} tapi_nvme_onvme_target_proc;

/** Default options for ONVMe target process */
#define TAPI_NVME_ONVME_TARGET_PROC_DEFAULTS (tapi_nvme_onvme_target_proc) {  \
    .onvme_job = NULL,                                                        \
    .out_chs = {},                                                            \
    .opts = TAPI_NVME_ONVME_TARGET_OPTS_DEFAULTS,                             \
}

/** ONVMe target methods */
#define TAPI_NVME_ONVME_TARGET_METHODS (tapi_nvme_target_methods) { \
    .init = tapi_nvme_onvme_target_init,                            \
    .setup = tapi_nvme_onvme_target_setup,                          \
    .cleanup = tapi_nvme_onvme_target_cleanup,                      \
    .fini = tapi_nvme_onvme_target_fini,                            \
}

/** Default ONVMe target initialization */
#define TAPI_NVME_ONVME_TARGET (tapi_nvme_target) {  \
    .list = LIST_HEAD_INITIALIZER(list),             \
    .rpcs = NULL,                                    \
    .transport = TAPI_NVME_TRANSPORT_TCP,            \
    .subnqn = "te_testing",                          \
    .nvmet_port = 1,                                 \
    .device = NULL,                                  \
    .addr = NULL,                                    \
    .methods = TAPI_NVME_ONVME_TARGET_METHODS,       \
    .impl = NULL,                                    \
}

/**
 * Init ONVMe implementation of target
 *
 * @param target        Target for setup
 * @param opts          ONVMe target options
 *
 * @return Status code
 */
extern te_errno tapi_nvme_onvme_target_init(
    tapi_nvme_target *target, void *opts);

/**
 * Setup ONVMe target
 *
 * @param target        Target for setup
 *
 * @return Status code
 */
extern te_errno tapi_nvme_onvme_target_setup(tapi_nvme_target *target);

/**
 * Cleanup ONVMe target
 *
 * @param target        Target for cleanup
 *
 * @return Status code
 */
extern void tapi_nvme_onvme_target_cleanup(tapi_nvme_target *target);

/**
 * Deinit ONVMe target
 *
 * @param target        Target for deinit
 *
 * @return Status code
 */
extern void tapi_nvme_onvme_target_fini(tapi_nvme_target *target);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __TAPI_NVME_ONVME_TARGET_H__ */

/**@} <!-- END tapi_nvme_onvme_target --> */
