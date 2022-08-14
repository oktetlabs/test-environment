/** @file
 * @brief Test API for RPC
 *
 * TAPI for remote calls of Socket API
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#include "te_config.h"

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_NETINET_IP_H
#include <netinet/ip.h>
#endif
#ifdef HAVE_NETINET_TCP_H
#include <netinet/tcp.h>
#endif
#ifdef HAVE_NETINET_IP_ICMP_H
#include <netinet/ip_icmp.h>
#endif
#ifdef HAVE_NETINET_ICMP6_H
#include <netinet/icmp6.h>
#endif
#ifdef HAVE_NET_IF_H
#include <net/if.h>
#else
#define IFNAMSIZ        16
#endif

#include "te_printf.h"
#include "te_str.h"
#include "log_bufs.h"
#include "tapi_rpc_internal.h"
#include "tapi_rpc_unistd.h"
#include "tapi_rpc_socket.h"
#include "tapi_rpcsock_macros.h"
#include "te_alloc.h"
#include "tapi_test.h"
#include "tapi_test_log.h"
#include "tad_common.h"

/* ICMP ECHO request data length. */
#define ICMP_DATALEN 56

void
tapi_rpc_msghdr_msg_flags_init_check(te_bool enable)
{
    rpc_msghdr_msg_flags_init_check_enabled = enable;
}

/* See description in tapi_rpc_socket.h */
struct cmsghdr *
rpc_cmsg_firsthdr(rpc_msghdr *rpc_msg)
{
    struct msghdr msg = { .msg_controllen = rpc_msg->msg_controllen,
                          .msg_control = rpc_msg->msg_control };

    return CMSG_FIRSTHDR(&msg);
}

/* See description in tapi_rpc_socket.h */
struct cmsghdr *
rpc_cmsg_nxthdr(rpc_msghdr *rpc_msg,
                struct cmsghdr *cmsg)
{
    struct msghdr msg = { .msg_controllen = rpc_msg->msg_controllen,
                          .msg_control = rpc_msg->msg_control };

    return CMSG_NXTHDR(&msg, cmsg);
}

/**
 * Check return value of te_string operation, go to the final part
 * of the current function if it failed.
 *
 * @param _expr       Expression to check.
 */
#define TE_STR_RC(_expr) \
    do {                          \
        rc = (_expr);             \
        if (rc != 0)              \
            goto finish;          \
    } while (0)

/**
 * Replace end of string with "...".
 *
 * @param str     Pointer to te_string structure.
 */
static void
str_final_dots(te_string *str)
{
    const char tail[] = "...";

    if (str->size >= sizeof(tail))
    {
        memcpy(str->ptr + MIN(str->len, str->size - sizeof(tail)),
               tail, sizeof(tail));
    }
    else
    {
        ERROR("%s(): string is too small to print dots at the end",
              __FUNCTION__);
    }
}

/* See description in tapi_rpc_socket.h */
const char *
msghdr_rpc2str(const rpc_msghdr *rpc_msg, te_string *str)
{
    unsigned int  i;
    te_errno      rc = 0;

    if (rpc_msg == NULL)
    {
        TE_STR_RC(te_string_append(str, "(nil)"));
        goto finish;
    }

    TE_STR_RC(te_string_append(str, "{ "));

    TE_STR_RC(te_string_append(
                       str,
                       "msg_name: %p [%s], ",
                       rpc_msg->msg_name,
                       sockaddr_h2str(rpc_msg->msg_name)));

    TE_STR_RC(te_string_append(str,
                               "msg_namelen: %" TE_PRINTF_SOCKLEN_T "u, ",
                               rpc_msg->msg_namelen));

    if (rpc_msg->msg_iov == NULL)
    {
        TE_STR_RC(te_string_append(str, "msg_iov: (nil), "));
    }
    else
    {
        TE_STR_RC(te_string_append(str, "msg_iov: { "));

        for (i = 0; i < rpc_msg->msg_riovlen; i++)
        {
            TE_STR_RC(te_string_append(
                       str,
                       "{ iov_base: %p, iov_len: %" TE_PRINTF_SIZE_T "u }",
                       rpc_msg->msg_iov[i].iov_base,
                       rpc_msg->msg_iov[i].iov_len));

            if (i < rpc_msg->msg_riovlen - 1)
                TE_STR_RC(te_string_append(str, ", "));
        }

        TE_STR_RC(te_string_append(str, " }, "));
    }

    TE_STR_RC(te_string_append(str, "msg_iovlen: %" TE_PRINTF_SIZE_T "u, ",
                               rpc_msg->msg_iovlen));

    TE_STR_RC(te_string_append(
                 str, "msg_control: %p, msg_controllen: %"
                 TE_PRINTF_SIZE_T "u, ",
                 rpc_msg->msg_control,
                 rpc_msg->msg_controllen));

    TE_STR_RC(te_string_append(
                        str, "msg_flags: %s",
                        send_recv_flags_rpc2str(rpc_msg->msg_flags)));

    TE_STR_RC(te_string_append(str, " }"));

finish:

    if (rc != 0)
        str_final_dots(str);

    return str->ptr;
}

/* See description in tapi_rpc_socket.h */
const char *
mmsghdrs_rpc2str(const struct rpc_mmsghdr *rpc_mmsgs, unsigned int num,
                 te_string *str)
{
    unsigned int  i;
    te_errno      rc = 0;

    if (rpc_mmsgs == NULL)
    {
        TE_STR_RC(te_string_append(str, "(nil)"));
        goto finish;
    }

    for (i = 0; i < num; i++)
    {
        TE_STR_RC(te_string_append(str, "{ msg_hdr: "));
        msghdr_rpc2str(&rpc_mmsgs[i].msg_hdr, str);
        TE_STR_RC(te_string_append(str, ", msg_len: %u }",
                                   rpc_mmsgs[i].msg_len));
        if (i < num - 1)
            TE_STR_RC(te_string_append(str, ", "));
    }

finish:

    if (rc != 0)
        str_final_dots(str);

    return str->ptr;
}

#undef TE_STR_RC

int
rpc_socket(rcf_rpc_server *rpcs,
           rpc_socket_domain domain, rpc_socket_type type,
           rpc_socket_proto protocol)
{
    tarpc_socket_in  in;
    tarpc_socket_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(socket, -1);
    }

    in.domain = domain;
    in.type = type;
    in.proto = protocol;

    rcf_rpc_call(rpcs, "socket", &in, &out);

    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(socket, out.fd);
    TAPI_RPC_LOG(rpcs, socket, "%s, %s, %s", "%d",
                 domain_rpc2str(domain), socktype_rpc2str(type),
                 proto_rpc2str(protocol), out.fd);
    RETVAL_INT(socket, out.fd);
}

/**
 * Generic function to call rpc_bind()
 *
 * @param rpcs      RPC server handle
 * @param s         socket descriptor
 * @param my_addr   pointer to a @b sockaddr structure
 * @param len       length to be passed to bind()
 * @param fwd_len   forward the specified length in parameter @a len
 *
 * @return 0 on success or -1 on failure
 * @note See @b bind manual page for more infrormation.
 */
static int
rpc_bind_gen(rcf_rpc_server *rpcs, int s, const struct sockaddr *my_addr,
             socklen_t len, te_bool fwd_len)
{
    tarpc_bind_in  in;
    tarpc_bind_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(bind, -1);
    }

    in.fd = s;
    in.len = len;
    in.fwd_len = fwd_len;
    sockaddr_input_h2rpc(my_addr, &in.addr);

    rcf_rpc_call(rpcs, "bind", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(bind, out.retval);
    TAPI_RPC_LOG(rpcs, bind, "%d, %s", "%d", s, sockaddr_h2str(my_addr),
                 out.retval);
    RETVAL_INT(bind, out.retval);
}

/* See description in the tapi_rpc_socket.h */
int
rpc_bind(rcf_rpc_server *rpcs, int s, const struct sockaddr *my_addr)
{
    return rpc_bind_gen(rpcs, s, my_addr, 0, FALSE);
}

/* See description in the tapi_rpc_socket.h */
int
rpc_bind_len(rcf_rpc_server *rpcs, int s, const struct sockaddr *my_addr,
             socklen_t addrlen)
{
    return rpc_bind_gen(rpcs, s, my_addr, addrlen, TRUE);
}

int rpc_bind_raw(rcf_rpc_server *rpcs, int s,
                 const struct sockaddr *my_addr, socklen_t addrlen)
{
    tarpc_bind_in  in;
    tarpc_bind_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(bind, -1);
    }

    in.fd = s;
    sockaddr_raw2rpc(my_addr, addrlen, &in.addr);

    rcf_rpc_call(rpcs, "bind", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(bind, out.retval);
    TAPI_RPC_LOG(rpcs, bind, "%d, %p, %d", "%d",
                 s, my_addr, addrlen, out.retval);
    RETVAL_INT(bind, out.retval);
}

int
rpc_check_port_is_free(rcf_rpc_server *rpcs, uint16_t port)
{
    tarpc_check_port_is_free_in  in;
    tarpc_check_port_is_free_out out;
    te_bool errno_change_check_prev = rpcs->errno_change_check;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(bind, -1);
    }

    in.port = port;

    /*
     * Bug 11721: check_port_is_free() may change errno to EAFNOSUPPORT on
     * systems without IPv6 support, so errno checking should be disabled.
     */
    rpcs->errno_change_check = 0;

    rcf_rpc_call(rpcs, "check_port_is_free", &in, &out);
    CHECK_RETVAL_VAR(check_port_is_free, out.retval,
                     (out.retval != TRUE && out.retval != FALSE), FALSE);
    TAPI_RPC_LOG(rpcs, check_port_is_free, "%d", "%d",
                 (int)port, out.retval);
    rpcs->errno_change_check = errno_change_check_prev;
    TAPI_RPC_OUT(check_port_is_free, FALSE);
    return out.retval; /* no jumps! */
}

