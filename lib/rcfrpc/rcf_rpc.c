/** @file
 * @brief SUN RPC control interface
 *
 * RCF RPC implementation.
 *
 *
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
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
 * @author Elena A. Vengerova <Elena.Vengerova@oktetlabs.ru>
 *
 * $Id$
 */

/* To get pthread_mutexattr_settype() definition in pthread.h */
#define _GNU_SOURCE 

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdarg.h>

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_PTHREAD_H
#include <pthread.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif
#ifdef HAVE_SEMAPHORE_H
#include <semaphore.h>
#else
#error "We have no semaphores"
#endif

#include "te_defs.h"
#include "te_stdint.h"
#include "te_errno.h"
#include "rcf_api.h"
#include "rcf_rpc.h"
#include "tarpc.h"

#define TE_LGR_USER     "RCF RPC"
#include "logger_api.h"
#include "logger_ten.h"


#undef FORK_FORWARD

static pid_t rcf_rpc_pid = -1;

/** Initialize mutex and forwarding semaphore for RPC server */
static int
rpc_server_sem_init(rcf_rpc_server *rpcs)
{
    pthread_mutexattr_t attr;
    
    pthread_mutexattr_init(&attr);
    if (pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP) != 0 ||
        pthread_mutex_init(&rpcs->lock, &attr) != 0)
    {
        ERROR("Cannot initialize mutex for RPC server");
        return TE_RC(TE_RCF_API, errno); 
    }

    if (sem_init(&rpcs->fw_sem, 0, 0) != 0)
    {
        ERROR("Cannot initialize semaphore for RPC server");
        return TE_RC(TE_RCF_API, errno);
    }
    
    return 0;
}


/**
 * Create RPC server.
 *
 * @param ta            a test agent
 * @param name          name of the new server
 * @param p_handle      location for new RPC server handle
 *
 * @return status code
 */
int 
rcf_rpc_server_create(const char *ta, const char *name,
                      rcf_rpc_server **p_handle)
{
    rcf_rpc_server *rpcs;
    
    int rc;
    int rc1;
    
    if (rcf_rpc_pid < 0)
        rcf_rpc_pid = getpid();

    if (ta == NULL || p_handle == NULL || name == NULL ||
        strlen(name) >= RCF_RPC_NAME_LEN)
    {
        return TE_RC(TE_RCF, EINVAL);
    }

    if ((rpcs = calloc(1, sizeof(*rpcs))) == NULL)
        return TE_RC(TE_RCF_API, ENOMEM);
        
    if ((rc = rcf_ta_call(ta, 0, "tarpc_server_create", &rpcs->pid, 1, 0,
                          RCF_STRING, name)) != 0)
    {
        free(rpcs);
        return rc;
    }
        
    if (rpcs->pid < 0)
    {
        free(rpcs);
        return TE_RC(TE_RCF_API, ETETARTN);
    }
    
    strcpy(rpcs->ta, ta);
    strcpy(rpcs->name, name);
    rpcs->op = RCF_RPC_CALL_WAIT;
    rpcs->def_timeout = RCF_RPC_DEFAULT_TIMEOUT;
    if ((rc = rpc_server_sem_init(rpcs)) != 0)
        return rc;

    if ((rc = rcf_ta_call(rpcs->ta, 0, "tarpc_add_server", &rc1, 2, 0,
                          RCF_STRING, rpcs->name, 
                          RCF_INT32, rpcs->pid)) != 0 ||
         (rc = rc1) != 0)
    {
        if (rcf_ta_kill_task(rpcs->ta, 0, rpcs->pid) != 0)
        {
            ERROR("Cannot connect kill RPC server process");
        }
        free(rpcs);
        return rc;
    }

    RING("RPC server (%s,%s) created, pid %d", 
         rpcs->ta, rpcs->name, rpcs->pid);
    
    *p_handle = rpcs;
        
    return 0;
}

static void
delete_thread(rcf_rpc_server *rpcs, int tid)
{
    tarpc_pthread_cancel_in  in;
    tarpc_pthread_cancel_out out;
    
    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));
    
    in.tid = tid;

    rcf_rpc_call(rpcs, _pthread_cancel, 
                 &in, (xdrproc_t)xdr_tarpc_pthread_cancel_in,
                 &out, (xdrproc_t)xdr_tarpc_pthread_cancel_out);
                 
    if (rpcs->_errno != 0 || out.retval != 0)
    {
        ERROR("Cannot delete thread TID %d on the RPC server %s", 
              tid, rpcs->name);
    }
}

