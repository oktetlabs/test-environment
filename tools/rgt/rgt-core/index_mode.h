/** @file
 * @brief Test Environment: Index mode specific routines.
 *
 * Interface for creating an index for raw log with information about
 * which parts of raw log represent which test iterations, packages and
 * sessions.
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_RGT_INDEX_MODE_H__
#define __TE_RGT_INDEX_MODE_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Set callback pointers to refer functions implementing index mode of
 * operation.
 *
 * @param ctrl_proc       Table of callbacks for processing control
 *                        log messages
 * @param reg_proc        Callback for processing regular message
 * @param root_proc       Callbacks for processing log start and end
 */
extern void index_mode_init(f_process_ctrl_log_msg
                                ctrl_proc[CTRL_EVT_LAST][NT_LAST],
                            f_process_reg_log_msg  *reg_proc,
                            f_process_log_root root_proc[CTRL_EVT_LAST]);

#ifdef __cplusplus
}
#endif

#endif /* __TE_RGT_INDEX_MODE_H__ */

