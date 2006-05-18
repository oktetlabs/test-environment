/** @file
 * @brief Configurator
 *
 * TA interaction auxiliary routines
 *
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 *
 * @author Elena Vengerova <Elena.Vengerova@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_CONF_TA_H__
#define __TE_CONF_TA_H__

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
 * 1. It is not allowed to perform any command after local SET/ADD command
 * until COMMIT is performed. All non-local commands shall fail with
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
 * Backup file name which reflects situation before local SET/ADD command 
 */
extern char local_cmd_bkp[1024];


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

#ifdef __cplusplus
}
#endif
#endif /* __TE_CONF_TA_H__ */