/**
 * Create thread in the process with RPC server.
 *
 * @param rpcs          existing RPC server handle
 * @param name          name of the new server
 * @param p_new         location for new RPC server handle
 *
 * @return status code
 */
int 
rcf_rpc_server_thread_create(rcf_rpc_server *rpcs, const char *name,
                             rcf_rpc_server **p_new)
{
    int rc;
    int rc1;

    rcf_rpc_server *tmp;
    
    tarpc_pthread_create_in in;
    tarpc_pthread_create_out out;
    
    if (rpcs == NULL || p_new == NULL || name == NULL ||
        strlen(name) >= RCF_RPC_NAME_LEN)
    {
        return TE_RC(TE_RCF, EINVAL);
    }

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));
    strcpy(in.name, name);

    if ((tmp = (rcf_rpc_server *)calloc(1, sizeof(*tmp))) == NULL)
        return TE_RC(TE_RCF_API, ENOMEM);

    rcf_rpc_call(rpcs, _pthread_create, 
                 &in, (xdrproc_t)xdr_tarpc_pthread_create_in,
                 &out, (xdrproc_t)xdr_tarpc_pthread_create_out);
                 
    if (rpcs->_errno != 0 || out.retval != 0)
    {
        ERROR("Cannot create thread on the RPC server %s", rpcs->name);
        free(tmp);
        return rpcs->_errno;
    }
    strcpy(tmp->ta, rpcs->ta);
    strcpy(tmp->name, name);
    tmp->op = RCF_RPC_CALL_WAIT;
    tmp->def_timeout = rpcs->def_timeout;
    tmp->father = rpcs;
    tmp->pid = rpcs->pid;
    tmp->tid = out.tid;
    if ((rc = rpc_server_sem_init(tmp)) != 0)
        return rc;
    rpcs->children++;
    if ((rc = rcf_ta_call(rpcs->ta, 0, "tarpc_add_server", &rc1, 2, 0,
                          RCF_STRING, tmp->name,
                          RCF_INT32, tmp->pid)) != 0 ||
         (rc = rc1) != 0)
                          
    {
        delete_thread(rpcs, tmp->tid);
        free(tmp);
        return rc;
    }
    
    *p_new = tmp;
       
    return 0;
}

/**
 * Fork RPC server.
 *
 * @param rpcs          existing RPC server handle
 * @param name          name of the new server
 * @param p_new         location for new RPC server handle
 *
 * @return status code
 */
int 
rcf_rpc_server_fork(rcf_rpc_server *rpcs, const char *name,
                    rcf_rpc_server **p_new)
{
    int rc, rc1;

    rcf_rpc_server *tmp;

    tarpc_fork_in  in;
    tarpc_fork_out out;

    if (rpcs == NULL || p_new == NULL || name == NULL ||
        strlen(name) >= RCF_RPC_NAME_LEN)
    {
        return TE_RC(TE_RCF, EINVAL);
    }

    if ((tmp = calloc(1, sizeof(*tmp))) == NULL)
        return TE_RC(TE_RCF_API, ENOMEM);

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));
    strcpy(in.name, name);

    rcf_rpc_call(rpcs, _fork, &in, (xdrproc_t)xdr_tarpc_fork_in, 
                 &out, (xdrproc_t)xdr_tarpc_fork_out);
                 
    if (rpcs->_errno != 0 || out.pid < 0)
    {
        ERROR("fork() failed on the RPC server %s", rpcs->name);
        free(tmp);
        return rpcs->_errno;
    }
    
    strcpy(tmp->ta, rpcs->ta);
    strcpy(tmp->name, name);
    tmp->op = RCF_RPC_CALL_WAIT;
    tmp->pid = out.pid;
    tmp->def_timeout = rpcs->def_timeout;
    if ((rc = rpc_server_sem_init(tmp)) != 0)
        return rc;

    if ((rc = rcf_ta_call(rpcs->ta, 0, "tarpc_add_server", &rc1, 2, 0,
                          RCF_STRING, tmp->name,
                          RCF_INT32, tmp->pid)) != 0 ||
         (rc = rc1) != 0)
    {
        ERROR("Remote call of tarpc_add_server failed for %s.", rpcs->name);
        if (rcf_ta_kill_task(rpcs->ta, 0, tmp->pid) != 0)
        {
            ERROR("Cannot kill RPC server process");
        }
        free(tmp);
        return rc;
    }
    
    *p_new = tmp;

    RING("RPC (%s,%s) fork() -> '%s' success, ppid %d, pid %d", 
         rpcs->ta, rpcs->name, tmp->name, (int)rpcs->pid, (int)tmp->pid);

    return 0;
}

