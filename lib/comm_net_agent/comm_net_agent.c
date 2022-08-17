/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Test Environment:
 *
 * Communication library - Test Agent side
 * Implementation of routines provided for library user
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#include "te_config.h"

#include <stdio.h>
#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#endif
#if HAVE_STRINGS_H
#include <strings.h>
#endif
#if HAVE_LIMITS_H
#include <limits.h>
#endif
#if HAVE_CTYPE_H
#include <ctype.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_ASSERT_H
#include <assert.h>
#endif
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#if  HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#if HAVE_NETINET_TCP_H
#include <netinet/tcp.h>
#endif
#if HAVE_FCNTL_H
#include <fcntl.h>
#endif

#include "te_errno.h"
#include "comm_agent.h"


/**
 * Error logging macro (TE logging facilities cannot be used here).
 *
 * When perror() is used here, it causes TA hang up inside perror() on
 * fclose() call.
 */
#define ERROR(_fmt...) \
    do {                        \
        fprintf(stderr, _fmt);  \
        fflush(stderr);         \
    } while (0)


/** This structure is used to store some context for each connection. */
struct rcf_comm_connection {
    int     socket;          /**< Connection socket */
    size_t  bytes_to_read;   /**< Number of bytes of attachment to read */
};


/* Static function declaration. See implementation for comments */
static int find_attach(char *buf, size_t len);
static int read_socket(int socket, void *buffer, size_t len);

/* See description in comm_agent.h */
te_errno
rcf_comm_agent_create_listener(int port, int *listener)
{
    struct sockaddr_in addr;
    int                optval = 1;
    int                s;
    int                rc;

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0)
    {
        ERROR("%s(): failed to create a socket, errno=%d ('%s')",
              __FUNCTION__, errno, strerror(errno));
        return TE_OS_RC(TE_COMM, errno);
    }

    rc = setsockopt(s, SOL_SOCKET, SO_REUSEADDR,
                    &optval, sizeof(optval));
    if (rc < 0)
    {
        ERROR("%s(): setsockopt(SO_REUSEADDR) failed, errno=%d ('%s')",
              __FUNCTION__, errno, strerror(errno));
        close(s);
        return TE_OS_RC(TE_COMM, errno);
    }

    rc = bind(s, (struct sockaddr *)&addr, sizeof(addr));
    if (rc < 0)
    {
        ERROR("%s(): failed to bind a socket, errno=%d ('%s')",
              __FUNCTION__, errno, strerror(errno));
        close(s);
        return TE_OS_RC(TE_COMM, errno);
    }

    rc = listen(s, 5);
    if (rc < 0)
    {
        ERROR("%s(): failed to listen(), errno=%d ('%s')",
              __FUNCTION__, errno, strerror(errno));
        close(s);
        return TE_OS_RC(TE_COMM, errno);
    }

    *listener = s;
    return 0;
}

/**
 * Wait for incoming connection from the Test Engine side of the
 * Communication library.
 *
 * @param config_str    Configuration string, content depends on the
 *                      communication type (network, serial, etc)
 * @param p_rcc         Pointer to to pointer the rcf_comm_connection
 *                      structure to be filled, used as handler.
 * @return Status code.
 * @retval 0            Success.
 * @retval other value  errno.
 */
int
rcf_comm_agent_init(const char *config_str,
                    struct rcf_comm_connection **p_rcc)
{
    int                 s = -1;
    int                 s1 = -1;
    te_errno            rc;
    int                 optval = 1;

    char               *env_val = NULL;
    char               *endptr = NULL;
    long int            env_num;

    /*
     * Listener socket may be created before exec(ta).
     * This is useful when TA is run in a different network namespace
     * to which RCF cannot connect directly.
     */
    env_val = getenv("TE_TA_RCF_LISTENER");
    if (env_val != NULL)
    {
        env_num = strtol(env_val, &endptr, 10);
        if (*endptr != '\0' || env_num < 0 || env_num > INT_MAX)
        {
            ERROR("Failed to convert TE_TA_RCF_LISTENER='%s' into "
                  "correct socket descriptor", env_val);
            return TE_RC(TE_COMM, TE_EINVAL);
        }
        s = env_num;
    }

