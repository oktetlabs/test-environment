/** @file
 * @brief Test API to FTP client routines
 *
 * Test API to FTP client routines.
 *
 *
 * Copyright (C) 2016 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
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
 * @author Ivan Melnikov <Ivan.Melnikov@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER     "TAPI Storage Client FTP"

#include "tapi_storage_client_ftp.h"

#ifdef HAVE_CTYPE_H
#include <ctype.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_ASSERT_H
#include <assert.h>
#endif

#include "tapi_rpc_socket.h"
#include "tapi_rpc_unistd.h"
#include "tapi_rpc_misc.h"
#include "tapi_rpcsock_macros.h"
#include "tapi_sockaddr.h"
#include "te_string.h"
#include "te_dbuf.h"
#include "te_alloc.h"


/*
 * FTP COMMANDS, see RFC959
 */
/*
 * ACCESS CONTROL COMMANDS
 */
/* The command to identify the user. */
#define FTP_CMD_USER_NAME       "USER"
/* The command to specify the user's password. */
#define FTP_CMD_PASSWORD        "PASS"
/* The command to identify the user's account. */
#define FTP_CMD_ACCOUNT         "ACCT"
/* The command to change working directory. */
#define FTP_CMD_CWD             "CWD"
/* The command to change to parent directory. */
#define FTP_CMD_CDUP            "CDUP"
/* The command to mount a different file system data structure. */
#define FTP_CMD_STRUCTURE_MOUNT "SMNT"
/* The command to terminate a USER, flushing all I/O and account info. */
#define FTP_CMD_REINITIALIZE    "REIN"
/* The command to terminate a USER and close the control connection. */
#define FTP_CMD_LOGOUT          "QUIT"

/*
 * TRANSFER PARAMETER COMMANDS
 */
/* The command to set the server in active mode. */
#define FTP_CMD_DATA_PORT       "PORT"
/* The command to set the server in passive mode. */
#define FTP_CMD_PASSIVE         "PASV"
/* The command to specify a type of file transfer. */
#define FTP_CMD_REPRESENTATION_TYPE "TYPE"
/* The command to specify a file structure. */
#define FTP_CMD_FILE_STRUCTURE  "STRU"
/* The command to specify the data transfer modes. */
#define FTP_CMD_TRANSFER_MODE   "MODE"

/*
 * FTP SERVICE COMMANDS
 */
/* The command to retrieve a copy of the file from the server. */
#define FTP_CMD_RETRIEVE        "RETR"
/* The command to transfer the file to the server. */
#define FTP_CMD_STORE           "STOR"
/* The command to transfer the file to the server and store it with unique
 * name which will return in response. */
#define FTP_CMD_STORE_UNIQUE    "STOU"
/* The command to transfer the data and save it in the server file. */
#define FTP_CMD_APPEND          "APPE"
/* The command to reserve sufficient storage to accommodate the new file to
 * be transferred. */
#define FTP_CMD_ALLOCATE        "ALLO"
/* The commant to represent the server marker at which file transfer is to
 * be restarted. */
#define FTP_CMD_RESTART         "REST"
/* The command to specify the old pathname of the file which is to be
 * renamed. */
#define FTP_CMD_RENAME_FROM     "RNFR"
/* The command to specify the new pathname of the file. */
#define FTP_CMD_RENAME_TO       "RNTO"
/* The command to abort the previous FTP service command and any associated
 * transfer of data. */
#define FTP_CMD_ABORT           "ABOR"
/* The command to delete a specified file at the server site. */
#define FTP_CMD_DELETE          "DELE"
/* The command to delete a specified directory at the server site. */
#define FTP_CMD_RMDIR           "RMD"
/* The command to create a directory at the server site. */
#define FTP_CMD_MKDIR           "MKD"
/* The command to print the current working directory. */
#define FTP_CMD_PWD             "PWD"
/* The command to retrieve a list of files. */
#define FTP_CMD_LIST            "LIST"
/* The command to retrieve a names of files. */
#define FTP_CMD_NAME_LIST       "NLST"
/* The command to provide services specific to the server system. */
#define FTP_CMD_SITE_PARAMETERS "SITE"
/* The command to find out the type of operating system at the server. */
#define FTP_CMD_SYSTEM          "SYST"
/* The command to request the current status. */
#define FTP_CMD_STATUS          "STAT"
/* The command to get a helpful information regarding server's
 * implementation status. */
#define FTP_CMD_HELP            "HELP"
/* This command specifies no action other than that the server send an OK
 * reply. */
#define FTP_CMD_NOOP            "NOOP"


/*
 * Format strings for particular commands.
 */
/* The command format to identify the user. */
#define FTP_CMD_USER_NAME_FMT   FTP_CMD_USER_NAME " %s\r\n"
/* The command format to specify the user's password. */
#define FTP_CMD_PASSWORD_FMT    FTP_CMD_PASSWORD " %s\r\n"
/* The command format to change working directory. */
#define FTP_CMD_CWD_FMT         FTP_CMD_CWD " %s\r\n"
/* The command format to terminate a USER and close the control connection.
 */
#define FTP_CMD_LOGOUT_FMT      FTP_CMD_LOGOUT "\r\n"

/* The command format to set the server in passive mode. */
#define FTP_CMD_PASSIVE_FMT     FTP_CMD_PASSIVE "\r\n"
/* The command format to specify Image type of file transfer. */
#define FTP_CMD_REPRESENTATION_TYPE_IMAGE_FMT \
    FTP_CMD_REPRESENTATION_TYPE " I\r\n"

/* The command format to retrieve a copy of the file from the server. */
#define FTP_CMD_RETRIEVE_FMT    FTP_CMD_RETRIEVE " %s\r\n"
/* The command format to transfer the file to the server. */
#define FTP_CMD_STORE_FMT       FTP_CMD_STORE " %s\r\n"
/* The command format to delete a specified file at the server site. */
#define FTP_CMD_DELETE_FMT      FTP_CMD_DELETE " %s\r\n"
/* The command format to delete a specified directory at the server site. */
#define FTP_CMD_RMDIR_FMT       FTP_CMD_RMDIR " %s\r\n"
/* The command format to create a directory at the server site. */
#define FTP_CMD_MKDIR_FMT       FTP_CMD_MKDIR " %s\r\n"
/* The command format to print the current working directory. */
#define FTP_CMD_PWD_FMT         FTP_CMD_PWD "\r\n"
/* The command format to retrieve a list of files. */
#define FTP_CMD_LIST_FMT        FTP_CMD_LIST " %s\r\n"
/* The command format to retrieve a names of files. */
#define FTP_CMD_NAME_LIST_FMT   FTP_CMD_NAME_LIST " %s\r\n"

/*
 * Reply Codes
 */
/* Entering Passive Mode (h1,h2,h3,h4,p1,p2). */
#define FTP_RC_ENTERING_PASSIVE_MODE    227
/* "PATHNAME" created. */
#define FTP_RC_PATHNAME_CREATED         257

/*
 * Negative Completion start value.
 *
 * There are five values for the first digit of the reply code:
 *  1yz   Positive Preliminary reply;
 *  2yz   Positive Completion reply;
 *  3yz   Positive Intermediate reply;
 *  4yz   Transient Negative Completion reply;
 *  5yz   Permanent Negative Completion reply.
 */
