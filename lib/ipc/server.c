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
 * @author Andrey Ivanov <andron@oktetlabs.ru>
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

#ifndef IPC_UNIX
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


/** Debug TCP? */
#define TCP_DEBUG 0


/**
 * The ipc_server_client structure is used to store information about
 * sender of the message. This structure used in both UDP/UNIX and 
 * TCP/IP implementations of IPC, but has different fields.
 */
struct ipc_server_client {
    /** Links to neighbour clients in the list */
    LIST_ENTRY(ipc_server_client)   links;

#ifdef IPC_UNIX
    struct sockaddr_un  sa;         /**< Address of the sender */
    socklen_t           sa_len;     /**< Length of the sockaddr_un struct */
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
    uint32_t    pending_octets; /**< Number of octets in current message
                                     left to read from socket and to
                                     return to user. This field MUST
                                     be 4-octets long. */
#endif
};


/** Structure to store state information about the server */
struct ipc_server {
    char    name[UNIX_PATH_MAX];    /**< Name of the server */
    int     socket;                 /**< Server socket */

    /** List of "active" IPC clients */
    LIST_HEAD(ipc_server_clients, ipc_server_client) clients;

#ifdef IPC_UNIX
    char                   *buffer;     /**< Used to avoid data copying
                                             on receiving datagrams */
    struct ipc_datagrams    datagrams;  /**< Delayed datagrams */
#else
    uint16_t    port;           /**< Port number used for this server */
    char       *out_buffer;     /**< Buffer for outgoing messages */
#endif
};


/*
 * Static functions declaration.
 */

#ifdef IPC_UNIX

static struct ipc_server_client * ipc_int_client_by_addr(
                                      struct ipc_server *ipcs,
                                      struct sockaddr_un *sa_ptr,
                                      socklen_t sa_len);

static int ipc_int_get_datagram_from_pool(struct ipc_server *ipcs,
                                          struct ipc_server_client
                                              **p_ipcsc);

static int ipc_int_get_datagram(struct ipc_server *ipcs,
                                struct ipc_server_client **p_ipcsc);

#else

static int read_socket(int socket, void *buffer, size_t len);
static int write_socket(int socket, const void *buffer, size_t len);

static int ipc_pmap_register_server(const char *server_name,
                                    uint16_t port);
static int ipc_pmap_unregister_server(const char *server_name,
                                      uint16_t port);

#endif


/*
 * Global functions implementation
 */

/* See description in ipc_server.h */
struct ipc_server *
ipc_register_server(const char *name)
{
    struct ipc_server *ipcs;

    if (name == NULL)
    {
        errno = EINVAL;
        return NULL;
    }
    if (strlen(name) >= (UNIX_PATH_MAX - 1))
    {
        errno = E2BIG;
        return NULL;
    }

    ipcs = calloc(1, sizeof(*ipcs));
    if (ipcs == NULL)
    {
        perror("ipc_register_server(): calloc() error");
        return NULL;
    }

    strcpy(ipcs->name, name);
    LIST_INIT(&ipcs->clients);

#ifdef IPC_UNIX
    TAILQ_INIT(&ipcs->datagrams);

    ipcs->buffer = calloc(1, IPC_SEGMENT_SIZE);
    if (ipcs->buffer == NULL)
    {
        perror("ipc_register_server(): calloc() error");
        free(ipcs);
        return NULL;
    }

    /* Create a socket */
    ipcs->socket = socket(PF_UNIX, SOCK_DGRAM, 0);
    if (ipcs->socket < 0)
    {
        perror("ipc_register_server(): socket() error");
        free(ipcs->buffer);
        free(ipcs);
        return NULL;
    }

    {
        struct sockaddr_un  sa;
        
        /* Assign a name to socket */
        sa.sun_family = AF_UNIX;
        memset(sa.sun_path, 0, UNIX_PATH_MAX);
        /* Sockets in the abstract namespace are started from '\0' */
        strcpy(sa.sun_path + 1, name);

        if (bind(ipcs->socket, SA(&sa), sizeof(sa)) != 0)
        {
            fprintf(stderr, "Failed to register IPC server '%s': ",
                            SUN_NAME(&sa));
            perror("");
            close(ipcs->socket);
            free(ipcs->buffer);
            free(ipcs);
            return NULL;
        }
    }

#else

    if (IPC_TCP_SERVER_BUFFER_SIZE != 0)
    {
        ipcs->out_buffer = calloc(1, IPC_TCP_SERVER_BUFFER_SIZE);
        if (ipcs->out_buffer == NULL)
        {
            perror("ipc_register_server(): calloc() error");
            free(ipcs);
            return NULL;
        }
    }

    /* Create a socket */
    ipcs->socket = socket(AF_INET, SOCK_STREAM, 0);
    if (ipcs->socket < 0)
    {
        perror("ipc_register_server(): socket() error");
        free(ipcs->out_buffer);
        free(ipcs);
        return NULL;
    }

    if (listen(ipcs->socket, SOMAXCONN) != 0)
    {
        perror("listen() error");
        close(ipcs->socket);
        free(ipcs->out_buffer);
        free(ipcs);
        return NULL;
    }

    {
        struct sockaddr_in addr;;
        int addr_len = sizeof(addr);

        if (getsockname(ipcs->socket, (struct sockaddr*)&addr,
                        &addr_len) != 0)
        {
            perror("getsockname() error");
            close(ipcs->socket);
            free(ipcs->out_buffer);
            free(ipcs);
            return NULL;
        }
        if (ipc_pmap_register_server(name, addr.sin_port) != 0)
        {
            perror("Cannot register server's port");
            close(ipcs->socket);
            free(ipcs->out_buffer);
            free(ipcs);
            return NULL;
        }
    }

#endif
#if HAVE_FCNTL_H
    /* 
     * Try to set close-on-exec flag, but ignore failures, 
     * since it's not critical.
     */
    (void)fcntl(ipcs->socket, F_SETFD, FD_CLOEXEC);
#endif

    return ipcs;
}


