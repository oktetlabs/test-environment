/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief IPC library
 *
 * Implementation of routines (client side) provided for library user.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#include "te_config.h"

#include <stdio.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#endif
#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_ASSERT_H
#include <assert.h>
#endif
#if HAVE_ERRNO_H
#include <errno.h>
#endif
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#if HAVE_TIME_H
#include <time.h>
#endif

#include "ipc_internal.h"

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
#endif /* !TE_IPC_AF_UNIX */

#include "te_defs.h"
#include "te_errno.h"
#include "ipc_client.h"


/** @name IPC Client retry parameters */

/** Maximum number of retries for IPC client to send message */
#define IPC_CLIENT_RETRY_MAX        100

/** Timeout in milliseconds between IPC client retries to send message */
#define IPC_CLIENT_RETRY_TIMEOUT    300

/*@}*/


/** @name IPC Client debugging parameters */

/** Debug reassembling? */
#define IPC_CLIENT_DEBUG_REASSEMBLING 0

/** Debug pool usage? */
#define IPC_CLIENT_DEBUG_POOL 0

/** Debug TCP? */
#define IPC_CLIENT_DEBUG_TCP 0

/*@}*/


/** Prototype of the function to send IPC client message */
typedef int (*ipc_send)(struct ipc_client *ipcc,
                        const char *server_name,
                        const void *msg, size_t msg_len);

/** Prototype of the function to receive IPC client message answer */
typedef int (*ipc_recv)(struct ipc_client *ipcc,
                        const char *server_name,
                        void *buf, size_t *p_buf_len);


/** Structure to store information about the servers. */
struct ipc_client_server {
    /* TCP/UDP independent part */
    struct ipc_client_server *next; /**< Link to the next item in pool
                                         or NULL if this is last item */

#ifdef TE_IPC_AF_UNIX
    struct sockaddr_un  sa;         /**< Address of the server */
    socklen_t           sa_len;     /**< Length of the sockaddr_un struct */
#else
    char    name[UNIX_PATH_MAX];    /**< Name of the server */
#endif

    union {
        /* Connectionless client data */
        struct {
            char   *buffer;             /**< Buffer for datagram */
            size_t  length;             /**< Length of the currently
                                             receiving message,
                                             0 if none */
            size_t  octets_received;    /**< Number of octets of message
                                             currently received, no
                                             meaning if
                                             message_length == 0 */
            size_t  fragment_size;      /**< Number of octets in
                                             partial-returned datagram,
                                             including header */
            size_t  octets_returned;    /**< Number of octets of current
                                             segment returned to user,
                                             no meaning if
                                             message_length == 0. if
                                             octets_returned = 0
                                             whole datagram processed */
        } dgram;
        /* Connection-oriented client data */
        struct {
            int     socket;     /**< Socket */
            size_t  pending;    /**< Number of octets in current message
                                     left to read from the socket and
                                     to return to user.
                                     This field MUST be 4-octets long. */

        } stream;
    };
};

/** Storage of IPC client state information */
struct ipc_client {
    te_bool conn;                       /**< Is connection-oriented? */
    char    name[UNIX_PATH_MAX];        /**< IPC client name */

    struct ipc_client_server   *pool;   /**< Pool of the used servers */

    ipc_send    send;       /**< Function to send message */
    ipc_recv    recv;       /**< Function to receive answer */
    ipc_recv    recv_rest;  /**< Function to receive rest part of the
                                 answer */

    union {
        /* Connectionless client data */
        struct {
            int                     socket;     /**< Socket */
            struct ipc_datagrams    datagrams;  /**< Pool for datagrams */
            char                   *tmp_buffer; /**< Buffer for datagram */
        } dgram;
        /* Connection-oriented client data */
        struct {
            char   *out_buffer; /**< Buffer for outgoing messages */
        } stream;
    };
};


static int ipc_dgram_send_message(struct ipc_client *ipcc,
                                  const char *server_name,
                                  const void *msg, size_t msg_len);

static int ipc_dgram_receive_answer(struct ipc_client *ipcc,
                                    const char *server_name,
                                    void *buf, size_t *p_buf_len);

static int ipc_dgram_receive_rest_answer(struct ipc_client *ipcc,
                                         const char *server_name,
                                         void *buf, size_t *p_buf_len);

static int ipc_stream_send_message(struct ipc_client *ipcc,
                                   const char *server_name,
                                   const void *msg, size_t msg_len);

static int ipc_stream_receive_answer(struct ipc_client *ipcc,
                                     const char *server_name,
                                     void *buf, size_t *p_buf_len);

static int ipc_stream_receive_rest_answer(struct ipc_client *ipcc,
                                          const char *server_name,
                                          void *buf, size_t *p_buf_len);


#ifndef TE_IPC_AF_UNIX
/**
 * Connect to the my pmap server and obtain port number for specified
 * server name.
 *
 * @param server_name     The name of the server.
 *
 * @return Port number, 0 on error.
 */