#define FTP_NEGATIVE_COMPLETION_START_VALUE     400

/*
 * Amount of bytes to expand the receive buffer. It is used in case of size
 * of buffer becames too small to store the whole received message.
 */
#define RBUFFER_GROW_SIZE   1024

/* Format of reply message to enter passive mode. */
#define PASSIVE_MODE_REPLY_TEMPLATE \
    "227 Entering Passive Mode (h1,h2,h3,h4,p1,p2)"


/* FTP client specific context. */
struct tapi_storage_client_ftp_context {
    int control_socket;             /* Socket of control connection. */
    int data_socket;                /* Socket of data connection. */
    struct sockaddr_storage addr;   /* Data connection server address. */
    te_string cmdbuf_w;             /* Buffer to accumulate request message
                                       (control connection). */
    te_dbuf cmdbuf_r;               /* Buffer to accumulate reply message
                                       (control connection). */
    te_dbuf databuf_r;              /* Buffer to accumulate received data
                                       (data connection). */
};


/**
 * Terminate received message by null-terminator ('\0').
 *
 * @param buf       Buffer contains message to terminate.
 *
 * @return Status code.
 */
static te_errno
terminate_received_message(te_dbuf *buf)
{
    te_errno rc;

    rc = te_dbuf_append(buf, "", 1);
    if (rc == 0)
        VERB("Reply: %s", buf->ptr);
    return rc;
}

/**
 * Flush receive buffer.
 *
 * @param buf       Buffer to flush.
 */
static void
flush_received_message(te_dbuf *buf)
{
    te_dbuf_reset(buf);
    te_dbuf_append(buf, "", 1);
    te_dbuf_reset(buf);
}

/**
 * Write data to the socket (send a request).
 *
 * @param rpcs      RPC server handle.
 * @param fd        Socket descriptor.
 * @param request   Request data.
 *
 * @return Status code.
 */
static te_errno
send_request(rcf_rpc_server *rpcs, int fd, const te_string *request)
{
    ssize_t written;

    VERB("Request: %s", request->ptr);
    written = rpc_send(rpcs, fd, request->ptr, request->len, 0);
    return (written == request->len ? 0 : TE_EIO);
}

/**
 * Read data from the socket (get a reply).
 *
 * @param rpcs      RPC server handle.
 * @param fd        Socket descriptor.
 * @param reply     Buffer to save read data to.
 *
 * @return Status code.
 */
static te_errno
read_reply(rcf_rpc_server *rpcs, int fd, te_dbuf *reply)
{
    ssize_t  retrieved;
    uint8_t *buf;
    size_t   size_to_read;  /* Size of data to read. Actually it is a size
                               of free space of receive buffer. */
    te_errno rc = 0;

    flush_received_message(reply);
    do {
        /* Prepare the buffer to save the message. */
        size_to_read = reply->size - reply->len;
        if (size_to_read == 0)
        {
            rc = te_dbuf_expand(reply, RBUFFER_GROW_SIZE);
            if (rc != 0)
                return rc;
        }
        size_to_read = reply->size - reply->len;
        buf = &reply->ptr[reply->len];
        /* Read a message. */
        retrieved = rpc_read(rpcs, fd, buf, size_to_read);
        if (retrieved > 0)
            reply->len += retrieved;
        else /* if (retrieved == 0) */
        {
            ERROR("Got EOF");
            return TE_ENODATA;
        }
    } while (buf[retrieved - 1] != '\n'); /* Wait for \r\n terminator. */

    return terminate_received_message(reply);
}

/**
 * Read chunk of data from the socket. It appends data to the @p buf, don't
 * forget to reset te_dbuf before collect the message.
 *
 * @param rpcs      RPC server handle.
 * @param fd        Socket descriptor.
 * @param buf       Buffer to append read data to.
 *
 * @return Status code.
 *
 * @sa te_dbuf_reset
 */
static te_errno
read_chunk(rcf_rpc_server *rpcs, int fd, te_dbuf *buf)
{
    int    retrieved;
    size_t size_to_read;    /* Size of data to read. Actually it is a free
                               space of receive buffer. */
    te_errno rc = 0;

    /* Prepare the buffer to save the message. */
    size_to_read = buf->size - buf->len;
    if (size_to_read == 0)
    {
        rc = te_dbuf_expand(buf, RBUFFER_GROW_SIZE);
        if (rc != 0)
            return rc;
    }
    size_to_read = buf->size - buf->len;
    /* Read a message. */
    retrieved = rpc_read(rpcs, fd, &buf->ptr[buf->len], size_to_read);
    if (retrieved > 0)
        buf->len += retrieved;
    else /* if (retrieved == 0) */
        return TE_ENODATA;      /* EOF */
    return 0;
}

/**
 * Flush the receive buffer of socket.
 *
 * @param rpcs      RPC server handle.
 * @param fd        Socket descriptor.
 */
static void
flush_socket_receive_buffer(rcf_rpc_server *rpcs, int fd)
{
    char tmp_buf[256];
    int  retrieved;
    struct rpc_pollfd fds = {
        .fd = fd,
        .events = RPC_POLLIN,
        .revents = 0
    };

    rpc_poll(rpcs, &fds, 1, 0);
    while ((fds.revents & RPC_POLLIN) != 0)
    {
        VERB("flush data of socket %d", fd);
        retrieved = rpc_read(rpcs, fd, tmp_buf, sizeof(tmp_buf));
        if (retrieved == 0)
            break;      /* EOF, or RPC_POLLHUP. */
        fds.revents = 0;
        rpc_poll(rpcs, &fds, 1, 0);
    }
}

/**
 * Get reply code.
 *
 * @param reply     Reply message.
 *
 * @return reply code, or @c -1 on fail to get one.
 */
static int
get_reply_code(const char *reply)
{
    return (reply != NULL ? atoi(reply) : -1);
}

/**
 * Check the @p reply is it Negative or Positive Completion reply.
 *
 * @param reply     Reply message.
 *
 * @return Status code, @c 0 on Positive Completion reply.
 */
static te_errno
check_reply_code_for_error(const char *reply)
{
    int reply_code;

    reply_code = get_reply_code(reply);
    if (reply_code >= FTP_NEGATIVE_COMPLETION_START_VALUE ||
        reply_code == -1)
    {
        return TE_EFAIL;
    }
    return 0;
}

/**
 * Fill a buffer with data from @p fmt and send this command to ftp server
 * over control connection.
 *
 * @param client        Client handle.
 * @param fmt           Format string
 * @param ap            List of arguments
 *
 * @return Status code.
 */
static te_errno
send_control_msg_va(tapi_storage_client *client,
                    const char          *fmt,
                    va_list              ap)
{
    tapi_storage_client_ftp_context *ftp_context = client->context;
    te_string *cmdbuf_w = &ftp_context->cmdbuf_w;
    te_errno   rc;

    cmdbuf_w->len = 0;
    rc = te_string_append_va(cmdbuf_w, fmt, ap);
    if (rc != 0)
        return rc;
    rc = send_request(client->rpcs, ftp_context->control_socket, cmdbuf_w);
    return rc;
}

