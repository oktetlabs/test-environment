/** @file
 * @brief Tester Subsystem
 *
 * TCE data collector
 *
 *
 * Copyright (C) 2005 Test Environment authors (see file AUTHORS
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
 * @author Artem V. Andreev <Artem.Andreev@oktetlabs.ru>
 *
 * $Id$
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <fnctl.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include "posix_tar.h"

static char tar_file_prefix[PATH_MAX + 1];

typedef struct bb_function_info 
{
    long checksum;
    int arc_count;
    const char *name;
    long long *counts;
} bb_function_info;

/* Structure emitted by --profile-arcs  */
typedef struct bb_object_info
{
    int peer_id;
    const char *filename;
    long ncounts;
    struct bb_object_info *next;
    struct bb_function_info *function_infos;
} bb_object_info;


typedef struct channel_data channel_data;
struct channel_data
{
    int fd;
    void (*state)(channel_data *me);
    char buffer[128];
    char *bufptr;
    int remaining;
    bb_object_info *object;
    bb_function_info *fi;
    long long *counter;
    channel_data *next;
};


int 
main(int argc, char *argv[])
{
    fd_set active;
    fd_set sockets;
    fd_set current;
    int listen_on = -1;
    int max_fd = -1;
    te_bool is_socket;


    FD_ZERO(&active);
    FD_ZERO(&sockets);
    for (argv++; *argv; argv++)
    {
        is_socket = FALSE;
        listen_on = -1;
        if (strncmp(*argv, "fifo:", 5) == 0)
        {
            listen_on = open(*argv + 5, O_RDONLY | O_NONBLOCK, 0);
            if (listen_on < 0)
            {
                fprintf(stderr, "tce_collector: can't open '%s' (%s), skipping\n", 
                        *argv + 5, strerror(errno));
            }
        }
        else if (strcmp(*argv, "unix:", 5) == 0)
        {
            struct sockaddr_un addr;
            is_socket = TRUE;
            listen_on = socket(PF_UNIX, SOCK_STREAM, 0);
            if (listen_on < 0)
            {
                fprintf(stderr, "tce_collector: can't create local socket (%s)\n", 
                        strerror(errno));
            }
            else
            {
                addr.sun_family = AF_UNIX;
                strcpy(addr.sun_path, *argv + 5);
                if (bind(listen_on, &addr, sizeof(addr)))
                {
                    fprintf(stderr, "tce_collector: can't bind to local socket %s (%s)\n",
                            *argv + 5, strerror(errno));
                    close(listen_on);
                    listen_on = -1;
                }
            }
        }
        else if (strcmp(*argv, "tcp:", 4) == 0)
        {
            char *tmp;
            struct sockaddr_in addr;
            is_socket = TRUE;
            listen_on = socket(PF_INET, SOCK_STREAM, 0);
            if (listen_on < 0)
            {
                fprintf(stderr, "tce_collector: can't create TCP socket (%s)\n", 
                        strerror(errno));
            }
            else
            {
                addr.sin_family = AF_INET;
                addr.sin_port = htons(strtoul(*argv + 4, &tmp, 0));
                
                if (addr.sin_port == 0)
                {
                    fprintf(stderr, "tce_collector: no port specified at '%'\n", *argv);
                    return EXIT_FAILURE;
                }

                if (*tmp)
                {
                    addr.sin_addr.s_addr = 0;
                }
                else
                {
                    inet_pton(AF_INET, tmp + 1, &addr.sin_addr);
                }
                setsockopt(listen_on, SOL_SOCKET, SO_REUSEADDR, NULL, 0);
                if (bind(listen_on, &addr, sizeof(addr)))
                {
                    fprintf(stderr, "tce_collector: can't bind to TCP socket %d:%s (%s)\n",
                            ntohs(addr.sin_port),
                            *tmp ? tmp + 1 : "*", strerror(errno));
                    close(listen_on);
                    listen_on = -1;
                }
            }
        }
        else
        {
            fprintf(stderr, "tce_collector: invalid argument '%s'\n", *argv);
            return EXIT_FAILURE;
        }
        if (listen_socket >= 0)
        {
            if (is_socket)
            {
                fcntl(listen_on, F_SETFL, O_NONBLOCK | fcntl(listen_on, F_GETFL));
                if (listen(listen_on, 5))
                {
                    fprintf(stderr, "tce_collector: can't listen at '%s'\n", *argv);
                    close(listen_on);
                    continue;
                }
                FD_SET(listen_on, &sockets);
            }
            FD_SET(listen_on, &active);
            if (listen_on > max_fd)
                max_fd = listen_on;
        }
    }

    if (max_fd < 0)
    {
        fputs("tce_collector: no channels specified\n", stderr);
        return EXIT_FAILURE;
    }
    for (;;)
    {
        memcpy(&current, &active, sizeof(current));
        result = select(max_fd + 1, &current, NULL, NULL, NULL);
        if (result < 0)
        {
            if (errno == EINTR)
            {
                if (catched_signo == SIGHUP)
                    dump_data();
                else if (catched_signo == SIGTERM)
                {
                    dump_data();
                    break;
                }
                catched_signo = 0;
            }
            else
            {
                fprintf(stderr, "tce_collector: select error %s\n", strerror(errno));
            }
        }
        else
        {
            int fd;
            for (fd = 0; fd <= max_fd && result; fd++)
            {
                if (FD_ISSET(fd, &current))
                {
                    result--;
                    if (FD_ISSET(fd, &sockets))
                    {
                        listen_on = accept(fd, NULL, NULL);
                        if (listen_on < 0)
                        {
                            fprintf(stderr, "tce_collector: accept error %s\n", strerror(errno));
                        }
                        else
                        {
                            fcntl(listen_on, F_SETFL, O_NONBLOCK | fcntl(listen_on, F_GETFL));
                            FD_SET(listen_on, &active);
                            if (listen_on > max_fd)
                                max_fd = listen_on;
                        }
                    }
                    else
                    {
                        read_data(channel);
                    }
                }
            }
        }
    }
}


