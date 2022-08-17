/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Test API to manage NTP daemon
 *
 * @defgroup tapi_conf_ntpd NTP daemon configuration
 * @ingroup tapi_conf
 * @{
 *
 * Definition of API to manage ntpd daemon.
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_TAPI_NTPD_H__
#define __TE_TAPI_NTPD_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "te_errno.h"
#include "rcf_rpc.h"

/**
 * Start ntpd daemon.
 *
 * @param rpcs RPC server
 */
static inline void
tapi_ntpd_enable(rcf_rpc_server *rpcs)
{
    CHECK_RC(cfg_set_instance_fmt(CVT_INTEGER, (void *)1,
                                  "/agent:%s/ntpd:/enable:",
                                  rpcs->ta));
    TAPI_WAIT_NETWORK;
}

/**
 * Stop ntpd daemon.
 *
 * @param rpcs RPC server
 */
static inline void
tapi_ntpd_disable(rcf_rpc_server *rpcs)
{
    CHECK_RC(cfg_set_instance_fmt(CVT_INTEGER, (void *)0,
                                  "/agent:%s/ntpd:/enable:",
                                  rpcs->ta));
}

/**
 * ntpd daemon status.
 *
 * @param rpcs RPC server
 *
 * @return @c TRUE if deaimon is enabled
 */
static inline te_bool
tapi_ntpd_status(rcf_rpc_server *rpcs)
{
    int val;

    CHECK_RC(cfg_get_instance_int_fmt(&val, "/agent:%s/ntpd:/enable:",
                                      rpcs->ta));

    return val == 0 ? FALSE : TRUE;
}


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ndef __TE_TAPI_NTPD_H__ */

/**@} <!-- END tapi_conf_ntpd --> */
