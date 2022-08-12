/** @file
 * @brief RCF RPC
 *
 * POSIX RPC transport (local or TCP sockets)
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 *
 *
 * @author Elena Vengerova <Elena.Vengerova@oktetlabs.ru>
 * @author Artem V. Andreev <Artem.Andreev@oktetlabs.ru>
 *
 */

#if (!defined(__CYGWIN__) && !defined(WINDOWS)) ||  \
    defined(ENABLE_TCP_TRANSPORT)
#include "te_config.h"

#define TE_LGR_USER     "RPC Transport"

#include "te_stdint.h"
#include "ta_common.h"
#include "te_errno.h"
#include "te_defs.h"
#include "rpc_transport.h"

/** Timeout for RPC operations */
#define RPC_TIMEOUT             10000   /* 10 seconds */

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_TIME_H
#include <time.h>
#endif
#if HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#if HAVE_FCNTL_H
#include <fcntl.h>
#endif
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#if HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif
#if HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

#if HAVE_NETINET_TCP_H
#include <netinet/tcp.h>
#endif

#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#if HAVE_SYS_UN_H
#include <sys/un.h>
#endif

/** Maximum length of the name in sockaddr_un */
#ifndef UNIX_PATH_MAX
#define UNIX_PATH_MAX   sizeof(((struct sockaddr_un *)0)->sun_path)
#endif

#include "logger_api.h"

/** rpc_transport_log auxiliary variables */
static char log[1024 * 16];
static char *log_ptr = log;

/**
 * A mechanism allowing to log messages without affecting any
 * network-related state. The log may be printed in the case
 * of RPC server deapth.
 */
void
rpc_transport_log(char *str)
{
    log_ptr += sprintf(log_ptr, "%s", str);
}

/** Listening socket */
static int lsock = -1;

/** Set for listening multiple sessions */
static fd_set rset;


/*
 * MSG_MORE is used for performance reasons.
 * If it is not supported, it is not critical.
 */
#ifndef MSG_MORE
#define MSG_MORE    0
#endif

#ifdef ENABLE_TCP_TRANSPORT

/**
 * Enable TCP_NODEALY on the socket, if possible.
 *
 * @param sock      A socket
 */
static void
tcp_nodelay_enable(int sock)
{
#ifdef TCP_NODELAY
    int nodelay = 1;

    if (setsockopt(sock,
#ifdef SOL_TCP
                   SOL_TCP,
#else
                   IPPROTO_TCP
#endif
                   TCP_NODELAY, (void *)&nodelay, sizeof(nodelay)) != 0)
    {
        int err = TE_OS_RC(TE_RCF_PCH, errno);

        WARN("Failed to enable TCP_NODELAY on RPC server socket: "
              "error=%r", err);
    }
#else
    UNUSED(sock);
#endif
}

#endif

/**
 * Initialize RPC transport.
 */
