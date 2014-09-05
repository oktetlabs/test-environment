/** @file
 * @brief Test API to manage NTP daemon
 *
 * Definition of API to manage ntpd daemon.
 *
 * Copyright (C) 2004-2014 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
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
 * @author Andrey Dmitrov <Andrey.Dmitrov@oktetlabs.ru>
 *
 * $Id$
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
    cfg_val_type val_type;
    int val;

    val_type = CVT_INTEGER;
    CHECK_RC(cfg_get_instance_fmt(&val_type, &val,
                                  "/agent:%s/ntpd:/enable:", rpcs->ta));

    return val == 0 ? FALSE : TRUE;
}


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ndef __TE_TAPI_NTPD_H__ */

