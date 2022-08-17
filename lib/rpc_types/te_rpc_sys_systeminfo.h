/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief RPC types definitions
 *
 * RPC analogues of definitions from SunOS' sys/systeminfo.h.
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_RPC_SYS_SYSTEMINFO_H__
#define __TE_RPC_SYS_SYSTEMINFO_H__

#include "te_rpc_defs.h"


#ifdef __cplusplus
extern "C" {
#endif

/**
 * TA-independent sysinfo() commands
 */
typedef enum rpc_sysinfo_command {
    RPC_SI_SYSNAME,
    RPC_SI_HOSTNAME,
    RPC_SI_RELEASE,
    RPC_SI_VERSION,
    RPC_SI_MACHINE,
    RPC_SI_ARCHITECTURE,
    RPC_SI_HW_SERIAL,
    RPC_SI_HW_PROVIDER,
    RPC_SI_SRPC_DOMAIN,
    RPC_SI_SET_HOSTNAME,
    RPC_SI_SET_SRPC_DOMAIN,
    RPC_SI_PLATFORM,
    RPC_SI_ISALIST,
    RPC_SI_DHCP_CACHE,
    RPC_SI_ARCHITECTURE_32,
    RPC_SI_ARCHITECTURE_64,
    RPC_SI_ARCHITECTURE_K,
    RPC_SI_ARCHITECTURE_NATIVE,
} rpc_sysinfo_command;

/**
 * Convert RPC resource type (setrlimit/getrllimit)
 * to string representation.
 */
extern const char * sysinfo_command_rpc2str(rpc_sysinfo_command resource);

/**
 * Convert RPC resource type (setrlimit/getrllimit) to native resource
 * type.
 */
extern int sysinfo_command_rpc2h(rpc_sysinfo_command resource);

extern rpc_sysinfo_command sysinfo_command_h2rpc(int command);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_RPC_SYS_RESOURCE_H__ */
