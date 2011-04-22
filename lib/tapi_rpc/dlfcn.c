/** @file
 * @brief Test API for RPC
 *
 * TAPI for remote calls of dynamic linking loader.
 *
 *
 * Copyright (C) 2010 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
 * Copyright (c) 2010 Level5 Networks Corp.
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
 * @author Nikita Rastegaev <Nikita.Rastegaev@oktetlabs.ru>
 *
 * $Id$
 */

#include "te_config.h"

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include "te_printf.h"
#include "log_bufs.h"
#include "tapi_rpc_internal.h"
#include "tapi_rpc_dlfcn.h"


rpc_dlhandle
rpc_dlopen(rcf_rpc_server *rpcs, const char *filename, int flag)
{
    rcf_rpc_op  op;

    tarpc_ta_dlopen_in  in;
    tarpc_ta_dlopen_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(ta_dlopen, (rpc_dlhandle)NULL);
    }
    op = rpcs->op;

    in.filename = (char *)filename;
    in.flag     = flag;

    rcf_rpc_call(rpcs, "ta_dlopen", &in, &out);

    TAPI_RPC_LOG("RPC (%s,%s)%s: dlopen(%s, %s) -> %llx (%s)",
                 rpcs->ta, rpcs->name, rpcop2str(op), in.filename,
                 dlopen_flags_rpc2str(in.flag), out.retval,
                 errno_rpc2str(RPC_ERRNO(rpcs)));

    TAPI_RPC_OUT(ta_dlopen, out.retval == (int64_t)NULL);
    return out.retval;
}

char *
rpc_dlerror(rcf_rpc_server *rpcs)
{
    rcf_rpc_op  op;

    tarpc_ta_dlerror_in  in;
    tarpc_ta_dlerror_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_PTR(ta_dlerror, NULL);
    }
    op = rpcs->op;

    rcf_rpc_call(rpcs, "ta_dlerror", &in, &out);

    TAPI_RPC_LOG("RPC (%s,%s)%s: dlerror() -> %s (%s)",
                 rpcs->ta, rpcs->name, rpcop2str(op),
                 out.retval, errno_rpc2str(RPC_ERRNO(rpcs)));
    RETVAL_PTR(ta_dlerror, out.retval);
}

rpc_dlsymaddr
rpc_dlsym(rcf_rpc_server *rpcs, rpc_dlhandle handle, const char *symbol)
{
    rcf_rpc_op  op;

    tarpc_ta_dlsym_in  in;
    tarpc_ta_dlsym_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(ta_dlsym, (rpc_dlsymaddr)NULL);
    }
    op = rpcs->op;

    in.handle = (tarpc_dlhandle)handle;
    in.symbol = (char *)symbol;

    rcf_rpc_call(rpcs, "ta_dlsym", &in, &out);

    TAPI_RPC_LOG("RPC (%s,%s)%s: dlsym(%llx, %s) -> %llx (%s)",
                 rpcs->ta, rpcs->name, rpcop2str(op), in.handle,
                 in.symbol, out.retval, errno_rpc2str(RPC_ERRNO(rpcs)));

    TAPI_RPC_OUT(ta_dlsym, FALSE);
    return out.retval;
}

int
rpc_dlsym_call(rcf_rpc_server *rpcs, rpc_dlhandle handle, const char *symbol)
{
    rcf_rpc_op  op;

    tarpc_ta_dlsym_call_in  in;
    tarpc_ta_dlsym_call_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(ta_dlsym_call, -1);
    }
    op = rpcs->op;

    in.handle = (tarpc_dlhandle)handle;
    in.symbol = (char *)symbol;

    rcf_rpc_call(rpcs, "ta_dlsym_call", &in, &out);

    TAPI_RPC_LOG("RPC (%s,%s)%s: dlsym_call(%llx, %s) -> %d (%s)",
                 rpcs->ta, rpcs->name, rpcop2str(op), in.handle,
                 in.symbol, out.retval, errno_rpc2str(RPC_ERRNO(rpcs)));
    RETVAL_INT(ta_dlsym_call, out.retval);
}

int
rpc_dlclose(rcf_rpc_server *rpcs, rpc_dlhandle handle)
{
    rcf_rpc_op  op;

    tarpc_ta_dlclose_in  in;
    tarpc_ta_dlclose_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_ZERO_INT(ta_dlclose, -1);
    }
    op = rpcs->op;

    in.handle = (tarpc_dlhandle)handle;

    rcf_rpc_call(rpcs, "ta_dlclose", &in, &out);

    TAPI_RPC_LOG("RPC (%s,%s)%s: dlclose(%llx) -> %d (%s)",
                 rpcs->ta, rpcs->name, rpcop2str(op), in.handle,
                 out.retval, errno_rpc2str(RPC_ERRNO(rpcs)));
    RETVAL_ZERO_INT(ta_dlclose, out.retval);
}