/**
 * Fill a buffer with data from @p fmt and send this command to ftp server
 * over control connection. This function calls @p send_control_msg_va.
 *
 * @param client        Client handle.
 * @param fmt           Format string.
 * @param ...           Format string arguments.
 *
 * @return Status code.
 *
 * @sa send_control_msg_va
 */
static te_errno
send_control_msg(tapi_storage_client *client, const char *fmt, ...)
{
    va_list  ap;
    te_errno rc;

    va_start(ap, fmt);
    rc = send_control_msg_va(client, fmt, ap);
    va_end(ap);
    return rc;
}

/**
 * Read reply from ftp server over control connection and check it on error.
 * This function overwrites the control connection read buffer. If you have
 * unread data in it use @b get_control_msg function instead.
 *
 * @param client        Client handle.
 *
 * @return Status code.
 *
 * @sa get_control_msg
 */
static te_errno
read_control_msg(tapi_storage_client *client)
{
    tapi_storage_client_ftp_context *ftp_context = client->context;
    te_dbuf  *cmdbuf_r = &ftp_context->cmdbuf_r;
    te_errno  rc;

    rc = read_reply(client->rpcs, ftp_context->control_socket, cmdbuf_r);
    if (rc != 0)
        return rc;
    /* Check the reply code. */
    rc = check_reply_code_for_error((const char *)cmdbuf_r->ptr);
    return rc;
}

/**
 * Get a next reply from the control connection read buffer cause it is
 * possible that during last read operation there are several messages was
 * got. If the message in buffer is not found, then the next read operation
 * is initiated, i.e. in this case it works as @b read_control_msg.
 *
 * @param client        Client handle.
 *
 * @return Status code.
 *
 * @sa read_control_msg
 */
static te_errno
get_control_msg(tapi_storage_client *client)
{
    tapi_storage_client_ftp_context *ftp_context = client->context;
    te_dbuf  *cmdbuf_r = &ftp_context->cmdbuf_r;
    char     *next_msg = NULL;
    te_errno  rc;

    /* Search for the not read (next) message. */
    next_msg = (char *)cmdbuf_r->ptr;
    while (*next_msg++ != '\n')
        ;   /* nop */
    if (*next_msg != '\0')
    {
        /* There is the next message in the buffer. */
        VERB("%s:%d: There is unread message in the buffer: %s",
             __FUNCTION__, __LINE__, next_msg);
        strcpy((char *)cmdbuf_r->ptr, next_msg);
    }
    else
    {
        rc = read_reply(client->rpcs, ftp_context->control_socket,
                        cmdbuf_r);
        if (rc != 0)
            return rc;
    }
    /* Check the reply code. */
    return check_reply_code_for_error((const char *)cmdbuf_r->ptr);
}

/**
 * Fill a buffer with data from @p fmt, send this command, read reply and
 * parse it.
 * it's wrapper for @p send_control_msg_va and @p read_control_msg.
 *
 * @param client        Client handle.
 * @param fmt           Format string.
 * @param ...           Format string arguments.
 *
 * @return Status code.
 *
 * @sa send_control_msg_va, read_control_msg
 */
static te_errno
send_command(tapi_storage_client *client, const char *fmt, ...)
{
    tapi_storage_client_ftp_context *ftp_context = client->context;
    te_string *cmdbuf_w = &ftp_context->cmdbuf_w;
    te_dbuf   *cmdbuf_r = &ftp_context->cmdbuf_r;
    va_list    ap;
    te_errno   rc;

    va_start(ap, fmt);
    rc = send_control_msg_va(client, fmt, ap);
    va_end(ap);
    if (rc != 0)
        return rc;
    rc = read_control_msg(client);
    if (rc != 0)
    {
        ERROR("Failed to execute command:\n"
              "command: %s"
              "reply: %s",
              cmdbuf_w->ptr, cmdbuf_r->ptr);
    }
    return rc;
}

/**
 * Open data connection.
 *
 * @param client        Client handle.
 *
 * @return Status code.
 */
static te_errno
open_data_connection(tapi_storage_client *client)
{
    tapi_storage_client_ftp_context *ftp_context = client->context;
    te_dbuf   *cmdbuf_r = &ftp_context->cmdbuf_r;
    te_errno   rc;
    char      *str;
    in_addr_t  addr;
    in_port_t  port;
    uint8_t   *octet;
    size_t     i;

    if (rpc_socket_domain_by_addr(client->auth.server_addr) != RPC_PF_INET)
    {
        ERROR("Now only IPv4 data connection supported");
        return TE_ENOSYS;
    }
    if (ftp_context->data_socket >= 0)
    {
        ERROR("FTP data connection is already established");
        return TE_EISCONN;
    }
    /* Set passive mode. */
    rc = send_command(client, FTP_CMD_PASSIVE_FMT);
    if (rc != 0)
        return rc;
    if (get_reply_code((const char *)cmdbuf_r->ptr) !=
                                            FTP_RC_ENTERING_PASSIVE_MODE)
    {
        return TE_EPROTO;
    }
    str = strchr((const char *)cmdbuf_r->ptr, '(');
    if (str == NULL)
    {
        ERROR("Broken response of entering passive mode: \"%s\". Expected"
              " message format is \"" PASSIVE_MODE_REPLY_TEMPLATE "\"",
              (const char *)cmdbuf_r->ptr);
        return TE_EBADMSG;
    }
    /* Here we assume that FTP server provides correct answer */
    tapi_sockaddr_clone_exact(client->auth.server_addr,
                              &ftp_context->addr);
    octet = (uint8_t *)&addr;

/*
 * Check that string representation of socket octet is valid, i.e. is not
 * @c NULL. Print error message in case of failure and return @c TE_EBADMSG
 * error code.
 *
 * @param val_      String representation of socket octet.
 */
#define CHECK_STR_OCTET(val_) \
    do {                                                                   \
        if (val_ == NULL)                                                  \
        {                                                                  \
            ERROR("Incorrect response of entering passive mode: \"%s\"."   \
                  " Expected message format is \""                         \
                  PASSIVE_MODE_REPLY_TEMPLATE "\"",                        \
                  (const char *)cmdbuf_r->ptr);                            \
            return TE_EBADMSG;                                             \
        }                                                                  \
    } while (0)

    for (i = 0, str++; i < 4; str = strchr(str, ',') + 1, i++)
    {
        CHECK_STR_OCTET(str);
        *octet++ = atoi(str);
    }
    octet = (uint8_t *)&port;
    CHECK_STR_OCTET(str);
    *octet++ = atoi(str);
    str = strchr(str, ',') + 1;
    CHECK_STR_OCTET(str);
    *octet = atoi(str);
#undef CHECK_STR_OCTET

    if (addr != 0)
        memcpy(&SIN(&ftp_context->addr)->sin_addr, &addr, sizeof(addr));
    te_sockaddr_set_port(SA(&ftp_context->addr), port);

    /* Connect to the server. */
    VERB("Connecting to data port: %u", ntohs(port));
    ftp_context->data_socket = rpc_socket(client->rpcs,
                    rpc_socket_domain_by_addr(CONST_SA(&ftp_context->addr)),
                    RPC_SOCK_STREAM, RPC_PROTO_DEF);
    rpc_connect(client->rpcs, ftp_context->data_socket,
                CONST_SA(&ftp_context->addr));
    VERB("Data connection is established on socket: %d",
         ftp_context->data_socket);

    return rc;
}

