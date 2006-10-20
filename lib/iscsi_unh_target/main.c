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
#include <ctype.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <iscsi_target.h>
#include <scsi_target.h>
#include <iscsi_target_api.h>
#include <iscsi_custom.h>
#include <my_login.h>

#include <logger_defs.h>
#include <logger_api.h>

const char *te_lgr_entity = "iSCSI standalone target";


static FILE *logfile;

static void
stderr_logging(const char *file, unsigned int line,
               te_log_ts_sec sec, te_log_ts_usec usec,
               unsigned int level, const char *entity, const char *user,
               const char *fmt, va_list ap)
{
    time_t  curtime = sec;
    va_list args;

    UNUSED(file);
    UNUSED(line);
    UNUSED(entity);
    UNUSED(user);

    fprintf(stderr, "[%d %s %u us] ",
            level, ctime(&curtime), (unsigned)usec);
    va_copy(args, ap);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fputc('\n', stderr);

    fprintf(logfile, "[%d %s %u us] ",
            level, ctime(&curtime), (unsigned)usec);
    vfprintf(logfile, fmt, ap);
    fputc('\n', logfile);
}

static sig_atomic_t need_async;
static void
send_async_message(int signo)
{
    UNUSED(signo);

    need_async = 1;
}

te_log_message_f te_log_message_va = stderr_logging;

extern int iscsi_server_init();

void
logfork_register_user(const char *name)
{
    UNUSED(name);
}

static pid_t server_pid = (pid_t)-1;

static void
exit_handler(void)
{
    if (server_pid != (pid_t)-1)
    {
        int unused;
        
        kill(server_pid, SIGTERM);
        wait(&unused);
    }
}

int
main(int argc, char *argv[])
{
    int                tcp_listen_socket;
    int                tcp_data_socket;
    int                server_data_socket;
    struct sockaddr_in listen_to;
    char             **iter;

    UNUSED(argc);

    logfile = fopen("target.log", "a");
    if (logfile == NULL)
    {
        perror("can't open log file");
        return EXIT_FAILURE;
    }
    TRACE(VERBOSE, "Initializing");
    server_pid = fork();
    if (server_pid == 0)
    {
        iscsi_server_init();
        exit(0);
    }
    else if (server_pid == (pid_t)-1)
    {
        TRACE_ERROR("fork() failed");
        exit(EXIT_FAILURE);
    }
    atexit(&exit_handler);
    sleep(1);

    tcp_listen_socket = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(tcp_listen_socket, SOL_SOCKET, SO_REUSEADDR, NULL, 0);
    listen_to.sin_family = AF_INET;
    listen_to.sin_port   = htons(3260);
    listen_to.sin_addr.s_addr = INADDR_ANY;
    if (bind(tcp_listen_socket, (struct sockaddr *)&listen_to, 
             sizeof(listen_to)) != 0)
    {
        perror("bind");
        return EXIT_FAILURE;
    }
    listen(tcp_listen_socket, 5);
    fputs("\nListen for incoming connection\n", stderr);

    
    for (iter = argv + 1; *iter != NULL; iter += 2)
    {
        if (strcmp(*iter, "security") == 0)
        {
            iscsi_target_send_msg(NULL, NULL, 
                                  "security", 
                                  "%s %s",
                                  iter[1], iter[2]);
            iter++;
        }
        else if (strcmp(*iter, "backfile") == 0)
        {
            iscsi_target_send_msg(NULL, NULL,
                                  "mmap",
                                  "0 0 %s", iter[1]);
        }
        else if (strcmp(*iter, "verbosity") == 0)
        {
            iscsi_target_send_msg(NULL, NULL,
                                  "verbosity", "%s",
                                  iter[1]);
        }
        else
        {
            iscsi_target_send_msg(NULL, NULL,
                                  "set", "%s=%s",
                                  *iter, iter[1]);
        }
    }
    
    {
        struct sigaction sa;
        sa.sa_handler = send_async_message;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags   = 0;
        sigaction(SIGQUIT, &sa, NULL);
    }
    
    for (;;)
    {
        tcp_data_socket = accept(tcp_listen_socket, NULL, NULL);
        if (tcp_data_socket < 0)
        {
            TRACE_ERROR("accept() failed");
            exit(EXIT_FAILURE);
        }
        server_data_socket = iscsi_target_connect();
        if (server_data_socket < 0)
        {
            TRACE_ERROR("Cannot connect to the target process");
            exit(EXIT_FAILURE);
        }
        for (;;)
        {
            static char buffer[4096];
            ssize_t datasize;

            fd_set readers;
            
            int result;

            FD_ZERO(&readers);
            
            FD_SET(tcp_data_socket, &readers);
            FD_SET(server_data_socket, &readers);
            
            result = select(MAX(tcp_data_socket, server_data_socket) + 1, 
                            &readers, NULL, NULL, NULL);
            if (result > 0)
            {
                if (FD_ISSET(tcp_data_socket, &readers))
                {
                    datasize = read(tcp_data_socket, buffer, sizeof(buffer));
                    if (datasize <= 0)
                    {
                        close(tcp_data_socket);
                        close(server_data_socket);
                        break;
                    }
                    write(server_data_socket, buffer, datasize);
                }
                if (FD_ISSET(server_data_socket, &readers))
                {
                    datasize = read(server_data_socket, buffer, sizeof(buffer));
                    if (datasize <= 0)
                    {
                        close(tcp_data_socket);
                        close(server_data_socket);
                        break;
                    }
                    write(tcp_data_socket, buffer, datasize);
                }
            }
        }
    }

    return 0;
}
