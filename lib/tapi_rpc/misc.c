/** @file
 * @brief Test API - RPC
 *
 * TAPI for auxilury remote socket calls implementation
 *
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
 *
 *
 * @author Elena A. Vengerova <Elena.Vengerova@oktetlabs.ru>
 *
 * $Id$
 */

#include "te_config.h"

#include <stdio.h>
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#endif
#if HAVE_STRINGS_H
#include <strings.h>
#endif
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#if HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif
#if HAVE_NET_IF_H
#include <net/if.h>
#endif

#include "te_defs.h"
#include "te_printf.h"
#include "te_string.h"
#include "tq_string.h"
#include "tapi_rpc_internal.h"
#include "tapi_rpc_unistd.h"
#include "tapi_rpc_misc.h"
#include "tapi_rpc_winsock2.h"
#include "tapi_rpc_signal.h"
#include "tapi_test.h"
#include "tapi_cfg.h"
#include "tapi_cfg_base.h"
#include "tapi_host_ns.h"

/* See description in tapi_rpc_misc.h */

te_bool
rpc_find_func(rcf_rpc_server *rpcs, const char * func_name)
{
    struct tarpc_rpc_find_func_in  in;
    struct tarpc_rpc_find_func_out out;

    int rc;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(rpc_find_func, -1);
    }

    if (func_name == NULL)
    {
        ERROR("%s(): NULL type name", __FUNCTION__);
        RETVAL_INT(rpc_find_func, -1);
    }

    in.func_name = strdup(func_name);

    rcf_rpc_call(rpcs, "rpc_find_func", &in, &out);

    free(in.func_name);
    rc = out.find_result;

    CHECK_RETVAL_VAR(rpc_find_func, rc, (rc < 0), -1);
    TAPI_RPC_LOG(rpcs, rpc_find_func, "%s", "%s", func_name,
                 errno_rpc2str(rc));
    RETVAL_INT(rpc_find_func, rc);
}

/* See description in tapi_rpc_misc.h */
int
rpc_vlan_get_parent(rcf_rpc_server *rpcs, const char *vlan_ifname,
                    char *parent_ifname)
{   
    int rc;

    struct tarpc_vlan_get_parent_in  in;
    struct tarpc_vlan_get_parent_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(vlan_get_parent, -1);
    }

    if (vlan_ifname == NULL)
    {
        ERROR("%s(): NULL interface name", __FUNCTION__);
        RETVAL_INT(vlan_get_parent, -1);
    }

    if (parent_ifname == NULL)
    {
        ERROR("%s(): Pointer parent_ifname is NULL", __FUNCTION__);
        RETVAL_INT(vlan_get_parent, -1);
    }

    in.ifname.ifname_val = strdup(vlan_ifname);
    in.ifname.ifname_len = strlen(vlan_ifname) + 1;

    rcf_rpc_call(rpcs, "vlan_get_parent", &in, &out);

    free(in.ifname.ifname_val);
    memcpy(parent_ifname, out.ifname.ifname_val,
           out.ifname.ifname_len);

    rc = out.retval;

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(rpc_vlan_get_parent, rc);
    TAPI_RPC_LOG(rpcs, rpc_vlan_get_parent, "%s, %p( %s )", "%d",
                 vlan_ifname, parent_ifname, parent_ifname, rc);
    RETVAL_INT(vlan_get_parent, rc);
}

/* See description in tapi_rpc_misc.h */
int
rpc_bond_get_slaves(rcf_rpc_server *rpcs, const char *bond_ifname,
                    tqh_strings *slaves, int *slaves_num)
{
    int          rc;
    unsigned int i;

    struct tarpc_bond_get_slaves_in  in;
    struct tarpc_bond_get_slaves_out out;

    te_string str = TE_STRING_INIT;

    TAILQ_INIT(slaves);
    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (slaves_num != NULL)
        *slaves_num = 0;

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(bond_get_slaves, -1);
    }

    if (bond_ifname == NULL)
    {
        ERROR("%s(): NULL interface name", __FUNCTION__);
        rpcs->_errno = TE_RC(TE_RCF_API, TE_EINVAL);
        RETVAL_INT(bond_get_slaves, -1);
    }

    if (slaves == NULL)
    {
        ERROR("%s(): Pointer to slaves list is NULL", __FUNCTION__);
        rpcs->_errno = TE_RC(TE_RCF_API, TE_EINVAL);
        RETVAL_INT(bond_get_slaves, -1);
    }

    in.ifname.ifname_val = strdup(bond_ifname);
    in.ifname.ifname_len = strlen(bond_ifname) + 1;

    if (in.ifname.ifname_val == NULL)
    {
        ERROR("%s(): out of memory", __FUNCTION__);
        rpcs->_errno = TE_RC(TE_RCF_API, TE_ENOMEM);
        RETVAL_INT(bond_get_slaves, -1);
    }

    rcf_rpc_call(rpcs, "bond_get_slaves", &in, &out);
    free(in.ifname.ifname_val);

    te_string_append(&str, "");
    for (i = 0; i < out.slaves.slaves_len; i++)
    {
        rc = tq_strings_add_uniq_gen(slaves,
                                     out.slaves.slaves_val[i].ifname,
                                     TRUE);
        if (rc == 0)
            rc = te_string_append(&str, "%s%s", (i == 0) ? "" : ", ",
                                  out.slaves.slaves_val[i].ifname);

        if (rc != 0)
        {
            ERROR("%s(): failed to add an interface to the list",
                  __FUNCTION__);
            rpcs->_errno = TE_RC(TE_RCF_API, rc);
            te_string_free(&str);
            tq_strings_free(slaves, &free);
            RETVAL_INT(bond_get_slaves, -1);
        }

    }

    if (slaves_num != NULL)
        *slaves_num = out.slaves.slaves_len;

    rc = out.retval;

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(rpc_bond_get_slaves, rc);
    TAPI_RPC_LOG(rpcs, rpc_bond_get_slaves, "%s, %p(%s), %d", "%d",
                 bond_ifname, slaves, str.ptr, out.slaves.slaves_len, rc);
    te_string_free(&str);
    RETVAL_INT(bond_get_slaves, rc);
}

/* See description in tapi_rpc_misc.h */
tarpc_ssize_t
rpc_get_sizeof(rcf_rpc_server *rpcs, const char *type_name)
{
    struct tarpc_get_sizeof_in  in;
    struct tarpc_get_sizeof_out out;
    int                         rc;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(get_sizeof, -1);
    }

    if (type_name == NULL)
    {
        ERROR("%s(): NULL type name", __FUNCTION__);
        RETVAL_INT(get_sizeof, -1);
    }

    in.typename = strdup(type_name);

    rcf_rpc_call(rpcs, "get_sizeof", &in, &out);

    free(in.typename);
    rc = out.size;

    CHECK_RETVAL_VAR(get_sizeof, rc, (rc < -1), -1);
    TAPI_RPC_LOG(rpcs, get_sizeof, "%s", "%d", type_name, rc);
    RETVAL_INT(get_sizeof, rc);
}

/* See description in tapi_rpc_misc.h */
te_bool 
rpc_protocol_info_cmp(rcf_rpc_server *rpcs, 
                      const uint8_t *buf1,
                      const uint8_t *buf2,
                      tarpc_bool is_wide1,
                      tarpc_bool is_wide2)
{
    struct tarpc_protocol_info_cmp_in  in;
    struct tarpc_protocol_info_cmp_out out;
    tarpc_bool                         rc;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(protocol_info_cmp, -1);
    }

    if (buf1 == NULL || buf2 == NULL)
    {
        ERROR("%s(): NULL buffer", __FUNCTION__);
        RETVAL_INT(protocol_info_cmp, -1);
    }

    in.buf1.buf1_val = (uint8_t *)buf1;
    in.buf2.buf2_val = (uint8_t *)buf2;
    if (is_wide1)
        in.buf1.buf1_len = rpc_get_sizeof(rpcs, "WSAPROTOCOL_INFOW");
    else
        in.buf1.buf1_len = rpc_get_sizeof(rpcs, "WSAPROTOCOL_INFOA");
    if (is_wide2)
        in.buf2.buf2_len = rpc_get_sizeof(rpcs, "WSAPROTOCOL_INFOW");
    else
        in.buf2.buf2_len = rpc_get_sizeof(rpcs, "WSAPROTOCOL_INFOA");
    in.is_wide1 = is_wide1;
    in.is_wide2 = is_wide2;
    

    rcf_rpc_call(rpcs, "protocol_info_cmp", &in, &out);

    rc = out.retval;

    TAPI_RPC_LOG(rpcs, protocol_info_cmp, "", "%d", rc);
    RETVAL_INT(protocol_info_cmp, rc);
}

/* See description in tapi_rpc_misc.h */
rpc_ptr
rpc_get_addrof(rcf_rpc_server *rpcs, const char *name)
{
    struct tarpc_get_addrof_in  in;
    struct tarpc_get_addrof_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_RPC_PTR(get_addrof, RPC_NULL);
    }

    if (name == NULL)
    {
        ERROR("%s(): NULL type name", __FUNCTION__);
        RETVAL_RPC_PTR(get_addrof, RPC_NULL);
    }

    in.name = strdup(name);

    rcf_rpc_call(rpcs, "get_addrof", &in, &out);

    free(in.name);

    TAPI_RPC_LOG(rpcs, get_addrof, "%s", "%u", name, out.addr);
    RETVAL_RPC_PTR(get_addrof, out.addr);
}

/* See description in tapi_rpc_misc.h */
uint64_t
rpc_get_var(rcf_rpc_server *rpcs, const char *name, tarpc_size_t size)
{
    struct tarpc_get_var_in  in;
    struct tarpc_get_var_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL || name == NULL ||
        !(size == 1 || size == 2 || size == 4 || size == 8))
    {
        ERROR("%s(): Invalid parameter is provided", __FUNCTION__);
        TAPI_JMP_DO(TE_EFAIL);
    }

    in.name = strdup(name);
    in.size = size;

    rcf_rpc_call(rpcs, "get_var", &in, &out);

    free(in.name);

    CHECK_RETVAL_VAR_IS_BOOL(get_var, out.found);
    TAPI_RPC_LOG(rpcs, get_var, "%s, %u", "%llu%s",
                 name, size, out.found ? out.val : 0,
                 out.found ? "" : " (not found)");
    TAPI_RPC_OUT(get_var, !out.found);
    return out.val;
}

/* See description in tapi_rpc_misc.h */
void
rpc_set_var(rcf_rpc_server *rpcs, const char *name,
            tarpc_size_t size, uint64_t val)
{
    struct tarpc_set_var_in  in;
    struct tarpc_set_var_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL || name == NULL ||
        !(size == 1 || size == 2 || size == 4 || size == 8))
    {
        ERROR("%s(): Invalid parameter is provided", __FUNCTION__);
        TAPI_JMP_DO(TE_EFAIL);
    }

    in.name = strdup(name);
    in.size = size;
    in.val = val;

    rcf_rpc_call(rpcs, "set_var", &in, &out);

    free(in.name);

    CHECK_RETVAL_VAR_IS_BOOL(get_var, out.found);
    TAPI_RPC_LOG(rpcs, set_var, "%s, %u, %llu", "%s",
                 name, size, in.val, out.found ? "OK" : "not found");
    TAPI_RPC_OUT(set_var, !out.found);
}


/**
 * Convert I/O vector to array.
 *
 * @param len      total data length
 * @param v        I/O vector
 * @param cnt      number of elements in vector
 *
 * @return Pointer to allocated array or NULL.
 */
static uint8_t *
rpc_iovec_to_array(size_t len, const struct rpc_iovec *v, size_t cnt)
{
    uint8_t *array = malloc(len);
    uint8_t *p     = array;
    size_t   i;

    if (array == NULL)
    {
        ERROR("Allocation of %u bytes failure", len);
        return NULL;
    }

    for (i = 0; i < cnt && len > 0; ++i)
    {
        size_t copylen = (v[i].iov_len < len) ? v[i].iov_len : len;

        memcpy(p, v[i].iov_base, copylen);
        p += copylen;
        len -= copylen;
    }
    if (len != 0)
    {
        ERROR("I/O vector total length is less than length by elements");
        free(array);
        return NULL;
    }
    return array;
}

