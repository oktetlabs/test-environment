/** @file
 * @brief Test API to FTP client routines
 *
 * Test API to FTP client routines.
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

#ifndef __TAPI_STORAGE_CLIENT_FTP_H__
#define __TAPI_STORAGE_CLIENT_FTP_H__


#include "rcf_rpc.h"
#include "tapi_storage_client.h"


#ifdef __cplusplus
extern "C" {
#endif


/**
 * FTP client specific context
 */
typedef struct tapi_storage_client_ftp_context {
    /** @todo FTP client specific context, i.e. sockets etc */
} tapi_storage_client_ftp_context;


/**
 * Pre initialized methods to operate the FTP client. It should be passed
 * to @b tapi_storage_client_init to initialize the service.
 *
 * @sa tapi_storage_client_init
 */
extern tapi_storage_client_methods tapi_storage_client_ftp_methods;


/**
 * Initialize FTP client context. Context should be released with
 * @b tapi_storage_client_ftp_context_fini when it is no longer needed.
 *
 * @param[out] context      FTP client context handle.
 *
 * @return Status code.
 *
 * @sa tapi_storage_client_ftp_context_fini
 *
 * @todo It is need an input initialization parameters.
 */
extern te_errno tapi_storage_client_ftp_context_init(
                                tapi_storage_client_ftp_context *context);

/**
 * Release FTP client context that was initialized with
 * @b tapi_storage_client_ftp_context_init.
 *
 * @param context       FTP client context handle.
 *
 * @sa tapi_storage_client_ftp_context_init
 */
extern void tapi_storage_client_ftp_context_fini(
                                tapi_storage_client_ftp_context *context);

/**
 * Initialize FTP client handle.
 * Client should be released with @b tapi_storage_client_ftp_fini when it is
 * no longer needed.
 *
 * @param[in]  rpcs         RPC server handle.
 * @param[in]  methods      Back-end client scpecific methods.
 * @param[in]  auth         Back-end client specific authorization
 *                          parameters.
 * @param[in]  context      Back-end client scpecific context.
 * @param[out] client       FTP client handle.
 *
 * @return Status code.
 *
 * @sa tapi_storage_client_ftp_fini
 */
extern te_errno tapi_storage_client_ftp_init(
                                rcf_rpc_server                    *rpcs,
                                const tapi_storage_client_methods *methods,
                                tapi_storage_auth_params          *auth,
                                tapi_storage_client_ftp_context   *context,
                                tapi_storage_client               *client);

/**
 * Release FTP client that was initialized with
 * @b tapi_storage_client_ftp_init.
 *
 * @param client        FTP client handle.
 *
 * @sa tapi_storage_client_ftp_init
 */
extern void tapi_storage_client_ftp_fini(tapi_storage_client *client);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TAPI_STORAGE_CLIENT_FTP_H__ */
