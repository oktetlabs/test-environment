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

#ifdef HAVE_UNISTD_H
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

#ifdef HAVE_PTHREAD_H
#include <pthread.h>
#endif

#ifdef HAVE_STDARG_H
#include <stdarg.h>
#endif


#include "te_defs.h"
#include "te_stdint.h"
#include "te_errno.h"
#include "logger_ta.h"
#include "logfork.h"


/** 
 * Structure to store notification about
 * child processes 
 */
typedef struct udp_msg{
    int        is_notif;                  /**< Is message just notification */
    
    pid_t      pid;                       /**< Child process pid */
    int        log_level;                 /**< Log level */
    
    union  msg_str{
        char    name[LOGFORK_MAXLEN];     /**< Child process name */    
        char    log_msg[LOGFORK_MAXLEN];  /**< Message */
    } msg_str;
} udp_msg;


/** Structure to store process info */
typedef struct list {
    struct list *next;

    char name[LOGFORK_MAXLEN];    
    pid_t  pid;
} list;



static pthread_mutex_t lock_sockd = PTHREAD_MUTEX_INITIALIZER;

/** Address to which notification listerner is bound */
static struct sockaddr_in logfork_saddr;

/** Socket used by all client to register */
static int clt_sockd = -1;


/** 
 * Find process name by its pid in the internal list of
 * processes
 *
 * @param          pid  process or thread identifier
 * @param          name pointer to process or thread name
 *          
 *
 * @retval  0   -  found
 * @retval -1   -  not found
 */
int 
logfork_find_name_by_pid(list **proc_list, char **name, int pid)
{
    list *tmp = *proc_list;
    for (; tmp; tmp = tmp->next)
    {
        if (tmp->pid == pid)
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
 * @param  proc_list pointer to the list
 * @param  name  pointer to searched process name
 * @param  pid   process pid
 *
 * @retval  0    success
 * @retval  -1   memory allocation failure
 */
int
logfork_list_add(list **proc_list, char *name, 
                 unsigned int pid)
{  
    list *item = NULL;

    if((item = malloc(sizeof(*item))) == NULL)
    {
        return -1;
    }
    
    strcpy(item->name, name);
    item->pid = pid;            
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
void
logfork_destroy_list(list **list)
{
    struct list *tmp;
    
    while (*list != NULL)
    {
        tmp = (*list)->next;
        free(*list);
        *list = tmp;
    }
    list = NULL;
}

/**
 * Close opened socket and clear the list of process info.
 *
 * @param  sockd  socket descriptor
 *
 */

void
logfork_cleanup(list **list, int sockd)
{
    struct list *tmp = *list;
    if (list != NULL)
        logfork_destroy_list(&tmp);
    
    close(sockd);
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
        
    size_t  addrlen = sizeof(struct sockaddr_in);
    
    udp_msg message;
    size_t  msg_len = sizeof(message);
    
    sockd = socket(PF_INET, SOCK_DGRAM, 0);
    if (sockd < 0)
    {
        fprintf(stderr, "log_fork: Cannot create socket.\n");
        return;
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(0);
    
    if (bind(sockd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
        ERROR("bind() failed; errno %d", errno);
        logfork_cleanup(&proc_list, sockd);
        return;
    }
    
    memset(&logfork_saddr, 0, sizeof(logfork_saddr));
    
    if (getsockname(sockd, (struct sockaddr *)&logfork_saddr, &addrlen) < 0)
    {
        ERROR("getsockname(); errno %d", errno);
        logfork_cleanup(&proc_list, sockd);
        return;
    }

    while (1)
    {         
        int len;
        
        if ((len = recv(sockd, (udp_msg *)&message, msg_len, 0)) <= 0)
        {
            WARN("recv() failed, len=%d; errno %d", len, errno);
            continue;
        }
        
        if (len != sizeof(message))
        {
            ERROR("Log message length is %d instead %d", 
                  len, sizeof(message));
            continue;
        }
        
        /* If udp message */
        if (message.is_notif == 0)
        {   
                        
            if (logfork_find_name_by_pid(&proc_list, &name, message.pid) == 0)
            {
                sprintf(name_pid, "%s.%u", name, message.pid);
                log_message(message.log_level, TE_LGR_USER, name_pid,
                            "%s", message.msg_str.log_msg);
            }
        }
        else 
        {
            if (logfork_find_name_by_pid(&proc_list, &name, message.pid) 
                == 0)
                continue;

            if (logfork_list_add(&proc_list, message.msg_str.name, 
                                 message.pid) != 0)
            {
                ERROR("Out of Memory");
                break;
            }
        }
        
    } /* while(1) */
    
    logfork_cleanup(&proc_list, sockd);
}



/** See description in logfork.h */
int
logfork_register_user(const char *name)
{
    
    udp_msg notification;
    
    memset(&notification, 0, sizeof(notification));
    strcpy((notification.msg_str.name), name);
    notification.pid = getpid();
    notification.is_notif = 1;
    
#ifdef HAVE_PTHREAD_H
    pthread_mutex_lock(&lock_sockd);
#endif
    
    if (clt_sockd == -1)
    {
        clt_sockd = socket(PF_INET, SOCK_DGRAM, 0);
        if (clt_sockd < 0)
        {
            fprintf(stderr, "logfork_register_user - socket() failed.\n");
#ifdef HAVE_PTHREAD_H
            pthread_mutex_unlock(&lock_sockd);
#endif
            return -1;
        }
    }
        
    
    if (sendto(clt_sockd, (udp_msg *)&notification, sizeof(notification),
               0,(struct sockaddr *)&logfork_saddr, sizeof(logfork_saddr)) 
        < 0)
    {
        fprintf(stderr, "logfork_register_user() - cannot send "
                "notification.\n");
        return -1;
    }
    
#ifdef HAVE_PTHREAD_H
    pthread_mutex_unlock(&lock_sockd);
#endif
    
    return 0;
}


/** See description in logfork.h*/

void
logfork_log_message(int level, char *lgruser, const char *fmt, ...)
{
    udp_msg log_message;
    va_list ap;
    
    memset(&log_message, 0, sizeof(log_message));
    UNUSED(lgruser);
    va_start(ap, fmt);
    vsnprintf(log_message.msg_str.log_msg, 
              sizeof(log_message.msg_str.log_msg), fmt, ap);
    va_end(ap);

    log_message.pid = getpid();
    log_message.log_level = level;
    log_message.is_notif = 0;

#ifdef HAVE_PTHREAD_H
    pthread_mutex_lock(&lock_sockd);
#endif
    
    sendto(clt_sockd, (udp_msg *)&log_message, sizeof(log_message), 
           0, (struct sockaddr *)&logfork_saddr, sizeof(logfork_saddr));
    
#ifdef HAVE_PTHREAD_H
    pthread_mutex_unlock(&lock_sockd);
#endif

}


