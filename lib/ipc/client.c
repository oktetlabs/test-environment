/** @file
 * @brief IPC library
 *
 * Implementation of routines (client side) provided for library user.
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_ASSERT_H
#include <assert.h>
#endif
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#include "ipc_internal.h"

#ifndef IPC_UNIX
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif
#endif /* !IPC_UNIX */

#include "te_defs.h"
#include "te_errno.h"
#include "ipc_client.h"


/** @name IPC Client retry parameters */

/** Maximum number of retries for IPC client to send message */
#define IPC_CLIENT_RETRY_MAX        10

/** Timeout in seconds between IPC client retries to send message */
#define IPC_CLIENT_RETRY_TIMEOUT    1

/*@}*/


/** @name IPC Client debugging parameters */

/** Debug reassembling? */
#define IPC_CLIENT_DEBUG_REASSEMBLING 0

/** Debug pool usage? */
#define IPC_CLIENT_DEBUG_POOL 0

/** Debug TCP? */
#define IPC_CLIENT_DEBUG_TCP 0

/*@}*/


/** Structure to store information about the servers. */
struct ipc_client_server {
    /* TCP/UDP independent part */
    struct ipc_client_server *next; /**< Link to the next item in pool
                                         or NULL if this is last item */
    struct sockaddr_un sa;          /**< Address of the server */
    socklen_t sa_len;               /**< Length of the sockaddr_un struct */

#ifdef IPC_UNIX
    char *buffer;            /**< Buffer for datagram */
    size_t length;           /**< Length of the currently receiving
                                  message, 0 if none */
    size_t octets_received;  /**< Number of octets of message currently
                                 received, no meaning if
                                 message_length == 0 */
    size_t fragment_size;   /**< Number of octets in partial-returned
                                 datagram, including header */
    size_t octets_returned;  /**< Number of octets of current segment
                                 returned to user,  no meaning if
                                 message_length == 0. if
                                 octets_returned = 0
                                 whole datagram processed */
#else
    int socket;                 /**< Socket */
    char name[UNIX_PATH_MAX];   /**< Name of the server */

    size_t pending_octets;       /**< Number of octets in current message
                                     left to read from socket and to
                                     return to user. This field
                                     MUST be 4-octets long. */
#endif
};

/** ipc_client structure is used to store state information about the client */
struct ipc_client {
    struct ipc_client_server *pool;   /**< Pool of the used servers */
    char   name[UNIX_PATH_MAX];       /**< IPC client name */
#ifdef IPC_UNIX
    int socket;                       /**< Socket */
    struct ipc_datagrams datagrams;   /**< Pool for the datagrams */
    char *tmp_buffer;                 /**< Buffer for datagram */
#else
    char *out_buffer;                 /**< Buffer for outgoing messages */
#endif
};

/**
 * Search in pool for the item with specified server name and return
 * pointer to this item. Allocate a new entry if entry not found. If new
 * entry is created, all fileds is set to zero expect sa, sa_len and buffer.
 *
 * @param       ipcc            Pointer to the ipcc structure.
 * @param       sa_ptr          Pointer to the expected address.
 * @param       sa_len          Length of the address.
 *
 * @return
 *    pointer to the item in the ipcc->pool pool
 */
static struct ipc_client_server *
get_pool_item_by_name(struct ipc_client *ipcc, const char *name)
{
    struct ipc_client_server *ipccs = ipcc->pool;
    struct ipc_client_server **parent = &ipcc->pool;

    while (ipccs != NULL)
    {
#ifdef IPC_UNIX
        if (strcmp(ipccs->sa.sun_path + 1, name) == 0)
#else
#if IPC_CLIENT_DEBUG_TCP
        printf("Scanning 0x%08X, %s\n", (int)ipccs, ipccs->name);
#endif
        if (strcmp(ipccs->name, name) == 0)
#endif
        {
            /* Found */
            return ipccs;
        }
        parent = &ipccs->next;
        ipccs = ipccs->next;
    }

    /* Not found. Allocate new */
    (*parent) = calloc(1, sizeof(struct ipc_client_server));
    if (*parent == NULL)
        return NULL;

#ifdef IPC_UNIX
    (*parent)->sa.sun_family = AF_UNIX;
    memset((*parent)->sa.sun_path, 0, UNIX_PATH_MAX);
    strcpy((*parent)->sa.sun_path + 1, name);

    (*parent)->sa_len = sizeof(struct sockaddr_un);
    (*parent)->buffer = calloc(1, IPC_SEGMENT_SIZE);
    if ((*parent)->buffer == NULL)
    {
        free(*parent);
        return NULL;
    }
#else
    strcpy((*parent)->name, name);
#endif

    return (*parent);
}

