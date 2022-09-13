/** @file
 * @brief RPC types definitions
 *
 * RPC analogues of definitions from sys/uio.h.
 *
 *
 * Copyright (C) 2022 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 * @author Nikita Grebennikov <tej@oktetlabs.ru>
 *
 */

#ifndef __TE_RPC_PREADV2_PWRITEV2_H__
#define __TE_RPC_PREADV2_PWRITEV2_H__

#if HAVE_SYS_UIO_H
#include <sys/uio.h>
#endif

#include "te_rpc_defs.h"
#include "tarpc.h"

typedef enum {
    RPC_RWF_HIPRI  = 0x00000001,
    RPC_RWF_DSYNC  = 0x00000002,
    RPC_RWF_SYNC   = 0x00000004,
    RPC_RWF_NOWAIT = 0x00000008,
    RPC_RWF_APPEND = 0x00000010,
} rpc_preadv2_pwritev2_flags;

#define PREADV2_PWRITEV2_FLAGS_MAPPING_LIST \
    RPC_BIT_MAP_ENTRY(RWF_HIPRI), \
    RPC_BIT_MAP_ENTRY(RWF_DSYNC), \
    RPC_BIT_MAP_ENTRY(RWF_SYNC), \
    RPC_BIT_MAP_ENTRY(RWF_NOWAIT), \
    RPC_BIT_MAP_ENTRY(RWF_APPEND)

/**
 * preadv2_pwritev2_flags_rpc2str()
 */
RPCBITMAP2STR(preadv2_pwritev2_flags, PREADV2_PWRITEV2_FLAGS_MAPPING_LIST)

extern int preadv2_pwritev2_flags_rpc2h(rpc_preadv2_pwritev2_flags flags);
extern rpc_preadv2_pwritev2_flags preadv2_pwritev2_flags_h2rpc(int flags);

#endif /* !__TE_RPC_PREADV2_PWRITEV2_H__ */
