/** @file 
 * @brief RCF Portable Command Handler
 *
 * RCF RPC server entry point. The file is included to another one
 * to create TA-builtin and standalone RPC servers.
 *
 * Copyright (C) 2006 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
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
 * @author Elena A. Vengerova <Elena.Vengerova@oktetlabs.ru>
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
 */

#include "ta_common.h"

#ifdef __CYGWIN__
#define TCP_TRANSPORT
#endif

/* 
 * MSG_MORE is used for performance reasons.
 * If it is not supported, it is not critical.
 */
#ifndef MSG_MORE
#define MSG_MORE    0
#endif

/** Sleep in waitable state */
extern void sleep_waitable(int msec);

#ifdef TCP_TRANSPORT

/**
 * Enable TCP_NODEALY on the socket. If TCP_NODELAY is not supported,
 * just return OK.
 *
 * @param sock      A socket
 *
 * @return errno or 0
 */
static int
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

        ERROR("Failed to enable TCP_NODELAY on RPC server socket: "
              "error=%r", err);
        return err;
    }
    return 0;
#else
    UNUSED(sock);
    return TE_RC(TE_RCF_PCH, TE_EOPNOTSUPP);
#endif
}
#endif /* TCP_TRANSPORT */
 
/** 
 * Receive data with specified timeout.
 *
 * @param s     socket
 * @param buf   buffer for reading
 * @param len   buffer length
 * @param t     timeout in seconds
 *
 * @return recv() return code or -2 if timeout expired
 */
static int
recv_timeout(int s, void *buf, int len, int t)
{
    int rc = -1;
    
    fd_set set;
    
#ifdef __CYGWIN__
    int i;
    
    for (i = 0; i < t && rc <= 0; i++)
    {
        struct timeval tv = { 0, 990000 };
        
        sleep_waitable(10);
        
        FD_ZERO(&set);
        FD_SET(s, &set);
    
        rc = select(s + 1, &set, NULL, NULL, &tv);
    }
    
    if (rc <= 0)
        return -2;
    
    return recv(s, buf, len, 0);

#else
    struct timeval tv = { t, 0 };
    
    while (rc < 0)
    {
        FD_ZERO(&set);
        FD_SET(s, &set);
    
        if ((rc = select(s + 1, &set, NULL, NULL, &tv)) == 0)
            return -2;
    }
    
    return recv(s, buf, len, 0);
#endif
}

#ifdef HAVE_SIGNAL_H
static void
sig_handler(int s)
{
    UNUSED(s);
    exit(1);
}
#endif


/**
 * Entry function for RPC server. 
 *
 * @param name   RPC server name
 *
 * @return NULL
 */
void *
rcf_pch_rpc_server(const char *name)
{
#ifdef TCP_TRANSPORT    
    struct sockaddr_in  addr;
#else    
    struct sockaddr_un  addr;
#endif    

    char *buf = NULL;
    int   s = -1;
    int   err;
    int   sock_len;

#define STOP(msg...)    \
    do {                \
        ERROR(msg);     \
        goto cleanup;   \
    } while (0)

#ifdef HAVE_SIGNAL_H
    signal(SIGTERM, sig_handler);
#endif

    if (logfork_register_user(name) != 0)
    {
        fprintf(stderr,
                "logfork_register_user() failed to register %s server\n",
                name);
    }
    
    if (getenv("TE_RPC_PORT") == NULL)
    {
        fprintf(stderr, "TE_RPC_PORT is not exported\n"); fflush(stderr);
        
        ERROR("RPC server %s: TE_RPC_PORT is not exported", name);
        
        return NULL;
    }

#ifndef __CYGWIN__
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
#endif    

    if ((buf = malloc(RCF_RPC_HUGE_BUF_LEN)) == NULL)
        STOP("Failed to allocate the buffer for RPC data");
    
    memset(&addr, 0, sizeof(addr));
#ifdef TCP_TRANSPORT    
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = atoi(getenv("TE_RPC_PORT"));
    sock_len = sizeof(struct sockaddr_in);
#else
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, getenv("TE_RPC_PORT"));
    sock_len = sizeof(struct sockaddr_un) - PIPENAME_LEN +  
               strlen(addr.sun_path);
#endif

#ifdef HAVE_STRUCT_SOCKADDR_SA_LEN
    ((struct sockaddr *)&addr)->sa_len = sock_len;
