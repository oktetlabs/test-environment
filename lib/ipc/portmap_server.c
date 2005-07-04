/** @file
 * @brief Test Environment: IPC library
 *
 * Definition of IPC PMAP routines (server side) provided for library user
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
 * @author Andrey Ivanov <andron@oktet.ru>
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
 */
 
#include "te_config.h"

#include "ipc_internal.h"
#include "ipc_server.h"

#ifdef TE_IPC_AF_UNIX

/* See description in ipc_server.h */
int
ipc_init(void)
{
    /* Nothing to do */
    return 0;
}

/* See description in ipc_server.h */
int
ipc_kill(void)
{
    /* Nothing to do */
    return 0;
}

#else

#include "te_config.h"

#include <stdio.h>

#ifdef  HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef  HAVE_ASSERT_H
#include <assert.h>
#endif
#ifdef  HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef  HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef  HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef  HAVE_NETDB_H
#include <netdb.h>
#endif
#ifdef HAVE_RPC_PMAP_CLNT_H
#include <rpc/pmap_clnt.h>
#endif

#include "te_errno.h"


#if 0
/** Turn pmap debug ON */
#define PMAP_DEBUG(_x...)   printf(_x)
#else
/** Turn pmap debug OFF */
#define PMAP_DEBUG(_x...)
#endif


/** This single linked-list is used to store pairs (server, port). */
struct ipc_pmap_node {
    struct ipc_pmap_node *next; /**< Pointer to next item or NULL */
    char name[UNIX_PATH_MAX];   /**< Name of the server */
    unsigned short port;        /**< Port number */
};

/** Pool of (server, port) pairs. */
static struct ipc_pmap_node *ipc_pool;


/**
 * Search for server with specified name in the pool and return
 * pointer to item.
 *
 * @param server_nam    Name of the server.
 *
 * @return Pointer to the ipc_pmap_node structure or NULL if server
 *         not found.
 */
static struct ipc_pmap_node *
ipc_get_server_by_name(const char *server_name)
{
    struct ipc_pmap_node *item = ipc_pool;

    while (item != NULL)
    {
        if (strcmp(item->name, server_name) == 0)
        {
            PMAP_DEBUG("Found server: %s->%d\n", item->name, item->port);
            return item;
        }
        item = item->next;
    }

    return 0;
}


/**
 * Search for server with specified name in the pool and delete it.
 *
 * @param       server_name     Name of the server.
 *
 * @return
 *      Status code.
 *
 * @retval      0       Success.
 * @retval      -1      Server with such name not found.
 */
static int
ipc_del_server_by_name(const char *server_name)
{
    struct ipc_pmap_node *item = ipc_pool;
    struct ipc_pmap_node **parent = &ipc_pool;

    while (item != NULL)
    {
        if (strcmp(item->name, server_name) == 0)
        {
            PMAP_DEBUG("Deleting server: %s->%d\n",
                       item->name, item->port);
            *parent = item->next;
            free(item);
            return 0;

        }
        parent = &item->next;
        item = item->next;
    }

    PMAP_DEBUG("Can not find server for deleting: >%s<\n", server_name);

    return -1;
}

/**
 * Main cycle of the IPC PMAP server.
 *
 * @param s     socket to listen to
 */
