/** @file
 * @brief Test API to DLNA UPnP routines
 *
 * @defgroup tapi_upnp_cp Test API to operate the UPnP Control Point
 * @ingroup tapi_upnp
 * @{
 *
 * Definition of API to configure UPnP Control Point.
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 *
 */

#ifndef __TAPI_UPNP_CP_H__
#define __TAPI_UPNP_CP_H__

#include "te_errno.h"
#include "rcf_rpc.h"

#ifdef __cplusplus
extern "C" {
#endif


/* Functions to manage TA UPnP Control Point. */

/**
 * Enable the UPnP Control Point.
 *
 * @param ta            Test Agent name.
 * @param target        Search Target for UPnP devices and/or services.
 *                      Can be @c NULL or zero-length string, in this case
 *                      @c TAPI_UPNP_ST_ALL_RESOURCES will be used.
 * @param iface         Network interface to use by Control Point.
 *
 * @return Status code
 */
extern te_errno tapi_upnp_cp_start(const char *ta,
                                   const char *target,
                                   const char *iface);

/**
 * Disable the UPnP Control Point.
 *
 * @param ta            Test Agent name.
 *
 * @return Status code
 */
extern te_errno tapi_upnp_cp_stop(const char *ta);

/**
 * Check for either UPnP Control Point started or not.
 *
 * @param ta            Test Agent name.
 *
 * @return The UPnP CP started condition.
 */
extern te_bool tapi_upnp_cp_started(const char *ta);


/* Functions to manage TA UPnP Control Point proxy. */

/**
 * Create UNIX socket connection with UPnP Control Point process.
 *
 * @param rpcs          RPC server handle.
 *
 * @return Status code
 */
extern te_errno rpc_upnp_cp_connect(rcf_rpc_server *rpcs);

/**
 * Destroy UNIX socket connection with UPnP Control Point process.
 *
 * @param rpcs          RPC server handle.
 *
 * @return Status code
 */
extern te_errno rpc_upnp_cp_disconnect(rcf_rpc_server *rpcs);

/**
 * Make a request for UPnP specific data of UPnP Control Point through
 * RPCS (proxy) and wait for reply (blocking function).
 * User should perform free on @p reply when it needs no more.
 *
 * @param[in]  rpcs         RPC server handle.
 * @param[in]  request      Request message.
 * @param[in]  request_len  Request message length.
 * @param[out] reply        Reply message. If @c NULL, then the memory will
 *                          be allocated. On otherwise the @p reply_len must
 *                          specify the @p reply buffer size.
 * @param[out] reply_len    Reply message length.
 *
 * @return Status code
 */
extern te_errno rpc_upnp_cp_action(rcf_rpc_server *rpcs,
                                   const void     *request,
                                   size_t          request_len,
                                   void          **reply,
                                   size_t         *reply_len);

/**@} <!-- END tapi_upnp_cp --> */

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TAPI_UPNP_CP_H__ */
