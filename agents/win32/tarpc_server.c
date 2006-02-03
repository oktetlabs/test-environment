/** @file
 * @brief Windows Test Agent
 *
 * RPC server implementation 
 *
 * Copyright (C) 2005 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 * Copyright (c) 2005 Level5 Networks Corp.
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

#include "tarpc_server.h"

extern HINSTANCE ta_hinstance;

LPFN_CONNECTEX            pf_connect_ex;
LPFN_DISCONNECTEX         pf_disconnect_ex;
LPFN_ACCEPTEX             pf_accept_ex;
LPFN_GETACCEPTEXSOCKADDRS pf_get_accept_ex_sockaddrs;
LPFN_TRANSMITFILE         pf_transmit_file;
LPFN_WSARECVMSG           pf_wsa_recvmsg;

void 
wsa_func_handles_discover()
{
    GUID  guid_connect_ex = WSAID_CONNECTEX;
    GUID  guid_disconnect_ex = WSAID_DISCONNECTEX;
    GUID  guid_accept_ex = WSAID_ACCEPTEX;
    GUID  guid_get_accept_ex_sockaddrs = WSAID_GETACCEPTEXSOCKADDRS;
    GUID  guid_transmit_file = WSAID_TRANSMITFILE;
    GUID  guid_wsa_recvmsg = WSAID_WSARECVMSG;
    DWORD bytes_returned;
    int   s = socket(AF_INET, SOCK_STREAM, 
                     wsp_proto_rpc2h(RPC_SOCK_STREAM, RPC_IPPROTO_TCP));
    
#define DISCOVER_FUNC(_func, _func_cup) \
    do {                                                                \
        if (WSAIoctl(s, SIO_GET_EXTENSION_FUNCTION_POINTER,             \
                     (LPVOID)&guid_##_func, sizeof(GUID),               \
                     (LPVOID)&pf_##_func, sizeof(LPFN_##_func_cup),     \
                      &bytes_returned, NULL, NULL) == SOCKET_ERROR)     \
        {                                                               \
            int _errno = GetLastError();                                \
            PRINT("Cannot retrieve %s pointer via WSAIoctl();"          \
                 " errno %d", #_func, _errno);                          \
        }                                                               \
    } while (0)

    DISCOVER_FUNC(connect_ex, CONNECTEX);
    DISCOVER_FUNC(disconnect_ex, DISCONNECTEX);
    DISCOVER_FUNC(accept_ex, ACCEPTEX);
    DISCOVER_FUNC(get_accept_ex_sockaddrs, GETACCEPTEXSOCKADDRS);
    DISCOVER_FUNC(transmit_file, TRANSMITFILE);
    DISCOVER_FUNC(wsa_recvmsg, WSARECVMSG);
    
#undef DISCOVER_FUNC    
    
    closesocket(s);
}

/** Sleep in waitable state */
void 
sleep_waitable(int msec)
{
    SleepEx(msec, TRUE);
} 

bool_t
_setlibname_1_svc(tarpc_setlibname_in *in, tarpc_setlibname_out *out,
                 struct svc_req *rqstp)
{
    UNUSED(rqstp);
    UNUSED(in);
    memset(out, 0, sizeof(*out));
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
    {"char", sizeof(char)},
    {"short", sizeof(short)},
    {"int", sizeof(int)},
    {"long", sizeof(long)},
    {"long long", sizeof(long long)},
    {"size_t", sizeof(size_t)},
    {"socklen_t", sizeof(socklen_t)},
    {"struct timeval", sizeof(struct timeval)},
    {"struct linger", sizeof(struct linger)},
    {"struct ip_mreq", sizeof(struct ip_mreq)},
    {"struct sockaddr_storage", sizeof(struct sockaddr_storage)},
    {"struct sockaddr", sizeof(struct sockaddr)},
#if 0
    {"struct ip_mreqn", sizeof(struct ip_mreqn)}
#endif
};

/*-------------- get_sizeof() ---------------------------------*/
bool_t
_get_sizeof_1_svc(tarpc_get_sizeof_in *in, tarpc_get_sizeof_out *out,
                  struct svc_req *rqstp)
{
    uint32_t i;
    
    UNUSED(rqstp);    
    
