/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2023 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief TAPI RPC for BPF
 *
 * Implementation of TAPI RPC for BPF
 */

#include "te_defs.h"
#include "rcf_rpc.h"
#include "tarpc.h"
#include "tapi_rpc_internal.h"
#include "te_string.h"
#include "te_rpc_bpf.h"
#include "tapi_bpf.h"
#include "tapi_file.h"

/*
 * Maximum length of string representation of UMEM or socket
 * configuration structure.
 */
#define CONFIG_STR_LEN 1024

/* Get string representation of tarpc_xsk_umem_config. */
static void
xsk_umem_config_tarpc2str(tarpc_xsk_umem_config *config, te_string *str)
{
    if (config == NULL)
    {
        te_string_append(str, "%s", "(null)");
    }
    else
    {
        te_string_append(
                 str,
                 "{.fill_size=%u, .comp_size=%u, .frame_size=%u, "
                 ".frame_headroom=%u, .flags=0x%x}",
                 config->fill_size, config->comp_size, config->frame_size,
                 config->frame_headroom, config->flags);
    }
}

/* See description in tapi_rpc_bpf.h */
int
rpc_xsk_umem__create(rcf_rpc_server *rpcs, rpc_ptr umem_area,
                     uint64_t size, tarpc_xsk_umem_config *config,
                     rpc_ptr *umem)
{
    tarpc_xsk_umem__create_in in;
    tarpc_xsk_umem__create_out out;

    te_string cfg_str = TE_STRING_INIT_STATIC(CONFIG_STR_LEN);

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.umem_area = umem_area;
    in.size = size;

    if (config != NULL)
    {
        in.config.config_val = config;
        in.config.config_len = 1;
    }

    rcf_rpc_call(rpcs, "xsk_umem__create", &in, &out);
    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(xsk_umem__create, out.retval);

    if (RPC_IS_CALL_OK(rpcs) && rpcs->op != RCF_RPC_WAIT)
        *umem = out.umem_ptr;

    xsk_umem_config_tarpc2str(config, &cfg_str);
    TAPI_RPC_LOG(rpcs, xsk_umem__create,
                 "umem_area=" RPC_PTR_FMT ", size=%" TE_PRINTF_64 "u, "
                 "config=%s", "%d (" RPC_PTR_FMT ")",
                 RPC_PTR_VAL(umem_area), size,
                 te_string_value(&cfg_str),
                 out.retval, RPC_PTR_VAL(out.umem_ptr));
    RETVAL_INT(xsk_umem__create, out.retval);
}

/* See description in tapi_rpc_bpf.h */
int
rpc_xsk_umem__delete(rcf_rpc_server *rpcs, rpc_ptr umem)
{
    tarpc_xsk_umem__delete_in in;
    tarpc_xsk_umem__delete_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.umem_ptr = umem;

    rcf_rpc_call(rpcs, "xsk_umem__delete", &in, &out);
    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(xsk_umem__delete, out.retval);

    TAPI_RPC_LOG(rpcs, xsk_umem__delete,
                 RPC_PTR_FMT, "%d",
                 RPC_PTR_VAL(umem), out.retval);
    RETVAL_INT(xsk_umem__delete, out.retval);
}

/* Get string representation of tarpc_xsk_socket_config. */
static void
xsk_socket_config_tarpc2str(tarpc_xsk_socket_config *config, te_string *str)
{
    if (config == NULL)
    {
        te_string_append(str, "%s", "(null)");
    }
    else
    {
        te_string_append(
                 str,
                 "{.rx_size=%u, .tx_size=%u, .libxdp_flags=%s, "
                 ".xdp_flags=0x%x, .bind_flags=%s}",
                 config->rx_size, config->tx_size,
                 xsk_libxdp_flags_rpc2str(config->libxdp_flags),
                 config->xdp_flags,
                 xdp_bind_flags_rpc2str(config->bind_flags));
    }
}

