/** @file
 * @brief Test API to DLNA UPnP routines
 *
 * Implementation of API to configure UPnP Control Point.
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

#include <glib-object.h>
#include "tapi_upnp.h"
#include "tapi_upnp_cp.h"
#include "tapi_rpc_internal.h"


/* Format string for UPnP Control Point node in the Configuration tree. */
#define TE_CFG_UPNP_CP_ROOT_FMT   "/agent:%s/upnp_cp:"


/* See description in tapi_upnp_cp.h. */
te_errno
tapi_upnp_cp_start(const char *ta, const char *target, const char *iface)
{
    te_errno    rc;
    const char *search_target;

    search_target = (target == NULL || target[0] == '\0'
                     ? TAPI_UPNP_ST_ALL_RESOURCES
                     : target);

    rc = cfg_set_instance_fmt(CFG_VAL(STRING, search_target),
                              TE_CFG_UPNP_CP_ROOT_FMT "/target:", ta);
    if (rc != 0)
    {
        ERROR("Failed to set the search target");
        return rc;
    }
    rc = cfg_set_instance_fmt(CFG_VAL(STRING, iface),
                              TE_CFG_UPNP_CP_ROOT_FMT "/iface:", ta);
    if (rc != 0)
    {
        ERROR("Failed to set the network interface");
        return rc;
    }
    rc = cfg_set_instance_fmt(CFG_VAL(INTEGER, 1),
                              TE_CFG_UPNP_CP_ROOT_FMT "/enable:", ta);
    if (rc != 0)
    {
        ERROR("Failed to enable and start UPnP Control Point");
        return rc;
    }
    return rc;
}

/* See description in tapi_upnp_cp.h. */
te_errno
tapi_upnp_cp_stop(const char *ta)
{
    return cfg_set_instance_fmt(CFG_VAL(INTEGER, 0),
                                TE_CFG_UPNP_CP_ROOT_FMT "/enable:", ta);
}

/* See description in tapi_upnp_cp.h. */
te_bool
tapi_upnp_cp_started(const char *ta)
{
    te_errno     rc;
    cfg_val_type type = CVT_INTEGER;
    int          enabled = 0;

    rc = cfg_get_instance_fmt(&type, &enabled,
                              TE_CFG_UPNP_CP_ROOT_FMT "/enable:", ta);
    if (rc != 0)
        ERROR("Failed to get the UPnP Control Point 'enable' value");

    return (enabled != 0);
}

/* See description in tapi_upnp_cp.h. */
te_errno
rpc_upnp_cp_connect(rcf_rpc_server *rpcs)
{
    tarpc_upnp_cp_connect_in  in = {};
    tarpc_upnp_cp_connect_out out = {};

    out.retval = -1;
    rcf_rpc_call(rpcs, "upnp_cp_connect", &in, &out);
    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(upnp_cp_connect, out.retval);
    TAPI_RPC_LOG(rpcs, upnp_cp_connect, "void", "%d", out.retval);
    RETVAL_INT(upnp_cp_connect, out.retval);
}

/* See description in tapi_upnp_cp.h. */
te_errno
rpc_upnp_cp_disconnect(rcf_rpc_server *rpcs)
{
    tarpc_upnp_cp_disconnect_in  in = {};
    tarpc_upnp_cp_disconnect_out out = {};

    out.retval = -1;
    rcf_rpc_call(rpcs, "upnp_cp_disconnect", &in, &out);
    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(upnp_cp_disconnect, out.retval);
    TAPI_RPC_LOG(rpcs, upnp_cp_disconnect, "void", "%d", out.retval);
    RETVAL_INT(upnp_cp_disconnect, out.retval);
}

/* See description in tapi_upnp_cp.h. */
te_errno
rpc_upnp_cp_action(rcf_rpc_server *rpcs,
                   const void     *request,
                   size_t          request_len,
                   void          **reply,
                   size_t         *reply_len)
{
    struct tarpc_upnp_cp_action_in  in = {};
    struct tarpc_upnp_cp_action_out out = {};

    if (request == NULL || request_len == 0)
    {
        ERROR("Request is missing");
        return TE_RC(TE_TAPI, TE_EINVAL);
    }
    if (reply_len == NULL)
    {
        ERROR("reply_len is NULL");
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    in.buf.buf_len = request_len;
    in.buf.buf_val = request;
    out.retval = -1;
    rcf_rpc_call(rpcs, "upnp_cp_action", &in, &out);
    /*
     * In case this was just a call without waiting for the result
     * we should not copy output parameters (they are not ready yet).
     */
    if (RPC_IS_CALL_OK(rpcs) && rpcs->op != RCF_RPC_WAIT)
    {
        if (out.buf.buf_val == NULL || out.buf.buf_len == 0)
            *reply_len = 0;
        else
        {
            if (*reply != NULL)
            {
                if (*reply_len < out.buf.buf_len)
                {
                    ERROR("The receive buffer is too small. It is required "
                          "at least %u bytes", out.buf.buf_len);
                    RETVAL_INT(upnp_cp_action, -1);
                }
            }
            else
            {
                *reply = malloc(out.buf.buf_len);
                if (*reply == NULL)
                {
                    ERROR("cannot allocate %u bytes", out.buf.buf_len);
                    RETVAL_INT(upnp_cp_action, -1);
                }
            }
            memcpy(*reply, out.buf.buf_val, out.buf.buf_len);
            *reply_len = out.buf.buf_len;
        }
    }
    TAPI_RPC_LOG(rpcs, upnp_cp_action, "%p, %d, %p %d", "%d",
                 in.buf.buf_val, in.buf.buf_len,
                 out.buf.buf_val, out.buf.buf_len,
                 out.retval);
    RETVAL_INT(upnp_cp_action, out.retval);
}