/**
 * Free pool of IPC client servers.
 *
 * @param ipcc      IPC client
 */
static void
ipc_free_client_server_pool(struct ipc_client *ipcc)
{
    struct ipc_client_server *ipccs;
    struct ipc_client_server *tmp;

    assert(ipcc != NULL);

    ipccs = ipcc->pool;
    while (ipccs != NULL)
    {
        tmp = ipccs->next;
#ifdef IPC_UNIX
        free(ipccs->buffer);
#else
        close(ipccs->socket);
#endif
        free(ipccs);
        ipccs = tmp;
    }
    ipcc->pool = NULL;
}


#ifdef IPC_UNIX

/**
 * Write datagram to the ipcc->pool pool from the ipcc->datagrams pool
 * or from the socket, this function may block.
 *
 * @param       ipcc            Pointer to the ipc_client structure.
 * @param       pool_item       Pointer to the ipc_client_server structure
 *                              if datagram from the specified client
 *                              expected, NULL otherwise.
 *
 * @return
 *      Pointer to the pool item on success, NULL on error.
 */
static struct ipc_client_server *
get_datagram(struct ipc_client *ipcc,
             struct ipc_client_server *pool_item)
{
    struct ipc_datagram *ptr;

    assert(ipcc != NULL);

    for (ptr = ipcc->datagrams.tqh_first;
         ptr != NULL;
         ptr = ptr->links.tqe_next)
    {
        if (pool_item == NULL ||
            ((pool_item->sa_len == ptr->sa_len) &&
             (memcmp(&pool_item->sa, &ptr->sa, pool_item->sa_len) == 0)))
        {
            assert(ptr->octets <= IPC_SEGMENT_SIZE);

            TAILQ_REMOVE(&ipcc->datagrams, ptr, links);

            if (pool_item == NULL)
            {
                /* Search for pool item */
                pool_item = get_pool_item_by_name(ipcc,
                                                  ptr->sa.sun_path + 1);
                if (pool_item == NULL)
                {
                    return NULL;
                }
#if IPC_CLIENT_DEBUG_POOL
                printf("Returned first as any\n");
#endif
            }

#if IPC_CLIENT_DEBUG_POOL
            printf("Poped %d octets \n>", ptr->octets);
            {
                int i;
                for (i = 0; i < ptr->octets; i++)
                {
                    char ch = ptr->buffer[i];

                    if (ch < 32)
                        ch = '-';

                    printf("%c|", ch);
                }
            }
            printf("<\n from %s (poped)\n", ptr->sa.sun_path + 1);
#endif

            /* Copy data */
            memcpy(pool_item->buffer, ptr->buffer, ptr->octets);

            /* Remember value to return */
            pool_item->fragment_size = ptr->octets;

            /* Free buffer */
            free(ptr->buffer);

            /* Free list item */
            free(ptr);

            return pool_item;
        }
    }

    /* No datagram in the pool or datargarm with specified address not found */

