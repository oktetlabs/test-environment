/** @file
 * @brief Test API - RPC
 *
 * TAPI for auxilury remote socket calls implementation
 *
 *
 * Copyright (C) 2005 Test Environment authors (see file AUTHORS in
 * the root directory of the distribution).
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
#include "tapi_rpc_internal.h"
#include "tapi_rpc_unistd.h"
#include "tapi_rpc_misc.h"
#include "tapi_rpc_winsock2.h"
#include "tapi_rpc_signal.h"
#include "tapi_test.h"
#include "tapi_cfg_base.h"

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

int
rpc_bond_get_slaves(rcf_rpc_server *rpcs, const char *bond_ifname,
                    char slaves[][IFNAMSIZ], int *slaves_num)
{
    int rc;
    int i;

    struct tarpc_bond_get_slaves_in  in;
    struct tarpc_bond_get_slaves_out out;
    char                   str_buf[1024];

    memset(str_buf, 0, sizeof(str_buf));
    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(bond_get_slaves, -1);
    }

    if (bond_ifname == NULL)
    {
        ERROR("%s(): NULL interface name", __FUNCTION__);
        RETVAL_INT(bond_get_slaves, -1);
    }

    if (slaves == NULL)
    {
        ERROR("%s(): Pointer slaves is NULL", __FUNCTION__);
        RETVAL_INT(bond_get_slaves, -1);
    }

    in.ifname.ifname_val = strdup(bond_ifname);
    in.ifname.ifname_len = strlen(bond_ifname) + 1;
    in.slaves_num = *slaves_num;

    rcf_rpc_call(rpcs, "bond_get_slaves", &in, &out);
    free(in.ifname.ifname_val);
    *slaves_num = out.slaves_num;
    for (i = 0; i < *slaves_num; i++)
    {
        memcpy(slaves[i], out.slaves.slaves_val[i].ifname,
               IFNAMSIZ);
        snprintf(str_buf + strlen(str_buf), sizeof(str_buf) -
                 strlen(str_buf), "%s%s", (i == 0) ? "" : ", ", slaves[i]);
    }

    rc = out.retval;

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(rpc_bond_get_slaves, rc);
    TAPI_RPC_LOG(rpcs, rpc_bond_get_slaves, "%s, %p(%s), %d", "%d",
                 bond_ifname, slaves, str_buf, *slaves_num, rc);
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
    struct tarpc_copy_fd2fd_in  in = {};
    struct tarpc_copy_fd2fd_out out = {};

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
 * For description see tapi_rpc_misc.h
 */
int 
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
tapi_set_if_mtu_smart(rcf_rpc_server *rpcs,
                      const struct if_nameindex *interface, int mtu,
                      int *old_mtu)
{
    char      if_par[IFNAMSIZ];
    te_bool   parent = FALSE;

    if (!tapi_interface_is_mine(rpcs, interface->if_name))
        TEST_FAIL("Interface %s is not owned", interface->if_name);

    memset(if_par, 0, sizeof(if_par));

    rpc_vlan_get_parent(rpcs, interface->if_name, if_par);
    if (strlen(if_par) != 0 && tapi_interface_is_mine(rpcs, if_par))
        parent = TRUE;

    if (parent)
        CHECK_RC(tapi_cfg_base_if_set_mtu_ext(rpcs->ta, if_par, mtu,
                                              old_mtu, TRUE));

    CHECK_RC(tapi_cfg_base_if_set_mtu(rpcs->ta, interface->if_name, mtu,
                                      old_mtu));
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
