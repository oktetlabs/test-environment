/** @file
 * @brief Test Environment: Index mode specific routines.
 *
 * Interface for creating an index for raw log with information about
 * which parts of raw log represent which test iterations, packages and
 * sessions.
 *
 * Copyright (C) 2016 Test Environment authors (see file AUTHORS in the
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
 * @author Dmitry Izbitsky  <Dmitry.Izbitsky@oktetlabs.ru>
 *
 * $Id$
 */

#include "rgt_common.h"

#ifdef HAVE_TIME_H
#include <time.h>
#endif

#include "log_msg.h"
#include "index_mode.h"

#include "te_errno.h"

static int index_process_test_start(node_info_t *node, GQueue *verdicts);
static int index_process_test_end(node_info_t *node, GQueue *verdicts);
static int index_process_pkg_start(node_info_t *node, GQueue *verdicts);
static int index_process_pkg_end(node_info_t *node, GQueue *verdicts);
static int index_process_sess_start(node_info_t *node, GQueue *verdicts);
static int index_process_sess_end(node_info_t *node, GQueue *verdicts);
static int index_process_branch_start(node_info_t *node, GQueue *verdicts);
static int index_process_branch_end(node_info_t *node, GQueue *verdicts);
static int index_process_regular_msg(log_msg *msg);

/** Offset of the previous message in raw log */
static off_t   prev_rawlog_fpos;
/** Is the first message in raw log being processed? */
static te_bool first_message;

/* See the description in index_mode.h */
void
index_mode_init(f_process_ctrl_log_msg ctrl_proc[CTRL_EVT_LAST][NT_LAST],
               f_process_reg_log_msg  *reg_proc)
{
    ctrl_proc[CTRL_EVT_START][NT_SESSION] = index_process_sess_start;
    ctrl_proc[CTRL_EVT_END][NT_SESSION] = index_process_sess_end;
    ctrl_proc[CTRL_EVT_START][NT_TEST] = index_process_test_start;
    ctrl_proc[CTRL_EVT_END][NT_TEST] = index_process_test_end;
    ctrl_proc[CTRL_EVT_START][NT_PACKAGE] = index_process_pkg_start;
    ctrl_proc[CTRL_EVT_END][NT_PACKAGE] = index_process_pkg_end;
    ctrl_proc[CTRL_EVT_START][NT_BRANCH] = index_process_branch_start;
    ctrl_proc[CTRL_EVT_END][NT_BRANCH] = index_process_branch_end;

    *reg_proc = index_process_regular_msg;
    prev_rawlog_fpos = 0;
    first_message = TRUE;
}

/** Print the length of previous message, finishing its description  */
static void
print_prev_length()
{
    if (first_message)
    {
        fprintf(rgt_ctx.out_fd, "0.0 0 0 0 FIRST %u",
                TE_TIN_INVALID);
        first_message = FALSE;
    }
    if (prev_rawlog_fpos >= 0)
        fprintf(rgt_ctx.out_fd, " %lld\n",
                (long long int)(rgt_ctx.rawlog_fpos - prev_rawlog_fpos));
    prev_rawlog_fpos = rgt_ctx.rawlog_fpos;
}

/** Process control message starting a new log node */
static int
print_node_start(node_info_t *node)
{
    print_prev_length();
    fprintf(rgt_ctx.out_fd, "%u.%.6u %lld %d %d START %u",
            node->start_ts[0], node->start_ts[1],
            (long long int)rgt_ctx.rawlog_fpos,
            node->parent_id, node->node_id,
            node->descr.tin);

    return 1;
}

/** Process control message for log node termination */
static int
print_node_end(node_info_t *node)
{
    print_prev_length();
    fprintf(rgt_ctx.out_fd, "%u.%.6u %lld %d %d END -1",
            node->end_ts[0], node->end_ts[1],
            (long long int)rgt_ctx.rawlog_fpos,
            node->parent_id, node->node_id);

    return 1;
}

/** Process "test started" control message */
static int
index_process_test_start(node_info_t *node, GQueue *verdicts)
{
    UNUSED(verdicts);

    return print_node_start(node);
}

/** Process "test finished" control message */
static int
index_process_test_end(node_info_t *node, GQueue *verdicts)
{
    UNUSED(verdicts);

    return print_node_end(node);
}

/** Process "package started" control message */
static int
index_process_pkg_start(node_info_t *node, GQueue *verdicts)
{
    UNUSED(verdicts);

    return print_node_start(node);
}

/** Process "package finished" control message */
static int
index_process_pkg_end(node_info_t *node, GQueue *verdicts)
{
    UNUSED(verdicts);

    return print_node_end(node);
}

/** Process "session started" control message */
static int
index_process_sess_start(node_info_t *node, GQueue *verdicts)
{
    UNUSED(verdicts);

    return print_node_start(node);
}

/** Process "session finished" control message */
static int
index_process_sess_end(node_info_t *node, GQueue *verdicts)
{
    UNUSED(verdicts);

    return print_node_end(node);
}

/** Process "branch start" event */
static int
index_process_branch_start(node_info_t *node, GQueue *verdicts)
{
    UNUSED(node);
    UNUSED(verdicts);

    /* Do nothing - this is "generation event", not raw log message */
    return 0;
}

/** Process "branch end" event */
static int
index_process_branch_end(node_info_t *node, GQueue *verdicts)
{
    UNUSED(node);
    UNUSED(verdicts);

    /* Do nothing - this is "generation event", not raw log message */
    return 0;
}

/** Process regular raw log message */
static int
index_process_regular_msg(log_msg *msg)
{
    UNUSED(msg);

    print_prev_length();
    fprintf(rgt_ctx.out_fd, "%u.%.6u %lld %u -1 REGULAR %u",
            msg->timestamp[0], msg->timestamp[1],
            (long long int)rgt_ctx.rawlog_fpos, msg->id,
            ((msg->flags & RGT_MSG_FLG_VERDICT) ? 1 : 0));

    return 1;
}