static unsigned short
ipc_pmap_get_server(const char *server_name)
{
    return ipc_pmap_process_command(IPC_PM_GET_SERVER, server_name, 0);
}
#endif


/* See description in ipc_client.h */
const char *
ipc_client_name(const struct ipc_client *ipcc)
{
    return (ipcc == NULL) ? NULL : ipcc->name;
}


/**
 * Search in pool for the item with specified server name and return
 * pointer to this item. Allocate a new entry if entry not found.
 * If new entry is created, all fileds is set to zero expect sa,
 * sa_len and buffer.
 *
 * @param ipcc          Pointer to the ipcc structure.
 * @param sa_ptr        Pointer to the expected address.
 * @param sa_len        Length of the address.
 *
 * @return
 *    pointer to the item in the ipcc->pool pool
 */
static struct ipc_client_server *
get_pool_item_by_name(struct ipc_client *ipcc, const char *name)
{
    struct ipc_client_server   *ipccs;
    struct ipc_client_server  **parent;

    for (ipccs = ipcc->pool, parent = &ipcc->pool;
         ipccs != NULL;
         parent = &ipccs->next, ipccs = ipccs->next)
    {
#ifdef TE_IPC_AF_UNIX
        if (strcmp(ipccs->sa.sun_path + 1, name) == 0)
#else
        if (strcmp(ipccs->name, name) == 0)
#endif
        {
            /* Found */
            return ipccs;
        }
    }

    /* Not found. Allocate new */
    (*parent) = calloc(1, sizeof(struct ipc_client_server));
    if (*parent == NULL)
    {
        perror("get_pool_item_by_name(): calloc() failed");
        return NULL;
    }

#ifdef TE_IPC_AF_UNIX
    (*parent)->sa.sun_family = AF_UNIX;
    strcpy((*parent)->sa.sun_path + 1, name);
    (*parent)->sa_len = sizeof(struct sockaddr_un);
#else
    strcpy((*parent)->name, name);
#endif

    if (ipcc->conn)
    {
        (*parent)->stream.socket = -1;
    }
    else
    {
        (*parent)->dgram.buffer = calloc(1, IPC_SEGMENT_SIZE);
        if ((*parent)->dgram.buffer == NULL)
        {
            free(*parent);
            return NULL;
        }
    }

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
        if (ipcc->conn)
        {
            if (ipccs->stream.socket >= 0)
                close(ipccs->stream.socket);
        }
        else
        {
            free(ipccs->dgram.buffer);
        }
        free(ipccs);
        ipccs = tmp;
    }
    ipcc->pool = NULL;
}


/**
 * Write datagram to the ipcc->pool pool from the ipcc->dgram.datagrams
 * pool * or from the socket, this function may block.
 *
 * @param ipcc          Pointer to the ipc_client structure.
 * @param pool_item     Pointer to the ipc_client_server structure
 *                      if datagram from the specified client expected,
 *                      NULL otherwise.
 *
 * @return Pointer to the pool item on success, NULL on error.
 */
static struct ipc_client_server *
get_datagram(struct ipc_client *ipcc,
             struct ipc_client_server *pool_item)
{
    struct ipc_datagram *ptr;

    assert(ipcc != NULL);

    TAILQ_FOREACH(ptr, &ipcc->dgram.datagrams, links)
    {
        if (pool_item == NULL ||
            ((pool_item->sa_len == ptr->sa_len) &&
             (memcmp(&pool_item->sa, &ptr->sa, pool_item->sa_len) == 0)))
        {
            assert(ptr->octets <= IPC_SEGMENT_SIZE);

            TAILQ_REMOVE(&ipcc->dgram.datagrams, ptr, links);

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
            memcpy(pool_item->dgram.buffer, ptr->buffer, ptr->octets);

            /* Remember value to return */
            pool_item->dgram.fragment_size = ptr->octets;

            /* Free buffer */
            free(ptr->buffer);

            /* Free list item */
            free(ptr);

            return pool_item;
        }
    }

