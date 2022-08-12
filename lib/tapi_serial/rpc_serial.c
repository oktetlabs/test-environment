/** @file
 * @brief Test API - RPC
 *
 * Implementation of TAPI for remote calls of serial console.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 *
 */
#include "te_defs.h"
#include "rcf_rpc.h"
#include "tarpc.h"
#include "tapi_rpc_internal.h"
#include "tapi_rpc_serial.h"

/* Default conserver port */
#define CONSERVER_DEFAULT_PORT  (3109)

/* Maximum length of buffer sent by rpc_serial_send_str() */
#define TAPI_SERIAL_STR_MAX_LEN (1024)
/* Maximum length of pattern */
#define MAX_PATTERN_LENGTH      (1024)

/* Verify that serial handle is initialized */
#define RPC_SERIAL_CHECK_HANDLE(_handle)                                    \
    do {                                                                    \
        if (_handle == NULL || _handle->sock <= 0)                          \
        {                                                                   \
            ERROR("Serial handle is not initialized");                      \
            return TE_EFAULT;                                               \
        }                                                                   \
    } while (0);

/* See description in tapi_rpc_serial.h */
int
rpc_serial_open(rcf_rpc_server *rpcs, tapi_serial_handle *p_handle,
                const char *user, const char *console,
                const struct sockaddr *address)
{
    tarpc_serial_open_in     in = { };
    tarpc_serial_open_out    out = { };
    char                     straddr[INET6_ADDRSTRLEN] = { 0 };
    struct sockaddr_storage  tmp;

    memset(straddr, 0, INET6_ADDRSTRLEN);
    in.user = (char *)user;
    in.console = (char *)console;
    if (address == NULL)
    {
        if (te_sockaddr_str2h("127.0.0.1", SA(&tmp)) != 0)
            RETVAL_INT(serial_open, -1);

        te_sockaddr_set_port(SA(&tmp), CONSERVER_DEFAULT_PORT);

        sockaddr_input_h2rpc(SA(&tmp), &in.sa);
    }
    else
    {
        if (address->sa_family != AF_INET && address->sa_family != AF_INET6)
        {
            ERROR("%s(): Protocol family %d is not supported", __FUNCTION__,
                  address->sa_family);
            RETVAL_INT(serial_open, -1);
        }

        sockaddr_input_h2rpc(address, &in.sa);
        tapi_sockaddr_clone_exact(address, &tmp);
    }

    RING("Using console \"%s\" and user \"%s\" for connecting to conserver",
         console, user);
    rcf_rpc_call(rpcs, "serial_open", &in, &out);
    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(serial_open, out.retval);

    if (RPC_IS_CALL_OK(rpcs) && rpcs->op != RCF_RPC_WAIT)
    {
        *p_handle = malloc(sizeof(tapi_serial));
        if (*p_handle == NULL)
            RETVAL_INT(serial_open, -1);

        (*p_handle)->sock = out.sock;
        (*p_handle)->rpcs = rpcs;
    }

    TAPI_RPC_LOG(rpcs, serial_open, "%s, %s, %s:%d", "%d",
                 in.user, in.console, te_sockaddr_get_ipstr(SA(&tmp)),
                 te_sockaddr_get_port(SA(&tmp)), out.sock);
    RETVAL_INT(serial_open, out.retval);
}

/* See description in tapi_rpc_serial.h */
int
rpc_serial_read(tapi_serial_handle handle, char *buffer, size_t *buflen,
                int timeout_ms)
{
    tarpc_serial_read_in  in = { };
    tarpc_serial_read_out out = { };
    rcf_rpc_server       *rpcs;

    RPC_SERIAL_CHECK_HANDLE(handle);
    rpcs = handle->rpcs;
    if (timeout_ms > 0)
        handle->rpcs->timeout += timeout_ms;

    in.buflen = *buflen;
    in.sock = handle->sock;
    in.timeout = timeout_ms;
    rcf_rpc_call(handle->rpcs, "serial_read", &in, &out);
    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(serial_read, out.retval);

    if (RPC_IS_CALL_OK(rpcs) && rpcs->op != RCF_RPC_WAIT)
    {
        *buflen = out.buflen;
        memcpy(buffer, out.buffer, out.buflen);
    }

    TAPI_RPC_LOG(handle->rpcs, serial_read, "length to read: %d",
                 "length read: %d", in.buflen, out.buflen);
    RETVAL_INT(serial_read, out.retval);
}

