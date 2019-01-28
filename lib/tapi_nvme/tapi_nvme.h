/** @file
 * @brief Control NVMeOF
 *
 * @defgroup tapi_nvme Control NVMeOF
 * @ingroup te_ts_tapi
 * @{
 *
 * API for control NVMe Over Fabrics
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * @author Nikita Somenkov <Nikita.Somenkov@oktetlabs.ru>
 */

#ifndef __TAPI_NVME_H__
#define __TAPI_NVME_H__

#include "te_errno.h"
#include "rcf_rpc.h"

#ifdef __cplusplus
extern "C" {
#endif

/** List of support NVMe transport */
typedef enum {
    TAPI_NVME_TRANSPORT_TCP,     /**< NVMe over TCP */
    TAPI_NVME_TRANSPORT_RDMA,    /**< NVMe over RDMA */
} tapi_nvme_transport;

/** Transport mapping list */
#define TAPI_NVME_TRANSPORT_MAPPING_LIST \
    { "tcp", TAPI_NVME_TRANSPORT_TCP }, \
    { "rdma", TAPI_NVME_TRANSPORT_RDMA }

/** NVMe Qualified Name */
typedef const char * tapi_nvme_subnqn;


struct tapi_nvme_target;

/**
 * Method for init target
 *
 * @param tgt   Target for init
 * @param opts  Options for init target
 *
 * @retrun Status code
 */
typedef te_errno (*tapi_nvme_target_method_init)(
    struct tapi_nvme_target *tgt, void *opts);

/**
 * Method for setup target
 *
 * @param tgt   Target for setup
 *
 * @retrun Status code
 */
typedef te_errno (*tapi_nvme_target_method_setup)(
    struct tapi_nvme_target *tgt);

/**
 * Method for cleanup target
 *
 * @param tgt   Target for cleanup
 *
 * @retrun Status code
 */
typedef void (*tapi_nvme_target_method_cleanup)(
    struct tapi_nvme_target *tgt);

/**
 * Method for fini target
 *
 * @param tgt   Target for setup
 *
 * @retrun Status code
 */
typedef void (*tapi_nvme_target_method_fini)(
    struct tapi_nvme_target *tgt);

/** Available methods of target */
typedef struct tapi_nvme_target_methods {
    tapi_nvme_target_method_init init;       /**< Method for init target */
    tapi_nvme_target_method_setup setup;     /**< Method for setup target */
    tapi_nvme_target_method_cleanup cleanup; /**< Method for cleanup target */
    tapi_nvme_target_method_fini fini;       /**< Method for fini target */
} tapi_nvme_target_methods;

/** Default available methods of target */
#define TAPI_NVME_TARGET_METHODS_DEFAULTS (tapi_nvme_target_methods)    \
{                                                                       \
    .init = NULL,                                                       \
    .setup = NULL,                                                      \
    .cleanup = NULL,                                                    \
    .fini = NULL,                                                       \
}

/** Target context  */
typedef struct tapi_nvme_target {
    LIST_ENTRY(tapi_nvme_target) list;  /**< A way to build lists of
                                          * targets */

    rcf_rpc_server *rpcs;               /**< RPC server handle */
    tapi_nvme_transport transport;      /**< Transport type */
    tapi_nvme_subnqn subnqn;            /**< NVMe Qualified Name */
    unsigned int nvmet_port;            /**< NVMe target port */
    const char *device;                 /**< Name of device */
    const struct sockaddr *addr;        /**< Endpoint to target */
    tapi_nvme_target_methods methods;   /**< Available methods of target */
    void *impl;                         /**< Target specified data */
} tapi_nvme_target;

/** Default target initialization */
#define TAPI_NVME_TARGET_DEFAULTS (struct tapi_nvme_target) {   \
    .rpcs = NULL,                                               \
    .transport = TAPI_NVME_TRANSPORT_TCP,                       \
    .subnqn = "te_testing",                                     \
    .nvmet_port = 1,                                            \
    .device = NULL,                                             \
    .addr = NULL,                                               \
    .methods = TAPI_NVME_TARGET_METHODS_DEFAULTS,               \
    .impl = NULL,                                               \
}

/** Initiator context */
typedef struct tapi_nvme_host_ctrl {
    LIST_ENTRY(tapi_nvme_host_ctrl) list;      /**< A way to build lists of
                                                  * host ctrls */
    rcf_rpc_server *rpcs;                      /**< RPC server handle */
    const tapi_nvme_target *connected_target;  /**< Connected target */
    char *device;                              /**< Name of device */
    char *admin_dev;                           /**< Admin device name */
} tapi_nvme_host_ctrl;

/** Default host_ctrl initialization */
#define TAPI_NVME_HOST_CTRL_DEFAULTS (struct tapi_nvme_host_ctrl) { \
    .rpcs = NULL,               \
    .connected_target = NULL,   \
    .device = NULL,             \
    .admin_dev = NULL,          \
}

/**
 * Convert transport str
 *
 * @param transport     NVMe transport
 * @return transport as string
 */
extern const char * tapi_nvme_transport_str(tapi_nvme_transport transport);

/**
 * Connect initiator host to target host.
 *
 * @param host_ctrl     handle of host_ctrl
 * @param target        handle of target
 *
 * @return TE error code
 */
extern te_errno tapi_nvme_initiator_connect(tapi_nvme_host_ctrl *host_ctrl,
                                            const tapi_nvme_target *target);

/**
 * Disconnect host_ctrl form connected target.
 *
 * @param host_ctrl     handle of host_ctrl
 *
 * @return TE error code
 */
extern te_errno tapi_nvme_initiator_disconnect(tapi_nvme_host_ctrl *host_ctrl);

extern te_errno tapi_nvme_initiator_list(tapi_nvme_host_ctrl *host_ctrl);

static inline void tapi_nvme_initiator_init(tapi_nvme_host_ctrl *host_ctrl)
{
    assert(host_ctrl);
    *host_ctrl = TAPI_NVME_HOST_CTRL_DEFAULTS;
}

/**
 * Target init
 *
 * @param target    handle of target
 * @param opts      options for init target
 * @return TE error code
 */
extern te_errno tapi_nvme_target_init(tapi_nvme_target *target, void *opts);

/**
 * Prepare target for accept connection
 *
 * @param target     handle of target
 *
 * @return TE error code
 */
extern te_errno tapi_nvme_target_setup(tapi_nvme_target *target);

/**
 * Cleanup target
 *
 * @param target     handle of target
 */
extern void tapi_nvme_target_cleanup(tapi_nvme_target *target);

/**
 * Target deinit
 *
 * @param target    handle of target
 * @return TE error code
 */
extern void tapi_nvme_target_fini(tapi_nvme_target *target);

/**
 * Format NVMe disk on target
 *
 * @param target     handle of target
 *
 * @return TE error code
 */
extern te_errno tapi_nvme_target_format(tapi_nvme_target *target);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __TAPI_NVME_H__ */

/**@} <!-- END tapi_nvme --> */
