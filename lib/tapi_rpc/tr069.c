/** @file
 * @brief Test API for RPC
 *
 * TAPI for remote calls of stdio routines
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
 * @author Edward Malarov <Edward.Makarov@oktetlabs.ru>
 *
 * $Id: tr069.c 43076 2007-09-24 06:48:00Z kam $
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

#include "tapi_rpc_internal.h"
#include "tapi_rpc_tr069.h"


#undef STR_EXECUTIVE
#undef STR

#undef RPC_CALL
#undef RPC_CALL_NO_REQ
#undef RPC_CALL_NO_RESP
#undef RPC_CALL_NO_REQ_NO_RESP

#define STR_EXECUTIVE(_fun) _fun

#define STR(_fun) STR_EXECUTIVE(_fun)

#define RPC_CALL(_fun, _actions_before_call,                             \
                 _actions_after_call, _actions_after_log)                \
    extern int rpc_##_fun(rcf_rpc_server *rpcs,                          \
                         _fun##_t *req,                                  \
                         _fun##_response_t *resp)                        \
    RPC_CALL_BODY(_fun, _actions_before_call,                            \
                  _actions_after_call, _actions_after_log,               \
                  rpcs != NULL && req != NULL && resp != NULL)

#define RPC_CALL_NO_REQ(_fun, _actions_before_call,                      \
                        _actions_after_call, _actions_after_log)         \
    extern int rpc_##_fun(rcf_rpc_server *rpcs, _fun##_response_t *resp) \
    RPC_CALL_BODY(_fun, _actions_before_call,                            \
                  _actions_after_call, _actions_after_log,               \
                  rpcs != NULL && resp != NULL)

#define RPC_CALL_NO_RESP(_fun, _actions_before_call,                     \
                         _actions_after_call, _actions_after_log)        \
    extern int rpc_##_fun(rcf_rpc_server *rpcs, _fun##_t *req)           \
    RPC_CALL_BODY(_fun, _actions_before_call,                            \
                  _actions_after_call, _actions_after_log,               \
                  rpcs != NULL && req != NULL)

#define RPC_CALL_NO_REQ_NO_RESP(_fun, _actions_before_call,              \
                                _actions_after_call, _actions_after_log) \
    extern int rpc_##_fun(rcf_rpc_server *rpcs)                          \
    RPC_CALL_BODY(_fun, _actions_before_call,                            \
                  _actions_after_call, _actions_after_log,               \
                  rpcs != NULL)

#define RPC_CALL_BODY(_fun, _actions_before_call, _actions_after_call,   \
                      _actions_after_log, _param_check_expression)       \
{                                                                        \
    size_t u;                                                            \
                                                                         \
    tarpc_##_fun##_in in;                                                \
    tarpc_##_fun##_out out;                                              \
                                                                         \
    memset(&in, 0, sizeof(in));                                          \
    memset(&out, 0, sizeof(out));                                        \
                                                                         \
    ERROR("RPC_API: rpc_" #_fun "()");                                   \
                                                                         \
    if (!(_param_check_expression))                     \
    {                                                                    \
        ERROR("%s(): Invalid RPC parameter", __FUNCTION__);              \
        RETVAL_INT(STR(_fun), -1);                                       \
    }                                                                    \
                                                                         \
    rpcs->op = RCF_RPC_CALL_WAIT;                                        \
                                                                         \
    { _actions_before_call }                                             \
                                                                         \
    rcf_rpc_call(rpcs, #_fun, &in, &out);           \
                                                                         \
    { _actions_after_call }                                              \
                                                                         \
    TAPI_RPC_LOG("RPC (%s,%s): " #_fun "() -> %d (%s)",                  \
                 rpcs->ta, rpcs->name,                                   \
                 out.retval, errno_rpc2str(RPC_ERRNO(rpcs)));            \
                                                                         \
    { _actions_after_log }                                               \
                                                                         \
    RETVAL_ZERO_INT(STR(_fun), out.retval);                              \
}


/* See description in tapi_rpc_tr069.h */
RPC_CALL_NO_REQ(cpe_get_rpc_methods,
{
    ERROR("out.method_list.method_list_val = %x, out.method_list.method_list_len = %u", out.method_list.method_list_val, out.method_list.method_list_len);
    for (u = 0; u < out.method_list.method_list_len; u++)
    {
        ERROR("out.method_list.method_list_val[%u] = '%s'", u, out.method_list.method_list_val[u]);
    }
},
{
    ERROR("out.method_list.method_list_val = %x, out.method_list.method_list_len = %u", out.method_list.method_list_val, out.method_list.method_list_len);
    for (u = 0; u < out.method_list.method_list_len; u++)
    {
        ERROR("out.method_list.method_list_val[%u] = '%s'", u, out.method_list.method_list_val[u]);
    }
},
{
    if (out.retval == 0)
    {
        resp->method_list = out.method_list.method_list_val;
        out.method_list.method_list_val = NULL;
        resp->n = out.method_list.method_list_len;
        out.method_list.method_list_len = 0;
    }
})

/* See description in tapi_rpc_tr069.h */
RPC_CALL(cpe_set_parameter_values, {}, {}, {})

/* See description in tapi_rpc_tr069.h */
RPC_CALL(cpe_get_parameter_values, {}, {}, {})

/* See description in tapi_rpc_tr069.h */
RPC_CALL(cpe_get_parameter_names, {}, {}, {})

/* See description in tapi_rpc_tr069.h */
RPC_CALL(cpe_set_parameter_attributes, {}, {}, {})

/* See description in tapi_rpc_tr069.h */
RPC_CALL(cpe_get_parameter_attributes, {}, {}, {})

/* See description in tapi_rpc_tr069.h */
RPC_CALL(cpe_add_object, {}, {}, {})

/* See description in tapi_rpc_tr069.h */
RPC_CALL(cpe_delete_object, {}, {}, {})

/* See description in tapi_rpc_tr069.h */
RPC_CALL(cpe_reboot, {}, {}, {})

/* See description in tapi_rpc_tr069.h */
RPC_CALL(cpe_download, {}, {}, {})

/* See description in tapi_rpc_tr069.h */
RPC_CALL(cpe_upload, {}, {}, {})

/* See description in tapi_rpc_tr069.h */
RPC_CALL(cpe_factory_reset, {}, {}, {})

/* See description in tapi_rpc_tr069.h */
RPC_CALL(cpe_get_queued_transfers, {}, {}, {})

/* See description in tapi_rpc_tr069.h */
RPC_CALL(cpe_get_all_queued_transfers, {}, {}, {})

/* See description in tapi_rpc_tr069.h */
RPC_CALL(cpe_schedule_inform, {}, {}, {})

/* See description in tapi_rpc_tr069.h */
RPC_CALL(cpe_set_vouchers, {}, {}, {})

/* See description in tapi_rpc_tr069.h */
RPC_CALL(cpe_get_options, {}, {}, {})

#undef STR_EXECUTIVE
#undef STR

#undef RPC_CALL
#undef RPC_CALL_NO_REQ
#undef RPC_CALL_NO_RESP
#undef RPC_CALL_NO_REQ_NO_RESP
