/** @file
 * @brief API to configure some system options via /proc/sys
 *
 * Implementation of API for access to some system options via /proc/sys.
 *
 *
 * Copyright (C) 2013 Test Environment authors (see file AUTHORS
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
 * @author Andrey Dmitrov <Andrey.Dmitrov@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER     "TAPI Proc"

#include "te_config.h"
#include "conf_api.h"
#include "tapi_proc.h"
#include "tapi_rpc_unistd.h"

/* See description in tapi_proc.h */
int
tapi_cfg_net_route_flush(rcf_rpc_server *rpcs)
{
    int     fd;
    te_bool wait_err = FALSE;

    if (RPC_AWAITING_ERROR(rpcs))
        wait_err = TRUE;

    if ((fd = rpc_open(rpcs, "/proc/sys/net/ipv4/route/flush",
                       RPC_O_WRONLY, 0)) < 0)
        return -1;

    if (wait_err)
        RPC_AWAIT_IUT_ERROR(rpcs);
    if (rpc_write(rpcs, fd, "1", 1) < 0)
    {
        rpc_close(rpcs, fd);
        return -1;
    }

    if (wait_err)
        RPC_AWAIT_IUT_ERROR(rpcs);
    if (rpc_close(rpcs, fd) != 0)
        return -1;

    return 0;
}

/* See description in tapi_proc.h */
te_errno
tapi_cfg_tcp_timestamps_set(rcf_rpc_server *rpcs, int value, int *old_value)
{
    cfg_val_type  val_type = CVT_INTEGER;
    te_errno      rc;

    if (old_value != NULL)
    {
        rc = cfg_get_instance_fmt(&val_type, old_value, 
                                  "/agent:%s/sys:/tcp_timestamps:",
                                  rpcs->ta);
        if (rc != 0)
        {
            ERROR("Failed to get tcp_timestamps value");
            return rc;
        }
    }

    rc = cfg_set_instance_fmt(CFG_VAL(INTEGER, value),
                              "/agent:%s/sys:/tcp_timestamps:",
                              rpcs->ta);
    if (rc != 0)
        ERROR("Failed to set new tcp_timestamps value");

    return rc;
}

/* See description in tapi_proc.h */
te_errno
tapi_cfg_tcp_timestamps_get(rcf_rpc_server *rpcs, int *value)
{
    cfg_val_type  val_type = CVT_INTEGER;
    te_errno      rc;

    rc = cfg_get_instance_fmt(&val_type, value, 
                              "/agent:%s/sys:/tcp_timestamps:",
                              rpcs->ta);
    if (rc != 0)
        ERROR("Failed to get tcp_timestamps value");
    return rc;
}

/* See description in tapi_proc.h */
te_errno 
tapi_cfg_if_rp_filter_get(rcf_rpc_server *rpcs, const char *ifname,
                          int *rp_filter)
{
    cfg_val_type  val_type = CVT_INTEGER;
    te_errno      rc;

    rc = cfg_get_instance_fmt(&val_type, rp_filter, 
                              "/agent:%s/interface:%s/rp_filter:",
                              rpcs->ta, ifname);
    if (rc != 0)
        ERROR("Failed to get interface rp_filter value");
    return rc;
}

/* See description in tapi_proc.h */
te_errno
tapi_cfg_if_rp_filter_set(rcf_rpc_server *rpcs, const char *ifname,
                          int rp_filter, int *old_value)
{
    te_errno rc;

    if (old_value != NULL)
    {
        rc = tapi_cfg_if_rp_filter_get(rpcs, ifname, old_value);
        if (rc != 0)
        {
            ERROR("Failed to get old rp_filter value");
            return rc;
        }
    }

    rc = cfg_set_instance_fmt(CFG_VAL(INTEGER, rp_filter),
                              "/agent:%s/interface:%s/rp_filter:",
                              rpcs->ta, ifname);
    if (rc != 0)
        ERROR("Failed to set rp_filter value for interface %s", ifname);

    return rc;
}

/**
 * Macros to define similar functions to get and set system values in /proc
 * 
 * @param _name     Field name to be used in functions name.
 * @param _path     Path in configurator tree, it should include '%s'
 *                  modifier for agent name.
 */
#define DEFINE_API_FUNC_TA_ONLY(_name, _path) \
te_errno                                                                \
tapi_cfg_##_name##_set(rcf_rpc_server *rpcs, int value, int *old_value) \
{                                                                       \
    cfg_val_type  val_type = CVT_INTEGER;                               \
    te_errno      rc;                                                   \
                                                                        \
    if (old_value != NULL)                                              \
    {                                                                   \
        rc = cfg_get_instance_fmt(&val_type, old_value, _path, rpcs->ta); \
        if (rc != 0)                                                    \
        {                                                               \
            ERROR("Failed to get " #_name " value");                    \
            return rc;                                                  \
        }                                                               \
    }                                                                   \
    rc = cfg_set_instance_fmt(CFG_VAL(INTEGER, value), _path, rpcs->ta); \
    if (rc != 0)                                                        \
        ERROR("Failed to set new " #_name " value");                    \
    return rc;                                                          \
}                                                                       \
                                                                        \
te_errno                                                                \
tapi_cfg_##_name##_get(rcf_rpc_server *rpcs, int *value)                \
{                                                                       \
    cfg_val_type  val_type = CVT_INTEGER;                               \
    te_errno      rc;                                                   \
                                                                        \
    rc = cfg_get_instance_fmt(&val_type, value, _path, rpcs->ta);       \
    if (rc != 0)                                                        \
        ERROR("Failed to get " #_name " value");                        \
    return rc;                                                          \
}

DEFINE_API_FUNC_TA_ONLY(tcp_syncookies, "/agent:%s/sys:/tcp_syncookies:")
DEFINE_API_FUNC_TA_ONLY(tcp_keepalive_time,
                        "/agent:%s/sys:/tcp_keepalive_time:")
DEFINE_API_FUNC_TA_ONLY(tcp_keepalive_probes,
                        "/agent:%s/sys:/tcp_keepalive_probes:")
DEFINE_API_FUNC_TA_ONLY(tcp_keepalive_intvl,
                        "/agent:%s/sys:/tcp_keepalive_intvl:")
DEFINE_API_FUNC_TA_ONLY(tcp_retries2,
                        "/agent:%s/sys:/tcp_retries2:")
DEFINE_API_FUNC_TA_ONLY(tcp_orphan_retries,
                        "/agent:%s/sys:/tcp_orphan_retries:")
DEFINE_API_FUNC_TA_ONLY(tcp_syn_retries,
                        "/agent:%s/sys:/tcp_syn_retries:")
DEFINE_API_FUNC_TA_ONLY(tcp_fin_timeout,
                        "/agent:%s/sys:/tcp_fin_timeout:")
DEFINE_API_FUNC_TA_ONLY(somaxconn, "/agent:%s/sys:/somaxconn:")

#undef DEFINE_API_FUNC_TA_ONLY