/* See description in ipc_server.h */
int
ipc_get_server_fd(const struct ipc_server *ipcs)
{
    assert(ipcs != NULL);

    return ipcs->socket;
}


/* See description in ipc_server.h */
int
ipc_close_server(struct ipc_server *ipcs)
{
    struct ipc_server_client *ipcsc;
#ifdef IPC_UNIX
    struct ipc_datagram *p;
#endif

    if (ipcs == NULL)
        return 0;

    close(ipcs->socket);

#ifdef IPC_UNIX
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
        LIST_REMOVE(ipcsc, links);
#ifdef IPC_UNIX
        free(ipcsc->buffer);
#else
        close(ipcsc->socket);
#endif
        free(ipcsc);
    }

#ifndef IPC_UNIX
    if (ipc_pmap_unregister_server(ipcs->name, ipcs->port) != 0)
    {
        perror("Cannot unregister server");
        return -1;
    }
#endif

    /* Free instance */
    free(ipcs);

    return 0;
}


#ifdef IPC_UNIX

/* See description in ipc_server.h */
int
ipc_receive_message(struct ipc_server *ipcs,
                    void *buf, size_t *p_buf_len,
                    struct ipc_server_client **p_ipcsc)
{
    struct ipc_server_client *client = *p_ipcsc;

    int     rc;
    size_t  buf_len = *p_buf_len; /* Size of buffer provided by caller */
    size_t  data_size;            /* Size of data in current fragment */
    void   *data;                 /* Pointer to the data to be copied */
    size_t  copy_len;             /* Size of data to be copied */


    if ((ipcs == NULL) || (buf == NULL) || (p_ipcsc == NULL) ||
        (p_buf_len == NULL) || (*p_buf_len <= 0))
    {
        return EINVAL;
    }

    KTRC("*p_buf_len=%u *p_ipcsc=0x%x\n", *p_buf_len, *p_ipcsc);
    do {
        if ((client == NULL) || (client->frag_rest == 0))
        {
            struct ipc_packet_header   *ipch;

            KTRC("Get datagram\n");
            /*
             * Get datagram for any client from already received list or
             * wait for network.
             */
            rc = ipc_int_get_datagram(ipcs, &client);
            if (rc != 0)
            {
                assert(0);
                return rc;
            }

            ipch = (struct ipc_packet_header*)client->buffer;

            KTRC("Datagram frag=%u len=%u left=%u\n",
                 client->frag_size, ipch->length, ipch->left);

            data_size = client->frag_size -
                            sizeof(struct ipc_packet_header);
            assert(data_size <= ipch->left);

            data = client->buffer + sizeof(struct ipc_packet_header);

            if (*p_ipcsc == NULL)
            {
                KTRC("New datagram for client %s\n",
                     SUN_NAME(&client->sa));
                *p_ipcsc = client;
                /* It must be the first fragment of the message */
                if (client->msg_len != 0)
                {
                    assert(FALSE);
                    return ESYNCFAILED;
                }
                client->msg_len = client->msg_rest = ipch->length;
            }
            else
            {
                KTRC("Continue with the client %s: msg_len=%u "
                     "msg_rest=%u\n", SUN_NAME(&client->sa),
                     client->msg_len, client->msg_rest);
                /* It may be not the first fragment of the message */
                if (client->msg_len == 0)
                {
                    client->msg_len = client->msg_rest = ipch->length;
                }
                else if (client->msg_len != ipch->length)
                {
                    assert(FALSE);
                    return ESYNCFAILED;
                }
            }
            if (client->msg_rest != ipch->left)
            {
                assert(FALSE);
                return ESYNCFAILED;
            }
        }
        else
        {
            KTRC("Return the rest of the last fragment len=%u\n",
                 client->frag_rest);
            data_size = client->frag_rest;
            data = client->buffer +
                       (client->frag_size - client->frag_rest);
        }

        copy_len = MIN(data_size, buf_len);
        memcpy(buf, data, copy_len);

        /* Shift buffer pointer */
        buf += copy_len;
        /* Calculate number of octets rest in the provided buffer */
        buf_len -= copy_len;
        /* Calculate number of octets rest in the current message */
        client->msg_rest -= copy_len;

    } while ((buf_len > 0) && (client->msg_rest > 0));

