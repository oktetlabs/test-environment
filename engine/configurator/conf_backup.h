/** @file
 * @brief Configurator
 *
 * Prototypes of functions related to backups processing
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

#ifndef __TE_CONF_BACKUP_H__
#define __TE_CONF_BACKUP_H__
#ifdef __cplusplus
extern "C" {
#endif

/**
 * Process "backup" configuration file or backup file.
 *
 * @param node    <backup> node pointer
 * @param restore if TRUE, the configuration should be restored after
 *                unsuccessful dynamic history restoring
 *
 * @return status code (errno.h)
 */
extern int cfg_backup_process_file(xmlNodePtr node, te_bool restore);

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
 *
 * @return status code (errno.h)
 */
extern int cfg_backup_create_file(const char *filename);
 
#ifdef __cplusplus
}
#endif
#endif /* __TE_CONF_BACKUP_H__ */

