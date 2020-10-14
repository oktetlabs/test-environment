/** @file
 * @brief netperf tool TAPI
 *
 * @defgroup tapi_netperf tool functions TAPI
 * @ingroup te_ts_tapi
 * @{
 *
 * TAPI to handle netperf tool.
 *
 * Copyright (C) 2019 OKTET Labs. All rights reserved.
 *
 * @author Artemii Morozov <Artemii.Morozov@oktetlabs.ru>
 */

#ifndef __TE_TAPI_NETPERF_H__
#define __TE_TAPI_NETPERF_H__

#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#include "tapi_job.h"
#include "te_string.h"
#include "te_errno.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Test name.
 * It corresponds to "-t" option of 2.7.0 netperf version.
 */
typedef enum tapi_netperf_test_name {
    TAPI_NETPERF_TEST_TCP_STREAM = 0,
    TAPI_NETPERF_TEST_UDP_STREAM,
    TAPI_NETPERF_TEST_TCP_MAERTS,
    TAPI_NETPERF_TEST_TCP_RR,
    TAPI_NETPERF_TEST_UDP_RR,
} tapi_netperf_test_name;

typedef enum tapi_netperf_test_type {
    TAPI_NETPERF_TYPE_STREAM = 0,
    TAPI_NETPERF_TYPE_RR,
    TAPI_NETPERF_TYPE_UNKNOWN,
} tapi_netperf_test_type;

/**
 * Test specific command line options.
 */
typedef struct tapi_netperf_test_opt {
    /** Type of test */
    tapi_netperf_test_type type;
    union {
        struct {
            /**
             * Request size. May be @c -1 (Default request size = 1
             * will be set)
             */
            int32_t request_size;
            /**
             * Response size. May be @c -1 (Respone size
             * will be the same as request_size)
             */
            int32_t response_size;
        } rr;
        struct {
            /**
             * Size of the buffer passed-in to the "send" calls.
             * May be @c -1 (use the system's default socket
             * buffer sizes).
             */
            int32_t buffer_send;
            /**
             * Size of the buffer passed-in to the "receive" calls.
             * May be @c -1 (use the system's default socket
             * buffer sizes).
             */
            int32_t buffer_recv;
            /**
             * This option sets the local (netperf) send and receive
             * socket buffer size for the data connection to the value(s)
             * specified. May be @c -1 (use the system's default socket
             * buffer sizes)
             */
            int32_t local_sock_buf;
            /**
             * This option sets the remote (netserver) send and receive
             * socket buffer size for the data connection to the value(s)
             * specified. May be @c -1 (use the system's default socket
             * buffer sizes)
             */
            int32_t remote_sock_buf;
        } stream;
    };
} tapi_netperf_test_opt;

/**
 * Command line options.
 */
typedef struct tapi_netperf_opt {
    /** Name of the test */
    tapi_netperf_test_name test_name;
    /** Netserver host */
    const struct sockaddr *dst_host;
    /**
     * Netperf host
     * May be @c NULL (netperf source
     * address will be not set)
     */
    const struct sockaddr *src_host;
    /**
     * Port to connect.  May be @c -1 (default
     * port 12865 will be set).
     * Host-Endian byteorder.
     */
    int port;
    /** IPv4 or IPv6 */
    sa_family_t ipversion;
    /** duration in seconds of the test */
    uint32_t duration;
    /** Test specific command line options */
    tapi_netperf_test_opt test_opt;
} tapi_netperf_opt;

/**
 * Netperf report.
 */
typedef struct tapi_netperf_report {
    /** Test type */
    tapi_netperf_test_type tst_type;
    union {
        struct {
            /** Transactions per second */
            double trps;
        } rr;
        struct {
            /** Megabits per second of send */
            double mbps_send;
            /** Megabits per second of receive */
            double mbps_recv;
        } stream;
    };
} tapi_netperf_report;

/** Default options initializer */
extern const tapi_netperf_opt tapi_netperf_default_opt;

/**
 * Netserver tool context
 */
typedef struct tapi_netperf_app_server_t tapi_netperf_app_server_t;

/**
 * Netperf tool context.
 */
typedef struct tapi_netperf_app_client_t tapi_netperf_app_client_t;

/**
 * Create netperf app for netperf.
 * All needed information to run netperf
 * is in @p opt.
 *
 * @param[in]  factory           Job factory.
 * @param[in]  opt               Command line options.
 * @param[out] app               netperf app handle.
 *
 * @return Status code.
 */
extern te_errno tapi_netperf_create_client(tapi_job_factory_t *factory,
                                           const tapi_netperf_opt *opt,
                                           tapi_netperf_app_client_t **app);


/**
 * Create netserver app for netperf.
 * All needed information to run netserver
 * is in @p opt.
 *
 * @param[in]  factory           Job factory.
 * @param[in]  opt               Command line options.
 * @param[out] app               netserver app handle.
 *
 * @return Status code.
 */
extern te_errno tapi_netperf_create_server(tapi_job_factory_t *factory,
                                           const tapi_netperf_opt *opt,
                                           tapi_netperf_app_server_t **app_server);