    if (pool_item == NULL) /* Datagram with any source address expected */
    {
        struct sockaddr_un  sa;
        socklen_t           sa_len = sizeof(sa);
        int                 r;

        r = recvfrom(ipcc->socket, ipcc->tmp_buffer, IPC_SEGMENT_SIZE, 0,
                     (struct sockaddr*)&sa, &sa_len);

        if (r < 0)
        {
            perror("recvfrom error");
            return NULL;
        }

        /* Find or assign a new item in pool */
        pool_item = get_pool_item_by_name(ipcc, sa.sun_path + 1);
        if (pool_item == NULL)
        {
            return NULL;
        }

        pool_item->fragment_size = r;

        /* Swap buffers */
        {
            char *tmp = pool_item->buffer;

            pool_item->buffer = ipcc->tmp_buffer;
            ipcc->tmp_buffer = tmp;
        }

        return pool_item;
    }
    else
    { /* Datagram with specified source address expected */
        while (TRUE)
        {
            struct sockaddr_un sa;
            socklen_t sa_len = sizeof(sa);

            int r;

            r = recvfrom(ipcc->socket,
                         pool_item->buffer, IPC_SEGMENT_SIZE,
                         0 /* flags */,
                         (struct sockaddr*)&sa, &sa_len);

            if (r < 0)
            {
                perror("recvfrom error");
                return NULL;
            }

            if ((sa_len != pool_item->sa_len) ||
                memcmp(&sa,&pool_item->sa, sa_len))
            {
                /*
                 * While we trying to recevie rest of the partial
                 * received message, we've got the message from other
                 * client. We have to save the datagram for future read.
                 */
                ipc_remember_datagram(&ipcc->datagrams,
                                      pool_item->buffer, r,
                                      &sa, sa_len);
                pool_item->buffer = calloc(1, IPC_SEGMENT_SIZE);
                if (pool_item->buffer == NULL)
                    return NULL;
            }
            else
            {
                /* This datagram is what we expecting */
                pool_item->fragment_size = r;
                return pool_item;
            }
        }
    }

    /* Unreachable place */
    assert(0);
}

#else
static unsigned short ipc_pmap_get_server(const char *server_name);

/**
 * Read specified number of octets (not less) from the connection.
 *
 * @param       socket          Connection socket.
 * @param       buffer          Buffer to store the data.
 * @param       len             Number of octets to read.
 *
 * @return
 *      Status code.
 *
 * @retval      0               Success.
 * @retval      other errno     errno.
 */
static int
read_socket(int socket, char *buffer, size_t len)
{
#if IPC_CLIENT_DEBUG_TCP
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

        len -= r;
        buffer += r;
    }
#if IPC_CLIENT_DEBUG_TCP
    printf("Done\n");
#endif

    return 0;
}
/**
 * Writes specified number of octets (not less) to the connection.
 *
 * @param       socket          Connection socket.
 * @param       buffer          Buffer with data to send.
 * @param       len             Number of octets to send.
 *
 * @return
 *      Status code.
 *
 * @retval      0               Success.
 * @retval      other errno     errno.
 */
static int
write_socket(int socket, const char *buffer, size_t len)
{
#if IPC_CLIENT_DEBUG_TCP
    printf("Sending %d octets... ", len);
#endif
    while (len > 0)
    {
        int r = send(socket, buffer, len, 0);
        if (r < 0)
        {
            perror("write_socket send() error");
            return errno;
        }

        len -= r;
        buffer += r;
    }

#if IPC_CLIENT_DEBUG_TCP
    printf("Sent\n");
#endif
    return 0;
}

#endif


#ifdef IPC_UNIX

/* See description in ipc_client.h */
struct ipc_client *
ipc_init_client(const char *name)
{
    struct ipc_client *ipcc;
    int s;

    if (name == NULL)
    {
        errno = EINVAL;
        return NULL;
    }
    /* Check length of the name */
    if (strlen(name) >= UNIX_PATH_MAX)
    {
        errno = E2BIG;
        return NULL;
    }

    /* Create a socket */
    s = socket(PF_UNIX, SOCK_DGRAM, 0);
    if (s < 0)
    {
        perror("ipc_init_client(): socket() error");
        return NULL;
    }

    /* Assign a name to socket */
    {
        struct sockaddr_un addr;

        addr.sun_family = AF_UNIX;
        memset(addr.sun_path, 0, UNIX_PATH_MAX);
        strcpy(addr.sun_path + 1, name);

        if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) != 0)
        {
            fprintf(stderr, "IPC client '%s' init failed: bind(): ", name);
            perror("");
            close(s);
            return NULL;
        }
    }

    ipcc = calloc(1, sizeof(struct ipc_client));
    if (ipcc == NULL)
    {
        close(s);
        return NULL;
    }

    ipcc->tmp_buffer = calloc(1, IPC_SEGMENT_SIZE);
    if (ipcc->tmp_buffer == NULL)
    {
        free(ipcc);
        close(s);
        return NULL;
    }

    strcpy(ipcc->name, name);
    ipcc->socket = s;
    ipcc->pool = NULL;
    TAILQ_INIT(&ipcc->datagrams);

    return ipcc;
}


