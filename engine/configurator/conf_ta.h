/** @file
 * @brief Configurator
 *
 * TA interaction auxiliary routines
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 * @author Elena Vengerova <Elena.Vengerova@oktetlabs.ru>
 *
 */

#ifndef __TE_CONF_TA_H__
#define __TE_CONF_TA_H__

#include "te_vector.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Buffer with TAs list */
extern char *cfg_ta_list;
/** Buffer for GET requests */
extern char *cfg_get_buf;

/*
 * NOTES:
 *
 * 1. It is not allowed to perform any command after local SET/ADD/DEL
 * command until COMMIT is performed. All non-local commands shall fail with
 * EACCESS error code notifying that there is open local-commnd sequence.
 *
 * 2. It is not allowed to COMMIT only a part of local changes
 * in Configuration DB, instead user shall COMMIT all their changes in
 * one COMMIT. Incorrect COMMIT commands shall fail with EPERM error code.
 */

/**
 * Whether local commands sequence is terminated or not.
 * This variable is indended to solve the problem mentioned in note 1 above.
 */
extern te_bool local_cmd_seq;

/**
 * Maximum allowed subtree value for commit operation.
 * This variable is indended to solve the problem mentioned in note 2 above.
 */
extern char max_commit_subtree[CFG_INST_NAME_MAX];

/**
 * Backup file name which reflects situation before local SET/ADD/DEL
 * command.
 */
extern char *local_cmd_bkp;

/**
 * Reboot all Test Agents (before re-initializing of the Configurator).
 */
extern void cfg_ta_reboot_all(void);

/**
 * Synchronize object instances tree with Test Agents.
 *
 * @param oid           identifier of the object instance or subtree
 *                      or NULL if whole database should be synchronized
 * @param subtree       1 if the subtree of the specified node should
 *                      be synchronized
 *
 * @return status code (see te_errno.h)
 */
extern int cfg_ta_sync(char *oid, te_bool subtree);

/**
 * Synchronize all instances with given object with Test Agents
 *
 * @param obj       object or object tree
 * @param subtree   1 if subtree should be synchronized
 */
extern void cfg_ta_sync_obj(cfg_object *obj, te_bool subtree);

/**
 * Add instances for all agents.
 *
 * @return status code (see te_errno.h)
 */
extern int cfg_ta_add_agent_instances(void);

/**
 * Commit changes in local Configurator database to the Test Agents.
 *
 * @param oid   - subtree OID or NULL if whole database should be
 *                synchronized
 *
 * @return status code (see te_errno.h)
 */
extern int cfg_tas_commit(const char *oid);

/**
 * Synchronize dependant nodes.
 *
 * @param inst Instance to
 *
 * @return 0 on success, error code otherwise
 */
extern int cfg_ta_sync_dependants(cfg_instance *inst);

/**
 * Toggles logging of all sync operations
 *
 * @param flag Is logging enabled
 *
 */
extern void cfg_ta_log_syncing(te_bool flag);

/**
 * Perform check whether local commands sequence is started or not.
 * If started then set msg @a _cfg_msg rc to TE_EACCES and return from the
 * current function.
 *
 * @param _cmd      Command name
 * @param _cfg_msg  cfg_msg argument
 * @param _ret_exp  Return expression
 */
#define CFG_CHECK_NO_LOCAL_SEQ_EXP(_cmd, _cfg_msg, _ret_exp) \
if (local_cmd_seq) \
{ \
    (_cfg_msg)->rc = TE_EACCES; \
    VERB("Non local %s command while local command " \
         "sequence is active %r", _cmd, (_cfg_msg)->rc); \
    _ret_exp \
}

/**
 * The CFG_CHECK_NO_LOCAL_SEQ_EXP wrapper, return status code
 */
#define CFG_CHECK_NO_LOCAL_SEQ_RC(_cmd, _cfg_msg) \
    CFG_CHECK_NO_LOCAL_SEQ_EXP(_cmd, _cfg_msg, {return (_cfg_msg)->rc;})

/**
 * The CFG_CHECK_NO_LOCAL_SEQ_EXP wrapper, perform break
 */
#define CFG_CHECK_NO_LOCAL_SEQ_BREAK(_cmd, _cfg_msg) \
    CFG_CHECK_NO_LOCAL_SEQ_EXP(_cmd, _cfg_msg, {break;})

/**
 * Reboot the test agents specified in the vector
 *
 * @param agents Vector of the agents
 *
 * @return Status code
 */
extern te_errno conf_ta_reboot_agents(const te_vec *agents);

#ifdef __cplusplus
}
#endif
#endif /* __TE_CONF_TA_H__ */