/**
 * Close data connection.
 *
 * @param client        Client handle.
 *
 * @return Status code.
 */
static te_errno
close_data_connection(tapi_storage_client *client)
{
    tapi_storage_client_ftp_context *ftp_context = client->context;

    VERB("Close ftp data connection");
    if (ftp_context->data_socket == -1)
    {
        ERROR("FTP data connection is not established");
        return TE_ENOTCONN;
    }
    RPC_CLOSE(client->rpcs, ftp_context->data_socket);
    return 0;
}

/**
 * Read all data from data connection. It assumes that data is fully
 * received by receiving a message from control connection.
 *
 * @param client        Client handle.
 *
 * @return Status code.
 */
static te_errno
read_data(tapi_storage_client *client)
{
    tapi_storage_client_ftp_context *ftp_context = client->context;
    te_dbuf *cmdbuf_r = &ftp_context->cmdbuf_r;
    te_dbuf *databuf_r = &ftp_context->databuf_r;
    enum { FD_CONTROL, FD_DATA };
    struct rpc_pollfd fds[] = {
        [FD_CONTROL] = {
            .fd = ftp_context->control_socket,
            .events = RPC_POLLIN,
            .revents = 0
        },
        [FD_DATA] = {
            .fd = ftp_context->data_socket,
            .events = RPC_POLLIN,
            .revents = 0
        }
    };
    int      num_revents;
    te_errno rc = 0;
    int      timeout = 5000;    /* Init value of timeout. */

    te_dbuf_reset(cmdbuf_r);
    te_dbuf_reset(databuf_r);
    num_revents = rpc_poll(client->rpcs, fds, TE_ARRAY_LEN(fds), timeout);
    while (num_revents > 0)
    {
        if ((fds[FD_CONTROL].revents & RPC_POLLIN) != 0)
        {
            rc = read_chunk(client->rpcs, fds[FD_CONTROL].fd, cmdbuf_r);
            if (rc != 0)
                return rc;

            if (cmdbuf_r->ptr[cmdbuf_r->len - 1] == '\n')
            {
                /* Control message is fully received. */
                fds[FD_CONTROL].fd = -1;
                fds[FD_CONTROL].events = 0;
                timeout = 0;  /* FIXME may be need to set positive value. */
            }
            fds[FD_CONTROL].revents = 0;
        }
        if ((fds[FD_DATA].revents & RPC_POLLIN) != 0)
        {
            rc = read_chunk(client->rpcs, fds[FD_DATA].fd, databuf_r);
            if (rc == TE_ENODATA)
            {
                fds[FD_DATA].fd = -1;
                fds[FD_DATA].events = 0;
                rc = 0;
            }
            if (rc != 0)
                return rc;

            fds[FD_DATA].revents = 0;
        }
        num_revents = rpc_poll(client->rpcs, fds, TE_ARRAY_LEN(fds),
                               timeout);
    }

    rc = terminate_received_message(cmdbuf_r);
    if (rc != 0)
        return rc;
    rc = terminate_received_message(databuf_r);
    return rc;
}

/**
 * Hook function to execute @c OPEN command over FTP.
 *
 * @param client        Client handle.
 *
 * @return Status code.
 */
static te_errno
ftp_open(tapi_storage_client *client)
{
    tapi_storage_client_ftp_context *ftp_context = client->context;
    te_dbuf *cmdbuf_r = &ftp_context->cmdbuf_r;
    const struct sockaddr *addr =
        (const struct sockaddr *)client->auth.server_addr;
    te_errno rc = 0;

    if (ftp_context->control_socket >= 0)
    {
        ERROR("FTP control connection is already established");
        return TE_RC(TE_TAPI, TE_EISCONN);
    }
    /* Connect to the server. */
    VERB("Connecting to control port");
    ftp_context->control_socket = rpc_socket(client->rpcs,
                                        rpc_socket_domain_by_addr(addr),
                                        RPC_SOCK_STREAM, RPC_PROTO_DEF);
    RPC_AWAIT_IUT_ERROR(client->rpcs);
    rc = rpc_connect(client->rpcs, ftp_context->control_socket, addr);
    if (rc != 0)
    {
        ERROR("Failed to connect to FTP server");
        RPC_CLOSE(client->rpcs, ftp_context->control_socket);
        return TE_RC(TE_TAPI, TE_ECONNREFUSED);
    }
    VERB("Control connection is established on socket: %d",
         ftp_context->control_socket);

    rc = read_control_msg(client);
    if (rc != 0)
    {
        ERROR("Failed to establish control connection: %s", cmdbuf_r->ptr);
        RPC_CLOSE(client->rpcs, ftp_context->control_socket);
        return TE_RC(TE_TAPI, rc);
    }

    do {
        /* User name. */
        rc = send_command(client, FTP_CMD_USER_NAME_FMT,
                          (client->auth.user != NULL ? client->auth.user
                                                     : "anonymous"));
        if (rc != 0)
            break;

        /* User password. */
        if (client->auth.password != NULL)
        {
            rc = send_command(client, FTP_CMD_PASSWORD_FMT,
                              client->auth.password);
            if (rc != 0)
                break;
        }
        /* Set an Image type of file transfer. */
        rc = send_command(client, FTP_CMD_REPRESENTATION_TYPE_IMAGE_FMT);
    } while (0);

    if (rc != 0)
        RPC_CLOSE(client->rpcs, ftp_context->control_socket);

    return TE_RC(TE_TAPI, rc);
}

/**
 * Hook function to execute @c LOGOUT command over FTP and close the
 * control connection.
 *
 * @param client        Client handle.
 *
 * @return Status code.
 */
static te_errno
ftp_close(tapi_storage_client *client)
{
    tapi_storage_client_ftp_context *ftp_context = client->context;
    te_errno rc;

    VERB("Close ftp control connection");
    if (ftp_context->control_socket == -1)
    {
        ERROR("FTP control connection is not established");
        return TE_RC(TE_TAPI, TE_ENOTCONN);
    }
    flush_socket_receive_buffer(client->rpcs, ftp_context->control_socket);
    rc = send_command(client, FTP_CMD_LOGOUT_FMT);
    RPC_CLOSE(client->rpcs, ftp_context->control_socket);
    return TE_RC(TE_TAPI, rc);
}

/**
 * Hook function to execute @c PWD command over FTP.
 *
 * @param client        Client handle.
 * @param directory     File context where the directory name will be saved.
 *
 * @return Status code.
 */
