/** @file
 * @brief Test Environment: IPC library
 *
 * Implemenatation of IPC PMAP routines (client side).
 *
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS in the
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
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 *
 * @author Andrey Ivanov <andron@oktetlabs.ru>
 *
 * $Id$
 */
 
#include "te_config.h"

#include "ipc_internal.h"

#ifndef IPC_UNIX

#ifdef HAVE_STDIO_H
#include <stdio.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef  HAVE_NETDB_H
#include <netdb.h>
#endif
#ifdef HAVE_RPC_PMAP_CLNT_H
#include <rpc/pmap_clnt.h>
#endif

#include "te_stdint.h"


/**
 * Connect to the IPC PMAP server, send specified command, get answer,
 * send bye-bye, get answer and disconnect.
 *
 * @param cmd_type      Type of the command.
 * @param server_name   String to be passed as server_name field in
 *                      the command body, can be NULL.
 * @param port          Number to be passed as server_port field in
 *                      the command body.
 *
 * @return Value from the server answer or zero on error.
 */
unsigned short
ipc_pmap_process_command(enum ipc_pm_command_type_e cmd_type,
                         const char *server_name, uint16_t port)
{
    struct ipc_pmap_command cmd;
    struct ipc_pmap_answer  answer;
    unsigned short          answer_data;
    long                    prg_num;
    int                     s;
    struct rpcent          *rpc;

    rpc = getrpcbyname(IPC_TE_NAME);
    if (rpc == NULL)
    {
        perror(IPC_TE_NAME " was not found in /etc/rpc file");
        return 0;
    }
    prg_num = rpc->r_number;

    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0)
    {
        perror("ipc_pmap_process_command: socket() failed");
        return 0;
    }

    /* Connect */
    {
        struct sockaddr_in  sa;
        uint16_t            port;

        memset(&sa, 0, sizeof(sa));
        port = pmap_getport(&sa, prg_num, 1, IPPROTO_TCP);

        memset(&sa, 0, sizeof(sa));
        sa.sin_family = AF_INET;
        sa.sin_port = port;
        sa.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

        if (connect(s, (struct sockaddr*)&sa, sizeof(sa)) != 0)
        {
            perror("ipc_pmap_process_command: connect() error");
            close(s);
            return 0;
        }
    }

    cmd.command_type = cmd_type;
    if (server_name != NULL)
    {
        strcpy(cmd.server_name, server_name);
    }
    cmd.server_port = port;
    if (send(s, &cmd, sizeof(cmd), 0) != sizeof(cmd))
    {
        perror("ipc_pmap_process_command: send() failed");
        close (s);
        return 0;
    }
    if (recv(s, &answer, sizeof(answer), 0) != sizeof(answer))
    {
        perror("ipc_pmap_process_command: recv() failed");
        close(s);
        return 0;
    }

    answer_data = answer.data;

    cmd.command_type = IPC_PM_BYE;
    if (send(s, &cmd, sizeof(cmd), 0) != sizeof(cmd))
    {
        perror("ipc_pmap_process_command: send() failed");
        close(s);
        return 0;
    }
    if (recv(s, &answer, sizeof(answer), 0) != sizeof(answer))
    {
        perror("ipc_pmap_process_command: recv() failed");
        close(s);
        return 0;
    }

    close(s);

    return answer_data;
}

#endif /* !IPC_UNIX */
