/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Test Environment: MI mode specific routines.
 *
 * Interface for extracting all MI messages from raw log.
 *
 * Copyright (C) 2025 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_RGT_MI_MODE_H__
#define __TE_RGT_MI_MODE_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Set callback pointers to refer functions implementing MI mode of
 * operation.
 *
 * @param ctrl_proc       Table of callbacks for processing control
 *                        log messages.
 * @param reg_proc        Callback for processing regular message.
 * @param root_proc       Callbacks for processing log start and end.
 */
extern void mi_mode_init(f_process_ctrl_log_msg
                                    ctrl_proc[CTRL_EVT_LAST][NT_LAST],
                         f_process_reg_log_msg  *reg_proc,
                         f_process_log_root root_proc[CTRL_EVT_LAST]);

#ifdef __cplusplus
}
#endif

#endif /* __TE_RGT_MI_MODE_H__ */
