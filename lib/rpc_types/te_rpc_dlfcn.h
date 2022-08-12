/** @file
 * @brief RPC types definitions
 *
 * RPC analogues of definitions from dlfcn.h.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 *
 */

#ifndef __TE_RPC_DLFCN_H__
#define __TE_RPC_DLFCN_H__

#include "te_rpc_defs.h"


#ifdef __cplusplus
extern "C" {
#endif

/**
 * All known dlopen mode flags.
 */
typedef enum rpc_dlopen_flags {
    RPC_RTLD_LOCAL          = 0,
    RPC_RTLD_LAZY           = 0x00001,
    RPC_RTLD_NOW            = 0x00002,
    RPC_RTLD_NOLOAD         = 0x00004,
    RPC_RTLD_DEEPBIND       = 0x00008,
    RPC_RTLD_GLOBAL         = 0x00100,
    RPC_RTLD_NODELETE       = 0x01000,
} rpc_dlopen_flags;


#define DLOPEN_FLAGS_MAPPING_LIST \
    RPC_BIT_MAP_ENTRY(RTLD_LOCAL), \
    RPC_BIT_MAP_ENTRY(RTLD_LAZY), \
    RPC_BIT_MAP_ENTRY(RTLD_NOW), \
    RPC_BIT_MAP_ENTRY(RTLD_NOLOAD), \
    RPC_BIT_MAP_ENTRY(RTLD_DEEPBIND), \
    RPC_BIT_MAP_ENTRY(RTLD_GLOBAL), \
    RPC_BIT_MAP_ENTRY(RTLD_NODELETE)

/**
 * dlopen_flags_rpc2str()
 */
RPCBITMAP2STR(dlopen_flags, DLOPEN_FLAGS_MAPPING_LIST)

/** Convert RPC dlopen mode flags to native flags */
extern unsigned int dlopen_flags_rpc2h(unsigned int flags);

/** Convert dlopen mode native flags to RPC flags */
extern unsigned int dlopen_flags_h2rpc(unsigned int flags);


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_RPC_DLFCN_H__ */
