/** @file 
 * @brief RCF Portable Command Handler
 *
 * RCF RPC server entry point. The file is included to another one
 * to create TA-builtin and standalone RPC servers.
 *
 * Copyright (C) 2006 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
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
 * @author Elena A. Vengerova <Elena.Vengerova@oktetlabs.ru>
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
 */

#include "rpc_server.h"
#include "ta_common.h"
#include "agentlib.h"
#include "rpc_transport.h"

#include "te_errno.h"
#include "te_sleep.h"

void
tarpc_init_checked_arg(checked_arg_list *list, uint8_t *real_arg,
                       size_t len, size_t len_visible, const char *name)
{
    checked_arg *arg;

    if (real_arg == NULL || len <= len_visible)
        return;

    if ((arg = calloc(1, sizeof(*arg))) == NULL)
    {
        ERROR("Out of memory");
        return;
    }

    if ((arg->pristine = malloc(len - len_visible)) == NULL)
    {
        ERROR("Out of memory");
        free(arg);
        return;
    }
    memcpy(arg->pristine, real_arg + len_visible, len - len_visible);
    arg->real_arg = real_arg;
    arg->len = len;
    arg->len_visible = len_visible;
    arg->name = name;
    STAILQ_INSERT_TAIL(list, arg, next);
}

te_errno
tarpc_check_args(checked_arg_list *list)
{
    te_errno rc = 0;
    checked_arg *cur;

    for (cur = STAILQ_FIRST(list); cur != NULL; cur = STAILQ_FIRST(list))
    {
        if (memcmp(cur->real_arg + cur->len_visible, cur->pristine,
                   cur->len - cur->len_visible) != 0)
        {
            ERROR("Argument %s:\nVisible length is "
                  "%"TE_PRINTF_SIZE_T"u.\nPristine is:%Tm"
                  "Current is:%Tm + %Tm",
                  cur->name,
                  (unsigned long)cur->len_visible,
                  cur->pristine, cur->len - cur->len_visible,
                  cur->real_arg, cur->len_visible,
                  cur->real_arg + cur->len_visible,
                  cur->len - cur->len_visible);
            rc = TE_RC(TE_TA_UNIX, TE_ECORRUPTED);
        }
        free(cur->pristine);
        STAILQ_REMOVE_HEAD(list, next);
        free(cur);
    }

    return rc;
}

static void
wait_start(uint64_t msec_start)
{
    struct timeval t;

    uint64_t msec_now;

    gettimeofday(&t, NULL);
    msec_now = ((uint64_t)(t.tv_sec)) * 1000ULL +
    (t.tv_usec) / 1000;

    if (msec_start > msec_now)
    {
        RING("Sleep %u microseconds before call",
             (unsigned)TE_MS2US(msec_start - msec_now));
        usleep(TE_MS2US(msec_start - msec_now));
    }
    else if (msec_start != 0)
        WARN("Start time is gone");
}

void tarpc_before_call(struct rpc_call_data *call, const char *id)
{
    tarpc_in_arg *in_common = (tarpc_in_arg *)((uint8_t *)call->in +
                                               call->info->in_common_offset);

    call->saved_errno = errno;
    wait_start(in_common->start);
    VERB("Calling: %s" , id);
    gettimeofday(&call->call_start, NULL);
}

void tarpc_after_call(struct rpc_call_data *call)
{
    tarpc_out_arg *out_common =
        (tarpc_out_arg *)((uint8_t *)call->out +
                          call->info->out_common_offset);
    struct timeval finish;
    int rc;

    out_common->errno_changed = (call->saved_errno != errno);
    out_common->_errno = RPC_ERRNO;
    gettimeofday(&finish, NULL);
    out_common->duration =
        TE_SEC2US(finish.tv_sec - call->call_start.tv_sec) +
        finish.tv_usec - call->call_start.tv_usec;
    rc = tarpc_check_args(&call->checked_args);
    if (out_common->_errno == 0 && rc != 0)
        out_common->_errno = rc;
}

