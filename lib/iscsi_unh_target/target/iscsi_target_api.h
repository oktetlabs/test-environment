/** @file
 * @brief iSCSI Target
 *
 * iSCSI Target external API
 *
 * Copyright (C) 2005 Test Environment authors (see file AUTHORS in
 * the root directory of the distribution).
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
 * @author Konstantin Abramenko <Konstantin.Abramenko@oktetlabs.ru>
 *
 * $Id: tad_iscsi_impl.h 20278 2005-10-29 10:46:56Z arybchik $
 */

#ifndef __TE_ISCSI_TARGET_H__
#define __TE_ISCSI_TARGET_H__

#include <te_config.h>
#include <te_defs.h>
#include <inttypes.h>

/**
 * Frees all resources (memory mappings and buffers)
 * associated with a given virtual iSCSI target device
 * 
 * @param target        Target No
 * @param lun           LUN
 * 
 * @return Status code
 */
extern int iscsi_free_device(uint8_t target, uint8_t lun);


/**
 * Causes a specified target device to use a named file
 * as a backend instead of a memory buffer
 * 
 * @param target        Target No
 * @param lun           LUN
 * @param fname         Name of a backend file
 * 
 * @return Status code
 */
extern int iscsi_mmap_device(uint8_t target, uint8_t lun, 
                             const char *fname);

/**
 * Gets information about the target device's backend parameters
 * 
 * @param target        Target No
 * @param lun           LUN
 * @param is_mmap       Set to TRUE if the device uses a file backend (OUT)
 * @param storage_size  The size of the device storage in bytes (OUT)
 * 
 * @return Status code
 */
extern int iscsi_get_device_param(uint8_t target, uint8_t lun,
                                  te_bool *is_mmap,
                                  uint32_t *storage_size);

/**
 * Causes the target device to sync its file backend to disk.
 * No-op for a memory backend.
 * 
 * @param target        Target No
 * @param lun           LUN
 * 
 * @return Status code
 */
extern int iscsi_sync_device(uint8_t target, uint8_t lun);

/**
 * Writes a specified amount of data to the target virtual device
 * at a given position.
 * 
 * @param target        Target No
 * @param lun           LUN
 * @param offset        Position to write at
 * @param buffer        Data to write
 * @param len           Length of data
 * 
 * @return Status code
 */
extern int iscsi_write_to_device(uint8_t target, uint8_t lun,
                                 uint32_t offset,
                                 void *buffer, uint32_t len);

/**
 * Verifies that the target virtual device contains specified data
 * at a given position.
 * 
 * @param target        Target No
 * @param lun           LUN
 * @param offset        Position to read from
 * @param buffer        Data to verify
 * @param len           Length of data
 * 
 * @return Status code
 */
extern int iscsi_verify_device_data(uint8_t target, uint8_t lun,
                                    uint32_t offset,
                                    void *buffer, uint32_t len);

/**
 * Informs the target that the following sessions will be 
 * somewhat different from the previous ones.
 * In practice, that means informing the target that a new test
 * is being run.
 * 
 */
extern void iscsi_start_new_session_group(void);

#endif
