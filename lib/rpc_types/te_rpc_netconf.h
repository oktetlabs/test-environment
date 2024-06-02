/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2024 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief RPC types for NETCONF-related calls
 *
 * RPC analogues of definitions from libnetconf2/ headers
 */

#ifndef __TE_RPC_NETCONF_H__
#define __TE_RPC_NETCONF_H__

#include "te_rpc_defs.h"

/**
 * TA-independent NC_VERB_LEVEL
 */
typedef enum {
    RPC_LVL_SILENT,
    RPC_LVL_RARE_CONDS,
    RPC_LVL_API_ENTRY,
    RPC_LVL_PKTS,
    RPC_LVL_FUNCS,
    RPC_LVL_INVAL,
} rpc_nc_verb_level;

/** Convert RPC nc_verb_level to string */
extern const char *nc_verb_level_rpc2str(rpc_nc_verb_level level);

/**
 * TA-independent NC_WITHDEFAULTS_MODE
 */
enum RPC_NC_WITHDEFAULTS_MODE {
    RPC_NC_WD_UNKNOWN,
    RPC_NC_WD_ALL,
    RPC_NC_WD_ALL_TAG,
    RPC_NC_WD_TRIM,
    RPC_NC_WD_EXPLICIT,
    RPC_NC_WD_INVAL,
};

/**
 * TA-independent NC_WD_MODE
 */
typedef enum RPC_NC_WITHDEFAULTS_MODE RPC_NC_WD_MODE;

/** Convert RPC_NC_WD_MODE to native NC_WD_MODE */
extern int nc_wd_mode_rpc2h(RPC_NC_WD_MODE mode);

/** Convert native NC_WD_MODE to RPC_NC_WD_MODE */
extern RPC_NC_WD_MODE nc_wd_mode_h2rpc(int mode);

/** Convert RPC_NC_WD_MODE to string */
extern const char *nc_wd_mode_rpc2str(RPC_NC_WD_MODE mode);

/**
 * TA-independent NC_DATASTORE_TYPE
 */
enum RPC_NC_DATASTORE_TYPE {
    RPC_NC_DATASTORE_ERROR,
    RPC_NC_DATASTORE_CONFIG,
    RPC_NC_DATASTORE_URL,
    RPC_NC_DATASTORE_RUNNING,
    RPC_NC_DATASTORE_STARTUP,
    RPC_NC_DATASTORE_CANDIDATE,
    RPC_NC_DATASTORE_INVAL,
};

/**
 * TA-independent RPC_NC_DATASTORE
 */
typedef enum RPC_NC_DATASTORE_TYPE RPC_NC_DATASTORE;

/** Convert RPC_NC_DATASTORE to native NC_DATASTORE */
extern int nc_datastore_rpc2h(RPC_NC_DATASTORE datastore);

/** Convert native NC_DATASTORE to RPC_NC_DATASTORE */
extern RPC_NC_DATASTORE nc_datastiore_h2rpc(int datastore);

/** Convert RPC_NC_DATASTORE to string */
extern const char *nc_datastore_rpc2str(RPC_NC_DATASTORE datastore);

/**
 * TA-independent NC_RPC_EDIT_DFLTOP
 */
typedef enum {
    RPC_NC_RPC_EDIT_DFLTOP_UNKNOWN,
    RPC_NC_RPC_EDIT_DFLTOP_MERGE,
    RPC_NC_RPC_EDIT_DFLTOP_REPLACE,
    RPC_NC_RPC_EDIT_DFLTOP_NONE,
    RPC_NC_RPC_EDIT_DFLTOP_INVAL,
} RPC_NC_RPC_EDIT_DFLTOP;

/** Convert RPC_NC_RPC_EDIT_DFLTOP to native NC_RPC_EDIT_DFLTOP */
extern int nc_rpc_edit_dfltop_rpc2h(RPC_NC_RPC_EDIT_DFLTOP dfltop);

/** Convert native NC_RPC_EDIT_DFLTOP to RPC_NC_RPC_EDIT_DFLTOP */
extern RPC_NC_RPC_EDIT_DFLTOP nc_rpc_edit_dfltop_h2rpc(int dfltop);

