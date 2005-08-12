/** @file
 * @brief IPC library
 *
 * Implementation of routines (server side) provided for library user.
 *
 *
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
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
 * @author Andrey Ivanov <andron@oktet.ru>
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
 */

#include "te_config.h"

#include <stdio.h>
#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#endif
#if HAVE_ERRNO_H
#include <errno.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_ASSERT_H
#include <assert.h>
#endif
#if HAVE_FCNTL_H
#include <fcntl.h>
#endif
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#if HAVE_SYS_QUEUE_H
#include <sys/queue.h>
#endif

#ifndef TE_IPC_AF_UNIX
#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#if HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#if HAVE_NETDB_H
#include <netdb.h>
#endif
#endif

#include "te_defs.h"
#include "te_errno.h"
#include "ipc_server.h"

#include "ipc_internal.h"


/**
 * The ipc_server_client structure is used to store information about
 * sender of the message. This structure used in both UDP/UNIX and 
 * TCP/IP implementations of IPC, but has different fields.
 */
struct ipc_server_client {
    /** Links to neighbour clients in the list */
    LIST_ENTRY(ipc_server_client)   links;

#ifdef TE_IPC_AF_UNIX
    struct sockaddr_un  sa;         /**< Address of the sender */
    socklen_t           sa_len;     /**< Length of the sockaddr_un struct */
#endif

#ifdef TE_IPC_CONNECTIONLESS
    void               *buffer;     /**< Buffer for datagram */
    size_t              frag_size;  /**< Size of current fragment */
    size_t              frag_rest;  /**< Number of octets in the current
                                         fragment not passed to the user */
    size_t              msg_len;    /**< Length of the currently receiving
                                         message, 0 if none */
    size_t              msg_rest;   /**< Length of the rest fragment of
                                         the current message */
#else
    int         socket;         /**< Connected socket to client */
    te_bool     is_ready;       /**< Is client socket ready for reading? */
    uint32_t    pending;        /**< Number of octets in current message
                                     left to read from socket and to
                                     return to user. This field MUST
                                     be 4-octets long. */
#endif
};


/** Structure to store state information about the server */
struct ipc_server {
    char    name[UNIX_PATH_MAX];    /**< Name of the server */
    int     socket;                 /**< Server socket */
    te_bool is_ready;               /**< Is server socket ready? */

    /** List of "active" IPC clients */
    LIST_HEAD(ipc_server_clients, ipc_server_client) clients;

#ifndef TE_IPC_AF_UNIX
    uint16_t    port;           /**< Port number used for this server */
#endif
#ifdef TE_IPC_CONNECTIONLESS
    char                   *buffer;     /**< Used to avoid data copying
                                             on receiving datagrams */
    struct ipc_datagrams    datagrams;  /**< Delayed datagrams */
#else
    char        *out_buffer;    /**< Buffer for outgoing messages */
#endif
};


/*
 * Static functions declaration.
 */

#ifndef TE_IPC_AF_UNIX

static int ipc_pmap_register_server(const char *server_name,
                                    uint16_t port);
static int ipc_pmap_unregister_server(const char *server_name,
                                      uint16_t port);

#endif /* !TE_IPC_AF_UNIX */


#ifdef TE_IPC_CONNECTIONLESS

#ifdef TE_IPC_AF_UNIX
static struct ipc_server_client * ipc_int_client_by_addr(
                                      struct ipc_server *ipcs,
                                      struct sockaddr_un *sa_ptr,
                                      socklen_t sa_len);
#endif

static int ipc_int_get_datagram_from_pool(struct ipc_server *ipcs,
                                          struct ipc_server_client
                                              **p_ipcsc);

static int ipc_int_get_datagram(struct ipc_server *ipcs,
                                struct ipc_server_client **p_ipcsc);

#else /* !TE_IPC_CONNECTIONLESS */

static int read_socket(int socket, void *buffer, size_t len);
static int write_socket(int socket, const void *buffer, size_t len);

#endif /* !TE_IPC_CONNECTIONLESS */


/*
 * Global functions implementation
 */

