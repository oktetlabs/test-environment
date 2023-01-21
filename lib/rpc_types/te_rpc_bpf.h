/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2023 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief RPC types for BPF-related calls
 *
 * Declaration of RPC types for BPF-related calls
 */

#ifndef __TE_RPC_BPF_H__
#define __TE_RPC_BPF_H__

#include "te_rpc_defs.h"

/** Namespace for pointers to internal ta_xsk_umem structure */
#define RPC_TYPE_NS_XSK_UMEM "xsk_umem"

/** Namespace for pointers to internal ta_xsk_socket structure */
#define RPC_TYPE_NS_XSK_SOCKET "xsk_socket"

/** Libxdp flags which can be specified when creating AF_XDP socket */
typedef enum rpc_xsk_libxdp_flags {
    /** Do not try to load default XDP program */
    RPC_XSK_LIBXDP_FLAGS__INHIBIT_PROG_LOAD = (1 << 0),
} rpc_xsk_libxdp_flags;

/**
 * List of values in rpc_xsk_libxdp_flags for the purpose of
 * converting flags to string.
 */
#define XSK_LIBXDP_FLAGS_MAPPING_LIST \
    RPC_BIT_MAP_ENTRY(XSK_LIBXDP_FLAGS__INHIBIT_PROG_LOAD)

/**
 * xsk_libxdp_flags_rpc2str()
 */
RPCBITMAP2STR(xsk_libxdp_flags, XSK_LIBXDP_FLAGS_MAPPING_LIST)

/**
 * @name Flags passed when binding AF_XDP socket.
 * @{
 */

/** UMEM is shared */
#define RPC_XDP_BIND_SHARED_UMEM (1 << 0)
/** Force copy mode */
#define RPC_XDP_BIND_COPY (1 << 1)
/** Force zero-copy mode */
#define RPC_XDP_BIND_ZEROCOPY (1 << 2)
/**
 * Enable XDP_RING_NEED_WAKEUP flag for FILL and
 * Tx rings. When that flag is set, kernel needs
 * syscall (poll() or sendto()) to wake up the driver.
 * Checking that flag can reduce number of syscalls
 * and improve performance.
 */
#define RPC_XDP_BIND_USE_NEED_WAKEUP (1 << 3)

/**@} */

/**
 * List of XDP socket bind flags for the purpose of
 * converting flags to string.
 */
#define XDP_BIND_FLAGS_MAPPING_LIST \
    RPC_BIT_MAP_ENTRY(XDP_BIND_SHARED_UMEM), \
    RPC_BIT_MAP_ENTRY(XDP_BIND_COPY), \
    RPC_BIT_MAP_ENTRY(XDP_BIND_ZEROCOPY), \
    RPC_BIT_MAP_ENTRY(XDP_BIND_USE_NEED_WAKEUP)

/**
 * xdp_bind_flags_rpc2str()
 */
RPCBITMAP2STR(xdp_bind_flags, XDP_BIND_FLAGS_MAPPING_LIST)

#endif /* !__TE_RPC_BPF_H__ */
