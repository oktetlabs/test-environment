/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief redis-server tool TAPI
 *
 * @defgroup tapi_redis_srv redis-server tool TAPI (tapi_redis_srv)
 * @ingroup te_ts_tapi
 * @{
 *
 * TAPI to handle redis-server tool.
 *
 * Copyright (C) 2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_TAPI_REDIS_SRV_H__
#define __TE_TAPI_REDIS_SRV_H__

#include "te_errno.h"
#include "tapi_job_opt.h"
#include "tapi_job.h"
#include "tapi_rpc_stdio.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TAPI_REDIS_SRV_TIMEOUT_MS   10000

/** Redis-server tool information. */
typedef struct tapi_redis_srv_app {
    /** TAPI job handle */
    tapi_job_t         *job;
    /** output channel handles */
    tapi_job_channel_t *out_chs[2];
} tapi_redis_srv_app;

/** Representation of possible values for loglevel option. */
typedef enum tapi_redis_srv_loglevel {
    TAPI_REDIS_SRV_LOGLEVEL_DEBUG,   /**< Show a lot of information. */
    TAPI_REDIS_SRV_LOGLEVEL_VERBOSE, /**< Show many rarely useful info. */
    TAPI_REDIS_SRV_LOGLEVEL_NOTICE,  /**< Show moderately verbose info. */
    TAPI_REDIS_SRV_LOGLEVEL_WARNING  /**< Show only very important info. */
} tapi_redis_srv_loglevel;

/** Representation of possible values for repl_diskless_load option. */
typedef enum tapi_redis_srv_rdl {
    TAPI_REDIS_SRV_RDL_DISABLED,   /**< Don't use diskless load. */
    TAPI_REDIS_SRV_RDL_SWAPDB,     /**< Use RAM while parsing from socket. */
    TAPI_REDIS_SRV_RDL_ON_EMPTY_DB /**< Use diskless load if it is safe. */
} tapi_redis_srv_rdl;

/** Redis-server configuration options. */
typedef struct tapi_redis_srv_opt {
    /** IP and port of the server. */
    const struct sockaddr  *server;
    /** Layer of security protection. */
    te_bool3                protected_mode;
    /** TCP listen() backlog. */
    tapi_job_opt_uint_t     tcp_backlog;
    /** Unix socket to listen on. */
    const char             *unixsocket;
    /**
     * Close the connection after a client is idle for specified amount of
     * seconds (0 to disable).
     */
    tapi_job_opt_uint_t     timeout;
    /** Use SO_KEEPALIVE to send TCP ACKs to clients (0 to disable). */
    tapi_job_opt_uint_t     tcp_keepalive;
    /** Server verbosity level. */
    tapi_redis_srv_loglevel loglevel;
    /** Log file name. Use empty string to log on the standart output. */
    const char             *logfile;
     /** Number of databases. */
    tapi_job_opt_uint_t     databases;
    /** Compress string objects when dump .rdb databases. */
    te_bool3                rdbcompression;
    /**
     * Place checksum at the end of the file.
     *
     * @note It will lower performance (around 10%) when saving and loading RDB
     * files.
     */
    te_bool3                rdbchecksum;
    /** Replication SYNC strategy: disk, if TRUE, socket otherwise. */
    te_bool3                repl_diskless_sync;
    /**
     * Replica can load the RDB it reads from the replication link directly
     * from the socket, or store the RDB to a file and read that file after it
     * was completely received from the master.
     */
    tapi_redis_srv_rdl      repl_diskless_load;
    /** Use append only file. */
    te_bool3                appendonly;
    /** Use active rehashing. */
    te_bool3                activerehashing;
    /** Number of I/O threads to use. */
    tapi_job_opt_uint_t     io_threads;
    /** Enable threading of reads and protocol parsing. */
    te_bool3                io_threads_do_reads;
    /** Path to redis-server exec (if @c NULL then "redis-server"). */
    const char             *exec_path;
} tapi_redis_srv_opt;

/** Redis-server configuration file default options. */
extern const tapi_redis_srv_opt tapi_redis_srv_default_opt;

/**
 * Create redis-server configuration file on TA. Create redis-server app.
 *
 * @param[in]   factory job factory
 * @param[in]   opt     redis-server options
 * @param[out]  app     redis-server app handle
 *
 * @return Status code.
 */
extern te_errno tapi_redis_srv_create(tapi_job_factory_t *factory,
                                      tapi_redis_srv_opt *opt,
                                      tapi_redis_srv_app **app);

/**
 * Start redis-server.
 *
 * @param app   redis-server app handle
 *
 * @return Status code.
 */
extern te_errno tapi_redis_srv_start(const tapi_redis_srv_app *app);

/**
 * Wait for redis-server completion.
 *
 * @param app           redis-server app handle
 * @param timeout_ms    wait timeout in milliseconds
 *
 * @return Status code.
 */
extern te_errno tapi_redis_srv_wait(const tapi_redis_srv_app *app,
                                    int timeout_ms);

/**
 * Stop redis-server. It can be started over with @ref tapi_redis_srv_start.
 *
 * @param app   redis-server app handle
 *
 * @return Status code.
 */
extern te_errno tapi_redis_srv_stop(const tapi_redis_srv_app *app);

/**
 * Send a signal to redis-server.
 *
 * @param app           redis-server app handle
 * @param signum        signal to send
 *
 * @return Status code.
 */
extern te_errno tapi_redis_srv_kill(const tapi_redis_srv_app *app, int signum);

/**
 * Destroy redis-server.
 *
 * @param app   redis app handle
 *
 * @return Status code.
 */
extern te_errno tapi_redis_srv_destroy(tapi_redis_srv_app *app);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __TE_TAPI_REDIS_SRV_H__ */

/**@} <!-- END tapi_redis_srv --> */