/* See description in tapi_rpcsock.h */
int
rpc_iovec_cmp(size_t v1len, const struct rpc_iovec *v1, size_t v1cnt,
              size_t v2len, const struct rpc_iovec *v2, size_t v2cnt)
{
    int      result;
    uint8_t *array1 = NULL;
    uint8_t *array2 = NULL;

    if (v1len != v2len)
        return -1;

    array1 = rpc_iovec_to_array(v1len, v1, v1cnt);
    array2 = rpc_iovec_to_array(v2len, v2, v2cnt);
    if (array1 == NULL || array2 == NULL)
    {
        result = -1;
    }
    else
    {
        result = (memcmp(array1, array2, v1len) == 0) ? 0 : -1;
    }

    free(array1);
    free(array2);

    return result;
}


/**
 * Convert 'struct tarpc_timeval' to string.
 *
 * @note Static buffer is used for return value.
 *
 * @param tv        Pointer to 'struct tarpc_timeval'
 *
 * @return NUL-terminated string
 */
const char *
tarpc_timeval2str(const struct tarpc_timeval *tv)
{

/* Number of buffers used in the function */
#define N_BUFS 10
#define BUF_SIZE 32

    static char buf[N_BUFS][BUF_SIZE];
    static char (*cur_buf)[BUF_SIZE] = (char (*)[BUF_SIZE])buf[0];

    char *ptr;

    /*
     * Firt time the function is called we start from the second buffer, but
     * then after a turn we'll use all N_BUFS buffer.
     */
    if (cur_buf == (char (*)[BUF_SIZE])buf[N_BUFS - 1])
        cur_buf = (char (*)[BUF_SIZE])buf[0];
    else
        cur_buf++;

    ptr = *cur_buf;

    if (tv == NULL)
    {
        snprintf(ptr, BUF_SIZE, "(nil)");
    }
    else
    {
        snprintf(ptr, BUF_SIZE, "{%ld,%ld}",
                 (long)tv->tv_sec, (long)tv->tv_usec);
    }

#undef BUF_SIZE
#undef N_BUFS

    return ptr;
}

/**
 * Convert 'struct timespec' to string.
 *
 * @note Static buffer is used for return value.
 *
 * @param tv    - pointer to 'struct timespec'
 *
 * @return null-terminated string
 */
const char *
timespec2str(const struct timespec *tv)
{
    static char buf[32];

    if (tv == NULL)
    {
        strcpy(buf, "(nil)");
    }
    else
    {
        sprintf(buf, "{%lld,%lld}", (long long)tv->tv_sec,
                (long long)tv->tv_nsec);
    }
    return buf;
}

/**
 * Convert 'struct tarpc_timespec' to string.
 *
 * @note Static buffer is used for return value.
 *
 * @param tv    - pointer to 'struct tarpc_timespec'
 *
 * @return null-terminated string
 */
const char *
tarpc_timespec2str(const struct tarpc_timespec *tv)
{
    static char buf[32];

    if (tv == NULL)
    {
        strcpy(buf, "(nil)");
    }
    else
    {
        sprintf(buf, "{%lld,%lld}", (long long)tv->tv_sec,
                (long long)tv->tv_nsec);
    }
    return buf;
}

/**
 * Convert 'struct tarpc_hwtstamp_config' to string.
 *
 * @note Static buffer is used for return value.
 *
 * @param hw_cfg     pointer to 'struct tarpc_hwtstamp_config'
 *
 * @return null-terminated string
 */
const char *
tarpc_hwtstamp_config2str(const tarpc_hwtstamp_config *hw_cfg)
{
    static char buf[32];

    if (hw_cfg == NULL)
        strcpy(buf, "(nil)");
    else
        sprintf(buf, "{%d,%d,%d}", hw_cfg->flags,
                hw_cfg->tx_type, hw_cfg->rx_filter);
    return buf;
}

/**
 * Simple sender.
 *
 * @param rpcs            RPC server
 * @param s                 a socket to be user for sending
 * @param size_min          minimum size of the message in bytes
 * @param size_max          maximum size of the message in bytes
 * @param size_rnd_once     if true, random size should be calculated
 *                          only once and used for all messages;
 *                          if false, random size is calculated for
 *                          each message
 * @param delay_min         minimum delay between messages in microseconds
 * @param delay_max         maximum delay between messages in microseconds
 * @param delay_rnd_once    if true, random delay should be calculated
 *                          only once and used for all messages;
 *                          if false, random delay is calculated for
 *                          each message
 * @param time2run          how long run (in seconds)
 * @param sent              location for number of sent bytes
 * @param ignore_err        Ignore errors while run
 *
 * @return Number of sent bytes or -1 in the case of failure if
 *         ignore_err set to FALSE or number of sent bytes or 0 if
 *         ignore_err set to TRUE
 */
int
rpc_simple_sender(rcf_rpc_server *rpcs,
                  int s, int size_min, int size_max,
                  int size_rnd_once, int delay_min, int delay_max,
                  int delay_rnd_once, int time2run, uint64_t *sent,
                  int ignore_err)
{
    tarpc_simple_sender_in  in;
    tarpc_simple_sender_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(simple_sender, -1);
    }

    in.s = s;
    in.size_min = size_min;
    in.size_max = size_max;
    in.size_rnd_once = size_rnd_once;
    in.delay_min = delay_min;
    in.delay_max = delay_max;
    in.delay_rnd_once = delay_rnd_once;
    in.time2run = time2run;
    in.ignore_err = ignore_err;

    if (rpcs->timeout == RCF_RPC_UNSPEC_TIMEOUT)
    {
        rpcs->timeout = TE_SEC2MS(time2run + TAPI_RPC_TIMEOUT_EXTRA_SEC);
    }

    rcf_rpc_call(rpcs, "simple_sender", &in, &out);

    if (out.retval == 0)
        *sent = out.bytes;

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(simple_sender, out.retval);
    TAPI_RPC_LOG(rpcs, simple_sender, "%d, %d, %d, %d, %d, %d, %d, %d, %d",
                 "%d %u",
                 s, size_min, size_max, size_rnd_once,
                 delay_min, delay_max, delay_rnd_once,
                 time2run, ignore_err,
                 out.retval, (unsigned int)*sent);
    RETVAL_INT(simple_sender, out.retval);
}

/**
 * Simple receiver.
 *
 * @param rpcs            RPC server
 * @param s               a socket to be user for receiving
 * @param received        location for number of received bytes
 *
 * @return number of received bytes or -1 in the case of failure
 */
int
rpc_simple_receiver(rcf_rpc_server *rpcs,
                    int s, uint32_t time2run, uint64_t *received)
{
    tarpc_simple_receiver_in  in;
    tarpc_simple_receiver_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(simple_receiver, -1);
    }

    in.s = s;
    in.time2run = time2run;
    if (rpcs->timeout == RCF_RPC_UNSPEC_TIMEOUT)
    {
        rpcs->timeout = TE_SEC2MS(time2run + TAPI_RPC_TIMEOUT_EXTRA_SEC);
    }

    rcf_rpc_call(rpcs, "simple_receiver", &in, &out);

    if (out.retval == 0)
        *received = out.bytes;

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(simple_receiver, out.retval);
    TAPI_RPC_LOG(rpcs, simple_receiver, "%d, %d", "%d received=%u",
                 s, time2run, out.retval, (unsigned long)*received);
    RETVAL_INT(simple_receiver, out.retval);
}

/** See description in tapi_rpc_misc.h */
int
rpc_pattern_sender(rcf_rpc_server *rpcs,
                   int s, char *fname, int iomux, int size_min,
                   int size_max, int size_rnd_once, int delay_min,
                   int delay_max, int delay_rnd_once, int time2run,
                   uint64_t *sent, int ignore_err, te_bool *send_failed)
{
    tarpc_pattern_sender_in  in;
    tarpc_pattern_sender_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(pattern_sender, -1);
    }

    if (fname == NULL)
    {
        ERROR("%s(): Invalid pattern generating function name",
              __FUNCTION__);
        RETVAL_INT(pattern_sender, -1);
    }

    in.s = s;
    in.fname.fname_len = strlen(fname) + 1;
    in.fname.fname_val = strdup(fname);
    in.iomux = iomux;
    in.size_min = size_min;
    in.size_max = size_max;
    in.size_rnd_once = size_rnd_once;
    in.delay_min = delay_min;
    in.delay_max = delay_max;
    in.delay_rnd_once = delay_rnd_once;
    in.time2run = time2run;
    in.ignore_err = ignore_err;

    if (rpcs->timeout == RCF_RPC_UNSPEC_TIMEOUT)
        rpcs->timeout = TE_SEC2MS(time2run + TAPI_RPC_TIMEOUT_EXTRA_SEC);

    rcf_rpc_call(rpcs, "pattern_sender", &in, &out);

    free(in.fname.fname_val);

    if (sent != NULL)
        *sent = out.bytes;

    if (send_failed != NULL)
        *send_failed = out.func_failed;

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(pattern_sender, out.retval);
    TAPI_RPC_LOG(rpcs, pattern_sender, "%d, %s, %s, %d, %d, %d, %d, "
                 "%d, %d, %d, %d", "%d sent=%u",
                 s, fname, iomux2str(iomux), size_min, size_max,
                 size_rnd_once, delay_min, delay_max, delay_rnd_once,
                 time2run, ignore_err, out.retval,
                 (unsigned int)out.bytes);
    RETVAL_INT(pattern_sender, out.retval);
}

/** See description in tapi_rpc_misc.h */
int
rpc_pattern_receiver(rcf_rpc_server *rpcs, int s,
                     char *fname, int iomux,
                     uint32_t time2run, uint64_t *received,
                     te_bool *recv_failed)
{
    tarpc_pattern_receiver_in  in;
    tarpc_pattern_receiver_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(pattern_receiver, -1);
    }

    if (fname == NULL)
    {
        ERROR("%s(): Invalid pattern generating function name",
              __FUNCTION__);
        RETVAL_INT(pattern_sender, -1);
    }

    in.s = s;
    in.fname.fname_len = strlen(fname) + 1;
    in.fname.fname_val = strdup(fname);
    in.iomux = iomux;
    in.time2run = time2run;
    if (rpcs->timeout == RCF_RPC_UNSPEC_TIMEOUT)
        rpcs->timeout = TE_SEC2MS(time2run + TAPI_RPC_TIMEOUT_EXTRA_SEC);

    rcf_rpc_call(rpcs, "pattern_receiver", &in, &out);

    free(in.fname.fname_val);

    if (received != NULL)
        *received = out.bytes;

    if (recv_failed != NULL)
        *recv_failed = out.func_failed;

    CHECK_RETVAL_VAR(pattern_receiver, out.retval,
                     !(out.retval <= 0 && out.retval >= -2), -1);
    TAPI_RPC_LOG(rpcs, pattern_receiver, "%d, %s, %s, %d",
                 "%d received=%u", s, fname, iomux2str(iomux), time2run,
                 out.retval, (unsigned long)out.bytes);
    RETVAL_INT(pattern_receiver, out.retval);
}

/**
 * Wait for readable socket.
 *
 * @param rpcs            RPC server
 * @param s               a socket to be user for select
 * @param timeout         Receive timeout (in milliseconds)
 *
 * @return result of select call
 */
int
rpc_wait_readable(rcf_rpc_server *rpcs,
                  int s, uint32_t timeout)
{
    tarpc_wait_readable_in    in;
    tarpc_wait_readable_out   out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(simple_receiver, -1);
    }

    in.s = s;
    in.timeout = timeout;
    if (rpcs->timeout == RCF_RPC_UNSPEC_TIMEOUT)
    {
        rpcs->timeout = timeout + TE_SEC2MS(TAPI_RPC_TIMEOUT_EXTRA_SEC);
    }

    rcf_rpc_call(rpcs, "wait_readable", &in, &out);

#if 0
    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(simple_receiver, out.retval);
