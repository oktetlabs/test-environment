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
#include <sys/socket.h>
#include <netinet/in.h>

#include <logger_defs.h>
#include <logger_api.h>

#include <iscsi_target.h>
#include <my_login.h>

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
    fprintf(stderr, "%s:%d %d %s:%s ", file, line, level, entity, user);
    vfprintf(stderr, fmt, args);
    va_end(args);
}

int 
iscsi_tad_send(int sock, uint8_t *buffer, size_t len)
{
    return write(sock, buffer, len);
}

int 
iscsi_tad_recv(int sock, uint8_t *buffer, size_t len)
{
    return read(sock, buffer, len);
}

te_log_message_f te_log_message = stderr_logging;

extern int iscsi_server_init();

int main()
{
    static struct iscsi_thread_param config;
    int server_socket;
    struct sockaddr_in listen_to;

    iscsi_server_init();
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    listen_to.sin_family = AF_INET;
    listen_to.sin_port   = htons(3260);
    listen_to.sin_addr.s_addr = INADDR_ANY;
    bind(server_socket, (struct sockaddr *)&listen_to, sizeof(listen_to));
    listen(server_socket, 5);
    config.send_recv_csap = 0;
    config.reject = 0;
    iscsi_server_rx_thread(&config);
    return 0;
}
