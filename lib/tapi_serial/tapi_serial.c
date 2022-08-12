/** @file
 * @brief DUT serial console TAPI
 *
 * Implementation of API for communicating with DUT via serial console.
 *
 * Copyright (C) 2018-2022 OKTET Labs. All rights reserved.
 *
 *
 */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#include "te_defs.h"
#include "te_errno.h"

#include "tapi_mem.h"
#include "tapi_serial.h"
#include "te_errno.h"
#include "te_sockaddr.h"
#include "rcf_rpc.h"
#include "logger_api.h"
#include "tapi_cfg.h"
#include "tapi_rpc_serial.h"
#include "conf_api.h"
#include "conf_oid.h"

/* A wrapper to return an error code corresponding to errno */
#define TAPI_SERIAL_RETURN_CODE(_rpcs, _action)                             \
    do {                                                                    \
        int __retval;                                                       \
        __retval = (_action);                                               \
        if (__retval == -1)                                                 \
            return TE_OS_RC(TE_TA_UNIX, RPC_ERRNO(_rpcs));                  \
        return 0;                                                           \
    } while (0);

/* See description in tapi_serial.h */
te_errno
tapi_serial_open_rpcs(rcf_rpc_server *rpcs, const char *console_name,
                      tapi_serial_handle *p_handle)
{
    te_errno         rc;
    char            *console = NULL;
    char            *user = NULL;
    struct sockaddr *address;
    int              retval;

    rc = cfg_get_instance_string_fmt(&console, "/agent:%s/console:%s",
                                     rpcs->ta, console_name);
    CHECK_NZ_RETURN(rc);

    rc = cfg_get_instance_string_fmt(&user, "/agent:%s/console:%s/user:",
                                     rpcs->ta, console_name);
    if (rc != 0)
        user = tapi_strdup(TAPI_SERIAL_DEFAULT_USER);

    rc = cfg_get_instance_addr_fmt(&address, "/agent:%s/console:%s/address:",
                                   rpcs->ta, console_name);
    if (rc != 0 || te_sockaddr_is_wildcard(address))
        address = NULL;

    retval = rpc_serial_open(rpcs, p_handle, user, console, address);

    free(console);
    free(user);
    free(address);
    if (retval == -1)
        return TE_OS_RC(TE_TA_UNIX, RPC_ERRNO(rpcs));

    return 0;
}

/* See description in tapi_serial.h */
te_errno
tapi_serial_read(tapi_serial_handle handle, char *buf, size_t *buflen,
                 int timeout)
{
    TAPI_SERIAL_RETURN_CODE(handle->rpcs,
                            rpc_serial_read(handle, buf, buflen, timeout));

}

/* See description in tapi_serial.h */
te_errno
tapi_serial_close(tapi_serial_handle handle)
{
    rcf_rpc_server *rpcs = handle->rpcs;

    TAPI_SERIAL_RETURN_CODE(rpcs, rpc_serial_close(handle));
}

/* See description in tapi_serial.h */
te_errno
tapi_serial_force_rw(tapi_serial_handle handle)
{
    TAPI_SERIAL_RETURN_CODE(handle->rpcs, rpc_serial_force_rw(handle));
}

/* See description in tapi_serial.h */
te_errno
tapi_serial_spy(tapi_serial_handle handle)
{
    TAPI_SERIAL_RETURN_CODE(handle->rpcs, rpc_serial_spy(handle));
}

/* See description in tapi_serial.h */
te_errno
tapi_serial_send_str(tapi_serial_handle handle,
                     const char *fmt, ...)
{
    int     retval;
    va_list vlist;

    va_start(vlist, fmt);
    retval = rpc_serial_send_str(handle, fmt, vlist);
    va_end(vlist);

    TAPI_SERIAL_RETURN_CODE(handle->rpcs, retval);
}

/* See description in tapi_serial.h */
te_errno
tapi_serial_send_cmd(tapi_serial_handle handle,
                     const char *fmt, ...)
{
    int         retval;
    va_list     vlist;
    int         fmt_length = strlen(fmt);
    char       *fmt_with_end = malloc(fmt_length + 2);

    snprintf(fmt_with_end, fmt_length, "%s\n", fmt);

    va_start(vlist, fmt);
    retval = rpc_serial_send_str(handle, fmt_with_end, vlist);
    va_end(vlist);
    free(fmt_with_end);

    TAPI_SERIAL_RETURN_CODE(handle->rpcs, retval);
}

/* See description in tapi_serial.h */
te_errno
tapi_serial_send_enter(tapi_serial_handle handle)
{
    TAPI_SERIAL_RETURN_CODE(handle->rpcs, rpc_serial_send_enter(handle));
}

/* See description in tapi_serial.h */
te_errno
tapi_serial_send_ctrl_c(tapi_serial_handle handle)
{
    TAPI_SERIAL_RETURN_CODE(handle->rpcs, rpc_serial_send_ctrl_c(handle));
}

/* See description in tapi_serial.h */
te_errno
tapi_serial_flush(tapi_serial_handle handle, size_t amount)
{
    TAPI_SERIAL_RETURN_CODE(handle->rpcs, rpc_serial_flush(handle, amount));
}

/* See description in tapi_serial.h */
te_errno
tapi_serial_check_pattern(tapi_serial_handle handle, int *offset,
                          const char *fmt, ...)
{
    int     retval;
    va_list vlist;

    va_start(vlist, fmt);
    retval = rpc_serial_check_pattern(handle, offset, fmt, vlist);
    va_end(vlist);

    TAPI_SERIAL_RETURN_CODE(handle->rpcs, retval);
}

/* See description in tapi_serial.h */
te_errno
tapi_serial_wait_pattern(tapi_serial_handle handle, int *offset, int timeout,
                         const char *fmt, ...)
{
    int     retval;
    va_list vlist;

    va_start(vlist, fmt);
    retval = rpc_serial_wait_pattern(handle, offset, timeout, fmt, vlist);
    va_end(vlist);

    TAPI_SERIAL_RETURN_CODE(handle->rpcs, retval);
}