#endif

    TAPI_RPC_LOG(rpcs, wait_readable, "%d, %d", "%d",
                 s, timeout, out.retval);
    RETVAL_INT(wait_readable, out.retval);
}


/* See description in tapi_rpc_misc.h */
int
rpc_recv_verify(rcf_rpc_server *rpcs, int s,
                const char *gen_data_fname, uint64_t start)
{
    tarpc_recv_verify_in  in;
    tarpc_recv_verify_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(recv_verify, -1);
    }
    if (gen_data_fname == NULL)
    {
        ERROR("%s(): NULL function name", __FUNCTION__);
        RETVAL_INT(recv_verify, -1);
    }

    RING("%s(): fname %s", __FUNCTION__, gen_data_fname);

    in.s = s;
    in.start = start;

    if (rpcs->op != RCF_RPC_WAIT)
    {
        in.fname.fname_len = strlen(gen_data_fname) + 1;
        in.fname.fname_val = strdup(gen_data_fname);
    }

    rcf_rpc_call(rpcs, "recv_verify", &in, &out);

    free(in.fname.fname_val);

    TAPI_RPC_LOG(rpcs, recv_verify, "%d, %d", "%d",
                 s, (uint32_t)start, out.retval);
    RETVAL_INT(recv_verify, out.retval);
}

/**
 * Convert 'struct tarpc_timeval' to string.
 *
 * @note Static buffer is used for return value.
 *
 * @param tv        Pointer to 'struct tarpc_timeval'
 *
 * @return NUL-terminated string
 */
const char *
tarpc_array2string(int len, const void *array, int member_size)
{

/* Number of buffers used in the function */
#define BUF_SIZE 128
#define N_BUFS  10

    static char buf[N_BUFS][BUF_SIZE];
    static char (*cur_buf)[BUF_SIZE] = (char (*)[BUF_SIZE])buf[0];

    char *ptr;
    int   i;
    int used;

    if (len <= 0 || array == NULL)
        return "";

    /*
     * Firt time the function is called we start from the second buffer, but
     * then after a turn we'll use all N_BUFS buffer.
     */
    if (cur_buf == (char (*)[BUF_SIZE])buf[N_BUFS - 1])
        cur_buf = (char (*)[BUF_SIZE])buf[0];
    else
        cur_buf++;

    ptr = *cur_buf;

    if (member_size == 4)
    {
        const int32_t *arr = array;
        used = snprintf(ptr, BUF_SIZE, "%"TE_PRINTF_32"d", arr[0]);
        for (i = 1; i < len; i++)
        {
            used += snprintf(ptr + used, BUF_SIZE - used,
                             ", %"TE_PRINTF_32"d", arr[i]);
        }
    }
    else if (member_size == 8)
    {
        const int64_t *arr = array;
        used = snprintf(ptr, BUF_SIZE, "%"TE_PRINTF_64"d", arr[0]);
        for (i = 1; i < len; i++)
        {
            used += snprintf(ptr + used, BUF_SIZE - used,
                             ", %"TE_PRINTF_64"d", arr[i]);
        }
    }
    ptr[BUF_SIZE - 1] = '\0';

#undef BUF_SIZE
#undef N_BUFS

    return ptr;
}


/* See description in tapi_rpcsock.h */
int
rpc_iomux_flooder(rcf_rpc_server *rpcs,
                  int *sndrs, int sndnum, int *rcvrs, int rcvnum,
                  int bulkszs, int time2run, int time2wait, int iomux,
                  uint64_t *tx_stat, uint64_t *rx_stat)
{
    tarpc_flooder_in  in;
    tarpc_flooder_out out;
    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(flooder, -1);
    }

    if (sndrs != NULL)
    {
        in.sndrs.sndrs_val = sndrs;
        in.sndrs.sndrs_len = sndnum;
    }
    if (rcvrs != NULL)
    {
        in.rcvrs.rcvrs_val = rcvrs;
        in.rcvrs.rcvrs_len = rcvnum;
    }
    in.bulkszs = bulkszs;
    in.time2run = time2run;
    in.time2wait = time2wait;
    in.iomux = iomux;

    if (rpcs->timeout == RCF_RPC_UNSPEC_TIMEOUT)
    {
        rpcs->timeout = TE_SEC2MS(time2run + TAPI_RPC_TIMEOUT_EXTRA_SEC);
    }

    if (tx_stat != NULL)
    {
        in.tx_stat.tx_stat_val = tx_stat;
        in.tx_stat.tx_stat_len = sndnum;
    }
    if (rx_stat != NULL)
    {
        in.rx_stat.rx_stat_val = rx_stat;
        in.rx_stat.rx_stat_len = rcvnum;
    }
    rcf_rpc_call(rpcs, "flooder", &in, &out);

    if (RPC_IS_CALL_OK(rpcs))
    {
        if (tx_stat != NULL)
        {
            memcpy(tx_stat, out.tx_stat.tx_stat_val,
                   out.tx_stat.tx_stat_len * sizeof(tx_stat[0]));
        }
        if (rx_stat != NULL)
        {
            memcpy(rx_stat, out.rx_stat.rx_stat_val,
                   out.rx_stat.rx_stat_len * sizeof(rx_stat[0]));
        }
    }
    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(flooder, out.retval);

    TAPI_RPC_LOG(rpcs, flooder, "[%s], [%s], %d, %d, %d, %s, %p, %p",
                 "%d tx=[%s] rx=[%s]",
                 tarpc_array2string(sndnum, sndrs, sizeof(sndrs[0])),
                 tarpc_array2string(rcvnum, rcvrs, sizeof(rcvrs[0])),
                 bulkszs, time2run, time2wait,
                 iomux2str(iomux), tx_stat, rx_stat, out.retval,
                 tarpc_array2string(sndnum, tx_stat, sizeof(tx_stat[0])),
                 tarpc_array2string(rcvnum, rx_stat, sizeof(rx_stat[0])));
    RETVAL_INT(flooder, out.retval);
}

/* See description in tapi_rpcsock.h */
int
rpc_iomux_echoer(rcf_rpc_server *rpcs,
                 int *sockets, int socknum, int time2run, int iomux,
                 uint64_t *tx_stat, uint64_t *rx_stat)
{
    tarpc_echoer_in  in;
    tarpc_echoer_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(echoer, -1);
    }
    if (sockets == NULL)
    {
        rpcs->_errno = RPC_EINVAL;
        RETVAL_INT(echoer, -1);
    }


    in.sockets.sockets_val = sockets;
    in.sockets.sockets_len = socknum;
    in.time2run = time2run;
    in.iomux = iomux;

    if (rpcs->timeout == RCF_RPC_UNSPEC_TIMEOUT)
    {
        rpcs->timeout = TE_SEC2MS(time2run + TAPI_RPC_TIMEOUT_EXTRA_SEC);
    }

    if (tx_stat != NULL)
    {
        in.tx_stat.tx_stat_val = tx_stat;
        in.tx_stat.tx_stat_len = socknum;
    }
    if (rx_stat != NULL)
    {
        in.rx_stat.rx_stat_val = rx_stat;
        in.rx_stat.rx_stat_len = socknum;
    }

    rcf_rpc_call(rpcs, "echoer", &in, &out);

    if (RPC_IS_CALL_OK(rpcs))
    {
        if (tx_stat != NULL)
        {
            memcpy(tx_stat, out.tx_stat.tx_stat_val,
                   out.tx_stat.tx_stat_len * sizeof(tx_stat[0]));
        }
        if (rx_stat != NULL)
        {
            memcpy(rx_stat, out.rx_stat.rx_stat_val,
                   out.rx_stat.rx_stat_len * sizeof(rx_stat[0]));
        }
    }

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(echoer, out.retval);
    TAPI_RPC_LOG(rpcs, echoer, "[%s], %d, %s", "%d tx=[%s] rx=[%s]",
                 tarpc_array2string(socknum, sockets, sizeof(sockets[0])),
                 time2run, iomux2str(iomux), out.retval,
                 tarpc_array2string(socknum, tx_stat, sizeof(tx_stat[0])),
                 tarpc_array2string(socknum, rx_stat, sizeof(rx_stat[0])));
    RETVAL_INT(echoer, out.retval);
}

/* See description in tapi_rpc_misc.h */
int
rpc_iomux_splice(rcf_rpc_server *rpcs, int iomux, int fd_in, int fd_out,
                 size_t len, int flags, int time2run)
{
    tarpc_iomux_splice_in  in;
    tarpc_iomux_splice_out out;
    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(iomux_splice, -1);
    }

    in.fd_in = fd_in;
    in.fd_out = fd_out;
    in.len = len;
    in.flags = flags;
    in.time2run = time2run;
    in.iomux = iomux;

    if (rpcs->timeout == RCF_RPC_UNSPEC_TIMEOUT)
    {
        rpcs->timeout = TE_SEC2MS(time2run + TAPI_RPC_TIMEOUT_EXTRA_SEC);
    }

    rcf_rpc_call(rpcs, "iomux_splice", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(iomux_splice, out.retval);

    TAPI_RPC_LOG(rpcs, iomux_splice, "%s, %d, %d, %d, %s, %d",
                 "%d",
                 iomux2str(iomux), fd_in, fd_out, len,
                 splice_flags_rpc2str(flags), time2run, out.retval);
    RETVAL_INT(iomux_splice, out.retval);
}

ssize_t
rpc_sendfile(rcf_rpc_server *rpcs, int out_fd, int in_fd,
             tarpc_off_t *offset, size_t count, tarpc_bool force64)
{
    tarpc_off_t        start = (offset != NULL) ? *offset : 0;
    tarpc_sendfile_in  in;
    tarpc_sendfile_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(sendfile, -1);
    }
    in.out_fd = out_fd;
    in.in_fd = in_fd;
    in.count = count;
    in.force64 = force64;
    if (offset != NULL && rpcs->op != RCF_RPC_WAIT)
    {
        in.offset.offset_len = 1;
        in.offset.offset_val = offset;
    }

    rcf_rpc_call(rpcs, "sendfile", &in, &out);


    if (RPC_IS_CALL_OK(rpcs))
    {
        if (offset != NULL && out.offset.offset_val != NULL)
            *offset = out.offset.offset_val[0];
    }

    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(sendfile, out.retval);
    TAPI_RPC_LOG(rpcs, sendfile, "%d, %d, %p(%lld), %u", "%d offset=%lld",
                 out_fd, in_fd, offset, start, (unsigned)count,
                 out.retval, (offset != NULL) ? (long long)*offset : 0LL);
    RETVAL_INT(sendfile, out.retval);
}

ssize_t
rpc_sendfile_via_splice(rcf_rpc_server *rpcs, int out_fd, int in_fd,
                        tarpc_off_t *offset, size_t count)
{
    tarpc_off_t                   start = (offset != NULL) ? *offset : 0;
    tarpc_sendfile_via_splice_in  in;
    tarpc_sendfile_via_splice_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(sendfile_via_splice, -1);
    }
    in.out_fd = out_fd;
    in.in_fd = in_fd;
    in.count = count;
    if (offset != NULL && rpcs->op != RCF_RPC_WAIT)
    {
        in.offset.offset_len = 1;
        in.offset.offset_val = offset;
    }

    rcf_rpc_call(rpcs, "sendfile_via_splice", &in, &out);


    if (RPC_IS_CALL_OK(rpcs))
    {
        if (offset != NULL && out.offset.offset_val != NULL)
            *offset = out.offset.offset_val[0];
    }

    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(sendfile_via_splice, out.retval);
    TAPI_RPC_LOG(rpcs, sendfile_via_splice,
                 "%d, %d, %p(%lld), %u", "%d offset=%lld",
                 out_fd, in_fd, offset, start, (unsigned)count,
                 out.retval, (offset != NULL) ? (long long)*offset : 0LL);
    RETVAL_INT(sendfile_via_splice, out.retval);
}

