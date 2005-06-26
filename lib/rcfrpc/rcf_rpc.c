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

#include "te_config.h"

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
#include "conf_api.h"
#include "rcf_api.h"
#include "rcf_rpc.h"
#include "tarpc.h"

#define TE_LGR_USER     "RCF RPC"
#include "logger_api.h"
#include "logger_ten.h"

/** Initialize mutex and forwarding semaphore for RPC server */
static int
rpc_server_sem_init(rcf_rpc_server *rpcs)
{
#ifdef HAVE_PTHREAD_H
    pthread_mutexattr_t attr;
    
    pthread_mutexattr_init(&attr);
    if (pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP) != 0 ||
        pthread_mutex_init(&rpcs->lock, &attr) != 0)
    {
        ERROR("Cannot initialize mutex for RPC server");
        return TE_RC(TE_RCF_API, errno); 
    }
#endif
    
    return 0;
}


/**
 * Obtain server handle. RPC server is created/restarted, if necessary.
 *
 * @param ta            a test agent
 * @param name          name of the new server (should not start from
 *                      fork_ or thread_)
 * @param father        father name or NULL (should be NULL if clear is 
 *                      FALSE or existing is TRUE)
 * @param thread        if TRUE, the thread should be created instead 
 *                      process
 * @param existing      get only existing RPC server
 * @param clear         get newly created or restarted RPC server
 * @param p_handle      location for new RPC server handle
 *
 * @return Status code
 */
