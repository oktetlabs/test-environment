/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Windows Test Agent
 *
 * RPC server implementation
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#include "tarpc_server.h"

#include "te_sockaddr.h"

LPFN_CONNECTEX            pf_connect_ex;
LPFN_DISCONNECTEX         pf_disconnect_ex;
LPFN_ACCEPTEX             pf_accept_ex;
LPFN_GETACCEPTEXSOCKADDRS pf_get_accept_ex_sockaddrs;
LPFN_TRANSMITFILE         pf_transmit_file;
LPFN_TRANSMITPACKETS      pf_transmit_packets;
LPFN_WSARECVMSG           pf_wsa_recvmsg;

void
wsa_func_handles_discover()
{
    GUID  guid_connect_ex = WSAID_CONNECTEX;
    GUID  guid_disconnect_ex = WSAID_DISCONNECTEX;
    GUID  guid_accept_ex = WSAID_ACCEPTEX;
    GUID  guid_get_accept_ex_sockaddrs = WSAID_GETACCEPTEXSOCKADDRS;
    GUID  guid_transmit_file = WSAID_TRANSMITFILE;
    GUID  guid_transmit_packets = WSAID_TRANSMITPACKETS;
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
            LOG_PRINT("Cannot retrieve %s pointer via WSAIoctl();"      \
                 " errno %d", #_func, _errno);                          \
        }                                                               \
    } while (0)

    DISCOVER_FUNC(connect_ex, CONNECTEX);
    DISCOVER_FUNC(disconnect_ex, DISCONNECTEX);
    DISCOVER_FUNC(accept_ex, ACCEPTEX);
    DISCOVER_FUNC(get_accept_ex_sockaddrs, GETACCEPTEXSOCKADDRS);
    DISCOVER_FUNC(transmit_packets, TRANSMITPACKETS);
    DISCOVER_FUNC(transmit_file, TRANSMITFILE);
    DISCOVER_FUNC(wsa_recvmsg, WSARECVMSG);

#undef DISCOVER_FUNC

    closesocket(s);
}

#ifdef WINDOWS
/** Calculate the auxiliary buffer length for msghdr */
static inline int
calculate_msg_controllen(struct tarpc_msghdr *rpc_msg)
{
    unsigned int i;
    int          len = 0;

    for (i = 0; i < rpc_msg->msg_control.msg_control_len; i++)
        len += WSA_CMSG_SPACE(rpc_msg->msg_control.msg_control_val[i].
                              data.data_len);
    return len;
}
#endif

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
    {"te_errno", sizeof(te_errno)},
    {"size_t", sizeof(size_t)},
    {"socklen_t", sizeof(socklen_t)},
    {"struct timeval", sizeof(struct timeval)},
    {"struct linger", sizeof(struct linger)},
    {"struct ip_mreq", sizeof(struct ip_mreq)},
    {"struct in_addr", sizeof(struct in_addr)},
    {"struct sockaddr", sizeof(struct sockaddr)},
    {"struct sockaddr_in", sizeof(struct sockaddr_in)},
    {"struct sockaddr_in6", sizeof(struct sockaddr_in6)},
    {"struct sockaddr_storage", sizeof(struct sockaddr_storage)},
    {"WSAPROTOCOL_INFOA", sizeof(WSAPROTOCOL_INFOA)},
    {"WSAPROTOCOL_INFOW", sizeof(WSAPROTOCOL_INFOW)},
    {"QOS", sizeof(QOS)}
#if 0
    {"struct ip_mreqn", sizeof(struct ip_mreqn)}
#endif
};

/*-------------- get_sizeof() ---------------------------------*/
int32_t
get_sizeof(tarpc_get_sizeof_in *in)
{
    uint32_t i;

    if (in->typename == NULL)
    {
        ERROR("Type name not specified");
        return FALSE;
    }

    if (in->typename[0] == '*')
    {
        return sizeof(void *);
    }

    for (i = 0; i < sizeof(type_info) / sizeof(type_info_t); i++)
    {
        if (strcmp(in->typename, type_info[i].type_name) == 0)
        {
            return type_info[i].type_size;
        }
    }

    ERROR("Unknown type (%s)", in->typename);
    return -1;
}

TARPC_FUNC(get_sizeof, {},
{
    MAKE_CALL(out->size = get_sizeof(in));
}
)

/*-------------- protocol_info_cmp() ---------------------------------*/
bool_t
_protocol_info_cmp_1_svc(tarpc_protocol_info_cmp_in *in,
                         tarpc_protocol_info_cmp_out *out,
                         struct svc_req *rqstp)
{
    WSAPROTOCOL_INFO *info1, *info2;

    int protocol_len = 0;
    int protocol_widelen = 0;

    char info1_char[sizeof(info1->szProtocol) * 2];
    char info2_char[sizeof(info1->szProtocol) * 2];

    UNUSED(rqstp);

    info1 = (WSAPROTOCOL_INFO *)(in->buf1.buf1_val);
    info2 = (WSAPROTOCOL_INFO *)(in->buf2.buf2_val);

    if (in->is_wide1)
        protocol_widelen = sizeof(info1->szProtocol) / sizeof(wchar_t);
    else if (in->is_wide2)
        protocol_widelen = sizeof(info2->szProtocol) / sizeof(wchar_t);

    if (!in->is_wide1)
        protocol_len = sizeof(info1->szProtocol);
    else if (!in->is_wide2)
        protocol_len = sizeof(info2->szProtocol);
    else
        protocol_len = protocol_widelen;

    out->retval = TRUE;

    if ( (info1->dwServiceFlags1 != info2->dwServiceFlags1) ||
       (info1->dwServiceFlags2 != info2->dwServiceFlags2) ||
       (info1->dwServiceFlags3 != info2->dwServiceFlags3) ||
       (info1->dwServiceFlags4 != info2->dwServiceFlags4) ||
       (info1->dwProviderFlags != info2->dwProviderFlags) ||
       (memcmp(&(info1->ProviderId), &(info2->ProviderId),
                  sizeof(GUID)) != 0) ||
       (info1->dwCatalogEntryId != info2->dwCatalogEntryId) ||
       (memcmp(&(info1->ProtocolChain), &(info2->ProtocolChain),
               sizeof(WSAPROTOCOLCHAIN)) != 0) ||
       (info1->iVersion != info2->iVersion) ||
       (info1->iAddressFamily != info2->iAddressFamily) ||
       (info1->iMaxSockAddr != info2->iMaxSockAddr) ||
       (info1->iSocketType != info2->iSocketType) ||
       (info1->iMinSockAddr != info2->iMinSockAddr) ||
       (info1->iProtocol != info2->iProtocol) ||
       (info1->iProtocolMaxOffset != info2->iProtocolMaxOffset) ||
       (info1->iNetworkByteOrder != info2->iNetworkByteOrder) ||
       (info1->iSecurityScheme != info2->iSecurityScheme) ||
       (info1->dwMessageSize != info2->dwMessageSize) )
        out->retval = FALSE;