    /*
     * No datagram in the pool or datargarm with specified address not
     * found
     */
    if (pool_item == NULL) /* Datagram with any source address expected */
    {
        struct sockaddr_un  sa;
        socklen_t           sa_len = sizeof(sa);
        int                 r;

        r = recvfrom(ipcc->dgram.socket, ipcc->dgram.tmp_buffer,
                     IPC_SEGMENT_SIZE, 0,
                     (struct sockaddr*)&sa, &sa_len);

        if (r < 0)
        {
            perror("get_datagram(): recvfrom() error");
            return NULL;
        }

        /* Find or assign a new item in pool */
        pool_item = get_pool_item_by_name(ipcc, sa.sun_path + 1);
        if (pool_item == NULL)
        {
            return NULL;
        }

        pool_item->dgram.fragment_size = r;

        /* Swap buffers */
        {
            char *tmp = pool_item->dgram.buffer;

            pool_item->dgram.buffer = ipcc->dgram.tmp_buffer;
            ipcc->dgram.tmp_buffer = tmp;
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

            r = recvfrom(ipcc->dgram.socket,
                         pool_item->dgram.buffer, IPC_SEGMENT_SIZE,
                         0 /* flags */,
                         (struct sockaddr*)&sa, &sa_len);

            if (r < 0)
            {
                perror("get_datagram(): recvfrom() error");
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
                ipc_remember_datagram(&ipcc->dgram.datagrams,
                                      pool_item->dgram.buffer, r,
                                      &sa, sa_len);
                pool_item->dgram.buffer = calloc(1, IPC_SEGMENT_SIZE);
                if (pool_item->dgram.buffer == NULL)
                    return NULL;
            }
            else
            {
                /* This datagram is what we expecting */
                pool_item->dgram.fragment_size = r;
                return pool_item;
            }
        }
    }

    /* Unreachable place */
    assert(0);
}

/**
 * Read specified number of octets (not less) from the connection.
 *
 * @param socket        Connection socket.
 * @param buffer        Buffer to store the data.
 * @param len           Number of octets to read.
 *
 * @return Status code.
 *
 * @retval 0            Success.
 * @retval other errno  errno.
 */
static int
read_socket(int socket, void *buffer, size_t len)
{
#if IPC_CLIENT_DEBUG_TCP
    printf("Reading %d octets...\n", len);
#endif
    while (len > 0)
    {
        int r = recv(socket, buffer, len, 0);

        if (r < 0)
        {
            te_errno rc = TE_OS_RC(TE_IPC, errno);

            /* ECONNRESET errno is set when server closes its socket */
            if (TE_RC_GET_ERROR(rc) != TE_ECONNRESET)
                perror("read_socket(): recv() error");
            return rc;
        }
        else if (r == 0)
        {
            return TE_RC(TE_IPC, TE_ECONNRESET);
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
            /*
             * Encountering EPIPE is part of normal operation
             * for some TE components
             */
            if (errno != EPIPE)
                perror("write_socket(): send() error");
            return TE_OS_RC(TE_IPC, errno);
        }

        len -= r;
        buffer += r;
    }

#if IPC_CLIENT_DEBUG_TCP
    printf("Sent\n");
#endif
    return 0;
}


/* See description in ipc_client.h */
int
ipc_init_client(const char *name, te_bool conn,
                struct ipc_client **p_client)
{
    struct ipc_client  *ipcc;
    int                 rc;
    int                 s = -1;

    if ((name == NULL) || (p_client == NULL))
    {
        if (p_client != NULL)
           *p_client = NULL;
        return TE_RC(TE_IPC, TE_EINVAL);
    }
    *p_client = NULL;

    /* Check length of the name */
    if (strlen(name) >= UNIX_PATH_MAX)
    {
        return TE_RC(TE_IPC, TE_E2BIG);
    }

    if (!conn)
    {
        /* Create a socket */
        s = socket(
#ifdef TE_IPC_AF_UNIX
                   PF_UNIX,
#else
                   PF_INET,
#endif
                   SOCK_DGRAM, 0);
        if (s < 0)
        {
            rc = errno;
            perror("ipc_init_client(): socket() error");
            return TE_OS_RC(TE_IPC, rc);
        }

#ifdef TE_IPC_AF_UNIX
        {
            int optval = 1;
            if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR,
                           &optval, sizeof(optval)) != 0)
            {
                rc = errno;
                fprintf(stderr, "IPC client '%s' init failed: "
                        "setsockopt(): %s\n",
                        name, strerror(rc));
                return TE_OS_RC(TE_IPC, rc);
            }
        }

        /* Assign a name to socket */
        {
            time_t now;
            struct sockaddr_un addr;

            addr.sun_family = AF_UNIX;
            memset(addr.sun_path, 0, UNIX_PATH_MAX);
            now = time(NULL);
            snprintf(addr.sun_path + 1, UNIX_PATH_MAX - 1,
                     "%s_%ld", name, now);

            if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) != 0)
            {
                rc = errno;
                fprintf(stderr, "IPC client '%s' init failed: bind(): %s\n",
                        name, strerror(rc));
                close(s);
                return TE_OS_RC(TE_IPC, rc);
            }
        }
