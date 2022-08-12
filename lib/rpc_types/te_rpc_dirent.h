/** @file
 * @brief RPC types definitions
 *
 * RPC analogues of definitions from dirent.h.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 * @author Oleg Kravtsov <Oleg.Kravtsov@oktetlabs.ru>
 *
 */

#ifndef __TE_RPC_DIRENT_H__
#define __TE_RPC_DIRENT_H__

#include "te_rpc_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * All known file control flags.
 */
typedef enum rpc_d_type {
    RPC_DT_UNKNOWN = 0,
    RPC_DT_FIFO = 1,
    RPC_DT_CHR = 2,
    RPC_DT_DIR = 4,
    RPC_DT_BLK = 8,
    RPC_DT_REG = 16,
    RPC_DT_LNK = 32,
    RPC_DT_SOCK = 64,
} rpc_d_type;

/** Convert RPC file type to native file type */
extern unsigned int d_type_rpc2h(unsigned int d_type);

/** Convert native file type to RPC file type */
extern unsigned int d_type_h2rpc(unsigned int d_type);

/** Convert RPC file type to string representation */
extern const char *d_type_rpc2str(rpc_d_type type);

/** Target system has 'd_type' field in struct dirent */
#define RPC_DIRENT_HAVE_D_TYPE   0x01
/** Target system has 'd_off' field in struct dirent */
#define RPC_DIRENT_HAVE_D_OFF    0x02
/** Target system has 'd_namelen' field in struct dirent */
#define RPC_DIRENT_HAVE_D_NAMLEN 0x04
/**
 * Target system has 'd_ino' field in struct dirent
 * (this field always present - it is mandatory).
 */
#define RPC_DIRENT_HAVE_D_INO    0x08

#define DIRENT_FLAGS_MAPPING_LIST \
    RPC_BIT_MAP_ENTRY(DIRENT_HAVE_D_TYPE), \
    RPC_BIT_MAP_ENTRY(DIRENT_HAVE_D_OFF), \
    RPC_BIT_MAP_ENTRY(DIRENT_HAVE_D_NAMLEN), \
    RPC_BIT_MAP_ENTRY(DIRENT_HAVE_D_INO)

/**
 * dirent_props_rpc2str()
 */
RPCBITMAP2STR(dirent_props, DIRENT_FLAGS_MAPPING_LIST)

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_RPC_DIRENT_H__ */

