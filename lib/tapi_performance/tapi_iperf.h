/** @file
 * @brief Performance Test API to iperf3 tool routines
 *
 * Test API to control 'iperf3' tool.
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

#ifndef __TAPI_IPERF_H__
#define __TAPI_IPERF_H__

#include "te_errno.h"
#include "rcf_rpc.h"
#include "te_string.h"


#ifdef __cplusplus
extern "C" {
#endif

/** Default port number (5201, see iperf3 manual) */
#define TAPI_IPERF_PORT_DEFAULT             (-1)
/** Default number of bytes to transmit */
#define TAPI_IPERF_OPT_BYTES_DEFAULT        (0)
/** Default time in seconds to transmit for (10 secs, see iperf3 manual)*/
#define TAPI_IPERF_OPT_TIME_DEFAULT         (0)
/** Default bandwidth (1 Mbit/sec for UDP, unlimited for TCP, see iperf3 manual) */
#define TAPI_IPERF_OPT_BANDWIDTH_DEFAULT    (0)
/** Default number of parallel client streams to run */
#define TAPI_IPERF_OPT_STREAMS_DEFAULT      (0)
/** PID value of not started (stopped) server/client */
#define TAPI_IPERF_PID_INVALID              (-1)


/** Format to report: Kbits, Mbits, KBytes, MBytes */
typedef enum tapi_iperf_format {
    TAPI_IPERF_FORMAT_DEFAULT,
    TAPI_IPERF_FORMAT_KBITS,
    TAPI_IPERF_FORMAT_MBITS,
    TAPI_IPERF_FORMAT_KBYTES,
    TAPI_IPERF_FORMAT_MBYTES
} tapi_iperf_format;

/** Internet protocol version */
typedef enum tapi_iperf_ipversion {
    TAPI_IPERF_IPVDEFAULT,
    TAPI_IPERF_IPV4,
    TAPI_IPERF_IPV6
} tapi_iperf_ipversion;

/** Options for iperf tool */
typedef struct tapi_iperf_options {
    tapi_iperf_format format;       /**< Format to report */
    int port;                       /**< Port to listen on/connect to,
                                         set to @ref TAPI_IPERF_PORT_DEFAULT
                                         to use default one */
    union {
        struct {
            /* Specific server options, is not used now. Reserved for future use. */
        } server;                   /**< Server specific options */
        struct {
            const char *host;       /**< Destination host (server) */
            tapi_iperf_ipversion ipversion; /**< IP version */
            rpc_socket_proto protocol;      /**< Transport protocol */
            uint64_t bandwidth;     /**< target bandwidth (bits/sec)
                                 set to @ref TAPI_IPERF_OPT_BANDWIDTH_DEFAULT
                                 to use default one */
            uint64_t bytes; /**< Number of bytes to transmit (instead of time),
                                 set to @ref TAPI_IPERF_OPT_BYTES_DEFAULT
                                 to use time instead of */
            uint32_t time;  /**< Time in seconds to transmit for,
                                 set to @ref TAPI_IPERF_OPT_TIME_DEFAULT
                                 to use default one */
            uint16_t streams;       /**< Number of parallel client streams
                                 to run; to use default one set it to
                                 @ref TAPI_IPERF_OPT_STREAMS_DEFAULT */
        } client;           /**< Client specific options */
    };
} tapi_iperf_options;

/**
 * On-stack iperf server options initializer.
 */
#define TAPI_IPERF_SERVER_OPT_INIT {        \
    .format = TAPI_IPERF_FORMAT_DEFAULT,    \
    .port = TAPI_IPERF_PORT_DEFAULT         \
}

/**
 * On-stack iperf client options initializer.
 */
#define TAPI_IPERF_CLIENT_OPT_INIT {                    \
    .format = TAPI_IPERF_FORMAT_DEFAULT,                \
    .port = TAPI_IPERF_PORT_DEFAULT,                    \
    .client = {                                         \
        .host = NULL,                                   \
        .ipversion = TAPI_IPERF_IPVDEFAULT,             \
        .protocol = RPC_PROTO_DEF,                      \
        .bandwidth = TAPI_IPERF_OPT_BANDWIDTH_DEFAULT,  \
        .bytes = TAPI_IPERF_OPT_BYTES_DEFAULT,          \
        .time = TAPI_IPERF_OPT_TIME_DEFAULT,            \
        .streams = TAPI_IPERF_OPT_STREAMS_DEFAULT       \
    }                                                   \
}

/** iperf application context (common for both server and client) */
typedef struct tapi_iperf_app {
    rcf_rpc_server *rpcs;   /**< RPC server handle */
    tarpc_pid_t pid;        /**< PID */
    int stdout;             /**< File descriptor to read from stdout stream */
    char *cmd;              /**< Command line string to run the application */
} tapi_iperf_app;