static te_errno
ftp_pwd(tapi_storage_client *client, tapi_local_file *directory)
{
    tapi_storage_client_ftp_context *ftp_context = client->context;
    te_dbuf  *cmdbuf_r = &ftp_context->cmdbuf_r;
    te_errno  rc;
    char     *path_begin;
    char     *path_end;
    ssize_t   path_len;     /* Pathname string length including '\0'. */

    assert(directory != NULL);
    rc = send_command(client, FTP_CMD_PWD_FMT);
    if (rc != 0)
        return TE_RC(TE_TAPI, rc);
    if (get_reply_code((const char *)cmdbuf_r->ptr) !=
                                                FTP_RC_PATHNAME_CREATED)
        return TE_RC(TE_TAPI, TE_EPROTO);

    /** @todo The directory name can contain any character;
     * embedded double-quotes should be escaped by double-quotes
     * (the "quote-doubling" convention).
     * https://tools.ietf.org/html/rfc959, Page 62:
     * MKD foo"bar
     * 257 "/usr/dm/foo""bar" directory created
     * CWD /usr/dm/foo"bar
     * 200 directory changed to /usr/dm/foo"bar
     * PWD
     * 257 "/usr/dm/foo""bar"
     */
    path_begin = strchr((const char *)cmdbuf_r->ptr, '"');
    path_end = strrchr((const char *)cmdbuf_r->ptr, '"');
    path_len = path_end - path_begin;
    if (path_begin == NULL || path_end == NULL || path_len < 1)
    {
        ERROR("Invalid ftp reply message: %s", cmdbuf_r->ptr);
        return TE_RC(TE_TAPI, TE_EBADMSG);
    }
    /* Save pathname. */
    directory->pathname = TE_ALLOC(path_len);
    if (directory->pathname == NULL)
        return TE_RC(TE_TAPI, TE_ENOMEM);
    memcpy((char *)directory->pathname, path_begin + 1, path_len - 1);
    ((char *)directory->pathname)[path_len - 1] = '\0';
    directory->type = TAPI_FILE_TYPE_DIRECTORY;
    return TE_RC(TE_TAPI, rc);
}

/**
 * Split @p line to slices and put it to the array of string. Note! This
 * function changes the @p line.
 * There are three examples of @p line:
 * drwxr-xr-x    7 0        0              160 Apr 13 11:06 dir1
 * drwxr-xr-x    3 0        0               50 Apr 10 2005  dir2
 * -rw-r--r--    1 0        0                6 Apr  8 16:42 file1
 * drwxrwx---    4 0        1             8192 Jan  1  1970 dir3
 *
 * @param[in]  line     Line of LIST command result to parse.
 * @param[out] file     Local files to save parsed data in. Note this
 *                      function places to file->pathname a static string
 *                      (slice of @p line), so it must not be freed, i.e.
 *                      do not call tapi_local_file_free_entry on @p file.
 *
 * @return Status code.
 * @retval 0            There are no errors, @p file contains valid data.
 * @retval TE_ENODATA   There is no data, @p file doesn't changed.
 */
static te_errno
parse_ftp_list_line(char *line, tapi_local_file *file)
{
    /*
     * Enumeration of fields in LIST command response.
     * Assumed that response contains a set of following lines:
     * mode      links owner    group         size mon day time filename
     * -rw-r--r--    1 0        0                6 Apr  8 16:42 file1
     */
    enum ftp_list_fields {
        FTPLF_MODE,
        FTPLF_LINKS,
        FTPLF_OWNER,
        FTPLF_GROUP,
        FTPLF_SIZE,
        FTPLF_MONTH,
        FTPLF_DAY,
        FTPLF_TIME_YEAR,
        FTPLF_FILENAME,

        FTPLF_TOTAL
    };
    /*
     * We assume that time/year field take a 6 symbols. And next file name
     * field cause file name can contains leading spaces and we should take
     * them.
     */
    const int     TIME_FIELD_WIDTH = 6;
    char         *ls_fields[FTPLF_TOTAL];
    size_t        i;

    for (i = FTPLF_MODE; i < FTPLF_TIME_YEAR; i++)
    {
        /* Skip leading spaces. */
        while(isspace(*line))
            line++;
        /* Save field pointer. */
        ls_fields[i] = line;
        while(*line != '\0' && !isspace(*line))
            line++;
        if (*line == '\0')
            return TE_ENODATA;
        *line++ = '\0';
    }
    /*
     * Time/year field is a special case because of the following filename
     * can contain a spaces.
     */
    ls_fields[FTPLF_TIME_YEAR] = line;
    line += TIME_FIELD_WIDTH;
    line[-1] = '\0';
    i = strlen(line) - 1;   /* The trailing character of line. */
    if (line[i] == '\r')
        line[i] = '\0';     /* Remove trailing '\r' symbol. */
    ls_fields[FTPLF_FILENAME] = line;

    /*
     * Put data to the local file.
     */
    /* File type. */
    switch (*ls_fields[FTPLF_MODE])
    {
        case '-':
            file->type = TAPI_FILE_TYPE_FILE;
            break;

        case 'd':
            file->type = TAPI_FILE_TYPE_DIRECTORY;
            break;

        default:
            return TE_ENODATA;
    }
    /* File name. */
    file->pathname = ls_fields[FTPLF_FILENAME];
    return 0;
}

/**
 * Extract files from message which is reply on command LIST.
 * Note this release extract only file type (only directory and regular
 * file) and file name.
 *
 * @param[in]  msg      Message contains reply on LIST command.
 * @param[in]  path     Path to listed file/directory.
 * @param[out] files    Container files to save to.
 *
 * @return Status code.
 */
static te_errno
extract_list_of_files(char                 *msg,
                      const char           *path,
                      tapi_local_file_list *files)
{
    tapi_local_file_le *f;
    tapi_local_file     tmp_file = { 0 };
    te_errno    rc = 0;
    char       *token;    /* File info. */
    const char *sep = (path[strlen(path) - 1] == '/' ? "" : "/");

    SLIST_INIT(files);
    for (token = strtok(msg, "\n");
         token != NULL;
         token = strtok(NULL, "\n"))
    {
        if (parse_ftp_list_line(token, &tmp_file) != 0)
            continue;
        f = TE_ALLOC(sizeof(*f));
        if (f == NULL)
        {
            rc = TE_ENOMEM;
            break;
        }
        f->file.type = tmp_file.type;
        f->file.pathname = TE_ALLOC(strlen(path) + strlen(sep) +
                                    strlen(tmp_file.pathname) + 1);
        if (f->file.pathname == NULL)
        {
            rc = TE_ENOMEM;
            free(f);
            break;
        }
        strcpy((char *)f->file.pathname, path);
        strcat((char *)f->file.pathname, sep);
        strcat((char *)f->file.pathname, tmp_file.pathname);
        SLIST_INSERT_HEAD(files, f, next);
    }
    if (rc != 0)
        tapi_local_fs_list_free(files);

    return TE_RC(TE_TAPI, rc);
}

/**
 * Find a file with name @p filename in message which is reply on LIST
 * command and extract it parametes.
 * Note this release extract only file type (only directory and regular
 * file) and file name.
 *
 * @param[in]  msg      Message contains reply on LIST command.
 * @param[in]  dirname  Path to listed file/directory.
 * @param[in]  filename Filename which info should be find and extracted.
 * @param[out] file     Local files to save info in.
 *
 * @return Status code.
 * @retval 0            There are no errors, @p file contains valid data.
 * @retval TE_ENODATA   File with @p filename not found, @p file doesn't
 *                      changed.
 */
