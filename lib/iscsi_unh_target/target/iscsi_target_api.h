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


extern int iscsi_free_device(uint8_t target, uint8_t lun);
extern int iscsi_mmap_device(uint8_t target, uint8_t lun, 
                             const char *fname);

extern int iscsi_get_device_param(uint8_t target, uint8_t lun,
                                  te_bool *is_mmap,
                                  uint32_t *storage_size);
extern int iscsi_sync_device(uint8_t target, uint8_t lun);

extern int iscsi_write_to_device(uint8_t target, uint8_t lun,
                                 uint32_t offset,
                                 void *buffer, uint32_t len);

extern int iscsi_verify_device_data(uint8_t target, uint8_t lun,
                                    uint32_t offset,
                                    void *buffer, uint32_t len);

#endif
