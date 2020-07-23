/** @file
 * @brief sfnt-pingpong tool TAPI
 *
 * @defgroup tapi_sfnt_pingpong tool functions TAPI
 * @ingroup te_ts_tapi
 * @{
 *
 * TAPI to handle sfnt-pingpong tool.
 *
 * Copyright (C) 2020 OKTET Labs. All rights reserved.
 *
 * @author Artemii Morozov <Artemii.Morozov@oktetlabs.ru>
 */
#ifndef __TE_TAPI_SFNT_PINGPONG_H__
#define __TE_TAPI_SFNT_PINGPONG_H__

#include "tapi_job.h"
#include "te_vector.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Type of iomux call. */
typedef enum {
    TAPI_SFNT_PP_MUXER_NONE = 0,
    TAPI_SFNT_PP_MUXER_POLL,
    TAPI_SFNT_PP_MUXER_SELECT,
    TAPI_SFNT_PP_MUXER_EPOLL
} tapi_sfnt_pp_muxer;

/**
 * The list of values allowed for parameter of
 * type 'tapi_sfnt_pp_muxer'
 */
#define TAPI_SFNT_PP_MUXER_MAP_LIST \
    {"none", TAPI_SFNT_PP_MUXER_NONE},     \
    {"poll", TAPI_SFNT_PP_MUXER_POLL},     \
    {"select", TAPI_SFNT_PP_MUXER_SELECT}, \
    {"epoll", TAPI_SFNT_PP_MUXER_EPOLL}

/**
 * Get the value of parameter of type 'tapi_sfnt_pp_muxer'
 *
 * @param var_name_  Name of the variable used to get the value of
 *                   "var_name_" parameter of type 'tapi_sfnt_pp_muxer'
 */
#define TEST_GET_SFNT_PP_MUXER(var_name_) \
    TEST_GET_ENUM_PARAM(var_name_, TAPI_SFNT_PP_MUXER_MAP_LIST)

/** sfnt-pingping tool specific command line options  */
typedef struct tapi_sfnt_pp_opt {
    /** Server host */
    const struct sockaddr *server;
    /** Prefix before sfnt-pingpong client side*/
    const char *prefix_client;
    /** Prefix before sfnt-pingpong server side*/
    const char *prefix_server;
    /** Transport protocol. IPPROTO_TCP or IPPROTO_UDP only. */
    uint8_t proto;
    /** IPv4 or IPv6 */
    sa_family_t ipversion;
    /**
     * Minimuim message size
     * May be @c -1 (default
     * sfnt-pingpong value will be set).
     */
    int min_msg;
    /**
     * Maximum message size
     * May be @c -1 (default
     * sfnt-pingpong value will be set).
     */
    int max_msg;
    /**
     * Minimum time per message size (ms)
     * May be @c -1 (default value is @c 1000).
     */
    int min_ms;
    /**
     * Maximum time per message size (ms)
     * May be @c -1 (default value is @c 3000).
     */
    int max_ms;
    /**
     * Minimum iterations for result.
     * May be @c -1 (default value is @c 1000).
     */
    int min_iter;
    /**
     * Maximum iterations for result.
     * May be @c -1 (default value is @c 1000000).
     */
    int max_iter;
    /**
     * Making non-blocking calls.
     * @p spin is @c TRUE means
     * @p timeout equal to zero.
     */
    te_bool spin;
    /**
     * Type of iomux call. poll, epoll, select, none.
     */
    tapi_sfnt_pp_muxer muxer;
    /**
     * Socket SEND/RECV timeout (ms)
     * This value pass to iomux functions as
     * a timeout param.
     * May be @c -1 (default value is @c -1).
     */
    int timeout_ms;
    /**
     * Message sizes vector.
     * Message sizes for which performance is measured.
     */
    te_vec *sizes;
} tapi_sfnt_pp_opt;

/** Default options initializer */
extern const tapi_sfnt_pp_opt tapi_sfnt_pp_opt_default_opt;

/**
 * Struct for storage report for one size value.
 *
 * The output of sfnt-pingpong in the form of a table:
 *
 *  size mean min median max %ile stddev iter
 *  0    0    0   0      0    0    0     0
 *  1    1    1   1      1    1    1     1
 *
 * So this structure stores the values of a single row.
 */
typedef struct tapi_sfnt_pp_report {
    /** payload size */
    unsigned int size;
    /** mean latency (nanoseconds) */
    unsigned int mean;
    /** minimum latency (nanoseconds) */
    unsigned int min;
    /** maximum latency (nanoseconds) */
    unsigned int max;
    /** median latency (nanoseconds) */
    unsigned int median;
    /** 99% percentile */
    unsigned int percentile;
    /** standard deviation */
    unsigned int stddev;
} tapi_sfnt_pp_report;

/** sfnt-pingpong server context */
typedef struct tapi_sfnt_pp_app_server_t tapi_sfnt_pp_app_server_t;

/** sfnt-pingpong clinet context */
typedef struct tapi_sfnt_pp_app_client_t tapi_sfnt_pp_app_client_t;