static te_errno
extract_fileinfo(char            *msg,
                 const char      *dirname,
                 const char      *filename,
                 tapi_local_file *file)
{
    tapi_local_file tmp_file = { 0 };
    char           *token;    /* File info. */
    const char     *sep = (dirname[strlen(dirname) - 1] == '/' ? "" : "/");

    for (token = strtok(msg, "\n");
         token != NULL;
         token = strtok(NULL, "\n"))
    {
        if (parse_ftp_list_line(token, &tmp_file) != 0)
            continue;
        if (strcmp(filename, tmp_file.pathname) == 0)
        {
            file->type = tmp_file.type;
            file->pathname = TE_ALLOC(strlen(dirname) + strlen(sep) +
                                      strlen(tmp_file.pathname) + 1);
            if (file->pathname == NULL)
                return TE_ENOMEM;
            strcpy((char *)file->pathname, dirname);
            strcat((char *)file->pathname, sep);
            strcat((char *)file->pathname, tmp_file.pathname);
            return 0;
        }
    }
    return TE_ENODATA;
}

/**
 * Extract from @p pathname a both dirname and basename.
 * Both @p dirname and @p basename should be freed when they are no longer
 * needed.
 * Examples:
 * "/"      -> "/", (null)
 * "/foo"   -> "/", "foo"
 * "foo/bar -> "foo", "bar"
 * "foo"    -> ".", "foo"
 *
 * @param[in]  pathname     Pathname to extract dirname and basename from.
 * @param[out] dirname      Dirname.
 * @param[out] filename     Filename.
 *
 * @return Status code.
 */
static te_errno
split_pathname(const char *pathname, char **dirname, char **filename)
{
    char       *dir;
    char       *name;
    size_t      len;
    const char *CURRENT_DIR = ".";

    dir = TE_ALLOC(strlen(pathname) + 1);
    if (dir == NULL)
        return TE_ENOMEM;
    strcpy(dir, pathname);
    /* Remove trailing '/'. */
    while (len = strlen(dir), len > 1 && dir[len - 1] == '/')
        dir[len - 1] = '\0';
    /* Search for filename. */
    if (len == 1 && *dir == '/')
    {
        *dirname = dir;
        *filename = NULL;
        return 0;
    }
    name = strrchr(dir, '/');
    if (name == NULL)
    {
        /* There is only name. */
        *dirname = TE_ALLOC(strlen(CURRENT_DIR) + 1);
        if (*dirname == NULL)
        {
            free(dir);
            return TE_ENOMEM;
        }
        strcpy(*dirname, CURRENT_DIR);
        *filename = dir;
        return 0;
    }
    /* Separator was found. */
    name++;
    *filename = TE_ALLOC(strlen(name) + 1);
    if (*filename == NULL)
    {
        free(dir);
        return TE_ENOMEM;
    }
    strcpy(*filename, name);
    /*
     *     name[0] = '\0'    name[-1] = '\0'
     *     |                 |
     *     v                 v
     *    /foo           /foo/bar
     *    ^^             ^    ^
     *    ||             |    |
     *    |name          |    name
     *    dir            dir
     */
    name[-(name - dir != 1)] = '\0';   /* Terminate dirname. */
    *dirname = dir;
    return 0;
}

/**
 * Get file info from ftp server. Check is it either directory or regular
 * file.
 *
 * @param[in]  client       Client handle.
 * @param[in]  pathname     Pathname which info should be gotten.
 * @param[out] file         Local file to save info in.
 *
 * @return Status code.
 */
static te_errno
get_fileinfo_from_parent(tapi_storage_client *client,
                         const char          *pathname,
                         tapi_local_file     *file)
{
    tapi_storage_client_ftp_context *ftp_context = client->context;
    te_dbuf  *cmdbuf_r = &ftp_context->cmdbuf_r;
    te_dbuf  *databuf_r = &ftp_context->databuf_r;
    te_errno  con_rc;
    te_errno  rc = 0;
    char     *parent_path;
    char     *filename;

    rc = split_pathname(pathname, &parent_path, &filename);
    if (rc != 0)
        return rc;
    if (filename == NULL || strcmp(filename, ".") == 0 ||
        strcmp(filename, "..") == 0)
    {
        /* It is directory obviously. */
        free(parent_path);
        free(filename);
        file->type = TAPI_FILE_TYPE_DIRECTORY;
        return 0;
    }
    con_rc = open_data_connection(client);
    if (con_rc != 0)
        return con_rc;
    do {
        rc = send_command(client, FTP_CMD_LIST_FMT, parent_path);
        if (rc != 0)
            break;
        rc = read_data(client);
        if (rc != 0)
            break;
        /*
         * Check the reply code of control message that was received
         * by @p read_data function.
         */
        rc = check_reply_code_for_error((const char *)cmdbuf_r->ptr);
        if (rc != 0)
            break;
        /* Retrieve info about our file. */
        rc = extract_fileinfo((char *)databuf_r->ptr, parent_path, filename,
                              file);
    } while (0);
    free(parent_path);
    free(filename);
    con_rc = close_data_connection(client);
    return (rc != 0 ? rc : con_rc);
}

/**
 * Function to execute @c LS command over FTP. Note @p path should be a
 * dirname cause it will be added to the name for all found items.
 *
 * @param[in]  client       Client handle.
 * @param[in]  path         Path to the files.
 * @param[out] files        Files list.
 *
 * @return Status code.
 */
static te_errno
ftp_ls_directory(tapi_storage_client  *client,
                 const char           *path,
                 tapi_local_file_list *files)
{
    tapi_storage_client_ftp_context *ftp_context = client->context;
    te_dbuf  *cmdbuf_r = &ftp_context->cmdbuf_r;
    te_dbuf  *databuf_r = &ftp_context->databuf_r;
    te_errno  con_rc;
    te_errno  rc = 0;

    con_rc = open_data_connection(client);
    if (con_rc != 0)
        return TE_RC(TE_TAPI, con_rc);
    do {
        rc = send_command(client, FTP_CMD_LIST_FMT, path);
        if (rc != 0)
            break;
        rc = read_data(client);
        if (rc != 0)
            break;
        /*
         * Check the reply code of control message that was received
         * by @p read_data function.
         */
        rc = check_reply_code_for_error((const char *)cmdbuf_r->ptr);
        if (rc != 0)
            break;
        /* Retrieve data. */
        rc = extract_list_of_files((char *)databuf_r->ptr, path, files);
    } while (0);
    con_rc = close_data_connection(client);
    return (rc != 0 ? rc : con_rc);
}

/**
 * Hook function to execute @c LS command over FTP.
 *
 * @param[in]  client       Client handle.
 * @param[in]  path         Path to the files.
 * @param[out] files        Files list.
 *
 * @return Status code.
 */
