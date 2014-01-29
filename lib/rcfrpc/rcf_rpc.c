/** @file
 * @brief SUN RPC control interface
 *
 * RCF RPC implementation.
 *
 *
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
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
 * @author Elena A. Vengerova <Elena.Vengerova@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER     "RCF RPC"

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
#include "logger_api.h"
#include "conf_api.h"
#include "rcf_api.h"
#include "rcf_rpc.h"
#include "rcf_internal.h"
#include "rpc_xdr.h"
#include "tarpc.h"
#include "te_rpc_errno.h"


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
        return TE_OS_RC(TE_RCF_API, errno); 
    }
#endif
    
    return 0;
}


/* See description in rcf_rpc.h */
te_errno 
rcf_rpc_server_get(const char *ta, const char *name,
                   const char *father, int flags,
                   rcf_rpc_server **p_handle)
{
    int   sid;
    char *val0 = NULL;
    int   rc, rc1;
    
    rcf_rpc_server *rpcs = NULL;
    cfg_handle      handle = CFG_HANDLE_INVALID;
    char            val[RCF_RPC_NAME_LEN];
    char            str_register[RCF_RPC_NAME_LEN] = "register_";
    
    /* Validate parameters */
    if (ta == NULL || name == NULL ||
        strlen(name) >= RCF_RPC_NAME_LEN - strlen("forkexec_register_") ||
        strcmp_start("fork_", name) == 0 ||
        strcmp_start("forkexec_", name) == 0 ||
        strcmp_start("register_", name) == 0 ||
        ((flags & (RCF_RPC_SERVER_GET_EXISTING |
           RCF_RPC_SERVER_GET_REUSE)) && father != NULL) || 
        ((flags & (RCF_RPC_SERVER_GET_THREAD |
                   RCF_RPC_SERVER_GET_REGISTER)) && father == NULL) ||
        ((flags & RCF_RPC_SERVER_GET_REGISTER) &&
         (flags & RCF_RPC_SERVER_GET_THREAD)))
    {
        return TE_RC(TE_RCF_API, TE_EINVAL);
    }

    if (strcmp_start("local_", name) == 0)
    {
        RING("RPC servers as local threads of Test Agent "
             "are not supported any more: they are dangerous");
    }

    if (!(flags & RCF_RPC_SERVER_GET_REGISTER))
        str_register[0] = '\0';
    /* Try to find existing RPC server */
    rc = cfg_get_instance_fmt(NULL, &val0, "/agent:%s/rpcserver:%s",
                              ta, name);

    if (rc != 0 && (flags & RCF_RPC_SERVER_GET_EXISTING))
        return TE_RC(TE_RCF_API, TE_ENOENT);
    
    if (cfg_get_instance_fmt(NULL, &sid, 
                             "/rpcserver_sid:%s:%s",
                             ta, name) != 0)
    {
         /* Server is created in the cs.conf */
        if ((rc1 = rcf_ta_create_session(ta, &sid)) != 0)
        {
            ERROR("Cannot allocate RCF SID");
            return rc1;
        }
            
        if ((rc1 = cfg_add_instance_fmt(&handle, CFG_VAL(INTEGER, sid),
                                        "/rpcserver_sid:%s:%s", 
                                        ta, name)) != 0)
        {
            ERROR("Failed to specify SID for the RPC server %s", name);
            return rc1;
        }
    }        
        
    /* FIXME: thread support to be done */
    if (father != NULL && 
        cfg_get_instance_fmt(NULL, NULL, "/agent:%s/rpcserver:%s", 
                             ta, father) != 0)
    {
        ERROR("Cannot find father %s to create server %s",
              father, name);
        return TE_RC(TE_RCF_API, TE_ENOENT);
    }

    if (father == NULL)
        *val = '\0';
    else if ((flags & RCF_RPC_SERVER_GET_THREAD))
        sprintf(val, "thread_%s", father);
    else if ((flags & RCF_RPC_SERVER_GET_EXEC))
        sprintf(val, "forkexec_%s%s", str_register, father);
    else
        sprintf(val, "fork_%s%s", str_register, father);

    if ((rpcs = (rcf_rpc_server *)
                    calloc(1, sizeof(rcf_rpc_server))) == NULL)
    {
        return TE_RC(TE_RCF_API, TE_ENOMEM);
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
        
    if (rc == 0 && (flags & RCF_RPC_SERVER_GET_REUSE))
    {
#ifdef RCF_RPC_CHECK_USABILITY    
        /* Check that it is not dead */
        tarpc_getpid_in  in;
        tarpc_getpid_out out;
        
        memset(&in, 0, sizeof(in));
        memset(&out, 0, sizeof(out));

        in.common.op = RCF_RPC_CALL_WAIT;
        in.common.use_libc = FASLE;
 
        if ((rc = rcf_ta_call_rpc(ta, sid, name, 1000, "getpid", 
                                  &in, &out)) != 0)
        {
            if ((flags & RCF_RPC_SERVER_GET_EXISTING))
            {
                if (TE_RC_GET_ERROR(rc) != TE_EBUSY)
                    RETERR(rc, "RPC server %s is dead and cannot be used",
                           name);
            }
            else
            {
                flags &= ~RCF_RPC_SERVER_GET_REUSE;
                WARN("RPC server %s is not usable and "
                     "will be restarted", name);
            }
        }
#endif
    }

    if (rc == 0 && !(flags & RCF_RPC_SERVER_GET_REUSE))
    {
        /* Restart it */
        if ((rc = cfg_del_instance_fmt(FALSE, "/agent:%s/rpcserver:%s",
                                       ta, name)) != 0)
        {
            if (TE_RC_GET_ERROR(rc) != TE_ENOENT)
                RETERR(rc, "Failed to delete RPC server %s", name);
            else
            {
                if ((rc = cfg_synchronize_fmt(FALSE,
                                              "/agent:%s/rpcserver:%s",
                                              ta, name)) != 0)
                    RETERR(rc,
                           "Failed to synchronize '/agent:%s/rpcserver:%s'",
                           ta, name);
                CFG_WAIT_CHANGES;
                if ((rc = cfg_del_instance_fmt(FALSE,
                                               "/agent:%s/rpcserver:%s",
                                               ta, name)) != 0)
                {
                    if (TE_RC_GET_ERROR(rc) != TE_ENOENT)
                        RETERR(rc, "Failed to delete RPC server %s", name);
                    else
                        ERROR("Failed to delete rpcserver %s", name);
                }
            }
        }
        if ((rc = cfg_add_instance_fmt(&handle, CVT_STRING, val0,
                                        "/agent:%s/rpcserver:%s",
                                        ta, name)) != 0)
        {
            RETERR(rc, "Failed to restart RPC server %s", name);
        }
    }
    else if (rc != 0)
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
    rpcs->timeout = RCF_RPC_UNSPEC_TIMEOUT;
    rpcs->sid = sid;
    if (p_handle != NULL)
        *p_handle = rpcs;
    else
        free(rpcs);
    
    free(val0);

    return 0;
}

/* See description in rcf_rpc.h */
te_errno
rcf_rpc_servers_restart_all(void)
{
    const char * const  pattern = "/agent:*/rpcserver:*";

    te_errno        rc;
    unsigned int    num;
    cfg_handle     *handles = NULL;
    unsigned int    i;
    cfg_oid        *oid = NULL;

    rc = cfg_find_pattern(pattern, &num, &handles);
    if (TE_RC_GET_ERROR(rc) == TE_ENOENT)
    {
        return 0;
    }
    else if (rc != 0)
    {
        ERROR("Failed to find by pattern '%s': %r", pattern, rc);
        return rc;
    }

    for (i = 0; i < num; ++i)
    {
        rc = cfg_get_oid(handles[i], &oid);
        if (rc != 0)
        {
            ERROR("%s(): cfg_get_oid() failed for #%u: %r",
                  __FUNCTION__, i, rc);
            break;
        }

        rc = rcf_rpc_server_get(CFG_OID_GET_INST_NAME(oid, 1),
                                CFG_OID_GET_INST_NAME(oid, 2),
                                NULL, RCF_RPC_SERVER_GET_EXISTING, NULL);
        if (rc != 0)
        {
            ERROR("%s(): rcf_rpc_server_get() failed for #%u: %r",
                  __FUNCTION__, i, rc);
            break;
        }

        cfg_free_oid(oid); oid = NULL;
    }
    cfg_free_oid(oid);
    free(handles);

    return rc;
}

/**
 * Mark threads as finished in result of execve() call.
 * In configuration tree, clear the value of the node of
 * the RPC server where execve() was called.
 *
 * @param rpcs          RPC server
 *
 * @return 0 on success or error code
 */
static te_errno
rcf_rpc_server_mark_deleted_threads(rcf_rpc_server *rpcs)
{
    unsigned int num, i;

    te_errno   rc = 0;
    char      *value = NULL;
    char      *name = NULL;
    char      *my_val = NULL;

    cfg_handle  *servers = NULL;
    cfg_handle   my_handle;

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        return -1;
    }

    if ((rc = cfg_find_fmt(&my_handle, "/agent:%s/rpcserver:%s",
                           rpcs->ta, rpcs->name)) != 0)
    {
        ERROR("%s(): Cannot find RPC server %s", __FUNCTION__,
              rpcs->name);
        return rc;
    }

    if ((rc = cfg_find_pattern_fmt(&num, &servers, "/agent:%s/rpcserver:*",
                                   rpcs->ta)) != 0)
    {
        ERROR("%s(): Cannot get the list of all RPC servers on "
              "the test agent %s", __FUNCTION__, rpcs->ta);
        return rc;
    }

    if ((rc = cfg_get_instance(my_handle, NULL, &my_val)) != 0)
    {
        ERROR("%s(): Cannot get the value of the RPC server %s node "
              "in configuration tree", __FUNCTION__, rpcs->name);
        free(servers);
        return rc;
    }

    for (i = 0; i < num; i++)
    {
        if ((rc = cfg_get_instance(servers[i], NULL, &value)) != 0)
            ERROR("%s(): Cannot get value of RPC server node by its"
                  " handle %d", __FUNCTION__, servers[i]);

        if (rc == 0 && (rc = cfg_get_inst_name(servers[i], &name)) != 0)
            ERROR("%s(): Cannot get name of RPC server node by its"
                  " handle %d", __FUNCTION__, servers[i]);

        if (rc == 0 &&
            ((strcmp_start("thread_", value) == 0 &&
             (strncmp(value + strlen("thread_"), rpcs->name,
                      RCF_RPC_NAME_LEN) == 0 ||
              strncmp(value, my_val, RCF_RPC_NAME_LEN) == 0)) ||
            (strcmp_start("thread_", value) != 0 &&
             strcmp_start("thread_", my_val) == 0 &&
             strncmp(my_val + strlen("thread_"), name,
                     RCF_RPC_NAME_LEN) == 0)))
        {
            if (servers[i] != my_handle)
            {
                rc = cfg_set_instance_fmt(
                                     CFG_VAL(INTEGER, 1), 
                                     "/agent:%s/rpcserver:%s/finished:",
                                     rpcs->ta, name);
                if (rc != 0)
                {
                    ERROR("%s(): Cannot mark as finished "
                          "RPC server %s", __FUNCTION__, name);
                }
            }
            else
            {
                rc = cfg_set_instance(servers[i], CVT_STRING, "");

                if (rc != 0)
                    ERROR("%s(): Cannot set new value for "
                          "RPC server %s", __FUNCTION__, name);
            }
        }

        free(value);
        free(name);
        value = NULL;
        name = NULL;

        if (rc != 0)
        {
            free(my_val);
            free(servers);
            return rc;
        }
    }

    free(my_val);
    free(servers);
    return 0;
}