/* See description in ipc_server.h */
int
ipc_register_server(const char *name, struct ipc_server **p_ipcs)
{
    struct ipc_server *ipcs;
    int                 rc;

    if ((name == NULL) || (p_ipcs == NULL))
    {
        if (p_ipcs != NULL)
            *p_ipcs = NULL;
        return TE_RC(TE_IPC, TE_EINVAL);
    }
    *p_ipcs = NULL;

    if (strlen(name) >= (UNIX_PATH_MAX - 1))
    {
        return TE_RC(TE_IPC, TE_E2BIG);
    }

    ipcs = calloc(1, sizeof(*ipcs));
    if (ipcs == NULL)
    {
        rc = errno;
        perror("ipc_register_server(): calloc() error");
        return TE_OS_RC(TE_IPC, rc);
    }

    strcpy(ipcs->name, name);
    LIST_INIT(&ipcs->clients);

    /* Create a socket */
    ipcs->socket = socket(
#ifdef TE_IPC_AF_UNIX
                          PF_UNIX,
#else
                          PF_INET,
#endif
#ifdef TE_IPC_CONNECTIONLESS
                          SOCK_DGRAM,
#else
                          SOCK_STREAM,
#endif
                          0);
    if (ipcs->socket < 0)
    {
        rc = errno;
        perror("ipc_register_server(): socket() error");
        free(ipcs);
        return TE_OS_RC(TE_IPC, rc);
    }

#ifdef TE_IPC_AF_UNIX
    {
        struct sockaddr_un  sa;
        
        /* Assign a name to socket */
        sa.sun_family = AF_UNIX;
        memset(sa.sun_path, 0, UNIX_PATH_MAX);
        /* Sockets in the abstract namespace are started from '\0' */
        strcpy(sa.sun_path + 1, name);

        if (bind(ipcs->socket, SA(&sa), sizeof(sa)) != 0)
        {
            rc = errno;
            fprintf(stderr, "Failed to register IPC server '%s': %s\n",
                            SUN_NAME(&sa), strerror(rc));
            close(ipcs->socket);
            free(ipcs);
            return TE_OS_RC(TE_IPC, rc);
        }
    }
#endif /* TE_IPC_AF_UNIX */

#ifndef TE_IPC_CONNECTIONLESS
    if (listen(ipcs->socket, SOMAXCONN) != 0)
    {
        rc = errno;
        perror("listen() error");
        close(ipcs->socket);
        free(ipcs);
        return TE_OS_RC(TE_IPC, rc);
    }
#endif /* !TE_IPC_CONNECTIONLESS */

#ifndef TE_IPC_AF_UNIX
    {
        struct sockaddr_in  addr;
        int                 addr_len = sizeof(addr);

        if (getsockname(ipcs->socket, (struct sockaddr*)&addr,
                        &addr_len) != 0)
        {
            rc = errno;
            perror("getsockname() error");
            close(ipcs->socket);
            free(ipcs);
            return TE_OS_RC(TE_IPC, rc);
        }
        if (ipc_pmap_register_server(name, addr.sin_port) != 0)
        {
            rc = errno;
            perror("Cannot register server's port");
            close(ipcs->socket);
            free(ipcs);
            return TE_OS_RC(TE_IPC, rc);
        }
    }
#endif /* !TE_IPC_AF_UNIX */

#if HAVE_FCNTL_H
    /* 
     * Try to set close-on-exec flag, but ignore failures, 
     * since it's not critical.
     */
    (void)fcntl(ipcs->socket, F_SETFD, FD_CLOEXEC);
#endif
    
#ifdef TE_IPC_CONNECTIONLESS

    TAILQ_INIT(&ipcs->datagrams);

    ipcs->buffer = calloc(1, IPC_SEGMENT_SIZE);
    if (ipcs->buffer == NULL)
    {
        rc = errno;
        perror("ipc_register_server(): calloc() error");
        close(ipcs->socket);
        free(ipcs);
        return TE_OS_RC(TE_IPC, rc);
    }

#else /* !TE_IPC_CONNECTIONLESS */
    
    if (IPC_TCP_SERVER_BUFFER_SIZE != 0)
    {
        ipcs->out_buffer = calloc(1, IPC_TCP_SERVER_BUFFER_SIZE);
        if (ipcs->out_buffer == NULL)
        {
            rc = errno;
            perror("ipc_register_server(): calloc() error");
            close(ipcs->socket);
            free(ipcs);
            return TE_OS_RC(TE_IPC, rc);
        }
    }

#endif /* !TE_IPC_AF_UNIX */

    *p_ipcs = ipcs;

    return 0;
}


/* See description in ipc_server.h */
int
ipc_get_server_fd(const struct ipc_server *ipcs)
{
    return (ipcs != NULL) ? ipcs->socket : -1;
}