/* See description in tapi_rpc_bpf.h */
int
rpc_xsk_socket__create(rcf_rpc_server *rpcs, const char *if_name,
                       uint32_t queue_id, rpc_ptr umem,
                       te_bool shared_umem,
                       tarpc_xsk_socket_config *config,
                       rpc_ptr *sock)
{
    tarpc_xsk_socket__create_in in;
    tarpc_xsk_socket__create_out out;

    te_string cfg_str = TE_STRING_INIT_STATIC(CONFIG_STR_LEN);

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.if_name = (char *)if_name;
    in.queue_id = queue_id;
    in.umem_ptr = umem;
    in.shared_umem = shared_umem;

    if (config != NULL)
    {
        in.config.config_val = config;
        in.config.config_len = 1;
    }

    rcf_rpc_call(rpcs, "xsk_socket__create", &in, &out);
    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(xsk_socket__create, out.retval);

    if (RPC_IS_CALL_OK(rpcs) && rpcs->op != RCF_RPC_WAIT)
        *sock = out.socket_ptr;

    xsk_socket_config_tarpc2str(config, &cfg_str);
    TAPI_RPC_LOG(rpcs, xsk_socket__create,
                 "if_name=%s, queue_id=%u, umem=" RPC_PTR_FMT ", "
                 "shared_umem=%s, config=%s", "%d (" RPC_PTR_FMT ")",
                 if_name, queue_id, RPC_PTR_VAL(umem),
                 shared_umem ? "TRUE" : "FALSE",
                 te_string_value(&cfg_str),
                 out.retval, RPC_PTR_VAL(out.socket_ptr));
    RETVAL_INT(xsk_socket__create, out.retval);
}

/* See description in tapi_rpc_bpf.h */
int
rpc_xsk_socket__delete(rcf_rpc_server *rpcs, rpc_ptr sock)
{
    tarpc_xsk_socket__delete_in in;
    tarpc_xsk_socket__delete_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.socket_ptr = sock;

    rcf_rpc_call(rpcs, "xsk_socket__delete", &in, &out);
    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(xsk_socket__delete, out.retval);

    TAPI_RPC_LOG(rpcs, xsk_socket__delete,
                 RPC_PTR_FMT, "%d",
                 RPC_PTR_VAL(sock), out.retval);
    RETVAL_INT(xsk_socket__delete, out.retval);
}

/* See description in tapi_rpc_bpf.h */
int
rpc_xsk_map_set(rcf_rpc_server *rpcs, int map_fd,
                unsigned int key, int fd)
{
    tarpc_xsk_map_set_in in;
    tarpc_xsk_map_set_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.map_fd = map_fd;
    in.key = key;
    in.fd = fd;

    rcf_rpc_call(rpcs, "xsk_map_set", &in, &out);
    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(xsk_map_set,
                                          out.retval);

    TAPI_RPC_LOG(rpcs, xsk_map_set,
                 "map_fd=%d, key=%u, fd=%d", "%d",
                 map_fd, key, fd, out.retval);
    RETVAL_INT(xsk_map_set, out.retval);
}

/* See description in tapi_rpc_bpf.h */
ssize_t
rpc_xsk_rx_fill_simple(rcf_rpc_server *rpcs, rpc_ptr umem,
                       const char *if_name, uint32_t queue_id,
                       size_t frames_cnt)
{
    tarpc_xsk_rx_fill_simple_in in;
    tarpc_xsk_rx_fill_simple_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.umem_ptr = umem;
    in.if_name = (char *)if_name;
    in.queue_id = queue_id;
    in.frames_cnt = frames_cnt;

    rcf_rpc_call(rpcs, "xsk_rx_fill_simple", &in, &out);
    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(xsk_rx_fill_simple, out.retval);

    TAPI_RPC_LOG(rpcs, xsk_rx_fill_simple,
                 RPC_PTR_FMT ", if_name=%s, queue_id=%u, "
                 "frames_cnt=%" TE_PRINTF_SIZE_T "u", "%d",
                 RPC_PTR_VAL(umem), if_name, queue_id, frames_cnt,
                 out.retval);
    RETVAL_INT64(xsk_rx_fill_simple, out.retval);
}