    /* Calculate number of octets rest in the current fragment */
    client->frag_rest = data_size - copy_len;

    *p_buf_len = client->msg_len;

    if ((client->msg_rest > 0) && (buf_len == 0))
    {
        assert(0);
        /* Buffer provided by caller is too small for the message */
        return ETESMALLBUF;
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

        return 0;
    }
}


/* See description in ipc_server.h */
int
ipc_send_answer(struct ipc_server *ipcs, struct ipc_server_client *ipcsc,
                const void *msg, size_t msg_len)
{
    struct ipc_packet_header   *ipch;
    size_t                      octets_sent;
    size_t                      segm_size;
    ssize_t                     r;

    KTRC("ipcs=0x%x ipcsc=%s msg=0x%x len=%u\n",
         ipcs, SUN_NAME(&ipcsc->sa), msg, msg_len);
    if ((ipcs == NULL) || (ipcsc == NULL) ||
        ((msg == NULL) != (msg_len == 0)))
    {
        return EINVAL;
    }

    /* We use ipcs->buffer to construct the datagram */
    ipch = (struct ipc_packet_header *)(ipcs->buffer);
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
                            "client '%s' failed: ",
                            ipcs->name, SUN_NAME(&ipcsc->sa));
            perror("");
            return errno;
        }

        if (msg_len == 0)
            break;
    }

    return 0;
}


#else /* IPC_UNIX */


/* See description in ipc_server.h */
int
ipc_receive_message(struct ipc_server *ipcs,
                    void *buf, size_t *p_buf_len,
                    struct ipc_server_client **p_ipcsc)
{
    fd_set                    my_set;
    struct ipc_server_client *ptr;
    int                       max_n;
    int                       rc;

    if ((ipcs == NULL) || (buf == NULL) || (p_ipcsc == NULL) ||
        (p_buf_len == NULL) || (*p_buf_len <= 0))
    {
        return EINVAL;
    }

    while (TRUE)
    {
        /*
         * Wait for one of:
         *  - client tries to establish connection
         *  - client sends data via established connection
         */
        FD_ZERO(&my_set);
        FD_SET(ipcs->socket, &my_set);
        max_n = ipcs->socket;

        ptr = ipcs->pool;
        while (ptr)
        {
            FD_SET(ptr->socket, &my_set);
            max_n = MAX(max_n, ptr->socket);
            ptr = ptr->next;
        }
#if TCP_DEBUG
        printf("Selecting...\n");
#endif
        /* Waiting forever */
        rc = select(max_n + 1, &my_set, NULL, NULL, NULL);
#if TCP_DEBUG
        printf("Selected\n");
#endif
        /* What's happened? */
        if (rc <= 0) /* Error? */
        {
            perror("select() error");
            return errno;
        }

        /* Data in the connection? */
        ptr = ipcs->pool;
        while (ptr)
        {
            if (FD_ISSET(ptr->socket, &my_set))
            {
                /*
                 * Connection with data found. Check that no pending data
                 * for them exists.
                 */
#if TCP_DEBUG
                printf("Connection wants to be read\n");
#endif
                if (ptr->pending_octets)
                {
                    /* Error! */
#if TCP_DEBUG
                    printf("IPC/TCP: Trying to read new message while "
                           "old one is not fully read %d!\n",
                           ptr->pending_octets);
#endif
                    return ESYNCFAILED;
                }

                /*
                 * Let's read the length of the message and call
                 * ipc_receive_rest_message.
                 */
                if (read_socket(ptr->socket, &(ptr->pending_octets),
                                sizeof(ptr->pending_octets)) != 0)
                {
                    return errno;
                }

                *p_ipcsc = ptr;

                return ipc_receive_rest_message(ipcs, buf, p_buf_len, ptr);
            }
            ptr = ptr->next;
        }

        /* New connection? */
        if (!FD_ISSET(ipcs->socket, &my_set))
        {
            /* No! */
            perror("Can't understand what is going on with select()");
            return -1;
        }

        /* New connection requested. Accept it */

        /* Create new entry in the pool. We can just insert it first */
        ptr = calloc(1, sizeof(struct ipc_server_client));
        assert(ptr != NULL);

        ptr->next = ipcs->pool;
        ipcs->pool = ptr;

#if TCP_DEBUG
        printf("Accepting socket\n");
#endif
        ptr->socket = accept(ipcs->socket, NULL, NULL);
#if TCP_DEBUG
        printf("Socket %d accepted, Entry = 0x%08X\n",
               ptr->socket, (int)ptr);
#endif
        /*
         * We've accepted the connection but have not receive any message.
         * So we have to repeat select.
         */
    }
    /* Unreachable */
}