/* See description in ipc_server.h */
int
ipc_get_server_fds(const struct ipc_server *ipcs, fd_set *set)
{
    int                             max_fd;
#ifndef TE_IPC_CONNECTIONLESS
    const struct ipc_server_client *client;
#endif

    if (ipcs == NULL)
        return -1;

    FD_SET(ipcs->socket, set);
    max_fd = ipcs->socket;
#ifndef TE_IPC_CONNECTIONLESS
    for (client = ipcs->clients.lh_first;
         client != NULL;
         client = client->links.le_next)
    {
        FD_SET(client->socket, set);
        max_fd = MAX(max_fd, client->socket);
    }
#endif
        
    return max_fd;
}

/* See description in ipc_server.h */
te_bool
ipc_is_server_ready(struct ipc_server *ipcs, const fd_set *set, int max_fd)
{
    te_bool                     is_ready = FALSE;
#ifndef TE_IPC_CONNECTIONLESS
    struct ipc_server_client   *client;
#endif

    if (ipcs == NULL || set == NULL)
        return is_ready;

    if (ipcs->socket <= max_fd)
    {
        is_ready = is_ready ||
            (ipcs->is_ready = FD_ISSET(ipcs->socket, set));
    }
#ifndef TE_IPC_CONNECTIONLESS
    for (client = ipcs->clients.lh_first;
         client != NULL;
         client = client->links.le_next)
    {
        if (client->socket <= max_fd)
        {
            is_ready = is_ready ||
                (client->is_ready = FD_ISSET(client->socket, set));
        }
    }
#endif
        
    return is_ready;
}

/**
 * Close IPC server association with client.
 *
 * @param ipcsc     IPC server client
 */
static void
ipc_server_close_client(struct ipc_server_client *ipcsc)
{
    LIST_REMOVE(ipcsc, links);
#ifdef TE_IPC_CONNECTIONLESS
    free(ipcsc->buffer);
#else
    close(ipcsc->socket);
#endif
    free(ipcsc);
}

/* See description in ipc_server.h */
int
ipc_close_server(struct ipc_server *ipcs)
{
    struct ipc_server_client *ipcsc;
#ifdef TE_IPC_CONNECTIONLESS
    struct ipc_datagram      *p;
#endif

    if (ipcs == NULL)
        return 0;

#ifndef TE_IPC_AF_UNIX
    if (ipc_pmap_unregister_server(ipcs->name, ipcs->port) != 0)
    {
        perror("Cannot unregister server");
        return TE_OS_RC(TE_IPC, errno);
    }
#endif

    if (close(ipcs->socket) != 0)
        fprintf(stderr, "close() failed\n");

#ifdef TE_IPC_CONNECTIONLESS
    if (ipcs->datagrams.tqh_first != NULL)
        fprintf(stderr, "IPC server: drop some datagrams\n");
    /* Free datagram's buffer */
    while ((p = ipcs->datagrams.tqh_first) != NULL)
    {
        TAILQ_REMOVE(&ipcs->datagrams, p, links);
        free(p->buffer);
        free(p);
    }
    free(ipcs->buffer);
#else
    free(ipcs->out_buffer);
#endif

    /* Free the pool */
    while ((ipcsc = ipcs->clients.lh_first) != NULL)
    {
        ipc_server_close_client(ipcsc);
    }

    /* Free instance */
    free(ipcs);

    return 0;
}

/* See description in ipc_server.h */
const char *
ipc_server_client_name(const struct ipc_server_client *ipcsc)
{
    if (ipcsc == NULL)
        return NULL;

#ifdef TE_IPC_AF_UNIX
    return (ipcsc->sa.sun_path[0] != '\0') ? ipcsc->sa.sun_path :
               (ipcsc->sa.sun_path + 1);
#else
    return "UNKNOWN - TODO";
#endif
}


#ifdef TE_IPC_CONNECTIONLESS

/* See description in ipc_server.h */
int
ipc_receive_message(struct ipc_server *ipcs,
                    void *buf, size_t *p_buf_len,
                    struct ipc_server_client **p_ipcsc)
{
    struct ipc_server_client *client;

    int     rc;
    size_t  buf_len;              /* Size of buffer provided by caller */
    size_t  data_size;            /* Size of data in current fragment */
    void   *data;                 /* Pointer to the data to be copied */
    size_t  copy_len;             /* Size of data to be copied */