    if (in->is_wide1 && !in->is_wide2)
    {
        WideCharToMultiByte(CP_ACP, 0, (wchar_t *)&(info1->szProtocol),
                            protocol_widelen, info1_char,
                            protocol_len, NULL, NULL);
        strncpy(info2_char, info2->szProtocol, protocol_len);
    }
    if (in->is_wide2 && !in->is_wide1)
    {
        WideCharToMultiByte(CP_ACP, 0, (wchar_t *)&(info2->szProtocol),
                            protocol_widelen, info2_char,
                            protocol_len, NULL, NULL);
        strncpy(info1_char, info1->szProtocol, protocol_len);
    }
    if (!in->is_wide1 && !in->is_wide2)
    {
        if (strcmp(info1->szProtocol, info2->szProtocol) != 0)
            out->retval = FALSE;
    }
    else
    {
        if (strcmp(info1_char, info2_char) != 0)
             out->retval = FALSE;
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
 * @param name          RPC server name
 * @param pid           location for process identifier
 * @param inherit       if TRUE, inherit file handles
 * @param net_init      if TRUE, initialize network
 *
 * @return Status code
 */
te_errno
create_process_rpc_server(const char *name, int32_t *pid, int flags)
{
    char  cmdline[256];
    char *tmp;
    char *val;

    const char *postfix[] = {
        "rpcserver64 %s %s",
        "rpcserver32 %s %s",
        "rpcserver %s %s",
        "ta rpcserver %s %s"
    };

    int i = 0;

    PROCESS_INFORMATION info;
    STARTUPINFO         si;
    SYSTEM_INFO         sys_info;

    strcpy(cmdline, GetCommandLine());
    if ((tmp = strstr(cmdline, " ")) == NULL)
    {
        ERROR("Failed to obtain pathname of the executable");
        return TE_RC(TE_TA_WIN32, TE_EINVAL);
    }
    *tmp = 0;
    if ((tmp = strrchr(cmdline, '/')) == NULL &&
        (tmp = strrchr(cmdline, '\\')) == NULL)
    {
        ERROR("Unexpected pathname of the executable: %s", cmdline);
        return TE_RC(TE_TA_WIN32, TE_EINVAL);
    }
    tmp++;

    memset(&sys_info, 0, sizeof(sys_info));
    GetNativeSystemInfo(&sys_info);

    if (sys_info.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL)
        i = 1;
    else if ((val =  getenv_reliable("TE_WIN32_TA")) != NULL &&
        strcmp(val, "yes") == 0)
    {
        i = 1;
    }

    for (; i < (int)TE_ARRAY_LEN(postfix); i++)
    {
        sprintf(tmp, postfix[i], name,
                (flags & RCF_RPC_SERVER_GET_NET_INIT) ? "net_init" : "");
        memset(&si, 0, sizeof(si));
        si.cb = sizeof(si);

        if (CreateProcess(NULL, cmdline, NULL, NULL,
                          !!(flags & RCF_RPC_SERVER_GET_INHERIT),
                          0, NULL, NULL,
                          &si, &info))
        {
            *pid = info.dwProcessId;
            return 0;
        }
    }

    ERROR("CreateProcess() failed with error %d", GetLastError());
    return RPC_ERRNO;
}

/*-------------- create_process() ---------------------------------*/
TARPC_FUNC(create_process, {},
{
    MAKE_CALL(out->common._errno =
                  create_process_rpc_server(in->name.name_val,
                                            &out->pid, flags));
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
    te_errno err;

    MAKE_CALL(err = thread_cancel(in->tid));
    out->common._errno = err;
    out->retval = err != 0 ? -1 : 0;
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

/*-------------- WSAStartup() ------------------------------*/
TARPC_FUNC(wsa_startup, {},
{
    WSADATA wsaData;
    WORD version;
    version = MAKEWORD(2, 0);

    MAKE_CALL(out->retval = WSAStartup(version, &wsaData));

})


/*-------------- WSACleanup() ------------------------------*/
TARPC_FUNC(wsa_cleanup, {},
{
    MAKE_CALL(out->retval = WSACleanup());
})



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
    PREPARE_ADDR(my_addr, in->addr, 0);
    MAKE_CALL(out->retval = bind(in->fd, my_addr, my_addrlen));
}
)

/*-------------- connect() ------------------------------*/

TARPC_FUNC(connect, {},
{
    PREPARE_ADDR(serv_addr, in->addr, 0);

    MAKE_CALL(out->retval = connect(in->fd, serv_addr, serv_addrlen));
}
)

/*-------------- ConnectEx() ------------------------------*/
TARPC_FUNC(connect_ex,
{
    COPY_ARG(len_sent);
},
{
    PREPARE_ADDR(serv_addr, in->addr, 0);
    MAKE_CALL(out->retval = (*pf_connect_ex)(in->fd,
                                serv_addr, serv_addrlen,
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
    PREPARE_ADDR(addr, out->addr,
                 out->len.len_len == 0 ? 0 : *out->len.len_val);

    MAKE_CALL(out->retval = accept(in->fd, addr,
                                   out->len.len_len == 0 ? NULL :
                                       (int *)(out->len.len_val)));

    sockaddr_output_h2rpc(addr, addrlen,
                          out->len.len_len == 0 ? 0 : *(out->len.len_val),
                          &(out->addr));
}
)

/*-------------- WSAAccept() ------------------------------*/

typedef struct accept_cond {
     unsigned short port;
     int            verdict;
     int            timeout;
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

    SleepEx(cond->timeout, TRUE);

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

    PREPARE_ADDR(addr, out->addr,
                 out->len.len_len == 0 ? 0 : *out->len.len_val);

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
            cond[i].timeout = in->cond.cond_val[i].timeout;
        }
    }

    MAKE_CALL(out->retval = WSAAccept(in->fd, addr,
                                      out->len.len_len == 0 ? NULL :
                                          (int *)(out->len.len_val),
                                      (LPCONDITIONPROC)accept_callback,
                                      (DWORD)cond));

    sockaddr_output_h2rpc(addr, addrlen,
                          out->len.len_len == 0 ? 0 : *(out->len.len_val),
                          &(out->addr));

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
        sockaddr_output_h2rpc(la,
                              out->l_sa_len.l_sa_len_val == NULL ? 0 :
                                  *(out->l_sa_len.l_sa_len_val),
                              out->l_sa_len.l_sa_len_val == NULL ? 0 :
                                  *(out->l_sa_len.l_sa_len_val),
                              &(out->laddr));
    }
    if (!in->r_sa_null)
    {
        sockaddr_output_h2rpc(ra,
                              out->r_sa_len.r_sa_len_val == NULL ? 0 :
                                  *(out->r_sa_len.r_sa_len_val),
                              out->r_sa_len.r_sa_len_val == NULL ? 0 :
                                  *(out->r_sa_len.r_sa_len_val),
                              &(out->raddr));
    }
}
)