int 
rcf_rpc_server_get(const char *ta, const char *name,
                   const char *father, te_bool thread, 
                   te_bool existing, te_bool clear, 
                   rcf_rpc_server **p_handle)
{
    int   sid;
    char *val0 = NULL, *tmp;
    int   rc, rc1;
    
    rcf_rpc_server *rpcs = NULL;
    cfg_handle      handle = CFG_HANDLE_INVALID;
    char            val[RCF_RPC_NAME_LEN];
    
    /* Validate parameters */
    if (ta == NULL || p_handle == NULL || name == NULL ||
        strlen(name) >= RCF_RPC_NAME_LEN - strlen("thread_") ||
        strcmp_start("thread_", name) == 0 ||
        strcmp_start("fork_", name) == 0 ||
        ((existing || !clear) && father != NULL) || 
        (thread && father == NULL))
    {
        return TE_RC(TE_RCF_API, EINVAL);
    }
    
    /* Try to find existing RPC server */
    rc = cfg_get_instance_fmt(NULL, &tmp, "/agent:%s/rpcserver:%s",
                              ta, name);
 

    if (rc != 0 && existing)
        return TE_RC(TE_RCF_API, ENOENT);
    
    if (cfg_get_instance_fmt(NULL, &sid, 
                             "/volatile:/rpcserver_sid:%s:%s",
                             ta, name) != 0)
    {
         /* Server is created in the configurator.conf */
        if ((rc1 = rcf_ta_create_session(ta, &sid)) != 0)
        {
            ERROR("Cannot allocate RCF SID");
            return rc1;
        }
            
        if ((rc1 = cfg_add_instance_fmt(&handle, CFG_VAL(INTEGER, sid),
                                        "/volatile:/rpcserver_sid:%s:%s", 
                                        ta, name)) != 0)
        {
            ERROR("Failed to specify SID for the RPC server %s", name);
            return rc1;
        }
    }        
        
    if (father != NULL && 
        cfg_get_instance_fmt(NULL, &val0, "/agent:%s/rpcserver:%s", 
                             ta, father) != 0)
    {
        ERROR("Cannot find father %s to create server %s", father, name);
        return TE_RC(TE_RCF_API, ENOENT);
    }
    
    if (father == NULL)
        *val = 0;
    else if (thread)
        sprintf(val, "thread_%s", father);
    else
        sprintf(val, "fork_%s", father);

    if ((rpcs = (rcf_rpc_server *)
                    calloc(1, sizeof(rcf_rpc_server))) == NULL)
    {
        return TE_RC(TE_RCF_API, ENOMEM);
    }
    
    if ((rc1 = rpc_server_sem_init(rpcs)) != 0)
    {
        free(rpcs);
        return rc1;
    }

#define RETERR(rc, msg...) \
    do {                                \
        free(rpcs);                     \
        free(val0);                     \
        ERROR(msg);                     \
        return TE_RC(TE_RCF_API, rc);   \
    } while (0)
        
    if (rc == 0 && !clear)
    {
#ifdef RCF_RPC_CHECK_USABILITY    
        /* Check that it is not dead */
        tarpc_getpid_in  in;
        tarpc_getpid_out out;
        char             lib = 0;
        
        memset(&in, 0, sizeof(in));
        memset(&out, 0, sizeof(out));

        in.common.op = RCF_RPC_CALL_WAIT;
        in.common.lib = &lib;
 
        if ((rc = rcf_ta_call_rpc(ta, sid, name, 1000, "getpid", 
                                  &in, &out)) != 0)
        {
            if (existing)
            {
                if (TE_RC_GET_ERROR(rc) != EBUSY)
                    RETERR(rc, "RPC server %s is dead and cannot be used",
                           name);
            }
            else
            {
                clear = TRUE;
                WARN("RPC server %s is not usable and "
                     "will be restarted", name);
            }
        }
#endif        
    }
    else if (rc == 0 && clear)
    {
        /* Restart it */
        if ((rc = cfg_del_instance_fmt(FALSE, "/agent:%s/rpcserver:%s", 
                                       ta, name)) != 0 ||
            (rc = cfg_add_instance_fmt(&handle, CVT_STRING, val0, 
                                       "/agent:%s/rpcserver:%s", 
                                       ta, name)) != 0)
        {
            RETERR(rc, "Failed to restart RPC server %s", name);
        }
    }
    else 
    {
        if ((rc = cfg_add_instance_fmt(&handle, CVT_STRING, val, 
                                       "/agent:%s/rpcserver:%s", 
                                       ta, name)) != 0)
        {
            RETERR(rc, "Cannot add RPC server instance");
        }
    }
        
#undef RETERR    

    /* Create RPC server structure and fill it */
    strcpy(rpcs->ta, ta);
    strcpy(rpcs->name, name);
    rpcs->iut_err_jump = TRUE;
    rpcs->op = RCF_RPC_CALL_WAIT;
    rpcs->def_timeout = RCF_RPC_DEFAULT_TIMEOUT;
    rpcs->sid = sid;
    *p_handle = rpcs;
    
    free(val0);

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
    tarpc_execve_in  in;
    tarpc_execve_out out;
    
    int  rc;
    char lib = 0;

    if (rpcs == NULL)
        return TE_RC(TE_RCF, EINVAL);

    if (pthread_mutex_lock(&rpcs->lock) != 0)
        ERROR("pthread_mutex_lock() failed");
        
    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));
    in.name = rpcs->name;
    in.common.lib = &lib;

    rpcs->op = RCF_RPC_CALL_WAIT;
    rc = rcf_ta_call_rpc(rpcs->ta, rpcs->sid, rpcs->name, 0xFFFFFFFF, 
                         "execve", &in, &out);
                         
    if (pthread_mutex_unlock(&rpcs->lock) != 0)
        ERROR("pthread_mutex_unlock() failed");

    return rc;
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
    
    if (rpcs == NULL)
        return TE_RC(TE_RCF, EINVAL);
    
    VERB("Destroy RPC server %s", rpcs->name);
    
    if (pthread_mutex_lock(&rpcs->lock) != 0)
        ERROR("pthread_mutex_lock() failed");

    if ((rc = cfg_del_instance_fmt(FALSE, "/agent:%s/rpcserver:%s",
                                   rpcs->ta, rpcs->name)) != 0 &&
        TE_RC_GET_ERROR(rc) != ENOENT)
    {
        ERROR("Failed to delete RPC server %s: errno 0x%X", rpcs->name, rc);
        if (pthread_mutex_unlock(&rpcs->lock) != 0)
            ERROR("pthread_mutex_unlock() failed");
        
        return rc;
    }

    free(rpcs);
    
    VERB("RPC server is destroyed successfully");

    return 0;
}