    if ((ipcs == NULL) || (buf == NULL) || (p_ipcsc == NULL) ||
        (p_buf_len == NULL))
    {
        return TE_RC(TE_IPC, TE_EINVAL);
    }
    client = *p_ipcsc;
    buf_len = *p_buf_len;

    KTRC("*p_buf_len=%u *p_ipcsc=%p\n", *p_buf_len, *p_ipcsc);
    do {
        if ((client == NULL) || (client->frag_rest == 0))
        {
            struct ipc_dgram_header   *ipch;

            KTRC("Get datagram\n");
            /*
             * Get datagram for any client from already received list or
             * wait for network.
             */
            rc = ipc_int_get_datagram(ipcs, &client);
            if (rc != 0)
            {
                return TE_RC(TE_IPC, rc);
            }

            ipch = (struct ipc_dgram_header*)client->buffer;

            KTRC("Datagram frag=%u len=%u left=%u\n",
                 client->frag_size, ipch->length, ipch->left);

            data_size = client->frag_size -
                            sizeof(struct ipc_dgram_header);
            if (data_size > ipch->left)
            {
                /* 
                 * Size of data in the current fragment is greater than
                 * declared total size of the message.
                 */
                fprintf(stderr, "%s(): Invalid IPC datagram\n",
                        __FUNCTION__);
                return TE_RC(TE_IPC, TE_ESYNCFAILED);
            }

            data = (uint8_t *)client->buffer +
                   sizeof(struct ipc_dgram_header);

            if (*p_ipcsc == NULL)
            {
                KTRC("New datagram for client %s\n",
                     SUN_NAME(&client->sa));
                *p_ipcsc = client;
                /* It must be the first fragment of the message */
                if (client->msg_len != 0)
                {
                    fprintf(stderr, "%s(): IPC internal error\n",
                            __FUNCTION__);
                    return TE_RC(TE_IPC, TE_ESYNCFAILED);
                }
                client->msg_len = client->msg_rest = ipch->length;
            }
            else
            {
                KTRC("Continue with the client %s: msg_len=%u "
                     "msg_rest=%u\n", SUN_NAME(&client->sa),
                     client->msg_len, client->msg_rest);
                if (client->msg_len == 0)
                {
                    /* New message for the client */
                    client->msg_len = client->msg_rest = ipch->length;
                }
                else if (client->msg_len != ipch->length)
                {
                    fprintf(stderr, "%s(): IPC protocol error:\n"
                            "Unexpected total message length in the "
                            "datagram\n", __FUNCTION__);
                    return TE_RC(TE_IPC, TE_ESYNCFAILED);
                }
            }
            if (client->msg_rest != ipch->left)
            {
                fprintf(stderr, "%s(): IPC protocol error:\n"
                        "Unexpected rest length of the message in the "
                        "datagram\n", __FUNCTION__);
                return TE_RC(TE_IPC, TE_ESYNCFAILED);
            }
        }
        else
        {
            KTRC("Return the rest of the last fragment len=%u\n",
                 client->frag_rest);
            data_size = client->frag_rest;
            data = (uint8_t *)client->buffer +
                       (client->frag_size - client->frag_rest);
        }

        copy_len = MIN(data_size, buf_len);
        memcpy(buf, data, copy_len);

        /* Shift buffer pointer */
        buf += copy_len;
        /* Calculate number of octets rest in the provided buffer */
        buf_len -= copy_len;
        /* Calculate number of octets rest in the current message */
        assert(client->msg_rest >= copy_len);
        client->msg_rest -= copy_len;
        /* Calculate number of octets rest in the current fragment */
        client->frag_rest = data_size - copy_len;

    } while ((buf_len > 0) && (client->msg_rest > 0));

    if (client->msg_rest > 0)
    {
        assert(buf_len == 0);
        *p_buf_len = client->msg_rest;
        KTRC("IPC: Buffer provided by caller is too small for the "
             "message\n");
        return TE_RC(TE_IPC, TE_ESMALLBUF);
    }
    else
    {
        /* Buffer provided by caller is enought */
        assert(client->msg_rest == 0);
        assert(client->frag_rest == 0);

        /*
         * Current message is copied to buffer,
         * there is no current message for the client.
         */
        client->msg_len = 0;

        *p_buf_len -= buf_len;

        return 0;
    }
}