/**
 * Create client app.
 * All needed information to run
 * is in @p opt.
 *
 * @param[in]  factory           Job factory.
 * @param[in]  opt               Command line options.
 * @param[in]  path_to_sfnt_pp   Path to sfnt-pingpong tool.
 * @param[out] app               Clinet app handle.
 *
 * @return Status code.
 */
extern te_errno tapi_sfnt_pp_create_client(tapi_job_factory_t *factory,
                                           const tapi_sfnt_pp_opt *opt,
                                           tapi_sfnt_pp_app_client_t **app);

/**
 * Create server app.
 * All needed information to run
 * is in @p opt.
 *
 * @param[in]  factory           Job factory.
 * @param[in]  opt               Command line options.
 * @param[in]  path_to_sfnt_pp   Path to sfnt-pingpong tool.
 * @param[out] app               Clinet app handle.
 *
 * @return Status code.
 */
extern te_errno tapi_sfnt_pp_create_server(tapi_job_factory_t *factory,
                                           const tapi_sfnt_pp_opt *opt,
                                           tapi_sfnt_pp_app_server_t **app);

/**
 * Create client and server app.
 *
 * @param[in]  client_factory    Client job factory.
 * @param[in]  server_factory    Server job factory.
 * @param[in]  opt               Command line options.
 * @param[in]  path_to_sfnt_pp   Path to sfnt-pingpong tool.
 * @param[out] client            Client app handle.
 * @param[out] server            Server app handle.
 *
 * @return Status code.
 */
extern te_errno tapi_sfnt_pp_create(tapi_job_factory_t *client_factory,
                                    tapi_job_factory_t *server_factory,
                                    const tapi_sfnt_pp_opt *opt,
                                    tapi_sfnt_pp_app_client_t **client,
                                    tapi_sfnt_pp_app_server_t **server);

/**
 * Start client.
 *
 * @param[in]  app          Client app handle.
 *
 * @return Status code.
 */
extern te_errno tapi_sfnt_pp_start_clinet(tapi_sfnt_pp_app_client_t *app);

/**
 * Start server.
 *
 * @param[in]  app          Server app handle.
 *
 * @return Status code.
 */
extern te_errno tapi_sfnt_pp_start_server(tapi_sfnt_pp_app_server_t *app);

/**
 * Start clinet and server.
 *
 * @param[in]  client          Client app handle.
 * @param[in]  server          Server app handle.
 *
 * @return Status code.
 */
extern te_errno tapi_sfnt_pp_start(tapi_sfnt_pp_app_client_t *client,
                                   tapi_sfnt_pp_app_server_t *server);

/**
 * Wait for client completion.
 *
 * @param[in]  app          Client app handle.
 * @param[in]  timeout_ms   Wait timeout in milliseconds.
 *                          (negative means tapi_job_get_timeout())
 *
 * @return Status code.
 */
extern te_errno tapi_sfnt_pp_wait_client(tapi_sfnt_pp_app_client_t *app,
                                         int timeout_ms);

/**
 * Wait for server completion.
 *
 * @param[in]  app          Server app handle.
 * @param[in]  timeout_ms   Wait timeout in milliseconds.
 *                          (negative means tapi_job_get_timeout())
 *
 * @return Status code.
 */
extern te_errno tapi_sfnt_pp_wait_server(tapi_sfnt_pp_app_server_t *app,
                                         int timeout_ms);

/**
 * Get sfnt-pingpong report.
 *
 * @note 'report' should be freed by user.
 *
 * @param[in]  app          Clinet app handle.
 * @param[out] report       sfnt-pingpong statistics report.
 *
 * @return Status code.
 */
extern te_errno tapi_sfnt_pp_get_report(tapi_sfnt_pp_app_client_t *app,
                                        tapi_sfnt_pp_report **report);

/**
 * Send a signal to clinet.
 *
 * @param[in]  app          Client app handle.
 * @param[in]  signo        Signal to send to client.
 *
 * @return Status code.
 */
extern te_errno tapi_sfnt_pp_kill_client(tapi_sfnt_pp_app_client_t *app,
                                         int signo);

/**
 * Send a signal to server.
 *
 * @param[in]  app          Server app handle.
 * @param[in]  signo        Signal to send to server.
 *
 * @return Status code.
 */
extern te_errno tapi_sfnt_pp_kill_server(tapi_sfnt_pp_app_server_t *app,
                                         int signo);

/**
 * Destroy client app.
 *
 * @param[in]  app          Client app handle.
 *
 * @return Status code.
 */
extern te_errno tapi_sfnt_pp_destroy_client(tapi_sfnt_pp_app_client_t *app);

/**
 * Destroy server app.
 *
 * @param[in]  app          Server app handle.
 *
 * @return Status code.
 */
extern te_errno tapi_sfnt_pp_destroy_server(tapi_sfnt_pp_app_server_t *app);

/**
 * Output sfnt-pingpong report via MI logger.
 *
 * @param[in] report sfnt-pingpong report
 *
 * @return Status code
 */
extern te_errno tapi_sfnt_pp_mi_report(const tapi_sfnt_pp_report *report);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __TE_TAPI_SFNT_PINGPONG_H__ */

/**@} <!-- END tapi_sfnt_pingpong --> */