    out->size = -1;
    for (i = 0; i < sizeof(type_info) / sizeof(type_info_t); i++)
    {
        if (strcmp(in->typename, type_info[i].type_name) == 0)
        {
            out->size = type_info[i].type_size;
        }
    }
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

/**
 * Create RPC server process using CreateProcess().
 *
 * @param name  RPC server name
 * @param pid   location for process identifier
 *
 * @return Status code
 */
te_errno
create_process_rpc_server(const char *name, int32_t *pid, te_bool inherit)
{
    char  cmdline[256];
    char *tmp;
    
    int i;
    
    PROCESS_INFORMATION info;
    STARTUPINFO         si;
    
    strcpy(cmdline, GetCommandLine());
    if ((tmp = strchr(cmdline, ' ')) != NULL)
        *tmp = 0;
    
    sprintf(cmdline + strlen(cmdline), "_rpcserver64 %s", name);    
    si.cb = sizeof(si);
    
    for (i = 0; i < 3; i++)
    {
        memset(&si, 0, sizeof(si));
        
        if (CreateProcess(NULL, cmdline, NULL, NULL, inherit, 0, NULL, NULL,
                          &si, &info))
            break;
           
       if (i == 2)
        {
            ERROR("CreateProcess() failed with error %d", GetLastError());
            return win_rpc_errno(GetLastError());
        }
        *tmp = 0;
        if (i == 0)
            sprintf(cmdline + strlen(cmdline), "_rpcserver %s", name);    
        else
            sprintf(cmdline + strlen(cmdline), " rpcserver %s", name);    
    }
    
    *pid = info.dwProcessId;
    
    return 0;
}

/*-------------- create_process() ---------------------------------*/
TARPC_FUNC(create_process, {}, 
{
    MAKE_CALL(out->common._errno = 
                  create_process_rpc_server(in->name.name_val, 
                                            &out->pid, TRUE));
}
)

/*-------------- thread_create() -----------------------------*/
TARPC_FUNC(thread_create, {},
{
    MAKE_CALL(out->common._errno = thread_create(rcf_pch_rpc_server,
                                                 strdup(in->name.name_val), 
                                                 &out->tid));
    out->retval = out->common._errno != 0 ? -1 : 0;
}
)


/*-------------- thread_cancel() -----------------------------*/
TARPC_FUNC(thread_cancel, {},
{
    MAKE_CALL(out->common._errno = thread_cancel(in->tid));
    out->retval = out->common._errno != 0 ? -1 : 0;
}
)

/*-------------- socket() ------------------------------*/

TARPC_FUNC(socket, {},
{
    MAKE_CALL(out->fd = socket(domain_rpc2h(in->domain),
                               socktype_rpc2h(in->type),
                               wsp_proto_rpc2h(in->type, in->proto)));
}
)

/*-------------- WSASocket() ------------------------------*/

TARPC_FUNC(wsa_socket, {},
{
    MAKE_CALL(out->fd = WSASocket(domain_rpc2h(in->domain),
                                  socktype_rpc2h(in->type),
                                  wsp_proto_rpc2h(in->type, in->proto),
                                  (LPWSAPROTOCOL_INFO)(in->info.info_val),
                                  0, 
                                  open_sock_flags_rpc2h(in->flags)));

}
)

/*-------------- CloseHandle() ------------------------------*/

TARPC_FUNC(close, {}, 
{ 
    MAKE_CALL(out->retval = CloseHandle((HANDLE)(in->fd)) ? 0 : -1); 
})

/*-------------- bind() ------------------------------*/

TARPC_FUNC(bind, {},
{
    PREPARE_ADDR(in->addr, 0);
    MAKE_CALL(out->retval = bind(in->fd, a, in->len));
}
)

/*-------------- connect() ------------------------------*/

TARPC_FUNC(connect, {},
{
    PREPARE_ADDR(in->addr, 0);

    MAKE_CALL(out->retval = connect(in->fd, a, in->len));
}
)

/*-------------- ConnectEx() ------------------------------*/
TARPC_FUNC(connect_ex,
{
    COPY_ARG(len_sent);
},
{
    PREPARE_ADDR(in->addr, 0);
    MAKE_CALL(out->retval = (*pf_connect_ex)(in->fd, a, in->len,
              rcf_pch_mem_get(in->send_buf),
              in->buflen,
              out->len_sent.len_sent_len == 0 ? NULL :
              (LPDWORD)(out->len_sent.len_sent_val),
              (LPWSAOVERLAPPED)IN_OVERLAPPED));
}
)

/*-------------- DisconnectEx() ------------------------------*/

TARPC_FUNC(disconnect_ex, {},
{
    LPWSAOVERLAPPED overlapped = NULL;

    if (IN_OVERLAPPED != NULL)
        overlapped = &(IN_OVERLAPPED->overlapped);
    MAKE_CALL(out->retval = (*pf_disconnect_ex)(in->fd, overlapped,
                                transmit_file_flags_rpc2h(in->flags), 0));
}
)

/*-------------- listen() ------------------------------*/

TARPC_FUNC(listen, {},
{
    MAKE_CALL(out->retval = listen(in->fd, in->backlog));
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

    MAKE_CALL(out->retval = accept(in->fd, a,
                                   out->len.len_len == 0 ? NULL :
                                   (int *)(out->len.len_val)));
    sockaddr_h2rpc(a, &(out->addr));
}
)

/*-------------- WSAAccept() ------------------------------*/

typedef struct accept_cond {
     unsigned short port;
     int            verdict;
} accept_cond;

static int CALLBACK
accept_callback(LPWSABUF caller_id, LPWSABUF caller_data, LPQOS sqos,
                LPQOS gqos, LPWSABUF callee_id, LPWSABUF callee_data,
                GROUP *g, DWORD_PTR user_data)
{
    accept_cond *cond = (accept_cond *)user_data;

    struct sockaddr_in *addr;

    UNUSED(caller_data);
    UNUSED(sqos);
    UNUSED(gqos);
    UNUSED(callee_id);
    UNUSED(callee_data);
    UNUSED(g);

    if (cond == NULL)
        return CF_ACCEPT;

    if (caller_id == NULL || caller_id->len == 0)
        return CF_REJECT;

    addr = (struct sockaddr_in *)(caller_id->buf);

    for (; cond->port != 0; cond++)
        if (cond->port == addr->sin_port)
               return cond->verdict;

    return CF_REJECT;
}

TARPC_FUNC(wsa_accept,
{
    COPY_ARG(len);
    COPY_ARG_ADDR(addr);
},
{
    accept_cond *cond = NULL;
    PREPARE_ADDR(out->addr, out->len.len_len == 0 ? 0 : *out->len.len_val);


    if (in->cond.cond_len != 0)
    {
        unsigned int i;

        /* FIXME: memory allocated here is lost */
        if ((cond = calloc(in->cond.cond_len + 1,
                           sizeof(accept_cond))) == NULL)
        {
            out->common._errno = TE_RC(TE_TA_WIN32, TE_ENOMEM);
            goto finish;
        }
        for (i = 0; i < in->cond.cond_len; i++)
        {
            cond[i].port = in->cond.cond_val[i].port;
            cond[i].verdict =
                in->cond.cond_val[i].verdict == TARPC_CF_ACCEPT ?
                    CF_ACCEPT :
                in->cond.cond_val[i].verdict == TARPC_CF_REJECT ?
                    CF_REJECT : CF_DEFER;
        }
    }

    MAKE_CALL(out->retval = WSAAccept(in->fd, a,
                                      out->len.len_len == 0 ? NULL :
                                      (int *)(out->len.len_val),
                                      (LPCONDITIONPROC)accept_callback,
                                      (DWORD)cond));
    sockaddr_h2rpc(a, &(out->addr));
    finish:
    ;
}
)

/*-------------- AcceptEx() ------------------------------*/

TARPC_FUNC(accept_ex,
{
    COPY_ARG(count);
},
{
    
    MAKE_CALL(out->retval =
              (*pf_accept_ex)(in->fd, in->fd_a,
                              rcf_pch_mem_get(in->out_buf), 
                              in->buflen,
                              in->laddr_len,
                              in->raddr_len,
                              out->count.count_len == 0 ? NULL :
                              (LPDWORD)out->count.count_val, 
                              (LPWSAOVERLAPPED)IN_OVERLAPPED)); 
    
    
}
)

/*-------------- GetAcceptExSockAddr() ---------------------------*/

TARPC_FUNC(get_accept_addr, 
{
    COPY_ARG(l_sa_len);
    COPY_ARG(r_sa_len);
},
{
    struct sockaddr *la = NULL;
    struct sockaddr *ra = NULL;

    UNUSED(list);

    (*pf_get_accept_ex_sockaddrs)(rcf_pch_mem_get(in->buf),
                                  in->buflen,
                                  in->laddr_len,
                                  in->raddr_len,
                                  in->l_sa_null ? NULL : &la, 
                                  (LPINT)out->l_sa_len.l_sa_len_val, 
                                  in->r_sa_null ? NULL : &ra, 
                                  (LPINT)out->r_sa_len.r_sa_len_val);

    if (!in->l_sa_null)
    {
        out->laddr.sa_data.sa_data_len = 
           sizeof(struct sockaddr_storage) - SA_COMMON_LEN;
        out->laddr.sa_data.sa_data_val = 
            calloc(sizeof(struct sockaddr_storage) - SA_COMMON_LEN, 1);
        sockaddr_h2rpc(la, &(out->laddr));
    }
    if (!in->r_sa_null)
    {
        out->raddr.sa_data.sa_data_len = 
           sizeof(struct sockaddr_storage) - SA_COMMON_LEN;
        out->raddr.sa_data.sa_data_val = 
            calloc(sizeof(struct sockaddr_storage) - SA_COMMON_LEN, 1);
        sockaddr_h2rpc(ra, &(out->raddr));
    }
}
)

/*-------------- TransmitFile() ----------------------------*/

TARPC_FUNC(transmit_file, {},
{
    TRANSMIT_FILE_BUFFERS  transmit_buffers;
    rpc_overlapped        *overlapped = IN_OVERLAPPED;

    memset(&transmit_buffers, 0, sizeof(transmit_buffers));
    if (in->head.head_len != 0)
         transmit_buffers.Head = in->head.head_val;
    transmit_buffers.HeadLength = in->head.head_len;
    if (in->tail.tail_len != 0)
         transmit_buffers.Tail = in->tail.tail_val;
    transmit_buffers.TailLength = in->tail.tail_len;

    if (overlapped != NULL)
    {
        rpc_overlapped_free_memory(overlapped);
        if ((overlapped->buffers = calloc(2, sizeof(WSABUF))) == NULL)
        {
            out->common._errno = TE_RC(TE_TA_WIN32, TE_ENOMEM);
            goto finish;
        }
        overlapped->buffers[0].buf = in->head.head_val;
        in->head.head_val = NULL;
        overlapped->buffers[0].len = in->head.head_len;
        in->head.head_len = 0;
        overlapped->buffers[1].buf = in->tail.tail_val;
        in->tail.tail_val = NULL;
        overlapped->buffers[1].len = in->tail.tail_len;
        in->tail.tail_len = 0;
        overlapped->bufnum = 2;
    }

    MAKE_CALL( out->retval = (*pf_transmit_file)(in->fd,
                              (HANDLE)(in->file),
                              in->len, in->len_per_send,
                              (LPWSAOVERLAPPED)overlapped,
                              &transmit_buffers,
                              transmit_file_flags_rpc2h(in->flags)); );
    finish:
    ;
}
)

/*----------- TransmitFile(), 2nd version ------------------*/

TARPC_FUNC(transmitfile_tabufs, {},
{
    TRANSMIT_FILE_BUFFERS  transmit_buffers;
    rpc_overlapped        *overlapped = IN_OVERLAPPED;

    memset(&transmit_buffers, 0, sizeof(transmit_buffers));
    transmit_buffers.Head = rcf_pch_mem_get(in->head);
    transmit_buffers.HeadLength = in->head_len;
    transmit_buffers.Tail = rcf_pch_mem_get(in->tail);
    transmit_buffers.TailLength = in->tail_len;

    if (overlapped != NULL)
    {
        rpc_overlapped_free_memory(overlapped);
        if ((overlapped->buffers = calloc(2, sizeof(WSABUF))) == NULL)
        {
            out->common._errno = TE_RC(TE_TA_WIN32, TE_ENOMEM);
            goto finish;
        }
        overlapped->buffers[0].buf = rcf_pch_mem_get(in->head);
        overlapped->buffers[0].len = in->head_len;
        overlapped->buffers[1].buf = rcf_pch_mem_get(in->tail);
        overlapped->buffers[1].len = in->tail_len;
        overlapped->bufnum = 2;
    }

    MAKE_CALL( out->retval = (*pf_transmit_file)(in->s,
                              (HANDLE)(in->file),
                              in->len, in->bytes_per_send,
                              (LPWSAOVERLAPPED)overlapped,
                              &transmit_buffers,
                              transmit_file_flags_rpc2h(in->flags)); );
    finish:
    ;
}
)

/*------------------------- CreateFile() -------------------------
 *------ Attention: not all flags currently are supported. ------*/
static inline DWORD 
cf_access_right_rpc2h(unsigned int ar)
{
    return (!!(ar & RPC_CF_GENERIC_EXECUTE) * GENERIC_EXECUTE) |
           (!!(ar & RPC_CF_GENERIC_READ) * GENERIC_READ) |
           (!!(ar & RPC_CF_GENERIC_WRITE) * GENERIC_WRITE);
}

static inline DWORD 
cf_share_mode_rpc2h(unsigned int sm)
{
    return (!!(sm & RPC_CF_FILE_SHARE_DELETE) * FILE_SHARE_DELETE) |
           (!!(sm & RPC_CF_FILE_SHARE_READ) * FILE_SHARE_READ) |
           (!!(sm & RPC_CF_FILE_SHARE_WRITE) * FILE_SHARE_WRITE);
}

static inline DWORD 
cf_creation_disposition_rpc2h(unsigned int cd)
{
    return (!!(cd & RPC_CF_CREATE_ALWAYS) * CREATE_ALWAYS) |
           (!!(cd & RPC_CF_CREATE_NEW) * CREATE_NEW) |
           (!!(cd & RPC_CF_OPEN_ALWAYS) * OPEN_ALWAYS) |
           (!!(cd & RPC_CF_OPEN_EXISTING) * OPEN_EXISTING) |
           (!!(cd & RPC_CF_TRUNCATE_EXISTING) * TRUNCATE_EXISTING);
}

static inline DWORD 
cf_flags_attributes_rpc2h(unsigned int fa)
{
    return (!!(fa & RPC_CF_FILE_ATTRIBUTE_NORMAL) * FILE_ATTRIBUTE_NORMAL);
}

TARPC_FUNC(create_file, {},
{
    MAKE_CALL(out->handle = (tarpc_handle)CreateFile(in->name.name_val,
        cf_access_right_rpc2h(in->desired_access),
        cf_share_mode_rpc2h(in->share_mode),
        (LPSECURITY_ATTRIBUTES)rcf_pch_mem_get(in->security_attributes),
        cf_creation_disposition_rpc2h(in->creation_disposition),
        cf_flags_attributes_rpc2h(in->flags_attributes),
        (HANDLE)(in->template_file))
    );
}
)

/*-------------- closesocket() --------------*/
TARPC_FUNC(closesocket, {},
{
    MAKE_CALL(out->retval = closesocket(in->s));
}
)

/*-------------- HasOverlappedIoCompleted() --------------*/
TARPC_FUNC(has_overlapped_io_completed, {},
{
    UNUSED(list);
    MAKE_CALL(out->retval =
              HasOverlappedIoCompleted((LPWSAOVERLAPPED)IN_OVERLAPPED));
}
)

/*-------------- CancelIo() --------------*/
TARPC_FUNC(cancel_io, {},
{
    UNUSED(list);
    MAKE_CALL(out->retval = CancelIo((HANDLE)(in->fd)));
}
)

/*-------------- GetCurrentProcessId() -------------------*/

TARPC_FUNC(get_current_process_id, {},
{
    UNUSED(in);
    UNUSED(list);
    UNUSED(in);
    out->retval = GetCurrentProcessId();
}
)

/* Get various system information */
TARPC_FUNC(get_sys_info, {},
{
    MEMORYSTATUS ms;
    SYSTEM_INFO si;

    UNUSED(in);
    UNUSED(list);

    memset(&ms, 0, sizeof(ms));
    GlobalMemoryStatus(&ms);
    out->ram_size = ms.dwTotalPhys;

    memset(&si, 0, sizeof(si));
    GetSystemInfo(&si);
    out->page_size = si.dwPageSize;
    out->number_of_processors = si.dwNumberOfProcessors;
}
)

/*-------------------- VM trasher --------------------*/

/* Lock */
static void *vm_trasher_lock;

/* Is set to TRUE when the VM trasher thread must exit */
static volatile te_bool vm_trasher_stop = FALSE;

/* VM trasher thread identifier */
static uint32_t vm_trasher_thread_id;

void *
vm_trasher_thread(void *arg)
{
    MEMORYSTATUS    ms;
    SIZE_T          len;
    SIZE_T          pos;
    double          dpos;
    char            *buf;
    struct timeval  tv;

    UNUSED(arg);

    memset(&ms, 0, sizeof(ms));
    GlobalMemoryStatus(&ms);

    len = ms.dwTotalPhys / 2 * 3; /* 1.5 RAM */
    buf = malloc(len);
    if (buf == NULL)
    {
        INFO("vm_trasher_thread() could not allocate "
             "%u bytes, errno = %d", len, errno);
        return (void *)-1;
    }

    /* Make dirty each page of buffer */
    for (pos = 0; pos < len; pos += 4096)
        buf[pos] = 0x5A;

    gettimeofday(&tv, NULL);
    srand(tv.tv_usec);
    
    /* Perform VM trashing to keep memory pressure */
    while (vm_trasher_stop == FALSE)
    {
        /* Choose a random page */
        dpos = (double)rand() / (double)RAND_MAX * (double)(len / 4096 - 1);
        /* Read and write a byte of the chosen page */
        buf[(SIZE_T)dpos * 4096] |= 0x5A;    
    }
    
    free(buf);
    return (void *)0;
}

TARPC_FUNC(vm_trasher, {},
{
    UNUSED(list);
    UNUSED(out);
    
    if (vm_trasher_lock == NULL)
        vm_trasher_lock = thread_mutex_create();

    thread_mutex_lock(vm_trasher_lock);

    if (in->start)
    {
        /* If the VM trasher thread is not started yet */
        if (vm_trasher_thread_id == 0)
        {
            /* Start the VM trasher thread */
            thread_create(vm_trasher_thread, NULL, &vm_trasher_thread_id);
        }
    }
    else
    {
        /* If the VM trasher thread is already started */
        if (vm_trasher_thread_id != 0)
        {
            int rc;
            
            /* Stop the VM trasher thread */
            vm_trasher_stop = TRUE;
            /* Wait for VM trasher thread exit */
            if ((rc = thread_join(vm_trasher_thread_id, NULL)) != 0)
            {
                INFO("vm_trasher: thread_join() failed %r", rc);
            }
            /* Allow another one VM trasher thread to start later */
            vm_trasher_stop = FALSE;
            vm_trasher_thread_id = 0;
        }
    }

    thread_mutex_unlock(vm_trasher_lock);
}
)

/*-------------- rpc_write_at_offset() -------------------*/

TARPC_FUNC(write_at_offset, {},
{
    MAKE_CALL(
        out->offset = SetFilePointer((HANDLE)(in->fd), in->offset, 
                                     NULL, FILE_BEGIN);
        if (out->offset != INVALID_SET_FILE_POINTER)
            WriteFile((HANDLE)(in->fd), in->buf.buf_val, 
                      in->buf.buf_len, &(out->written), NULL);
    );
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

    MAKE_CALL(out->retval = recvfrom(in->fd, out->buf.buf_val, in->len,
                                     send_recv_flags_rpc2h(in->flags), a,
                                     out->fromlen.fromlen_len == 0 ? NULL :
                                     (int *)(out->fromlen.fromlen_val)));
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

    MAKE_CALL(out->retval = recv(in->fd, out->buf.buf_val, in->len,
                                 send_recv_flags_rpc2h(in->flags)));
}
)

/*-------------- WSARecvEx() ------------------------------*/

TARPC_FUNC(wsa_recv_ex,
{
    COPY_ARG(buf);
    COPY_ARG(flags);
},
{
    INIT_CHECKED_ARG(out->buf.buf_val, out->buf.buf_len, in->len);
    if (out->flags.flags_len > 0)
        out->flags.flags_val[0] =
            send_recv_flags_rpc2h(out->flags.flags_val[0]);

    MAKE_CALL(out->retval = WSARecvEx(in->fd, in->len == 0 ? NULL :
                                      out->buf.buf_val,
                                      in->len,
                                      out->flags.flags_len == 0 ? NULL :
                                      (int *)(out->flags.flags_val)));

    if (out->flags.flags_len > 0)
        out->flags.flags_val[0] =
            send_recv_flags_h2rpc(out->flags.flags_val[0]);
}
)

/*-------------- read() ------------------------------*/

TARPC_FUNC(read,
{
    COPY_ARG(buf);
},
{
    DWORD rc;
    
    WSAOVERLAPPED overlapped;
    
    memset(&overlapped, 0, sizeof(overlapped));
    
    INIT_CHECKED_ARG(out->buf.buf_val, out->buf.buf_len, in->len);

    MAKE_CALL(out->retval = ReadFile((HANDLE)(in->fd), out->buf.buf_val, 
                                      in->len, &rc, &overlapped));

    if (out->retval == 0)
    {
        if (out->common._errno != RPC_E_IO_PENDING)
        {
            INFO("read(): ReadFile() failed with error %r (%d)", 
                  out->common._errno, GetLastError());
            rc = -1;
        }
        else if (GetOverlappedResult((HANDLE)(in->fd), &overlapped, 
                                     &rc, 1) == 0)
        {
            out->common._errno = win_rpc_errno(GetLastError());
            ERROR("read(): GetOverlappedResult() failed with error %r (%d)",
                  out->common._errno, GetLastError());
            rc = -1;
        }
    }
    out->retval = rc;
}
)

/*-------------- write() ------------------------------*/

TARPC_FUNC(write, {},
{
    DWORD rc;
    
    WSAOVERLAPPED overlapped;
    
    memset(&overlapped, 0, sizeof(overlapped));
    
    INIT_CHECKED_ARG(in->buf.buf_val, in->buf.buf_len, 0);

    MAKE_CALL(out->retval = WriteFile((HANDLE)(in->fd), in->buf.buf_val, 
                                      in->len, &rc, &overlapped));

    if (out->retval == 0)
    {
        if (out->common._errno != RPC_E_IO_PENDING)
        {
            INFO("write(): WriteFile() failed with error %r (%d)", 
                 out->common._errno, GetLastError());
            rc = -1;
        }
        else if (GetOverlappedResult((HANDLE)(in->fd), &overlapped, 
                                     &rc, 1) == 0)
        {
            out->common._errno = win_rpc_errno(GetLastError());
            ERROR("write(): GetOverlappedResult() failed with "
                 "error %r (%d)", out->common._errno, GetLastError());
            rc = -1;
        }
    }
    out->retval = rc;
}
)

/*-------------- ReadFile() ------------------------------*/

TARPC_FUNC(read_file,
{
    COPY_ARG(received);
    COPY_ARG(buf);
},
{
    rpc_overlapped *overlapped = IN_OVERLAPPED;
    rpc_overlapped  tmp;

    if (overlapped == NULL)
    {
        memset(&tmp, 0, sizeof(tmp));
        overlapped = &tmp;
    }
    
    if (buf2overlapped(overlapped, out->buf.buf_len, out->buf.buf_val) != 0)
    {
        out->common._errno = TE_RC(TE_TA_WIN32, TE_ENOMEM);
        goto finish;
    }
    
    MAKE_CALL(out->retval = ReadFile((HANDLE)(in->fd), 
                                     overlapped->buffers[0].buf,
                                     in->len, 
                                     (LPDWORD)(out->received.received_val),
                                     (LPWSAOVERLAPPED)overlapped));
                                     
    if (out->retval)
    {
        /* Non-overlapped operation */
        free(overlapped->buffers);
        overlapped->buffers = NULL;
        overlapped->bufnum = 0;
    }
    else if (out->common._errno != RPC_E_IO_PENDING)
    {
        /* Fatal error */
        rpc_overlapped_free_memory(overlapped);
    }
    else
    {
        /* 
         * Overlapped request is posted, let's avoid releasing of the
         * buffer by RPC.
         */
        out->buf.buf_val = NULL;
        out->buf.buf_len = 0;
    }
    finish:
    ;
}
)

/*-------------- WriteFile() ------------------------------*/

TARPC_FUNC(write_file, 
{
    COPY_ARG(sent);
},
{
    rpc_overlapped *overlapped = IN_OVERLAPPED;
    rpc_overlapped  tmp;

    if (overlapped == NULL)
    {
        memset(&tmp, 0, sizeof(tmp));
        overlapped = &tmp;
    }

    if (buf2overlapped(overlapped, in->buf.buf_len, in->buf.buf_val) != 0)
    {
        out->common._errno = TE_RC(TE_TA_WIN32, TE_ENOMEM);
        goto finish;
    }
    in->buf.buf_val = NULL;
    in->buf.buf_len = 0;

    MAKE_CALL(out->retval = WriteFile((HANDLE)(in->fd),
                                      overlapped->buffers[0].buf,
                                      in->len, 
                                      (LPDWORD)(out->sent.sent_val),
                                      (LPWSAOVERLAPPED)overlapped));
                        
    /* 
     * If the operation is not overlapped or fatal error occured,  
     * release memory.
     */
    if (out->retval || out->common._errno != RPC_E_IO_PENDING)
        rpc_overlapped_free_memory(overlapped);
        
    finish:
    ;
}
)


/*-------------- shutdown() ------------------------------*/

TARPC_FUNC(shutdown, {},
{
    MAKE_CALL(out->retval = shutdown(in->fd, shut_how_rpc2h(in->how)));
}
)

/*-------------- sendto() ------------------------------*/

TARPC_FUNC(sendto, {},
{
    PREPARE_ADDR(in->to, 0);

    INIT_CHECKED_ARG(in->buf.buf_val, in->buf.buf_len, in->len);

    MAKE_CALL(out->retval = sendto(in->fd, in->buf.buf_val, in->len,
                                   send_recv_flags_rpc2h(in->flags),
                                   a, in->tolen));
}
)

/*-------------- send() ------------------------------*/

TARPC_FUNC(send, {},
{
    INIT_CHECKED_ARG(in->buf.buf_val, in->buf.buf_len, in->len);

    MAKE_CALL(out->retval = send(in->fd, in->buf.buf_val, in->len,
                                 send_recv_flags_rpc2h(in->flags)));
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

    MAKE_CALL(out->retval = getsockname(in->fd, a,
                                        out->len.len_len == 0 ? NULL :
                                        (int *)(out->len.len_val)));
                                        
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

    MAKE_CALL(out->retval = getpeername(in->fd, a,
                                        out->len.len_len == 0 ? NULL :
                                        (int *)(out->len.len_val)));
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
    if ((set = (fd_set *)malloc(sizeof(fd_set))) == NULL)
    {
        out->common._errno = TE_RC(TE_TA_WIN32, TE_ENOMEM);
    }
    else
    {
        out->common._errno = win_rpc_errno(GetLastError());
        out->retval = rcf_pch_mem_alloc(set);
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
    out->common._errno = win_rpc_errno(GetLastError());

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

    FD_SET((unsigned int)in->fd, IN_FDSET);
    return TRUE;
}

/*-------------- FD_CLR --------------------------------*/

bool_t
_do_fd_clr_1_svc(tarpc_do_fd_clr_in *in, tarpc_do_fd_clr_out *out,
                 struct svc_req *rqstp)
{
    UNUSED(rqstp);

    memset(out, 0, sizeof(*out));

    FD_CLR((unsigned int)in->fd, IN_FDSET);
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
    {
        tv.tv_sec = out->timeout.timeout_val[0].tv_sec;
        tv.tv_usec = out->timeout.timeout_val[0].tv_usec;
    }

    MAKE_CALL(out->retval = select(in->n, 
                                   (fd_set *)rcf_pch_mem_get(in->readfds),
                                   (fd_set *)rcf_pch_mem_get(in->writefds),
                                   (fd_set *)rcf_pch_mem_get(in->exceptfds),
                                   out->timeout.timeout_len == 0 ? NULL :
                                   &tv));

    if (out->timeout.timeout_len > 0)
    {
        out->timeout.timeout_val[0].tv_sec = tv.tv_sec;
        out->timeout.timeout_val[0].tv_usec = tv.tv_usec;
    }
}
)

/*-------------- setsockopt() ------------------------------*/

TARPC_FUNC(setsockopt, {},
{
    if (in->optval.optval_val == NULL)
    {
        MAKE_CALL(out->retval = setsockopt(in->s,
                                           socklevel_rpc2h(in->level),
                                           sockopt_rpc2h(in->optname),
                                           NULL, in->optlen));
    }
    else
    {
        char *opt;
        int   optlen;

        struct linger   linger;
        struct in_addr  addr;
        struct timeval  tv;
        
        if (in->optname == RPC_SO_SNDTIMEO || 
            in->optname == RPC_SO_RCVTIMEO)
        {
            static int optval;
            
            optval = 
                in->optval.optval_val[0].option_value_u.
                opt_timeval.tv_sec * 1000 + 
                in->optval.optval_val[0].option_value_u.
                opt_timeval.tv_usec / 1000;
            
            opt = (char *)&optval;
            optlen = sizeof(int);
            goto call_setsockopt;
        }

        switch (in->optval.optval_val[0].opttype)
        {
            case OPT_INT:
            {
                opt = (char *)&(in->optval.optval_val[0].
                                option_value_u.opt_int);
                optlen = sizeof(int);
                break;
            }

            case OPT_LINGER:
            {
                opt = (char *)&linger;
                linger.l_onoff = in->optval.optval_val[0].
                                 option_value_u.opt_linger.l_onoff;
                linger.l_linger = in->optval.optval_val[0].
                                  option_value_u.opt_linger.l_linger;
                optlen = sizeof(linger);
                break;
            }

            case OPT_IPADDR:
            {
                opt = (char *)&addr;

                memcpy(&addr,
                       &(in->optval.optval_val[0].
                         option_value_u.opt_ipaddr),
                       sizeof(struct in_addr));
                optlen = sizeof(addr);
                break;
            }

            case OPT_TIMEVAL:
            {
                opt = (char *)&tv;

                tv.tv_sec = in->optval.optval_val[0].option_value_u.
                    opt_timeval.tv_sec;
                tv.tv_usec = in->optval.optval_val[0].option_value_u.
                    opt_timeval.tv_usec;
                optlen = sizeof(tv);
                break;
            }

            case OPT_STRING:
            {
                opt = (char *)in->optval.optval_val[0].option_value_u.
                    opt_string.opt_string_val;
                optlen = in->optval.optval_val[0].option_value_u.
                    opt_string.opt_string_len;
                break;
            }

            default:
                ERROR("incorrect option type %d is received",
                      in->optval.optval_val[0].opttype);
                out->common._errno = TE_RC(TE_TA_WIN32, TE_EINVAL);
                out->retval = -1;
                goto finish;
                break;
        }
        call_setsockopt:
        INIT_CHECKED_ARG(opt, optlen, 0);
        if (in->optlen == RPC_OPTLEN_AUTO)
            in->optlen = optlen;
        MAKE_CALL(out->retval = setsockopt(in->s,
                                           socklevel_rpc2h(in->level),
                                           sockopt_rpc2h(in->optname),
                                           opt, in->optlen));
    }
    finish:
    ;
}
)

/*-------------- getsockopt() ------------------------------*/

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
        MAKE_CALL(out->retval = 
                      getsockopt(in->s, socklevel_rpc2h(in->level),
                      sockopt_rpc2h(in->optname),
                      NULL, (int *)(out->optlen.optlen_val)));
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
                    optlen_in = optlen_out = sizeof(int);
                    break;
                    
                case OPT_LINGER:
                    optlen_in = optlen_out = sizeof(struct linger);
                    break;

                case OPT_IPADDR:
                    optlen_in = optlen_out = sizeof(struct in_addr);
                    break;

                case OPT_TIMEVAL:
                    optlen_in = optlen_out = sizeof(struct timeval);
                    break;

                default:
                    ERROR("incorrect option type %d is received",
                          out->optval.optval_val[0].opttype);
                    break;
            }
        }
        else if (out->optlen.optlen_val)
            optlen_in = optlen_out = *(out->optlen.optlen_val);

        memset(opt, 0, sizeof(opt));
        INIT_CHECKED_ARG(opt, sizeof(opt), optlen_in);

        MAKE_CALL(out->retval = 
                      getsockopt(in->s, socklevel_rpc2h(in->level),
                                 sockopt_rpc2h(in->optname),
                                 opt, out->optlen.optlen_val == NULL ?
                                      NULL : &optlen_out));
                                 
        if (optlen_in != optlen_out)                          
            *(out->optlen.optlen_val) = optlen_out;

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
                    *(int *)opt = win_rpc_errno(*(int *)opt);
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

            case OPT_IPADDR:
            {
                struct in_addr *addr = (struct in_addr *)opt;

                memcpy(&(out->optval.optval_val[0].
                         option_value_u.opt_ipaddr),
                       addr, sizeof(*addr));
                break;
            }

            case OPT_TIMEVAL:
            {
                if (in->optname == RPC_SO_SNDTIMEO || 
                    in->optname == RPC_SO_RCVTIMEO)
                {
                    int msec = *(int *)opt;
                    
                    out->optval.optval_val[0].option_value_u.
                        opt_timeval.tv_sec = msec / 1000;
                    out->optval.optval_val[0].option_value_u.
                        opt_timeval.tv_usec = (msec % 1000) * 1000;
                }
                else
                {
                    struct timeval *tv = (struct timeval *)opt;
                
                    out->optval.optval_val[0].option_value_u.
                        opt_timeval.tv_sec = tv->tv_sec;
                    out->optval.optval_val[0].option_value_u.
                        opt_timeval.tv_usec = tv->tv_usec;
                }
                break;
            }

            case OPT_STRING:
            {
                char *str = (char *)opt;

                memcpy(out->optval.optval_val[0].option_value_u.opt_string.
                       opt_string_val, str,
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

TARPC_FUNC(ioctl,
{
    COPY_ARG(req);
},
{
    char *req = NULL;
    int   reqlen = 0;

    static struct timeval req_timeval;
    static int            req_int;

    DWORD ret_num;

    if (out->req.req_val != NULL)
    {
        switch(out->req.req_val[0].type)
        {
            case IOCTL_TIMEVAL:
            {
                req = (char *)&req_timeval;
                reqlen = sizeof(struct timeval);
                req_timeval.tv_sec =
                    out->req.req_val[0].ioctl_request_u.req_timeval.tv_sec;
                req_timeval.tv_usec =
                    out->req.req_val[0].ioctl_request_u.req_timeval.tv_usec;

                break;
            }

            case IOCTL_INT:
            {
                req = (char *)&req_int;
                req_int = out->req.req_val[0].ioctl_request_u.req_int;
                reqlen = sizeof(int);
                break;
            }

            default:
                ERROR("incorrect ioctl request type %d is received",
                      out->req.req_val[0].type);
                out->common._errno = TE_RC(TE_TA_WIN32, TE_EINVAL);
                out->retval = -1;
                goto finish;
                break;
        }
    }

    if (in->access == IOCTL_WR)
        INIT_CHECKED_ARG(req, reqlen, 0);
    MAKE_CALL(out->retval = ioctlsocket(in->s, ioctl_rpc2h(in->code),
                                        (u_long *) req));
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

           default: /* Should not occur */
               break;
        }
    }
    finish:
    ;
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
                 malloc(rpc_he->h_length)) == NULL)
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

    MAKE_CALL(he = (struct hostent *)gethostbyname(in->name.name_val));
    if (he != NULL)
    {
        if ((out->res.res_val = hostent_h2rpc(he)) == NULL)
            out->common._errno = TE_RC(TE_TA_WIN32, TE_ENOMEM);
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

    MAKE_CALL(he = (struct hostent *)gethostbyaddr(in->addr.val.val_val,
                                         in->addr.val.val_len,
                                          addr_family_rpc2h(in->type)));
    if (he != NULL)
    {
        if ((out->res.res_val = hostent_h2rpc(he)) == NULL)
            out->common._errno = TE_RC(TE_TA_WIN32, TE_ENOMEM);
        else
            out->res.res_len = 1;
    }
}
)