te_errno
rpc_transport_init(const char *tmp_path)
{
    char port[256];

#ifdef ENABLE_TCP_TRANSPORT
    struct sockaddr_in  addr;
#endif
#ifdef ENABLE_LOCAL_TRANSPORT
    struct sockaddr_un  addr;
#endif
   char addr_str[INET_ADDRSTRLEN];

    int len;

#define RETERR(msg...) \
    do {                                           \
        te_errno rc = TE_OS_RC(TE_RCF_PCH, errno); \
                                                   \
        ERROR(msg);                                \
        if (lsock >= 0)                            \
            close(lsock);                          \
        return rc;                                 \
    } while (0)

#ifdef ENABLE_TCP_TRANSPORT
    if ((lsock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        RETERR("Failed to open listening socket for RPC servers");
#endif

#ifdef ENABLE_LOCAL_TRANSPORT
    if ((lsock = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
        RETERR("Failed to open listening socket for RPC servers");
#endif

#if HAVE_FCNTL_H
    /*
     * Try to set close-on-exec flag, but ignore failures,
     * since it's not critical.
     */
    (void)fcntl(lsock, F_SETFD, FD_CLOEXEC);
#endif

    memset(&addr, 0, sizeof(addr));
#ifdef ENABLE_TCP_TRANSPORT
    addr.sin_family = AF_INET;
    len = sizeof(struct sockaddr_in);
#else
    addr.sun_family = AF_UNIX;
    snprintf(addr.sun_path, sizeof(addr.sun_path),
             "/%s/terpc_%lu", tmp_path, (unsigned long)getpid());
    len = sizeof(addr) - UNIX_PATH_MAX + strlen(addr.sun_path);
    unlink(addr.sun_path);
#endif

#ifdef HAVE_STRUCT_SOCKADDR_SA_LEN
    ((struct sockaddr *)&addr)->sa_len = len;
#endif

    if (bind(lsock, (struct sockaddr *)&addr, len) < 0)
        RETERR("Failed to bind RPC listening socket (%s): %s",
               (SA(&addr)->sa_family == AF_UNIX) ?
                   ((struct sockaddr_un *)&addr)->sun_path :
                   inet_ntop(AF_INET, &SIN(&addr)->sin_addr,
                             addr_str, sizeof(addr_str)),
                   strerror(errno));

    if (listen(lsock, 1) < 0)
        RETERR("listen() failed for RPC listening socket");

#ifdef ENABLE_TCP_TRANSPORT
    /* TCP_NODEALY is not critical */
    tcp_nodelay_enable(lsock);

    if (getsockname(lsock, (struct sockaddr *)&addr, &len) < 0)
        RETERR("getsockname() failed for listening socket for RPC servers");

    sprintf(port, "%d", addr.sin_port);
#else
    strcpy(port, addr.sun_path);
#endif

    if (setenv("TE_RPC_PORT", port, 1) < 0)
        RETERR("Failed to set TE_RPC_PORT environment variable");

    return 0;

#undef RETERR

}

/**
 * Shutdown RPC transport.
 */
void
rpc_transport_shutdown()
{
#ifdef ENABLE_LOCAL_TRANSPORT
    char *name = getenv("TE_RPC_PORT");

    if (name != NULL)
        unlink(name);
#endif
}

/**
 * Await connection from RPC server.
 *
 * @param name          name of the RPC server
 * @param p_handle      connection handle location
 *
 * @return Status code.
 */
te_errno
rpc_transport_connect_rpcserver(const char *name,
                                rpc_transport_handle *p_handle)
{
    struct timeval  tv = { RPC_TIMEOUT / 1000, 0 };
    fd_set          set;
    int             sock;
    int             rc;

#define RETERR(msg...) \
    do {                                           \
        te_errno rc = TE_OS_RC(TE_RCF_PCH, errno); \
                                                   \
        ERROR(msg);                                \
        return rc;                                 \
    } while (0)

    UNUSED(name);

    FD_ZERO(&set);
    FD_SET(lsock, &set);

    while ((rc = select(lsock + 1, &set, NULL, NULL, &tv)) <= 0)
    {
        if (rc == 0)
        {
            ERROR("RPC server '%s' does not try to connect", name);
            return TE_RC(TE_RCF_PCH, TE_ETIMEDOUT);
        }
        else if (errno != EINTR)
        {
            RETERR("select() failed with unexpected errno %d", errno);
        }
    }

    if ((sock = accept(lsock, NULL, NULL)) < 0)
        RETERR("Failed to accept connection from RPC server %s", name);

#if HAVE_FCNTL_H
    /*
     * Try to set close-on-exec flag, but ignore failures,
     * since it's not critical.
     */
    (void)fcntl(sock, F_SETFD, FD_CLOEXEC);
#endif

    *p_handle = (rpc_transport_handle)sock;

    return 0;

#undef RETERR
}

/**
 * Connect from RPC server to Test Agent
 *
 * @param name  name of the RPC server
 * @param p_handle      connection handle location
 *
 * @return Status code.
 */
te_errno
rpc_transport_connect_ta(const char *name, rpc_transport_handle *p_handle)
{
    char *port = getenv("TE_RPC_PORT");

    if (port == NULL)
    {
        ERROR("TE_RPC_PORT is not exported\n");

        return TE_RC(TE_RCF_PCH, TE_EINVAL);
    }

#define RETERR(msg...) \
    do {                                           \
        te_errno rc = TE_OS_RC(TE_RCF_PCH, errno); \
                                                   \
        ERROR(msg);                                \
        if (s >= 0)                                \
            close(s);                              \
        return rc;                                 \
    } while (0)

#ifdef ENABLE_TCP_TRANSPORT
    struct sockaddr_in  addr;
#else
    struct sockaddr_un  addr;
#endif

    int   s = -1;
    int   sock_len;

    UNUSED(name);

    memset(&addr, 0, sizeof(addr));
#ifdef ENABLE_TCP_TRANSPORT
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = atoi(port);
    sock_len = sizeof(struct sockaddr_in);
#else
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, port);
    sock_len = sizeof(addr) - UNIX_PATH_MAX + strlen(addr.sun_path);
#endif

#ifdef HAVE_STRUCT_SOCKADDR_SA_LEN
    ((struct sockaddr *)&addr)->sa_len = sock_len;
#endif

#ifdef ENABLE_TCP_TRANSPORT
    s = socket(AF_INET, SOCK_STREAM, 0);
#else
    s = socket(AF_UNIX, SOCK_STREAM, 0);
#endif
    if (s < 0)
    {
        perror("socket");
        RETERR("%s: failed to open socket", __FUNCTION__);
    }

    if (connect(s, (struct sockaddr *)&addr, sock_len) != 0)
    {
        perror("connect");
        RETERR("Failed to connect to TA", __FUNCTION__);
    }

    /* Enable linger with positive timeout on the socket  */
    {
        struct linger l = { 1, 1 };

        if (setsockopt(s, SOL_SOCKET, SO_LINGER, (void *)&l,
                       sizeof(l)) != 0)
        {
            RETERR("Failed to enable linger on RPC server socket");
        }
    }

#ifdef ENABLE_TCP_TRANSPORT
    tcp_nodelay_enable(s);
#endif

    fcntl(s, F_SETFD, FD_CLOEXEC);

    *p_handle = (rpc_transport_handle)s;

    return 0;
}

/**
 * Break the connection.
 *
 * @param handle      connection handle
 */
void
rpc_transport_close(rpc_transport_handle handle)
{
    if ((int)handle > 0 && close((int)handle) < 0)
        ERROR("close() for RPC transport socket failed with errno", errno);
}

/**
 * Reset set of descriptors to wait.
 */
void
rpc_transport_read_set_init()
{
    FD_ZERO(&rset);
}

/**
 * Add the handle to the read set.
 *
 * @param handle        connection handle
 */
void
rpc_transport_read_set_add(rpc_transport_handle handle)
{
    FD_SET((int)handle, &rset);
}

/**
 * Wait for the read event.
 *
 * @param timeout       timeout in seconds
 *
 * @return TRUE is the read event is received or FALSE otherwise
 */
te_bool
rpc_transport_read_set_wait(int timeout)
{
    struct timeval tv;

    tv.tv_sec = timeout;
    tv.tv_usec = 0;

    if (select(FD_SETSIZE, &rset, NULL, NULL, &tv) < 0)
    {
        if (errno != EINTR)
            return FALSE;
    }

    return TRUE;
}

/**
 * Check if data are pending on the connection.
 *
 * @param handle        connection handle
 *
 * @return TRUE is data are pending or FALSE otherwise
 */
te_bool
rpc_transport_is_readable(rpc_transport_handle handle)
{
    return FD_ISSET((int)handle, &rset);
}

/** Recieve exact number of bytes from the stream */
static te_errno
recv_from_stream(int handle, uint8_t *buf, size_t len, int timeout)
{
    size_t rcvd = 0;

    while (rcvd < len)
    {
        fd_set set;
        int    rc;

        struct timeval tv = { timeout, 0 };

        FD_ZERO(&set);
        FD_SET(handle, &set);

        rc = select(handle + 1, &set, NULL, NULL, &tv);

        if (rc == 0)
        {
            return TE_RC(TE_RCF_PCH, TE_ETIMEDOUT);
        }
        else if (rc < 0)
        {
            if (errno == EINTR)
                continue;
            return TE_OS_RC(TE_RCF_PCH, errno);
        }

        rc = recv(handle, buf + rcvd, len - rcvd, 0);

        if (rc <= 0)
            return TE_RC(TE_RCF_PCH, TE_ECONNRESET);

        rcvd += rc;
    }

    return 0;
}

/**
 * Receive message with specified timeout.
 *
 * @param handle   connection handle
 * @param buf      buffer for reading
 * @param p_len    IN: buffer length; OUT: length of received message
 * @param timeout  timeout in seconds
 *
 * @return Status code.
 * @retval TE_ETIMEDOUT         Timeout ocurred
 * @retval TE_ECONNRESET        Connection is broken
 * @retval TE_ENOMEM            Message is too long
 */
te_errno
rpc_transport_recv(rpc_transport_handle handle, uint8_t *buf,
                   size_t *p_len, int timeout)
{
    uint8_t  lenbuf[4];
    size_t   len;
    te_errno rc;

    if ((rc = recv_from_stream((int)handle, lenbuf,
                               sizeof(lenbuf), timeout)) != 0)
    {
        return rc;
    }

    len = ((int)lenbuf[0] << 24) + ((int)lenbuf[1] << 16) +
          ((int)lenbuf[2] << 8) + (int)lenbuf[3];

    if (len > *p_len)
        return TE_RC(TE_RCF_PCH, TE_ENOMEM);

    if (timeout == 0)
        timeout = RPC_TIMEOUT / 1000;

    if ((rc = recv_from_stream((int)handle, buf, len, timeout)) != 0)
        return TE_RC(TE_RCF_PCH, TE_ECONNRESET);

    *p_len = len;

    return 0;
}

/**
 * Send message.
 *
 * @param handle   connection handle
 * @param buf      buffer for writing
 * @param len      message length
 *
 * @return Status code.
 * @retval TE_ECONNRESET        Connection is broken
 */
te_errno
rpc_transport_send(rpc_transport_handle handle, const uint8_t *buf,
                   size_t len)
{
    uint8_t lenbuf[4];

    lenbuf[0] = (uint8_t)(len >> 24);
    lenbuf[1] = (uint8_t)((len >> 16) & 0xFF);
    lenbuf[2] = (uint8_t)((len >> 8) & 0xFF);
    lenbuf[3] = (uint8_t)(len & 0xFF);

    if (send((int)handle, lenbuf, sizeof(lenbuf), 0) != sizeof(lenbuf) ||
        send((int)handle, buf, len, 0) != (int)len)
    {
        return TE_RC(TE_RCF_PCH, TE_ECONNRESET);
    }

    return 0;
}

#endif /* !CYGWIN && !WINDOWS */