/* See description in ipc_server.h */
int
ipc_send_answer(struct ipc_server *ipcs, struct ipc_server_client *ipcsc,
                const void *msg, size_t msg_len)
{
    struct ipc_dgram_header   *ipch;
    size_t                      octets_sent;
    size_t                      segm_size;
    ssize_t                     r;

    KTRC("ipcs=%p ipcsc=%s msg=%p len=%u\n",
         ipcs, SUN_NAME(&ipcsc->sa), msg, msg_len);
    if ((ipcs == NULL) || (ipcsc == NULL) ||
        ((msg == NULL) != (msg_len == 0)))
    {
        return TE_RC(TE_IPC, TE_EINVAL);
    }

    /* We use ipcs->buffer to construct the datagram */
    ipch = (struct ipc_dgram_header *)(ipcs->buffer);
    memset(ipch, 0, sizeof(*ipch));
    ipch->length = msg_len;

    for (octets_sent = 0;
         (octets_sent < msg_len) || (msg_len == 0);
         octets_sent += segm_size, msg += segm_size)
    {
        segm_size = MIN(IPC_SEGMENT_SIZE - sizeof(*ipch),
                        msg_len - octets_sent);

        memcpy(ipcs->buffer + sizeof(*ipch), msg, segm_size);

        ipch->left = msg_len - octets_sent;

        r = sendto(ipcs->socket, ipcs->buffer, sizeof(*ipch) + segm_size,
                   0 /* flags */, SA(&ipcsc->sa), ipcsc->sa_len);
        if (r != (ssize_t)(sizeof(*ipch) + segm_size))
        {
            fprintf(stderr, "Send IPC message from server '%s' to "
                            "client '%s' failed: %s\n", ipcs->name,
                            SUN_NAME(&ipcsc->sa), strerror(errno));
            return TE_OS_RC(TE_IPC, errno);
        }

        if (msg_len == 0)
            break;
    }

    return 0;
}


#else /* !TE_IPC_CONNECTIONLESS */


/**
 * Receive the message from IPC client.
 *
 * @param ipcs          Pointer to the ipc_server structure returned
 *                      by ipc_register_server()
 * @param buf           Buffer for the message
 * @param p_buf_len     Pointer to the variable to store:
 *                          on entry - length of the buffer;
 *                          on exit - length of the received message
 *                                    (or full length of the message if
 *                                     TE_ESMALLBUF returned).
 * @param ipcsc         Pointer to the ipc_server_client structure
 *                      returned by the ipc_receive_message() function
 *
 * @return Status code.
 *
 * @retval 0            Success
 * @retval TE_ESMALLBUF  Buffer is too small for the rest of the
 *                      message, the part of the message is written
 *                      to the buffer, the ipc_receive_message()
 *                      for the client should be called again
 * @retval errno        Other failure
 */
static int
ipc_server_receive(struct ipc_server *ipcs,
                       void *buf, size_t *p_buf_len,
                       struct ipc_server_client *ipcsc)
{
    size_t octets_to_read;
    int    rc;

    assert(ipcs != NULL);
    assert(buf != NULL);
    assert(p_buf_len != NULL);
    assert(*p_buf_len > 0);
    assert(ipcsc != NULL);

    octets_to_read = MIN(*p_buf_len, ipcsc->pending);
    rc = read_socket(ipcsc->socket, buf, octets_to_read);
    if (rc != 0)
    {
        fprintf(stderr, "ipc_server_receive(): read_socket() failed "
                        "in the middle of message\n");
        return rc;
    }

    ipcsc->pending -= octets_to_read;
    if (ipcsc->pending > 0)
    {
        *p_buf_len = ipcsc->pending;
        return TE_RC(TE_IPC, TE_ESMALLBUF);
    }
    else
    {
        *p_buf_len = octets_to_read;
        return 0;
    }
}

/* See description in ipc_server.h */
int
ipc_receive_message(struct ipc_server *ipcs,
                    void *buf, size_t *p_buf_len,
                    struct ipc_server_client **p_ipcsc)
{
    fd_set                    my_set;
    struct ipc_server_client *client;
    struct ipc_server_client *next_client;
    int                       max_fd;
    int                       rc;

    if ((ipcs == NULL) || (buf == NULL) || (p_ipcsc == NULL) ||
        (p_buf_len == NULL))
    {
        return TE_RC(TE_IPC, TE_EINVAL);
    }

    if (*p_ipcsc != NULL)
    {
        client = *p_ipcsc;

        if (client->pending == 0)
        {
            rc = read_socket(client->socket, &(client->pending),
                             sizeof(client->pending));
            if (rc != 0)
            {
                if (rc != TE_RC(TE_IPC, TE_ECONNABORTED))
                {
                    return rc;
                }
                else
                {
                    ipc_server_close_client(client);
                    return rc;
                }
            }
        }

        return ipc_server_receive(ipcs, buf, p_buf_len, client);
    }

