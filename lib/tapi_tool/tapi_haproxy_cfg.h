/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2023 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief HAProxy tool config file generation TAPI
 *
 * @defgroup tapi_haproxy_cfg HAProxy tool config file generation TAPI
 * @ingroup te_ts_tapi
 * @{
 *
 * TAPI to handle HAProxy tool config file generation.
 */

#ifndef __TE_TAPI_HAPROXY_CFG_H__
#define __TE_TAPI_HAPROXY_CFG_H__

#include "tapi_job.h"
#include "tapi_job_opt.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TAPI_HAPROXY_CONF_DEFAULT_TIMEOUT_MS 30000

/** Representation of pair of address and port. */
typedef struct tapi_haproxy_cfg_address {
    /** Address, interface or host name. */
    const char *addr;
    /** Port number. */
    tapi_job_opt_uint_t port;
} tapi_haproxy_cfg_address;

/** Representation of possible frontend listener shards option value sources. */
typedef enum tapi_haproxy_cfg_shards {
    /** Set shards number manually. */
    TAPI_HAPROXY_CFG_SHARDS_NUMBER,
    /** Create as many shards as there are threads on the "bind" line. */
    TAPI_HAPROXY_CFG_SHARDS_BY_THREAD,
    /** Create one shard per thread group. */
    TAPI_HAPROXY_CFG_SHARDS_BY_GROUP,
} tapi_haproxy_cfg_shards;

/** Backend server representation for HAProxy configuration. */
typedef struct tapi_haproxy_cfg_backend {
    /** Server name. */
    char *name;
    /** Server address representation. */
    tapi_haproxy_cfg_address backend_addr;
} tapi_haproxy_cfg_backend;

/** HAProxy config file options. */
typedef struct tapi_haproxy_cfg_opt {
    /** Number of threads to run HAProxy on. */
    tapi_job_opt_uint_t nbthread;
    /**
     * If @c TRUE, HAProxy listener spreads the incoming traffic to all
     * threads a frontend "bind" line is allowed to run on instead of taking
     * them for itself.
     */
    te_bool tune_listener_multi_queue;
    /**
     * If @c TRUE, idle connection pools are shared between threads for a same
     * server.
     */
    te_bool tune_idle_pool_shared;

    /** The maximum inactivity time on the client side (in milliseconds). */
    tapi_job_opt_uint_t timeout_client_ms;
    /**
     * The maximum time for pending data staying into output buffer
     * (in milliseconds).
     */
    tapi_job_opt_uint_t timeout_server_ms;
    /**
     * The maximum time to wait for a connection attempt to a server to succeed
     * (in milliseconds).
     */
    tapi_job_opt_uint_t timeout_connect_ms;

    /** Frontend listener bind configuration. */
    struct {
        /** Frontend group name in configuration file. */
        const char *name;
        /** Listener address representation. */
        tapi_haproxy_cfg_address frontend_addr;
        /**
         * In multi-threaded mode, source of number of listeners
         * on the same address.
         * If the value is not #TAPI_HAPROXY_CFG_SHARDS_NUMBER, #shards_n field
         * should be set to #TAPI_JOB_OPT_UINT_UNDEF.
         */
        tapi_haproxy_cfg_shards shards;
        /**
         * In multi-threaded mode, number of listeners on the same address.
         * The field is ommited if it's value is #TAPI_JOB_OPT_UINT_UNDEF.
        */
        tapi_job_opt_uint_t shards_n;
    } frontend;

    /** Array of backend servers representations. */
    struct {
        /** Backend group name in configuration file. */
        const char *name;
        /** Size of backend servers representations array. */
        size_t n;
        /** Pointer to backend servers representations array. */
        tapi_haproxy_cfg_backend *backends;
    } backend;
} tapi_haproxy_cfg_opt;

/** Default options initializer. */
extern const tapi_haproxy_cfg_opt tapi_haproxy_cfg_default_opt;

/**
 * Generate config file for HAProxy app. Save it in /tmp subdir
 * of Test Agent working dir.
 *
 * @note @p result_pathname must be free'd.
 *
 * @param[in]  ta                 Test Agent name.
 * @param[in]  opt                Configs for HAProxy tool.
 * @param[out] result_pathname    Resulting path to the file (must not be @c NULL).
 *
 * @return    Status code.
 */
extern te_errno tapi_haproxy_cfg_create(const char *ta,
                                        const tapi_haproxy_cfg_opt *opt,
                                        char **result_pathname);

/**
 * Destroy generated config file for HAProxy app.
 *
 * @param ta            Test Agent name.
 * @param cfg_file      Path to generated config file.
 */
extern void tapi_haproxy_cfg_destroy(const char *ta, const char *cfg_file);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __TE_TAPI_HAPROXY_CFG_H__ */
/** @} <!-- END tapi_haproxy_cfg --> */