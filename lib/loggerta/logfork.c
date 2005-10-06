/** @file
 * @brief Logger subsystem API - TA side
 *
 * TA side Logger functionality for
 * forked TA processes and newly created threads
 * 
 *
 * Copyright (C) 2005 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
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
 *
 * @author Mamadou Ngom <Mamadou.Ngom@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER     "LogFork"

#include "te_config.h"

#if HAVE_STDIO_H
#include <stdio.h>
#endif
#if HAVE_SYS_TYPES_H
#include <sys/socket.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#endif 
#if HAVE_STRINGS_H
#include <strings.h>
#endif 
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#if HAVE_PTHREAD_H
#include <pthread.h>
#endif
#if HAVE_STDARG_H
#include <stdarg.h>
#endif
#if HAVE_FCNTL_H
#include <fcntl.h>
#endif

#include "te_defs.h"
#include "te_stdint.h"
#include "te_errno.h"
#include "logger_ta.h"
#include "logfork.h"
#include "te_tools.h"


/** Common information in the message */
typedef struct udp_msg {
    te_bool  is_notif;
    pid_t    pid;
    uint32_t tid;
    union {
        struct {
            char name[LOGFORK_MAXUSER]; /** Logfork user name */
        } notify;
        struct {
            int  log_level;                /**< Log level */
            char lgr_user[32];             /**< Log user  */
            char log_msg[LOGFORK_MAXLEN];  /**< Message   */
        } log;
    } msg;
} udp_msg;

/** Macros for fast access to structure fields */
#define __log_level  msg.log.log_level
#define __lgr_user   msg.log.lgr_user
#define __log_msg    msg.log.log_msg 
#define __name       msg.notify.name


/** Structure to store process info */
typedef struct list {
    struct list *next;

    char     name[LOGFORK_MAXLEN];    
    pid_t    pid;
    uint32_t tid;
} list;

/** Socket used by all client to register */
static int logfork_clnt_sockd = -1;

#if HAVE_PTHREAD_H
static pthread_mutex_t logfork_clnt_sockd_lock =
                           PTHREAD_MUTEX_INITIALIZER;
#endif

/** 
 * Find process name by its pid and tid in the internal list of
 * processes
 *
 * @param pid   process or thread identifier
 * @param tid   thread identifier      
 * @param name  pointer to process or thread name
 *
 * @retval  0      found
 * @retval -1      not found
 */
static int 
logfork_find_name_by_pid(list **proc_list, char **name, int pid, 
                         uint32_t tid)
{
    list *tmp = *proc_list;
    
    for (; tmp; tmp = tmp->next)
    {
        if (tmp->pid == pid && tmp->tid == tid)
        {
            *name = tmp->name;
            return 0;
        }
    }
    return -1;
}

/** 
 * Used to add process info in the internal list
 *
 * @param  proc_list  pointer to the list
 * @param  name       pointer to searched process name
 * @param  pid        process pid
 * @param  tid        thread pid
 *
 * @retval  0    success
 * @retval  -1   memory allocation failure
 */
static int
logfork_list_add(list **proc_list, char *name, 
                 pid_t pid, uint32_t tid)
{  
    list *item = NULL;

    if((item = malloc(sizeof(*item))) == NULL)
    {
        return -1;
    }
    
    strcpy(item->name, name);
    item->pid = pid;            
    item->tid = tid;
    if (*proc_list == NULL)
    {
        *proc_list = item; 
        (*proc_list)->next = NULL;
    }
    else
    {
        item->next = *proc_list;
        *proc_list = item;
    }
    
    return 0;
}

/**
 * Destroy the internal list of process info, when some failure
 * happens. When everything is going well this routine is never
 * called. Memory is destroyed when TA is going down. 
 *
 * @param  list   pointer to the list
 */
static void
logfork_destroy_list(list **list)
{
    struct list *tmp;
    
    while (*list != NULL)
    {
        tmp = (*list)->next;
        free(*list);
        *list = tmp;
    }
}

/**
 * Close opened socket and clear the list of process info.
 *
 * @param  sockd  socket descriptor
 */
static void
logfork_cleanup(list **list, int sockd)
{
    if (list != NULL)
        logfork_destroy_list(list);
    
    (void)close(sockd);
}