/**
 * Create netserver and netperf app.
 *
 * @param[in]  client_factory    Client job factory.
 * @param[in]  server_factory    Server job factory.
 * @param[in]  opt               Command line options.
 * @param[out] client            netperf app handle.
 * @param[out] server            netserver app handle.
 *
 * @return Status code.
 */
extern te_errno tapi_netperf_create(tapi_job_factory_t *client_factory,
                                    tapi_job_factory_t *server_factory,
                                    const tapi_netperf_opt *opt,
                                    tapi_netperf_app_client_t **client,
                                    tapi_netperf_app_server_t **server);

/**
 * Start netperf.
 *
 * @param[in]  app          netperf app handle.
 *
 * @return Status code.
 */
extern te_errno tapi_netperf_start_client(tapi_netperf_app_client_t *app);

/**
 * Start netserver.
 *
 * @param[in]  app          netserver app handle.
 *
 * @return Status code.
 */
extern te_errno tapi_netperf_start_server(tapi_netperf_app_server_t *app);

/**
 * Start netperf and netserver.
 *
 * @param[in]  client          netperf app handle.
 * @param[in]  server          netserver app handle.
 *
 * @return Status code.
 */
extern te_errno tapi_netperf_start(tapi_netperf_app_client_t *client,
                                   tapi_netperf_app_server_t *server);

/**
 * Wait for netperf completion.
 *
 * @param[in]  app          netperf app handle.
 * @param[in]  timeout_ms   Wait timeout in milliseconds.
 *                          (negative means tapi_job_get_timeout())
 *
 * @return Status code.
 */
extern te_errno tapi_netperf_wait_client(tapi_netperf_app_client_t *app, int timeout_ms);


/**
 * Wait for netserver completion.
 *
 * @param[in]  app          netserver app handle.
 * @param[in]  timeout_ms   Wait timeout in milliseconds.
 *                          (negative means tapi_job_get_timeout())
 *
 * @return Status code.
 */
extern te_errno tapi_netperf_wait_server(tapi_netperf_app_server_t *app, int timeout_ms);

/**
 * Get netperf report.
 *
 * @param[in]  app          netperf app handle.
 * @param[out] report       netperf statistics report.
 *
 * @return Status code.
 */
extern te_errno tapi_netperf_get_report(tapi_netperf_app_client_t *app,
                                        tapi_netperf_report *report);

/**
 * Send a signal to netperf.
 *
 * @param[in]  app          netperf app handle.
 * @param[in]  signo        Signal to send to netperf.
 *
 * @return Status code.
 */
extern te_errno tapi_netperf_kill_client(tapi_netperf_app_client_t *app, int signo);

/**
 * Send a signal to netserver.
 *
 * @param[in]  app          netserver app handle.
 * @param[in]  signo        Signal to send to netperf.
 *
 * @return Status code.
 */
extern te_errno tapi_netperf_kill_server(tapi_netperf_app_server_t *app, int signo);


/**
 * Send a signal to netperf and netserver.
 *
 * @param[in]  clinet       netperf app hande.
 * @param[in]  server       netserver app handle.
 * @param[in]  signo        Signal to send to netperf.
 *
 * @return Status code.
 */
extern te_errno tapi_netperf_kill(tapi_netperf_app_client_t *client,
                                  tapi_netperf_app_server_t *server,
                                  int signo);

/**
 * Destroy netperf app.
 *
 * @param[in]  app          netperf app handle.
 *
 * @return Status code.
 */
extern te_errno tapi_netperf_destroy_client(tapi_netperf_app_client_t *app);

/**
 * Destroy netserver app.
 *
 * @param[in]  app          netserver app handle.
 *
 * @return Status code.
 */
extern te_errno tapi_netperf_destroy_server(tapi_netperf_app_server_t *app);

/**
 * Output netperf report via MI logger.
 *
 * @param[in]  report       netperf report.
 *
 * @return Status code.
 */
extern te_errno tapi_netperf_mi_report(const tapi_netperf_report *report);

/**
 * Add a wrapper tool/script to netperf
 *
 * @param[in]  app      netperf app handle.
 * @param[in]  tool     Path to the wrapper tool.
 * @param[in]  argv     Wrapper arguments (last item should be @c NULL)
 * @param[in]  priority Wrapper priority.
 * @param[out] wrap     Wrapper instance handle
 *
 * @return Status code.
 */
extern te_errno tapi_netperf_client_wrapper_add(tapi_netperf_app_client_t *app,
                                                const char *tool,
                                                const char **argv,
                                                tapi_job_wrapper_priority_t priority,
                                                tapi_job_wrapper_t **wrap);

/**
 * Add a scheduling parameters to netperf
 *
 * @param app          netperf app handle.
 * @param sched_param  Array of scheduling parameters. The last element must
 *                     have the type @c TAPI_JOB_SCHED_END and data pointer to
 *                     @c NULL.
 *
 * @return Status code
 */
extern te_errno tapi_netperf_client_add_sched_param(
                                 tapi_netperf_app_client_t *app,
                                 tapi_job_sched_param *sched_param);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __TE_TAPI_NETPERF_H__ */

/**@} <!-- END tapi_netperf --> */