#endif /* TE_IPC_AF_UNIX */
    }

    ipcc = calloc(1, sizeof(struct ipc_client));
    if (ipcc == NULL)
    {
        rc = errno;
        if (s >= 0)
            close(s);
        return TE_RC(TE_IPC, rc);
    }
    strcpy(ipcc->name, name);
    ipcc->pool = NULL;
    ipcc->conn = conn;

    if (!conn)
    {
        ipcc->dgram.socket = s;
        ipcc->dgram.tmp_buffer = calloc(1, IPC_SEGMENT_SIZE);
        if (ipcc->dgram.tmp_buffer == NULL)
        {
            rc = errno;
            free(ipcc);
            close(s);
            return TE_OS_RC(TE_IPC, rc);
        }

        TAILQ_INIT(&ipcc->dgram.datagrams);

        ipcc->send = ipc_dgram_send_message;
        ipcc->recv = ipc_dgram_receive_answer;
        ipcc->recv_rest = ipc_dgram_receive_rest_answer;
    }
    else
    {
        if (IPC_TCP_CLIENT_BUFFER_SIZE != 0)
        {
            ipcc->stream.out_buffer = calloc(1, IPC_TCP_CLIENT_BUFFER_SIZE);
            if (ipcc->stream.out_buffer == NULL)
            {
                free(ipcc);
                return TE_OS_RC(TE_IPC, errno);
            }
        }
        else
        {
            ipcc->stream.out_buffer = NULL;
        }

        ipcc->send = ipc_stream_send_message;
        ipcc->recv = ipc_stream_receive_answer;
        ipcc->recv_rest = ipc_stream_receive_rest_answer;
    }

    *p_client = ipcc;

    return 0;
}

/* See description in ipc_client.h */
int
ipc_close_client(struct ipc_client *ipcc)
{
    if (ipcc == NULL)
        return 0;

    ipc_free_client_server_pool(ipcc);

    if (ipcc->conn)
    {
        free(ipcc->stream.out_buffer);
    }
    else
    {
        if (ipcc->dgram.socket >= 0 && close(ipcc->dgram.socket) < 0)
        {
            fprintf(stderr, "%s(): close() failed: %s",
                    __FUNCTION__, strerror(errno));
            return TE_OS_RC(TE_IPC, errno);
        }
        free(ipcc->dgram.tmp_buffer);
    }

    free(ipcc);

    return 0;
}


/* See description in ipc_client.h */
static int
ipc_dgram_send_message(struct ipc_client *ipcc, const char *server_name,
                       const void *msg, size_t msg_len)
{
    struct sockaddr_un          dst;
    struct ipc_dgram_header   *ipch;
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
        return TE_RC(TE_IPC, TE_EINVAL);
    }

    memset(&dst, 0, sizeof(dst));
    dst.sun_family = AF_UNIX;
    strcpy(dst.sun_path + 1, server_name);

    ipch = (struct ipc_dgram_header *)(ipcc->dgram.tmp_buffer);
    memset(ipch, 0, sizeof(struct ipc_dgram_header));
    ipch->length = msg_len;

    for (octets_sent = 0;
         (octets_sent < msg_len) || (msg_len == 0);
         octets_sent += segm_size, msg += segm_size)
    {
        segm_size = MIN(IPC_SEGMENT_SIZE - sizeof(struct ipc_dgram_header),
                        msg_len - octets_sent);

        ipch->left = msg_len - octets_sent;

        memcpy(ipcc->dgram.tmp_buffer + sizeof(struct ipc_dgram_header),
               msg, segm_size);

        ipc_msg_size = segm_size + sizeof(struct ipc_dgram_header);

        do {
            r = sendto(ipcc->dgram.socket,
                       ipcc->dgram.tmp_buffer, ipc_msg_size,
                       0 /* flags */, SA(&dst), sizeof(dst));

            if (r < 0)
            {
                if (retry > IPC_CLIENT_RETRY_MAX)
                    break;
                retry++;
                usleep(IPC_CLIENT_RETRY_TIMEOUT);
            }
            else
                retry = 0;
        } while (r < 0);

        if (r != (ssize_t)(ipc_msg_size))
        {
            fprintf(stderr, "IPC client '%s' failed to send message "
                            "of the length %u (sent %d) to '%s': %s(%d)\n",
                            ipcc->name, (unsigned)ipc_msg_size, (int)r,
                            server_name, strerror(errno), errno);
            return TE_OS_RC(TE_IPC, errno);
        }
        if (msg_len == 0)
        {
            break;
        }
    }

    return 0;

} /* ipc_send_message() */