/**
 * Perform execve() on the RPC server. Filename of the running process
 * if used as the first argument.
 *
 * @param rpcs          RPC server handle
 *
 * @return status code
 */
int 
rcf_rpc_server_exec(rcf_rpc_server *rpcs)
{
    pid_t old_pid;
    tarpc_execve_in     in;
    tarpc_execve_out    out;
    tarpc_getpid_in     getpid_in;
    tarpc_getpid_out    getpid_out;
    
    int rc, rc1;

    if (rpcs == NULL)
        return TE_RC(TE_RCF, EINVAL);

    old_pid = rpcs->pid;
        
    if (pthread_mutex_lock(&rpcs->lock) != 0)
        ERROR("pthread_mutex_lock() failed");
        
    if (rpcs->tid != 0)
    {
        ERROR("Cannot remove the RPC server - children exist");
        if (pthread_mutex_unlock(&rpcs->lock) != 0)
           ERROR("pthread_mutex_unlock() failed");
        return TE_RC(TE_RCF, EBUSY);
    }

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    rpcs->op = RCF_RPC_CALL;
    rcf_rpc_call(rpcs, _execve, &in, (xdrproc_t)xdr_tarpc_execve_in, 
                 &out, (xdrproc_t)xdr_tarpc_execve_out);
                 
    if (rpcs->_errno != 0)
    {
        ERROR("Failed to call execve() on the RPC server %s", rpcs->name);
        if (pthread_mutex_unlock(&rpcs->lock) != 0)
            ERROR("pthread_mutex_unlock() failed");
        return rpcs->_errno;
    }
    rpcs->op = RCF_RPC_CALL_WAIT;
    rpcs->tid0 = 0;

    if ((rc = rcf_ta_call(rpcs->ta, 0, "tarpc_del_server", &rc1, 1, 0,
                          RCF_STRING, rpcs->name)) != 0 ||
        (rc = rc1) != 0)
    {
        rpcs->dead = TRUE;
        ERROR("Remote call of tarpc_del_server failed for %s. "
              "RPC server is unusable.", rpcs->name);
        if (rcf_ta_kill_task(rpcs->ta, 0, rpcs->pid) != 0)
        {
            ERROR("Cannot kill RPC server process");
        }
        if (pthread_mutex_unlock(&rpcs->lock) != 0)
            ERROR("pthread_mutex_unlock() failed");
        return rc;
    }

    if ((rc = rcf_ta_call(rpcs->ta, 0, "tarpc_add_server", &rc1, 2, 0,
                          RCF_STRING, rpcs->name,
                          RCF_INT32, rpcs->pid)) != 0 ||
         (rc = rc1) != 0)
    {
        rpcs->dead = TRUE;
        ERROR("Remote call of tarpc_add_server failed for %s. "
              "RPC server is unusable.", rpcs->name);
        if (rcf_ta_kill_task(rpcs->ta, 0, rpcs->pid) != 0)
        {
            ERROR("Cannot kill RPC server process");
        }
        if (pthread_mutex_unlock(&rpcs->lock) != 0)
            ERROR("pthread_mutex_unlock() failed");
        return rc;
    }


    memset(&getpid_in, 0, sizeof(getpid_in));
    memset(&getpid_out, 0, sizeof(getpid_out));

    rcf_rpc_call(rpcs, _getpid,
                 &getpid_in,  (xdrproc_t)xdr_tarpc_getpid_in, 
                 &getpid_out, (xdrproc_t)xdr_tarpc_getpid_out);
                 
    if (rpcs->_errno != 0)
    {
        ERROR("Failed to call getpid() on the RPC server %s", rpcs->name);
        if (pthread_mutex_unlock(&rpcs->lock) != 0)
            ERROR("pthread_mutex_unlock() failed");
        return rpcs->_errno;
    }
    rpcs->pid = getpid_out.retval;

    if ((rc = rcf_ta_call(rpcs->ta, 0, "tarpc_set_server_pid", &rc1, 2, 0,
                          RCF_STRING, rpcs->name,
                          RCF_INT32, rpcs->pid)) != 0 ||
         (rc = rc1) != 0)
    {
        ERROR("Remote call of tarpc_set_server_pid failed for %s.",
              rpcs->name);
        if (pthread_mutex_unlock(&rpcs->lock) != 0)
            ERROR("pthread_mutex_unlock() failed");
        return rc;
    }
    if (pthread_mutex_unlock(&rpcs->lock) != 0)
        ERROR("pthread_mutex_unlock() failed");

    RING("RPC (%s,%s) exec() success, old pid %d, new pid %d", 
         rpcs->ta, rpcs->name, (int)old_pid, (int)rpcs->pid);
    
    return 0;
}


