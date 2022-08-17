/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Socket API RPC definitions
 *
 * Definition data types used in Socket API RPC.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_RPC_TYPES_H__
#define __TE_RPC_TYPES_H__


#define RPC_NULL            0
#define RPC_UNKNOWN_ADDR    0xFFFFFFFF

/** Option length should be calculated automatically */
#define RPC_OPTLEN_AUTO         0xFFFFFFFF


typedef uint32_t rpc_ptr;
typedef rpc_ptr rpc_fd_set_p;
typedef rpc_ptr rpc_sigset_p;
typedef rpc_ptr  rpc_aiocb_p;

typedef rpc_ptr rpc_iomux_state_p;

/** Pointer to the memory area */
typedef struct rpc_ptr_off {
    rpc_ptr     base;   /**< Handle of the base address */
    uint32_t    offset; /**< Offset from the base */
} rpc_ptr_off;

#define RPC_PTR_OFF_IS_NULL(ptr_)    ((ptr_).base == RPC_NULL)
#define RPC_PTR_OFF_INC(ptr_, off_)  ((ptr_).offset += (off_))
#define RPC_PTR_OFF_DEC(ptr_, off_)  ((ptr_).offset -= (off_))
#define RPC_PTR_OFF_GET_BASE(ptr_)   ((ptr_).base)
#define RPC_PTR_OFF_GET_OFFSET(ptr_) ((ptr_).offset)

#ifndef WINDOWS

#include "te_rpc_aio.h"
#include "te_rpc_netdb.h"
#include "te_rpc_net_if_arp.h"
#include "te_rpc_net_if.h"
#include "te_rpc_linux_net_tstamp.h"
#include "te_rpc_signal.h"
#include "te_rpc_sys_poll.h"
#include "te_rpc_sys_epoll.h"
#include "te_rpc_sys_stat.h"
#include "te_rpc_sys_wait.h"
#include "te_rpc_sys_resource.h"
#include "te_rpc_sys_systeminfo.h"
#include "te_rpc_sys_time.h"
#include "te_rpc_sys_mman.h"
#include "te_rpc_sysconf.h"
#include "te_rpc_errno.h"
#include "te_rpc_dlfcn.h"
#include "te_rpc_dirent.h"
#include "te_rpc_pthread.h"

#else

#include "te_win_defs.h"

#endif

#include "te_rpc_wsa.h"
#include "te_rpc_sys_socket.h"
#include "te_rpc_fcntl.h"

/**
 * Pattern passed to set_buf_pattern to indicate that the buffer should
 * be filled by random bytes.
 */
#define TAPI_RPC_BUF_RAND       256

/**
 * An identifier corresponding to memory address
 *
 * @note    It is related to the definition of the pointer type @b tarpc_ptr in
 *          include/tarpc.x.m4
 */
typedef uint32_t rpc_ptr_id_index;

/** An identifier corresponding to namespace of memory pointers group */
typedef uint32_t rpc_ptr_id_namespace;

/** Invalid namespace */
#define RPC_PTR_ID_NS_INVALID       0xFFFFFFFF

/** The count of bits for namespace value in a pointer id */
#define RPC_PTR_ID_NS_BITCOUNT      8

/** The amount of bits for index in @b ids in a pointer id */
#define RPC_PTR_ID_INDEX_BITCOUNT   (           \
        sizeof(rpc_ptr_id_index) * CHAR_BIT -   \
        RPC_PTR_ID_NS_BITCOUNT                  \
    )

/** Maximum index value in @b ids in a pointer id */
#define RPC_PTR_ID_INDEX_LIMIT      (1 << RPC_PTR_ID_INDEX_BITCOUNT)

/** Mask of ids in a pointer id */
#define RPC_PTR_ID_INDEX_MASK       (RPC_PTR_ID_INDEX_LIMIT - 1)

/**
 * Create a composite identifier
 *
 * @param _ns       Index in the @b namespaces array
 * @param _index    Index in the @b ids array
 *
 * @return          Pointer id
 *
 * @note            @c 0 is equivalent to @c NULL for @b rpc_ptr and @c 1 is
 *                  added to support this.
 */
#define RPC_PTR_ID_MAKE(_ns, _index) (          \
        ((_ns) << RPC_PTR_ID_INDEX_BITCOUNT) +  \
        ((_index) & RPC_PTR_ID_INDEX_MASK) +    \
        1                                       \
    )

/**
 * @defgroup rpc_type_ns RPC pointer namespaces
 * @ingroup te_lib_rpc
 * @{
 * List of available namespaces to use in RPC pointers.
 */

/** Generic namespace used by default. */
#define RPC_TYPE_NS_GENERIC    ""

/** FD set to use in select() like RPC calls. */
#define RPC_TYPE_NS_FD_SET     "fd_set"

/**@} <!-- END rpc_type_ns --> */

/**
 * Extract index of @b ids array item from the identifier @p _id
 *
 * @param _id       Pointer id
 *
 * @return          index of @b ids array item
 */
#define RPC_PTR_ID_GET_INDEX(_id) (((_id) - 1) & RPC_PTR_ID_INDEX_MASK)

/**
 * Extract index of @b namespaces array item from the identifier @p _id
 *
 * @param _id       Pointer id
 *
 * @return          index of @b namespaces array item
 */
#define RPC_PTR_ID_GET_NS(_id) (((_id) - 1) >> RPC_PTR_ID_INDEX_BITCOUNT)

#endif /* !__TE_RPC_TYPES_H__ */