    while (TRUE)
    {
        /* Data in the connection? */
        for (client = ipcs->clients.lh_first;
             client != NULL;
             client = next_client)
        {
            next_client = client->links.le_next;

            if (client->is_ready)
            {
                client->is_ready = FALSE;

                /*
                 * Connection with data found. Check that no pending data
                 * for them exists.
                 */
                if (client->pending != 0)
                {
                    fprintf(stderr, "IPC(%d): Unexpected client "
                            "connection state, pending=%u\n",
                            (int)getpid(), (unsigned)client->pending);
                    return TE_RC(TE_IPC, TE_ESYNCFAILED);
                }

                /*
                 * Let's read the length of the message and call
                 * ipc_receive_rest_message.
                 */
                rc = read_socket(client->socket, &(client->pending),
                                 sizeof(client->pending));
                if (rc != 0)
                {
                    if (rc != TE_RC(TE_IPC, TE_ECONNABORTED))
                    {
                        return rc;
                    }
                    else
                    {
                        ipc_server_close_client(client);
                        continue;
                    }
                }

                *p_ipcsc = client;

                return ipc_server_receive(ipcs, buf, p_buf_len, client);
            }
        }

        if (ipcs->is_ready)
        {
            ipcs->is_ready = FALSE;

            /* New connection requested. Accept it */
 
            client = calloc(1, sizeof(*client));
            assert(client != NULL);

#ifdef TE_IPC_AF_UNIX
            client->sa_len = sizeof(client->sa);
#endif

            client->socket = accept(ipcs->socket,
#ifdef TE_IPC_AF_UNIX
                                    SA(&client->sa), &client->sa_len
#else
                                    NULL, NULL
#endif
                                    );
            if (client->socket < 0)
            {
                perror("accept() failed");
                free(client);
            }
            else
            {
                LIST_INSERT_HEAD(&ipcs->clients, client, links);
            }

            /*
             * We've accepted the connection but have not receive any
             * message. So we have to repeat select.
             */
        }

        /*
         * Wait for one of:
         *  - client tries to establish connection
         *  - client sends data via established connection
         */
        FD_ZERO(&my_set);
        max_fd = ipc_get_server_fds(ipcs, &my_set);
        
        /* Waiting forever */
        rc = select(max_fd + 1, &my_set, NULL, NULL, NULL);
        if (rc <= 0) /* Error? */
        {
            perror("select() error");
            return TE_OS_RC(TE_IPC, errno);
        }

        (void)ipc_is_server_ready(ipcs, &my_set, max_fd);
    }
    /* Unreachable */
}


/* See description in ipc_server.h */
int
ipc_send_answer(struct ipc_server *ipcs, struct ipc_server_client *ipcsc,
                const void *msg, size_t msg_len)
{
    uint32_t len = msg_len;

    if ((ipcs == NULL) || (ipcsc == NULL) ||
        ((msg == NULL) != (msg_len == 0)))
    {
        return TE_RC(TE_IPC, TE_EINVAL);
    }

    if ((msg_len + sizeof(len)) > IPC_TCP_SERVER_BUFFER_SIZE)
    {
        /* Message is too long to fit into the internal buffer */

        if (write_socket(ipcsc->socket, &len, sizeof(len)) != 0)
        {
            return TE_OS_RC(TE_IPC, errno);
        }

        return write_socket(ipcsc->socket, msg, msg_len);
    }
    else
    {
        memcpy(ipcs->out_buffer,               &len, sizeof(len));
        memcpy(ipcs->out_buffer + sizeof(len), msg,  msg_len);

        return write_socket(ipcsc->socket, ipcs->out_buffer,
                            msg_len + sizeof(len));
    }
}

#endif /* !TE_IPC_CONNECTIONLESS */


/*
 * Local functions implementation
 */

#ifndef TE_IPC_AF_UNIX

/**
 * Connect to the my pmap server and register pair (server, port)
 *
 * @param server_name   The name of the server.
 * @param port          Port number.
 *
 * @return Status code.
 *
 * @retval 0        success
 * @retval -1       failure
 */
static int
ipc_pmap_register_server(const char *server_name, uint16_t port)
{
    unsigned short rc =
        ipc_pmap_process_command(IPC_PM_REG_SERVER, server_name, port);

    if (rc)
        return 0;
    else
        return -1;
}