static te_errno
ftp_ls(tapi_storage_client  *client,
       const char           *path,
       tapi_local_file_list *files)
{
    tapi_local_file_le *f;
    tapi_local_file     file = { 0 };
    te_errno rc = 0;

    assert(files != NULL);
    if (path == NULL)
        return TE_RC(TE_TAPI, TE_EINVAL);
    /*
     * Extract info about requested file, i.e. is it either regular file
     * or directory.
     */
    rc = get_fileinfo_from_parent(client, path, &file);
    if (rc != 0)
        return TE_RC(TE_TAPI, rc);
    if (file.type == TAPI_FILE_TYPE_FILE)
    {
        /* It is a file. */
        SLIST_INIT(files);
        f = TE_ALLOC(sizeof(*f));
        if (f == NULL)
            return TE_RC(TE_TAPI, TE_ENOMEM);
        f->file.type = file.type;
        f->file.pathname = file.pathname;
        SLIST_INSERT_HEAD(files, f, next);
        return 0;
    }
    tapi_local_file_free_entry(&file);
    /* It is a directory. */
    rc = ftp_ls_directory(client, path, files);
    return TE_RC(TE_TAPI, rc);
}

/**
 * Hook function to execute @c CD command over FTP.
 *
 * @param client            Client handle.
 * @param pathname          Remote directory pathname.
 *
 * @return Status code.
 */
static te_errno
ftp_cd(tapi_storage_client *client, const char *pathname)
{
    te_errno rc;

    if (pathname == NULL)
        return TE_RC(TE_TAPI, TE_EINVAL);
    rc = send_command(client, FTP_CMD_CWD_FMT, pathname);
    return TE_RC(TE_TAPI, rc);
}

/**
 * Hook function to execute @c STOR command over FTP.
 *
 * @param client        Client handle.
 * @param local_file    Local file name.
 * @param remote_file   Remote file name or @c NULL to use the same.
 *
 * @return Status code.
 */
static te_errno
ftp_put(tapi_storage_client *client,
        const char          *local_file,
        const char          *remote_file)
{
    tapi_storage_client_ftp_context *ftp_context = client->context;
    te_dbuf    *cmdbuf_r = &ftp_context->cmdbuf_r;
    te_errno    con_rc;
    te_errno    rc = 0;
    const char *dst = (remote_file == NULL ? local_file : remote_file);
    int         fd;
    rpc_stat    stbuf;
    ssize_t     sent;
    off_t       file_size;

    if (local_file == NULL)
        return TE_RC(TE_TAPI, TE_EINVAL);
    /* Open the local file. */
    fd = rpc_open(client->rpcs, local_file, RPC_O_RDONLY, 0);
    rpc_fstat(client->rpcs, fd, &stbuf);
    if (!S_ISREG(stbuf.st_mode))
    {
        rpc_close(client->rpcs, fd);
        ERROR("\"%s\" is not a regular file", local_file);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }
    file_size = stbuf.st_size;
    /* Send the file. */
    con_rc = open_data_connection(client);
    if (con_rc != 0)
    {
        rpc_close(client->rpcs, fd);
        return TE_RC(TE_TAPI, con_rc);
    }
    do {
        rc = send_command(client, FTP_CMD_STORE_FMT, dst);
        if (rc != 0)
            break;
        do {
            RPC_AWAIT_IUT_ERROR(client->rpcs);
            sent = rpc_sendfile(client->rpcs, ftp_context->data_socket, fd,
                                NULL, file_size, FALSE);
            if (sent >= 0)
                file_size -= sent;
            else
            {
                rc = RPC_ERRNO(client->rpcs);
                break;
            }
        } while (file_size > 0);
    } while (0);
    con_rc = close_data_connection(client);
    rpc_close(client->rpcs, fd);
    if (rc == 0)
    {
        rc = get_control_msg(client);
        if (rc != 0)
            ERROR("Failed to send a file: %s", cmdbuf_r->ptr);
    }
    return TE_RC(TE_TAPI, (rc != 0 ? rc : con_rc));
}

/**
 * Hook function to execute @c RETR command over FTP.
 *
 * @param client        Client handle.
 * @param remote_file   Remote file name.
 * @param local_file    Local file name or @c NULL to use the same.
 *
 * @return Status code.
 */
static te_errno
ftp_get(tapi_storage_client *client,
        const char          *remote_file,
        const char          *local_file)
{
    tapi_storage_client_ftp_context *ftp_context = client->context;
    te_dbuf    *cmdbuf_r = &ftp_context->cmdbuf_r;
    te_errno    con_rc;
    te_errno    rc = 0;
    const char *dst = (local_file == NULL ? remote_file : local_file);
    int         fd;
    int64_t     written;

    if (remote_file == NULL)
        return TE_RC(TE_TAPI, TE_EINVAL);
    /* Get the file. */
    con_rc = open_data_connection(client);
    if (con_rc != 0)
        return TE_RC(TE_TAPI, con_rc);
    do {
        rc = send_command(client, FTP_CMD_RETRIEVE_FMT, remote_file);
        if (rc != 0)
            break;
        /*
         * FIXME Get a file size from response message which looks like:
         * "150 Opening BINARY connection for /file2 (6 bytes)"
         * Then use it in function to receive the file.
         */
        fd = rpc_open(client->rpcs, dst, RPC_O_CREAT | RPC_O_WRONLY,
                      RPC_S_IRUSR | RPC_S_IWUSR | RPC_S_IRGRP);
        written = rpc_copy_fd2fd(client->rpcs, fd, ftp_context->data_socket,
                                 5000, 0);
        rpc_close(client->rpcs, fd);
        if (written == -1)
        {
            ERROR("Failed to get a file");
            rc = TE_EFAIL;
        }
        /* FIXME Check a number of written bytes. */
    } while (0);
    con_rc = close_data_connection(client);
    if (rc == 0)
    {
        rc = get_control_msg(client);
        if (rc != 0)
            ERROR("Failed to get a file: %s", cmdbuf_r->ptr);
    }
    return TE_RC(TE_TAPI, (rc != 0 ? rc : con_rc));
}

/**
 * Function to recursive remove the files on ftp server.
 *
 * @param client        Client handle.
 * @param files         List of files to remove.
 *
 * @return Status code.
 */
static te_errno
remove_files(tapi_storage_client *client, tapi_local_file_list *files)
{
    tapi_local_file_list  subfiles;
    tapi_local_file_le   *f;
    te_errno              rc = 0;

    SLIST_FOREACH(f, files, next)
    {
        if (f->file.type == TAPI_FILE_TYPE_FILE)
            rc = send_command(client, FTP_CMD_DELETE_FMT, f->file.pathname);
        else /* if (f->file.type == TAPI_FILE_TYPE_DIRECTORY) */
        {
            rc = ftp_ls_directory(client, f->file.pathname, &subfiles);
            if (rc != 0)
                break;
            rc = remove_files(client, &subfiles);
            tapi_local_fs_list_free(&subfiles);
            if (rc != 0)
                break;
            rc = send_command(client, FTP_CMD_RMDIR_FMT, f->file.pathname);
        }
        if (rc != 0)
            break;
    }
    return rc;
}

/**
 * Check if @p path is either root directory or not.
 *
 * @param path      Pathname.
 *
 * @return @c TRUE if @p path is root directory, @c FALSE in otherwise.
 */
static te_bool
is_rootdir(const char *path)
{
    size_t num_points = 0;

    while (*path != '\0')
    {
        if (*path != '.')
        {
            if (*path != '/')
                return FALSE;
            num_points = 0;
        }
        else if (++num_points > 2)
            return FALSE;
        path++;
    }
    return TRUE;
}

