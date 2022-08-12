/** @file
 * @brief Configurator
 *
 * Configurator dynamic history definitions
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
 *
 *
 * @author Elena Vengerova <Elena.Vengerova@oktetlabs.ru>
 *
 */

#ifndef __TE_CONF_DH_H__
#define __TE_CONF_DH_H__

#include "te_vector.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Process "history" configuration file - execute all commands
 * and add them to dynamic history.
 * Note: this routine does not reboot Test Agents.
 *
 * @param node          <history> node pointer
 * @param expand_vars   List of key-value pairs for expansion in file,
 *                      @c NULL if environment variables are used for
 *                      substitutions
 * @param postsync      is processing performed after sync with TA
 *
 * @return status code (errno.h)
 */
extern int cfg_dh_process_file(xmlNodePtr node, te_kvpair_h *expand_vars,
                               te_bool postsync);

/**
 * Create "history" configuration file with specified name.
 *
 * @param filename      name of the file to be created
 *
 * @return status code (errno.h)
 */
extern int cfg_dh_create_file(char *filename);

/**
 * Attach backup to the last command.
 *
 * @param filename      name of the backup file
 *
 * @return status code (see te_errno.h)
 */
extern int cfg_dh_attach_backup(char *filename);

/**
 * Restore backup with specified name using reversed command
 * of the dynamic history. Processed commands are removed
 * from the history.
 *
 * @param filename      name of the backup file
 * @param hard_check    whether hard check should be applied
 *                      on restore backup. For instance if on deleting
 *                      some instance we got ESRCH or ENOENT, we should
 *                      kepp processing without any error.
 *
 * @return status code (see te_errno.h)
 * @retval TE_ENOENT       there is not command in dynamic history to which
 *                      the specified backup is attached
 */
extern int cfg_dh_restore_backup(char *filename, te_bool hard_check);

/**
 * Restore backup when the configurator shuts down reversing the dynamic
 * history. Processed commands are removed from the history.
 *
 * @return status code (see te_errno.h)
 */
extern int cfg_dh_restore_backup_on_shutdown();

/**
 * Push a command to the history.
 *
 * @param msg     message with set, add or delete user request.
 * @param local   whether this command is local or not.
 * @param old_val The old value of the instance. It is used for @c CFG_SET and
 *                @c CFG_DEL command.
 *
 * @return 0 (success) or TE_ENOMEM
 */
extern int cfg_dh_push_command(cfg_msg *msg, te_bool local,
                               const cfg_inst_val *old_val);

/**
 * Notify history DB about successfull commit operation.
 * The result of calling of this function is that some entries in DH DB
 * enter committed state.
 *
 * @param oid  OID of the instance that was successfully committed
 *
 * @return status code (errno.h)
 */
extern int cfg_dh_apply_commit(const char *oid);

/**
 * Delete last command from the history.
 */
extern void cfg_dh_delete_last_command(void);

/**
 * Destroy dynamic hostory before shut down.
 */
extern void cfg_dh_destroy(void);

/**
 * Remove useless command sequences.
 */
extern void cfg_dh_optimize(void);

/**
 * Release history after backup.
 *
 * @param filename      name of the backup file
 */
extern void cfg_dh_release_after(char *filename);

/**
 * Forget about this backup.
 *
 * @param filename      name of the backup file
 *
 * @return status code
 */
extern int cfg_dh_release_backup(char *filename);

/**
 * Restore TA configuration using the direct order of the DH commands.
 * If the command from DH has a backup file, this file will be
 * compared with current state and TA will be restore using the attached
 * file. Processed commands are not removed from the history.
 *
 * @param ta Test agent name
 *
 * @return Status code
 */
extern te_errno cfg_dh_restore_agents(const te_vec *ta_list);

#ifdef __cplusplus
}
#endif
#endif /* __TE_CONF_DH_H__ */

