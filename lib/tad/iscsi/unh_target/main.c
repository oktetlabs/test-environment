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


static FILE *logfile;

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
    fputc('\n', stderr);
    va_end(args);
    va_start(args, fmt);
    fprintf(logfile, "[%d] ", level);
    vfprintf(logfile, fmt, args);
    fputc('\n', logfile);
    va_end(args);
}

int 
iscsi_tad_send(int sock, uint8_t *buffer, size_t len)
{
    int result = write(sock, buffer, len);
    unsigned i;
    int width = 0;

    fputs("\n> ", stderr);
#if 1
    for (i = 0; i < len; i++)
    {
        if (iscntrl(buffer[i]) || !isascii(buffer[i]))
        {
            fprintf(stderr, "\\%2.2x", buffer[i]);
            width += 3;
        }
        else
        {
            fputc(buffer[i], stderr);
            width++;
        }
        if (width > 32)
        {
            fputs("\\\n> ", stderr);
            width = 0;
        }
    }
    fputc('\n', stderr);
#endif
    return result >= 0 ? result : -errno;
}

int 
iscsi_tad_recv(int sock, uint8_t *buffer, size_t len)
{
    int result = read(sock, buffer, len);
    unsigned i;
#if 1
    for (i = 0; i < (unsigned)result; i++)
        fprintf(stderr, "%2.2x %s", buffer[i], i % 8 == 7 ? "\n" : "");
#endif
    return result >= 0 ? result : -errno;
}

te_log_message_f te_log_message = stderr_logging;

extern int iscsi_server_init();

