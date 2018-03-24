/** @file
 * @brief RPC for UPnP Control Point
 *
 * Definition of RPC structures and functions for UPnP Control Point
 *
 * Copyright (C) 2003-2018 OKTET Labs.
 *
 * @author Ivan Melnikov <Ivan.Melnikov@oktetlabs.ru>
 */

/*
 * The is in fact appended to tarpc.x.m4, so it may reuse any types
 * defined there.
 */

/** Create connection arguments. */
typedef struct tarpc_void_in tarpc_upnp_cp_connect_in;
typedef struct tarpc_int_retval_out tarpc_upnp_cp_connect_out;

/** Destroy connection arguments. */
typedef struct tarpc_void_in tarpc_upnp_cp_disconnect_in;
typedef struct tarpc_int_retval_out tarpc_upnp_cp_disconnect_out;

/** Request/reply routine arguments. */
struct tarpc_upnp_cp_action_in {
    struct tarpc_in_arg common;

    uint8_t             buf<>;      /**< Buffer with request data */
};
struct tarpc_upnp_cp_action_out {
    struct tarpc_out_arg  common;

    uint8_t             buf<>;      /**< Buffer with response data */
    tarpc_int           retval;     /**< Status code */
};

program upnp_cp
{
    version ver0
    {
        RPC_DEF(upnp_cp_connect)
        RPC_DEF(upnp_cp_disconnect)
        RPC_DEF(upnp_cp_action)
    } = 1;
} = 3;
