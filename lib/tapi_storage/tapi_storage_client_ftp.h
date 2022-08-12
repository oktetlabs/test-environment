/** @file
 * @brief Test API to FTP client routines
 *
 * @defgroup tapi_storage_client_ftp Test API to control the storage FTP client
 * @ingroup tapi_storage
 * @{
 *
 * Test API to FTP client routines.
 *
 * Copyright (C) 2004-2018 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 * @author Ivan Melnikov <Ivan.Melnikov@oktetlabs.ru>
 *
 */

#ifndef __TAPI_STORAGE_CLIENT_FTP_H__
#define __TAPI_STORAGE_CLIENT_FTP_H__


#include "tapi_storage_client.h"


#ifdef __cplusplus
extern "C" {
#endif


/* Forward declaration of FTP client specific context structure. */
struct tapi_storage_client_ftp_context;
typedef struct tapi_storage_client_ftp_context
    tapi_storage_client_ftp_context;


/**
 * Pre initialized methods to operate the FTP client. It should be passed
 * to @p tapi_storage_client_init to initialize the service.
 *
 * @sa tapi_storage_client_init
 */
extern tapi_storage_client_methods tapi_storage_client_ftp_methods;


/**
 * Initialize FTP client context. Context should be released with
 * @p tapi_storage_client_ftp_context_fini when it is no longer needed.
 *
 * @param[out] context      FTP client context handle.
 *
 * @return Status code.
 *
 * @sa tapi_storage_client_ftp_context_fini
 */
extern te_errno tapi_storage_client_ftp_context_init(
                                tapi_storage_client_ftp_context **context);

/**
 * Release FTP client context that was initialized with
 * @p tapi_storage_client_ftp_context_init.
 *
 * @param context       FTP client context handle.
 *
 * @sa tapi_storage_client_ftp_context_init
 */
extern void tapi_storage_client_ftp_context_fini(
                                tapi_storage_client_ftp_context *context);

/**
 * Initialize FTP client handle.
 * Client should be released with @p tapi_storage_client_ftp_fini when it is
 * no longer needed.
 *
 * @param[in]  rpcs         RPC server handle.
 * @param[in]  methods      Back-end client scpecific methods. If @c NULL
 *                          then default methods will be used. Default
 *                          methods is defined in tapi_storage_client_ftp.c.
 * @param[in]  auth         Back-end client specific authorization
 *                          parameters. May be @c NULL.
 * @param[in]  context      Back-end client scpecific context. Don't free
 *                          the @p context before finishing work with
                            @p client.
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
 * @p tapi_storage_client_ftp_init.
 *
 * @param client        FTP client handle.
 *
 * @sa tapi_storage_client_ftp_init
 */
extern void tapi_storage_client_ftp_fini(tapi_storage_client *client);

/**@} <!-- END tapi_storage_client_ftp --> */

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TAPI_STORAGE_CLIENT_FTP_H__ */
