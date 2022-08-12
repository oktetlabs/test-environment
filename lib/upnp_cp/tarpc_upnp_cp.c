/** @file
 * @brief UPnP Control Point process
 *
 * UPnP Control Point proxy functions implementation.
 *
 *
 * Copyright (C) 2004-2018 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 * @author Ivan Melnikov <Ivan.Melnikov@oktetlabs.ru>
 *
 */

#ifndef TE_LGR_USER
#define TE_LGR_USER      "TARPC UPnP Control Point"
#endif

#include "te_config.h"

#include "te_defs.h"
#include "rpc_server.h"
#include "logger_api.h"
#include "te_dbuf.h"
#include "rcf_common.h"
#include "te_upnp.h"
#include "conf_upnp_cp.h"


/** File descriptor for a UNIX socket (client side). */
static int client = -1;


/**
 * Write data to the socket (send a request).
 *
 * @param fd    Socket descriptor.
 * @param buf   Buffer to send data from.
 * @param size  Buffer size.
 *
 * @return @c 0 on success and @c -1 on error.
 */
static int
send_request(int fd, const void *buf, size_t size)
{
    ssize_t     remaining = size;
    ssize_t     written;
    const char *data = buf;

    while (remaining > 0)
    {
        written = write(fd, data, remaining);
        if (written != remaining)
        {
            if (written >= 0)
            {
                if (written == 0)
                    VERB("Write error: nothing was written");
                else
                    VERB("Write error: partial write. "
                         "Written: %u/%u bytes.", written, remaining);
            }
            else /* if (wc == -1) */
            {
                ERROR("Write error: %s", strerror(errno));
                return -1;
            }
        }
        remaining -= written;
        data += written;
    }
    return 0;
}

/**
 * Read data from the socket (get a reply).
 *
 * @param[in]  fd    Socket descriptor.
 * @param[out] data  Buffer to write read data to. Note that buffer
 *                   allocated in this function.
 * @param[out] size  Buffer size.
 *
 * @return @c 0 on success and @c -1 on error.
 */
static int
get_reply(int fd, uint8_t **buf, size_t *size)
{
    static te_dbuf reply = TE_DBUF_INIT(0);
    static char    tmp_buf[128];
    ssize_t        rc;

    te_dbuf_reset(&reply);
    do {
        rc = read(fd, tmp_buf, sizeof(tmp_buf));
        if (rc > 0)
        {
            te_dbuf_append(&reply, tmp_buf, rc);
        }
        else
        {
            if (rc == 0)
                ERROR("Got EOF");
            else
                ERROR("Read error: %s", strerror(errno));
            te_dbuf_free(&reply);
            return -1;
        }
    } while (tmp_buf[rc - 1] != '\0'); /* Wait for null-terminated string */

    *buf = reply.ptr;
    *size = reply.len;
    reply.ptr = NULL;
    reply.len = reply.size = 0;
    return 0;
}


/**
 * Create the UNIX socket and perform connection with UPnP Control Point.
 *
 * @param in    TA RPC parameter containing no input value.
 *
 * @return On success, @c 0. On error, @c -1, and errno is set
 *         appropriately.
 */
static int
upnp_cp_connect(tarpc_upnp_cp_connect_in *in)
{
    UNUSED(in);
    struct sockaddr_un addr;
    int rc;

    if (client != -1)
    {
        errno = EISCONN;
        ERROR("Connection is already established: %s", strerror(errno));
        return -1;
    }
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, ta_unix_conf_upnp_cp_get_socket_name(),
            sizeof(addr.sun_path) - 1);
    client = socket(AF_UNIX, SOCK_STREAM, 0);
    if (client == -1)
    {
        ERROR("Socket error: %s", strerror(errno));
        return -1;
    }
    rc = connect(client, (struct sockaddr*)&addr, sizeof(addr));
    if (rc == -1)
    {
        ERROR("Connect error: %s", strerror(errno));
        if (close(client) == -1)
            ERROR("Close error: %s", strerror(errno));
        client = -1;
        return -1;
    }
    return 0;
}

/**
 * Perform disconnection from UPnP Control Point and destroy the UNIX
 * socket.
 *
 * @param in    TA RPC parameter containing no input value.
 *
 * @return On success, @c 0. On error, @c -1, and errno is set
 *         appropriately.
 */
static int
upnp_cp_disconnect(tarpc_upnp_cp_disconnect_in *in)
{
    UNUSED(in);
    int rc;

    if (client == -1)
    {
        errno = ENOTCONN;
        ERROR("Connection is not established yet: %s", strerror(errno));
        return -1;
    }
    rc = close(client);
    client = -1;
    if (rc == -1)
    {
        ERROR("Close error: %s", strerror(errno));
        return -1;
    }
    return 0;
}

/**
 * Transmit a message from TEN to UPnP Control Point over UNIX socket and
 * back.
 *
 * @param[in]  in       TA RPC parameter containing request.
 * @param[out] out      TA RPC parameter containing reply.
 *
 * @return @c 0, on success, and @c -1, on error.
 */
static int
upnp_cp_action(tarpc_upnp_cp_action_in *in, tarpc_upnp_cp_action_out *out)
{
    int rc;
#if UPNP_DEBUG > 1
    char msg_dbg[4 * 1024 - 32];
#endif /* UPNP_DEBUG */

#if UPNP_DEBUG > 1
    strncpy(msg_dbg, in->buf.buf_val, sizeof(msg_dbg) - 1);
    msg_dbg[sizeof(msg_dbg) - 1] = '\0';
    VERB("IN message [%u]: %s%s", in->buf.buf_len, msg_dbg,
         (in->buf.buf_len < sizeof(msg_dbg) ? "" : "\n..."));
#endif /* UPNP_DEBUG */

    rc = send_request(client, in->buf.buf_val, in->buf.buf_len);
    if (rc != 0)
    {
        ERROR("Fail to send request");
        return -1;
    }
    rc = get_reply(client, &out->buf.buf_val, (size_t* )&out->buf.buf_len);
    if (rc != 0)
    {
        ERROR("Fail to get reply");
        return -1;
    }

#if UPNP_DEBUG > 1
    strncpy(msg_dbg, out->buf.buf_val, sizeof(msg_dbg) - 1);
    msg_dbg[sizeof(msg_dbg) - 1] = '\0';
    VERB("OUT message [%u]: %s%s", out->buf.buf_len, msg_dbg,
         (out->buf.buf_len < sizeof(msg_dbg) ? "" : "\n..."));
#endif /* UPNP_DEBUG */

    return 0;
}


/*-------------- upnp_cp_connect() -------------------------*/
TARPC_FUNC_STATIC(upnp_cp_connect, {},
{
    MAKE_CALL(out->retval = func_ptr(in));
})

/*-------------- upnp_cp_disconnect() ----------------------*/
TARPC_FUNC_STATIC(upnp_cp_disconnect, {},
{
    MAKE_CALL(out->retval = func_ptr(in));
})

/*-------------- upnp_cp_action() --------------------------*/
TARPC_FUNC_STATIC(upnp_cp_action, {},
{
    MAKE_CALL(out->retval = func_ptr(in, out));
})
