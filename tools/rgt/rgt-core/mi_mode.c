/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Test Environment: MI mode specific routines.
 *
 * In MI mode all MI messages from raw log are extracted.
 *
 * Copyright (C) 2025 OKTET Labs Ltd. All rights reserved.
 */

#include "te_config.h"

#if HAVE_STDIO_H
#include <stdio.h>
#endif

#include "rgt_common.h"
#include "log_msg.h"
#include "mi_mode.h"

/** True until the first message is processed. */
static bool first_msg = true;
/** Timestamp of the last processed message. */
uint32_t last_ts[2] = {0, 0};

/* Process log message. */
static int
mi_process_regular_msg(log_msg *log)
{
    if (rgt_ctx.mi_ts)
    {
        if (first_msg)
        {
            /*
             * Insert special MI message so that timestamp of the very
             * first message is saved.
             */
            fprintf(rgt_ctx.out_fd, "%u.%06u\n",
                    log->timestamp[0], log->timestamp[1]);
            fprintf(rgt_ctx.out_fd,
                    "{\"type\": \"log_start\"}\n");
        }

        if (log->timestamp[0] > last_ts[0] ||
            (log->timestamp[0] == last_ts[0] &&
             log->timestamp[1] > last_ts[1]))
        {
            last_ts[0] = log->timestamp[0];
            last_ts[1] = log->timestamp[1];
        }
    }

    if (log->level & TE_LL_MI)
    {
        rgt_expand_log_msg(log);
        if (log->txt_msg == NULL)
            return 0;

        if (rgt_ctx.mi_ts)
        {
            fprintf(rgt_ctx.out_fd, "%u.%06u\n",
                    log->timestamp[0], log->timestamp[1]);
        }

        fprintf(rgt_ctx.out_fd, "%s\n", log->txt_msg);
    }

    first_msg = false;
    return 0;
}

/* Called after the last message was processed. */
static int
mi_process_close(void)
{
    if (rgt_ctx.mi_ts && !first_msg)
    {
        /*
         * Insert special MI message so that timestamp of the very
         * last message is saved.
         */
        fprintf(rgt_ctx.out_fd, "%u.%06u\n",
                last_ts[0], last_ts[1]);
        fprintf(rgt_ctx.out_fd,
                "{\"type\": \"log_end\"}\n");
    }

    return 0;
}

/* See the description in mi_mode.h */
void
mi_mode_init(f_process_ctrl_log_msg
                        ctrl_proc[CTRL_EVT_LAST][NT_LAST],
             f_process_reg_log_msg *reg_proc,
             f_process_log_root root_proc[CTRL_EVT_LAST])
{
    *reg_proc = mi_process_regular_msg;
    root_proc[CTRL_EVT_END] = mi_process_close;
}
