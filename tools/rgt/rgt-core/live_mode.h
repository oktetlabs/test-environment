/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Test Environment: Live mode specific routines.
 *
 * Interface for output control message events and regular messages
 * into the screen.
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_RGT_LIVE_MODE_H__
#define __TE_RGT_LIVE_MODE_H__

#ifdef __cplusplus
extern "C" {
#endif

extern void live_mode_init(f_process_ctrl_log_msg
                                  ctrl_proc[CTRL_EVT_LAST][NT_LAST],
                           f_process_reg_log_msg *reg_proc,
                           f_process_log_root root_proc[CTRL_EVT_LAST]);

#ifdef __cplusplus
}
#endif

#endif /* __TE_RGT_LIVE_MODE_H__ */

