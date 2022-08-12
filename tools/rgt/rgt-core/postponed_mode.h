/** @file
 * @brief Test Environment: Postponed mode specific routines.
 *
 * Interface for output control message events and regular messages
 * into the XML file.
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 * @author Oleg N. Kravtsov  <Oleg.Kravtsov@oktetlabs.ru>
 *
 */


#ifndef __TE_RGT_POSTPONED_MODE_H__
#define __TE_RGT_POSTPONED_MODE_H__

#ifdef __cplusplus
extern "C" {
#endif

extern void postponed_mode_init(f_process_ctrl_log_msg
                                    ctrl_proc[CTRL_EVT_LAST][NT_LAST],
                                f_process_reg_log_msg *reg_proc,
                                f_process_log_root
                                    root_proc[CTRL_EVT_LAST]);

#ifdef __cplusplus
}
#endif

#endif /* __TE_RGT_POSTPONED_MODE_H__ */