/* See description in ipc_client.h */
int
ipc_send_message(struct ipc_client *ipcc, const char *server_name,
                 const void *msg, size_t msg_len)
{
    struct sockaddr_un          dst;
    struct ipc_packet_header   *ipch;
    size_t                      octets_sent;
    size_t                      segm_size;
    size_t                      ipc_msg_size;
    ssize_t                     r;
    unsigned int                retry = 0;


    KTRC("Send IPC msg from '%s' (thread 0x%x) to '%s' len=%u\n",
         ipcc->name, (unsigned int)pthread_self(), server_name, msg_len);
    if ((ipcc == NULL) || (server_name == NULL) ||
        (msg == NULL) != (msg_len == 0))
    {
        return EINVAL;
    }

    memset(&dst, 0, sizeof(dst));
    dst.sun_family = AF_UNIX;
    strcpy(dst.sun_path + 1, server_name);

    ipch = (struct ipc_packet_header *)(ipcc->tmp_buffer);
    memset(ipch, 0, sizeof(struct ipc_packet_header));
    ipch->length = msg_len;

    for (octets_sent = 0;
         (octets_sent < msg_len) || (msg_len == 0);
         octets_sent += segm_size, msg += segm_size)
    {
        segm_size = MIN(IPC_SEGMENT_SIZE - sizeof(struct ipc_packet_header),
                        msg_len - octets_sent);

        ipch->left = msg_len - octets_sent;

        memcpy(ipcc->tmp_buffer + sizeof(struct ipc_packet_header),
               msg, segm_size);

        ipc_msg_size = segm_size + sizeof(struct ipc_packet_header);

        do {
            r = sendto(ipcc->socket,
                       ipcc->tmp_buffer, ipc_msg_size,
                       MSG_DONTWAIT, SA(&dst), sizeof(dst));

        } while ((r < 0) &&
                 ((++retry) < IPC_CLIENT_RETRY_MAX) &&
                 (sleep(IPC_CLIENT_RETRY_TIMEOUT) == 0));
                
        if (r != (int)(ipc_msg_size))
        {
            fprintf(stderr, "IPC client '%s' failed to send message "
                            "to '%s': ", ipcc->name, server_name);
            perror("");
            return errno;
        }
        if (msg_len == 0)
        {
            break;
        }
    }

    return 0;

} /* ipc_send_message() */


