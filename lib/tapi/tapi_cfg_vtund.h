/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Test API to configure VTund.
 *
 * @defgroup tapi_conf_vtund VTund configuration
 * @ingroup tapi_conf
 * @{
 *
 * Definition of API to configure VTund.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_TAPI_CFG_VTUND_H__
#define __TE_TAPI_CFG_VTUND_H__

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#include "te_errno.h"
#include "conf_api.h"


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup tapi_conf_vtund VTun (Virtual Tunnel) daemon configuration
 * @ingroup tapi_conf
 * @{
 */

/**
 * Create a tunnel between two hosts.
 *
 * @param ta_srv        Test Agent with VTund server
 * @param ta_clnt       Test Agent with VTund client
 * @param srv_addr      Address and port for VTund server to listen to
 *                      and for VTund client to connect to (if port is 0,
 *                      default VTund port is used)
 * @param ta_srv_if     Configurator handle of the created interface
 *                      on the Test Agent with VTund server
 * @param ta_clnt_if    Configurator handle of the created interface
 *                      on the Test Agent with VTund client
 *
 * @return Status code
 */
extern te_errno tapi_cfg_vtund_create_tunnel(
                    const char            *ta_srv,
                    const char            *ta_clnt,
                    const struct sockaddr *srv_addr,
                    cfg_handle            *ta_srv_if,
                    cfg_handle            *ta_clnt_if);

/**@} <!-- END tapi_conf_vtund --> */

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_CFG_VTUND_H__ */

/**@} <!-- END tapi_conf_vtund --> */