int main(int argc, char *argv[])
{
    iscsi_target_thread_params_t *config;

    int                server_socket;
    int                data_socket;
    struct sockaddr_in listen_to;
    char             **iter;
    pthread_t          thread;
    int                int_val;
    int                neg_mode = KEY_TO_BE_NEGOTIATED;

    UNUSED(argc);

    logfile = fopen("target.log", "a");
    if (logfile == NULL)
    {
        perror("can't open log file");
        return EXIT_FAILURE;
    }
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
        if (strcmp(*iter, "force") == 0)
        {
            iter++;
            if (strcmp(*iter, "s") == 0)
                devdata->force |= USE_SECURITY_PHASE;
            else if (strcmp(*iter, "o") == 0)
                devdata->force |= USE_OPERATIONAL_PHASE;
            else if (strcmp(*iter, "r") == 0)
                devdata->force |= USE_FULL_REPLIES;
            else if (strcmp(*iter, "xok") == 0)
                devdata->force |= USE_REFLECT_XKEYS;
            else if (strcmp(*iter, "tk1") == 0)
                devdata->force |= USE_ONE_KEY_PER_TEXT;
            else if (strncmp(*iter, "n=", 2) == 0)
                devdata->nop_period =
                    strtoul(*iter + 2, NULL, 0) * 100;
            else if (strncmp(*iter, "v=", 2) == 0) {	
                /* formerly set draft number to enforce */
                int_val = strtoul(*iter + 2, NULL, 0);
                if (int_val != DRAFT20) {
                    TRACE(TRACE_ISCSI, "Draft number %u ignored\n",
                          int_val / DRAFT_MULTIPLIER);
                }
            }

            /* set phase-collapse choice */
            else if (strncmp(*iter, "p=", 2) == 0) {
                int_val = strtoul(*iter + 2, NULL, 0);
                if (int_val > 2 || int_val < 0) {
                    TRACE_ERROR("Bad value for phase-collapse "
                                "setting: %d", int_val);
                } else
                    devdata->phase_collapse = int_val;
            }
            /* set the r2t period - SAI */
            else if (strncmp(*iter, "r2tp=", 5) == 0) {
                devdata->r2t_period =
                    strtoul(*iter + 5, NULL, 0) * 100;
            }
            /* chap and srp support - CHONG */
            else if (strcmp(*iter, "t") == 0) {
                TRACE(TRACE_ISCSI, "target confirmation enabled\n");
                devdata->auth_parameter.auth_flags |=
                    USE_TARGET_CONFIRMATION;
            } else if (strcmp(*iter, "b") == 0) {
                TRACE(TRACE_ISCSI, "base64 number enabled\n");
                devdata->auth_parameter.auth_flags |= USE_BASE64;
                devdata->auth_parameter.chap_local_ctx->number_format =
                    BASE64_FORMAT;
                devdata->auth_parameter.chap_peer_ctx->number_format =
                    BASE64_FORMAT;
            } else if (strncmp(*iter, "px=", 3) == 0) {
                TRACE(TRACE_ISCSI, "CHAP peer secret set to %s\n",
                      *iter + 3);
                CHAP_SetSecret(*iter + 3,
                               devdata->auth_parameter.chap_peer_ctx);
            } else if (strncmp(*iter, "pn=", 3) == 0) {
                TRACE(TRACE_ISCSI, "CHAP peer name set to %s\n",
                      *iter + 3);
                CHAP_SetName(*iter + 3,
                             devdata->auth_parameter.chap_peer_ctx);
            } else if (strncmp(*iter, "lx=", 3) == 0) {
                TRACE(TRACE_ISCSI, "CHAP local secret set to %s\n",
                      *iter + 3);
                CHAP_SetSecret(*iter + 3,
                               devdata->auth_parameter.chap_local_ctx);
            } else if (strncmp(*iter, "ln=", 3) == 0) {
                TRACE(TRACE_ISCSI, "CHAP local name set to %s\n",
                      *iter + 3);
                CHAP_SetName(*iter + 3,
                             devdata->auth_parameter.chap_local_ctx);
            } else if (strncmp(*iter, "cl=", 3) == 0) {
                int_val = strtoul(*iter + 3, NULL, 0);
                if (int_val <= 0 || int_val > MAX_CHAP_BINARY_LENGTH) {
                    TRACE_ERROR("invalid CHAP challenge length %d\n",
                                int_val);
                }
                TRACE(TRACE_ISCSI, "challenge length set to %d\n",
                      int_val);
                CHAP_SetChallengeLength(int_val,
                                        devdata->auth_parameter.
                                        chap_local_ctx);
            } else if (strncmp(*iter, "sx=", 3) == 0) {
                TRACE(TRACE_ISCSI, "SRP secret set to \"%s\"\n",
                      *iter + 3);
                SRP_SetSecret(*iter + 3,
                              devdata->auth_parameter.srp_ctx);
            } else if (strncmp(*iter, "sn=", 3) == 0) {
                TRACE(TRACE_ISCSI, "SRP name set to \"%s\"\n",
                      *iter + 3);
                SRP_SetName(*iter + 3,
                            devdata->auth_parameter.srp_ctx);
            } else {
                TRACE_ERROR("unknown force \"%s\"\n", 
                            *iter);
            }
        }
        else if (strcmp(*iter, "permanent") == 0)
        {
            neg_mode = 0;
        }
        else if (strcmp(*iter, "fixed") == 0)
        {
            neg_mode = KEY_TO_BE_NEGOTIATED | KEY_BREAK_CONN_OR_RJT;
        }
        else if (strcmp(*iter, "negotiate") == 0)
        {
            neg_mode = KEY_TO_BE_NEGOTIATED;
        }
        else
        {
            configure_parameter(neg_mode,
                                *iter,
                                *devdata->param_tbl);
        }
    }

    for(;;)
    {
        data_socket = accept(server_socket, NULL, NULL);
        if (data_socket < 0)
        {
            perror("accept");
            return EXIT_FAILURE;
        }
        config = malloc(sizeof(*config)); 
/** will be freed by iscsi_server_tx_thread **/
        config->send_recv_csap = data_socket;
        config->reject = 0;
        fputs("Accepted\n", stderr);
        if (pthread_create(&thread, NULL, iscsi_server_rx_thread, config) == 0)
            fputs("thread created\n", stderr);
    }
    return 0;
}
