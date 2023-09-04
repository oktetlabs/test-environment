/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2023 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief RPCs for BPF
 *
 * Definitions of RPC structures and functions for BPF.
 */

struct tarpc_xsk_umem_config {
    uint32_t fill_size;
    uint32_t comp_size;
    uint32_t frame_size;
    uint32_t frame_headroom;
    uint32_t flags;
};

struct tarpc_xsk_umem__create_in {
    struct tarpc_in_arg common;

    tarpc_ptr umem_area;
    uint64_t size;
    tarpc_xsk_umem_config config<>;
};

struct tarpc_xsk_umem__create_out {
    struct tarpc_out_arg common;

    tarpc_ptr umem_ptr;
    tarpc_int retval;
};

struct tarpc_xsk_umem__delete_in {
    struct tarpc_in_arg common;

    tarpc_ptr umem_ptr;
};

typedef struct tarpc_int_retval_out tarpc_xsk_umem__delete_out;

struct tarpc_xsk_socket_config {
    uint32_t rx_size;
    uint32_t tx_size;
    uint32_t libxdp_flags;
    uint32_t xdp_flags;
    uint32_t bind_flags;
};

struct tarpc_xsk_socket__create_in {
    struct tarpc_in_arg common;

    string if_name<>;
    uint32_t queue_id;
    tarpc_ptr umem_ptr;
    tarpc_bool shared_umem;
    tarpc_xsk_socket_config config<>;
};

struct tarpc_xsk_socket__create_out {
    struct tarpc_out_arg common;

    tarpc_ptr socket_ptr;
    tarpc_int retval;
};

struct tarpc_xsk_socket__delete_in {
    struct tarpc_in_arg common;

    tarpc_ptr socket_ptr;
};

typedef struct tarpc_int_retval_out tarpc_xsk_socket__delete_out;

struct tarpc_xsk_map_set_in {
    struct tarpc_in_arg common;

    int map_fd;
    uint32_t key;
    int fd;
};

typedef struct tarpc_int_retval_out tarpc_xsk_map_set_out;

struct tarpc_xsk_rx_fill_simple_in {
    struct tarpc_in_arg common;

    tarpc_ptr umem_ptr;
    string if_name<>;
    uint32_t queue_id;
    tarpc_size_t frames_cnt;
};

typedef struct tarpc_ssize_t_retval_out tarpc_xsk_rx_fill_simple_out;

struct tarpc_xsk_receive_simple_in {
    struct tarpc_in_arg common;

    tarpc_ptr socket_ptr;
};

struct tarpc_xsk_receive_simple_out {
    struct tarpc_out_arg common;

    uint8_t data<>;
    tarpc_ssize_t retval;
};

struct tarpc_xsk_send_simple_in {
    struct tarpc_in_arg common;

    tarpc_ptr socket_ptr;
    uint8_t data<>;
};

typedef struct tarpc_int_retval_out tarpc_xsk_send_simple_out;

struct tarpc_bpf_obj_get_in {
    struct tarpc_in_arg common;

    string path<>;
};

typedef struct tarpc_int_retval_out tarpc_bpf_obj_get_out;

program bpf
{
    version ver0
    {
        RPC_DEF(xsk_umem__create)
        RPC_DEF(xsk_umem__delete)
        RPC_DEF(xsk_socket__create)
        RPC_DEF(xsk_socket__delete)
        RPC_DEF(xsk_map_set)
        RPC_DEF(xsk_rx_fill_simple)
        RPC_DEF(xsk_receive_simple)
        RPC_DEF(xsk_send_simple)
        RPC_DEF(bpf_obj_get)
    } = 1;
} = 1;
