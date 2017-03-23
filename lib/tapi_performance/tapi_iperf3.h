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

#ifndef __TAPI_IPERF3_H__
#define __TAPI_IPERF3_H__

#include "tapi_performance.h"


#ifdef __cplusplus
extern "C" {
#endif

/** Default port number (5201, see iperf3 manual) */
#define TAPI_IPERF3_PORT_DEFAULT             (-1)
/** Default number of bytes to transmit */
#define TAPI_IPERF3_OPT_BYTES_DEFAULT        (0)
/** Default time in seconds to transmit for (10 secs, see iperf3 manual)*/
#define TAPI_IPERF3_OPT_TIME_DEFAULT         (0)
/** Default bandwidth (1 Mbit/sec for UDP, unlimited for TCP, see iperf3 manual) */
#define TAPI_IPERF3_OPT_BANDWIDTH_DEFAULT    (0)
/** Default length of buffer (128 KB for TCP, 8KB for UDP, see iperf3 manual) */
#define TAPI_IPERF3_OPT_LENGTH_DEFAULT       (0)
/** Default number of parallel client streams to run */
#define TAPI_IPERF3_OPT_STREAMS_DEFAULT      (0)


/** Internet protocol version */
typedef enum tapi_iperf3_ipversion {
    TAPI_IPERF3_IPVDEFAULT,
    TAPI_IPERF3_IPV4,
    TAPI_IPERF3_IPV6
} tapi_iperf3_ipversion;

/** Options for iperf3 tool */
typedef struct tapi_iperf3_options {
    TAPI_PERF_OPTS_COMMON;
    int port;                       /**< Port to listen on/connect to,
                                         set to @ref TAPI_IPERF3_PORT_DEFAULT
                                         to use default one */
    union {
        struct {
            /* Specific server options, is not used now.
             * Reserved for future use. */
        } server;                   /**< Server specific options */
        struct {
            const char *host;       /**< Destination host (server) */
            tapi_iperf3_ipversion ipversion;    /**< IP version */
            rpc_socket_proto protocol;          /**< Transport protocol */
            uint64_t bandwidth;     /**< target bandwidth (bits/sec)
                                 set to @ref TAPI_IPERF3_OPT_BANDWIDTH_DEFAULT
                                 to use default one */
            uint64_t bytes; /**< Number of bytes to transmit (instead of time),
                                 set to @ref TAPI_IPERF3_OPT_BYTES_DEFAULT
                                 to use time instead of */
            uint32_t time;  /**< Time in seconds to transmit for,
                                 set to @ref TAPI_IPERF3_OPT_TIME_DEFAULT
                                 to use default one */
            uint32_t length;        /**< Length of buffer to read or write;
                                 set to @ref TAPI_IPERF3_OPT_LENGTH_DEFAULT
                                 to use default one */
            uint16_t streams;       /**< Number of parallel client streams
                                 to run; to use default one set it to
                                 @ref TAPI_IPERF3_OPT_STREAMS_DEFAULT */
            te_bool reverse;        /**< Whether run in reverse mode (server
                                 sends, client receives), or not */
        } client;           /**< Client specific options */
    };
} tapi_iperf3_options;

/**
 * On-stack iperf3 server options initializer.
 */
#define TAPI_IPERF3_SERVER_OPTS_INIT {              \
    TAPI_PERF_OPTS_COMMON_INIT(TAPI_PERF_IPERF3),   \
    .port = TAPI_IPERF3_PORT_DEFAULT,               \
}

/**
 * On-stack iperf3 client options initializer.
 */
#define TAPI_IPERF3_CLIENT_OPTS_INIT {                  \
    TAPI_PERF_OPTS_COMMON_INIT(TAPI_PERF_IPERF3),       \
    .port = TAPI_IPERF3_PORT_DEFAULT,                   \
    .client = {                                         \
        .host = NULL,                                   \
        .ipversion = TAPI_IPERF3_IPVDEFAULT,            \
        .protocol = RPC_PROTO_DEF,                      \
        .bandwidth = TAPI_IPERF3_OPT_BANDWIDTH_DEFAULT, \
        .bytes = TAPI_IPERF3_OPT_BYTES_DEFAULT,         \
        .time = TAPI_IPERF3_OPT_TIME_DEFAULT,           \
        .length = TAPI_IPERF3_OPT_LENGTH_DEFAULT,       \
        .streams = TAPI_IPERF3_OPT_STREAMS_DEFAULT,     \
        .reverse = FALSE                                \
    }                                                   \
}


/**
 * Initialize iperf3 server context with the options and certain methods.
 *
 * @param server            Server tool context.
 * @param options           Server tool specific options.
 *
 * @sa tapi_iperf3_server_fini
 */
extern void tapi_iperf3_server_init(tapi_perf_server *server,
                                    const tapi_iperf3_options *options);

/**
 * Uninitialize iperf3 server context.
 *
 * @param server            Server tool context.
 *
 * @sa tapi_iperf3_server_init
 */
extern void tapi_iperf3_server_fini(tapi_perf_server *server);

/**
 * Initialize iperf3 client context with the options and certain methods.
 *
 * @param client            Client tool context.
 * @param options           Client tool specific options.
 *
 * @sa tapi_iperf3_client_fini
 */
extern void tapi_iperf3_client_init(tapi_perf_client *client,
                                    const tapi_iperf3_options *options);

/**
 * Uninitialize iperf3 client context.
 *
 * @param client            Client tool context.
 *
 * @sa tapi_iperf3_client_init
 */
extern void tapi_iperf3_client_fini(tapi_perf_client *client);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TAPI_IPERF3_H__ */
