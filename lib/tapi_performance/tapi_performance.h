/** @file
 * @brief Generic Test API to network throughput test tools
 *
 * Generic high level test API to control a network throughput test tool.
 *
 *
 * Copyright (C) 2017 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 *
 * @author Ivan Melnikov <Ivan.Melnikov@oktetlabs.ru>
 */

#ifndef __TAPI_PERFORMANCE_H__
#define __TAPI_PERFORMANCE_H__

#include "te_errno.h"
#include "rcf_rpc.h"
#include "te_string.h"


#ifdef __cplusplus
extern "C" {
#endif

/**
 * Supported network throughput test tools list.
 */
typedef enum tapi_perf_type {
    TAPI_PERF_IPERF3,
} tapi_perf_type;

/** Network throughput test tool report. */
typedef struct tapi_perf_report {
    uint64_t bytes;         /**< Number of bytes was transmitted */
    double seconds;         /**< Number of seconds was expired during test */
    double bits_per_second; /**< Throughput */
} tapi_perf_report;


/* Forward declaration of network throughput test server tool */
struct tapi_perf_server;
typedef struct tapi_perf_server tapi_perf_server;

/**
 * Start perf server.
 *
 * @param server            Server context.
 * @param rpcs              RPC server handle.
 *
 * @return Status code.
 *
 * @sa tapi_perf_server_stop
 */
typedef te_errno (* tapi_perf_server_method_start)(tapi_perf_server *server,
                                                   rcf_rpc_server *rpcs);

/**
 * Stop perf server.
 *
 * @param server            Server context.
 *
 * @return Status code.
 *
 * @sa tapi_perf_server_start
 */
typedef te_errno (* tapi_perf_server_method_stop)(tapi_perf_server *server);

/**
 * Get server report. The function reads client output (stdout, stderr).
 *
 * @param[in]  server       Server context.
 * @param[out] report       Report with results.
 *
 * @return Status code.
 */
typedef te_errno (* tapi_perf_server_method_get_report)(
                                                tapi_perf_server *server,
                                                tapi_perf_report *report);

/**
 * Methods to operate the server network throughput test tool.
 */
typedef struct tapi_perf_server_methods {
    tapi_perf_server_method_start      start;
    tapi_perf_server_method_stop       stop;
    tapi_perf_server_method_get_report get_report;
} tapi_perf_server_methods;


/* Forward declaration of network throughput test client tool */
struct tapi_perf_client;
typedef struct tapi_perf_client tapi_perf_client;

/**
 * Start perf client.
 *
 * @param client            Client context.
 * @param rpcs              RPC server handle.
 *
 * @return Status code.
 *
 * @sa tapi_perf_client_stop
 */
typedef te_errno (* tapi_perf_client_method_start)(tapi_perf_client *client,
                                                   rcf_rpc_server *rpcs);

/**
 * Stop perf client.
 *
 * @param client            Client context.
 *
 * @return Status code.
 *
 * @sa tapi_perf_client_start
 */
typedef te_errno (* tapi_perf_client_method_stop)(tapi_perf_client *client);

/**
 * Wait while client finishes his work. Note, function jumps to cleanup if
 * timeout is expired.
 *
 * @param client        Client context.
 * @param timeout       Time to wait for client results (seconds). It MUST be
 *                      big enough to finish client normally (it depends
 *                      on client's options), otherwise the function will be
 *                      failed.
 *
 * @return Status code.
 */
typedef te_errno (* tapi_perf_client_method_wait)(tapi_perf_client *client,
                                                  uint16_t timeout);

/**
 * Get client report. The function reads client output (stdout, stderr).
 *
 * @param[in]  client       Client context.
 * @param[out] report       Report with results.
 *
 * @return Status code.
 */
typedef te_errno (* tapi_perf_client_method_get_report)(
                                                tapi_perf_client *client,
                                                tapi_perf_report *report);

/**
 * Methods to operate the client network throughput test tool.
 */
typedef struct tapi_perf_client_methods {
    tapi_perf_client_method_start      start;
    tapi_perf_client_method_stop       stop;
    tapi_perf_client_method_wait       wait;
    tapi_perf_client_method_get_report get_report;
} tapi_perf_client_methods;


/* Common part of options of any perf tool */
#define TAPI_PERF_OPTS_COMMON \
    tapi_perf_type type

/** On-stack initializer of common part of perf tool options. */
#define TAPI_PERF_OPTS_COMMON_INIT(_type)    \
    .type = _type

/** Network throughput test tool options */
typedef struct tapi_perf_opts {
    TAPI_PERF_OPTS_COMMON;
} tapi_perf_opts;

/** Network throughput test tool context (common for both server and client) */
typedef struct tapi_perf_app {
    tapi_perf_opts *opts;   /**< Tool options */
    rcf_rpc_server *rpcs;   /**< RPC server handle */
    tarpc_pid_t pid;        /**< PID */
    int stdout;             /**< File descriptor to read from stdout stream */
    int stderr;             /**< File descriptor to read from stderr stream */
    char *cmd;              /**< Command line string to run the application */
    te_string report;       /**< Buffer to save a raw report */
    te_string err;          /**< Error message */
} tapi_perf_app;

/** Network throughput test server tool context */
typedef struct tapi_perf_server {
    tapi_perf_app app;      /**< Tool context */
    const tapi_perf_server_methods *methods; /**< Methods to operate the tool */
} tapi_perf_server;

/** Network throughput test client tool context */
typedef struct tapi_perf_client {
    tapi_perf_app app;      /**< Tool context */
    const tapi_perf_client_methods *methods; /**< Methods to operate the tool */
} tapi_perf_client;


/**
 * Create server network throughput test tool proxy.
 *
 * @param type              Sort of tool, see @ref tapi_perf_type to get a list
 *                          of supported tools.
 * @param options           Server tool specific options.
 *
 * @return Status code.
 *
 * @sa tapi_perf_server_destroy
 */
extern tapi_perf_server *tapi_perf_server_create(tapi_perf_type type,
                                                 const void *options);

/**
 * Destroy server network throughput test tool proxy.
 *
 * @param server            Server context.
 *
 * @sa tapi_perf_server_create
 */
extern void tapi_perf_server_destroy(tapi_perf_server *server);

/**
 * Start perf server.
 *
 * @param server            Server context.
 * @param rpcs              RPC server handle.
 *
 * @return Status code.
 *
 * @sa tapi_perf_server_stop
 */
extern te_errno tapi_perf_server_start(tapi_perf_server *server,
                                       rcf_rpc_server *rpcs);

/**
 * Stop perf server.
 *
 * @param server            Server context.
 *
 * @return Status code.
 *
 * @sa tapi_perf_server_start
 */
extern te_errno tapi_perf_server_stop(tapi_perf_server *server);

/**
 * Get server report. The function reads client output (stdout, stderr).
 *
 * @param[in]  server       Server context.
 * @param[out] report       Report with results.
 *
 * @return Status code.
 */
extern te_errno tapi_perf_server_get_report(tapi_perf_server *server,
                                            tapi_perf_report *report);


/**
 * Create client network throughput test tool proxy.
 *
 * @param type              Sort of tool, see @ref tapi_perf_type to get a list
 *                          of supported tools.
 * @param options           Client tool specific options.
 *
 * @return Status code.
 *
 * @sa tapi_perf_client_destroy
 */
extern tapi_perf_client *tapi_perf_client_create(tapi_perf_type type,
                                                 const void *options);

/**
 * Destroy client network throughput test tool proxy.
 *
 * @param client            Client context.
 *
 * @sa tapi_perf_client_create
 */
extern void tapi_perf_client_destroy(tapi_perf_client *client);

/**
 * Start perf client.
 *
 * @param client            Client context.
 * @param rpcs              RPC server handle.
 *
 * @return Status code.
 *
 * @sa tapi_perf_client_stop
 */
extern te_errno tapi_perf_client_start(tapi_perf_client *client,
                                       rcf_rpc_server *rpcs);

/**
 * Stop perf client.
 *
 * @param client            Client context.
 *
 * @return Status code.
 *
 * @sa tapi_perf_client_start
 */
extern te_errno tapi_perf_client_stop(tapi_perf_client *client);

/**
 * Wait while client finishes his work. Note, function jumps to cleanup if
 * timeout is expired.
 *
 * @param client        Client context.
 * @param timeout       Time to wait for client results (seconds). It MUST be
 *                      big enough to finish client normally (it depends
 *                      on client's options), otherwise the function will be
 *                      failed.
 *
 * @return Status code.
 */
extern te_errno tapi_perf_client_wait(tapi_perf_client *client,
                                      uint16_t timeout);

/**
 * Get client report. The function reads client output (stdout, stderr).
 *
 * @param[in]  client       Client context.
 * @param[out] report       Report with results.
 *
 * @return Status code.
 */
extern te_errno tapi_perf_client_get_report(tapi_perf_client *client,
                                            tapi_perf_report *report);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TAPI_PERFORMANCE_H__ */