void tarpc_call_unsupported(const char *name, void *out,
                            size_t outsize, size_t common_offset)
{
    tarpc_out_arg *out_common =
       (tarpc_out_arg *)((uint8_t *)out + common_offset);
    memset((uint8_t *)out, 0, outsize);
    out_common->_errno = RPC_ERPCNOTSUPP;
    RING("Unsupported RPC '%s' has been called", name);
}

static void *
tarpc_generic_service_thread(void *arg)
{
    rpc_call_data *call = arg;

    logfork_register_user(call->info->funcname);

    VERB("Entry thread %s", call->info->funcname);

    if (pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL) != 0)
        ERROR("Failed to set thread cancel type for the non-blocking call "
              "thread: %r", RPC_ERRNO);

    sigprocmask(SIG_SETMASK, &(call->mask), NULL);

    call->info->wrapper(call);

    call->done = TRUE;

    aux_threads_del();
    return arg;
}

void
tarpc_generic_service(rpc_call_data *call)
{
    int rc;
    tarpc_in_arg *in_common = (tarpc_in_arg *)((uint8_t *)call->in +
                                               call->info->in_common_offset);
    tarpc_out_arg *out_common = (tarpc_out_arg *)((uint8_t *)call->out +
                                                  call->info->
                                                  out_common_offset);

    memset(call->out, 0, call->info->out_size);

    if (call->func == NULL)
    {
        rc = tarpc_find_func(in_common->use_libc, call->info->funcname,
                             &call->func);
        if (rc != 0)
        {
            out_common->_errno = rc;
            return;
        }
    }

    if (call->info->copy(call->in, call->out))
        return;

    switch (in_common->op)
    {
        case RCF_RPC_CALL_WAIT:
        {
            VERB("%s(): CALL-WAIT", call->info->funcname);

            call->info->wrapper(call);

            break;
        }
        case RCF_RPC_CALL:
        {
            rpc_call_data *copy_call;
            pthread_t tid;

            VERB("%s(): CALL", call->info->funcname);

            if ((copy_call = calloc(1, sizeof(*copy_call) +
                                    call->info->in_size +
                                    call->info->out_size)) == NULL)
            {
                out_common->_errno = TE_RC(TE_TA_UNIX, TE_ENOMEM);
                break;
            }

            *copy_call = *call;
            sigprocmask(SIG_SETMASK, NULL, &(copy_call->mask));
            copy_call->in = (uint8_t *)copy_call + sizeof(*copy_call);
            copy_call->out = (uint8_t *)copy_call->in +
                             copy_call->info->in_size;
            memcpy(copy_call->in, call->in, call->info->in_size);
            memcpy(copy_call->out, call->out, call->info->out_size);

            if (pthread_create(&tid, NULL, tarpc_generic_service_thread,
                               copy_call) != 0)
            {
                free(copy_call);
                out_common->_errno = TE_OS_RC(TE_TA_UNIX, errno);
                break;
            }
            aux_threads_add(tid);

            /*
             * Preset 'in' and 'out' with zeros to avoid any
             * resource deallocations by the caller.
             * 'out' is preset with zeros above, but may be
             * modified in '_copy_args'.
             */
            memset(call->in,  0, call->info->in_size);
            memset(call->out, 0, call->info->out_size);

            out_common->tid = (tarpc_pthread_t)tid;
            out_common->done = rcf_pch_mem_alloc(&copy_call->done);

            break;
        }
        case RCF_RPC_WAIT:
        {
            rpc_call_data *copy_call;
            pthread_t     tid =  (pthread_t)in_common->tid;
            enum xdr_op   op;

            VERB("%s(): WAIT", call->info->funcname);

            /*
             * If WAIT is called, all resources are deallocated
             * in any case.
             */
            rcf_pch_mem_free(in_common->done);

            if (tid == (pthread_t)(long)NULL)
            {
                ERROR("No thread with ID %llu to wait",
                      (unsigned long long int)in_common->tid);
                out_common->_errno = TE_RC(TE_TA_UNIX, TE_ENOENT);
                break;
            }
            if (pthread_join(tid, (void **)&copy_call) != 0)
            {
                ERROR("pthread_join() failed");
                out_common->_errno = TE_OS_RC(TE_TA_UNIX, errno);
                break;
            }
            if (copy_call == NULL)
            {
                ERROR("pthread_join() returned invalid thread "
                      "return value");
                out_common->_errno = TE_RC(TE_TA_UNIX, TE_EINVAL);
                break;
            }

            /* Free locations copied in 'out' by _copy_args' */
            op = XDR_FREE;
            if (!call->info->xdr_out((XDR *)&op, call->out))
                ERROR("xdr_tarpc_%s_out() failed", call->info->funcname);

            /* Copy output prepared in the thread */
            memcpy(call->out, copy_call->out, call->info->out_size);
            free(copy_call);

            break;
        }

        default:
            ERROR("Unknown RPC operation");
            out_common->_errno = TE_RC(TE_TA_UNIX, TE_EINVAL);
            break;
    }
}