/*-------------- simple_sender() -----------------------------*/
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
    char buf[1024];

    uint64_t sent = 0;

    int size = rand_range(in->size_min, in->size_max);
    int delay = rand_range(in->delay_min, in->delay_max);

    time_t start;
    time_t now;
#ifdef TA_DEBUG
    uint64_t control = 0;
#endif

    if (in->size_max > (int)sizeof(buf) || in->size_min > in->size_max ||
        in->delay_min > in->delay_max)
    {
        ERROR("Incorrect size of delay parameters");
        return -1;
    }

    memset(buf, 0xAB, sizeof(buf));

    for (start = now = time(NULL);
         now - start <= (time_t)in->time2run;
         now = time(NULL))
    {
        int len;

        if (!in->size_rnd_once)
            size = rand_range(in->size_min, in->size_max);

        if (!in->delay_rnd_once)
            delay = rand_range(in->delay_min, in->delay_max);

        if (delay / 1000000 > (time_t)in->time2run - (now - start) + 1)
            break;

        SleepEx(delay / 1000 + 1, TRUE);

        len = send(in->s, buf, size, 0);
    
        if (len < 0)
        {
            int err = 0;
            if (in->ignore_err)
                continue;
            err = GetLastError();
            ERROR("send() failed in simple_sender(): errno %d", err);
            return -1;
        }

        if (len < size)
        {
            if (in->ignore_err)
                continue;
            ERROR("send() returned %d instead %d in simple_sender()",
                  len, size);
            return -1;
        }

        sent += len;
#ifdef TA_DEBUG
        control += len;
        if (control > 0x20000)
        {
            char buf[128];

            sprintf(buf,
                    "echo \"Intermediate %llu time %d %d %d\" >> "
                    "/tmp/sender",
                    sent, now, start, in->time2run);
            system(buf);
            control = 0;
        }
#endif
    }

    {
        char buf[64] = {0,};
        sprintf(buf, "Sent %llu", sent);
        RING(buf);
    }

    out->bytes = sent;

    return 0;
}