/**
 * Call SUN RPC on the TA via RCF. The function is also used for
 * checking of status of non-blocking RPC call and waiting for
 * the finish of the non-blocking RPC call.
 *
 * @param rpcs          RPC server
 * @param proc          RPC to be called
 * @param in_arg        Input argument
 * @param out_arg       Output argument
 *
 * @attention The status code is returned in rpcs _errno.
 *            If rpcs is NULL the function does nothing.
 */
void
rcf_rpc_call(rcf_rpc_server *rpcs, const char *proc, 
             void *in_arg, void *out_arg)
{
    tarpc_in_arg  *in = (tarpc_in_arg *)in_arg;
    tarpc_out_arg *out = (tarpc_out_arg *)out_arg;
    
    te_bool op_is_done;

    if (rpcs == NULL)
    {
        ERROR("Invalid RPC server handle is passed to rcf_rpc_call()");
        return;
    }
        
    if (in_arg == NULL || out_arg == NULL || proc == NULL)
    {
        rpcs->_errno = TE_RC(TE_RCF_API, EINVAL);
        return;
    }
    
    op_is_done = strcmp(proc, "rpc_is_op_done") == 0;

    VERB("Calling RPC %s", proc);
        
    pthread_mutex_lock(&rpcs->lock);
    rpcs->_errno = 0;
    rpcs->err_log = FALSE;
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
    else 
    {
        if (rpcs->tid0 == 0)
        {
            ERROR("Try to wait not called RPC");
            rpcs->_errno = TE_RC(TE_RCF_API, EALREADY);
            pthread_mutex_unlock(&rpcs->lock);
            return;
        }
        else if (strcmp(rpcs->proc, proc) != 0 && !op_is_done)
        {
            ERROR("Try to wait RPC %s instead called RPC %s", 
                  proc, rpcs->proc);
            rpcs->_errno = TE_RC(TE_RCF_API, EPERM);
            pthread_mutex_unlock(&rpcs->lock);
            return;
        }
    }
    
    in->start = rpcs->start;
    in->op = rpcs->op;
    in->tid = rpcs->tid0;
    in->done = rpcs->is_done_ptr;
    in->lib = rpcs->lib;

    if (!op_is_done)
        strcpy(rpcs->proc, proc); 

    rpcs->_errno = rcf_ta_call_rpc(rpcs->ta, rpcs->sid, rpcs->name, 
                                   rpcs->timeout, proc, in, out);

    rpcs->timeout = 0;
    rpcs->start = 0;
    *(rpcs->lib) = '\0';
    if (TE_RC_GET_ERROR(rpcs->_errno) == ETERPCTIMEOUT ||
        TE_RC_GET_ERROR(rpcs->_errno) == ETIMEDOUT ||
        TE_RC_GET_ERROR(rpcs->_errno) == ETERPCDEAD)
    {
        rpcs->timed_out = TRUE;
    }
    
    if (rpcs->_errno == 0)
    {
        rpcs->duration = out->duration; 
        rpcs->_errno = out->_errno;
        rpcs->win_error = out->win_error;
        rpcs->timed_out = FALSE;
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
}

/* See description in rcf_rpc.h */
int
rcf_rpc_server_is_op_done(rcf_rpc_server *rpcs, te_bool *done)
{
    tarpc_rpc_is_op_done_in  in;
    tarpc_rpc_is_op_done_out out;

    if (rpcs == NULL || done == NULL)
        return TE_RC(TE_RCF, EINVAL);

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.common.done = rpcs->is_done_ptr;

    rpcs->op = RCF_RPC_IS_DONE;
    rcf_rpc_call(rpcs, "rpc_is_op_done", &in, &out);
    
    if (rpcs->_errno != 0)
    {
        ERROR("Failed to call rpc_is_op_done() on the RPC server %s",
              rpcs->name);
        return rpcs->_errno;
    }

    *done = (out.common.done != 0);

    return 0;
}
