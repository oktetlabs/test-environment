/** @file
 * @brief RPC for DPDK
 *
 * Definition of RPC structures and functions for DPDK (rte_*)
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
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 */

/*
 * The is in fact appended to tarpc.x.m4, so it may reuse any types
 * defined there.
 */

/** Handle of the 'rte_mempool' or 0 */
typedef tarpc_ptr    tarpc_rte_mempool;

/** Handle of the 'rte_mbuf' or 0 */
typedef tarpc_ptr    tarpc_rte_mbuf;

/* Just to make two-dimensional array of strings */
struct tarpc_string {
    string  str<>;
};

/* rte_eal_init() */
struct tarpc_rte_eal_init_in {
    struct tarpc_in_arg         common;
    tarpc_int                   argc;
    tarpc_string                argv<>;
};

typedef struct tarpc_int_retval_out tarpc_rte_eal_init_out;


/** enum rte_proc_type_t */
enum tarpc_rte_proc_type_t {
    TARPC_RTE_PROC_AUTO,
    TARPC_RTE_PROC_PRIMARY,
    TARPC_RTE_PROC_SECONDARY,
    TARPC_RTE_PROC_INVALID,

    TARPC_RTE_PROC__UNKNOWN
};

/** rte_eal_process_type() */
struct tarpc_rte_eal_process_type_in {
    struct tarpc_in_arg         common;
};

struct tarpc_rte_eal_process_type_out {
    struct tarpc_out_arg        common;
    enum tarpc_rte_proc_type_t  retval;
};

struct tarpc_mbuf_in {
    struct tarpc_in_arg     common;
    tarpc_rte_mbuf          m;
};

struct tarpc_mbuf_retval_out {
    struct tarpc_out_arg    common;
    tarpc_rte_mbuf          retval;
};

/** rte_pktmbuf_pool_create() */
struct tarpc_rte_pktmbuf_pool_create_in {
    struct tarpc_in_arg     common;
    string                  name<>;
    uint32_t                n;
    uint32_t                cache_size;
    uint16_t                priv_size;
    uint16_t                data_room_size;
    tarpc_int               socket_id;
};

struct tarpc_rte_pktmbuf_pool_create_out {
    struct tarpc_out_arg    common;
    tarpc_rte_mempool       retval;
};

/** rte_pktmbuf_alloc() */
struct tarpc_rte_pktmbuf_alloc_in {
    struct tarpc_in_arg     common;
    tarpc_rte_mempool       mp;
};

typedef struct tarpc_mbuf_retval_out tarpc_rte_pktmbuf_alloc_out;

/** rte_pktmbuf_free() */
typedef struct tarpc_mbuf_in tarpc_rte_pktmbuf_free_in;

typedef struct tarpc_void_out tarpc_rte_pktmbuf_free_out;

/** rte_pktmbuf_append_data() */
struct tarpc_rte_pktmbuf_append_data_in {
    struct tarpc_in_arg     common;
    tarpc_rte_mbuf          m;
    uint8_t                 buf<>;
};

typedef struct tarpc_int_retval_out tarpc_rte_pktmbuf_append_data_out;

/** rte_pktmbuf_read_data() */
struct tarpc_rte_pktmbuf_read_data_in {
    struct tarpc_in_arg     common;
    tarpc_rte_mbuf          m;
    tarpc_size_t            offset;
    tarpc_size_t            len;
    uint8_t                 buf<>;
};

struct tarpc_rte_pktmbuf_read_data_out {
    struct tarpc_out_arg    common;
    tarpc_ssize_t           retval;
    uint8_t                 buf<>;
};

/** rte_pktmbuf_clone() */
struct tarpc_rte_pktmbuf_clone_in {
    struct tarpc_in_arg     common;
    tarpc_rte_mbuf          m;
    tarpc_rte_mempool       mp;
};

typedef struct tarpc_mbuf_retval_out tarpc_rte_pktmbuf_clone_out;

/** rte_pktmbuf_prepend_data() */
struct tarpc_rte_pktmbuf_prepend_data_in {
    struct tarpc_in_arg     common;
    tarpc_rte_mbuf          m;
    uint8_t                 buf<>;
};

typedef struct tarpc_int_retval_out tarpc_rte_pktmbuf_prepend_data_out;

/** rte_pktmbuf_get_next() */
typedef struct tarpc_mbuf_in tarpc_rte_pktmbuf_get_next_in;

typedef struct tarpc_mbuf_retval_out tarpc_rte_pktmbuf_get_next_out;

/** rte_pktmbuf_get_pkt_len() */
typedef struct tarpc_mbuf_in tarpc_rte_pktmbuf_get_pkt_len_in;

struct tarpc_rte_pktmbuf_get_pkt_len_out {
    struct tarpc_out_arg    common;
    uint32_t                retval;
};

/** rte_pktmbuf_alloc_bulk() */
struct tarpc_rte_pktmbuf_alloc_bulk_in {
    struct tarpc_in_arg     common;
    tarpc_rte_mempool       mp;
    tarpc_uint              count;
};

struct tarpc_rte_pktmbuf_alloc_bulk_out {
    struct tarpc_out_arg    common;
    tarpc_rte_mbuf          bulk<>;
    tarpc_int               retval;
};

/** rte_pktmbuf_chain() */
struct tarpc_rte_pktmbuf_chain_in {
    struct tarpc_in_arg     common;
    tarpc_rte_mbuf          head;
    tarpc_rte_mbuf          tail;
};

typedef struct tarpc_int_retval_out tarpc_rte_pktmbuf_chain_out;

/** rte_pktmbuf_get_nb_segs() */
typedef struct tarpc_mbuf_in tarpc_rte_pktmbuf_get_nb_segs_in;

struct tarpc_rte_pktmbuf_get_nb_segs_out {
    struct tarpc_out_arg    common;
    uint8_t                 retval;
};

/** rte_pktmbuf_get_port() */
typedef struct tarpc_mbuf_in tarpc_rte_pktmbuf_get_port_in;

struct tarpc_rte_pktmbuf_get_port_out {
    struct tarpc_out_arg    common;
    uint8_t                 retval;
};

program dpdk
{
    version ver0
    {
        RPC_DEF(rte_eal_init)
        RPC_DEF(rte_eal_process_type)

        RPC_DEF(rte_pktmbuf_pool_create)
        RPC_DEF(rte_pktmbuf_alloc)
        RPC_DEF(rte_pktmbuf_free)
        RPC_DEF(rte_pktmbuf_append_data)
        RPC_DEF(rte_pktmbuf_read_data)
        RPC_DEF(rte_pktmbuf_clone)
        RPC_DEF(rte_pktmbuf_prepend_data)
        RPC_DEF(rte_pktmbuf_get_next)
        RPC_DEF(rte_pktmbuf_get_pkt_len)
        RPC_DEF(rte_pktmbuf_alloc_bulk)
        RPC_DEF(rte_pktmbuf_chain)
        RPC_DEF(rte_pktmbuf_get_nb_segs)
        RPC_DEF(rte_pktmbuf_get_port)
    } = 1;
} = 2;