static void
ipc_pmap_cycle(int s)
{
    int finish = 0;

    while (finish == 0)
    {
        int new_s = accept(s, NULL, 0);
        int close_connect = 0;

        while (!close_connect)
        {
            struct ipc_pmap_command cmd;
            struct ipc_pmap_answer answer;
            
            memset(&answer, 0, sizeof(answer));

            if (recv(new_s, &cmd, sizeof(cmd), 0) != sizeof(cmd))
            {
                perror("Can not read cmd\n");
                exit(EXIT_FAILURE);
            }

            switch(cmd.command_type)
            {
                case IPC_PM_REG_SERVER:
                    if (ipc_get_server_by_name(cmd.server_name) != NULL)
                    {
                        PMAP_DEBUG("REG_SERVER: Server %s already "
                                   "registered\n", cmd.server_name);
                        answer.data = 0;
                    }
                    else
                    {
                        struct ipc_pmap_node *next = ipc_pool;

                        ipc_pool = calloc(1, sizeof(*ipc_pool));
                        memcpy(ipc_pool->name, cmd.server_name,
                               UNIX_PATH_MAX);
                        ipc_pool->port = cmd.server_port;
                        ipc_pool->next = next;
                        PMAP_DEBUG("REG_SERVER: Added %s<->%d\n",
                                   ipc_pool->name, ipc_pool->port);
                        answer.data = 1;
                    }
                    break;

                case IPC_PM_UNREG_SERVER:
                    PMAP_DEBUG(("Entered UNREG_SERVER\n"));
                    if (ipc_del_server_by_name(cmd.server_name) != 0)
                    {
                        PMAP_DEBUG("UNREG_SERVER: Can't del %s\n",
                                   cmd.server_name);
                        answer.data = 0;
                    }
                    else
                    {
                        answer.data = 1;
                        PMAP_DEBUG("UNREG_SERVER: Deleted %s\n",
                                   cmd.server_name);
                    }
                    break;

                case IPC_PM_GET_SERVER:
                {
                    struct ipc_pmap_node *item;

                    PMAP_DEBUG("Entered IPC_PM_GET_SERVER\n");
                    item = ipc_get_server_by_name(cmd.server_name);

                    PMAP_DEBUG("Returned from search\n");
                    if (item == NULL)
                    {
                        answer.data = 0;
                        /* Server not found */
                        PMAP_DEBUG("GET_SERVER: Server not found: >%s<\n",
                                   cmd.server_name);
                    }
                    else
                    {
                        answer.data = item->port;
                        PMAP_DEBUG("GET_SERVER: Returning: >%d<\n",
                                   answer.data);
                    }
                    PMAP_DEBUG(("Leaving IPC_PM_GET_SERVER\n"));
                    break;
                }

                case IPC_PM_BYE:
                    PMAP_DEBUG(("IPC_PM_BYE\n"));
                    close_connect = 1;
                    answer.data = 1;
                    break;

                case IPC_PM_KILL:
                    PMAP_DEBUG(("IPC_PM_KILL\n"));
                    finish = 1;
                    answer.data = 1;
                    break;
            }

            if (send(new_s, &answer, sizeof(answer), 0) != sizeof(answer))
            {
                perror("ipc pmap server: send() error");
                return;
            }
        }
        close(new_s);
    }
}


/* See description in ipc_server.h */
int
ipc_init(void)
{
    int r;
    int s;
    int cntr = 100;
    long prg_num;
    struct rpcent *rpc;

    rpc = getrpcbyname(IPC_TE_NAME);
    if (rpc == NULL)
    {
        perror(IPC_TE_NAME " was not found in /etc/rpc file");
        return 0;
    }
    prg_num = rpc->r_number;

    while (TRUE)
    {
        /* Create a socket */
        s = socket(AF_INET, SOCK_STREAM, 0);
        assert(s != -1);

        { /* Assign a name to socket */
            struct sockaddr_in addr;

            memset(&addr, 0, sizeof(addr));

            /* Listen */
            if (listen(s, SOMAXCONN) != 0)
            {
                perror("ipc_init(): listen() failed");
                return errno;
            }

            {
                int l = sizeof(addr);

                if (getsockname(s, (struct sockaddr*)&addr, &l) != 0)
                {
                    perror("ipc_init(): getsockname() failed");
                    return errno;
                }
            }

            PMAP_DEBUG("binded to port %d\n", addr.sin_port);

            /* Unset port number if it set early */
            pmap_unset(prg_num, 1);

            /* Public the port number */
            if (pmap_set(prg_num, 1, IPPROTO_TCP, addr.sin_port) == 0)
            {
                /*
                 * Sometimes Linux binds socket to the port below 1024.
                 * It is not alloowed to public such ports. So we'll try
                 * to listen/public a number of times.
                 */
                if (cntr != 0)
                {
                    cntr--;
                    close(s);
                    continue;
                }
                else
                {
                    perror("ipc_init(): can not do pmap_set()");
                    close(s);
                    return errno;
                }
            }
            else
            {
                break;
            }
        }
    }

    /* Fork a process */
    r = fork();
    if (r == 0)
    {
        /* Child */
        ipc_pmap_cycle(s);
        close(s);
        exit(EXIT_SUCCESS);
    }

    if (r == -1)
    {
        perror("ipc_init(): fork() failed");
        return errno;
    }

    return 0;
}


/* See description in ipc_server.h */
int
ipc_kill(void)
{
    /* We have to send KILL message to PM-server */
    if (!ipc_pmap_process_command(IPC_PM_KILL, 0, 0))
    {
        return -1;
    }

    return 0;
}

#endif
