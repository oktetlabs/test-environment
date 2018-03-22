/** @file
 * @brief RPC types definitions
 *
 * RPC analogues of constant names for sysconf().
 * 
 * 
 * Copyright (C) 2003-2018 OKTET Labs. All rights served.
 *
 * 
 *
 *
 * @author Dmitry Izbitsky <Dmitry.Izbitsky@oktetlabs.ru>
 *
 * $Id$
 */
 
#ifndef __TE_RPC_SYSCONF_H__
#define __TE_RPC_SYSCONF_H__

#include "te_rpc_defs.h"


#ifdef __cplusplus
extern "C" {
#endif

/**
 * TA-independent sysconf() names.
 */
typedef enum rpc_sysconf_name {
    RPC_SC_ARG_MAX,
    RPC_SC_CHILD_MAX,
    RPC_SC_HOST_NAME_MAX,
    RPC_SC_OPEN_MAX,
    RPC_SC_PAGESIZE,
    RPC_SC_UNKNOWN,
} rpc_sysconf_name;

/** 
 * Convert RPC sysconf() name
 * to string representation.
 */
extern const char *sysconf_name_rpc2str(rpc_sysconf_name name);

/**
 * Convert RPC sysconf() name to native one.
 */
extern int sysconf_name_rpc2h(rpc_sysconf_name name);

/**
 * Convert native sysconf() nameto RPC one.
 */
extern rpc_sysconf_name sysconf_name_h2rpc(int name);


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_RPC_SYSCONF_H__ */