/** Keepalive time for connection with TA */
#define RPC_TRANSPORT_RECV_TIMEOUT 0xFFFFF

#if defined(ENABLE_RPC_PLUGINS)
/** Data corresponding to the active RPC server plugin */
typedef struct rpcserver_plugin_context {
    int     pid;    /**< Process ID */
    int     tid;    /**< Thread ID */
    te_bool enable; /**< Status of enabling of the plugin */
    te_bool installed; /**< Status of installation of the plugin */

    void       *context; /**< Plugin context */

    /** Callback for creating the context and initializing the plugin */
    te_errno  (*install  )(void **context);

    /** Callback for action of RPC server plugin */
    te_errno  (*action   )(void  *context);

    /** Callback for deinitializing the plugin and removing the context */
    te_errno  (*uninstall)(void **context);

    /** Timeout to detect that connection with TA is broken */
    struct timeval timeout;
} rpcserver_plugin_context;

/** The data of current RPC server plugin */
static rpcserver_plugin_context plugin = {
    .enable = FALSE
};

/**
 * Detect if connection with TA is broken.
 *
 * @return @c TRUE in case connection with TA is broken
 */
static te_bool
plugin_timeout(void)
{
    static struct timeval now;
    gettimeofday(&now, NULL);
    return TIMEVAL_SUB(plugin.timeout, now) < 0;
}

/**
 * Restart the timeout to detect that connection with TA is broken.
 */
static void
plugin_time_restart(void)
{
    gettimeofday(&plugin.timeout, NULL);
    plugin.timeout.tv_sec += RPC_TRANSPORT_RECV_TIMEOUT;
}

/**
 * Execute actions related with the RPC server plugin.
 */
static void
plugin_action(void)
{
    te_errno rc;
    const int pid = getpid();
    const int tid = thread_self();

    if (plugin.pid != pid || plugin.tid != tid)
    {
        ERROR("RPC server plugin disabled "
              "(Unexpected pid=%d, tid=%d, expected %d/%d)",
              pid, tid, plugin.pid, plugin.tid);
        plugin.enable = FALSE;
        return;
    }

    if (!plugin.installed)
    {
        assert(plugin.install != NULL);
        rc = plugin.install(&plugin.context);
        if (TE_RC_GET_ERROR(rc) == TE_EPENDING)
            return;

        if (rc != 0)
        {
            ERROR("Failed to install RPC server plugin: %r",
                  rc);
            plugin.enable = FALSE;
            return;
        }
        plugin.installed = TRUE;
    }

    if (plugin.action == NULL)
        return;

    if ((rc = plugin.action(plugin.context)) != 0)
    {
        if (TE_RC_GET_ERROR(rc) == TE_EPENDING)
            return;

        ERROR("RPC server plugin disabled "
              "(Action fail with exit code: %r)",
              rc);
        plugin.enable = FALSE;
    }
}

