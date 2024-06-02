/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2024 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief RPC for Agent netconf client control
 *
 * Definition of RPC structures and functions for Agent netconf client control
 */

typedef uint64_t tarpc_nc_session_ptr;
typedef uint64_t tarpc_nc_rpc_ptr;

/*
 * libnetconf2/log.h
 */
struct tarpc_nc_libssh_thread_verbosity_in {
    struct tarpc_in_arg common;

    tarpc_int           level;
};
typedef struct tarpc_void_out tarpc_nc_libssh_thread_verbosity_out;

/*
 * libnetconf2/session.h
 */
/* nc_session_free */
struct tarpc_nc_session_free_in {
    struct tarpc_in_arg     common;

    tarpc_nc_session_ptr    session;
};
typedef struct tarpc_void_out tarpc_nc_session_free_out;

/*
 * libnetconf2/session_client.h
 */
/* nc_client_init() */
typedef struct tarpc_void_in tarpc_nc_client_init_in;
typedef struct tarpc_void_out tarpc_nc_client_init_out;

/* nc_client_destroy() */
typedef struct tarpc_void_in tarpc_nc_client_destroy_in;
typedef struct tarpc_void_out tarpc_nc_client_destroy_out;

/* nc_client_ssh_set_username() */
struct tarpc_nc_client_ssh_set_username_in {
    struct tarpc_in_arg common;

    string              username<>;
};
typedef struct tarpc_int_retval_out tarpc_nc_client_ssh_set_username_out;

/* nc_client_ssh_add_keypair() */
struct tarpc_nc_client_ssh_add_keypair_in {
    struct tarpc_in_arg common;

    string              pub_key<>;
    string              priv_key<>;
};
typedef struct tarpc_int_retval_out tarpc_nc_client_ssh_add_keypair_out;

/* nc_connect_ssh() */
struct tarpc_nc_connect_ssh_in {
    struct tarpc_in_arg common;

    string              host<>;
    tarpc_int           port;
};
struct tarpc_nc_connect_ssh_out {
    struct tarpc_out_arg common;

    tarpc_nc_session_ptr session;
};

/* nc_send_rpc() */
struct tarpc_nc_send_rpc_in {
    struct tarpc_in_arg     common;

    tarpc_nc_session_ptr    session;
    tarpc_nc_rpc_ptr        rpc;
    tarpc_int               timeout;
};
struct tarpc_nc_send_rpc_out {
    struct tarpc_out_arg    common;

    tarpc_int               msg_type;
    uint64_t                msgid;
};

/* nc_recv_reply() */
struct tarpc_nc_recv_reply_in {
    struct tarpc_in_arg     common;

    tarpc_nc_session_ptr    session;
    tarpc_nc_rpc_ptr        rpc;
    uint64_t                msgid;
    tarpc_int               timeout;
};
struct tarpc_nc_recv_reply_out {
    struct tarpc_out_arg    common;

    tarpc_int               msg_type;
    string                  envp<>;
    string                  op<>;
};

/*
 * libnetconf2/messages_client.h
 */
/* Common */
struct tarpc_nc_rpc_ptr_out {
    struct tarpc_out_arg    common;

    tarpc_nc_rpc_ptr        rpc;
};

/* nc_rpc_get() */
struct tarpc_nc_rpc_get_in {
    struct tarpc_in_arg common;

    string              filter<>;
    tarpc_int           wd_mode;
};
typedef struct tarpc_nc_rpc_ptr_out tarpc_nc_rpc_get_out;

/* nc_rpc_getconfig() */
struct tarpc_nc_rpc_getconfig_in {
    struct tarpc_in_arg common;

    string              filter<>;
    tarpc_int           source;
    tarpc_int           wd_mode;
};
typedef struct tarpc_nc_rpc_ptr_out tarpc_nc_rpc_getconfig_out;

/* nc_rpc_edit() */
struct tarpc_nc_rpc_edit_in {
    struct tarpc_in_arg common;

    tarpc_int           target;
    tarpc_int           default_op;
    tarpc_int           test_opt;
    tarpc_int           error_opt;
    string              edit_content<>;
};
typedef struct tarpc_nc_rpc_ptr_out tarpc_nc_rpc_edit_out;

/* nc_rpc_copy() */
struct tarpc_nc_rpc_copy_in {
    struct tarpc_in_arg common;

    tarpc_int           target;
    tarpc_int           source;
    tarpc_int           wd_mode;
    string              url_trg<>;
    string              url_or_config_src<>;
};
typedef struct tarpc_nc_rpc_ptr_out tarpc_nc_rpc_copy_out;

/* nc_rpc_free() */
struct tarpc_nc_rpc_free_in {
    struct tarpc_in_arg common;

    tarpc_nc_rpc_ptr    rpc;
};
typedef struct tarpc_void_out tarpc_nc_rpc_free_out;

program netconfc
{
    version ver0
    {
        RPC_DEF(nc_libssh_thread_verbosity)
        RPC_DEF(nc_session_free)
        RPC_DEF(nc_client_init)
        RPC_DEF(nc_client_destroy)
        RPC_DEF(nc_client_ssh_set_username)
        RPC_DEF(nc_client_ssh_add_keypair)
        RPC_DEF(nc_connect_ssh)
        RPC_DEF(nc_send_rpc)
        RPC_DEF(nc_recv_reply)
        RPC_DEF(nc_rpc_get)
        RPC_DEF(nc_rpc_getconfig)
        RPC_DEF(nc_rpc_edit)
        RPC_DEF(nc_rpc_copy)
        RPC_DEF(nc_rpc_free)
    } = 1;
} = 2;