void
read_data(int channel)
{
    static channel_data *channels;
    channel_data *found;
    
    for (found = channels; found != NULL; found = found->next)
    {
        if (found->fd == channel && found->state != NULL)
            break;
    }
    if (found == NULL)
    {
        found = calloc(1, sizeof(*found));
        found->state = auth_state;
        found->bufptr = found->buffer;
        found->remaining = sizeof(found->buffer) - 1;
        found->next = channels;
        channels = found;
    }
    collect_line(found);
}

void
collect_line(channel_data *ch)
{
    int len;
    len = read(ch->fd, ch->bufptr, ch->remaining);
    if (len < 0)
    {
        if (errno != EPIPE)
        {
            fprintf(stderr, "tce_collector: read error on %d: %s\n", ch->fd,
                    strerror(errno));
        }
        ch->state = NULL;
    }
    else
    {
        te_bool found_newline = memchr(ch->bufptr, '\n', len);
        ch->bufptr += len;
        ch->remaining -= len;
        if (found_newline)
        {
            *ch->bufptr = '\0';
            ch->bufptr = ch->buffer;
            ch->remaining = sizeof(ch->buffer) - 1;
            ch->state(ch);
        }
        else if (ch->remaining == 0)
        {
            fprintf(stderr, "tce_collector: too long line on %d\n", ch->fd);
            ch->state = NULL;
        }
    }
}

#define BB_HASH_SIZE 11113
static object_info *bb_hash_table[BB_HASH_SIZE];

static unsigned hash(int peer_id, const char *str)
{
    unsigned int ret = peer_id;
    unsigned int ctr = 0;


    while (*str)
    {
        ret ^= (*str++ & 0xFF) << ctr;
        ctr = (ctr + 1) % sizeof (void *);
    }
    return ret % BB_HASH_SIZE;
}


bb_object_info *get_object_info(int peer_id, const char *filename)
{
    
}