/* See description in rcf_rpc.h */
te_errno 
rcf_rpc_server_exec(rcf_rpc_server *rpcs)
{
    tarpc_execve_in  in;
    tarpc_execve_out out;
    
    te_errno  rc;

    if (rpcs == NULL)
        return TE_RC(TE_RCF, TE_EINVAL);

#ifdef HAVE_PTHREAD_H
    if (pthread_mutex_lock(&rpcs->lock) != 0)
        ERROR("%s(): pthread_mutex_lock() failed", __FUNCTION__);
#endif


    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));
    in.name = rpcs->name;
    in.common.use_libc = FALSE;

    rpcs->op = RCF_RPC_CALL_WAIT;
    rc = rcf_ta_call_rpc(rpcs->ta, rpcs->sid, rpcs->name, 0xFFFFFFFF, 
                         "execve", &in, &out);

#ifdef HAVE_PTHREAD_H
    if (pthread_mutex_unlock(&rpcs->lock) != 0)
        ERROR("pthread_mutex_unlock() failed");
#endif

    if (rc == 0)
    {
        RING("RPC (%s,%s): execve() -> (%s)",
             rpcs->ta, rpcs->name, errno_rpc2str(RPC_ERRNO(rpcs)));
        rc = rcf_rpc_server_mark_deleted_threads(rpcs);
    }
    else
        ERROR("RPC (%s,%s): execve() -> (%s), rc=%r",
              rpcs->ta, rpcs->name, errno_rpc2str(RPC_ERRNO(rpcs)), rc);

    return rc;
}