int
rpc_connect(rcf_rpc_server *rpcs,
            int s, const struct sockaddr *addr)
{
    tarpc_connect_in  in;
    tarpc_connect_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(connect, -1);
    }

    in.fd = s;
    sockaddr_input_h2rpc(addr, &in.addr);

    rcf_rpc_call(rpcs, "connect", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(connect, out.retval);
    TAPI_RPC_LOG(rpcs, connect, "%d, %s", "%d",
                 s, sockaddr_h2str(addr), out.retval);
    RETVAL_INT(connect, out.retval);
}

int
rpc_connect_raw(rcf_rpc_server *rpcs, int s,
            const struct sockaddr *addr, socklen_t addrlen)
{
    tarpc_connect_in  in;
    tarpc_connect_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(connect, -1);
    }

    in.fd = s;
    sockaddr_raw2rpc(addr, addrlen, &in.addr);

    rcf_rpc_call(rpcs, "connect", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(connect, out.retval);
    TAPI_RPC_LOG(rpcs, connect, "%d, %p, %d", "%d",
                 s, addr, addrlen, out.retval);
    RETVAL_INT(connect, out.retval);
}

int
rpc_listen(rcf_rpc_server *rpcs, int fd, int backlog)
{
    tarpc_listen_in  in;
    tarpc_listen_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(listen, -1);
    }

    in.fd = fd;
    in.backlog = backlog;

    rcf_rpc_call(rpcs, "listen", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(listen, out.retval);
    TAPI_RPC_LOG(rpcs, listen, "%d, %d", "%d",
                 fd, backlog, out.retval);
    RETVAL_INT(listen, out.retval);
}

#define MAKE_ACCEPT_CALL(_accept_func) \
    if (rpcs == NULL)                                           \
    {                                                           \
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__); \
        RETVAL_INT(_accept_func, -1);                           \
    }                                                           \
                                                                \
    if (addr != NULL && addrlen != NULL && *addrlen > raddrlen) \
    {                                                           \
        rpcs->_errno = TE_RC(TE_RCF, TE_EINVAL);                \
        RETVAL_INT(_accept_func, -1);                           \
    }                                                           \
                                                                \
    in.fd = s;                                                  \
    if (addrlen != NULL && rpcs->op != RCF_RPC_WAIT)            \
    {                                                           \
        in.len.len_len = 1;                                     \
        in.len.len_val = addrlen;                               \
    }                                                           \
    if (rpcs->op != RCF_RPC_WAIT)                               \
    {                                                           \
        sockaddr_raw2rpc(addr, raddrlen, &in.addr);             \
    }                                                           \
                                                                \
    rcf_rpc_call(rpcs, #_accept_func, &in, &out);               \
                                                                \
    if (RPC_IS_CALL_OK(rpcs) && rpcs->op != RCF_RPC_WAIT)       \
    {                                                           \
        sockaddr_rpc2h(&out.addr, addr, raddrlen,               \
                       NULL, addrlen);                          \
                                                                \
        if (addrlen != NULL && out.len.len_val != NULL)         \
            *addrlen = out.len.len_val[0];                      \
    }                                                           \
    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(_accept_func,             \
                                      out.retval)

int
rpc_accept_gen(rcf_rpc_server *rpcs,
               int s, struct sockaddr *addr, socklen_t *addrlen,
               socklen_t raddrlen)
{
    socklen_t save_addrlen =
                (addrlen == NULL) ? (socklen_t)-1 : *addrlen;

    tarpc_accept_in  in;
    tarpc_accept_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    MAKE_ACCEPT_CALL(accept);

    TAPI_RPC_LOG(rpcs, accept, "%d, %p[%u], %p(%u)",
                 "%d peer=%s addrlen=%u",
                 s, addr, raddrlen, addrlen, save_addrlen,
                 out.retval, sockaddr_h2str(addr),
                 (addrlen == NULL) ? (socklen_t)-1 : *addrlen);
    RETVAL_INT(accept, out.retval);
}

int
rpc_accept4_gen(rcf_rpc_server *rpcs,
               int s, struct sockaddr *addr, socklen_t *addrlen,
               socklen_t raddrlen, int flags)
{
    socklen_t save_addrlen =
                (addrlen == NULL) ? (socklen_t)-1 : *addrlen;

    tarpc_accept4_in  in;
    tarpc_accept4_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.flags = socket_flags_rpc2h(flags);

    MAKE_ACCEPT_CALL(accept4);

    TAPI_RPC_LOG(rpcs, accept4, "%d, %p[%u], %p(%u), %s",
                 "%d peer=%s addrlen=%u",
                 s, addr, raddrlen, addrlen, save_addrlen,
                 socket_flags_rpc2str(flags),
                 out.retval, sockaddr_h2str(addr),
                 (addrlen == NULL) ? (socklen_t)-1 : *addrlen);
    RETVAL_INT(accept4, out.retval);
}

#undef MAKE_ACCEPT_CALL

ssize_t
rpc_recvfrom_gen(rcf_rpc_server *rpcs,
                 int s, void *buf, size_t len, rpc_send_recv_flags flags,
                 struct sockaddr *from, socklen_t *fromlen,
                 size_t rbuflen, socklen_t rfrombuflen)
{
    socklen_t          save_fromlen =
                           (fromlen == NULL) ? (socklen_t)-1 : *fromlen;

    tarpc_recvfrom_in  in;
    tarpc_recvfrom_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(recvfrom, -1);
    }

    if (from != NULL && fromlen != NULL && *fromlen > rfrombuflen)
    {
        rpcs->_errno = TE_RC(TE_RCF, TE_EINVAL);
        RETVAL_INT(recvfrom, -1);
    }

    in.chk_func = TEST_BEHAVIOUR(use_chk_funcs);

    if (buf != NULL && len > rbuflen && !(in.chk_func))
    {
        ERROR("%s(): len > rbuflen and __recvfrom_chk() is not tested",
              __FUNCTION__);
        rpcs->_errno = TE_RC(TE_RCF, TE_EINVAL);
        RETVAL_INT(recvfrom, -1);
    }

    in.fd = s;
    in.len = len;
    if (fromlen != NULL && rpcs->op != RCF_RPC_WAIT)
    {
        in.fromlen.fromlen_len = 1;
        in.fromlen.fromlen_val = fromlen;
    }
    if (rpcs->op != RCF_RPC_WAIT)
    {
        sockaddr_raw2rpc(from, rfrombuflen, &in.from);
    }
    if (buf != NULL && rpcs->op != RCF_RPC_WAIT)
    {
        in.buf.buf_len = rbuflen;
        in.buf.buf_val = buf;
    }
    in.flags = flags;

    rcf_rpc_call(rpcs, "recvfrom", &in, &out);

    if (RPC_IS_CALL_OK(rpcs) && rpcs->op != RCF_RPC_WAIT)
    {
        if (buf != NULL && out.buf.buf_val != NULL)
            memcpy(buf, out.buf.buf_val, out.buf.buf_len);

        sockaddr_rpc2h(&out.from, from, rfrombuflen,
                       NULL, fromlen);

        if (fromlen != NULL && out.fromlen.fromlen_val != NULL)
            *fromlen = out.fromlen.fromlen_val[0];
    }

    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(recvfrom, out.retval);
    TAPI_RPC_LOG(rpcs, recvfrom, "%d, %p[%u], %u, %s, %p[%u], %d, "
                 "chk_func=%s", "%d from=%s fromlen=%d",
                 s, buf, rbuflen, len, send_recv_flags_rpc2str(flags),
                 from, rfrombuflen, (int)save_fromlen,
                 (in.chk_func ? "TRUE" : "FALSE"),
                 out.retval, sockaddr_h2str(from),
                 (fromlen == NULL) ? -1 : (int)*fromlen);
    RETVAL_INT(recvfrom, out.retval);
}

ssize_t
rpc_recv_gen(rcf_rpc_server *rpcs,
                 int s, void *buf, size_t len, rpc_send_recv_flags flags,
                 size_t rbuflen)
{
    tarpc_recv_in  in;
    tarpc_recv_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(recv, -1);
    }

    in.chk_func = TEST_BEHAVIOUR(use_chk_funcs);

    if (buf != NULL && len > rbuflen && !(in.chk_func))
    {
        ERROR("%s(): len > rbuflen and __recv_chk() is not tested",
              __FUNCTION__);
        rpcs->_errno = TE_RC(TE_RCF, TE_EINVAL);
        RETVAL_INT(recv, -1);
    }

    in.fd = s;
    in.len = len;
    if (buf != NULL && rpcs->op != RCF_RPC_WAIT)
    {
        in.buf.buf_len = rbuflen;
        in.buf.buf_val = buf;
    }
    in.flags = flags;

    rcf_rpc_call(rpcs, "recv", &in, &out);

    if (RPC_IS_CALL_OK(rpcs))
    {
        if (buf != NULL && out.buf.buf_val != NULL)
            memcpy(buf, out.buf.buf_val, out.buf.buf_len);
    }

    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(recv, out.retval);
    TAPI_RPC_LOG(rpcs, recv, "%d, %p[%u], %u, %s, chk_func=%s", "%d",
                 s, buf, rbuflen, len, send_recv_flags_rpc2str(flags),
                 (in.chk_func ? "TRUE" : "FALSE"), out.retval);
    RETVAL_INT(recv, out.retval);
}

tarpc_ssize_t
rpc_recvbuf_gen(rcf_rpc_server *rpcs, int fd, rpc_ptr buf, size_t buf_off,
                size_t count, rpc_send_recv_flags flags)
{
    tarpc_recvbuf_in  in;
    tarpc_recvbuf_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(write, -1);
    }

    in.fd  = fd;
    in.len = count;
    in.buf = buf;
    in.off = buf_off;

    rcf_rpc_call(rpcs, "recvbuf", &in, &out);

    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(readbuf, out.retval);
    TAPI_RPC_LOG(rpcs, recvbuf, "%d, %u (off %u), %u, %s", "%d",
                 fd, buf, buf_off, count, send_recv_flags_rpc2str(flags),
                 out.retval);
    RETVAL_INT(recvbuf, out.retval);
}

