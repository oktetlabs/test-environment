/** @file
 * @brief Generic Test API for packetdrill test tool
 *
 * @defgroup tapi_packetdrill Test API to use packetdrill test tool
 * @ingroup te_ts_tapi
 * @{
 *
 * Generic API to use packetdrill test tool.
 *
 * Copyright (C) 2018-2022 OKTET Labs. All rights reserved.
 */

#ifndef __TAPI_PACKETDRILL_H__
#define __TAPI_PACKETDRILL_H__

#include "te_errno.h"
#include "rcf_rpc.h"
#include "te_string.h"
#include "tapi_job.h"

#if HAVE_NET_IF_H
#include <net/if.h>
#else
#define IF_NAMESIZE 16
#endif

#ifdef __cplusplus
extern "C" {
#endif

/** @page lib-tapi_packetdrill-instruction Instruction to work with packetdrill API
 *
 * \code{.sh}
 * git clone --single-branch --branch te https://git.oktetlabs.ru/git/oktetlabs/packetdrill.git
 * cd packetdrill/gtests/net/packetdrill
 * ./configure
 * make
 * export TE_PACKETDRILL_DIR=$PWD
 * \endcode
 */

/** Flavors of IP versions we support. */
typedef enum {
    TAPI_PACKETDRILL_IP_UNKNOWN = -1,
    TAPI_PACKETDRILL_IP_VERSION_4,          /**< Native IPv4, with AF_INET
                                                 sockets and IPv4 addresses. */
    TAPI_PACKETDRILL_IP_VERSION_4_MAPPED_6, /**< IPv4-Mapped IPv6 addresses:
                                                 (see RFC 4291 sec. 2.5.5.2)
                                                 we use AF_INET6 sockets but
                                                 all connect(), bind(), and
                                                 accept() calls are for IPv4
                                                 addresses mapped into IPv6
                                                 address space.
                                                 So all interface addresses
                                                 and packets on the wire
                                                 are IPv4. */
    TAPI_PACKETDRILL_IP_VERSION_6,          /**< Native IPv6, with AF_INET6
                                                 sockets and IPv6 addresses. */
} tapi_packetdrill_ip_version_t;

/** Packetdrill test tool options */
/*
 * Missing parameters are represented as negative values for `int` parameters
 * and as `NULL` for address parameters.
 */
typedef struct tapi_packetdrill_opts {
    /* General packetdrill options */
    tapi_packetdrill_ip_version_t ip_version; /**< v4, v4-mapped-v6, v6 */
    const char *ip_version_str;               /**< String representation of
                                                   IP version, is set in
                                                   @b build_argv() function */
    int                     bind_port;         /**< local port for bind() */
    int                     connect_port;      /**< remote port for connect() */
    const struct sockaddr  *local_ip;          /**< local interface IP */
    const struct sockaddr  *remote_ip;         /**< remote interface IP */
    const struct sockaddr  *gateway_ip;        /**< gateway interface IP */
    const struct sockaddr  *netmask_ip;        /**< network mask */
    char                   *non_fatal;         /**< treat asserts as non-fatal:
                                                    packet,syscall */
    /* For remote on-the-wire testing using a real NIC. */
    te_bool                is_client;                  /**< be client or server */
    const char            *wire_device;                /**< iface name */
    const struct sockaddr *wire_server_ip;             /**< IP of on-the-wire
                                                            server */
    int                    wire_server_port;           /**< server server
                                                            listens on */

    /* TE test specific options */
    char     src_test_dir[PATH_MAX];    /**< Path to packetdrill script */
    char     short_test_name[PATH_MAX]; /**< Short packetdrill script name */

    const char     *prefix;     /**< String to pass as a prefix
                                     before 'packetdrill' */
} tapi_packetdrill_opts;

/** Packetdrill test tool context */
typedef struct tapi_packetdrill_app tapi_packetdrill_app;

/**
 * Initiate packetdrill app.
 *
 * @param[in]  factory  Job factory.
 * @param[in]  opts     Packetdrill options.
 * @param[out] app      The application handle.
 *
 * @return Status code.
 *
 * @sa tapi_packetdrill_app_destroy
 */
extern te_errno tapi_packetdrill_app_init(tapi_job_factory_t *factory,
                                          tapi_packetdrill_opts *opts,
                                          tapi_packetdrill_app **app);

/**
 * Destroy packetdrill app.
 *
 * @param app            Packetdrill app context.
 *
 * @sa tapi_packetdrill_app_init
 */
extern void tapi_packetdrill_app_destroy(tapi_packetdrill_app *app);

/**
 * Start packetdrill app
 *
 * @param app            Packetdrill app context.
 *
 * @return Status code.
 *
 * @sa tapi_packetdrill_app_stop
 */
extern te_errno tapi_packetdrill_app_start(tapi_packetdrill_app *app);

/**
 * Wait while packetdrill client-specific app finishes its work.
 * Note, function jumps to cleanup if timeout is expired.
 *
 * @param app           Packetdrill app context.
 * @param timeout_s     Time to wait for app results
 *                      It MUST be big enough to finish client normally.
 *
 * @return Status code.
 */
extern te_errno tapi_packetdrill_app_wait(tapi_packetdrill_app *app,
                                          int timeout_s);

/**
 * Print logs. The function reads packetdrill app output (stdout, stderr).
 *
 * @param  app           Packetdrill app context.
 *
 * @note This function makes sense only with client-specific app.
 */
extern void tapi_packetdrill_print_logs(tapi_packetdrill_app *app);

/**@} <!-- END tapi_packetdrill --> */

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TAPI_PACKETDRILL_H__ */
