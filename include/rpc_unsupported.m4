
/*
 * This file is automatically generated by program te_unsupported_rpc.
 * DO NOT EDIT IT!!!
 *
 */

#include "config.h"
#include "te_defs.h"
#include "rcf_rpc_defs.h"
#include "tarpc.h"
#include "ta_rpc_log.h"
#include "rpc_supported.h"

changequote([,])
define([RPC_DEF], 
[
#ifndef HAVE_$1_1_svc
bool_t
_$1_1_svc(tarpc_$1_in *in, tarpc_$1_out *out, struct svc_req *rqstp)                              
{                  
    UNUSED(rqstp);
    UNUSED(in);
    UNUSED(out);
    ERROR("Unsupported function '$1' is called");
    return FALSE;
}
#endif
])

