/** @file
 * @brief Test API for RPC
 *
 * TAPI for remote calls of stdio routines
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#include "te_config.h"

#include <stdio.h>
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_STDARG_H
#include <stdarg.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#endif
#if HAVE_STRINGS_H
#include <strings.h>
#endif

#ifndef WITH_TR069_SUPPORT
#error This source should not be compiled without tr069 configured support.
#endif
// #include "tapi_rpc_internal.h"
#include "tapi_rpc_tr069.h"



te_errno
rpc_cwmp_op_call(rcf_rpc_server *rpcs,
                 const char *acs_name, const char *cpe_name,
                 te_cwmp_rpc_cpe_t cwmp_rpc,
                 uint8_t *buf, size_t buflen,
                 acse_request_id_t *request_id)
{
    tarpc_cwmp_op_call_in  in;
    tarpc_cwmp_op_call_out out;

    if (NULL == rpcs)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        return -1;
    }
    if (NULL == acs_name || NULL == cpe_name)
    {
        ERROR("%s(): Invalid ACS/CPE handle", __FUNCTION__);
        return -1;
    }

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    rpcs->op = RCF_RPC_CALL_WAIT;

    in.acs_name = strdup(acs_name);
    in.cpe_name = strdup(cpe_name);
    in.cwmp_rpc = cwmp_rpc;

    if (buf != NULL && buflen != 0)
    {
        in.buf.buf_len = buflen;
        in.buf.buf_val = buf;
    }
    else
    {
        in.buf.buf_len = 0;
        in.buf.buf_val = NULL;
    }

    rcf_rpc_call(rpcs, "cwmp_op_call", &in, &out);

    free(in.acs_name);
    free(in.cpe_name);

    RING("RPC (%s,%s): cwmp_op_call(%s, %s, rpc %d) -> %r",
                 rpcs->ta, rpcs->name,
                 acs_name, cpe_name, (int)cwmp_rpc,
                 (te_errno)out.status);

    if (NULL != request_id)
        *request_id = out.request_id;

    return out.status;
}


te_errno
rpc_cwmp_op_check(rcf_rpc_server *rpcs,
                  const char *acs_name, const char *cpe_name,
                  acse_request_id_t request_id,
                  te_cwmp_rpc_acs_t cwmp_rpc_acs,
                  te_cwmp_rpc_cpe_t *cwmp_rpc,
                  uint8_t **buf, size_t *buflen)
{
    tarpc_cwmp_op_check_in  in;
    tarpc_cwmp_op_check_out out;

    if (NULL == rpcs)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        return -1;
    }
    if (NULL == acs_name || NULL == cpe_name)
    {
        ERROR("%s(): Invalid ACS/CPE handle", __FUNCTION__);
        return -1;
    }

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    rpcs->op = RCF_RPC_CALL_WAIT;

    in.acs_name = strdup(acs_name);
    in.cpe_name = strdup(cpe_name);
    in.request_id = request_id;
    in.cwmp_rpc = cwmp_rpc_acs;

    rcf_rpc_call(rpcs, "cwmp_op_check", &in, &out);

    free(in.acs_name);
    free(in.cpe_name);

    if (buf != NULL && buflen != NULL && out.buf.buf_val != NULL)
    {
        *buflen = out.buf.buf_len;
        *buf = malloc(out.buf.buf_len);
        memcpy(*buf, out.buf.buf_val, out.buf.buf_len);
    }

    if (NULL != cwmp_rpc)
        *cwmp_rpc = out.cwmp_rpc;

    TAPI_RPC_LOG("RPC (%s,%s): cwmp_op_check(%s, %s, req %d) -> %r",
                 rpcs->ta, rpcs->name,
                 acs_name, cpe_name, (int)request_id,
                 (te_errno)out.status);

    return out.status;
}


te_errno
rpc_cwmp_conn_req(rcf_rpc_server *rpcs,
                  const char *acs_name, const char *cpe_name)
{
    tarpc_cwmp_conn_req_in  in;
    tarpc_cwmp_conn_req_out out;

    RING("%s() called, srv %s, to %s/%s",
         __FUNCTION__, rpcs->name, acs_name, cpe_name);
    if (NULL == rpcs)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        return -1;
    }
    if (NULL == acs_name || NULL == cpe_name)
    {
        ERROR("%s(): Invalid ACS/CPE handle", __FUNCTION__);
        return -1;
    }

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    rpcs->op = RCF_RPC_CALL_WAIT;

    in.acs_name = strdup(acs_name);
    in.cpe_name = strdup(cpe_name);

    rcf_rpc_call(rpcs, "cwmp_conn_req", &in, &out);

    free(in.acs_name);
    free(in.cpe_name);

    return out.status;
}