/* See description in ipc_client.h */
static int
ipc_dgram_receive_answer(struct ipc_client *ipcc, const char *server_name,
                         void *buf, size_t *p_buf_len)
{
    size_t                      octets_received;
    struct ipc_dgram_header   *iph;
    struct ipc_client_server   *server;


    if ((ipcc == NULL) || (server_name == NULL) || (p_buf_len == NULL) ||
        ((buf == NULL) && (*p_buf_len != 0)))
    {
        return TE_RC(TE_IPC, TE_EINVAL);
    }
    if (strlen(server_name) >= UNIX_PATH_MAX)
    {
        return TE_RC(TE_IPC, TE_E2BIG);
    }

    server = get_pool_item_by_name(ipcc, server_name);
    if (server == NULL)
    {
        return TE_OS_RC(TE_IPC, errno);
    }

    get_datagram(ipcc, server);
    server->dgram.octets_received = 0;
    server->dgram.octets_returned = 0;


    iph = (struct ipc_dgram_header*)server->dgram.buffer;

    if (iph->length != iph->left)
    {
#if IPC_CLIENT_DEBUG_REASSEMBLING
        printf("Segment not the first segment!!!\n");
#endif
        return TE_RC(TE_IPC, TE_ESYNCFAILED);
    }


    octets_received =
        server->dgram.fragment_size - sizeof(struct ipc_dgram_header);

    assert(octets_received > 0);

#if IPC_CLIENT_DEBUG_REASSEMBLING
    {
        int i;

        printf("Recv: pld = %d, len = %d, left = %d, body: >|",
               octets_received, iph->length, iph->left);
        for (i = 0; i < octets_received; i++)
            printf("%c|", server->dgram.buffer[i + sizeof(*iph)]);

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
                   server->dgram.buffer + sizeof(struct ipc_dgram_header),
                   iph->length);

            *p_buf_len = iph->length;

            return 0;
        }
        else
        {
            /*
             * Message can not fit into the user buffer, return part of it.
             */
            memcpy(buf,
                   server->dgram.buffer + sizeof(struct ipc_dgram_header),
                   *p_buf_len);

            /* Do not touch *p_buf_len */

            /* Remember position in the buffer */
            server->dgram.octets_returned = *p_buf_len;

            /* Remember length of the message */
            server->dgram.length = iph->length;

            /*
             * Remember number of octets received (= full length
             * of the msg)
             */
            server->dgram.octets_received = octets_received;

            server->dgram.fragment_size =
                octets_received + sizeof(struct ipc_dgram_header);

            *p_buf_len = server->dgram.length;

            return TE_RC(TE_IPC, TE_ESMALLBUF);
        }
        /* Unreachable place */
    }

    /* Multi-datargams message, process it */
    {
        size_t total_octets_written = 0;
        size_t full_message_length = iph->length;

        /* Count received octets */
        server->dgram.octets_received = octets_received;

        while (TRUE)
        {
            /* Write portion of data to the user buffer */
            if (total_octets_written + octets_received <= *p_buf_len)
            {
                /* Segment can be written to the buffer */
                memcpy(buf,
                       server->dgram.buffer +
                           sizeof(struct ipc_dgram_header),
                       octets_received);

                /* Skip used space in the buffer */
                buf += octets_received;
                total_octets_written += octets_received;
            }
            else
            {
                /*
                 * Only a part of the segment can be written to the buffer.
                 */
                memcpy(buf,
                       server->dgram.buffer +
                           sizeof(struct ipc_dgram_header),
                       *p_buf_len - total_octets_written);

                /* Store position in the current segment */
                server->dgram.octets_returned =
                    *p_buf_len - total_octets_written;

                /* Do not touch *p_buf_len */

                /* Remember length of the message */
                server->dgram.length = iph->length;

                server->dgram.fragment_size =
                    octets_received + sizeof(struct ipc_dgram_header);

                *p_buf_len = server->dgram.length;

                return TE_RC(TE_IPC, TE_ESMALLBUF);
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
                server->dgram.octets_returned = 0;

                /* Remember length of the message */
                server->dgram.length = iph->length;

                /* Store fragment size */
                server->dgram.fragment_size =
                    octets_received + sizeof(struct ipc_dgram_header);

                *p_buf_len = server->dgram.length;
                return TE_RC(TE_IPC, TE_ESMALLBUF);
            }

#if IPC_CLIENT_DEBUG_REASSEMBLING
            printf("%d <> %d\n",
                   total_octets_written, server->dgram.length);
#endif
            assert(total_octets_written < iph->length);

            {
                size_t l = server->dgram.length;
                size_t l2 = ((struct ipc_dgram_header*)
                                 server->dgram.buffer)->length;

                if (get_datagram(ipcc, server) == NULL)
                {
                    return TE_OS_RC(TE_IPC, errno);
                }

                if (server->dgram.length != l)
                {
#if IPC_CLIENT_DEBUG_REASSEMBLING
                    printf("On reading datagram size changed: %d/%d\n",
                           l, server->dgram.length);
#endif
                    return TE_RC(TE_IPC, TE_ESYNCFAILED);
                }
                if (l2 != ((struct ipc_dgram_header*)
                               server->dgram.buffer)->length)
                {
#if IPC_CLIENT_DEBUG_REASSEMBLING
                    printf("Header changed: %d != %d\n", l2,
                           ((struct ipc_dgram_header*)server->
                                dgram.buffer)->length);
                    printf("author: %s\n", server->sa.sun_path+1);
#endif
                    return TE_RC(TE_IPC, TE_ESYNCFAILED);
                }
            }

            octets_received = server->dgram.fragment_size -
                              sizeof(struct ipc_dgram_header);

            assert(octets_received > 0);

#if IPC_CLIENT_DEBUG_REASSEMBLING
            {
                int i;

                printf("Recv2: pld = %d, len = %d, left = %d, body: >|",
                       octets_received, iph->length, iph->left);
                for (i = 0; i < octets_received; i++)
                    printf("%c|", server->dgram.buffer[i + sizeof(*iph)]);
                printf("<\n");
            }
#endif
            if (full_message_length != iph->length)
            {
#if IPC_CLIENT_DEBUG_REASSEMBLING
                perror("Different length 1 of the single message!!");
                printf("%d != %d\n", full_message_length, iph->length);
#endif
                return TE_RC(TE_IPC, TE_ESYNCFAILED);
            }
            if (server->dgram.octets_received != iph->length - iph->left)
            {
#if IPC_CLIENT_DEBUG_REASSEMBLING
                printf("Packet reassembling failed (%d, %d, %d)",
                       (int)iph->length,
                       (int)server->dgram.octets_received,
                       (int)iph->left);
#endif
                return TE_RC(TE_IPC, TE_ESYNCFAILED);
            }
            server->dgram.octets_received += octets_received;
        }
    }
}