    *p_rcc = NULL;

    if (s < 0)
    {
        rc = rcf_comm_agent_create_listener(atoi(config_str), &s);
        if (rc != 0)
            return rc;
    }

    s1 = accept(s, NULL, NULL);
    if (s1 < 0)
    {
        rc = TE_OS_RC(TE_COMM, errno);
        ERROR("accept() error: errno=%d\n", errno);
        (void)close(s);
        return rc;
    }
    /* Connection established */

    if (close(s) != 0)
    {
        rc = TE_OS_RC(TE_COMM, errno);
        ERROR("close(s) error: errno=%d\n", errno);
        (void)close(s1);
        return rc;
    }

#if HAVE_FCNTL_H
    /*
     * Try to set close-on-exec flag, but ignore failures,
     * since it's not critical.
     */
    (void)fcntl(s1, F_SETFD, FD_CLOEXEC);
#endif
#if HAVE_NETINET_TCP_H
    /* Set TCE_NODELAY=1 to force TCP to send all data ASAP. */
    if (setsockopt(s1,
#ifdef SOL_TCP
                   SOL_TCP,
#else
                   IPPROTO_TCP,
#endif
                   TCP_NODELAY, &optval, sizeof(optval)) != 0)
    {
        rc = TE_OS_RC(TE_COMM, errno);
        ERROR("setsockopt(SOL_TCP, TCP_NODELAY, enabled): errno=%d\n",
              errno);
        (void)close(s1);
        return rc;
    }
#endif

    /* It's time to allocate memory for rcc and fill it */
    *p_rcc = (struct rcf_comm_connection*)calloc(1, sizeof(**p_rcc));
    if ((*p_rcc) == NULL)
    {
        rc = TE_OS_RC(TE_COMM, errno);
        ERROR("memory allocation error: errno=%d\n", errno);
        (void)close(s1);
        return rc;
    }

    /* All field is set to zero. Just set the socket */
    (*p_rcc)->socket = s1;

    return 0;
}

/**
 * Wait for command from the Test Engine via Network Communication library.
 *
 * @param rcc           Handler received from rcf_comm_agent_init
 * @param buffer        Buffer for data
 * @param pbytes        Pointer to variable with:
 *                      on entry - size of the buffer;
 *                      on return:
 *                      number of bytes really written if 0 returned
 *                      (success);
 *                      unchanged if TE_ESMALLBUF returned;
 *                      number of bytes in the message (with attachment)
 *                      if TE_EPENDING returned. (Note: If the function
 *                      called a number of times to receive one big message,
 *                      a full number of bytes will be returned on first
 *                      call.
 *                      On the next calls number of bytes in the message
 *                      minus number of bytes previously read by this
 *                      function will be returned.);
 *                      undefined if other errno returned.
 * @param pba           Address of the pointer that will hold on return
 *                      address of the first byte of attachment (or NULL if
 *                      no attachment attached to the command). If this
 *                      function called more than once (to receive big
 *                      attachment) this pointer will be not touched.
 *
 * @return Status code.
 * @retval 0            Success (message received and written to the
 *                      buffer).
 * @retval TE_ESMALLBUF Buffer is too small for the message. The part of
 *                      the message is written to the buffer. Other
 *                      part(s) of the message can be read by the next
 *                      calls to the rcf_comm_agent_wait. The ETSMALLBUF
 *                      will be returned until last part of the message
 *                      will be read.
 * @retval TE_EPENDING  Attachment is too big to fit into the buffer. Part
 *                      of the message with attachment is written to the
 *                      buffer. Other part(s) can be read by the next calls
 *                      to the rcf_comm_engine_receive. The TE_EPENDING
 *                      will be returned until last part of the message
 *                      will be read.
 * @retval other value  errno.
 */
