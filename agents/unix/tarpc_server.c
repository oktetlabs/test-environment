/** @file
 * @brief Unix Test Agent
 *
 * RPC routines implementation 
 *
 * Copyright (C) 2004 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 * Copyright (c) 2005 Level5 Networks Corp.
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
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

#define TE_LGR_USER     "RPC"

#include "tarpc_server.h"

#include "config.h"

#ifdef HAVE_DLFCN_H
#include <dlfcn.h>
#endif

#if HAVE_NETINET_IN_SYSTM_H /* Required for FreeBSD build */
#include <netinet/in_systm.h>
#endif

#include "te_tools.h"

#include "unix_internal.h"


#if HAVE_AIO_H
void *dummy = aio_read;
#endif

extern sigset_t rpcs_received_signals;


static te_bool dynamic_library_set = FALSE;
static void *dynamic_library_handle = NULL;


/**
 * Set name of the dynamic library to be used to resolve function
 * called via RPC.
 *
 * @param libname       Full name of the dynamic library or NULL
 *
 * @return Status code.
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
    dynamic_library_handle = dlopen(libname == '\0' ? NULL : libname,
                                    RTLD_LAZY);
    if (dynamic_library_handle == NULL)
    {
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

/**
 * Find the function by its name.
 *
 * @param lib   library name or empty string
 * @param name  function name
 * @param func  location for function address
 *
 * @return status code
 */