/* See description in ipc_client.h */
int
ipc_receive_answer(struct ipc_client *ipcc, const char *server_name,
                   void *buf, size_t *p_buf_len)
{
    size_t                      octets_received;
    struct ipc_packet_header   *iph;
    struct ipc_client_server   *server;


    if ((ipcc == NULL) || (server_name == NULL) || (buf == NULL) ||
        (p_buf_len == NULL) || (*p_buf_len == 0))
    {
        return EINVAL;
    }
    if (strlen(server_name) >= UNIX_PATH_MAX)
    {
        return E2BIG;
    }

    server = get_pool_item_by_name(ipcc, server_name);
    if (server == NULL)
    {
        return errno;
    }

    get_datagram(ipcc, server);
    server->octets_received = 0;
    server->octets_returned = 0;


    iph = (struct ipc_packet_header*)server->buffer;

    if (iph->length != iph->left)
    {
#if IPC_CLIENT_DEBUG_REASSEMBLING
        printf("Segment not the first segment!!!\n");
#endif
        return ESYNCFAILED;
    }


    octets_received = server->fragment_size - sizeof(struct ipc_packet_header);

    assert(octets_received > 0);

#if IPC_CLIENT_DEBUG_REASSEMBLING
    {
        int i;

        printf("Recv: pld = %d, len = %d, left = %d, body: >|",
               octets_received, iph->length, iph->left);
        for (i = 0; i < octets_received; i++)
            printf("%c|", server->buffer[i + sizeof(*iph)]);

        printf("<\n");
    }
#endif

    /* Is it only one segment in the message? */
    if (iph->length == octets_received)
    {
        /*
         * Single-datagram message, process it in special way
         * to increase performance of the small messages.
         */
        if (iph->length <= *p_buf_len)
        {
            /* The full message can be returned */
            memcpy(buf,
                   server->buffer + sizeof(struct ipc_packet_header),
                   iph->length);

            *p_buf_len = iph->length;

            return 0;
        }
        else
        {
            /* Message can not fit into the user buffer, return part of it */
            memcpy(buf, server->buffer + sizeof(struct ipc_packet_header),
                   *p_buf_len);

            /* Do not touch *p_buf_len */

            /* Remember position in the buffer */
            server->octets_returned = *p_buf_len;

            /* Remember length of the message */
            server->length = iph->length;

            /* Remember number of octets received (= full length of the msg) */
            server->octets_received = octets_received;

            server->fragment_size = octets_received +
                                        sizeof(struct ipc_packet_header);

            *p_buf_len = server->length;

            return ETESMALLBUF;
        }
        /* Unreachable place */
    }

    /* Multi-datargams message, process it */
    {
        size_t total_octets_written = 0;
        size_t full_message_length = iph->length;

        server->octets_received = octets_received; /* Count received octets */

        while (TRUE)
        {
            /* Write portion of data to the user buffer */
            if (total_octets_written + octets_received <= *p_buf_len)
            {
                /* Segment can be written to the buffer */
                memcpy(buf,
                       server->buffer + sizeof(struct ipc_packet_header),
                       octets_received);

                /* Skip used space in the buffer */
                buf += octets_received;
                total_octets_written += octets_received;
            }
            else
            {
                /* Only a part of the segment can be written to the buffer */
                memcpy(buf,
                       server->buffer + sizeof(struct ipc_packet_header),
                       *p_buf_len - total_octets_written);

                /* Store position in the current segment */
                server->octets_returned = *p_buf_len - total_octets_written;

                /* Do not touch *p_buf_len */

                /* Remember length of the message */
                server->length = iph->length;

                server->fragment_size = octets_received +
                                        sizeof(struct ipc_packet_header);

                *p_buf_len = server->length;

                return ETESMALLBUF;
            }

            /* Is it all? */
            if (total_octets_written == full_message_length)
            {
                /* All done, whole message read */
                *p_buf_len = total_octets_written;
                return 0;
            }

             /* Is any space left? */
            if (total_octets_written == *p_buf_len)
            {
                /* No, do not read anymore */

                /* Store position in the current segment */
                server->octets_returned = 0;

                /* Remember length of the message */
                server->length = iph->length;

                /* Store fragment size */
                server->fragment_size = octets_received +
                                        sizeof(struct ipc_packet_header);

                *p_buf_len = server->length;
                return ETESMALLBUF;
            }

#if IPC_CLIENT_DEBUG_REASSEMBLING
            printf("%d <> %d\n", total_octets_written, server->length);
#endif
            assert(total_octets_written < iph->length);

            {
                size_t l = server->length;
                size_t l2 = ((struct ipc_packet_header*)server->buffer)->length;

                if (get_datagram(ipcc, server) == NULL)
                {
                    perror("get_datagram");
                    return errno;
                }

                if (server->length != l)
                {
#if IPC_CLIENT_DEBUG_REASSEMBLING
                    printf("On reading datagram size changed: %d/%d\n",
                           l, server->length);
#endif
                    return ESYNCFAILED;
                }
                if (l2 != ((struct ipc_packet_header*)server->buffer)->length)
                {
#if IPC_CLIENT_DEBUG_REASSEMBLING
                    printf("Header changed: %d != %d\n", l2,
                           ((struct ipc_packet_header*)server->buffer)->length);
                    printf("author: %s\n", server->sa.sun_path+1);
#endif
                    return ESYNCFAILED;
                }
            }

            octets_received = server->fragment_size -
                              sizeof(struct ipc_packet_header);

            assert(octets_received > 0);

#if IPC_CLIENT_DEBUG_REASSEMBLING
            {
                int i;

                printf("Recv2: pld = %d, len = %d, left = %d, body: >|",
                       octets_received, iph->length, iph->left);
                for (i = 0; i < octets_received; i++)
                    printf("%c|", server->buffer[i + sizeof(*iph)]);
                printf("<\n");
            }
#endif
            if (full_message_length != iph->length)
            {
#if IPC_CLIENT_DEBUG_REASSEMBLING
                perror("DIfferent length 1 of the single message!!");
                printf("%d != %d\n", full_message_length, iph->length);
#endif
                return ESYNCFAILED;
            }
            if (server->octets_received != iph->length - iph->left)
            {
#if IPC_CLIENT_DEBUG_REASSEMBLING
                printf("Packet reassembling failed (%d, %d, %d)",
                       (int)iph->length,
                       (int)server->octets_received,
                       (int)iph->left);
#endif
                return ESYNCFAILED;
            }
            server->octets_received += octets_received;
        }
    }
}


