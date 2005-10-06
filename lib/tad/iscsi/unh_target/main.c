/** @file
 * @brief iSCSI target
 *
 * Main file for a standalone target
 *
 *
 * Copyright (C) 2004 Test Environment authors (see file AUTHORS
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

#include <te_config.h>

#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <tad_iscsi_impl.h>
#include <iscsi_target.h>
#include <my_login.h>

#include <logger_defs.h>
#include <logger_api.h>

const char *te_lgr_entity = "iSCSI standalone target";

static void
stderr_logging(const char   *file,
               unsigned int  line,
               unsigned int  level,
               const char   *entity,
               const char   *user,
               const char   *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    UNUSED(file);
    UNUSED(line);
    UNUSED(entity);
    UNUSED(user);
    fprintf(stderr, "[%d] ", level);
    vfprintf(stderr, fmt, args);
    va_end(args);
}

int 
iscsi_tad_send(int sock, uint8_t *buffer, size_t len)
{
    int result = write(sock, buffer, len);
    return result >= 0 ? result : -errno;
}

int 
iscsi_tad_recv(int sock, uint8_t *buffer, size_t len)
{
    int result = read(sock, buffer, len);
    return result >= 0 ? result : -errno;
}

te_log_message_f te_log_message = stderr_logging;

extern int iscsi_server_init();

int main(int argc, char *argv[])
{
    static iscsi_target_thread_params_t config;

    int                server_socket;
    int                data_socket;
    struct sockaddr_in listen_to;
    char             **iter;
    pthread_t          thread;

    UNUSED(argc);

    TRACE_SET(TRACE_ALL); 
    TRACE(TRACE_VERBOSE, "Initializing");
    iscsi_server_init();
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, NULL, 0);
    listen_to.sin_family = AF_INET;
    listen_to.sin_port   = htons(3260);
    listen_to.sin_addr.s_addr = INADDR_ANY;
    if (bind(server_socket, (struct sockaddr *)&listen_to, 
             sizeof(listen_to)) != 0)
    {
        perror("bind");
        return EXIT_FAILURE;
    }
    listen(server_socket, 5);
    fputs("\nListen for incoming connection\n", stderr);

    for (iter = argv + 1; *iter != NULL; iter++)
    {
        configure_parameter(KEY_TO_BE_NEGOTIATED,
                            *iter,
                            *devdata->param_tbl);
    }

    for(;;)
    {
        data_socket = accept(server_socket, NULL, NULL);
        if (data_socket < 0)
        {
            perror("accept");
            return EXIT_FAILURE;
        }
        config.send_recv_csap = data_socket;
        config.reject = 0;
        fputs("Accepted\n", stderr);
        if (pthread_create(&thread, NULL, iscsi_server_rx_thread, &config) == 0)
            fputs("thread created\n", stderr);
    }
    return 0;
}