/** iperf server context */
typedef struct tapi_iperf_server {
    tapi_iperf_app app;     /**< Application context */
} tapi_iperf_server;

/** iperf client context */
typedef struct tapi_iperf_client {
    tapi_iperf_app app;     /**< Application context */
    te_string report;       /**< Buffer to save a raw report */
    te_string err;          /**< Error message */
} tapi_iperf_client;

/**
 * On-stack iperf application context initializer.
 */
#define TAPI_IPERF_APP_INIT {           \
    .rpcs = NULL,                       \
    .pid = TAPI_IPERF_PID_INVALID,      \
    .stdout = -1,                       \
    .cmd = NULL                         \
}

/**
 * On-stack iperf server context initializer.
 */
#define TAPI_IPERF_SERVER_INIT {        \
    .app = TAPI_IPERF_APP_INIT          \
}

/**
 * On-stack iperf client context initializer.
 */
#define TAPI_IPERF_CLIENT_INIT {        \
    .app = TAPI_IPERF_APP_INIT,         \
    .report = TE_STRING_INIT,           \
    .err = TE_STRING_INIT               \
}

/** iperf report */
typedef struct tapi_iperf_report {
    uint64_t bytes;         /**< Number of bytes was transmitted */
    double seconds;         /**< Number of seconds was expired during test */
    double bits_per_second; /**< Throughput */
} tapi_iperf_report;


/**
 * Start iperf server. Start auxiliary RPC server and initialize server context.
 * Note, @b tapi_iperf_server_stop should be called to stop the server, to
 * release its resources @b tapi_iperf_server_release should be called.
 *
 * @param[in]  rpcs         RPC server handle.
 * @param[in]  options      iperf tool options.
 * @param[out] server       Server context.
 *
 * @return Status code.
 *
 * @sa tapi_iperf_server_stop, tapi_iperf_server_release
 */
extern te_errno tapi_iperf_server_start(rcf_rpc_server *rpcs,
                                        const tapi_iperf_options *options,
                                        tapi_iperf_server *server);

/**
 * Stop iperf server.
 *
 * @param[inout] server     Server context.
 *
 * @return Status code.
 *
 * @sa tapi_iperf_server_start
 */
extern te_errno tapi_iperf_server_stop(tapi_iperf_server *server);

/**
 * Stop iperf server, and release its context.
 *
 * @param[inout] server     Server context.
 *
 * @return Status code.
 *
 * @sa tapi_iperf_server_start, tapi_iperf_server_stop
 */
extern te_errno tapi_iperf_server_release(tapi_iperf_server *server);


/**
 * Start iperf client. Start auxiliary RPC server and initialize client context.
 * Note, @b tapi_iperf_client_stop should be called to stop the client, to
 * release its resources @b tapi_iperf_client_release should be called.
 *
 * @param[in]  rpcs         RPC server handle.
 * @param[in]  options      iperf tool options.
 * @param[out] client       Client context.
 *
 * @return Status code.
 *
 * @sa tapi_iperf_client_stop, tapi_iperf_client_release
 */
extern te_errno tapi_iperf_client_start(rcf_rpc_server *rpcs,
                                        const tapi_iperf_options *options,
                                        tapi_iperf_client *client);

/**
 * Stop iperf client.
 *
 * @param[inout] client     Client context.
 *
 * @return Status code.
 *
 * @sa tapi_iperf_client_start
 */
extern te_errno tapi_iperf_client_stop(tapi_iperf_client *client);

/**
 * Stop iperf client, and release its context.
 *
 * @param[inout] client     Client context.
 *
 * @return Status code.
 *
 * @sa tapi_iperf_client_start, tapi_iperf_client_stop
 */
extern te_errno tapi_iperf_client_release(tapi_iperf_client *client);

/**
 * Wait for client results. Note, function jumps to cleanup if timeout is
 * expired, or if it failed to read client's stdout.
 *
 * @param client        Client context.
 * @param timeout       Time to wait for client results (seconds). It MUST be
 *                      big enough to finish iperf client normally (it depends
 *                      on both '--time' and '--bytes' iperf options),
 *                      otherwise the function will be failed.
 *
 * @return Status code.
 */
extern te_errno tapi_iperf_client_wait(tapi_iperf_client *client,
                                       uint16_t timeout);

/**
 * Get client results.
 *
 * @param[in]  client       Client context.
 * @param[out] report       Report with results.
 *
 * @return Status code.
 */
extern te_errno tapi_iperf_client_get_report(tapi_iperf_client *client,
                                             tapi_iperf_report *report);

/**
 * Print report info. Note, it calls @b RING as print function.
 *
 * @param report        Report with results.
 */
extern void tapi_iperf_client_print_report(const tapi_iperf_report *report);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TAPI_IPERF_H__ */