int
rpc_shutdown(rcf_rpc_server *rpcs, int s, rpc_shut_how how)
{
    tarpc_shutdown_in  in;
    tarpc_shutdown_out out;

    memset((char *)&in, 0, sizeof(in));
    memset((char *)&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(shutdown, -1);
    }

    in.fd = s;
    in.how = how;

    rcf_rpc_call(rpcs, "shutdown", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(shutdown, out.retval);
    TAPI_RPC_LOG(rpcs, shutdown, "%d, %s", "%d",
                 s, shut_how_rpc2str(how), out.retval);
    RETVAL_INT(shutdown, out.retval);
}

ssize_t
rpc_sendto(rcf_rpc_server *rpcs,
           int s, const void *buf, size_t len,
           rpc_send_recv_flags flags,
           const struct sockaddr *to)
{
    tarpc_sendto_in  in;
    tarpc_sendto_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(sendto, -1);
    }

    in.fd = s;
    in.len = len;
    if (rpcs->op != RCF_RPC_WAIT)
    {
        sockaddr_input_h2rpc(to, &in.to);
    }
    if (buf != NULL && rpcs->op != RCF_RPC_WAIT)
    {
        in.buf.buf_len = len;
        in.buf.buf_val = (uint8_t *)buf;
    }
    in.flags = flags;

    rcf_rpc_call(rpcs, "sendto", &in, &out);

    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(sendto, out.retval);
    TAPI_RPC_LOG(rpcs, sendto, "%d, %p, %u, %s, %s", "%d",
                 s, buf, len, send_recv_flags_rpc2str(flags),
                 sockaddr_h2str(to),
                 out.retval);
    RETVAL_INT(sendto, out.retval);
}

ssize_t
rpc_sendto_raw(rcf_rpc_server *rpcs,
               int s, const void *buf, size_t len,
               rpc_send_recv_flags flags,
               const struct sockaddr *to, socklen_t tolen)
{
    tarpc_sendto_in  in;
    tarpc_sendto_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(sendto, -1);
    }

    in.fd = s;
    in.len = len;
    if (rpcs->op != RCF_RPC_WAIT)
    {
        sockaddr_raw2rpc(to, tolen, &in.to);
    }
    if (buf != NULL && rpcs->op != RCF_RPC_WAIT)
    {
        in.buf.buf_len = len;
        in.buf.buf_val = (uint8_t *)buf;
    }
    in.flags = flags;

    rcf_rpc_call(rpcs, "sendto", &in, &out);

    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(sendto, out.retval);
    TAPI_RPC_LOG(rpcs, sendto, "%d, %p, %u, %s, %p, %d", "%d",
                 s, buf, len, send_recv_flags_rpc2str(flags),
                 to, tolen, out.retval);
    RETVAL_INT(sendto, out.retval);
}

ssize_t
rpc_send(rcf_rpc_server *rpcs,
           int s, const void *buf, size_t len,
           rpc_send_recv_flags flags)
{
    tarpc_send_in  in;
    tarpc_send_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(send, -1);
    }

    in.fd = s;
    in.len = len;
    if (buf != NULL && rpcs->op != RCF_RPC_WAIT)
    {
        in.buf.buf_len = len;
        in.buf.buf_val = (uint8_t *)buf;
    }
    in.flags = flags;

    rcf_rpc_call(rpcs, "send", &in, &out);

    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(send, out.retval);
    TAPI_RPC_LOG(rpcs, send, "%d, %p, %u, %s", "%d",
                 s, buf, len, send_recv_flags_rpc2str(flags), out.retval);
    RETVAL_INT(send, out.retval);
}

ssize_t
rpc_sendbuf_gen(rcf_rpc_server *rpcs, int s, rpc_ptr buf, size_t buf_off,
                size_t len, rpc_send_recv_flags flags)
{
    tarpc_sendbuf_in  in;
    tarpc_sendbuf_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(send, -1);
    }

    in.fd = s;
    in.len = len;
    in.buf = buf;
    in.off = buf_off;
    in.flags = flags;

    rcf_rpc_call(rpcs, "sendbuf", &in, &out);

    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(send, out.retval);
    TAPI_RPC_LOG(rpcs, sendbuf, "%d, %u (off %u), %u, %s", "%d",
                 s, buf, buf_off, len, send_recv_flags_rpc2str(flags),
                 out.retval);
    RETVAL_INT(sendbuf, out.retval);
}

/* See description in the tapi_rpc_socket.h */
ssize_t
rpc_send_msg_more_ext(rcf_rpc_server *rpcs, int s, rpc_ptr buf,
                      size_t first_len, size_t second_len,
                      tarpc_send_function first_func,
                      tarpc_send_function second_func, te_bool set_nodelay)
{
    tarpc_send_msg_more_in  in;
    tarpc_send_msg_more_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(send, -1);
    }

    in.fd = s;
    in.first_len = first_len;
    in.second_len = second_len;
    in.first_func = first_func;
    in.second_func = second_func;
    in.set_nodelay = set_nodelay;
    in.buf = buf;

    rcf_rpc_call(rpcs, "send_msg_more", &in, &out);

    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(send_msg_more, out.retval);
    TAPI_RPC_LOG(rpcs, send_msg_more, "%d, " RPC_PTR_FMT ", first_len=%"
                 TE_PRINTF_SIZE_T "u, second_len=%" TE_PRINTF_SIZE_T "u, "
                 "first_func=%s, second_func=%s, set_nodelay=%s", "%d",
                 s, RPC_PTR_VAL(buf), first_len, second_len,
                 send_function_tarpc2str(first_func),
                 send_function_tarpc2str(second_func),
                 (set_nodelay ? "TRUE" : "FALSE"),
                 out.retval);
    RETVAL_INT(send_msg_more, out.retval);
}

/* See description in the tapi_rpc_socket.h */
ssize_t
rpc_send_msg_more(rcf_rpc_server *rpcs, int s, rpc_ptr buf,
                  size_t first_len, size_t second_len)
{
    return rpc_send_msg_more_ext(rpcs, s, buf, first_len, second_len,
                                 TARPC_SEND_FUNC_SEND,
                                 TARPC_SEND_FUNC_SEND, FALSE);
}

ssize_t
rpc_send_one_byte_many(rcf_rpc_server *rpcs, int s, int duration)
{
    tarpc_send_one_byte_many_in  in;
    tarpc_send_one_byte_many_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(send_one_byte_many, -1);
    }

    in.fd = s;
    in.duration = duration;

    rpcs->errno_change_check = 0;
    rcf_rpc_call(rpcs, "send_one_byte_many", &in, &out);

    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(send_one_byte_many, out.retval);
    TAPI_RPC_LOG(rpcs, send_one_byte_many, "%d, %d", "%d", s, duration,
                 out.retval);
    RETVAL_INT(send_one_byte_many, out.retval);
}

/**
 * Check field @b msg_flags value is correct, skip this check if flag
 * @c RPC_MSG_FLAGS_NO_CHECK is set into field @b msg_flags_mode.
 *
 * @param msg   Returned msghdr structure
 * @param ok    Was the recvmsg() call sucessful?
 */
static void
msghdr_check_msg_flags(rpc_msghdr *msg, te_bool ok)
{
    if (msg == NULL || !rpc_msghdr_msg_flags_init_check_enabled ||
        (msg->msg_flags_mode & RPC_MSG_FLAGS_NO_CHECK) != 0)
        return;

    if (ok && msg->msg_flags != 0)
    {
        ERROR("Returned flags value: %s",
              send_recv_flags_rpc2str(msg->msg_flags));
        RING_VERDICT("Non-zero msg_flags value was returned");
    }
    else if (!ok && msg->in_msg_flags != msg->msg_flags)
    {
        ERROR("Returned -> expected flags value: %s -> %s",
              send_recv_flags_rpc2str(msg->msg_flags),
              send_recv_flags_rpc2str(msg->in_msg_flags));
        RING_VERDICT("msg_flags field have changed its value");
    }
}

ssize_t
rpc_sendmsg(rcf_rpc_server *rpcs,
            int s, const struct rpc_msghdr *msg, rpc_send_recv_flags flags)
{
    te_string str_msg = TE_STRING_INIT_STATIC(1024);

    tarpc_sendmsg_in  in;
    tarpc_sendmsg_out out;

    struct tarpc_msghdr rpc_msg;
    te_errno            rc;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));
    memset(&rpc_msg, 0, sizeof(rpc_msg));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(sendmsg, -1);
    }

    in.s = s;
    in.flags = flags;

    if (msg != NULL && rpcs->op != RCF_RPC_WAIT)
    {
        in.msg.msg_val = &rpc_msg;
        in.msg.msg_len = 1;

        rc = msghdr_rpc2tarpc(msg, &rpc_msg, FALSE);
        if (rc != 0)
        {
            rpcs->_errno = TE_RC(TE_TAPI, rc);
            tarpc_msghdr_free(&rpc_msg);
            RETVAL_INT(sendmsg, -1);
        }
    }

    rcf_rpc_call(rpcs, "sendmsg", &in, &out);

    tarpc_msghdr_free(&rpc_msg);

    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(sendmsg, out.retval);
    TAPI_RPC_LOG(rpcs, sendmsg, "%d, %p (%s), %s", "%d",
         s, msg, msghdr_rpc2str(msg, &str_msg),
         send_recv_flags_rpc2str(flags),
         out.retval);
    RETVAL_INT(sendmsg, out.retval);
}