ssize_t
rpc_splice(rcf_rpc_server *rpcs, int fd_in, tarpc_off_t *off_in,
           int fd_out, tarpc_off_t *off_out, size_t len, int flags)
{
    tarpc_off_t      start_in = (off_in != NULL) ? *off_in : 0;
    tarpc_off_t      start_out = (off_out != NULL) ? *off_out : 0;
    tarpc_splice_in  in;
    tarpc_splice_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(splice, -1);
    }
    in.fd_in = fd_in;
    in.fd_out = fd_out;
    in.len = len;
    in.flags = flags;
    if (off_in != NULL && rpcs->op != RCF_RPC_WAIT)
    {
        in.off_in.off_in_len = 1;
        in.off_in.off_in_val = off_in;
    }
    if (off_out != NULL && rpcs->op != RCF_RPC_WAIT)
    {
        in.off_out.off_out_len = 1;
        in.off_out.off_out_val = off_out;
    }

    rcf_rpc_call(rpcs, "splice", &in, &out);

    if (RPC_IS_CALL_OK(rpcs))
    {
        if (off_in != NULL && out.off_in.off_in_val != NULL)
            *off_in = out.off_in.off_in_val[0];
        if (off_out != NULL && out.off_out.off_out_val != NULL)
            *off_out = out.off_out.off_out_val[0];
    }

    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(splice, out.retval);
    TAPI_RPC_LOG(rpcs, splice, "%d, %p(%lld), %d, %p(%lld), %u, %s",
                 "%d off_in=%lld off_in=%lld",
                 fd_in, off_in, start_in, fd_out, off_out, start_out,
                 (unsigned)len, splice_flags_rpc2str(flags),
                 out.retval, (off_in != NULL) ? (long long)*off_in : 0LL,
                 (off_out != NULL) ? (long long)*off_out : 0LL);
    RETVAL_INT(splice, out.retval);
}

/* See description in tapi_rpcsock.h */
ssize_t
rpc_socket_to_file(rcf_rpc_server *rpcs, int sock,
                   const char *path, long timeout)
{
    tarpc_socket_to_file_in  in;
    tarpc_socket_to_file_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(socket_to_file, -1);
    }

    in.sock = sock;
    in.timeout = timeout;
    if (rpcs->timeout == RCF_RPC_UNSPEC_TIMEOUT)
    {
        rpcs->timeout = TE_SEC2MS(timeout + TAPI_RPC_TIMEOUT_EXTRA_SEC);
    }
    if (path != NULL && rpcs->op != RCF_RPC_WAIT)
    {
        in.path.path_len = strlen(path) + 1;
        in.path.path_val = (char *)path;
    }

    rcf_rpc_call(rpcs, "socket_to_file", &in, &out);

    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(socket_to_file, out.retval);
    TAPI_RPC_LOG(rpcs, socket_to_file, "%d, %s, %ld", "%d",
                 sock, path, timeout, out.retval);
    RETVAL_INT(socket_to_file, out.retval);
}

/* See description in tapi_rpc_misc.h */
int64_t
rpc_copy_fd2fd(rcf_rpc_server *rpcs, int out_fd, int in_fd, int timeout,
               uint64_t count)
{
    struct tarpc_copy_fd2fd_in  in;
    struct tarpc_copy_fd2fd_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.out_fd = out_fd;
    in.in_fd = in_fd;
    in.timeout = timeout;
    in.count = count;

    out.retval = -1;
    rcf_rpc_call(rpcs, "copy_fd2fd", &in, &out);
    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(copy_fd2fd, out.retval);
    TAPI_RPC_LOG(rpcs, copy_fd2fd, "%d, %d, %d, %llu", "%lld",
                 in.out_fd, in.in_fd, in.timeout, in.count, out.retval);
    RETVAL_INT64(copy_fd2fd, out.retval);
}

int
rpc_ftp_open(rcf_rpc_server *rpcs,
             char *uri, te_bool rdonly, te_bool passive, int offset,
             int *sock)
{
    tarpc_ftp_open_in  in;
    tarpc_ftp_open_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(ftp_open, -1);
    }

    in.uri.uri_val = uri;
    in.uri.uri_len = strlen(uri) + 1;
    in.rdonly = rdonly;
    in.passive = passive;
    in.offset = offset;
    if (sock == NULL)
        in.sock.sock_len = 0;
    else
    {
        in.sock.sock_val = (tarpc_int *)sock;
        in.sock.sock_len = 1;
    }

    rcf_rpc_call(rpcs, "ftp_open", &in, &out);

    if (RPC_IS_CALL_OK(rpcs) && sock != NULL)
        *sock = out.sock;

    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(ftp_open, out.fd);
    TAPI_RPC_LOG(rpcs, ftp_open, "%s, %s, %s, %d, %p", "%d",
                 uri, rdonly ? "get" : "put",
                 passive ? "passive": "active", offset, sock, out.fd);
    RETVAL_INT(ftp_open, out.fd);
}

int
rpc_ftp_close(rcf_rpc_server *rpcs, int sock)
{
    tarpc_ftp_close_in  in;
    tarpc_ftp_close_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(ftp_close, -1);
    }

    in.sock = sock;

    rcf_rpc_call(rpcs, "ftp_close", &in, &out);

    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(ftp_close, out.ret);
    TAPI_RPC_LOG(rpcs, ftp_close, "%d", "%d", sock, out.ret);
    RETVAL_INT(ftp_open, out.ret);
}

int
rpc_overfill_buffers_gen(rcf_rpc_server *rpcs, int sock, uint64_t *sent,
                         iomux_func iomux)
{
    tarpc_overfill_buffers_in  in;
    tarpc_overfill_buffers_out out;

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(overfill_buffers, -1);
    }

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.sock = sock;
    in.is_nonblocking = FALSE;
    in.iomux = iomux;

    if (rpcs->timeout == RCF_RPC_UNSPEC_TIMEOUT)
        rpcs->timeout = RCF_RPC_DEFAULT_TIMEOUT * 4;

    rcf_rpc_call(rpcs, "overfill_buffers", &in, &out);

    if ((out.retval == 0) && (sent != NULL))
        *sent = out.bytes;

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(overfill_buffers, out.retval);
    TAPI_RPC_LOG(rpcs, overfill_buffers, "%d, %s", "%d sent=%lld",
                 sock, iomux2str(iomux),
                 out.retval, (long long int)(out.bytes));
    RETVAL_INT(overfill_buffers, out.retval);
}

int
rpc_overfill_fd(rcf_rpc_server *rpcs, int write_end, uint64_t *sent)
{
    tarpc_overfill_fd_in  in;
    tarpc_overfill_fd_out out;

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(overfill_fd, -1);
    }

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.write_end = write_end;

    rcf_rpc_call(rpcs, "overfill_fd", &in, &out);

    if ((out.retval == 0) && (sent != NULL))
        *sent = out.bytes;

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(overfill_fd, out.retval);
    TAPI_RPC_LOG(rpcs, overfill_fd, "%d", "%d sent=%lld",
                 write_end, out.retval, (long long int)(out.bytes));
    RETVAL_INT(overfill_fd, out.retval);
}

/**
 * Copy the data from src_buf to the dst_buf buffer located at TA.
 */
void
rpc_set_buf_gen(rcf_rpc_server *rpcs, const uint8_t *src_buf,
            size_t len, rpc_ptr dst_buf, size_t dst_off)
{
    char               *src_buf_cpy;
    tarpc_set_buf_in    in;
    tarpc_set_buf_out   out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_VOID(set_buf);
    }

    in.dst_buf = dst_buf;
    in.dst_off = dst_off;

    if (src_buf == NULL)
    {
        src_buf_cpy = NULL;
        in.src_buf.src_buf_val = NULL;
        in.src_buf.src_buf_len = 0;
    }
    else
    {
        /*
         * Duplicate input buffer, because it could be constant and may need
         * to be modified.
         */
        src_buf_cpy = malloc(len);
        if (src_buf_cpy == NULL)
        {
            ERROR("%s(): Failed to allocate %u bytes of memory",
                  __FUNCTION__, len);
            RETVAL_VOID(set_buf);
        }
        memcpy(src_buf_cpy, src_buf, len);

        in.src_buf.src_buf_val = src_buf_cpy;
        in.src_buf.src_buf_len = len;
    }

    rcf_rpc_call(rpcs, "set_buf", &in, &out);

    free(src_buf_cpy);

    TAPI_RPC_LOG(rpcs, set_buf, "%p, %u, %u (off %u)", "",
                 src_buf, len, dst_buf, dst_off);
    RETVAL_VOID(set_buf);
}

/**
 * Copy the data from the src_buf buffer located at TA to dst_buf buffer.
 */
void
rpc_get_buf_gen(rcf_rpc_server *rpcs, rpc_ptr src_buf, size_t src_off,
            size_t len, uint8_t *dst_buf)
{
    tarpc_get_buf_in  in;
    tarpc_get_buf_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_VOID(get_buf);
    }

    in.src_buf = src_buf;
    in.src_off = src_off;
    in.len = len;

    rcf_rpc_call(rpcs, "get_buf", &in, &out);

    TAPI_RPC_LOG(rpcs, get_buf, "%p, %u, %u (off %u)", "",
                 src_buf, len, src_buf, src_off);

    if ((out.dst_buf.dst_buf_len != 0) && (out.dst_buf.dst_buf_val != NULL))
        memcpy(dst_buf, out.dst_buf.dst_buf_val, out.dst_buf.dst_buf_len);

    RETVAL_VOID(get_buf);
}

/**
 * Fill buffer by the pattern
 */
void
rpc_set_buf_pattern_gen(rcf_rpc_server *rpcs, int pattern,
                        size_t len, rpc_ptr dst_buf, size_t dst_off)
{
    tarpc_set_buf_pattern_in  in;
    tarpc_set_buf_pattern_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_VOID(set_buf_pattern);
    }

    in.dst_buf = dst_buf;
    in.dst_off = dst_off;
    in.pattern = pattern;
    in.len = len;

    rcf_rpc_call(rpcs, "set_buf_pattern", &in, &out);

    TAPI_RPC_LOG(rpcs, set_buf_pattern, "0x%x, %u, %u (off %u)", "",
                 pattern, len, dst_buf, dst_off);
    RETVAL_VOID(set_buf_pattern);
}

int
rpc_memcmp(rcf_rpc_server *rpcs, rpc_ptr_off *s1, rpc_ptr_off *s2, size_t n)
{
    tarpc_memcmp_in  in;
    tarpc_memcmp_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(memcmp, -2);
    }

    in.s1_base = s1->base;
    in.s1_off = s1->offset;
    in.s2_base = s2->base;
    in.s2_off = s2->offset;
    in.n = n;

    rcf_rpc_call(rpcs, "memcmp", &in, &out);

    TAPI_RPC_LOG(rpcs, memcmp, "%u (off %u), %u (off %u), %u", "%d",
                 s1->base, s1->offset, s2->base, s2->offset,
                 n, out.retval);
    TAPI_RPC_OUT(memcmp, FALSE);
    return (int)out.retval;
}

void
rpc_vm_trasher(rcf_rpc_server *rpcs, te_bool start)
{
    tarpc_vm_trasher_in  in;
    tarpc_vm_trasher_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_VOID(vm_trasher);
    }

    in.start = start;

    rcf_rpc_call(rpcs, "vm_trasher", &in, &out);

    TAPI_RPC_LOG(rpcs, vm_trasher, "%s", "", start ? "start" : "stop");
    RETVAL_VOID(vm_trasher);
}

