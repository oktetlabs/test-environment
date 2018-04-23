/** @file
 * @brief Control NVMeOF
 *
 * @defgroup tapi_nvme_nvme Control NVMeOF
 * @ingroup tapi_nvme
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

/** Target context  */
typedef struct tapi_nvme_target {
    rcf_rpc_server *rpcs;           /**< RPC server handle */
    tapi_nvme_transport transport;  /**< Transport type */
    tapi_nvme_subnqn subnqn;        /**< NVMe Qualified Name */
    unsigned int nvmet_port;        /**< NVMe target port */
    const char *device;             /**< Name of device */
    const struct sockaddr *addr;    /**< Endpoint to target */
} tapi_nvme_target;

/** Default target initialization */
#define TAPI_NVME_TARGET_DEFAULTS (struct tapi_nvme_target) { \
    .rpcs = NULL,                           \
    .transport = TAPI_NVME_TRANSPORT_TCP,   \
    .nvmet_port = 1,                        \
    .device = NULL,                         \
}

/** Initiator context */
typedef struct tapi_nvme_host_ctrl {
    rcf_rpc_server *rpcs;                      /**< RPC server handle */
    const tapi_nvme_target *connected_target;  /**< Connected target */
    char *device;                              /**< Name of device */
} tapi_nvme_host_ctrl;

/** Default host_ctrl initialization */
#define TAPI_NVME_HOST_CTRL_DEFAULTS (struct tapi_nvme_host_ctrl) { \
    .rpcs = NULL,               \
    .connected_target = NULL,   \
    .device = NULL,             \
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
 */
extern void tapi_nvme_initiator_disconnect(tapi_nvme_host_ctrl *host_ctrl);

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