/* See description in rcf_pch.h */
te_errno
rpcserver_plugin_enable(
        const char *install, const char *action, const char *uninstall)
{
    te_errno rc = rpcserver_plugin_disable();

    if (rc != 0)
        return rc;

/**
 * Find a callback for the plugin or fail with error if it is impossible
 *
 * @param _name     The field name of callback
 *
 * @note    Function name can be empty string or @c NULL
 * @note    If function is not found, then return status code @c ENOENT
 */
#define SET_CALLBACK_OR_FAIL(_name)                                     \
    do {                                                                \
        if (_name == NULL || _name[0] == '\0')                          \
            plugin._name = NULL;                                        \
        else                                                            \
        {                                                               \
            plugin._name = rcf_ch_symbol_addr(_name, TRUE);             \
            if (plugin._name == NULL)                                   \
            {                                                           \
                ERROR("Failed to enable the RPC server plugin. "        \
                      "Can not find the " #_name " callback \"%s\" "    \
                      "for plugin.", _name);                            \
                plugin.action = NULL;                                   \
                plugin.install = NULL;                                  \
                plugin.uninstall = NULL;                                \
                return TE_RC(TE_RCF_API, TE_ENOENT);                    \
            }                                                           \
        }                                                               \
    } while (0)

    SET_CALLBACK_OR_FAIL(install);
    SET_CALLBACK_OR_FAIL(action);
    SET_CALLBACK_OR_FAIL(uninstall);

#undef SET_CALLBACK_OR_FAIL

    if (plugin.install == NULL && plugin.action == NULL &&
            plugin.uninstall == NULL)
    {
        ERROR("Failed to enable the RPC server plugin. "
              "The plugin must have at least one callback.");
        return TE_RC(TE_RCF_API, TE_EFAULT);
    }

    plugin.enable = TRUE;
    plugin.installed = plugin.install == NULL;
    plugin.pid = getpid();
    plugin.tid = thread_self();

    plugin_time_restart();

    return 0;
}

/* See description in rcf_pch.h */
te_errno
rpcserver_plugin_disable(void)
{
    te_errno rc = 0;

    if (plugin.enable && plugin.installed && plugin.uninstall != NULL)
        rc = plugin.uninstall(&plugin.context);

    plugin.enable    = FALSE;
    plugin.installed = FALSE;
    plugin.install   = NULL;
    plugin.action    = NULL;
    plugin.uninstall = NULL;

    return rc;
}

#endif /* ENABLE_RPC_PLUGINS */


#ifdef HAVE_SIGNAL_H
static void
sig_handler(int s)
{
    UNUSED(s);
    exit(1);
}
#endif

/**
 * Entry function for RPC server. 
 *
 * @param name   RPC server name
 *
 * @return NULL
 */