/* See description in ipc_client.h */
int
ipc_receive_rest_answer(struct ipc_client *ipcc, const char *server_name,
                        void *buf, size_t *p_buf_len)
{
    struct ipc_client_server *server;
    size_t total_octets_written = 0;

    if ((ipcc == NULL) || (server_name == NULL) || (buf == NULL) ||
        (p_buf_len == NULL) || (*p_buf_len == 0))
    {
        return EINVAL;
    }

    server = get_pool_item_by_name(ipcc, server_name);
    if (server == NULL)
    {
        return errno;
    }

#if IPC_CLIENT_DEBUG_REASSEMBLING
    printf("ipc_receive_rest_answer (%d)\n", server->octets_returned);
#endif

    if (server->length == 0)
    {
        perror("Nothing to receive rest");
        return ENOENT;
    }

#if IPC_CLIENT_DEBUG_REASSEMBLING
    printf("server->octets_returned = %d, server->length = %d,\n"
           "server->octets_received = %d\n", server->octets_returned,
           server->length, server->octets_received);
#endif

    /* First, write the rest of the last datagram */
    if (server->octets_returned)
    {
        size_t n;

        n = MIN(*p_buf_len, server->fragment_size - 
                                sizeof(struct ipc_packet_header) -
                                server->octets_returned);

        memcpy(buf,
               server->buffer + sizeof(struct ipc_packet_header) +
                   server->octets_returned,
               n);

        /* Is it all? */
        if (server->length == server->octets_received /* Last datargam */ &&
            server->fragment_size - sizeof(struct ipc_packet_header) -
                server->octets_returned <= *p_buf_len)
        {
            /* Yes. */
            *p_buf_len = n;
            return 0;
        }

        /* Is it any space? */
        if (*p_buf_len == n)
        {
            /* No. */

            /* Update position in the current segment */
            server->octets_returned += n;
            *p_buf_len = server->length;
            return ETESMALLBUF;
        }


        total_octets_written = n;
        buf += total_octets_written;
    }

#if IPC_CLIENT_DEBUG_REASSEMBLING
    printf("total_octets_written = %d\n", total_octets_written);
#endif

    {
        /* And now do the same as in function above */
        size_t octets_received = 0;
        struct ipc_packet_header *iph =
            (struct ipc_packet_header *)server->buffer;

        while (TRUE)
        {
            if(get_datagram(ipcc, server) == NULL)
            {
                perror("get_datagram");
                return errno;
            }

            octets_received = server->fragment_size -
            sizeof(struct ipc_packet_header);

            assert(octets_received > 0);

#if IPC_CLIENT_DEBUG_REASSEMBLING
            {
        int i;
        printf("Recv2: pld = %d, len = %d, left = %d, body: >|",
               octets_received, iph->length, iph->left);
        for (i = 0; i < octets_received; i++)
            printf("%c|", server->buffer[i + sizeof(*iph)]);

        printf("<\n");
            }
#endif
            if (server->length != iph->length)
            {
        perror("DIfferent length 2 of the single message!!");
        return ESYNCFAILED;
            }
            if (server->octets_received != iph->length - iph->left)
            {
#if IPC_CLIENT_DEBUG_REASSEMBLING
        printf("Packet reassembling failed (%d, %d, %d)",
               (int)iph->length,
               (int)server->octets_received,
               (int)iph->left);
#endif
        return ESYNCFAILED;
            }
            server->octets_received += octets_received;

            /* Write portion of data to the user buffer */
            if (total_octets_written + octets_received <= *p_buf_len)
            {
        /* Segment can be written to the buffer */
        memcpy(buf,
               server->buffer + sizeof(struct ipc_packet_header),
               octets_received);

        /* Skip used space in the buffer */
        buf += octets_received;
        total_octets_written += octets_received;

        /* Is it last segment? */
        if (octets_received == iph->left)
        {
            /* All done, whole message read */
            *p_buf_len = total_octets_written;
            server->length = 0;
            if (server->octets_received != iph->length)
            {
        return ESYNCFAILED;
            }
            return 0;
        }

        /* Is any space left? */
        if (total_octets_written == *p_buf_len)
        {
            /* No, do not read anymore */

            /* Store position in the current segment */
            server->octets_returned = 0;

            /* Remember length of the message */
            server->length = iph->length;

            /* Store fragment size */
            server->fragment_size = octets_received
        - sizeof(struct ipc_packet_header);

            *p_buf_len = server->length;

            return ETESMALLBUF;

        }
            }
            else
            {
        /* Only a part of the segment can be written to the buffer */
        memcpy(buf,
               server->buffer + sizeof(struct ipc_packet_header),
               *p_buf_len - total_octets_written);

        /* Store position in the current segment */
        server->octets_returned = *p_buf_len - total_octets_written;

        /* Do not touch *p_buf_len */
        server->fragment_size = octets_received +
            sizeof(struct ipc_packet_header);

        *p_buf_len = server->length;

        return ETESMALLBUF;
            }

#if IPC_CLIENT_DEBUG_REASSEMBLING
            printf("%d <> %d\n", total_octets_written, server->length);
#endif
            assert(total_octets_written < server->length);

        }
    }

}

