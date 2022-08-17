/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief RPC types definitions
 *
 * RPC analogues of definitions from sys/stat.h.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_RPC_SYS_STAT_H__
#define __TE_RPC_SYS_STAT_H__

#include "te_rpc_defs.h"


#ifdef __cplusplus
extern "C" {
#endif

/**
 * All known file mode flags.
 */
typedef enum rpc_file_mode_flags {
    RPC_S_ISUID  = (1 << 1),
    RPC_S_ISGID  = (1 << 2),
    RPC_S_IRUSR  = (1 << 3),
    RPC_S_IWUSR  = (1 << 4),
    RPC_S_IXUSR  = (1 << 5),
    RPC_S_IRWXU  = (1 << 6),
    RPC_S_IREAD  = (1 << 7),
    RPC_S_IWRITE = (1 << 8),
    RPC_S_IEXEC  = (1 << 9),
    RPC_S_IRGRP  = (1 << 10),
    RPC_S_IWGRP  = (1 << 11),
    RPC_S_IXGRP  = (1 << 12),
    RPC_S_IRWXG  = (1 << 13),
    RPC_S_IROTH  = (1 << 14),
    RPC_S_IWOTH  = (1 << 15),
    RPC_S_IXOTH  = (1 << 16),
    RPC_S_IRWXO  = (1 << 17),
} rpc_file_mode_flags;

#define FILE_MODE_FLAGS_MAPPING_LIST \
    RPC_BIT_MAP_ENTRY(S_ISUID), \
    RPC_BIT_MAP_ENTRY(S_ISGID), \
    RPC_BIT_MAP_ENTRY(S_IRUSR), \
    RPC_BIT_MAP_ENTRY(S_IWUSR), \
    RPC_BIT_MAP_ENTRY(S_IXUSR), \
    RPC_BIT_MAP_ENTRY(S_IRWXU), \
    RPC_BIT_MAP_ENTRY(S_IREAD), \
    RPC_BIT_MAP_ENTRY(S_IWRITE), \
    RPC_BIT_MAP_ENTRY(S_IEXEC), \
    RPC_BIT_MAP_ENTRY(S_IRGRP), \
    RPC_BIT_MAP_ENTRY(S_IWGRP), \
    RPC_BIT_MAP_ENTRY(S_IXGRP), \
    RPC_BIT_MAP_ENTRY(S_IRWXG), \
    RPC_BIT_MAP_ENTRY(S_IROTH), \
    RPC_BIT_MAP_ENTRY(S_IWOTH), \
    RPC_BIT_MAP_ENTRY(S_IXOTH), \
    RPC_BIT_MAP_ENTRY(S_IRWXO)

/**
 * file_mode_flags_rpc2str()
 */
RPCBITMAP2STR(file_mode_flags, FILE_MODE_FLAGS_MAPPING_LIST)

/** Convert RPC mode flags to native flags */
extern unsigned int file_mode_flags_rpc2h(unsigned int flags);

typedef enum rpc_access_mode_flags {
    RPC_F_OK = 0,
    RPC_R_OK = (1 << 0),
    RPC_W_OK = (1 << 1),
    RPC_X_OK = (1 << 2)
} rpc_access_mode_flags;

/* F_OK not listed as it is defined to zero */
#define ACCESS_MODE_FLAGS_MAPPING_LIST \
    RPC_BIT_MAP_ENTRY(R_OK), \
    RPC_BIT_MAP_ENTRY(W_OK), \
    RPC_BIT_MAP_ENTRY(X_OK)

/**
 * access_mode_flags_rpc2str()
 */
RPCBITMAP2STR(access_mode_flags, ACCESS_MODE_FLAGS_MAPPING_LIST)

/** Convert RPC access mode to native flags */
extern int access_mode_flags_rpc2h(int mode);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_RPC_SYS_STAT_H__ */
