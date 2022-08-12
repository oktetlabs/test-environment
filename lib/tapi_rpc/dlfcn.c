/** @file
 * @brief Test API for RPC
 *
 * TAPI for remote calls of dynamic linking loader.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
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
    tarpc_ta_dlopen_in  in;
    tarpc_ta_dlopen_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_PTR64(ta_dlopen, RPC_DLHANDLE_NULL);
    }

    in.filename = (char *)filename;
    in.flag     = flag;

    rcf_rpc_call(rpcs, "ta_dlopen", &in, &out);

    TAPI_RPC_LOG(rpcs, dlopen, "%s, %s", "%llx",
                 in.filename, dlopen_flags_rpc2str(in.flag), out.retval);
    RETVAL_PTR64(ta_dlopen, out.retval);
}

char *
rpc_dlerror(rcf_rpc_server *rpcs)
{
    tarpc_ta_dlerror_in  in;
    tarpc_ta_dlerror_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_PTR(ta_dlerror, NULL);
    }

    rcf_rpc_call(rpcs, "ta_dlerror", &in, &out);

    TAPI_RPC_LOG(rpcs, dlerror, "", "%s", out.retval);
    RETVAL_PTR(ta_dlerror, out.retval);
}

rpc_dlsymaddr
rpc_dlsym(rcf_rpc_server *rpcs, rpc_dlhandle handle, const char *symbol)
{
    tarpc_ta_dlsym_in  in;
    tarpc_ta_dlsym_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_PTR64(ta_dlsym, RPC_DLSYM_NULL);
    }

    /* tarpc_dlhandle and rpc_dlhandle are expected to be the same type */
    in.handle = (tarpc_dlhandle)handle;
    in.symbol = (char *)symbol;

    rcf_rpc_call(rpcs, "ta_dlsym", &in, &out);

    TAPI_RPC_LOG(rpcs, dlsym, "%llx %s", "%llx",
                 in.handle, in.symbol, out.retval);
    RETVAL_PTR64(ta_dlsym, out.retval);
}

int
rpc_dlsym_call(rcf_rpc_server *rpcs, rpc_dlhandle handle, const char *symbol)
{
    tarpc_ta_dlsym_call_in  in;
    tarpc_ta_dlsym_call_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(ta_dlsym_call, -1);
    }

    /* tarpc_dlhandle and rpc_dlhandle are expected to be the same type */
    in.handle = (tarpc_dlhandle)handle;
    in.symbol = (char *)symbol;

    rcf_rpc_call(rpcs, "ta_dlsym_call", &in, &out);

    TAPI_RPC_LOG(rpcs, dlsym_call, "%llx %s", "%d",
                 in.handle, in.symbol, out.retval);
    RETVAL_INT(ta_dlsym_call, out.retval);
}

int
rpc_dlclose(rcf_rpc_server *rpcs, rpc_dlhandle handle)
{
    tarpc_ta_dlclose_in  in;
    tarpc_ta_dlclose_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_ZERO_INT(ta_dlclose, -1);
    }

    /* tarpc_dlhandle and rpc_dlhandle are expected to be the same type */
    in.handle = (tarpc_dlhandle)handle;

    rcf_rpc_call(rpcs, "ta_dlclose", &in, &out);

    TAPI_RPC_LOG(rpcs, dlclose, "%llx", "%d", in.handle, out.retval);
    RETVAL_ZERO_INT(ta_dlclose, out.retval);
}