/*-------------- TransmitPackets() -------------------------*/
TARPC_FUNC(transmit_packets, {},
{
    TRANSMIT_PACKETS_ELEMENT   *transmit_buffers;
    rpc_overlapped             *overlapped = IN_OVERLAPPED;
    rpc_overlapped              tmp;
    unsigned int                i;

    if (overlapped == NULL)
    {
        memset(&tmp, 0, sizeof(tmp));
        overlapped = &tmp;
    }

    transmit_buffers = (TRANSMIT_PACKETS_ELEMENT *)
                       calloc(in->packet_array.packet_array_len,
                              sizeof(TRANSMIT_PACKETS_ELEMENT));
    memset(transmit_buffers, 0,
           in->packet_array.packet_array_len *
           sizeof(TRANSMIT_PACKETS_ELEMENT));

    overlapped->buffers = calloc(in->packet_array.packet_array_len,
                                 sizeof(WSABUF));

    for (i = 0; i < in->packet_array.packet_array_len; i++)
    {
        switch (in->packet_array.packet_array_val[i].packet_src.type)
        {
            case TARPC_TP_MEM:
                transmit_buffers[i].dwElFlags = TP_ELEMENT_MEMORY;
                transmit_buffers[i].pBuffer =
                overlapped->buffers[i].buf = in->packet_array.
                                       packet_array_val[i].packet_src.
                                       tarpc_transmit_packet_source_u.
                                       buf.buf_val;
                overlapped->buffers[i].len = in->packet_array.
                                       packet_array_val[i].packet_src.
                                       tarpc_transmit_packet_source_u.
                                       buf.buf_len;
                in->packet_array.packet_array_val[i].packet_src.
                    tarpc_transmit_packet_source_u.buf.buf_val = NULL;
                in->packet_array.packet_array_val[i].packet_src.
                    tarpc_transmit_packet_source_u.buf.buf_len = 0;

                break;

            case TARPC_TP_FILE:
            {
                overlapped->buffers[i].buf = NULL;
                overlapped->buffers[i].len = 0;
                transmit_buffers[i].dwElFlags = TP_ELEMENT_FILE;
                transmit_buffers[i].hFile =
                    (HANDLE)in->packet_array.packet_array_val[i].packet_src.
                    tarpc_transmit_packet_source_u.file_data.file;
                transmit_buffers[i].nFileOffset.QuadPart =
                    in->packet_array.packet_array_val[i].packet_src.
                    tarpc_transmit_packet_source_u.file_data.offset;
                break;
             }

            default:
                ERROR("Incorrect data source: %d",
                      in->packet_array.packet_array_val[i].packet_src.type);
                out->common._errno = TE_RC(TE_TA_WIN32, TE_EINVAL);
        }
        transmit_buffers[i].cLength =
            in->packet_array.packet_array_val[i].length;
    }
    MAKE_CALL( out->retval =
                   (*pf_transmit_packets)((SOCKET)in->s, transmit_buffers,
                                          in->packet_array.packet_array_len,
                                          in->send_size,
                                          (LPWSAOVERLAPPED)in->overlapped,
                                          in->flags); );
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

/*-------------- write_at_offset() -------------------*/

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
    PREPARE_ADDR(from, out->from, out->fromlen.fromlen_len == 0 ? 0 :
                                      *out->fromlen.fromlen_val);


    INIT_CHECKED_ARG(out->buf.buf_val, out->buf.buf_len, in->len);

    MAKE_CALL(out->retval = recvfrom(in->fd, out->buf.buf_val, in->len,
                                     send_recv_flags_rpc2h(in->flags),
                                     from,
                                     out->fromlen.fromlen_len == 0 ? NULL :
                                     (int *)(out->fromlen.fromlen_val)));

    sockaddr_output_h2rpc(from, fromlen,
                          out->fromlen.fromlen_len == 0 ? 0 :
                              *(out->fromlen.fromlen_val),
                          &(out->from));
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

/** This variable is used to minimize side-effects on tests
 *  which do not call lseek()
 */
static te_bool lseek_has_been_called = FALSE;

static void
set_overlapped_filepos(WSAOVERLAPPED *ovr, HANDLE handle)
{
    if (lseek_has_been_called)
    {
        ovr->Offset = SetFilePointer(handle, 0, NULL, FILE_CURRENT);
        if (ovr->Offset + 1 == 0)
        {
             WARN("Unable to get file position");
             ovr->Offset = 0;
        }
    }
}

/*-------------- read() ------------------------------*/

TARPC_FUNC(read,
{
    COPY_ARG(buf);
},
{
    DWORD rc;

    WSAOVERLAPPED overlapped;

    memset(&overlapped, 0, sizeof(overlapped));

    overlapped.hEvent = WSACreateEvent();
    set_overlapped_filepos(&overlapped, (HANDLE)in->fd);

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
            goto finish;
        }

        if (GetOverlappedResult((HANDLE)(in->fd), &overlapped,
                                     &rc, 1) == 0)
        {
            out->common._errno = RPC_ERRNO;
            ERROR("read(): GetOverlappedResult() failed with error %r (%d)",
                  out->common._errno, GetLastError());
            rc = -1;
            goto finish;
        }

        out->common._errno = RPC_ERRNO;
    }
finish:
    WSACloseEvent(overlapped.hEvent);
    out->retval = rc;
}
)

/*-------------- write() ------------------------------*/

TARPC_FUNC(write, {},
{
    DWORD rc;

    WSAOVERLAPPED overlapped;

    memset(&overlapped, 0, sizeof(overlapped));

    overlapped.hEvent = WSACreateEvent();
    set_overlapped_filepos(&overlapped, (HANDLE)in->fd);

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
            goto finish;
        }

        if (GetOverlappedResult((HANDLE)(in->fd), &overlapped,
                                     &rc, 1) == 0)
        {
            out->common._errno = RPC_ERRNO;
            ERROR("write(): GetOverlappedResult() failed with error %r "
                  "(%d)", out->common._errno, GetLastError());
            rc = -1;
            goto finish;
        }

        out->common._errno = RPC_ERRNO;
    }
finish:
    WSACloseEvent(overlapped.hEvent);
    out->retval = rc;
}
)

/*-------------- readbuf() ------------------------------*/

TARPC_FUNC(readbuf,
{},
{
    DWORD rc;

    WSAOVERLAPPED overlapped;

    memset(&overlapped, 0, sizeof(overlapped));

    overlapped.hEvent = WSACreateEvent();
    set_overlapped_filepos(&overlapped, (HANDLE)in->fd);

    MAKE_CALL(out->retval = ReadFile((HANDLE)(in->fd),
                           (char *)rcf_pch_mem_get(in->buf) + in->off,
                           in->len, &rc, &overlapped));

    if (out->retval == 0)
    {
        if (out->common._errno != RPC_E_IO_PENDING)
        {
            INFO("read(): ReadFile() failed with error %r (%d)",
                  out->common._errno, GetLastError());
            rc = -1;
            goto finish;
        }

        if (GetOverlappedResult((HANDLE)(in->fd), &overlapped,
                                     &rc, 1) == 0)
        {
            out->common._errno = RPC_ERRNO;
            ERROR("read(): GetOverlappedResult() failed with error %r (%d)",
                  out->common._errno, GetLastError());
            rc = -1;
            goto finish;
        }

        out->common._errno = RPC_ERRNO;
    }
finish:
    WSACloseEvent(overlapped.hEvent);
    out->retval = rc;
}
)

/*-------------- write() ------------------------------*/

TARPC_FUNC(writebuf, {},
{
    DWORD rc;

    WSAOVERLAPPED overlapped;

    memset(&overlapped, 0, sizeof(overlapped));

    overlapped.hEvent = WSACreateEvent();
    set_overlapped_filepos(&overlapped, (HANDLE)in->fd);

    MAKE_CALL(out->retval = WriteFile((HANDLE)(in->fd),
                                      rcf_pch_mem_get(in->buf) + in->off,
                                      in->len, &rc, &overlapped));

    if (out->retval == 0)
    {
        if (out->common._errno != RPC_E_IO_PENDING)
        {
            INFO("write(): WriteFile() failed with error %r (%d)",
                  out->common._errno, GetLastError());
            rc = -1;
            goto finish;
        }

        if (GetOverlappedResult((HANDLE)(in->fd), &overlapped,
                                     &rc, 1) == 0)
        {
            out->common._errno = RPC_ERRNO;
            ERROR("write(): GetOverlappedResult() failed with error %r "
                  "(%d)", out->common._errno, GetLastError());
            rc = -1;
            goto finish;
        }

        out->common._errno = RPC_ERRNO;
    }
finish:
    WSACloseEvent(overlapped.hEvent);
    out->retval = rc;
}
)


/*-------------- lseek() ------------------------------*/

TARPC_FUNC(lseek, {},
{
    int mode;

    lseek_has_been_called = TRUE;
    switch (in->mode)
    {
        case RPC_SEEK_SET:
            mode = FILE_BEGIN;
            break;
        case RPC_SEEK_CUR:
            mode = FILE_CURRENT;
            break;
        case RPC_SEEK_END:
            mode = FILE_END;
            break;
        default:
            ERROR("Internal error: Invalid seek mode");
            mode = 0;
    }
    MAKE_CALL(out->retval = SetFilePointer((HANDLE)in->fd, in->pos, NULL,
              mode));
})

/*-------------- fsync() ------------------------------*/