/* See description in tapi_rpc_bpf.h */
ssize_t
rpc_xsk_receive_simple(rcf_rpc_server *rpcs, rpc_ptr sock,
                       uint8_t *buf, size_t len)
{
    tarpc_xsk_receive_simple_in in;
    tarpc_xsk_receive_simple_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.socket_ptr = sock;

    rcf_rpc_call(rpcs, "xsk_receive_simple", &in, &out);
    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(xsk_receive_simple, out.retval);

    if (RPC_IS_CALL_OK(rpcs))
    {
        if (buf != NULL && out.data.data_val != NULL)
            memcpy(buf, out.data.data_val, MIN(len, out.data.data_len));
    }

    TAPI_RPC_LOG(rpcs, xsk_receive_simple,
                 RPC_PTR_FMT ", %p, %" TE_PRINTF_SIZE_T "u",
                 "%" TE_PRINTF_SIZE_T "d",
                 RPC_PTR_VAL(sock), buf, len, out.retval);
    RETVAL_INT64(xsk_receive_simple, out.retval);
}

/* See description in tapi_rpc_bpf.h */
int
rpc_xsk_send_simple(rcf_rpc_server *rpcs, rpc_ptr sock,
                    void *buf, size_t len)
{
    tarpc_xsk_send_simple_in in;
    tarpc_xsk_send_simple_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.socket_ptr = sock;
    in.data.data_val = buf;
    in.data.data_len = len;

    rcf_rpc_call(rpcs, "xsk_send_simple", &in, &out);
    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(xsk_send_simple, out.retval);

    TAPI_RPC_LOG(rpcs, xsk_send_simple,
                 RPC_PTR_FMT ", %p, %" TE_PRINTF_SIZE_T "u", "%d",
                 RPC_PTR_VAL(sock), buf, len, out.retval);
    RETVAL_INT(xsk_send_simple, out.retval);
}

/* See description in tapi_rpc_bpf.h */
int
rpc_bpf_obj_get(rcf_rpc_server *rpcs, const char *path)
{
    tarpc_bpf_obj_get_in in;
    tarpc_bpf_obj_get_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    in.path = (char *)path;

    rcf_rpc_call(rpcs, "bpf_obj_get", &in, &out);
    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(bpf_obj_get, out.retval);

    TAPI_RPC_LOG(rpcs, bpf_obj_get,
                 "%s", "%d", path, out.retval);
    RETVAL_INT(bpf_obj_get, out.retval);
}

/* See description in tapi_rpc_bpf.h */
te_errno
tapi_bpf_map_pin_get_fd(rcf_rpc_server *rpcs, unsigned int bpf_id,
                        const char *map_name, int *fd)
{
    te_string file_path_str = TE_STRING_INIT;
    char *pin_path = NULL;
    te_errno rc = 0;

    rc = tapi_bpf_map_get_pin(rpcs->ta, bpf_id, map_name, &pin_path);
    if (rc != 0)
        goto finish;

    if (pin_path == NULL)
    {
        te_string_append(&file_path_str, "/sys/fs/bpf/");
        tapi_file_make_name(&file_path_str);
        te_string_move(&pin_path, &file_path_str);

        rc = tapi_bpf_map_set_pin(rpcs->ta, bpf_id, map_name, pin_path);
        if (rc != 0)
            goto finish;
    }

    RPC_AWAIT_ERROR(rpcs);
    *fd = rpc_bpf_obj_get(rpcs, pin_path);
    if (*fd < 0)
    {
        rc = RPC_ERRNO(rpcs);
        goto finish;
    }

finish:

    free(pin_path);
    return rc;
}