/* See description in ipc_client.h */
int
ipc_close_client(struct ipc_client *ipcc)
{
    if (ipcc == NULL)
        return 0;

    if (close(ipcc->socket) < 0)
    {
        perror("close");
        return errno;
    }

    ipc_free_client_server_pool(ipcc);
    free(ipcc->tmp_buffer);
    free(ipcc);

    return 0;
}

#else

/* See description in ipc_client.h */
struct ipc_client *
ipc_init_client(const char *client_name)
{
    struct ipc_client *ipcc;

    UNUSED(client_name);

    ipcc = calloc(1, sizeof(struct ipc_client));
    if (ipcc == NULL)
        return NULL;

    ipcc->pool = NULL;

    if (IPC_TCP_CLIENT_BUFFER_SIZE != 0)
    {
        ipcc->out_buffer = calloc(1, IPC_TCP_CLIENT_BUFFER_SIZE);
        if (ipcc->out_buffer == NULL)
        {
            free(ipcc);
            return NULL;
        }
    }
    else
    {
        ipcc->out_buffer = 0;
    }

    return ipcc;
}

/* See description in ipc_client.h */
int
ipc_send_message(struct ipc_client *ipcc, const char *server_name,
                 const void *msg, size_t msg_len)
{
    struct ipc_client_server *server;

    if ((ipcc == NULL) || (server_name == NULL) ||
        (msg == NULL) != (msg_len == 0))
    {
        return EINVAL;
    }

    server = get_pool_item_by_name(ipcc, server_name);
    if (server == NULL)
    {
        return errno;
    }

    if (server->socket == 0)
    {
        /* Not connected yet. Let's connect to server  */
        int port = 0;

        /* We are trying to get server port in the endless loop */
        while (port == 0)
        {
            port = ipc_pmap_get_server(server_name);
        }

        /* Create socket */
        server->socket = socket(AF_INET, SOCK_STREAM, 0);

        if (server->socket < 0)
        {
            perror("socket() error");
            return errno;
        }

        /* Connect */
        {
            struct sockaddr_in sa;
            int tries = 0;

            sa.sin_family = AF_INET;
            sa.sin_port = port; /* Already in network byte order */
            sa.sin_addr.s_addr = inet_addr("127.0.0.1");

            while (connect(server->socket, (struct sockaddr*)&sa,
                           sizeof(sa)) != 0)
            {
                if (++tries == IPC_RETRY)
                {
                    perror("Can't connect to server");
                    return errno;
                }
                sleep(IPC_SLEEP);
            }
        }
#if IPC_CLIENT_DEBUG_TCP
        printf("Created connection with '%s', 0x%08X\n", server_name,
               (int)server);
#endif
    }

    /* At this point we have established connection. Send data */

    if (msg_len + 8 > IPC_TCP_CLIENT_BUFFER_SIZE)
    {
        /* Message is too long to fit into the internal buffer */
        uint32_t len = msg_len;

        if (write_socket(server->socket, (char *)&len, sizeof(len)) != 0)
        {
            perror("write_socket");
            return errno;
        }

        return write_socket(server->socket, msg, msg_len);
    }
    else
    {
        uint32_t len = msg_len;

        memcpy(ipcc->out_buffer, &len, sizeof(len));
        memcpy(ipcc->out_buffer + sizeof(len), msg, msg_len);
        return write_socket(server->socket, ipcc->out_buffer,
                            msg_len + sizeof(len));
    }
}