/* See description in rcf_rpc.h */
te_errno 
rcf_rpc_server_destroy(rcf_rpc_server *rpcs)
{
    int rc;
    
    if (rpcs == NULL)
        return 0;
    
    VERB("Destroy RPC server %s", rpcs->name);
    
#ifdef HAVE_PTHREAD_H
    if (pthread_mutex_lock(&rpcs->lock) != 0)
        ERROR("%s(): pthread_mutex_lock() failed", __FUNCTION__);
#endif

    if ((rc = cfg_del_instance_fmt(FALSE, "/agent:%s/rpcserver:%s",
                                   rpcs->ta, rpcs->name)) != 0 &&
        TE_RC_GET_ERROR(rc) != TE_ENOENT)
    {
        ERROR("Failed to delete RPC server %s: error=%r", rpcs->name, rc);

#ifdef HAVE_PTHREAD_H
        if (pthread_mutex_unlock(&rpcs->lock) != 0)
            ERROR("pthread_mutex_unlock() failed");
#endif
        
        return rc;
    }

    free(rpcs->nv_lib);
    free(rpcs);
    
    VERB("RPC server is destroyed successfully");

    return 0;
}

/* See description in rcf_rpc.h */
void
rcf_rpc_call(rcf_rpc_server *rpcs, const char *proc, 
             void *in_arg, void *out_arg)
{
    tarpc_in_arg  *in = (tarpc_in_arg *)in_arg;
    tarpc_out_arg *out = (tarpc_out_arg *)out_arg;
    
    te_bool     op_is_done;
    te_bool     is_alive;

    if (rpcs == NULL)
    {
        ERROR("Invalid RPC server handle is passed to rcf_rpc_call()");
        return;
    }
        
    if (in_arg == NULL || out_arg == NULL || proc == NULL)
    {
        rpcs->_errno = TE_RC(TE_RCF_API, TE_EINVAL);
        return;
    }
    
    op_is_done = (strcmp(proc, "rpc_is_op_done") == 0);
    is_alive = (strcmp(proc, "rpc_is_alive") == 0);

    VERB("Calling RPC %s", proc);
        
#ifdef HAVE_PTHREAD_H
    pthread_mutex_lock(&rpcs->lock);
#endif    
    rpcs->_errno = 0;
    rpcs->err_log = FALSE;
    if (rpcs->timeout == RCF_RPC_UNSPEC_TIMEOUT)
        rpcs->timeout = rpcs->def_timeout;
    
    if (rpcs->op == RCF_RPC_CALL || rpcs->op == RCF_RPC_CALL_WAIT)
    {
        if (rpcs->tid0 != 0)
        {
            rpcs->_errno = TE_RC(TE_RCF_API, TE_EBUSY);
            rpcs->timed_out = TRUE;
#ifdef HAVE_PTHREAD_H
            pthread_mutex_unlock(&rpcs->lock);
#endif            
            return;
        }
    }
    else if (!is_alive)
    {
        if (rpcs->tid0 == 0)
        {
            ERROR("Try to wait not called RPC");
            rpcs->_errno = TE_RC(TE_RCF_API, TE_EALREADY);
            rpcs->timed_out = TRUE;
#ifdef HAVE_PTHREAD_H
            pthread_mutex_unlock(&rpcs->lock);
#endif            
            return;
        }
        else if (strcmp(rpcs->proc, proc) != 0 && !op_is_done)
        {
            ERROR("Try to wait RPC %s instead called RPC %s", 
                  proc, rpcs->proc);
            rpcs->_errno = TE_RC(TE_RCF_API, TE_EPERM);
            rpcs->timed_out = TRUE;
#ifdef HAVE_PTHREAD_H
            pthread_mutex_unlock(&rpcs->lock);
#endif            
            return;
        }
    }
    
    in->start = rpcs->start;
    in->op = rpcs->op;
    in->tid = rpcs->tid0;
    in->done = rpcs->is_done_ptr;
    in->use_libc = rpcs->use_libc || rpcs->use_libc_once;

    rpcs->last_op = rpcs->op;
    rpcs->last_use_libc = rpcs->use_libc_once;

    if (!op_is_done && !is_alive)
        strcpy(rpcs->proc, proc); 

    rpcs->_errno = rcf_ta_call_rpc(rpcs->ta, rpcs->sid, rpcs->name, 
                                   rpcs->timeout, proc, in, out);

    if (rpcs->op != RCF_RPC_CALL)
        rpcs->timeout = RCF_RPC_UNSPEC_TIMEOUT;
    rpcs->start = 0;
    rpcs->use_libc_once = FALSE;
    if (TE_RC_GET_ERROR(rpcs->_errno) == TE_ERPCTIMEOUT ||
        TE_RC_GET_ERROR(rpcs->_errno) == TE_ETIMEDOUT ||
        TE_RC_GET_ERROR(rpcs->_errno) == TE_ERPCDEAD)
    {
        rpcs->timed_out = TRUE;
    }
    
    if (rpcs->_errno == 0)
    {
        rpcs->duration = out->duration; 
        rpcs->_errno = out->_errno;
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

#ifdef HAVE_PTHREAD_H
    pthread_mutex_unlock(&rpcs->lock);
#endif    
}

/* See description in rcf_rpc.h */
te_errno
rcf_rpc_server_is_op_done(rcf_rpc_server *rpcs, te_bool *done)
{
    tarpc_rpc_is_op_done_in  in;
    tarpc_rpc_is_op_done_out out;

    if (rpcs == NULL || done == NULL)
        return TE_RC(TE_RCF, TE_EINVAL);

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

/* See description in rcf_rpc.h */
te_bool
rcf_rpc_server_is_alive(rcf_rpc_server *rpcs)
{
    tarpc_rpc_is_alive_in   in;
    tarpc_rpc_is_alive_out  out;
    rcf_rpc_op              old_op;

    if (rpcs == NULL)
        return FALSE;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    old_op = rpcs->op;
    rpcs->op = RCF_RPC_IS_DONE;
    rcf_rpc_call(rpcs, "rpc_is_alive", &in, &out);
    rpcs->op = old_op;
    
    if (rpcs->_errno != 0)
    {
        ERROR("Failed to call rpc_is_alive() on the RPC server %s",
              rpcs->name);
        return FALSE;
    }
    else
        RING("RPC server %s is alive", rpcs->name);

    return TRUE;
}

/* See description in rcf_rpc.h */
te_errno
rcf_rpc_setlibname(rcf_rpc_server *rpcs, const char *libname)
{
    tarpc_setlibname_in  in;
    tarpc_setlibname_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        return -1;
    }

    in.libname.libname_len = (libname == NULL) ? 0 : (strlen(libname) + 1);
    in.libname.libname_val = (char *)libname;

    rpcs->op = RCF_RPC_CALL_WAIT;
    rcf_rpc_call(rpcs, "setlibname", &in, &out);

    if (!RPC_IS_CALL_OK(rpcs))
        out.retval = -1;

    LOG_MSG(out.retval != 0 ? TE_LL_ERROR : TE_LL_RING,
            "RPC (%s,%s) setlibname(%s) -> %d (%s)",
            rpcs->ta, rpcs->name, libname ? : "(NULL)",
            out.retval, errno_rpc2str(RPC_ERRNO(rpcs)));

    if (out.retval == 0)
    {
        free(rpcs->nv_lib);
        rpcs->nv_lib = (libname != NULL) ? strdup(libname) : NULL;
        assert((rpcs->nv_lib == NULL) == (libname == NULL));
    }

    return out.retval;
}

extern int rcf_send_recv_msg(rcf_msg *send_buf, size_t send_size,
                             rcf_msg *recv_buf, size_t *recv_size, 
                             rcf_msg **p_answer);

/**
 * Call SUN RPC on the TA.
 *
 * @param ta_name       Test Agent name                 
 * @param session       TA session or 0
 * @param rpcserver     Name of the RPC server
 * @param timeout       RPC timeout in milliseconds or 0 (unlimited)
 * @param rpc_name      Name of the RPC (e.g. "bind")
 * @param in            Input parameter C structure
 * @param out           Output parameter C structure
 *
 * @return Status code
 */
te_errno 
rcf_ta_call_rpc(const char *ta_name, int session, 
                const char *rpcserver, int timeout,
                const char *rpc_name, void *in, void *out)
{
/** Length of RPC data inside the message */     
#define INSIDE_LEN \
    (sizeof(((rcf_msg *)0)->file) + sizeof(((rcf_msg *)0)->value))
    
/** Length of message part before RPC data */
#define PREFIX_LEN  \
    (sizeof(rcf_msg) - INSIDE_LEN)

    char     msg_buf[PREFIX_LEN + RCF_RPC_BUF_LEN];
    rcf_msg *msg = (rcf_msg *)msg_buf;
    rcf_msg *ans = NULL;
    int      rc;
    size_t   anslen = sizeof(msg_buf);
    size_t   len;
    
    if (ta_name == NULL || strlen(ta_name) >= RCF_MAX_NAME || 
        rpcserver == NULL || rpc_name == NULL || 
        in == NULL || out == NULL)
    {
        return TE_RC(TE_RCF_API, TE_EINVAL);
    }
    
    if (strlen(rpc_name) >= RCF_RPC_MAX_NAME)
    {
        ERROR("Too long RPC name: %s - change RCF_RPC_MAX_NAME constant", 
              rpc_name);
        return TE_RC(TE_RCF_API, TE_EINVAL);
    }
    
    len = RCF_RPC_BUF_LEN;
    if ((rc = rpc_xdr_encode_call(rpc_name, msg->file, &len, in)) != 0)
    {
        if (TE_RC_GET_ERROR(rc) == TE_ENOENT)
        {
            ERROR("Unknown RPC %s", rpc_name);
            return rc;
        }
        
        /* Try to allocate more memory */
        len = RCF_RPC_HUGE_BUF_LEN;
        anslen = PREFIX_LEN + RCF_RPC_HUGE_BUF_LEN;
        if ((msg = (rcf_msg *)malloc(anslen)) == NULL)
            return TE_RC(TE_RCF_API, TE_ENOMEM);

        if ((rc = rpc_xdr_encode_call(rpc_name, msg->file, &len, in)) != 0)
        {
            ERROR("Encoding of RPC %s input parameters failed: error %r",
                  rpc_name, rc);
            free(msg);
            return rc;
        }
    }
    
    memset(msg, 0, PREFIX_LEN);
    msg->opcode = RCFOP_RPC;
    msg->sid = session;
    strcpy(msg->ta, ta_name);
    strcpy(msg->id, rpcserver);
    msg->timeout = timeout;
    msg->intparm = len;
    msg->data_len = len - INSIDE_LEN;

    rc = rcf_send_recv_msg(msg, PREFIX_LEN + len, msg, &anslen, &ans);
                           
    if (ans != NULL)
    {
        if ((char *)msg != msg_buf)
            free(msg);
            
        msg = ans;
    }
    
    if (rc != 0 || (rc = msg->error) != 0)
    {
       if ((char *)msg != msg_buf)
            free(msg);
        return rc;
    }
    
    rc = rpc_xdr_decode_result(rpc_name, msg->file, msg->intparm, out);
    
    if (rc != 0)
        ERROR("Decoding of RPC %s output parameters failed: error %r",
              rpc_name, rc);
        
    if ((char *)msg != msg_buf)
        free(msg);
        
    return rc;        
    
#undef INSIDE_LEN    
#undef PREFIX_LEN    
}

/* See description in rcf_rpc.h */
te_bool
rcf_rpc_server_has_children(rcf_rpc_server *rpcs)
{
    unsigned int num, i;
    cfg_handle  *servers;
    
    if (rpcs == NULL)
        return FALSE;
    
    if (cfg_find_pattern_fmt(&num, &servers, "/agent:%s/rpcserver:*",
                             rpcs->ta) != 0)
    {
        return FALSE;
    }
    
    for (i = 0; i < num; i++)
    {
        char *name;
        
        if (cfg_get_instance(servers[i], NULL, &name) != 0)
            continue;
            
        if (strcmp_start("thread_", name) == 0 &&
            strcmp(name + strlen("thread_"), rpcs->name) == 0)
        {
            free(name);
            free(servers);
            return TRUE;
        }
        free(name);
    }              
    
    free(servers);
    return FALSE;    
}

/* See description in rcf_rpc.h */
te_errno 
rcf_rpc_server_create_process(rcf_rpc_server *rpcs, 
                              const char *name, int flags,
                              rcf_rpc_server **p_new)
{
    tarpc_create_process_in  in;
    tarpc_create_process_out out;
    te_errno                 rc;

    if (rpcs == NULL)
        return TE_RC(TE_RCF_API, TE_EINVAL);

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));
    
    in.common.op = RCF_RPC_CALL_WAIT;
    in.common.use_libc = FALSE;
    in.name.name_len = strlen(name) + 1;
    in.name.name_val = strdup(name);
    in.flags = flags;
    
    if ((rc = rcf_ta_call_rpc(rpcs->ta, rpcs->sid, rpcs->name,
                              1000, "create_process", &in, &out)) != 0)
    { 
        free(in.name.name_val);
        return rc;
    }
    
    free(in.name.name_val);
        
    if (out.pid < 0)
    {
        ERROR("RPC create_process() failed on the server %s with errno %r", 
              rpcs->name, out.common._errno);
        return (out.common._errno != 0) ?
                   out.common._errno : TE_RC(TE_RCF_API, TE_ECORRUPTED);
    }
    
    /* 
     * Flag RCF_RPC_SERVER_EXEC is passed just for proper value setting 
     * of RPC server node in configuration tree.
     */
    rc = rcf_rpc_server_get(rpcs->ta, name, rpcs->name,
                            RCF_RPC_SERVER_GET_REGISTER |
                            (flags & RCF_RPC_SERVER_GET_EXEC), p_new);
    
    if (rc != 0)
    {
        ERROR("Failed to register created RPC server %s on TA: %r", rc, 
              name);
              
        if (rcf_ta_kill_task(rpcs->ta, 0, out.pid) != 0)
            ERROR("Failed to kill created RPC server");
    }
    
    return rc;
}                           