/* See description in ipc_client.h */
static int
ipc_dgram_receive_rest_answer(struct ipc_client *ipcc,
                              const char *server_name,
                              void *buf, size_t *p_buf_len)
{
    struct ipc_client_server *server;
    size_t total_octets_written = 0;

    if ((ipcc == NULL) || (server_name == NULL) || (buf == NULL) ||
        (p_buf_len == NULL) || (*p_buf_len == 0))
    {
        return TE_RC(TE_IPC, TE_EINVAL);
    }

    server = get_pool_item_by_name(ipcc, server_name);
    if (server == NULL)
    {
        return TE_OS_RC(TE_IPC, errno);
    }

#if IPC_CLIENT_DEBUG_REASSEMBLING
    printf("ipc_receive_rest_answer (%d)\n", server->dgram.octets_returned);
#endif

    if (server->dgram.length == 0)
    {
        perror("Nothing to receive rest");
        return TE_RC(TE_IPC, TE_ENOENT);
    }

#if IPC_CLIENT_DEBUG_REASSEMBLING
    printf("server->octets_returned = %d, server->length = %d,\n"
           "server->octets_received = %d\n", server->dgram.octets_returned,
           server->dgram.length, server->dgram.octets_received);
#endif

    /* First, write the rest of the last datagram */
    if (server->dgram.octets_returned)
    {
        size_t n;

        n = MIN(*p_buf_len, server->dgram.fragment_size -
                                sizeof(struct ipc_dgram_header) -
                                server->dgram.octets_returned);

        memcpy(buf,
               server->dgram.buffer + sizeof(struct ipc_dgram_header) +
                   server->dgram.octets_returned,
               n);

        /* Is it all? */
        if (/* Last datargam */
            server->dgram.length == server->dgram.octets_received
            &&
            server->dgram.fragment_size - sizeof(struct ipc_dgram_header) -
                server->dgram.octets_returned <= *p_buf_len)
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
            server->dgram.octets_returned += n;
            *p_buf_len = server->dgram.length;
            return TE_RC(TE_IPC, TE_ESMALLBUF);
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
        struct ipc_dgram_header *iph =
            (struct ipc_dgram_header *)server->dgram.buffer;

        while (TRUE)
        {
            if(get_datagram(ipcc, server) == NULL)
            {
                return TE_OS_RC(TE_IPC, errno);
            }

            octets_received = server->dgram.fragment_size -
                              sizeof(struct ipc_dgram_header);

            assert(octets_received > 0);

#if IPC_CLIENT_DEBUG_REASSEMBLING
            {
        int i;
        printf("Recv2: pld = %d, len = %d, left = %d, body: >|",
               octets_received, iph->length, iph->left);
        for (i = 0; i < octets_received; i++)
            printf("%c|", server->dgram.buffer[i + sizeof(*iph)]);

        printf("<\n");
            }
#endif
            if (server->dgram.length != iph->length)
            {
        perror("Different length 2 of the single message!!");
        return TE_RC(TE_IPC, TE_ESYNCFAILED);
            }
            if (server->dgram.octets_received != iph->length - iph->left)
            {
#if IPC_CLIENT_DEBUG_REASSEMBLING
        printf("Packet reassembling failed (%d, %d, %d)",
               (int)iph->length,
               (int)server->dgram.octets_received,
               (int)iph->left);
#endif
        return TE_RC(TE_IPC, TE_ESYNCFAILED);
            }
            server->dgram.octets_received += octets_received;

            /* Write portion of data to the user buffer */
            if (total_octets_written + octets_received <= *p_buf_len)
            {
        /* Segment can be written to the buffer */
        memcpy(buf,
               server->dgram.buffer + sizeof(struct ipc_dgram_header),
               octets_received);

        /* Skip used space in the buffer */
        buf += octets_received;
        total_octets_written += octets_received;

        /* Is it last segment? */
        if (octets_received == iph->left)
        {
            /* All done, whole message read */
            *p_buf_len = total_octets_written;
            server->dgram.length = 0;
            if (server->dgram.octets_received != iph->length)
            {
                return TE_RC(TE_IPC, TE_ESYNCFAILED);
            }
            return 0;
        }

        /* Is any space left? */
        if (total_octets_written == *p_buf_len)
        {
            /* No, do not read anymore */

            /* Store position in the current segment */
            server->dgram.octets_returned = 0;

            /* Remember length of the message */
            server->dgram.length = iph->length;

            /* Store fragment size */
            server->dgram.fragment_size =
                octets_received - sizeof(struct ipc_dgram_header);

            *p_buf_len = server->dgram.length;

            return TE_RC(TE_IPC, TE_ESMALLBUF);

        }
            }
            else
            {
        /* Only a part of the segment can be written to the buffer */
        memcpy(buf,
               server->dgram.buffer + sizeof(struct ipc_dgram_header),
               *p_buf_len - total_octets_written);

        /* Store position in the current segment */
        server->dgram.octets_returned = *p_buf_len - total_octets_written;

        /* Do not touch *p_buf_len */
        server->dgram.fragment_size =
            octets_received + sizeof(struct ipc_dgram_header);

        *p_buf_len = server->dgram.length;

        return TE_RC(TE_IPC, TE_ESMALLBUF);
            }

#if IPC_CLIENT_DEBUG_REASSEMBLING
            printf("%d <> %d\n",
                   total_octets_written, server->dgram.length);
#endif
            assert(total_octets_written < server->dgram.length);

        }
    }

}


