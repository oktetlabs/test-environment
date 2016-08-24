/** @file
 * @brief RPC for UPnP Control Point
 *
 * Definition of RPC structures and functions for UPnP Control Point
 *
 * Copyright (C) 2016 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
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