/* See description in tapi_rpc_serial.h */
int
rpc_serial_close(tapi_serial_handle handle)
{
    rcf_rpc_server        *rpcs;
    tarpc_serial_close_in  in = { };
    tarpc_serial_close_out out = { };

    RPC_SERIAL_CHECK_HANDLE(handle);
    rpcs = handle->rpcs;
    in.sock = handle->sock;
    rcf_rpc_call(rpcs, "serial_close", &in, &out);

    free(handle);
    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(serial_close, out.retval);
    TAPI_RPC_LOG(rpcs, serial_close, "%d", "%d",
                 in.sock, out.retval);
    RETVAL_INT(serial_close, out.retval);
}

/* See description in tapi_rpc_serial.h */
int
rpc_serial_force_rw(tapi_serial_handle handle)
{
    rcf_rpc_server           *rpcs;
    tarpc_serial_force_rw_in  in = { };
    tarpc_serial_force_rw_out out = { };

    RPC_SERIAL_CHECK_HANDLE(handle);
    rpcs = handle->rpcs;

    in.sock = handle->sock;
    rcf_rpc_call(rpcs, "serial_force_rw", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(serial_force_rw, out.retval);
    TAPI_RPC_LOG(rpcs, serial_force_rw, "%d", "%d",
                 in.sock, out.retval);
    RETVAL_INT(serial_force_rw, out.retval);
}

/* See description in tapi_rpc_serial.h */
int
rpc_serial_spy(tapi_serial_handle handle)
{
    rcf_rpc_server      *rpcs;
    tarpc_serial_spy_in  in = { };
    tarpc_serial_spy_out out = { };

    RPC_SERIAL_CHECK_HANDLE(handle);
    rpcs = handle->rpcs;

    in.sock = handle->sock;

    rcf_rpc_call(rpcs, "serial_spy", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(serial_spy, out.retval);
    TAPI_RPC_LOG(rpcs, serial_spy, "%d", "%d",
                 in.sock, out.retval);
    RETVAL_INT(serial_spy, out.retval);
}

/* See description in tapi_rpc_serial.h */
int
rpc_serial_send_str(tapi_serial_handle handle, const char *fmt, va_list vlist)
{
    rcf_rpc_server           *rpcs;
    tarpc_serial_send_str_in  in = { };
    tarpc_serial_send_str_out out = { };
    char                      str[TAPI_SERIAL_STR_MAX_LEN];
    int                       buflen;

    RPC_SERIAL_CHECK_HANDLE(handle);
    rpcs = handle->rpcs;
    buflen = vsnprintf(str, TAPI_SERIAL_STR_MAX_LEN, fmt, vlist);
    if (buflen < 0)
        RETVAL_INT(serial_send_str, -1);
    in.sock = handle->sock;
    in.str = str;
    in.buflen = buflen;

    rcf_rpc_call(rpcs, "serial_send_str", &in, &out);
    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(serial_send_str, out.retval);
    TAPI_RPC_LOG(rpcs, serial_send_str, "%d, %s, %u", "%d",
                 in.sock, in.str, (unsigned int)in.buflen, out.retval);
    RETVAL_INT(serial_send_str, out.retval);
}

/* See description in tapi_rpc_serial.h */
int
rpc_serial_send_enter(tapi_serial_handle handle)
{
    rcf_rpc_server             *rpcs;
    tarpc_serial_send_enter_in  in = { };
    tarpc_serial_send_enter_out out = { };

    RPC_SERIAL_CHECK_HANDLE(handle);
    rpcs = handle->rpcs;
    in.sock = handle->sock;

    rcf_rpc_call(rpcs, "serial_send_enter", &in, &out);
    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(serial_send_enter, out.retval);
    TAPI_RPC_LOG(rpcs, serial_send_enter, "%d", "%d",
                 in.sock, out.retval);
    RETVAL_INT(serial_send_enter, out.retval);
}

/* See description in tapi_rpc_serial.h */
int
rpc_serial_send_ctrl_c(tapi_serial_handle handle)
{
    rcf_rpc_server              *rpcs;
    tarpc_serial_send_ctrl_c_in  in = { };
    tarpc_serial_send_ctrl_c_out out = { };

    RPC_SERIAL_CHECK_HANDLE(handle);

    rpcs = handle->rpcs;
    in.sock = handle->sock;

    rcf_rpc_call(rpcs, "serial_send_ctrl_c", &in, &out);
    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(serial_send_ctrl_c, out.retval);
    TAPI_RPC_LOG(rpcs, serial_send_ctrl_c, "%d", "%d",
                 in.sock, out.retval);
    RETVAL_INT(serial_send_ctrl_c, out.retval);
}

/* See description in tapi_rpc_serial.h */
int
rpc_serial_flush(tapi_serial_handle handle, size_t amount)
{
    rcf_rpc_server        *rpcs;
    tarpc_serial_flush_in  in = { };
    tarpc_serial_flush_out out = { };

    RPC_SERIAL_CHECK_HANDLE(handle);
    rpcs = handle->rpcs;

    in.sock = handle->sock;
    in.amount = amount;

    rcf_rpc_call(rpcs, "serial_flush", &in, &out);
    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(serial_flush, out.retval);
    TAPI_RPC_LOG(rpcs, serial_flush, "%d, %u", "%d",
                 in.sock, (unsigned int)in.amount, out.retval);
    RETVAL_INT(serial_flush, out.retval);
}

/* See description in tapi_rpc_serial.h */
int
rpc_serial_check_pattern(tapi_serial_handle handle, int *offset,
                         const char *fmt, va_list vlist)
{
    tarpc_serial_check_pattern_in  in = { };
    tarpc_serial_check_pattern_out out = { };
    rcf_rpc_server *rpcs;
    char            pattern[MAX_PATTERN_LENGTH];
    int             length;

    RPC_SERIAL_CHECK_HANDLE(handle);
    rpcs = handle->rpcs;

    in.sock = handle->sock;
    length = vsnprintf(pattern, MAX_PATTERN_LENGTH, fmt, vlist);
    if (length < 0)
        RETVAL_INT(serial_check_pattern, -1);
    in.pattern_length = length;
    in.pattern = pattern;

    rcf_rpc_call(handle->rpcs, "serial_check_pattern", &in, &out);

    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(serial_check_pattern, out.retval);
    TAPI_RPC_LOG(handle->rpcs, serial_check_pattern, "%d, %u, %s", "%d, %d",
                 in.sock, (unsigned int)in.pattern_length, in.pattern,
                 out.offset, out.retval);
    if (offset != NULL)
        *offset = out.offset;
    RETVAL_INT(serial_check_pattern, out.retval);
}

/* See description in tapi_rpc_serial.h */
int
rpc_serial_wait_pattern(tapi_serial_handle handle, int *offset, int timeout_ms,
                        const char *fmt, va_list vlist)
{
    tarpc_serial_wait_pattern_in  in = { };
    tarpc_serial_wait_pattern_out out = { };
    rcf_rpc_server *rpcs;
    char            pattern[MAX_PATTERN_LENGTH];
    int             length;

    RPC_SERIAL_CHECK_HANDLE(handle);
    rpcs = handle->rpcs;

    if (timeout_ms > 0)
    {
        /*
         * Add some seconds to be sure that there would be enough time for
         * RPC taking rounding into account
         */
        handle->rpcs->timeout += timeout_ms + TE_SEC2MS(10);
    }

    in.sock = handle->sock;
    in.timeout = timeout_ms;
    length = vsnprintf(pattern, MAX_PATTERN_LENGTH, fmt, vlist);
    if (length < 0)
        RETVAL_INT(serial_wait_pattern, -1);
    in.pattern_length = length;
    in.pattern = pattern;

    rcf_rpc_call(handle->rpcs, "serial_wait_pattern", &in, &out);

    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(serial_wait_pattern, out.retval);
    TAPI_RPC_LOG(handle->rpcs, serial_wait_pattern, "%d, %d, %u, %s", "%d, %d",
                 in.sock, in.timeout, (unsigned int)in.pattern_length,
                 in.pattern, out.offset, out.retval);

    if (offset != NULL)
        *offset = out.offset;
    RETVAL_INT(serial_wait_pattern, out.retval);
}