#endif

#ifdef TCP_TRANSPORT    
    s = socket(AF_INET, SOCK_STREAM, 0);
#else
    s = socket(AF_UNIX, SOCK_STREAM, 0);
#endif    
    if (s < 0)
    {
        ERROR("Failed to open socket");
        goto cleanup;
    }

    if (connect(s, (struct sockaddr *)&addr, sizeof(addr)) != 0)
    {
        err = TE_OS_RC(TE_RCF_PCH, errno);
        STOP("Failed to connect to TA; errno = %r", err);
    }
    /* Enable linger with positive timeout on the socket  */
    {
        struct linger l = { 1, 1 };

        if (setsockopt(s, SOL_SOCKET, SO_LINGER, (void *)&l, 
                       sizeof(l)) != 0)
        {
            err = TE_OS_RC(TE_RCF_PCH, errno);
            STOP("Failed to enable linger on RPC server socket; "
                 "errno = %r", err);
        }
    }

#ifdef TCP_TRANSPORT    
    /* TCP_NODEALY is not critical */
    (void)tcp_nodelay_enable(s);
#endif

    RING("RPC server '%s' (%s-bit) (re-)started (PID %d, TID %u)",
         name, sizeof(void *) == 8 ? "64" : "32",  
         (int)getpid(), thread_self());

    while (TRUE)
    {
        char      rpc_name[RCF_RPC_MAX_NAME];        
                                        /* RPC name, i.e. "bind" */
        void     *in = NULL;            /* Input parameter C structure */
        void     *out = NULL;           /* Output parameter C structure */
        rpc_info *info = NULL;          /* RPC information */
        te_bool   result = FALSE;       /* "rc" attribute */

        size_t    buflen;
        uint32_t  len, offset = 0;
        
        strcpy(rpc_name, "Unknown");
        
        /* 
         * We cannot call recv() here, but should call select() because
         * otherwise in the case of winsock2 the thread is not in
         * alertable state and async I/O callbacks are not called.
         */
        
        if (recv_timeout(s, &len, sizeof(len), 0xFFFF) <  (int)sizeof(len))
            STOP("recv() failed in RPC server main loop");
        
        if (strcmp((char *)&len, "FIN") == 0)
        {
            if (send(s, "OK", sizeof("OK"), 0) == (ssize_t)sizeof("OK"))
                RING("RPC server '%s' finished", name);
            else
                ERROR("RPC server '%s' failed to send 'OK' in response "
                      "to 'FIN'", name);
            goto cleanup;
        }
        
        if (len > RCF_RPC_HUGE_BUF_LEN)
            STOP("Too long RPC data bulk %u", len);
        
        while (offset < len)
        {
            int n = recv(s, buf + offset, RCF_RPC_HUGE_BUF_LEN - offset, 0);
            
            if (n <= 0)
                STOP("recv() returned %d in main RPC server loop", n);

            offset += n;
        }
        if (rpc_xdr_decode_call(buf, len, rpc_name, &in) != 0)
        {
            ERROR("Decoding of RPC %s call failed", rpc_name);
            goto result;
        }
        
        info = rpc_find_info(rpc_name);
        assert(info != NULL);
        
        if ((out = calloc(1, info->out_len)) == NULL)
        {
            ERROR("Memory allocation failure");
            goto result;
        }
        
        result = (info->rpc)(in, out, NULL);
        
    result: /* Send an answer */
        
        if (in != NULL && info != NULL)
            rpc_xdr_free(info->in, in);
        free(in);
            
        buflen = RCF_RPC_HUGE_BUF_LEN;
        if (rpc_xdr_encode_result(rpc_name, result, buf, &buflen, out) != 0)
            STOP("Fatal error: encoding of RPC %s output "
                 "parameters failed", rpc_name);
            
        if (info != NULL)
            rpc_xdr_free(info->out, out);
        free(out);
        
        len = buflen;
        if (send(s, (void *)&len, sizeof(len), MSG_MORE) < 
            (ssize_t)sizeof(len) || send(s, buf, len, 0) < (ssize_t)len)
        {
            STOP("send() failed in main RPC server loop");
        }
    }

cleanup:    
    free(buf);
    if (s >= 0)
#ifdef __CYGWIN__
        closesocket(s);
#else
        close(s);
#endif                
    
#undef STOP    

    return NULL;
}