/**
 * Destroy RPC server. The caller should assume the RPC server non-existent 
 * even if the function returned non-zero.
 *
 * @param rpcs          RPC server handle
 *
 * @return status code
 */
int 
rcf_rpc_server_destroy(rcf_rpc_server *rpcs)
{
    int rc;
    int rc1;
    
    if (rpcs == NULL)
        return TE_RC(TE_RCF, EINVAL);
    
    VERB("Detroy RPC server %s", rpcs->name);
    
    if (pthread_mutex_lock(&rpcs->lock) != 0)
        ERROR("pthread_mutex_lock() failed");
    
    if (rpcs->tid != 0)
    {
        delete_thread(rpcs->father, rpcs->tid);
        rpcs->father->children--;
    }
    else
    {
        if (rpcs->children > 0)
        {
            ERROR("Cannot remove the RPC server - children exist");
            if (pthread_mutex_unlock(&rpcs->lock) != 0)
               ERROR("pthread_mutex_unlock() failed");
            return TE_RC(TE_RCF, EBUSY);
        }
        if (rcf_ta_kill_task(rpcs->ta, 0, rpcs->pid) != 0)
        {
            ERROR("Cannot kill RPC server process");
        }
        VERB("Process is killed");
    }
    VERB("Removing the server from the list");
    if ((rc = rcf_ta_call(rpcs->ta, 0, "tarpc_del_server", &rc1, 1, 0,
                          RCF_STRING, rpcs->name)) != 0 ||
        (rc = rc1) != 0)
    {
        ERROR("Remote call of tarpc_del_server failed for %s",
              rpcs->name);
        if (pthread_mutex_unlock(&rpcs->lock) != 0)
            ERROR("pthread_mutex_unlock() failed");
        return rc;
    }
    VERB("Removed OK");

    if (pthread_mutex_unlock(&rpcs->lock) != 0)
        ERROR("pthread_mutex_unlock() failed");
    if (pthread_mutex_destroy(&rpcs->lock) != 0)
        ERROR("pthread_mutex_destroy() failed");

    free(rpcs);
    
    VERB("RPC server is destroyed successfully");

    return 0;
}


/**
 * Forwarder context cleanup function.
 * 
 * @param unused    UNUSED
 */
static void
forward_cleanup(void *unused)
{
    UNUSED(unused);

    rcf_api_cleanup();
    log_client_close();
}

/**
 * Forward data of RPC call to the remote RPC server.
 *
 * @param arg   RPC server parameters
 *
 * @return nothing
 */
