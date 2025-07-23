/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Test API for SPDK rpc.py
 *
 * @defgroup tapi_spdk_rpc Test API for SPDK RPC tool
 * @ingroup te_ts_tapi
 * @{
 *
 * Test API to control SPDK 'rpc.py' tool
 *
 * Copyright (C) 2025 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_TAPI_SPDK_RPC_H__
#define __TE_TAPI_SPDK_RPC_H__

#include "te_defs.h"
#include "te_errno.h"
#include "te_string.h"
#include "tapi_job.h"
#include "tapi_job_opt.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Forward declaration of SPDK RPC application handle */
typedef struct tapi_spdk_rpc_app tapi_spdk_rpc_app;

/** SPDK RPC server connection options */
typedef struct tapi_spdk_rpc_server_opt {
    /** Server socket path (default: /var/tmp/spdk.sock) or IP address */
    const char *server;
    /** RPC port number (if server is IP address) */
    tapi_job_opt_uint_t port;
    /** RPC timeout in seconds */
    tapi_job_opt_uint_t timeout;
    /** Retry connecting to the RPC server N times with 0.2s interval. */
    tapi_job_opt_uint_t conn_retries;
    /** Set verbose mode to info */
    bool verbose;
} tapi_spdk_rpc_server_opt;

extern const tapi_spdk_rpc_server_opt tapi_spdk_rpc_server_default_opt;

/** Options for bdev_malloc_create command */
typedef struct tapi_spdk_rpc_bdev_malloc_create_opt {
    /** Size in MB (positional argument) */
    unsigned int size_mb;
    /** Block size in bytes (positional argument) */
    unsigned int block_size;
    /** Name of the block device (-b option) */
    const char *name;
} tapi_spdk_rpc_bdev_malloc_create_opt;

/** Options for bdev_malloc_delete command */
typedef struct tapi_spdk_rpc_bdev_malloc_delete_opt {
    /** Name of the block device */
    const char *name;
} tapi_spdk_rpc_bdev_malloc_delete_opt;

typedef enum tapi_spdk_rpc_nvmf_transport_type {
    TAPI_SPDK_RPC_NVMF_TRANSPORT_TYPE_TCP
} tapi_spdk_rpc_nvmf_transport_type;

/** Options for nvmf_create_transport command */
typedef struct tapi_spdk_rpc_nvmf_create_transport_opt {
    /** Transport type (-t option) */
    tapi_spdk_rpc_nvmf_transport_type type;
    /** Enable zero-copy receive (-z flag) */
    bool zero_copy_recv;
} tapi_spdk_rpc_nvmf_create_transport_opt;

/** Options for nvmf_create_subsystem command */
typedef struct tapi_spdk_rpc_nvmf_create_subsystem_opt {
    /** NVMe Qualified Name (positional) */
    const char *nqn;
    /** Serial number (-s option) */
    const char *serial_number;
    /** Allow any host to connect (-a flag) */
    bool allow_any_host;
    /** Enable ANA reporting (-r flag) */
    bool ana_reporting;
} tapi_spdk_rpc_nvmf_create_subsystem_opt;

/** Options for nvmf_create_subsystem command */
typedef struct tapi_spdk_rpc_nvmf_delete_subsystem_opt {
    /** NVMe Qualified Name (positional) */
    const char *nqn;
} tapi_spdk_rpc_nvmf_delete_subsystem_opt;

/**
 * Create SPDK RPC application
 *
 * @param[in]  factory     Job factory
 * @param[in]  rpc_path    Path to rpc.py script
 * @param[in]  opt         Server connection options.
 * @param[out] app         Location for the app handle
 *
 * @return Status code
 */
extern te_errno tapi_spdk_rpc_create(tapi_job_factory_t             *factory,
                                     const char                     *rpc_path,
                                     const tapi_spdk_rpc_server_opt *opt,
                                     tapi_spdk_rpc_app             **app);

/**
 * Execute an RPC command with given arguments
 *
 * @param app         SPDK RPC app handle
 * @param method      RPC method name
 * @param binds       Method-specific option bindings
 * @param opt         Method-specific options
 *
 * @return Status code
 */
extern te_errno tapi_spdk_rpc_do_command(tapi_spdk_rpc_app       *app,
                                         const char              *method,
                                         const tapi_job_opt_bind *binds,
                                         const void              *opt);

/**
 * Execute bdev_malloc_create command
 *
 * @param app     SPDK RPC app handle
 * @param opt     Command options
 *
 * @return Status code
 */
extern te_errno tapi_spdk_rpc_bdev_malloc_create(
    tapi_spdk_rpc_app *app, const tapi_spdk_rpc_bdev_malloc_create_opt *opt);

/**
 * Execute bdev_malloc_delete command
 *
 * @param app     SPDK RPC app handle
 * @param opt     Command options
 *
 * @return Status code
 */
extern te_errno tapi_spdk_rpc_bdev_malloc_delete(
    tapi_spdk_rpc_app *app, const tapi_spdk_rpc_bdev_malloc_delete_opt *opt);

/**
 * Execute nvmf_create_transport command
 *
 * @param app     SPDK RPC app handle
 * @param opt     Command options
 *
 * @return Status code
 */
extern te_errno tapi_spdk_rpc_nvmf_create_transport(
    tapi_spdk_rpc_app                             *app,
    const tapi_spdk_rpc_nvmf_create_transport_opt *opt);

/**
 * Execute nvmf_create_subsystem command
 *
 * @param app     SPDK RPC app handle
 * @param opt     Command options
 *
 * @return Status code
 */
extern te_errno tapi_spdk_rpc_nvmf_create_subsystem(
    tapi_spdk_rpc_app                             *app,
    const tapi_spdk_rpc_nvmf_create_subsystem_opt *opt);

/**
 * Execute nvmf_delete_subsystem command
 *
 * @param app     SPDK RPC app handle
 * @param opt     Command options
 *
 * @return Status code
 */
extern te_errno tapi_spdk_rpc_nvmf_delete_subsystem(
    tapi_spdk_rpc_app                             *app,
    const tapi_spdk_rpc_nvmf_delete_subsystem_opt *opt);

/**
 * Destroy SPDK RPC application
 *
 * @param app     App handle
 */
extern void tapi_spdk_rpc_destroy(tapi_spdk_rpc_app *app);


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __TE_TAPI_SPDK_RPC__H__ */

/**@} <!-- END tapi_spdk_rpc --> */
