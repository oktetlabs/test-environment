/** @file
 * @brief Test Environment: 
 *
 * UNH iSCSI Target port
 * 
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
 * @author Konstantin Abramenko <Konstantin.Abramenko@oktetlabs.ru>
 *
 * $Id: tad_iscsi_impl.h 19205 2005-10-10 16:02:41Z artem $
 */

#ifndef __TE_TAD_ISCSI_CUSTOM_H__
#define __TE_TAD_ISCSI_CUSTOM_H__

typedef struct iscsi_custom_data iscsi_custom_data;

iscsi_custom_data *iscsi_register_custom(int id);
void iscsi_deregister_custom(iscsi_custom_data *block);

int iscsi_set_custom_value(int id, const char *param, const char *value);

int iscsi_get_custom_value(iscsi_custom_data *block, const char *param);
te_bool iscsi_is_changed_custom_value(iscsi_custom_data *block, const char *param);

int iscsi_custom_wait_change(iscsi_custom_data *block);

#endif /* __TE_TAD_ISCSI_CUSTOM_H__ */