/** Thread entry point */
void 
logfork_entry(void)
{
    struct sockaddr_in servaddr;
    
    int   sockd = -1;
    char *name;
    char  name_pid[LOGFORK_MAXLEN];
    list *proc_list = NULL;
    char  port[16];
        
    socklen_t   addrlen = sizeof(struct sockaddr_in);
    
    udp_msg msg;
    size_t  msg_len = sizeof(msg);

    sockd = socket(PF_INET, SOCK_DGRAM, 0);
    if (sockd < 0)
    {
        fprintf(stderr, "logfork_entry(): cannot create socket\n");
        return;
    }

#if HAVE_FCNTL_H
    /* 
     * Try to set close-on-exec flag, but ignore failures, 
     * since it's not critical.
     */
    (void)fcntl(sockd, F_SETFD, FD_CLOEXEC);
#endif

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    servaddr.sin_port = htons(0);
    
    if (bind(sockd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
        ERROR("logfork_entry(): bind() failed; errno %d", errno);
        logfork_cleanup(&proc_list, sockd);
        return;
    }
    
    if (getsockname(sockd, (struct sockaddr *)&servaddr, &addrlen) < 0)
    {
        ERROR("logfork_entry(): getsockname() failed ; errno %d", errno);
        logfork_cleanup(&proc_list, sockd);
        return;
    }
    
    sprintf(port, "%d", ntohs(servaddr.sin_port));
    if (setenv("TE_LOG_PORT", port, 1) < 0)
    {
        int err = TE_OS_RC(TE_RCF_PCH, errno);
        
        ERROR("Failed to set TE_LOG_PORT environment variable: "
              "error=%r", err);
    }

    while (1)
    {         
        int len;
        
        if ((len = recv(sockd, &msg, msg_len, 0)) <= 0)
        {
            WARN("logfork_entry(): recv() failed, len=%d; errno %d", 
                 len, errno);
            continue;
        }
        
        if (len != sizeof(msg))
        {
            ERROR("logfork_entry(): log message length is %d instead %d", 
                  len, sizeof(msg));
            continue;
        }
        
        /* If udp message */
        if (!msg.is_notif)
        {   
            if (logfork_find_name_by_pid(&proc_list, &name, 
                                         msg.pid, msg.tid) != 0)
            {
                name = "Unnamed";
            }
            sprintf(name_pid, "%s.%u.%u",
                    name, (unsigned)msg.pid, (unsigned)msg.tid);

            te_log_message(__FILE__, __LINE__,
                           msg.__log_level, TE_LGR_ENTITY, 
                           strdup(msg.__lgr_user), 
                           "%s: %s", name_pid, msg.__log_msg);
        }
        else 
        {
            if (logfork_find_name_by_pid(&proc_list, &name, 
                                         msg.pid, msg.tid) == 0)
            {
                snprintf(name, LOGFORK_MAXUSER, "%s", msg.__name);
                continue;
            }

            if (logfork_list_add(&proc_list, msg.__name, 
                                 msg.pid, msg.tid) != 0)
            {
                ERROR("logfork_entry(): out of Memory");
                break;
            }
        }
        
    } /* while(1) */
    
    logfork_cleanup(&proc_list, sockd);
}


/**
 * Open client socket. 
 *
 * @retval 0    Success
 * @retval -1   Failure
 *
 * @note should be called under lock
 */
static inline int
open_sock(void)
{
    char *port;
    int   sock;
    
    struct sockaddr_in addr;

    if ((port = getenv("TE_LOG_PORT")) == NULL)
    {
        fprintf(stderr, "Failed to register logfork user: "
                        "TE_LOG_PORT is not exported\n");
        return -1;
    }

    sock = socket(PF_INET, SOCK_DGRAM, 0);
    if (sock < 0)
    {
        fprintf(stderr, 
                "Failed to register logfork user: socket() failed\n");
        return -1;
    }
    
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = htons(atoi(port));
    
    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        fprintf(stderr, 
                "Failed to register logfork user: - connect() failed\n");
        close(sock);
        return -1;
    }

#if HAVE_PTHREAD_H
    pthread_mutex_lock(&logfork_clnt_sockd_lock);
#endif
    if (logfork_clnt_sockd < 0)
    {
        logfork_clnt_sockd = sock;
#if HAVE_PTHREAD_H
        pthread_mutex_unlock(&logfork_clnt_sockd_lock);
#endif
    }
    else
    {
#if HAVE_PTHREAD_H
        pthread_mutex_unlock(&logfork_clnt_sockd_lock);
#endif
        close(sock);
    }

    return 0;
}

/** See description in logfork.h */
int
logfork_register_user(const char *name)
{
    udp_msg msg;
    
    memset(&msg, 0, sizeof(msg));
    strncpy(msg.__name, name, sizeof(msg.__name) - 1);
    msg.pid = getpid();
#if HAVE_PTHREAD_H
    msg.tid = (uint32_t)pthread_self();
#endif    
    msg.is_notif = TRUE;
    
    if (logfork_clnt_sockd == -1 && open_sock() != 0)
    {
        return -1;
    }

    if (send(logfork_clnt_sockd, (udp_msg *)&msg, sizeof(msg), 0) !=
            (ssize_t)sizeof(msg))
    {
        fprintf(stderr, "logfork_register_user() - cannot send "
                "notification: %s\n", strerror(errno));
        return -1;
    }
    
    return 0;
}

/** 
 * Function for logging to be used by forked processes.
 *
 * This function complies with te_log_message_f prototype.
 */
void 
logfork_log_message(const char *file, unsigned int line,
                    unsigned int level, const char *entity,
                    const char *user, const char *fmt, ...)
{
    va_list ap;
    udp_msg msg;

    struct te_log_out_params cm =
        { NULL, msg.__log_msg, sizeof(msg.__log_msg), 0 };

    UNUSED(file);
    UNUSED(line);
    UNUSED(entity);

    memset(&msg, 0, sizeof(msg));
    cm.buf = msg.__log_msg;
    cm.buflen = sizeof(msg.__log_msg);

    va_start(ap, fmt);
    (void)te_log_vprintf(&cm, fmt, ap);
    va_end(ap);

    msg.pid = getpid();
#if HAVE_PTHREAD_H
    msg.tid = (uint32_t)pthread_self();
#endif    
    msg.is_notif = FALSE;
    strncpy(msg.__lgr_user, user, sizeof(msg.__lgr_user) - 1);
    msg.__log_level = level;

    if (logfork_clnt_sockd == -1 && open_sock() != 0)
    {
        fprintf(stderr, "%s(): %s %s\n", __FUNCTION__, user,
                msg.__log_msg);
        return;
    }

    if (send(logfork_clnt_sockd, (udp_msg *)&msg, sizeof(msg), 0) !=
            (ssize_t)sizeof(msg))
    {       
        fprintf(stderr, "%s(): sendto() failed: %s\n",
                __FUNCTION__, strerror(errno));
    }
}