/**
 * Hook function to execute @c RM command over FTP.
 *
 * @param client        Client handle.
 * @param filename      Remote file name to remove.
 * @param recursive     Perform recursive removing if @c TRUE and specified
 *                      file is directory.
 *
 * @return Status code.
 */
static te_errno
ftp_rm(tapi_storage_client *client,
       const char          *filename,
       te_bool              recursive)
{
    tapi_local_file_list files;
    te_errno rc;

    if (filename == NULL)
        return TE_RC(TE_TAPI, TE_EINVAL);
    /* At first assume that it is a regular file. */
    rc = send_command(client, FTP_CMD_DELETE_FMT, filename);
    if (rc == 0)
        return 0;
    /* Removing of file is failed, assume that it is a directory. */
    if (recursive)
    {
        /* Remove subitems. */
        rc = ftp_ls_directory(client, filename, &files);
        if (rc != 0)
            return TE_RC(TE_TAPI, rc);
        rc = remove_files(client, &files);
        tapi_local_fs_list_free(&files);
        if (rc != 0)
            return TE_RC(TE_TAPI, rc);
    }
    /*
     * ftp server returns error reply code on attempt to delete root
     * directory but we are expecting another behaviour.
     */
    rc = (is_rootdir(filename) ? 0
          : send_command(client, FTP_CMD_RMDIR_FMT, filename));
    return TE_RC(TE_TAPI, rc);
}

/**
 * Hook function to execute @c MKDIR command over FTP. It creates a parent
 * directories as needed.
 *
 * @param client            Client handle.
 * @param pathname          Directory name to create on remote storage.
 *
 * @return Status code.
 */
static te_errno
ftp_mkdir(tapi_storage_client *client, const char *pathname)
{
    te_errno  rc = 0;
    char     *path = NULL;          /* Path to be split to tokens. */
    char     *token;
    char     *current_path = NULL;  /* Accumulate path to create. */
    size_t    path_len;

    if (pathname == NULL)
        return TE_RC(TE_TAPI, TE_EINVAL);

    /* Try to create directory. */
    rc = send_command(client, FTP_CMD_MKDIR_FMT, pathname);
    if (rc != TE_EFAIL)
        return TE_RC(TE_TAPI, rc);

    /*
     * Failed to create the directory, try to create intermediate (parent)
     * directories.
     */
    path_len = strlen(pathname);
    current_path = TE_ALLOC(path_len + 1);
    if (current_path == NULL)
        return TE_RC(TE_TAPI, TE_ENOMEM);
    path = TE_ALLOC(path_len + 1);
    if (path == NULL)
    {
        free(current_path);
        return TE_RC(TE_TAPI, TE_ENOMEM);
    }
    strcpy(path, pathname);

    for (token = strtok(path, "/");
         token != NULL;
         token = strtok(NULL, "/"))
    {
        /* Add '/' for either subdir or root dir. */
        if (strlen(current_path) > 0 || pathname[0] == '/')
            strncat(current_path, "/", path_len - strlen(current_path));
        /* Add subdir. */
        strncat(current_path, token, path_len - strlen(current_path));
        if (strcmp(token, ".") == 0 || strcmp(token, "..") == 0)
            continue;

        rc = send_command(client, FTP_CMD_MKDIR_FMT, current_path);
        /*
         * Here we ignoring @b rc == TE_EFAIL cause the intermediate dirs
         * may be already presents and in this case we should skip the
         * error. Only last error will be interesting for us.
         */
        if (rc != 0 && rc != TE_EFAIL)
        {
            ERROR("Failed to create \"%s\" directory", current_path);
            break;
        }
        if (rc == TE_EFAIL)
            VERB("Failed to create \"%s\", probably the directory is"
                 " already exists", current_path);
    }
    free(path);
    free(current_path);
    return TE_RC(TE_TAPI, rc);
}

/**
 * Hook function to execute @c RMDIR command over FTP. Note function returns
 * @c TE_EFAIL on delete root directory (it's normal ftp server behaviour).
 *
 * @param client        Client handle.
 * @param pathname      Directory name to remove it from the remote storage.
 *
 * @return Status code.
 */
static te_errno
ftp_rmdir(tapi_storage_client *client, const char *pathname)
{
    te_errno rc;

    if (pathname == NULL)
        return TE_RC(TE_TAPI, TE_EINVAL);
    rc = send_command(client, FTP_CMD_RMDIR_FMT, pathname);
    return TE_RC(TE_TAPI, rc);
}


/* See description in tapi_storage_client_ftp.h. */
tapi_storage_client_methods tapi_storage_client_ftp_methods = {
    .open = ftp_open,
    .close = ftp_close,
    .pwd = ftp_pwd,
    .ls = ftp_ls,
    .cd = ftp_cd,
    .put = ftp_put,
    .get = ftp_get,
    .rm = ftp_rm,
    .mkdir = ftp_mkdir,
    .rmdir = ftp_rmdir
};


/* See description in tapi_storage_client_ftp.h. */
te_errno
tapi_storage_client_ftp_context_init(
                                tapi_storage_client_ftp_context **context)
{
    tapi_storage_client_ftp_context *ftp_context;

    ftp_context = TE_ALLOC(sizeof(*ftp_context));
    if (ftp_context == NULL)
        return TE_RC(TE_TAPI, TE_ENOMEM);

    ftp_context->control_socket = -1;
    ftp_context->data_socket = -1;
    ftp_context->addr = (struct sockaddr_storage){ 0 };
    ftp_context->cmdbuf_w = (te_string)TE_STRING_INIT;
    ftp_context->cmdbuf_r = (te_dbuf)TE_DBUF_INIT(0);
    ftp_context->databuf_r = (te_dbuf)TE_DBUF_INIT(0);

    *context = ftp_context;

    return 0;
}

/* See description in tapi_storage_client_ftp.h. */
void
tapi_storage_client_ftp_context_fini(
                                tapi_storage_client_ftp_context *context)
{
    if (context != NULL)
    {
        te_string_free(&context->cmdbuf_w);
        te_dbuf_free(&context->cmdbuf_r);
        te_dbuf_free(&context->databuf_r);
        free(context);
    }
}

/* See description in tapi_storage_client_ftp.h. */
te_errno
tapi_storage_client_ftp_init(rcf_rpc_server                    *rpcs,
                             const tapi_storage_client_methods *methods,
                             tapi_storage_auth_params          *auth,
                             tapi_storage_client_ftp_context   *context,
                             tapi_storage_client               *client)
{
    assert(client != NULL);

    if (auth == NULL)
        return TE_RC(TE_TAPI, TE_EINVAL);

    client->type = TAPI_STORAGE_SERVICE_FTP;
    client->rpcs = rpcs;
    client->methods = (methods == NULL ? &tapi_storage_client_ftp_methods
                                       : methods);
    client->context = context;
    return tapi_storage_auth_params_copy(&client->auth, auth);
}

/* See description in tapi_storage_client_ftp.h. */
void
tapi_storage_client_ftp_fini(tapi_storage_client *client)
{
    client->type = TAPI_STORAGE_SERVICE_UNSPECIFIED;
    client->rpcs = NULL;
    client->methods = NULL;
    client->context = NULL;
    tapi_storage_auth_params_fini(&client->auth);
}