int
tarpc_find_func(const char *lib, const char *name, api_func *func)
{
    te_errno    rc;
    void       *handle;
    te_bool     use_libc = FALSE;

    /* FIXME */
    if (strcmp(name, "getpid") == 0)
    {
        *func = (void *)getpid;
        return 0;
    }

    if (!dynamic_library_set &&
        (rc = tarpc_setlibname(getenv("TARPC_DL_NAME"))) != 0)
    {
        /* Error is always logged from tarpc_setlibname() */
        return rc;
    }

    /*
     * Use libc when user intensionally wants it, or if we were not
     * informed about something special to use before
     * (with tarpc_setlibname() function).
     */
    if (strcmp(lib, "libc") == 0 ||
        (*lib == '\0' && dynamic_library_handle == NULL))
    {
        use_libc = TRUE;
    }

    if (use_libc)
    {
        static void *libc_handle = NULL;

        if (libc_handle == NULL)
        {
            if ((libc_handle = dlopen(NULL, RTLD_LAZY)) == NULL)
            {
                ERROR("dlopen() failed for myself: %s", dlerror());
                return TE_RC(TE_TA_UNIX, TE_ENOENT);
            }
        }
        handle = libc_handle;
        VERB("Call from libc");
    }
    else if (*lib != '\0')
    {
        /* User wants something special for this call */
        if ((handle = dlopen(lib, RTLD_LAZY)) == NULL)
        {
            ERROR("Cannot load shared library %s: %s", lib, dlerror());
            return TE_RC(TE_TA_UNIX, TE_ENOENT);
        }
        VERB("Call from smth special");
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

    /* 
     * Close library only in case it is something special
     * particulary for this call.
     */
    if (*lib != '\0' && !use_libc)
        dlclose(handle);
    
    if (*func == NULL)
    {
        VERB("Cannot resolve symbol %s in libraries: %s", name, dlerror());
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
 * in the case if function is implemented as a static one.
 *
 * @param name  function name (or pointer value converted to string)
 * @param handler returned pointer to function or NULL if error
 *
 * @return errno if error or 0
 */
static int
name2handler(const char *name, void **handler)
{
    if (name == NULL || *name == 0)
    {
        *handler = NULL;
        return 0;
    } 

    *handler = rcf_ch_symbol_addr(name, 1);
    if (*handler == NULL)
    {
        char *tmp;
        int   id;
        
        id = strtol(name, &tmp, 10);
        if (tmp == name || *tmp != 0)
            return TE_RC(TE_TA_UNIX, TE_ENOENT);
            
        *handler = rcf_pch_mem_get(id);
    }
    return 0;
}

/**
 * Find the function name in table according to pointer to one.
 * Try to convert pointer value to string in the case if function
 * is implemented as a static one.
 *
 * @param handler  pointer to function
 *
 * @return Allocated name or NULL in the case of memory allocation failure
 */
static char *
handler2name(void *handler)
{
    char *tmp;
    
    if (handler == NULL)
        tmp = strdup("0");
    else if ((tmp = rcf_ch_symbol_name(handler)) != NULL)
        tmp = strdup(tmp);
    else if ((tmp = calloc(1, 16)) != NULL)
    {
        /* FIXME */
        int id = rcf_pch_mem_get_id(handler);
        
        if (id == 0)
            id = rcf_pch_mem_alloc(handler);

        /* FIXME */
        sprintf(tmp, "%d", id);
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
    VERB("PID=%d TID=%d: Entry %s",
         (int)getpid(), (int)pthread_self(), "setlibname");

    out->common._errno = tarpc_setlibname((in->libname.libname_len == 0) ?
                                          NULL : in->libname.libname_val);
    out->retval = (out->common._errno == 0) ? 0 : -1;
    out->common.duration = 0;

    return TRUE;
}


/*-------------- rpc_is_op_done() -----------------------------*/

bool_t
_rpc_is_op_done_1_svc(tarpc_rpc_is_op_done_in  *in,
                      tarpc_rpc_is_op_done_out *out,
                      struct svc_req           *rqstp)
{
    te_bool *is_done = (te_bool *)rcf_pch_mem_get(in->common.done);

    UNUSED(rqstp);

    memset(out, 0, sizeof(*out));

    if ((is_done != NULL) && (in->common.op == RCF_RPC_IS_DONE))
    {
        out->common._errno = 0;
        out->common.done = (*is_done) ? in->common.done : 0;
    }
    else
    {
        out->common._errno = TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    return TRUE;
}

/*-------------- fork() ---------------------------------*/

TARPC_FUNC(fork, {},
{
    MAKE_CALL(out->pid = func_void());

    if (out->pid == 0)
    {
        rcf_pch_detach();
        rcf_pch_rpc_server(in->name.name_val);
        exit(EXIT_FAILURE);
    }
}
)

/*-------------- pthread_create() -----------------------------*/
TARPC_FUNC(pthread_create, {},
{
    pthread_t tid;
    
    MAKE_CALL(out->retval = func_ptr(&tid, NULL, rcf_pch_rpc_server,
                                     strdup(in->name.name_val)));
    if (out->retval == 0)                                 
        out->tid = rcf_pch_mem_alloc((void *)tid);
}
)

/*-------------- pthread_cancel() -----------------------------*/
TARPC_FUNC(pthread_cancel, {}, 
{ 
    MAKE_CALL(out->retval = func_ptr(rcf_pch_mem_get(in->tid)));
    rcf_pch_mem_free(in->tid);
}
)

/**
 * Check, if some signals were received by the RPC server (as a process)
 * and return the mask of received signals.
 */

bool_t
_sigreceived_1_svc(tarpc_sigreceived_in *in, tarpc_sigreceived_out *out,
                   struct svc_req *rqstp)
{
    static rcf_pch_mem_id id = 0;
    
    UNUSED(in);
    UNUSED(rqstp);
    memset(out, 0, sizeof(*out));
    
    if (id == 0)
        id = rcf_pch_mem_alloc(&rpcs_received_signals);
    out->set = id;

    return TRUE;
}

/*-------------- execve() ---------------------------------*/
TARPC_FUNC(execve, {},
{
    const char *argv[5];
    
    int rc;
    
    memset(argv, 0, sizeof(argv));
    argv[0] = ta_execname;
    argv[1] = "exec";
    argv[2] = "rcf_pch_rpc_server_argv";
    argv[3] = in->name;

    /* Wait until main thread sends answer to non-blocking RPC call */
    sleep(1);

    VERB("execve() args: %s, %s, %s, %s", 
         argv[0], argv[1], argv[2], argv[3]);
    /* Call execve() */
    MAKE_CALL(rc = func_ptr((void *)ta_execname, argv, environ));
    if (rc != 0)
    {
        rc = errno;
        PRINT("execve() failed: errno=%d", rc);
    }
}
)

/*-------------- getpid() --------------------------------*/
TARPC_FUNC(getpid, {}, { MAKE_CALL(out->retval = func_void()); })

/*-------------- gettimeofday() --------------------------------*/
TARPC_FUNC(gettimeofday,
{
    COPY_ARG(tv);
    COPY_ARG(tz);
},
{
    struct timeval  tv;
    struct timezone tz;

    if (out->tv.tv_len != 0)
        TARPC_CHECK_RC(timeval_rpc2h(out->tv.tv_val, &tv));
    if (out->tz.tz_len != 0)
        TARPC_CHECK_RC(timezone_rpc2h(out->tz.tz_val, &tz));

    if (out->common._errno != 0)
    {
        out->retval = -1;
    }
    else
    {
        MAKE_CALL(out->retval = func_ptr(out->tv.tv_len == 0 ? NULL : &tv,
                                         out->tz.tz_len == 0 ? NULL : &tz));

        if (out->tv.tv_len != 0)
            TARPC_CHECK_RC(timeval_h2rpc(&tv, out->tv.tv_val));
        if (out->tz.tz_len != 0)
            TARPC_CHECK_RC(timezone_h2rpc(&tz, out->tz.tz_val));
        if (TE_RC_GET_ERROR(out->common._errno) == TE_EH2RPC)
            out->retval = -1;
    }
}
)

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

TARPC_FUNC(dup2, {}, { MAKE_CALL(out->fd = func(in->oldfd, in->newfd)); })

/*-------------- close() ------------------------------*/

TARPC_FUNC(close, {}, { MAKE_CALL(out->retval = func(in->fd)); })

/*-------------- bind() ------------------------------*/

TARPC_FUNC(bind, {},
{
    PREPARE_ADDR(in->addr, 0);
    MAKE_CALL(out->retval = func(in->fd, a, in->len));
}
)

/*-------------- connect() ------------------------------*/

TARPC_FUNC(connect, {},
{
    PREPARE_ADDR(in->addr, 0);

    MAKE_CALL(out->retval = func(in->fd, a, in->len));
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
    PREPARE_ADDR(out->addr, out->len.len_len == 0 ? 0 : *out->len.len_val);

    MAKE_CALL(out->retval = func(in->fd, a,
                                 out->len.len_len == 0 ? NULL :
                                 out->len.len_val));
    sockaddr_h2rpc(a, &(out->addr));
}
)

/*-------------- recvfrom() ------------------------------*/


TARPC_FUNC(recvfrom,
{
    COPY_ARG(buf);
    COPY_ARG(fromlen);
    COPY_ARG_ADDR(from);
},
{
    PREPARE_ADDR(out->from, out->fromlen.fromlen_len == 0 ? 0 :
                            *out->fromlen.fromlen_val);


    INIT_CHECKED_ARG(out->buf.buf_val, out->buf.buf_len, in->len);

    MAKE_CALL(out->retval = func(in->fd, out->buf.buf_val, in->len,
                                 send_recv_flags_rpc2h(in->flags), a,
                                 out->fromlen.fromlen_len == 0 ? NULL :
                                 out->fromlen.fromlen_val));
    sockaddr_h2rpc(a, &(out->from));
}
)


/*-------------- recv() ------------------------------*/

TARPC_FUNC(recv,
{
    COPY_ARG(buf);
},
{
    INIT_CHECKED_ARG(out->buf.buf_val, out->buf.buf_len, in->len);

    MAKE_CALL(out->retval = func(in->fd, out->buf.buf_val, in->len,
                                 send_recv_flags_rpc2h(in->flags)));
}
)

/*-------------- shutdown() ------------------------------*/

TARPC_FUNC(shutdown, {},
{
    MAKE_CALL(out->retval = func(in->fd, shut_how_rpc2h(in->how)));
}
)

/*-------------- sendto() ------------------------------*/

TARPC_FUNC(sendto, {},
{
    PREPARE_ADDR(in->to, 0);

    INIT_CHECKED_ARG(in->buf.buf_val, in->buf.buf_len, 0);

    MAKE_CALL(out->retval = func(in->fd, in->buf.buf_val, in->len,
                                 send_recv_flags_rpc2h(in->flags),
                                 a, in->tolen));
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

/*-------------- write() ------------------------------*/

TARPC_FUNC(write, {},
{
    INIT_CHECKED_ARG(in->buf.buf_val, in->buf.buf_len, 0);

    MAKE_CALL(out->retval = func(in->fd, in->buf.buf_val, in->len));
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
    unsigned int    i;

    memset(iovec_arr, 0, sizeof(iovec_arr));
    for (i = 0; i < out->vector.vector_len; i++)
    {
        INIT_CHECKED_ARG(out->vector.vector_val[i].iov_base.iov_base_val,
                         out->vector.vector_val[i].iov_base.iov_base_len,
                         out->vector.vector_val[i].iov_len);
        iovec_arr[i].iov_base =
            out->vector.vector_val[i].iov_base.iov_base_val;
        iovec_arr[i].iov_len = out->vector.vector_val[i].iov_len;
    }
    INIT_CHECKED_ARG((char *)iovec_arr, sizeof(iovec_arr), 0);

    MAKE_CALL(out->retval = func(in->fd, iovec_arr, in->count));
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
    unsigned int    i;

    memset(iovec_arr, 0, sizeof(iovec_arr));
    for (i = 0; i < in->vector.vector_len; i++)
    {
        INIT_CHECKED_ARG(in->vector.vector_val[i].iov_base.iov_base_val,
                         in->vector.vector_val[i].iov_base.iov_base_len, 0);
        iovec_arr[i].iov_base =
            in->vector.vector_val[i].iov_base.iov_base_val;
        iovec_arr[i].iov_len = in->vector.vector_val[i].iov_len;
    }
    INIT_CHECKED_ARG((char *)iovec_arr, sizeof(iovec_arr), 0);

    MAKE_CALL(out->retval = func(in->fd, iovec_arr, in->count));
}
)

/*-------------- getsockname() ------------------------------*/
TARPC_FUNC(getsockname,
{
    COPY_ARG(len);
    COPY_ARG_ADDR(addr);
},
{
    PREPARE_ADDR(out->addr, out->len.len_len == 0 ? 0 : *out->len.len_val);

    MAKE_CALL(out->retval = func(in->fd, a,
                                 out->len.len_len == 0 ? NULL :
                                 out->len.len_val));
    sockaddr_h2rpc(a, &(out->addr));
}
)

/*-------------- getpeername() ------------------------------*/

TARPC_FUNC(getpeername,
{
    COPY_ARG(len);
    COPY_ARG_ADDR(addr);
},
{
    PREPARE_ADDR(out->addr, out->len.len_len == 0 ? 0 : *out->len.len_val);

    MAKE_CALL(out->retval = func(in->fd, a,
                                 out->len.len_len == 0 ? NULL :
                                 out->len.len_val));
    sockaddr_h2rpc(a, &(out->addr));
}
)

/*-------------- fd_set constructor ----------------------------*/

bool_t
_fd_set_new_1_svc(tarpc_fd_set_new_in *in, tarpc_fd_set_new_out *out,
                  struct svc_req *rqstp)
{
    fd_set *set;

    UNUSED(rqstp);
    UNUSED(in);

    memset(out, 0, sizeof(*out));

    errno = 0;
    if ((set = (fd_set *)calloc(1, sizeof(fd_set))) == NULL)
    {
        out->common._errno = TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }
    else
    {
        out->retval = rcf_pch_mem_alloc(set);
        out->common._errno = RPC_ERRNO;
    }

    return TRUE;
}

/*-------------- fd_set destructor ----------------------------*/

bool_t
_fd_set_delete_1_svc(tarpc_fd_set_delete_in *in,
                     tarpc_fd_set_delete_out *out,
                     struct svc_req *rqstp)
{
    UNUSED(rqstp);

    memset(out, 0, sizeof(*out));

    errno = 0;
    free(IN_FDSET);
    rcf_pch_mem_free(in->set);
    out->common._errno = RPC_ERRNO;

    return TRUE;
}

/*-------------- FD_ZERO --------------------------------*/

bool_t
_do_fd_zero_1_svc(tarpc_do_fd_zero_in *in, tarpc_do_fd_zero_out *out,
                  struct svc_req *rqstp)
{
    UNUSED(rqstp);

    memset(out, 0, sizeof(*out));

    FD_ZERO(IN_FDSET);
    return TRUE;
}

/*-------------- FD_SET --------------------------------*/

bool_t
_do_fd_set_1_svc(tarpc_do_fd_set_in *in, tarpc_do_fd_set_out *out,
                 struct svc_req *rqstp)
{
    UNUSED(rqstp);

    memset(out, 0, sizeof(*out));

    FD_SET(in->fd, IN_FDSET);
    return TRUE;
}

/*-------------- FD_CLR --------------------------------*/

bool_t
_do_fd_clr_1_svc(tarpc_do_fd_clr_in *in, tarpc_do_fd_clr_out *out,
                 struct svc_req *rqstp)
{
    UNUSED(rqstp);

    memset(out, 0, sizeof(*out));

    FD_SET(in->fd, IN_FDSET);
    return TRUE;
}

/*-------------- FD_ISSET --------------------------------*/

bool_t
_do_fd_isset_1_svc(tarpc_do_fd_isset_in *in, tarpc_do_fd_isset_out *out,
                   struct svc_req *rqstp)
{
    UNUSED(rqstp);

    memset(out, 0, sizeof(*out));

    out->retval = FD_ISSET(in->fd, IN_FDSET);
    return TRUE;
}

/*-------------- select() --------------------------------*/

TARPC_FUNC(select,
{
    COPY_ARG(timeout);
},
{
    struct timeval tv;

    if (out->timeout.timeout_len > 0)
        TARPC_CHECK_RC(timeval_rpc2h(out->timeout.timeout_val, &tv));

    if (out->common._errno != 0)
    {
        out->retval = -1;
    }
    else
    {
        MAKE_CALL(out->retval = func(in->n, 
                      (fd_set *)rcf_pch_mem_get(in->readfds),
                      (fd_set *)rcf_pch_mem_get(in->writefds),
                      (fd_set *)rcf_pch_mem_get(in->exceptfds),
                      out->timeout.timeout_len == 0 ? NULL :
                      &tv));

        if (out->timeout.timeout_len > 0)
            TARPC_CHECK_RC(timeval_h2rpc(&tv, out->timeout.timeout_val));
        if (TE_RC_GET_ERROR(out->common._errno) == TE_EH2RPC)
            out->retval = -1;
    }
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
    char *name;

    memcmp(name, out->ifname.ifname_val, out->ifname.ifname_len);

    MAKE_CALL(name = (char *)func_ret_ptr(in->ifindex, 
                                          out->ifname.ifname_val));

    if (name != NULL && name != out->ifname.ifname_val)
    {
        ERROR("if_indextoname returned incorrect pointer");
        out->common._errno = TE_RC(TE_TA_UNIX, TE_ECORRUPTED);
    }

    if (name == NULL &&
        memcmp(name, out->ifname.ifname_val, out->ifname.ifname_len) != 0)
    {
        out->common._errno = TE_RC(TE_TA_UNIX, TE_ECORRUPTED);
    }
}
)


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

/*-------------- kill() --------------------------------*/

TARPC_FUNC(kill, {},
{
    MAKE_CALL(out->retval = func(in->pid, signum_rpc2h(in->signum)));
}
)

/*-------------- waitpid() --------------------------------*/

TARPC_FUNC(waitpid, {},
{
    int             st;
    rpc_wait_status r_st;

    func = (api_func)ta_waitpid;
    MAKE_CALL(out->pid = func(in->pid, &st, 
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
        
        if (old_handler != SIG_ERR)
        {
            /* FIXME */
            out->handler = handler2name(old_handler);

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

/*-------------- sigaction() --------------------------------*/

/** Return pointer to sa_restorer field of the structure or dummy address */
static void **
get_sa_restorer(struct sigaction *sa)
{
#if HAVE_STRUCT_SIGACTION_SA_RESTORER
    return (void **)&(sa->sa_restorer);
#else    
    static void *dummy = NULL;
    
    UNUSED(sa);
    return &dummy;
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
    tarpc_sigaction *in_act;
    tarpc_sigaction *out_oldact;

    struct sigaction  act;
    struct sigaction *p_act = NULL;
    struct sigaction  oldact;
    struct sigaction *p_oldact = NULL;

    memset(&act, 0, sizeof(act));
    memset(&oldact, 0, sizeof(oldact));
    in_act = in->act.act_val;
    out_oldact = out->oldact.oldact_val;
    
    if (in->act.act_len != 0)
    {
        p_act = &act;
        
        act.sa_flags = sigaction_flags_rpc2h(in_act->flags);
        act.sa_mask = *((sigset_t *)rcf_pch_mem_get(in_act->mask));

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
        
        out->common._errno = name2handler(in_act->restorer,
                                          get_sa_restorer(&act));
        if (out->common._errno != 0)
        {
            out->retval = -1;
            goto finish;     
        }
    }

    if (out->oldact.oldact_len != 0)
        p_oldact = &oldact;

    MAKE_CALL(out->retval = func(signum_rpc2h(in->signum),
                                 p_act, p_oldact));

    if (out->retval == 0 && p_oldact != NULL)
    {
        out_oldact->flags = sigaction_flags_h2rpc(oldact.sa_flags);
        out_oldact->mask = rcf_pch_mem_alloc(&oldact.sa_mask);
        out_oldact->handler = handler2name(oldact.sa_handler);
        out_oldact->restorer = handler2name(*(get_sa_restorer(&oldact)));
    }
    
    finish:
    ;
}
)

/*-------------- setsockopt() ------------------------------*/

typedef union opt_param {
    int                 integer;
    char               *str;
    struct linger       linger;
#ifdef __linux__
    struct ip_mreqn     mreqn;
#endif
    struct ip_mreq      mreq;
    struct in_addr      addr;
    struct timeval      tv;
} opt_param;

static void
tarpc_setsockopt(tarpc_setsockopt_in *in, tarpc_setsockopt_out *out,
                 opt_param *param, socklen_t *optlen)
{
    switch (in->optval.optval_val[0].opttype)
    {
        case OPT_INT:
        {
            param->integer = in->optval.optval_val[0].
                                 option_value_u.opt_int;
            *optlen = sizeof(int);
            break;
        }

        case OPT_LINGER:
        {
            param->linger.l_onoff = in->optval.optval_val[0].
                                    option_value_u.opt_linger.l_onoff;
            param->linger.l_linger = in->optval.optval_val[0].
                                     option_value_u.opt_linger.l_linger;
            *optlen = sizeof(param->linger);
            break;
        }
        
        case OPT_MREQ:
        {
            memcpy(&param->mreq.imr_multiaddr, &in->optval.optval_val[0].
                   option_value_u.opt_mreq.imr_multiaddr,
                   sizeof(param->mreq.imr_multiaddr));
            memcpy(&param->mreq.imr_interface, &in->optval.optval_val[0].
                   option_value_u.opt_mreq.imr_address,
                   sizeof(param->mreq.imr_interface));
            *optlen = sizeof(param->mreq);
            break;
        }

#ifdef __linux__
        case OPT_MREQN:
        {
            memcpy((char *)&(param->mreqn.imr_multiaddr),
                   &in->optval.optval_val[0].option_value_u.opt_mreqn.
                   imr_multiaddr, sizeof(param->mreqn.imr_multiaddr));
            memcpy((char *)&(param->mreqn.imr_address),
                   &in->optval.optval_val[0].option_value_u.opt_mreqn.
                   imr_address, sizeof(param->mreqn.imr_address));
            
            param->mreqn.imr_ifindex = in->optval.optval_val[0].
                                       option_value_u.
                                       opt_mreqn.imr_ifindex;
            *optlen = sizeof(param->mreqn);
            break;
        }
#endif

        case OPT_IPADDR:
        {
            memcpy(&param->addr,
                   &in->optval.optval_val[0].option_value_u.opt_ipaddr,
                   sizeof(struct in_addr));
            *optlen = sizeof(&param->addr);
            break;
        }

        case OPT_TIMEVAL:
        {
            param->tv.tv_sec = in->optval.optval_val[0].option_value_u.
                               opt_timeval.tv_sec;
            param->tv.tv_usec = in->optval.optval_val[0].option_value_u.
                                opt_timeval.tv_usec;
            *optlen = sizeof(param->tv);
            break;
        }

        case OPT_STRING:
        {
            param->str = in->optval.optval_val[0].option_value_u.
                             opt_string.opt_string_val;
            *optlen = in->optval.optval_val[0].option_value_u.
                          opt_string.opt_string_len;
            break;
        }

        default:
            ERROR("incorrect option type %d is received",
                  in->optval.optval_val[0].opttype);
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
                                     NULL, in->optlen));
    }
    else
    {
        opt_param  param;
        socklen_t  optlen;
        void      *optval;

        tarpc_setsockopt(in, out, &param, &optlen);
        if (out->retval == 0)
        {
            if (in->optval.optval_val[0].opttype == OPT_STRING)
                optval = param.str;
            else
                optval = &param;
            INIT_CHECKED_ARG(optval, optlen, 0);
            if (in->optlen == RPC_OPTLEN_AUTO)
                in->optlen = optlen;
            MAKE_CALL(out->retval = func(in->s, socklevel_rpc2h(in->level),
                                         sockopt_rpc2h(in->optname),
                                         optval, in->optlen));
        }
    }       
}
)


/*-------------- getsockopt() ------------------------------*/

#define COPY_TCP_INFO_FIELD(_name) \
    do {                                                               \
        out->optval.optval_val[0].option_value_u.                      \
            opt_tcp_info._name = info->_name;                          \
    } while (0)

#ifdef __linux__

TARPC_FUNC(getsockopt,
{
    COPY_ARG(optval);
    COPY_ARG(optlen);
},
{
    int optlen_in = 0;
    int optlen_out = 0;
    
    if (out->optval.optval_val == NULL)
    {
        MAKE_CALL(out->retval = func(in->s, socklevel_rpc2h(in->level),
                                     sockopt_rpc2h(in->optname),
                                     NULL, out->optlen.optlen_val));
    }
    else
    {
        char opt[sizeof(struct linger)
                 + sizeof(struct ip_mreqn) + sizeof(struct tcp_info)];
                 
        if (out->optlen.optlen_val != NULL &&
            *(out->optlen.optlen_val) == RPC_OPTLEN_AUTO)
        {
            switch (out->optval.optval_val[0].opttype)
            {
                case OPT_INT:
                    optlen_in = optlen_out = sizeof(int);
                    break;
                    
                case OPT_LINGER:
                    optlen_in = optlen_out = sizeof(struct linger);
                    break;

                case OPT_MREQN:
                    optlen_in = optlen_out = sizeof(struct ip_mreqn);
                    break;

                case OPT_IPADDR:
                    optlen_in = optlen_out = sizeof(struct in_addr);
                    break;

                case OPT_TIMEVAL:
                    optlen_in = optlen_out = sizeof(struct timeval);
                    break;

                case OPT_TCP_INFO:
                    optlen_in = optlen_out = sizeof(struct tcp_info);
                    break;

                default:
                    ERROR("incorrect option type %d is received",
                          out->optval.optval_val[0].opttype);
                    break;
            }
        }
        else if (out->optlen.optlen_val != NULL)
            optlen_in = optlen_out = *(out->optlen.optlen_val);

        memset(opt, 0, sizeof(opt));
        INIT_CHECKED_ARG(opt, sizeof(opt), optlen_in);

        MAKE_CALL(out->retval = 
                      func(in->s, socklevel_rpc2h(in->level),
                           sockopt_rpc2h(in->optname),
                           opt, 
                           out->optlen.optlen_val == NULL ? NULL 
                                                          : &optlen_out));
        if (optlen_in != optlen_out)
        {
            /* Workaround to avoid corrupting of "auto" length */
            *(out->optlen.optlen_val) = optlen_out;
        }
                                                          
        switch (out->optval.optval_val[0].opttype)
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
                if (in->level == RPC_SOL_SOCKET &&
                    in->optname == RPC_SO_TYPE)
                {
                    *(int *)opt = socktype_h2rpc(*(int *)opt);
                }
                out->optval.optval_val[0].option_value_u.opt_int =
                    *(int *)opt;
                break;
            }
            case OPT_LINGER:
            {
                struct linger *linger = (struct linger *)opt;

                out->optval.optval_val[0].option_value_u.
                    opt_linger.l_onoff = linger->l_onoff;
                out->optval.optval_val[0].option_value_u.
                    opt_linger.l_linger = linger->l_linger;
                break;
            }

            case OPT_MREQN:
            {
                struct ip_mreqn *mreqn = (struct ip_mreqn *)opt;

                memcpy(&out->optval.optval_val[0].option_value_u.opt_mreqn.
                       imr_multiaddr, (char *)&(mreqn->imr_multiaddr),
                       sizeof(mreqn->imr_multiaddr));
                memcpy(&out->optval.optval_val[0].option_value_u.opt_mreqn.
                       imr_address, (char *)&(mreqn->imr_address),
                       sizeof(mreqn->imr_address));
                out->optval.optval_val[0].option_value_u.opt_mreqn.
                    imr_ifindex = mreqn->imr_ifindex;
                break;
            }

            case OPT_MREQ:
            {
                struct ip_mreq *mreq = (struct ip_mreq *)opt;

                memcpy(&out->optval.optval_val[0].option_value_u.opt_mreq.
                       imr_multiaddr, (char *)&(mreq->imr_multiaddr),
                       sizeof(mreq->imr_multiaddr));
                memcpy(&out->optval.optval_val[0].option_value_u.opt_mreq.
                       imr_address, (char *)&(mreq->imr_interface),
                       sizeof(mreq->imr_interface));
                break;
            }
            
            case OPT_IPADDR:
            {
                struct in_addr *addr = (struct in_addr *)opt;

                memcpy(&out->optval.optval_val[0].option_value_u.opt_ipaddr,
                       addr, sizeof(*addr));
                break;
            }

            case OPT_TIMEVAL:
            {
                struct timeval *tv = (struct timeval *)opt;

                out->optval.optval_val[0].option_value_u.
                    opt_timeval.tv_sec = tv->tv_sec;
                out->optval.optval_val[0].option_value_u.
                    opt_timeval.tv_usec = tv->tv_usec;
                break;
            }

            case OPT_STRING:
            {
                char *str = (char *)opt;

                memcpy(out->optval.optval_val[0].option_value_u.opt_string.
                           opt_string_val,
                       str,
                       out->optval.optval_val[0].option_value_u.opt_string.
                           opt_string_len);
                break;
            }

            case OPT_TCP_INFO:
            {
                struct tcp_info *info = (struct tcp_info *)opt;

                COPY_TCP_INFO_FIELD(tcpi_state);
                COPY_TCP_INFO_FIELD(tcpi_ca_state);
                COPY_TCP_INFO_FIELD(tcpi_retransmits);
                COPY_TCP_INFO_FIELD(tcpi_probes);
                COPY_TCP_INFO_FIELD(tcpi_backoff);
                COPY_TCP_INFO_FIELD(tcpi_options);
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
                break;
            }

            default:
                ERROR("incorrect option type %d is received",
                      out->optval.optval_val[0].opttype);
                break;
        }
    }
}
)

#else

TARPC_FUNC(getsockopt,
{
    COPY_ARG(optval);
    COPY_ARG(optlen);
},
{
    if (out->optval.optval_val == NULL)
    {
        MAKE_CALL(out->retval = func(in->s, socklevel_rpc2h(in->level),
                                     sockopt_rpc2h(in->optname),
                                     NULL, out->optlen.optlen_val));
    }
    else
    {
        char opt[sizeof(struct linger)];

        if (out->optlen.optlen_val != NULL &&
            *(out->optlen.optlen_val) == RPC_OPTLEN_AUTO)
        {
            switch (out->optval.optval_val[0].opttype)
            {
                case OPT_INT:
                    *(out->optlen.optlen_val) = sizeof(int);
                    break;
                    
                case OPT_LINGER:
                    *(out->optlen.optlen_val) = sizeof(struct linger);
                    break;

                case OPT_MREQ:
                    *(out->optlen.optlen_val) = sizeof(struct ip_mreq);
                    break;

                case OPT_IPADDR:
                    *(out->optlen.optlen_val) = sizeof(struct in_addr);
                    break;

                case OPT_TIMEVAL:
                    *(out->optlen.optlen_val) = sizeof(struct timeval);
                    break;

                default:
                    ERROR("incorrect option type %d is received",
                          out->optval.optval_val[0].opttype);
                    break;
            }
        }

        memset(opt, 0, sizeof(opt));
        INIT_CHECKED_ARG(opt, sizeof(opt),
                         (out->optlen.optlen_val == NULL) ?
                            0 : *out->optlen.optlen_val);
        MAKE_CALL(out->retval = func(in->s, socklevel_rpc2h(in->level),
                                     sockopt_rpc2h(in->optname),
                                     opt, out->optlen.optlen_val));
        switch (out->optval.optval_val[0].opttype)
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
                if (in->level == RPC_SOL_SOCKET &&
                    in->optname == RPC_SO_TYPE)
                {
                    *(int *)opt = socktype_h2rpc(*(int *)opt);
                }
                out->optval.optval_val[0].option_value_u.opt_int =
                    *(int *)opt;
                break;
            }
            case OPT_LINGER:
            {
                struct linger *linger = (struct linger *)opt;

                out->optval.optval_val[0].option_value_u.
                    opt_linger.l_onoff = linger->l_onoff;
                out->optval.optval_val[0].option_value_u.
                    opt_linger.l_linger = linger->l_linger;
                break;
            }

            case OPT_MREQ:
            {
                struct ip_mreq *mreq = (struct ip_mreq *)opt;
                
                memcpy(&out->optval.optval_val[0].option_value_u.opt_mreq.
                       imr_multiaddr, (char *)&(mreq->imr_multiaddr),
                       sizeof(mreq->imr_multiaddr));
                memcpy(&out->optval.optval_val[0].option_value_u.opt_mreq.
                       imr_address, (char *)&(mreq->imr_address),
                       sizeof(mreq->imr_address));
                break;
            }

            case OPT_IPADDR:
            {
                struct in_addr *addr = (struct in_addr *)opt;

                memcpy(&out->optval.optval_val[0].option_value_u.opt_ipaddr,
                       addr, sizeof(*addr));
                break;
            }

            case OPT_TIMEVAL:
            {
                struct timeval *tv = (struct timeval *)opt;

                out->optval.optval_val[0].option_value_u.
                    opt_timeval.tv_sec = tv->tv_sec;
                out->optval.optval_val[0].option_value_u.
                    opt_timeval.tv_usec = tv->tv_usec;
                break;
            }

            case OPT_STRING:
            {
                char *str = (char *)opt;

                memcpy(out->optval.optval_val[0].option_value_u.opt_string.
                           opt_string_val,
                       str,
                       out->optval.optval_val[0].option_value_u.opt_string.
                           opt_string_len);
                break;
            }

            default:
                ERROR("incorrect option type %d is received",
                      out->optval.optval_val[0].opttype);
                break;
        }
    }
}
)

#endif

#undef COPY_TCP_INFO_FIELD

/*-------------- pselect() --------------------------------*/

TARPC_FUNC(pselect, {},
{
    struct timespec tv;

    if (in->timeout.timeout_len > 0)
    {
        tv.tv_sec = in->timeout.timeout_val[0].tv_sec;
        tv.tv_nsec = in->timeout.timeout_val[0].tv_nsec;
    }
    INIT_CHECKED_ARG((char *)&tv, sizeof(tv), 0);

    /* 
     * The pointer may be a NULL and, therefore, contain uninitialized
     * data, but we want to check that the data are unchanged even in
     * this case.
     */
    INIT_CHECKED_ARG((char *)rcf_pch_mem_get(in->sigmask), 
                     sizeof(sigset_t), 0);

    MAKE_CALL(out->retval = func(in->n,
                                 (fd_set *)rcf_pch_mem_get(in->readfds),
                                 (fd_set *)rcf_pch_mem_get(in->writefds),
                                 (fd_set *)rcf_pch_mem_get(in->exceptfds),
                                 in->timeout.timeout_len == 0 ? NULL : &tv,
                                 rcf_pch_mem_get(in->sigmask)));
}
)

TARPC_FUNC(fcntl, {},
{
    long arg = in->arg;
    
    if (in->cmd == RPC_F_SETFL)
        arg = fcntl_flags_rpc2h(in->arg);

    if (in->cmd == RPC_F_GETFD || in->cmd == RPC_F_GETFL)
        MAKE_CALL(out->retval = func(in->fd, fcntl_rpc2h(in->cmd)));
    else
        MAKE_CALL(out->retval = func(in->fd, fcntl_rpc2h(in->cmd), arg));

    if (in->cmd == RPC_F_GETFL)
        out->retval = fcntl_flags_h2rpc(out->retval);
}
)

#ifdef __linux__

TARPC_FUNC(ioctl,
{
    COPY_ARG(req);
},
{
    char *req = NULL;
    int   reqlen = 0;

    static struct timeval req_timeval;
    static int            req_int;
    static struct ifreq   req_ifreq;
    static struct ifconf  req_ifconf;
    static struct arpreq  req_arpreq;

    if (out->req.req_val != NULL)
    {
        switch (out->req.req_val[0].type)
        {
            case IOCTL_TIMEVAL:
            {
                req = (char *)&req_timeval;
                reqlen = sizeof(struct timeval);
                req_timeval.tv_sec = out->req.req_val[0].
                    ioctl_request_u.req_timeval.tv_sec;
                req_timeval.tv_usec = out->req.req_val[0].
                    ioctl_request_u.req_timeval.tv_usec;
                break;
            }

            case IOCTL_INT:
            {
                req = (char *)&req_int;
                req_int = out->req.req_val[0].ioctl_request_u.req_int;
                reqlen = sizeof(int);
                break;
            }

            case IOCTL_IFREQ:
            {
                req = (char *)&req_ifreq;
                reqlen = sizeof(struct ifreq);

                memset(req, 0, reqlen);
                /* Copy the whole 'ifr_name' buffer, not just strcpy() */
                memcpy(req_ifreq.ifr_name,
                       out->req.req_val[0].ioctl_request_u.req_ifreq.
                           rpc_ifr_name.rpc_ifr_name_val,
                       sizeof(req_ifreq.ifr_name));

                INIT_CHECKED_ARG(req_ifreq.ifr_name,
                                 strlen(req_ifreq.ifr_name) + 1, 0);

                switch (in->code)
                {
                    case RPC_SIOCSIFFLAGS:
                        req_ifreq.ifr_flags =
                            if_fl_rpc2h((uint32_t)(unsigned short int)
                                out->req.req_val[0].ioctl_request_u.
                                    req_ifreq.rpc_ifr_flags);
                        break;

                    case RPC_SIOCSIFMTU:
                        req_ifreq.ifr_mtu =
                            out->req.req_val[0].ioctl_request_u.
                            req_ifreq.rpc_ifr_mtu;
                        break;

                    case RPC_SIOCSIFADDR:
                    case RPC_SIOCSIFNETMASK:
                    case RPC_SIOCSIFBRDADDR:
                    case RPC_SIOCSIFDSTADDR:
                        sockaddr_rpc2h(&(out->req.req_val[0].
                            ioctl_request_u.req_ifreq.rpc_ifr_addr),
                            (struct sockaddr_storage *)
                                (&(req_ifreq.ifr_addr)));
                       break;
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

                req = (char *)&req_ifconf;
                reqlen = sizeof(req_ifconf);
                
                if (buflen > 0 && (buf = calloc(1, buflen + 64)) == NULL)
                {
                    ERROR("Out of memory");
                    out->common._errno = TE_RC(TE_TA_UNIX, TE_ENOMEM);
                    goto finish;
                }
                req_ifconf.ifc_buf = buf;
                req_ifconf.ifc_len = buflen;

                if (buf != NULL)
                    INIT_CHECKED_ARG(buf, buflen + 64, buflen);
                break;
            }
            case IOCTL_ARPREQ:
            {
                req = (char *)&req_arpreq;
                reqlen = sizeof(req_arpreq);

                memset(req, 0, reqlen);
                /* Copy protocol address for all requests */
                sockaddr_rpc2h(&(out->req.req_val[0].ioctl_request_u.
                               req_arpreq.rpc_arp_pa),
                               (struct sockaddr_storage *)
                                   (&(req_arpreq.arp_pa)));
                if (in->code == RPC_SIOCSARP)
                {
                    /* Copy HW address */
                    sockaddr_rpc2h(&(out->req.req_val[0].ioctl_request_u.
                                   req_arpreq.rpc_arp_ha),
                                   (struct sockaddr_storage *)
                                       (&(req_arpreq.arp_ha)));
                    /* Copy ARP flags */
                    req_arpreq.arp_flags =
                        arp_fl_rpc2h(out->req.req_val[0].ioctl_request_u.
                                     req_arpreq.rpc_arp_flags);
                }

                if (in->code == RPC_SIOCGARP)
                {
                     /* Copy device */
                    strcpy(req_arpreq.arp_dev,
                           out->req.req_val[0].ioctl_request_u.
                           req_arpreq.rpc_arp_dev.rpc_arp_dev_val);
                }
                break;
            }

            default:
                ERROR("incorrect request type %d is received",
                      out->req.req_val[0].type);
                out->common._errno = TE_RC(TE_TA_UNIX, TE_EINVAL);
                goto finish;
                break;
        }
    }

    if (in->access == IOCTL_WR)
        INIT_CHECKED_ARG(req, reqlen, 0);
    MAKE_CALL(out->retval = func(in->s, ioctl_rpc2h(in->code), req));
    if (req != NULL)
    {
        switch(out->req.req_val[0].type)
        {
            case IOCTL_INT:
                out->req.req_val[0].ioctl_request_u.req_int = req_int;
                break;

            case IOCTL_TIMEVAL:
                out->req.req_val[0].ioctl_request_u.req_timeval.tv_sec =
                    req_timeval.tv_sec;
                out->req.req_val[0].ioctl_request_u.req_timeval.tv_usec =
                    req_timeval.tv_usec;
                break;

            case IOCTL_IFREQ:
                switch (in->code)
                {
                    case RPC_SIOCGIFFLAGS:
                    case RPC_SIOCSIFFLAGS:
                        out->req.req_val[0].ioctl_request_u.req_ifreq.
                            rpc_ifr_flags = if_fl_h2rpc(
                                (uint32_t)(unsigned short int)
                                    req_ifreq.ifr_flags);
                        break;

                    case RPC_SIOCGIFMTU:
                    case RPC_SIOCSIFMTU:
                        out->req.req_val[0].ioctl_request_u.req_ifreq.
                            rpc_ifr_mtu = req_ifreq.ifr_mtu;
                        break;

                    case RPC_SIOCGIFADDR:
                    case RPC_SIOCSIFADDR:
                    case RPC_SIOCGIFNETMASK:
                    case RPC_SIOCSIFNETMASK:
                    case RPC_SIOCGIFBRDADDR:
                    case RPC_SIOCSIFBRDADDR:
                    case RPC_SIOCGIFDSTADDR:
                    case RPC_SIOCSIFDSTADDR:
                    case RPC_SIOCGIFHWADDR:
                        sockaddr_h2rpc(&(req_ifreq.ifr_addr),
                                       &(out->req.req_val[0].
                                         ioctl_request_u.
                                         req_ifreq.rpc_ifr_addr));
                        break;

                    default:
                        ERROR("Unsupported IOCTL request %d of type IFREQ",
                              in->code);
                        out->common._errno = TE_RC(TE_TA_UNIX, TE_EINVAL);
                        goto finish;
                }
                break;

            case IOCTL_IFCONF:
            {
                struct ifreq       *req_c;
                struct tarpc_ifreq *req_t;

                int n = 1;
                int i;

                n = out->req.req_val[0].ioctl_request_u.req_ifconf.nmemb =
                    req_ifconf.ifc_len / sizeof(struct ifreq);
                out->req.req_val[0].ioctl_request_u.req_ifconf.extra =
                    req_ifconf.ifc_len % sizeof(struct ifreq);

                if (req_ifconf.ifc_req == NULL)
                    break;

                if ((req_t = calloc(n, sizeof(*req_t))) == NULL)
                {
                    free(req_ifconf.ifc_buf);
                    ERROR("Out of memory");
                    out->common._errno = TE_RC(TE_TA_UNIX, TE_ENOMEM);
                    goto finish;
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
                        free(req_ifconf.ifc_buf);
                        ERROR("Out of memory");
                        out->common._errno = TE_RC(TE_TA_UNIX, TE_ENOMEM);
                        goto finish;
                    }
                    memcpy(req_t->rpc_ifr_name.rpc_ifr_name_val,
                           req_c->ifr_name, sizeof(req_c->ifr_name));
                    req_t->rpc_ifr_name.rpc_ifr_name_len =
                        sizeof(req_c->ifr_name);
                    if ((req_t->rpc_ifr_addr.sa_data.sa_data_val =
                         calloc(1, sizeof(struct sockaddr) - SA_COMMON_LEN))
                         == NULL)
                    {
                        /* 
                         * Already allocated memory will be released 
                         * by RPC
                         */
                        free(req_ifconf.ifc_buf);
                        ERROR("Out of memory");
                        out->common._errno = TE_RC(TE_TA_UNIX, TE_ENOMEM);
                        goto finish;
                    }
                    req_t->rpc_ifr_addr.sa_data.sa_data_len =
                        sizeof(struct sockaddr) - SA_COMMON_LEN;
                    sockaddr_h2rpc(&(req_c->ifr_addr),
                                   &(req_t->rpc_ifr_addr));
                }
                free(req_ifconf.ifc_buf);
                break;
            }

            case IOCTL_ARPREQ:
            {
                if (in->code == RPC_SIOCGARP)
                {
                    /* Copy protocol address */
                    sockaddr_h2rpc(&(req_arpreq.arp_pa),
                                   &(out->req.req_val[0].ioctl_request_u.
                                   req_arpreq.rpc_arp_pa));
                    /* Copy HW address */
                    sockaddr_h2rpc(&(req_arpreq.arp_ha),
                                   &(out->req.req_val[0].ioctl_request_u.
                                   req_arpreq.rpc_arp_ha));

                    /* Copy flags */
                    out->req.req_val[0].ioctl_request_u.req_arpreq.
                        rpc_arp_flags = arp_fl_h2rpc(req_arpreq.arp_flags);
                }
                break;
            }

        }
    }
    finish:
    ;
}
)

#else

TARPC_FUNC(ioctl,
{
    COPY_ARG(req);
},
{
    char *req = NULL;
    int   reqlen = 0;

    static struct timeval req_timeval;
    static int            req_int;
    static struct ifreq   req_ifreq;
    static struct ifconf  req_ifconf;
    static struct arpreq  req_arpreq;

    if (out->req.req_val != NULL)
    {
        switch (out->req.req_val[0].type)
        {
            case IOCTL_TIMEVAL:
            {
                req = (char *)&req_timeval;
                reqlen = sizeof(struct timeval);
                req_timeval.tv_sec = out->req.req_val[0].
                    ioctl_request_u.req_timeval.tv_sec;
                req_timeval.tv_usec = out->req.req_val[0].
                    ioctl_request_u.req_timeval.tv_usec;
                break;
            }

            case IOCTL_INT:
            {
                req = (char *)&req_int;
                req_int = out->req.req_val[0].ioctl_request_u.req_int;
                reqlen = sizeof(int);
                break;
            }

            case IOCTL_IFREQ:
            {
                req = (char *)&req_ifreq;
                reqlen = sizeof(struct ifreq);

                memset(req, 0, reqlen);
                /* Copy the whole 'ifr_name' buffer, not just strcpy() */
                memcpy(req_ifreq.ifr_name,
                       out->req.req_val[0].ioctl_request_u.req_ifreq.
                           rpc_ifr_name.rpc_ifr_name_val,
                       sizeof(req_ifreq.ifr_name));

                INIT_CHECKED_ARG(req_ifreq.ifr_name,
                                 strlen(req_ifreq.ifr_name) + 1, 0);

                switch (in->code)
                {
                    case RPC_SIOCSIFFLAGS:
                        req_ifreq.ifr_flags =
                            if_fl_rpc2h((uint32_t)(unsigned short int)
                                out->req.req_val[0].ioctl_request_u.
                                    req_ifreq.rpc_ifr_flags);
                        break;

                    case RPC_SIOCSIFMTU:
                        req_ifreq.ifr_mtu =
                            out->req.req_val[0].ioctl_request_u.
                            req_ifreq.rpc_ifr_mtu;
                        break;

                    case RPC_SIOCSIFADDR:
                    case RPC_SIOCSIFNETMASK:
                    case RPC_SIOCSIFBRDADDR:
                    case RPC_SIOCSIFDSTADDR:
                        sockaddr_rpc2h(&(out->req.req_val[0].
                            ioctl_request_u.req_ifreq.rpc_ifr_addr),
                            (struct sockaddr_storage *)
                                (&(req_ifreq.ifr_addr)));
                       break;
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

                req = (char *)&req_ifconf;
                reqlen = sizeof(req_ifconf);
                
                if (buflen > 0 && (buf = calloc(1, buflen + 64)) == NULL)
                {
                    ERROR("Out of memory");
                    out->common._errno = TE_RC(TE_TA_UNIX, TE_ENOMEM);
                    goto finish;
                }
                req_ifconf.ifc_buf = buf;
                req_ifconf.ifc_len = buflen;

                if (buf != NULL)
                    INIT_CHECKED_ARG(buf, buflen + 64, buflen);
                break;
            }

            case IOCTL_ARPREQ:
            {
                req = (char *)&req_arpreq;
                reqlen = sizeof(req_arpreq);

                memset(req, 0, reqlen);
                /* Copy protocol address for all requests */
                sockaddr_rpc2h(&(out->req.req_val[0].ioctl_request_u.
                               req_arpreq.rpc_arp_pa),
                               (struct sockaddr_storage *)
                                   (&(req_arpreq.arp_pa)));
                if (in->code == RPC_SIOCSARP)
                {
                    /* Copy HW address */
                    sockaddr_rpc2h(&(out->req.req_val[0].ioctl_request_u.
                                   req_arpreq.rpc_arp_ha),
                                   (struct sockaddr_storage *)
                                       (&(req_arpreq.arp_ha)));
                    /* Copy ARP flags */
                    req_arpreq.arp_flags =
                        arp_fl_rpc2h(out->req.req_val[0].ioctl_request_u.
                                     req_arpreq.rpc_arp_flags);
                }
                break;
            }

            default:
                ERROR("incorrect request type %d is received",
                      out->req.req_val[0].type);
                out->common._errno = TE_RC(TE_TA_UNIX, TE_EINVAL);
                goto finish;
                break;
        }
    }

    if (in->access == IOCTL_WR)
        INIT_CHECKED_ARG(req, reqlen, 0);
    MAKE_CALL(out->retval = func(in->s, ioctl_rpc2h(in->code), req));
    if (req != NULL)
    {
        switch(out->req.req_val[0].type)
        {
            case IOCTL_INT:
                out->req.req_val[0].ioctl_request_u.req_int = req_int;
                break;

            case IOCTL_TIMEVAL:
                out->req.req_val[0].ioctl_request_u.req_timeval.tv_sec =
                    req_timeval.tv_sec;
                out->req.req_val[0].ioctl_request_u.req_timeval.tv_usec =
                    req_timeval.tv_usec;
                break;

            case IOCTL_IFREQ:
                switch (in->code)
                {
                    case RPC_SIOCGIFFLAGS:
                    case RPC_SIOCSIFFLAGS:
                        out->req.req_val[0].ioctl_request_u.req_ifreq.
                            rpc_ifr_flags = if_fl_h2rpc(
                                (uint32_t)(unsigned short int)
                                    req_ifreq.ifr_flags);
                        break;

                    case RPC_SIOCGIFMTU:
                    case RPC_SIOCSIFMTU:
                        out->req.req_val[0].ioctl_request_u.req_ifreq.
                            rpc_ifr_mtu = req_ifreq.ifr_mtu;
                        break;

                    case RPC_SIOCGIFADDR:
                    case RPC_SIOCSIFADDR:
                    case RPC_SIOCGIFNETMASK:
                    case RPC_SIOCSIFNETMASK:
                    case RPC_SIOCGIFBRDADDR:
                    case RPC_SIOCSIFBRDADDR:
                    case RPC_SIOCGIFDSTADDR:
                    case RPC_SIOCSIFDSTADDR:
                    case RPC_SIOCGIFHWADDR:
                        sockaddr_h2rpc(&(req_ifreq.ifr_addr),
                                       &(out->req.req_val[0].
                                         ioctl_request_u.
                                         req_ifreq.rpc_ifr_addr));
                        break;

                    default:
                        ERROR("Unsupported IOCTL request %d of type IFREQ",
                              in->code);
                        out->common._errno = TE_RC(TE_TA_UNIX, TE_EINVAL);
                        goto finish;
                }
                break;

            case IOCTL_IFCONF:
            {
                struct ifreq       *req_c;
                struct tarpc_ifreq *req_t;

                int n = 1;
                int i;

                n = out->req.req_val[0].ioctl_request_u.req_ifconf.nmemb =
                    req_ifconf.ifc_len / sizeof(struct ifreq);
                out->req.req_val[0].ioctl_request_u.req_ifconf.extra =
                    req_ifconf.ifc_len % sizeof(struct ifreq);

                if (req_ifconf.ifc_req == NULL)
                    break;

                if ((req_t = calloc(n, sizeof(*req_t))) == NULL)
                {
                    free(req_ifconf.ifc_buf);
                    ERROR("Out of memory");
                    out->common._errno = TE_RC(TE_TA_UNIX, TE_ENOMEM);
                    goto finish;
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
                        free(req_ifconf.ifc_buf);
                        ERROR("Out of memory");
                        out->common._errno = TE_RC(TE_TA_UNIX, TE_ENOMEM);
                        goto finish;
                    }
                    memcpy(req_t->rpc_ifr_name.rpc_ifr_name_val,
                           req_c->ifr_name, sizeof(req_c->ifr_name));
                    req_t->rpc_ifr_name.rpc_ifr_name_len =
                        sizeof(req_c->ifr_name);
                    if ((req_t->rpc_ifr_addr.sa_data.sa_data_val =
                         calloc(1, sizeof(struct sockaddr) - SA_COMMON_LEN))
                         == NULL)
                    {
                        /* 
                         * Already allocated memory will be released 
                         * by RPC
                         */
                        free(req_ifconf.ifc_buf);
                        ERROR("Out of memory");
                        out->common._errno = TE_RC(TE_TA_UNIX, TE_ENOMEM);
                        goto finish;
                    }
                    req_t->rpc_ifr_addr.sa_data.sa_data_len =
                        sizeof(struct sockaddr) - SA_COMMON_LEN;
                    sockaddr_h2rpc(&(req_c->ifr_addr),
                                   &(req_t->rpc_ifr_addr));
                }
                free(req_ifconf.ifc_buf);
                break;
            }

            case IOCTL_ARPREQ:
            {
                if (in->code == RPC_SIOCGARP)
                {
                    /* Copy protocol address */
                    sockaddr_h2rpc(&(req_arpreq.arp_pa),
                                   &(out->req.req_val[0].ioctl_request_u.
                                   req_arpreq.rpc_arp_pa));
                    /* Copy HW address */
                    sockaddr_h2rpc(&(req_arpreq.arp_ha),
                                   &(out->req.req_val[0].ioctl_request_u.
                                   req_arpreq.rpc_arp_ha));

                    /* Copy flags */
                    out->req.req_val[0].ioctl_request_u.req_arpreq.
                        rpc_arp_flags = arp_fl_h2rpc(req_arpreq.arp_flags);
                }
                break;
            }

        }
    }
    finish:
    ;
}
)

#endif

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

TARPC_FUNC(sendmsg,
{
    if (in->msg.msg_val != NULL &&
        in->msg.msg_val[0].msg_iov.msg_iov_val != NULL &&
        in->msg.msg_val[0].msg_iov.msg_iov_len > RCF_RPC_MAX_IOVEC)
    {
        ERROR("Too long iovec is provided");
        out->common._errno = TE_RC(TE_TA_UNIX, TE_ENOMEM);
        return TRUE;
    }
},
{
    struct iovec iovec_arr[RCF_RPC_MAX_IOVEC];

    unsigned int i;

    memset(iovec_arr, 0, sizeof(iovec_arr));

    if (in->msg.msg_val == NULL)
    {
        MAKE_CALL(out->retval = func(in->s, NULL,
                                     send_recv_flags_rpc2h(in->flags)));
    }
    else
    {
        struct msghdr msg;

        struct tarpc_msghdr *rpc_msg = in->msg.msg_val;

        memset(&msg, 0, sizeof(msg));

        PREPARE_ADDR(rpc_msg->msg_name, 0);
        msg.msg_namelen = rpc_msg->msg_namelen;
        msg.msg_name = a;
        msg.msg_iovlen = rpc_msg->msg_iovlen;

        if (rpc_msg->msg_iov.msg_iov_val != NULL)
        {
            for (i = 0; i < rpc_msg->msg_iov.msg_iov_len; i++)
            {
                INIT_CHECKED_ARG(
                    rpc_msg->msg_iov.msg_iov_val[i].iov_base.iov_base_val,
                    rpc_msg->msg_iov.msg_iov_val[i].iov_base.iov_base_len,
                    0);
                iovec_arr[i].iov_base =
                    rpc_msg->msg_iov.msg_iov_val[i].iov_base.iov_base_val;
                iovec_arr[i].iov_len =
                    rpc_msg->msg_iov.msg_iov_val[i].iov_len;
            }
            msg.msg_iov = iovec_arr;
            INIT_CHECKED_ARG((char *)iovec_arr, sizeof(iovec_arr), 0);
        }
        
        if (rpc_msg->msg_control.msg_control_val != NULL)
        {
            struct cmsghdr *c;
            unsigned int    i;
            int             len = calculate_msg_controllen(rpc_msg);
            
            struct tarpc_cmsghdr *rpc_c = 
                rpc_msg->msg_control.msg_control_val;
            
            if ((msg.msg_control = calloc(1, len)) == NULL)
            {
                out->common._errno = TE_RC(TE_TA_UNIX, TE_ENOMEM);
                goto finish;
            }
            msg.msg_controllen = len;
            
            for (i = 0, c = CMSG_FIRSTHDR(&msg); 
                 i < rpc_msg->msg_control.msg_control_len; 
                 i++, c = CMSG_NXTHDR(&msg, c), rpc_c++)
            {
                c->cmsg_level = socklevel_rpc2h(rpc_c->level);
                c->cmsg_type = sockopt_rpc2h(rpc_c->type);
                c->cmsg_len = CMSG_LEN(rpc_c->data.data_len);
                if (rpc_c->data.data_val != NULL)
                    memcpy(CMSG_DATA(c), rpc_c->data.data_val, 
                           rpc_c->data.data_len);
            }
            
            INIT_CHECKED_ARG(msg.msg_control, msg.msg_controllen, 0);
        }
        
        msg.msg_flags = send_recv_flags_rpc2h(rpc_msg->msg_flags);
        INIT_CHECKED_ARG((char *)&msg, sizeof(msg), 0);

        VERB("sendmsg(): s=%d, msg=%s, flags=0x%x", in->s,
             msghdr2str(&msg), send_recv_flags_rpc2h(in->flags));
        MAKE_CALL(out->retval = func(in->s, &msg,
                                     send_recv_flags_rpc2h(in->flags)));
        free(msg.msg_control);
    }
    finish:
    ;
}
)

/*-------------- recvmsg() ------------------------------*/

TARPC_FUNC(recvmsg,
{
    if (in->msg.msg_val != NULL &&
        in->msg.msg_val[0].msg_iov.msg_iov_val != NULL &&
        in->msg.msg_val[0].msg_iov.msg_iov_len > RCF_RPC_MAX_IOVEC)
    {
        ERROR("Too long iovec is provided");
        out->common._errno = TE_RC(TE_TA_UNIX, TE_ENOMEM);
        return TRUE;
    }
    COPY_ARG(msg);
},
{
    struct iovec iovec_arr[RCF_RPC_MAX_IOVEC];

    unsigned int  i;
    struct msghdr msg;

    memset(iovec_arr, 0, sizeof(iovec_arr));
    memset(&msg, 0, sizeof(msg));
    
    if (out->msg.msg_val == NULL)
    {
        MAKE_CALL(out->retval = func(in->s, NULL,
                                     send_recv_flags_rpc2h(in->flags)));
    }
    else
    {
        struct tarpc_msghdr *rpc_msg = out->msg.msg_val;

        PREPARE_ADDR(rpc_msg->msg_name, rpc_msg->msg_namelen);
        msg.msg_namelen = rpc_msg->msg_namelen;
        msg.msg_name = a;

        msg.msg_iovlen = rpc_msg->msg_iovlen;
        if (rpc_msg->msg_iov.msg_iov_val != NULL)
        {
            for (i = 0; i < rpc_msg->msg_iov.msg_iov_len; i++)
            {
                INIT_CHECKED_ARG(
                    rpc_msg->msg_iov.msg_iov_val[i].iov_base.iov_base_val,
                    rpc_msg->msg_iov.msg_iov_val[i].iov_base.iov_base_len,
                    rpc_msg->msg_iov.msg_iov_val[i].iov_len);
                iovec_arr[i].iov_base =
                    rpc_msg->msg_iov.msg_iov_val[i].iov_base.iov_base_val;
                iovec_arr[i].iov_len =
                    rpc_msg->msg_iov.msg_iov_val[i].iov_len;
            }
            msg.msg_iov = iovec_arr;
            INIT_CHECKED_ARG((char *)iovec_arr, sizeof(iovec_arr), 0);
        }
        if (rpc_msg->msg_control.msg_control_val != NULL)
        {
            int len = calculate_msg_controllen(rpc_msg);
            int rlen = len * 2;
            int data_len = rpc_msg->msg_control.msg_control_val[0].
                           data.data_len;
            
            free(rpc_msg->msg_control.msg_control_val[0].data.data_val);
            free(rpc_msg->msg_control.msg_control_val);
            rpc_msg->msg_control.msg_control_val = NULL;
            rpc_msg->msg_control.msg_control_len = 0;
            
            msg.msg_controllen = len;
            if ((msg.msg_control = calloc(1, rlen)) == NULL)
            {
                out->common._errno = TE_RC(TE_TA_UNIX, TE_ENOMEM);
                goto finish;
            }
            CMSG_FIRSTHDR(&msg)->cmsg_len = CMSG_LEN(data_len);
            INIT_CHECKED_ARG((char *)(msg.msg_control), rlen, len);
        }
        msg.msg_flags = send_recv_flags_rpc2h(rpc_msg->msg_flags);

        /*
         * msg_name, msg_iov, msg_iovlen and msg_control MUST NOT be
         * changed.
         *
         * msg_namelen, msg_controllen and msg_flags MAY be changed.
         */
        INIT_CHECKED_ARG((char *)&msg.msg_name,
                         sizeof(msg.msg_name), 0);
        INIT_CHECKED_ARG((char *)&msg.msg_iov,
                         sizeof(msg.msg_iov), 0);
        INIT_CHECKED_ARG((char *)&msg.msg_iovlen,
                         sizeof(msg.msg_iovlen), 0);
        INIT_CHECKED_ARG((char *)&msg.msg_control,
                         sizeof(msg.msg_control), 0);

        VERB("recvmsg(): in msg=%s", msghdr2str(&msg));
        MAKE_CALL(out->retval = func(in->s, &msg,
                                     send_recv_flags_rpc2h(in->flags))
                  );
        VERB("recvmsg(): out msg=%s", msghdr2str(&msg));

        rpc_msg->msg_flags = send_recv_flags_h2rpc(msg.msg_flags);
        sockaddr_h2rpc(a, &(rpc_msg->msg_name));
        rpc_msg->msg_namelen = msg.msg_namelen;
        if (rpc_msg->msg_iov.msg_iov_val != NULL)
        {
            for (i = 0; i < rpc_msg->msg_iov.msg_iov_len; i++)
            {
                rpc_msg->msg_iov.msg_iov_val[i].iov_len =
                    iovec_arr[i].iov_len;
            }
        }

        if (msg.msg_control != NULL)
        {
            struct cmsghdr *c;
            int             i;

            struct tarpc_cmsghdr *rpc_c;
            
            /* Calculate number of elements to allocate an array */
            for (i = 0, c = CMSG_FIRSTHDR(&msg);
                 c != NULL;
                 i++, c = CMSG_NXTHDR(&msg, c));

            rpc_c = rpc_msg->msg_control.msg_control_val = 
                calloc(1, sizeof(*rpc_c) * i);
                
            if (rpc_msg->msg_control.msg_control_val == NULL)
            {
                out->common._errno = TE_RC(TE_TA_UNIX, TE_ENOMEM);
                goto finish;
            }
            /* Fill the array */
            for (i = 0, c = CMSG_FIRSTHDR(&msg); 
                 c != NULL;
                 i++, c = CMSG_NXTHDR(&msg, c))
            {
                char *data = CMSG_DATA(c);
                
                rpc_c->level = socklevel_h2rpc(c->cmsg_level);
                rpc_c->type = sockopt_h2rpc(c->cmsg_level, c->cmsg_type);
                if ((rpc_c->data.data_len = 
                         c->cmsg_len - (data - (char *)c)) > 0)
                {
                    rpc_c->data.data_val = malloc(rpc_c->data.data_len);
                    if (rpc_c->data.data_val == NULL)
                    {
                        for (i--, rpc_c--; i >= 0; i--, rpc_c--)
                            free(rpc_c->data.data_val);
                        free(rpc_msg->msg_control.msg_control_val);
                        rpc_msg->msg_control.msg_control_val = NULL;
                        
                        out->common._errno = TE_RC(TE_TA_UNIX, TE_ENOMEM);
                        goto finish;
                    }
                    memcpy(rpc_c->data.data_val, data, 
                           rpc_c->data.data_len);
                }
            }
            rpc_msg->msg_control.msg_control_len = i;
        }
    }
    finish:
    free(msg.msg_control);
}
)

/*-------------- poll() --------------------------------*/

TARPC_FUNC(poll,
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
        VERB("poll(): IN fd=%d events=%hd revents=%hd",
             ufds[i].fd, ufds[i].events, ufds[i].revents);
    }

    VERB("poll(): call with ufds=0x%lx, nfds=%u, timeout=%d",
         (unsigned long int)ufds, in->nfds, in->timeout);
    MAKE_CALL(out->retval = func_ptr(ufds, in->nfds, in->timeout));
    VERB("poll(): retval=%d", out->retval);

    for (i = 0; i < out->ufds.ufds_len; i++)
    {
        out->ufds.ufds_val[i].revents = poll_event_h2rpc(ufds[i].revents);
        VERB("poll(): OUT host-revents=%hd rpc-revents=%hd",
             ufds[i].revents, out->ufds.ufds_val[i].revents);
    }
}
)

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

    if (ai->ai_addr != NULL)
    {
        if ((ai_rpc->addr.sa_data.sa_data_val =
             calloc(1, ai_rpc->addrlen)) == NULL)
        {
            return -1;
        }
        ai_rpc->addr.sa_family = addr_family_h2rpc(ai->ai_addr->sa_family);
        memcpy(ai_rpc->addr.sa_data.sa_data_val, ai->ai_addr->sa_data,
               ai_rpc->addrlen);
        ai_rpc->addr.sa_data.sa_data_len = ai_rpc->addrlen;
    }

    if (ai->ai_canonname != NULL)
    {
        if ((ai_rpc->canonname.canonname_val =
             strdup(ai->ai_canonname)) == NULL)
        {
            free(ai_rpc->addr.sa_data.sa_data_val);
            return -1;
        }
        ai_rpc->canonname.canonname_len = strlen(ai->ai_canonname) + 1;
    }

    return 0;
}

TARPC_FUNC(getaddrinfo, {},
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
        a = sockaddr_rpc2h(&(in->hints.hints_val[0].addr), &addr);
        INIT_CHECKED_ARG((char *)a,
                         in->hints.hints_val[0].addr.sa_data.sa_data_len +
                         SA_COMMON_LEN, 0);
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
    /* I do not understand, which function is found by usual way */
    func = (api_func)getaddrinfo;
    MAKE_CALL(out->retval = func_ptr(in->node.node_val,
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
                        free(arr[k].addr.sa_data.sa_data_val);
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
TARPC_FUNC(freeaddrinfo, {},
{
    func = (api_func)freeaddrinfo;
    MAKE_CALL(func_ptr(rcf_pch_mem_get(in->mem_ptr)));
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

/*-------------- open() --------------------------------*/
TARPC_FUNC(open, {},
{
    MAKE_CALL(out->fd = func_ptr((in->path.path_len == 0) ? NULL :
                                     in->path.path_val,
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
                       in->out_fd ? &out->out_fd : NULL));
}
)

/*-------------- system() ----------------------------------*/
TARPC_FUNC(system, {},
{
    int             st;
    rpc_wait_status r_st;

    func = (api_func)ta_system;
    MAKE_CALL(st = func_ptr(in->cmd.cmd_val));
    r_st = wait_status_h2rpc(st);
    out->status_flag = r_st.flag;
    out->status_value = r_st.value;
}
)

/*-------------- getenv() --------------------------------*/
TARPC_FUNC(getenv, {},
{
    char *val;
    
    MAKE_CALL(val = func_ptr_ret_ptr(in->name));
    out->val = strdup(val == NULL ? "" : val);
}
)

/*-------------- setenv() --------------------------------*/
TARPC_FUNC(setenv, {},
{
    MAKE_CALL(out->retval = func_ptr(in->name, in->val, 
                                     (int)(in->overwrite)));
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
                goto finish;                                            \
            }                                                           \
            out->passwd._field._field##_len =                           \
                strlen(out->passwd._field._field##_val) + 1;            \
        } while (0)
TARPC_FUNC(getpwnam, {}, 
{ 
    struct passwd *pw;
    
    MAKE_CALL(pw = (struct passwd *)func_ptr_ret_ptr(in->name.name_val));
    
    if (pw != NULL)
    {
        PUT_STR(name);
        PUT_STR(passwd);
        out->passwd.uid = pw->pw_uid;
        out->passwd.gid = pw->pw_gid;
        PUT_STR(gecos);
        PUT_STR(dir);
        PUT_STR(shell);

    } 
    else
    {
        ERROR("getpwnam() returned NULL");
    }
finish:
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

/*-------------- getuid() --------------------------------*/
TARPC_FUNC(getuid, {}, { MAKE_CALL(out->uid = func_void()); })

/*-------------- getuid() --------------------------------*/
TARPC_FUNC(geteuid, {}, { MAKE_CALL(out->uid = func_void()); })

/*-------------- setuid() --------------------------------*/
TARPC_FUNC(setuid, {}, { MAKE_CALL(out->retval = func(in->uid)); })

/*-------------- seteuid() --------------------------------*/
TARPC_FUNC(seteuid, {}, { MAKE_CALL(out->retval = func(in->uid)); })

/*-------------- simple_sender() -----------------------------*/
TARPC_FUNC(simple_sender, {},
{
    MAKE_CALL(out->retval = func_ptr(in, out));
}
)

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
    api_func send_func;
    char         *buf;

    int size = rand_range(in->size_min, in->size_max);
    int delay = rand_range(in->delay_min, in->delay_max);

    time_t start;
    time_t now;

#ifdef TA_DEBUG
    uint64_t control = 0;
#endif

    out->bytes = 0;
    in->size_max = 1500;

    RING("%s() started", __FUNCTION__);

    if (in->size_min > in->size_max || in->delay_min > in->delay_max)
    {
        ERROR("Incorrect size or delay parameters");
        return -1;
    }
    
    if (tarpc_find_func(in->common.lib, "send", &send_func) != 0)
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

        if (delay / 1000000 > (int)(in->time2run) - (now - start) + 1)
            break;

        usleep(delay);
        
        len = send_func(in->s, buf, size, 0);

        if (len < 0)
        {
            if (!in->ignore_err)
            {
                ERROR("send() failed in simple_sender(): errno %x", errno);
                free(buf);
                return -1;
            }
            else
            {
                len = errno = 0;
                continue;
            }
        }
#if 0 /* it's a legal situation if len < size */
        if (len < size)
        {
            ERROR("send() returned %d instead %d in simple_sender()",
                  len, size);
            free(buf);
            return -1;
        }
#endif
        out->bytes += len;
    }

    RING("simple_sender() stopped, sent %llu bytes",
         out->bytes);
         
    free(buf);

    return 0;
}

/*--------------simple_receiver() --------------------------*/
TARPC_FUNC(simple_receiver, {},
{
    MAKE_CALL(out->retval = func_ptr(in, out));
}
)

#define MAX_PKT (1024 * 1024)

/**
 * Simple receiver.
 *
 * @param in                input RPC argument
 *
 * @return number of received bytes or -1 in the case of failure
 */
int
simple_receiver(tarpc_simple_receiver_in *in,
                tarpc_simple_receiver_out *out)
{
    api_func   select_func;
    api_func   recv_func;
    char           *buf;
    fd_set          set;
    int             rc;
    ssize_t         len;
    struct timeval  tv;

    time_t          start;
    time_t          now;

    out->bytes = 0;

    RING("%s() started", __FUNCTION__);

    if (tarpc_find_func(in->common.lib, "select", &select_func) != 0 ||
        tarpc_find_func(in->common.lib, "recv", &recv_func) != 0)
    {
        return -1;
    }
    
    if ((buf = malloc(MAX_PKT)) == NULL)
    {
        ERROR("Out of memory");
        return -1;
    }

    for (start = now = time(NULL);
         (in->time2run != 0) ?
          ((unsigned int)(now - start) <= in->time2run) : TRUE;
         now = time(NULL))
    {
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        FD_ZERO(&set);
        FD_SET(in->s, &set);

        rc = select_func(in->s + 1, &set, NULL, NULL, &tv);
        if (rc < 0)
        {
            ERROR("select() failed in simple_receiver(): errno %x", errno);
            free(buf);
            return -1;
        }
        else if (rc == 0)
        {
            if ((in->time2run != 0) || (out->bytes == 0))
                continue;
            else
                break;
        }
        else if (!FD_ISSET(in->s, &set))
        {
            ERROR("select() waited for reading on the socket, "
                  "returned %d, but the socket in not in set", rc);
            free(buf);
            return -1;
        }

        len = recv_func(in->s, buf, MAX_PKT, 0);
        if (len < 0)
        {
            ERROR("recv() failed in simple_receiver(): errno %x", errno);
            free(buf);
            return -1;
        }
        if (len == 0)
        {
            RING("recv() returned 0 in simple_receiver() because of "
                 "peer shutdown");
            break;
        }

        out->bytes += len;
    }

    free(buf);
    RING("simple_receiver() stopped, received %llu bytes",
         out->bytes);

    return 0;
}

#undef MAX_PKT

/*-------------- recv_verify() --------------------------*/
TARPC_FUNC(recv_verify, {},
{
    MAKE_CALL(out->retval = func_ptr(in, out));
}
)

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

    out->retval = 0;

    RING("%s() started", __FUNCTION__);

    if (tarpc_find_func(in->common.lib, "recv", &recv_func) != 0)
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
                errno = 0;
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


#define FLOODER_ECHOER_WAIT_FOR_RX_EMPTY        1
#define FLOODER_BUF                             4096

/*-------------- flooder() --------------------------*/
TARPC_FUNC(flooder, {},
{
    MAKE_CALL(out->retval = func_ptr(in));
    COPY_ARG(tx_stat);
    COPY_ARG(rx_stat);
}
)

typedef int (*flood_api_func)(struct pollfd *ufds,
                              unsigned int nfds, int timeout);

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
    api_func select_func;
    api_func pselect_func;
    api_func p_func;
    api_func write_func;
    api_func read_func;
    api_func send_func;
    api_func recv_func;
    api_func ioctl_func;

    flood_api_func poll_func;

    int        *rcvrs = in->rcvrs.rcvrs_val;
    int         rcvnum = in->rcvrs.rcvrs_len;
    int        *sndrs = in->sndrs.sndrs_val;
    int         sndnum = in->sndrs.sndrs_len;
    int         bulkszs = in->bulkszs;
    int         time2run = in->time2run;
    int         time2wait = in->time2wait;
    iomux_func  iomux = in->iomux;
    te_bool     rx_nb = in->rx_nonblock;

    uint64_t   *tx_stat = in->tx_stat.tx_stat_val;
    uint64_t   *rx_stat = in->rx_stat.rx_stat_val;

    int      i;
    int      rc;
    int      sent;
    int      received;
    char     rcv_buf[FLOODER_BUF];
    char     snd_buf[FLOODER_BUF];

    fd_set          rfds, wfds;
    struct pollfd   ufds[RPC_POLL_NFDS_MAX] = { {0, 0, 0}, };
    int             ufds_elements = (sndnum >= rcvnum) ? sndnum: rcvnum;
    int             max_descr = 0;

    struct timeval  timeout;
    struct timeval  timestamp;
    struct timeval  call_timeout;
    struct timespec ts;
    te_bool         time2run_not_expired = TRUE;
    te_bool         session_rx;

    INFO("%d flooder start", getpid());
    memset(rcv_buf, 0x0, FLOODER_BUF);
    memset(snd_buf, 'X', FLOODER_BUF);

    if ((tarpc_find_func(in->common.lib, "select", &select_func) != 0)   ||
        (tarpc_find_func(in->common.lib, "pselect", &pselect_func) != 0) ||
        (tarpc_find_func(in->common.lib, "poll", &p_func) != 0)          ||
        (tarpc_find_func(in->common.lib, "read", &read_func) != 0)       ||
        (tarpc_find_func(in->common.lib, "write", &write_func) != 0)     ||
        (tarpc_find_func(in->common.lib, "recv", &recv_func) != 0)       ||
        (tarpc_find_func(in->common.lib, "send", &send_func) != 0)       ||
        (tarpc_find_func(in->common.lib, "ioctl", &ioctl_func) != 0))
    {
        ERROR("failed to resolve function");
        return -1;
    }

    poll_func = (flood_api_func)p_func;

    if (rx_nb)
    {
        int on = 1;

        for (i = 0; i < rcvnum; ++i)
        {
            if ((ioctl_func(rcvrs[i], FIONBIO, &on)) != 0)
            {
                ERROR("%s(): ioctl(FIONBIO) failed: %d",
                      __FUNCTION__, errno);
                return -1;
            }
        }
    }

    /* Calculate max descriptor */
    for (i = 0; i < rcvnum; ++i)
    {
        if (rcvrs[i] > max_descr)
            max_descr = rcvrs[i];
    }
    for (i = 0; i < sndnum; ++i)
    {
        if (sndrs[i] > max_descr)
            max_descr = sndrs[i];
    }

    /*
     * FIXME
     * If 'b_array' does not contain all descriptors in 'l_array',
     * the last will be missing.
     */
    if (iomux == FUNC_POLL)
    {
        int  j;
        int *b_array = (sndnum >= rcvnum) ? sndrs: rcvrs;
        int  b_length = (sndnum >= rcvnum) ? sndnum: rcvnum;
        int  b_flag = (sndnum >= rcvnum) ? POLLOUT: POLLIN;
        int *l_array = (sndnum >= rcvnum) ? rcvrs: sndrs;
        int  l_length = (sndnum >= rcvnum) ? rcvnum: sndnum;
        int  l_flag = (sndnum >= rcvnum) ? POLLIN: POLLOUT;

        for (i = 0; i < b_length; i++)
        {
            ufds[i].fd = b_array[i];
            ufds[i].events = b_flag;
            for (j = 0; j < l_length; j ++)
            {
                if (ufds[i].fd == l_array[j])
                    ufds[i].events |= l_flag;
            }
        }
    }

    if (gettimeofday(&timeout, NULL))
    {
        ERROR("%s(): gettimeofday(timeout) failed: %d",
              __FUNCTION__, errno);
        return -1;
    }
    timeout.tv_sec += time2run;

    call_timeout.tv_sec = time2run;
    call_timeout.tv_usec = 0;

    INFO("%s(): time2run=%d, timeout=%ld.%06ld", __FUNCTION__,
         time2run, (long)timeout.tv_sec, (long)timeout.tv_usec);

    do {
        session_rx = FALSE;

        /* If select() or pselect() should be used as iomux function */
        if (iomux == FUNC_SELECT ||
            iomux == FUNC_PSELECT)
        {
            /* Prepare sets of file descriptors */
            FD_ZERO(&rfds);
            FD_ZERO(&wfds);
            for (i = 0; time2run_not_expired && (i < sndnum); ++i)
                FD_SET(sndrs[i], &wfds);
            for (i = 0; i < rcvnum; ++i)
                FD_SET(rcvrs[i], &rfds);

            if (iomux == FUNC_SELECT)
            {
                rc = select_func(max_descr + 1, &rfds,
                                 time2run_not_expired ? &wfds : NULL,
                                 NULL, &call_timeout);
            }
            else
            {
                ts.tv_sec  = call_timeout.tv_sec;
                ts.tv_nsec = call_timeout.tv_usec * 1000;
                rc = pselect_func(max_descr + 1, &rfds,
                                  time2run_not_expired ? &wfds : NULL,
                                  NULL, &ts, NULL);
            }
            if (rc < 0)
            {
                ERROR("%s(): (p)select() failed: %d", __FUNCTION__, errno);
                return -1;
            }

            /*
             * Send data to sockets that are ready for sending
             * if time is not expired.
             */
            if (time2run_not_expired && (rc > 0))
            {
                for (i = 0; i < sndnum; i++)
                {
                    if (FD_ISSET(sndrs[i], &wfds))
                    {
#ifdef USE_WRITE_FUNC
                        sent = write_func(sndrs[i], snd_buf, bulkszs);
#else
                        sent = send_func(sndrs[i], snd_buf, bulkszs,
                                         MSG_DONTWAIT);
#endif
                        if ((sent < 0) &&
                            (errno != EAGAIN) && (errno != EWOULDBLOCK))
                        {
                            ERROR("%s(): write() failed: %d",
                                  __FUNCTION__, errno);
                            return -1;
                        }
                        if ((sent > 0) && (tx_stat != NULL))
                        {
                            tx_stat[i] += sent;
                        }
                    }
                }
            }

            /* Receive data from sockets that are ready */
            for (i = 0; rc > 0 && i < rcvnum; i++)
            {
                if (FD_ISSET(rcvrs[i], &rfds))
                {
#ifdef USE_READ_FUNC
                    received = read_func(rcvrs[i], rcv_buf,
                                         sizeof(rcv_buf));
#else
                    received = recv_func(rcvrs[i], rcv_buf,
                                         sizeof(rcv_buf), MSG_DONTWAIT);
#endif
                    if ((received < 0) &&
                        (errno != EAGAIN) && (errno != EWOULDBLOCK))
                    {
                        ERROR("%s(): read() failed: %d",
                              __FUNCTION__, errno);
                        return -1;
                    }
                    if (received > 0)
                    {
                        session_rx = TRUE;
                        if (rx_stat != NULL)
                        {
                            rx_stat[i] += received;
                        }
                        if (!time2run_not_expired)
                            VERB("FD=%d Rx=%d", rcvrs[i], received);
                    }
                }
            }
        }
        else if (iomux == FUNC_POLL) /* poll() should be used as iomux */
        {
            rc = poll_func(ufds, ufds_elements,
                           call_timeout.tv_sec * 1000 +
                           call_timeout.tv_usec / 1000);

            if (rc < 0)
            {
                ERROR("%s(): poll() failed: %d", __FUNCTION__, errno);
                return -1;
            }

            for (i = 0; (rc > 0) && i < ufds_elements; i++)
            {
                if (time2run_not_expired && (ufds[i].revents & POLLOUT))
                {
                    sent = write_func(ufds[i].fd, snd_buf, bulkszs);
                    if ((sent < 0) &&
                        (errno != EAGAIN) && (errno != EWOULDBLOCK))
                    {
                        ERROR("%s(): write() failed: %d",
                              __FUNCTION__, errno);
                        return -1;
                    }
                    if ((sent > 0) && (tx_stat != NULL))
                    {
                        tx_stat[i] += sent;
                    }
                }
                if (ufds[i].revents & POLLIN)
                {
                    received = read_func(ufds[i].fd, rcv_buf,
                                         sizeof(rcv_buf));
                    if ((received < 0) &&
                        (errno != EAGAIN) && (errno != EWOULDBLOCK))
                    {
                        ERROR("%s(): read() failed: %d",
                              __FUNCTION__, errno);
                        return -1;
                    }
                    if (received > 0)
                    {
                        session_rx = TRUE;
                        if (rx_stat != NULL)
                        {
                            rx_stat[i] += received;
                        }
                        if (!time2run_not_expired)
                            VERB("FD=%d Rx=%d", ufds[i].fd, received);
                    }
                }
#ifdef DEBUG
                if ((!time2run_not_expired) &&
                    (ufds[i].revents & ~POLLIN))
                {
                    WARN("poll() returned unexpected events: 0x%x",
                         ufds[i].revents);
                }
#endif
            }
        }
        else
        {
            ERROR("%s(): unknown iomux() function", __FUNCTION__);
            return -1;
        }


        if (time2run_not_expired)
        {
            if (gettimeofday(&timestamp, NULL))
            {
                ERROR("%s(): gettimeofday(timestamp) failed): %d",
                      __FUNCTION__, errno);
                return -1;
            }
            call_timeout.tv_sec  = timeout.tv_sec  - timestamp.tv_sec;
            call_timeout.tv_usec = timeout.tv_usec - timestamp.tv_usec;
            if (call_timeout.tv_usec < 0)
            {
                --(call_timeout.tv_sec);
                call_timeout.tv_usec += 1000000;
#ifdef DEBUG
                if (call_timeout.tv_usec < 0)
                {
                    ERROR("Unexpected situation, assertation failed\n"
                          "%s:%d", __FILE__, __LINE__);
                }
#endif
            }
            if (call_timeout.tv_sec < 0)
            {
                time2run_not_expired = FALSE;
                /* Clean up POLLOUT requests for all descriptors */
                for (i = 0; i < ufds_elements; ++i)
                {
                    ufds[i].events &= ~POLLOUT;
                }
                /* Just to make sure that we'll get all from buffers */
                session_rx = TRUE;
                INFO("%s(): time2run expired", __FUNCTION__);
            }
#ifdef DEBUG
            else if (call_timeout.tv_sec < time2run)
            {
                VERB("%s(): timeout %ld.%06ld", __FUNCTION__,
                     call_timeout.tv_sec, call_timeout.tv_usec);
                time2run >>= 1;
            }
#endif
        }

        if (!time2run_not_expired)
        {
            call_timeout.tv_sec = time2wait;
            call_timeout.tv_usec = 0;
            VERB("%s(): Waiting for empty Rx queue, Rx=%d",
                 __FUNCTION__, session_rx);
        }

    } while (time2run_not_expired || session_rx);

    if (rx_nb)
    {
        int off = 0;

        for (i = 0; i < rcvnum; ++i)
        {
            if ((ioctl_func(rcvrs[i], FIONBIO, &off)) != 0)
            {
                ERROR("%s(): ioctl(FIONBIO) failed: %d",
                      __FUNCTION__, errno);
                return -1;
            }
        }
    }

    INFO("%s(): OK", __FUNCTION__);

    /* Clean up errno */
    errno = 0;

    return 0;
}

/*-------------- echoer() --------------------------*/
TARPC_FUNC(echoer, {},
{
    MAKE_CALL(out->retval = func_ptr(in));
    COPY_ARG(tx_stat);
    COPY_ARG(rx_stat);
}
)

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
    api_func select_func;
    api_func pselect_func;
    api_func p_func;
    api_func write_func;
    api_func read_func;

    flood_api_func poll_func;

    int        *sockets = in->sockets.sockets_val;
    int         socknum = in->sockets.sockets_len;
    int         time2run = in->time2run;
    iomux_func  iomux = in->iomux;

    uint64_t   *tx_stat = in->tx_stat.tx_stat_val;
    uint64_t   *rx_stat = in->rx_stat.rx_stat_val;

    int      i;
    int      rc;
    int      sent;
    int      received;
    char     buf[FLOODER_BUF];

    fd_set          rfds;
    struct pollfd   ufds[RPC_POLL_NFDS_MAX] = { {0, 0, 0}, };
    int             ufds_elements = socknum;
    int             max_descr = 0;

    struct timeval  timeout;
    struct timeval  timestamp;
    struct timeval  call_timeout;
    struct timespec ts;
    te_bool         time2run_not_expired = TRUE;
    te_bool         session_rx;


    memset(buf, 0x0, FLOODER_BUF);

    if ((tarpc_find_func(in->common.lib, "select", &select_func) != 0)   ||
        (tarpc_find_func(in->common.lib, "pselect", &pselect_func) != 0) ||
        (tarpc_find_func(in->common.lib, "poll", &p_func) != 0)          ||
        (tarpc_find_func(in->common.lib, "read", &read_func) != 0)       ||
        (tarpc_find_func(in->common.lib, "write", &write_func) != 0))
    {
        return -1;
    }

    poll_func = (flood_api_func)p_func;

    /* Calculate max descriptor */
    for (i = 0; i < socknum; i++)
    {
        if (sockets[i] > max_descr)
            max_descr = sockets[i];
    }

    if (iomux == FUNC_POLL)
    {
        for (i = 0; i < socknum; i++)
        {
            ufds[i].fd = sockets[i];
            ufds[i].events = POLLIN;
        }
    }

    if (gettimeofday(&timeout, NULL))
    {
        ERROR("%s(): gettimeofday(timeout) failed: %d",
              __FUNCTION__, errno);
        return -1;
    }
    timeout.tv_sec += time2run;

    call_timeout.tv_sec = time2run;
    call_timeout.tv_usec = 0;

    INFO("%s(): time2run=%d, timeout timestamp=%ld.%06ld", __FUNCTION__,
         time2run, (long)timeout.tv_sec, (long)timeout.tv_usec);

    do {
        session_rx = FALSE;

        /* If select() or pselect() should be used as iomux function */
        if (iomux == FUNC_SELECT ||
            iomux == FUNC_PSELECT)
        {
            /* Prepare sets of file descriptors */
            FD_ZERO(&rfds);
            for (i = 0; i < socknum; i++)
                FD_SET(sockets[i], &rfds);

            if (iomux == FUNC_SELECT)
            {
                rc = select_func(max_descr + 1, &rfds, NULL, NULL,
                                 &call_timeout);
            }
            else
            {
                ts.tv_sec  = call_timeout.tv_sec;
                ts.tv_nsec = call_timeout.tv_usec * 1000;
                rc = pselect_func(max_descr + 1, &rfds, NULL, NULL, &ts,
                                  NULL);
            }
            if (rc < 0)
            {
                ERROR("%s(): (p)select() failed: %d", __FUNCTION__, errno);
                return -1;
            }

            /* Receive data from sockets that are ready */
            for (i = 0; (rc > 0) && i < socknum; i++)
            {
                if (FD_ISSET(sockets[i], &rfds))
                {
                    received = read_func(sockets[i], buf, sizeof(buf));
                    if (received < 0)
                    {
                        ERROR("%s(): read() failed: %d",
                              __FUNCTION__, errno);
                        return -1;
                    }
                    if (rx_stat != NULL)
                        rx_stat[i] += received;
                    session_rx = TRUE;

                    sent = write_func(sockets[i], buf, received);
                    if (sent < 0)
                    {
                        ERROR("%s(): write() failed: %d",
                              __FUNCTION__, errno);
                        return -1;
                    }
                    if (tx_stat != NULL)
                        tx_stat[i] += sent;
                }
            }
        }
        else if (iomux == FUNC_POLL) /* poll() should be used as iomux */
        {
            rc = poll_func(ufds, ufds_elements,
                           call_timeout.tv_sec * 1000 +
                           call_timeout.tv_usec / 1000);

            if (rc < 0)
            {
                ERROR("%s(): poll() failed: %d", __FUNCTION__, errno);
                return -1;
            }

            for (i = 0; i < ufds_elements; i++)
            {
                if (ufds[i].revents & POLLIN)
                {
                    received = read_func(ufds[i].fd, buf, sizeof(buf));
                    if (received < 0)
                    {
                        ERROR("%s(): read() failed: %d",
                              __FUNCTION__, errno);
                        return -1;
                    }
                    if (rx_stat != NULL)
                        rx_stat[i] += received;
                    session_rx = TRUE;

                    sent = write_func(ufds[i].fd, buf, received);
                    if (sent < 0)
                    {
                        ERROR("%s(): write() failed: %d",
                              __FUNCTION__, errno);
                        return -1;
                    }
                    if (tx_stat != NULL)
                        tx_stat[i] += sent;
                }
            }
        }
        else
        {
            ERROR("%s(): unknown iomux() function", __FUNCTION__);
            return -1;
        }


        if (time2run_not_expired)
        {
            if (gettimeofday(&timestamp, NULL))
            {
                ERROR("%s(): gettimeofday(timestamp) failed: %d",
                      __FUNCTION__, errno);
                return -1;
            }
            call_timeout.tv_sec  = timeout.tv_sec  - timestamp.tv_sec;
            call_timeout.tv_usec = timeout.tv_usec - timestamp.tv_usec;
            if (call_timeout.tv_usec < 0)
            {
                --(call_timeout.tv_sec);
                call_timeout.tv_usec += 1000000;
#ifdef DEBUG
                if (call_timeout.tv_usec < 0)
                {
                    ERROR("Unexpected situation, assertation failed\n"
                          "%s:%d", __FILE__, __LINE__);
                }
#endif
            }
            if (call_timeout.tv_sec < 0)
            {
                time2run_not_expired = FALSE;
                /* Just to make sure that we'll get all from buffers */
                session_rx = TRUE;
                INFO("%s(): time2run expired", __FUNCTION__);
            }
#ifdef DEBUG
            else if (call_timeout.tv_sec < time2run)
            {
                VERB("%s(): timeout %ld.%06ld", __FUNCTION__,
                     call_timeout.tv_sec, call_timeout.tv_usec);
                time2run >>= 1;
            }
#endif
        }

        if (!time2run_not_expired)
        {
            call_timeout.tv_sec = FLOODER_ECHOER_WAIT_FOR_RX_EMPTY;
            call_timeout.tv_usec = 0;
            VERB("%s(): Waiting for empty Rx queue", __FUNCTION__);
        }

    } while (time2run_not_expired || session_rx);

    INFO("%s(): OK", __FUNCTION__);

    return 0;
}


/*-------------- sendfile() ------------------------------*/

TARPC_FUNC(sendfile,
{
    COPY_ARG(offset);
},
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
)

/*-------------- socket_to_file() ------------------------------*/
#define SOCK2FILE_BUF_LEN  4096

TARPC_FUNC(socket_to_file, {},
{
   MAKE_CALL(out->retval = func_ptr(in));
}
)

/**
 * Routine which receives data from socket and write data
 * to specified path.
 *
 * @return -1 in the case of failure or some positive value in other cases
 */
int
socket_to_file(tarpc_socket_to_file_in *in)
{
    api_func select_func;
    api_func write_func;
    api_func read_func;

    int      sock = in->sock;
    char    *path = in->path.path_val;
    long     time2run = in->timeout;

    int      rc = 0;
    int      file_d = -1;
    int      written;
    int      received;
    size_t   total = 0;
    char     buffer[SOCK2FILE_BUF_LEN];

    fd_set          rfds;

    struct timeval  timeout;
    struct timeval  timestamp;
    struct timeval  call_timeout;
    te_bool         time2run_not_expired = TRUE;
    te_bool         session_rx;

    path[in->path.path_len] = '\0';

    INFO("%s() called with: sock=%d, path=%s, timeout=%ld",
         __FUNCTION__, sock, path, time2run);

    if ((tarpc_find_func(in->common.lib, "select", &select_func) != 0)   ||
        (tarpc_find_func(in->common.lib, "read", &read_func) != 0)       ||
        (tarpc_find_func(in->common.lib, "write", &write_func) != 0))
    {
        ERROR("Failed to resolve functions addresses");
        rc = -1;
        goto local_exit;
    }

    file_d = open(path, O_CREAT | O_RDWR, S_IRWXU | S_IRWXG | S_IRWXO);
    if (file_d < 0)
    {
        ERROR("%s(): open(%s, O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO) "
              "failed: %d", __FUNCTION__, path, errno);
        rc = -1;
        goto local_exit;
    }
    INFO("%s(): file '%s' opened with descriptor=%d", __FUNCTION__,
         path, file_d);

    if (gettimeofday(&timeout, NULL))
    {
        ERROR("%s(): gettimeofday(timeout) failed: %d",
              __FUNCTION__, errno);
        rc = -1;
        goto local_exit;
    }
    timeout.tv_sec += time2run;

    call_timeout.tv_sec = time2run;
    call_timeout.tv_usec = 0;

    INFO("%s(): time2run=%ld, timeout timestamp=%ld.%06ld", __FUNCTION__,
         time2run, (long)timeout.tv_sec, (long)timeout.tv_usec);

    do {
        session_rx = FALSE;

        /* Prepare sets of file descriptors */
        FD_ZERO(&rfds);
        FD_SET(sock, &rfds);

        rc = select_func(sock + 1, &rfds, NULL, NULL, &call_timeout);
        if (rc < 0)
        {
            ERROR("%s(): select() failed: %d", __FUNCTION__, errno);
            break;
        }
        VERB("%s(): select finishes for waiting of events", __FUNCTION__);

        /* Receive data from socket that are ready */
        if (FD_ISSET(sock, &rfds))
        {
            VERB("%s(): select observes data for reading on the "
                 "socket=%d", __FUNCTION__, sock);
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
                written = write(file_d, buffer, received);
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

        if (time2run_not_expired)
        {
            if (gettimeofday(&timestamp, NULL))
            {
                ERROR("%s(): gettimeofday(timestamp) failed): %d",
                      __FUNCTION__, errno);
                rc = -1;
                break;
            }
            call_timeout.tv_sec  = timeout.tv_sec  - timestamp.tv_sec;
            call_timeout.tv_usec = timeout.tv_usec - timestamp.tv_usec;
            if (call_timeout.tv_usec < 0)
            {
                --(call_timeout.tv_sec);
                call_timeout.tv_usec += 1000000;
#ifdef DEBUG
                if (call_timeout.tv_usec < 0)
                {
                    ERROR("Unexpected situation, assertion failed\n"
                          "%s:%d", __FILE__, __LINE__);
                }
#endif
            }
            if (call_timeout.tv_sec < 0)
            {
                time2run_not_expired = FALSE;
                /* Just to make sure that we'll get all from buffers */
                session_rx = TRUE;
                INFO("%s(): time2run expired", __FUNCTION__);
            }
#ifdef DEBUG
            else if (call_timeout.tv_sec < time2run)
            {
                VERB("%s(): timeout %ld.%06ld", __FUNCTION__,
                     call_timeout.tv_sec, call_timeout.tv_usec);
                time2run >>= 1;
            }
#endif
        }

        if (!time2run_not_expired)
        {
            call_timeout.tv_sec = FLOODER_ECHOER_WAIT_FOR_RX_EMPTY;
            call_timeout.tv_usec = 0;
            VERB("%s(): Waiting for empty Rx queue, Rx=%d",
                 __FUNCTION__, session_rx);
        }

    } while (time2run_not_expired || session_rx);

local_exit:
    RING("Stop to get data from socket %d and put to file %s, %s, "
         "received %u", sock, path,
         (!time2run_not_expired) ? "timeout expired" :
                                   "unexpected failure",
         total);
    INFO("%s(): %s", __FUNCTION__, (rc == 0) ? "OK" : "FAILED");

    if (file_d != -1)
        close(file_d);

    /* Clean up errno */
    if (rc == 0)
    {
        errno = 0;
        rc = total;
    }
    return rc;
}

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

/*-------------- overfill_buffers() -----------------------------*/
TARPC_FUNC(overfill_buffers,{},
{
    MAKE_CALL(out->retval = func_ptr(in, out));
}
)

int
overfill_buffers(tarpc_overfill_buffers_in *in,
                 tarpc_overfill_buffers_out *out)
{
    ssize_t         rc = 0;
    int             errno_save = errno;
    api_func        send_func;
    api_func        select_func;
    size_t          max_len = 4096;
    uint8_t        *buf = NULL;
    uint64_t        total = 0;
    fd_set          writefds;
    struct timeval  tv;
    int             unchanged = 0;

    out->bytes = 0;

    buf = calloc(1, max_len);
    if (buf == NULL)
    {
        ERROR("%s(): Out of memory", __FUNCTION__);
        out->common._errno = TE_RC(TE_TA_UNIX, TE_ENOMEM);
        rc = -1;
        goto overfill_buffers_exit;
    }

    /* FIXME */
    memset(buf, 0xDEADBEEF, sizeof(max_len));

    if (tarpc_find_func(in->common.lib, "send", &send_func) != 0)
    {
        ERROR("%s(): Failed to resolve send() function", __FUNCTION__);
        rc = -1;
        goto overfill_buffers_exit;
    }

    if (tarpc_find_func(in->common.lib, "select", &select_func) != 0)
    {
        ERROR("%s(): Failed to resolve select() function", __FUNCTION__);
        rc = -1;
        goto overfill_buffers_exit;
    }

    /*
     * If total bytes is left unchanged after 3 attempts the socket
     * can be considered as not writable.
     */
    do {
        FD_ZERO(&writefds);
        FD_SET(in->sock, &writefds);
        tv.tv_sec  = 0;
        tv.tv_usec = 100 * 1000;
        rc = select_func(in->sock + 1, NULL, &writefds, NULL, &tv);
        if (rc < 0)
        {
            ERROR("%s(): select() failed", __FUNCTION__);
            out->common._errno = TE_OS_RC(TE_TA_UNIX, errno);
            goto overfill_buffers_exit;
        }

        rc = 0;
        do {
            out->bytes += rc;
            rc = send_func(in->sock, buf, max_len, MSG_DONTWAIT);
        } while (rc > 0);
        if (errno != EAGAIN)
        {
            ERROR("%s(): send() failed", __FUNCTION__);
            out->common._errno = TE_OS_RC(TE_TA_UNIX, errno);
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
            rc = 0;
        }
    } while (unchanged != 3);

overfill_buffers_exit:

    free(buf);
    if (rc == 0)
        errno = errno_save;
    return rc;
}

#ifdef HAVE_AIO_H

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
#ifdef HAVE_UNION_SIGVAL_SIVAL_PTR
        cb->aio_sigevent.sigev_value.sival_ptr = 
#elif defined(HAVE_UNION_SIGVAL_SIGVAL_PTR)
        cb->aio_sigevent.sigev_value.sigval_ptr = 
#else        
#error "Failed to discover memeber names of the union sigval."
#endif
            rcf_pch_mem_get(in->sigevent.value.tarpc_sigval_u.sival_ptr);
    }
    else
    {
#ifdef HAVE_UNION_SIGVAL_SIVAL_INT
        cb->aio_sigevent.sigev_value.sival_int = 
#elif defined(HAVE_UNION_SIGVAL_SIGVAL_INT)
        cb->aio_sigevent.sigev_value.sigval_int = 
#else        
#error "Failed to discover memeber names of the union sigval."
#endif
            in->sigevent.value.tarpc_sigval_u.sival_int;
    }
    
    cb->aio_sigevent.sigev_signo = signum_rpc2h(in->sigevent.signo);
    cb->aio_sigevent.sigev_notify = sigev_notify_rpc2h(in->sigevent.notify);
    if (strlen(in->sigevent.function) > 0)
    {
#ifdef SIGEV_THREAD    
        memset(&(cb->aio_sigevent._sigev_un), 0, 
               sizeof(cb->aio_sigevent._sigev_un));
        if ((cb->aio_sigevent._sigev_un._sigev_thread._function = 
             rcf_ch_symbol_addr(in->sigevent.function, 1)) == NULL)
        {
            WARN("Failed to find address of AIO callback %s - "
                 "use NULL callback", in->sigevent.function);
        }
#else
        out->common._errno = TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP);
#endif        
    }
    else
    {
#ifdef SIGEV_THREAD        
        memset(&(cb->aio_sigevent._sigev_un), 0, 
               sizeof(cb->aio_sigevent._sigev_un));
#endif            
    }

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
    
    MAKE_CALL(out->retval = func_ptr(cb, in->n, 
                                     in->timeout.timeout_len == 0 ? NULL 
                                                                  : &tv));
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
#ifdef HAVE_UNION_SIGVAL_SIVAL_PTR
            sig.sigev_value.sival_ptr = 
#elif defined(HAVE_UNION_SIGVAL_SIGVAL_PTR)
            sig.sigev_value.sigval_ptr = 
#else        
#error "Failed to discover memeber names of the union sigval."
#endif
                rcf_pch_mem_get(ev->value.tarpc_sigval_u.sival_ptr);
        }
        else
        {
#ifdef HAVE_UNION_SIGVAL_SIVAL_INT
            sig.sigev_value.sival_int = 
#elif defined(HAVE_UNION_SIGVAL_SIGVAL_INT)
            sig.sigev_value.sigval_int = 
#else        
#error "Failed to discover memeber names of the union sigval."
#endif
                ev->value.tarpc_sigval_u.sival_int;
        }

        sig.sigev_signo = signum_rpc2h(ev->signo);
        sig.sigev_notify = sigev_notify_rpc2h(ev->notify);
        if (strlen(ev->function) > 0)
        {
#ifdef SIGEV_THREAD        
            memset(&(sig._sigev_un), 0, sizeof(sig._sigev_un));
            if ((sig._sigev_un._sigev_thread._function = 
                     rcf_ch_symbol_addr(ev->function, 1)) == NULL)
            {
                WARN("Failed to find address of AIO callback %s - "
                     "use NULL callback", ev->function);
            }
#else
            out->common._errno = TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP);
#endif        
        }
        else
        {
#ifdef SIGEV_THREAD        
            memset(&(sig._sigev_un), 0, sizeof(sig._sigev_un));
#endif            
        }
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
    
    UNUSED(list);
    
    buf = func_ret_ptr(in->size);

    if (buf == NULL)
        out->common._errno = TE_RC(TE_TA_WIN32, TE_ENOMEM);
    else
        out->retval = rcf_pch_mem_alloc(buf);
}
)

