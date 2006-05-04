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

#ifndef __TE_ISCSI_TARGET_API_H__
#define __TE_ISCSI_TARGET_API_H__

#include <te_config.h>
#include <te_defs.h>
#include <inttypes.h>

#define ISCSI_TARGET_CONTROL_SOCKET "/tmp/te_iscsi_target_control"
#define ISCSI_TARGET_DATA_SOCKET "/tmp/te_iscsi_target_data"

#define ISCSI_TARGET_SHARED_MEMORY (4 * 1024 * 1024)

extern int iscsi_server_init(void);
extern te_errno iscsi_target_send_msg(te_errno (*process)(char *buf, int size, void *data),
                                      void *data,
                                      const char *msg, const char *fmt, ...);
extern te_errno iscsi_target_send_simple_msg(const char *msg, const char *arg);
extern int iscsi_target_connect(void);
extern te_bool iscsi_server_check(void);

#endif