/* See description in ipc_client.h */
static int
ipc_stream_send_message(struct ipc_client *ipcc, const char *server_name,
                        const void *msg, size_t msg_len)
{
    struct ipc_client_server *server;

    if ((ipcc == NULL) || (server_name == NULL) ||
        (msg == NULL) != (msg_len == 0))
    {
        return TE_RC(TE_IPC, TE_EINVAL);
    }

    server = get_pool_item_by_name(ipcc, server_name);
    if (server == NULL)
    {
        return TE_OS_RC(TE_IPC, errno);
    }

    if (server->stream.socket == -1)
    {
#ifndef TE_IPC_AF_UNIX
        /* Not connected yet. Let's connect to server  */
        int port = 0;

        /* We are trying to get server port in the endless loop */
        while (port == 0)
        {
            port = ipc_pmap_get_server(server_name);
        }
#endif

        /* Create socket */
        server->stream.socket = socket(
#ifdef TE_IPC_AF_UNIX
                                       AF_UNIX,
#else
                                       AF_INET,
#endif
                                       SOCK_STREAM, 0);
        if (server->stream.socket < 0)
        {
            perror("ipc_send_message(): socket() error");
            return TE_OS_RC(TE_IPC, errno);
        }

        /* Connect */
        {
            int tries = 0;

#ifndef TE_IPC_AF_UNIX
            struct sockaddr_in sa;

            sa.sin_family = AF_INET;
            sa.sin_port = port; /* Already in network byte order */
            sa.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
#endif /* !TE_IPC_AF_UNIX */

            while (connect(server->stream.socket,
#ifdef TE_IPC_AF_UNIX
                           SA(&server->sa), server->sa_len
#else
                           SA(&sa), sizeof(sa)
#endif
                           ) != 0)
            {
                if (++tries == IPC_RETRY)
                {
                    perror("ipc_send_message(): Can't connect to server");
                    return TE_OS_RC(TE_IPC, errno);
                }
                sleep(IPC_SLEEP);
            }
        }
    }

    /* At this point we have established connection. Send data */

    if (msg_len + 8 > IPC_TCP_CLIENT_BUFFER_SIZE)
    {
        /* Message is too long to fit into the internal buffer */
        size_t len = msg_len;

        if (write_socket(server->stream.socket,
                         (char *)&len, sizeof(len)) != 0)
        {
            /*
             * Encountering EPIPE is part of normal operation
             * for some TE components
             */
            if (errno != EPIPE)
                perror("ipc_send_message(): write_socket()");
            return TE_OS_RC(TE_IPC, errno);
        }

        return write_socket(server->stream.socket, msg, msg_len);
    }
    else
    {
        size_t len = msg_len;

        memcpy(ipcc->stream.out_buffer, &len, sizeof(len));
        memcpy(ipcc->stream.out_buffer + sizeof(len), msg, msg_len);
        return write_socket(server->stream.socket,
                            ipcc->stream.out_buffer,
                            msg_len + sizeof(len));
    }
}