ssize_t
rpc_recvmsg(rcf_rpc_server *rpcs,
            int s, struct rpc_msghdr *msg, rpc_send_recv_flags flags)
{
    te_string str_msg = TE_STRING_INIT_STATIC(1024);

    tarpc_recvmsg_in  in;
    tarpc_recvmsg_out out;

    struct tarpc_msghdr rpc_msg;
    te_errno            rc;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));
    memset(&rpc_msg, 0, sizeof(rpc_msg));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(recvmsg, -1);
    }

    in.s = s;
    in.flags = flags;

    if (msg != NULL && rpcs->op != RCF_RPC_WAIT)
    {
        in.msg.msg_val = &rpc_msg;
        in.msg.msg_len = 1;

        rc = msghdr_rpc2tarpc(msg, &rpc_msg, TRUE);
        if (rc != 0)
        {
            rpcs->_errno = TE_RC(TE_TAPI, rc);
            tarpc_msghdr_free(&rpc_msg);
            RETVAL_INT(recvmsg, -1);
        }
    }

    rcf_rpc_call(rpcs, "recvmsg", &in, &out);

    tarpc_msghdr_free(&rpc_msg);

    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(recvmsg, out.retval);

    if (RPC_IS_CALL_OK(rpcs) && rpcs->op != RCF_RPC_WAIT &&
        msg != NULL && out.msg.msg_val != NULL)
    {
        rpc_msg = out.msg.msg_val[0];

        rc = msghdr_tarpc2rpc(&rpc_msg, msg);
        if (rc != 0)
        {
            rpcs->_errno = TE_RC(TE_TAPI, rc);
            RETVAL_INT(recvmsg, -1);
        }
    }

    TAPI_RPC_LOG(rpcs, recvmsg, "%d, %p (%s), %s", "%ld",
                 s, msg, msghdr_rpc2str(msg, &str_msg),
                 send_recv_flags_rpc2str(flags),
                 (long)out.retval);

    if (RPC_IS_CALL_OK(rpcs) && rpcs->op != RCF_RPC_WAIT)
        msghdr_check_msg_flags(msg, out.retval >= 0);

    RETVAL_INT(recvmsg, out.retval);
}

int
rpc_cmsg_data_parse_ip_pktinfo(rcf_rpc_server *rpcs,
                               uint8_t *data, uint32_t data_len,
                               struct in_addr *ipi_spec_dst,
                               struct in_addr *ipi_addr,
                               int *ipi_ifindex)
{
    tarpc_cmsg_data_parse_ip_pktinfo_in     in;
    tarpc_cmsg_data_parse_ip_pktinfo_out    out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    UNUSED(rpcs);
    UNUSED(data);
    UNUSED(data_len);
    UNUSED(ipi_spec_dst);
    UNUSED(ipi_addr);
    UNUSED(ipi_ifindex);

    RING("%s(): this function is no longer supported since "
         "IP_PKTINFO is now processed correctly by TE", __FUNCTION__);
    RETVAL_INT(cmsg_data_parse_ip_pktinfo, -1);
}

int
rpc_getsockname_gen(rcf_rpc_server *rpcs,
                    int s, struct sockaddr *name,
                    socklen_t *namelen,
                    socklen_t rnamelen)
{
    socklen_t   namelen_save =
        (namelen == NULL) ? (unsigned int)-1 : *namelen;

    tarpc_getsockname_in  in;
    tarpc_getsockname_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(getsockname, -1);
    }
    if (name != NULL && namelen != NULL && *namelen > rnamelen)
    {
        rpcs->_errno = TE_RC(TE_RCF, TE_EINVAL);
        RETVAL_INT(getsockname, -1);
    }

    in.fd = s;
    if (namelen != NULL && rpcs->op != RCF_RPC_WAIT)
    {
        in.len.len_len = 1;
        in.len.len_val = namelen;
    }
    sockaddr_raw2rpc(name, rnamelen, &in.addr);

    rcf_rpc_call(rpcs, "getsockname", &in, &out);

    if (RPC_IS_CALL_OK(rpcs) && rpcs->op != RCF_RPC_CALL)
    {
        sockaddr_rpc2h(&out.addr, name, rnamelen,
                       NULL, namelen);

        if (namelen != NULL && out.len.len_val != NULL)
            *namelen = out.len.len_val[0];
    }

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(getsockname, out.retval);
    TAPI_RPC_LOG(rpcs, getsockname, "%d, %p[%u], %u",
                 "%d name=%s namelen=%u",
                 s, name, rnamelen, namelen_save,
                 out.retval, sockaddr_h2str(name),
                 (namelen == NULL) ? (unsigned int)-1 : *namelen);
    RETVAL_INT(getsockname, out.retval);
}

int
rpc_getpeername_gen(rcf_rpc_server *rpcs,
                    int s, struct sockaddr *name,
                    socklen_t *namelen,
                    socklen_t rnamelen)
{
    socklen_t   namelen_save =
        (namelen == NULL) ? (unsigned int)-1 : *namelen;

    tarpc_getpeername_in  in;
    tarpc_getpeername_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(getpeername, -1);
    }

    if (name != NULL && namelen != NULL && *namelen > rnamelen)
    {
        rpcs->_errno = TE_RC(TE_RCF, TE_EINVAL);
        RETVAL_INT(getpeername, -1);
    }

    in.fd = s;
    if (namelen != NULL && rpcs->op != RCF_RPC_WAIT)
    {
        in.len.len_len = 1;
        in.len.len_val = namelen;
    }
    sockaddr_raw2rpc(name, rnamelen, &in.addr);

    rcf_rpc_call(rpcs, "getpeername", &in, &out);

    if (RPC_IS_CALL_OK(rpcs) && rpcs->op != RCF_RPC_CALL)
    {
        sockaddr_rpc2h(&out.addr, name, rnamelen,
                       NULL, namelen);

        if (namelen != NULL && out.len.len_val != NULL)
            *namelen = out.len.len_val[0];
    }

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(getpeername, out.retval);
    TAPI_RPC_LOG(rpcs, getpeername, "%d, %p[%u], %u",
                 "%d name=%s namelen=%u",
                 s, name, rnamelen, namelen_save,
                 out.retval, sockaddr_h2str(name),
                 (namelen == NULL) ? (unsigned int)-1 : *namelen);
    RETVAL_INT(getpeername, out.retval);
}

static te_bool
mreq_source_cpy(tarpc_mreq_source *opt, struct option_value *val,
                rpc_sockopt optname)
{
    if (opt->type != OPT_MREQ_SOURCE)
    {
        ERROR("Unknown option type for %s get request",
              sockopt_rpc2str(optname));
        return FALSE;
    }

    val->opttype = opt->type;

    memcpy(&val->option_value_u.opt_mreq_source.imr_multiaddr,
           &opt->multiaddr, sizeof(opt->multiaddr));
    val->option_value_u.opt_mreq_source.imr_multiaddr =
      ntohl(val->option_value_u.opt_mreq_source.imr_multiaddr);

    memcpy(&val->option_value_u.opt_mreq_source.imr_interface,
           &opt->interface, sizeof(opt->interface));
    val->option_value_u.opt_mreq_source.imr_interface =
      ntohl(val->option_value_u.opt_mreq_source.imr_interface);

    memcpy(&val->option_value_u.opt_mreq_source.imr_sourceaddr,
           &opt->sourceaddr, sizeof(opt->sourceaddr));
    val->option_value_u.opt_mreq_source.imr_sourceaddr =
      ntohl(val->option_value_u.opt_mreq_source.imr_sourceaddr);

    return TRUE;
}