int
rcf_comm_agent_wait(struct rcf_comm_connection *rcc,
                    char *buffer, size_t *pbytes, void **pba)
{
    int     ret;
    size_t  l = 0;

    if (rcc->bytes_to_read)
    {
        /* Some data from previous message should be returned */
        if (rcc->bytes_to_read <= *pbytes)
        {
            /* Enought space */
            *pbytes = rcc->bytes_to_read;
            rcc->bytes_to_read = 0;
            return read_socket(rcc->socket, buffer, *pbytes);
        }
        else
        {
            /* Buffer is too small for the attachment */
            if ((ret = read_socket(rcc->socket, buffer, *pbytes)) != 0)
                return ret; /* Some error occured */

            {
                size_t tmp = *pbytes;

                *pbytes = rcc->bytes_to_read;
                rcc->bytes_to_read -= tmp;
            }
            return TE_RC(TE_COMM, TE_EPENDING);
        }
    }

    while (1)
    {
        int r;
        errno = 0;

        r = recv(rcc->socket, buffer + l, 1, 0);
        if (r < 0)
        {
            if (errno == EINTR) /* Valgrind work-around */
                continue;
            ERROR("recv() failed\n");
            return TE_OS_RC(TE_COMM, errno);
        }
        if (r == 0)
        {
            ERROR("%s(): recv() returned 0, connection is closed\n",
                  __FUNCTION__);
            return TE_RC(TE_COMM, TE_EPIPE);
        }

        if (buffer[l] == 0
#ifdef TE_COMM_DEBUG_PROTO
            || buffer[l] == '\n'
#endif
            )
        {
            /* The whole message received */
            int attach_size;

#ifdef TE_COMM_DEBUG_PROTO
            if (buffer[l] == '\n')
            {
                buffer[l] = 0;           /* Change '\n' to zero... */

                if ((l > 0) && (buffer[l - 1] == '\r'))
                {
                    /* ... and change '\r' to the space */
                    buffer[l - 1] = ' ';
                }
            }
#endif

            l++;

            attach_size = find_attach(buffer, l);

            if (attach_size == -1)
            {
                /* No attachment */
                *pbytes = l;

                /* Set pba to NULL because no attachment attached */
                if (pba != NULL)
                    *pba = NULL;

                return 0;
            }
            else
            {
                /* Attachment found. */

                /* Set pba to the first byte of the attachment */
                if (pba != NULL)
                    *pba = buffer + l;

                if (*pbytes >= l + attach_size)
                {
                    /* Buffer is enought to write attachment */
                    *pbytes = l + attach_size;
                    return read_socket(rcc->socket, buffer + l,
                                       attach_size);
                }
                else
                {
                    /* Buffer is too small to write attachment */
                    size_t to_read = *pbytes - l;

                    ret = read_socket(rcc->socket, buffer + l, to_read);
                    if (ret != 0)
                        return ret; /* Some error occured */
                    rcc->bytes_to_read = attach_size - to_read;
                    *pbytes = attach_size + l;
                    return TE_RC(TE_COMM, TE_EPENDING);
                }
            }
        }

        if (l == (*pbytes - 1))
            return TE_RC(TE_COMM, TE_ESMALLBUF);

        l += r;
    }
}

/**
 * Send reply to the Test Engine side of Network Communication library.
 *
 * @param rcc           Handler received from rcf_comm_agent_init.
 * @param buffer        Buffer with reply.
 * @param length        Length of the data to send.
 *
 * @return Status code.
 * @retval 0            Success.
 * @retval other value  errno.
 */
int
rcf_comm_agent_reply(struct rcf_comm_connection *rcc, const void *buffer,
                     size_t length)
{
    size_t  total_sent_len = 0;
    ssize_t sent_len;

    if (length == 0)
        return 0;
#ifdef TE_COMM_DEBUG_PROTO
    {
        /* Change \x0 to \n in the user (!!!) buffer before sending */
        int n = strlen((const char *)buffer);

        assert(n <= length);
        ((char *)buffer)[n] = '\n';
    }
#endif

    while (length > 0)
    {
        sent_len = send(rcc->socket,
                        (const uint8_t *)buffer + total_sent_len,
                        length, 0 /* flags */);
        if (sent_len < 0)
        {
            ERROR("%s(): send(%d) failed: errno=%d\n",
                  __FUNCTION__, rcc->socket, errno);
            return TE_OS_RC(TE_COMM, errno);
        }

        total_sent_len += sent_len;
        length -= sent_len;
    }

    return 0;
}



