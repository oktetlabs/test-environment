/** @file
 * @brief RPC types definitions
 *
 * RPC analogues of definitions from netdb.h.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 */

#ifndef __TE_RPC_NETDB_H__
#define __TE_RPC_NETDB_H__

#include "te_rpc_defs.h"


#ifdef __cplusplus
extern "C" {
#endif

/** TA-independent addrinfo flags */
typedef enum rpc_ai_flags {
    RPC_AI_PASSIVE     = 1,    /**< Socket address is intended for `bind` */
    RPC_AI_CANONNAME   = 2,    /**< Request for canonical name */
    RPC_AI_NUMERICHOST = 4,    /**< Don't use name resolution */
    RPC_AI_UNKNOWN     = 8     /**< Invalid flags */
} rpc_ai_flags;

#define AI_INVALID      0xFFFFFFFF
#define AI_ALL_FLAGS    (AI_PASSIVE | AI_CANONNAME | AI_NUMERICHOST)

/** Convert RPC AI flags to native ones */
extern int ai_flags_rpc2h(rpc_ai_flags flags);

/** Convert native AI flags to RPC ones */
extern rpc_ai_flags ai_flags_h2rpc(int flags);


/* TA-independent getaddrinfo() return codes  */
typedef enum rpc_ai_rc {
    RPC_EAI_BADFLAGS,       /**< Invalid value for `ai_flags` field */
    RPC_EAI_NONAME,         /**< NAME or SERVICE is unknown */
    RPC_EAI_AGAIN,          /**< Temporary failure in name resolution */
    RPC_EAI_FAIL,           /**< Non-recoverable failure in name res */
    RPC_EAI_NODATA,         /**< No address associated with NAME */
    RPC_EAI_FAMILY,         /**< `ai_family` not supported */
    RPC_EAI_SOCKTYPE,       /**< `ai_socktype` not supported */
    RPC_EAI_SERVICE,        /**< SERVICE not supported for `ai_socktype` */
    RPC_EAI_ADDRFAMILY,     /**< Address family for NAME not supported */
    RPC_EAI_MEMORY,         /**< Memory allocation failure */
    RPC_EAI_SYSTEM,         /**< System error returned in `errno` */
    RPC_EAI_INPROGRESS,     /**< Processing request in progress */
    RPC_EAI_CANCELED,       /**< Request canceled */
    RPC_EAI_NOTCANCELED,    /**< Request not canceled */
    RPC_EAI_ALLDONE,        /**< All requests done */
    RPC_EAI_INTR,           /**< Interrupted by a signal */
    RPC_EAI_UNKNOWN
} rpc_ai_rc;

extern rpc_ai_rc ai_rc_h2rpc(int rc);


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_RPC_NETDB_H__ */
