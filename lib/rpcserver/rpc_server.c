/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief RCF Portable Command Handler
 *
 * RCF RPC server entry point. The file is included to another one
 * to create TA-builtin and standalone RPC servers.
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#include "rpc_server.h"
#include "ta_common.h"
#include "agentlib.h"
#include "rpc_transport.h"

#include "te_errno.h"
#include "te_sleep.h"
#include "te_alloc.h"

/* See description in rpc_server.h */
int
tarpc_mutex_lock(pthread_mutex_t *mutex)
{
    int rc;

    rc = pthread_mutex_lock(mutex);
    if (rc != 0)
    {
        te_rpc_error_set(TE_OS_RC(TE_TA, rc),
                         "pthread_mutex_lock() failed");
    }

    return rc;
}

/* See description in rpc_server.h */
int
tarpc_mutex_unlock(pthread_mutex_t *mutex)
{
    int rc;

    rc = pthread_mutex_unlock(mutex);
    if (rc != 0)
    {
        te_rpc_error_set(TE_OS_RC(TE_TA, rc),
                         "pthread_mutex_unlock() failed");
    }

    return rc;
}

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

    arg->name = strdup(name);
    if (arg->name == NULL)
    {
        ERROR("%s(): out of memory when duplicating argument name",
              __FUNCTION__);
        free(arg->pristine);
        free(arg);
        return;
    }

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
        free(cur->name);
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

#ifdef TE_THREAD_LOCAL
/** Thread-local variable for storing RPC error information. */
static TE_THREAD_LOCAL te_rpc_error_data te_rpc_err = { NULL, 0, "" };
#endif

/* See description in rpc_server.h */
void
te_rpc_error_set_target(tarpc_out_arg *out_common)
{
#ifdef TE_THREAD_LOCAL
    te_rpc_err.out_common = out_common;
    te_rpc_err.err = 0;
    te_rpc_err.str[0] = '\0';
#endif
}

/* See description in rpc_server.h */
void
te_rpc_error_set(te_errno err, const char *msg, ...)
{
#ifdef TE_THREAD_LOCAL
    if (te_rpc_err.out_common == NULL)
    {
        ERROR("te_rpc_error_set() seems to be called outside of "
              "TARPC_FUNC_COMMON()");
    }
    else
    {
        te_string   str = TE_STRING_BUF_INIT(te_rpc_err.str);
        char       *s;
        va_list     ap;
        te_errno    ret;

        tarpc_out_arg *out_common = te_rpc_err.out_common;

        te_rpc_err.err = err;
        out_common->_errno = err;
        out_common->errno_changed = TRUE;

        va_start(ap, msg);
        ret = te_string_append_va(&str, msg, ap);
        va_end(ap);
        if (ret != 0)
        {
            ERROR("te_string_append() failed to write error "
                  "message, rc = %r", ret);
            te_rpc_err.str[0] = '\0';
        }
        else if (str.len > 0)
        {
            ERROR("%s", str.ptr);
        }

        s = strdup(te_rpc_err.str);
        if (s != NULL)
        {
            free(out_common->err_str.err_str_val);
            out_common->err_str.err_str_val = s;
            out_common->err_str.err_str_len = strlen(s) + 1;
        }
        else
        {
            ERROR("Out of memory when trying to copy RPC error string");
        }
    }

#else
    ERROR("te_rpc_error_set() cannot be used since there is no "
          "thread-local specifier support");
#endif
}

