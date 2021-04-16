/** @file
 * @brief Unix Test Agent
 *
 * RPC routines implementation
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
 *
 *
 * @author Artem V. Andreev <Artem.Andreev@oktetlabs.ru>
 * @author Elena A. Vengerova <Elena.Vengerova@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER     "RPC"

#include "rpc_server.h"

#ifdef HAVE_DLFCN_H
#include <dlfcn.h>
#endif

#if HAVE_NETINET_IN_SYSTM_H /* Required for FreeBSD build */
#include <netinet/in_systm.h>
#endif

#ifdef HAVE_SCSI_SG_H
#include <scsi/sg.h>
#elif defined HAVE_CAM_SCSI_SCSI_SG_H
#include <cam/scsi/scsi_sg.h>
#endif

#if HAVE_SYS_SYSTEMINFO_H
#include <sys/systeminfo.h>
#endif

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif

#if HAVE_LINUX_ETHTOOL_H
#include "te_ethtool.h"
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_SYS_STATVFS_H
#include <sys/statvfs.h>
#endif
#ifdef HAVE_DIRENT_H
#include <dirent.h>
#endif

#ifdef HAVE_LINUX_NET_TSTAMP_H
#include <linux/net_tstamp.h>
#endif

#ifdef HAVE_PTHREAD_H
#include <pthread.h>
#endif

#include "te_defs.h"
#include "te_queue.h"
#include "te_tools.h"
#include "te_dbuf.h"
#include "te_str.h"
#include "tq_string.h"

#include "agentlib.h"
#include "iomux.h"
#include "rpcs_msghdr.h"
#include "rpcs_conv.h"

#ifndef MSG_MORE
#define MSG_MORE 0
#endif

#ifdef LIO_READ
void *dummy = aio_read;
#endif

/* FIXME: The following are defined inside Agent */
extern const char *ta_name;
extern const char *ta_execname;
extern char ta_dir[RCF_MAX_PATH];


/** User environment */
extern char **environ;

#if (defined(__sun) || defined(sun)) && \
    (defined(__SVR4) || defined(__svr4__))
#define SOLARIS TRUE
#else
#define SOLARIS FALSE
#endif

/** Entry in a queue of FD close hooks */
typedef struct close_fd_hook_entry {
    TAILQ_ENTRY(close_fd_hook_entry)   links;  /**< Queue links */
    tarpc_close_fd_hook               *hook;   /**< Hook function pointer */
    void                              *cookie; /**< Pointer which should be
                                                    passed to each hook
                                                    invocation */
} close_fd_hook_entry;

/** Hooks called just before closing FD */
static TAILQ_HEAD(close_fd_hook_entries,
                  close_fd_hook_entry) close_fd_hooks =
                              TAILQ_HEAD_INITIALIZER(close_fd_hooks);

/** Lock protecting close_fd_hooks */
static pthread_mutex_t      close_fd_hooks_lock = PTHREAD_MUTEX_INITIALIZER;

extern sigset_t         rpcs_received_signals;
extern tarpc_siginfo_t  last_siginfo;

static te_bool   dynamic_library_set = FALSE;
static char      dynamic_library_name[RCF_MAX_PATH];
static void     *dynamic_library_handle = NULL;

/* See description in rpc_server.h */
void
tarpc_close_fd_hooks_call(int fd)
{
    int                  res = 0;
    close_fd_hook_entry *entry;

    /*
     * It seems safe to check this without mutex protection,
     * and it will allow to avoid mutex locking/unlocking in case
     * there is no hooks.
     */
    if (TAILQ_EMPTY(&close_fd_hooks))
        return;

    res = tarpc_mutex_lock(&close_fd_hooks_lock);
    if (res != 0)
        return;

    TAILQ_FOREACH(entry, &close_fd_hooks, links)
    {
        entry->hook(fd, entry->cookie);
    }

    tarpc_mutex_unlock(&close_fd_hooks_lock);
}

/* See description in rpc_server.h */
int
tarpc_close_fd_hook_register(tarpc_close_fd_hook *hook, void *cookie)
{
    close_fd_hook_entry *entry;
    int                  rc;

    if (hook == NULL)
    {
        te_rpc_error_set(TE_RC(TE_TA_UNIX, TE_EINVAL),
                         "%s(): hook cannot be NULL",
                         __FUNCTION__);
        return -1;
    }

    entry = calloc(1, sizeof(*entry));
    if (entry == NULL)
    {
        te_rpc_error_set(TE_RC(TE_TA_UNIX, TE_ENOMEM),
                         "%s(): failed to allocate memory",
                         __FUNCTION__);
        return -1;
    }
    entry->hook = hook;
    entry->cookie = cookie;

    rc = tarpc_mutex_lock(&close_fd_hooks_lock);
    if (rc != 0)
    {
        free(entry);
        return -1;
    }
    TAILQ_INSERT_TAIL(&close_fd_hooks, entry, links);
    tarpc_mutex_unlock(&close_fd_hooks_lock);

    return 0;
}

/* See description in rpc_server.h */
int
tarpc_close_fd_hook_unregister(tarpc_close_fd_hook *hook, void *cookie)
{
    close_fd_hook_entry *entry;
    te_bool              found = FALSE;
    int                  rc;

    rc = tarpc_mutex_lock(&close_fd_hooks_lock);
    if (rc != 0)
        return -1;

    /*
     * List is walked in reverse order so that the last added hook
     * is unregistered firstly (in case of hooks duplication).
     */
    TAILQ_FOREACH_REVERSE(entry, &close_fd_hooks, close_fd_hook_entries,
                          links)
    {
        if (entry->hook == hook && entry->cookie == cookie)
        {
            found = TRUE;
            TAILQ_REMOVE(&close_fd_hooks, entry, links);
            free(entry);
            break;
        }
    }

    tarpc_mutex_unlock(&close_fd_hooks_lock);

    if (!found)
    {
        te_rpc_error_set(TE_RC(TE_TA_UNIX, TE_ENOENT),
                         "%s(): failed to find hook %p",
                         __FUNCTION__, hook);
        return -1;
    }

    return 0;
}

/* See description in rpc_server.h */
int
tarpc_call_close_with_hooks(api_func close_func, int fd)
{
    tarpc_close_fd_hooks_call(fd);
    return close_func(fd);
}

/**
 * Set name of the dynamic library to be used to resolve function
 * called via RPC.
 *
 * @param libname       Full name of the dynamic library or NULL
 *
 * @return Status code.
 *
 * @note The dinamic library is opened with RTLD_NODELETE flag.
 * This flag is necessary for all libraries using atfork since there is no
 * way to undo the atfork call.  This flag is also necessary if the library
 * does not have correct _fini.  See man dlopen of other details.
 */
te_errno
tarpc_setlibname(const char *libname)
{
    extern int (*tce_notify_function)(void);
    extern int (*tce_get_peer_function)(void);
    extern const char *(*tce_get_conn_function)(void);

    void (*tce_initializer)(const char *, int) = NULL;

    if (libname == NULL)
        libname = "";

    if (dynamic_library_set)
    {
        char *old = getenv("TARPC_DL_NAME");

        if (old == NULL)
        {
            ERROR("Inconsistent state of dynamic library flag and "
                  "Environment");
            return TE_RC(TE_TA_UNIX, TE_EFAULT);
        }
        if (strcmp(libname, old) == 0)
        {
            /* It is OK, if we try to set the same library once more */
            return 0;
        }
        ERROR("Dynamic library has already been set to %s", old);
        return TE_RC(TE_TA_UNIX, TE_EEXIST);
    }
    dynamic_library_handle = dlopen(*libname == '\0' ? NULL : libname,
                                    RTLD_LAZY
#ifdef HAVE_RTLD_NODELETE
                                    | RTLD_NODELETE
#endif
                                    );
    if (dynamic_library_handle == NULL)
    {
        if (*libname == 0)
        {
            dynamic_library_set = TRUE;
            return 0;
        }
        ERROR("Cannot load shared library '%s': %s", libname, dlerror());
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }
    if (setenv("TARPC_DL_NAME", libname, 1) != 0)
    {
        ERROR("No enough space in environment to save dynamic library "
              "'%s' name", libname);
        (void)dlclose(dynamic_library_handle);
        dynamic_library_handle = NULL;
        return TE_RC(TE_TA_UNIX, TE_ENOSPC);
    }
    dynamic_library_set = TRUE;
    TE_STRLCPY(dynamic_library_name, libname,
               sizeof(dynamic_library_name));
    RING("Dynamic library is set to '%s'", libname);

    if (tce_get_peer_function != NULL)
    {
        tce_initializer = dlsym(dynamic_library_handle,
                                "__bb_init_connection");
        if (tce_initializer != NULL)
        {
            const char *ptc = tce_get_conn_function();

            if (ptc == NULL)
                WARN("tce_init_connect() has not been called");
            else
            {
                if (tce_notify_function != NULL)
                    tce_notify_function();
                tce_initializer(ptc, tce_get_peer_function());
                RING("TCE initialized for dynamic library '%s'",
                     getenv("TARPC_DL_NAME"));
            }
        }
    }

    return 0;
}

/* See the description in tarpc_server.h */
te_bool
tarpc_dynamic_library_loaded(void)
{
    return dynamic_library_set && dynamic_library_handle != NULL;
}

/**
 * Find the function by its name.
 *
 * @param lib_flags     how to resolve function name
 * @param name          function name
 * @param func          location for function address
 *
 * @return status code
 */
int
tarpc_find_func(tarpc_lib_flags lib_flags, const char *name, api_func *func)
{
    te_errno    rc;
    void       *handle;
    char       *tarpc_dl_name;

    *func = NULL;

    tarpc_dl_name = getenv("TARPC_DL_NAME");
    if (!dynamic_library_set && tarpc_dl_name != NULL &&
        (rc = tarpc_setlibname(tarpc_dl_name)) != 0)
    {
        /* Error is always logged from tarpc_setlibname() */
        return rc;
    }
#if defined (__QNX__)
    /*
     * QNX may set errno to ESRCH even after successful
     * call to 'getenv'.
     */
    errno = 0;
#endif

#define TARPC_MAX_FUNC_NAME 64

    if (lib_flags & TARPC_LIB_USE_SYSCALL)
    {
        char func_syscall_wrap_name[TARPC_MAX_FUNC_NAME] = {0};

        if ((lib_flags & TARPC_LIB_USE_LIBC) || !tarpc_dynamic_library_loaded())
            TE_SPRINTF(func_syscall_wrap_name, "%s_te_wrap_syscall", name);
        else
            TE_SPRINTF(func_syscall_wrap_name, "%s_te_wrap_syscall_dl", name);

        if ((*func = rcf_ch_symbol_addr(func_syscall_wrap_name, 1)) != NULL)
            return 0;

        /* Wrapper was not found, continues to standart name resolving */
    }

    if ((lib_flags & TARPC_LIB_USE_LIBC) || !tarpc_dynamic_library_loaded())
    {
        static void *libc_handle = NULL;
        static te_bool dlopen_null = FALSE;

        if (dlopen_null)
            goto try_ta_symtbl;

        if (libc_handle == NULL)
        {
            if ((libc_handle = dlopen(NULL, RTLD_LAZY)) == NULL)
            {
                dlopen_null = TRUE;
                goto try_ta_symtbl;
            }
        }
        handle = libc_handle;
        VERB("Call from libc");
    }
    else
    {
        /*
         * We get this branch of the code only if user set some
         * library to be used with tarpc_setlibname() function earlier,
         * and so we should use it to find symbol.
         */
        assert(dynamic_library_set == TRUE);
        assert(dynamic_library_handle != NULL);

        handle = dynamic_library_handle;
        VERB("Call from registered library");
    }

    *func = dlsym(handle, name);

try_ta_symtbl:
    if (*func == NULL)
    {
        if ((*func = rcf_ch_symbol_addr(name, 1)) == NULL)
        {
            ERROR("Cannot resolve symbol %s", name);
            return TE_RC(TE_TA_UNIX, TE_ENOENT);
        }
    }
    return 0;
}

/**
 * Find the pointer to function by its name in table.
 * Try to convert string to long int and cast it to the pointer
 * in the case if function is implemented as a static one. Use it
 * for signal handlers only.
 *
 * @param name  function name (or pointer value converted to string)
 * @param handler returned pointer to function or NULL if error
 *
 * @return errno if error or 0
 */
static te_errno
name2handler(const char *name, void **handler)
{
    if (name == NULL || *name == '\0')
    {
        *handler = NULL;
        return 0;
    }

    *handler = rcf_ch_symbol_addr(name, 1);
    if (*handler == NULL)
    {
        char *tmp;
        int   id;

        if (strcmp(name, "SIG_ERR") == 0)
            *handler = (void *)SIG_ERR;
        else if (strcmp(name, "SIG_DFL") == 0)
            *handler = (void *)SIG_DFL;
        else if (strcmp(name, "SIG_IGN") == 0)
            *handler = (void *)SIG_IGN;
        else if (strcmp(name, "NULL") == 0)
            *handler = NULL;
        else
        {
            id = strtol(name, &tmp, 10);
            if (tmp == name || *tmp != '\0')
                return TE_RC(TE_TA_UNIX, TE_ENOENT);

            *handler = rcf_pch_mem_get(id);
        }
    }
    return 0;
}

/**
 * Find the function name in table according to pointer to one.
 * Try to convert pointer value to string in the case if function
 * is implemented as a static one. Use it for signal handlers only.
 *
 * @param handler  pointer to function
 *
 * @return Allocated name or NULL in the case of memory allocation failure
 */
static char *
handler2name(void *handler)
{
    const char *sym_name;
    char *tmp;

    if (handler == (void *)SIG_ERR)
        tmp = strdup("SIG_ERR");
    else if (handler == (void *)SIG_DFL)
        tmp = strdup("SIG_DFL");
    else if (handler == (void *)SIG_IGN)
        tmp = strdup("SIG_IGN");
    else if (handler == NULL)
        tmp = strdup("NULL");
    else if ((sym_name = rcf_ch_symbol_name(handler)) != NULL)
        tmp = strdup(sym_name);
    else if ((tmp = calloc(1, 16)) != NULL)
    {
        rpc_ptr  id = 0;
        te_errno rc;

        rc = rcf_pch_mem_index_ptr_to_mem_gen(handler,
                                              rcf_pch_mem_ns_generic(),
                                              &id);
        if (rc == TE_RC(TE_RCF_PCH, TE_ENOENT))
        {
            id = rcf_pch_mem_alloc(handler);
            RING("Unknown signal handler %p is registered as "
                 "ID %u in RPC server memory", handler, id);
        }
        else if (rc != 0)
            ERROR("Failed to get RPC pointer id for %p: %r", handler, rc);

        /* FIXME */
        sprintf(tmp, "%u", id);
    }

    if (tmp == NULL)
    {
        ERROR("Out of memory");
        /* FIXME */
        return strdup("");
    }

    return tmp;
}


/*-------------- setlibname() -----------------------------*/

bool_t
_setlibname_1_svc(tarpc_setlibname_in *in, tarpc_setlibname_out *out,
                 struct svc_req *rqstp)
{
    UNUSED(rqstp);
    memset(out, 0, sizeof(*out));
    VERB("PID=%d TID=%llu: Entry %s",
         (int)getpid(), (unsigned long long int)pthread_self(),
         "setlibname");

    out->common._errno = tarpc_setlibname((in->libname.libname_len == 0) ?
                                          NULL : in->libname.libname_val);
    out->retval = (out->common._errno == 0) ? 0 : -1;
    out->common.duration = 0;

    return TRUE;
}

/*-------------- rpc_find_func() ----------------------*/

bool_t
_rpc_find_func_1_svc(tarpc_rpc_find_func_in  *in,
                     tarpc_rpc_find_func_out *out,
                     struct svc_req          *rqstp)
{
    api_func func;

    UNUSED(rqstp);

    memset(out, 0, sizeof(*out));

    out->find_result = tarpc_find_func(in->common.lib_flags,
                                       in->func_name, &func);
    return TRUE;
}

/*-------------- sizeof() -------------------------------*/
#define MAX_TYPE_NAME_SIZE 30
typedef struct {
    char           type_name[MAX_TYPE_NAME_SIZE];
    tarpc_ssize_t  type_size;
} type_info_t;

static type_info_t type_info[] =
{
    {"te_bool", sizeof(te_bool)},
    {"char", sizeof(char)},
    {"short", sizeof(short)},
    {"int", sizeof(int)},
    {"long", sizeof(long)},
    {"long long", sizeof(long long)},
    {"te_errno", sizeof(te_errno)},
    {"size_t", sizeof(size_t)},
    {"socklen_t", sizeof(socklen_t)},
    {"struct timeval", sizeof(struct timeval)},
   {"struct linger", sizeof(struct linger)},
    {"struct in_addr", sizeof(struct in_addr)},
    {"struct ip_mreq", sizeof(struct ip_mreq)},
    {"struct tcp_info", sizeof(struct tcp_info)},
    {"struct ip_mreq_source", sizeof(struct ip_mreq_source)},
#if HAVE_STRUCT_IP_MREQN
    {"struct ip_mreqn", sizeof(struct ip_mreqn)},
#endif
    {"struct sockaddr", sizeof(struct sockaddr)},
    {"struct sockaddr_in", sizeof(struct sockaddr_in)},
    {"struct sockaddr_in6", sizeof(struct sockaddr_in6)},
    {"struct sockaddr_storage", sizeof(struct sockaddr_storage)},
    {"struct cmsghdr", sizeof(struct cmsghdr)},
    {"struct msghdr", sizeof(struct msghdr)},
};

/*-------------- get_sizeof() ---------------------------------*/
bool_t
_get_sizeof_1_svc(tarpc_get_sizeof_in *in, tarpc_get_sizeof_out *out,
                  struct svc_req *rqstp)
{
    uint32_t i;

    UNUSED(rqstp);

    out->size = -1;

    if (in->typename == NULL)
    {
        ERROR("Name of type not specified");
        return FALSE;
    }

    if (in->typename[0] == '*')
    {
        out->size = sizeof(void *);
        return TRUE;
    }

    for (i = 0; i < sizeof(type_info) / sizeof(type_info_t); i++)
    {
        if (strcmp(in->typename, type_info[i].type_name) == 0)
        {
            out->size = type_info[i].type_size;
            return TRUE;
        }
    }

    ERROR("Unknown type (%s)", in->typename);
#if 0
    out->common._errno = TE_RC(TE_TA_UNIX, TE_EINVAL);
#endif
    return TRUE;
}

/*-------------- get_addrof() ---------------------------------*/
bool_t
_get_addrof_1_svc(tarpc_get_addrof_in *in, tarpc_get_addrof_out *out,
                  struct svc_req *rqstp)
{
    void *addr = rcf_ch_symbol_addr(in->name, 0);

    UNUSED(rqstp);

    out->addr = addr == NULL ? 0 : rcf_pch_mem_alloc(addr);

    return TRUE;
}

/*-------------- get_var() ---------------------------------*/
bool_t
_get_var_1_svc(tarpc_get_var_in *in, tarpc_get_var_out *out,
                   struct svc_req *rqstp)
{
    void *addr = rcf_ch_symbol_addr(in->name, 0);

    UNUSED(rqstp);

    if (addr == NULL)
    {
        ERROR("Variable %s is not found", in->name);
        out->found = FALSE;
        return TRUE;
    }

    out->found = TRUE;

    switch (in->size)
    {
        case 1: out->val = *(uint8_t *)addr; break;
        case 2: out->val = *(uint16_t *)addr; break;
        case 4: out->val = *(uint32_t *)addr; break;
        case 8: out->val = *(uint64_t *)addr; break;
        default: return FALSE;
    }

    return TRUE;
}

/*-------------- set_var() ---------------------------------*/
bool_t
_set_var_1_svc(tarpc_set_var_in *in, tarpc_set_var_out *out,
               struct svc_req *rqstp)
{
    void *addr = rcf_ch_symbol_addr(in->name, 0);

    UNUSED(rqstp);
    UNUSED(out);

    if (addr == NULL)
    {
        ERROR("Variable %s is not found", in->name);
        out->found = FALSE;
        return TRUE;
    }

    out->found = TRUE;

    switch (in->size)
    {
        case 1: *(uint8_t *)addr  = in->val; break;
        case 2: *(uint16_t *)addr = in->val; break;
        case 4: *(uint32_t *)addr = in->val; break;
        case 8: *(uint64_t *)addr = in->val; break;
        default: return FALSE;
    }

    return TRUE;
}

/*-------------- create_process() ---------------------------------*/
void
ta_rpc_execve(const char *name)
{
    const char   *argv[5] = {ta_execname,
                             "exec",
                             "rcf_pch_rpc_server_argv",
                             name,
                             NULL};
    api_func_ptr  func;
    int rc;

    VERB("execve() args: %s, %s, %s, %s",
         argv[0], argv[1], argv[2], argv[3]);
    /* Call execve() */
    rc = tarpc_find_func(TARPC_LIB_DEFAULT, "execve", (api_func *)&func);
    if (rc != 0)
    {
        rc = errno;
        LOG_PRINT("No execve function: errno=%d", rc);
        exit(1);
    }

    rc = func((void *)ta_execname, argv, environ);
    if (rc != 0)
    {
        rc = errno;
        LOG_PRINT("execve() failed: errno=%d", rc);
    }

}

bool_t
_create_process_1_svc(tarpc_create_process_in *in,
                      tarpc_create_process_out *out,
                      struct svc_req *rqstp)
{
    UNUSED(rqstp);
    memset(out, 0, sizeof(*out));

    out->pid = fork();

    if (out->pid == -1)
    {
        out->common._errno = TE_OS_RC(TE_TA_UNIX, errno);
        return TRUE;
    }
    if (out->pid == 0)
    {
        /*
         * Change the process group to allow killing all the children
         * together with this RPC server and to disallow killing of this
         * process when its parent RPC server is killed
         */
        setpgid(getpid(), getpid());

        if (in->flags & RCF_RPC_SERVER_GET_EXEC)
            ta_rpc_execve(in->name.name_val);
        rcf_pch_rpc_server(in->name.name_val);
        exit(EXIT_FAILURE);
    }

    return TRUE;
}

/*-------------- vfork() -------------------------------------*/
bool_t
_vfork_1_svc(tarpc_vfork_in *in,
             tarpc_vfork_out *out,
             struct svc_req *rqstp)
{
    struct timeval  t_start;
    struct timeval  t_finish;
    api_func_void   func;
    int             rc;

    UNUSED(rqstp);
    memset(out, 0, sizeof(*out));

    rc = tarpc_find_func(in->common.lib_flags, "vfork", (api_func *)&func);
    if (rc != 0)
    {
        rc = errno;
        ERROR("No vfork() function: errno=%d", rc);
        out->common._errno = TE_OS_RC(TE_TA_UNIX, rc);
        return TRUE;
    }

    run_vfork_hooks(VFORK_HOOK_PHASE_PREPARE);
    gettimeofday(&t_start, NULL);
    out->pid = func();
    gettimeofday(&t_finish, NULL);
    out->elapsed_time = (t_finish.tv_sec - t_start.tv_sec) * 1000 +
                        (t_finish.tv_usec - t_start.tv_usec) / 1000;

    if (out->pid == -1)
    {
        out->common._errno = TE_OS_RC(TE_TA_UNIX, errno);
        run_vfork_hooks(VFORK_HOOK_PHASE_PARENT);
        return TRUE;
    }

    if (out->pid == 0)
    {
        /*
         * Change the process group to allow killing all the children
         * together with this RPC server and to disallow killing of this
         * process when its parent RPC server is killed
         */
        setpgid(getpid(), getpid());

        run_vfork_hooks(VFORK_HOOK_PHASE_CHILD);
        rcf_pch_rpc_server(in->name.name_val);
        exit(EXIT_FAILURE);
    }
    else
    {
        usleep(in->time_to_wait * 1000);
        run_vfork_hooks(VFORK_HOOK_PHASE_PARENT);
    }

    return TRUE;
}

/*-------------- thread_create() -----------------------------*/
bool_t
_thread_create_1_svc(tarpc_thread_create_in *in,
                     tarpc_thread_create_out *out,
                     struct svc_req *rqstp)
{
    pthread_t tid;

    UNUSED(rqstp);

    TE_COMPILE_TIME_ASSERT(sizeof(pthread_t) <= sizeof(tarpc_pthread_t));

    memset(out, 0, sizeof(*out));

    out->retval = pthread_create(&tid, NULL, (void *)rcf_pch_rpc_server,
                                 strdup(in->name.name_val));

    if (out->retval == 0)
        out->tid = (tarpc_pthread_t)tid;

    return TRUE;
}

/*-------------- thread_cancel() -----------------------------*/
bool_t
_thread_cancel_1_svc(tarpc_thread_cancel_in *in,
                     tarpc_thread_cancel_out *out,
                     struct svc_req *rqstp)
{
    UNUSED(rqstp);
    memset(out, 0, sizeof(*out));

    out->retval = pthread_cancel((pthread_t)in->tid);

    return TRUE;
}

/*-------------- thread_join() -----------------------------*/
bool_t
_thread_join_1_svc(tarpc_thread_join_in *in,
                   tarpc_thread_join_out *out,
                   struct svc_req *rqstp)
{
    UNUSED(rqstp);
    memset(out, 0, sizeof(*out));

    out->retval = pthread_join((pthread_t)in->tid, NULL);
    return TRUE;
}

/**
 * Check, if some signals were received by the RPC server (as a process)
 * and return the mask of received signals.
 */

bool_t
_sigreceived_1_svc(tarpc_sigreceived_in *in, tarpc_sigreceived_out *out,
                   struct svc_req *rqstp)
{
    static rpc_ptr ptr = 0;

    UNUSED(in);
    UNUSED(rqstp);
    memset(out, 0, sizeof(*out));

    if (ptr == 0)
        ptr = rcf_pch_mem_alloc(&rpcs_received_signals);
    out->set = ptr;

    return TRUE;
}

/**
 * Get siginfo_t structure for the lastly received signal.
 */
bool_t
_siginfo_received_1_svc(tarpc_siginfo_received_in *in,
                        tarpc_siginfo_received_out *out,
                        struct svc_req *rqstp)
{
    UNUSED(in);
    UNUSED(rqstp);
    memset(out, 0, sizeof(*out));

    memcpy(&out->siginfo, &last_siginfo, sizeof(last_siginfo));

    return TRUE;
}

/*-------------- execve() ---------------------------------*/
TARPC_FUNC_STANDALONE(execve, {},
{
    /* Wait until main thread sends answer to non-blocking RPC call */
    sleep(1);

    MAKE_CALL(ta_rpc_execve(in->name));
}
)

/*-------------- execve_gen() ---------------------------------*/

/**
 * Convert iovec array to NULL terminated array
 *
 * @param list_ptr  List of arguments for INIT_CHECKED_ARG macros
 * @param iov       iovec array
 * @param arr       Location for NULL terminated array
 */
static void
unistd_iov_to_arr_null(checked_arg_list *arglist, struct tarpc_iovec *iov,
                       size_t len, char *arr[])
{
    size_t i;

    if (len == 0)
        return;

    for (i = 0; i < len; i++)
    {
        INIT_CHECKED_ARG(iov[i].iov_base.iov_base_val,
                         iov[i].iov_base.iov_base_len,
                         iov[i].iov_base.iov_base_len);
        arr[i] = (char *)iov[i].iov_base.iov_base_val;
    }
}

int
execve_gen(const char *filename, char *const argv[], char *const envp[])
{
    api_func_ptr func_execve;

    if (tarpc_find_func(TARPC_LIB_DEFAULT, "execve",
                        (api_func *)&func_execve) != 0)
    {
        ERROR("Failed to find function execve()");
        return -1;
    }

    return func_execve((void *)filename, argv, envp);
}

TARPC_FUNC(execve_gen, {},
{
    char *argv[in->argv.argv_len];
    char *envp[in->envp.envp_len];

    unistd_iov_to_arr_null(arglist, in->argv.argv_val,
                           in->argv.argv_len, argv);
    unistd_iov_to_arr_null(arglist, in->envp.envp_val,
                           in->envp.envp_len, envp);

    /* Wait until main thread sends answer to non-blocking RPC call */
    sleep(1);

    MAKE_CALL(func_ptr((void *)in->filename,
                       in->argv.argv_len == 0 ? NULL : argv,
                       in->envp.envp_len == 0 ? NULL : envp));
}
)


/*-------------- exit() --------------------------------*/
TARPC_FUNC(exit, {}, { MAKE_CALL(func(in->status)); })
TARPC_FUNC(_exit, {}, { MAKE_CALL(func(in->status)); })

/*-------------- getpid() --------------------------------*/
TARPC_FUNC(getpid, {}, { MAKE_CALL(out->retval = func_void()); })

/*-------------- pthread_self() --------------------------*/
TARPC_FUNC(pthread_self, {},
{
    MAKE_CALL(out->retval = (tarpc_pthread_t)func());
}
)

/*-------------- pthread_cancel() --------------------------*/
TARPC_FUNC(pthread_cancel, {},
{
    MAKE_CALL(out->retval = func(in->tid));
    if (out->retval != 0)
    {
        te_rpc_error_set(TE_OS_RC(TE_RPC, out->retval), "");
        out->retval = -1;
    }
}
)

/*-------------- pthread_setcancelstate() --------------------------*/
TARPC_FUNC(pthread_setcancelstate, {},
{
    int oldstate;

    MAKE_CALL(out->retval = func(pthread_cancelstate_rpc2h(in->state),
                                 &oldstate));

    out->oldstate = pthread_cancelstate_h2rpc(oldstate);
    if (out->retval != 0)
    {
        te_rpc_error_set(TE_OS_RC(TE_RPC, out->retval), "");
        out->retval = -1;
    }
}
)

/*-------------- pthread_setcanceltype() --------------------------*/
TARPC_FUNC(pthread_setcanceltype, {},
{
    int oldtype;

    MAKE_CALL(out->retval = func(pthread_canceltype_rpc2h(in->type),
                                 &oldtype));

    out->oldtype = pthread_cancelstate_h2rpc(oldtype);
    if (out->retval != 0)
    {
        te_rpc_error_set(TE_OS_RC(TE_RPC, out->retval), "");
        out->retval = -1;
    }
}
)

/*-------------- pthread_join() --------------------------*/
TARPC_FUNC(pthread_join, {},
{
    void *p;
    MAKE_CALL(out->retval = func(in->tid, &p));
    out->ret = (uintptr_t)p;
    if (out->retval != 0)
    {
        te_rpc_error_set(TE_OS_RC(TE_RPC, out->retval), "");
        out->retval = -1;
    }
}
)

/*-------------- access() --------------------------------*/
TARPC_FUNC(access, {},
{
    MAKE_CALL(out->retval = func_ptr(in->path.path_val,
        access_mode_flags_rpc2h(in->mode)));
})


/*-------------- gettimeofday() --------------------------------*/
TARPC_FUNC(gettimeofday,
{
    COPY_ARG_NOTNULL(tv);
    COPY_ARG(tz);
},
{
    struct timeval  tv;
    struct timezone tz;

    TARPC_CHECK_RC(timeval_rpc2h(out->tv.tv_val, &tv));
    if (out->tz.tz_len != 0)
        TARPC_CHECK_RC(timezone_rpc2h(out->tz.tz_val, &tz));

    if (out->common._errno != 0)
    {
        out->retval = -1;
    }
    else
    {
        MAKE_CALL(out->retval = func_ptr(&tv,
                                         out->tz.tz_len == 0 ? NULL : &tz));

        TARPC_CHECK_RC(timeval_h2rpc(&tv, out->tv.tv_val));
        if (out->tz.tz_len != 0)
            TARPC_CHECK_RC(timezone_h2rpc(&tz, out->tz.tz_val));
        if (TE_RC_GET_ERROR(out->common._errno) == TE_EH2RPC)
            out->retval = -1;
    }
}
)

/*-------------- gethostname() --------------------------------*/
TARPC_FUNC(gethostname,
{
    COPY_ARG(name);
},
{
    MAKE_CALL(out->retval = func_ptr(out->name.name_val, in->len));
}
)

#if defined(ENABLE_TELEPHONY)
/*-------------- telephony_check_dial_tone() -----------------------*/

TARPC_FUNC(telephony_open_channel, {},
{
    MAKE_CALL(out->retval = func(in->port));
}
)

/*-------------- telephony_check_dial_tone() -----------------------*/

TARPC_FUNC(telephony_close_channel, {},
{
    MAKE_CALL(out->retval = func(in->chan));
}
)

/*-------------- telephony_pickup() -----------------------*/

TARPC_FUNC(telephony_pickup, {},
{
    MAKE_CALL(out->retval = func(in->chan));
}
)

/*-------------- telephony_hangup() -----------------------*/

TARPC_FUNC(telephony_hangup, {},
{
    MAKE_CALL(out->retval = func(in->chan));
}
)

/*-------------- telephony_check_dial_tone() -----------------------*/

TARPC_FUNC(telephony_check_dial_tone, {},
{
    MAKE_CALL(out->retval = func(in->chan, in->plan));
}
)

/*-------------- telephony_dial_number() -----------------------*/

TARPC_FUNC(telephony_dial_number, {},
{
    MAKE_CALL(out->retval = func(in->chan, in->number));
}
)

/*-------------- telephony_dial_number() -----------------------*/

TARPC_FUNC(telephony_call_wait, {},
{
    MAKE_CALL(out->retval = func(in->chan, in->timeout));
}
)
#endif /* ENABLE_TELEPHONY */


/*-------------- socket() ------------------------------*/

TARPC_FUNC(socket, {},
{
    MAKE_CALL(out->fd = func(domain_rpc2h(in->domain),
                             socktype_rpc2h(in->type),
                             proto_rpc2h(in->proto)));
}
)

/*-------------- dup() --------------------------------*/

TARPC_FUNC(dup, {}, { MAKE_CALL(out->fd = func(in->oldfd)); })

/*-------------- dup2() -------------------------------*/

TARPC_FUNC(dup2, {},
{
    tarpc_close_fd_hooks_call(in->newfd);
    MAKE_CALL(out->fd = func(in->oldfd, in->newfd));
})

/*-------------- dup3() -------------------------------*/

TARPC_FUNC(dup3, {},
{
    tarpc_close_fd_hooks_call(in->newfd);

    MAKE_CALL(out->fd = func(in->oldfd, in->newfd, in->flags));
}
)

/*-------------- close() ------------------------------*/

TARPC_FUNC(close, {},
{
    tarpc_close_fd_hooks_call(in->fd);

    MAKE_CALL(out->retval = func(in->fd));
})

/*-------------- closesocket() ------------------------------*/

int
closesocket(tarpc_closesocket_in *in)
{
    api_func close_func;

    if (tarpc_find_func(in->common.lib_flags, "close", &close_func) != 0)
    {
        ERROR("Failed to find function \"close\"");
        return -1;
    }

    return tarpc_call_close_with_hooks(close_func, in->s);
}

TARPC_FUNC(closesocket, {}, { MAKE_CALL(out->retval = func_ptr(in)); })

/*-------------- bind() ------------------------------*/

TARPC_FUNC(bind, {},
{
    if (in->addr.flags & TARPC_SA_RAW &&
        in->addr.raw.raw_len > sizeof(struct sockaddr_storage))
    {
        MAKE_CALL(out->retval =
                  func(in->fd,
                       (const struct sockaddr *)(in->addr.raw.raw_val),
                       in->addr.raw.raw_len));
    }
    else
    {
        PREPARE_ADDR(my_addr, in->addr, 0);
        MAKE_CALL(out->retval = func(in->fd, my_addr,
                                     in->fwd_len ? in->len :
                                                   my_addrlen));
    }
}
)

/*------------- rpc_check_port_is_free() ----------------*/

te_bool
check_port_is_free(uint16_t port)
{
    return agent_check_l4_port_is_free(0, 0, port);
}

TARPC_FUNC(check_port_is_free, {},
{
    MAKE_CALL(out->retval = func(in->port));
}
)

/*-------------- connect() ------------------------------*/

TARPC_FUNC(connect, {},
{
    if (in->addr.flags & TARPC_SA_RAW &&
        in->addr.raw.raw_len > sizeof(struct sockaddr_storage))
    {
        MAKE_CALL(out->retval =
                  func(in->fd,
                       (const struct sockaddr *)(in->addr.raw.raw_val),
                       in->addr.raw.raw_len));
    }
    else
    {
        PREPARE_ADDR(serv_addr, in->addr, 0);
        MAKE_CALL(out->retval = func(in->fd, serv_addr, serv_addrlen));
    }
}
)

/*-------------- listen() ------------------------------*/

TARPC_FUNC(listen, {},
{
    MAKE_CALL(out->retval = func(in->fd, in->backlog));
}
)

/*-------------- accept() ------------------------------*/

TARPC_FUNC(accept,
{
    COPY_ARG(len);
    COPY_ARG_ADDR(addr);
},
{
    PREPARE_ADDR(addr, out->addr,
                 out->len.len_len == 0 ? 0 : *out->len.len_val);

    MAKE_CALL(out->retval = func(in->fd, addr,
                                 out->len.len_len == 0 ? NULL :
                                 out->len.len_val));

    sockaddr_output_h2rpc(addr, addrlen,
                          out->len.len_len == 0 ? 0 :
                              *(out->len.len_val),
                          &(out->addr));
}
)

/*-------------- accept4() ------------------------------*/

TARPC_FUNC(accept4,
{
    COPY_ARG(len);
    COPY_ARG_ADDR(addr);
},
{
    PREPARE_ADDR(addr, out->addr,
                 out->len.len_len == 0 ? 0 : *out->len.len_val);

    MAKE_CALL(out->retval = func(in->fd, addr,
                                 out->len.len_len == 0 ? NULL :
                                 out->len.len_val, in->flags));

    sockaddr_output_h2rpc(addr, addrlen,
                          out->len.len_len == 0 ? 0 :
                              *(out->len.len_val),
                          &(out->addr));
}
)

/*-------------- socket_connect_close() -----------------------*/
int
socket_connect_close(int domain, const struct sockaddr *addr,
                     socklen_t addrlen, uint32_t time2run)
{
    int     s;
    int     rc;
    time_t  start;
    time_t  now;

    api_func    socket_func;
    api_func    connect_func;
    api_func    close_func;

    if (tarpc_find_func(TARPC_LIB_DEFAULT, "socket", &socket_func) != 0)
        return -1;
    if (tarpc_find_func(TARPC_LIB_DEFAULT, "connect", &connect_func) != 0)
        return -1;
    if (tarpc_find_func(TARPC_LIB_DEFAULT, "close", &close_func) != 0)
        return -1;

    start = now = time(NULL);
    while ((unsigned int)(now - start) <= time2run)
    {
        now = time(NULL);
        s = socket_func(domain, SOCK_STREAM, 0);
        rc = connect_func(s, addr, addrlen);
        if( rc != 0  && errno != ECONNREFUSED && errno != ECONNABORTED )
            return -1;
        tarpc_call_close_with_hooks(close_func, s);
    }
    return 0;
}

TARPC_FUNC(socket_connect_close, {},
{
    PREPARE_ADDR(serv_addr, in->addr, 0);
    MAKE_CALL(out->retval = func_ptr(domain_rpc2h(in->domain), serv_addr,
                                     serv_addrlen, in->time2run));
}
)

/*-------------- socket_listen_close() -----------------------*/
int
socket_listen_close(int domain, const struct sockaddr *addr,
                    socklen_t addrlen, uint32_t time2run)
{
    int     s;
    int     rc;
    time_t  start;
    time_t  now;

    api_func    socket_func;
    api_func    bind_func;
    api_func    listen_func;
    api_func    close_func;

    if (tarpc_find_func(TARPC_LIB_DEFAULT, "socket", &socket_func) != 0)
        return -1;
    if (tarpc_find_func(TARPC_LIB_DEFAULT, "bind", &bind_func) != 0)
        return -1;
    if (tarpc_find_func(TARPC_LIB_DEFAULT, "listen", &listen_func) != 0)
        return -1;
    if (tarpc_find_func(TARPC_LIB_DEFAULT, "close", &close_func) != 0)
        return -1;

    start = now = time(NULL);
    while ((unsigned int)(now - start) <= time2run)
    {
        now = time(NULL);
        s = socket_func(domain, SOCK_STREAM, 0);
        rc = bind_func(s, addr, addrlen);
        if( rc != 0 )
        {
            ERROR("%s(): bind() function failed", __FUNCTION__);
            return -1;
        }
        rc = listen_func(s, 1);
        if( rc != 0 )
        {
            ERROR("%s(): listen() function failed", __FUNCTION__);
            return -1;
        }
        tarpc_call_close_with_hooks(close_func, s);
    }
    return 0;
}

TARPC_FUNC(socket_listen_close, {},
{
    PREPARE_ADDR(serv_addr, in->addr, 0);
    MAKE_CALL(out->retval = func_ptr(domain_rpc2h(in->domain), serv_addr,
                                     serv_addrlen, in->time2run));
}
)

/*-------------- recvfrom() ------------------------------*/

/**
 * Dynamically resolve and call recvfrom() or __recvfrom_chk().
 *
 * @param fd          Socket FD.
 * @param buf         Where to save received data.
 * @param len         Buffer length passed to recvfrom().
 * @param rlen        Actual buffer length (makes sense only for
 *                    __recvfrom_chk()).
 * @param flags       Receive flags (@c MSG_DONTWAIT, etc).
 * @param addr        Where to save source address.
 * @param addrlen     On input, number of bytes available for source
 *                    address; on output, its actual length.
 * @param chk_func    If @c TRUE, use __recvfrom_chk() instead of recvfrom().
 * @param lib_flags   Flags for dynamic function resolution.
 *
 * @return Value returned by the target function or
 *         @c -1 in case of failure.
 */
static int
recvfrom_rpc_handler(int fd, void *buf, size_t len, size_t rlen,
                     int flags, struct sockaddr *addr, socklen_t *addrlen,
                     te_bool chk_func, tarpc_lib_flags lib_flags)
{
    api_func recvfrom_func;
    const char *func_name = (chk_func ? "__recvfrom_chk" : "recvfrom");

    TARPC_FIND_FUNC_RETURN(lib_flags, func_name, &recvfrom_func);

    if (chk_func)
        return recvfrom_func(fd, buf, len, rlen, flags, addr, addrlen);
    else
        return recvfrom_func(fd, buf, len, flags, addr, addrlen);
}

TARPC_FUNC_STANDALONE(recvfrom,
{
    COPY_ARG(buf);
    COPY_ARG(fromlen);
    COPY_ARG_ADDR(from);
},
{
    te_bool          free_name = FALSE;
    struct sockaddr *addr_ptr;
    socklen_t        addr_len;

    PREPARE_ADDR(from, out->from, out->fromlen.fromlen_len == 0 ? 0 :
                                        *out->fromlen.fromlen_val);
    if (out->from.raw.raw_len > sizeof(struct sockaddr_storage))
    {
        /*
         * Do not just assign - sockaddr_output_h2rpc()
         * converts RAW address only if it was changed by the
         * function.
         */
        addr_len = out->from.raw.raw_len;
        if (addr_len > 0 &&
            out->from.raw.raw_val != NULL)
        {
            addr_ptr = calloc(1, addr_len);
            if (addr_ptr == NULL)
            {
                ERROR("%s(): Failed to allocate memory for an address",
                      __FUNCTION__);
                out->common._errno = TE_RC(TE_TA_UNIX, TE_ENOMEM);
                goto finish;
            }
            free_name = TRUE;
            memcpy(addr_ptr, out->from.raw.raw_val, addr_len);
        }
        else
            addr_ptr = (struct sockaddr *)out->from.raw.raw_val;
    }
    else
    {
        addr_ptr = from;
        addr_len = fromlen;
    }

    INIT_CHECKED_ARG(out->buf.buf_val, out->buf.buf_len, in->len);

    MAKE_CALL(out->retval =
                recvfrom_rpc_handler(
                            in->fd, out->buf.buf_val, in->len,
                            out->buf.buf_len,
                            send_recv_flags_rpc2h(in->flags), addr_ptr,
                            (out->fromlen.fromlen_len == 0 ?
                                NULL :
                                out->fromlen.fromlen_val),
                            in->chk_func,
                            in->common.lib_flags));

    sockaddr_output_h2rpc(addr_ptr, addr_len,
                          out->fromlen.fromlen_len == 0 ? 0 :
                              *(out->fromlen.fromlen_val),
                          &(out->from));

finish:
    if (free_name)
        free(addr_ptr);
}
)



/*-------------- recv() ------------------------------*/

/**
 * Dynamically resolve and call recv() or __recv_chk().
 *
 * @param fd          Socket FD.
 * @param buf         Where to save received data.
 * @param len         Buffer length passed to recv().
 * @param rlen        Actual buffer length (makes sense only for
 *                    __recv_chk()).
 * @param flags       Receive flags (@c MSG_DONTWAIT, etc).
 * @param chk_func    If @c TRUE, use __recv_chk() instead of recv().
 * @param lib_flags   Flags for dynamic function resolution.
 *
 * @return Value returned by the target function or
 *         @c -1 in case of failure.
 */
static int
recv_rpc_handler(int fd, void *buf, size_t len, size_t rlen,
                 int flags, te_bool chk_func, tarpc_lib_flags lib_flags)
{
    api_func recv_func;
    const char *func_name = (chk_func ? "__recv_chk" : "recv");

    TARPC_FIND_FUNC_RETURN(lib_flags, func_name, &recv_func);

    if (chk_func)
        return recv_func(fd, buf, len, rlen, flags);
    else
        return recv_func(fd, buf, len, flags);
}

TARPC_FUNC_STANDALONE(recv,
{
    COPY_ARG(buf);
},
{
    INIT_CHECKED_ARG(out->buf.buf_val, out->buf.buf_len, in->len);

    MAKE_CALL(out->retval = recv_rpc_handler(
                                 in->fd, out->buf.buf_val, in->len,
                                 out->buf.buf_len,
                                 send_recv_flags_rpc2h(in->flags),
                                 in->chk_func,
                                 in->common.lib_flags));
}
)

/*-------------- shutdown() ------------------------------*/

TARPC_FUNC(shutdown, {},
{
    MAKE_CALL(out->retval = func(in->fd, shut_how_rpc2h(in->how)));
}
)

/*--------------- fstat() -------------------------------*/



#define FSTAT_COPY(tobuf, outbuf) \
    tobuf->st_dev = outbuf.st_dev;             \
    tobuf->st_ino = outbuf.st_ino;             \
    tobuf->st_mode = outbuf.st_mode;           \
    tobuf->st_nlink = outbuf.st_nlink;         \
    tobuf->st_uid = outbuf.st_uid;             \
    tobuf->st_gid = outbuf.st_gid;             \
    tobuf->st_rdev = outbuf.st_rdev;           \
    tobuf->st_size = outbuf.st_size;           \
    tobuf->st_blksize = outbuf.st_blksize;     \
    tobuf->st_blocks = outbuf.st_blocks;       \
    tobuf->ifsock = S_ISSOCK(outbuf.st_mode);  \
    tobuf->iflnk = S_ISLNK(outbuf.st_mode);    \
    tobuf->ifreg = S_ISREG(outbuf.st_mode);    \
    tobuf->ifblk = S_ISBLK(outbuf.st_mode);    \
    tobuf->ifdir = S_ISDIR(outbuf.st_mode);    \
    tobuf->ifchr = S_ISCHR(outbuf.st_mode);    \
    tobuf->ififo = S_ISFIFO(outbuf.st_mode);   \
    tobuf->te_atime = outbuf.st_atime;         \
    tobuf->te_ctime = outbuf.st_ctime;         \
    tobuf->te_mtime = outbuf.st_mtime

int
te_fstat(tarpc_lib_flags lib_flags, int fd, rpc_stat *rpcbuf)
{
#if defined (__QNX__) || defined (__ANDROID__)
    api_func    stat_func;
    int         rc;
    struct stat buf;

    memset(&buf, 0, sizeof(buf));

    if ((rc = fstat(fd, &buf)) < 0)
        return rc;

    rpcbuf->te_atime = buf.st_atime;
    rpcbuf->te_ctime = buf.st_ctime;
    rpcbuf->te_mtime = buf.st_mtime;

    return 0;
#elif defined __linux__
    api_func    stat_func;
    int         rc;
    struct stat buf;

    memset(&buf, 0, sizeof(buf));

#ifdef _STAT_VER
    /*
     * In libc before version 2.33 fstat() cannot be resolved
     * dynamically, so this hack is needed.
     */
    if (tarpc_find_func(lib_flags, "__fxstat", &stat_func) != 0)
    {
        ERROR("Failed to find __fxstat function");
        return -1;
    }

    rc = stat_func(_STAT_VER, fd, &buf);
    if (rc < 0)
        return rc;
#else
    /* Since libc 2.33 fstat() can be resolved dynamically. */
    if (tarpc_find_func(lib_flags, "fstat", &stat_func) != 0)
    {
        ERROR("Failed to find fstat() function");
        return -1;
    }

    rc = stat_func(fd, &buf);
    if (rc < 0)
        return rc;
#endif

    FSTAT_COPY(rpcbuf, buf);
    return 0;
#else
    UNUSED(lib_flags);
    UNUSED(rpcbuf);

/*
 * #error "fstat family is not currently supported for non-linux unixes."
*/
    errno = EOPNOTSUPP;
    return -1;
#endif
}

int
te_fstat64(tarpc_lib_flags lib_flags, int fd, rpc_stat *rpcbuf)
{
/**
 * To have __USE_LARGEFILE64 defined in Linux, specify
 * -D_GNU_SOURCE (or other related feature test macro) in
 * TE_PLATFORM macro in your builder.conf
 */
#if defined __linux__ && defined __USE_LARGEFILE64
    api_func      stat_func;
    int           rc;
    struct stat64 buf;

    memset(&buf, 0, sizeof(buf));

#ifdef _STAT_VER
    /*
     * In libc before version 2.33 fstat64() cannot be resolved
     * dynamically, so this hack is needed.
     */
    if (tarpc_find_func(lib_flags, "__fxstat64", &stat_func) != 0)
    {
        ERROR("Failed to find __fxstat64 function");
        return -1;
    }

    rc = stat_func(_STAT_VER, fd, &buf);
    if (rc < 0)
        return rc;
#else
    /* Since libc 2.33 fstat64() can be resolved dynamically. */
    if (tarpc_find_func(lib_flags, "fstat64", &stat_func) != 0)
    {
        ERROR("Failed to find fstat64() function");
        return -1;
    }

    rc = stat_func(fd, &buf);
    if (rc < 0)
        return rc;
#endif

    FSTAT_COPY(rpcbuf, buf);
    return 0;
#else
/*
 * #error "fstat family is not currently supported for non-linux unixes."
 */
    UNUSED(lib_flags);
    UNUSED(fd);
    UNUSED(rpcbuf);

    ERROR("fstat64 is not supported");
    return -1;
#endif
}

TARPC_FUNC(te_fstat, {},
{
    MAKE_CALL(out->retval = func(in->common.lib_flags, in->fd, &out->buf));
}
)

TARPC_FUNC(te_fstat64, {},
{
    MAKE_CALL(out->retval = func(in->common.lib_flags, in->fd, &out->buf));
}
)

#undef FSTAT_COPY

#ifndef TE_POSIX_FS_PROVIDED
/*-------------- link() --------------------------------*/
TARPC_FUNC(link, {},
{
    TARPC_ENSURE_NOT_NULL(path1);
    TARPC_ENSURE_NOT_NULL(path2);
    MAKE_CALL(out->retval = func_ptr(in->path1.path1_val,
                                     in->path2.path2_val));
}
)

/*-------------- symlink() --------------------------------*/
TARPC_FUNC(symlink, {},
{
    TARPC_ENSURE_NOT_NULL(path1);
    TARPC_ENSURE_NOT_NULL(path2);
    MAKE_CALL(out->retval = func_ptr(in->path1.path1_val,
                                     in->path2.path2_val));
}
)

/*-------------- unlink() --------------------------------*/
TARPC_FUNC(unlink, {},
{
    TARPC_ENSURE_NOT_NULL(path);
    MAKE_CALL(out->retval = func_ptr(in->path.path_val));
}
)

/*-------------- rename() --------------------------------*/
TARPC_FUNC(rename, {},
{
    TARPC_ENSURE_NOT_NULL(path_old);
    TARPC_ENSURE_NOT_NULL(path_new);
    MAKE_CALL(out->retval = func_ptr(in->path_old.path_old_val,
                                     in->path_new.path_new_val));
}
)

/*-------------- mkdir() --------------------------------*/
TARPC_FUNC(mkdir, {},
{
    TARPC_ENSURE_NOT_NULL(path);
    MAKE_CALL(out->retval = func_ptr(in->path.path_val,
                                     file_mode_flags_rpc2h(in->mode)));
}
)

/*-------------- mkdirp() --------------------------------*/
TARPC_FUNC(mkdirp, {},
{
    te_errno rc;

    TARPC_ENSURE_NOT_NULL(path);
    MAKE_CALL(rc = func_ptr(in->path.path_val,
                            file_mode_flags_rpc2h(in->mode)));
    if (rc != 0)
        out->common._errno = TE_RC(TE_RPC, TE_RC_GET_ERROR(rc));
    out->retval = (rc == 0 ? 0 : -1);
}
)

/*-------------- rmdir() --------------------------------*/
TARPC_FUNC(rmdir, {},
{
    TARPC_ENSURE_NOT_NULL(path);
    MAKE_CALL(out->retval = func_ptr(in->path.path_val));
}
)

#ifdef HAVE_SYS_STATVFS_H
/*-------------- fstatvfs()-----------------------------*/
TARPC_FUNC(fstatvfs, {},
{
    struct statvfs stat;

    MAKE_CALL(out->retval = func(in->fd, &stat));

    out->buf.f_bsize = stat.f_bsize;
    out->buf.f_blocks = stat.f_blocks;
    out->buf.f_bfree = stat.f_bfree;
}
)

/*-------------- statvfs()-----------------------------*/
TARPC_FUNC(statvfs, {},
{
    struct statvfs stat;

    TARPC_ENSURE_NOT_NULL(path);
    MAKE_CALL(out->retval = func_ptr(in->path.path_val, &stat));

    out->buf.f_bsize = stat.f_bsize;
    out->buf.f_blocks = stat.f_blocks;
    out->buf.f_bfree = stat.f_bfree;
}
)
#endif /* HAVE_SYS_STATVFS_H */

#ifdef HAVE_DIRENT_H
/* struct_dirent_props */
unsigned int
struct_dirent_props(void)
{
    unsigned int props = 0;

#ifdef HAVE_STRUCT_DIRENT_D_TYPE
    props |= RPC_DIRENT_HAVE_D_TYPE;
#endif
#if defined HAVE_STRUCT_DIRENT_D_OFF || defined HAVE_STRUCT_DIRENT_D_OFFSET
    props |= RPC_DIRENT_HAVE_D_OFF;
#endif
#ifdef HAVE_STRUCT_DIRENT_D_NAMELEN
    props |= RPC_DIRENT_HAVE_D_NAMLEN;
#endif
    props |= RPC_DIRENT_HAVE_D_INO;
    return props;
}

TARPC_FUNC(struct_dirent_props, {},
{
    MAKE_CALL(out->retval = func_void());
}
)
#endif /* HAVE_DIRENT_H */

/*-------------- opendir() --------------------------------*/
TARPC_FUNC(opendir, {},
{
    TARPC_ENSURE_NOT_NULL(path);
    MAKE_CALL(out->mem_ptr =
        rcf_pch_mem_alloc(func_ptr_ret_ptr(in->path.path_val)));
}
)

/*-------------- closedir() --------------------------------*/
TARPC_FUNC(closedir, {},
{
    MAKE_CALL(out->retval = func_ptr(rcf_pch_mem_get(in->mem_ptr)));
    rcf_pch_mem_free(in->mem_ptr);
}
)

/*-------------- readdir() --------------------------------*/
TARPC_FUNC(readdir, {},
{
    struct dirent *dent;

    MAKE_CALL(dent = (struct dirent *)
                        func_ptr(rcf_pch_mem_get(in->mem_ptr)));
    if (dent == NULL)
    {
        out->ret_null = TRUE;
    }
    else
    {
        tarpc_dirent *rpc_dent;

        rpc_dent = (tarpc_dirent *)calloc(1, sizeof(*rpc_dent));
        if (rpc_dent == NULL)
            out->common._errno = TE_RC(TE_TA_UNIX, TE_ENOMEM);
        else
        {
            out->dent.dent_len = 1;

            out->ret_null = FALSE;
            out->dent.dent_val = rpc_dent;

            rpc_dent->d_name.d_name_val = strdup(dent->d_name);
            rpc_dent->d_name.d_name_len = strlen(dent->d_name) + 1;
            rpc_dent->d_ino = dent->d_ino;
#ifdef HAVE_STRUCT_DIRENT_D_OFF
            rpc_dent->d_off = dent->d_off;
#elif defined HAVE_STRUCT_DIRENT_D_OFFSET
            rpc_dent->d_off = dent->d_offset;
#else
            rpc_dent->d_off = 0;
#endif

#ifdef HAVE_STRUCT_DIRENT_D_TYPE
            rpc_dent->d_type = d_type_h2rpc(dent->d_type);
#else
            rpc_dent->d_type = RPC_DT_UNKNOWN;
#endif
#ifdef HAVE_STRUCT_DIRENT_D_NAMELEN
            rpc_dent->d_namelen = dent->d_namelen;
#else
            rpc_dent->d_namelen = 0;
#endif
            rpc_dent->d_props = struct_dirent_props();
        }
    }
}
)
#endif

/*-------------- sendto() ------------------------------*/

TARPC_FUNC(sendto, {},
{
    PREPARE_ADDR(to, in->to, 0);

    INIT_CHECKED_ARG(in->buf.buf_val, in->buf.buf_len, 0);

    if (!(in->to.flags & TARPC_SA_RAW &&
          in->to.raw.raw_len > sizeof(struct sockaddr_storage)))
    {
        MAKE_CALL(out->retval = func(in->fd, in->buf.buf_val, in->len,
                                     send_recv_flags_rpc2h(in->flags),
                                     to, tolen));
    }
    else
    {
        MAKE_CALL(out->retval = func(in->fd, in->buf.buf_val, in->len,
                                     send_recv_flags_rpc2h(in->flags),
                                     (const struct sockaddr *)
                                                (in->to.raw.raw_val),
                                     in->to.raw.raw_len));
    }
}
)

/*-------------- send() ------------------------------*/

TARPC_FUNC(send, {},
{
    INIT_CHECKED_ARG(in->buf.buf_val, in->buf.buf_len, 0);

    MAKE_CALL(out->retval = func(in->fd, in->buf.buf_val, in->len,
                                 send_recv_flags_rpc2h(in->flags)));
}
)

/*-------------- read() ------------------------------*/

TARPC_FUNC(read,
{
    COPY_ARG(buf);
},
{
    INIT_CHECKED_ARG(out->buf.buf_val, out->buf.buf_len, in->len);

    MAKE_CALL(out->retval = func(in->fd, out->buf.buf_val, in->len));
}
)

/*-------------- read_via_splice() ------------------------------*/

tarpc_ssize_t
read_via_splice(tarpc_read_via_splice_in *in,
                tarpc_read_via_splice_out *out)
{
    api_func_ptr    pipe_func;
    api_func        splice_func;
    api_func        close_func;
    api_func        read_func;
    ssize_t         to_pipe;
    ssize_t         from_pipe = 0;
    int             pipefd[2];
    unsigned int    flags = 0;
    int             ret = 0;

#ifdef SPLICE_F_NONBLOCK
    flags = SPLICE_F_MOVE;
#endif

    if (tarpc_find_func(in->common.lib_flags, "pipe",
                        (api_func *)&pipe_func) != 0)
    {
        ERROR("%s(): Failed to resolve pipe() function", __FUNCTION__);
        return -1;
    }
    if (tarpc_find_func(in->common.lib_flags, "splice", &splice_func) != 0)
    {
        ERROR("%s(): Failed to resolve splice() function", __FUNCTION__);
        return -1;
    }
    if (tarpc_find_func(in->common.lib_flags, "close", &close_func) != 0)
    {
        ERROR("%s(): Failed to resolve close() function", __FUNCTION__);
        return -1;
    }
    if (tarpc_find_func(in->common.lib_flags, "read", &read_func) != 0)
    {
        ERROR("%s(): Failed to resolve read() function", __FUNCTION__);
        return -1;
    }

    if (pipe_func(pipefd) != 0)
    {
        ERROR("pipe() failed with error %r", TE_OS_RC(TE_TA_UNIX, errno));
        return -1;
    }
    if (in->fd == pipefd[0] || in->fd == pipefd[1])
    {
        ERROR("Aux pipe fd and in fd is the same",
              TE_OS_RC(TE_TA_UNIX, EFAULT));
        errno = EFAULT;
        ret = -1;
        goto read_via_splice_exit;
    }

    if ((to_pipe = splice_func(in->fd, NULL,
                               pipefd[1], NULL, in->len, flags)) < 0)
    {
        ERROR("splice() to pipe failed with error %r",
              TE_OS_RC(TE_TA_UNIX, errno));
        ret = -1;
        goto read_via_splice_exit;
    }
    if ((from_pipe = read_func(pipefd[0], out->buf.buf_val, in->len)) < 0)
    {
        ERROR("read() from pipe failed with error %r",
              TE_OS_RC(TE_TA_UNIX, errno));
        ret = -1;
        goto read_via_splice_exit;
    }
    if (to_pipe != from_pipe)
    {
        ERROR("read() and splice() calls return different amount of data",
              TE_OS_RC(TE_TA_UNIX, EMSGSIZE));
        errno = EMSGSIZE;
        ret = -1;
    }
read_via_splice_exit:
    if (tarpc_call_close_with_hooks(close_func, pipefd[0]) < 0 ||
        tarpc_call_close_with_hooks(close_func, pipefd[1]) < 0)
        ret = -1;
    return ret == -1 ? ret : from_pipe;
}

TARPC_FUNC(read_via_splice,
{
    COPY_ARG(buf);
},
{
    MAKE_CALL(out->retval = func_ptr(in, out));
}
)

/*-------------- write() ------------------------------*/

TARPC_FUNC(write, {},
{
    INIT_CHECKED_ARG(in->buf.buf_val, in->buf.buf_len, 0);

    MAKE_CALL(out->retval = func(in->fd, in->buf.buf_val, in->len));
}
)

/*-------------- write_via_splice() ------------------------------*/

tarpc_ssize_t
write_via_splice(tarpc_write_via_splice_in *in)
{
    api_func_ptr    pipe_func;
    api_func        splice_func;
    api_func        close_func;
    api_func        write_func;
    ssize_t         to_pipe;
    ssize_t         from_pipe = 0;
    int             pipefd[2];
    unsigned int    flags = 0;
    int             ret = 0;

#ifdef SPLICE_F_NONBLOCK
    flags = SPLICE_F_MOVE;
#endif

    if (tarpc_find_func(in->common.lib_flags, "pipe",
                        (api_func *)&pipe_func) != 0)
    {
        ERROR("%s(): Failed to resolve pipe() function", __FUNCTION__);
        return -1;
    }
    if (tarpc_find_func(in->common.lib_flags, "splice", &splice_func) != 0)
    {
        ERROR("%s(): Failed to resolve splice() function", __FUNCTION__);
        return -1;
    }
    if (tarpc_find_func(in->common.lib_flags, "close", &close_func) != 0)
    {
        ERROR("%s(): Failed to resolve close() function", __FUNCTION__);
        return -1;
    }
    if (tarpc_find_func(in->common.lib_flags, "write", &write_func) != 0)
    {
        ERROR("%s(): Failed to resolve write() function", __FUNCTION__);
        return -1;
    }

    if (pipe_func(pipefd) != 0)
    {
        ERROR("pipe() failed with error %r", TE_OS_RC(TE_TA_UNIX, errno));
        return -1;
    }
    if (in->fd == pipefd[0] || in->fd == pipefd[1])
    {
        ERROR("Aux pipe fd and in fd is the same",
              TE_OS_RC(TE_TA_UNIX, EFAULT));
        errno = EFAULT;
        ret = -1;
        goto write_via_splice_exit;
    }

    if ((to_pipe = write_func(pipefd[1], in->buf.buf_val, in->len)) < 0)
    {
        ERROR("write() to pipe failed with error %r",
              TE_OS_RC(TE_TA_UNIX, errno));
        ret = -1;
        goto write_via_splice_exit;
    }
    if ((from_pipe = splice_func(pipefd[0], NULL, in->fd, NULL,
                                 in->len, flags)) < 0)
    {
        ERROR("splice() from pipe failed with error %r",
              TE_OS_RC(TE_TA_UNIX, errno));
        ret = -1;
        goto write_via_splice_exit;
    }
    if (to_pipe != from_pipe)
    {
        ERROR("write() and splice() calls return different amount of data",
              TE_OS_RC(TE_TA_UNIX, EMSGSIZE));
        errno = EMSGSIZE;
        ret = -1;
    }
write_via_splice_exit:
    if (tarpc_call_close_with_hooks(close_func, pipefd[0]) < 0 ||
        tarpc_call_close_with_hooks(close_func, pipefd[1]) < 0)
        ret = -1;
    return ret == -1 ? ret : from_pipe;
}

TARPC_FUNC(write_via_splice, {},
{
    MAKE_CALL(out->retval = func_ptr(in));
}
)

/*------------ write_and_close() ----------------------*/
bool_t
_write_and_close_1_svc(tarpc_write_and_close_in *in,
                       tarpc_write_and_close_out *out,
                       struct svc_req *rqstp)
{
    api_func write_func;
    api_func close_func;
    int      rc = 0;

    UNUSED(rqstp);
    memset(out, 0, sizeof(*out));

    if (tarpc_find_func(in->common.lib_flags, "write", &write_func) != 0)
    {
        ERROR("Failed to find function \"write\"");
        out->retval =  -1;
    }
    else if (tarpc_find_func(in->common.lib_flags, "close",
                             &close_func) != 0)
    {
        ERROR("Failed to find function \"close\"");
        out->retval =  -1;
    }
    else
    {
        out->retval = write_func(in->fd, in->buf.buf_val, in->len);

        if (out->retval >= 0)
        {
            rc = tarpc_call_close_with_hooks(close_func, in->fd);
            if (rc < 0)
                out->retval = rc;
        }
    }

    return TRUE;
}

/*-------------- readbuf() ------------------------------*/
ssize_t
readbuf(tarpc_readbuf_in *in)
{
    api_func read_func;

    if (tarpc_find_func(in->common.lib_flags, "read", &read_func) != 0)
    {
        ERROR("Failed to find function \"read\"");
        return -1;
    }

    return read_func(in->fd, rcf_pch_mem_get(in->buf) + in->off,
                     in->len);
}

TARPC_FUNC(readbuf, {},
{
    MAKE_CALL(out->retval = func_ptr(in));
}
)

/*-------------- recvbuf() ------------------------------*/
ssize_t
recvbuf(tarpc_recvbuf_in *in)
{
    api_func recv_func;

    if (tarpc_find_func(in->common.lib_flags, "recv", &recv_func) != 0)
    {
        ERROR("Failed to find function \"recv\"");
        return -1;
    }

    return recv_func(in->fd, rcf_pch_mem_get(in->buf) + in->off,
                     in->len, in->flags);
}

TARPC_FUNC(recvbuf, {},
{
    MAKE_CALL(out->retval = func_ptr(in));
}
)

/*-------------- writebuf() ------------------------------*/
ssize_t
writebuf(tarpc_writebuf_in *in)
{
    api_func write_func;

    if (tarpc_find_func(in->common.lib_flags, "write", &write_func) != 0)
    {
        ERROR("Failed to find function \"write\"");
        return -1;
    }
    return write_func(in->fd,
                      rcf_pch_mem_get(in->buf) + in->off,
                      in->len);
}

TARPC_FUNC(writebuf, {},
{
    MAKE_CALL(out->retval = func_ptr(in));
}
)

/*-------------- sendbuf() ------------------------------*/
ssize_t
sendbuf(tarpc_sendbuf_in *in)
{
    api_func send_func;

    if (tarpc_find_func(in->common.lib_flags, "send", &send_func) != 0)
    {
        ERROR("Failed to find function \"send\"");
        return -1;
    }
    return send_func(in->fd, rcf_pch_mem_get(in->buf) + in->off,
                     in->len, send_recv_flags_rpc2h(in->flags));
}

TARPC_FUNC(sendbuf, {},
{
    MAKE_CALL(out->retval = func_ptr(in));
}
)

/*------------ send_msg_more() --------------------------*/

/**
 * Find pointer to a send function.
 *
 * @param lib_flags     How to resolve function name.
 * @param send_func     Send function type.
 * @param func          Pointer to the function.
 *
 * @return Status code.
 */
static te_errno
tarpc_get_send_function(tarpc_lib_flags lib_flags, tarpc_send_function send_func,
                        api_func *func)
{
    int rc;

    switch (send_func)
    {
        case TARPC_SEND_FUNC_WRITE:
            rc = tarpc_find_func(lib_flags, "write",
                                 (api_func *)func);
            break;

        case TARPC_SEND_FUNC_WRITEV:
            rc = tarpc_find_func(lib_flags, "writev", (api_func *)func);
            break;

        case TARPC_SEND_FUNC_SEND:
            rc = tarpc_find_func(lib_flags, "send", (api_func *)func);
            break;

        case TARPC_SEND_FUNC_SENDTO:
            rc = tarpc_find_func(lib_flags, "sendto", (api_func *)func);
            break;

        case TARPC_SEND_FUNC_SENDMSG:
            rc = tarpc_find_func(lib_flags, "sendmsg", (api_func *)func);
            break;

        case TARPC_SEND_FUNC_SENDMMSG:
            rc = tarpc_find_func(lib_flags, "sendmmsg", (api_func *)func);
            break;

        default:
            ERROR("Invalid send function index: %d", send_func);
            return  TE_RC(TE_TA_UNIX, EINVAL);
    }

    return rc;
}

/**
 * Call a sending function which accepts flags.
 *
 * @param s         Socket FD.
 * @param buf       Buffer with data to send.
 * @param len       How many bytes to send.
 * @param flags     Flags to pass to the function.
 * @param func      Which function to use.
 * @param func_ptr  Pointer to the sending function.
 *
 * @return Value returned by the called function on success,
 *         @c -1 on failure.
 */
ssize_t
send_buf_with_flags(int s, uint8_t *buf, size_t len, int flags,
                    tarpc_send_function func, api_func func_ptr)
{
    struct mmsghdr mmsg;
    struct iovec iov;
    ssize_t rc;

    switch (func)
    {
        case TARPC_SEND_FUNC_SEND:
            return func_ptr(s, buf, len, flags);

        case TARPC_SEND_FUNC_SENDTO:
            return func_ptr(s, buf, len, flags, NULL, 0);

        case TARPC_SEND_FUNC_SENDMSG:
        case TARPC_SEND_FUNC_SENDMMSG:

            memset(&mmsg, 0, sizeof(mmsg));
            mmsg.msg_hdr.msg_iov = &iov;
            mmsg.msg_hdr.msg_iovlen = 1;
            iov.iov_base = buf;
            iov.iov_len = len;

            if (func == TARPC_SEND_FUNC_SENDMSG)
            {
                return func_ptr(s, &mmsg.msg_hdr, flags);
            }
            else
            {
                rc = func_ptr(s, &mmsg, 1, flags);
                if (rc > 1)
                {
                    te_rpc_error_set(TE_RC(TE_TA_UNIX, TE_EFAIL),
                                     "sendmmsg() returned too big number");
                    return -1;
                }
                else if (rc > 0)
                {
                    rc = mmsg.msg_len;
                }

                return rc;
            }

            break;

        default:

            te_rpc_error_set(TE_RC(TE_TA_UNIX, TE_EINVAL),
                             "function %d is not supported", func);
            return -1;
    }

    /* It should never get to here */
    assert(0);
    return -1;
}

ssize_t
send_msg_more(tarpc_send_msg_more_in *in)
{
    ssize_t res1, res2;
    te_errno rc;
    int res;
    uint8_t *buf;

    api_func send_func1;
    api_func send_func2;
    api_func setsockopt_func;

    rc = tarpc_get_send_function(in->common.lib_flags, in->first_func,
                                 &send_func1);
    if (rc != 0)
    {
        te_rpc_error_set(TE_RC(TE_TA_UNIX, rc),
                         "failed to resolve the first function");
        return -1;
    }

    rc = tarpc_get_send_function(in->common.lib_flags, in->second_func,
                                 &send_func2);
    if (rc != 0)
    {
        te_rpc_error_set(TE_RC(TE_TA_UNIX, rc),
                         "failed to resolve the second function");
        return -1;
    }

    if (in->set_nodelay)
    {
        rc = tarpc_find_func(in->common.lib_flags, "setsockopt",
                             &setsockopt_func);
        if (rc != 0)
        {
            te_rpc_error_set(TE_RC(TE_TA_UNIX, rc),
                             "failed to resolve setsockopt()");
            return -1;
        }
    }

    buf = rcf_pch_mem_get(in->buf);
    if (buf == NULL)
    {
        te_rpc_error_set(TE_RC(TE_TA_UNIX, TE_EINVAL),
                         "passed buffer is NULL");
        return -1;
    }

    res1 = send_buf_with_flags(in->fd, buf, in->first_len, MSG_MORE,
                               in->first_func, send_func1);
    if (res1 < 0)
        return res1;

    if (in->set_nodelay)
    {
        int optval = 1;
        socklen_t optlen = sizeof(optval);

        res = setsockopt_func(in->fd, IPPROTO_TCP, TCP_NODELAY, &optval,
                              optlen);
        if (res < 0)
        {
            te_rpc_error_set(TE_OS_RC(TE_TA_UNIX, errno),
                             "setsockopt() failed to enable TCP_NODELAY "
                             "option");
            return -1;
        }
    }

    res2 = send_buf_with_flags(in->fd, buf + in->first_len, in->second_len,
                               0, in->second_func, send_func2);
    if (res2 < 0)
        return res2;

    return res1 + res2;
}

TARPC_FUNC(send_msg_more, {},
{
    MAKE_CALL(out->retval = func_ptr(in));
}
)

/*------------ send_one_byte_many() --------------------------*/
ssize_t
send_one_byte_many(tarpc_send_one_byte_many_in *in)
{
    int      sent = 0;
    int      rc;
    char     buf = 'A';
    api_func send_func;

    struct timeval t, lim;

    if (tarpc_find_func(in->common.lib_flags, "send", &send_func) != 0)
    {
        ERROR("Failed to find function \"send\"");
        return -1;
    }

    gettimeofday(&lim, NULL);
    lim.tv_sec += in->duration;

    do {
        rc = send_func(in->fd, &buf, 1, MSG_DONTWAIT);
        if (rc < 0)
        {
          if (errno != EAGAIN)
              return sent;
          rc = 0;
        }
        sent += rc;

        gettimeofday(&t, NULL);
    } while (TIMEVAL_SUB(lim, t) > 0);

    return sent;
}

TARPC_FUNC(send_one_byte_many, {},
{
    MAKE_CALL(out->retval = func_ptr(in));
}
)

/*-------------- readv() ------------------------------*/

TARPC_FUNC(readv,
{
    if (out->vector.vector_len > RCF_RPC_MAX_IOVEC)
    {
        ERROR("Too long iovec is provided");
        out->common._errno = TE_RC(TE_TA_UNIX, TE_ENOMEM);
        return TRUE;
    }
    COPY_ARG(vector);
},
{
    struct iovec    iovec_arr[RCF_RPC_MAX_IOVEC];
    struct iovec    *res = NULL;

    if (out->vector.vector_val != NULL)
    {
        rpcs_iovec_tarpc2h(out->vector.vector_val, iovec_arr,
                           out->vector.vector_len, TRUE,
                           arglist);
        res = iovec_arr;
    }

    MAKE_CALL(out->retval = func(in->fd, res, in->count));

}
)

/*-------------- writev() ------------------------------*/

TARPC_FUNC(writev,
{
    if (in->vector.vector_len > RCF_RPC_MAX_IOVEC)
    {
        ERROR("Too long iovec is provided");
        out->common._errno = TE_RC(TE_TA_UNIX, TE_ENOMEM);
        return TRUE;
    }
},
{
    struct iovec    iovec_arr[RCF_RPC_MAX_IOVEC];
    struct iovec    *res = NULL;

    if (in->vector.vector_val != NULL)
    {
        rpcs_iovec_tarpc2h(in->vector.vector_val, iovec_arr,
                           in->vector.vector_len, FALSE,
                           arglist);
        res = iovec_arr;
    }

    MAKE_CALL(out->retval = func(in->fd, res, in->count));
}
)

#ifndef TE_POSIX_FS_PROVIDED
/*-------------- lseek() ------------------------------*/

TARPC_FUNC(lseek, {},
{
    if (sizeof(off_t) == 4)
    {
        if (in->pos > UINT_MAX)
        {
            ERROR("'offset' value passed to lseek "
                  "exceeds 'off_t' data type range");
            out->common._errno = TE_RC(TE_TA_UNIX, TE_EINVAL);
        }
        else
        {
            MAKE_CALL(out->retval = func(in->fd, (off_t)in->pos,
                                         lseek_mode_rpc2h(in->mode)));
        }
    }
    else if (sizeof(off_t) == 8)
    {
        MAKE_CALL(out->retval = func_ret_int64(in->fd, in->pos,
                                               lseek_mode_rpc2h(in->mode)));
    }
    else
    {
        ERROR("Unexpected size of 'off_t' for lseek call");
        out->common._errno = TE_RC(TE_TA_UNIX, TE_EINVAL);
    }
})
#endif

/*-------------- fsync() ------------------------------*/

TARPC_FUNC(fsync, {},
{
    MAKE_CALL(out->retval = func(in->fd));
})


/*-------------- getsockname() ------------------------------*/
TARPC_FUNC(getsockname,
{
    COPY_ARG(len);
    COPY_ARG_ADDR(addr);
},
{
    PREPARE_ADDR(name, out->addr,
                 out->len.len_len == 0 ? 0 : *out->len.len_val);

    MAKE_CALL(out->retval = func(in->fd, name,
                                 out->len.len_len == 0 ? NULL :
                                 out->len.len_val));

    sockaddr_output_h2rpc(name, namelen,
                          out->len.len_len == 0 ? 0 :
                              *(out->len.len_val),
                          &(out->addr));
}
)

/*-------------- getpeername() ------------------------------*/

TARPC_FUNC(getpeername,
{
    COPY_ARG(len);
    COPY_ARG_ADDR(addr);
},
{
    PREPARE_ADDR(name, out->addr,
                 out->len.len_len == 0 ? 0 : *out->len.len_val);

    MAKE_CALL(out->retval = func(in->fd, name,
                                 out->len.len_len == 0 ? NULL :
                                 out->len.len_val));

    sockaddr_output_h2rpc(name, namelen,
                          out->len.len_len == 0 ? 0 :
                              *(out->len.len_val),
                          &(out->addr));
}
)

/*-------------- fd_set constructor ----------------------------*/

void
fd_set_new(tarpc_fd_set_new_out *out)
{
    RPC_PCH_MEM_WITH_NAMESPACE(ns, RPC_TYPE_NS_FD_SET, {
        fd_set *ptr = calloc(1, sizeof(fd_set));
        out->retval = RCF_PCH_MEM_INDEX_ALLOC(ptr, ns);
    });
}

TARPC_FUNC_STATIC(fd_set_new, {},
{
    MAKE_CALL(func(out));
})

/*-------------- fd_set destructor ----------------------------*/

te_errno
fd_set_delete(tarpc_fd_set_delete_in *in,
              tarpc_fd_set_delete_out *out)
{
    RPC_PCH_MEM_WITH_NAMESPACE(ns, RPC_TYPE_NS_FD_SET, {
        free(IN_FDSET_NS(ns));
        return RCF_PCH_MEM_INDEX_FREE(in->set, ns);
    });
    return out->common._errno;
}

TARPC_FUNC_STATIC(fd_set_delete, {},
{
    te_errno rc;
    MAKE_CALL(rc = func(in, out));
    if (out->common._errno == 0)
        out->common._errno = rc;
})

/*-------------- FD_ZERO --------------------------------*/

void
do_fd_zero(tarpc_do_fd_zero_in *in)
{
    RPC_PCH_MEM_WITH_NAMESPACE(ns, RPC_TYPE_NS_FD_SET, {
        FD_ZERO(IN_FDSET_NS(ns));
    });
}

TARPC_FUNC_STATIC(do_fd_zero, {},
{
    MAKE_CALL(func(in));
})

/*-------------- FD_SET --------------------------------*/

void
do_fd_set(tarpc_do_fd_set_in *in)
{
    RPC_PCH_MEM_WITH_NAMESPACE(ns, RPC_TYPE_NS_FD_SET, {
        FD_SET(in->fd, IN_FDSET_NS(ns));
    });
}

TARPC_FUNC_STATIC(do_fd_set, {},
{
    MAKE_CALL(func(in));
})

/*-------------- FD_CLR --------------------------------*/

void
do_fd_clr(tarpc_do_fd_clr_in *in)
{
    RPC_PCH_MEM_WITH_NAMESPACE(ns, RPC_TYPE_NS_FD_SET, {
        FD_CLR(in->fd, IN_FDSET_NS(ns));
    });
}

TARPC_FUNC_STATIC(do_fd_clr, {},
{
    MAKE_CALL(func(in));
})

/*-------------- FD_ISSET --------------------------------*/

void
do_fd_isset(tarpc_do_fd_isset_in *in,
            tarpc_do_fd_isset_out *out)
{
    RPC_PCH_MEM_WITH_NAMESPACE(ns, RPC_TYPE_NS_FD_SET, {
        out->retval = FD_ISSET(in->fd, IN_FDSET_NS(ns));
    });
}

TARPC_FUNC_STATIC(do_fd_isset, {},
{
    MAKE_CALL(func(in, out));
})

/*-------------- select() --------------------------------*/

TARPC_FUNC(select,
{
    COPY_ARG(timeout);
},
{
    fd_set *rfds;
    fd_set *wfds;
    fd_set *efds;
    struct timeval tv;
    static rpc_ptr_id_namespace ns = RPC_PTR_ID_NS_INVALID;

    out->retval = -1;
    RCF_PCH_MEM_NS_CREATE_IF_NEEDED_RETURN(&ns, RPC_TYPE_NS_FD_SET, );

    if (out->timeout.timeout_len > 0)
        TARPC_CHECK_RC(timeval_rpc2h(out->timeout.timeout_val, &tv));

    if (out->common._errno != 0)
        return;

    RCF_PCH_MEM_INDEX_TO_PTR_RPC(rfds, in->readfds, ns, );
    RCF_PCH_MEM_INDEX_TO_PTR_RPC(wfds, in->writefds, ns, );
    RCF_PCH_MEM_INDEX_TO_PTR_RPC(efds, in->exceptfds, ns, );

    MAKE_CALL(out->retval = func(in->n, rfds, wfds, efds,
                    out->timeout.timeout_len == 0 ? NULL : &tv));

    if (out->timeout.timeout_len > 0)
        TARPC_CHECK_RC(timeval_h2rpc(
                &tv, out->timeout.timeout_val));
    if (TE_RC_GET_ERROR(out->common._errno) == TE_EH2RPC)
        out->retval = -1;
}
)

/*-------------- if_nametoindex() --------------------------------*/

TARPC_FUNC(if_nametoindex, {},
{
    INIT_CHECKED_ARG(in->ifname.ifname_val, in->ifname.ifname_len, 0);
    MAKE_CALL(out->ifindex = func_ptr(in->ifname.ifname_val));
}
)

/*-------------- if_indextoname() --------------------------------*/

TARPC_FUNC(if_indextoname,
{
    COPY_ARG(ifname);
},
{
    if (out->ifname.ifname_val == NULL ||
        out->ifname.ifname_len >= IF_NAMESIZE)
    {
        char *name;

        MAKE_CALL(name = (char *)func_ret_ptr(in->ifindex,
                                              out->ifname.ifname_val));

        if (name != NULL && name != out->ifname.ifname_val)
        {
            ERROR("if_indextoname() returned incorrect pointer");
            out->common._errno = TE_RC(TE_TA_UNIX, TE_ECORRUPTED);
        }
    }
    else
    {
        ERROR("if_indextoname() cannot be called with 'ifname' location "
              "size less than IF_NAMESIZE");
        out->common._errno = TE_RC(TE_TA_UNIX, TE_EINVAL);
    }
}
)

#if HAVE_STRUCT_IF_NAMEINDEX
/*-------------- if_nameindex() ------------------------------*/

TARPC_FUNC(if_nameindex, {},
{
    struct if_nameindex *ret;
    tarpc_if_nameindex  *arr = NULL;

    int i = 0;

    MAKE_CALL(ret = (struct if_nameindex *)func_void_ret_ptr());

    if (ret != NULL)
    {
        int j;

        out->mem_ptr = rcf_pch_mem_alloc(ret);
        while (ret[i++].if_index != 0);
        arr = (tarpc_if_nameindex *)calloc(sizeof(*arr) * i, 1);
        if (arr == NULL)
        {
            out->common._errno = TE_RC(TE_TA_UNIX, TE_ENOMEM);
        }
        else
        {
            for (j = 0; j < i - 1; j++)
            {
                arr[j].ifindex = ret[j].if_index;
                arr[j].ifname.ifname_val = strdup(ret[j].if_name);
                if (arr[j].ifname.ifname_val == NULL)
                {
                    for (j--; j >= 0; j--)
                        free(arr[j].ifname.ifname_val);
                    free(arr);
                    out->common._errno = TE_RC(TE_TA_UNIX, TE_ENOMEM);
                    arr = NULL;
                    i = 0;
                    break;
                }
                arr[j].ifname.ifname_len = strlen(ret[j].if_name) + 1;
            }
        }
    }
    out->ptr.ptr_val = arr;
    out->ptr.ptr_len = i;
}
)

/*-------------- if_freenameindex() ----------------------------*/

TARPC_FUNC(if_freenameindex, {},
{
    MAKE_CALL(func_ptr(rcf_pch_mem_get(in->mem_ptr)));
    rcf_pch_mem_free(in->mem_ptr);
}
)
#endif /* HAVE_STRUCT_IF_NAMEINDEX */

/*-------------- sigset_t constructor ---------------------------*/

bool_t
_sigset_new_1_svc(tarpc_sigset_new_in *in, tarpc_sigset_new_out *out,
                  struct svc_req *rqstp)
{
    sigset_t *set;

    UNUSED(rqstp);
    UNUSED(in);

    memset(out, 0, sizeof(*out));

    errno = 0;
    if ((set = (sigset_t *)calloc(1, sizeof(sigset_t))) == NULL)
    {
        out->common._errno = TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }
    else
    {
        out->common._errno = RPC_ERRNO;
        out->set = rcf_pch_mem_alloc(set);
    }

    return TRUE;
}

/*-------------- sigset_t destructor ----------------------------*/

bool_t
_sigset_delete_1_svc(tarpc_sigset_delete_in *in,
                     tarpc_sigset_delete_out *out,
                     struct svc_req *rqstp)
{
    UNUSED(rqstp);

    memset(out, 0, sizeof(*out));

    errno = 0;
    free(IN_SIGSET);
    rcf_pch_mem_free(in->set);
    out->common._errno = RPC_ERRNO;

    return TRUE;
}

/*-------------- sigemptyset() ------------------------------*/

TARPC_FUNC(sigemptyset, {},
{
    MAKE_CALL(out->retval = func_ptr(IN_SIGSET));
}
)

/*-------------- sigpendingt() ------------------------------*/

TARPC_FUNC(sigpending, {},
{
    MAKE_CALL(out->retval = func_ptr(IN_SIGSET));
}
)

/*-------------- sigsuspend() ------------------------------*/

TARPC_FUNC(sigsuspend, {},
{
    MAKE_CALL(out->retval = func_ptr(IN_SIGSET));
}
)

/*-------------- sigfillset() ------------------------------*/

TARPC_FUNC(sigfillset, {},
{
    MAKE_CALL(out->retval = func_ptr(IN_SIGSET));
}
)

/*-------------- sigaddset() -------------------------------*/

TARPC_FUNC(sigaddset, {},
{
    MAKE_CALL(out->retval = func_ptr(IN_SIGSET, signum_rpc2h(in->signum)));
}
)

/*-------------- sigdelset() -------------------------------*/

TARPC_FUNC(sigdelset, {},
{
    MAKE_CALL(out->retval = func_ptr(IN_SIGSET, signum_rpc2h(in->signum)));
}
)

/*-------------- sigismember() ------------------------------*/

TARPC_FUNC(sigismember, {},
{
    INIT_CHECKED_ARG((char *)(IN_SIGSET), sizeof(sigset_t), 0);
    MAKE_CALL(out->retval = func_ptr(IN_SIGSET, signum_rpc2h(in->signum)));
}
)

/*-------------- sigprocmask() ------------------------------*/
TARPC_FUNC(sigprocmask, {},
{
    INIT_CHECKED_ARG((char *)IN_SIGSET, sizeof(sigset_t), 0);
    MAKE_CALL(out->retval = func(sighow_rpc2h(in->how), IN_SIGSET,
                                 (sigset_t *)rcf_pch_mem_get(in->oldset)));
}
)

/*-------------- sigset_cmp() ------------------------------*/

/**
 * Compare two signal masks.
 *
 * @param sig_first     The first signal mask
 * @param sig_second    The second signal mask
 *
 * @return -1, 0 or 1 as a result of comparison
 */
int
sigset_cmp(sigset_t *sig_first, sigset_t *sig_second)
{
    int          i;
    int          in_first;
    int          in_second;
    int          saved_errno = errno;

    for (i = 1; i <= SIGRTMAX; i++)
    {
        in_first = sigismember(sig_first, i);
        in_second = sigismember(sig_second, i);
        if (in_first != in_second)
        {
            errno = saved_errno;
            return (in_first < in_second) ? -1 : 1;
        }
    }

    errno = saved_errno;
    return 0;
}

TARPC_FUNC(sigset_cmp, {},
{
    sigset_t    *sig1 = (sigset_t *)rcf_pch_mem_get(in->first_set);
    sigset_t    *sig2 = (sigset_t *)rcf_pch_mem_get(in->second_set);

    MAKE_CALL(out->retval = func_ptr(sig1, sig2));
}
)

/*-------------- kill() --------------------------------*/

TARPC_FUNC(kill, {},
{
    MAKE_CALL(out->retval = func(in->pid, signum_rpc2h(in->signum)));
}
)

/*-------------- pthread_kill() ------------------------*/

TARPC_FUNC(pthread_kill, {},
{
    MAKE_CALL(out->retval = func(in->tid, signum_rpc2h(in->signum)));
}
)

/*-------------- tgkill() ------------------------------*/

/**
 * Use tgkill() system call.
 *
 * @param tgid      Thread GID
 * @param tid       Thread ID
 * @param sig       Signal to be sent
 *
 * @return Status code
 */
int
call_tgkill(int tgid, int tid, int sig)
{
#ifndef SYS_tgkill
    UNUSED(tgid);
    UNUSED(tid);
    UNUSED(sig);

    ERROR("tgkill() is not defined");
    errno = ENOENT;
    return -1;
#else
    return syscall(SYS_tgkill, tgid, tid, sig);
#endif
}

TARPC_FUNC(call_tgkill, {},
{
    MAKE_CALL(out->retval = func(in->tgid, in->tid,
                                 signum_rpc2h(in->sig)));
}
)

/*-------------- gettid() ------------------------------*/

/**
 * Use gettid() system call.
 *
 * @return Thread ID or -1
 */
int
call_gettid(void)
{
#ifndef SYS_gettid
    ERROR("gettid() is not defined");
    errno = ENOENT;
    return -1;
#else
    return syscall(SYS_gettid);
#endif
}

TARPC_FUNC(call_gettid, {},
{
    MAKE_CALL(out->retval = func_void());
}
)

/*-------------- waitpid() --------------------------------*/

TARPC_FUNC(waitpid, {},
{
    int             st;
    rpc_wait_status r_st;
    pid_t (*real_func)(pid_t pid, int *status, int options) = func;

    if (!(in->options & RPC_WSYSTEM))
        real_func = ta_waitpid;
    MAKE_CALL(out->pid = real_func(in->pid, &st,
                                   waitpid_opts_rpc2h(in->options)));
    r_st = wait_status_h2rpc(st);
    out->status_flag = r_st.flag;
    out->status_value = r_st.value;
}
)

/*-------------- ta_kill_death() --------------------------------*/

TARPC_FUNC(ta_kill_death, {},
{
    MAKE_CALL(out->retval = func(in->pid));
}
)

/*-------------- ta_kill_and_wait() -----------------------------*/

TARPC_FUNC(ta_kill_and_wait, {},
{
    MAKE_CALL(out->retval = func(in->pid, signum_rpc2h(in->sig), in->timeout));
}
)

sigset_t rpcs_received_signals;

/* See description in unix_internal.h */
void
signal_registrar(int signum)
{
    sigaddset(&rpcs_received_signals, signum);
}

TARPC_FUNC_STANDALONE(signal_registrar_cleanup, {},
{
    int rpc_signum;
    int native_signum;

    for (rpc_signum = RPC_SIG_ZERO + 1; rpc_signum != RPC_SIGUNKNOWN;
         rpc_signum++)
    {
        native_signum = signum_rpc2h(rpc_signum);

        sigdelset(&rpcs_received_signals, native_signum);
    }
})

/* Lastly received signal information */
tarpc_siginfo_t last_siginfo;

/* See description in unix_internal.h */
void
signal_registrar_siginfo(int signum, siginfo_t *siginfo, void *context)
{
#define COPY_SI_FIELD(_field) \
    last_siginfo.sig_ ## _field = siginfo->si_ ## _field

    UNUSED(context);

    sigaddset(&rpcs_received_signals, signum);
    memset(&last_siginfo, 0, sizeof(last_siginfo));

    COPY_SI_FIELD(signo);
    COPY_SI_FIELD(errno);
    COPY_SI_FIELD(code);
#ifdef HAVE_SIGINFO_T_SI_TRAPNO
    COPY_SI_FIELD(trapno);
#endif
    COPY_SI_FIELD(pid);
    COPY_SI_FIELD(uid);
    COPY_SI_FIELD(status);
#ifdef HAVE_SIGINFO_T_SI_UTIME
    COPY_SI_FIELD(utime);
#endif
#ifdef HAVE_SIGINFO_T_SI_STIME
    COPY_SI_FIELD(stime);
#endif

    /** 
     * FIXME: si_value, si_ptr and si_addr fields are not
     * supported yet
     */

#ifdef HAVE_SIGINFO_T_SI_INT
    COPY_SI_FIELD(int);
#endif
#ifdef HAVE_SIGINFO_T_SI_OVERRUN
    COPY_SI_FIELD(overrun);
#endif
#ifdef HAVE_SIGINFO_T_SI_TIMERID
    COPY_SI_FIELD(timerid);
#endif
#ifdef HAVE_SIGINFO_T_SI_BAND
    COPY_SI_FIELD(band);
#endif
#ifdef HAVE_SIGINFO_T_SI_FD
    COPY_SI_FIELD(fd);
#endif

#ifdef HAVE_SIGINFO_T_SI_ADDR_LSB
    COPY_SI_FIELD(addr_lsb);
#endif

#undef COPY_SI_FIELD
}


/*-------------- signal() --------------------------------*/

TARPC_FUNC(signal,
{
    if (in->signum == RPC_SIGINT)
    {
        out->common._errno = TE_RC(TE_TA_UNIX, TE_EPERM);
        return TRUE;
    }
},
{
    sighandler_t handler;

    if ((out->common._errno = name2handler(in->handler,
                                           (void **)&handler)) == 0)
    {
        int     signum = signum_rpc2h(in->signum);
        void   *old_handler;

        MAKE_CALL(old_handler = func_ret_ptr(signum, handler));

        out->handler = handler2name(old_handler);
        if (old_handler != SIG_ERR)
        {
            /*
             * Delete signal from set of received signals when
             * signal registrar is set for the signal.
             */
            if ((handler == signal_registrar) &&
                RPC_IS_ERRNO_RPC(out->common._errno))
            {
                sigdelset(&rpcs_received_signals, signum);
            }
        }
    }
}
)

/*-------------- bsd_signal() --------------------------------*/

/*
 * bsd_signal() declaration in /usr/include/signal.h may be disabled
 * with recent libc because it was removed in POSIX.1-2008.
 */
#ifdef __USE_XOPEN2K8
extern sighandler_t bsd_signal(int signum, sighandler_t handler);
#endif

TARPC_FUNC(bsd_signal,
{
    if (in->signum == RPC_SIGINT)
    {
        out->common._errno = TE_RC(TE_TA_UNIX, TE_EPERM);
        return TRUE;
    }
},
{
    sighandler_t handler;

    if ((out->common._errno = name2handler(in->handler,
                                           (void **)&handler)) == 0)
    {
        int     signum = signum_rpc2h(in->signum);
        void   *old_handler;

        MAKE_CALL(old_handler = func_ret_ptr(signum, handler));

        out->handler = handler2name(old_handler);
        if (old_handler != SIG_ERR)
        {

            /*
             * Delete signal from set of received signals when
             * signal registrar is set for the signal.
             */
            if ((handler == signal_registrar) &&
                RPC_IS_ERRNO_RPC(out->common._errno))
            {
                sigdelset(&rpcs_received_signals, signum);
            }
        }
    }
}
)

/*-------------- sysv_signal() --------------------------------*/

TARPC_FUNC(sysv_signal,
{
    if (in->signum == RPC_SIGINT)
    {
        out->common._errno = TE_RC(TE_TA_UNIX, TE_EPERM);
        return TRUE;
    }
},
{
    sighandler_t handler;

    if ((out->common._errno = name2handler(in->handler,
                                           (void **)&handler)) == 0)
    {
        int     signum = signum_rpc2h(in->signum);
        void   *old_handler;

        MAKE_CALL(old_handler = func_ret_ptr(signum, handler));

        out->handler = handler2name(old_handler);
        if (old_handler != SIG_ERR)
        {
            /*
             * Delete signal from set of received signals when
             * signal registrar is set for the signal.
             */
            if ((handler == signal_registrar) &&
                RPC_IS_ERRNO_RPC(out->common._errno))
            {
                sigdelset(&rpcs_received_signals, signum);
            }
        }
    }
}
)

/*-------------- siginterrupt() --------------------------------*/

TARPC_FUNC(siginterrupt, {},
{
    MAKE_CALL(out->retval = func(signum_rpc2h(in->signum), in->flag));
}
)


/*-------------- sigaction() --------------------------------*/

/** Return opaque value of sa_restorer field of @p sa. */
static uint64_t
get_sa_restorer(struct sigaction *sa)
{
#if HAVE_STRUCT_SIGACTION_SA_RESTORER
    return (uint64_t)sa->sa_restorer;
#else
    UNUSED(sa);
    return 0;
#endif
}

/** Set opaque value @p restorer to sa_restorer field of @p sa. */
static void
set_sa_restorer(struct sigaction *sa, uint64_t restorer)
{
#if HAVE_STRUCT_SIGACTION_SA_RESTORER
    sa->sa_restorer = (void *)restorer;
#else
    UNUSED(sa);
    UNUSED(restorer);
#endif
}


TARPC_FUNC(sigaction,
{
    if (in->signum == RPC_SIGINT)
    {
        out->common._errno = TE_RC(TE_TA_UNIX, TE_EPERM);
        return TRUE;
    }
    COPY_ARG(oldact);
},
{
    tarpc_sigaction  *out_oldact = out->oldact.oldact_val;

    int               signum = signum_rpc2h(in->signum);
    struct sigaction  act;
    struct sigaction *p_act = NULL;
    struct sigaction  oldact;
    struct sigaction *p_oldact = NULL;
    sigset_t         *oldact_mask = NULL;

    memset(&act, 0, sizeof(act));
    memset(&oldact, 0, sizeof(oldact));

    if (in->act.act_len != 0)
    {
        tarpc_sigaction *in_act = in->act.act_val;
        sigset_t        *act_mask;

        p_act = &act;

        act.sa_flags = sigaction_flags_rpc2h(in_act->flags);
        act_mask = (sigset_t *) rcf_pch_mem_get(in_act->mask);
        if (act_mask == NULL)
        {
            out->common._errno = TE_RC(TE_TA_UNIX, TE_EFAULT);
            out->retval = -1;
            goto finish;
        }
        act.sa_mask = *act_mask;

        out->common._errno =
            name2handler(in_act->handler,
                         (act.sa_flags & SA_SIGINFO) ?
                         (void **)&(act.sa_sigaction) :
                         (void **)&(act.sa_handler));

        if (out->common._errno != 0)
        {
            out->retval = -1;
            goto finish;
        }

        set_sa_restorer(&act, in_act->restorer);
    }

    if (out->oldact.oldact_len != 0)
    {
        p_oldact = &oldact;

        oldact.sa_flags = sigaction_flags_rpc2h(out_oldact->flags);
        if ((out_oldact->mask != RPC_NULL) &&
            (oldact_mask =
                (sigset_t *) rcf_pch_mem_get(out_oldact->mask)) == NULL)
        {
            out->common._errno = TE_RC(TE_TA_UNIX, TE_EFAULT);
            out->retval = -1;
            goto finish;
        }
        if (oldact_mask != NULL)
            oldact.sa_mask = *oldact_mask;

        out->common._errno =
            name2handler(out_oldact->handler,
                         (oldact.sa_flags & SA_SIGINFO) ?
                         (void **)&(oldact.sa_sigaction) :
                         (void **)&(oldact.sa_handler));

        if (out->common._errno != 0)
        {
            ERROR("Cannot convert incoming `oldact.sa_handler` function name "
                  "'%s' to handler: %r", out_oldact->handler,
                  out->common._errno);
            out->retval = -1;
            goto finish;
        }

        set_sa_restorer(&oldact, out_oldact->restorer);
    }

    MAKE_CALL(out->retval = func(signum, p_act, p_oldact));

    if ((out->retval == 0) && (p_act != NULL) &&
        (((p_act->sa_flags & SA_SIGINFO) ?
              (void *)(act.sa_sigaction) :
              (void *)(act.sa_handler)) == signal_registrar))
    {
        /*
         * Delete signal from set of received signals when
         * signal registrar is set for the signal.
         */
        sigdelset(&rpcs_received_signals, signum);
    }

    if (p_oldact != NULL)
    {
        out_oldact->flags = sigaction_flags_h2rpc(oldact.sa_flags);
        if (oldact_mask != NULL)
            *oldact_mask = oldact.sa_mask;
        out_oldact->handler = handler2name((oldact.sa_flags & SA_SIGINFO) ?
                                           (void *)oldact.sa_sigaction :
                                           (void *)oldact.sa_handler);
        out_oldact->restorer = get_sa_restorer(&oldact);
    }

    finish:
    ;
}
)

/** Convert tarpc_stack_t to stack_t.
 *
 * @param tarpc_s   Pointer to TARPC structure
 * @param h_s       Pointer to native structure
 *
 * @return @c 0 on success or @c -1
 * */
int
stack_t_tarpc2h(tarpc_stack_t *tarpc_s, stack_t *h_s)
{
    if (tarpc_s == NULL || h_s == NULL)
        return -1;

    h_s->ss_sp = rcf_pch_mem_get(tarpc_s->ss_sp);
    h_s->ss_flags = sigaltstack_flags_rpc2h(tarpc_s->ss_flags);
    h_s->ss_size = tarpc_s->ss_size;

    return 0;
}

/** Convert stack_t to tarpc_stack_t.
 *
 * @param h_s       Pointer to native structure
 * @param tarpc_s   Pointer to TARPC structure
 *
 * @return @c 0 on success or @c -1
 * */
int
stack_t_h2tarpc(stack_t *h_s, tarpc_stack_t *tarpc_s)
{
    if (tarpc_s == NULL || h_s == NULL)
        return -1;

    tarpc_s->ss_sp = rcf_pch_mem_get_id(h_s->ss_sp);
    if (tarpc_s->ss_sp == 0 && h_s->ss_sp != NULL)
        tarpc_s->ss_sp = RPC_UNKNOWN_ADDR;
    tarpc_s->ss_flags = sigaltstack_flags_h2rpc(h_s->ss_flags);
    tarpc_s->ss_size = h_s->ss_size;

    return 0;
}

/*-------------- sigaltstack() -----------------------------*/
TARPC_FUNC(sigaltstack,
{
    COPY_ARG(oss);
},
{
    tarpc_stack_t      *out_ss = NULL;

    stack_t      ss;
    stack_t      oss;
    stack_t     *ss_arg = NULL;
    stack_t     *oss_arg = NULL;

    memset(&ss, 0, sizeof(ss));
    memset(&oss, 0, sizeof(ss));

    if (in->ss.ss_len != 0)
    {
        stack_t_tarpc2h(in->ss.ss_val, &ss);
        ss_arg = &ss;
    }

    if (out->oss.oss_len != 0)
    {
        out_ss = out->oss.oss_val;
        stack_t_tarpc2h(out->oss.oss_val, &oss);
        oss_arg = &oss;
    }

    MAKE_CALL(out->retval = func_ptr(ss_arg, oss_arg));

    if (oss_arg != NULL)
        stack_t_h2tarpc(oss_arg, out_ss);
})

/*-------------- setsockopt() ------------------------------*/

#if HAVE_STRUCT_IPV6_MREQ_IPV6MR_IFINDEX
#define IPV6MR_IFINDEX  ipv6mr_ifindex
#elif HAVE_STRUCT_IPV6_MREQ_IPV6MR_INTERFACE
#define IPV6MR_IFINDEX  ipv6mr_interface
#else
#error No interface index in struct ipv6_mreq
#endif

typedef union sockopt_param {
    int                   integer;
    char                 *str;
    struct timeval        tv;
    struct linger         linger;
    struct in_addr        addr;
    struct in6_addr       addr6;
    struct ip_mreq        mreq;
    struct ip_mreq_source mreq_source;
#if HAVE_STRUCT_IP_MREQN
    struct ip_mreqn       mreqn;
#endif
    struct ipv6_mreq      mreq6;
#if HAVE_STRUCT_TCP_INFO
    struct tcp_info       tcpi;
#endif
#if HAVE_STRUCT_GROUP_REQ
    struct group_req      gr_req;
#endif
} sockopt_param;

static void
tarpc_setsockopt(tarpc_setsockopt_in *in, tarpc_setsockopt_out *out,
                 sockopt_param *param, socklen_t *optlen)
{
    option_value   *in_optval = in->optval.optval_val;

    switch (in_optval->opttype)
    {
        case OPT_INT:
        {
            param->integer = in_optval->option_value_u.opt_int;
            *optlen = sizeof(int);

            if (in->level == RPC_SOL_IP &&
                in->optname == RPC_IP_MTU_DISCOVER)
                param->integer = mtu_discover_arg_rpc2h(param->integer);
#ifdef HAVE_LINUX_NET_TSTAMP_H
            if (in->level == RPC_SOL_SOCKET &&
                in->optname == RPC_SO_TIMESTAMPING)
                param->integer = hwtstamp_instr_rpc2h(param->integer);
#endif
            break;
        }

        case OPT_TIMEVAL:
        {
            param->tv.tv_sec =
                in_optval->option_value_u.opt_timeval.tv_sec;
            param->tv.tv_usec =
                in_optval->option_value_u.opt_timeval.tv_usec;
            *optlen = sizeof(param->tv);
            break;
        }

        case OPT_LINGER:
        {
            param->linger.l_onoff =
                in_optval->option_value_u.opt_linger.l_onoff;
            param->linger.l_linger =
                in_optval->option_value_u.opt_linger.l_linger;
            *optlen = sizeof(param->linger);
            break;
        }

        case OPT_MREQ:
        {
            memcpy(&param->mreq.imr_multiaddr,
                   &in_optval->option_value_u.opt_mreq.imr_multiaddr,
                   sizeof(param->mreq.imr_multiaddr));
            param->mreq.imr_multiaddr.s_addr =
                htonl(param->mreq.imr_multiaddr.s_addr);
            memcpy(&param->mreq.imr_interface,
                   &in_optval->option_value_u.opt_mreq.imr_address,
                   sizeof(param->mreq.imr_interface));
            param->mreq.imr_interface.s_addr =
                htonl(param->mreq.imr_interface.s_addr);
            *optlen = sizeof(param->mreq);
            break;
        }

        case OPT_MREQ_SOURCE:
        {
            memcpy((char *)&(param->mreq_source.imr_multiaddr),
                   &in_optval->option_value_u.opt_mreq_source.imr_multiaddr,
                   sizeof(param->mreq_source.imr_multiaddr));
            param->mreq_source.imr_multiaddr.s_addr =
                htonl(param->mreq_source.imr_multiaddr.s_addr);

            memcpy((char *)&(param->mreq_source.imr_interface),
                   &in_optval->option_value_u.opt_mreq_source.imr_interface,
                   sizeof(param->mreq_source.imr_interface));
            param->mreq_source.imr_interface.s_addr =
                htonl(param->mreq_source.imr_interface.s_addr);

            memcpy((char *)&(param->mreq_source.imr_sourceaddr),
                  &in_optval->option_value_u.opt_mreq_source.imr_sourceaddr,
                   sizeof(param->mreq_source.imr_sourceaddr));
            param->mreq_source.imr_sourceaddr.s_addr =
                htonl(param->mreq_source.imr_sourceaddr.s_addr);

            *optlen = sizeof(param->mreq_source);
            break;
        }

        case OPT_MREQN:
        {
#if HAVE_STRUCT_IP_MREQN
            memcpy((char *)&(param->mreqn.imr_multiaddr),
                   &in_optval->option_value_u.opt_mreqn.imr_multiaddr,
                   sizeof(param->mreqn.imr_multiaddr));
            param->mreqn.imr_multiaddr.s_addr =
                htonl(param->mreqn.imr_multiaddr.s_addr);
            memcpy((char *)&(param->mreqn.imr_address),
                   &in_optval->option_value_u.opt_mreqn.imr_address,
                   sizeof(param->mreqn.imr_address));
            param->mreqn.imr_address.s_addr =
                htonl(param->mreqn.imr_address.s_addr);

            param->mreqn.imr_ifindex =
                in_optval->option_value_u.opt_mreqn.imr_ifindex;
            *optlen = sizeof(param->mreqn);
            break;
#else
            ERROR("'struct ip_mreqn' is not defined");
            out->common._errno = TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP);
            out->retval = -1;
            break;
#endif
        }
        case OPT_MREQ6:
        {
            memcpy(&(param->mreq6.ipv6mr_multiaddr),
                   &in_optval->option_value_u.opt_mreq6.
                       ipv6mr_multiaddr.ipv6mr_multiaddr_val,
                   sizeof(struct in6_addr));
            param->mreq6.ipv6mr_interface =
                in_optval->option_value_u.opt_mreq6.ipv6mr_ifindex;
            *optlen = sizeof(param->mreq6);
            break;
        }

        case OPT_IPADDR:
        {
            memcpy(&param->addr, &in_optval->option_value_u.opt_ipaddr,
                   sizeof(struct in_addr));
            param->addr.s_addr = htonl(param->addr.s_addr);
            *optlen = sizeof(param->addr);
            break;
        }

        case OPT_IPADDR6:
        {
            memcpy(&param->addr6, in_optval->option_value_u.opt_ipaddr6,
                   sizeof(struct in6_addr));
            *optlen = sizeof(param->addr6);
            break;
        }

        case OPT_GROUP_REQ:
        {
#if HAVE_STRUCT_GROUP_REQ
            sockaddr_rpc2h(&in_optval->option_value_u.
                           opt_group_req.gr_group,
                           (struct sockaddr *)&(param->gr_req.gr_group),
                           sizeof(param->gr_req.gr_group), NULL, NULL);
            param->gr_req.gr_interface =
                in_optval->option_value_u.opt_group_req.gr_interface;
            *optlen = sizeof(param->gr_req);
            break;
#else
            ERROR("'struct group_req' is not defined");
            out->common._errno = TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP);
            out->retval = -1;
            break;
#endif
        }

        default:
            ERROR("incorrect option type %d is received",
                  in_optval->opttype);
            out->common._errno = TE_RC(TE_TA_UNIX, TE_EINVAL);
            out->retval = -1;
            break;
    }
}

TARPC_FUNC(setsockopt,
{},
{
    if (in->optval.optval_val == NULL)
    {
        MAKE_CALL(out->retval = func(in->s, socklevel_rpc2h(in->level),
                                     sockopt_rpc2h(in->optname),
                                     in->raw_optval.raw_optval_val,
                                     in->raw_optlen));
    }
    else
    {
        sockopt_param  opt;
        socklen_t      optlen;

        tarpc_setsockopt(in, out, &opt, &optlen);
        if (out->retval == 0)
        {
            uint8_t    *val;
            socklen_t   len;

            if (in->raw_optval.raw_optval_val != NULL)
            {
                len = optlen + in->raw_optlen;
                val = malloc(len);
                assert(val != NULL);
                memcpy(val, &opt, optlen);
                memcpy(val + optlen, in->raw_optval.raw_optval_val,
                       in->raw_optval.raw_optval_len);
            }
            else
            {
                len = optlen;
                val = (uint8_t *)&opt;
            }

            INIT_CHECKED_ARG(val, len, 0);

            MAKE_CALL(out->retval = func(in->s, socklevel_rpc2h(in->level),
                                         sockopt_rpc2h(in->optname),
                                         val, len));

            if (val != (uint8_t *)&opt)
                free(val);
        }
    }
}
)


/*-------------- getsockopt() ------------------------------*/

#define COPY_TCP_INFO_FIELD(_name) \
    do {                                          \
        out->optval.optval_val[0].option_value_u. \
            opt_tcp_info._name = info->_name;     \
    } while (0)

#define CONVERT_TCP_INFO_FIELD(_name, _func) \
    do {                                             \
        out->optval.optval_val[0].option_value_u.    \
            opt_tcp_info._name = _func(info->_name); \
    } while (0)

static socklen_t
tarpc_sockoptlen(const option_value *optval)
{
    switch (optval->opttype)
    {
        case OPT_INT:
            return sizeof(int);

        case OPT_TIMEVAL:
            return sizeof(struct timeval);

        case OPT_LINGER:
            return sizeof(struct linger);

        case OPT_MREQN:
#if HAVE_STRUCT_IP_MREQN
            return sizeof(struct ip_mreqn);
#endif

        case OPT_MREQ:
            return sizeof(struct ip_mreq);

        case OPT_MREQ_SOURCE:
            return sizeof(struct ip_mreq_source);

        case OPT_MREQ6:
            return sizeof(struct ipv6_mreq);

        case OPT_IPADDR:
            return sizeof(struct in_addr);

        case OPT_IPADDR6:
            return sizeof(struct in6_addr);

#if HAVE_STRUCT_TCP_INFO
        case OPT_TCP_INFO:
            return sizeof(struct tcp_info);
#endif

        default:
            ERROR("incorrect option type %d is received",
                  optval->opttype);
            return 0;
    }
}

static void
tarpc_getsockopt(tarpc_getsockopt_in *in, tarpc_getsockopt_out *out,
                 const void *opt, socklen_t optlen)
{
    option_value   *out_optval = out->optval.optval_val;

    if (out_optval->opttype == OPT_MREQN
#if HAVE_STRUCT_IP_MREQN
        && optlen < (socklen_t)sizeof(struct ip_mreqn)
#endif
        )
    {
        out_optval->opttype = OPT_MREQ;
    }
    if (out_optval->opttype == OPT_MREQ &&
        optlen < (socklen_t)sizeof(struct ip_mreq))
    {
        out_optval->opttype = OPT_IPADDR;
    }

    switch (out_optval->opttype)
    {
        case OPT_INT:
        {
            /*
             * SO_ERROR socket option keeps the value of the last
             * pending error occurred on the socket, so that we should
             * convert its value to host independend representation,
             * which is RPC errno
             */
            if (in->level == RPC_SOL_SOCKET &&
                in->optname == RPC_SO_ERROR)
            {
                *(int *)opt = errno_h2rpc(*(int *)opt);
            }
            /*
             * SO_TYPE and SO_STYLE socket option keeps the value of
             * socket type they are called for, so that we should
             * convert its value to host independend representation,
             * which is RPC socket type
             */
            else if (in->level == RPC_SOL_SOCKET &&
                     in->optname == RPC_SO_TYPE)
            {
                *(int *)opt = socktype_h2rpc(*(int *)opt);
            }
            else if (in->level == RPC_SOL_SOCKET &&
                     in->optname == RPC_SO_PROTOCOL)
            {
                *(int *)opt = proto_h2rpc(*(int *)opt);
            }
            else if (in->level == RPC_SOL_IP &&
                     in->optname == RPC_IP_MTU_DISCOVER)
            {
                *(int *)opt = mtu_discover_arg_h2rpc(*(int *)opt);
            }

            out_optval->option_value_u.opt_int = *(int *)opt;
            break;
        }

        case OPT_TIMEVAL:
        {
            struct timeval *tv = (struct timeval *)opt;

            out_optval->option_value_u.opt_timeval.tv_sec = tv->tv_sec;
            out_optval->option_value_u.opt_timeval.tv_usec = tv->tv_usec;
            break;
        }

        case OPT_LINGER:
        {
            struct linger *linger = (struct linger *)opt;

            out_optval->option_value_u.opt_linger.l_onoff =
                linger->l_onoff;
            out_optval->option_value_u.opt_linger.l_linger =
                linger->l_linger;
            break;
        }

        case OPT_MREQN:
        {
#if HAVE_STRUCT_IP_MREQN
            struct ip_mreqn *mreqn = (struct ip_mreqn *)opt;

            memcpy(&out_optval->option_value_u.opt_mreqn.imr_multiaddr,
                   &(mreqn->imr_multiaddr), sizeof(mreqn->imr_multiaddr));
            out_optval->option_value_u.opt_mreqn.imr_multiaddr =
                ntohl(out_optval->option_value_u.opt_mreqn.imr_multiaddr);
            memcpy(&out_optval->option_value_u.opt_mreqn.imr_address,
                   &(mreqn->imr_address), sizeof(mreqn->imr_address));
            out_optval->option_value_u.opt_mreqn.imr_address =
                ntohl(out_optval->option_value_u.opt_mreqn.imr_address);
            out_optval->option_value_u.opt_mreqn.imr_ifindex =
                mreqn->imr_ifindex;
#else
            ERROR("'struct ip_mreqn' is not defined");
            out->common._errno = TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP);
            out->retval = -1;
#endif
            break;
        }

        case OPT_MREQ:
        {
            struct ip_mreq *mreq = (struct ip_mreq *)opt;

            memcpy(&out_optval->option_value_u.opt_mreq.imr_multiaddr,
                   &(mreq->imr_multiaddr), sizeof(mreq->imr_multiaddr));
            out_optval->option_value_u.opt_mreq.imr_multiaddr =
                ntohl(out_optval->option_value_u.opt_mreq.imr_multiaddr);
            memcpy(&out_optval->option_value_u.opt_mreq.imr_address,
                   &(mreq->imr_interface), sizeof(mreq->imr_interface));
            out_optval->option_value_u.opt_mreq.imr_address =
                ntohl(out_optval->option_value_u.opt_mreq.imr_address);
            break;
        }

        case OPT_MREQ_SOURCE:
        {
            struct ip_mreq_source *mreq = (struct ip_mreq_source *)opt;

            memcpy(&out_optval->option_value_u.opt_mreq_source.
                   imr_multiaddr,
                   &(mreq->imr_multiaddr), sizeof(mreq->imr_multiaddr));
            out_optval->option_value_u.opt_mreq_source.imr_multiaddr =
                ntohl(out_optval->option_value_u.opt_mreq_source.
                      imr_multiaddr);

            memcpy(&out_optval->option_value_u.opt_mreq_source.
                   imr_interface,
                   &(mreq->imr_interface), sizeof(mreq->imr_interface));
            out_optval->option_value_u.opt_mreq_source.imr_interface =
                ntohl(out_optval->option_value_u.opt_mreq_source.
                      imr_interface);

            memcpy(&out_optval->option_value_u.opt_mreq_source.
                   imr_sourceaddr,
                   &(mreq->imr_sourceaddr), sizeof(mreq->imr_sourceaddr));
            out_optval->option_value_u.opt_mreq_source.imr_sourceaddr =
                ntohl(out_optval->option_value_u.opt_mreq_source.
                      imr_sourceaddr);
            break;
        }

        case OPT_MREQ6:
        {
            struct ipv6_mreq *mreq6 = (struct ipv6_mreq *)opt;

            memcpy(&out_optval->option_value_u.opt_mreq6.ipv6mr_multiaddr,
                   &(mreq6->ipv6mr_multiaddr), sizeof(struct ipv6_mreq));
            out_optval->option_value_u.opt_mreq6.ipv6mr_ifindex =
                mreq6->IPV6MR_IFINDEX;
            break;
        }

        case OPT_IPADDR:
            memcpy(&out_optval->option_value_u.opt_ipaddr,
                   opt, sizeof(struct in_addr));
            out_optval->option_value_u.opt_ipaddr =
                ntohl(out_optval->option_value_u.opt_ipaddr);
            break;

        case OPT_IPADDR6:
            memcpy(out_optval->option_value_u.opt_ipaddr6,
                   opt, sizeof(struct in6_addr));
            break;

        case OPT_TCP_INFO:
        {
#if HAVE_STRUCT_TCP_INFO
            struct tcp_info *info = (struct tcp_info *)opt;

            CONVERT_TCP_INFO_FIELD(tcpi_state, tcp_state_h2rpc);
            CONVERT_TCP_INFO_FIELD(tcpi_ca_state, tcp_ca_state_h2rpc);
            COPY_TCP_INFO_FIELD(tcpi_retransmits);
            COPY_TCP_INFO_FIELD(tcpi_probes);
            COPY_TCP_INFO_FIELD(tcpi_backoff);
            CONVERT_TCP_INFO_FIELD(tcpi_options, tcpi_options_h2rpc);
            COPY_TCP_INFO_FIELD(tcpi_snd_wscale);
            COPY_TCP_INFO_FIELD(tcpi_rcv_wscale);
            COPY_TCP_INFO_FIELD(tcpi_rto);
            COPY_TCP_INFO_FIELD(tcpi_ato);
            COPY_TCP_INFO_FIELD(tcpi_snd_mss);
            COPY_TCP_INFO_FIELD(tcpi_rcv_mss);
            COPY_TCP_INFO_FIELD(tcpi_unacked);
            COPY_TCP_INFO_FIELD(tcpi_sacked);
            COPY_TCP_INFO_FIELD(tcpi_lost);
            COPY_TCP_INFO_FIELD(tcpi_retrans);
            COPY_TCP_INFO_FIELD(tcpi_fackets);
            COPY_TCP_INFO_FIELD(tcpi_last_data_sent);
            COPY_TCP_INFO_FIELD(tcpi_last_ack_sent);
            COPY_TCP_INFO_FIELD(tcpi_last_data_recv);
            COPY_TCP_INFO_FIELD(tcpi_last_ack_recv);
            COPY_TCP_INFO_FIELD(tcpi_pmtu);
            COPY_TCP_INFO_FIELD(tcpi_rcv_ssthresh);
            COPY_TCP_INFO_FIELD(tcpi_rtt);
            COPY_TCP_INFO_FIELD(tcpi_rttvar);
            COPY_TCP_INFO_FIELD(tcpi_snd_ssthresh);
            COPY_TCP_INFO_FIELD(tcpi_snd_cwnd);
            COPY_TCP_INFO_FIELD(tcpi_advmss);
            COPY_TCP_INFO_FIELD(tcpi_reordering);
#if HAVE_STRUCT_TCP_INFO_TCPI_RCV_RTT
            COPY_TCP_INFO_FIELD(tcpi_rcv_rtt);
            COPY_TCP_INFO_FIELD(tcpi_rcv_space);
            COPY_TCP_INFO_FIELD(tcpi_total_retrans);
#endif

#else
            ERROR("'struct tcp_info' is not defined");
            out->common._errno = TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP);
            out->retval = -1;
#endif
            break;
        }

        case OPT_IP_PKTOPTIONS:
        {
#define OPTVAL out_optval->option_value_u.opt_ip_pktoptions. \
                                            opt_ip_pktoptions_val
#define OPTLEN out_optval->option_value_u.opt_ip_pktoptions. \
                                            opt_ip_pktoptions_len
            if (optlen > 0)
            {
                int                  rc;

                rc = msg_control_h2rpc((uint8_t *)opt, optlen,
                                       &OPTVAL, &OPTLEN,
                                       NULL, NULL);
                if (rc != 0)
                {
                    ERROR("Failed to process IP_PKTOPTIONS value");
                    out->retval = -1;
                    out->common._errno = TE_RC(TE_TA_UNIX, rc);
                    break;
                }
            }

            break;
#undef OPTVAL
#undef OPTLEN
        }

        default:
            ERROR("incorrect option type %d is received",
                  out_optval->opttype);
            break;
    }
}

TARPC_FUNC(getsockopt,
{
    COPY_ARG(optval);
    COPY_ARG(raw_optval);
    COPY_ARG(raw_optlen);
},
{
    if (out->optval.optval_val == NULL)
    {

        INIT_CHECKED_ARG(out->raw_optval.raw_optval_val,
                         out->raw_optval.raw_optval_len,
                         out->raw_optlen.raw_optlen_val == NULL ? 0 :
                                        *(out->raw_optlen.raw_optlen_val));

        MAKE_CALL(out->retval = func(in->s, socklevel_rpc2h(in->level),
                                     sockopt_rpc2h(in->optname),
                                     out->raw_optval.raw_optval_val,
                                     out->raw_optlen.raw_optlen_val));

        if (in->level == RPC_SOL_IP && in->optname == RPC_IP_PKTOPTIONS)
        {
            out->optval.optval_len = 1;
            out->optval.optval_val = calloc(1,
                                            sizeof(struct option_value));
            assert(out->optval.optval_val != NULL);

            out->optval.optval_val[0].opttype = OPT_IP_PKTOPTIONS;
            out->optval.optval_val[0].option_value_u.opt_ip_pktoptions.
                        opt_ip_pktoptions_val = NULL;
            out->optval.optval_val[0].option_value_u.opt_ip_pktoptions.
                        opt_ip_pktoptions_len = 0;

            if (out->retval >= 0)
                tarpc_getsockopt(in, out, out->raw_optval.raw_optval_val,
                                 out->raw_optlen.raw_optlen_val == NULL ?
                                 0 : *(out->raw_optlen.raw_optlen_val));
        }
    }
    else
    {
        socklen_t   optlen = tarpc_sockoptlen(out->optval.optval_val);
        socklen_t   rlen = optlen + out->raw_optval.raw_optval_len;
        socklen_t   len = optlen +
                          (out->raw_optlen.raw_optlen_val == NULL ? 0 :
                               *out->raw_optlen.raw_optlen_val);
        void       *buf = calloc(1, rlen);

        assert(buf != NULL);
        INIT_CHECKED_ARG(buf, rlen, len);

        MAKE_CALL(out->retval =
                      func(in->s, socklevel_rpc2h(in->level),
                           sockopt_rpc2h(in->optname), buf, &len));

        tarpc_getsockopt(in, out, buf, len);
        free(buf);
    }

}
)

#undef COPY_TCP_INFO_FIELD
#undef CONVERT_TCP_INFO_FIELD

/*-------------- pselect() --------------------------------*/

TARPC_FUNC(pselect,
{
    COPY_ARG(timeout);
},
{
    out->retval = -1;
    RPC_PCH_MEM_WITH_NAMESPACE(ns, RPC_TYPE_NS_FD_SET, {
        struct timespec tv;

        if (out->timeout.timeout_len > 0)
        {
            tv.tv_sec = out->timeout.timeout_val[0].tv_sec;
            tv.tv_nsec = out->timeout.timeout_val[0].tv_nsec;
        }
        if (out->common._errno == 0)
        {
            fd_set *rfds = RCF_PCH_MEM_INDEX_MEM_TO_PTR(in->readfds, ns);
            fd_set *wfds = RCF_PCH_MEM_INDEX_MEM_TO_PTR(in->writefds, ns);
            fd_set *efds = RCF_PCH_MEM_INDEX_MEM_TO_PTR(in->exceptfds, ns);
            sigset_t *sigmask = rcf_pch_mem_get(in->sigmask);
            /*
             * The pointer may be a NULL and, therefore, contain
             * uninitialized data, but we want to check that the data are
             * unchanged even in this case.
             */
            INIT_CHECKED_ARG(sigmask, sizeof(sigset_t), 0);

            MAKE_CALL(out->retval = func(in->n, rfds, wfds, efds,
                            out->timeout.timeout_len == 0 ? NULL : &tv,
                            sigmask));

            if (out->timeout.timeout_len > 0)
            {
                out->timeout.timeout_val[0].tv_sec = tv.tv_sec;
                out->timeout.timeout_val[0].tv_nsec = tv.tv_nsec;
            }
        }

#ifdef __linux__
        if (out->retval >= 0 && out->common.errno_changed &&
            out->common._errno == RPC_ENOSYS)
        {
            WARN("pselect() returned non-negative value, but changed "
                 "errno to ENOSYS");
            out->common.errno_changed = 0;
        }
#endif
    });
}
)

/*-------------- fcntl() --------------------------------*/

TARPC_FUNC(fcntl,
{
    COPY_ARG(arg);
},
{
    long int_arg;

    if (in->cmd == RPC_F_GETFD || in->cmd == RPC_F_GETFL ||
        in->cmd == RPC_F_GETSIG
#if defined (F_GETPIPE_SZ)
        || in->cmd == RPC_F_GETPIPE_SZ
#endif
        )
        MAKE_CALL(out->retval = func(in->fd, fcntl_rpc2h(in->cmd)));
#if defined (F_GETOWN_EX) && defined (F_SETOWN_EX)
    else if (in->cmd == RPC_F_GETOWN_EX || in->cmd == RPC_F_SETOWN_EX)
    {
        struct f_owner_ex foex_arg;

        foex_arg.type =
            out->arg.arg_val[0].fcntl_request_u.req_f_owner_ex.type;
        foex_arg.pid =
            out->arg.arg_val[0].fcntl_request_u.req_f_owner_ex.pid;
        MAKE_CALL(out->retval = func(in->fd, fcntl_rpc2h(in->cmd),
                                     &foex_arg));
        out->arg.arg_val[0].fcntl_request_u.req_f_owner_ex.type =
            foex_arg.type;
        out->arg.arg_val[0].fcntl_request_u.req_f_owner_ex.pid =
            foex_arg.pid;
    }
#endif
    else
    {
        int_arg = out->arg.arg_val[0].fcntl_request_u.req_int;
        if (in->cmd == RPC_F_SETFL)
            int_arg = fcntl_flags_rpc2h(int_arg);
        else if (in->cmd == RPC_F_SETSIG)
            int_arg = signum_rpc2h(int_arg);
        MAKE_CALL(out->retval =
                    func(in->fd, fcntl_rpc2h(in->cmd), int_arg));
    }

    if (in->cmd == RPC_F_GETFL)
        out->retval = fcntl_flags_h2rpc(out->retval);
    else if (in->cmd == RPC_F_GETSIG)
        out->retval = signum_h2rpc(out->retval);
}
)


/*-------------- ioctl() --------------------------------*/

typedef union ioctl_param {
    int              integer;
    struct timeval   tv;
    struct timespec  ts;
    struct ifreq     ifreq;
    struct ifconf    ifconf;
    struct arpreq    arpreq;
#ifdef HAVE_STRUCT_SG_IO_HDR
    struct sg_io_hdr sg;
#endif
} ioctl_param;

static void
tarpc_ioctl_pre(tarpc_ioctl_in *in, tarpc_ioctl_out *out,
                ioctl_param *req, checked_arg_list *arglist)
{
    size_t  reqlen;

    assert(in != NULL);
    assert(out != NULL);
    assert(req != NULL);

    switch (out->req.req_val[0].type)
    {
        case IOCTL_INT:
            reqlen = sizeof(int);
            req->integer = out->req.req_val[0].ioctl_request_u.req_int;
            break;

        case IOCTL_TIMEVAL:
            reqlen = sizeof(struct timeval);
            req->tv.tv_sec =
                out->req.req_val[0].ioctl_request_u.req_timeval.tv_sec;
            req->tv.tv_usec =
                out->req.req_val[0].ioctl_request_u.req_timeval.tv_usec;
            break;

        case IOCTL_TIMESPEC:
            reqlen = sizeof(struct timespec);
            req->ts.tv_sec =
                out->req.req_val[0].ioctl_request_u.req_timespec.tv_sec;
            req->ts.tv_nsec =
                out->req.req_val[0].ioctl_request_u.req_timespec.tv_nsec;
            break;

        case IOCTL_IFREQ:
        {
            reqlen = sizeof(struct ifreq);

            /* Copy the whole 'ifr_name' buffer, not just strcpy() */
            memcpy(req->ifreq.ifr_name,
                   out->req.req_val[0].ioctl_request_u.req_ifreq.
                       rpc_ifr_name.rpc_ifr_name_val,
                   sizeof(req->ifreq.ifr_name));

            if (in->code != RPC_SIOCGIFNAME)
                INIT_CHECKED_ARG(req->ifreq.ifr_name,
                                 strlen(req->ifreq.ifr_name) + 1, 0);

            switch (in->code)
            {
                case RPC_SIOCSIFFLAGS:
                    req->ifreq.ifr_flags =
                        if_fl_rpc2h((uint32_t)(unsigned short int)
                            out->req.req_val[0].ioctl_request_u.
                                req_ifreq.rpc_ifr_flags);
                    break;

                    /* QNX does not support SIOCGIFNAME */
#ifdef SIOCGIFNAME
                case RPC_SIOCGIFNAME:
#if SOLARIS
                    req->ifreq.ifr_index =
                        out->req.req_val[0].ioctl_request_u.
                        req_ifreq.rpc_ifr_ifindex;
#else
                    req->ifreq.ifr_ifindex =
                        out->req.req_val[0].ioctl_request_u.
                        req_ifreq.rpc_ifr_ifindex;
#endif
                    break;
#endif /* SIOCGIFNAME */

                case RPC_SIOCSIFMTU:
#if HAVE_STRUCT_IFREQ_IFR_MTU
                    req->ifreq.ifr_mtu =
                        out->req.req_val[0].ioctl_request_u.
                        req_ifreq.rpc_ifr_mtu;
#else
                    WARN("'struct ifreq' has no 'ifr_mtu'");
#endif
                    break;

                case RPC_SIOCSIFADDR:
                case RPC_SIOCSIFNETMASK:
                case RPC_SIOCSIFBRDADDR:
                case RPC_SIOCSIFDSTADDR:
                    sockaddr_rpc2h(&(out->req.req_val[0].
                        ioctl_request_u.req_ifreq.rpc_ifr_addr),
                        &(req->ifreq.ifr_addr),
                        sizeof(req->ifreq.ifr_addr), NULL, NULL);
                   break;

#if HAVE_LINUX_ETHTOOL_H
                case RPC_SIOCETHTOOL:
                    ethtool_data_rpc2h(
                                    &(out->req.req_val[0].ioctl_request_u.
                                      req_ifreq.rpc_ifr_ethtool),
                                    &req->ifreq.ifr_data);
                    break;
#endif /* HAVE_LINUX_ETHTOOL_H */
#if HAVE_LINUX_NET_TSTAMP_H
                case RPC_SIOCSHWTSTAMP:
                case RPC_SIOCGHWTSTAMP:
                    hwtstamp_config_data_rpc2h(
                                    &(out->req.req_val[0].ioctl_request_u.
                                      req_ifreq.rpc_ifr_hwstamp),
                                    &req->ifreq.ifr_data);
                    break;
#endif
            }
            break;
        }
        case IOCTL_IFCONF:
        {
            char *buf = NULL;
            int   buflen = out->req.req_val[0].ioctl_request_u.
                           req_ifconf.nmemb * sizeof(struct ifreq) +
                           out->req.req_val[0].ioctl_request_u.
                           req_ifconf.extra;

            reqlen = sizeof(req->ifconf);

            if (buflen > 0 && (buf = calloc(1, buflen + 64)) == NULL)
            {
                ERROR("Out of memory");
                out->common._errno = TE_RC(TE_TA_UNIX, TE_ENOMEM);
                return;
            }
            req->ifconf.ifc_buf = buf;
            req->ifconf.ifc_len = buflen;

            if (buf != NULL)
                INIT_CHECKED_ARG(buf, buflen + 64, buflen);
            break;
        }

        case IOCTL_ARPREQ:
            reqlen = sizeof(req->arpreq);

            /* Copy protocol address for all requests */
            sockaddr_rpc2h(&(out->req.req_val[0].ioctl_request_u.
                                 req_arpreq.rpc_arp_pa),
                           &(req->arpreq.arp_pa),
                           sizeof(req->arpreq.arp_pa), NULL, NULL);
            if (in->code == RPC_SIOCSARP)
            {
                /* Copy HW address */
                sockaddr_rpc2h(&(out->req.req_val[0].ioctl_request_u.
                                     req_arpreq.rpc_arp_ha),
                               &(req->arpreq.arp_ha),
                               sizeof(req->arpreq.arp_ha), NULL, NULL);
                /* Copy ARP flags */
                req->arpreq.arp_flags =
                    arp_fl_rpc2h(out->req.req_val[0].ioctl_request_u.
                                     req_arpreq.rpc_arp_flags);
            }

#if HAVE_STRUCT_ARPREQ_ARP_DEV
            if (in->code == RPC_SIOCGARP)
            {
                /* Copy device */
                strcpy(req->arpreq.arp_dev,
                       out->req.req_val[0].ioctl_request_u.
                           req_arpreq.rpc_arp_dev.rpc_arp_dev_val);
            }
#endif
            break;
#ifdef HAVE_STRUCT_SG_IO_HDR
        case IOCTL_SGIO:
            {
                size_t psz = getpagesize();
                reqlen = sizeof(struct sg_io_hdr);

                req->sg.interface_id = out->req.req_val[0].ioctl_request_u.
                    req_sgio.interface_id;
                req->sg.dxfer_direction = out->req.req_val[0].
                    ioctl_request_u.req_sgio.dxfer_direction;
                req->sg.cmd_len = out->req.req_val[0].ioctl_request_u.
                    req_sgio.cmd_len;
                req->sg.mx_sb_len = out->req.req_val[0].ioctl_request_u.
                    req_sgio.mx_sb_len;
                req->sg.iovec_count = out->req.req_val[0].ioctl_request_u.
                    req_sgio.iovec_count;

                req->sg.dxfer_len = out->req.req_val[0].ioctl_request_u.
                    req_sgio.dxfer_len;

                req->sg.flags = out->req.req_val[0].ioctl_request_u.
                    req_sgio.flags;

                req->sg.dxferp = calloc(req->sg.dxfer_len + psz, 1);
                if ((req->sg.flags & SG_FLAG_DIRECT_IO) ==
                    SG_FLAG_DIRECT_IO)
                {
                    req->sg.dxferp =
                        (unsigned char *)
                        (((unsigned long)req->sg.dxferp + psz - 1) &
                                          (~(psz - 1)));
                }
                memcpy(req->sg.dxferp, out->req.req_val[0].ioctl_request_u.
                       req_sgio.dxferp.dxferp_val,
                       req->sg.dxfer_len);

                req->sg.cmdp = calloc(req->sg.cmd_len, 1);
                memcpy(req->sg.cmdp, out->req.req_val[0].ioctl_request_u.
                       req_sgio.cmdp.cmdp_val,
                       req->sg.cmd_len);

                req->sg.sbp = calloc(req->sg.mx_sb_len, 1);
                memcpy(req->sg.sbp, out->req.req_val[0].ioctl_request_u.
                       req_sgio.sbp.sbp_val,
                       req->sg.mx_sb_len);


                req->sg.timeout = out->req.req_val[0].ioctl_request_u.
                    req_sgio.timeout;
                req->sg.pack_id = out->req.req_val[0].ioctl_request_u.
                    req_sgio.pack_id;

                break;
            }
#endif /* HAVE_STRUCT_SG_IO_HDR */
        default:
            ERROR("Incorrect request type %d is received",
                  out->req.req_val[0].type);
            out->common._errno = TE_RC(TE_TA_UNIX, TE_EINVAL);
            return;
    }
    if (in->access == IOCTL_WR)
        INIT_CHECKED_ARG(req, reqlen, 0);
}

static void
tarpc_ioctl_post(tarpc_ioctl_in *in, tarpc_ioctl_out *out,
                 ioctl_param *req)
{
    switch (out->req.req_val[0].type)
    {
        case IOCTL_INT:
            out->req.req_val[0].ioctl_request_u.req_int = req->integer;
            break;

        case IOCTL_TIMEVAL:
            out->req.req_val[0].ioctl_request_u.req_timeval.tv_sec =
                req->tv.tv_sec;
            out->req.req_val[0].ioctl_request_u.req_timeval.tv_usec =
                req->tv.tv_usec;
            break;

        case IOCTL_TIMESPEC:
            out->req.req_val[0].ioctl_request_u.req_timespec.tv_sec =
                req->ts.tv_sec;
            out->req.req_val[0].ioctl_request_u.req_timespec.tv_nsec =
                req->ts.tv_nsec;
            break;

        case IOCTL_IFREQ:
            switch (in->code)
            {
                case RPC_SIOCGIFFLAGS:
                case RPC_SIOCSIFFLAGS:
                    out->req.req_val[0].ioctl_request_u.req_ifreq.
                        rpc_ifr_flags = if_fl_h2rpc(
                            (uint32_t)(unsigned short int)
                                req->ifreq.ifr_flags);
                    break;

                case RPC_SIOCGIFMTU:
                case RPC_SIOCSIFMTU:
#if HAVE_STRUCT_IFREQ_IFR_MTU
                    out->req.req_val[0].ioctl_request_u.req_ifreq.
                        rpc_ifr_mtu = req->ifreq.ifr_mtu;
#else
                    WARN("'struct ifreq' has no 'ifr_mtu'");
#endif
                    break;

                case RPC_SIOCGIFNAME:
                    memcpy(out->req.req_val[0].ioctl_request_u.req_ifreq.
                           rpc_ifr_name.rpc_ifr_name_val,
                           req->ifreq.ifr_name,
                           sizeof(req->ifreq.ifr_name));
                    out->req.req_val[0].ioctl_request_u.req_ifreq.
                            rpc_ifr_name.rpc_ifr_name_len =
                            sizeof(req->ifreq.ifr_name);
                    break;

                    /* QNX does not support SIOCGIFINDEX */
#ifdef SIOCGIFINDEX
                case RPC_SIOCGIFINDEX:
#if SOLARIS
                    out->req.req_val[0].ioctl_request_u.req_ifreq.
                            rpc_ifr_ifindex = req->ifreq.ifr_index;
#else
                    out->req.req_val[0].ioctl_request_u.req_ifreq.
                            rpc_ifr_ifindex = req->ifreq.ifr_ifindex;
#endif
                    break;
#endif /* SIOCGIFINDEX */

                case RPC_SIOCGIFADDR:
                case RPC_SIOCSIFADDR:
                case RPC_SIOCGIFNETMASK:
                case RPC_SIOCSIFNETMASK:
                case RPC_SIOCGIFBRDADDR:
                case RPC_SIOCSIFBRDADDR:
                case RPC_SIOCGIFDSTADDR:
                case RPC_SIOCSIFDSTADDR:
                case RPC_SIOCGIFHWADDR:
                    TE_IOCTL_AF_LOCAL2ETHER(req->ifreq.ifr_addr.sa_family);
                    sockaddr_output_h2rpc(&(req->ifreq.ifr_addr),
                                          sizeof(req->ifreq.ifr_addr),
                                          sizeof(req->ifreq.ifr_addr),
                                          &(out->req.req_val[0].
                                              ioctl_request_u.
                                              req_ifreq.rpc_ifr_addr));
                    break;

#if HAVE_LINUX_ETHTOOL_H
                case RPC_SIOCETHTOOL:
                    ethtool_data_h2rpc(
                                    &(out->req.req_val[0].ioctl_request_u.
                                      req_ifreq.rpc_ifr_ethtool),
                                    req->ifreq.ifr_data);
                    free(req->ifreq.ifr_data);
                    break;
#endif /* HAVE_LINUX_ETHTOOL_H */
#if HAVE_LINUX_NET_TSTAMP_H
                case RPC_SIOCSHWTSTAMP:
                case RPC_SIOCGHWTSTAMP:
                    hwtstamp_config_data_h2rpc(
                                    &(out->req.req_val[0].ioctl_request_u.
                                      req_ifreq.rpc_ifr_hwstamp),
                                    req->ifreq.ifr_data);
                    free(req->ifreq.ifr_data);
                    break;
#endif /* HAVE_LINUX_NET_TSTAMP_H */
                default:
                    ERROR("Unsupported IOCTL request %d of type IFREQ",
                          in->code);
                    out->common._errno = TE_RC(TE_TA_UNIX, TE_EINVAL);
                    return;
            }
            break;

        case IOCTL_IFCONF:
        {
            struct ifreq       *req_c;
            struct tarpc_ifreq *req_t;

            int n = 1;
            int i;

            n = out->req.req_val[0].ioctl_request_u.req_ifconf.nmemb =
                req->ifconf.ifc_len / sizeof(struct ifreq);
            out->req.req_val[0].ioctl_request_u.req_ifconf.extra =
                req->ifconf.ifc_len % sizeof(struct ifreq);

            if (req->ifconf.ifc_req == NULL)
                break;

            if ((req_t = calloc(n, sizeof(*req_t))) == NULL)
            {
                free(req->ifconf.ifc_buf);
                ERROR("Out of memory");
                out->common._errno = TE_RC(TE_TA_UNIX, TE_ENOMEM);
                return;
            }
            out->req.req_val[0].ioctl_request_u.req_ifconf.
                rpc_ifc_req.rpc_ifc_req_val = req_t;
            out->req.req_val[0].ioctl_request_u.req_ifconf.
                rpc_ifc_req.rpc_ifc_req_len = n;
            req_c = ((struct ifconf *)req)->ifc_req;

            for (i = 0; i < n; i++, req_t++, req_c++)
            {
                req_t->rpc_ifr_name.rpc_ifr_name_val =
                    calloc(1, sizeof(req_c->ifr_name));
                if (req_t->rpc_ifr_name.rpc_ifr_name_val == NULL)
                {
                    free(req->ifconf.ifc_buf);
                    ERROR("Out of memory");
                    out->common._errno = TE_RC(TE_TA_UNIX, TE_ENOMEM);
                    return;
                }
                memcpy(req_t->rpc_ifr_name.rpc_ifr_name_val,
                       req_c->ifr_name, sizeof(req_c->ifr_name));
                req_t->rpc_ifr_name.rpc_ifr_name_len =
                    sizeof(req_c->ifr_name);

                sockaddr_output_h2rpc(&(req_c->ifr_addr),
                                      sizeof(req_c->ifr_addr),
                                      sizeof(req_c->ifr_addr),
                                      &(req_t->rpc_ifr_addr));
            }
            free(req->ifconf.ifc_buf);
            break;
        }

        case IOCTL_ARPREQ:
        {
            if (in->code == RPC_SIOCGARP)
            {
                /* Copy protocol address */
                sockaddr_output_h2rpc(&(req->arpreq.arp_pa),
                                      sizeof(req->arpreq.arp_pa),
                                      sizeof(req->arpreq.arp_pa),
                                      &(out->req.req_val[0].ioctl_request_u.
                                          req_arpreq.rpc_arp_pa));
                TE_IOCTL_AF_LOCAL2ETHER(req->arpreq.arp_ha.sa_family);
                /* Copy HW address */
                sockaddr_output_h2rpc(&(req->arpreq.arp_ha),
                                      sizeof(req->arpreq.arp_ha),
                                      sizeof(req->arpreq.arp_ha),
                                      &(out->req.req_val[0].ioctl_request_u.
                                          req_arpreq.rpc_arp_ha));

                /* Copy flags */
                out->req.req_val[0].ioctl_request_u.req_arpreq.
                    rpc_arp_flags = arp_fl_h2rpc(req->arpreq.arp_flags);
            }
            break;
        }
#ifdef HAVE_STRUCT_SG_IO_HDR
        case IOCTL_SGIO:
        {
            out->req.req_val[0].ioctl_request_u.req_sgio.
                status = req->sg.status;
            out->req.req_val[0].ioctl_request_u.req_sgio.
                masked_status = req->sg.masked_status;
            out->req.req_val[0].ioctl_request_u.req_sgio.
                msg_status = req->sg.msg_status;
            out->req.req_val[0].ioctl_request_u.req_sgio.
                sb_len_wr = req->sg.sb_len_wr;
            out->req.req_val[0].ioctl_request_u.req_sgio.
                host_status = req->sg.host_status;
            out->req.req_val[0].ioctl_request_u.req_sgio.
                driver_status = req->sg.driver_status;
            out->req.req_val[0].ioctl_request_u.req_sgio.
                resid = req->sg.resid;
            out->req.req_val[0].ioctl_request_u.req_sgio.
                duration = req->sg.duration;
            out->req.req_val[0].ioctl_request_u.req_sgio.
                info = req->sg.info;
            break;
        }
#endif /* HAVE_STRUCT_SG_IO_HDR */
        default:
            assert(FALSE);
    }
}

TARPC_FUNC(ioctl,
{
    COPY_ARG(req);
},
{
    ioctl_param  req_local;
    void        *req_ptr;

    if (out->req.req_val != NULL)
    {
        memset(&req_local, 0, sizeof(req_local));
        req_ptr = &req_local;
        tarpc_ioctl_pre(in, out, req_ptr, arglist);
        if (out->common._errno != 0)
            goto finish;
    }
    else
    {
        req_ptr = NULL;
    }

    MAKE_CALL(out->retval = func(in->s, ioctl_rpc2h(in->code), req_ptr));
    if (req_ptr != NULL)
    {
        tarpc_ioctl_post(in, out, req_ptr);
    }

finish:
    ;
}
)


static const char *
msghdr2str(const struct msghdr *msg)
{
    static char buf[256];

    char   *buf_end = buf + sizeof(buf);
    char   *p = buf;
    int     i;

    p += snprintf(p, buf_end - p, "{name={0x%lx,%u},{",
                  (long unsigned int)msg->msg_name, msg->msg_namelen);
    if (p >= buf_end)
        return "(too long)";
    for (i = 0; i < (int)msg->msg_iovlen; ++i)
    {
        p += snprintf(p, buf_end - p, "%s{0x%lx,%u}",
                      (i == 0) ? "" : ",",
                      (long unsigned int)msg->msg_iov[i].iov_base,
                      (unsigned int)msg->msg_iov[i].iov_len);
        if (p >= buf_end)
            return "(too long)";
    }
    p += snprintf(p, buf_end - p, "},control={0x%lx,%u},flags=0x%x}",
                  (unsigned long int)msg->msg_control,
                  (unsigned int)msg->msg_controllen,
                  (unsigned int)msg->msg_flags);
    if (p >= buf_end)
        return "(too long)";

    return buf;
}

#if !HAVE_STRUCT_MMSGHDR
struct mmsghdr {
    struct msghdr msg_hdr;  /* Message header */
    unsigned int  msg_len;  /* Number of received bytes for header */
};
#endif

static const char *
mmsghdr2str(const struct mmsghdr *mmsg, int len)
{
    int          i;
    static char  buf[256];
    char        *buf_end = buf + sizeof(buf);
    char        *p = buf;

    for (i = 0; i < len; i++)
    {
        p += snprintf(p, buf_end - p, "%s{%s, %d}%s%s",
                      (i == 0) ? "{" : "",
                      msghdr2str(&mmsg[i].msg_hdr), mmsg[i].msg_len,
                      (i == 0) ? "" : ",", (i == len - 1) ? "" : "}");
        if (p >= buf_end)
            return "(too long)";
    }
    return buf;
}

/** Calculate the auxiliary buffer length for msghdr */
static inline int
calculate_msg_controllen(struct tarpc_msghdr *rpc_msg)
{
    unsigned int i;
    int          len = 0;

    for (i = 0; i < rpc_msg->msg_control.msg_control_len; i++)
        len += CMSG_SPACE(rpc_msg->msg_control.msg_control_val[i].
                          data.data_len);

    return len;
}

/*-------------- sendmsg() ------------------------------*/

TARPC_FUNC(sendmsg, {},
{
    rpcs_msghdr_helper   msg_helper;
    struct msghdr        msg;
    te_errno             rc;

    memset(&msg_helper, 0, sizeof(msg_helper));
    memset(&msg, 0, sizeof(msg));

    if (in->msg.msg_val == NULL)
    {
        MAKE_CALL(out->retval = func(in->s, NULL,
                                     send_recv_flags_rpc2h(in->flags)));
    }
    else
    {
        struct tarpc_msghdr *rpc_msg = in->msg.msg_val;

        rc = rpcs_msghdr_tarpc2h(RPCS_MSGHDR_CHECK_ARGS_SEND,
                                 rpc_msg, &msg_helper,
                                 &msg, arglist, "msg");
        if (rc != 0)
        {
            out->common._errno = TE_RC(TE_TA_UNIX, rc);
            goto finish;
        }

        VERB("sendmsg(): s=%d, msg=%s, flags=0x%x", in->s,
             msghdr2str(&msg), send_recv_flags_rpc2h(in->flags));

        MAKE_CALL(out->retval = func(in->s, &msg,
                                     send_recv_flags_rpc2h(in->flags)));
    }

finish:

    rpcs_msghdr_helper_clean(&msg_helper, &msg);
}
)

/*-------------- recvmsg() ------------------------------*/

TARPC_FUNC(recvmsg,
{
    COPY_ARG(msg);
},
{
    rpcs_msghdr_helper   msg_helper;
    struct msghdr        msg;
    te_errno             rc;

    memset(&msg_helper, 0, sizeof(msg_helper));
    memset(&msg, 0, sizeof(msg));

    rc = rpcs_msghdr_tarpc2h(RPCS_MSGHDR_CHECK_ARGS_RECV, out->msg.msg_val,
                             &msg_helper, &msg, arglist, "msg");
    if (rc != 0)
    {
        out->common._errno = TE_RC(TE_TA_UNIX, rc);
        goto finish;
    }

    VERB("recvmsg(): in msg=%s", msghdr2str(&msg));
    MAKE_CALL(out->retval = func(in->s, &msg,
                                 send_recv_flags_rpc2h(in->flags)));
    VERB("recvmsg(): out msg=%s", msghdr2str(&msg));

    rc = rpcs_msghdr_h2tarpc(&msg, &msg_helper, out->msg.msg_val);
    if (rc != 0)
        out->common._errno = TE_RC(TE_TA_UNIX, rc);

finish:

    rpcs_msghdr_helper_clean(&msg_helper, &msg);
}
)

/*-------------- poll() --------------------------------*/

/**
 * Dynamically resolve and call poll() or __poll_chk().
 *
 * @param fds         Array of poll event structures.
 * @param nfds        Number of elements in the array.
 * @param timeout     Poll timeout in ms.
 * @param chk_func    If @c TRUE, use __poll_chk() instead of poll().
 * @param fdslen      Size of memory occupied by @p fds (makes sense only
 *                    for __poll_chk()).
 * @param lib_flags   Flags for dynamic function resolution.
 *
 * @return Value returned by the target function or
 *         @c -1 in case of failure.
 */
static int
poll_rpc_handler(struct pollfd *fds, unsigned int nfds,
                 int timeout, te_bool chk_func, size_t fdslen,
                 tarpc_lib_flags lib_flags)
{
    api_func_ptr poll_func;
    const char *func_name = (chk_func ? "__poll_chk" : "poll");

    TARPC_FIND_FUNC_RETURN(lib_flags, func_name, (api_func *)&poll_func);

    if (chk_func)
        return poll_func(fds, nfds, timeout, fdslen);
    else
        return poll_func(fds, nfds, timeout);
}

TARPC_FUNC_STANDALONE(poll,
{
    if (in->ufds.ufds_len > RPC_POLL_NFDS_MAX)
    {
        ERROR("Too big nfds is passed to the poll()");
        out->common._errno = TE_RC(TE_TA_UNIX, TE_ENOMEM);
        return TRUE;
    }
    COPY_ARG(ufds);
},
{
    struct pollfd ufds[RPC_POLL_NFDS_MAX];

    unsigned int i;

    VERB("poll(): IN ufds=0x%lx[%u] nfds=%u timeout=%d",
         (unsigned long int)out->ufds.ufds_val, out->ufds.ufds_len,
         in->nfds, in->timeout);
    for (i = 0; i < out->ufds.ufds_len; i++)
    {
        ufds[i].fd = out->ufds.ufds_val[i].fd;
        INIT_CHECKED_ARG((char *)&(ufds[i].fd), sizeof(ufds[i].fd), 0);
        ufds[i].events = poll_event_rpc2h(out->ufds.ufds_val[i].events);
        INIT_CHECKED_ARG((char *)&(ufds[i].events),
                         sizeof(ufds[i].events), 0);
        ufds[i].revents = poll_event_rpc2h(out->ufds.ufds_val[i].revents);
        VERB("poll(): IN fd=%d events=%hx(rpc %hx) revents=%hx",
             ufds[i].fd, ufds[i].events, out->ufds.ufds_val[i].events,
             ufds[i].revents);
    }

    VERB("poll(): call with ufds=0x%lx, nfds=%u, timeout=%d",
         (unsigned long int)ufds, in->nfds, in->timeout);
    MAKE_CALL(out->retval = poll_rpc_handler(
                                ufds, in->nfds, in->timeout, in->chk_func,
                                out->ufds.ufds_len * sizeof(struct pollfd),
                                in->common.lib_flags));
    VERB("poll(): retval=%d", out->retval);

    for (i = 0; i < out->ufds.ufds_len; i++)
    {
        out->ufds.ufds_val[i].revents = poll_event_h2rpc(ufds[i].revents);
        VERB("poll(): OUT host-revents=%hx rpc-revents=%hx",
             ufds[i].revents, out->ufds.ufds_val[i].revents);
    }
}
)

/*-------------- ppoll() --------------------------------*/

/**
 * Dynamically resolve and call ppoll() or __ppoll_chk().
 *
 * @param fds         Array of poll event structures.
 * @param nfds        Number of elements in the array.
 * @param ts          Poll timeout.
 * @param sigmask     If not @c NULL, this signal mask is set
 *                    for ppoll() call.
 * @param chk_func    If @c TRUE, use __ppoll_chk() instead of ppoll().
 * @param fdslen      Size of memory occupied by @p fds (makes sense only
 *                    for __ppoll_chk()).
 * @param lib_flags   Flags for dynamic function resolution.
 *
 * @return Value returned by the target function or
 *         @c -1 in case of failure.
 */
static int
ppoll_rpc_handler(struct pollfd *fds, unsigned int nfds,
                  const struct timespec *ts, const sigset_t *sigmask,
                  te_bool chk_func, size_t fdslen,
                  tarpc_lib_flags lib_flags)
{
    api_func_ptr ppoll_func;
    const char *func_name = (chk_func ? "__ppoll_chk" : "ppoll");

    TARPC_FIND_FUNC_RETURN(lib_flags, func_name, (api_func *)&ppoll_func);

    if (chk_func)
        return ppoll_func(fds, nfds, ts, sigmask, fdslen);
    else
        return ppoll_func(fds, nfds, ts, sigmask);
}

TARPC_FUNC_STANDALONE(ppoll,
{
    if (in->ufds.ufds_len > RPC_POLL_NFDS_MAX)
    {
        ERROR("Too big nfds is passed to the ppoll()");
        out->common._errno = TE_RC(TE_TA_UNIX, TE_ENOMEM);
        return TRUE;
    }
    COPY_ARG(ufds);
    COPY_ARG(timeout);
},
{
    struct pollfd ufds[RPC_POLL_NFDS_MAX];
    struct timespec tv;
    unsigned int i;

    if (out->timeout.timeout_len > 0)
    {
        tv.tv_sec = out->timeout.timeout_val[0].tv_sec;
        tv.tv_nsec = out->timeout.timeout_val[0].tv_nsec;
    }
    INIT_CHECKED_ARG((char *)rcf_pch_mem_get(in->sigmask),
                     sizeof(sigset_t), 0);

    VERB("ppoll(): IN ufds=0x%lx[%u] nfds=%u",
         (unsigned long int)out->ufds.ufds_val, out->ufds.ufds_len,
         in->nfds);
    for (i = 0; i < out->ufds.ufds_len; i++)
    {
        ufds[i].fd = out->ufds.ufds_val[i].fd;
        INIT_CHECKED_ARG((char *)&(ufds[i].fd), sizeof(ufds[i].fd), 0);
        ufds[i].events = poll_event_rpc2h(out->ufds.ufds_val[i].events);
        INIT_CHECKED_ARG((char *)&(ufds[i].events),
                         sizeof(ufds[i].events), 0);
        ufds[i].revents = poll_event_rpc2h(out->ufds.ufds_val[i].revents);
        VERB("ppoll(): IN fd=%d events=%hx(rpc %hx) revents=%hx",
             ufds[i].fd, ufds[i].events, out->ufds.ufds_val[i].events,
             ufds[i].revents);
    }

    VERB("ppoll(): call with ufds=0x%lx, nfds=%u, timeout=%p",
         (unsigned long int)ufds, in->nfds,
         out->timeout.timeout_len > 0 ? out->timeout.timeout_val : NULL);
    MAKE_CALL(out->retval = ppoll_rpc_handler(
                               ufds, in->nfds,
                               out->timeout.timeout_len == 0 ? NULL :
                                                               &tv,
                               rcf_pch_mem_get(in->sigmask),
                               in->chk_func,
                               out->ufds.ufds_len * sizeof(struct pollfd),
                               in->common.lib_flags));
    VERB("ppoll(): retval=%d", out->retval);

    if (out->timeout.timeout_len > 0)
    {
        out->timeout.timeout_val[0].tv_sec = tv.tv_sec;
        out->timeout.timeout_val[0].tv_nsec = tv.tv_nsec;
    }

    for (i = 0; i < out->ufds.ufds_len; i++)
    {
        out->ufds.ufds_val[i].revents = poll_event_h2rpc(ufds[i].revents);
        VERB("ppoll(): OUT host-revents=%hx rpc-revents=%hx",
             ufds[i].revents, out->ufds.ufds_val[i].revents);
    }
}
)

#if HAVE_STRUCT_EPOLL_EVENT
/*-------------- epoll_create() ------------------------*/

TARPC_FUNC(epoll_create, {},
{
    MAKE_CALL(out->retval = func(in->size));
}
)

/*-------------- epoll_create1() ------------------------*/

TARPC_FUNC(epoll_create1, {},
{
    MAKE_CALL(out->retval = func(epoll_flags_rpc2h(in->flags)));
}
)

/*-------------- epoll_ctl() --------------------------------*/

TARPC_FUNC(epoll_ctl, {},
{
    struct epoll_event  event;
    struct epoll_event *ptr;

    if (in->event.event_len)
    {
        ptr = &event;
        event.events = epoll_event_rpc2h(in->event.event_val[0].events);
        /* TODO: Should be substituted by correct handling of union */
        event.data.fd = in->fd;
    }
    else
        ptr = NULL;

    VERB("epoll_ctl(): call with epfd=%d op=%d fd=%d event=0x%lx",
         in->epfd, in->op, in->fd,
         (in->event.event_len) ? (unsigned long int)in->event.event_val :
                                 0);

    MAKE_CALL(out->retval = func(in->epfd, in->op, in->fd, ptr));
    VERB("epoll_ctl(): retval=%d", out->retval);
}
)

/*-------------- epoll_wait() --------------------------------*/

TARPC_FUNC(epoll_wait,
{
    /* TODO: RPC_POLL_NFDS_MAX should be substituted */
    if (in->events.events_len > RPC_POLL_NFDS_MAX)
    {
        ERROR("Too many events is passed to the epoll_wait()");
        out->common._errno = TE_RC(TE_TA_UNIX, TE_ENOMEM);
        return TRUE;
    }
    COPY_ARG(events);
},
{
    /* TODO: RPC_POLL_NFDS_MAX should be substituted */
    struct epoll_event *events = NULL;
    int len = out->events.events_len;
    unsigned int i;

    if (len)
        events = calloc(len, sizeof(struct epoll_event));

    VERB("epoll_wait(): call with epfd=%d, events=0x%lx, maxevents=%d,"
         " timeout=%d",
         in->epfd, (unsigned long int)events, in->maxevents, in->timeout);
    MAKE_CALL(out->retval = func(in->epfd, events, in->maxevents,
                                 in->timeout));
    VERB("epoll_wait(): retval=%d", out->retval);

    for (i = 0; i < out->events.events_len; i++)
    {
        out->events.events_val[i].events =
            epoll_event_h2rpc(events[i].events);
        /* TODO: should be substituted by correct handling of union */
        out->events.events_val[i].data.type = TARPC_ED_INT;
        out->events.events_val[i].data.tarpc_epoll_data_u.fd =
            events[i].data.fd;
    }
    free(events);
}
)

/*-------------- epoll_pwait() --------------------------------*/

TARPC_FUNC(epoll_pwait,
{
    /* TODO: RPC_POLL_NFDS_MAX should be substituted */
    if (in->events.events_len > RPC_POLL_NFDS_MAX)
    {
        ERROR("Too many events is passed to the epoll_pwait()");
        out->common._errno = TE_RC(TE_TA_UNIX, TE_ENOMEM);
        return TRUE;
    }
    COPY_ARG(events);
},
{
    /* TODO: RPC_POLL_NFDS_MAX should be substituted */
    struct epoll_event *events = NULL;
    int len = out->events.events_len;
    unsigned int i;

    if (len)
        events = calloc(len, sizeof(struct epoll_event));

    VERB("epoll_pwait(): call with epfd=%d, events=0x%lx, maxevents=%d,"
         " timeout=%d sigmask=%p",
         in->epfd, (unsigned long int)events, in->maxevents, in->timeout,
         in->sigmask);

    /*
     * The pointer may be a NULL and, therefore, contain uninitialized
     * data, but we want to check that the data are unchanged even in
     * this case.
     */
    INIT_CHECKED_ARG((char *)rcf_pch_mem_get(in->sigmask),
                     sizeof(sigset_t), 0);

    MAKE_CALL(out->retval = func(in->epfd, events, in->maxevents,
                                 in->timeout,
                                 rcf_pch_mem_get(in->sigmask)));
    VERB("epoll_pwait(): retval=%d", out->retval);

    for (i = 0; i < out->events.events_len; i++)
    {
        out->events.events_val[i].events =
            epoll_event_h2rpc(events[i].events);
        /* TODO: should be substituted by correct handling of union */
        out->events.events_val[i].data.type = TARPC_ED_INT;
        out->events.events_val[i].data.tarpc_epoll_data_u.fd =
            events[i].data.fd;
    }
    free(events);
}
)
#endif

/**
 * Convert host representation of the hostent to the RPC one.
 * The memory is allocated by the routine.
 *
 * @param he   source structure
 *
 * @return RPC structure or NULL is memory allocation failed
 */
static tarpc_hostent *
hostent_h2rpc(struct hostent *he)
{
    tarpc_hostent *rpc_he = (tarpc_hostent *)calloc(1, sizeof(*rpc_he));

    unsigned int i;
    unsigned int k;

    if (rpc_he == NULL)
        return NULL;

    if (he->h_name != NULL)
    {
        if ((rpc_he->h_name.h_name_val = strdup(he->h_name)) == NULL)
            goto release;
        rpc_he->h_name.h_name_len = strlen(he->h_name) + 1;
    }

    if (he->h_aliases != NULL)
    {
        char **ptr;

        for (i = 1, ptr = he->h_aliases; *ptr != NULL; ptr++, i++);

        if ((rpc_he->h_aliases.h_aliases_val =
             (tarpc_h_alias *)calloc(i, sizeof(tarpc_h_alias))) == NULL)
        {
            goto release;
        }
        rpc_he->h_aliases.h_aliases_len = i;

        for (k = 0; k < i - 1; k++)
        {
            if ((rpc_he->h_aliases.h_aliases_val[k].name.name_val =
                 strdup((he->h_aliases)[k])) == NULL)
            {
                goto release;
            }
            rpc_he->h_aliases.h_aliases_val[k].name.name_len =
                strlen((he->h_aliases)[k]) + 1;
        }
    }

    rpc_he->h_addrtype = domain_h2rpc(he->h_addrtype);
    rpc_he->h_length = he->h_length;

    if (he->h_addr_list != NULL)
    {
        char **ptr;

        for (i = 1, ptr = he->h_addr_list; *ptr != NULL; ptr++, i++);

        if ((rpc_he->h_addr_list.h_addr_list_val =
             (tarpc_h_addr *)calloc(i, sizeof(tarpc_h_addr))) == NULL)
        {
            goto release;
        }
        rpc_he->h_addr_list.h_addr_list_len = i;

        for (k = 0; k < i - 1; k++)
        {
            if ((rpc_he->h_addr_list.h_addr_list_val[i].val.val_val =
                 calloc(1, rpc_he->h_length)) == NULL)
            {
                goto release;
            }
            rpc_he->h_addr_list.h_addr_list_val[i].val.val_len =
                rpc_he->h_length;
            memcpy(rpc_he->h_addr_list.h_addr_list_val[i].val.val_val,
                   he->h_addr_list[i], rpc_he->h_length);
        }
    }

    return rpc_he;

release:
    /* Release the memory in the case of failure */
    free(rpc_he->h_name.h_name_val);
    if (rpc_he->h_aliases.h_aliases_val != NULL)
    {
        for (i = 0; i < rpc_he->h_aliases.h_aliases_len - 1; i++)
             free(rpc_he->h_aliases.h_aliases_val[i].name.name_val);
        free(rpc_he->h_aliases.h_aliases_val);
    }
    if (rpc_he->h_addr_list.h_addr_list_val != NULL)
    {
        for (i = 0; i < rpc_he->h_addr_list.h_addr_list_len - 1; i++)
            free(rpc_he->h_addr_list.h_addr_list_val[i].val.val_val);
        free(rpc_he->h_addr_list.h_addr_list_val);
    }
    free(rpc_he);
    return NULL;
}

/*-------------- gethostbyname() -----------------------------*/

TARPC_FUNC(gethostbyname, {},
{
    struct hostent *he;

    MAKE_CALL(he = (struct hostent *)func_ptr_ret_ptr(in->name.name_val));
    if (he != NULL)
    {
        if ((out->res.res_val = hostent_h2rpc(he)) == NULL)
            out->common._errno = TE_RC(TE_TA_UNIX, TE_ENOMEM);
        else
            out->res.res_len = 1;
    }
}
)

/*-------------- gethostbyaddr() -----------------------------*/

TARPC_FUNC(gethostbyaddr, {},
{
    struct hostent *he;

    INIT_CHECKED_ARG(in->addr.val.val_val, in->addr.val.val_len, 0);

    MAKE_CALL(he = (struct hostent *)
                       func_ptr_ret_ptr(in->addr.val.val_val,
                                        in->addr.val.val_len,
                                        addr_family_rpc2h(in->type)));
    if (he != NULL)
    {
        if ((out->res.res_val = hostent_h2rpc(he)) == NULL)
            out->common._errno = TE_RC(TE_TA_UNIX, TE_ENOMEM);
        else
            out->res.res_len = 1;
    }
}
)


/*-------------- getaddrinfo() -----------------------------*/

/**
 * Convert host native addrinfo to the RPC one.
 *
 * @param ai            host addrinfo structure
 * @param rpc_ai        pre-allocated RPC addrinfo structure
 *
 * @return 0 in the case of success or -1 in the case of memory allocation
 * failure
 */
static int
ai_h2rpc(struct addrinfo *ai, struct tarpc_ai *ai_rpc)
{
    ai_rpc->flags = ai_flags_h2rpc(ai->ai_flags);
    ai_rpc->family = domain_h2rpc(ai->ai_family);
    ai_rpc->socktype = socktype_h2rpc(ai->ai_socktype);
    ai_rpc->protocol = proto_h2rpc(ai->ai_protocol);
    ai_rpc->addrlen = ai->ai_addrlen - SA_COMMON_LEN;

    sockaddr_output_h2rpc(ai->ai_addr, sizeof(*ai->ai_addr),
                          sizeof(*ai->ai_addr), &ai_rpc->addr);

    if (ai->ai_canonname != NULL)
    {
        if ((ai_rpc->canonname.canonname_val =
             strdup(ai->ai_canonname)) == NULL)
        {
            return -1;
        }
        ai_rpc->canonname.canonname_len = strlen(ai->ai_canonname) + 1;
    }

    return 0;
}

/* I do not understand, which function may be found by dynamic lookup */
TARPC_FUNC_STATIC(getaddrinfo, {},
{
    struct addrinfo  hints;
    struct addrinfo *info = NULL;
    struct addrinfo *res = NULL;

    struct sockaddr_storage addr;
    struct sockaddr        *a;

    memset(&hints, 0, sizeof(hints));
    if (in->hints.hints_val != NULL)
    {
        info = &hints;
        hints.ai_flags = ai_flags_rpc2h(in->hints.hints_val[0].flags);
        hints.ai_family = domain_rpc2h(in->hints.hints_val[0].family);
        hints.ai_socktype = socktype_rpc2h(in->hints.hints_val[0].socktype);
        hints.ai_protocol = proto_rpc2h(in->hints.hints_val[0].protocol);
        hints.ai_addrlen = in->hints.hints_val[0].addrlen + SA_COMMON_LEN;
        sockaddr_rpc2h(&(in->hints.hints_val[0].addr), SA(&addr),
                       sizeof(addr), &a, NULL);
        hints.ai_addr = a;
        hints.ai_canonname = in->hints.hints_val[0].canonname.canonname_val;
        INIT_CHECKED_ARG(in->hints.hints_val[0].canonname.canonname_val,
                         in->hints.hints_val[0].canonname.canonname_len, 0);
        hints.ai_next = NULL;
        INIT_CHECKED_ARG((char *)info, sizeof(*info), 0);
    }
    INIT_CHECKED_ARG(in->node.node_val, in->node.node_len, 0);
    INIT_CHECKED_ARG(in->service.service_val,
                     in->service.service_len, 0);
    MAKE_CALL(out->retval = func(in->node.node_val,
                                 in->service.service_val, info, &res));
    /* GLIBC getaddrinfo clean up errno on success */
    out->common.errno_changed = FALSE;
    if (out->retval != 0 && res != NULL)
    {
        out->common._errno = TE_RC(TE_TA_UNIX, TE_ECORRUPTED);
        res = NULL;
    }
    if (res != NULL)
    {
        int i;

        struct tarpc_ai *arr;

        for (i = 0, info = res; info != NULL; i++, info = info->ai_next);

        if ((arr = calloc(i, sizeof(*arr))) != NULL)
        {
            int k;

            for (k = 0, info = res; k < i; k++, info = info->ai_next)
            {
                if (ai_h2rpc(info, arr + k) < 0)
                {
                    for (k--; k >= 0; k--)
                    {
                        free(arr[k].canonname.canonname_val);
                    }
                    free(arr);
                    arr = NULL;
                    break;
                }
            }
        }
        if (arr == NULL)
        {
            out->common._errno = TE_RC(TE_TA_UNIX, TE_ENOMEM);
            freeaddrinfo(res);
        }
        else
        {
            out->mem_ptr = rcf_pch_mem_alloc(res);
            out->res.res_val = arr;
            out->res.res_len = i;
        }
    }
}
)

/*-------------- freeaddrinfo() -----------------------------*/
/* I do not understand, which function may be found by dynamic lookup */
TARPC_FUNC_STATIC(freeaddrinfo, {},
{
    MAKE_CALL(func(rcf_pch_mem_get(in->mem_ptr)));
    rcf_pch_mem_free(in->mem_ptr);
}
)

/*-------------- pipe() --------------------------------*/
TARPC_FUNC(pipe,
{
    COPY_ARG(filedes);
},
{
    MAKE_CALL(out->retval = func_ptr(out->filedes.filedes_len > 0 ?
                                     out->filedes.filedes_val : NULL));
}
)


/*-------------- pipe2() --------------------------------*/
TARPC_FUNC(pipe2,
{
    COPY_ARG(filedes);
},
{
    MAKE_CALL(out->retval = func_ptr(out->filedes.filedes_len > 0 ?
                                     out->filedes.filedes_val : NULL,
                                     in->flags));
}
)

/*-------------- socketpair() ------------------------------*/

TARPC_FUNC(socketpair,
{
    COPY_ARG(sv);
},
{
    MAKE_CALL(out->retval = func(domain_rpc2h(in->domain),
                                 socktype_rpc2h(in->type),
                                 proto_rpc2h(in->proto),
                                 (out->sv.sv_len > 0) ?
                                     out->sv.sv_val : NULL));
}
)

#ifndef TE_POSIX_FS_PROVIDED
/*-------------- open() --------------------------------*/
TARPC_FUNC(open, {},
{
    TARPC_ENSURE_NOT_NULL(path);
    MAKE_CALL(out->fd = func_ptr(in->path.path_val,
                                 fcntl_flags_rpc2h(in->flags),
                                 file_mode_flags_rpc2h(in->mode)));
}
)
#endif

/*-------------- open64() --------------------------------*/
TARPC_FUNC(open64, {},
{
    TARPC_ENSURE_NOT_NULL(path);
    MAKE_CALL(out->fd = func_ptr(in->path.path_val,
                                 fcntl_flags_rpc2h(in->flags),
                                 file_mode_flags_rpc2h(in->mode)));
}
)

/*-------------- fopen() --------------------------------*/
TARPC_FUNC(fopen, {},
{
    MAKE_CALL(out->mem_ptr =
                  rcf_pch_mem_alloc(func_ptr_ret_ptr(in->path,
                                                     in->mode)));
}
)

/*-------------- fdopen() --------------------------------*/
TARPC_FUNC(fdopen, {},
{
    MAKE_CALL(out->mem_ptr =
                  rcf_pch_mem_alloc(func_ret_ptr(in->fd,
                                                     in->mode)));
}
)

/*-------------- fclose() -------------------------------*/
TARPC_FUNC(fclose, {},
{
    MAKE_CALL(out->retval = func_ptr(rcf_pch_mem_get(in->mem_ptr)));
    rcf_pch_mem_free(in->mem_ptr);
}
)

/*-------------- fileno() --------------------------------*/
TARPC_FUNC(fileno, {},
{
    MAKE_CALL(out->fd = func_ptr(rcf_pch_mem_get(in->mem_ptr)));
}
)

/*-------------- popen() --------------------------------*/
TARPC_FUNC(popen, {},
{
    MAKE_CALL(out->mem_ptr =
                  rcf_pch_mem_alloc(func_ptr_ret_ptr(in->cmd,
                                                     in->mode)));
}
)

/*-------------- pclose() -------------------------------*/
TARPC_FUNC(pclose, {},
{
    MAKE_CALL(out->retval = func_ptr(rcf_pch_mem_get(in->mem_ptr)));
    rcf_pch_mem_free(in->mem_ptr);
}
)

/*-------------- te_shell_cmd() --------------------------------*/
TARPC_FUNC(te_shell_cmd, {},
{
    MAKE_CALL(out->pid =
              func_ptr(in->cmd.cmd_val, in->uid,
                       in->in_fd ? &out->in_fd : NULL,
                       in->out_fd ? &out->out_fd : NULL,
                       in->err_fd ? &out->err_fd : NULL));
}
)

/*-------------- system() ----------------------------------*/
TARPC_FUNC_STANDALONE(system, {},
{
    int             st;
    rpc_wait_status r_st;

    MAKE_CALL(st = ta_system(in->cmd.cmd_val));
    r_st = wait_status_h2rpc(st);
    out->status_flag = r_st.flag;
    out->status_value = r_st.value;
}
)

/*-------------- chroot() --------------------------------*/
TARPC_FUNC(chroot, {},
{
    char *chroot_path = NULL;
    char *ta_dir_path = NULL;
    char *ta_execname_path = NULL;
    char *port_path = getenv("TE_RPC_PORT");

    chroot_path = realpath(in->path.path_val, NULL);
    ta_dir_path = realpath(ta_dir, NULL);
    ta_execname_path = realpath(ta_execname, NULL);
    port_path = realpath(port_path, NULL);

    if (chroot_path == NULL || ta_dir_path == NULL ||
        ta_execname_path == NULL || port_path == NULL)
    {
        if (chroot_path == NULL)
            ERROR("%s(): failed to determine absolute path of "
                  "chroot() argument", __FUNCTION__);
        if (ta_dir_path == NULL)
            ERROR("%s(): failed to determine absolute path of ta_dir",
                  __FUNCTION__);
        if (ta_execname_path == NULL)
            ERROR("%s(): failed to determine absolute path "
                  "of ta_execname", __FUNCTION__);
        /**
         * Path for port can be undefined if we do not use
         * AF_UNIX sockets for communication.
         */
        out->common._errno = TE_RC(TE_TA_UNIX, TE_ENOMEM);
        out->retval = -1;
        goto finish;
    }

    if (strstr(ta_dir_path, chroot_path) != ta_dir_path ||
        strstr(ta_execname_path, chroot_path) != ta_execname_path ||
        (port_path != NULL && strstr(port_path, chroot_path) != port_path))
    {
        ERROR("%s(): argument of chroot() must be such that TA "
              "folder is inside new root tree");
        out->common._errno = TE_RC(TE_TA_UNIX, TE_EINVAL);
        out->retval = -1;
        goto finish;
    }

    MAKE_CALL(out->retval = func_ptr(chroot_path));

    if (out->retval == 0)
    {
        /*
         * Change paths used by TE so that they will be
         * inside a new root.
         */

        strcpy(ta_dir, ta_dir_path + strlen(chroot_path));
        strcpy((char *)ta_execname,
               ta_execname_path + strlen(chroot_path));
        if (port_path != NULL)
            setenv("TE_RPC_PORT",
                   strdup(port_path + strlen(chroot_path)), 1);
   }

finish:

    free(chroot_path);
    free(ta_dir_path);
    free(ta_execname_path);
    free(port_path);
}
)

/*-------------- copy_ta_libs ---------------------------*/
/** Maximum shell command lenght */
#define MAX_CMD 1000

/**
 * Check that string was not truncated.
 *
 * @param _call     snprintf() call
 * @param _str      String
 * @param _size     Maximum available size
 */
#define CHECK_SNPRINTF(_call, _str, _size) \
    do {                                        \
        int _rc;                                \
        _rc = _call;                            \
        if (_rc >= (int)(_size))                \
        {                                       \
            ERROR("%s(): %s was truncated",     \
                  __FUNCTION__, (_str));        \
            return -1;                          \
        }                                       \
    } while (0)

/**
 * Call system() and check result.
 *
 * @param _cmd  Shell command to execute
 */
#define SYSTEM(_cmd) \
    if (system(_cmd) < 0)                               \
    {                                                   \
        if (errno == ECHILD)                            \
            errno = 0;                                  \
        else                                            \
        {                                               \
            ERROR("%s(): system(%s) failed with %r",    \
                  __FUNCTION__, cmd, errno);            \
            return -1;                                  \
        }                                               \
    }

/**
 * Obtain string wihtout spaces on both ends.
 *
 * @param str   String
 *
 * @return Pointer to first non-space position in
 *         str.
 */
char *
trim(char *str)
{
    int i = 0;

    for (i = strlen(str) - 1; i >= 0; i--)
    {
        if (str[i] == ' ' || str[i] == '\t'
            || str[i] == '\n' || str[i] == '\r')
            str[i] = '\0';
        else
            break;
    }

    for (i = 0; i < (int)strlen(str); i++)
        if (str[i] != ' ' && str[i] != '\t')
            break;

    return str + i;
}

/**
 * Copy shared libraries to TA folder.
 *
 * @param path Path to TA folder.
 *
 * @return 0 on success or -1
 */
int
copy_ta_libs(char *path)
{
    char    path_to_lib[RCF_MAX_PATH];
    char    path_to_chmod[RCF_MAX_PATH];
    char    str[MAX_CMD];
    char    cmd[MAX_CMD];
    char   *begin_path = 0;
    te_bool was_cut = FALSE;
    char   *s;
    FILE   *f;
    FILE   *f_list;
    te_bool ld_found = FALSE;
    int     saved_errno = errno;

    struct stat file_stat;

    errno = 0;

    CHECK_SNPRINTF(
        snprintf(str, MAX_CMD, "%s/ta_libs_list", path),
        str, MAX_CMD);
    f_list = fopen(str, "w");
    if (f_list == NULL)
    {
        ERROR("%s(): failed to create file to store list of libs",
              __FUNCTION__);
        return -1;
    }

    CHECK_SNPRINTF(snprintf(cmd, MAX_CMD,
                            "(ldd %s | sed \"s/.*=>[ \t]*//\" "
                            "| sed \"s/(0x[0-9a-f]*)$//\")", ta_execname),
                   cmd, MAX_CMD);

    if (dynamic_library_set && strlen(dynamic_library_name) != 0)
        CHECK_SNPRINTF(
                snprintf(cmd + strlen(cmd), MAX_CMD - strlen(cmd),
                         " && (ldd %s | sed \"s/.*=>[ \t]*//\" "
                         "| sed \"s/(0x[0-9a-f]*)$//\") && "
                         "(echo \"%s\")", dynamic_library_name,
                         dynamic_library_name),
                cmd + strlen(cmd), MAX_CMD - strlen(cmd));

    f = popen(cmd, "r");
    if (f == NULL)
    {
        ERROR("%s(): failed to obtain ldd output for TA", __FUNCTION__);
        return -1;
    }

    while (fgets(str, RCF_MAX_PATH, f) != NULL)
    {
        begin_path = trim(str);

        if (strstr(begin_path, "/ld-") != NULL ||
            strstr(begin_path, "/ld.") != NULL)
            ld_found = TRUE;

        if (stat(begin_path, &file_stat) >= 0)
        {
            CHECK_SNPRINTF(snprintf(path_to_lib, RCF_MAX_PATH,
                                    "%s/%s", path, begin_path),
                           path_to_lib, RCF_MAX_PATH);

            fprintf(f_list, "%s\n", path_to_lib);
            was_cut = FALSE;

            while ((s = strrchr(path_to_lib, '/')) != NULL)
            {
                if (stat(path_to_lib, &file_stat) >= 0)
                    break;
                *s = '\0';
                was_cut = TRUE;
            }
            if (was_cut)
            {
                s = path_to_lib + strlen(path_to_lib);
                *s = '/';
            }

            fprintf(f_list, "%s\n", path_to_lib);
            memcpy(path_to_chmod, path_to_lib, RCF_MAX_PATH);

            CHECK_SNPRINTF(snprintf(path_to_lib, RCF_MAX_PATH, "%s/%s",
                                    path, begin_path),
                           path_to_lib, RCF_MAX_PATH);
            s = strrchr(path_to_lib, '/');

            if (s == NULL)
            {
                ERROR("%s(): incorrect path %s", __FUNCTION__,
                      path_to_lib);
                return -1;
            }
            else
                *s = '\0';

            CHECK_SNPRINTF(snprintf(cmd, MAX_CMD, "mkdir -p \"%s\" && "
                                    "cp \"%s\" \"%s\" && "
                                    "chmod -R a+rwx \"%s\"",
                                    path_to_lib, begin_path, path_to_lib,
                                    path_to_chmod),
                           cmd, MAX_CMD);
            SYSTEM(cmd);
        }
    }

    if (!ld_found)
    {
        CHECK_SNPRINTF(snprintf(cmd, MAX_CMD, "cp /lib/ld.* \"%s/lib\"",
                                path),
                       cmd, MAX_CMD);
        SYSTEM(cmd);
    }

    if (pclose(f) < 0)
    {
        if (errno == ECHILD)
            errno = 0;
        else
        {
            ERROR("%s(): pclose() failed with %r",
                  __FUNCTION__, errno);
            return -1;
        }
    }

    fclose(f_list);

    if (errno == 0)
        errno = saved_errno;
    return 0;
}

TARPC_FUNC(copy_ta_libs, {},
{
    MAKE_CALL(out->retval = func_ptr(in->path.path_val));
}
)

/*-------------- rm_ta_libs ---------------------------*/
/**
 * Remove libraries copied by copy_ta_libs().
 *
 * @param path From where to remove.
 *
 * @return 0 on success or -1
 */
int
rm_ta_libs(char *path)
{
    char    str[MAX_CMD];
    char    cmd[RCF_MAX_PATH];
    char   *s;
    FILE   *f_list;
    int     saved_errno = errno;

    errno = 0;

    CHECK_SNPRINTF(snprintf(str, MAX_CMD, "%s/ta_libs_list", path),
                            str, MAX_CMD);
    f_list = fopen(str, "r");
    if (f_list == NULL)
    {
        ERROR("%s(): failed to create file to store list of libs",
              __FUNCTION__);
        return -1;
    }

    while (fgets(str, RCF_MAX_PATH, f_list) != NULL)
    {
        s = trim(str);
        if (strstr(s, path) != s)
            ERROR("Attempt to delete %s not in TA folder", s);
        else
        {
            CHECK_SNPRINTF(snprintf(cmd, RCF_MAX_PATH, "rm -rf %s", s),
                           cmd, RCF_MAX_PATH);
            SYSTEM(cmd);
        }
    }

    fclose(f_list);
    CHECK_SNPRINTF(snprintf(cmd, RCF_MAX_PATH,
                           "rm -rf %s/ta_libs_list", ta_dir),
             cmd, RCF_MAX_PATH);
    SYSTEM(cmd);

    if (errno == 0)
        errno = saved_errno;
    return 0;
}

TARPC_FUNC(rm_ta_libs, {},
{
    MAKE_CALL(out->retval = func_ptr(in->path.path_val));
}
)

#undef SYSTEM
#undef MAX_CMD

/*-------------- vlan_get_parent----------------------*/
bool_t
_vlan_get_parent_1_svc(tarpc_vlan_get_parent_in *in,
                       tarpc_vlan_get_parent_out *out,
                       struct svc_req *rqstp)
{
    char *str;

    UNUSED(rqstp);
    memset(out, 0, sizeof(*out));
    VERB("PID=%d TID=%llu: Entry %s",
         (int)getpid(), (unsigned long long int)pthread_self(),
         "vlan_get_parent");

    if ((str = (char *)calloc(IF_NAMESIZE, 1)) == NULL)
    {
        out->common._errno = TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }
    else
    {
        out->ifname.ifname_val = str;
        out->ifname.ifname_len = IF_NAMESIZE;
    }

    out->common._errno = ta_vlan_get_parent(in->ifname.ifname_val,
                                            out->ifname.ifname_val);

    out->retval = (out->common._errno == 0) ? 0 : -1;

    return TRUE;
}

/*-------------- bond_get_slaves----------------------*/
bool_t
_bond_get_slaves_1_svc(tarpc_bond_get_slaves_in *in,
                       tarpc_bond_get_slaves_out *out,
                       struct svc_req *rqstp)
{
    int i;

    int          slaves_num = 0;
    tqh_strings  slaves;
    tqe_string  *slave;

    UNUSED(rqstp);
    TAILQ_INIT(&slaves);
    memset(out, 0, sizeof(*out));
    VERB("PID=%d TID=%llu: Entry %s",
         (int)getpid(), (unsigned long long int)pthread_self(),
         "bond_get_slaves");

    out->common._errno = ta_bond_get_slaves(in->ifname.ifname_val,
                                            &slaves, &slaves_num,
                                            NULL);
    if (out->common._errno != 0)
        goto cleanup;

    if ((out->slaves.slaves_val =
            (tarpc_ifname *)calloc(slaves_num,
                                   sizeof(tarpc_ifname))) == NULL)
    {
        out->common._errno = TE_RC(TE_TA_UNIX, TE_ENOMEM);
        goto cleanup;
    }

    out->slaves.slaves_len = slaves_num;

    slave = TAILQ_FIRST(&slaves);
    for (i = 0; i < slaves_num; i++)
    {
        if (slave == NULL)
        {
            ERROR("%s(): bond slaves number is wrong", __FUNCTION__);
            out->common._errno = TE_RC(TE_TA_UNIX, TE_EFAIL);
            goto cleanup;
        }

        strncpy(out->slaves.slaves_val[i].ifname, slave->v, IFNAMSIZ);
        if (out->slaves.slaves_val[i].ifname[IFNAMSIZ - 1] != '\0')
        {
            ERROR("%s(): interface name is too long", __FUNCTION__);
            out->slaves.slaves_val[i].ifname[IFNAMSIZ - 1] = '\0';
            out->common._errno = TE_RC(TE_TA_UNIX, TE_ESMALLBUF);
            goto cleanup;
        }

        slave = TAILQ_NEXT(slave, links);
    }

cleanup:

    out->retval = (out->common._errno == 0) ? 0 : -1;
    tq_strings_free(&slaves, &free);

    return TRUE;
}

/*-------------- getenv() --------------------------------*/
TARPC_FUNC(getenv, {},
{
    char *val;

    MAKE_CALL(val = func_ptr_ret_ptr(in->name));
    /*
     * fixme kostik: dirty hack as we can't encode
     * NULL string pointer - STRING differs from pointer
     * in RPC representation
     */
    out->val_null = (val == NULL);
    out->val = strdup(val ? val : "");
}
)

/*-------------- setenv() --------------------------------*/
TARPC_FUNC(setenv, {},
{
    MAKE_CALL(out->retval = func_ptr(in->name, in->val,
                                     (int)(in->overwrite)));
}
)

/*-------------- unsetenv() --------------------------------*/
TARPC_FUNC(unsetenv, {},
{
    MAKE_CALL(out->retval = func_ptr(in->name));
}
)


/*-------------- getpwnam() --------------------------------*/
#define PUT_STR(_field) \
        do {                                                            \
            out->passwd._field._field##_val = strdup(pw->pw_##_field);  \
            if (out->passwd._field._field##_val == NULL)                \
            {                                                           \
                ERROR("Failed to duplicate string '%s'",                \
                      pw->pw_##_field);                                 \
                out->common._errno = TE_RC(TE_TA_UNIX, TE_ENOMEM);      \
                return -1;                                              \
            }                                                           \
            out->passwd._field._field##_len =                           \
                strlen(out->passwd._field._field##_val) + 1;            \
        } while (0)

/**
 * Copy the content of 'struct passwd' to RPC output structure.
 *
 * @param out   RPC output structure to fill in
 * @param pw    system native 'struct passwd' structure
 *
 * @return Status of the operation
 * @retval 0   on success
 * @retval -1  on failure
 *
 * @note We added this function because some systems might not have
 *       all the fields and we need to track this. For example Android
 *       does not export 'gecos' field.
 */
static int
copy_passwd_struct(struct tarpc_getpwnam_out *out, struct passwd *pw)
{
    PUT_STR(name);
    PUT_STR(passwd);
    out->passwd.uid = pw->pw_uid;
    out->passwd.gid = pw->pw_gid;
#ifdef HAVE_STRUCT_PASSWD_PW_GECOS
    PUT_STR(gecos);
#endif
    PUT_STR(dir);
    PUT_STR(shell);

    return 0;
}

TARPC_FUNC(getpwnam, {},
{
    struct passwd *pw;

    MAKE_CALL(pw = (struct passwd *)func_ptr_ret_ptr(in->name.name_val));
    /* GLIBC getpwnam clean up errno on success */
    out->common.errno_changed = FALSE;

    if (pw != NULL)
    {
        copy_passwd_struct(out, pw);
    }
    else
    {
        ERROR("getpwnam() returned NULL");
    }

    if (!RPC_IS_ERRNO_RPC(out->common._errno))
    {
        free(out->passwd.name.name_val);
        free(out->passwd.passwd.passwd_val);
        free(out->passwd.gecos.gecos_val);
        free(out->passwd.dir.dir_val);
        free(out->passwd.shell.shell_val);
        memset(&(out->passwd), 0, sizeof(out->passwd));
    }
    ;
}
)

#undef PUT_STR

/*-------------- uname() --------------------------------*/

#define PUT_STR(_dst, _field)                                       \
        do {                                                        \
            out->buf._dst._dst##_val = strdup(uts._field);          \
            if (out->buf._dst._dst##_val == NULL)                   \
            {                                                       \
                ERROR("Failed to duplicate string '%s'",            \
                      uts._field);                                  \
                out->common._errno = TE_RC(TE_TA_UNIX, TE_ENOMEM);  \
                goto finish;                                        \
            }                                                       \
            out->buf._dst._dst##_len =                              \
                strlen(out->buf._dst._dst##_val) + 1;               \
        } while (0)

TARPC_FUNC(uname, {},
{
    struct utsname uts;

    UNUSED(in);

    MAKE_CALL(out->retval = func_ptr(&uts));
/* inequality because Solaris' uname() returns
 * "non-negative value" in case of success
 */
    if (out->retval >= 0)
    {
        out->retval = 0;
        PUT_STR(sysname, sysname);
        PUT_STR(nodename, nodename);
        PUT_STR(release, release);
        PUT_STR(osversion, version);
        PUT_STR(machine, machine);
    }
    else
    {
        ERROR("uname() returned error");
    }
finish:
    if (!RPC_IS_ERRNO_RPC(out->common._errno))
    {
        free(out->buf.sysname.sysname_val);
        free(out->buf.nodename.nodename_val);
        free(out->buf.release.release_val);
        free(out->buf.osversion.osversion_val);
        free(out->buf.machine.machine_val);
        memset(&(out->buf), 0, sizeof(out->buf));
    }
    ;
}
)

#undef PUT_STR


/*-------------- getuid() --------------------------------*/
TARPC_FUNC(getuid, {}, { MAKE_CALL(out->uid = func_void()); })

/*-------------- getuid() --------------------------------*/
TARPC_FUNC(geteuid, {}, { MAKE_CALL(out->uid = func_void()); })

/*-------------- setuid() --------------------------------*/
TARPC_FUNC(setuid, {}, { MAKE_CALL(out->retval = func(in->uid)); })

/*-------------- seteuid() --------------------------------*/
TARPC_FUNC(seteuid, {}, { MAKE_CALL(out->retval = func(in->uid)); })


#ifdef WITH_TR069_SUPPORT
#include "acse_rpc.h"
/*-------------- cwmp_op_call() -------------------*/
TARPC_FUNC(cwmp_op_call, {},
{
    MAKE_CALL(func_ptr(in, out));
}
)

/*-------------- cwmp_op_check() -------------------*/
TARPC_FUNC(cwmp_op_check, {},
{
    MAKE_CALL(func_ptr(in, out));
}
)

/*-------------- cwmp_conn_req() -------------------*/
TARPC_FUNC(cwmp_conn_req, {}, { MAKE_CALL(func_ptr(in, out)); })

/*-------------- cwmp_acse_start() -------------------*/
TARPC_FUNC(cwmp_acse_start, {}, { MAKE_CALL(func_ptr(in, out)); })

#endif

/*-------------- simple_sender() -------------------------*/
/**
 * Simple sender.
 *
 * @param in                input RPC argument
 *
 * @return number of sent bytes or -1 in the case of failure
 */
int
simple_sender(tarpc_simple_sender_in *in, tarpc_simple_sender_out *out)
{
    int         errno_save = errno;
    api_func    send_func;
    char       *buf;

    int size = rand_range(in->size_min, in->size_max);
    int delay = rand_range(in->delay_min, in->delay_max);

    time_t start;
    time_t now;

#ifdef TA_DEBUG
    uint64_t control = 0;
#endif

    out->bytes = 0;

    RING("%s() started", __FUNCTION__);

    if (in->size_min > in->size_max || in->delay_min > in->delay_max)
    {
        ERROR("Incorrect size or delay parameters");
        return -1;
    }

    if (tarpc_find_func(in->common.lib_flags, "send", &send_func) != 0)
        return -1;

    if ((buf = malloc(in->size_max)) == NULL)
    {
        ERROR("Out of memory");
        return -1;
    }

    memset(buf, 'A', in->size_max);

    for (start = now = time(NULL);
         (unsigned int)(now - start) <= in->time2run;
         now = time(NULL))
    {
        int len;

        if (!in->size_rnd_once)
            size = rand_range(in->size_min, in->size_max);

        if (!in->delay_rnd_once)
            delay = rand_range(in->delay_min, in->delay_max);

        if (TE_US2SEC(delay) > (int)(in->time2run) - (now - start) + 1)
            break;

        usleep(delay);

        len = send_func(in->s, buf, size, 0);

        if (len < 0)
        {
            if (!in->ignore_err)
            {
                ERROR("send() failed in simple_sender(): errno %s(%x)",
                      strerror(errno), errno);
                free(buf);
                return -1;
            }
            else
            {
                len = errno = 0;
                continue;
            }
        }
        out->bytes += len;
    }

    RING("simple_sender() stopped, sent %llu bytes",
         out->bytes);

    free(buf);

    /* Clean up errno */
    errno = errno_save;

    return 0;
}

TARPC_FUNC(simple_sender, {},
{
    MAKE_CALL(out->retval = func_ptr(in, out));
}
)

/*--------------simple_receiver() --------------------------*/
#define MAX_PKT (1024 * 1024)

/**
 * Simple receiver.
 *
 * @param in                input RPC argument
 *
 * @return 0 on success or -1 in the case of failure
 */
int
simple_receiver(tarpc_simple_receiver_in *in,
                tarpc_simple_receiver_out *out)
{
    iomux_funcs     iomux_f;
    api_func        recv_func;
    char           *buf;
    int             rc;
    ssize_t         len;
    iomux_func      iomux = get_default_iomux();

    time_t          start;
    time_t          now;

    iomux_state             iomux_st;
    iomux_return            iomux_ret;

    int                     fd = -1;
    int                     events = 0;

    out->bytes = 0;

    RING("%s() started", __FUNCTION__);

    if (iomux_find_func(in->common.lib_flags, &iomux, &iomux_f) != 0 ||
        tarpc_find_func(in->common.lib_flags, "recv", &recv_func) != 0)
    {
        ERROR("failed to resolve function(s)");
        return -1;
    }

    if ((buf = malloc(MAX_PKT)) == NULL)
    {
        ERROR("Out of memory");
        return -1;
    }

    /* Create iomux status and fill it with our fds. */
    if ((rc = iomux_create_state(iomux, &iomux_f, &iomux_st)) != 0)
    {
        free(buf);
        return rc;
    }
    if ((rc = iomux_add_fd(iomux, &iomux_f, &iomux_st,
                           in->s, POLLIN)))
    {
        goto simple_receiver_exit;
    }

    for (start = now = time(NULL);
         (in->time2run != 0) ?
          ((unsigned int)(now - start) <= in->time2run) : TRUE;
         now = time(NULL))
    {
        rc = iomux_wait(iomux, &iomux_f, &iomux_st, &iomux_ret,
                        1000);
        if (rc < 0 || rc > 1)
        {
            if (rc < 0)
                ERROR("%s() failed in %s(): errno %r",
                      iomux2str(iomux), __FUNCTION__,
                      TE_OS_RC(TE_TA_UNIX, errno));
            else
                ERROR("%s() returned more then one fd",
                      iomux2str(iomux));
            rc = -1;
            goto simple_receiver_exit;
        }
        else if (rc == 0)
        {
            if ((in->time2run != 0) || (out->bytes == 0))
                continue;
            else
                break;
        }

        iomux_return_iterate(iomux, &iomux_st, &iomux_ret,
                             IOMUX_RETURN_ITERATOR_START, &fd, &events);

        if (fd != in->s || !(events & POLLIN))
        {
            ERROR("%s() returned strange event or socket",
                  iomux2str(iomux));
            rc = -1;
            goto simple_receiver_exit;
        }

        len = recv_func(in->s, buf, MAX_PKT, 0);
        if (len < 0)
        {
            ERROR("recv() failed in %s(): errno %r",
                  __FUNCTION__, TE_OS_RC(TE_TA_UNIX, errno));
            rc = -1;
            goto simple_receiver_exit;
        }
        if (len == 0)
        {
            RING("recv() returned 0 in %s() because of "
                 "peer shutdown", __FUNCTION__);
            break;
        }

        if (out->bytes == 0)
            RING("First %d bytes are received", len);
        out->bytes += len;
    }

    RING("%s() stopped, received %llu bytes", __FUNCTION__, out->bytes);

simple_receiver_exit:
    free(buf);
    iomux_close(iomux, &iomux_f, &iomux_st);

    return (rc < 0) ? rc : 0;
}

#undef MAX_PKT

TARPC_FUNC(simple_receiver, {},
{
    MAKE_CALL(out->retval = func_ptr(in, out));
}
)

/*--------------wait_readable() --------------------------*/
/**
 * Wait until the socket becomes readable.
 *
 * @param in                input RPC argument
 *
 * @return number of received bytes or -1 in the case of failure
 */
int
wait_readable(tarpc_wait_readable_in *in,
              tarpc_wait_readable_out *out)
{
    iomux_funcs     iomux_f;
    int             rc;
    iomux_func      iomux = get_default_iomux();

    iomux_state             iomux_st;
    iomux_return            iomux_ret;

    int                     fd = -1;
    int                     events = 0;

    UNUSED(out);

    RING("%s() started", __FUNCTION__);

    if (iomux_find_func(in->common.lib_flags, &iomux, &iomux_f) != 0)
    {
        return -1;
    }

    /* Create iomux status and fill it with our fds. */
    if ((rc = iomux_create_state(iomux, &iomux_f, &iomux_st)) != 0)
        return rc;
    if ((rc = iomux_add_fd(iomux, &iomux_f, &iomux_st,
                           in->s, POLLIN)))
    {
        goto wait_readable_exit;
    }

    rc = iomux_wait(iomux, &iomux_f, &iomux_st, &iomux_ret,
                    in->timeout);
    if (rc < 0)
    {
        ERROR("%s() failed in wait_readable(): errno %r",
              iomux2str(iomux), TE_OS_RC(TE_TA_UNIX, errno));
        rc = -1;
        goto wait_readable_exit;
    }
    else if ((rc > 0) && (fd != in->s || !(events & POLLIN)))
    {
        ERROR("%s() waited for reading on the socket, "
              "returned %d, but returned incorrect socket or event",
              iomux2str(iomux), rc);
        rc = -1;
        goto wait_readable_exit;
    }

wait_readable_exit:
    iomux_close(iomux, &iomux_f, &iomux_st);

    return rc;
}

TARPC_FUNC(wait_readable, {},
{
    MAKE_CALL(out->retval = func_ptr(in, out));
}
)

/*-------------- recv_verify() --------------------------*/
#define RCV_VF_BUF (1024)

/**
 * Simple receiver.
 *
 * @param in                input RPC argument
 *
 * @return number of received bytes or -1 in the case of failure
 */
int
recv_verify(tarpc_recv_verify_in *in, tarpc_recv_verify_out *out)
{
    api_func   recv_func;
    char           *rcv_buf;
    char           *pattern_buf;
    int             rc;

    int saved_errno = errno;

    out->retval = 0;

    RING("%s() started", __FUNCTION__);

    if (tarpc_find_func(in->common.lib_flags, "recv", &recv_func) != 0)
    {
        return -1;
    }

    if ((rcv_buf = malloc(RCV_VF_BUF)) == NULL)
    {
        ERROR("Out of memory");
        return -1;
    }

    while (1)
    {
        rc = recv_func(in->s, rcv_buf, RCV_VF_BUF, MSG_DONTWAIT);
        if (rc < 0)
        {
            if (errno == EAGAIN)
            {
                errno = saved_errno;
                RING("recv() returned -1(EGAIN) in recv_verify(), "
                     "no more data just now");
                break;
            }
            else
            {
                ERROR("recv() failed in recv_verify(): errno %x", errno);
                free(rcv_buf);
                out->retval = -1;
                return -1;
            }
        }
        if (rc == 0)
        {
            RING("recv() returned 0 in recv_verify() because of "
                 "peer shutdown");
            break;
        }

        /* TODO: check data here, set reval to -2 if not matched. */
        UNUSED(pattern_buf);
        out->retval += rc;
    }

    free(rcv_buf);
    RING("recv_verify() stopped, received %d bytes", out->retval);

    return 0;
}

#undef RCV_VF_BUF

TARPC_FUNC(recv_verify, {},
{
    MAKE_CALL(out->retval = func_ptr(in, out));
}
)

/*-------------- flooder() --------------------------*/
#define FLOODER_ECHOER_WAIT_FOR_RX_EMPTY        1
#define FLOODER_BUF                             4096

/**
 * Routine which receives data from specified set of sockets and sends data
 * to specified set of sockets with maximum speed using I/O multiplexing.
 *
 * @param pco       - PCO to be used
 * @param rcvrs     - set of receiver sockets
 * @param rcvnum    - number of receiver sockets
 * @param sndrs     - set of sender sockets
 * @param sndnum    - number of sender sockets
 * @param bulkszs   - sizes of data bulks to send for each sender
 *                    (in bytes, 1024 bytes maximum)
 * @param time2run  - how long send data (in seconds)
 * @param time2wait - how long wait data (in seconds)
 * @param iomux     - type of I/O Multiplexing function
 *                    (@b select(), @b pselect(), @b poll())
 *
 * @return 0 on success or -1 in the case of failure
 */
int
flooder(tarpc_flooder_in *in)
{
    int errno_save = errno;

    iomux_funcs iomux_f;
    api_func send_func;
    api_func recv_func;
    api_func ioctl_func;

    int        *rcvrs = in->rcvrs.rcvrs_val;
    int         rcvnum = in->rcvrs.rcvrs_len;
    int        *sndrs = in->sndrs.sndrs_val;
    int         sndnum = in->sndrs.sndrs_len;
    int         bulkszs = in->bulkszs;
    int         time2run = in->time2run;
    int         time2wait = in->time2wait;
    iomux_func  iomux = in->iomux;

    uint64_t   *tx_stat = in->tx_stat.tx_stat_val;
    uint64_t   *rx_stat = in->rx_stat.rx_stat_val;

    int      i;
    int      j;
    int      rc;
    char     rcv_buf[FLOODER_BUF];
    char     snd_buf[FLOODER_BUF];

    iomux_state             iomux_st;
    iomux_return            iomux_ret;
    iomux_return_iterator   it;

    struct timeval  timeout;   /* time when we should go out */
    int             iomux_timeout;
    te_bool         time2run_expired = FALSE;
    te_bool         session_rx;

    INFO("%d flooder start", getpid());
    memset(rcv_buf, 0x0, FLOODER_BUF);
    memset(snd_buf, 'X', FLOODER_BUF);

    if ((iomux_find_func(in->common.lib_flags, &iomux, &iomux_f) != 0)    ||
        (tarpc_find_func(in->common.lib_flags, "recv", &recv_func) != 0) ||
        (tarpc_find_func(in->common.lib_flags, "send", &send_func) != 0) ||
        (tarpc_find_func(in->common.lib_flags, "ioctl", &ioctl_func) != 0))
    {
        ERROR("failed to resolve function");
        return -1;
    }

    if (bulkszs > (int)sizeof(snd_buf))
    {
        ERROR("Size of sent data is too long");
        return -1;
    }
    /* Create iomux status and fill it with our fds. */
    if ((rc = iomux_create_state(iomux, &iomux_f, &iomux_st)) != 0)
    {
        iomux_close(iomux, &iomux_f, &iomux_st);
        return rc;
    }
    for (i = 0; i < sndnum; i++)
    {
        if ((rc = iomux_add_fd(iomux, &iomux_f, &iomux_st,
                               sndrs[i], POLLOUT)))
        {
            iomux_close(iomux, &iomux_f, &iomux_st);
            return rc;
        }
    }
    for (i = 0; i < rcvnum; i++)
    {
        int found = FALSE;

        for (j = 0; j < sndnum; j++)
        {
            if (sndrs[j] != rcvrs[i])
                continue;
            if ((rc = iomux_mod_fd(iomux, &iomux_f, &iomux_st,
                                   rcvrs[i], POLLIN | POLLOUT)))
            {
                iomux_close(iomux, &iomux_f, &iomux_st);
                return rc;
            }
            found = TRUE;
            break;
        }

        if (!found &&
            (rc = iomux_add_fd(iomux, &iomux_f, &iomux_st,
                               rcvrs[i], POLLIN)))
        {
            iomux_close(iomux, &iomux_f, &iomux_st);
            return rc;
        }
    }

    if (gettimeofday(&timeout, NULL))
    {
        ERROR("%s(): gettimeofday(timeout) failed: %d",
              __FUNCTION__, errno);
        return -1;
    }
    timeout.tv_sec += time2run;
    iomux_timeout = TE_SEC2MS(time2run);

    INFO("%s(): time2run=%d, timeout=%ld.%06ld", __FUNCTION__,
         time2run, (long)timeout.tv_sec, (long)timeout.tv_usec);

    do {
        int fd = -1;    /* Shut up compiler warning */
        int events = 0; /* Shut up compiler warning */

        session_rx = FALSE;
        rc = iomux_wait(iomux, &iomux_f, &iomux_st, &iomux_ret,
                        iomux_timeout);

        if (rc < 0)
        {
            if (errno == EINTR)
                continue;
            ERROR("%s(): %s wait failed: %d", __FUNCTION__,
                  iomux2str(iomux), errno);
            iomux_close(iomux, &iomux_f, &iomux_st);
            return -1;
        }

        it = IOMUX_RETURN_ITERATOR_START;
        for (it = iomux_return_iterate(iomux, &iomux_st, &iomux_ret,
                                       it, &fd, &events);
             it != IOMUX_RETURN_ITERATOR_END;
             it = iomux_return_iterate(iomux, &iomux_st, &iomux_ret,
                                       it, &fd, &events))
        {
            int sent;
            int received;
            int eperm_cnt = 0;

            if (!time2run_expired && (events & POLLOUT))
            {
                sent = send_func(fd, snd_buf, bulkszs, 0);
                while ((sent < 0) && (errno == EPERM) &&
                       (++eperm_cnt) < 10)
                {
                    /* Don't stop on EPERM, but report it */
                    if (eperm_cnt == 1)
                        ERROR("%s(): send(%d) failed: %d",
                              __FUNCTION__, fd, errno);
                    usleep(10000);
                    sent = send_func(fd, snd_buf, bulkszs, 0);
                }

                if ((sent < 0) && (errno != EINTR) &&
                    (errno != EAGAIN) && (errno != EWOULDBLOCK))
                {
                    ERROR("%s(): send(%d) failed: %d",
                          __FUNCTION__, fd, errno);
                    iomux_close(iomux, &iomux_f, &iomux_st);
                    return -1;
                }
                else if ((sent > 0) && (tx_stat != NULL))
                {
                    for (i = 0; i < sndnum; i++)
                    {
                        if (sndrs[i] != fd)
                            continue;
                        tx_stat[i] += sent;
                        break;
                    }
                }
            }
            if ((events & POLLIN))
            {
                /* We use recv() instead of read() here to avoid false
                 * positives from iomux functions.  On linux, select()
                 * sometimes return false read events.
                 * Such misbihaviour may be tested in separate functions,
                 * not here. */
                received = recv_func(fd, rcv_buf, sizeof(rcv_buf),
                                     MSG_DONTWAIT);
                if ((received < 0) && (errno != EINTR) &&
                    (errno != EAGAIN) && (errno != EWOULDBLOCK))
                {
                    ERROR("%s(): recv(%d) failed: %d",
                          __FUNCTION__, fd, errno);
                    iomux_close(iomux, &iomux_f, &iomux_st);
                    return -1;
                }
                else if (received > 0)
                {
                    session_rx = TRUE;
                    if (rx_stat != NULL)
                    {
                        for (i = 0; i < rcvnum; i++)
                        {
                            if (rcvrs[i] != fd)
                                continue;
                            rx_stat[i] += received;
                            break;
                        }
                    }
                    if (time2run_expired)
                        VERB("FD=%d Rx=%d", fd, received);
                }
            }
#ifdef DEBUG
            if ((time2run_expired) && ((events & POLLIN)))
            {
                WARN("%s() returned unexpected events: 0x%x",
                     iomux2str(iomux), events);
            }
#endif
        }

        if (!time2run_expired)
        {
            struct timeval now;

            if (gettimeofday(&now, NULL))
            {
                ERROR("%s(): gettimeofday(now) failed): %d",
                      __FUNCTION__, errno);
                iomux_close(iomux, &iomux_f, &iomux_st);
                return -1;
            }
            iomux_timeout = TE_SEC2MS(timeout.tv_sec  - now.tv_sec) +
                TE_US2MS(timeout.tv_usec - now.tv_usec);
            if (iomux_timeout < 0)
            {
                time2run_expired = TRUE;

                /* Clean up POLLOUT requests for all descriptors */
                for (i = 0; i < sndnum; i++)
                {
                    fd = sndrs[i];
                    events = 0;

                    for (j = 0; j < rcvnum; j++)
                    {
                        if (sndrs[i] != rcvrs[j])
                            continue;
                        events = POLLIN;
                        break;
                    }
                    if (iomux_mod_fd(iomux, &iomux_f, &iomux_st,
                                     fd, events) != 0)
                    {
                            ERROR("%s(): iomux_mod_fd() function failed "
                                  "with iomux=%s", __FUNCTION__,
                                  iomux2str(iomux));
                            iomux_close(iomux, &iomux_f, &iomux_st);
                            return -1;
                    }
                }

                /* Just to make sure that we'll get all from buffers */
                session_rx = TRUE;
                INFO("%s(): time2run expired", __FUNCTION__);
            }
        }

        if (time2run_expired)
        {
            iomux_timeout = TE_SEC2MS(time2wait);
            VERB("%s(): Waiting for empty Rx queue, Rx=%d",
                 __FUNCTION__, session_rx);
        }

    } while (!time2run_expired || session_rx);

    iomux_close(iomux, &iomux_f, &iomux_st);
    INFO("%s(): OK", __FUNCTION__);

    /* Clean up errno */
    errno = errno_save;

    return 0;
}

TARPC_FUNC(flooder, {},
{
    MAKE_CALL(out->retval = func_ptr(in));
    COPY_ARG(tx_stat);
    COPY_ARG(rx_stat);
}
)

/*-------------- echoer() --------------------------*/

typedef struct buffer {
    TAILQ_ENTRY(buffer)    links;
    char                   buf[FLOODER_BUF];
    int                    size;
} buffer;

typedef TAILQ_HEAD(buffers, buffer) buffers;

/**
 * Routine to free buffers queue.
 *
 * @param p     Buffers queue head pointer
 */
void free_buffers(buffers *p)
{
    buffer  *q;

    if (p == NULL)
        return;

    while ((q = TAILQ_FIRST(p)) != NULL)
    {
        TAILQ_REMOVE(p, q, links);
        free(q);
    }
}

/**
 * Routine which receives data from specified set of
 * sockets using I/O multiplexing and sends them back
 * to the socket.
 *
 * @param pco       - PCO to be used
 * @param sockets   - set of sockets to be processed
 * @param socknum   - number of sockets to be processed
 * @param time2run  - how long send data (in seconds)
 * @param iomux     - type of I/O Multiplexing function
 *                    (@b select(), @b pselect(), @b poll())
 *
 * @return 0 on success or -1 in the case of failure
 */
int
echoer(tarpc_echoer_in *in)
{
    iomux_funcs iomux_f;
    api_func write_func;
    api_func read_func;

    int        *sockets = in->sockets.sockets_val;
    int         socknum = in->sockets.sockets_len;
    int         time2run = in->time2run;

    uint64_t   *tx_stat = in->tx_stat.tx_stat_val;
    uint64_t   *rx_stat = in->rx_stat.rx_stat_val;
    iomux_func  iomux = in->iomux;

    int      i;
    int      rc;

    buffers                 buffs;
    buffer                 *buf = NULL;

    iomux_state             iomux_st;
    iomux_return            iomux_ret;
    iomux_return_iterator   it;

    struct timeval  timeout;   /* time when we should go out */
    int             iomux_timeout;
    te_bool         time2run_expired = FALSE;
    te_bool         session_rx;

    TAILQ_INIT(&buffs);

    if ((iomux_find_func(in->common.lib_flags, &iomux, &iomux_f) != 0)    ||
        (tarpc_find_func(in->common.lib_flags, "read", &read_func) != 0) ||
        (tarpc_find_func(in->common.lib_flags, "write", &write_func) != 0))
    {
        return -1;
    }

    /* Create iomux status and fill it with our fds. */
    if ((rc = iomux_create_state(iomux, &iomux_f, &iomux_st)) != 0)
    {
        iomux_close(iomux, &iomux_f, &iomux_st);
        return rc;
    }

    for (i = 0; i < socknum; i++)
    {
        if ((rc = iomux_add_fd(iomux, &iomux_f, &iomux_st,
                               sockets[i], POLLIN | POLLOUT)) != 0)
        {
            ERROR("%s(): failed to add fd to iomux list", __FUNCTION__);
            iomux_close(iomux, &iomux_f, &iomux_st);
            return rc;
        }
    }

    if (gettimeofday(&timeout, NULL))
    {
        ERROR("%s(): gettimeofday(timeout) failed: %d",
              __FUNCTION__, errno);
        iomux_close(iomux, &iomux_f, &iomux_st);
        return -1;
    }
    timeout.tv_sec += time2run;
    iomux_timeout = TE_SEC2MS(time2run);

    INFO("%s(): time2run=%d, timeout timestamp=%ld.%06ld", __FUNCTION__,
         time2run, (long)timeout.tv_sec, (long)timeout.tv_usec);

    do {
        int fd = -1;
        int events = 0;

        session_rx = FALSE;
        rc = iomux_wait(iomux, &iomux_f, &iomux_st, &iomux_ret,
                        iomux_timeout);

        if (rc < 0)
        {
            if (errno == EINTR)
                continue;
            ERROR("%s(): %spoll() failed: %d", __FUNCTION__,
                  iomux2str(iomux), errno);
            iomux_close(iomux, &iomux_f, &iomux_st);
            free_buffers(&buffs);
            return -1;
        }

        for (it = IOMUX_RETURN_ITERATOR_START;
             it != IOMUX_RETURN_ITERATOR_END;
             it = iomux_return_iterate(iomux, &iomux_st, &iomux_ret,
                                       it, &fd, &events))
        {
            int sent = 0;
            int received = 0;

            if ((events & POLLIN))
            {
                buf = TE_ALLOC(sizeof(*buf));
                if (buf == NULL)
                {
                    ERROR("%s(): out of memory", __FUNCTION__);
                    iomux_close(iomux, &iomux_f, &iomux_st);
                    free_buffers(&buffs);
                    return - 1;
                }

                TAILQ_INSERT_HEAD(&buffs, buf, links);
                received = buf->size = read_func(fd, buf->buf,
                                                 sizeof(buf->buf));
                if (received < 0)
                {
                    ERROR("%s(): read() failed: %d", __FUNCTION__, errno);
                    iomux_close(iomux, &iomux_f, &iomux_st);
                    free_buffers(&buffs);
                    return -1;
                }
                session_rx = TRUE;
            }
            if ((events & POLLOUT) &&
                (buf = TAILQ_LAST(&buffs, buffers)) != NULL)
            {
                sent = write_func(fd, buf->buf, buf->size);
                if (sent < 0)
                {
                    ERROR("%s(): write() failed: %d", __FUNCTION__, errno);
                    iomux_close(iomux, &iomux_f, &iomux_st);
                    free_buffers(&buffs);
                    return -1;
                }
                TAILQ_REMOVE(&buffs, buf, links);
                free(buf);
            }

            if ((received > 0 && rx_stat != NULL) ||
                (sent > 0 && tx_stat != NULL))
            {
                for (i = 0; i < socknum; i++)
                {
                    if (sockets[i] != fd)
                        continue;
                    if (rx_stat != NULL)
                        rx_stat[i] += received;
                    if (tx_stat != NULL)
                        tx_stat[i] += sent;
                    break;
                }
            }
        }

        if (!time2run_expired)
        {
            struct timeval now;

            if (gettimeofday(&now, NULL))
            {
                ERROR("%s(): gettimeofday(now) failed: %d",
                      __FUNCTION__, errno);
                iomux_close(iomux, &iomux_f, &iomux_st);
                free_buffers(&buffs);
                return -1;
            }
            iomux_timeout = TE_SEC2MS(timeout.tv_sec  - now.tv_sec) +
                TE_US2MS(timeout.tv_usec - now.tv_usec);
            if (iomux_timeout < 0)
            {
                time2run_expired = TRUE;
                /* Just to make sure that we'll get all from buffers */
                session_rx = TRUE;
                INFO("%s(): time2run expired", __FUNCTION__);
            }
        }

        if (time2run_expired)
        {
            iomux_timeout = FLOODER_ECHOER_WAIT_FOR_RX_EMPTY;
            VERB("%s(): Waiting for empty Rx queue", __FUNCTION__);
        }

    } while (!time2run_expired || session_rx);

    iomux_close(iomux, &iomux_f, &iomux_st);
    free_buffers(&buffs);
    INFO("%s(): OK", __FUNCTION__);

    return 0;
}

TARPC_FUNC(echoer, {},
{
    MAKE_CALL(out->retval = func_ptr(in));
    COPY_ARG(tx_stat);
    COPY_ARG(rx_stat);
}
)

/*-------------- pattern_sender() --------------------------*/
/* Count of numbers in a sequence (should not be greater that 65280). */
#define SEQUENCE_NUM 10000
/* Period of a sequence */
#define SEQUENCE_PERIOD_NUM 255 + (SEQUENCE_NUM - 255) * 2

/**
 * Get nth element of a string which is a concatenation of
 * a periodic sequence 1, 2, 3, ..., SEQUENCE_PERIOD_NUM, 1, 2, ...
 * where numbers are written in a positional base 256 system.
 *
 * @param n     Number
 *
 * @return Character code
 */
static char
get_nth_elm(int n)
{
    int m;

    n = n % SEQUENCE_PERIOD_NUM + 1;

    if (n <= 255)
        return n;
    else
    {
        m = n - 256;
        return m % 2 == 0 ? (m / 2 / 255) + 1 : (m / 2) % 255 + 1;
    }
}

/**
 * Fill a buffer with values provided by @b get_nth_elm().
 *
 * @param buf            Buffer
 * @param size           Buffer size
 * @param arg            Pointer to @ref tarpc_pat_gen_arg structure, where
 *                       - coef1 is a starting number in a sequence
 *
 * @return 0 on success
 */
te_errno
fill_buff_with_sequence(char *buf, int size, tarpc_pat_gen_arg *arg)
{
    int i;
    int start_n = arg->coef1 % SEQUENCE_PERIOD_NUM;
    arg->coef1 += size;

    for (i = 0; i < size; i++)
    {
        buf[i] = get_nth_elm(start_n + i);
    }

    return 0;
}

/**
 * Fills the buffer with a linear congruential sequence
 * and updates @b arg parameter for the next call.
 *
 * Each element is calculated using the formula:
 * X[n] = a * X[n-1] + c, where @a a and @a c are taken from @b arg parameter:
 * - @a a is @b arg->coef2,
 * - @a c is @b arg->coef3
 *
 * @param buf            Buffer
 * @param size           Buffer size in bytes
 * @param arg            Pointer to @ref tarpc_pat_gen_arg structure, where
 *                       - coef1 is @a x0 - starting number in a sequence,
 *                       - coef2 is @a a - multiplying constant,
 *                       - coef3 is @a c - additive constant
 *
 * @return 0 on success
 */
te_errno
fill_buff_with_sequence_lcg(char *buf, int size, tarpc_pat_gen_arg *arg)
{
    int i;
    uint32_t x0 = arg->coef1;
    uint32_t a = arg->coef2;
    uint32_t c = arg->coef3;
    uint32_t *p32buf = (uint32_t *)buf;
    int word_size = (size + arg->offset + 3) / 4;

    if (size == 0)
        return 0;

    arg->offset = (size + arg->offset) % 4;
    p32buf[0] = htonl(x0);

    for (i = 1; i < word_size; ++i)
    {
        uint32_t curr_elem = a * x0 + c;
        p32buf[i] = htonl(curr_elem);
        x0 = curr_elem;
    }

    arg->coef1 = arg->offset ? x0 : (a * x0 + c);
    return 0;
}

/**
 * Pattern sender.
 *
 * @param in                input RPC argument
 * @param out               output RPC argument
 *
 * @return 0 on success or -1 in the case of failure
 */
int
pattern_sender(tarpc_pattern_sender_in *in, tarpc_pattern_sender_out *out)
{
#define MAX_OFFSET \
    ((void*)pattern_gen_func == (void*)fill_buff_with_sequence_lcg ? 3 : 0)

    int             errno_save = errno;
    api_func_ptr    pattern_gen_func;
    api_func        send_func;
    api_func_ptr    send_wrapper = NULL;
    void           *send_wrapper_data = NULL;
    iomux_funcs     iomux_f;
    char           *buf = NULL;
    char           *send_ptr;
    size_t          send_size;
    iomux_func      iomux = in->iomux;

    api_func_ptr    pollerr_handler = NULL;
    void           *pollerr_handler_data = NULL;

    int size = rand_range(in->size_min, in->size_max);
    int delay = rand_range(in->delay_min, in->delay_max);

    int fd = -1;
    int events = 0;
    int rc = 0;

    /*
     * Number of bytes from the current data chunk which
     * are not yet sent.
     */
    int bytes_rest = 0;
    int send_flags = iomux == FUNC_NO_IOMUX ? 0 : MSG_DONTWAIT;

    int                     iomux_timeout;
    iomux_state             iomux_st;
    iomux_return            iomux_ret;
    iomux_return_iterator   itr;

    struct timeval tv_start;
    struct timeval tv_now;

    tarpc_pat_gen_arg prev_gen_arg = in->gen_arg;
    out->gen_arg = in->gen_arg;
    out->bytes = 0;

    RING("%s() started", __FUNCTION__);

    if (in->size_min > in->size_max || in->delay_min > in->delay_max)
    {
        te_rpc_error_set(TE_RC(TE_TA_UNIX, TE_EINVAL),
                         "Incorrect size or delay parameters");
        return -1;
    }

    /* 1 is length of empty string here. */
    if (in->swrapper.swrapper_len > 1)
    {
        send_wrapper = (api_func_ptr)
                          rcf_ch_symbol_addr(
                              in->swrapper.swrapper_val,
                              TRUE);
        if (send_wrapper == NULL)
        {
            te_rpc_error_set(TE_RC(TE_TA_UNIX, TE_ENOENT),
                             "failed to find function '%s'",
                             in->swrapper.swrapper_val);
            return -1;
        }

        if (in->swrapper_data != RPC_NULL)
        {
            send_wrapper_data = rcf_pch_mem_get(in->swrapper_data);
            if (send_wrapper_data == NULL)
            {
                te_rpc_error_set(TE_RC(TE_TA_UNIX, TE_ENOENT),
                                 "failed to resolve swrapper_data");
                return -1;
            }
        }
    }
    else
    {
        rc = tarpc_find_func(in->common.lib_flags, "send", &send_func);
        if (rc != 0)
        {
            te_rpc_error_set(TE_RC(TE_TA_UNIX, rc),
                             "failed to resolve 'send'");
            return -1;
        }
    }

    if (in->pollerr_handler.pollerr_handler_len > 1)
    {
        pollerr_handler = (api_func_ptr)
                          rcf_ch_symbol_addr(
                              in->pollerr_handler.pollerr_handler_val,
                              TRUE);
        if (pollerr_handler == NULL)
        {
            te_rpc_error_set(TE_RC(TE_TA_UNIX, TE_ENOENT),
                             "failed to find function '%s'",
                             in->pollerr_handler.pollerr_handler_val);
            return -1;
        }

        if (in->pollerr_handler_data != RPC_NULL)
        {
            pollerr_handler_data = rcf_pch_mem_get(in->pollerr_handler_data);
            if (pollerr_handler_data == NULL)
            {
                te_rpc_error_set(TE_RC(TE_TA_UNIX, TE_ENOENT),
                                 "failed to resolve pollerr_handler_data");
                return -1;
            }
        }
    }

    if ((pattern_gen_func =
                rcf_ch_symbol_addr(in->fname.fname_val, TRUE)) == NULL)
    {
        te_rpc_error_set(TE_RC(TE_TA_UNIX, TE_ENOENT),
                         "failed to resolve '%s'", in->fname.fname_val);
        return -1;
    }

    if (iomux_find_func(in->common.lib_flags, &iomux, &iomux_f) != 0)
    {
        te_rpc_error_set(TE_RC(TE_TA_UNIX, TE_ENOENT),
                         "failed to resolve iomux function");
        return -1;
    }

    if ((rc = iomux_create_state(iomux, &iomux_f, &iomux_st)) != 0)
    {
        te_rpc_error_set(TE_OS_RC(TE_TA_UNIX, errno),
                         "failed to create iomux state");
        iomux_close(iomux, &iomux_f, &iomux_st);
        return rc;
    }

    if ((rc = iomux_add_fd(iomux, &iomux_f, &iomux_st,
                           in->s, POLLOUT)) != 0)
    {
        te_rpc_error_set(TE_OS_RC(TE_TA_UNIX, errno),
                         "failed to add file descriptor to iomux set");
        iomux_close(iomux, &iomux_f, &iomux_st);
        return rc;
    }

    if ((buf = malloc(in->size_max + MAX_OFFSET)) == NULL)
    {
        te_rpc_error_set(TE_RC(TE_TA_UNIX, TE_ENOMEM),
                         "out of memory");
        iomux_close(iomux, &iomux_f, &iomux_st);
        return -1;
    }

#define PTRN_SEND_ERROR \
    do {                                                        \
        if (bytes_rest != 0)                                    \
        {                                                       \
            /*                                                  \
             * If some of the data from the last data chunk are \
             * not sent, we need to recompute pattern generator \
             * argument, passing to generator the amount of     \
             * data which was actually sent.                    \
             * Then if we choose to resume sending and pass     \
             * out->gen_arg to this function, it will resume    \
             * data sequence generation from the first byte     \
             * which was not sent in the previous attempt.      \
             */                                                 \
            pattern_gen_func(buf, size - bytes_rest, &prev_gen_arg); \
            out->gen_arg = prev_gen_arg;                        \
        }                                                       \
        else                                                    \
        {                                                       \
            out->gen_arg = in->gen_arg;                         \
        }                                                       \
        iomux_close(iomux, &iomux_f, &iomux_st);                \
        free(buf);                                              \
        return -1;                                              \
    } while (0)

#define MSEC_DIFF \
    (TE_SEC2MS(tv_now.tv_sec - tv_start.tv_sec) + \
     TE_US2MS(tv_now.tv_usec - tv_start.tv_usec))

    for (gettimeofday(&tv_start, NULL), gettimeofday(&tv_now, NULL);
         MSEC_DIFF <= (int)TE_SEC2MS(in->time2run);
         gettimeofday(&tv_now, NULL))
    {
        int len = 0;
        /* The offset within generated sequence to send data from. */
        uint32_t offset = 0;

        if (!in->size_rnd_once && bytes_rest == 0)
            size = rand_range(in->size_min, in->size_max);

        if (in->total_size > 0)
        {
            uint64_t max_size;

            if (out->bytes >= in->total_size)
                break;

            max_size = in->total_size - out->bytes;
            if ((uint64_t)size > max_size)
                size = max_size;
        }

        if (!in->delay_rnd_once)
            delay = rand_range(in->delay_min, in->delay_max);

        if (TE_US2MS(delay) > (int)TE_SEC2MS(in->time2run) - MSEC_DIFF)
            break;

        usleep(delay);
        gettimeofday(&tv_now, NULL);

        /* Wait for writability until time2run expires. */
        iomux_timeout = (int)TE_SEC2MS(in->time2run) - MSEC_DIFF;
        if (iomux_timeout <= 0)
            break;

        /*
         * However if time2wait is positive, wait no more than
         * time2wait before terminating.
         */
        if (in->time2wait > 0)
            iomux_timeout = MIN((unsigned int)iomux_timeout, in->time2wait);

        rc = iomux_wait(iomux, &iomux_f, &iomux_st, &iomux_ret,
                        iomux_timeout);

        if (rc < 0)
        {
            if (errno == EINTR)
                continue;

            te_rpc_error_set(TE_OS_RC(TE_TA_UNIX, errno),
                             "%s wait failed: %r",
                             iomux2str(iomux), te_rc_os2te(errno));
            PTRN_SEND_ERROR;
        }
        else if (rc > 1)
        {
            te_rpc_error_set(TE_RC(TE_TA_UNIX, TE_EFAIL),
                             "%s wait returned more then one fd",
                             iomux2str(iomux));
            PTRN_SEND_ERROR;
        }
        else if (rc == 0  && iomux != FUNC_NO_IOMUX)
        {
            break;
        }

        itr = IOMUX_RETURN_ITERATOR_START;
        itr = iomux_return_iterate(iomux, &iomux_st, &iomux_ret,
                                   itr, &fd, &events);
        if (fd != in->s && iomux != FUNC_NO_IOMUX)
        {
            te_rpc_error_set(TE_RC(TE_TA_UNIX, TE_EFAIL),
                             "%s wait returned incorrect fd %d "
                             "instead of %d",
                             iomux2str(iomux), fd, in->s);
            PTRN_SEND_ERROR;
        }

        if ((events & POLLERR) && pollerr_handler != NULL)
        {
            rc = pollerr_handler(pollerr_handler_data,
                                 in->s);
            if (rc < 0)
                PTRN_SEND_ERROR;

            if (!(events & POLLOUT))
                continue;
        }

        if (!(events & POLLOUT) && iomux != FUNC_NO_IOMUX)
        {
            te_rpc_error_set(TE_RC(TE_TA_UNIX, TE_EFAIL),
                             "%s wait succeeded but returned events "
                             "%s instead of POLLOUT", iomux2str(iomux),
                             poll_event_rpc2str(poll_event_h2rpc(events)));
            PTRN_SEND_ERROR;
        }

        /*
         * If send function sends only part of data passed to it,
         * we save number of remaining bytes in bytes_rest and try
         * to send remaining data in the next iteration.
         * Only after all the data generated by pattern_gen_func() is sent,
         * we generate the next data chunk.
         */
        if (bytes_rest == 0)
        {
            offset = in->gen_arg.offset;
            if (offset > MAX_OFFSET)
            {
                te_rpc_error_set(TE_RC(TE_TA_UNIX, TE_EINVAL),
                                 "offset is too big");
                PTRN_SEND_ERROR;
            }
            bytes_rest = size;
            prev_gen_arg = in->gen_arg;
            if ((rc = pattern_gen_func(buf, size, &in->gen_arg)) != 0)
            {
                te_rpc_error_set(TE_RC(TE_TA_UNIX, rc),
                                 "failed to generate a pattern");
                PTRN_SEND_ERROR;
            }
            send_ptr = buf + offset;
            send_size = size;
        }
        else
        {
            offset = prev_gen_arg.offset;
            send_ptr = buf + offset + (size - bytes_rest);
            send_size = bytes_rest;
        }

        if (send_wrapper != NULL)
        {
            len = send_wrapper(send_wrapper_data, in->s, send_ptr,
                               send_size, send_flags);
        }
        else
        {
            len = send_func(in->s, send_ptr, send_size, send_flags);
        }

        if (len < 0)
        {
            if (!in->ignore_err)
            {
                ERROR("send() failed in pattern_sender(): errno %s (%x)",
                      strerror(errno), errno);
                out->func_failed = TRUE;
                PTRN_SEND_ERROR;
            }
            else
            {
                len = errno = 0;
                continue;
            }
        }
        bytes_rest -= len;
        out->bytes += len;
    }
#undef PTRN_SEND_ERROR
#undef MSEC_DIFF

    RING("pattern_sender() stopped, sent %llu bytes",
         out->bytes);

    if (bytes_rest != 0)
    {
        pattern_gen_func(buf, size - bytes_rest, &prev_gen_arg);
        out->gen_arg = prev_gen_arg;
    }
    else
        out->gen_arg = in->gen_arg;

    iomux_close(iomux, &iomux_f, &iomux_st);
    free(buf);

    /* Clean up errno */
    errno = errno_save;

    return 0;
}

TARPC_FUNC(pattern_sender, {},
{
    MAKE_CALL(out->retval = func_ptr(in, out));
}
)

/*-------------- pattern_receiver() --------------------------*/
/**
 * Pattern receiver.
 *
 * @param in                input RPC argument
 * @param out               output RPC argument
 *
 * @return 0 on success, -2 in case of data not matching the pattern
 *         received or -1 in the case of another failure
 */
int
pattern_receiver(tarpc_pattern_receiver_in *in,
                 tarpc_pattern_receiver_out *out)
{
#define MAX_PKT (1024 * 1024)
    int             errno_save = errno;
    api_func_ptr    pattern_gen_func;
    api_func        recv_func;
    iomux_funcs     iomux_f;
    char           *buf = NULL;
    char           *check_buf = NULL;
    iomux_func      iomux = in->iomux;
    api_func        setsockopt_func;

    int fd = -1;
    int events = 0;
    int rc = 0;
    int recv_flags = iomux == FUNC_NO_IOMUX ? 0 : MSG_DONTWAIT;

    int                     iomux_timeout;
    iomux_state             iomux_st;
    iomux_return            iomux_ret;
    iomux_return_iterator   itr;

    struct timeval tv_start;
    struct timeval tv_now;
    int default_recv_timeout = 0;

    out->gen_arg = in->gen_arg;
    out->bytes = 0;

    RING("%s() started", __FUNCTION__);

    if (iomux == FUNC_NO_IOMUX)
    {
        api_func getsockopt_func;
        struct timeval tv = {0,0};
        socklen_t tv_len = sizeof(tv);

        if (tarpc_find_func(in->common.lib_flags, "setsockopt",
                        &setsockopt_func) != 0)
            return -1;

        if (tarpc_find_func(in->common.lib_flags, "getsockopt",
                        &getsockopt_func) != 0)
            return -1;

        if (getsockopt_func(in->s, SOL_SOCKET, SO_RCVTIMEO, &tv, &tv_len) == 0)
        {
            default_recv_timeout = TE_SEC2US(tv.tv_sec);
            default_recv_timeout += tv.tv_usec;
        }
        else
        {
            ERROR("%s(): getsockopt() failed to get default "
                  "timeout with errno %s (%d)",
                  __FUNCTION__, strerror(errno), errno);
            return -1;
        }
    }
    else
    {
        if (iomux_find_func(in->common.lib_flags, &iomux, &iomux_f) != 0)
            return -1;

        if ((rc = iomux_create_state(iomux, &iomux_f, &iomux_st)) != 0)
        {
            iomux_close(iomux, &iomux_f, &iomux_st);
            return rc;
        }

        if ((rc = iomux_add_fd(iomux, &iomux_f, &iomux_st,
                               in->s, POLLIN)) != 0)
        {
            iomux_close(iomux, &iomux_f, &iomux_st);
            return rc;
        }
    }

    if (tarpc_find_func(in->common.lib_flags, "recv", &recv_func) != 0 ||
        (pattern_gen_func =
                rcf_ch_symbol_addr(in->fname.fname_val, TRUE)) == NULL)
        return -1;

    if ((buf = malloc(MAX_PKT)) == NULL ||
        (check_buf = malloc(MAX_PKT + MAX_OFFSET)) == NULL)
    {
        ERROR("Out of memory");
        free(buf);
        free(check_buf);
        iomux_close(iomux, &iomux_f, &iomux_st);
        return -1;
    }

#define SET_RECV_TIMEOUT(timeout_us)                         \
    do {                                                     \
        struct timeval tv = {0,0};                           \
        TE_US2TV(timeout_us, &tv);                           \
        rc = setsockopt_func(in->s, SOL_SOCKET, SO_RCVTIMEO, \
                             &tv, sizeof(tv));               \
    } while (0)

#define PTRN_RECV_ERROR \
    do {                                                             \
        iomux_close(iomux, &iomux_f, &iomux_st);                     \
        free(buf);                                                   \
        free(check_buf);                                             \
        out->gen_arg = in->gen_arg;                                  \
        if (iomux == FUNC_NO_IOMUX)                                  \
            SET_RECV_TIMEOUT(default_recv_timeout);                  \
        return -1;                                                   \
    } while (0)

#define MSEC_DIFF \
    (TE_SEC2MS(tv_now.tv_sec - tv_start.tv_sec) + \
     TE_US2MS(tv_now.tv_usec - tv_start.tv_usec))

    for (gettimeofday(&tv_start, NULL), gettimeofday(&tv_now, NULL);
         MSEC_DIFF <= (int)TE_SEC2MS(in->time2run);
         gettimeofday(&tv_now, NULL))
    {
        int len = 0;
        uint32_t offset = in->gen_arg.offset;

        /* Wait for readability until time2run expires. */
        iomux_timeout = (int)TE_SEC2MS(in->time2run) - MSEC_DIFF;
        if (iomux_timeout <= 0)
            break;

        /*
         * However if time2wait is positive, wait no more than
         * time2wait before terminating.
         */
        if (in->time2wait > 0)
            iomux_timeout = MIN((unsigned int)iomux_timeout, in->time2wait);

        if (iomux == FUNC_NO_IOMUX)
        {
            SET_RECV_TIMEOUT(TE_MS2US(iomux_timeout));
            if (rc != 0)
            {
                ERROR("%s(): setsockopt() failed to set %d ms"
                      "timeout with errno %s (%d)",
                      __FUNCTION__, iomux_timeout,
                      strerror(errno), errno);
                PTRN_RECV_ERROR;
            }
        }
        else
        {
            rc = iomux_wait(iomux, &iomux_f, &iomux_st, &iomux_ret,
                            iomux_timeout);
            if (rc < 0)
            {
                if (errno == EINTR)
                    continue;
                ERROR("%s(): %s wait failed: %d", __FUNCTION__,
                      iomux2str(iomux), errno);
                PTRN_RECV_ERROR;
            }
            else if (rc > 1)
            {
                te_rpc_error_set(TE_RC(TE_TA_UNIX, TE_EINVAL),
                                 "%s(): iomux function returned more then "
                                 "one fd", __FUNCTION__);
                PTRN_RECV_ERROR;
            }
            else if (rc == 0)
                break;

            itr = IOMUX_RETURN_ITERATOR_START;
            itr = iomux_return_iterate(iomux, &iomux_st, &iomux_ret,
                                       itr, &fd, &events);
            if (fd != in->s)
            {
                ERROR("%s(): %s wait returned incorrect fd %d instead of %d",
                      __FUNCTION__, iomux2str(iomux), fd, in->s);

                te_rpc_error_set(TE_RC(TE_TA_UNIX, TE_EINVAL),
                                 "%s(): iomux function returned incorrect "
                                 "fd", __FUNCTION__);
                PTRN_RECV_ERROR;
            }

            if (!(events & POLLIN))
            {
                if ((events & POLLERR) && in->ignore_pollerr)
                {
                    /*
                     * Sleep for 10ms to avoid loading CPU with
                     * an infinite loop with iomux reporting POLLERR
                     * again and again.
                     */
                    usleep(10000);
                    continue;
                }

                ERROR("%s(): %s wait successeed but the socket is "
                      "not readable, reported events 0x%x", __FUNCTION__,
                      iomux2str(iomux), events);

                te_rpc_error_set(TE_RC(TE_TA_UNIX, TE_EINVAL),
                                 "%s(): iomux function returned unexpected "
                                 "events instead of POLLIN", __FUNCTION__);

                PTRN_RECV_ERROR;
            }
        }

        len = recv_func(in->s, buf, MAX_PKT, recv_flags);
        if (len < 0)
        {
            int recv_errno = errno;

            if (iomux == FUNC_NO_IOMUX &&
                (recv_errno == EAGAIN || recv_errno == EWOULDBLOCK))
                continue;

            ERROR("recv() failed in pattern_receiver(): errno %s (%x)",
                  strerror(errno), errno);
            out->func_failed = TRUE;
            if (recv_errno != ECONNRESET)
                PTRN_RECV_ERROR;
            else
                len = 0;
        }
        else
        {
            if ((rc = pattern_gen_func(check_buf, len, &in->gen_arg)) != 0)
            {
                ERROR("%s(): failed to generate a pattern", __FUNCTION__);

                te_rpc_error_set(TE_RC(TE_TA_UNIX, TE_EINVAL),
                                 "%s(): failed to generate data according "
                                 "to the pattern", __FUNCTION__);
                PTRN_RECV_ERROR;
            }

            if (memcmp(buf, check_buf + offset, len) != 0)
            {
                te_rpc_error_set(TE_RC(TE_TA_UNIX, TE_EINVAL),
                                 "%s(): received data does not match the "
                                 "pattern", __FUNCTION__);
                iomux_close(iomux, &iomux_f, &iomux_st);
                free(buf);
                free(check_buf);
                if (iomux == FUNC_NO_IOMUX)
                    SET_RECV_TIMEOUT(default_recv_timeout);
                return -2;
            }
        }
        out->bytes += len;

        if (in->exp_received > 0)
        {
            if (out->bytes >= in->exp_received)
                break;
        }
    }
#undef PTRN_RECV_ERROR
#undef MSEC_DIFF
#undef MAX_OFFSET

    RING("pattern_receiver() stopped, received %llu bytes",
         out->bytes);

    out->gen_arg = in->gen_arg;

    iomux_close(iomux, &iomux_f, &iomux_st);
    free(buf);
    free(check_buf);

    if (iomux == FUNC_NO_IOMUX)
    {
        SET_RECV_TIMEOUT(default_recv_timeout);
        if (rc != 0)
        {
            ERROR("%s(): setsockopt() failed to set default"
                  "timeout with errno %s (%d)",
                  __FUNCTION__, strerror(errno), errno);
            return -1;
        }
    }

    /* Clean up errno */
    errno = errno_save;

    return 0;
#undef MAX_PKT
#undef SET_RECV_TIMEOUT
}

TARPC_FUNC(pattern_receiver, {},
{
    MAKE_CALL(out->retval = func_ptr(in, out));
}
)

/*-------------- sendfile() ------------------------------*/

#if (SIZEOF_OFF_T == 8)
typedef off_t   ta_off64_t;
#else
typedef uint64_t ta_off64_t;
#endif

/* FIXME: sort off the correct prototype */
TARPC_FUNC_DYNAMIC_UNSAFE(sendfile,
{
    COPY_ARG(offset);
},
{
    if (in->force64 == TRUE)
    {
        do {
            int         rc;
            api_func    real_func = func;
            api_func    func64;
            ta_off64_t  offset = 0;
            const char *real_func_name = "sendfile64";

            if ((rc = tarpc_find_func(in->common.lib_flags,
                                      real_func_name, &func64)) == 0)
            {
                real_func = func64;
            }
            else if (sizeof(off_t) == 8)
            {
                INFO("Using sendfile() instead of sendfile64() since "
                     "sizeof(off_t) is 8");
                real_func_name = "sendfile";
            }
            else
            {
                ERROR("Cannot find sendfile64() function.\n"
                      "Unable to use sendfile() since sizeof(off_t) "
                      "is %u", (unsigned)sizeof(off_t));
                out->common._errno = TE_RC(TE_TA_UNIX, TE_ENOENT);
                break;
            }

            assert(real_func != NULL);

            if (out->offset.offset_len > 0)
                offset = *out->offset.offset_val;

            VERB("Call %s(out=%d, int=%d, offset=%lld, count=%d)",
                 real_func_name, in->out_fd, in->in_fd,
                 (long long)offset, in->count);

            MAKE_CALL(out->retval =
                      real_func(in->out_fd, in->in_fd,
                                out->offset.offset_len == 0 ? NULL : &offset,
                                in->count));

            VERB("%s() returns %d, errno=%d, offset=%lld",
                 real_func_name, out->retval, errno, (long long)offset);

            if (out->offset.offset_len > 0)
                out->offset.offset_val[0] = (tarpc_off_t)offset;

        } while (0);
    }
    else
    {
        off_t offset = 0;

        if (out->offset.offset_len > 0)
            offset = *out->offset.offset_val;

        MAKE_CALL(out->retval =
            func(in->out_fd, in->in_fd,
                 out->offset.offset_len == 0 ? NULL : &offset,
                 in->count));
        if (out->offset.offset_len > 0)
            out->offset.offset_val[0] = (tarpc_off_t)offset;
    }
}
)

/*-------------- sendfile_via_splice() ------------------------------*/

tarpc_ssize_t
sendfile_via_splice(tarpc_sendfile_via_splice_in *in,
                    tarpc_sendfile_via_splice_out *out)
{
    api_func_ptr    pipe_func;
    api_func        splice_func;
    api_func        close_func;
    ssize_t         to_pipe;
    ssize_t         from_pipe = 0;
    int             pipefd[2];
    unsigned int    flags = 0;
    off_t           offset = 0;
    int             ret = 0;

#ifdef SPLICE_F_NONBLOCK
    flags = SPLICE_F_NONBLOCK | SPLICE_F_MOVE;
#endif

    if (tarpc_find_func(in->common.lib_flags, "pipe",
                        (api_func *)&pipe_func) != 0)
    {
        ERROR("%s(): Failed to resolve pipe() function", __FUNCTION__);
        return -1;
    }
    if (tarpc_find_func(in->common.lib_flags, "splice", &splice_func) != 0)
    {
        ERROR("%s(): Failed to resolve splice() function", __FUNCTION__);
        return -1;
    }
    if (tarpc_find_func(in->common.lib_flags, "close", &close_func) != 0)
    {
        ERROR("%s(): Failed to resolve close() function", __FUNCTION__);
        return -1;
    }

    if (pipe_func(pipefd) != 0)
    {
        ERROR("pipe() failed with error %r", TE_OS_RC(TE_TA_UNIX, errno));
        return -1;
    }

    if (out->offset.offset_len > 0)
            offset = *out->offset.offset_val;
    if ((to_pipe = splice_func(in->in_fd,
                               out->offset.offset_len == 0 ? NULL : &offset,
                               pipefd[1], NULL, in->count, flags)) < 0)
    {
        ERROR("splice() to pipe failed with error %r",
              TE_OS_RC(TE_TA_UNIX, errno));
        ret = -1;
        goto sendfile_via_splice_exit;
    }
    if (out->offset.offset_len > 0)
            out->offset.offset_val[0] = (tarpc_off_t)offset;

    if ((from_pipe = splice_func(pipefd[0], NULL, in->out_fd, NULL,
                                 in->count, flags)) < 0)
    {
        ERROR("splice() from pipe failed with error %r",
              TE_OS_RC(TE_TA_UNIX, errno));
        ret = -1;
        goto sendfile_via_splice_exit;
    }
    if (to_pipe != from_pipe)
    {
        ERROR("Two splice() calls return different amount of data",
              TE_OS_RC(TE_TA_UNIX, EMSGSIZE));
        errno = EMSGSIZE;
        ret = -1;
    }
sendfile_via_splice_exit:
    if (tarpc_call_close_with_hooks(close_func, pipefd[0]) < 0 ||
        tarpc_call_close_with_hooks(close_func, pipefd[1]) < 0)
        ret = -1;
    return ret == -1 ? ret : from_pipe;
}

TARPC_FUNC(sendfile_via_splice,
{
    COPY_ARG(offset);
},
{
    MAKE_CALL(out->retval = func_ptr(in, out));
}
)

/*-------------- splice() ------------------------------*/
TARPC_FUNC(splice,
{
    COPY_ARG(off_in);
    COPY_ARG(off_out);
},
{
    off_t off_in = 0;
    off_t off_out = 0;

    if (out->off_in.off_in_len > 0)
        off_in = *out->off_in.off_in_val;
    if (out->off_out.off_out_len > 0)
        off_out = *out->off_out.off_out_val;

    MAKE_CALL(out->retval =
        func(in->fd_in,
             out->off_in.off_in_len == 0 ? NULL : (off64_t *)&off_in,
             in->fd_out,
             out->off_out.off_out_len == 0 ? NULL : (off64_t *)&off_out,
             in->len, splice_flags_rpc2h(in->flags)));
    if (out->off_in.off_in_len > 0)
        out->off_in.off_in_val[0] = (tarpc_off_t)off_in;
    if (out->off_out.off_out_len > 0)
        out->off_out.off_out_val[0] = (tarpc_off_t)off_out;
}
)

/*-------------- socket_to_file() ------------------------------*/
#define SOCK2FILE_BUF_LEN  4096

/**
 * Routine which receives data from socket and write data
 * to specified path.
 *
 * @return -1 in the case of failure or some positive value in other cases
 */
int
socket_to_file(tarpc_socket_to_file_in *in)
{
    iomux_funcs  iomux_f;
    api_func     write_func;
    api_func     read_func;
    api_func     close_func;
    api_func_ptr open_func;
    iomux_func   iomux = get_default_iomux();

    int      sock = in->sock;
    char    *path = in->path.path_val;
    long     time2run = in->timeout;

    int      rc = 0;
    int      file_d = -1;
    int      written;
    int      received;
    size_t   total = 0;
    char     buffer[SOCK2FILE_BUF_LEN];

    iomux_state             iomux_st;
    iomux_return            iomux_ret;

    struct timeval  timeout;
    struct timeval  timestamp;
    int             iomux_timeout;
    te_bool         time2run_expired = FALSE;
    te_bool         session_rx;

    path[in->path.path_len] = '\0';

    INFO("%s() called with: sock=%d, path=%s, timeout=%ld",
         __FUNCTION__, sock, path, time2run);

    if ((iomux_find_func(in->common.lib_flags, &iomux, &iomux_f) != 0) ||
        (tarpc_find_func(in->common.lib_flags, "read", &read_func) != 0) ||
        (tarpc_find_func(in->common.lib_flags, "write", &write_func) != 0) ||
        (tarpc_find_func(in->common.lib_flags, "close", &close_func) != 0) ||
        (tarpc_find_func(in->common.lib_flags, "open",
                         (api_func *)&open_func) != 0))
    {
        ERROR("Failed to resolve functions addresses");
        rc = -1;
        goto local_exit;
    }

    file_d = open_func(path, O_CREAT | O_RDWR, S_IRWXU | S_IRWXG | S_IRWXO);
    if (file_d < 0)
    {
        ERROR("%s(): open(%s, O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO) "
              "failed: %d", __FUNCTION__, path, errno);
        rc = -1;
        goto local_exit;
    }
    INFO("%s(): file '%s' opened with descriptor=%d", __FUNCTION__,
         path, file_d);

    /* Create iomux status and fill it with our fds. */
    if ((rc = iomux_create_state(iomux, &iomux_f, &iomux_st)) != 0)
    {
        rc = -1;
        goto local_exit;
    }
    if ((rc = iomux_add_fd(iomux, &iomux_f, &iomux_st,
                           sock, POLLIN)) != 0)
        goto local_exit;

    if (gettimeofday(&timeout, NULL))
    {
        ERROR("%s(): gettimeofday(timeout) failed: %d",
              __FUNCTION__, errno);
        rc = -1;
        goto local_exit;
    }
    timeout.tv_sec += time2run;
    iomux_timeout = TE_SEC2MS(time2run);

    INFO("%s(): time2run=%ld, timeout timestamp=%ld.%06ld", __FUNCTION__,
         time2run, (long)timeout.tv_sec, (long)timeout.tv_usec);

    do {
        int fd = -1;
        int events = 0;
        session_rx = FALSE;

        rc = iomux_wait(iomux, &iomux_f, &iomux_st, &iomux_ret,
                        iomux_timeout);
        if (rc < 0)
        {
            ERROR("%s(): %s() failed: %d", __FUNCTION__, iomux2str(iomux),
                  errno);
            break;
        }
        VERB("%s(): %s finishes for waiting of events", __FUNCTION__,
             iomux2str(iomux));

        iomux_return_iterate(iomux, &iomux_st, &iomux_ret,
                             IOMUX_RETURN_ITERATOR_START, &fd, &events);

        /* Receive data from socket that are ready */
        if (events & POLLIN)
        {
            VERB("%s(): %s observes data for reading on the "
                 "socket=%d", __FUNCTION__, iomux2str(iomux), sock);
            received = read_func(sock, buffer, sizeof(buffer));
            VERB("%s(): read() retrieve %d bytes", __FUNCTION__, received);
            if (received < 0)
            {
                ERROR("%s(): read() failed: %d", __FUNCTION__, errno);
                rc = -1;
                break;
            }
            else if (received > 0)
            {
                session_rx = TRUE;

                total += received;
                VERB("%s(): write retrieved data to file", __FUNCTION__);
                written = write_func(file_d, buffer, received);
                VERB("%s(): %d bytes are written to file",
                     __FUNCTION__, written);
                if (written < 0)
                {
                    ERROR("%s(): write() failed: %d", __FUNCTION__, errno);
                    rc = -1;
                    break;
                }
                if (written != received)
                {
                    ERROR("%s(): write() cannot write all received in "
                          "the buffer data to the file "
                          "(received=%d, written=%d): %d",
                          __FUNCTION__, received, written, errno);
                    rc = -1;
                    break;
                }
            }
        }

        if (!time2run_expired)
        {
            if (gettimeofday(&timestamp, NULL))
            {
                ERROR("%s(): gettimeofday(timestamp) failed): %d",
                      __FUNCTION__, errno);
                rc = -1;
                break;
            }
            iomux_timeout = TE_SEC2MS(timeout.tv_sec  - timestamp.tv_sec) +
                TE_US2MS(timeout.tv_usec - timestamp.tv_usec);
            if (iomux_timeout < 0)
            {
                time2run_expired = TRUE;
                /* Just to make sure that we'll get all from buffers */
                session_rx = TRUE;
                INFO("%s(): time2run expired", __FUNCTION__);
            }
#ifdef DEBUG
            else if (iomux_timeout < TE_SEC2MS(time2run))
            {
                VERB("%s(): timeout %d", __FUNCTION__, iomux_timeout);
                time2run >>= 1;
            }
#endif
        }

        if (time2run_expired)
        {
            iomux_timeout = TE_SEC2MS(FLOODER_ECHOER_WAIT_FOR_RX_EMPTY);
            VERB("%s(): Waiting for empty Rx queue, Rx=%d",
                 __FUNCTION__, session_rx);
        }

    } while (!time2run_expired || session_rx);

local_exit:
    iomux_close(iomux, &iomux_f, &iomux_st);
    RING("Stop to get data from socket %d and put to file %s, %s, "
         "received %u", sock, path,
         (time2run_expired) ? "timeout expired" :
                              "unexpected failure",
         total);
    INFO("%s(): %s", __FUNCTION__, (rc == 0) ? "OK" : "FAILED");

    if (file_d != -1)
        tarpc_call_close_with_hooks(close_func, file_d);

    if (rc == 0)
    {
        /* Probably, we should restore the errno to the original */
        rc = total;
    }
    return rc;
}

TARPC_FUNC(socket_to_file, {},
{
   MAKE_CALL(out->retval = func_ptr(in));
}
)

#if defined(ENABLE_FTP)
/*-------------- ftp_open() ------------------------------*/

TARPC_FUNC(ftp_open, {},
{
    MAKE_CALL(out->fd = func_ptr(in->uri.uri_val,
                                 in->rdonly ? O_RDONLY : O_WRONLY,
                                 in->passive, in->offset,
                                 (in->sock.sock_len == 0) ? NULL:
                                     in->sock.sock_val));
    if (in->sock.sock_len > 0)
        out->sock = in->sock.sock_val[0];
}
)

/*-------------- ftp_close() ------------------------------*/

TARPC_FUNC(ftp_close, {},
{
    MAKE_CALL(out->ret = func(in->sock));
}
)
#endif

/*-------------- overfill_buffers() -----------------------------*/
int
overfill_buffers(tarpc_overfill_buffers_in *in,
                 tarpc_overfill_buffers_out *out)
{
    int             ret = 0;
    ssize_t         sent = 0;
    int             errno_save = errno;
    api_func        ioctl_func;
    api_func        send_func;
    iomux_funcs     iomux_f;
    iomux_func      iomux = in->iomux;
    size_t          max_len = 4096;
    uint8_t        *buf = NULL;
    uint64_t        total = 0;
    int             unchanged = 0;
    iomux_state     iomux_st;

    te_dbuf sent_data = TE_DBUF_INIT(0);
    te_errno rc;

    out->bytes = 0;

    buf = calloc(1, max_len);
    if (buf == NULL)
    {
        ERROR("%s(): Out of memory", __FUNCTION__);
        out->common._errno = TE_RC(TE_TA_UNIX, TE_ENOMEM);
        ret = -1;
        goto overfill_buffers_exit;
    }

    memset(buf, 0xAD, sizeof(max_len));

    if (tarpc_find_func(in->common.lib_flags, "ioctl", &ioctl_func) != 0)
    {
        ERROR("%s(): Failed to resolve ioctl() function", __FUNCTION__);
        ret = -1;
        goto overfill_buffers_exit;
    }

    if (tarpc_find_func(in->common.lib_flags, "send", &send_func) != 0)
    {
        ERROR("%s(): Failed to resolve send() function", __FUNCTION__);
        ret = -1;
        goto overfill_buffers_exit;
    }

    if (iomux_find_func(in->common.lib_flags, &iomux, &iomux_f) != 0)
    {
        ERROR("%s(): Failed to resolve iomux %s function(s)",
              __FUNCTION__, iomux2str(iomux));
        ret = -1;
        goto overfill_buffers_exit;
    }
    iomux_state_init_invalid(iomux, &iomux_st);

#ifdef __sun__
    /* SunOS has MSG_DONTWAIT flag, but does not support it for send */
    if (!in->is_nonblocking)
    {
        int val = 1;

        if (ioctl_func(in->sock, FIONBIO, &val) != 0)
        {
            out->common._errno = TE_OS_RC(TE_TA_UNIX, errno);
            ERROR("%s(): ioctl() failed: %r", __FUNCTION__,
                  out->common._errno);
            ret = -1;
            goto overfill_buffers_exit;
        }
    }
#endif

    /* Create iomux status and fill it with out fd. */
    if ((ret = iomux_create_state(iomux, &iomux_f, &iomux_st)) != 0 ||
        (ret = iomux_add_fd(iomux, &iomux_f, &iomux_st,
                           in->sock, POLLOUT)) != 0)
    {
        ERROR("%s(): failed to set up iomux %s state", __FUNCTION__,
              iomux2str(iomux));
        goto overfill_buffers_exit;
    }

    /*
     * If total bytes is left unchanged after 3 attempts the socket
     * can be considered as not writable.
     */
    do {
        ret = iomux_wait(iomux, &iomux_f, &iomux_st, NULL, 1000);
        if (ret < 0)
        {
            if (errno == EINTR)
                continue; /* probably, SIGCHLD */
            out->common._errno = TE_OS_RC(TE_TA_UNIX, errno);
            ERROR("%s(): select() failed", __FUNCTION__);
            goto overfill_buffers_exit;
        }

        sent = 0;
        do {
            out->bytes += sent;
            if (in->return_data && sent > 0)
            {
                rc = te_dbuf_append(&sent_data, buf, sent);
                if (rc != 0)
                {
                    te_rpc_error_set(TE_RC(TE_TA_UNIX, rc),
                                     "te_dbuf_append() failed");
                    ret = -1;
                    goto overfill_buffers_exit;
                }
            }

            te_fill_buf(buf, max_len);
            sent = send_func(in->sock, buf, max_len, MSG_DONTWAIT);
            if ((ret > 0) && (sent <= 0))
            {
                if (errno_h2rpc(errno) == RPC_EAGAIN)
                    ERROR("%s(): I/O multiplexing has returned write "
                          "event, but send() function with MSG_DONTWAIT "
                          "hasn't sent any data", __FUNCTION__);
                else
                    ERROR("Send operation failed with %r",
                          errno_h2rpc(errno));
                ret = -1;
                goto overfill_buffers_exit;
            }
            ret = 0;
        } while (sent > 0);
        if (errno != EAGAIN)
        {
            out->common._errno = TE_OS_RC(TE_TA_UNIX, errno);
            ERROR("%s(): send() failed", __FUNCTION__);
            goto overfill_buffers_exit;
        }

        if (total != out->bytes)
        {
            total = out->bytes;
            unchanged = 0;
        }
        else
        {
            unchanged++;
            ret = 0;
        }
    } while (unchanged != 4);

overfill_buffers_exit:
    iomux_close(iomux, &iomux_f, &iomux_st);

#ifdef __sun__
    if (!in->is_nonblocking)
    {
        int val = 0;

        if (ioctl_func(in->sock, FIONBIO, &val) != 0)
        {
            out->common._errno = TE_OS_RC(TE_TA_UNIX, errno);
            ERROR("%s(): ioctl() failed: %r", __FUNCTION__,
                  out->common._errno);
            ret = -1;
        }
    }
#endif

    free(buf);
    if (ret == 0)
        errno = errno_save;

    if (in->return_data)
    {
        if (ret == 0)
        {
            out->data.data_val = sent_data.ptr;
            out->data.data_len = sent_data.len;
        }
        else
        {
            te_dbuf_free(&sent_data);
        }
    }

    return ret;
}

TARPC_FUNC(overfill_buffers,{},
{
    MAKE_CALL(out->retval = func_ptr(in, out));
}
)

/*-------------- iomux_splice() -----------------------------*/
int
iomux_splice(tarpc_iomux_splice_in *in,
             tarpc_iomux_splice_out *out)
{
    int                     ret = 0;
    api_func                splice_func;
    iomux_funcs             iomux_f;
    iomux_func              iomux = in->iomux;
    iomux_state             iomux_st;
    iomux_state             iomux_st_rd;
    iomux_return            iomux_ret;
    iomux_return_iterator   itr;
    struct timeval          now;
    struct timeval          end;
    te_bool                 out_ev = FALSE;
    int                     fd = -1;
    int                     events = 0;

    if (gettimeofday(&end, NULL))
    {
        ERROR("%s(): gettimeofday(now) failed): %d",
              __FUNCTION__, errno);
        return -1;
    }
    end.tv_sec += (time_t)(in->time2run);

    if (tarpc_find_func(in->common.lib_flags, "splice", &splice_func) != 0)
    {
        ERROR("%s(): Failed to resolve splice() function", __FUNCTION__);
        ret = -1;
        goto iomux_splice_exit;
    }

    if (iomux_find_func(in->common.lib_flags, &iomux, &iomux_f) != 0)
    {
        ERROR("%s(): Failed to resolve iomux %s function(s)",
              __FUNCTION__, iomux2str(iomux));
        ret = -1;
        goto iomux_splice_exit;
    }
    iomux_state_init_invalid(iomux, &iomux_st);
    iomux_state_init_invalid(iomux, &iomux_st_rd);

    /* Create iomux status and fill it with in and out fds. */
    if ((ret = iomux_create_state(iomux, &iomux_f, &iomux_st)) != 0 ||
        (ret = iomux_add_fd(iomux, &iomux_f, &iomux_st,
                           in->fd_in, POLLIN)) != 0 ||
        (ret = iomux_add_fd(iomux, &iomux_f, &iomux_st,
                           in->fd_out, POLLOUT)) != 0)
    {
        ERROR("%s(): failed to set up iomux %s state", __FUNCTION__,
              iomux2str(iomux));
        goto iomux_splice_exit;
    }
    /* Create iomux status and fill it with in fd. */
    if ((ret = iomux_create_state(iomux, &iomux_f,
                                  &iomux_st_rd)) != 0 ||
        (ret = iomux_add_fd(iomux, &iomux_f, &iomux_st_rd,
                            in->fd_in, POLLIN)) != 0)
    {
        ERROR("%s(): failed to set up iomux %s state", __FUNCTION__,
              iomux2str(iomux));
        goto iomux_splice_exit;
    }

    do {
        if (out_ev)
            ret = iomux_wait(iomux, &iomux_f, &iomux_st_rd, NULL, 1000);
        else
            ret = iomux_wait(iomux, &iomux_f, &iomux_st, &iomux_ret,
                             1000);
        if (ret < 0)
        {
            if (errno == EINTR)
                continue; /* probably, SIGCHLD */
            out->common._errno = TE_OS_RC(TE_TA_UNIX, errno);
            ERROR("%s(): %s() failed", __FUNCTION__, iomux2str(iomux));
            break;
        }

        if (gettimeofday(&now, NULL))
        {
            ERROR("%s(): gettimeofday(now) failed): %d",
                  __FUNCTION__, errno);
            ret = -1;
            break;
        }

        if (ret == 1 && !out_ev)
        {
            itr = IOMUX_RETURN_ITERATOR_START;
            itr = iomux_return_iterate(iomux, &iomux_st, &iomux_ret,
                                       itr, &fd, &events);
            if (!(events & POLLOUT))
            {
                usleep(10000);
                continue;
            }
            out_ev = TRUE;
            continue;
        }

        if (out_ev && ret == 0)
            continue;
        if (ret == 0)
        {
            usleep(10000);
            continue;
        }

        ret = splice_func(in->fd_in, NULL, in->fd_out, NULL, in->len,
                          splice_flags_rpc2h(in->flags));
        if (ret != (int)in->len)
        {
            ERROR("splice() returned %d instead of %d",
                  ret, in->len);
            ret = -1;
            break;
        }
        out_ev = FALSE;
    } while (end.tv_sec > now.tv_sec);

iomux_splice_exit:
    iomux_close(iomux, &iomux_f, &iomux_st);
    iomux_close(iomux, &iomux_f, &iomux_st_rd);

    return (ret > 0) ? 0 : ret;
}

TARPC_FUNC(iomux_splice,{},
{
    MAKE_CALL(out->retval = func_ptr(in, out));
}
)

/*-------------- overfill_fd() -----------------------------*/
int
overfill_fd(tarpc_overfill_fd_in *in,
              tarpc_overfill_fd_out *out)
{
    int             ret = 0;
    ssize_t         sent = 0;
    int             errno_save = errno;
    api_func        fcntl_func;
    api_func        write_func;
    size_t          max_len = 4096;
    uint8_t        *buf = NULL;
    int             fdflags = -1;

    buf = calloc(1, max_len);
    if (buf == NULL)
    {
        ERROR("%s(): Out of memory", __FUNCTION__);
        out->common._errno = TE_RC(TE_TA_UNIX, TE_ENOMEM);
        ret = -1;
        goto overfill_fd_exit;
    }

    memset(buf, 0xAD, sizeof(max_len));

    if (tarpc_find_func(in->common.lib_flags, "fcntl", &fcntl_func) != 0)
    {
        ERROR("%s(): Failed to resolve fcntl() function", __FUNCTION__);
        ret = -1;
        goto overfill_fd_exit;
    }

    if (tarpc_find_func(in->common.lib_flags, "write", &write_func) != 0)
    {
        ERROR("%s(): Failed to resolve write() function", __FUNCTION__);
        ret = -1;
        goto overfill_fd_exit;
    }

    if ((fdflags = fcntl_func(in->write_end, F_GETFL, O_NONBLOCK)) == -1)
    {
        out->common._errno = TE_OS_RC(TE_TA_UNIX, errno);
        ERROR("%s(): fcntl(F_GETFL) failed: %r", __FUNCTION__,
              out->common._errno);
        ret = -1;
        goto overfill_fd_exit;
    }

    if (!(fdflags & O_NONBLOCK))
    {
        if (fcntl_func(in->write_end, F_SETFL, O_NONBLOCK) == -1)
        {
            out->common._errno = TE_OS_RC(TE_TA_UNIX, errno);
            ERROR("%s(): fcntl(F_SETFL) failed: %r", __FUNCTION__,
                  out->common._errno);
            ret = -1;
            goto overfill_fd_exit;
        }
    }

    sent = 0;
    do {
        out->bytes += sent;
        sent = write_func(in->write_end, buf, max_len);
    } while (sent > 0);

    if (errno != EAGAIN)
    {
        out->common._errno = TE_OS_RC(TE_TA_UNIX, errno);
        ERROR("%s(): write() failed", __FUNCTION__);
        goto overfill_fd_exit;
    }

overfill_fd_exit:

    if (fdflags != -1 && !(fdflags & O_NONBLOCK))
    {
        if (fcntl_func(in->write_end, F_SETFL, fdflags) == -1)
        {
            out->common._errno = TE_OS_RC(TE_TA_UNIX, errno);
            ERROR("%s(): cleanup fcntl(F_SETFL) failed: %r", __FUNCTION__,
                  out->common._errno);
            ret = -1;
        }
    }

    free(buf);
    if (ret == 0)
        errno = errno_save;
    return ret;
}

TARPC_FUNC(overfill_fd,{},
{
    MAKE_CALL(out->retval = func_ptr(in, out));
}
)

TARPC_FUNC(iomux_create_state,{},
{
    iomux_funcs     iomux_f;
    iomux_func      iomux = in->iomux;
    iomux_state    *iomux_st = NULL;
    te_bool         is_fail = FALSE;

    if (iomux_find_func(in->common.lib_flags, &iomux, &iomux_f) != 0)
    {
        ERROR("%s(): Failed to resolve iomux %s function(s)",
              __FUNCTION__, iomux2str(iomux));
        out->common._errno = TE_RC(TE_TA_UNIX, TE_ENOENT);
        is_fail = TRUE;
        goto finish;
    }

    if ((iomux_st = (iomux_state *)malloc(sizeof(*iomux_st))) == NULL)
    {
        out->common._errno = TE_RC(TE_TA_UNIX, TE_ENOMEM);
        is_fail = TRUE;
        goto finish;
    }

    MAKE_CALL(out->retval = func_ptr(iomux, &iomux_f, iomux_st));

    RPC_PCH_MEM_WITH_NAMESPACE(ns, RPC_TYPE_NS_IOMUX_STATE, {
        out->iomux_st = RCF_PCH_MEM_INDEX_ALLOC(iomux_st, ns);
    });

finish:
    if (is_fail)
    {
        out->iomux_st = 0;
        out->retval = -1;
    }
}
)

int
iomux_close_state(tarpc_lib_flags lib_flags, iomux_func iomux,
                  tarpc_iomux_state tapi_iomux_st)
{
    iomux_funcs     iomux_f;
    iomux_state    *iomux_st = NULL;
    int             sock;

    if (iomux_find_func(lib_flags, &iomux, &iomux_f) != 0)
    {
        ERROR("%s(): Failed to resolve iomux %s function(s)",
              __FUNCTION__, iomux2str(iomux));
        return -1;
    }

    RPC_PCH_MEM_WITH_NAMESPACE(ns, RPC_TYPE_NS_IOMUX_STATE, {
        iomux_st = RCF_PCH_MEM_INDEX_MEM_TO_PTR(tapi_iomux_st, ns);
    });

    sock = iomux_close(iomux, &iomux_f, iomux_st);

    free(iomux_st);

    RPC_PCH_MEM_WITH_NAMESPACE(ns, RPC_TYPE_NS_IOMUX_STATE, {
        RCF_PCH_MEM_INDEX_FREE(tapi_iomux_st, ns);
    });

    return sock;
}

TARPC_FUNC(iomux_close_state,{},
{
    MAKE_CALL(out->retval = func_ptr(in->common.lib_flags, in->iomux,
                                     in->iomux_st));
}
)

int
multiple_iomux_wait_common(iomux_funcs iomux_f, iomux_func iomux,
                           iomux_state *iomux_st, tarpc_int tapi_events,
                           tarpc_int fd, tarpc_int count,
                           tarpc_int duration, tarpc_int exp_rc,
                           tarpc_int *number, tarpc_int *last_rc,
                           tarpc_int *zero_rc)
{
    int             ret;
    int             events;
    int             i;
    int             saved_errno = 0;
    int             zero_ret = 0;
    struct timeval  tv_start;
    struct timeval  tv_finish;

    events = poll_event_rpc2h(tapi_events);

    if ((ret = iomux_add_fd(iomux, &iomux_f, iomux_st,
                            fd, events)) != 0)
    {
        ERROR("%s(): failed to set up iomux %s state", __FUNCTION__,
              iomux2str(iomux));
        return -1;
    }

    if (duration != -1)
        gettimeofday(&tv_start, NULL);

    for (i = 0; i < count || count == -1; i++)
    {
        ret = iomux_wait(iomux, &iomux_f, iomux_st, NULL, 0);
        if (ret == 0)
            zero_ret++;
        else if (ret < 0)
        {
            saved_errno = errno;
            ERROR("%s(): iomux failed with errno %s",
                  strerror(saved_errno));
            break;
        }
        else if (ret != exp_rc)
        {
            ERROR("%s(): unexpected value %d was returned by iomux call",
                  __FUNCTION__, ret);
            break;
        }

        if (duration != -1)
        {
            gettimeofday(&tv_finish, NULL);
            if (duration < (tv_finish.tv_sec - tv_start.tv_sec) * 1000 +
                           (tv_finish.tv_usec - tv_start.tv_usec) / 1000)
            break;
        }
    }

    *number = i;
    *last_rc = ret;
    *zero_rc = zero_ret;

    if (saved_errno != 0)
        errno = saved_errno;

    return 0;
}

/*-------------- multiple_iomux_wait() ----------------------*/
int
multiple_iomux_wait(tarpc_multiple_iomux_wait_in *in,
                    tarpc_multiple_iomux_wait_out *out)
{
    iomux_funcs     iomux_f;
    iomux_func      iomux = in->iomux;
    iomux_state    *iomux_st = NULL;
    int             rc;

    if (iomux_find_func(in->common.lib_flags, &iomux, &iomux_f) != 0)
    {
        ERROR("%s(): Failed to resolve iomux %s function(s)",
              __FUNCTION__, iomux2str(iomux));
        return -1;
    }

    RPC_PCH_MEM_WITH_NAMESPACE(ns, RPC_TYPE_NS_IOMUX_STATE, {
        iomux_st = RCF_PCH_MEM_INDEX_MEM_TO_PTR(in->iomux_st, ns);
    });

    rc = multiple_iomux_wait_common(iomux_f, iomux, iomux_st, in->events,
                                    in->fd, in->count, in->duration,
                                    in->exp_rc, &out->number, &out->last_rc,
                                    &out->zero_rc);

    if (out->last_rc != in->exp_rc || out->number < in->count || rc != 0)
        return -1;

    return 0;
}

TARPC_FUNC(multiple_iomux_wait,{},
{
    MAKE_CALL(out->retval = func_ptr(in, out));
}
)

/*-------------- multiple_iomux() ----------------------*/
int
multiple_iomux(tarpc_multiple_iomux_in *in,
               tarpc_multiple_iomux_out *out)
{
    iomux_funcs     iomux_f;
    iomux_func      iomux = in->iomux;
    iomux_state     iomux_st;
    int             ret;

    if (iomux_find_func(in->common.lib_flags, &iomux, &iomux_f) != 0)
    {
        ERROR("%s(): Failed to resolve iomux %s function(s)",
              __FUNCTION__, iomux2str(iomux));
        return -1;
    }

    if ((ret = iomux_create_state(iomux, &iomux_f, &iomux_st)) != 0)
    {
        ERROR("%s(): failed to set up iomux %s state", __FUNCTION__,
              iomux2str(iomux));
        return -1;
    }

    ret = multiple_iomux_wait_common(iomux_f, iomux, &iomux_st, in->events,
                                     in->fd, in->count, in->duration,
                                     in->exp_rc, &out->number, &out->last_rc,
                                     &out->zero_rc);

    iomux_close(iomux, &iomux_f, &iomux_st);

    if (out->last_rc != in->exp_rc || out->number < in->count || ret != 0)
        return -1;

    return 0;
}

TARPC_FUNC(multiple_iomux,{},
{
    MAKE_CALL(out->retval = func_ptr(in, out));
}
)

#ifdef LIO_READ

#ifdef HAVE_UNION_SIGVAL_SIVAL_PTR
#define SIVAL_PTR sival_ptr
#elif defined(HAVE_UNION_SIGVAL_SIGVAL_PTR)
#define SIVAL_PTR sigval_ptr
#else
#error "Failed to discover memeber names of the union sigval."
#endif

#ifdef HAVE_UNION_SIGVAL_SIVAL_INT
#define SIVAL_INT       sival_int
#elif defined(HAVE_UNION_SIGVAL_SIGVAL_INT)
#define SIVAL_INT       sigval_int
#else
#error "Failed to discover memeber names of the union sigval."
#endif

#ifdef SIGEV_THREAD
static te_errno
fill_sigev_thread(struct sigevent *sig, char *function)
{
    if (strlen(function) > 0)
    {
        if ((sig->sigev_notify_function =
                rcf_ch_symbol_addr(function, 1)) == NULL)
        {
            if (strcmp(function, AIO_WRONG_CALLBACK) == 0)
                sig->sigev_notify_function =
                    (void *)(long)rand_range(1, 0xFFFFFFFF);
            else
                WARN("Failed to find address of AIO callback %s - "
                     "use NULL callback", function);
        }
    }
    else
    {
        sig->sigev_notify_function = NULL;
    }
    sig->sigev_notify_attributes = NULL;

    return 0;
}
#else
static te_errno
fill_sigev_thread(struct sigevent *sig, char *function)
{
    UNUSED(sig);
    UNUSED(function);
    return TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP);
}
#endif

/*-------------- AIO control block constructor -------------------------*/
bool_t
_create_aiocb_1_svc(tarpc_create_aiocb_in *in, tarpc_create_aiocb_out *out,
                    struct svc_req *rqstp)
{
    struct aiocb *cb;

    UNUSED(rqstp);
    UNUSED(in);

    memset(out, 0, sizeof(*out));

    errno = 0;
    if ((cb = (struct aiocb *)malloc(sizeof(struct aiocb))) == NULL)
    {
        out->common._errno = TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }
    else
    {
        memset((void *)cb, 0, sizeof(*cb));
        out->cb = rcf_pch_mem_alloc(cb);
        out->common._errno = RPC_ERRNO;
    }

    return TRUE;
}

/*-------------- AIO control block constructor --------------------------*/
bool_t
_fill_aiocb_1_svc(tarpc_fill_aiocb_in *in,
                  tarpc_fill_aiocb_out *out,
                  struct svc_req *rqstp)
{
    struct aiocb *cb = IN_AIOCB;

    UNUSED(rqstp);

    memset(out, 0, sizeof(*out));

    if (cb == NULL)
    {
        ERROR("Try to fill NULL AIO control block");
        out->common._errno = TE_RC(TE_TA_UNIX, TE_EINVAL);
        return TRUE;
    }

    cb->aio_fildes = in->fildes;
    cb->aio_lio_opcode = lio_opcode_rpc2h(in->lio_opcode);
    cb->aio_reqprio = in->reqprio;
    cb->aio_buf = rcf_pch_mem_get(in->buf);
    cb->aio_nbytes = in->nbytes;
    if (in->sigevent.value.pointer)
    {
        cb->aio_sigevent.sigev_value.SIVAL_PTR =
            rcf_pch_mem_get(in->sigevent.value.tarpc_sigval_u.sival_ptr);
    }
    else
    {
        cb->aio_sigevent.sigev_value.SIVAL_INT =
            in->sigevent.value.tarpc_sigval_u.sival_int;
    }

    cb->aio_sigevent.sigev_signo = signum_rpc2h(in->sigevent.signo);
    cb->aio_sigevent.sigev_notify = sigev_notify_rpc2h(in->sigevent.notify);
    out->common._errno = fill_sigev_thread(&(cb->aio_sigevent),
                                           in->sigevent.function);
    return TRUE;
}


/*-------------- AIO control block destructor --------------------------*/
bool_t
_delete_aiocb_1_svc(tarpc_delete_aiocb_in *in,
                     tarpc_delete_aiocb_out *out,
                     struct svc_req *rqstp)
{
    UNUSED(rqstp);

    memset(out, 0, sizeof(*out));

    errno = 0;
    free(IN_AIOCB);
    rcf_pch_mem_free(in->cb);
    out->common._errno = RPC_ERRNO;

    return TRUE;
}

/*---------------------- aio_read() --------------------------*/
TARPC_FUNC(aio_read, {},
{
    MAKE_CALL(out->retval = func_ptr(IN_AIOCB));
}
)

/*---------------------- aio_write() --------------------------*/
TARPC_FUNC(aio_write, {},
{
    MAKE_CALL(out->retval = func_ptr(IN_AIOCB));
}
)

/*---------------------- aio_return() --------------------------*/
TARPC_FUNC(aio_return, {},
{
    MAKE_CALL(out->retval = func_ptr(IN_AIOCB));
}
)

/*---------------------- aio_error() --------------------------*/
TARPC_FUNC(aio_error, {},
{
    MAKE_CALL(out->retval = TE_OS_RC(TE_RPC, func_ptr(IN_AIOCB)));
}
)

/*---------------------- aio_cancel() --------------------------*/
TARPC_FUNC(aio_cancel, {},
{
    MAKE_CALL(out->retval =
                  aio_cancel_retval_h2rpc(func(in->fd, IN_AIOCB)));
}
)

/*---------------------- aio_fsync() --------------------------*/
TARPC_FUNC(aio_fsync, {},
{
    MAKE_CALL(out->retval = func(fcntl_flags_rpc2h(in->op), IN_AIOCB));
}
)

/*---------------------- aio_suspend() --------------------------*/
TARPC_FUNC(aio_suspend, {},
{
    struct aiocb  **cb = NULL;
    struct timespec tv;
    int             i;

    if (in->timeout.timeout_len > 0)
    {
        tv.tv_sec = in->timeout.timeout_val[0].tv_sec;
        tv.tv_nsec = in->timeout.timeout_val[0].tv_nsec;
        INIT_CHECKED_ARG((char *)&tv, sizeof(tv), 0);
    }

    if (in->cb.cb_len > 0 &&
        (cb = (struct aiocb **)calloc(in->cb.cb_len,
                                      sizeof(void *))) == NULL)
    {
        ERROR("Out of memory");
        out->common._errno = TE_RC(TE_TA_UNIX, TE_ENOMEM);
        goto finish;
    }

    for (i = 0; i < (int)(in->cb.cb_len); i++)
        cb[i] = (struct aiocb *)rcf_pch_mem_get(in->cb.cb_val[i]);

    INIT_CHECKED_ARG((void *)cb, sizeof(void *) * in->cb.cb_len,
                     sizeof(void *) * in->cb.cb_len);

    MAKE_CALL(out->retval = func_ptr((const struct aiocb * const *)cb, in->n,
                                     in->timeout.timeout_len == 0 ?
                                     NULL : &tv));
    free(cb);

    finish:
    ;
}
)


/*---------------------- lio_listio() --------------------------*/
TARPC_FUNC(lio_listio, {},
{
    struct aiocb  **cb = NULL;
    struct sigevent sig;
    int             i;

    if (in->sig.sig_len > 0)
    {
        tarpc_sigevent *ev = in->sig.sig_val;

        if (ev->value.pointer)
        {
            sig.sigev_value.SIVAL_PTR =
                rcf_pch_mem_get(ev->value.tarpc_sigval_u.sival_ptr);
        }
        else
        {
            sig.sigev_value.SIVAL_INT =
                ev->value.tarpc_sigval_u.sival_int;
        }

        sig.sigev_signo = signum_rpc2h(ev->signo);
        sig.sigev_notify = sigev_notify_rpc2h(ev->notify);
        out->common._errno = fill_sigev_thread(&sig, ev->function);
        INIT_CHECKED_ARG((char *)&sig, sizeof(sig), 0);
    }

    if (in->cb.cb_len > 0 &&
        (cb = (struct aiocb **)calloc(in->cb.cb_len,
                                      sizeof(void *))) == NULL)
    {
        ERROR("Out of memory");
        out->common._errno = TE_RC(TE_TA_UNIX, TE_ENOMEM);
        goto finish;
    }

    for (i = 0; i < (int)(in->cb.cb_len); i++)
        cb[i] = (struct aiocb *)rcf_pch_mem_get(in->cb.cb_val[i]);

    INIT_CHECKED_ARG((void *)cb, sizeof(void *) * in->cb.cb_len,
                     sizeof(void *) * in->cb.cb_len);

    MAKE_CALL(out->retval = func(lio_mode_rpc2h(in->mode), cb, in->nent,
                                 in->sig.sig_len == 0 ? NULL : &sig));
    free(cb);

    finish:
    ;
}
)

#endif

/*--------------------------- malloc ---------------------------------*/
TARPC_FUNC(malloc, {},
{
    void *buf;

    buf = func_ret_ptr(in->size);

    if (buf == NULL)
        out->common._errno = TE_RC(TE_TA_UNIX, TE_ENOMEM);
    else
        out->retval = rcf_pch_mem_alloc(buf);
}
)

/*--------------------------- free ---------------------------------*/
TARPC_FUNC(free, {},
{
    UNUSED(out);
    func_ptr(rcf_pch_mem_get(in->buf));
    rcf_pch_mem_free(in->buf);
}
)

/*------------ get_addr_by_id ---------------------------*/
bool_t
_get_addr_by_id_1_svc(tarpc_get_addr_by_id_in  *in,
                      tarpc_get_addr_by_id_out *out,
                      struct svc_req           *rqstp)
{
    UNUSED(rqstp);
    out->retval =
            (uint64_t)((uint8_t *)rcf_pch_mem_get(in->id) -
                       (uint8_t *)NULL);

    return TRUE;
}

/*------------ raw2integer ---------------------------*/
/**
 * Convert raw data to integer.
 *
 * @param in    Input data
 * @param out   Output data
 *
 * @return @c 0 on success or @c -1
 */
int
raw2integer(tarpc_raw2integer_in *in,
            tarpc_raw2integer_out *out)
{
    uint8_t     single_byte;
    uint16_t    two_bytes;
    uint32_t    four_bytes;
    uint64_t    eight_bytes;

    if (in->data.data_val == NULL ||
        in->data.data_len == 0)
    {
        RING("%s(): trying to convert zero-length value",
             __FUNCTION__);
        return 0;
    }

    if (in->data.data_len == sizeof(single_byte))
    {
        single_byte = *(uint8_t *)(in->data.data_val);
        out->number = single_byte;
    }
    else if (in->data.data_len == sizeof(two_bytes))
    {
        two_bytes = *(uint16_t *)(in->data.data_val);
        out->number = two_bytes;
    }
    else if (in->data.data_len == sizeof(four_bytes))
    {
        four_bytes = *(uint32_t *)(in->data.data_val);
        out->number = four_bytes;
    }
    else if (in->data.data_len == sizeof(eight_bytes))
    {
        eight_bytes = *(uint64_t *)(in->data.data_val);
        out->number = eight_bytes;
    }
    else if (in->data.data_len <= sizeof(out->number))
    {
        int      x;
        uint8_t *p;

        WARN("%s(): incorrect length %d for raw data, "
             "trying to interpret according to endianness",
             __FUNCTION__, in->data.data_len);

        out->number = 0;
        x = 1;
        p = (uint8_t *)&x;
        if (p[sizeof(x) - 1] > 0)
        {
            /* Big-endian */
            p = (uint8_t *)&out->number +
                    (sizeof(out->number) - in->data.data_len);
            memcpy(p, in->data.data_val, in->data.data_len);
        }
        else /* Little-endian */
            memcpy(&out->number, in->data.data_val,
                   in->data.data_len);
    }
    else
    {
        ERROR("%s(): incorrect length %d for integer data",
              __FUNCTION__, in->data.data_len);
        out->common._errno = TE_RC(TE_TA_UNIX, TE_EINVAL);
        return -1;
    }

    return 0;
}

TARPC_FUNC(raw2integer, {},
{
    MAKE_CALL(out->retval = func_ptr(in, out));
}
)

/*------------ integer2raw ---------------------------*/
/**
 * Convert integer value to raw representation.
 *
 * @param in    Input data
 * @param out   Output data
 *
 * @return @c 0 on success or @c -1
 */
int
integer2raw(tarpc_integer2raw_in *in,
            tarpc_integer2raw_out *out)
{
    uint8_t     single_byte;
    uint16_t    two_bytes;
    uint32_t    four_bytes;
    uint64_t    eight_bytes;

    void *p = NULL;

    if (in->len == 0)
    {
        RING("%s(): trying to convert zero-length value",
             __FUNCTION__);
        return 0;
    }

    out->data.data_val = NULL;
    out->data.data_len = 0;

    if (in->len == sizeof(single_byte))
    {
        single_byte = (uint8_t)in->number;
        p = &single_byte;
    }
    else if (in->len == sizeof(two_bytes))
    {
        two_bytes = (uint16_t)in->number;
        p = &two_bytes;
    }
    else if (in->len == sizeof(four_bytes))
    {
        four_bytes = (uint32_t)in->number;
        p = &four_bytes;
    }
    else if (in->len == sizeof(eight_bytes))
    {
        eight_bytes = (uint64_t)in->number;
        p = &eight_bytes;
    }
    else
    {
        ERROR("%s(): incorrect length %d for integer data",
              __FUNCTION__, in->len);
        out->common._errno = TE_RC(TE_TA_UNIX, TE_EINVAL);
        return -1;
    }

    out->data.data_val = calloc(1, in->len);
    if (out->data.data_val == NULL)
    {
        ERROR("%s(): failed to allocate space for integer data",
              __FUNCTION__);
        out->common._errno = TE_RC(TE_TA_UNIX, TE_ENOMEM);
        return -1;
    }

    memcpy(out->data.data_val, p, in->len);
    out->data.data_len = in->len;

    return 0;
}

TARPC_FUNC(integer2raw, {},
{
    MAKE_CALL(out->retval = func_ptr(in, out));
}
)

/*-------------- memalign() ------------------------------*/

/* FIXME: provide prototype (proper header inclusion?) */
TARPC_FUNC_DYNAMIC_UNSAFE(memalign, {},
{
    void *buf;

    buf = func_ret_ptr(in->alignment, in->size);

    if (buf == NULL)
        out->common._errno = TE_RC(TE_TA_UNIX, errno);
    else
        out->retval = rcf_pch_mem_alloc(buf);
}
)

/*-------------- mmap() ------------------------------*/

TARPC_FUNC(mmap, {},
{
    void *p;

    MAKE_CALL(p = func_ptr_ret_ptr(
                           (void *)(uintptr_t)(in->addr),
                           (size_t)(in->length),
                           prot_flags_rpc2h(in->prot),
                           map_flags_rpc2h(in->flags),
                           in->fd, (off_t)(in->offset)));

    if (p != MAP_FAILED)
        out->retval = rcf_pch_mem_alloc(p);
    else
        out->retval = RPC_NULL;
}
)

/*-------------- munmap() ------------------------------*/

TARPC_FUNC(munmap, {},
{
    MAKE_CALL(out->retval = func_ptr(rcf_pch_mem_get(in->addr),
                                     (size_t)(in->length)));
    if (out->retval >= 0)
        rcf_pch_mem_free(in->addr);
}
)

/*-------------- madvise() ------------------------------*/

TARPC_FUNC(madvise, {},
{
    MAKE_CALL(out->retval = func_ptr(rcf_pch_mem_get(in->addr),
                                     (size_t)(in->length),
                                     madv_value_rpc2h(in->advise)));
}
)

/*-------------- memcmp() ------------------------------*/

TARPC_FUNC(memcmp, {},
{
    out->retval = func_void(rcf_pch_mem_get(in->s1_base) + in->s1_off,
                            rcf_pch_mem_get(in->s2_base) + in->s2_off,
                            in->n);
}
)

/*-------------------------- Fill buffer ----------------------------*/
void
set_buf(const char *src_buf,
        tarpc_ptr dst_buf_base, size_t dst_offset, size_t len)
{
    char *dst_buf = rcf_pch_mem_get(dst_buf_base);

    if (dst_buf != NULL && len != 0)
        memcpy(dst_buf + dst_offset, src_buf, len);
    else if (len != 0)
        errno = EFAULT;
}

TARPC_FUNC(set_buf, {},
{
    MAKE_CALL(func_ptr(in->src_buf.src_buf_val, in->dst_buf, in->dst_off,
                       in->src_buf.src_buf_len));
}
)

/*-------------------------- Read buffer ----------------------------*/
void
get_buf(tarpc_ptr src_buf_base, size_t src_offset,
        char **dst_buf, size_t *len)
{
    char *src_buf = rcf_pch_mem_get(src_buf_base);

    *dst_buf = NULL;
    if (src_buf != NULL && *len != 0)
    {
        char *buf = malloc(*len);

        if (buf == NULL)
        {
            RING("%s(): failed to allocate %ld bytes",
                 __FUNCTION__, (long int)*len);
            *len = 0;
            errno = ENOMEM;
        }
        else
        {
            memcpy(buf, src_buf + src_offset, *len);
            *dst_buf = buf;
        }
    }
    else if (*len != 0)
    {
        RING("%s(): trying to get bytes from NULL address",
             __FUNCTION__);
        errno = EFAULT;
        *len = 0;
    }
}

TARPC_FUNC(get_buf, {},
{
    size_t len = in->len;

    MAKE_CALL(func(in->src_buf, in->src_off,
                   &out->dst_buf.dst_buf_val,
                   &len));

    out->dst_buf.dst_buf_len = len;
}
)

/*---------------------- Fill buffer by the pattern ----------------------*/
void
set_buf_pattern(int pattern,
                tarpc_ptr dst_buf_base, size_t dst_offset, size_t len)
{
    char *dst_buf = rcf_pch_mem_get(dst_buf_base);

    if (dst_buf != NULL && len != 0)
    {
        if (pattern < TAPI_RPC_BUF_RAND)
            memset(dst_buf + dst_offset, pattern, len);
        else
        {
            unsigned int i;

            for (i = 0; i < len; i++)
                dst_buf[i] = rand() % TAPI_RPC_BUF_RAND;
        }
    }
    else if (len != 0)
        errno = EFAULT;

}

TARPC_FUNC(set_buf_pattern, {},
{
    MAKE_CALL(func(in->pattern, in->dst_buf, in->dst_off, in->len));
}
)

/*-------------- setrlimit() ------------------------------*/

bool_t
_setrlimit_1_svc(tarpc_setrlimit_in *in,
                 tarpc_setrlimit_out *out,
                 struct svc_req *rqstp)
{
    struct rlimit   rlim;
    api_func        func;
    int             rc;

/*
 * Checking for __USE_FILE_OFFSET64 seems enough in both i386 and x86_64
 * cases: the setrlimit64 function does exist.
 */
#ifdef __USE_FILE_OFFSET64
    static const char *func_name = "setrlimit64";
#else
    static const char *func_name = "setrlimit";
#endif

    UNUSED(rqstp);

    rlim.rlim_cur = in->rlim.rlim_val->rlim_cur;
    rlim.rlim_max = in->rlim.rlim_val->rlim_max;

    VERB("%s() looking for %s() function", __FUNCTION__, func_name);
    if ((rc = tarpc_find_func(in->common.lib_flags, func_name, &func)) != 0)
    {
        ERROR("Failed to resolve \"%s\" function address", func_name);
        out->common._errno = rc;
        return TRUE;
    }

    out->retval = func(rlimit_resource_rpc2h(in->resource), &rlim);

    return TRUE;
}

/*-------------- getrlimit() ------------------------------*/

bool_t
_getrlimit_1_svc(tarpc_getrlimit_in *in,
                 tarpc_getrlimit_out *out,
                 struct svc_req *rqstp)
{
    struct rlimit   rlim;
    api_func        func;
    int             rc;

/*
 * Checking for __USE_FILE_OFFSET64 seems enough in both i386 and x86_64
 * cases: the getrlimit64 function does exist.
 */
#ifdef __USE_FILE_OFFSET64
    static const char *func_name = "getrlimit64";
#else
    static const char *func_name = "getrlimit";
#endif

    UNUSED(rqstp);
    COPY_ARG(rlim);

    if (out->rlim.rlim_len > 0)
    {
        rlim.rlim_cur = out->rlim.rlim_val->rlim_cur;
        rlim.rlim_max = out->rlim.rlim_val->rlim_max;
    }

    VERB("%s() looking for %s() function", __FUNCTION__, func_name);
    rc = tarpc_find_func(in->common.lib_flags, func_name, &func);
    if (rc != 0)
    {
        ERROR("Failed to resolve \"%s\" function address", func_name);
        out->common._errno = rc;
        return TRUE;
    }

    out->retval = func(rlimit_resource_rpc2h(in->resource), &rlim);

    if (out->rlim.rlim_len > 0)
    {
        out->rlim.rlim_val->rlim_cur = rlim.rlim_cur;
        out->rlim.rlim_val->rlim_max = rlim.rlim_max;
    }

    return TRUE;
}

/*-------------- sysconf() ------------------------------*/

TARPC_FUNC(sysconf, {},
{
    MAKE_CALL(out->retval = func(sysconf_name_rpc2h(in->name)));
}
)

#if defined(ENABLE_POWER_SW)
/*------------ power_sw() -----------------------------------*/
/* FIXME: provide proper prototype */
TARPC_FUNC(power_sw, {},
{
    MAKE_CALL(out->retval = func(in->type, in->dev, in->mask, in->cmd));
}
)
#endif

/*------------ mcast_join_leave() ---------------------------*/
void
mcast_join_leave(tarpc_mcast_join_leave_in  *in,
                 tarpc_mcast_join_leave_out *out)
{
    api_func            setsockopt_func;
    api_func_ret_ptr    if_indextoname_func;
    api_func            ioctl_func;

    if (tarpc_find_func(in->common.lib_flags, "setsockopt",
                        &setsockopt_func) != 0 ||
        tarpc_find_func(in->common.lib_flags, "if_indextoname",
                        (api_func *)&if_indextoname_func) != 0 ||
        tarpc_find_func(in->common.lib_flags, "ioctl",
                        &ioctl_func) != 0)
    {
        ERROR("Cannot resolve function name");
        out->retval = -1;
        out->common._errno = TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP);
        return;
    }

    memset(out, 0, sizeof(tarpc_mcast_join_leave_out));
    if (in->family == RPC_AF_INET6)
    {
        assert(in->multiaddr.multiaddr_len == sizeof(struct in6_addr));
        switch (in->how)
        {
            case TARPC_MCAST_ADD_DROP:
            {
#ifdef IPV6_ADD_MEMBERSHIP
                struct ipv6_mreq mreq;

                memcpy(&mreq.ipv6mr_multiaddr, in->multiaddr.multiaddr_val,
                       sizeof(struct in6_addr));
                mreq.ipv6mr_interface = in->ifindex;
                out->retval = setsockopt_func(in->fd, IPPROTO_IPV6,
                                              in->leave_group ?
                                                  IPV6_DROP_MEMBERSHIP :
                                                  IPV6_ADD_MEMBERSHIP,
                                              &mreq, sizeof(mreq));
                if (out->retval != 0)
                {
                    ERROR("Attempt to join IPv6 multicast group failed");
                    out->common._errno = TE_RC(TE_TA_UNIX, errno);
                }
#else
                ERROR("IPV6_ADD_MEMBERSHIP is not supported "
                      "for current Agent type");
                out->retval = -1;
                out->common._errno = TE_RC(TE_TA_UNIX, TE_EINVAL);
#endif
                break;
            }

            case TARPC_MCAST_JOIN_LEAVE:
            {
#ifdef MCAST_LEAVE_GROUP
                struct group_req     gr_req;
                struct sockaddr_in6 *sin6;

                sin6 = SIN6(&gr_req.gr_group);
                sin6->sin6_family = AF_INET6;
                memcpy(&sin6->sin6_addr, in->multiaddr.multiaddr_val,
                       sizeof(struct in6_addr));
                gr_req.gr_interface = in->ifindex;
                out->retval = setsockopt_func(in->fd, IPPROTO_IPV6,
                                              in->leave_group ?
                                                  MCAST_LEAVE_GROUP :
                                                  MCAST_JOIN_GROUP,
                                              &gr_req, sizeof(gr_req));
                if (out->retval != 0)
                {
                    ERROR("Attempt to join IPv6 multicast group failed");
                    out->common._errno = TE_RC(TE_TA_UNIX, errno);
                }
#else
                ERROR("MCAST_LEAVE_GROUP is not supported "
                      "for current Agent type");
                out->retval = -1;
                out->common._errno = TE_RC(TE_TA_UNIX, TE_EINVAL);
#endif
                break;
            }
            default:
                ERROR("Unknown multicast join method");
                out->retval = -1;
                out->common._errno = TE_RC(TE_TA_UNIX, TE_EINVAL);
        }
        return;
    }
    else if (in->family == RPC_AF_INET)
    {
        assert(in->multiaddr.multiaddr_len == sizeof(struct in_addr));
        switch (in->how)
        {
            case TARPC_MCAST_ADD_DROP:
            {
#if HAVE_STRUCT_IP_MREQN
                struct ip_mreqn mreq;

                memset(&mreq, 0, sizeof(mreq));
                mreq.imr_ifindex = in->ifindex;
#else
                char              if_name[IFNAMSIZ];
                struct ifreq      ifrequest;
                struct ip_mreq    mreq;

                memset(&mreq, 0, sizeof(mreq));

                if (in->ifindex != 0)
                {
                    if (if_indextoname_func(in->ifindex, if_name) == NULL)
                    {
                        ERROR("Invalid interface index specified");
                        out->retval = -1;
                        out->common._errno = TE_RC(TE_TA_UNIX, TE_ENXIO);
                        return;
                    }
                    else
                    {
                        memset(&ifrequest, 0, sizeof(struct ifreq));
                        memcpy(&(ifrequest.ifr_name), if_name, IFNAMSIZ);
                        if (ioctl_func(in->fd, SIOCGIFADDR, &ifrequest) < 0)
                        {
                            ERROR("No IPv4 address on interface %s",
                                  if_name);
                            out->retval = -1;
                            out->common._errno = TE_RC(TE_TA_UNIX,
                                                       TE_ENXIO);
                            return;
                        }

                        memcpy(&mreq.imr_interface,
                               &SIN(&ifrequest.ifr_addr)->sin_addr,
                               sizeof(struct in_addr));
                    }
                }
#endif
                memcpy(&mreq.imr_multiaddr, in->multiaddr.multiaddr_val,
                       sizeof(struct in_addr));
                out->retval = setsockopt_func(in->fd, IPPROTO_IP,
                                              in->leave_group ?
                                                  IP_DROP_MEMBERSHIP :
                                                  IP_ADD_MEMBERSHIP,
                                              &mreq, sizeof(mreq));
                if (out->retval != 0)
                {
                    ERROR("Attempt to join IPv4 multicast group failed");
                    out->common._errno = TE_RC(TE_TA_UNIX, errno);
                }
                break;
            }

            case TARPC_MCAST_JOIN_LEAVE:
            {
#ifdef MCAST_LEAVE_GROUP
                struct group_req     gr_req;
                struct sockaddr_in  *sin;

                sin = SIN(&gr_req.gr_group);
                sin->sin_family = AF_INET;
                memcpy(&sin->sin_addr, in->multiaddr.multiaddr_val,
                       sizeof(struct in_addr));
                gr_req.gr_interface = in->ifindex;
                out->retval = setsockopt_func(in->fd, IPPROTO_IP,
                                              in->leave_group ?
                                                  MCAST_LEAVE_GROUP :
                                                  MCAST_JOIN_GROUP,
                                              &gr_req, sizeof(gr_req));
                if (out->retval != 0)
                {
                    ERROR("Attempt to join IP multicast group failed");
                    out->common._errno = TE_RC(TE_TA_UNIX, errno);
                }
#else
                ERROR("MCAST_LEAVE_GROUP is not supported "
                      "for current Agent type");
                out->retval = -1;
                out->common._errno = TE_RC(TE_TA_UNIX, TE_EINVAL);
#endif
                break;
            }
            default:
                ERROR("Unknown multicast join method");
                out->retval = -1;
                out->common._errno = TE_RC(TE_TA_UNIX, TE_EINVAL);
        }
    }
    else
    {
        ERROR("Unknown multicast address family %d", in->family);
        out->retval = -1;
        out->common._errno = TE_RC(TE_TA_UNIX, TE_EINVAL);
    }
}

TARPC_FUNC(mcast_join_leave, {},
{
    MAKE_CALL(func_ptr(in, out));
})

/*------------ mcast_source_join_leave() -----------------------*/
void
mcast_source_join_leave(tarpc_mcast_source_join_leave_in  *in,
                        tarpc_mcast_source_join_leave_out *out)
{
    api_func            setsockopt_func;
    api_func_ret_ptr    if_indextoname_func;
    api_func            ioctl_func;

    if (tarpc_find_func(in->common.lib_flags, "setsockopt",
                        &setsockopt_func) != 0 ||
        tarpc_find_func(in->common.lib_flags, "if_indextoname",
                        (api_func *)&if_indextoname_func) != 0 ||
        tarpc_find_func(in->common.lib_flags, "ioctl",
                        &ioctl_func) != 0)
    {
        ERROR("Cannot resolve function name");
        out->retval = -1;
        out->common._errno = TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP);
        return;
    }

    memset(out, 0, sizeof(tarpc_mcast_source_join_leave_out));
    if (in->family == RPC_AF_INET)
    {
        assert(in->multiaddr.multiaddr_len == sizeof(struct in_addr));
        assert(in->sourceaddr.sourceaddr_len == sizeof(struct in_addr));
        switch (in->how)
        {
            case TARPC_MCAST_SOURCE_ADD_DROP:
            {
#ifdef IP_DROP_SOURCE_MEMBERSHIP
                char                    if_name[IFNAMSIZ];
                struct ifreq            ifrequest;
                struct ip_mreq_source   mreq;

                memset(&mreq, 0, sizeof(mreq));

                if (in->ifindex != 0)
                {
                    if (if_indextoname_func(in->ifindex, if_name) == NULL)
                    {
                        ERROR("Invalid interface index specified");
                        out->retval = -1;
                        out->common._errno = TE_RC(TE_TA_UNIX, TE_ENXIO);
                        return;
                    }
                    else
                    {
                        memset(&ifrequest, 0, sizeof(struct ifreq));
                        memcpy(&(ifrequest.ifr_name), if_name, IFNAMSIZ);
                        if (ioctl_func(in->fd, SIOCGIFADDR, &ifrequest) < 0)
                        {
                            ERROR("No IPv4 address on interface %s",
                                  if_name);
                            out->retval = -1;
                            out->common._errno = TE_RC(TE_TA_UNIX,
                                                       TE_ENXIO);
                            return;
                        }

                        memcpy(&mreq.imr_interface,
                               &SIN(&ifrequest.ifr_addr)->sin_addr,
                               sizeof(struct in_addr));
                    }
                }
                memcpy(&mreq.imr_multiaddr, in->multiaddr.multiaddr_val,
                       sizeof(struct in_addr));
                memcpy(&mreq.imr_sourceaddr, in->sourceaddr.sourceaddr_val,
                       sizeof(struct in_addr));
                out->retval = setsockopt_func(in->fd, IPPROTO_IP,
                                              in->leave_group ?
                                                IP_DROP_SOURCE_MEMBERSHIP :
                                                IP_ADD_SOURCE_MEMBERSHIP,
                                              &mreq, sizeof(mreq));
                if (out->retval != 0)
                {
                    ERROR("Attempt to join IPv4 multicast group failed");
                    out->common._errno = TE_RC(TE_TA_UNIX, errno);
                }
#else
                ERROR("MCAST_DROP_SOURCE_MEMBERSHIP is not supported "
                      "for current Agent type");
                out->retval = -1;
                out->common._errno = TE_RC(TE_TA_UNIX, TE_EINVAL);
#endif
                break;
            }

            case TARPC_MCAST_SOURCE_JOIN_LEAVE:
            {
#ifdef MCAST_LEAVE_SOURCE_GROUP
                struct group_source_req     gsr_req;
                struct sockaddr_in         *sin_multicast;
                struct sockaddr_in         *sin_source;

                sin_multicast = SIN(&gsr_req.gsr_group);
                sin_multicast->sin_family = AF_INET;
                memcpy(&sin_multicast->sin_addr,
                       in->multiaddr.multiaddr_val,
                       sizeof(struct in_addr));
                sin_source = SIN(&gsr_req.gsr_source);
                sin_source->sin_family = AF_INET;
                memcpy(&sin_source->sin_addr, in->sourceaddr.sourceaddr_val,
                       sizeof(struct in_addr));
                gsr_req.gsr_interface = in->ifindex;
                out->retval = setsockopt_func(in->fd, IPPROTO_IP,
                                              in->leave_group ?
                                                  MCAST_LEAVE_SOURCE_GROUP :
                                                  MCAST_JOIN_SOURCE_GROUP,
                                              &gsr_req, sizeof(gsr_req));
                if (out->retval != 0)
                {
                    ERROR("Attempt to join IP multicast group failed");
                    out->common._errno = TE_RC(TE_TA_UNIX, errno);
                }
#else
                ERROR("MCAST_LEAVE_SOURCE_GROUP is not supported "
                      "for current Agent type");
                out->retval = -1;
                out->common._errno = TE_RC(TE_TA_UNIX, TE_EINVAL);
#endif
                break;
            }
            default:
                ERROR("Unknown multicast source join method");
                out->retval = -1;
                out->common._errno = TE_RC(TE_TA_UNIX, TE_EINVAL);
        }
    }
    else
    {
        ERROR("Unsupported multicast address family %d", in->family);
        out->retval = -1;
        out->common._errno = TE_RC(TE_TA_UNIX, TE_EINVAL);
    }
}

TARPC_FUNC(mcast_source_join_leave, {},
{
    MAKE_CALL(func_ptr(in, out));
})

/*-------------- dlopen() --------------------------*/
/**
 * Loads the dynamic labrary file.
 *
 * @param filename  the name of the file to load
 * @param flag      dlopen flags.
 *
 * @return dynamic library handle on success or NULL in the case of failure
 */
void *
ta_dlopen(tarpc_ta_dlopen_in *in)
{
    api_func_ptr_ret_ptr    dlopen_func;
    api_func_void_ret_ptr   dlerror_func;

    if ((tarpc_find_func(in->common.lib_flags, "dlopen",
                         (api_func *)&dlopen_func) != 0) ||
        (tarpc_find_func(in->common.lib_flags, "dlerror",
                         (api_func *)&dlerror_func) != 0))
    {
        ERROR("Failed to resolve functions, %s", __FUNCTION__);
        return NULL;
    }

    return dlopen_func(in->filename, dlopen_flags_rpc2h(in->flag));
}

TARPC_FUNC(ta_dlopen, {},
{
    MAKE_CALL(out->retval =
                        (tarpc_dlhandle)((uintptr_t)func_ptr_ret_ptr(in)));
}
)

/*-------------- dlsym() --------------------------*/
/**
 * Returns the address where a certain symbol from dynamic labrary
 * is loaded into memory.
 *
 * @param handle    handle of a dynamic library returned by dlopen()
 * @param symbol    null-terminated symbol name
 *
 * @return address of the symbol or NULL if symbol is not found.
 */
void *
ta_dlsym(tarpc_ta_dlsym_in *in)
{
    api_func_ptr_ret_ptr    dlsym_func;
    api_func_void_ret_ptr   dlerror_func;

    if ((tarpc_find_func(in->common.lib_flags, "dlsym",
                         (api_func *)&dlsym_func) != 0) ||
        (tarpc_find_func(in->common.lib_flags, "dlerror",
                         (api_func *)&dlerror_func) != 0))
    {
        ERROR("Failed to resolve functions, %s", __FUNCTION__);
        return NULL;
    }

    return dlsym_func((void *)((uintptr_t)in->handle), in->symbol);
}

TARPC_FUNC(ta_dlsym, {},
{
    MAKE_CALL(out->retval =
                    (tarpc_dlsymaddr)((uintptr_t)func_ptr_ret_ptr(in)));
}
)

/*-------------- dlsym_call() --------------------------*/
/**
 * Calls a certain symbol from dynamic library as a function with
 * no arguments and returns its return code.
 *
 * @param handle    handle of a dynamic library returned by dlopen()
 * @param symbol    null-terminated symbol name
 *
 * @return return code of called symbol or -1 on error.
 */
int
ta_dlsym_call(tarpc_ta_dlsym_call_in *in)
{
    api_func_ptr_ret_ptr    dlsym_func;
    api_func_void_ret_ptr   dlerror_func;
    char                   *error;

    int (*func)(void);

    if ((tarpc_find_func(in->common.lib_flags, "dlsym",
                         (api_func *)&dlsym_func) != 0) ||
        (tarpc_find_func(in->common.lib_flags, "dlerror",
                         (api_func *)&dlerror_func) != 0))
    {
        ERROR("Failed to resolve functions, %s", __FUNCTION__);
        return -1;
    }

    dlerror_func();

    *(void **) (&func) = dlsym_func((void *)((uintptr_t)in->handle),
                                    in->symbol);
    if ((error = (char *)dlerror_func()) != NULL)
    {
        ERROR("%s: dlsym call failed, %s", __FUNCTION__, error);
        return -1;
    }

    return (*func)();
}

TARPC_FUNC(ta_dlsym_call, {},
{
    MAKE_CALL(out->retval = func_ptr(in));
}
)

/*-------------- dlerror() --------------------------*/
/**
 * Returns a human readable string describing the most recent error
 * that occured from dlopen(), dlsym() or dlclose().
 *
 * @return a pointer to string or NULL if no errors occured.
 */
char *
ta_dlerror(tarpc_ta_dlerror_in *in)
{
    api_func_void_ret_ptr   dlerror_func;

    if (tarpc_find_func(in->common.lib_flags, "dlerror",
                        (api_func *)&dlerror_func) != 0)
    {
        ERROR("Failed to resolve functions, %s", __FUNCTION__);
        return NULL;
    }

    return (char *)dlerror_func();
}

TARPC_FUNC(ta_dlerror, {},
{
    MAKE_CALL(out->retval = func_ptr_ret_ptr(in));
}
)

/*-------------- dlclose() --------------------------*/
/**
 * Decrements the reference count on the dynamic library handle.
 * If the reference count drops to zero and no other loaded libraries
 * use symbols in it, then the dynamic library is unloaded.
 *
 * @param handle    handle of a dynamic library returned by dlopen()
 *
 * @return 0 on success, and non-zero on error.
 */
int
ta_dlclose(tarpc_ta_dlclose_in *in)
{
    api_func_ptr    dlclose_func;

    if (tarpc_find_func(in->common.lib_flags, "dlclose",
                        (api_func *)&dlclose_func) != 0)
    {
        ERROR("Failed to resolve functions, %s", __FUNCTION__);
        return -1;
    }

    return dlclose_func((void *)((uintptr_t)in->handle));
}

TARPC_FUNC(ta_dlclose, {},
{
    MAKE_CALL(out->retval = func_ptr(in));
}
)

#ifdef NO_DL
taprc_dlhandle
dlopen(const char *filename, int flag)
{
    UNUSED(filename);
    UNUSED(flag);
    return (tarpc_dlhandle)NULL;
}

const char *
dlerror(void)
{
    return "Unsupported";
}

tarpc_dlsymaddr
dlsym(void *handle, const char *symbol)
{
    UNUSED(handle);
    UNUSED(symbol);

    return (tarpc_dlsymaddr)NULL;
}

int
dlclose(void *handle)
{
    UNUSED(handle);
    return 0;
}
#endif

/*------------ recvmmsg_alt() ---------------------------*/
int
recvmmsg_alt(int fd, struct mmsghdr *mmsghdr, unsigned int vlen,
             unsigned int flags, struct timespec *timeout,
             tarpc_lib_flags lib_flags)
{
    api_func            recvmmsg_func;

    if (tarpc_find_func(lib_flags, "recvmmsg",
                        &recvmmsg_func) == 0)
        return recvmmsg_func(fd, mmsghdr, vlen, flags, timeout);
    else
#if defined (__QNX__)
        return -1;
#else
        return syscall(SYS_recvmmsg, fd, mmsghdr, vlen, flags, timeout);
#endif
}

TARPC_FUNC(recvmmsg_alt,
{
    COPY_ARG(mmsg);
},
{
    struct mmsghdr        *mmsg = NULL;
    rpcs_msghdr_helper    *msg_helpers = NULL;
    te_errno               rc;

    struct timespec  tv;
    struct timespec *ptv = NULL;

    if (in->timeout.timeout_len > 0)
    {
        tv.tv_sec = in->timeout.timeout_val[0].tv_sec;
        tv.tv_nsec = in->timeout.timeout_val[0].tv_nsec;
        ptv = &tv;
    }

    if (out->mmsg.mmsg_val == NULL)
    {
        MAKE_CALL(out->retval = func(in->fd, NULL, in->vlen,
                                     send_recv_flags_rpc2h(in->flags),
                                     ptv, in->common.lib_flags));
    }
    else
    {
        rc = rpcs_mmsghdrs_tarpc2h(RPCS_MSGHDR_CHECK_ARGS_RECV,
                                   out->mmsg.mmsg_val,
                                   out->mmsg.mmsg_len,
                                   &msg_helpers, &mmsg, arglist);
        if (rc != 0)
        {
            out->common._errno = TE_RC(TE_TA_UNIX, rc);
            goto finish;
        }

        VERB("recvmmsg_alt(): in mmsg=%s",
             mmsghdr2str(mmsg, out->mmsg.mmsg_len));
        MAKE_CALL(out->retval = func(in->fd, mmsg, in->vlen,
                                     send_recv_flags_rpc2h(in->flags),
                                     ptv, in->common.lib_flags));
        VERB("recvmmsg_alt(): out mmsg=%s",
             mmsghdr2str(mmsg, out->retval));

        rc = rpcs_mmsghdrs_h2tarpc(mmsg, msg_helpers,
                                   out->mmsg.mmsg_val, out->mmsg.mmsg_len);
        if (rc != 0)
        {
            out->common._errno = TE_RC(TE_TA_UNIX, rc);
            goto finish;
        }
    }

finish:
    rpcs_mmsghdrs_helpers_clean(msg_helpers, mmsg, out->mmsg.mmsg_len);
}
)

/*------------ sendmmsg_alt() ---------------------------*/
int
sendmmsg_alt(int fd, struct mmsghdr *mmsghdr, unsigned int vlen,
             unsigned int flags, tarpc_lib_flags lib_flags)
{
    api_func            sendmmsg_func;

    if (tarpc_find_func(lib_flags, "sendmmsg",
                        &sendmmsg_func) == 0)
        return sendmmsg_func(fd, mmsghdr, vlen, flags);
    else
#if defined (__QNX__)
        return -1;
#else
        return syscall(SYS_sendmmsg, fd, mmsghdr, vlen, flags);
#endif
}

TARPC_FUNC(sendmmsg_alt,
{
    COPY_ARG(mmsg);
},
{
    struct mmsghdr      *mmsg = NULL;
    rpcs_msghdr_helper  *msg_helpers = NULL;
    te_errno             rc;
    unsigned int         i;

    if (out->mmsg.mmsg_val == NULL)
    {
        MAKE_CALL(out->retval = func(in->fd, NULL, in->vlen,
                                     send_recv_flags_rpc2h(in->flags),
                                     in->common.lib_flags));
    }
    else
    {
        rc = rpcs_mmsghdrs_tarpc2h(RPCS_MSGHDR_CHECK_ARGS_SEND,
                                   out->mmsg.mmsg_val,
                                   out->mmsg.mmsg_len,
                                   &msg_helpers, &mmsg, arglist);
        if (rc != 0)
        {
            out->common._errno = TE_RC(TE_TA_UNIX, rc);
            goto finish;
       }

        VERB("sendmmsg_alt(): in mmsg=%s",
             mmsghdr2str(mmsg, out->mmsg.mmsg_len));
        MAKE_CALL(out->retval = func(in->fd, mmsg, in->vlen,
                                     send_recv_flags_rpc2h(in->flags),
                                     in->common.lib_flags));
        VERB("sendmmsg_alt(): out mmsg=%s",
             mmsghdr2str(mmsg, out->retval));

        /*
         * Reverse conversion is not done because this function should
         * not change anything except msg_len fields, and that nothing
         * else changed is checked with tarpc_check_args().
         */
        for (i = 0; i < in->vlen; i++)
            out->mmsg.mmsg_val[i].msg_len = mmsg[i].msg_len;
    }

finish:
    rpcs_mmsghdrs_helpers_clean(msg_helpers, mmsg, out->mmsg.mmsg_len);
}
)

/*------------ vfork_pipe_exec() -----------------------*/
int
vfork_pipe_exec(tarpc_vfork_pipe_exec_in *in)
{
    api_func_ptr        pipe_func;
    api_func_void       vfork_func;
    api_func            read_func;
    api_func_ptr        execve_func;
    api_func            write_func;

    int pipefd[2];
    int pid;
    te_errno      rc;
    struct pollfd fds;

    static volatile int global_var = 1;
    volatile int stack_var = 1;

    memset(&fds, 0, sizeof(fds));

    char       *argv[4];

    if (tarpc_find_func(in->common.lib_flags, "pipe",
                        (api_func *)&pipe_func) != 0)
    {
        ERROR("Failed to find function \"pipe\"");
        return -1;
    }
    if (tarpc_find_func(in->common.lib_flags, "vfork",
                        (api_func *)&vfork_func) != 0)
    {
        ERROR("Failed to find function \"vfork\"");
        return -1;
    }
    if (tarpc_find_func(in->common.lib_flags, "read",
                        (api_func *)&read_func) != 0)
    {
        ERROR("Failed to find function \"read\"");
        return -1;
    }
    if (tarpc_find_func(in->common.lib_flags, "execve",
                        (api_func *)&execve_func) != 0)
    {
        ERROR("Failed to find function \"execve\"");
        return -1;
    }
    if (tarpc_find_func(in->common.lib_flags, "write",
                        (api_func *)&write_func) != 0)
    {
        ERROR("Failed to find function \"write\"");
        return -1;
    }

    if ((rc = pipe_func(pipefd)) != 0)
    {
        ERROR("pipe() failed with error %r", TE_OS_RC(TE_TA_UNIX, errno));
        return -1;
    }

    pid = vfork_func();

    if (pid < 0)
    {
        ERROR("vfork() failed with error %r", TE_OS_RC(TE_TA_UNIX, errno));
        return pid;
    }

    if (pid > 0)
    {
        const char *test_msg = "Test message";
        ssize_t ret;

        ret = write(pipefd[1], test_msg, strlen(test_msg));
        if (ret != (ssize_t)strlen(test_msg))
        {
            ERROR("Write to pipefd[1] failed, ret=%d", (int)ret);
            return -1;
        }
        RING("Parent process is unblocked");
        if (global_var != 2)
        {
            ERROR("'global_var' was not changed from the child process");
            return -1;
        }
        if (stack_var != 2)
        {
            ERROR("'stack_var' was not changed from the child process");
            return -1;
        }
        return 0;
    }
    else
    {
        sleep(1);
        global_var = 2;
        stack_var = 2;
        fds.fd = pipefd[0];
        fds.events = POLLIN;
        if (poll(&fds, 1, 1000) != 0)
        {
            ERROR("vfork() doesn't hang!");
            return -1;
        }
        else
            RING("Parent process is still hanging");

        if (in->use_exec)
        {
            memset(argv, 0, sizeof(argv));
            argv[0] = (char *)ta_execname;
            argv[1] = "exec";
            argv[2] = "sleep_and_print";

            if ((rc = execve_func((void *)ta_execname, argv, environ)) < 0)
            {
                ERROR("execve() failed with error %r",
                      TE_OS_RC(TE_TA_UNIX, errno));
                return rc;
            }
            return 0;
        }
        else
        {
            RING("Child process is finished.");
            _exit(0);
        }
    }

    return 0;
}

int
sleep_and_print(void)
{
    sleep(1);
    return 0;
}

TARPC_FUNC(vfork_pipe_exec, {},
{
    MAKE_CALL(out->retval = func_ptr(in));
}
)

/*------------ namespace_id2str() -----------------------*/

te_errno
namespace_id2str(tarpc_namespace_id2str_in *in,
                 tarpc_namespace_id2str_out *out)
{
    te_errno rc;
    const char *buf = NULL;
    rc = rcf_pch_mem_ns_get_string(in->id, &buf);
    if (rc != 0)
        return rc;
    if (buf == NULL)
        return TE_RC(TE_RPC, TE_ENOENT);

    const int str_len = strlen(buf);
    out->str.str_val = malloc(str_len);
    if (out->str.str_val == NULL)
        return TE_RC(TE_RPC, TE_ENOMEM);

    out->str.str_len = str_len;
    memcpy(out->str.str_val, buf, str_len);
    return 0;
}

TARPC_FUNC(namespace_id2str,
{
    /* Only blocking operation is supported for @b namespace_id2str */
    in->common.op = RCF_RPC_CALL_WAIT;
},
{
    MAKE_CALL(out->retval = func_ptr(in, out));
})

/*------------ release_rpc_ptr() -----------------------*/

TARPC_FUNC_STANDALONE(release_rpc_ptr, {},
{
    MAKE_CALL(
        RPC_PCH_MEM_WITH_NAMESPACE(ns, in->ns_string, {
            RCF_PCH_MEM_INDEX_FREE(in->ptr, ns);
    }));
})

/*------------ get_rw_ability() -----------------------*/
int
get_rw_ability(tarpc_get_rw_ability_in *in)
{
    iomux_funcs iomux_f;
    iomux_func  iomux = get_default_iomux();
    iomux_state iomux_st;

    int rc;

    if (iomux_find_func(in->common.lib_flags, &iomux, &iomux_f) != 0)
    {
        ERROR("failed to resolve iomux function");
        return -1;
    }

    /* Create iomux status and fill it with our fds. */
    if ((rc = iomux_create_state(iomux, &iomux_f, &iomux_st)) != 0)
        return rc;
    if ((rc = iomux_add_fd(iomux, &iomux_f, &iomux_st,
                           in->sock,
                           in->check_rd ? POLLIN : POLLOUT)) != 0)
    {
        iomux_close(iomux, &iomux_f, &iomux_st);
        return rc;
    }

    rc = iomux_wait(iomux, &iomux_f, &iomux_st, NULL, in->timeout);

    iomux_close(iomux, &iomux_f, &iomux_st);
    return rc;
}

TARPC_FUNC(get_rw_ability, {},
{
    MAKE_CALL(out->retval = func_ptr(in));
})

/*------------ rpcserver_plugin_enable() -----------------------*/

TARPC_FUNC(rpcserver_plugin_enable, {},
{
    MAKE_CALL(out->retval = func_ptr(
               in->install,
               in->action,
               in->uninstall));
})

/*------------ rpcserver_plugin_disable() -----------------------*/

TARPC_FUNC(rpcserver_plugin_disable, {},
{
    MAKE_CALL(out->retval = func_ptr());
})

/*-------------------- send_flooder_iomux() --------------------------*/

/* Maximum iov vectors number to be sent. */
#define TARPC_SEND_IOMUX_FLOODER_MAX_IOVCNT 10

/* Multiplexer timeout to get a socket writable, milliseconds. */
#define TARPC_SEND_IOMUX_FLOODER_TIMEOUT 500

/**
 * Send packets during a period of time, call an iomux to check OUT
 * event if send operation is failed.
 *
 * @param lib_flags     How to resolve function name.
 * @param sock          Socket.
 * @param iomux         Multiplexer function.
 * @param send_func     Transmitting function.
 * @param msg_dontwait  Use flag @b MSG_DONTWAIT.
 * @param packet_size   Payload size to be sent by a single call, bytes.
 * @param duration      How long transmit datagrams, milliseconds.
 * @param packets       Sent packets (datagrams) number.
 * @param errors        @c EAGAIN errors counter.
 *
 * @return @c 0 on success or @c -1 in the case of failure.
 */
static int
send_flooder_iomux(tarpc_lib_flags lib_flags, int sock, iomux_func iomux,
                   tarpc_send_function send_func, te_bool msg_dontwait,
                   int packet_size, int duration, uint64_t *packets,
                   uint32_t *errors)
{
    api_func        func_send;
    struct timeval  tv_start;
    struct timeval  tv_now;
    struct iovec   *iov = NULL;
    void           *buf = NULL;
    int             flags = msg_dontwait ? MSG_DONTWAIT : 0;
    struct msghdr   msg;
    iomux_funcs     iomux_f;
    iomux_state     iomux_st;
    iomux_return    iomux_ret;
    uint64_t        i;
    int             rc;
    te_bool         writable = FALSE;
    int             back_errno = errno;
    size_t iovcnt = rand_range(1, TARPC_SEND_IOMUX_FLOODER_MAX_IOVCNT);


    if (tarpc_get_send_function(lib_flags, send_func, &func_send) != 0 ||
        iomux_find_func(lib_flags, &iomux, &iomux_f) != 0)
    {
        return -1;
    }

    if ((rc = iomux_create_state(iomux, &iomux_f, &iomux_st)) != 0)
    {
        iomux_close(iomux, &iomux_f, &iomux_st);
        return -1;
    }

    *packets = 0;
    *errors = 0;

    if ((rc = gettimeofday(&tv_start, NULL)) != 0)
    {
        ERROR("gettimeofday() failed, rc = %d, errno %r", rc, RPC_ERRNO);
        iomux_close(iomux, &iomux_f, &iomux_st);
        return -1;
    }

    if ((rc = iomux_add_fd(iomux, &iomux_f, &iomux_st, sock, POLLOUT)))
    {
        iomux_close(iomux, &iomux_f, &iomux_st);
        return -1;
    }

    buf = TE_ALLOC(packet_size);

    if (send_func == TARPC_SEND_FUNC_WRITEV ||
        send_func == TARPC_SEND_FUNC_SENDMSG)
    {
        int offt = 0;

        iov = TE_ALLOC(iovcnt * sizeof(*iov));

        for (i = 0; i < iovcnt; i++)
        {
            if (i == iovcnt - 1)
                iov[i].iov_len = packet_size - offt;
            else
                iov[i].iov_len = packet_size / iovcnt;
            iov[i].iov_base = buf + offt;
            offt += iov[i].iov_len;
        }

        if (send_func == TARPC_SEND_FUNC_SENDMSG)
        {
            memset(&msg, 0, sizeof(msg));
            msg.msg_iov = iov;
            msg.msg_iovlen = iovcnt;
        }
    }

    for (i = 0;; i++)
    {
        switch (send_func)
        {
            case TARPC_SEND_FUNC_WRITE:
                rc = func_send(sock, buf, packet_size);
                break;

            case TARPC_SEND_FUNC_WRITEV:
                rc = func_send(sock, iov, iovcnt);
                break;

            case TARPC_SEND_FUNC_SEND:
                rc = func_send(sock, buf, packet_size, flags);
                break;

            case TARPC_SEND_FUNC_SENDTO:
                rc = func_send(sock, buf, packet_size, flags, NULL);
                break;

            case TARPC_SEND_FUNC_SENDMSG:
                rc = func_send(sock, &msg, flags);
                break;

            default:
                ERROR("Invalid send function index: %d", send_func);
                return  TE_RC(TE_TA_UNIX, TE_EINVAL);
        }

        if (rc == -1 && RPC_ERRNO == RPC_EAGAIN)
        {
            (*errors)++;

            if( writable )
            {
                ERROR("Iomux call declares socket writable when a send "
                      "call failed with EAGAIN");
                rc = -1;
                errno = EBUSY;
                break;
            }
            rc = iomux_wait(iomux, &iomux_f, &iomux_st, &iomux_ret, 0);
            /* Check no OUT event. */
            rc = iomux_fd_is_writable(sock, iomux, &iomux_st, &iomux_ret,
                                      rc, &writable);
            if (rc != 0)
                break;
            if (writable)
                continue;

            rc = iomux_wait(iomux, &iomux_f, &iomux_st, &iomux_ret,
                            TARPC_SEND_IOMUX_FLOODER_TIMEOUT);
            /* Unblocked with OUT event. */
            rc = iomux_fd_is_writable(sock, iomux, &iomux_st, &iomux_ret,
                                      rc, &writable);
            if (rc != 0)
                break;
            if (!writable)
            {
                ERROR("Iomux call declares socket unwritable after the "
                      "timeout %d expiration ",
                      TARPC_SEND_IOMUX_FLOODER_TIMEOUT);
                rc = -1;
                errno = ETIMEDOUT;
                break;
            }
        }
        else
        {
            if (rc != packet_size)
            {
                ERROR("Send call #%llu returned unexpected value, rc = %d "
                      "(%r)", i, rc, RPC_ERRNO);
                break;
            }
            writable = FALSE;
        }

        (*packets)++;

        if ((rc = gettimeofday(&tv_now, NULL)) != 0)
        {
            ERROR("gettimeofday() failed, rc = %d, errno %r", rc,
                  RPC_ERRNO);
            rc = -1;
            break;
        }

        if (duration < TIMEVAL_SUB(tv_now, tv_start) / 1000L)
            break;
    }

    free(buf);
    free(iov);
    iomux_close(iomux, &iomux_f, &iomux_st);

    if (rc >= 0 && back_errno != errno && errno == EAGAIN)
        errno = back_errno;

    return rc;
}

TARPC_FUNC_STATIC(send_flooder_iomux, {},
{
    MAKE_CALL(out->retval = func_ptr(in->common.lib_flags, in->sock,
                                     in->iomux, in->send_func,
                                     in->msg_dontwait, in->packet_size,
                                     in->duration, &out->packets,
                                     &out->errors));
})

/*-------------- copy_fd2fd() ------------------------------*/
/**
 * Copy data between one file descriptor and another.
 *
 * @param in        TA RPC input parameter.
 *
 * @return If the transfer was successful, the number of copied bytes is
 *         returned. On error, @c -1 is returned, and errno is set
 *         appropriately.
 */
int64_t
copy_fd2fd(tarpc_copy_fd2fd_in *in)
{
    /* Whether limited number of bytes to copy or not. */
    const te_bool partial_copy = (in->count != 0);
    api_func      write_func;
    api_func      read_func;
    iomux_funcs   iomux_f;
    iomux_state   iomux_st;
    iomux_return  iomux_ret;
    iomux_func    iomux = get_default_iomux();
    int           num_events;
    int           events;
    char          buf[4 * 1024];
    uint64_t      remains = (partial_copy ? in->count : sizeof(buf));
    size_t        count;
    int           received;
    int64_t       total = 0;
    int           written;
    int           rc;

#define COPY_FD2FD_EXIT_WITH_ERROR() \
    do {                                            \
        iomux_close(iomux, &iomux_f, &iomux_st);    \
        return -1;                                  \
    } while (0)

    if ((iomux_find_func(in->common.lib_flags, &iomux, &iomux_f) != 0) ||
        (tarpc_find_func(in->common.lib_flags, "read", &read_func) != 0) ||
        (tarpc_find_func(in->common.lib_flags, "write", &write_func) != 0))
    {
        ERROR("Failed to resolve functions addresses");
        return -1;
    }

    /* Create iomux status and fill it with our fd. */
    rc = iomux_create_state(iomux, &iomux_f, &iomux_st);
    if (rc != 0)
        return -1;
    rc = iomux_add_fd(iomux, &iomux_f, &iomux_st, in->in_fd, POLLIN);
    if (rc != 0)
        COPY_FD2FD_EXIT_WITH_ERROR();

    do {
        int fd = -1;

        num_events = iomux_wait(iomux, &iomux_f, &iomux_st, &iomux_ret,
                                in->timeout);
        if (num_events == -1)
        {
            ERROR("%s:%d: iomux_wait is failed: %r",
                  __FUNCTION__, __LINE__, RPC_ERRNO);
            COPY_FD2FD_EXIT_WITH_ERROR();
        }
        if (num_events == 0)
        {
            WARN("%s:%d: iomux_wait: timeout is expired",
                 __FUNCTION__, __LINE__);
            break;
        }
        events = 0;
        iomux_return_iterate(iomux, &iomux_st, &iomux_ret,
                             IOMUX_RETURN_ITERATOR_START, &fd, &events);

        /* Receive data from socket that are ready */
        if ((events & POLLIN) != 0)
        {
            /* Read routine. */
            count = (remains < sizeof(buf) ? remains : sizeof(buf));
            received = read_func(fd, buf, count);
            if (received < 0)
            {
                ERROR("%s:%d: Failed to read: %r",
                      __FUNCTION__, __LINE__, RPC_ERRNO);
                COPY_FD2FD_EXIT_WITH_ERROR();
            }
            if (received == 0)  /* EOF */
                break;
            /* Write routine. */
            do {
                written = write_func(in->out_fd, buf, received);
                if (written < 0)
                {
                    ERROR("%s:%d: Failed to write: %r",
                          __FUNCTION__, __LINE__, RPC_ERRNO);
                    COPY_FD2FD_EXIT_WITH_ERROR();
                }
                received -= written;
            } while (received > 0);
            total += written;
            if (partial_copy)
            {
                remains -= written;
                if (remains == 0)
                    break;
            }
        }
    } while ((events & POLLIN) != 0);

    iomux_close(iomux, &iomux_f, &iomux_st);
    return total;
#undef COPY_FD2FD_EXIT_WITH_ERROR
}

TARPC_FUNC_STATIC(copy_fd2fd, {},
{
   MAKE_CALL(out->retval = func_ptr(in));
})

/**
 * Read all data on an fd.
 *
 * @param lib_flags How to resolve function name.
 * @param fd        File descriptor or socket.
 * @param size      Intermediate read buffer size, bytes.
 * @param time2wait Time to wait for data, milliseconds. Negative value means
                    an infinite timeout.
 * @param amount    Number of bytes to read, if @c 0 then only @p time2wait
 *                  limits it.
 * @param buf       Location of the buffer to read the data in; may be @c NULL
 *                  to drop the read data.
 * @param read      Location for the amount of data read.
 *
 * @return @c -1 in the case of failure or @c 0 on success (timeout is expired,
 * @p amount bytes is read, EOF is got).
 */
static int
read_fd(tarpc_lib_flags lib_flags, int fd, size_t size, int time2wait,
        size_t amount, uint8_t **buf, size_t *read)
{
    api_func      read_func;
    iomux_funcs   iomux_f;
    iomux_state   iomux_st;
    iomux_return  iomux_ret;
    iomux_func    iomux = get_default_iomux();
    te_dbuf       dbuf = TE_DBUF_INIT(0);
    size_t        size_to_read;
    int num = 0;
    int rc;

    if (tarpc_find_func(lib_flags, "read", &read_func) != 0)
    {
        ERROR("Failed to resolve read function address");
        return -1;
    }

    if (iomux_find_func(lib_flags, &iomux, &iomux_f) != 0)
    {
        ERROR("Failed to resolve iomux function address");
        return -1;
    }

    rc = iomux_create_state(iomux, &iomux_f, &iomux_st);
    if (rc != 0)
        return -1;
    rc = iomux_add_fd(iomux, &iomux_f, &iomux_st, fd, POLLIN);
    if (rc != 0)
    {
        iomux_close(iomux, &iomux_f, &iomux_st);
        return -1;
    }

    *read = 0;

    do {
        num = iomux_wait(iomux, &iomux_f, &iomux_st, &iomux_ret, time2wait);
        if (num <= 0)
        {
            rc = num < 0 ? -1 : 0;
            break;
        }

        /*
         * Prepare the buffer to save the message. If buf == NULL, dbuf.len will
         * not be changed, so here intermediate buffer memory will be allocated
         * only once, i.e. size_to_read == 0 only at the first loop iteration
         */
        if (dbuf.size == dbuf.len)
        {
            size_to_read = amount > 0 && amount < size ? amount : size;
            rc = te_dbuf_expand(&dbuf, size_to_read);
            if (rc != 0)
            {
                rc = -1;
                break;
            }
        }
        else
        {
            size_to_read = dbuf.size - dbuf.len;
            if (amount > 0 && amount < size_to_read)
                size_to_read = amount;
        }
        /* Read data */
        rc = read_func(fd, &dbuf.ptr[dbuf.len], size_to_read);
        if (rc > 0)
        {
            (*read) += rc;
            if (buf != NULL)
                dbuf.len += rc;
            if (amount > 0)
            {
                amount -= rc;
                if (amount == 0)
                    rc = 0;     /* Finish reading */
            }
        }
    } while (rc > 0);

    iomux_close(iomux, &iomux_f, &iomux_st);

    if (buf != NULL)
        *buf = dbuf.ptr;
    else
        te_dbuf_free(&dbuf);

    return rc;
}

TARPC_FUNC_STATIC(read_fd, {},
{
    size_t read;

    if (in->amount > UINT_MAX)
    {
        ERROR("'amount' value passed to read_fd exceeds "
              "the size of receive buffer");
        out->common._errno = TE_RC(TE_TA_UNIX, TE_EOVERFLOW);
        out->retval = -1;
    }
    else
    {
        MAKE_CALL(out->retval = func(in->common.lib_flags, in->fd, in->size,
                                     in->time2wait, in->amount,
                                     &out->buf.buf_val, &read));
        if (read <= UINT_MAX)
            out->buf.buf_len = read;
        else
        {
            ERROR("receive buffer is too small to get the whole data");
            free(out->buf.buf_val);
            out->buf.buf_val = NULL;
            out->buf.buf_len = 0;
            out->common._errno = TE_RC(TE_TA_UNIX, TE_ESMALLBUF);
            out->retval = -1;
        }
    }
})

/**
 * Drain all data on a fd.
 *
 * @param lib_flags How to resolve function name.
 * @param fd        File descriptor or socket.
 * @param size      Read buffer size, bytes.
 * @param time2wait Time to wait for extra data, milliseconds. If a negative
 *                  value is set, then blocking @c recv() will be used to
 *                  read data.
 * @param read      Read data amount.
 *
 * @return The last return code of @c recv() function: @c -1 in the case of
 * failure or @c 0 on success.
 */
static int
drain_fd(tarpc_lib_flags lib_flags, int fd, size_t size, int time2wait,
         uint64_t *read)
{
    api_func      recv_func;
    iomux_funcs   iomux_f;
    iomux_state   iomux_st;
    iomux_return  iomux_ret;
    iomux_func    iomux = get_default_iomux();
    void         *buf = NULL;
    int flags = MSG_DONTWAIT;
    int num = 0;
    int rc;

    if (tarpc_find_func(lib_flags, "recv", &recv_func) != 0)
    {
        ERROR("Failed to resolve recv function address");
        return -1;
    }

    if (time2wait < 0)
    {
        flags = 0;
    }
    else if (time2wait > 0)
    {
        if (iomux_find_func(lib_flags, &iomux, &iomux_f) != 0)
        {
            ERROR("Failed to resolve iomux function address");
            return -1;
        }

        rc = iomux_create_state(iomux, &iomux_f, &iomux_st);
        if (rc != 0)
            return -1;
        rc = iomux_add_fd(iomux, &iomux_f, &iomux_st, fd, POLLIN);
        if (rc != 0)
        {
            iomux_close(iomux, &iomux_f, &iomux_st);
            return -1;
        }
    }

    if ((buf = TE_ALLOC(size)) == NULL)
    {
        if (time2wait > 0)
            iomux_close(iomux, &iomux_f, &iomux_st);
        return -1;
    }

    *read = 0;

    do {
        rc = recv_func(fd, buf, size, flags);
        if (rc < 0)
        {
            if (errno != EAGAIN)
                break;
            if (time2wait <= 0)
                break;

            num = iomux_wait(iomux, &iomux_f, &iomux_st, &iomux_ret,
                             time2wait);
            if (num <= 0)
                break;
        }

        if (rc > 0)
            (*read) += rc;
    } while (rc != 0);

    if (time2wait > 0)
        iomux_close(iomux, &iomux_f, &iomux_st);
    free(buf);
    return rc;
}

TARPC_FUNC_STATIC(drain_fd, {},
{
   MAKE_CALL(out->retval = func(in->common.lib_flags, in->fd, in->size,
                                in->time2wait, &out->read));
})


/*---------------------- wrappers for syscall -------------------------------*/
/*
 * This is not an exhaustive list of syscalls, it is designed to test the
 * capabilities of some libraries by some test suites,
 * see: https://bugzilla.oktetlabs.ru/show_bug.cgi?id=9863
 *
 * WARNING: Some architectures have very quirky syscalls,
 * for example x86-64 and ARM there is no socketcall() system call,
 * see man socketcall(2) for details.
 * There are other quirky architectures and quirky syscalls.
 */

#ifdef SYS_setrlimit
TARPC_SYSCALL_WRAPPER(setrlimit, int,
                      (int a, const struct rlimit *b), a, b)
#endif

#ifdef SYS_socket
TARPC_SYSCALL_WRAPPER(socket, int,
                      (int a, int b, int c), a, b, c)
#endif

#ifdef SYS_bind
TARPC_SYSCALL_WRAPPER(bind, int,
                      (int a, const struct sockaddr *b, socklen_t c), a, b, c)
#endif

#ifdef SYS_listen
TARPC_SYSCALL_WRAPPER(listen, int,
                      (int a, int b), a, b)
#endif

#ifdef SYS_accept
TARPC_SYSCALL_WRAPPER(accept, int, (int a, struct sockaddr *b, socklen_t *c),
                      a, b, c)
#endif

#ifdef SYS_accept4
TARPC_SYSCALL_WRAPPER(accept4, int, (int a, struct sockaddr *b, socklen_t *c,
                      int d), a, b, c, d)
#endif

#ifdef SYS_connect
TARPC_SYSCALL_WRAPPER(connect, int, (int a, const struct sockaddr *b,
                      socklen_t c), a, b, c)
#endif

#ifdef SYS_shutdown
TARPC_SYSCALL_WRAPPER(shutdown, int, (int a, int b), a, b)
#endif

#ifdef SYS_getsockname
TARPC_SYSCALL_WRAPPER(getsockname, int,
                      (int a, struct sockaddr *b, socklen_t *c), a, b, c)
#endif

#ifdef SYS_getpeername
TARPC_SYSCALL_WRAPPER(getpeername, int,
                      (int a, struct sockaddr *b, socklen_t *c), a, b, c)
#endif

#ifdef SYS_getsockopt
TARPC_SYSCALL_WRAPPER(getsockopt, int, (int a, int b, int c, void *d,
                      socklen_t *e), a, b, c, d, e)
#endif

#ifdef SYS_setsockopt
TARPC_SYSCALL_WRAPPER(setsockopt, int, (int a, int b, int c, const void *d,
                      socklen_t e), a, b, c, d, e)
#endif

#ifdef SYS_recvfrom
TARPC_SYSCALL_WRAPPER(recvfrom, ssize_t, (int a, void *b, size_t c, int d,
                       struct sockaddr *e, socklen_t *f), a, b, c, d, e, f)
#endif

#ifdef SYS_recvmsg
TARPC_SYSCALL_WRAPPER(recvmsg, ssize_t, (int a, struct msghdr *b, int c),
                      a, b, c)
#endif

#ifdef SYS_recvmmsg
TARPC_SYSCALL_WRAPPER(recvmmsg, int, (int a, struct mmsghdr *b, unsigned int c,
                      unsigned int d, struct timespec *e), a, b, c, d, e)
#endif

#ifdef SYS_sendto
TARPC_SYSCALL_WRAPPER(sendto, ssize_t, (int a, const void *b, size_t c, int d,
                      const struct sockaddr *e, socklen_t f), a, b, c, d, e, f)
#endif

#ifdef SYS_sendmsg
TARPC_SYSCALL_WRAPPER(sendmsg, ssize_t, (int a, const struct msghdr *b, int c),
                      a, b, c)
#endif

#ifdef SYS_select
TARPC_SYSCALL_WRAPPER(select, int, (int a, fd_set *b, fd_set *c, fd_set *d,
                      struct timeval *e), a, b, c, d, e)
#endif

#ifdef SYS_poll
TARPC_SYSCALL_WRAPPER(poll, int, (struct pollfd *a, nfds_t b, int c), a, b, c)
#endif

#ifdef SYS_ppoll
TARPC_SYSCALL_WRAPPER(ppoll, int, (struct pollfd *a, nfds_t b,
                      const struct timespec *c, const sigset_t *d), a, b, c, d)
#endif

#ifdef SYS_splice
TARPC_SYSCALL_WRAPPER(splice, ssize_t, (int a, loff_t *b, int c, loff_t *d,
                      size_t e, unsigned int f), a, b, c, d, e, f)
#endif

#ifdef SYS_read
TARPC_SYSCALL_WRAPPER(read, ssize_t, (int a, void *b, size_t c), a, b, c)
#endif


#ifdef SYS_write
TARPC_SYSCALL_WRAPPER(write, ssize_t, (int a, const void *b, size_t c), a, b, c)
#endif

#ifdef SYS_readv
TARPC_SYSCALL_WRAPPER(readv, ssize_t, (int a, const struct iovec *b, int c),
                      a, b, c)
#endif

#ifdef SYS_writev
TARPC_SYSCALL_WRAPPER(writev, ssize_t, (int a, const struct iovec *b, int c),
                      a, b, c)
#endif

#ifdef SYS_close
TARPC_SYSCALL_WRAPPER(close, int, (int a), a)
#endif

#ifdef SYS_ioctl
TARPC_SYSCALL_WRAPPER(ioctl, int, (int a, unsigned long b, void *c), a, b, c)
#endif

#ifdef SYS_dup
TARPC_SYSCALL_WRAPPER(dup, int, (int a), a)
#endif

#ifdef SYS_dup2
TARPC_SYSCALL_WRAPPER(dup2, int, (int a, int b), a, b)
#endif

#ifdef SYS_dup3
TARPC_SYSCALL_WRAPPER(dup3, int, (int a, int b, int c), a, b, c)
#endif

/**
 * NOTE: vfork() does not work properly when called via libc syscall().
 * See https://bugzilla.oktetlabs.ru/show_bug.cgi?id=9976
 */
#if 0
TARPC_SYSCALL_WRAPPER(vfork, pid_t, (void))
#endif

/**
 * NOTE: <man 2 open>
 * open() can called with two or three arguments,
 * TE uses variant with 3 arguments only
 */
#ifdef SYS_open
TARPC_SYSCALL_WRAPPER(open, int, (const char *a, int b, mode_t c), a, b, c)
#endif

#ifdef SYS_creat
TARPC_SYSCALL_WRAPPER(creat, int, (const char *a, mode_t b), a, b)
#endif

#ifdef SYS_socketpair
TARPC_SYSCALL_WRAPPER(socketpair, int, (int a, int b, int c, int *d),
                      a, b, c, d)
#endif

#ifdef SYS_pipe
TARPC_SYSCALL_WRAPPER(pipe, int, (int a[2]), a)
#endif

#ifdef SYS_pipe2
TARPC_SYSCALL_WRAPPER(pipe2, int, (int a[2], int b), a, b)
#endif

#ifdef SYS_setuid
TARPC_SYSCALL_WRAPPER(setuid, int, (uid_t a), a)
#endif

#ifdef SYS_chroot
TARPC_SYSCALL_WRAPPER(chroot, int, (const char *a), a)
#endif

#ifdef SYS_execve
TARPC_SYSCALL_WRAPPER(execve, int, (const char *a, char *const b[],
                      char *const c[]), a, b, c)
#endif

#ifdef SYS_epoll_create
TARPC_SYSCALL_WRAPPER(epoll_create, int, (int a), a)
#endif

#ifdef SYS_epoll_create1
TARPC_SYSCALL_WRAPPER(epoll_create1, int, (int a), a)
#endif

#ifdef SYS_epoll_ctl
TARPC_SYSCALL_WRAPPER(epoll_ctl, int, (int a, int b, int c,
                      struct epoll_event *d), a, b, c, d)
#endif

#ifdef SYS_epoll_wait
TARPC_SYSCALL_WRAPPER(epoll_wait, int, (int a, struct epoll_event *b, int c,
                      int d), a, b, c, d)
#endif

#ifdef SYS_epoll_pwait
TARPC_SYSCALL_WRAPPER(epoll_pwait, int, (int a, struct epoll_event *b, int c,
                      int d, const sigset_t *e), a, b, c, d, e)
#endif

/**
 * Implement common syscall operations for fcntl().
 *
 * @param use_libc      How to resolve syscall function.
 * @param fd            Opened file descriptor.
 * @param cmd           Operation to execute.
 * @param argp          Optional argument. Whether or not this argument is
 *                      required is determined by @c cmd
 *
 * @return              Return the syscall() result.
 */
static int
fcntl_te_wrap_syscall_common(te_bool use_libc,
                             int fd,
                             int cmd,
                             va_list argp)
{
    int             int_arg;
    api_func        syscall_func = NULL;
    static api_func syscall_func_use_libc = NULL;
    static api_func syscall_func_default = NULL;

    syscall_func = use_libc ? syscall_func_use_libc : syscall_func_default;

    if (syscall_func == NULL &&
        tarpc_find_func(use_libc ? TARPC_LIB_USE_LIBC : TARPC_LIB_DEFAULT,
                        "syscall", &syscall_func) != 0)
    {
        if (use_libc)
            syscall_func_use_libc = NULL;
        else
            syscall_func_default = NULL;

        ERROR("Failed to find function \"syscall\" in %s",
              use_libc ? "libc" : "dynamic lib");
        return -1;
    }

    if (cmd == F_GETFD || cmd == F_GETFL ||cmd == F_GETSIG
#if defined (F_GETPIPE_SZ)
        || cmd == F_GETPIPE_SZ
#endif
        )
        return syscall_func(SYS_fcntl, fd, cmd);
#if defined (F_GETOWN_EX) && defined (F_SETOWN_EX)
    else if (cmd == F_GETOWN_EX || cmd == F_SETOWN_EX)
    {
        struct f_owner_ex *foex_arg;
        foex_arg = va_arg(argp, struct f_owner_ex*);
        return syscall_func(SYS_fcntl, fd, cmd, foex_arg);
    }
#endif
    int_arg = va_arg(argp, int);
    return syscall_func(SYS_fcntl, fd, cmd, int_arg);
}




/**
 * Wrapper for syscall(fcntl) from libc.
 *
 * @param fd        Opened file descriptor.
 * @param cmd       Operation to execute.
 * @param ...       Optional argument. Whether or not this argument is
 *                  required is determined by @c cmd
 *
 * @return @c -1 in the case of syscall was not found
 *         or return the result of syscall.
 */
int fcntl_te_wrap_syscall(int fd, int cmd, ...)
{
    va_list argp;
    int rc;

    va_start(argp, cmd);
    rc = fcntl_te_wrap_syscall_common(TRUE, fd, cmd, argp);
    va_end(argp);
    return rc;
}

/**
 * Wrapper for syscall(fcntl) from dynamic library.
 *
 * @param fd        Opened file descriptor.
 * @param cmd       Operation to execute.
 * @param ...       Optional argument. Whether or not this argument is
 *                  required is determined by @c cmd
 *
 * @return @c -1 in the case of syscall was not found
 *         or return the result of syscall.
 */
int fcntl_te_wrap_syscall_dl(int fd, int cmd, ...)
{
    va_list argp;
    int rc;

    va_start(argp, cmd);
    rc = fcntl_te_wrap_syscall_common(FALSE, fd, cmd, argp);
    va_end(argp);
    return rc;
}