/* See description in ipc_client.h */
int
ipc_receive_answer(struct ipc_client *ipcc, const char *server_name,
                   void *buf, size_t *p_buf_len)
{
    struct ipc_client_server *server;

    if ((ipcc == NULL) || (server_name == NULL) || (buf == NULL) ||
        (p_buf_len == NULL) || (*p_buf_len == 0))
    {
        return EINVAL;
    }

    server = get_pool_item_by_name(ipcc, server_name);
    if (server == NULL)
    {
        return errno;
    }

    if (server->socket == 0)
    {
#if IPC_CLIENT_DEBUG_TCP
        printf("Socket not connected yet\n");
#endif
        return -1;
    }

    if (server->pending_octets)
    {
        /* Error! */
#if IPC_CLIENT_DEBUG_TCP
        printf("IPC/TCP: Trying to read new message while old one "
               "is not fully read!\n");
#endif
        return ESYNCFAILED;
    }

    /*
     * Let's read the length of the message and call
     * ipc_receive_rest_answer.
     */
    if (read_socket(server->socket,
            (char *)(void *)&server->pending_octets,
            sizeof(server->pending_octets)) != 0)
    {
        perror("read_socket");
        return errno;
    }
#if IPC_CLIENT_DEBUG_TCP
    printf("Read four octets: %d\n", server->pending_octets);
#endif

    return ipc_receive_rest_answer(ipcc, server_name, buf, p_buf_len);
}

/* See description in ipc_client.h */
int
ipc_receive_rest_answer(struct ipc_client *ipcc, const char *server_name,
                        void *buf, size_t *p_buf_len)
{
    struct ipc_client_server *server;
    int                       octets_to_read;

    if ((ipcc == NULL) || (server_name == NULL) || (buf == NULL) ||
        (p_buf_len == NULL) || (*p_buf_len == 0))
    {
        return EINVAL;
    }

    server = get_pool_item_by_name(ipcc, server_name);
    if (server == NULL)
    {
        return errno;
    }

    octets_to_read = MIN(*p_buf_len, server->pending_octets);

    if (octets_to_read)
    {
        /* If we have something to read */
        if (read_socket(server->socket, buf, octets_to_read) != 0)
        {
            /* Error occured */
            perror("read_socket");
            return errno;
        }
    }

    server->pending_octets -= octets_to_read;

    if (server->pending_octets)
    {
        *p_buf_len = server->pending_octets + octets_to_read;
        return ETESMALLBUF; /* Something to read more */
    }
    else
    {
        *p_buf_len = octets_to_read;
        return 0; /* Nothing to read */
    }
}

/* See description in ipc_client.h */
int
ipc_close_client(struct ipc_client *ipcc)
{
    if (ipcc == NULL)
        return 0;

    ipc_free_client_server_pool(ipcc);

    /* Free out_buffer */
    if (ipcc->out_buffer != NULL)
        free(ipcc->out_buffer);

    /* Free instance */
    free(ipcc);
    return 0;
}


/**
 * Connect to the my pmap server and obtain port number for specified server
 * name.
 *
 * @param       server_name     The name of the server.
 *
 * @return
 *      Port number, 0 on error.
 */
static unsigned short
ipc_pmap_get_server(const char *server_name)
{
    return ipc_pmap_process_command(IPC_PM_GET_SERVER, server_name, 0);
}

#endif


/* See description in ipc_client.h */
int
ipc_send_message_with_answer(struct ipc_client *ipcc,
                             const char *server_name,
                             const void *msg, size_t msg_len,
                             void *recv_buf, size_t *p_buf_len)
{
    int rc;

    rc = ipc_send_message(ipcc, server_name, msg, msg_len);
    if (rc != 0)
    {
        return rc;
    }

    rc = ipc_receive_answer(ipcc, server_name, recv_buf, p_buf_len);

    return rc;
}
