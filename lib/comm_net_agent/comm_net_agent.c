/** @file
 * @brief Test Environment:
 *
 * Communication library - Test Agent side
 * Definition of routines provided for library user *
 *
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 * Author: Andrey Ivanov <andron@oktetlabs.ru>
 *
 * @(#) $Id$
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
#if HAVE_FCNTL_H
#include <fcntl.h>
#endif

#include "te_errno.h"
#include "comm_agent.h"


/** This structure is used to store some context for each connection. */
struct rcf_comm_connection{
    int     socket;          /**< Connection socket */
    size_t  bytes_to_read;   /**< Number of bytes of attachment to read */
};

/* Static function declaration. See implementation for comments */
static int find_attach(char *buf, size_t len);
static int read_socket(int socket, void *buffer, size_t len);

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
    struct sockaddr_in addr;
    int s, s1, rc;
    int optval = 1;

    *p_rcc = NULL;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(config_str));
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0)
    {
        /* Socket error */
        perror("socket() error");
        return errno;
    }

    rc = setsockopt(s, SOL_SOCKET, SO_REUSEADDR,
                    &optval, sizeof(optval));
    if (rc < 0)
    {
        perror("setsockopt() error");
        return errno;
    }

    rc = bind(s, (struct sockaddr *)&addr, sizeof(addr));
    if (rc < 0)
    {
        perror("bind() error");
        return errno;
    }

    rc = listen(s, 5);
    if (rc != 0)
    {
        perror("listen() error");
        return errno;
    }

    s1 = accept(s, NULL, NULL);
    if (s1 < 0)
    {
        perror("accept() error");
        return errno;
    }

    /* Connection established */
    if (close(s) < 0)
    {
        perror("close(s) error");
        return errno;
    }

#if HAVE_FCNTL_H
    /* 
     * Try to set close-on-exec flag, but ignore failures, 
     * since it's not critical.
     */
    (void)fcntl(s1, F_SETFD, FD_CLOEXEC);
#endif

    /* It's time to allocate memory for rcc and fill it */
    *p_rcc = (struct rcf_comm_connection*)calloc(1, sizeof(**p_rcc));

    if ((*p_rcc) == NULL)
    {
        perror("memory allocation error");
        return errno;
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
 *                      unchanged if ETESMALLBUF returned;
 *                      number of bytes in the message (with attachment)
 *                      if ETEPENDING returned. (Note: If the function
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
 * @retval ETESMALLBUF  Buffer is too small for the message. The part of
 *                      the message is written to the buffer. Other
 *                      part(s) of the message can be read by the next
 *                      calls to the rcf_comm_agent_wait. The ETSMALLBUF
 *                      will be returned until last part of the message
 *                      will be read.
 * @retval ETEPENDING   Attachment is too big to fit into the buffer. Part
 *                      of the message with attachment is written to the
 *                      buffer. Other part(s) can be read by the next calls
 *                      to the rcf_comm_engine_receive. The ETEPENDING
 *                      will be returned until last part of the message
 *                      will be read.
 * @retval other value  errno.
 */
int
rcf_comm_agent_wait(struct rcf_comm_connection *rcc,
                    char *buffer, size_t *pbytes, char **pba)
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
            return ETEPENDING;
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
            perror("socket read");
            return errno;
        }
        if (r == 0)
        {
            printf ("recv 0, exit\n");
            exit(1);
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
                    return ETEPENDING;
                }
            }
        }

        if (l == (*pbytes - 1))
            return ETESMALLBUF;

        l += r;
    }
}

/**
 * Send reply to the Test Engine side of Network Communication library.
 *
 * @param rcc           Handler received from rcf_comm_agent_connect.
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
                        length, 0);
        if (sent_len < 0)
            return errno;

        total_sent_len += sent_len;
        length -= sent_len;
    }

    return 0;
}



/**
 * Close connection.
 *
 * @param p_rcc   Pointer to variable with handler received from
 *                rcf_comm_agent_connect
 *
 * @return Status code.
 * @retval 0            Success.
 * @retval other value  errno.
 */
int
rcf_comm_agent_close(struct rcf_comm_connection **p_rcc)
{
    if (*p_rcc == NULL)
        return 0;

    if (close((*p_rcc)->socket) < 0)
    {
        perror("close");
        return errno;
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
 * Read specified number of bytes (not less) from the connection
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
            perror ("read socket: ");
            return errno;
        }
        assert((size_t)r <= len);

        len -= r;
        buffer = (uint8_t *)buffer + r;
    }

    return 0;
}

