/** @file
 * @brief Test API for RPC
 *
 * TAPI for remote calls of interface name/index routines.
 *
 *
 * Copyright (C) 2005 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
 * Copyright (c) 2005 Level5 Networks Corp.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
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
 * @author Elena A. Vengerova <Elena.Vengerova@oktetlabs.ru>
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
 */

#include "te_config.h"

#if HAVE_STRING_H
#include <string.h>
#endif
#if HAVE_STRINGS_H
#include <strings.h>
#endif
#if HAVE_NET_IF_H
#include <net/if.h>
#endif

#include "tapi_rpc_internal.h"


unsigned int
rpc_if_nametoindex(rcf_rpc_server *rpcs,
                   const char *ifname)
{
    tarpc_if_nametoindex_in  in;
    tarpc_if_nametoindex_out out;

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        return 0;
    }

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    rpcs->op = RCF_RPC_CALL_WAIT;

    in.ifname.ifname_val = (char *)ifname;
    in.ifname.ifname_len = (ifname == NULL) ? 0 : (strlen(ifname) + 1);

    rcf_rpc_call(rpcs, "if_nametoindex", &in, &out);

    CHECK_RETVAL_VAR(if_nametoindex, out.ifindex, FALSE, 0);
    TAPI_RPC_LOG(rpcs, if_nametoindex, "%s", "%u", 
                 ifname == NULL ? "" : ifname, out.ifindex);
    RETVAL_INT(if_nametoindex, out.ifindex);
}

char *
rpc_if_indextoname(rcf_rpc_server *rpcs,
                   unsigned int ifindex, char *ifname)
{
    tarpc_if_indextoname_in  in;
    tarpc_if_indextoname_out out;

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        return NULL;
    }

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    rpcs->op = RCF_RPC_CALL_WAIT;

    in.ifindex = ifindex;
    in.ifname.ifname_val = ifname;
    in.ifname.ifname_len = (ifname == NULL) ? 0 : (strlen(ifname) + 1);

    rcf_rpc_call(rpcs, "if_indextoname", &in, &out);

    if (RPC_IS_CALL_OK(rpcs))
    {
        if (ifname != NULL && out.ifname.ifname_val != NULL)
            memcpy(ifname, out.ifname.ifname_val, out.ifname.ifname_len);
    }

    TAPI_RPC_LOG(rpcs, if_indextoname, "%u", "%s",
                 ifindex, out.ifname.ifname_val != NULL ? ifname : "");
    RETVAL_PTR(if_indextoname,
               out.ifname.ifname_val != NULL ? ifname : NULL);
}

struct if_nameindex *
rpc_if_nameindex(rcf_rpc_server *rpcs)
{
    tarpc_if_nameindex_in  in;
    tarpc_if_nameindex_out out;

    struct if_nameindex *res = NULL;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_PTR(if_nameindex, NULL);
    }

    rpcs->op = RCF_RPC_CALL_WAIT;

    rcf_rpc_call(rpcs, "if_nameindex", &in, &out);

    if (RPC_IS_CALL_OK(rpcs) && out.ptr.ptr_val != NULL)
    {
        int i;

        if ((res = calloc(sizeof(*res) * out.ptr.ptr_len +
                          sizeof(unsigned int), 1)) == NULL)
        {
            rpcs->_errno = TE_RC(TE_RCF, TE_ENOMEM);
            RETVAL_PTR(if_nameindex, NULL);
        }

        *(unsigned int *)res = out.mem_ptr;
        res = (struct if_nameindex *)((unsigned int *)res + 1);

        for (i = 0; i < (int)out.ptr.ptr_len - 1; i++)
        {
            if ((res[i].if_name =
                 strdup(out.ptr.ptr_val[i].ifname.ifname_val)) == NULL)
            {
                for (i--; i >= 0; i--)
                    free(res[i].if_name);

                res = (struct if_nameindex *)((unsigned int *)res - 1);
                free(res);
                rpcs->_errno = TE_RC(TE_RCF, TE_ENOMEM);
                RETVAL_PTR(if_nameindex, NULL);
            }
        }
    }

    TAPI_RPC_LOG(rpcs, if_nameindex, "", "%p", res);
    RETVAL_PTR(if_nameindex, res);
}

void
rpc_if_freenameindex(rcf_rpc_server *rpcs,
                     struct if_nameindex *ptr)
{
    tarpc_if_freenameindex_in  in;
    tarpc_if_freenameindex_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_VOID(if_freenameindex);
    }

    rpcs->op = RCF_RPC_CALL_WAIT;

    if (ptr == NULL)
        in.mem_ptr = 0;
    else
    {
        unsigned int *p = (unsigned int *)ptr - 1;
        in.mem_ptr = *p;
        free(p);
    }

    rcf_rpc_call(rpcs, "if_freenameindex", &in, &out);

    TAPI_RPC_LOG(rpcs, if_nameindex, "%p", "", ptr);
    RETVAL_VOID(if_freenameindex);
}