/**
 * Connect to the my pmap server and unregister pair (server, port)
 *
 * @param server_name   The name of the server.
 * @param port          Port number.
 *
 * @return Status code.
 *
 * @retval 0        success
 * @retval -1       failure
 */
static int
ipc_pmap_unregister_server(const char *server_name, uint16_t port)
{
    unsigned short rc =
        ipc_pmap_process_command(IPC_PM_UNREG_SERVER, server_name, port);

    if (rc)
        return 0;
    else
        return -1;
}

#endif /* !TE_IPC_AF_UNIX */


#ifdef TE_IPC_CONNECTIONLESS

/**
 * Search in pool for the item with specified address and return
 * pointer to this item.  Allocate a new entry, if entry not found.
 * If new entry is created, all fileds is set to zero expect sa,
 * sa_len and buffer.
 *
 * @param ipcs      - pointer to the IPC server structure
 * @param sa_ptr    - pointer to the expected address
 * @param sa_len    - length of the address
 * @param p_ipcsc   - location for pointer to IPC server client structure
 *
 * @return Status code.
 */
static struct ipc_server_client *
ipc_int_client_by_addr(struct ipc_server *ipcs,
                       struct sockaddr_un *sa_ptr, socklen_t sa_len)
{
    struct ipc_server_client *ipcsc;

    for (ipcsc = ipcs->clients.lh_first;
         (ipcsc != NULL) &&
         ((sa_len != ipcsc->sa_len) ||
          (memcmp(sa_ptr, &ipcsc->sa, sa_len) != 0));
         ipcsc = ipcsc->links.le_next);

    if (ipcsc == NULL)
    {
        /* Not found. Allocate new. */
        ipcsc = calloc(1, sizeof(*ipcsc));
        if (ipcsc != NULL)
        {
            ipcsc->sa     = *sa_ptr;
            ipcsc->sa_len = sa_len;
            ipcsc->buffer = calloc(1, IPC_SEGMENT_SIZE);
            if (ipcsc->buffer == NULL)
            {
                free(ipcsc);
                ipcsc = NULL;
            }
            else 
            {
                LIST_INSERT_HEAD(&ipcs->clients, ipcsc, links);
            }
        }
    }

    return ipcsc;
}


/**
 * Get datagram from the list of already received.
 *
 * @param ipcs      - pointer to the ipc_server structure.
 * @param p_ipcsc   - location of the pointer to the ipc_server_client
 *                    structure, if datagram from the specified client
 *                    expected, NULL otherwise
 *
 * @return Status code.
 */
static int
ipc_int_get_datagram_from_pool(struct ipc_server *ipcs,
                               struct ipc_server_client **p_ipcsc)
{
    struct ipc_server_client   *ipcsc;
    struct ipc_datagram        *p;

    assert(ipcs != NULL);
    assert(p_ipcsc != NULL);

    ipcsc = *p_ipcsc;
    for (p = ipcs->datagrams.tqh_first;
         p != NULL;
         p = p->links.tqe_next)
    {
        if ((ipcsc == NULL) ||
            ((ipcsc->sa_len == p->sa_len) &&
             (memcmp(&ipcsc->sa, &p->sa, ipcsc->sa_len) == 0)))
        {
            if (ipcsc == NULL)
            {
                ipcsc = ipc_int_client_by_addr(ipcs, &p->sa, p->sa_len);
                if (ipcsc == NULL)
                {
                    fprintf(stderr, "Memory allocation failure\n");
                    return TE_RC(TE_IPC, TE_ENOMEM);
                }
            }

            free(ipcsc->buffer);
            ipcsc->buffer = p->buffer;
            ipcsc->frag_size = p->octets;

            /* Free datagram */
            TAILQ_REMOVE(&ipcs->datagrams, p, links);
            free(p);

            *p_ipcsc = ipcsc;
            KTRC("Got datagram from pool for client %s\n",
                 SUN_NAME(&ipcsc->sa));

            return 0;
        }
    }

    return TE_RC(TE_IPC, TE_ESRCH);
}


/**
 * Write datagram to the ipcs->pool pool from the ipcs->datagrams pool
 * or from the socket, this function may block.
 *
 * @param ipcs      - pointer to the ipc_server structure.
 * @param p_ipcsc   - location of the pointer to the ipc_server_client
 *                    structure, if datagram from the specified client
 *                    expected, NULL otherwise
 *
 * @return Status code.
 */
