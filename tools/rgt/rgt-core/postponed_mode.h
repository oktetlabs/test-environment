/** @file 
 * @brief Test Environment: Postponed mode specific routines.
 *
 * Interface for output control message events and regular messages 
 * into the XML file.
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
 * @author Oleg N. Kravtsov  <Oleg.Kravtsov@oktetlabs.ru>
 *
 * $Id$
 */


#ifndef __RGT_POSTPONED_MODE_H__
#define __RGT_POSTPONED_MODE_H__

#ifdef __cplusplus
extern "C" {
#endif

void
postponed_mode_init(f_process_ctrl_log_msg ctrl_proc[CTRL_EVT_LAST][NT_LAST], 
                    f_process_reg_log_msg  *reg_proc);

int postponed_process_open();
int postponed_process_close();

#ifdef __cplusplus
}
#endif

#endif /* __RGT_POSTPONED_MODE_H__ */

