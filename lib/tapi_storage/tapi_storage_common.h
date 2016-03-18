/** @file
 * @brief Common test API to both client and server storage routines
 *
 * Common definitions for both client and server storage services.
 *
 *
 * Copyright (C) 2016 Test Environment authors (see file AUTHORS
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
 *
 * $Id$
 */

#ifndef __TAPI_STORAGE_COMMON_H__
#define __TAPI_STORAGE_COMMON_H__


#ifdef __cplusplus
extern "C" {
#endif

/**
 * Back-end service type.
 */
typedef enum tapi_storage_service_type {
    TAPI_STORAGE_SERVICE_FTP,       /**< FTP service. */
    TAPI_STORAGE_SERVICE_SAMBA,     /**< Samba service. */
    TAPI_STORAGE_SERVICE_DLNA,      /**< DLNA service. */
} tapi_storage_service_type;

/**
 * Authorization parameters of service.
 */
typedef struct tapi_storage_auth_params {
    struct sockaddr_storage *server_addr,   /**< IP address of server. */
    uint16_t                 port,          /**< Service port. */
    const char              *user,          /**< User name to log in. */
    const char              *password,      /**< User password. */
} tapi_storage_auth_params;


/**
 * Set up service authorization parameters. @p auth_params should be
 * released with @b tapi_storage_auth_params_fini when it is no longer
 * needed.
 *
 * @param[in]  server_addr  The server address.
 * @param[in]  port         The server port.
 * @param[in]  user         User name for access.
 * @param[in]  password     Password.
 * @param[out] auth_params  Authorization parameters.
 *
 * @return Status code.
 *
 * @sa tapi_storage_auth_params_fini
 */
extern te_errno tapi_storage_auth_params_init(
                                struct sockaddr_storage  *server_addr,
                                uint16_t                  port,
                                const char               *user,
                                const char               *password,
                                tapi_storage_auth_params *auth_params);

/**
 * Release service authorization parameters that was initialized with
 * @b tapi_storage_auth_params_init.
 *
 * @param auth_params   Authorization parameters.
 *
 * @sa tapi_storage_auth_params_init
 */
extern void tapi_storage_auth_params_fini(
                                tapi_storage_auth_params *auth_params);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TAPI_STORAGE_COMMON_H__ */
