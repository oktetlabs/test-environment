/** @file
 * @brief Kernel target
 *
 * @defgroup tapi_nvme_kern_target Kernel target
 * @ingroup tapi_nvme
 * @{
 *
 * API for control kernel target of NVMe Over Fabrics
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * @author Nikita Somenkov <Nikita.Somenkov@oktetlabs.ru>
 */

#ifndef __TAPI_NVME_KERN_TARGET_H__
#define __TAPI_NVME_KERN_TARGET_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "tapi_nvme.h"

/** Kernel target methods */
#define TAPI_NVME_KERN_METHODS_DEFAULTS (tapi_nvme_target_methods) {    \
    .init = tapi_nvme_kern_target_init,                                 \
    .setup = tapi_nvme_kern_target_setup,                               \
    .cleanup = tapi_nvme_kern_target_cleanup,                           \
    .fini = tapi_nvme_kern_target_fini,                                 \
}

/** Default kernel target initialization */
#define TAPI_NVME_KENR_TARGET (tapi_nvme_target) {  \
    .list = LIST_HEAD_INITIALIZER(list),            \
    .rpcs = NULL,                                   \
    .transport = TAPI_NVME_TRANSPORT_TCP,           \
    .subnqn = "te_testing",                         \
    .nvmet_port = 1,                                \
    .device = NULL,                                 \
    .addr = NULL,                                   \
    .methods = TAPI_NVME_KERN_METHODS_DEFAULTS,     \
    .impl = NULL,                                   \
}

/**
 * Init kernel version implementation of target
 *
 * @param target        Target for setup
 *
 * @return Status code
 */
extern te_errno tapi_nvme_kern_target_init(struct tapi_nvme_target *target, void *opts);

/**
 * Setup kernel version implementation of target
 *
 * @param target        Target for setup
 *
 * @return Status code
 */
extern te_errno tapi_nvme_kern_target_setup(struct tapi_nvme_target *target);

/**
 * Cleanup kernel version implementation of target
 *
 * @param target        Target for setup
 *
 * @return Status code
 */
extern void tapi_nvme_kern_target_cleanup(struct tapi_nvme_target *target);

/**
 * Deinit kernel version implementation of target
 *
 * @param target        Target for setup
 *
 * @return Status code
 */
extern void tapi_nvme_kern_target_fini(struct tapi_nvme_target *target);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __TAPI_NVME_KERN_TARGET_H__ */