/*--------------------------- free ---------------------------------*/
TARPC_FUNC(free, {},
{
    UNUSED(list);
    UNUSED(out);
    func_ptr(rcf_pch_mem_get(in->buf));
    rcf_pch_mem_free(in->buf);
}
)

/*-------------------------- Fill buffer ----------------------------*/
bool_t
_set_buf_1_svc(tarpc_set_buf_in *in, tarpc_set_buf_out *out,
               struct svc_req *rqstp)
{
    uint8_t *dst_buf;
    
    UNUSED(rqstp);
    UNUSED(out);
    
    dst_buf = rcf_pch_mem_get(in->dst_buf);
    if (dst_buf != NULL && in->src_buf.src_buf_len != 0)
    {
        memcpy(dst_buf + (unsigned int)in->offset,
               in->src_buf.src_buf_val, in->src_buf.src_buf_len);
    }
    
    return TRUE;
}

/*-------------------------- Read buffer ----------------------------*/
bool_t
_get_buf_1_svc(tarpc_get_buf_in *in, tarpc_get_buf_out *out,
               struct svc_req *rqstp)
{
    void *src_buf; 
    
    UNUSED(rqstp);

    src_buf = rcf_pch_mem_get(in->src_buf);
    if (src_buf != NULL && in->len != 0)
    {
        char *buf;

        buf = malloc(in->len);
        if (buf == NULL)
            out->common._errno = TE_RC(TE_TA_WIN32, TE_ENOMEM);
        else
        {
            memcpy(buf, src_buf + (unsigned int)in->offset, in->len);
            out->dst_buf.dst_buf_val = buf;
            out->dst_buf.dst_buf_len = in->len;
        }
    }
    else
    {
        out->dst_buf.dst_buf_val = NULL;
        out->dst_buf.dst_buf_len = 0;
    }
    
    return TRUE;
}

/*---------------------- Fill buffer by the pattern ----------------------*/
bool_t
_set_buf_pattern_1_svc(tarpc_set_buf_pattern_in *in, 
                       tarpc_set_buf_pattern_out *out,
                       struct svc_req *rqstp)
{
    uint8_t *dst_buf;
    
    UNUSED(rqstp);
    UNUSED(out);
    
    dst_buf = rcf_pch_mem_get(in->dst_buf) + (unsigned int)in->offset;
    if (dst_buf != NULL)
    {
        if (in->pattern < TAPI_RPC_BUF_RAND)
        {
            memset(dst_buf, in->pattern, in->len);
        }
        else
        {
            unsigned int i;
            
            for (i = 0; i < in->len; i++)
                dst_buf[i] = rand() % TAPI_RPC_BUF_RAND;
        }
    }
    
    return TRUE;
}
