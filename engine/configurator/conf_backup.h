/** @file
 * @brief Configurator
 *
 * Prototypes of functions related to backups processing
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
 *
 *
 * @author Elena Vengerova <Elena.Vengerova@oktetlabs.ru>
 *
 */

#ifndef __TE_CONF_BACKUP_H__
#define __TE_CONF_BACKUP_H__

#include "te_vector.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Process "backup" configuration file or backup file.
 *
 * @param node     <backup> node pointer
 * @param restore  if TRUE, the configuration should be restored after
 *                 unsuccessful dynamic history restoring
 * @param subtrees Vector of the subtrees to restore. May be @c NULL for
 *                 the root.
 *
 * @return status code (errno.h)
 */
extern int cfg_backup_process_file(xmlNodePtr node, te_bool restore,
                                   const te_vec *subtrees);

/**
 * Save current version of the TA subtree,
 * synchronize DB with TA and restore TA configuration.
 *
 * @param ta    TA name
 *
 * @return status code (see te_errno.h)
 */
extern int cfg_backup_restore_ta(char *ta);

/**
 * Create "backup" configuration file with specified name.
 *
 * @param filename   name of the file to be created
 * @param subtrees   Vector of the subtrees to create a backup file.
 *                   @c NULL to create backup fo all the subtrees
 *
 * @return status code (errno.h)
 */
extern int cfg_backup_create_file(const char *filename,
                                  const te_vec *subtrees);

/**
 * Create file XML file with subtrees to filter backup file
 *
 * @param filename Name of the filter file
 * @param subtrees Vector of the subtrees
 *
 * @return Status code
 */
extern te_errno cfg_backup_create_filter_file(const char *filename,
                                              const te_vec *subtrees);

/**
 * Verify backup configuration file
 *
 * @param filename Name of the backup file
 * @param subtree  Vector of subtrees
 *
 * @return Status code
 */
extern te_errno cfg_backup_verify(const char *filename,
                                  const te_vec *subtrees);

/**
 * Restore backup configuration file
 *
 * @param filename Name of the backup file
 * @param subtree  Vector of subtrees
 *
 * @return Status code
 */
extern te_errno cfg_backup_restore_nohistory(const char *filename,
                                             const te_vec *subtrees);

/**
 * Verify backup and try to restore by backup
 *
 * @param filename Name of the backup file
 * @param subtree  Vector of subtrees
 *
 * @return Status code
 */
extern te_errno cfg_backup_verify_and_restore(const char *filename,
                                              const te_vec *subtrees);

/**
 * Verify backup and try to restore by backup by specified agents
 *
 * @param filename Name of the backup file
 * @param ta_list  Vector of the test agents
 *
 * @return Status code
 */
extern te_errno cfg_backup_verify_and_restore_ta_subtrees(
                                                const char *filename,
                                                const te_vec *ta_list);

#ifdef __cplusplus
}
#endif
#endif /* __TE_CONF_BACKUP_H__ */