TARPC_FUNC(simple_sender, {},
{
    MAKE_CALL(out->retval = simple_sender(in, out));
}
)

/*-------------- simple_receiver() --------------------------*/

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
    char *buf;

    uint64_t received = 0;
#ifdef TA_DEBUG
    uint64_t control = 0;
    int start = time(NULL);
#endif

    if ((buf = malloc(MAX_PKT)) == NULL)
    {
        ERROR("Out of memory");
        return -1;
    }

    while (TRUE)
    {
        struct timeval tv = { 1, 0 };
        fd_set         set;
        int            len;

        FD_ZERO(&set);
        FD_SET((unsigned int)in->s, &set);
        if (select(in->s + 1, &set, NULL, NULL, &tv) < 0)
        {
            int err = 0;
            
            err = GetLastError();
            ERROR("select() failed in simple_receiver(): errno %d", err);
            free(buf);
            return -1;
        }
        if (!FD_ISSET(in->s, &set))
        {
            if (received > 0)
                break;
            else
                continue;
        }

        len = recv(in->s, buf, MAX_PKT, 0);

        if (len < 0)
        {
            int err = 0;
            
            err = GetLastError();
            ERROR("recv() failed in simple_receiver(): errno %d", err);
            free(buf);
            return -1;
        }

        if (len == 0)
        {
            ERROR("recv() returned 0 in simple_receiver()");
            free(buf);
            return -1;
        }

        received += len;
#ifdef TA_DEBUG
        control += len;
        if (control > 0x20000)
        {
            char buf[128];
            sprintf(buf,
                    "echo \"Intermediate %llu time %d\" >> /tmp/receiver",
                    received, time(NULL) - start);
            system(buf);
            control = 0;
        }
#endif
    }

    RING("Received %llu", received);
    free(buf);

    out->bytes = received;

    return 0;
}

TARPC_FUNC(simple_receiver, {},
{
    MAKE_CALL(out->retval = simple_receiver(in, out));
}
)

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
 * @param iomux     - type of I/O Multiplexing function
 *                    (@b select(), @b pselect(), @b poll())
 *
 * @return 0 on success or -1 in the case of failure
 */
int
flooder(tarpc_flooder_in *in)
{
    int      *rcvrs = (int *)(in->rcvrs.rcvrs_val);
    int       rcvnum = in->rcvrs.rcvrs_len;
    int      *sndrs = (int *)(in->sndrs.sndrs_val);
    int       sndnum = in->sndrs.sndrs_len;
    int       bulkszs = in->bulkszs;
    int       time2run = in->time2run;
    te_bool   rx_nb = in->rx_nonblock;

    uint32_t *tx_stat = (uint32_t *)(in->tx_stat.tx_stat_val);
    uint32_t *rx_stat = (uint32_t *)(in->rx_stat.rx_stat_val);

    int      i;
    int      rc, err;
    int      sent;
    int      received;
    char     rcv_buf[FLOODER_BUF];
    char     snd_buf[FLOODER_BUF];

    fd_set          rfds0, wfds0;
    fd_set          rfds, wfds;
    int             max_descr = 0;

    struct timeval  timeout;
    struct timeval  timestamp;
    struct timeval  call_timeout;
    te_bool         time2run_not_expired = TRUE;
    te_bool         session_rx = TRUE;

    memset(rcv_buf, 0x0, FLOODER_BUF);
    memset(snd_buf, 0xA, FLOODER_BUF);
    if (rx_nb)
    {
        u_long on = 1;

        for (i = 0; i < rcvnum; ++i)
        {
            if ((ioctlsocket(rcvrs[i], FIONBIO, &on)) != 0)
            {
                ERROR("%s(): ioctl(FIONBIO) failed: %d",
                      __FUNCTION__, errno);
                return -1;
            }
        }
    }

    /* Calculate max descriptor */
    FD_ZERO(&rfds0);
    FD_ZERO(&wfds0);
    for (i = 0; i < rcvnum; ++i)
    {
        if (rcvrs[i] > max_descr)
            max_descr = rcvrs[i];
        FD_SET((unsigned int)rcvrs[i], &rfds0);
    }
    for (i = 0; i < sndnum; ++i)
    {
        if (sndrs[i] > max_descr)
            max_descr = sndrs[i];
        FD_SET((unsigned int)sndrs[i], &wfds0);
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
        if (time2run_not_expired)
        {
            wfds = wfds0;
        }
        else
        {
            FD_ZERO(&wfds);
            session_rx = FALSE;
        }
        rfds = rfds0;

        if ((rc = select(max_descr + 1, &rfds,
                         time2run_not_expired ? &wfds : NULL,
                         NULL, &call_timeout)) < 0)
        {
            ERROR("%s(): (p)select() failed: %d", __FUNCTION__, 
                  GetLastError());
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
                    sent = send(sndrs[i], snd_buf, bulkszs, 0);
                    err = GetLastError();
                    if (sent < 0 && err != WSAEWOULDBLOCK)
                    {
                        ERROR("%s(): write() failed: %d",
                              __FUNCTION__, err);
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
                received = recv(rcvrs[i], rcv_buf, sizeof(rcv_buf), 0);
                err = GetLastError();
                if (received < 0 && err != WSAEWOULDBLOCK)
                {
                    ERROR("%s(): read() failed: %d", __FUNCTION__, err);
                    return -1;
                }
                if (received > 0)
                {
                    if (rx_stat != NULL)
                        rx_stat[i] += received;
                    if (!time2run_not_expired)
                        VERB("FD=%d Rx=%d", rcvrs[i], received);
                    session_rx = TRUE;
                }
            }
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
            }
            if (call_timeout.tv_sec < 0)
            {
                time2run_not_expired = FALSE;
                INFO("%s(): time2run expired", __FUNCTION__);
            }
        }

        if (!time2run_not_expired)
        {
            call_timeout.tv_sec = FLOODER_ECHOER_WAIT_FOR_RX_EMPTY;
            call_timeout.tv_usec = 0;
        }
    } while (time2run_not_expired || session_rx);

    if (rx_nb)
    {
        u_long off = 0;

        for (i = 0; i < rcvnum; ++i)
        {
            if ((ioctlsocket((unsigned int)rcvrs[i], FIONBIO, &off)) != 0)
            {
                ERROR("%s(): ioctl(FIONBIO) failed: %d",
                      __FUNCTION__, GetLastError());
                return -1;
            }
        }
    }

    INFO("%s(): OK", __FUNCTION__);

    /* Clean up errno */
    errno = 0;

    return 0;
}

/*-------------- flooder() --------------------------*/
TARPC_FUNC(flooder, {},
{
    if (in->iomux != FUNC_SELECT)
    {
       ERROR("Unsipported iomux type for flooder");
       out->retval = TE_RC(TE_TA_WIN32, TE_EOPNOTSUPP);
       return 0;
    }
    MAKE_CALL(out->retval = flooder(in));
    COPY_ARG(tx_stat);
    COPY_ARG(rx_stat);
}
)

