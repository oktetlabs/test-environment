/** @file
 * @brief Common test API to both client and server storage routines
 *
 * Common definitions for both client and server storage services.
 *
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights served.
 *
 * 
 *
 *
 * @author Ivan Melnikov <Ivan.Melnikov@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TAPI_STORAGE_COMMON_H__
#define __TAPI_STORAGE_COMMON_H__

#include "te_errno.h"
#include "te_stdint.h"

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif


#ifdef __cplusplus
extern "C" {
#endif

/** On-stack tapi_storage_auth_params structure initializer. */
#define TAPI_STORAGE_AUTH_INIT { \
    .server_addr = NULL,            \
    .port = 0,                      \
    .user = NULL,                   \
    .password = NULL                \
}

/**
 * Back-end service type.
 */
typedef enum tapi_storage_service_type {
    TAPI_STORAGE_SERVICE_FTP,       /**< FTP service. */
    TAPI_STORAGE_SERVICE_SAMBA,     /**< Samba service. */
    TAPI_STORAGE_SERVICE_DLNA,      /**< DLNA service. */

    TAPI_STORAGE_SERVICE_UNSPECIFIED    /**< Unspecified service, marks it
                                             as uninitialized. */
} tapi_storage_service_type;

/**
 * Authorization parameters of service.
 */
typedef struct tapi_storage_auth_params {
    struct sockaddr *server_addr;   /**< IP address of server. */
    uint16_t         port;          /**< Service port. */
    char            *user;          /**< User name to log in. */
    char            *password;      /**< User password. */
} tapi_storage_auth_params;


/**
 * Set up service authorization parameters. @p auth_params should be
 * released with @p tapi_storage_auth_params_fini when it is no longer
 * needed.
 *
 * @param[in]  server_addr  The server address, may be @c NULL.
 * @param[in]  port         The server port.
 * @param[in]  user         User name for access, may be @c NULL.
 * @param[in]  password     Password, may be @c NULL.
 * @param[out] auth_params  Authorization parameters.
 *
 * @return Status code.
 *
 * @sa tapi_storage_auth_params_fini
 */
extern te_errno tapi_storage_auth_params_init(
                                const struct sockaddr    *server_addr,
                                uint16_t                  port,
                                const char               *user,
                                const char               *password,
                                tapi_storage_auth_params *auth_params);

/**
 * Release service authorization parameters that was initialized with
 * @p tapi_storage_auth_params_init.
 *
 * @param auth_params   Authorization parameters.
 *
 * @sa tapi_storage_auth_params_init
 */
extern void tapi_storage_auth_params_fini(
                                tapi_storage_auth_params *auth_params);

/**
 * Make a deep copy of authorization parameters. Actually this function
 * performs initialization of @p to with @p from's parameters.
 *
 * @param to        Auth params to copy to.
 * @param from      Auth params to copy from.
 *
 * @return Status code.
 *
 * @sa tapi_storage_auth_params_init
 */
extern te_errno tapi_storage_auth_params_copy(
                                tapi_storage_auth_params *to,
                                const tapi_storage_auth_params *from);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TAPI_STORAGE_COMMON_H__ */