#if 0
/* See description in ipc_server.h */
int
ipc_receive_rest_message(struct ipc_server *ipcs,
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

    octets_to_read = MIN(*p_buf_len, ipcsc->pending_octets);

    /* If we have something to read */
    rc = read_socket(ipcsc->socket, buf, octets_to_read);
    if (rc != 0)
    {
        /* Error occured */
        return rc;
    }

    ipcsc->pending_octets -= octets_to_read;

    if (ipcsc->pending_octets > 0)
    {
        *p_buf_len = ipcsc->pending_octets + octets_to_read;

        /* Something to read more */
        return ETESMALLBUF;
    }
    else
    {
        *p_buf_len = octets_to_read;

        /* Nothing to read */
        return 0;
    }
}
#endif


/* See description in ipc_server.h */
int
ipc_send_answer(struct ipc_server *ipcs, struct ipc_server_client *ipcsc,
                const void *msg, size_t msg_len)
{
    uint32_t len = msg_len;

    if ((ipcs == NULL) || (ipcsc == NULL) ||
        ((msg == NULL) != (msg_len == 0)))
    {
        return EINVAL;
    }

    if ((msg_len + sizeof(len)) > IPC_TCP_SERVER_BUFFER_SIZE)
    {
        /* Message is too long to fit into the internal buffer */

        if (write_socket(ipcsc->socket, &len, sizeof(len)) != 0)
        {
            return errno;
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

#endif


/*
 * Local functions implementation
 */

#ifdef IPC_UNIX

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
            LIST_INSERT_HEAD(&ipcs->clients, ipcsc, links);
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
 * @retval ESRCH    - no datagram found
 */
static int
ipc_int_get_datagram_from_pool(struct ipc_server *ipcs,
                               struct ipc_server_client **p_ipcsc)
{
    struct ipc_datagram        *p;
    struct ipc_server_client   *ipcsc = *p_ipcsc;

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
                    return ENOMEM;
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

    return ESRCH;
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
    if (rc != ESRCH)
    {
        /* Datagram has been got or processing failed */
        return rc;
    }

    /* No appropriate atagram in the pool found */
    while (rc == ESRCH)
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
                            "message: ", ipcs->name);
            perror("");
            return errno;
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
                fprintf(stderr, "ipc_int_remember_datagram() failed");
                return rc2;
            }
            /* IPC server buffer was owned by previous routine */
            ipcs->buffer = calloc(1, IPC_SEGMENT_SIZE);
            if (ipcs->buffer == NULL)
            {
                return ENOMEM;
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
                    return ENOMEM;
                }
            }

            SWAP_PTRS(ipcsc->buffer, ipcs->buffer);
            ipcsc->frag_size = r;

            *p_ipcsc = ipcsc;
            rc = 0;
            KTRC("Got datagram from net for client %s\n",
                 SUN_NAME(&ipcsc->sa));
        }
    }

    return rc;
}

#else

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
#if TCP_DEBUG
    printf("Reading %d octets...\n", len);
#endif
    while (len > 0)
    {
        int r = recv(socket, buffer, len, 0);

        if (r < 0)
        {
            perror("read_socket recv() error\n");
            return errno;
        }
        else if (r == 0)
        {
            fprintf(stderr, "Remote peer closed connection\n");
        }

        len -= r;
        buffer += r;
    }

#if TCP_DEBUG
    printf("Done\n");
#endif

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
#if TCP_DEBUG
    printf("Writing %d octets... \n", len);
#endif

    while (len > 0)
    {
        int r = send(socket, buffer, len, 0);

        if (r < 0)
        {
            perror("write_socket send() error");
            return errno;
        }
        else if (r == 0)
        {
            fprintf(stderr, "Remote peer closed connection\n");
        }

        len -= r;
        buffer += r;
    }

#if TCP_DEBUG
    printf("Done\n");
#endif

    return 0;
}

#endif