/*-------------- echoer() --------------------------*/

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
    int *sockets = (int *)(in->sockets.sockets_val);
    int  socknum = in->sockets.sockets_len;
    int  time2run = in->time2run;

    uint32_t *tx_stat = (uint32_t *)(in->tx_stat.tx_stat_val);
    uint32_t *rx_stat = (uint32_t *)(in->rx_stat.rx_stat_val);

    int      i;
    int      sent;
    int      received;
    char     buf[FLOODER_BUF];

    fd_set          rfds;
    int             max_descr = 0;

    struct timeval  timeout;
    struct timeval  timestamp;
    struct timeval  call_timeout;
    te_bool         time2run_not_expired = TRUE;
    te_bool         session_rx;

    memset(buf, 0x0, FLOODER_BUF);


    /* Calculate max descriptor */
    for (i = 0; i < socknum; i++)
    {
        if (sockets[i] > max_descr)
            max_descr = sockets[i];
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

        /* Prepare sets of file descriptors */
        FD_ZERO(&rfds);
        for (i = 0; i < socknum; i++)
            FD_SET((unsigned int)sockets[i], &rfds);

        if (select(max_descr + 1, &rfds, NULL, NULL, &call_timeout) < 0)
        {
            ERROR("%s(): select() failed: %d", __FUNCTION__, errno);
            return -1;
        }

        /* Receive data from sockets that are ready */
        for (i = 0; i < socknum; i++)
        {
            if (FD_ISSET(sockets[i], &rfds))
            {
                received = recv(sockets[i], buf, sizeof(buf), 0);
                if (received < 0)
                {
                    ERROR("%s(): read() failed: %d", __FUNCTION__, errno);
                    return -1;
                }
                if (rx_stat != NULL)
                    rx_stat[i] += received;
                session_rx = TRUE;

                sent = send(sockets[i], buf, received, 0);
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

TARPC_FUNC(echoer, {},
{
    if (in->iomux != FUNC_SELECT)
    {
       ERROR("Unsipported iomux type for echoer");
       out->retval = TE_RC(TE_TA_WIN32, TE_EOPNOTSUPP);
       return 0;
    }
    MAKE_CALL(out->retval = echoer(in));
    COPY_ARG(tx_stat);
    COPY_ARG(rx_stat);
}
)

/*-------------- WSACreateEvent ----------------------------*/

TARPC_FUNC(create_event, {},
{
    UNUSED(list);
    UNUSED(in);
    out->retval = rcf_pch_mem_alloc(WSACreateEvent());
}
)

/*-------------- WSACloseEvent ----------------------------*/

TARPC_FUNC(close_event, {},
{
    UNUSED(list);
    out->retval = WSACloseEvent(IN_HEVENT);
    rcf_pch_mem_free(in->hevent);
}
)

/*-------------- WSAResetEvent ----------------------------*/

TARPC_FUNC(reset_event, {},
{
    UNUSED(list);
    out->retval = WSAResetEvent(IN_HEVENT);
}
)

/*-------------- WSASetEvent ----------------------------*/
TARPC_FUNC(set_event, {},
{
    UNUSED(list);
    out->retval = WSASetEvent(IN_HEVENT);
}
)

/*-------------- WSAEventSelect ----------------------------*/

TARPC_FUNC(event_select, {},
{
    UNUSED(list);
    SetLastError(ERROR_UNSPEC);  
    out->retval = WSAEventSelect(in->fd, IN_HEVENT,
                                 network_event_rpc2h(in->event));
    out->common._errno = win_rpc_errno(GetLastError());
}
)

/*-------------- WSAEnumNetworkEvents ----------------------------*/

TARPC_FUNC(enum_network_events,
{
    COPY_ARG(event);
},
{
    WSANETWORKEVENTS  events_occured;

    UNUSED(list);
    out->retval = WSAEnumNetworkEvents(in->fd, IN_HEVENT,
                                       out->event.event_len == 0 ? NULL :
                                       &events_occured);
    if (out->event.event_len != 0)
        out->event.event_val[0] =
            network_event_h2rpc(events_occured.lNetworkEvents);
}
)
/*-------------- CreateWindow ----------------------------*/

static LRESULT CALLBACK
message_callback(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg > WM_USER)
        PRINT("Unexpected message %d is received", uMsg - WM_USER);

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

bool_t
_create_window_1_svc(tarpc_create_window_in *in,
                     tarpc_create_window_out *out,
                     struct svc_req *rqstp)
{
    static te_bool init = FALSE;

    UNUSED(rqstp);
    UNUSED(in);

    memset(out, 0, sizeof(*out));

    /* Should not be called in thread to prevent double initialization */

    if (!init)
    {
        WNDCLASSEX wcex;
        wcex.cbSize = sizeof(WNDCLASSEX);
        wcex.style = CS_HREDRAW | CS_VREDRAW;
        wcex.lpfnWndProc = (WNDPROC)message_callback;
        wcex.cbClsExtra = 0;
        wcex.cbWndExtra = 0;
        wcex.hInstance = ta_hinstance;
        wcex.hIcon = NULL;
        wcex.hCursor = NULL;
        wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wcex.lpszMenuName = NULL;
        wcex.lpszClassName = "MainWClass";
        wcex.hIconSm = NULL;

        if (!RegisterClassEx(&wcex))
        {
            ERROR("Failed to register class\n");
            out->hwnd = 0;
            return TRUE;
        }
        init = TRUE;
    }

    out->hwnd = rcf_pch_mem_alloc(
                    CreateWindow("MainWClass", "tawin32",
                                 WS_OVERLAPPEDWINDOW, CW_USEDEFAULT,
                                 0, CW_USEDEFAULT, 0, NULL, NULL,
                                 ta_hinstance, NULL));
    return TRUE;
}

/*-------------- DestroyWindow ----------------------------*/

TARPC_FUNC(destroy_window, {},
{
    UNUSED(out);
    UNUSED(list);
    DestroyWindow(IN_HWND);
    rcf_pch_mem_free(in->hwnd);
}
)

/*-------------- WSAAsyncSelect ---------------------------*/
bool_t
_wsa_async_select_1_svc(tarpc_wsa_async_select_in *in,
                        tarpc_wsa_async_select_out *out,
                        struct svc_req *rqstp)
{
    /*
     * Should not be called in thread to avoid troubles with threads
     * when message is retrieved.
     */

    UNUSED(rqstp);
    memset(out, 0, sizeof(*out));
    SetLastError(ERROR_UNSPEC);  
    out->retval = WSAAsyncSelect(in->sock, IN_HWND, WM_USER + 1,
                                 network_event_rpc2h(in->event));
    out->common._errno = win_rpc_errno(GetLastError());
    return TRUE;
}

/*-------------- PeekMessage ---------------------------------*/
bool_t
_peek_message_1_svc(tarpc_peek_message_in *in,
                   tarpc_peek_message_out *out,
                   struct svc_req *rqstp)
{
    MSG msg;

    /*
     * Should not be called in thread to avoid troubles with threads
     * when message is retrieved.
     */

    UNUSED(rqstp);
    memset(out, 0, sizeof(*out));

    while ((out->retval = PeekMessage(&msg, IN_HWND,
                                      0, 0, PM_REMOVE)) != 0 &&
           msg.message != WM_USER + 1);

    if (out->retval != 0)
    {
        out->sock = msg.wParam;
        out->event = network_event_h2rpc(msg.lParam);
    }

    return TRUE;
}

/*-------------- Create WSAOVERLAPPED --------------------------*/

TARPC_FUNC(create_overlapped, {},
{
    rpc_overlapped *tmp;

    UNUSED(list);
    if ((tmp = (rpc_overlapped *)calloc(1, sizeof(rpc_overlapped))) == NULL)
    {
        out->common._errno = TE_RC(TE_TA_WIN32, TE_ENOMEM);
    }
    else
    {
        tmp->overlapped.hEvent = IN_HEVENT;
        tmp->overlapped.Offset = in->offset;
        tmp->overlapped.OffsetHigh = in->offset_high;
        tmp->cookie1 = in->cookie1;
        tmp->cookie2 = in->cookie2;
        out->retval = rcf_pch_mem_alloc(tmp);
        RING("Overlapped structure %x (index %d) is allocated", tmp, 
            out->retval);
    }
}
)

/*-------------- Delete WSAOVERLAPPED ----------------------------*/

TARPC_FUNC(delete_overlapped, {},
{
    UNUSED(list);
    UNUSED(out);
    
    rpc_overlapped_free_memory(IN_OVERLAPPED);
    free(IN_OVERLAPPED);
    rcf_pch_mem_free(in->overlapped);
}
)

/*-------------- Completion callback-related staff ---------------------*/
static int completion_called = 0;
static int completion_error = 0;
static int completion_bytes = 0;
static tarpc_overlapped completion_overlapped = 0;
static void *completion_lock;

void CALLBACK
default_completion_callback(DWORD error, DWORD bytes, 
                            LPWSAOVERLAPPED overlapped,
                            DWORD flags)
{
    UNUSED(flags);

    thread_mutex_lock(completion_lock);
    completion_called++;
    completion_error = win_rpc_errno(error);
    completion_bytes = bytes;
    completion_overlapped = 
        (tarpc_overlapped)rcf_pch_mem_get_id(overlapped);
    thread_mutex_unlock(completion_lock);
}

TARPC_FUNC(completion_callback, {},
{
    UNUSED(list);
    UNUSED(in);
    
    completion_callback_register("default_completion_callback",
                                 default_completion_callback);
    
    if (completion_lock == NULL)
        completion_lock = thread_mutex_create();

    thread_mutex_lock(completion_lock);
    out->called = completion_called;
    completion_called = 0;
    out->bytes = completion_bytes;
    completion_bytes = 0;
    out->error = completion_error;
    out->overlapped = completion_overlapped;
    thread_mutex_unlock(completion_lock);
}
)

/*-------------- WSASend() ------------------------------*/
TARPC_FUNC(wsa_send,
{
    COPY_ARG(bytes_sent);
},
{
    rpc_overlapped *overlapped = IN_OVERLAPPED;
    rpc_overlapped  tmp;

    if (overlapped == NULL)
    {
        memset(&tmp, 0, sizeof(tmp));
        overlapped = &tmp;
    }
    if (iovec2overlapped(overlapped, in->vector.vector_len,
                         in->vector.vector_val) != 0)
    {
        out->common._errno = TE_RC(TE_TA_WIN32, TE_ENOMEM);
        goto finish;
    }

    MAKE_CALL(out->retval =
                  WSASend(in->s, overlapped->buffers, in->count,
                          out->bytes_sent.bytes_sent_len == 0 ? NULL :
                          (LPDWORD)(out->bytes_sent.bytes_sent_val),
                          send_recv_flags_rpc2h(in->flags),
                          in->overlapped == 0 ? NULL
                                  : (LPWSAOVERLAPPED)overlapped,
                          IN_CALLBACK));

    if (in->overlapped == 0 || out->retval >= 0 ||
        out->common._errno != RPC_E_IO_PENDING)
    {
        rpc_overlapped_free_memory(overlapped);
    }
    finish:
    ;
}
)

/*-------------- WSARecv() ------------------------------*/
TARPC_FUNC(wsa_recv,
{
    COPY_ARG(bytes_received);
    COPY_ARG(flags);
},
{
    rpc_overlapped *overlapped = IN_OVERLAPPED;
    rpc_overlapped  tmp;

    if (overlapped == NULL)
    {
        memset(&tmp, 0, sizeof(tmp));
        overlapped = &tmp;
    }
    if (iovec2overlapped(overlapped, in->vector.vector_len,
                         in->vector.vector_val) != 0)
    {
        out->common._errno = TE_RC(TE_TA_WIN32, TE_ENOMEM);
        goto finish;
    }
    if (out->flags.flags_len > 0)
    {
        out->flags.flags_val[0] =
            send_recv_flags_rpc2h(out->flags.flags_val[0]);
    }       

    MAKE_CALL(out->retval =
                  WSARecv(in->s, overlapped->buffers, in->count,
                          out->bytes_received.bytes_received_len == 0 ?
                          NULL :
                          (LPDWORD)(out->bytes_received.bytes_received_val),
                          out->flags.flags_len > 0 ?
                          (LPDWORD)(out->flags.flags_val) : NULL,
                          in->overlapped == 0 ? NULL :
                          (LPWSAOVERLAPPED)overlapped,
                          IN_CALLBACK));
     
    if ((out->retval >= 0) || (out->common._errno == RPC_EMSGSIZE))
    {
        overlapped2iovec(overlapped, &(out->vector.vector_len),
                         &(out->vector.vector_val));
        if (out->flags.flags_len > 0)
            out->flags.flags_val[0] =
                send_recv_flags_h2rpc(out->flags.flags_val[0]);
    }
    else if (in->overlapped == 0 ||
             out->common._errno != RPC_E_IO_PENDING)
    {
        rpc_overlapped_free_memory(overlapped);
    }
    finish:
    ;
}
)

TARPC_FUNC(wsa_get_overlapped_result,
{
    COPY_ARG(bytes);
    COPY_ARG(flags);
},
{
    rpc_overlapped *overlapped = IN_OVERLAPPED;

    UNUSED(list);
    MAKE_CALL(out->retval =
                  WSAGetOverlappedResult(in->s,
                                         (LPWSAOVERLAPPED)overlapped,
                                         out->bytes.bytes_len == 0 ? NULL :
                                         (LPDWORD)(out->bytes.bytes_val),
                                         in->wait,
                                         out->flags.flags_len > 0 ?
                                         (LPDWORD)(out->flags.flags_val) :
                                         NULL));
    if (out->retval)
    {
        if (out->flags.flags_len > 0)
            out->flags.flags_val[0] =
                send_recv_flags_h2rpc(out->flags.flags_val[0]);

        if (in->get_data)
        {
            overlapped2iovec(overlapped, &out->vector.vector_len,
                             &out->vector.vector_val);
        }
        else
        {
            out->vector.vector_val = NULL;
            out->vector.vector_len = 0;
        }
    }
}
)

/*-------------- getpid() --------------------------------*/
TARPC_FUNC(getpid, {}, { MAKE_CALL(out->retval = GetCurrentProcessId()); })

/*-------------- WSADuplicateSocket() ---------------------------*/
TARPC_FUNC(duplicate_socket,
{
    if (in->info.info_len != 0 &&
        in->info.info_len < sizeof(WSAPROTOCOL_INFO))
    {
        ERROR("Too short buffer for protocol info is provided");
        out->common._errno = TE_RC(TE_TA_WIN32, TE_ENOMEM);
        return TRUE;
    }
    COPY_ARG(info);
},
{
    MAKE_CALL(out->retval =
                  WSADuplicateSocket(in->s, in->pid,
                      out->info.info_len == 0 ? NULL :
                      (LPWSAPROTOCOL_INFO)(out->info.info_val)));
    out->info.info_len = sizeof(WSAPROTOCOL_INFO);
}
)

/*-------------- WSAWaitForMultipleEvents() -------------------------*/
#define MULTIPLE_EVENTS_MAX     128
TARPC_FUNC(wait_for_multiple_events, 
{
    if (in->events.events_len > MULTIPLE_EVENTS_MAX)
    {
        ERROR("Too many events are awaited");
        out->common._errno = TE_RC(TE_TA_WIN32, TE_ENOMEM);
        return TRUE;
    }
},
{
    WSAEVENT events[MULTIPLE_EVENTS_MAX];
    uint32_t i;
    
    for (i = 0; i < in->events.events_len; i++)
        events[i] = rcf_pch_mem_get(in->events.events_val[i]);
    
    INIT_CHECKED_ARG((char *)events, sizeof(events), 0);

    MAKE_CALL(out->retval = WSAWaitForMultipleEvents(in->events.events_len,
                                                     events,
                                                     in->wait_all, 
                                                     in->timeout, 
                                                     in->alertable));
    switch (out->retval)
    {
        case WSA_WAIT_FAILED:
            out->retval = TARPC_WSA_WAIT_FAILED;
            break;

        case WAIT_IO_COMPLETION:
            out->retval = TARPC_WAIT_IO_COMPLETION;
            break;

        case WSA_WAIT_TIMEOUT:
            out->retval = TARPC_WSA_WAIT_TIMEOUT;
            break;

        default:
            out->retval = TARPC_WSA_WAIT_EVENT_0 +
                         (out->retval - WSA_WAIT_EVENT_0);
    }
}
)


/*----------------- WSASendTo() -------------------------*/
TARPC_FUNC(wsa_send_to,
{
    COPY_ARG(bytes_sent);
},
{
    rpc_overlapped *overlapped = IN_OVERLAPPED;
    rpc_overlapped  tmp;

    PREPARE_ADDR(in->to, 0);

    if (overlapped == NULL)
    {
        memset(&tmp, 0, sizeof(tmp));
        overlapped = &tmp;
    }
    if (iovec2overlapped(overlapped, in->vector.vector_len,
                         in->vector.vector_val) != 0)
    {
        out->common._errno = TE_RC(TE_TA_WIN32, TE_ENOMEM);
        goto finish;
    }

    MAKE_CALL(out->retval =
        WSASendTo(in->s,
            overlapped->buffers,
            in->count,
            out->bytes_sent.bytes_sent_len == 0 ? NULL :
                (LPDWORD)(out->bytes_sent.bytes_sent_val),
            send_recv_flags_rpc2h(in->flags),
            a,
            in->tolen,
            in->overlapped == 0 ? NULL : (LPWSAOVERLAPPED)overlapped,
            IN_CALLBACK));

    if (in->overlapped == 0 || out->retval >= 0 ||
        out->common._errno != RPC_E_IO_PENDING)
    {
        rpc_overlapped_free_memory(overlapped);
    }

    finish:
    ;
}
)

/*----------------- WSARecvFrom() -------------------------*/
TARPC_FUNC(wsa_recv_from,
{
    COPY_ARG(bytes_received);
    COPY_ARG(flags);
    COPY_ARG(fromlen);
    COPY_ARG_ADDR(from);
},
{
    rpc_overlapped *overlapped = IN_OVERLAPPED;
    rpc_overlapped  tmp;

    PREPARE_ADDR(out->from, out->fromlen.fromlen_len == 0 ? 0 :
                                    *out->fromlen.fromlen_val);

    if (overlapped == NULL)
    {
        memset(&tmp, 0, sizeof(tmp));
        overlapped = &tmp;
    }
    if (iovec2overlapped(overlapped, in->vector.vector_len,
                         in->vector.vector_val) != 0)
    {
        out->common._errno = TE_RC(TE_TA_WIN32, TE_ENOMEM);
        goto finish;
    }
    if (out->flags.flags_len > 0)
        out->flags.flags_val[0] =
            send_recv_flags_rpc2h(out->flags.flags_val[0]);

    MAKE_CALL(out->retval =
        WSARecvFrom(in->s,
            overlapped->buffers,
            in->count,
            out->bytes_received.bytes_received_len == 0 ?
                NULL : (LPDWORD)(out->bytes_received.bytes_received_val),
            out->flags.flags_len > 0 ?
                (LPDWORD)(out->flags.flags_val) : NULL,
            a,
            out->fromlen.fromlen_len == 0 ? NULL :
                (LPINT)out->fromlen.fromlen_val,
            in->overlapped == 0 ? NULL : (LPWSAOVERLAPPED)overlapped,
            IN_CALLBACK));
    
    if ((out->retval >= 0) || (out->common._errno == RPC_EMSGSIZE))
    {
        overlapped2iovec(overlapped, &(out->vector.vector_len),
                         &(out->vector.vector_val));
        if (out->flags.flags_len > 0)
            out->flags.flags_val[0] =
                send_recv_flags_h2rpc(out->flags.flags_val[0]);

        sockaddr_h2rpc(a, &(out->from));    
    }
    else if (in->overlapped == 0 ||
             out->common._errno != RPC_E_IO_PENDING)
    {
        rpc_overlapped_free_memory(overlapped);
    }

    finish:
    ;
}
)

/*----------------- WSASendDisconnect() -------------------------*/
TARPC_FUNC(wsa_send_disconnect, {},
{
    rpc_overlapped  tmp;
    rpc_overlapped *overlapped = &tmp;

    memset(&tmp, 0, sizeof(tmp));

    if (iovec2overlapped(overlapped, in->vector.vector_len,
                         in->vector.vector_val) != 0)
    {
        out->common._errno = TE_RC(TE_TA_WIN32, TE_ENOMEM);
        goto finish;
    }
    
    MAKE_CALL(out->retval = WSASendDisconnect(in->s, overlapped->buffers));

    if (out->retval >= 0)
        rpc_overlapped_free_memory(overlapped);

    finish:
    ;
}
)

/*----------------- WSARecvDisconnect() -------------------------*/
TARPC_FUNC(wsa_recv_disconnect, {},
{
    rpc_overlapped  tmp;
    rpc_overlapped *overlapped = &tmp;

    memset(&tmp, 0, sizeof(tmp));
    
    if (iovec2overlapped(overlapped, in->vector.vector_len,
                         in->vector.vector_val) != 0)
    {
        out->common._errno = TE_RC(TE_TA_WIN32, TE_ENOMEM);
        goto finish;
    }

    MAKE_CALL(out->retval = WSARecvDisconnect(in->s, overlapped->buffers));

    if (out->retval >= 0)
    {
        overlapped2iovec(overlapped, &(out->vector.vector_len),
                         &(out->vector.vector_val));
    }
    else
        rpc_overlapped_free_memory(overlapped);

    finish:
    ;
}
)

/*--------------- WSARecvMsg() -----------------------------*/
TARPC_FUNC(wsa_recv_msg,
{
    if (in->msg.msg_val != NULL &&
        in->msg.msg_val[0].msg_iov.msg_iov_val != NULL &&
        in->msg.msg_val[0].msg_iov.msg_iov_len > RCF_RPC_MAX_IOVEC)
    {
        ERROR("Too long iovec is provided");
        out->common._errno = TE_RC(TE_TA_WIN32, TE_ENOMEM);
        return TRUE;
    }
    COPY_ARG(msg);
    COPY_ARG(bytes_received);
},
{
    WSAMSG                msg;
    rpc_overlapped       *overlapped = IN_OVERLAPPED;
    rpc_overlapped        tmp;
    struct tarpc_msghdr  *rpc_msg;

    memset(&msg, 0, sizeof(msg));

    if (overlapped == NULL)
    {
        memset(&tmp, 0, sizeof(tmp));
        overlapped = &tmp;    
    }

    rpc_msg = out->msg.msg_val;

    if (rpc_msg == NULL)
    {
        MAKE_CALL(out->retval =
            (*pf_wsa_recvmsg)(in->s, NULL,
                out->bytes_received.bytes_received_len == 0 ? NULL :
                    (LPDWORD)(out->bytes_received.bytes_received_val),
                in->overlapped == 0 ? NULL : (LPWSAOVERLAPPED)overlapped,
                IN_CALLBACK));
    }
    else
    {
        PREPARE_ADDR(rpc_msg->msg_name, rpc_msg->msg_namelen);
        
        msg.namelen = rpc_msg->msg_namelen;
        msg.name = a;

        msg.dwBufferCount = rpc_msg->msg_iovlen;
        if (rpc_msg->msg_iov.msg_iov_val != NULL)
        {
            if (iovec2overlapped(overlapped, rpc_msg->msg_iov.msg_iov_len,
                                 rpc_msg->msg_iov.msg_iov_val) != 0)
            {
                out->common._errno = TE_RC(TE_TA_WIN32, TE_ENOMEM);
                goto finish;
            }
            msg.lpBuffers = overlapped->buffers;
        }

        if (rpc_msg->msg_control.msg_control_len > 0)
        {
            ERROR("Non-zero Control is not supported");
            out->common._errno = TE_RC(TE_TA_WIN32, TE_EINVAL);
            goto finish;
        }

        msg.dwFlags = send_recv_flags_rpc2h(rpc_msg->msg_flags);

        /*
         * msg_name, msg_iov, msg_iovlen and msg_control MUST NOT be
         * changed.
         * msg_namelen, msg_controllen and msg_flags MAY be changed.
         */
        INIT_CHECKED_ARG((char *)&msg.name, sizeof(msg.name), 0);
        INIT_CHECKED_ARG((char *)&msg.lpBuffers, sizeof(msg.lpBuffers), 0);
        INIT_CHECKED_ARG((char *)&msg.dwBufferCount,
                         sizeof(msg.dwBufferCount), 0);
        INIT_CHECKED_ARG((char *)&msg.Control, sizeof(msg.Control), 0);
            
        MAKE_CALL(out->retval =
            (*pf_wsa_recvmsg)(in->s, &msg,
                out->bytes_received.bytes_received_len == 0 ? NULL :
                    (LPDWORD)(out->bytes_received.bytes_received_val),
                in->overlapped == 0 ? NULL : (LPWSAOVERLAPPED)overlapped,
                IN_CALLBACK));

        if (out->retval >= 0)
        {
            unsigned int i;

            /* Free the current buffers of vector */
            for (i = 0; i < rpc_msg->msg_iov.msg_iov_len; i++)
            {
                free(rpc_msg->msg_iov.msg_iov_val[i].iov_base.iov_base_val);
                rpc_msg->msg_iov.msg_iov_val[i].iov_base.iov_base_val =
                                                                       NULL;
            }

            /* Make the buffers of overlapped structure
             * to become the buffers of vector */
            overlapped2iovec(overlapped, &(rpc_msg->msg_iov.msg_iov_len),
                             &(rpc_msg->msg_iov.msg_iov_val));

            sockaddr_h2rpc(a, &(rpc_msg->msg_name));
            rpc_msg->msg_namelen = msg.namelen;
            rpc_msg->msg_flags = send_recv_flags_h2rpc(msg.dwFlags);
        }
        else if (in->overlapped == 0 ||
                 out->common._errno != RPC_E_IO_PENDING)
        {
            rpc_overlapped_free_memory(overlapped);
        }
    }

    finish:
    ;
}
)

/*-------------- kill() --------------------------------*/

TARPC_FUNC(kill, {},
{
    HANDLE hp;

    if ((hp = OpenProcess(SYNCHRONIZE | PROCESS_TERMINATE, FALSE, 
                          in->pid)) == NULL)
    {
        out->common._errno = win_rpc_errno(GetLastError());
        out->retval = -1;
        ERROR("Cannot open process, error = %d\n", GetLastError());
        goto finish;
    }

    MAKE_CALL(out->retval = TerminateProcess(hp, 0) ? 0 : -1);

    CloseHandle(hp);
    
    finish:
    ;
}
)

/*-------------- overfill_buffers() -----------------------------*/
int
overfill_buffers(tarpc_overfill_buffers_in *in,
                 tarpc_overfill_buffers_out *out)
{
    ssize_t    rc = 0;
    int        err = 0;
    uint64_t   total = 0;
    int        unchanged = 0;
    u_long     val = 1;
    uint8_t    c = 0xAB;

    out->bytes = 0;
    
    /* ATTENTION! socket is assumed in blocking state */
    
    if (ioctlsocket(in->sock, FIONBIO, &val) < 0)
    {
        err = GetLastError();
        rc = -1;
        ERROR("%s(): Failed to move socket to non-blocking state; error %r",
              __FUNCTION__, win_rpc_errno(err));
        goto overfill_buffers_exit;
    }

    do {
        do {
            rc = send(in->sock, &c, 1, 0);
            err = GetLastError();
            
            if (rc == -1 && err != WSAEWOULDBLOCK)
            {
                ERROR("%s(): send() failed; error %r", __FUNCTION__, 
                      win_rpc_errno(err));
                goto overfill_buffers_exit;
            }
            if (rc != -1)
                out->bytes++;
            else
                Sleep(100);
        } while (err != WSAEWOULDBLOCK);

        if (total != out->bytes)
        {
            total = out->bytes;
            unchanged = 0;
        }
        else
        {
            unchanged++;
        }
        rc = 0;
        err = 0;
    } while (unchanged != 30);
    
overfill_buffers_exit:
    val = 0;
    if (ioctlsocket(in->sock, FIONBIO, &val) < 0)
    {
        err = GetLastError();
        rc = -1;
        ERROR("%s(): Failed to move socket back to blocking state; "
              "error %r", __FUNCTION__, win_rpc_errno(err));
    }
    out->common._errno = win_rpc_errno(err);

    return rc;
}

TARPC_FUNC(overfill_buffers,{},
{
    MAKE_CALL(out->retval = overfill_buffers(in, out));
}
)

/*-------------- WSAAddressToString ---------------------*/
TARPC_FUNC(wsa_address_to_string,
{
    COPY_ARG(addrstr);
    COPY_ARG(addrstr_len);
},
{
    PREPARE_ADDR(in->addr, 0);

    MAKE_CALL(out->retval = WSAAddressToString(a, in->addrlen,
                                (LPWSAPROTOCOL_INFO)(in->info.info_val),
                                out->addrstr.addrstr_val,
                                out->addrstr_len.addrstr_len_val));
}
)

/*-------------- WSAStringToAddress ---------------------*/
TARPC_FUNC(wsa_string_to_address,
{
    COPY_ARG(addrlen);
},
{
    PREPARE_ADDR(out->addr, out->addrlen.addrlen_len == 0 ? 0 :
                                    *out->addrlen.addrlen_val);

    MAKE_CALL(out->retval = WSAStringToAddress(in->addrstr.addrstr_val,
                                domain_rpc2h(in->address_family),
                                (LPWSAPROTOCOL_INFO)(in->info.info_val),
                                a,
                                (LPINT)out->addrlen.addrlen_val));

    if (out->retval == 0)
        sockaddr_h2rpc(a, &(out->addr));
}
)

/*-------------- WSACancelAsyncRequest ------------------*/
TARPC_FUNC(wsa_cancel_async_request, {},
{
    MAKE_CALL(out->retval = WSACancelAsyncRequest(
                  (HANDLE)rcf_pch_mem_get(in->async_task_handle)));
    rcf_pch_mem_free(in->async_task_handle);
}
)

/**
 * Allocate a single buffer of specified size and return a pointer to it.
 */
TARPC_FUNC(malloc, {},
{
    void *buf;
    
    UNUSED(list);
    
    buf = malloc(in->size);

    if (buf == NULL)
        out->common._errno = TE_RC(TE_TA_WIN32, TE_ENOMEM);
    else
        out->retval = rcf_pch_mem_alloc(buf);
}
)

/**
 * Free a previously allocated buffer.
 */
TARPC_FUNC(free, {},
{
    UNUSED(list);
    UNUSED(out);
    
    free(rcf_pch_mem_get(in->buf));
    rcf_pch_mem_free(in->buf);
}
)

/**
 * Fill in the buffer.
 */
TARPC_FUNC(set_buf, {},
{
    uint8_t *dst_buf;
    
    UNUSED(list);
    UNUSED(out);
    
    dst_buf = rcf_pch_mem_get(in->dst_buf);
    if (dst_buf != NULL && in->src_buf.src_buf_len != 0)
    {
        memcpy(dst_buf + (unsigned int)in->offset,
               in->src_buf.src_buf_val, in->src_buf.src_buf_len);
    }
}
)

TARPC_FUNC(get_buf, {},
{
    uint8_t *src_buf; 
    
    UNUSED(list);

    src_buf = rcf_pch_mem_get(in->src_buf);
    if (src_buf != NULL && in->len != 0)
    {
        uint8_t *buf;

        buf = malloc(in->len);
        if (buf == NULL)
            out->common._errno = TE_RC(TE_TA_WIN32, TE_ENOMEM);
        else
        {
            memcpy(buf, (char *)src_buf + 
                   (unsigned int)in->offset, in->len);
            out->dst_buf.dst_buf_val = buf;
            out->dst_buf.dst_buf_len = in->len;
        }
    }
    else
    {
        out->dst_buf.dst_buf_val = NULL;
        out->dst_buf.dst_buf_len = 0;
    }
}
)

/*---------------------- Fill buffer by the pattern ----------------------*/
bool_t
_set_buf_pattern_1_svc(tarpc_set_buf_pattern_in *in, 
                       tarpc_set_buf_pattern_out *out,
                       struct svc_req *rqstp)
{
    uint8_t *dst_buf;
    
    UNUSED(rqstp);
    UNUSED(out);
    
    dst_buf = (uint8_t *)rcf_pch_mem_get(in->dst_buf) + 
              (unsigned int)in->offset;
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

/**
 * Allocate a single WSABUF structure and a buffer of specified size;
 * set the fields of the allocated structure according to the allocated
 * buffer pointer and length. Return two pointers: to the structure and
 * to the buffer.
 */
TARPC_FUNC(alloc_wsabuf, {},
{
    WSABUF *wsabuf;
    void *buf;

    UNUSED(list);

    wsabuf = malloc(sizeof(WSABUF));
    if ((wsabuf != NULL) && (in->len != 0))
        buf = calloc(1, in->len);
    else
        buf = NULL;

    if ((wsabuf == NULL) || ((buf == NULL) && (in->len != 0)))
    {
        if (wsabuf != NULL)
            free(wsabuf);
        if (buf != NULL)
            free(buf);
        out->common._errno = TE_RC(TE_TA_WIN32, TE_ENOMEM);
        out->wsabuf = 0;
        out->wsabuf_buf = 0;
        out->retval = -1;
    }
    else
    {
        wsabuf->buf = buf;
        wsabuf->len = in->len;
        out->wsabuf = rcf_pch_mem_alloc(wsabuf);
        out->wsabuf_buf = rcf_pch_mem_alloc(buf);
        out->retval = 0;
    }
}
)

/**
 * Free a previously allocated WSABUF structure and its buffer.
 */
TARPC_FUNC(free_wsabuf, {},
{
    WSABUF *wsabuf;

    UNUSED(list);
    UNUSED(out);
    wsabuf = (WSABUF *)rcf_pch_mem_get(in->wsabuf);
    if (wsabuf != NULL)
    {
        rcf_pch_mem_free_mem(wsabuf->buf);
        free(wsabuf->buf);
        free(wsabuf);
    }
    rcf_pch_mem_free(in->wsabuf);
}
)

/*
 * Copy the data from tarpc_flowspec to FLOWSPEC structure.
 */
static void flowspec_rpc2h(FLOWSPEC *fs, tarpc_flowspec *tfs)
{
    fs->TokenRate = tfs->TokenRate;
    fs->TokenBucketSize = tfs->TokenBucketSize;
    fs->PeakBandwidth = tfs->PeakBandwidth;
    fs->Latency = tfs->Latency;
    fs->DelayVariation = tfs->DelayVariation;
    fs->ServiceType =
        (SERVICETYPE)servicetype_flags_rpc2h(tfs->ServiceType);
    fs->MaxSduSize = tfs->MaxSduSize;
    fs->MinimumPolicedSize = tfs->MinimumPolicedSize;
}

/*
 * Copy the data from FLOWSPEC to tarpc_flowspec structure.
 */
static void flowspec_h2rpc(FLOWSPEC *fs, tarpc_flowspec *tfs)
{
    tfs->TokenRate = fs->TokenRate;
    tfs->TokenBucketSize = fs->TokenBucketSize;
    tfs->PeakBandwidth = fs->PeakBandwidth;
    tfs->Latency = fs->Latency;
    tfs->DelayVariation = fs->DelayVariation;
    tfs->ServiceType =
        servicetype_flags_h2rpc((unsigned int)fs->ServiceType);
    tfs->MaxSduSize = fs->MaxSduSize;
    tfs->MinimumPolicedSize = fs->MinimumPolicedSize;
}

/*-------------- WSAConnect -----------------------------*/
TARPC_FUNC(wsa_connect, {},
{
    QOS sqos;
    QOS *psqos;
    PREPARE_ADDR(in->addr, 0);

    if (in->sqos_is_null == TRUE)
        psqos = NULL;
    else
    {
        psqos = &sqos;
        memset(&sqos, 0, sizeof(sqos));
        flowspec_rpc2h(&sqos.SendingFlowspec, &in->sqos.sending);
        flowspec_rpc2h(&sqos.ReceivingFlowspec, &in->sqos.receiving);
        sqos.ProviderSpecific.buf =
            (char*)in->sqos.provider_specific_buf.provider_specific_buf_val;
        sqos.ProviderSpecific.len =
            in->sqos.provider_specific_buf.provider_specific_buf_len;
    }

    MAKE_CALL(out->retval = WSAConnect(in->s, a, in->addrlen,
                                       (LPWSABUF)
                                       rcf_pch_mem_get(in->caller_wsabuf),
                                       (LPWSABUF)
                                       rcf_pch_mem_get(in->callee_wsabuf),
                                       psqos, NULL));
}
)

/**
 * Convert the TA-dependent result (output) of the WSAIoctl() call into
 * the wsa_ioctl_request structure representation.
 */
static int
convert_wsa_ioctl_result(DWORD code, char *buf, wsa_ioctl_request *res)
{
    switch (code)
    {
        case RPC_SIO_ADDRESS_LIST_QUERY:
        {
            SOCKET_ADDRESS_LIST *sal;
            tarpc_sa            *tsa;
            int                  i;

            res->type = WSA_IOCTL_SAA;
        
            sal = (SOCKET_ADDRESS_LIST *)buf;
            tsa = (tarpc_sa *)calloc(sal->iAddressCount, sizeof(tarpc_sa));
        
            for (i = 0; i < sal->iAddressCount; i++)
            {
                tsa[i].sa_data.sa_data_len = sal->Address[i].
                                                  iSockaddrLength;
                tsa[i].sa_data.sa_data_val = (uint8_t *)malloc(sal->
                                             Address[i].iSockaddrLength);
                sockaddr_h2rpc(sal->Address[i].lpSockaddr, &tsa[i]);
            }

            res->wsa_ioctl_request_u.req_saa.req_saa_val = tsa;
            res->wsa_ioctl_request_u.req_saa.req_saa_len = i;
            
            break;
        }

        case RPC_SIO_GET_BROADCAST_ADDRESS:
        case RPC_SIO_ROUTING_INTERFACE_QUERY:
            res->type = WSA_IOCTL_SA;
            sockaddr_h2rpc((struct sockaddr *)buf,
                &res->wsa_ioctl_request_u.req_sa);
            break;

        case RPC_SIO_GET_EXTENSION_FUNCTION_POINTER:
            res->type = WSA_IOCTL_PTR;
            res->wsa_ioctl_request_u.req_ptr = 
                rcf_pch_mem_alloc(*(void **)buf);
            break;

        case RPC_SIO_GET_GROUP_QOS:
        case RPC_SIO_GET_QOS:
        {
            QOS       *qos;
            tarpc_qos *tqos;
            
            res->type = WSA_IOCTL_QOS;
            
            qos = (QOS *)buf;
            tqos = &res->wsa_ioctl_request_u.req_qos;

            flowspec_h2rpc(&qos->SendingFlowspec, &tqos->sending);
            flowspec_h2rpc(&qos->ReceivingFlowspec, &tqos->receiving);
                                             
            if (qos->ProviderSpecific.len != 0)
            {
               tqos->provider_specific_buf.provider_specific_buf_val =
                   malloc(qos->ProviderSpecific.len);
               if (tqos->provider_specific_buf.provider_specific_buf_val == 
                   NULL)
               {
                   ERROR("Failed to allocate memory for ProviderSpecific");
                   return -1;
               }
               memcpy(tqos->provider_specific_buf.provider_specific_buf_val,
                   qos->ProviderSpecific.buf, qos->ProviderSpecific.len);
               tqos->provider_specific_buf.provider_specific_buf_len =
                   qos->ProviderSpecific.len;
            }
            else
            {
               tqos->provider_specific_buf.provider_specific_buf_val = NULL;
               tqos->provider_specific_buf.provider_specific_buf_len = 0;
            }
        }
        
        default:
            res->type = WSA_IOCTL_INT;
            res->wsa_ioctl_request_u.req_int = *(int *)buf;
            break;
    }
    return 0;
}

/*-------------- WSAIoctl -------------------------------*/
TARPC_FUNC(wsa_ioctl, 
{
    COPY_ARG(outbuf);
    COPY_ARG(bytes_returned);
},
{
    rpc_overlapped           *overlapped = NULL;
    void                     *inbuf = NULL;
    void                     *outbuf = NULL;
    struct sockaddr_storage   addr;
    QOS                       qos;
    struct tcp_keepalive      tka;
    GUID                      guid;
    int                       inbuf_len = 0;
    wsa_ioctl_request        *req = in->inbuf.inbuf_val;
    
    /* Prepare output buffer */
    if (out->outbuf.outbuf_val != NULL)
    {
        if ((outbuf = malloc(in->outbuf_len + 1)) == NULL)
        {
            out->common._errno = TE_RC(TE_TA_WIN32, TE_ENOMEM);
            goto finish;
        }
        INIT_CHECKED_ARG(outbuf, in->outbuf_len + 1, in->outbuf_len);
    }

    if (in->overlapped != 0)
    {
        overlapped = IN_OVERLAPPED;
        rpc_overlapped_free_memory(overlapped);

        if (outbuf != NULL)
        {
            overlapped->buffers = malloc(sizeof(WSABUF));
            if (overlapped->buffers == NULL)
            {
                out->common._errno = TE_RC(TE_TA_WIN32, TE_ENOMEM);
                free(outbuf);
                goto finish;
            }
            overlapped->buffers[0].buf = outbuf;
            overlapped->buffers[0].len = in->outbuf_len + 1;
        }
    }

    
    /* Prepare input data */
    if (in->inbuf.inbuf_val == NULL)
        goto call;

    switch (req->type)
    {
        case WSA_IOCTL_VOID:
        case WSA_IOCTL_SAA:
            break;

        case WSA_IOCTL_INT:
            inbuf = &req->wsa_ioctl_request_u.req_int;
            inbuf_len = sizeof(int);
            break;

        case WSA_IOCTL_SA:
            inbuf = sockaddr_rpc2h(&req->wsa_ioctl_request_u.req_sa, &addr);
            inbuf_len = sizeof(addr);
            break;

        case WSA_IOCTL_GUID:
            guid.Data1 = req->wsa_ioctl_request_u.req_guid.data1;
            guid.Data2 = req->wsa_ioctl_request_u.req_guid.data2;
            guid.Data3 = req->wsa_ioctl_request_u.req_guid.data3;
            memcpy(guid.Data4,
                   req->wsa_ioctl_request_u.req_guid.data4,
                   sizeof(guid.Data4)); 
            inbuf = &guid;
            inbuf_len = sizeof(GUID);
            break;

        case WSA_IOCTL_TCP_KEEPALIVE:
        {
            tarpc_tcp_keepalive *intka;

            intka = &req->wsa_ioctl_request_u.req_tka;
            tka.onoff = intka->onoff;
            tka.keepalivetime = intka->keepalivetime;
            tka.keepaliveinterval = intka->keepaliveinterval;
            inbuf = &tka;
            inbuf_len = sizeof(tka);
            break;
        }

        case WSA_IOCTL_QOS:
        {
            tarpc_qos *inqos;

            inqos = &req->wsa_ioctl_request_u.req_qos;
            flowspec_rpc2h(&qos.SendingFlowspec, &inqos->sending);
            flowspec_rpc2h(&qos.ReceivingFlowspec, &inqos->receiving);
            qos.ProviderSpecific.buf =
                inqos->provider_specific_buf.provider_specific_buf_val;
            qos.ProviderSpecific.len =
                inqos->provider_specific_buf.provider_specific_buf_len;
            INIT_CHECKED_ARG(qos.ProviderSpecific.buf, 
                             qos.ProviderSpecific.len, 0);
            inbuf = &qos;
            inbuf_len = sizeof(QOS);
            break;
        }

        case WSA_IOCTL_PTR:
            inbuf = rcf_pch_mem_get(req->wsa_ioctl_request_u.req_ptr);
            inbuf_len = in->inbuf_len;
            break;
    }

    INIT_CHECKED_ARG(inbuf, inbuf_len, 0);


call:
    MAKE_CALL(out->retval = WSAIoctl(in->s, ioctl_rpc2h(in->code),
                                     inbuf,
                                     in->inbuf_len,
                                     outbuf,
                                     in->outbuf_len,
                                     out->bytes_returned.bytes_returned_val,
                                     in->overlapped == 0 ? NULL
                                       : (LPWSAOVERLAPPED)overlapped,
                                     IN_CALLBACK));
    if (out->retval == 0)
    {
        if (outbuf != NULL && out->outbuf.outbuf_val != NULL)
            convert_wsa_ioctl_result(in->code, outbuf, 
                                     out->outbuf.outbuf_val);

        if (overlapped != NULL)
            rpc_overlapped_free_memory(overlapped);
        else
            free(outbuf);
    }
    else if ((overlapped != NULL) &&
            (out->common._errno != RPC_E_IO_PENDING))
    {
        rpc_overlapped_free_memory(overlapped);
    }

    finish:
    ;
}
)

TARPC_FUNC(get_wsa_ioctl_overlapped_result,
{
    COPY_ARG(bytes);
    COPY_ARG(flags);
},
{
    rpc_overlapped *overlapped = IN_OVERLAPPED;

    UNUSED(list);
    
    out->result.type = WSA_IOCTL_INT;
    out->result.wsa_ioctl_request_u.req_int = 0;
    
    MAKE_CALL(out->retval = WSAGetOverlappedResult(in->s,
                                (LPWSAOVERLAPPED)overlapped,
                                out->bytes.bytes_len == 0 ? NULL :
                                (LPDWORD)(out->bytes.bytes_val),
                                in->wait,
                                out->flags.flags_len > 0 ?
                                (LPDWORD)(out->flags.flags_val) :
                                NULL));

    if (out->retval)
    {
        if (out->flags.flags_len > 0)
            out->flags.flags_val[0] =
                send_recv_flags_h2rpc(out->flags.flags_val[0]);

        convert_wsa_ioctl_result(in->code,
                                 overlapped->buffers[0].buf, &out->result);

        rpc_overlapped_free_memory(overlapped);
    }
}
)

/*-------------- WSAAsyncGetHostByAddr ------------------*/
TARPC_FUNC(wsa_async_get_host_by_addr, {},
{
    MAKE_CALL(out->retval = rcf_pch_mem_alloc(WSAAsyncGetHostByAddr(
                                IN_HWND, in->wmsg, in->addr.addr_val,
                                in->addrlen, addr_family_rpc2h(in->type),
                                rcf_pch_mem_get(in->buf), in->buflen)));
}
)

/*-------------- WSAAsyncGetHostByName ------------------*/
TARPC_FUNC(wsa_async_get_host_by_name, {},
{
    MAKE_CALL(out->retval = rcf_pch_mem_alloc(WSAAsyncGetHostByName(
                                IN_HWND, in->wmsg, in->name.name_val,
                                rcf_pch_mem_get(in->buf), in->buflen)));
}
)

/*-------------- WSAAsyncGetProtoByName -----------------*/
TARPC_FUNC(wsa_async_get_proto_by_name, {},
{
    MAKE_CALL(out->retval = rcf_pch_mem_alloc(WSAAsyncGetProtoByName(
                                IN_HWND, in->wmsg, in->name.name_val,
                                rcf_pch_mem_get(in->buf), in->buflen)));
}
)

/*-------------- WSAAsyncGetProtoByNumber ---------------*/
TARPC_FUNC(wsa_async_get_proto_by_number, {},
{
    MAKE_CALL(out->retval = rcf_pch_mem_alloc(WSAAsyncGetProtoByNumber(
                                IN_HWND, in->wmsg, in->number,
                                rcf_pch_mem_get(in->buf), in->buflen)));
}
)

/*-------------- WSAAsyncGetServByName ---------------*/
TARPC_FUNC(wsa_async_get_serv_by_name, {},
{
    MAKE_CALL(out->retval = rcf_pch_mem_alloc(WSAAsyncGetServByName(
                                IN_HWND, in->wmsg,
                                in->name.name_val, in->proto.proto_val,
                                rcf_pch_mem_get(in->buf), in->buflen)));
}
)

/*-------------- WSAAsyncGetServByPort ---------------*/
TARPC_FUNC(wsa_async_get_serv_by_port, {},
{
    MAKE_CALL(out->retval = rcf_pch_mem_alloc(WSAAsyncGetServByPort(
                                IN_HWND, in->wmsg, in->port,
                                in->proto.proto_val, 
                                rcf_pch_mem_get(in->buf), in->buflen)));
}
)
/*-------------- WSAJoinLeaf -----------------------------*/
TARPC_FUNC(wsa_join_leaf, {},
{
    QOS sqos;
    QOS *psqos;
    PREPARE_ADDR(in->addr, 0);

    if (in->sqos_is_null == TRUE)
        psqos = NULL;
    else
    {
        psqos = &sqos;
        memset(&sqos, 0, sizeof(sqos));
        flowspec_rpc2h(&sqos.SendingFlowspec, &in->sqos.sending);
        flowspec_rpc2h(&sqos.ReceivingFlowspec, &in->sqos.receiving);
        sqos.ProviderSpecific.buf =
            (char*)in->sqos.provider_specific_buf.provider_specific_buf_val;
        sqos.ProviderSpecific.len =
            in->sqos.provider_specific_buf.provider_specific_buf_len;
    }

    MAKE_CALL(out->retval = WSAJoinLeaf(in->s, a, in->addrlen,
                                       (LPWSABUF)
                                       rcf_pch_mem_get(in->caller_wsabuf),
                                       (LPWSABUF)
                                       rcf_pch_mem_get(in->callee_wsabuf),
                                       psqos, NULL, 
                                       join_leaf_flags_rpc2h(in->flags)));
}
)
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
        out->common._errno = TE_RC(TE_TA_WIN32, TE_EINVAL);
    }

    return TRUE;
}

/*------------ CreateIoCompletionPort() -------------------*/

TARPC_FUNC(create_io_completion_port,
{},
{
    HANDLE file_handle = NULL;

    UNUSED(list);

    file_handle = in->file_handle == -1 ?
                  INVALID_HANDLE_VALUE : (HANDLE)in->file_handle;

    MAKE_CALL(out->retval = (tarpc_int)CreateIoCompletionPort(file_handle,
                         (HANDLE)(in->existing_completion_port), 
                         (ULONG_PTR)in->completion_key,
                         (DWORD)in->number_of_concurrent_threads)
    );
}
)

/*------------ GetQueuedCompletionStatus() -------------------*/

TARPC_FUNC(get_queued_completion_status,
{},
{
    OVERLAPPED *overlapped = NULL;

    UNUSED(list);

    MAKE_CALL(out->retval = GetQueuedCompletionStatus(
        (HANDLE)(in->completion_port),
        (DWORD *)&out->number_of_bytes,
        (ULONG_PTR *)&out->completion_key,
        &overlapped, (DWORD)in->milliseconds)
    );

    if (overlapped != NULL)
        out->overlapped = (tarpc_overlapped)rcf_pch_mem_get_id(overlapped);
    else
        out->overlapped = (tarpc_overlapped)0;
}
)

/*------------ PostQueuedCompletionStatus() -------------------*/

TARPC_FUNC(post_queued_completion_status,
{},
{
    UNUSED(list);

    MAKE_CALL(out->retval = PostQueuedCompletionStatus(
        (HANDLE)(in->completion_port),
        (DWORD)in->number_of_bytes,
        (ULONG_PTR)in->completion_key,
        in->overlapped == 0 ? NULL :
            (HANDLE)rcf_pch_mem_get(in->overlapped))
    );
}
)

/*-------------- gettimeofday() --------------------------------*/
TARPC_FUNC(gettimeofday,
{
    COPY_ARG(tv);
},
{
    struct timeval  tv;

    if (out->tv.tv_len != 0)
        TARPC_CHECK_RC(timeval_rpc2h(out->tv.tv_val, &tv));

    if (out->common._errno != 0)
    {
        out->retval = -1;
    }
    else
    {
        MAKE_CALL(out->retval = 
                      gettimeofday(out->tv.tv_len == 0 ? NULL : &tv,
                                   NULL));
        if (out->tv.tv_len != 0)
            TARPC_CHECK_RC(timeval_h2rpc(&tv, out->tv.tv_val));
        if (TE_RC_GET_ERROR(out->common._errno) == TE_EH2RPC)
            out->retval = -1;
    }
}
)

#define MAX_CALLBACKS   1024

/** Completion callbacks registry */
static struct {
    const char                        *name;
    LPWSAOVERLAPPED_COMPLETION_ROUTINE callback;
} callback_registry[MAX_CALLBACKS] = { 
    { "default_completion_callback", default_completion_callback }, };

/**
 * Get address of completion callback.
 *
 * @param name  name of the callback
 *
 * @return Callback address
 */                          
LPWSAOVERLAPPED_COMPLETION_ROUTINE 
completion_callback_addr(const char *name)
{
    int i;
    
    if (name == NULL || *name == 0)
        return NULL;
    
    for (i = 0; 
         i < MAX_CALLBACKS && callback_registry[i].name != NULL; 
         i++)
    {
        if (strcmp(callback_registry[i].name, name) == 0)
            return callback_registry[i].callback;
    }
    
    ERROR("Failed to find completion callback for %s", name);
    
    return NULL;
}

/**
 * Register pair name:callback.
 *
 * @param name     symbolic name of the callback which may be passed in RPC
 * @param callback callback function
 *
 * @return Status code
 */
te_errno
completion_callback_register(const char *name, 
                             LPWSAOVERLAPPED_COMPLETION_ROUTINE callback)
{
    int i;
    
    if (name == NULL || *name == 0 || callback == NULL)
    {
        ERROR("Try to register completion callback "
              "with invalid name/address");
        return TE_RC(TE_TA_WIN32, TE_EINVAL);
    }
    
    for (i = 0; 
         i < MAX_CALLBACKS && callback_registry[i].name != NULL; 
         i++)
    {
        if (strcmp(callback_registry[i].name, name) == 0)
            return 0;
    }
    
    if (i == MAX_CALLBACKS)
    {
        ERROR("Too many callbacks are registered");
        return TE_RC(TE_TA_WIN32, TE_ENOMEM);
    }
    
    callback_registry[i].name = name;
    callback_registry[i].callback = callback;

    return 0;
}                             