static void *
forward(void *arg)
{
    struct sockaddr_un addr;
    
    rcf_rpc_server *rpcs = (rcf_rpc_server *)arg;
    
    int sock;
    int s = -1;
    int len;
    int rc;
    
    char *tmp = getenv("TE_TMP");
    
    char *buf;
    char  file[RCF_MAX_PATH] = {0, };
    char  rfile[RCF_MAX_PATH] = {0, };
    FILE *f = NULL;

#ifndef FORK_FORWARD    
    pthread_cleanup_push(forward_cleanup, NULL); 
#endif    

    VERB("Forward RPC");

    if (tmp == NULL)
        tmp = "/tmp";
        
    if ((buf = (char *)calloc(1, RCF_RPC_MAX_BUF)) == NULL)
    {
        rpcs->_errno = TE_RC(TE_RCF_API, ENOMEM);
        log_client_close();
        return NULL;
    }

    if ((sock = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
    {
        rpcs->_errno = TE_RC(TE_RCF_API, errno);
        free(buf);
        return NULL;
    }
    memset(&addr, '\0', sizeof(addr));
    addr.sun_family = AF_UNIX;
#ifdef FORK_FORWARD    
    sprintf(addr.sun_path, "tmp/te_rcfrpc_pipe_%u",
            (unsigned int)rcf_rpc_pid);
#else    
    sprintf(addr.sun_path, "tmp/te_rcfrpc_pipe_%u_%u",
            (unsigned int)rcf_rpc_pid, (unsigned int)pthread_self()); 
#endif            
    sprintf(file, "%s/te_rcfrpc_file_%u_%u", tmp,
            (unsigned int)rcf_rpc_pid, (unsigned int)pthread_self());
    len = sizeof(addr.sun_family) + strlen(addr.sun_path) + 1;

    if (bind(sock, (struct sockaddr *)&addr, len) < 0 ||
        listen(sock, 1) < 0)
    {
        rpcs->_errno = TE_RC(TE_RCF_API, errno);
        ERROR("Cannot receive connection from RPC client - bind() failed");
        close(sock);
        free(buf);
        return NULL;
    }
#ifndef FORK_FORWARD    
    sem_post(&rpcs->fw_sem) < 0;
#endif        
    len = sizeof(addr);
    VERB("Waiting for the connection");
    if ((s = accept(sock, (struct sockaddr *)&addr, &len)) < 0)
    {
        rpcs->_errno = TE_RC(TE_RCF_API, errno);
        ERROR("Cannot receive connection from RPC client - "
              "accept() failed");
        close(sock);
        free(buf);
        return NULL;
    }
    close(sock);
    VERB("Local RPC server %s accepted connection", addr.sun_path);
    len = 0;
    while (1)
    {
        struct timeval tv;
        fd_set         set;
        int            n;

        tv.tv_sec = RCF_RPC_EOR_TIMEOUT / 1000000;
        tv.tv_usec = RCF_RPC_EOR_TIMEOUT % 1000000;
        FD_ZERO(&set);
        FD_SET(s, &set);
        if (select(s + 1, &set, NULL, NULL, &tv) <= 0)
        {
            if (len == 0)
            {
                rpcs->_errno = TE_RC(TE_RCF_API, ENODATA);
                ERROR("Cannot read data from RPC client");
                goto release;
            }
            else
                break;
        }
        
        if (len == RCF_RPC_MAX_BUF)
        {
            rpcs->_errno = TE_RC(TE_RCF_API, ENOMEM);
            ERROR("RPC data are too long - increase RCF_RPC_MAX_BUF");
            goto release;
        }
        
        if ((n = read(s, buf + len, RCF_RPC_MAX_BUF - len)) < 0)
        {
            rpcs->_errno = TE_RC(TE_RCF_API, errno);
            ERROR("Cannot read data from RPC client");
            goto release;
        }
        len += n;
        VERB("%d bytes are read from pipe", n);
    }
    VERB("%d bytes of RPC data are read", len);
     
    if ((f = fopen(file, "w")) == NULL)
    {
        rpcs->_errno = TE_RC(TE_RCF_API, errno);
        ERROR("Cannot open file %s for RPC data for writing", file);
        goto release;
    }
    if (fwrite(buf, len, 1, f) <= 0)
    {
        rpcs->_errno = TE_RC(TE_RCF_API, errno);
        ERROR("Cannot write RPC data to file");
        goto release;
    }
    fclose(f);
    f = NULL;
    VERB("RPC request saved in file %s", file);
    
    sprintf(rfile, "/tmp/te_rcfrpc_%u_%u", 
            (uint32_t)rcf_rpc_pid, (uint32_t)pthread_self());

    if ((rpcs->_errno = rcf_ta_put_file(rpcs->ta, 0, file, rfile)) != 0)
    {
        ERROR("Cannot put file %s with RPC data to the TA", file);
        goto release;
    }

    VERB("File with RPC request put on TA %s - %s",
                         rpcs->ta, rfile);

    if ((rpcs->_errno = rcf_ta_call(rpcs->ta, 0, "tarpc_call", &rc, 3, 0,
                                    RCF_UINT32, rpcs->timeout,
                                    RCF_STRING, rpcs->name, 
                                    RCF_STRING, rfile)) != 0)
    {
        ERROR("Remote call of tarpc_call failed");
        goto release;
    }
    if (rc != 0)
    {
        rpcs->_errno = rc;
        goto release;
    }

    VERB("tarpc_call OK");
            
    if ((rpcs->_errno = rcf_ta_get_file(rpcs->ta, 0, rfile, file)) != 0)
    {
        ERROR("Cannot get file %s with RPC data from the TA", rfile);
        goto release;
    }
    
    VERB("File with RPC reply got from on TA %s", rpcs->ta);

    if ((f = fopen(file, "r")) == NULL)
    {
        rpcs->_errno = TE_RC(TE_RCF_API, errno);
        ERROR("Cannot open file %s for RPC data for reading", file);
        goto release;
    }
    if ((len = fread(buf, 1, RCF_RPC_MAX_BUF, f)) <= 0)
    {
        rpcs->_errno = TE_RC(TE_RCF_API, errno);
        ERROR("Cannot read RPC data from the file");
        goto release;
    }
    if (write(s, buf, len) < len)
    {
        rpcs->_errno = TE_RC(TE_RCF_API, errno);
        ERROR("Cannot write RPC data to the pipe");
    }
    VERB("Reply (length %d) is forwarded", len); 

release:
    if (s >= 0)
    {
        if (close(s) != 0)
            ERROR("close() failed errno %d", errno);
    }
        
    if (f != NULL)
        fclose(f);
        
    if (*file != 0)
        unlink(file);
    
    if (*rfile != 0)
        rcf_ta_call(rpcs->ta, 0, "ta_rtn_unlink", &rc, 1, 0,
                    RCF_STRING, rfile);
        
    free(buf);

#ifndef FORK_FORWARD    
    pthread_cleanup_pop(TRUE); 
#endif    
    
    return NULL;
}

/**
 * Call SUN RPC on the TA via RCF. The function is also used for
 * checking of status of non-blocking RPC call and waiting for
 * the finish of the non-blocking RPC call.
 *
 * @param rpcs          RPC server
 * @param proc          RPC to be called
 * @param in_arg        Input argument
 * @param in_proc       Function for handling of the input argument
 *                      (generated)
 * @param out_arg       Output argument
 * @param out_proc      Function for handling of output argument
 *                      (generated)
 *
 * @attention The status code is returned in rpcs _errno.
 *            If rpcs is NULL the function does nothing.
 */
void
rcf_rpc_call(rcf_rpc_server *rpcs, int proc,
             void *in_arg, xdrproc_t in_proc, 
             void *out_arg, xdrproc_t out_proc)
{
    CLIENT   *clnt;
    char      host[RCF_MAX_PATH];
#ifndef FORK_FORWARD
    pthread_t t;
    void     *retval;
#endif    
                      
    struct timeval tv;
    
    tarpc_in_arg  *in = (tarpc_in_arg *)in_arg;
    tarpc_out_arg *out = (tarpc_out_arg *)out_arg;

    VERB("Calling RPC %d", proc);

    if (rpcs == NULL)
        return;
        
    if (rpcs->dead)
    {
        rpcs->_errno = TE_RC(TE_RCF, ETERPCDEAD);
        return;
    }
    
    if (in_arg == NULL || out_arg == NULL || 
        in_proc == NULL ||  out_proc == NULL)
    {
        rpcs->_errno = TE_RC(TE_RCF, EINVAL);
        return;
    }
    pthread_mutex_lock(&rpcs->lock);
    rpcs->_errno = 0;
    if (rpcs->timeout == 0)
        rpcs->timeout = rpcs->def_timeout;
    
    if (rpcs->op == RCF_RPC_CALL || rpcs->op == RCF_RPC_CALL_WAIT)
    {
        if (rpcs->tid0 != 0)
        {
            rpcs->_errno = TE_RC(TE_RCF_API, EBUSY);
            pthread_mutex_unlock(&rpcs->lock);
            return;
        }
    }
    else if (rpcs->tid0 == 0)
    {
        rpcs->_errno = TE_RC(TE_RCF_API, EALREADY);
        pthread_mutex_unlock(&rpcs->lock);
        return;
    }
    
    VERB("Create forwarding thread");
#ifdef FORK_FORWARD    
    if (fork() == 0)
    {
        forward(rpcs);
        exit(1);
    }
    sprintf(host, "tmp/te_rcfrpc_pipe_%u", (unsigned int)rcf_rpc_pid);
    {
        struct stat st;

        while (stat(host, &st) < 0)
            usleep(1000);
        usleep(1000);
    }
#else    
    if (pthread_create(&t, NULL, forward, (void *)rpcs) != 0)
    {
        ERROR("pthread_create() failed %d", errno);
        rpcs->_errno = TE_RC(TE_RCF_API, errno);
        pthread_mutex_unlock(&rpcs->lock);
        return;
    }
    sem_wait(&rpcs->fw_sem);

    sprintf(host, "tmp/te_rcfrpc_pipe_%u_%u",
            (unsigned int )rcf_rpc_pid, (unsigned int)t); 
#endif    

    if ((clnt = clnt_create(host, 1, 1, "unix")) == NULL)
    {
        rpcs->_errno = TE_RC(TE_RCF_API, errno);
        ERROR("%s:%s", host, clnt_spcreateerror("clnt_create"));
#ifndef FORK_FORWARD    
        pthread_cancel(t);
#endif        
        pthread_mutex_unlock(&rpcs->lock);
        return;
    }

    strcpy(in->name, rpcs->name);
    in->start = rpcs->start;
    in->op = rpcs->op;
    in->tid = rpcs->tid0;

    tv.tv_sec = rpcs->timeout / 1000;
    tv.tv_usec = (rpcs->timeout % 1000) * 1000;
    rpcs->stat = clnt_call(clnt, proc, in_proc, in_arg,
                           out_proc, out_arg, tv);

#ifdef FORK_FORWARD
    {
        int   status;
        pid_t pid;
        
        pid = wait(&status);
        
        VERB("Process is joined PID %d errno %d status %d",
             pid, errno, status);
    }
#else
    if (pthread_join(t, &retval) != 0)
        ERROR("pthread_join failed %d", errno); 
    VERB("Thread is joined");
#endif

    rpcs->timeout = 0;
    rpcs->start = 0;
    
    if ((rpcs->_errno & ETERPCTIMEOUT) == ETERPCTIMEOUT)
    {
        rpcs->stat = RPC_TIMEDOUT;
        rpcs->dead = TRUE;
    }
    else if (rpcs->_errno == 0 && rpcs->stat != RPC_SUCCESS)
    {
        struct rpc_err err;

        CLNT_GETERR(clnt, &err);
        ERROR("SUN RPC error %d errno %d", rpcs->stat, err.re_errno);
        rpcs->_errno = TE_RC(TE_RCF_API, ETESUNRPC);
        rpcs->dead = TRUE;
    }

    if (rpcs->_errno == 0)
    {
        rpcs->duration = out->duration; 
        rpcs->_errno = out->_errno;
        rpcs->win_error = out->win_error;
        if (rpcs->op == RCF_RPC_CALL)
        {
            rpcs->tid0 = out->tid;
            rpcs->is_done_ptr = out->done;
            rpcs->op = RCF_RPC_WAIT;
        }
        else if (rpcs->op == RCF_RPC_WAIT)
        {
            rpcs->tid0 = 0;
            rpcs->is_done_ptr = 0;
            rpcs->op = RCF_RPC_CALL_WAIT;
        }
        else if (rpcs->op == RCF_RPC_IS_DONE)
        {
            rpcs->op = RCF_RPC_WAIT;
        }
    }

    pthread_mutex_unlock(&rpcs->lock);
    unlink(host);
    clnt_destroy(clnt);
}


/* See description in rcf_rpc.h */
int
rcf_rpc_server_is_op_done(rcf_rpc_server *rpcs, te_bool *done)
{
    tarpc_rpc_is_op_done_in     in;
    tarpc_rpc_is_op_done_out    out;

    if (rpcs == NULL || done == NULL)
    {
        return TE_RC(TE_RCF, EINVAL);
    }

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.common.done = rpcs->is_done_ptr;

    rpcs->op = RCF_RPC_IS_DONE;
    rcf_rpc_call(rpcs, _rpc_is_op_done,
                 &in,  (xdrproc_t)xdr_tarpc_rpc_is_op_done_in,
                 &out, (xdrproc_t)xdr_tarpc_rpc_is_op_done_out);
    
    if (rpcs->_errno != 0)
    {
        ERROR("Failed to call rpc_is_op_done() on the RPC server %s",
              rpcs->name);
        return rpcs->_errno;
    }

    *done = (out.common.done != 0);

    return 0;
}