/**
 * Receive answer from server.
 *
 * @param server        The server
 * @param buf           Buffer for answer data
 * @param p_buf_len     Location of the buffer size
 *
 * @return Status code.
 */
static int
ipc_client_int_receive(struct ipc_client_server *server,
                       void *buf, size_t *p_buf_len)
{
    size_t octets_to_read;

    assert(server != NULL);
    assert(p_buf_len != NULL);
    assert(buf != NULL || *p_buf_len == 0);

    octets_to_read = MIN(*p_buf_len, server->stream.pending);
    if (octets_to_read > 0)
    {
        int rc = read_socket(server->stream.socket, buf, octets_to_read);

        /* If we have something to read */
        if (rc != 0)
        {
            /* Error occured */
            if (TE_RC_GET_ERROR(rc) != TE_ECONNRESET)
                fprintf(stderr, "%s(): read_socket() failed: %s\n",
                                __FUNCTION__, te_rc_err2str(rc));
            return TE_RC(TE_IPC, rc);
        }
    }

    server->stream.pending -= octets_to_read;
    if (server->stream.pending > 0)
    {
        *p_buf_len = server->stream.pending + octets_to_read;
        return TE_RC(TE_IPC, TE_ESMALLBUF);
    }
    else
    {
        *p_buf_len = octets_to_read;
        return 0;
    }
}

/* See description in ipc_client.h */
static int
ipc_stream_receive_answer(struct ipc_client *ipcc, const char *server_name,
                          void *buf, size_t *p_buf_len)
{
    struct ipc_client_server *server;
    int                       rc;

    if ((ipcc == NULL) || (server_name == NULL) || (p_buf_len == NULL) ||
        ((buf == NULL) && (*p_buf_len != 0)))
    {
        return TE_RC(TE_IPC, TE_EINVAL);
    }

    server = get_pool_item_by_name(ipcc, server_name);
    if (server == NULL)
    {
        return TE_OS_RC(TE_IPC, errno);
    }
    if (server->stream.socket == -1)
    {
        return TE_RC(TE_IPC, TE_EINVAL); /* FIXME */
    }

    if (server->stream.pending != 0)
    {
        return TE_RC(TE_IPC, TE_ESYNCFAILED);
    }

    /*
     * Let's read the length of the message and call
     * ipc_receive_rest_answer.
     */
    rc = read_socket(server->stream.socket, &server->stream.pending,
                     sizeof(server->stream.pending));
    if (rc != 0)
    {
        /* ECONNRESET errno is set when server closes its socket */
        if (TE_RC_GET_ERROR(rc) != TE_ECONNRESET)
            fprintf(stderr, "%s(): client '%s' read_socket() failed: "
                            "%s\n", __FUNCTION__, ipcc->name,
                            te_rc_err2str(rc));
        return TE_RC(TE_IPC, rc);
    }

    return ipc_client_int_receive(server, buf, p_buf_len);
}

/* See description in ipc_client.h */
static int
ipc_stream_receive_rest_answer(struct ipc_client *ipcc,
                               const char *server_name,
                               void *buf, size_t *p_buf_len)
{
    struct ipc_client_server *server;

    if ((ipcc == NULL) || (server_name == NULL) || (buf == NULL) ||
        (p_buf_len == NULL))
    {
        return TE_RC(TE_IPC, TE_EINVAL);
    }

    server = get_pool_item_by_name(ipcc, server_name);
    if (server == NULL)
    {
        return TE_OS_RC(TE_IPC, errno);
    }
    if (server->stream.socket == -1)
    {
        return TE_RC(TE_IPC, TE_EINVAL); /* FIXME */
    }

    return ipc_client_int_receive(server, buf, p_buf_len);
}


/* See description in ipc_client.h */
int
ipc_send_message(struct ipc_client *ipcc, const char *server_name,
                 const void *msg, size_t msg_len)
{
    return ipcc->send(ipcc, server_name, msg, msg_len);
}


/* See description in ipc_client.h */
int
ipc_receive_answer(struct ipc_client *ipcc, const char *server_name,
                   void *buf, size_t *p_buf_len)
{
    return ipcc->recv(ipcc, server_name, buf, p_buf_len);
}

/* See description in ipc_client.h */
int
ipc_receive_rest_answer(struct ipc_client *ipcc, const char *server_name,
                        void *buf, size_t *p_buf_len)
{
    return ipcc->recv_rest(ipcc, server_name, buf, p_buf_len);
}

/* See description in ipc_client.h */
int
ipc_send_message_with_answer(struct ipc_client *ipcc,
                             const char *server_name,
                             const void *msg, size_t msg_len,
                             void *recv_buf, size_t *p_buf_len)
{
    int rc;

    rc = ipcc->send(ipcc, server_name, msg, msg_len);
    if (rc != 0)
    {
        return rc;
    }

    rc = ipcc->recv(ipcc, server_name, recv_buf, p_buf_len);

    return rc;
}
