/** @file
 * @brief RPC types definitions
 *
 * RPC analogues of definitions from sys/resource.h.
 *
 *
 * Copyright (C) 2004-2018 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 * @author Alexander Kukuta <Alexander.Kukuta@oktetlabs.ru>
 *
 */

#ifndef __TE_RPC_SYS_RESOURCE_H__
#define __TE_RPC_SYS_RESOURCE_H__

#include "te_rpc_defs.h"


#ifdef __cplusplus
extern "C" {
#endif

/**
 * TA-independent rlimit resource types.
 */
typedef enum rpc_rlimit_resource {
    RPC_RLIMIT_CPU,
    RPC_RLIMIT_FSIZE,
    RPC_RLIMIT_DATA,
    RPC_RLIMIT_STACK,
    RPC_RLIMIT_CORE,
    RPC_RLIMIT_RSS,
    RPC_RLIMIT_NOFILE,
    RPC_RLIMIT_AS,
    RPC_RLIMIT_NPROC,
    RPC_RLIMIT_MEMLOCK,
    RPC_RLIMIT_LOCKS,
    RPC_RLIMIT_SIGPENDING,
    RPC_RLIMIT_MSGQUEUE,
    RPC_RLIMIT_SBSIZE,
    RPC_RLIMIT_NLIMITS,
} rpc_rlimit_resource;

/**
 * Convert RPC resource type (setrlimit/getrllimit)
 * to string representation.
 */
extern const char * rlimit_resource_rpc2str(rpc_rlimit_resource resource);

/**
 * Convert RPC resource type (setrlimit/getrllimit) to native resource
 * type.
 */
extern int rlimit_resource_rpc2h(rpc_rlimit_resource resource);

/**
 * Convert native resource type (setrlimit/getrllimit) to RPC resource
 * type.
 */
extern rpc_rlimit_resource rlimit_resource_h2rpc(int resource);


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_RPC_SYS_RESOURCE_H__ */
