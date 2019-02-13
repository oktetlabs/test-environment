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

/* Additional opts for nvme connect */
typedef struct tapi_nvme_connect_opts {
    te_bool hdr_digest;         /* Enable transport protocol header */
    te_bool data_digest;        /* Enable transport protocol data */
} tapi_nvme_connect_opts;

/**
 * Connect initiator host to target host with additional options
 *
 * @param host_ctrl     handle of host_ctrl
 * @param target        handle of target
 * @param opts          additional opts for nvme connect
 *
 * @return TE error code
 */
extern te_errno tapi_nvme_initiator_connect_opts(
    tapi_nvme_host_ctrl *host_ctrl,
    const tapi_nvme_target *target,
    const tapi_nvme_connect_opts *opts);

/**
 * Disconnect host_ctrl form connected target.
 *
 * @param host_ctrl     handle of host_ctrl
 *
 * @return TE error code
 */
extern te_errno tapi_nvme_initiator_disconnect(tapi_nvme_host_ctrl *host_ctrl);

/**
 * Call 'nvme list' on the initiator side.
 *
 * @param host_ctrl     handle of host_ctrl
 *
 * @return TE error code
 */
extern te_errno tapi_nvme_initiator_list(tapi_nvme_host_ctrl *host_ctrl);

/**
 * Send NVMe Identify Controller
 *
 * nvme id-ctrl /dev/nvme0n1
 *
 * @param host_ctrl     handle of host_ctrl
 *
 * @return TE error code
 */
extern te_errno tapi_nvme_initiator_id_ctrl(tapi_nvme_host_ctrl *host_ctrl);

/**
 * Send NVMe Identify Namespace
 *
 * nvme id-ns /dev/nvme0n1
 *
 * @param host_ctrl     handle of host_ctrl
 *
 * @return TE error code
 */
extern te_errno tapi_nvme_initiator_id_ns(tapi_nvme_host_ctrl *host_ctrl);

/**
 * Retrieve the namespace ID of opened block device
 *
 * nvme get-ns-id /dev/nvme0n1
 *
 * @param host_ctrl     handle of host ctrl on initiator side
 *
 * @return TE error code
 */
extern te_errno tapi_nvme_initiator_get_id_ns(tapi_nvme_host_ctrl *host_ctrl);

/**
 * Reads and shows the defined NVMe controller registers
 *
 * nvme show-regs /dev/nvme0n1
 *
 * @param host_ctrl     handle of host ctrl on initiator side
 *
 * @return TE error code
 */
extern te_errno tapi_nvme_initiator_show_regs(tapi_nvme_host_ctrl *host_ctrl);

/**
 * Retrieve FW Log, show it
 *
 * nvme fw-log /dev/nvme0n1
 *
 * @param host_ctrl     handle of host ctrl on initiator side
 *
 * @return TE error code
 */
extern te_errno tapi_nvme_initiator_fw_log(tapi_nvme_host_ctrl *host_ctrl);

/**
 * Retrieve SMART Log, show it
 *
 * nvme smart-log /dev/nvme0n1
 *
 * @param host_ctrl     handle of host ctrl on initiator side
 *
 * @return TE error code
 */
extern te_errno tapi_nvme_initiator_smart_log(tapi_nvme_host_ctrl *host_ctrl);

/**
 * Retrieve Error Log, show it
 *
 * nvme error-log /dev/nvme0n1
 *
 * @param host_ctrl     handle of host ctrl on initiator side
 *
 * @return TE error code
 */
extern te_errno tapi_nvme_initiator_error_log(tapi_nvme_host_ctrl *host_ctrl);

/** Supported nvme feature */
typedef enum tapi_nvme_feature {
    TAPI_NVME_FEAT_ARBITRATION      = 0x01,
    TAPI_NVME_FEAT_POWER_MGMT       = 0x02,
    TAPI_NVME_FEAT_LBA_RANGE        = 0x03,
    TAPI_NVME_FEAT_TEMP_THRESH      = 0x04,
    TAPI_NVME_FEAT_ERR_RECOVERY     = 0x05,
    TAPI_NVME_FEAT_VOLATILE_WC      = 0x06,
    TAPI_NVME_FEAT_NUM_QUEUES       = 0x07,
    TAPI_NVME_FEAT_IRQ_COALESCE     = 0x08,
    TAPI_NVME_FEAT_IRQ_CONFIG       = 0x09,
    TAPI_NVME_FEAT_WRITE_ATOMIC     = 0x0a,
    TAPI_NVME_FEAT_ASYNC_EVENT      = 0x0b,
    TAPI_NVME_FEAT_AUTO_PST         = 0x0c,
    TAPI_NVME_FEAT_HOST_MEM_BUF     = 0x0d,
    TAPI_NVME_FEAT_TIMESTAMP        = 0x0e,
    TAPI_NVME_FEAT_KATO             = 0x0f,
    TAPI_NVME_FEAT_HCTM             = 0x10,
    TAPI_NVME_FEAT_NOPSC            = 0x11,
    TAPI_NVME_FEAT_RRL              = 0x12,
    TAPI_NVME_FEAT_PLM_CONFIG       = 0x13,
    TAPI_NVME_FEAT_PLM_WINDOW       = 0x14,
    TAPI_NVME_FEAT_HOST_BEHAVIOR    = 0x16,
    TAPI_NVME_FEAT_SW_PROGRESS      = 0x80,
    TAPI_NVME_FEAT_HOST_ID          = 0x81,
    TAPI_NVME_FEAT_RESV_MASK        = 0x82,
    TAPI_NVME_FEAT_RESV_PERSIST     = 0x83,
    TAPI_NVME_FEAT_WRITE_PROTECT    = 0x84,
} tapi_nvme_feature;

/**
 * Get feature and show the resulting value
 *
 * nvme get-feature /dev/nvme0n1
 *
 * @param host_ctrl     handle of host ctrl on initiator side
 *
 * @return TE error code
 */
extern te_errno tapi_nvme_initiator_get_feature(tapi_nvme_host_ctrl *host_ctrl,
                                                int feature);

/**
 * Submit flush command
 *
 * nvme flush /dev/nvme0n1 [-n <namespace>]
 *
 * @param host_ctrl     handle of host ctrl on initiator side
 * @param namespace     NULL (all) or namespace name
 *
 * @return TE error code
 */
extern te_errno tapi_nvme_initiator_flush(tapi_nvme_host_ctrl *host_ctrl,
                                          const char *namespace);

/**
 * Submit nvme discover command
 *
 * @param host_ctrl     handle of host ctrl on initiator side
 *
 * @return TE error code
 */
extern te_errno tapi_nvme_initiator_discover_from(
    tapi_nvme_host_ctrl *host_ctrl);

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