void
rpc_create_child_process_socket(const char *method,
                                rcf_rpc_server *pco_father, int father_s,
                                rpc_socket_domain domain,
                                rpc_socket_type sock_type,
                                rcf_rpc_server **pco_child, int *child_s)
{
    unsigned char info[512];
    int         info_len = sizeof(info);
    char        process_name[12];
    static int  counter = 1;

    sprintf(process_name, "pco_child%d", counter++);
    
    if (strcmp(method, "inherit") == 0)
    {
        rcf_rpc_server_fork(pco_father, process_name, pco_child);
        *child_s = father_s;
        RING("Inherit socket %d from process %d to process %d",
             father_s, rpc_getpid(pco_father), rpc_getpid(*pco_child));
        return;
    }

    if (strcmp(method, "forkandexec") == 0)
    {
        rcf_rpc_server_fork_exec(pco_father, process_name, pco_child);
        *child_s = father_s;
        RING("Inherit socket %d from process %d to process %d",
             father_s, rpc_getpid(pco_father), rpc_getpid(*pco_child));
        return;
    }
    
    if (strcmp(method, "inherit_no_net_init") == 0)    
    {
        rcf_rpc_server_create_process(pco_father, process_name, 0,
                                      pco_child);
        *child_s = father_s;
        RING("Inherit socket %d from process %d to process %d",
             father_s, rpc_getpid(pco_father), rpc_getpid(*pco_child));
        return;        
    }
    
    if (strcmp(method, "DuplicateSocket") == 0)
    {
        pid_t pid;
        
        rcf_rpc_server_create(pco_father->ta, process_name, pco_child);
        pid = rpc_getpid(*pco_child);
        rpc_wsa_duplicate_socket(pco_father, father_s, pid,
                                 info, &info_len);
        *child_s = rpc_wsa_socket(*pco_child, domain, sock_type,
                                  RPC_PROTO_DEF, info, info_len, 
                                  RPC_WSA_FLAG_OVERLAPPED);
        RING("Duplicate socket: %d (process %d) -> %d (process %d)",
             father_s, rpc_getpid(pco_father), *child_s, pid);
    }
    else if (strcmp(method, "DuplicateHandle") == 0)
    {
        pid_t pid1, pid2;
        
        rcf_rpc_server_create(pco_father->ta, process_name, pco_child);
        pid1 = rpc_getpid(pco_father);
        pid2 = rpc_getpid(*pco_child);
        rpc_duplicate_handle(pco_father, pid1, father_s, pid2, child_s);
        RING("Duplicate handle: %d (process %d) -> %d (process %d)",
             father_s, pid1, *child_s, pid2);
    }
    else if (strcmp(method, "DuplicateSocket_self") == 0)
    {
        rpc_wsa_duplicate_socket(pco_father, father_s, 
                                 rpc_getpid(pco_father), info, &info_len);
        *pco_child = pco_father;
        *child_s = rpc_wsa_socket(pco_father, domain, sock_type,
                                  RPC_PROTO_DEF, info, info_len, 
                                  RPC_WSA_FLAG_OVERLAPPED);
    }
    else if (strcmp(method, "DuplicateHandle_self") == 0)
    {
        pid_t pid = rpc_getpid(pco_father);
        
        rpc_duplicate_handle(pco_father, pid, father_s, pid, child_s);
        *pco_child = pco_father;
    }
    else
    {
        ERROR("Incorrect method %s is passed to %s", method,
              __FUNCTION__);
        TAPI_JMP_DO(TE_EFAIL);
    }
}

/**
 * Get readability (there are data to read) or writability (it is allowed
 * to write) of a particular socket.
 *
 * @param answer     answer location
 * @param rpcs       RPC server handle
 * @param s          socket to be checked
 * @param timeout    timeout in milliseconds
 * @param type       type of checking: "READ" or "WRITE"
 *
 * @return status code
 */
int
rpc_get_rw_ability(te_bool *answer, rcf_rpc_server *rpcs,
                   int s, int timeout, char *type)
{
    struct tarpc_get_rw_ability_in  in;
    struct tarpc_get_rw_ability_out out;
    int                             rc;

    in.sock = s;
    in.timeout = timeout;
    in.check_rd = (type[0] == 'R') ? TRUE : FALSE;

    if ((timeout > 0) && (rpcs->timeout == RCF_RPC_UNSPEC_TIMEOUT))
    {
        rpcs->timeout = TE_SEC2MS(TAPI_RPC_TIMEOUT_EXTRA_SEC) + timeout;
    }
    rcf_rpc_call(rpcs, "get_rw_ability", &in, &out);

    *answer = (out.retval == 1);
    rc = (out.retval > 0) ? 0 : out.retval;

    CHECK_RETVAL_VAR(rpc_get_rw_ability, rc, (rc < 0), -1);
    TAPI_RPC_LOG(rpcs, rpc_get_rw_ability, "%d %d %s", "%d", s, timeout,
                 type, out.retval);
    RETVAL_INT(rpc_find_func, rc);
}


