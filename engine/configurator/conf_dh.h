/** @file
 * @brief Configurator
 *
 * Configurator dynamic history definitions
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

#ifndef __TE_CONF_DH_H__
#define __TE_CONF_DH_H__
#ifdef __cplusplus
extern "C" {
#endif

/**
 * Process "history" configuration file - execute all commands
 * and add them to dynamic history. 
 * Note: this routine does not reboot Test Agents.
 *
 * @param node  <history> node pointer
 *
 * @return status code (errno.h)
 */
int cfg_dh_process_file(xmlNodePtr node);

/**
 * Create "history" configuration file with specified name.
 *
 * @param filename      name of the file to be created
 *
 * @return status code (errno.h)
 */
int cfg_dh_create_file(char *filename);

/**
 * Attach backup to the last command.
 * 
 * @param filename      name of the backup file
 *
 * @return status code (see te_errno.h)
 */
int cfg_dh_attach_backup(char *filename);

/**
 * Restore backup with specified name using reversed command
 * of the dynamic history. Processed commands are removed
 * from the history.
 * 
 * @param filename      name of the backup file
 *
 * @return status code (see te_errno.h)
 * @retval ENOENT       there is not command in dynamic history to which
 *                      the specified backup is attached
 */
int cfg_dh_restore_backup(char *filename);

/**
 * Add a command to the history.
 *
 * @param msg   message with set, add or delete user request.
 *
 * @return 0 (success) or ENOMEM
 */
int cfg_dh_add_command(cfg_msg *msg);

/**
 * Delete last command from the history.
 */
void cfg_dh_delete_last_command(void);

/**
 * Destroy dynamic hostory before shut down.
 */
void cfg_dh_destroy(void);

/**
 * Remove useless command sequences.
 */
void cfg_dh_optimize(void);

#ifdef __cplusplus
}
#endif
#endif /* __TE_CONF_DH_H__ */