/**
 * Close connection.
 *
 * @param p_rcc   Pointer to variable with handler received from
 *                rcf_comm_agent_init.
 *
 * @return Status code.
 * @retval 0            Success.
 * @retval other value  errno.
 */
int
rcf_comm_agent_close(struct rcf_comm_connection **p_rcc)
{
    if (p_rcc == NULL)
        return TE_RC(TE_COMM, TE_EINVAL);

    if (*p_rcc == NULL)
        return 0;

    if (close((*p_rcc)->socket) < 0)
    {
        ERROR("close() failed\n");
        return TE_OS_RC(TE_COMM, errno);
    }

    free(*p_rcc);
    *p_rcc = NULL;

    return 0;
}

/**
 * Search in the string for the "attach <number>" entry at the end. Inserts
 * ZERO before 'attach' word.
 *
 * @param str           String to process
 * @param len           Length of the string
 *
 * @return Status code
 * @retval -1           No such entry found
 * @retval other value  Number (value) from the entry
 */
static int
find_attach(char *buf, size_t len)
{
    /* Pointer tmp will scan the buffer */
    char *tmp;

    /* Pointer number will hold the address of the "<number>" entry */
    const char *number;

    /* Strings less than 9 chars can not contain the string we search for */
    if (len < 9)
        return -1;

    /* Make tmp point to the last char in the string */
    tmp = buf + len - 1;

    if (*tmp == 0)
        tmp--; /* Skip the \x00 at the end */

    /* Skip whitespaces at the end (if any) */
    while (isspace(*tmp))
    {
        tmp--;
        if (tmp == buf) /* Begining of the buf */
            return -1;
    }

    /* Check that last non-whitespace character is digit */
    if (!isdigit(*tmp))
        return -1;

    tmp --;

    /* Scan all digits */
    while (isdigit(*tmp))
    {
        tmp--;
        if (tmp == buf) /* Begining of the buf */
            return -1;
    }

    /* Check that before group of digits is whitespace character */
    if (!isspace(*tmp))
    {
        return -1;
    }

    /* Make a number point to the begining of the group of numbers */
    number = tmp+1;

    tmp--;

    /* Skip whitespace characters */
    while (isspace(*tmp))
    {
        tmp--;
        if (tmp == buf) /* Begining of the buf */
            return -1;
    }

    /* Check that more than 5+1 characters left */
    if ((tmp-buf) < 7)
        return -1;

    tmp -= 5;

    /* Is it 'attach' string ? */
    if (tmp[0] != 'a' ||
        tmp[1] != 't' ||
        tmp[2] != 't' ||
        tmp[3] != 'a' ||
        tmp[4] != 'c' ||
        tmp[5] != 'h')
    {
        /* No */
        return -1;
    }

    /* Insert ZERO before 'attach' word */
    *(tmp-1) = 0;


    /* Convert string into the int and return the value */
     return atol(number);
}

/**
 * Read specified number of bytes (not less) from the connection.
 *
 * @param socket        Connection socket
 * @param buf           Buffer to store the data
 * @param len           Number of bytes to read
 *
 * @return Status code.
 *
 * @retval 0            Success
 * @retval other value  errno
 */
static int
read_socket(int socket, void *buffer, size_t len)
{
    ssize_t r;

    while (len > 0)
    {
        r = recv(socket, buffer, len, 0);
        if (r < 0)
        {
            ERROR("recv() from socket\n");
            return TE_OS_RC(TE_COMM, errno);
        }
        else if (r == 0)
        {
            ERROR("%s(): recv() returned 0, connection is closed\n",
                  __FUNCTION__);
            return TE_RC(TE_COMM, TE_EPIPE);
        }
        assert((size_t)r <= len);

        len -= r;
        buffer = (uint8_t *)buffer + r;
    }

    return 0;
}