/* See the description in tapi_rpc_misc.h */
te_errno
tapi_sigaction_simple(rcf_rpc_server *rpcs,
                      rpc_signum signum, const char *handler,
                      struct rpc_struct_sigaction *oldact)
{
    rpc_struct_sigaction    act;

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handler", __FUNCTION__);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }
    if (handler == NULL || strlen(handler) >= RCF_RPC_MAX_FUNC_NAME)
    {
        ERROR("%s(): Invalid 'handler'", __FUNCTION__);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    memset(&act, 0, sizeof(act));
    act.mm_flags = RPC_SA_SIGINFO | RPC_SA_RESTART;
    act.mm_mask = rpc_sigset_new(rpcs);
    rpc_sigemptyset(rpcs, act.mm_mask);
    rpc_sigaddset(rpcs, act.mm_mask, signum);
    strcpy(act.mm_handler, handler);

    if (oldact != NULL && oldact->mm_mask == RPC_NULL)
        oldact->mm_mask = rpc_sigset_new(rpcs);

    rpc_sigaction(rpcs, signum, &act, oldact);

    rpc_sigset_delete(rpcs, act.mm_mask);

    return 0;
}

/*
 * Join or leave multicast group.
 *
 * @param  rpcs        RPC server handle
 * @param  s           socket descriptor
 * @param  mcast_addr  multicast address (IPv4 or IPv6).
 * @param  if_index    interface index
 * @param  leave_group if @c TRUE, leave a multicasting group,
 *                     join otherwise
 * @param  how         joining method:
 *
 *    @value TARPC_MCAST_ADD_DROP     sockopt IP_ADD/DROP_MEMBERSHIP
 *    @value TARPC_MCAST_JOIN_LEAVE   sockopt MCAST_JOIN/LEAVE_GROUP
 *    @value TARPC_MCAST_WSA          WSAJoinLeaf(), no leave
 *
 * @return 0 on success, -1 on failure
 */
static int
rpc_mcast_join_leave(rcf_rpc_server *rpcs, int s,
                     const struct sockaddr *mcast_addr,
                     int if_index, te_bool leave_group,
                     tarpc_joining_method how)
{
    struct tarpc_mcast_join_leave_in    in;
    struct tarpc_mcast_join_leave_out   out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }
    if (mcast_addr == NULL)
    {
        ERROR("%s(): Invalid 'mcast_addr'", __FUNCTION__);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    in.fd = s;
    in.ifindex = if_index;
    in.leave_group = leave_group;
    in.family = addr_family_h2rpc(mcast_addr->sa_family);

    if ((in.multiaddr.multiaddr_len =
         te_netaddr_get_size(mcast_addr->sa_family)) == 0)
    {
        ERROR("%s(): 'te_netaddr_get_size(%s)' has returned error",
              __FUNCTION__, addr_family_rpc2str(mcast_addr->sa_family));
        RETVAL_INT(mcast_join_leave, -1);
    }
    if ((in.multiaddr.multiaddr_val =
         te_sockaddr_get_netaddr(mcast_addr)) == NULL)
    {
        ERROR("%s(): 'te_sockaddr_get_netaddr(%s)' has returned error",
              __FUNCTION__, te_sockaddr2str(mcast_addr));
        RETVAL_INT(mcast_join_leave, -1);
    }

    in.how = how;    

    rcf_rpc_call(rpcs, "mcast_join_leave", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(mcast_join_leave, out.retval);
    TAPI_RPC_LOG(rpcs, mcast_join_leave, "%d, %s, %d, %s, %s", "%d",
                 s, te_sockaddr2str(mcast_addr), if_index,
                 leave_group? "LEAVE" : "JOIN",
                 how == TARPC_MCAST_ADD_DROP ? "IP_(ADD|DROP)_MEMBERSHIP" :
                     how == TARPC_MCAST_JOIN_LEAVE ?
                     "MCAST_(JOIN|LEAVE)_GROUP" : "WSAJoinLeaf",
                 out.retval);
    RETVAL_INT(mcast_join_leave, out.retval);
}

int 
rpc_mcast_join(rcf_rpc_server *rpcs, int s,
               const struct sockaddr *mcast_addr, int if_index,
               tarpc_joining_method how)
{
    return rpc_mcast_join_leave(rpcs, s, mcast_addr, if_index, FALSE, how);
}

int 
rpc_mcast_leave(rcf_rpc_server *rpcs, int s,
                const struct sockaddr *mcast_addr, int if_index,
                tarpc_joining_method how)
{
    return rpc_mcast_join_leave(rpcs, s, mcast_addr, if_index, TRUE, how);
}

/* 
 * Join or leave multicast group with source.
 * For description see tapi_rpc_misc.h
 */
int
rpc_mcast_source_join_leave(rcf_rpc_server *rpcs, int s,
                            const struct sockaddr *mcast_addr,
                            const struct sockaddr *source_addr,
                            int if_index, te_bool leave_group,
                            tarpc_joining_method how)
{
    struct tarpc_mcast_source_join_leave_in    in;
    struct tarpc_mcast_source_join_leave_out   out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }
    if (mcast_addr == NULL)
    {
        ERROR("%s(): Invalid 'mcast_addr'", __FUNCTION__);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }
    if (source_addr == NULL)
    {
        ERROR("%s(): Invalid 'mcast_addr'", __FUNCTION__);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    in.fd = s;
    in.ifindex = if_index;
    in.leave_group = leave_group;
    in.family = addr_family_h2rpc(mcast_addr->sa_family);

    if ((in.multiaddr.multiaddr_len =
         te_netaddr_get_size(mcast_addr->sa_family)) == 0)
    {
        ERROR("%s(): 'te_netaddr_get_size(%s)' has returned error",
              __FUNCTION__, addr_family_rpc2str(mcast_addr->sa_family));
        RETVAL_INT(mcast_join_leave, -1);
    }
    if ((in.multiaddr.multiaddr_val =
         te_sockaddr_get_netaddr(mcast_addr)) == NULL)
    {
        ERROR("%s(): 'te_sockaddr_get_netaddr(%s)' has returned error",
              __FUNCTION__, te_sockaddr2str(mcast_addr));
        RETVAL_INT(mcast_join_leave, -1);
    }

    if ((in.sourceaddr.sourceaddr_len =
         te_netaddr_get_size(source_addr->sa_family)) == 0)
    {
        ERROR("%s(): 'te_netaddr_get_size(%s)' has returned error",
              __FUNCTION__, addr_family_rpc2str(source_addr->sa_family));
        RETVAL_INT(mcast_source_join_leave, -1);
    }
    if ((in.sourceaddr.sourceaddr_val =
         te_sockaddr_get_netaddr(source_addr)) == NULL)
    {
        ERROR("%s(): 'te_sockaddr_get_netaddr(%s)' has returned error",
              __FUNCTION__, te_sockaddr2str(source_addr));
        RETVAL_INT(mcast_source_join_leave, -1);
    }

    in.how = how;

    rcf_rpc_call(rpcs, "mcast_source_join_leave", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(mcast_source_join_leave,
                                          out.retval);
    TAPI_RPC_LOG(rpcs, mcast_source_join_leave, "%d, %s, %s, %d, %s, %s",
                 "%d", s, te_sockaddr2str(mcast_addr),
                 te_sockaddr2str(source_addr), if_index,
                 leave_group? "LEAVE" : "JOIN",
                 how == TARPC_MCAST_SOURCE_ADD_DROP ?
                    "IP_(ADD|DROP)_SOURCE_MEMBERSHIP" :
                     "MCAST_(JOIN|LEAVE)_SOURCE_GROUP",
                 out.retval);
    RETVAL_INT(mcast_source_join_leave, out.retval);
}

int
rpc_mcast_source_join(rcf_rpc_server *rpcs, int s,
                      const struct sockaddr *mcast_addr,
                      const struct sockaddr *source_addr,
                      int if_index, tarpc_joining_method how)
{
    return rpc_mcast_source_join_leave(rpcs, s, mcast_addr, source_addr,
                                       if_index, FALSE, how);
}
int
rpc_mcast_source_leave(rcf_rpc_server *rpcs, int s,
                       const struct sockaddr *mcast_addr,
                       const struct sockaddr *source_addr,
                       int if_index, tarpc_joining_method how)
{
    return rpc_mcast_source_join_leave(rpcs, s, mcast_addr, source_addr,
                                       if_index, TRUE, how);
}

int
rpc_common_mcast_join(rcf_rpc_server *rpcs, int s,
                      const struct sockaddr *mcast_addr,
                      const struct sockaddr *source_addr,
                      int if_index, tarpc_joining_method how)
{
    if (how == TARPC_MCAST_ADD_DROP || how == TARPC_MCAST_JOIN_LEAVE)
        return rpc_mcast_join_leave(rpcs, s, mcast_addr, if_index,
                                    FALSE, how);
    else
        return rpc_mcast_source_join_leave(rpcs, s, mcast_addr, source_addr,
                                           if_index, FALSE, how);
}

int
rpc_common_mcast_leave(rcf_rpc_server *rpcs, int s,
                       const struct sockaddr *mcast_addr,
                       const struct sockaddr *source_addr,
                       int if_index, tarpc_joining_method how)
{
    if (how == TARPC_MCAST_ADD_DROP || how == TARPC_MCAST_JOIN_LEAVE)
        return rpc_mcast_join_leave(rpcs, s, mcast_addr, if_index,
                                    TRUE, how);
    else
        return rpc_mcast_source_join_leave(rpcs, s, mcast_addr, source_addr,
                                           if_index, TRUE, how);
}

#if HAVE_LINUX_ETHTOOL_H
int
rpc_ioctl_ethtool(rcf_rpc_server *rpcs, int fd, 
                  const char *ifname, void *edata)
{
    struct ifreq    ifreq;

    memset(&ifreq, 0, sizeof(ifreq));
    strncpy(ifreq.ifr_name, ifname, sizeof(ifreq.ifr_name));
    ifreq.ifr_data = (caddr_t)edata;
    return rpc_ioctl(rpcs, fd, RPC_SIOCETHTOOL, &ifreq);
}
#endif /* HAVE_LINUX_ETHTOOL_H */

int
rpc_iomux_create_state(rcf_rpc_server *rpcs, iomux_func iomux,
                       tarpc_iomux_state *iomux_st)
{
    tarpc_iomux_create_state_in    in;
    tarpc_iomux_create_state_out   out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    if (iomux_st == NULL)
    {
        ERROR("%s(): Invalid 'iomux_st' argument", __FUNCTION__);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    in.iomux = iomux;

    rcf_rpc_call(rpcs, "iomux_create_state", &in, &out);
    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(iomux_create_state, out.retval);

    *iomux_st = (tarpc_iomux_state)out.iomux_st;

    TAPI_RPC_LOG(rpcs, iomux_create_state, "%s", RPC_PTR_FMT ", %d",
                 iomux2str(iomux), RPC_PTR_VAL(out.iomux_st), out.retval);
    RETVAL_INT(iomux_create_state, out.retval);

}

int
rpc_multiple_iomux_wait(rcf_rpc_server *rpcs, int fd, iomux_func iomux,
                        tarpc_iomux_state iomux_st,int events, int count,
                        int duration, int exp_rc, int *number, int *last_rc,
                        int *zero_rc)
{
    struct tarpc_multiple_iomux_wait_in    in;
    struct tarpc_multiple_iomux_wait_out   out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    in.fd = fd;
    in.iomux = iomux;
    in.iomux_st = (tarpc_iomux_state)iomux_st;
    in.events = events;
    in.count = count;
    in.exp_rc = exp_rc;
    in.duration = duration;

    rcf_rpc_call(rpcs, "multiple_iomux_wait", &in, &out);
    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(multiple_iomux_wait, out.retval);

    if (number != NULL)
        *number = out.number;
    if (last_rc != NULL)
        *last_rc = out.last_rc;
    if (zero_rc != NULL)
        *zero_rc = out.zero_rc;

    TAPI_RPC_LOG(rpcs, multiple_iomux_wait, "%d, %s" RPC_PTR_FMT ", %s"
                 "%d, %d, %p, %p", "%d number=%d last_rc=%d, zero_rc=%d",
                 fd, iomux2str(iomux), RPC_PTR_VAL(in.iomux_st),
                 poll_event_rpc2str(events), count, exp_rc, number, last_rc,
                 out.retval, out.number, out.last_rc, out.zero_rc);
    RETVAL_INT(multiple_iomux_wait, out.retval);
}


int
rpc_iomux_close_state(rcf_rpc_server *rpcs, iomux_func iomux,
                      tarpc_iomux_state iomux_st)
{
    tarpc_iomux_close_state_in    in;
    tarpc_iomux_close_state_out   out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    in.iomux = iomux;
    in.iomux_st = (tarpc_iomux_state)iomux_st;

    rcf_rpc_call(rpcs, "iomux_close_state", &in, &out);
    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(iomux_close_state, out.retval);

    TAPI_RPC_LOG(rpcs, iomux_close_state, "%s" RPC_PTR_FMT, "%d",
                 iomux2str(iomux), RPC_PTR_VAL(in.iomux_st), out.retval);
    RETVAL_INT(iomux_close_state, out.retval);
}

int
rpc_multiple_iomux(rcf_rpc_server *rpcs, int fd, iomux_func iomux,
                   int events, int count, int duration, int exp_rc,
                   int *number, int *last_rc, int *zero_rc)
{
    struct tarpc_multiple_iomux_in    in;
    struct tarpc_multiple_iomux_out   out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    in.fd = fd;
    in.iomux = iomux;
    in.events = events;
    in.count = count;
    in.exp_rc = exp_rc;
    in.duration = duration;

    rcf_rpc_call(rpcs, "multiple_iomux", &in, &out);
    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(multiple_iomux, out.retval);

    if (number != NULL)
        *number = out.number;
    if (last_rc != NULL)
        *last_rc = out.last_rc;
    if (zero_rc != NULL)
        *zero_rc = out.zero_rc;

    TAPI_RPC_LOG(rpcs, multiple_iomux, "%d, %s, %s, %d, %d, %p, %p",
                 "%d number=%d last_rc=%d, zero_rc=%d",
                 fd, iomux2str(iomux), poll_event_rpc2str(events),
                 count, exp_rc, number, last_rc, out.retval, out.number,
                 out.last_rc, out.zero_rc);
    RETVAL_INT(multiple_iomux, out.retval);
}

int
rpc_raw2integer(rcf_rpc_server *rpcs, uint8_t *data,
                size_t len)
{
    tarpc_raw2integer_in    in;
    tarpc_raw2integer_out   out;

    char    *str;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }
    if (data == NULL || len == 0)
    {
        ERROR("%s(): Invalid 'data'", __FUNCTION__);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    in.data.data_val = data;
    in.data.data_len = len;

    rcf_rpc_call(rpcs, "raw2integer", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(raw2integer, out.retval);

    str = raw2string(data, len);

    if (out.retval == 0)
    {
        memset(data, 0, len);
        if (len == 1)
            *(uint8_t *)data = out.number;
        else if (len == 2)
            *(uint16_t *)data = out.number;
        else if (len == 4)
            *(uint32_t *)data = out.number;
        else if (len == 8)
            *(uint64_t *)data = out.number;
        else
        {
            WARN("%s(): incorrect length of integer data",
                 __FUNCTION__);
            memcpy(data, &out.number, len);
        }
    }

    TAPI_RPC_LOG(rpcs, raw2integer, "%p (%s), %d", "%d number=%lld",
                 data, str == NULL ? "" : str, len, out.retval,
                 (long long int)out.number);
    free(str);
    RETVAL_INT(raw2integer, out.retval);
}

int
rpc_integer2raw(rcf_rpc_server *rpcs, uint64_t number,
                uint8_t *data, size_t len)
{
    tarpc_integer2raw_in    in;
    tarpc_integer2raw_out   out;

    char    *str;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }
    if (data == NULL || len == 0)
    {
        ERROR("%s(): Invalid 'data'", __FUNCTION__);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    in.number = number;
    in.len = len;

    rcf_rpc_call(rpcs, "integer2raw", &in, &out);

    if (out.retval == 0)
        memcpy(data, out.data.data_val, out.data.data_len);

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(integer2raw, out.retval);
    str = raw2string(data, len);
    TAPI_RPC_LOG(rpcs, integer2raw, "%lld, %p, %d", "%d raw=%s",
                 (long long int)number, data, len, out.retval,
                 str == NULL ? "" : str);
    free(str);
    RETVAL_INT(integer2raw, out.retval);
}

int
rpc_vfork_pipe_exec(rcf_rpc_server *rpcs, te_bool use_exec)
{
    tarpc_vfork_pipe_exec_in    in;
    tarpc_vfork_pipe_exec_out   out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    in.use_exec = use_exec;

    rcf_rpc_call(rpcs, "vfork_pipe_exec", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(vfork_pipe_exec, out.retval);
    TAPI_RPC_LOG(rpcs, vfork_pipe_exec, "%d", "%d",
                 use_exec, out.retval);
    RETVAL_INT(vfork_pipe_exec, out.retval);
}

/* See description in tapi_rpc_misc.h */
void
tapi_saved_mtus_free(te_saved_mtus *mtus)
{
    te_saved_mtu *saved_mtu;

    if (mtus == NULL)
        return;

    while ((saved_mtu = LIST_FIRST(mtus)) != NULL)
    {
        LIST_REMOVE(saved_mtu, links);
        free(saved_mtu);
    }
}

/* See description in tapi_rpc_misc.h */
te_errno
tapi_saved_mtus2str(te_saved_mtus *mtus, char **str)
{
    te_string     s = TE_STRING_INIT;
    te_saved_mtu *saved_mtu;
    te_errno      rc;

    /*
     * This is done to ensure that on success empty string
     * is returned if list of MTU values is empty,
     * not NULL pointer.
     */
    rc = te_string_append(&s, "%s", "");
    if (rc != 0)
    {
        te_string_free(&s);
        return TE_RC(TE_TAPI, rc);
    }

    LIST_FOREACH(saved_mtu, mtus, links)
    {
        rc = te_string_append(&s, "%s/%s=%d;", saved_mtu->ta,
                              saved_mtu->if_name, saved_mtu->mtu);
        if (rc != 0)
        {
            te_string_free(&s);
            return TE_RC(TE_TAPI, rc);
        }
    }

    *str = s.ptr;
    return 0;
}

/* See description in tapi_rpc_misc.h */
te_errno
tapi_str2saved_mtus(const char *str, te_saved_mtus *mtus)
{
    te_saved_mtu    *saved_mtu = NULL;
    char             if_name[IFNAMSIZ];
    char             ta[RCF_MAX_NAME];
    int              mtu;
    char             buf[RCF_MAX_NAME];
    unsigned int     p;
    unsigned int     q;
    te_errno         rc = 0;

    tapi_saved_mtus_free(mtus);

    for (p = 0, q = 0; str[p] != '\0'; p++)
    {
        if (str[p] != '=' && str[p] != ';' && str[p] != '/')
        {
            if (q >= sizeof(buf) - 1)
            {
                ERROR("%s(): too long substring encountered",
                      __FUNCTION__);
                rc = TE_RC(TE_TAPI, TE_EINVAL);
                goto cleanup;
            }

            buf[q] = str[p];
            q++;
        }
        else
        {
            buf[q] = '\0';
            q = 0;

            if (str[p] == '=')
            {
                strncpy(if_name, buf, IFNAMSIZ);
            }
            else if (str[p] == '/')
            {
                strncpy(ta, buf, RCF_MAX_NAME);
            }
            else
            {
                mtu = atoi(buf);

                saved_mtu = TE_ALLOC(sizeof(*saved_mtu));
                if (saved_mtu == NULL)
                {
                    ERROR("%s(): out of memory", __FUNCTION__);
                    rc = TE_RC(TE_TAPI, TE_ENOMEM);
                    goto cleanup;
                }

                saved_mtu->mtu = mtu;
                strncpy(saved_mtu->if_name, if_name, IFNAMSIZ);
                strncpy(saved_mtu->ta, ta, RCF_MAX_NAME);

                LIST_INSERT_HEAD(mtus, saved_mtu, links);
            }
        }
    }

    if (q != 0)
    {
        ERROR("%s(): MTU values string is malformed",
              __FUNCTION__);
        rc = TE_RC(TE_TAPI, TE_EINVAL);
    }

cleanup:

    if (rc != 0)
        tapi_saved_mtus_free(mtus);
    return rc;
}

/**
 * Get pointer to a saved MTU instance if it is in the list.
 *
 * @param mtus      Saved MTU's list
 * @param ta        Test agent name
 * @param if_name   Interface name
 *
 * @return Poitner to saved MTU instance or @c NULL.
 */
static te_saved_mtu *
te_saved_mtus_get(te_saved_mtus *mtus, const char *ta, const char *if_name)
{
    te_saved_mtu *saved_mtu;

    LIST_FOREACH(saved_mtu, mtus, links)
    {
        if (strcmp(saved_mtu->if_name, if_name) == 0 &&
            strcmp(saved_mtu->ta, ta) == 0)
        {
            return saved_mtu;
        }
    }

    return NULL;
}

/**
 * Save MTU value for a given interface in a list.
 *
 * @note If MTU value for a given interface is already in the list,
 *       this function will not update it.
 *
 * @param mtus      List of MTU values.
 * @param ta        Test agent name.
 * @param if_name   Interface name.
 * @param mtu       MTU value.
 *
 * @return Status code.
 */
static te_errno
te_saved_mtus_put(te_saved_mtus *mtus,
                  const char *ta,
                  const char *if_name,
                  int mtu)
{
    te_saved_mtu *saved_mtu;

    if (strlen(if_name) > IFNAMSIZ - 1)
    {
        ERROR("%s(): interface name '%s' is too long",
              __FUNCTION__, if_name);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    saved_mtu = te_saved_mtus_get(mtus, ta, if_name);
    if (saved_mtu != NULL)
        return TE_RC(TE_TAPI, TE_EEXIST);

    saved_mtu = TE_ALLOC(sizeof(*saved_mtu));
    if (saved_mtu == NULL)
        return TE_RC(TE_TAPI, TE_ENOMEM);

    strncpy(saved_mtu->ta, ta, RCF_MAX_NAME);
    strncpy(saved_mtu->if_name, if_name, IFNAMSIZ);
    saved_mtu->mtu = mtu;
    LIST_INSERT_HEAD(mtus, saved_mtu, links);

    return 0;
}

/* Predeclaration. */
static te_errno save_descendants_mtus(const char *ta,
                                      const char *if_name,
                                      te_saved_mtus *mtus,
                                      te_bool save_target);
static te_errno tapi_set_if_mtu_smart_aux(const char *ta,
                                          const char *if_name,
                                          int mtu, int *old_mtu_p,
                                          te_saved_mtus *mtus,
                                          te_bool ancestor,
                                          te_bool skip_target);

/**
 * Callback function to iterate an interface descendants.
 *
 * @param ta        Test agent name
 * @param ifname    Interface name
 * @param opaque    Saved MTU's list (@b te_saved_mtus)
 *
 * @return Status code.
 */
static te_errno
save_descendant_cb(const char *ta, const char *ifname, void *opaque)
{
    return save_descendants_mtus(ta, ifname, (te_saved_mtus *)opaque, TRUE);
}

/**
 * Save current MTU values for all interfaces based
 * on the given one (VLANs, MACVLANs).
 *
 * @param ta            Test agent name
 * @param if_name       Interface name.
 * @param mtus          Where to store MTU values.
 * @param save_target   If @c TRUE, save MTU value
 *                      for the interface itself too.
 *
 * @return Status code.
 */
static te_errno
save_descendants_mtus(const char *ta,
                      const char *if_name,
                      te_saved_mtus *mtus,
                      te_bool save_target)
{
    int       old_mtu;
    te_errno  rc = 0;

    if (!tapi_interface_is_mine(ta, if_name))
    {
        ERROR("Interface %s is not grabbed by agent %s", if_name, ta);
        return TE_RC(TE_TAPI, TE_EPERM);
    }

    if (save_target)
    {
        rc = tapi_cfg_base_if_get_mtu_u(ta, if_name, &old_mtu);
        if (rc != 0)
            return rc;

        rc = te_saved_mtus_put(mtus, ta, if_name, old_mtu);
        if (rc != 0)
        {
            /* Avoid loop referencing. */
            if (rc == TE_RC(TE_TAPI, TE_EEXIST))
                return 0;
            return rc;
        }
    }

    return tapi_host_ns_if_child_iter(ta, if_name, &save_descendant_cb,
                                      (void *)mtus);
}

/* Context to iterate parent interfaces with the callback function
 * set_parent_mtu_cb(). */
typedef struct parent_mtu_ctx {
    int            mtu;
    te_bool        aggr;
    te_saved_mtus *mtus;
} parent_mtu_ctx;

/**
 * Callback function to iterate an interface parents.
 *
 * @param ta        Test agent name
 * @param ifname    Interface name
 * @param opaque    The context (@b parent_mtu_ctx)
 *
 * @return Status code.
 */
static te_errno
set_parent_mtu_cb(const char *ta, const char *ifname, void *opaque)
{
    parent_mtu_ctx *ctx = (parent_mtu_ctx *)opaque;

    return tapi_set_if_mtu_smart_aux(ta, ifname, ctx->mtu, NULL, ctx->mtus,
                                     TRUE, ctx->aggr);
}

/**
 * Auxiliary function used to implement tapi_set_if_mtu_smart().
 * It increases MTU for the ancestors of a given interface if required,
 * and then sets it for the interface itself.
 *
 * @param ta            Test agent name
 * @param if_name       Interface name
 * @param mtu           MTU value
 * @param old_mtu_p     Where to save old MTU value for target interface
 *                      (may be @c NULL)
 * @param ancestor      If @c TRUE, this is an ancestor interface
 * @param skip_target   If @c TRUE, do not try to change MTU of
 *                      the passed interface, only change MTU of
 *                      its ancestors if required. This is useful
 *                      when the target is a slave of an aggregation
 *                      interface, and MTU for it should change
 *                      automatically when it is changed for the
 *                      aggregation as a whole.
 *
 * @return Status code.
 */
static te_errno
tapi_set_if_mtu_smart_aux(const char *ta,
                          const char *if_name,
                          int mtu, int *old_mtu_p,
                          te_saved_mtus *mtus,
                          te_bool ancestor,
                          te_bool skip_target)
{
    int           rc;
    int           old_mtu;

    if (!tapi_interface_is_mine(ta, if_name))
    {
        ERROR("Interface %s is not grabbed by agent %s", if_name, ta);
        return TE_RC(TE_TAPI, TE_EPERM);
    }

    rc = tapi_cfg_base_if_get_mtu_u(ta, if_name, &old_mtu);
    if (rc != 0)
        return rc;

    if (old_mtu_p != NULL && !ancestor)
    {
        *old_mtu_p = old_mtu;
    }

    if (mtu == old_mtu)
        return 0;

    if (mtu > old_mtu)
    {
        parent_mtu_ctx ctx = {.mtu = mtu, .aggr = FALSE, .mtus = mtus};
        te_interface_kind kind;

        rc = tapi_cfg_get_if_kind(ta, if_name, &kind);
        if (rc != 0)
            return rc;
        if (kind == TE_INTERFACE_KIND_BOND || kind == TE_INTERFACE_KIND_TEAM)
            ctx.aggr = TRUE;

        rc = tapi_host_ns_if_parent_iter(ta, if_name,
                                         &set_parent_mtu_cb, &ctx);
        if (rc != 0)
            return rc;
    }
    else if (ancestor)
    {
        /*
         * No need to adjust MTU on ancestors if it has higher
         * value there.
         */
        return 0;
    }

    if (skip_target)
        return 0;

    if (mtus != NULL)
    {
        rc = te_saved_mtus_put(mtus, ta, if_name, old_mtu);
        /* Don't save an interface the second time to avoid intermediate
         * steps rolling back a configuration. Keep only the first saved
         * value. */
        if (rc != 0 && rc != TE_RC(TE_TAPI, TE_EEXIST))
            return rc;
    }

    return tapi_cfg_base_if_set_mtu_ext(ta, if_name, mtu, NULL, ancestor);
}

/* See description in tapi_rpc_misc.h */
te_errno
tapi_set_if_mtu_smart(const char *ta, const struct if_nameindex *interface,
                      int mtu, int *old_mtu)
{
    return tapi_set_if_mtu_smart_aux(ta, interface->if_name,
                                     mtu, old_mtu, NULL, FALSE, FALSE);
}

/* See description in tapi_rpc_misc.h */
te_errno
tapi_set_if_mtu_smart2(const char *ta, const char *if_name,
                       int mtu, te_saved_mtus *backup)
{
    te_errno  rc = 0;

    rc = save_descendants_mtus(ta, if_name, backup, FALSE);
    if (rc != 0)
        return rc;

    return tapi_set_if_mtu_smart_aux(ta, if_name, mtu, NULL,
                                     backup, FALSE, FALSE);
}

/* See description in tapi_rpc_misc.h */
te_errno
tapi_set_if_mtu_smart2_rollback(te_saved_mtus *backup)
{
    te_saved_mtu  *saved_mtu = NULL;
    te_errno       rc = 0;

    LIST_FOREACH(saved_mtu, backup, links)
    {
        rc = tapi_set_if_mtu_smart_aux(saved_mtu->ta, saved_mtu->if_name,
                                       saved_mtu->mtu, NULL,
                                       NULL, FALSE, FALSE);
        if (rc != 0)
            break;
    }

    tapi_saved_mtus_free(backup);
    return rc;
}

/* See description in tapi_rpc_misc.h */
te_errno
tapi_store_saved_mtus(const char *ta,
                      const char *name,
                      te_saved_mtus *mtus)
{
    char      *mtus_str = NULL;
    te_errno   rc = 0;
    ssize_t    sys_rc = 0;
    size_t     len;
    int        fd = -1;
    te_string  path = TE_STRING_INIT;

    rc = tapi_saved_mtus2str(mtus, &mtus_str);
    if (rc != 0)
        return rc;

    rc = te_string_append(&path, "/tmp/sapi_ts_mtus_%s_%s_XXXXXX",
                          ta, name);
    if (rc != 0)
        goto cleanup;

    fd = mkstemp(path.ptr);
    if (fd < 0)
    {
        te_string_reset(&path);
        rc = te_rc_os2te(errno);
        ERROR("%s(): mkstemp() failed: %r",
              __FUNCTION__, rc);
        goto cleanup;
    }

    len = strlen(mtus_str) + 1;
    sys_rc = write(fd, mtus_str, len);
    if (sys_rc < 0)
    {
        rc = te_rc_os2te(errno);
        ERROR("%s(): write() failed: %r",
              __FUNCTION__, rc);
        goto cleanup;
    }
    else if (sys_rc != (ssize_t)len)
    {
        ERROR("%s(): write() did not write expected number of bytes",
              __FUNCTION__);
        rc = TE_RC(TE_TAPI, TE_EFAIL);
        goto cleanup;
    }

    sys_rc = close(fd);
    fd = -1;
    if (sys_rc != 0)
    {
        rc = te_rc_os2te(errno);
        ERROR("%s(): close() failed: %r",
              __FUNCTION__, rc);
    }

    rc = cfg_add_instance_fmt(NULL, CVT_STRING, path.ptr,
                              "/local:%s/saved_mtus:%s",
                              ta, name);
    if (rc != 0)
    {
        ERROR("%s(): failed to add 'saved_mtus' instance "
              "in Configurator tree",
              __FUNCTION__);
    }

cleanup:

    free(mtus_str);

    if (fd >= 0)
        close(fd);
    if (rc != 0 && path.len > 0)
        unlink(path.ptr);

    te_string_free(&path);

    if (rc == 0)
        tapi_saved_mtus_free(mtus);

    return rc;
}

/* See description in tapi_rpc_misc.h */
te_bool
tapi_stored_mtus_exist(const char *ta,
                       const char *name)
{
    te_errno      rc;
    cfg_val_type  val_type = CVT_STRING;

    rc = cfg_get_instance_fmt(&val_type, NULL,
                              "/local:%s/saved_mtus:%s",
                              ta, name);
    if (rc == 0)
        return TRUE;

    return FALSE;
}

/* See description in tapi_rpc_misc.h */
te_errno
tapi_retrieve_saved_mtus(const char *ta,
                         const char *name,
                         te_saved_mtus *mtus)
{
#define MTUS_BUF_LEN 1024
    char         *fname = NULL;
    te_errno      rc = 0;
    te_errno      rc2 = 0;
    cfg_val_type  val_type = CVT_STRING;
    FILE         *f = NULL;
    te_string     str = TE_STRING_INIT;
    char          buf[MTUS_BUF_LEN];
    ssize_t       sys_rc;

    UNUSED(name);
    UNUSED(mtus);

    rc = cfg_get_instance_fmt(&val_type, &fname,
                              "/local:%s/saved_mtus:%s",
                              ta, name);
    if (rc != 0)
    {
        ERROR("%s(): failed to get file name for '%s'",
              __FUNCTION__, name);
        return rc;
    }

    f = fopen(fname, "r");
    if (f == NULL)
    {
        ERROR("%s(): failed to open '%s'",
              __FUNCTION__, fname);
        rc = te_rc_os2te(errno);
        goto cleanup;
    }

    while (!feof(f))
    {
        sys_rc = fread(buf, 1, MTUS_BUF_LEN - 1, f);
        if (ferror(f) != 0)
        {
            ERROR("%s(): failed to read from '%s'",
                  __FUNCTION__, fname);
            rc = TE_RC(TE_TAPI, TE_EFAIL);
            goto cleanup;
        }
        buf[sys_rc] = '\0';

        rc = te_string_append(&str, "%s", buf);
        if (rc != 0)
            goto cleanup;
    }

    rc = tapi_str2saved_mtus(str.ptr, mtus);

cleanup:

    if (f != NULL)
    {
        sys_rc = fclose(f);
        if (sys_rc != 0)
        {
            ERROR("%s(): fclose() failed", __FUNCTION__);
            if (rc == 0)
                rc = te_rc_os2te(errno);
        }
    }

    rc2 = cfg_del_instance_fmt(FALSE,
                               "/local:%s/saved_mtus:%s",
                               ta, name);
    if (rc2 != 0)
    {
        ERROR("%s(): failed to delete file name from Configurator tree",
              __FUNCTION__);

        if (rc == 0)
            rc = rc2;
    }

    if (fname != NULL)
    {
        sys_rc = unlink(fname);
        if (sys_rc != 0)
        {
            ERROR("%s(): unlink() failed", __FUNCTION__);
            if (rc == 0)
                rc = te_rc_os2te(errno);
        }
    }

    free(fname);
    te_string_free(&str);

    return rc;
#undef MTUS_BUF_LEN
}

/* See description in tapi_rpc_misc.h */
te_bool
tapi_interface_is_vlan(rcf_rpc_server *rpcs,
                       const struct if_nameindex *interface)
{
    char if_par[IFNAMSIZ] = {0};

    rpc_vlan_get_parent(rpcs, interface->if_name, if_par);

    if (strlen(if_par) > 0)
        return TRUE;

    return FALSE;
}

/**
 * Callback function to count VLAN interfaces number.
 *
 * @param ta        Test agent name
 * @param if_name   Interface name
 * @param opaque    Location for the counter (@c size_t)
 *
 * @return Status code.
 */
static te_errno
vlan_count_cb(const char *ta, const char *ifname, void *opaque)
{
    te_interface_kind kind;
    te_errno          rc;
    size_t           *num = (size_t *)opaque;

    rc = tapi_cfg_get_if_kind(ta, ifname, &kind);
    if (rc != 0)
        return rc;

    if (kind == TE_INTERFACE_KIND_VLAN)
        (*num)++;

    return tapi_host_ns_if_parent_iter(ta, ifname, &vlan_count_cb,
                                       (void *)num);
}

/* See description in tapi_rpc_misc.h */
te_errno
tapi_interface_vlan_count(const char *ta, const char *if_name, size_t *num)
{
    *num = 0;
    return vlan_count_cb(ta, if_name, (void *)num);
}

/* See description in tapi_rpc_misc.h */
void
rpc_release_rpc_ptr(rcf_rpc_server *rpcs, rpc_ptr ptr, char *ns_string)
{
    tarpc_release_rpc_ptr_in  in;
    tarpc_release_rpc_ptr_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    TAPI_RPC_NAMESPACE_CHECK_JUMP(rpcs, ptr, ns_string);
    in.ptr = ptr;
    in.ns_string = ns_string;

    rcf_rpc_call(rpcs, "release_rpc_ptr", &in, &out);

    TAPI_RPC_LOG(rpcs, release_rpc_ptr, RPC_PTR_FMT, "", RPC_PTR_VAL(ptr));
    RETVAL_VOID(release_rpc_ptr);
}

/* See the description in tapi_rpc_misc.h */
int
rpc_send_flooder_iomux(rcf_rpc_server *rpcs, int sock, iomux_func iomux,
                       tarpc_send_function send_func,
                       te_bool msg_dontwait, int packet_size,
                       int duration, uint64_t *packets, uint32_t *errors)
{
    tarpc_send_flooder_iomux_in  in;
    tarpc_send_flooder_iomux_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.sock = sock;
    in.iomux = iomux;
    in.send_func = send_func;
    in.msg_dontwait = msg_dontwait;
    in.packet_size = packet_size;
    in.duration = duration;

    rcf_rpc_call(rpcs, "send_flooder_iomux", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(send_flooder_iomux,
                                          out.retval);

    TAPI_RPC_LOG(rpcs, send_flooder_iomux, "sock = %d, iomux = %s, "
                 "send_func = %d, msg_dontwait = %d, packet_size = %d, "
                 "duration = %d, packets = %llu, "
                 "errors = %lu", "%d", sock, iomux2str(iomux), send_func,
                 msg_dontwait, packet_size, duration, out.packets,
                 out.errors, out.retval);

    if (RPC_IS_CALL_OK(rpcs) && rpcs->op != RCF_RPC_WAIT)
    {
        if (packets != NULL)
            *packets = out.packets;
        if (errors != NULL)
            *errors = out.errors;
    }

    RETVAL_ZERO_INT(send_flooder_iomux, out.retval);
}

/* See the description in tapi_rpc_misc.h */
int
rpc_drain_fd(rcf_rpc_server *rpcs, int fd, size_t size, int time2wait,
             uint64_t *read)
{
    tarpc_drain_fd_in  in;
    tarpc_drain_fd_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.fd = fd;
    in.size = size;
    in.time2wait = time2wait;

    rcf_rpc_call(rpcs, "drain_fd", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(drain_fd, out.retval);

    TAPI_RPC_LOG(rpcs, drain_fd, "fd = %d, size = %"TE_PRINTF_SIZE_T"u, "
                 "time2wait = %d, read = %"TE_PRINTF_64"u", "%d", fd, size,
                 time2wait, out.read, out.retval);

    if (RPC_IS_CALL_OK(rpcs) && rpcs->op != RCF_RPC_WAIT && read != NULL)
        *read = out.read;

    RETVAL_ZERO_INT(drain_fd, out.retval);
}

/* See the description in tapi_rpc_misc.h */
int
rpc_drain_fd_simple(rcf_rpc_server *rpcs, int fd, uint64_t *read)
{
    int rc;

    RPC_AWAIT_IUT_ERROR(rpcs);
    rc = rpc_drain_fd(rpcs, fd, TAPI_READ_BUF_SIZE,
                      TAPI_WAIT_NETWORK_DELAY, read);

    if (rc == 0)
        return rc;
    if (rc != -1)
        TEST_VERDICT("RPC call drain_fd() returned unexpected "
                     "value %d", rc);
    if (RPC_ERRNO(rpcs) != RPC_EAGAIN)
        TEST_VERDICT("RPC call drain_fd failed with unexpected "
                     "errno %r instead of %r", RPC_ERRNO(rpcs),
                     RPC_EAGAIN);

    return rc;
}

/* See the description in tapi_rpc_misc.h */
int
rpc_read_fd2te_dbuf_append(rcf_rpc_server *rpcs, int fd, int time2wait,
                           size_t amount, te_dbuf *dbuf)
{
    tarpc_read_fd_in  in;
    tarpc_read_fd_out out;
    te_errno rc;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.fd = fd;
    in.size = TAPI_READ_BUF_SIZE;
    in.time2wait = time2wait;
    in.amount = amount;

    rcf_rpc_call(rpcs, "read_fd", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(read_fd, out.retval);

    TAPI_RPC_LOG(rpcs, read_fd, "fd = %d, time2wait = %d, "
                 "amount = %"TE_PRINTF_SIZE_T"u, buf = %p, "
                 "read = %"TE_PRINTF_SIZE_T"u", "%d",
                 fd, time2wait, amount, out.buf.buf_val, out.buf.buf_len,
                 out.retval);

    if (RPC_IS_CALL_OK(rpcs) && rpcs->op != RCF_RPC_WAIT)
    {
        if (out.buf.buf_val != NULL && out.buf.buf_len != 0)
        {
            rc = te_dbuf_append(dbuf, out.buf.buf_val, out.buf.buf_len);
            if (rc != 0)
            {
                ERROR("Failed to save read data");
                RETVAL_INT(read_fd, -1);
            }
        }
    }

    RETVAL_ZERO_INT(read_fd, out.retval);
}

/* See the description in tapi_rpc_misc.h */
int
rpc_read_fd2te_dbuf(rcf_rpc_server *rpcs, int fd, int time2wait,
                    size_t amount, te_dbuf *dbuf)
{
    if (dbuf == NULL)
        TEST_FAIL("Invalid argument");

    te_dbuf_reset(dbuf);
    return rpc_read_fd2te_dbuf_append(rpcs, fd, time2wait, amount, dbuf);
}

/* See the description in tapi_rpc_misc.h */
int
rpc_read_fd(rcf_rpc_server *rpcs, int fd, int time2wait,
            size_t amount, void **buf, size_t *read)
{
    te_dbuf dbuf = TE_DBUF_INIT(0);
    int rc;

    if (buf == NULL || read == NULL)
        TEST_FAIL("Invalid arguments");

    rc = rpc_read_fd2te_dbuf_append(rpcs, fd, time2wait, amount, &dbuf);
    *buf = dbuf.ptr;
    *read = dbuf.len;

    return rc;
}

/* See the description in tapi_rpc_misc.h */
int
rpc_read_fd2te_string_append(rcf_rpc_server *rpcs, int fd, int time2wait,
                             size_t amount, te_string *testr)
{
    te_dbuf dbuf = TE_DBUF_INIT(0);
    te_bool awaiting_error;
    int rc;

    dbuf.ptr = (uint8_t *)testr->ptr;
    dbuf.size = testr->size;
    dbuf.len = testr->len;

    awaiting_error = RPC_AWAITING_ERROR(rpcs);
    rc = rpc_read_fd2te_dbuf_append(rpcs, fd, time2wait, amount, &dbuf);
    if (rc == 0)
    {
        if (te_dbuf_append(&dbuf, "", 1) != 0)  /* Add null-terminator */
        {
            rc = -1;
            if (!awaiting_error)
                TAPI_JMP_DO(TE_EFAIL);
        }
    }

    testr->ptr = (char *)dbuf.ptr;
    testr->size = dbuf.size;
    if (dbuf.len > 0 && rc == 0)
        testr->len = dbuf.len - 1;  /* Ignore null terminator */
    else
        testr->len = dbuf.len;

    return rc;
}

/* See the description in tapi_rpc_misc.h */
int
rpc_read_fd2te_string(rcf_rpc_server *rpcs, int fd, int time2wait,
                      size_t amount, te_string *testr)
{
    if (testr == NULL)
        TEST_FAIL("Invalid argument");

    te_string_reset(testr);
    return rpc_read_fd2te_string_append(rpcs, fd, time2wait, amount, testr);
}