void *
rcf_pch_rpc_server(const char *name)
{
    rpc_transport_handle handle;
    uint8_t             *buf = NULL;
    int                  pid = getpid();
    int                  tid = thread_self();
    
#define STOP(msg...)    \
    do {                \
        ERROR(msg);     \
        goto cleanup;   \
    } while (0)
    
#ifdef HAVE_SIGNAL_H
    signal(SIGTERM, sig_handler);
#endif

    /*
     * This is done to delete user registered by rcf_ch_start_process(),
     * if it was created by it but not destroyed (otherwise harmless).
     */
    logfork_delete_user(pid, tid);

    if (logfork_register_user(name) != 0)
    {
        fprintf(stderr,
                "logfork_register_user() failed to register %s server\n",
                name);
        fflush(stderr);
    }
    
#ifdef HAVE_PTHREAD_H
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
#endif
    
    if (rpc_transport_connect_ta(name, &handle) != 0)
        return NULL;

    if ((buf = malloc(RCF_RPC_HUGE_BUF_LEN)) == NULL)
        STOP("Failed to allocate the buffer for RPC data");

#if defined(__CYGWIN__) && !defined(WINDOWS)
    RING("RPC server '%s' (%u-bit, cygwin) (re-)started (PID %d, TID %u)",
         name, (unsigned)(sizeof(void *) << 3),
         (int)pid, (unsigned)tid);
#else
    RING("RPC server '%s' (%u-bit) (re-)started (PID %d, TID %u)",
         name, (unsigned)(sizeof(void *) << 3),
         (int)pid, (unsigned)tid);
#endif             

#ifdef __unix__
    if (rcf_rpc_server_init() != 0)
        STOP("Failed to initialize RPC server");
#endif

    rcf_pch_mem_init();

    while (TRUE)
    {
        const char *reply = "OK";
        char      rpc_name[RCF_RPC_MAX_NAME];        
                                        /* RPC name, i.e. "bind" */
        void     *in = NULL;            /* Input parameter C structure */
        void     *out = NULL;           /* Output parameter C structure */
        rpc_info *info = NULL;          /* RPC information */
        te_bool   result = FALSE;       /* "rc" attribute */
        size_t    len = RCF_RPC_HUGE_BUF_LEN;
        te_errno  rc;

        strcpy(rpc_name, "Unknown");

#if defined(ENABLE_RPC_PLUGINS)
        if (!plugin.enable)
#endif
        {
            rc = rpc_transport_recv(
                        handle, buf, &len, RPC_TRANSPORT_RECV_TIMEOUT);
        }
#if defined(ENABLE_RPC_PLUGINS)
        else
        {
            rc = rpc_transport_recv(handle, buf, &len, 0);
            if (TE_RC_GET_ERROR(rc) != TE_ETIMEDOUT)
                plugin_time_restart();
            else if (!plugin_timeout())
            {
                plugin_action();
                continue;
            }
        }
#endif
        if (rc != 0)
            STOP("Connection with TA is broken!");
            
        if (strcmp((char *)buf, "FIN") == 0)
        {
#ifdef __unix__
            if (rcf_rpc_server_finalize() != 0)
                reply = "FAILED";
#endif

            if (rpc_transport_send(handle, (uint8_t *)reply,
                                   strlen(reply) + 1) == 0)
                RING("RPC server '%s' finishing status: %s", name, reply);
            else
                ERROR("Failed to send 'OK' in response to 'FIN'");

            goto cleanup;
        }
        
        if (rpc_xdr_decode_call(buf, len, rpc_name, &in) != 0)
        {
            ERROR("Decoding of RPC %s call failed", rpc_name);
            goto result;
        }

        info = rpc_find_info(rpc_name);
        assert(info != NULL);
        
        if ((out = calloc(1, info->out_len)) == NULL)
        {
            ERROR("Memory allocation failure");
            goto result;
        }
        
        result = (info->rpc)(in, out, NULL);
        
    result: /* Send an answer */
        
        if (in != NULL && info != NULL)
            rpc_xdr_free(info->in, in);
        free(in);
            
        len = RCF_RPC_HUGE_BUF_LEN;
        if (rpc_xdr_encode_result(rpc_name, result, (char *)buf, 
                                  &len, out) != 0)
        {
            STOP("Fatal error: encoding of RPC %s output "
                 "parameters failed", rpc_name);
        }
            
        if (info != NULL)
            rpc_xdr_free(info->out, out);
        free(out);
        
        if (rpc_transport_send(handle, buf, len) != 0)
            STOP("Sending data failed in main RPC server loop");
    }

cleanup:
    logfork_delete_user(pid, tid);
    rpc_transport_close(handle);
    free(buf);
    
#undef STOP    

    return NULL;
}

void
rcf_pch_rpc_server_argv(int argc, char **argv)
{
    UNUSED(argc);

    rcf_pch_rpc_server(argv[0]);
}
