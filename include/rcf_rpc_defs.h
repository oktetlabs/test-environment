/** @file
 * @brief RCF RPC definitions
 *
 * Definition of functions to create/destroy RPC servers on Test
 * Agents and to set/get RPC server context parameters.
 *
 *
 * Copyright (C) 2004-2018 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 * @author Elena Vengerova <Elena.Vengerova@oktetlabs.ru>
 *
 */

#ifndef __TE_RCF_RPC_DEFS_H__
#define __TE_RCF_RPC_DEFS_H__

/** Operations for RPC */
typedef enum {
    RCF_RPC_CALL,       /**< Call non-blocking RPC (if supported) */
    RCF_RPC_WAIT,       /**< Wait until non-blocking RPC is finished */
    RCF_RPC_CALL_WAIT   /**< Call blocking RPC */
} rcf_rpc_op;

#define RCF_RPC_NAME_LEN    64

#define RCF_RPC_MAX_BUF     1048576
#define RCF_RPC_MAX_IOVEC   32
#define RCF_RPC_MAX_CMSGHDR 8

#define RCF_RPC_MAX_MSGHDR  32

/** Check, if errno is RPC errno, not other TE errno */
#define RPC_IS_ERRNO_RPC(_errno) \
    (((_errno) == 0) || (TE_RC_GET_MODULE(_errno) == TE_RPC))



/** @name Flags for get/create RPC server */

/** Create a subthread of the existing RPC server */
#define RCF_RPC_SERVER_GET_THREAD   0x01
/** Get only existing RPC server */
#define RCF_RPC_SERVER_GET_EXISTING 0x02
/** Reuse existing RPC server if possible without restart */
#define RCF_RPC_SERVER_GET_REUSE    0x04
/**
 * Register in configuration tree existing RPC server process
 * (created without help of configurator previously, for example,
 * to avoid setting flags like RCF_RPC_SERVER_GET_INHERIT
 * automatically by configurator for the process creation).
 */
#define RCF_RPC_SERVER_GET_REGISTER 0x40

/** Next flags may be passed to RPC create_process */
/** exec after fork */
#define RCF_RPC_SERVER_GET_EXEC     0x08
/** Windows-specific: inherit file handles */
#define RCF_RPC_SERVER_GET_INHERIT  0x10
/** Windows-specific: initialize network */
#define RCF_RPC_SERVER_GET_NET_INIT 0x20
/*@}*/

/** Maximum length of string describing error. */
#define RPC_ERROR_MAX_LEN 1024

#ifdef __unix__
/**
 * Initialize RPC server.
 *
 * @note This feature is supported only for unix agent.
 *
 * @return Status code
 */
extern int rcf_rpc_server_init(void);

/**
 * Finalize RPC server work.
 *
 * @note This feature is supported only for unix agent.
 *
 * @return Status code
 */
extern int rcf_rpc_server_finalize(void);
#endif

#endif /* !__TE_RCF_RPC_DEFS_H__ */