/* See description in rpc_server.h */
te_errno
te_rpc_error_get_num(void)
{
#ifdef TE_THREAD_LOCAL
    return te_rpc_err.err;
#endif

    return 0;
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

#ifndef TE_THREAD_LOCAL
    out_common->_errno = RPC_ERRNO;
    out_common->errno_changed = (call->saved_errno != errno);
#else
    if (te_rpc_err.err == 0)
    {
        out_common->_errno = RPC_ERRNO;
        out_common->errno_changed = (call->saved_errno != errno);
    }
#endif

    gettimeofday(&finish, NULL);
    out_common->duration =
        TE_SEC2US(finish.tv_sec - call->call_start.tv_sec) +
        finish.tv_usec - call->call_start.tv_usec;
    rc = tarpc_check_args(&call->checked_args);
    if (rc != 0)
    {
        out_common->_errno = rc;
        free(out_common->err_str.err_str_val);
        out_common->err_str.err_str_val = NULL;
        out_common->err_str.err_str_len = 0;
    }
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

typedef struct deferred_call {
    TAILQ_ENTRY(deferred_call) next;
    uintptr_t      jobid;
    rpc_call_data *call;
} deferred_call;

TAILQ_HEAD(deferred_call_list, deferred_call);

te_errno
tarpc_defer_call(deferred_call_list *list,
                 uintptr_t jobid, rpc_call_data *call)
{
    deferred_call *defer = TE_ALLOC(sizeof(*defer));

    if (defer == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    defer->jobid = jobid;
    defer->call  = call;
    TAILQ_INSERT_TAIL(list, defer, next);

    return 0;
}

te_bool
tarpc_has_deferred_calls(const deferred_call_list *list)
{
    deferred_call *defer = NULL;
    TAILQ_FOREACH(defer, list, next)
        if (!defer->call->done)
            return TRUE;
    return FALSE;
}

static rpc_call_data *
tarpc_find_deferred(deferred_call_list *list,
                    uintptr_t jobid, te_bool complete)
{
    rpc_call_data *call;
    deferred_call *defer = NULL;

    TAILQ_FOREACH(defer, list, next)
    {
        if (defer->jobid == jobid)
            break;
    }
    if (defer == NULL)
        return NULL;

    call = defer->call;
    if (complete)
    {
        if (!defer->call->done)
            defer->call->info->wrapper(defer->call);

        TAILQ_REMOVE(list, defer, next);
        free(defer);
    }
    return call;
}

static void
tarpc_run_deferred(deferred_call_list *list, rpc_transport_handle handle)
{
    deferred_call *defer = NULL;
    tarpc_rpc_is_op_done_out result;
    char enc_result[RCF_MAX_VAL];
    size_t enc_len;
    te_errno rc;

    TAILQ_FOREACH(defer, list, next)
    {
        if (!defer->call->done)
        {
            defer->call->info->wrapper(defer->call);
            defer->call->done = TRUE;

            enc_len = sizeof(enc_result);
            memset(&result, 0, sizeof(result));
            result.common.jobid = defer->jobid;
            result.common.unsolicited = TRUE;
            result.done = TRUE;

            rc = rpc_xdr_encode_result("rpc_is_op_done", TRUE,
                                       enc_result, &enc_len,
                                       &result);
            if (rc != 0)
            {
                ERROR("Cannot encode rpc_op_is_done result: %r", rc);
            }
            else
            {
                rc = rpc_transport_send(handle, (uint8_t *)enc_result, enc_len);
                if (rc != 0)
                {
                    ERROR("Cannot send async call notification: %r", rc);
                }
            }
        }
    }
}

void
tarpc_generic_service(deferred_call_list *async_list, rpc_call_data *call)
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
        rc = tarpc_find_func(in_common->lib_flags, call->info->funcname,
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

            VERB("%s(): CALL", call->info->funcname);

            if ((copy_call = calloc(1, sizeof(*copy_call) +
                                    call->info->in_size +
                                    call->info->out_size)) == NULL)
            {
                out_common->_errno = TE_RC(TE_TA_UNIX, TE_ENOMEM);
                break;
            }

            *copy_call = *call;
            assert(STAILQ_EMPTY(&call->checked_args));
            STAILQ_INIT(&copy_call->checked_args);
            copy_call->in = (uint8_t *)copy_call + sizeof(*copy_call);
            copy_call->out = (uint8_t *)copy_call->in +
                             copy_call->info->in_size;
            memcpy(copy_call->in, call->in, call->info->in_size);
            memcpy(copy_call->out, call->out, call->info->out_size);

            if ((rc = tarpc_defer_call(async_list, (uintptr_t)copy_call,
                                       copy_call)) != 0)
            {
                free(copy_call);
                out_common->_errno = rc;
                break;
            }

            /*
             * Preset 'in' and 'out' with zeros to avoid any
             * resource deallocations by the caller.
             * 'out' is preset with zeros above, but may be
             * modified in '_copy_args'.
             */
            memset(call->in,  0, call->info->in_size);
            memset(call->out, 0, call->info->out_size);

            out_common->jobid = (uintptr_t)copy_call;

            break;
        }
        case RCF_RPC_WAIT:
        {
            rpc_call_data *copy_call;
            enum xdr_op   op;

            VERB("%s(): WAIT", call->info->funcname);

            copy_call = tarpc_find_deferred(async_list, in_common->jobid,
                                            TRUE);

            if (copy_call == NULL)
            {
                ERROR("No call with ID %llu to wait",
                      (unsigned long long int)in_common->jobid);
                out_common->_errno = TE_RC(TE_TA_UNIX, TE_ENOENT);
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
    te_errno  (*action   )(deferred_call_list *async_list, void *context);

    /** Callback for deinitializing the plugin and removing the context */
    te_errno  (*uninstall)(void **context);

    /** Timeout to detect that connection with TA is broken */
    struct timeval timeout;
} rpcserver_plugin_context;

/** The data of current RPC server plugin */
static __thread rpcserver_plugin_context plugin = {
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
plugin_action(deferred_call_list *call_list)
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

    if ((rc = plugin.action(call_list, plugin.context)) != 0)
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
    deferred_call_list   deferred_calls =
        TAILQ_HEAD_INITIALIZER(deferred_calls);
    /* We do not really need any of svcreq stuff, but
     * we need to pass the local deferred_calls pointer
     * to underlying RPC implementations
     */
#ifdef HAVE_SVCXPRT
    SVCXPRT         pseudo_xprt = {
#else
    struct SVCXPRT  pseudo_xprt = {
#endif
        .xp_p1 = (void *)&deferred_calls
    };
    struct svc_req       pseudo_req = {
        .rq_xprt = &pseudo_xprt
    };

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
                plugin_action(&deferred_calls);
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

        result = (info->rpc)(in, out, &pseudo_req);

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

        tarpc_run_deferred(&deferred_calls, handle);
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