static int
ipc_int_get_datagram(struct ipc_server *ipcs,
                     struct ipc_server_client **p_ipcsc)
{
    int rc;

    assert(ipcs != NULL);
    assert(p_ipcsc != NULL);

    /* At first try to find appropriate datagram from pool */
    rc = ipc_int_get_datagram_from_pool(ipcs, p_ipcsc);
    if (rc != TE_RC(TE_IPC, TE_ESRCH))
    {
        /* Datagram has been got or processing failed */
        return TE_RC(TE_IPC, rc);
    }

    /* No appropriate atagram in the pool found */
    while (TRUE)
    {
        struct sockaddr_un          sa;
        socklen_t                   sa_len = sizeof(sa);
        ssize_t                     r;
        struct ipc_server_client   *ipcsc = *p_ipcsc;

        r = recvfrom(ipcs->socket, ipcs->buffer, IPC_SEGMENT_SIZE,
                     0 /* flags */, SA(&sa), &sa_len);
        if (r < 0)
        {
            fprintf(stderr, "IPC server '%s' failed to receive "
                            "message: %s\n", ipcs->name, strerror(errno));
            return TE_OS_RC(TE_IPC, errno);
        }

        if ((ipcsc != NULL) &&
            ((sa_len != ipcsc->sa_len) ||
             (memcmp(&sa, &ipcsc->sa, sa_len) != 0)))
        {
            int rc2;

            /*
             * While we trying to recevie rest of the partial-received
             * message, we've got the message from other client.
             * We have to save the datagram for future read.
             */
            rc2 = ipc_remember_datagram(&ipcs->datagrams, ipcs->buffer,
                                        r, &sa, sa_len);
            if (rc2 != 0)
            {
                fprintf(stderr, "ipc_int_remember_datagram() failed\n");
                return TE_RC(TE_IPC, rc2);
            }
            /* IPC server buffer was owned by previous routine */
            ipcs->buffer = calloc(1, IPC_SEGMENT_SIZE);
            if (ipcs->buffer == NULL)
            {
                return TE_RC(TE_IPC, TE_ENOMEM);
            }
        }
        else
        {
            if (ipcsc == NULL)
            {
                ipcsc = ipc_int_client_by_addr(ipcs, &sa, sa_len);
                if (ipcsc == NULL)
                {
                    fprintf(stderr, "Memory allocation failure\n");
                    return TE_RC(TE_IPC, TE_ENOMEM);
                }
                *p_ipcsc = ipcsc;
            }

            SWAP_PTRS(ipcsc->buffer, ipcs->buffer);
            ipcsc->frag_size = r;

            KTRC("Got datagram from net for client %s\n",
                 SUN_NAME(&ipcsc->sa));
            return 0;
        }
    }

    /* Unreachable */
    assert(0);
    return TE_RC(TE_IPC, TE_EFAULT);
}

#else /* !TE_IPC_CONNECTIONLESS */

/**
 * Read specified number of octets (not less) from the connection.
 *
 * @param socket    Connection socket.
 * @param buffer    Buffer to store the data.
 * @param len       Number of octets to read.
 *
 * @return Status code.
 *
 * @retval 0        success
 * @retval errno    failure
 */
static int
read_socket(int socket, void *buffer, size_t len)
{
    while (len > 0)
    {
        int r = recv(socket, buffer, len, 0);

        if (r < 0)
        {
            perror("read_socket(): recv() error");
            return TE_OS_RC(TE_IPC, errno);
        }
        else if (r == 0)
        {
            return TE_RC(TE_IPC, TE_ECONNABORTED);
        }

        len -= r;
        buffer += r;
    }

    return 0;
}


/**
 * Write specified number of octets (not less) to the connection.
 *
 * @param socket    Connection socket.
 * @param buffer    Buffer with data to send.
 * @param len       Number of octets to send.
 *
 * @return Status code.
 *
 * @retval 0        success
 * @retval errno    failure
 */
static int
write_socket(int socket, const void *buffer, size_t len)
{
    while (len > 0)
    {
        int r = send(socket, buffer, len, 0);

        if (r < 0)
        {
            perror("write_socket send() error");
            return TE_OS_RC(TE_IPC, errno);
        }
        else if (r == 0)
        {
            fprintf(stderr, "Remote peer closed connection\n");
        }

        len -= r;
        buffer += r;
    }

    return 0;
}

#endif /* !TE_IPC_CONNECTIONLESS */