/** Convert RPC_NC_RPC_EDIT_DFLTOP to string */
extern const char *nc_rpc_edit_dfltop_rpc2str(RPC_NC_RPC_EDIT_DFLTOP dfltop);

/**
 * TA-independent NC_RPC_EDIT_TESTOPT
 */
typedef enum {
    RPC_NC_RPC_EDIT_TESTOPT_UNKNOWN,
    RPC_NC_RPC_EDIT_TESTOPT_TESTSET,
    RPC_NC_RPC_EDIT_TESTOPT_SET,
    RPC_NC_RPC_EDIT_TESTOPT_TEST,
    RPC_NC_RPC_EDIT_TESTOPT_INVAL,
} RPC_NC_RPC_EDIT_TESTOPT;

/** Convert RPC_NC_RPC_EDIT_TESTOPT to native NC_RPC_EDIT_TESTOPT */
extern int nc_rpc_edit_testopt_rpc2h(RPC_NC_RPC_EDIT_TESTOPT testopt);

/** Convert native NC_RPC_EDIT_TESTOPT to RPC_NC_RPC_EDIT_TESTOPT */
extern RPC_NC_RPC_EDIT_TESTOPT nc_rpc_edit_testopt_h2rpc(int testopt);

/** Convert RPC_NC_RPC_EDIT_TESTOPT to string */
extern const char *nc_rpc_edit_testopt_rpc2str(RPC_NC_RPC_EDIT_TESTOPT testopt);

/**
 * TA-independent NC_RPC_EDIT_ERROPT
 */
typedef enum {
    RPC_NC_RPC_EDIT_ERROPT_UNKNOWN,
    RPC_NC_RPC_EDIT_ERROPT_STOP,
    RPC_NC_RPC_EDIT_ERROPT_CONTINUE,
    RPC_NC_RPC_EDIT_ERROPT_ROLLBACK,
    RPC_NC_RPC_EDIT_ERROPT_INVAL,
} RPC_NC_RPC_EDIT_ERROPT;

/** Convert RPC_NC_RPC_EDIT_ERROPT to native NC_RPC_EDIT_ERROPT */
extern int nc_rpc_edit_erropt_rpc2h(RPC_NC_RPC_EDIT_ERROPT erropt);

/** Convert native NC_RPC_EDIT_ERROPT to RPC_NC_RPC_EDIT_ERROPT */
extern RPC_NC_RPC_EDIT_ERROPT nc_rpc_edit_erropt_h2rpc(int erropt);

/** Convert RPC_NC_RPC_EDIT_ERROPT to string */
extern const char *nc_rpc_edit_erropt_rpc2str(RPC_NC_RPC_EDIT_ERROPT erropt);

/**
 * TA-independent NC_MSG_TYPE
 */
typedef enum {
    RPC_NC_MSG_ERROR,
    RPC_NC_MSG_WOULDBLOCK,
    RPC_NC_MSG_NONE,
    RPC_NC_MSG_HELLO,
    RPC_NC_MSG_BAD_HELLO,
    RPC_NC_MSG_RPC,
    RPC_NC_MSG_REPLY,
    RPC_NC_MSG_REPLY_ERR_MSGID,
    RPC_NC_MSG_NOTIF,
    RPC_NC_MSG_INVAL,
} RPC_NC_MSG_TYPE;

/** Convert RPC_NC_MSG_TYPE to native NC_MSG_TYPE */
extern int nc_msg_type_rpc2h(RPC_NC_MSG_TYPE msg);

/** Convert native NC_MSG_TYPE to RPC_NC_MSG_TYPE */
extern RPC_NC_MSG_TYPE nc_msg_type_h2rpc(int msg);

/** Convert RPC_NC_MSG_TYPE to string */
extern const char *nc_msg_type_rpc2str(RPC_NC_MSG_TYPE msg);

/**
 * TA-independent nc_session structure
 */
struct rpc_nc_session;

/**
 * TA-independent nc_rpc structure
 */
struct rpc_nc_rpc;

#endif /* !__TE_RPC_NETCONF_H__ */