/* See description in rcf_rpc.h */
te_errno
rcf_rpc_server_vfork(rcf_rpc_server *rpcs, 
                     const char *name,
                     uint32_t time_to_wait,
                     pid_t *pid, uint32_t *elapsed_time)
{
    tarpc_vfork_in      in;
    tarpc_vfork_out     out;
    te_errno            rc;

    if (rpcs == NULL)
        return TE_RC(TE_RCF_API, TE_EINVAL);

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs->timeout == RCF_RPC_UNSPEC_TIMEOUT)
        rpcs->timeout = rpcs->def_timeout;

    in.common.op = RCF_RPC_CALL_WAIT;
    in.common.use_libc = rpcs->use_libc || rpcs->use_libc_once;
    in.name.name_len = strlen(name) + 1;
    in.name.name_val = strdup(name);
    in.time_to_wait = time_to_wait;
    
    rc = rcf_ta_call_rpc(rpcs->ta, rpcs->sid, rpcs->name,
                         rpcs->timeout, "vfork", &in, &out);
    
    free(in.name.name_val);

    if (pid != NULL)
        *pid = out.pid;
    if (elapsed_time != NULL)
        *elapsed_time = out.elapsed_time;

    RING("RPC (%s, %s): vfork(%s, %u) -> %d (elapsed=%u)",
         rpcs->ta, rpcs->name, name, time_to_wait, out.pid,
         out.elapsed_time);
    return rc;
} 

/**
 * Entry function for a thread with rcf_rpc_server_vfork() called.
 *
 * @param data      Input/output parameters
 *
 * @return data
 */
static void *
vfork_thread_func(void *data)
{
    vfork_thread_data *p = data;

    p->err = rcf_rpc_server_vfork(p->rpcs, p->name, p->time_to_wait,
                                  &(p->pid), &(p->elapsed_time));
    return data;
}

/* See description in rcf_rpc.h */
te_errno
rcf_rpc_server_vfork_in_thread(vfork_thread_data *data,
                               pthread_t *thread_id,
                               rcf_rpc_server **p_new)
{
    int rc;

    if (data == NULL || thread_id == NULL || p_new == NULL)
        return TE_RC(TE_RCF_API, TE_EINVAL);

    rc = pthread_create(thread_id, NULL, &vfork_thread_func, data);
    if (rc != 0)
        return rc;

    usleep(500000);
    rc = rcf_rpc_server_get(data->rpcs->ta, data->name, data->rpcs->name,
                            RCF_RPC_SERVER_GET_REGISTER, p_new);
    return rc;
}