int
rpc_getsockopt_gen(rcf_rpc_server *rpcs,
                   int s, rpc_socklevel level, rpc_sockopt optname,
                   void *optval, void *raw_optval,
                   socklen_t *raw_optlen, socklen_t raw_roptlen)
{
    tarpc_getsockopt_in   in;
    tarpc_getsockopt_out  out;
    struct option_value   val;
    te_log_buf           *opt_val_str = NULL;
    char                  opt_len_str[32] = "(nil)";

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(getsockopt, -1);
    }

    /* Copy parameters to tarpc_getsockopt_in structure */
    rpcs->op = RCF_RPC_CALL_WAIT;
    in.s = s;
    in.level = level;
    in.optname = optname;

    if (optval != NULL || raw_optlen != NULL)
    {
        opt_len_str[0] = '\0';
        if (optval != NULL)
        {
            TE_SPRINTF(opt_len_str, "AUTO");
        }
        if (raw_optlen != NULL)
        {
            snprintf(opt_len_str + strlen(opt_len_str),
                     sizeof(opt_len_str) - strlen(opt_len_str),
                     "%s%u",  (optval != NULL) ? "+" : "",
                     (unsigned)*raw_optlen);
        }
    }

    if (optval != NULL)
    {
        memset(&val, 0, sizeof(val));

        in.optval.optval_len = 1;
        in.optval.optval_val = &val;

        switch (optname)
        {
            case RPC_SO_LINGER:
                val.opttype = OPT_LINGER;
                val.option_value_u.opt_linger.l_onoff =
                    ((tarpc_linger *)optval)->l_onoff;
                val.option_value_u.opt_linger.l_linger =
                    ((tarpc_linger *)optval)->l_linger;
                break;

            case RPC_SO_RCVTIMEO:
            case RPC_SO_SNDTIMEO:
                val.opttype = OPT_TIMEVAL;
                val.option_value_u.opt_timeval =
                    *((tarpc_timeval *)optval);
                break;

            case RPC_IPV6_PKTOPTIONS:
                ERROR("IPV6_PKTOPTIONS is not supported yet");
                RETVAL_INT(getsockopt, -1);
                break;

            case RPC_IPV6_ADD_MEMBERSHIP:
            case RPC_IPV6_DROP_MEMBERSHIP:
            case RPC_IPV6_JOIN_ANYCAST:
            case RPC_IPV6_LEAVE_ANYCAST:
            {
                struct ipv6_mreq *opt = (struct ipv6_mreq *)optval;

                val.opttype = OPT_MREQ6;

                memcpy(&val.option_value_u.opt_mreq6.ipv6mr_multiaddr.
                       ipv6mr_multiaddr_val,
                       &opt->ipv6mr_multiaddr, sizeof(struct in6_addr));
                val.option_value_u.opt_mreq6.ipv6mr_ifindex =
                    opt->ipv6mr_interface;
                break;
            }

            case RPC_IP_ADD_MEMBERSHIP:
            case RPC_IP_DROP_MEMBERSHIP:
            case RPC_IP_MULTICAST_IF:
            {
                tarpc_mreqn *opt = (tarpc_mreqn *)optval;

                val.opttype = opt->type;
                switch (opt->type)
                {
                    case OPT_IPADDR:
                        memcpy(&val.option_value_u.opt_ipaddr,
                               &opt->address, sizeof(opt->address));
                        val.option_value_u.opt_ipaddr =
                            ntohl(val.option_value_u.opt_ipaddr);
                        break;

                    case OPT_MREQ:
                        memcpy(&val.option_value_u.opt_mreq.imr_multiaddr,
                               &opt->multiaddr, sizeof(opt->multiaddr));
                        val.option_value_u.opt_mreq.imr_multiaddr =
                          ntohl(val.option_value_u.opt_mreq.imr_multiaddr);
                        memcpy(&val.option_value_u.opt_mreq.imr_address,
                               &opt->address, sizeof(opt->address));
                        val.option_value_u.opt_mreq.imr_address =
                            ntohl(val.option_value_u.opt_mreq.imr_address);
                        break;

                    case OPT_MREQN:
                        memcpy(&val.option_value_u.opt_mreqn.imr_multiaddr,
                               &opt->multiaddr, sizeof(opt->multiaddr));
                        val.option_value_u.opt_mreqn.imr_multiaddr =
                          ntohl(val.option_value_u.opt_mreqn.imr_multiaddr);
                        memcpy(&val.option_value_u.opt_mreqn.imr_address,
                               &opt->address, sizeof(opt->address));
                        val.option_value_u.opt_mreqn.imr_address =
                          ntohl(val.option_value_u.opt_mreqn.imr_address);
                        val.option_value_u.opt_mreqn.imr_ifindex =
                            opt->ifindex;
                        break;

                    default:
                        ERROR("Unknown option type for %s get request",
                              sockopt_rpc2str(optname));
                        break;
                }
                break;
            }

            case RPC_IP_ADD_SOURCE_MEMBERSHIP:
            case RPC_IP_DROP_SOURCE_MEMBERSHIP:
            case RPC_IP_BLOCK_SOURCE:
            case RPC_IP_UNBLOCK_SOURCE:
                mreq_source_cpy((tarpc_mreq_source *)optval, &val, optname);
                break;

            case RPC_TCP_INFO:
                val.opttype = OPT_TCP_INFO;

#define COPY_TCP_INFO_FIELD(_name) \
    val.option_value_u.opt_tcp_info._name = \
        ((struct rpc_tcp_info *)optval)->_name

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
                COPY_TCP_INFO_FIELD(tcpi_rcv_rtt);
                COPY_TCP_INFO_FIELD(tcpi_rcv_space);
                COPY_TCP_INFO_FIELD(tcpi_total_retrans);

#undef COPY_TCP_INFO_FIELD

                break;

            case RPC_IPV6_NEXTHOP:
                val.opttype = OPT_IPADDR6;
                memcpy(val.option_value_u.opt_ipaddr6, optval,
                       sizeof(val.option_value_u.opt_ipaddr6));
                break;

            default:
                val.opttype = OPT_INT;
                val.option_value_u.opt_int = *(int *)optval;
                break;
        }
    }
    if (raw_optval != NULL)
    {
        in.raw_optval.raw_optval_len = raw_roptlen;
        in.raw_optval.raw_optval_val = raw_optval;
    }
    if (raw_optlen != NULL)
    {
        in.raw_optlen.raw_optlen_len = 1;
        in.raw_optlen.raw_optlen_val = raw_optlen;
    }

    rcf_rpc_call(rpcs, "getsockopt", &in, &out);

    if (RPC_IS_CALL_OK(rpcs))
    {
        if (optval != NULL)
        {
            opt_val_str = te_log_buf_alloc();
            switch (optname)
            {
                case RPC_SO_LINGER:
                    ((tarpc_linger *)optval)->l_onoff =
                        out.optval.optval_val[0].option_value_u.
                            opt_linger.l_onoff;
                    ((tarpc_linger *)optval)->l_linger =
                       out.optval.optval_val[0].option_value_u.
                            opt_linger.l_linger;
                    te_log_buf_append(opt_val_str,
                        "{ l_onoff: %d, l_linger: %d }",
                        ((tarpc_linger *)optval)->l_onoff,
                        ((tarpc_linger *)optval)->l_linger);
                    break;

                case RPC_SO_RCVTIMEO:
                case RPC_SO_SNDTIMEO:
                    *((tarpc_timeval *)optval) =
                        out.optval.optval_val[0].option_value_u.
                            opt_timeval;
                    te_log_buf_append(opt_val_str,
                        "{ tv_sec: %" TE_PRINTF_64 "d, "
                        "tv_usec: %" TE_PRINTF_64 "d }",
                        ((tarpc_timeval *)optval)->tv_sec,
                        ((tarpc_timeval *)optval)->tv_usec);
                    break;

                case RPC_IP_ADD_MEMBERSHIP:
                case RPC_IP_DROP_MEMBERSHIP:
                case RPC_IP_MULTICAST_IF:
                {
                    tarpc_mreqn *opt = (tarpc_mreqn *)optval;
                    char         addr_buf[INET_ADDRSTRLEN];
                    char         addr_buf2[INET_ADDRSTRLEN];

                    memset(opt, 0, sizeof(*opt));
                    opt->type = out.optval.optval_val[0].opttype;
                    switch (opt->type)
                    {
                        case OPT_IPADDR:
                            memcpy(&opt->address,
                                   &out.optval.optval_val[0].
                                       option_value_u.opt_ipaddr,
                                   sizeof(opt->address));
                            opt->address = htonl(opt->address);
                            te_log_buf_append(opt_val_str, "{ %s }",
                                inet_ntop(AF_INET, &opt->address,
                                          addr_buf, sizeof(addr_buf)));
                            break;

                        case OPT_MREQ:
                            memcpy(&opt->multiaddr,
                                   &out.optval.optval_val[0].
                                     option_value_u.opt_mreq.imr_multiaddr,
                                   sizeof(opt->multiaddr));
                            opt->multiaddr = htonl(opt->multiaddr);
                            memcpy(&opt->address,
                                   &out.optval.optval_val[0].
                                     option_value_u.opt_mreq.imr_address,
                                   sizeof(opt->address));
                            opt->address = htonl(opt->address);
                            te_log_buf_append(opt_val_str,
                                "{ imr_multiaddr: %s, imr_interface: %s }",
                                inet_ntop(AF_INET, &opt->multiaddr,
                                          addr_buf, sizeof(addr_buf)),
                                inet_ntop(AF_INET, &opt->address,
                                          addr_buf2, sizeof(addr_buf2)));
                            break;

                        case OPT_MREQN:
                            memcpy(&opt->multiaddr,
                                   &out.optval.optval_val[0].
                                     option_value_u.opt_mreqn.imr_multiaddr,
                                   sizeof(opt->multiaddr));
                            opt->multiaddr = htonl(opt->multiaddr);
                            memcpy(&opt->address,
                                   &out.optval.optval_val[0].
                                     option_value_u.opt_mreqn.imr_address,
                                   sizeof(opt->address));
                            opt->address = htonl(opt->address);
                            opt->ifindex =
                                val.option_value_u.opt_mreqn.imr_ifindex;
                            te_log_buf_append(opt_val_str,
                                "{ imr_multiaddr: %s, imr_address: %s, "
                                "imr_ifindex: %d }",
                                inet_ntop(AF_INET, &opt->multiaddr,
                                          addr_buf, sizeof(addr_buf)),
                                inet_ntop(AF_INET, &opt->address,
                                          addr_buf2, sizeof(addr_buf2)),
                                opt->ifindex);
                            break;

                        default:
                            ERROR("Unknown option type for %s get reply",
                                  sockopt_rpc2str(optname));
                            break;
                    }
                    break;
                }

            case RPC_IP_ADD_SOURCE_MEMBERSHIP:
            case RPC_IP_DROP_SOURCE_MEMBERSHIP:
            case RPC_IP_BLOCK_SOURCE:
            case RPC_IP_UNBLOCK_SOURCE:
            {
                tarpc_mreq_source *opt = (tarpc_mreq_source *)optval;
                char addr_buf[3][INET_ADDRSTRLEN] = {{0,}, {0,}, {0,}};

                memset(opt, 0, sizeof(*opt));
                opt->type = out.optval.optval_val[0].opttype;

                if (opt->type != OPT_MREQ_SOURCE)
                {
                    ERROR("Unknown option type for %s get reply",
                          sockopt_rpc2str(optname));
                    break;
                }

                memcpy(&opt->multiaddr, &out.optval.optval_val->
                       option_value_u.opt_mreq_source.imr_multiaddr,
                       sizeof(opt->multiaddr));
                opt->multiaddr = htonl(opt->multiaddr);

                memcpy(&opt->interface, &out.optval.optval_val->
                       option_value_u.opt_mreq_source.imr_interface,
                       sizeof(opt->interface));
                opt->interface = htonl(opt->interface);

                memcpy(&opt->sourceaddr, &out.optval.optval_val->
                       option_value_u.opt_mreq_source.imr_sourceaddr,
                       sizeof(opt->sourceaddr));
                opt->sourceaddr = htonl(opt->sourceaddr);

                te_log_buf_append(opt_val_str,
                    "{ imr_multiaddr: %s, imr_interface: %s, "
                    "imr_sourceaddr: %s }",
                    inet_ntop(AF_INET, &opt->multiaddr, addr_buf[0],
                             sizeof(addr_buf[0])),
                    inet_ntop(AF_INET, &opt->interface, addr_buf[1],
                             sizeof(addr_buf[1])),
                    inet_ntop(AF_INET, &opt->sourceaddr, addr_buf[2],
                             sizeof(addr_buf[2])));
                break;
            }

                case RPC_TCP_INFO:
#define COPY_TCP_INFO_FIELD(_name) \
    do {                                                     \
        ((struct rpc_tcp_info *)optval)->_name =             \
            out.optval.optval_val[0].option_value_u.         \
            opt_tcp_info._name;                              \
        if (strcmp(#_name, "tcpi_state") == 0)               \
            te_log_buf_append(opt_val_str, #_name ": %s ",   \
                 tcp_state_rpc2str(out.optval.optval_val[0]. \
                     option_value_u.opt_tcp_info._name));    \
        else if (strcmp(#_name, "tcpi_ca_state") == 0)       \
            te_log_buf_append(opt_val_str, #_name ": %s ",   \
                 tcp_ca_state_rpc2str(out.optval.            \
                     optval_val[0].option_value_u.           \
                     opt_tcp_info._name));                   \
         else if (strcmp(#_name, "tcpi_options") == 0)       \
            te_log_buf_append(opt_val_str, #_name ": %s ",   \
                 tcpi_options_rpc2str(out.optval.            \
                     optval_val[0].option_value_u.           \
                     opt_tcp_info._name));                   \
        else                                                 \
            te_log_buf_append(opt_val_str, #_name ": %u ",   \
                 out.optval.optval_val[0].                   \
                     option_value_u.opt_tcp_info._name);     \
    } while (0)

                    te_log_buf_append(opt_val_str, "{ ");
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
                    COPY_TCP_INFO_FIELD(tcpi_rcv_rtt);
                    COPY_TCP_INFO_FIELD(tcpi_rcv_space);
                    COPY_TCP_INFO_FIELD(tcpi_total_retrans);

                    te_log_buf_append(opt_val_str, " }");
#undef COPY_TCP_INFO_FIELD
                    break;

                case RPC_IPV6_NEXTHOP:
                {
                    char addr_buf[INET6_ADDRSTRLEN];

                    memcpy(optval,
                           out.optval.optval_val[0].option_value_u.
                           opt_ipaddr6, sizeof(struct in6_addr));
                    te_log_buf_append(opt_val_str, "{ %s }",
                                        inet_ntop(AF_INET6, optval,
                                                  addr_buf,
                                                  sizeof(addr_buf)));
                    break;
                }

                case RPC_SO_TIMESTAMPING:
                    *(int *)optval =
                        out.optval.optval_val[0].option_value_u.opt_int;
                    te_log_buf_append(opt_val_str, "%s",
                        timestamping_flags_rpc2str(*(int *)optval));
                    break;

                default:
                    *(int *)optval =
                        out.optval.optval_val[0].option_value_u.opt_int;
                    if (level == RPC_SOL_SOCKET &&
                        optname == RPC_SO_ERROR)
                    {
                        te_log_buf_append(opt_val_str, "%s",
                                            te_rc_err2str(*(int *)optval));
                    }
                    else
                    {
                        te_log_buf_append(opt_val_str, "%d",
                                            *(int *)optval);
                    }
                    break;
            }
        }
        if (raw_optlen != NULL)
        {
            *raw_optlen = *out.raw_optlen.raw_optlen_val;
        }
        if (raw_optval != NULL)
        {
            unsigned int i;
            te_bool      show_hidden = FALSE;

            if (optname != RPC_IP_PKTOPTIONS || out.retval < 0 ||
                out.optval.optval_val[0].option_value_u.
                    opt_ip_pktoptions.opt_ip_pktoptions_len == 0 ||
                raw_optlen == NULL)
                memcpy(raw_optval, out.raw_optval.raw_optval_val,
                       out.raw_optval.raw_optval_len);
            else if (*out.raw_optlen.raw_optlen_val > 0)
            {
                unsigned int    len = out.optval.optval_val[0].
                                        option_value_u.
                                        opt_ip_pktoptions.
                                        opt_ip_pktoptions_len;

                struct tarpc_cmsghdr *rpc_c = out.optval.optval_val[0].
                                                option_value_u.
                                                opt_ip_pktoptions.
                                                opt_ip_pktoptions_val;

                int      rc;
                size_t   tmp_optlen = raw_roptlen;

                rc = msg_control_rpc2h(rpc_c, len, NULL, 0, raw_optval,
                                       &tmp_optlen);
                if (rc != 0)
                {
                    ERROR("%s(): failed to convert control message",
                          __FUNCTION__);
                    rpcs->_errno = TE_RC(TE_RCF, rc);
                    RETVAL_INT(getsockopt, -1);
                }
                if (raw_optlen != NULL)
                    *raw_optlen = tmp_optlen;
            }

            if (opt_val_str == NULL)
                opt_val_str = te_log_buf_alloc();
            te_log_buf_append(opt_val_str, "[");
            for (i = 0; i < out.raw_optval.raw_optval_len; ++i)
            {
                if (i == (raw_optlen == NULL ? 0 : *raw_optlen))
                {
                    show_hidden = TRUE;
                    te_log_buf_append(opt_val_str, " (");
                }
                te_log_buf_append(opt_val_str, " %#02x",
                                    ((uint8_t *)raw_optval)[i]);
            }
            te_log_buf_append(opt_val_str, "%s ]",
                                show_hidden ? " )" : "");
        }
    }

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(getsockopt, out.retval);
    TAPI_RPC_LOG(rpcs, getsockopt, "%d, %s, %s, %p, %s",
                 "%d optval=%s raw_optlen=%d",
                 s, socklevel_rpc2str(level), sockopt_rpc2str(optname),
                 optval != NULL ? optval : raw_optval, opt_len_str,
                 out.retval,
                 opt_val_str == NULL ? "(nil)" :
                                       te_log_buf_get(opt_val_str),
                 (raw_optlen == NULL) ? -1 : (int)*raw_optlen);

    te_log_buf_free(opt_val_str);

    RETVAL_INT(getsockopt, out.retval);
}

int
rpc_setsockopt_gen(rcf_rpc_server *rpcs,
                   int s, rpc_socklevel level, rpc_sockopt optname,
                   const void *optval,
                   const void *raw_optval, socklen_t raw_optlen,
                   socklen_t raw_roptlen)
{
    tarpc_setsockopt_in     in;
    tarpc_setsockopt_out    out;
    struct option_value     val;
    te_log_buf           *opt_val_str = NULL;
    char                    opt_len_str[32] = { 0, };

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(setsockopt, -1);
    }
    if (raw_optval == NULL && raw_roptlen != 0)
    {
        ERROR("%s(): 'raw_roptlen' must be 0, if 'raw_optval' is NULL",
              __FUNCTION__);
        rpcs->_errno = TE_RC(TE_TAPI, TE_EINVAL);
        RETVAL_INT(setsockopt, -1);
    }
    if (raw_optval != NULL && raw_roptlen < raw_optlen)
    {
        ERROR("%s(): 'raw_roptlen' must be greater or equal to "
              "'raw_optlen'", __FUNCTION__);
        rpcs->_errno = TE_RC(TE_TAPI, TE_EINVAL);
        RETVAL_INT(setsockopt, -1);
    }

    rpcs->op = RCF_RPC_CALL_WAIT;

    in.s = s;
    in.level = level;
    in.optname = optname;
    in.raw_optlen = raw_optlen;

    if (optval != NULL)
        TE_SPRINTF(opt_len_str, "AUTO");
    if (raw_optval != NULL || raw_optlen != 0 || optval == NULL)
        snprintf(opt_len_str + strlen(opt_len_str),
                 sizeof(opt_len_str) - strlen(opt_len_str),
                 "%s%u", optval != NULL ? "+" : "", (unsigned)raw_optlen);

    if (optval != NULL)
    {
        opt_val_str = te_log_buf_alloc();

        in.optval.optval_len = 1;
        in.optval.optval_val = &val;
        switch (optname)
        {
            case RPC_SO_LINGER:
                val.opttype = OPT_LINGER;
                val.option_value_u.opt_linger =
                    *((tarpc_linger *)optval);
                te_log_buf_append(opt_val_str,
                    "{ l_onoff: %d, l_linger: %d }",
                    ((tarpc_linger *)optval)->l_onoff,
                    ((tarpc_linger *)optval)->l_linger);
                break;

            case RPC_SO_RCVTIMEO:
            case RPC_SO_SNDTIMEO:
                val.opttype = OPT_TIMEVAL;
                val.option_value_u.opt_timeval =
                    *((tarpc_timeval *)optval);
                te_log_buf_append(opt_val_str,
                    "{ tv_sec: %" TE_PRINTF_64 "d, "
                    "tv_usec: %" TE_PRINTF_64 "d }",
                    ((tarpc_timeval *)optval)->tv_sec,
                    ((tarpc_timeval *)optval)->tv_usec);
                break;

            case RPC_IPV6_PKTOPTIONS:
                ERROR("IPV6_PKTOPTIONS is not supported yet");
                rpcs->_errno = TE_RC(TE_TAPI, TE_ENOMEM);
                RETVAL_INT(setsockopt, -1);
                break;

            case RPC_IPV6_ADD_MEMBERSHIP:
            case RPC_IPV6_DROP_MEMBERSHIP:
            case RPC_IPV6_JOIN_ANYCAST:
            case RPC_IPV6_LEAVE_ANYCAST:
            {
                char buf[INET6_ADDRSTRLEN];

                inet_ntop(AF_INET6,
                          &((struct ipv6_mreq *)optval)->ipv6mr_multiaddr,
                          buf, sizeof(buf));

                val.opttype = OPT_MREQ6;
                val.option_value_u.opt_mreq6.ipv6mr_multiaddr.
                ipv6mr_multiaddr_val = (uint32_t *)
                    &((struct ipv6_mreq *)optval)->ipv6mr_multiaddr;

                val.option_value_u.opt_mreq6.ipv6mr_multiaddr.
                ipv6mr_multiaddr_len = sizeof(struct in6_addr);

                val.option_value_u.opt_mreq6.ipv6mr_ifindex =
                    ((struct ipv6_mreq *)optval)->ipv6mr_interface;

                te_log_buf_append(opt_val_str,
                    "{ multiaddr: %s, ifindex: %d }",
                    buf, val.option_value_u.opt_mreq6.ipv6mr_ifindex);
                break;
            }

            case RPC_IP_ADD_MEMBERSHIP:
            case RPC_IP_DROP_MEMBERSHIP:
            case RPC_IP_MULTICAST_IF:
            {
                tarpc_mreqn *opt = (tarpc_mreqn *)optval;

                char addr_buf1[INET_ADDRSTRLEN];
                char addr_buf2[INET_ADDRSTRLEN];

                val.opttype = opt->type;

                switch (opt->type)
                {
                    case OPT_IPADDR:
                    {
                        if (optname != RPC_IP_MULTICAST_IF)
                        {
                            ERROR("%s socket option does not support "
                                  "'struct in_addr' argument",
                                  sockopt_rpc2str(optname));
                            rpcs->_errno = TE_RC(TE_TAPI, TE_EINVAL);
                            RETVAL_INT(setsockopt, -1);
                        }

                        memcpy(&val.option_value_u.opt_ipaddr,
                               (char *)&(opt->address),
                               sizeof(opt->address));
                        val.option_value_u.opt_ipaddr =
                            ntohl(val.option_value_u.opt_ipaddr);
                        te_log_buf_append(opt_val_str, "{ %s }",
                                            inet_ntop(AF_INET,
                                                (char *)&(opt)->address,
                                                addr_buf1,
                                                sizeof(addr_buf1)));
                        break;
                    }
                    case OPT_MREQ:
                    {
                        memcpy(&val.option_value_u.opt_mreq.imr_multiaddr,
                              (char *)&(opt->multiaddr),
                              sizeof(opt->multiaddr));
                        val.option_value_u.opt_mreq.imr_multiaddr =
                          ntohl(val.option_value_u.opt_mreq.imr_multiaddr);
                        memcpy(&val.option_value_u.opt_mreq.imr_address,
                               (char *)&(opt->address),
                               sizeof(opt->address));
                        val.option_value_u.opt_mreq.imr_address =
                          ntohl(val.option_value_u.opt_mreq.imr_address);

                        te_log_buf_append(opt_val_str,
                            "{ imr_multiaddr: %s, imr_interface: %s }",
                            inet_ntop(AF_INET,
                                      (char *)&(opt->multiaddr),
                                       addr_buf1, sizeof(addr_buf1)),
                            inet_ntop(AF_INET,
                                      (char *)&(opt->address),
                                       addr_buf2, sizeof(addr_buf2)));
                        break;
                    }
                    case OPT_MREQN:
                    {
                        memcpy(&val.option_value_u.opt_mreqn.imr_multiaddr,
                              (char *)&(opt->multiaddr),
                              sizeof(opt->multiaddr));
                        val.option_value_u.opt_mreqn.imr_multiaddr =
                          ntohl(val.option_value_u.opt_mreqn.imr_multiaddr);
                        memcpy(&val.option_value_u.opt_mreqn.imr_address,
                               (char *)&(opt->address),
                               sizeof(opt->address));
                        val.option_value_u.opt_mreqn.imr_address =
                          ntohl(val.option_value_u.opt_mreqn.imr_address);
                        val.option_value_u.opt_mreqn.imr_ifindex =
                        opt->ifindex;

                        te_log_buf_append(opt_val_str,
                            "{ imr_multiaddr: %s, imr_address: %s, "
                            "imr_ifindex: %d }",
                            inet_ntop(AF_INET,
                                      (char *)&(opt->multiaddr),
                                       addr_buf1, sizeof(addr_buf1)),
                            inet_ntop(AF_INET,
                                      (char *)&(opt->address),
                                       addr_buf2, sizeof(addr_buf2)),
                            opt->ifindex);
                        break;
                    }

                    default:
                    {
                        ERROR("Invalid argument type %d for socket option",
                              opt->type);
                        rpcs->_errno = TE_RC(TE_TAPI, TE_EINVAL);
                        RETVAL_INT(setsockopt, -1);
                    }

                }

                break;
            }

            case RPC_IP_ADD_SOURCE_MEMBERSHIP:
            case RPC_IP_DROP_SOURCE_MEMBERSHIP:
            case RPC_IP_BLOCK_SOURCE:
            case RPC_IP_UNBLOCK_SOURCE:
            {
                char addr_buf[3][INET_ADDRSTRLEN] = {{0,}, {0,}, {0,}};
                tarpc_mreq_source *opt = (tarpc_mreq_source *)optval;

                if (!mreq_source_cpy(opt, &val, optname))
                    break;

                te_log_buf_append(opt_val_str,
                    "{ imr_multiaddr: %s, imr_interface: %s, "
                    "imr_sourceaddr: %s }",
                    inet_ntop(AF_INET, &opt->multiaddr, addr_buf[0],
                             sizeof(addr_buf[0])),
                    inet_ntop(AF_INET, &opt->interface, addr_buf[1],
                             sizeof(addr_buf[1])),
                    inet_ntop(AF_INET, &opt->sourceaddr, addr_buf[2],
                             sizeof(addr_buf[2])));
                break;
            }

            case RPC_MCAST_JOIN_GROUP:
            case RPC_MCAST_LEAVE_GROUP:
            {
                char gr_addr_buf[INET_ADDRSTRLEN];
                struct group_req *opt = (struct group_req *)optval;
                struct sockaddr *group = (struct sockaddr *)&opt->gr_group;
                struct sockaddr_in *group_in = (struct sockaddr_in *)group;

                val.opttype = OPT_GROUP_REQ;
                val.option_value_u.opt_group_req.gr_interface =
                    opt->gr_interface;
                sockaddr_input_h2rpc(group,
                            &val.option_value_u.opt_group_req.gr_group);

                if (group->sa_family == AF_INET)
                {
                    te_log_buf_append(opt_val_str,
                                      "{ gr_group: %s, gr_interface: %d }",
                                      inet_ntop(AF_INET,
                                                &group_in->sin_addr,
                                                gr_addr_buf,
                                                sizeof(gr_addr_buf)),
                                      opt->gr_interface);
                }
                break;
            }

            case RPC_SO_UPDATE_ACCEPT_CONTEXT:
                val.opttype = OPT_HANDLE;
                val.option_value_u.opt_handle = *(int *)optval;
                break;

            case RPC_IPV6_NEXTHOP:
                val.opttype = OPT_IPADDR6;
                memcpy(val.option_value_u.opt_ipaddr6, optval,
                       sizeof(struct in6_addr));
                break;

            case RPC_SO_TIMESTAMPING:
                val.opttype = OPT_INT;
                val.option_value_u.opt_int = *(int *)optval;
                te_log_buf_append(opt_val_str, "%s",
                    timestamping_flags_rpc2str(*(int *)optval));
                break;

            default:
                val.opttype = OPT_INT;
                val.option_value_u.opt_int = *(int *)optval;
                te_log_buf_append(opt_val_str, "%d", *(int *)optval);
                break;
        }
    }

    if (raw_optval != NULL)
    {
        unsigned int i;
        te_bool      show_hidden = FALSE;

        if (opt_val_str == NULL)
            opt_val_str = te_log_buf_alloc();

        in.raw_optval.raw_optval_len = raw_roptlen;
        in.raw_optval.raw_optval_val = (void *)raw_optval;

        if (optname == RPC_SO_BINDTODEVICE &&
            strnlen(raw_optval, raw_roptlen) < raw_roptlen)
            te_log_buf_append(opt_val_str, "%s ", raw_optval);

        te_log_buf_append(opt_val_str, "[");
        for (i = 0; i < raw_roptlen; ++i)
        {
            if (i == raw_optlen)
            {
                show_hidden = TRUE;
                te_log_buf_append(opt_val_str, " (");
            }
            te_log_buf_append(opt_val_str, " %#02x",
                                ((uint8_t *)raw_optval)[i]);
        }
        te_log_buf_append(opt_val_str, "%s ]", show_hidden ? " )" : "");
    }

    rcf_rpc_call(rpcs, "setsockopt", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(setsockopt, out.retval);
    TAPI_RPC_LOG(rpcs, setsockopt, "%d, %s, %s, %s, %s", "%d",
                 s, socklevel_rpc2str(level), sockopt_rpc2str(optname),
                 opt_val_str == NULL ? "(nil)" :
                                       te_log_buf_get(opt_val_str),
                 opt_len_str, out.retval);

    te_log_buf_free(opt_val_str);

    RETVAL_INT(setsockopt, out.retval);
}

int
rpc_recvmmsg_alt(rcf_rpc_server *rpcs, int fd, struct rpc_mmsghdr *mmsg,
                 unsigned int vlen, rpc_send_recv_flags flags,
                 struct tarpc_timespec *timeout)
{
    te_string str_msg = TE_STRING_INIT_STATIC(4096);

    char                   str_buf[256] = {0};
    tarpc_recvmmsg_alt_in  in;
    tarpc_recvmmsg_alt_out out;

    struct tarpc_mmsghdr *tarpc_mmsg = NULL;
    unsigned int          j;
    te_errno              rc;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(recvmmsg_alt, -1);
    }

    in.fd = fd;
    in.flags = flags;
    in.vlen = vlen;

    if (timeout != NULL && rpcs->op != RCF_RPC_WAIT)
    {
        in.timeout.timeout_len = 1;
        in.timeout.timeout_val = timeout;
    }

    if (mmsg != NULL && rpcs->op != RCF_RPC_WAIT)
    {
        rc = mmsghdrs_rpc2tarpc(mmsg, vlen, &tarpc_mmsg, TRUE);
        if (rc != 0)
        {
            rpcs->_errno = TE_RC(TE_TAPI, rc);
            RETVAL_INT(recvmmsg_alt, -1);
        }

        in.mmsg.mmsg_val = tarpc_mmsg;
        in.mmsg.mmsg_len = vlen;
    }

    rcf_rpc_call(rpcs, "recvmmsg_alt", &in, &out);
    tarpc_mmsghdrs_free(tarpc_mmsg, vlen);

    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(recvmmsg_alt, out.retval);

    if (RPC_IS_CALL_OK(rpcs) && rpcs->op != RCF_RPC_WAIT &&
        mmsg != NULL && out.mmsg.mmsg_val != NULL)
    {
        rc = mmsghdrs_tarpc2rpc(out.mmsg.mmsg_val, mmsg,
                                out.mmsg.mmsg_len);
        if (rc != 0)
        {
            rpcs->_errno = TE_RC(TE_TAPI, rc);
            RETVAL_INT(recvmmsg_alt, -1);
        }
    }

    if (timeout == NULL)
    {
        TE_SPRINTF(str_buf, "(nil)");
    }
    else
    {
        TE_SPRINTF(str_buf, "{ %lld, %lld }",
                   (long long int)timeout->tv_sec,
                   (long long int)timeout->tv_nsec);
    }
    /* To avoid too bad output in case of some sprintf() failure. */
    str_buf[sizeof(str_buf) - 1] = '\0';

    TAPI_RPC_LOG(rpcs, recvmmsg_alt, "%d, %p (%s), %u, %s, %s", "%d",
                 fd, mmsg, mmsghdrs_rpc2str(mmsg, vlen, &str_msg),
                 vlen, send_recv_flags_rpc2str(flags), str_buf, out.retval);

    if (RPC_IS_CALL_OK(rpcs) && rpcs->op != RCF_RPC_WAIT &&
        mmsg != NULL && out.mmsg.mmsg_val != NULL && out.retval >= 0)
    {
        for (j = 0; j < out.mmsg.mmsg_len; j++)
            msghdr_check_msg_flags(&mmsg[j].msg_hdr, (int)j < out.retval);
    }

    RETVAL_INT(recvmmsg_alt, out.retval);
}

int
rpc_sendmmsg_alt(rcf_rpc_server *rpcs, int fd, struct rpc_mmsghdr *mmsg,
                 unsigned int vlen, rpc_send_recv_flags flags)
{
    te_string str_msg = TE_STRING_INIT_STATIC(4096);

    tarpc_sendmmsg_alt_in  in;
    tarpc_sendmmsg_alt_out out;

    tarpc_mmsghdr        *tarpc_mmsg = NULL;
    unsigned int          j;
    te_errno              rc;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(sendmmsg_alt, -1);
    }

    in.fd = fd;
    in.flags = flags;
    in.vlen = vlen;

    if (mmsg != NULL && rpcs->op != RCF_RPC_WAIT)
    {
        rc = mmsghdrs_rpc2tarpc(mmsg, vlen, &tarpc_mmsg, FALSE);
        if (rc != 0)
        {
            rpcs->_errno = TE_RC(TE_TAPI, rc);
            RETVAL_INT(sendmmsg_alt, -1);
        }

        in.mmsg.mmsg_val = tarpc_mmsg;
        in.mmsg.mmsg_len = vlen;
    }

    rcf_rpc_call(rpcs, "sendmmsg_alt", &in, &out);
    tarpc_mmsghdrs_free(tarpc_mmsg, vlen);

    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(sendmmsg_alt, out.retval);

    if (RPC_IS_CALL_OK(rpcs) && rpcs->op != RCF_RPC_WAIT &&
        mmsg != NULL && out.mmsg.mmsg_val != NULL)
    {
        /*
         * Reverse conversion is not done because this function should
         * not change anything except msg_len fields, and that nothing
         * else changed is checked with tarpc_check_args() on TA side.
         */
        if (out.mmsg.mmsg_len > vlen)
        {
            ERROR("%s(): too many mmsghdr structures were retrieved "
                  "from TA", __FUNCTION__);
            rpcs->_errno = TE_RC(TE_TAPI, TE_EINVAL);
            RETVAL_INT(sendmmsg_alt, -1);
        }
        for (j = 0; j < out.mmsg.mmsg_len; j++)
            mmsg[j].msg_len = out.mmsg.mmsg_val[j].msg_len;
    }

    TAPI_RPC_LOG(rpcs, sendmmsg_alt, "%d, %p (%s), %u, %s", "%d",
                 fd, mmsg, mmsghdrs_rpc2str(mmsg, vlen, &str_msg),
                 vlen, send_recv_flags_rpc2str(flags), out.retval);
    RETVAL_INT(sendmmsg_alt, out.retval);
}

int
rpc_socket_connect_close(rcf_rpc_server *rpcs,
                         rpc_socket_domain domain,
                         const struct sockaddr *addr,
                         uint32_t time2run)
{
    tarpc_socket_connect_close_in  in;
    tarpc_socket_connect_close_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(socket_connect_close, -1);
    }

    in.domain = domain;
    in.time2run = time2run;
    sockaddr_input_h2rpc(addr, &in.addr);

    rpcs->errno_change_check = 0;
    rcf_rpc_call(rpcs, "socket_connect_close", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(socket_connect_close, out.retval);
    TAPI_RPC_LOG(rpcs, socket_connect_close, "%s, %s, %d", "%d",
                 domain_rpc2str(domain), sockaddr_h2str(addr), time2run,
                 out.retval);
    RETVAL_INT(socket_connect_close, out.retval);
}

int
rpc_socket_listen_close(rcf_rpc_server *rpcs,
                        rpc_socket_domain domain,
                        const struct sockaddr *addr,
                        uint32_t time2run)
{
    tarpc_socket_listen_close_in  in;
    tarpc_socket_listen_close_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(socket_listen_close, -1);
    }

    in.domain = domain;
    in.time2run = time2run;
    sockaddr_input_h2rpc(addr, &in.addr);

    rcf_rpc_call(rpcs, "socket_listen_close", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(socket_listen_close, out.retval);
    TAPI_RPC_LOG(rpcs, socket_listen_close, "%s, %s, %d", "%d",
                 domain_rpc2str(domain), sockaddr_h2str(addr), time2run,
                 out.retval);
    RETVAL_INT(socket_listen_close, out.retval);
}

int
rpc_setsockopt_check_int(rcf_rpc_server *rpcs,
                         int s, rpc_sockopt optname, int optval)
{
    int rc;
    int getval;
    int awaiting_err = rpcs->iut_err_jump;

    rc = rpc_setsockopt_gen(rpcs, s, rpc_sockopt2level(optname),
                            optname, &optval, NULL, 0, 0);
    if (rc != 0)
        return rc;

    rpcs->iut_err_jump = awaiting_err;
    rpc_getsockopt(rpcs, s, optname, &getval);
    if (optval != getval)
    {
        ERROR("Changing %s value failure: set %d, got %d",
              sockopt_rpc2str(optname), optval, getval);
        rpcs->_errno = TE_RC(TE_TAPI, TE_EINVAL);
        rpcs->err_log = TRUE;
        rc = -1;
        if (awaiting_err)
            TAPI_JMP_DO(TE_EFAIL);
    }

    return rc;
}


/**
 * Send ICMPv4 ECHO request packet to @p addr to provoke ARP resolution from
 * both sides.
 *
 * @note The function jumps to @b cleanup in case of failure.
 *
 * @param rpcs  RPC server handle.
 * @param addr  Destination address, should be IPv4 family
 */
static void
tapi_rpc_send_icmp4_echo(struct rcf_rpc_server *rpcs,
                         const struct sockaddr *addr)
{
#ifdef HAVE_NETINET_IP_ICMP_H
    struct icmphdr *icp;
    char            buf[ICMP_DATALEN + sizeof(*icp)] = {0};
    size_t          req_len = sizeof(buf);
    rpc_msghdr      msg;
    rpc_iovec       iov;
    int             sock;

    memset(&msg, 0, sizeof(msg));
    memset(&iov, 0, sizeof(iov));

    sock = rpc_socket(rpcs, rpc_socket_domain_by_addr(addr),
                      RPC_SOCK_RAW, RPC_IPPROTO_ICMP);

    icp = (struct icmphdr *)buf;
    icp->type = ICMP_ECHO;
    icp->checksum = calculate_checksum(buf, req_len);
    icp->checksum = ~icp->checksum & 0xffff;

    iov.iov_base = buf;
    iov.iov_len = iov.iov_rlen = req_len;
    msg.msg_name = SA(addr);
    msg.msg_namelen = te_sockaddr_get_size(addr);
    msg.msg_iov = &iov;
    msg.msg_iovlen = msg.msg_riovlen = 1;

    rpc_sendmsg(rpcs, sock, &msg, 0);
    rpc_close(rpcs, sock);
#else
    UNUSED(rpcs);
    UNUSED(addr);
    TEST_FAIL("Cannot create ICMP message due to lack of netinet/ip_icmp.h");
#endif
}

/**
 * Send ICMPv6 ECHO request packet to @p addr to provoke ARP resolution from
 * both sides.
 *
 * @note The function jumps to @b cleanup in case of failure.
 *
 * @param rpcs  RPC server handle.
 * @param addr  Destination address, should be IPv6 family
 */
static void
tapi_rpc_send_icmp6_echo(struct rcf_rpc_server *rpcs,
                    const struct sockaddr *addr)
{
#ifdef HAVE_NETINET_ICMP6_H
    struct icmp6_hdr        icp = {0};
    rpc_msghdr              msg = {0};
    rpc_iovec               iov = {0};
    int                     sock;
    struct sockaddr_storage ping_addr;

    sock = rpc_socket(rpcs, RPC_PF_INET6, RPC_SOCK_RAW, RPC_IPPROTO_ICMPV6);

    tapi_sockaddr_clone_exact(addr, &ping_addr);
    te_sockaddr_set_port(SA(&ping_addr), 0);

    icp.icmp6_type = ICMP6_ECHO_REQUEST;
    icp.icmp6_code = 0;

    iov.iov_base = &icp;
    iov.iov_len = iov.iov_rlen = sizeof(icp);
    msg.msg_name = SA(&ping_addr);
    msg.msg_namelen = te_sockaddr_get_size(SA(&ping_addr));
    msg.msg_iov = &iov;
    msg.msg_iovlen = msg.msg_riovlen = 1;

    rpc_sendmsg(rpcs, sock, &msg, 0);
    rpc_close(rpcs, sock);
#else
    UNUSED(rpcs);
    UNUSED(addr);
    TEST_FAIL("Cannot create ICMPv6 message due to lack of netinet/icmp6.h");
#endif
}

/* See description in the tapi_rpc_socket.h */
void
tapi_rpc_provoke_arp_resolution(struct rcf_rpc_server *rpcs,
                                const struct sockaddr *addr)
{
    if (addr->sa_family == AF_INET)
        tapi_rpc_send_icmp4_echo(rpcs, addr);
    else if (addr->sa_family == AF_INET6)
        tapi_rpc_send_icmp6_echo(rpcs, addr);
    else
        TEST_FAIL("Address family %d is not supported", addr->sa_family);
}