TARPC_FUNC(fsync, {},
{
    MAKE_CALL(out->retval = !FlushFileBuffers((HANDLE)in->fd));
})


/*-------------- ReadFile() ------------------------------*/

TARPC_FUNC(read_file,
{
    COPY_ARG(received);
    COPY_ARG(buf);
},
{
    rpc_overlapped *overlapped = IN_OVERLAPPED;

    if (overlapped == NULL)
    {
        ERROR("NULL overlapped is passed to the ReadFile()");
        out->common._errno = TE_RC(TE_TA_WIN32, TE_EINVAL);
        goto finish;
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

/*-------------- ReadFileEx() ------------------------------*/

TARPC_FUNC(read_file_ex, {},
{
    rpc_overlapped *overlapped = IN_OVERLAPPED;

    if (overlapped == NULL)
    {
        ERROR("NULL overlapped is passed to the ReadFileEx()");
        out->common._errno = TE_RC(TE_TA_WIN32, TE_EINVAL);
        goto finish;
    }

    if (buf2overlapped(overlapped, in->buf.buf_len, in->buf.buf_val) != 0)
    {
        out->common._errno = TE_RC(TE_TA_WIN32, TE_ENOMEM);
        goto finish;
    }
    in->buf.buf_val = NULL;
    in->buf.buf_len = 0;

    MAKE_CALL(out->retval = ReadFileEx((HANDLE)(in->fd),
                                       overlapped->buffers[0].buf,
                                       in->len,
                                       (LPWSAOVERLAPPED)overlapped,
                                       IN_FILE_CALLBACK));

    if (!out->retval)
        rpc_overlapped_free_memory(overlapped);

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

    if (overlapped == NULL)
    {
        ERROR("NULL overlapped is passed to the WriteFile()");
        out->common._errno = TE_RC(TE_TA_WIN32, TE_EINVAL);
        goto finish;
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

/*-------------- WriteFileEx() ------------------------------*/

TARPC_FUNC(write_file_ex, {},
{
    rpc_overlapped *overlapped = IN_OVERLAPPED;

    if (overlapped == NULL)
    {
        ERROR("NULL overlapped is passed to the WriteFileEx()");
        out->common._errno = TE_RC(TE_TA_WIN32, TE_EINVAL);
        goto finish;
    }

    if (buf2overlapped(overlapped, in->buf.buf_len, in->buf.buf_val) != 0)
    {
        out->common._errno = TE_RC(TE_TA_WIN32, TE_ENOMEM);
        goto finish;
    }
    in->buf.buf_val = NULL;
    in->buf.buf_len = 0;

    MAKE_CALL(out->retval = WriteFileEx((HANDLE)(in->fd),
                                        overlapped->buffers[0].buf,
                                        in->len,
                                        (LPWSAOVERLAPPED)overlapped,
                                        IN_FILE_CALLBACK));

    if (!out->retval)
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
    PREPARE_ADDR(to, in->to, 0);

    INIT_CHECKED_ARG(in->buf.buf_val, in->buf.buf_len, in->len);

    MAKE_CALL(out->retval = sendto(in->fd, in->buf.buf_val, in->len,
                                   send_recv_flags_rpc2h(in->flags),
                                   to, tolen));
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
    PREPARE_ADDR(name, out->addr,
                 out->len.len_len == 0 ? 0 : *out->len.len_val);

    MAKE_CALL(out->retval = getsockname(in->fd, name,
                                        out->len.len_len == 0 ? NULL :
                                        (int *)(out->len.len_val)));

    sockaddr_output_h2rpc(name, namelen,
                          out->len.len_len == 0 ? 0 : *(out->len.len_val),
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

    MAKE_CALL(out->retval = getpeername(in->fd, name,
                                        out->len.len_len == 0 ? NULL :
                                        (int *)(out->len.len_val)));

    sockaddr_output_h2rpc(name, namelen,
                          out->len.len_len == 0 ? 0 : *(out->len.len_val),
                          &(out->addr));
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
        out->common._errno = RPC_ERRNO;
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
                                           in->raw_optval.raw_optval_val,
                                           in->raw_optlen));
    }
    else
    {
        option_value   *in_optval = in->optval.optval_val;
        char           *opt;
        int             optlen;
        HANDLE          handle;

        struct linger           linger;
        struct in_addr          addr;
        struct timeval          tv;
        struct ip_mreq          mreq;
        struct ipv6_mreq        mreq6;
        struct in6_addr         addr6;

        if (in->optname == RPC_SO_SNDTIMEO ||
            in->optname == RPC_SO_RCVTIMEO)
        {
            static int optval;
            optval =
                in_optval->option_value_u.opt_timeval.tv_sec * 1000 +
                in_optval->option_value_u.opt_timeval.tv_usec / 1000;

            opt = (char *)&optval;
            optlen = sizeof(int);
        }
        else switch (in_optval->opttype)
        {
            case OPT_INT:
            {
                opt = (char *)&(in_optval->option_value_u.opt_int);
                optlen = sizeof(int);
                break;
            }

            case OPT_LINGER:
            {
                opt = (char *)&linger;
                linger.l_onoff =
                    in_optval->option_value_u.opt_linger.l_onoff;
                linger.l_linger =
                    in_optval->option_value_u.opt_linger.l_linger;
                optlen = sizeof(linger);
                break;
            }

            case OPT_IPADDR:
            {
                opt = (char *)&addr;

                memcpy(&addr,
                       &(in_optval->option_value_u.opt_ipaddr),
                       sizeof(struct in_addr));
                addr.s_addr = htonl(addr.s_addr);
                optlen = sizeof(addr);
                break;
            }

            case OPT_TIMEVAL:
            {
                opt = (char *)&tv;

                tv.tv_sec = in_optval->option_value_u.opt_timeval.tv_sec;
                tv.tv_usec = in_optval->option_value_u.opt_timeval.tv_usec;
                optlen = sizeof(tv);
                break;
            }

            case OPT_HANDLE:
            {
                opt = (char *)&handle;
                handle = (HANDLE)(in_optval->option_value_u.opt_handle);
                optlen = sizeof(handle);
                break;
            }

            case OPT_MREQN:
            case OPT_MREQ:
            {
                opt = (char *)&mreq;
                memcpy(&mreq.imr_multiaddr,
                       &(in_optval->option_value_u.opt_mreqn.imr_multiaddr),
                       sizeof(struct in_addr));
                mreq.imr_multiaddr.s_addr
                    = htonl(mreq.imr_multiaddr.s_addr);
                memcpy(&mreq.imr_interface,
                       &(in_optval->option_value_u.opt_mreqn.imr_address),
                       sizeof(struct in_addr));
                mreq.imr_interface.s_addr
                    = htonl(mreq.imr_interface.s_addr);

                optlen = sizeof(mreq);
                break;
            }

            case OPT_MREQ6:
            {
                opt = (char *)&mreq6;

                memcpy(&mreq6.ipv6mr_multiaddr,
                       &(in_optval->option_value_u.opt_mreq6.
                             ipv6mr_multiaddr), sizeof(struct in6_addr));
                mreq6.ipv6mr_interface =
                    in_optval->option_value_u.opt_mreq6.ipv6mr_ifindex;
                optlen = sizeof(mreq6);
                break;
            }

            case OPT_IPADDR6:
            {
                opt = (char *)&addr6;

                memcpy(&addr6, &in_optval->option_value_u.opt_ipaddr6,
                       sizeof(struct in6_addr));
                optlen = sizeof(addr6);
                break;
            }

            default:
                ERROR("incorrect option type %d is received",
                      in_optval->opttype);
                out->common._errno = TE_RC(TE_TA_WIN32, TE_EINVAL);
                out->retval = -1;
                goto finish;
                break;
        }

        INIT_CHECKED_ARG(opt, optlen, 0);

        MAKE_CALL(out->retval = setsockopt(in->s,
                                           socklevel_rpc2h(in->level),
                                           sockopt_rpc2h(in->optname),
                                           opt, optlen));
    }
    finish:
    ;
}
)

/*-------------- getsockopt() ------------------------------*/

TARPC_FUNC(getsockopt,
{
    COPY_ARG(optval);
    COPY_ARG(raw_optval);
    COPY_ARG(raw_optlen);
},
{
    if (out->optval.optval_val == NULL)
    {
        MAKE_CALL(out->retval =
                      getsockopt(in->s, socklevel_rpc2h(in->level),
                                 sockopt_rpc2h(in->optname),
                                 out->raw_optval.raw_optval_val,
                                 (int *)(out->raw_optlen.raw_optlen_val)));
    }
    else
    {
        int optlen_in = 0;
        int optlen_out = 0;

        /* Assume that this size is large enough */
        char opt[sizeof(WSAPROTOCOL_INFOW)];

        memset(opt, 0, sizeof(opt));

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

            case OPT_MREQ:
            case OPT_MREQN:
                optlen_in = optlen_out = sizeof(struct ip_mreq);
                break;

            case OPT_MREQ6:
                optlen_in = optlen_out = sizeof(struct ipv6_mreq);
                break;

            default:
                ERROR("incorrect option type %d is received",
                      out->optval.optval_val[0].opttype);
                break;
        }

        memset(opt, 0, sizeof(opt));
        INIT_CHECKED_ARG(opt, sizeof(opt), optlen_in);

        MAKE_CALL(out->retval =
                      getsockopt(in->s, socklevel_rpc2h(in->level),
                                 sockopt_rpc2h(in->optname),
                                 opt, &optlen_out));

        /*
         * For such option as IP_MULTICAST_IF adjust OPT_MREQ type
         * to OPT_IPADDR
         */
        if ((out->optval.optval_val[0].opttype == OPT_MREQ ||
             out->optval.optval_val[0].opttype == OPT_MREQN) &&
            optlen_out == sizeof(struct in_addr))
        {
            out->optval.optval_val[0].opttype = OPT_IPADDR;
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
                out->optval.optval_val[0].
                         option_value_u.opt_ipaddr =
                        ntohl(out->optval.optval_val[0].
                                 option_value_u.opt_ipaddr);
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

/*-------------- uname() --------------------------------*/

#define PUT_STR(_dst, _field)                                       \
        do {                                                        \
            out->buf._dst._dst##_val = strdup(_field);              \
            if (out->buf._dst._dst##_val == NULL)                   \
            {                                                       \
                ERROR("Failed to duplicate string '%s'",            \
                      _field);                                      \
                out->common._errno = TE_RC(TE_TA_WIN32, TE_ENOMEM); \
                goto finish;                                        \
            }                                                       \
            out->buf._dst._dst##_len =                              \
                strlen(out->buf._dst._dst##_val) + 1;               \
        } while (0)

TARPC_FUNC(uname, {},
{
    SYSTEM_INFO sysinfo;
    OSVERSIONINFO verinfo;
    char buf[100];
    unsigned int buf_len = 100;

    UNUSED(in);

    out->retval = 0;
    GetNativeSystemInfo(&sysinfo);
    if (sysinfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL)
    {
        PUT_STR(machine,"i686");
    }
    else if (sysinfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64)
    {
        PUT_STR(machine,"amd64");
    }
    else
    {
        RING("Unsupported CPU architecture: %d",
             sysinfo.wProcessorArchitecture);
        out->common._errno = TE_RC(TE_TA_WIN32, TE_ENOMEM);
        goto finish;
    }

    memset(&verinfo, 0, sizeof(OSVERSIONINFO));
    verinfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if (!GetVersionEx(&verinfo))
    {
        RING("GetVersionEx failed, err=%d", GetLastError());
        goto finish;
    }
    sprintf(buf, "WINNT_%d.%d", (int)verinfo.dwMajorVersion,
            (int)verinfo.dwMinorVersion);
    PUT_STR(release, buf);
    sprintf(buf, "%d.%d", (int)verinfo.dwMajorVersion,
            (int)verinfo.dwMinorVersion);
    PUT_STR(osversion, buf);

    PUT_STR(sysname, "win");

    if (!GetComputerNameA(buf, (DWORD*)&buf_len))
    {
        RING("GetComputerName failed, err=%d", GetLastError());
        goto finish;
    }
    PUT_STR(nodename, buf);

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
    char *buf;

    uint64_t sent = 0;

    int size = rand_range(in->size_min, in->size_max);
    int delay = rand_range(in->delay_min, in->delay_max);

    time_t start;
    time_t now;
#ifdef TA_DEBUG
    uint64_t control = 0;
#endif

    if ((buf = malloc(in->size_max)) == NULL)
    {
        ERROR("Out of memory");
        return -1;
    }

    if (in->size_min > in->size_max || in->delay_min > in->delay_max)
    {
        ERROR("Incorrect size of delay parameters");
        return -1;
    }

    memset(buf, 0xAB, in->size_max);

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
            free(buf);
            return -1;
        }

        if (len < size)
        {
            if (in->ignore_err)
                continue;
            ERROR("send() returned %d instead %d in simple_sender()",
                  len, size);
            free(buf);
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

    free(buf);
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
    char           *buf;
    fd_set          set;
    int             rc;
    ssize_t         len;
    struct timeval  tv;
    int err = 0;

    time_t          start;
    time_t          now;

    out->bytes = 0;

    RING("%s() started", __FUNCTION__);

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

        rc = select(in->s + 1, &set, NULL, NULL, &tv);
        if (rc < 0)
        {
            err = GetLastError();
            ERROR("select() failed in simple_receiver(): errno %d", err);
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

        len = recv(in->s, buf, MAX_PKT, 0);
        if (len < 0)
        {
            err = GetLastError();
            ERROR("recv() failed in simple_receiver(): errno %d", err);
            free(buf);
            return -1;
        }
        if (len == 0)
        {
            break;
        }

        if (out->bytes == 0)
            RING("First %d bytes are received", len);
        out->bytes += len;
    }

    free(buf);
    RING("simple_receiver() stopped, received %llu bytes",
         out->bytes);
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

/*-------------- WSACreateEvent and set lower bit----------*/

TARPC_FUNC(create_event_with_bit, {},
{
    UNUSED(list);
    UNUSED(in);
    out->retval = rcf_pch_mem_alloc(
             (void*)(((unsigned int)WSACreateEvent())|1));
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
    out->common._errno = RPC_ERRNO;
}
)

/*-------------- WSAEnumNetworkEvents ----------------------------*/

TARPC_FUNC(enum_network_events,
{
    COPY_ARG(events);
},
{
    WSANETWORKEVENTS  events_occured;
    uint32_t i;
    memset(&events_occured, 0, sizeof(events_occured));
    UNUSED(list);

    out->retval = WSAEnumNetworkEvents(in->fd, IN_HEVENT,
                                       out->events.events_len == 0 ? NULL :
                                       &events_occured);
    if (out->events.events_len != 0)
    {

        out->events.events_val[0].network_events =
            network_event_h2rpc(events_occured.lNetworkEvents);
        for(i = 0; i < 10; i++)
        {
            out->events.events_val[0].error_code[i] =
            win_rpc_errno(events_occured.iErrorCode[i]);
        }
    }
}
)
/*-------------- CreateWindow ----------------------------*/

static LRESULT CALLBACK
message_callback(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg > WM_USER)
        LOG_PRINT("Unexpected message %d is received", uMsg - WM_USER);

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
        wcex.hInstance = GetModuleHandle(NULL);
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
                                 GetModuleHandle(NULL), NULL));
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
    out->common._errno = RPC_ERRNO;
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
        out->common._errno = 0;
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

void CALLBACK
default_file_completion_callback(DWORD error, DWORD bytes,
                                 LPOVERLAPPED overlapped)
{
    default_completion_callback(error, bytes, overlapped, 0);
}

/** Dummy callback */
void CALLBACK
empty_file_completion_callback(DWORD error, DWORD bytes,
                               LPOVERLAPPED overlapped)
{
    UNUSED(error);
    UNUSED(bytes);
    UNUSED(overlapped);
}

TARPC_FUNC(completion_callback, {},
{
    UNUSED(list);
    UNUSED(in);

    completion_callback_register("default_completion_callback",
                                 default_completion_callback);

    completion_callback_register("default_file_completion_callback",
                                 default_file_completion_callback);

    completion_callback_register("empty_file_completion_callback",
                                 default_file_completion_callback);

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
    out->common._errno = 0;
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

/*-------------- DuplicateHandle() ---------------------------*/
TARPC_FUNC(duplicate_handle, {},
{
    HANDLE src;
    HANDLE tgt;
    HANDLE old_fd;
    HANDLE new_fd;

    if ((src = OpenProcess(SYNCHRONIZE | PROCESS_DUP_HANDLE,
                           FALSE, in->src)) == NULL)
    {
        out->common._errno = RPC_ERRNO;
        out->retval = FALSE;
        ERROR("Cannot open process, error = %d\n", GetLastError());
        goto finish;
    }

    if ((tgt = OpenProcess(SYNCHRONIZE | PROCESS_DUP_HANDLE, FALSE,
                           in->tgt)) == NULL)
    {
        out->common._errno = RPC_ERRNO;
        out->retval = FALSE;
        ERROR("Cannot open process, error = %d\n", GetLastError());
        CloseHandle(src);
        goto finish;
    }

    old_fd = (HANDLE)(in->fd);

    MAKE_CALL(out->retval = DuplicateHandle(src, old_fd, tgt, &new_fd,
                                            0, TRUE,
                                            DUPLICATE_SAME_ACCESS));

    out->fd = (tarpc_int)new_fd;

    finish:
    ;
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

    PREPARE_ADDR(to, in->to, 0);

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
            to, tolen,
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

    PREPARE_ADDR(from, out->from, out->fromlen.fromlen_len == 0 ? 0 :
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
            from,
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

        sockaddr_output_h2rpc(from, fromlen,
                              out->fromlen.fromlen_len == 0 ? 0 :
                                  *(out->fromlen.fromlen_val),
                              &(out->from));
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

/** Copy control-related stuff from RPC */
static te_errno
wsa_recv_msg_control_in(struct tarpc_msghdr *rpc_msg, WSAMSG *msg)
{
#ifdef WINDOWS
    int len = calculate_msg_controllen(rpc_msg);
    int rlen = len * 2;
    int data_len = rpc_msg->msg_control.msg_control_val[0].data.data_len;

    free(rpc_msg->msg_control.msg_control_val[0].data.data_val);
    free(rpc_msg->msg_control.msg_control_val);
    rpc_msg->msg_control.msg_control_val = NULL;
    rpc_msg->msg_control.msg_control_len = 0;
    msg->Control.len = len;
    if ((msg->Control.buf = calloc(1, rlen)) == NULL)
        return TE_RC(TE_TA_WIN32, TE_ENOMEM);

    WSA_CMSG_FIRSTHDR(msg)->cmsg_len = WSA_CMSG_LEN(data_len);
    return 0;
#else
    UNUSED(rpc_msg);
    UNUSED(msg);
    ERROR("Non-zero Control is not supported");
    return TE_RC(TE_TA_WIN32, TE_EINVAL);
#endif
}

/** Copy control-related stuff to RPC */
static te_errno
wsa_recv_msg_control_out(struct tarpc_msghdr *rpc_msg, WSAMSG *msg)
{
#ifdef WINDOWS
    WSACMSGHDR     *c;
    int             i;

    struct tarpc_cmsghdr *rpc_c;

    /* Calculate number of elements to allocate an array */
    for (i = 0, c = WSA_CMSG_FIRSTHDR(msg);
         c != NULL;
         i++, c = WSA_CMSG_NXTHDR(msg, c));

    rpc_c = rpc_msg->msg_control.msg_control_val =
        calloc(1, sizeof(*rpc_c) * i);

    if (rpc_msg->msg_control.msg_control_val == NULL)
        return TE_RC(TE_TA_WIN32, TE_ENOMEM);

    /* Fill the array */
    for (i = 0, c = WSA_CMSG_FIRSTHDR(msg);
         c != NULL;
         i++, c = WSA_CMSG_NXTHDR(msg, c))
    {
        char *data = WSA_CMSG_DATA(c);

        rpc_c->level = socklevel_h2rpc(c->cmsg_level);
        rpc_c->type = cmsg_type_h2rpc(c->cmsg_level,
                                      c->cmsg_type);
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

                return TE_RC(TE_TA_WIN32, TE_ENOMEM);
            }
            memcpy(rpc_c->data.data_val, data, rpc_c->data.data_len);
        }
    }
    rpc_msg->msg_control.msg_control_len = i;
#endif

    UNUSED(rpc_msg);
    UNUSED(msg);

    return 0;
}

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
    WSAMSG               *msg;
    rpc_overlapped       *overlapped = IN_OVERLAPPED;
    rpc_overlapped        tmp;
    struct tarpc_msghdr  *rpc_msg;

    if (overlapped == NULL)
    {
        memset(&tmp, 0, sizeof(tmp));
        overlapped = &tmp;
    }

    msg = &overlapped->msg;
    memset(msg, 0, sizeof(*msg));

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
        PREPARE_ADDR(name, rpc_msg->msg_name, rpc_msg->msg_namelen);

        msg->namelen = rpc_msg->msg_namelen;
        msg->name = name;

        msg->dwBufferCount = rpc_msg->msg_iovlen;
        if (rpc_msg->msg_iov.msg_iov_val != NULL)
        {
            if (iovec2overlapped(overlapped, rpc_msg->msg_iov.msg_iov_len,
                                 rpc_msg->msg_iov.msg_iov_val) != 0)
            {
                out->common._errno = TE_RC(TE_TA_WIN32, TE_ENOMEM);
                goto finish;
            }
            msg->lpBuffers = overlapped->buffers;
        }

        if (rpc_msg->msg_control.msg_control_len > 0)
        {
            out->common._errno = wsa_recv_msg_control_in(rpc_msg, msg);
            if (out->common._errno != 0)
            {
                out->retval = -1;
                goto finish;
            }
        }

        msg->dwFlags = send_recv_flags_rpc2h(rpc_msg->msg_flags);

        /*
         * msg_name, msg_iov, msg_iovlen and msg_control MUST NOT be
         * changed.
         * msg_namelen, msg_controllen and msg_flags MAY be changed.
         */
        INIT_CHECKED_ARG((char *)&msg->name, sizeof(msg->name), 0);
        INIT_CHECKED_ARG((char *)&msg->lpBuffers,
                         sizeof(msg->lpBuffers), 0);
        INIT_CHECKED_ARG((char *)&msg->dwBufferCount,
                         sizeof(msg->dwBufferCount), 0);
        INIT_CHECKED_ARG((char *)&msg->Control, sizeof(msg->Control), 0);

        MAKE_CALL(out->retval =
            (*pf_wsa_recvmsg)(in->s, msg,
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

            sockaddr_output_h2rpc(name, namelen, rpc_msg->msg_namelen,
                                  &(rpc_msg->msg_name));
            rpc_msg->msg_namelen = msg->namelen;
            rpc_msg->msg_flags = send_recv_flags_h2rpc(msg->dwFlags);

            if (msg->Control.buf != NULL && out->retval >= 0)
            {
                out->common._errno =
                    wsa_recv_msg_control_out(rpc_msg, msg);
                if (out->common._errno != 0)
                {
                    out->retval = -1;
                    goto finish;
                }
            }
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
        out->common._errno = RPC_ERRNO;
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

/*-------------- ta_kill_death() --------------------------------*/

TARPC_FUNC(ta_kill_death, {},
{

    HANDLE hp;
    DWORD ex_code;

    if ((hp = OpenProcess(SYNCHRONIZE | PROCESS_TERMINATE, FALSE,
                          in->pid)) == NULL)
    {
        out->common._errno = RPC_ERRNO;
        out->retval = -1;
        ERROR("Cannot open process, error = %d\n", GetLastError());
        goto finish;
    }

    GetExitCodeProcess(hp, &ex_code);
    if (ex_code != STILL_ACTIVE)
    {
        RING("The process was already terminated");
        goto finish;
    }

    if (!TerminateProcess(hp, ex_code))
    {
        ERROR("TerminateProcess failed with error %d", GetLastError());
        out->common._errno = RPC_ERRNO;
        out->retval = -1;
    }

    finish:
    CloseHandle(hp);
}
)

/*-------------- te_shell_cmd() --------------------------------*/
/* This possibility has not been debugged yet */
#if 0
    if (in->in_fd || in->out_fd || in->err_fd)
        si.dwFlags = STARTF_USESTDHANDLES;

    if (in->in_fd)
        si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);

    if (in->out_fd)
        si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);

    if (in->err_fd)
        si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
#endif
#if 0
        if (in->in_fd)
            out->in_fd = ((int *)(si.hStdInput));
        if (in->out_fd)
            out->out_fd = ((int *)(si.hStdOutput));
        if (in->err_fd)
            out->err_fd = ((int *)(si.hStdError));
#endif
TARPC_FUNC(te_shell_cmd,{},
{
    PROCESS_INFORMATION info;
    STARTUPINFO         si;

    if (in->uid != 0)
        RING("%d is given as uid instead of 0. "
             "It isn't supported in Windows", in->uid);

    memset(&si, 0, sizeof(si));
    memset(&info, 0, sizeof(info));
    si.cb = sizeof(si);

    /* INSERT content of the first #if 0 here */

    if (CreateProcess(NULL, in->cmd.cmd_val, NULL, NULL,
                      TRUE, 0, NULL, NULL,
                      &si, &info))
    {
        out->pid = info.dwProcessId;
        /* INSERT content of the second #if 0 here */
        goto finish;
    }
    else
    {
        ERROR("CreateProcess() failed with error %d", GetLastError());
        out->common._errno = RPC_ERRNO;
    }
finish:
    ;
}
)


/*-------------- overfill_buffers() --------------------------*/
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

    if (!in->is_nonblocking)
    {
        if (ioctlsocket(in->sock, FIONBIO, &val) < 0)
        {
            err = GetLastError();
            rc = -1;
            ERROR("%s(): Failed to move socket to non-blocking state; "
                  "error %r",
                  __FUNCTION__, win_rpc_errno(err));
            goto overfill_buffers_exit;
        }
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
    if (!in->is_nonblocking)
    {
        val = 0;
        if (ioctlsocket(in->sock, FIONBIO, &val) < 0)
        {
            err = GetLastError();
            rc = -1;
            ERROR("%s(): Failed to move socket back to blocking state; "
                  "error %r", __FUNCTION__, win_rpc_errno(err));
        }
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
    PREPARE_ADDR(addr, in->addr, 0);

    MAKE_CALL(out->retval = WSAAddressToString(addr, addrlen,
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
    PREPARE_ADDR(addr, out->addr, out->addrlen.addrlen_len == 0 ? 0 :
                                      *out->addrlen.addrlen_val);

    MAKE_CALL(out->retval = WSAStringToAddress(in->addrstr.addrstr_val,
                                domain_rpc2h(in->address_family),
                                (LPWSAPROTOCOL_INFO)(in->info.info_val),
                                addr,
                                (LPINT)out->addrlen.addrlen_val));

    if (out->retval == 0)
        sockaddr_output_h2rpc(addr, addrlen,
                              out->addrlen.addrlen_len == 0 ? 0 :
                                  *out->addrlen.addrlen_val,
                              &(out->addr));
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
    {
        out->common._errno = RPC_ENOMEM;
        out->retval = 0;
    }
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

/*-------------- memalign() ------------------------------*/
/** Note: this is a temporary solution. It does not
 *  actually allocate aligned memory.
 *  We definitely need to do something with that and with
 *  memalign() in *nix RPC server, if stability and portability
 *  is to be achieved.
 */
TARPC_FUNC(memalign, {},
{
    void *buf;

    buf = malloc(in->size);

    if (buf == NULL)
        out->common._errno = TE_RC(TE_TA_UNIX, errno);
    else
        out->retval = rcf_pch_mem_alloc(buf);
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
    MAKE_CALL(set_buf(in->src_buf.src_buf_val, in->dst_buf, in->dst_off,
                      in->src_buf.src_buf_len));
}
)
/*-------------- sendbuf() ------------------------------*/


ssize_t
sendbuf(int fd, rpc_ptr buf_base, size_t buf_offset,
        size_t count, int flags)
{
    return send(fd, rcf_pch_mem_get(buf_base) + buf_offset, count, flags);
}

TARPC_FUNC(sendbuf, {},
{
    MAKE_CALL(out->retval = sendbuf(in->fd, in->buf, in->off,
                                 in->len, in->flags));
}
)

/*-------------------------- Read buffer ----------------------------*/
void
get_buf(tarpc_ptr src_buf_base, size_t src_offset,
        char **dst_buf, size_t *len)
{
    char *src_buf = rcf_pch_mem_get(src_buf_base);

    if (src_buf != NULL && *len != 0)
    {
        char *buf = malloc(*len);

        if (buf == NULL)
        {
            len = 0;
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
        errno = EFAULT;
        len = 0;
    }
}

TARPC_FUNC(get_buf, {},
{
    out->dst_buf.dst_buf_len = in->len;
    MAKE_CALL(get_buf(in->src_buf, in->src_off,
              &out->dst_buf.dst_buf_val,
              &out->dst_buf.dst_buf_len));
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
    MAKE_CALL(set_buf_pattern(in->pattern, in->dst_buf, in->dst_off,
                              in->len));
}
)


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
    PREPARE_ADDR(serv_addr, in->addr, 0);

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

    MAKE_CALL(out->retval = WSAConnect(in->s, serv_addr, serv_addrlen,
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
        case RPC_SIO_ADDRESS_LIST_SORT:
        {
            SOCKET_ADDRESS_LIST *sal;
            tarpc_sa            *tsa;
            int                  i;

            res->type = WSA_IOCTL_SAA;

            sal = (SOCKET_ADDRESS_LIST *)buf;
            tsa = (tarpc_sa *)calloc(sal->iAddressCount, sizeof(tarpc_sa));

            for (i = 0; i < sal->iAddressCount; i++)
            {
                sockaddr_output_h2rpc(sal->Address[i].lpSockaddr,
                                      sal->Address[i].iSockaddrLength,
                                      sal->Address[i].iSockaddrLength,
                                      &tsa[i]);
            }

            res->wsa_ioctl_request_u.req_saa.req_saa_val = tsa;
            res->wsa_ioctl_request_u.req_saa.req_saa_len = i;

            break;
        }

        case RPC_SIO_GET_BROADCAST_ADDRESS:
        case RPC_SIO_ROUTING_INTERFACE_QUERY:
            res->type = WSA_IOCTL_SA;

            sockaddr_output_h2rpc((struct sockaddr *)buf,
                                  sizeof(struct sockaddr), /* FIXME */
                                  sizeof(struct sockaddr), /* FIXME */
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
            break;

        case WSA_IOCTL_SAA:
        {
            SOCKET_ADDRESS_LIST     *sal;
            SOCKET_ADDRESS          *p;
            int                      i;
            struct tarpc_sa         *q = (tarpc_sa *)req->
                                         wsa_ioctl_request_u.req_saa.
                                         req_saa_val;

            inbuf_len = sizeof(uint32_t) + req->wsa_ioctl_request_u.
                        req_saa.req_saa_len * sizeof(SOCKET_ADDRESS);
            inbuf = malloc(inbuf_len);
            sal = (SOCKET_ADDRESS_LIST *)inbuf;
            sal->iAddressCount = req->wsa_ioctl_request_u.req_saa.
                                 req_saa_len;

            p = sal->Address;

            for (i = 0; i < sal->iAddressCount; i++, p++, q++)
            {
                p->lpSockaddr = (LPSOCKADDR)
                                malloc(sizeof(struct sockaddr_storage));
                sockaddr_rpc2h(q, SA(p->lpSockaddr),
                               sizeof(struct sockaddr_storage),
                               NULL, NULL);
                p->iSockaddrLength = (q->sa_family == RPC_AF_INET)?
                                     sizeof(SOCKADDR_IN) :
                                     sizeof(SOCKADDR_IN6);
            }

            break;
        }

        case WSA_IOCTL_INT:
            inbuf = &req->wsa_ioctl_request_u.req_int;
            inbuf_len = sizeof(int);
            break;

        case WSA_IOCTL_SA:
            sockaddr_rpc2h(&req->wsa_ioctl_request_u.req_sa,
                           SA(&addr), sizeof(struct sockaddr_storage),
                           (struct sockaddr **)&inbuf, &inbuf_len);
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
    PREPARE_ADDR(addr, in->addr, 0);

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

    MAKE_CALL(out->retval = WSAJoinLeaf(in->s, addr, addrlen,
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
    ULONG_PTR   key = 0;

    UNUSED(list);

    MAKE_CALL(out->retval = GetQueuedCompletionStatus(
        (HANDLE)(in->completion_port),
        (DWORD *)&out->number_of_bytes, &key,
        &overlapped, (DWORD)in->milliseconds)
    );

    if (overlapped != NULL)
        out->overlapped = (tarpc_overlapped)rcf_pch_mem_get_id(overlapped);
    else
        out->overlapped = (tarpc_overlapped)0;
    out->completion_key = key;
}
)

/*------------ PostQueuedCompletionStatus() -------------------*/

TARPC_FUNC(post_queued_completion_status,
{},
{
    UNUSED(list);

    RING("Before call PostQueuedCompletionStatus()");

    MAKE_CALL(out->retval = PostQueuedCompletionStatus(
        (HANDLE)(in->completion_port),
        (DWORD)in->number_of_bytes,
        (ULONG_PTR)in->completion_key,
        in->overlapped == 0 ? NULL :
            (HANDLE)rcf_pch_mem_get(in->overlapped))
    );

    RING("After call PostQueuedCompletionStatus()");
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

TARPC_FUNC(cmsg_data_parse_ip_pktinfo,
{},
{
    IN_PKTINFO *pktinfo = (IN_PKTINFO *)(in->data.data_val);

    UNUSED(list);

    if (in->data.data_len < sizeof(IN_PKTINFO))
    {
        ERROR("Too small buffer is provided as pktinfo data");
        out->retval = -1;
    }
    else
    {
        out->ipi_spec_dst = 0;
        out->ipi_addr = pktinfo->ipi_addr.S_un.S_addr;
        out->ipi_ifindex = pktinfo->ipi_ifindex;
        out->retval = 0;
    }
}
)

TARPC_FUNC(mcast_join_leave,
{},
{
    struct in_addr          addr;
    DWORD                   rc = 0;
    struct sockaddr_storage a;
    struct ip_mreq          mreq;
/*  api_func                setsockopt_func;

    if (tarpc_find_func(in->common.lib, "setsockopt",
                        &setsockopt_func) != 0)
    {
        ERROR("Cannot find setsockopt() implementation");
        out->retval = -1;
        out->common._errno = TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP);
        return;
    }
*/
    memset(&addr, 0, sizeof(addr));

    if (in->family != RPC_AF_INET)
    {
        out->common._errno = RPC_EAFNOSUPPORT;
        out->retval = -1;
    }

    else if ((in->ifindex != 0) &&
             (rc = get_addr_by_ifindex(in->ifindex, &addr)) != 0)
    {
        ERROR("Cannot get address for interface");
        out->common._errno = rc;
        out->retval = -1;
    }
    else switch (in->how)
    {
        case TARPC_MCAST_WSA:
            if (in->leave_group)
            {
                out->common._errno = RPC_EOPNOTSUPP;
                out->retval = -1;
            }
            else
            {
                rc = setsockopt(in->fd, IPPROTO_IP, IP_MULTICAST_IF,
                                (void *)&addr, sizeof(addr));
                if (rc != 0)
                {
                    ERROR("Setting interface for multicasting failed");
                    out->common._errno = TE_RC(TE_TA_WIN32, rc);
                    out->retval = -1;
                }
                else
                {
                    SOCKET rc;

                    memset(&a, 0, sizeof(a));
                    a.ss_family = addr_family_rpc2h(in->family);
                    assert(te_netaddr_get_size(a.ss_family) ==
                           in->multiaddr.multiaddr_len);
                    memcpy(te_sockaddr_get_netaddr(SA(&a)),
                           in->multiaddr.multiaddr_val,
                           in->multiaddr.multiaddr_len);

                    MAKE_CALL(rc = WSAJoinLeaf(in->fd, SA(&a),
                                               sizeof(struct sockaddr_in),
                                               NULL, NULL, NULL, NULL,
                                               JL_BOTH));
                    if (rc == INVALID_SOCKET)
                    {
                        out->common._errno = TE_RC(TE_RPC,
                                                   WSAGetLastError());
                    }
                    out->retval = (rc == INVALID_SOCKET)? -1 : 0;
                }
            }
            break;

        case TARPC_MCAST_ADD_DROP:

            memcpy(&mreq.imr_multiaddr, in->multiaddr.multiaddr_val,
                   sizeof(struct in_addr));
            memcpy(&mreq.imr_interface, &addr, sizeof(struct in_addr));

            MAKE_CALL(rc = setsockopt(in->fd, IPPROTO_IP,
                                      in->leave_group?
                                      IP_DROP_MEMBERSHIP :
                                      IP_ADD_MEMBERSHIP,
                                      (void *)&mreq, sizeof(mreq)));
            if (rc != 0)
            {
                out->common._errno = TE_RC(TE_RPC, errno);
            }
            out->retval = rc;
            break;

        default:
            ERROR("Unsupported joining method requested");
            out->common._errno = TE_RC(TE_RPC, TE_EOPNOTSUPP);
            out->retval = rc;
    }
}
)

#define MAX_CALLBACKS   1024

/** Completion callbacks registry */
static struct {
    const char *name;
    void       *callback;
} callback_registry[MAX_CALLBACKS] = {
    { "default_completion_callback", default_completion_callback },
    { "default_file_completion_callback",
      default_file_completion_callback },
    { "empty_file_completion_callback", empty_file_completion_callback },
};

/**
 * Get address of completion callback.
 *
 * @param name  name of the callback
 *
 * @return Callback address
 */
void *
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
                             void *callback)
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

/** Sleep in waitable state */
void
sleep_ex(int msec)
{
    SleepEx(msec, TRUE);
}

/*-------------- memcmp() ------------------------------*/

TARPC_FUNC(memcmp, {},
{
     out->retval = memcmp(rcf_pch_mem_get(in->s1_base) + in->s1_off,
                            rcf_pch_mem_get(in->s2_base) + in->s2_off,
                            in->n);
}
)
