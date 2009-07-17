/** @file
 * @brief Test Environment: Live mode specific routines.
 *
 * Interface for output control message events and regular messages 
 * into the screen.
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

#include "rgt_common.h"

#ifdef HAVE_TIME_H
#include <time.h>
#endif

#include "log_msg.h"
#include "live_mode.h"

#include "te_errno.h"

static int live_process_test_start(node_info_t *node, GQueue *verdicts);
static int live_process_test_end(node_info_t *node, GQueue *verdicts);
static int live_process_pkg_start(node_info_t *node, GQueue *verdicts);
static int live_process_pkg_end(node_info_t *node, GQueue *verdicts);
static int live_process_sess_start(node_info_t *node, GQueue *verdicts);
static int live_process_sess_end(node_info_t *node, GQueue *verdicts);
static int live_process_branch_start(node_info_t *node, GQueue *verdicts);
static int live_process_branch_end(node_info_t *node, GQueue *verdicts);
static int live_process_regular_msg(log_msg *msg);

static void rgt_expand_regular_log_msg(log_msg *msg);

void
live_mode_init(f_process_ctrl_log_msg ctrl_proc[CTRL_EVT_LAST][NT_LAST], 
               f_process_reg_log_msg  *reg_proc)
{
    ctrl_proc[CTRL_EVT_START][NT_SESSION] = live_process_sess_start;
    ctrl_proc[CTRL_EVT_END][NT_SESSION] = live_process_sess_end;
    ctrl_proc[CTRL_EVT_START][NT_TEST] = live_process_test_start;
    ctrl_proc[CTRL_EVT_END][NT_TEST] = live_process_test_end;
    ctrl_proc[CTRL_EVT_START][NT_PACKAGE] = live_process_pkg_start;
    ctrl_proc[CTRL_EVT_END][NT_PACKAGE] = live_process_pkg_end;
    ctrl_proc[CTRL_EVT_START][NT_BRANCH] = live_process_branch_start;
    ctrl_proc[CTRL_EVT_END][NT_BRANCH] = live_process_branch_end;

    *reg_proc = live_process_regular_msg;
}

static void
print_ts(uint32_t *ts)
{
#define TIME_BUF_LEN 40

    time_t     time_block;
    char       time_buf[TIME_BUF_LEN];
    struct tm  tm, *tm_tmp;
    size_t     res;

    time_block = ts[0];
    if ((tm_tmp = localtime(&time_block)) == NULL)
    {
        fprintf(stderr, "Incorrect timestamp specified\n");
        THROW_EXCEPTION;
    }
    memcpy(&tm, tm_tmp, sizeof(tm));
    
#if 0
    /* Long date/time format (date & time) */
    res = strftime(time_buf, TIME_BUF_LEN, "%b %d %T", &tm);
#else
    /* Short date/time format (only time) */
    res = strftime(time_buf, TIME_BUF_LEN, "%T", &tm);
#endif
    assert(res > 0);
    fprintf(rgt_ctx.out_fd, "%s %u ms", time_buf, ts[1] / 1000);

#undef TIME_BUF_LEN
}

static void
print_params(param *prms)
{
    param *prm = prms;
    
    if (prm != NULL)
    {
        fprintf(rgt_ctx.out_fd, "|- Parameters:\n");
    }
    
    while (prm != NULL)
    {
        fprintf(rgt_ctx.out_fd, "     + %s = %s\n", prm->name, prm->val);
        prm = prm->next;
    }
}

static inline int
live_process_start_event(node_info_t *node, const char *node_name,
                         GQueue *verdicts)
{
    UNUSED(verdicts);

    fprintf(rgt_ctx.out_fd, "| Starting %s: %s\n",
            node_name, node->descr.name);
    if (node->descr.tin != TE_TIN_INVALID)
        fprintf(rgt_ctx.out_fd, "|- TIN: %u\n", node->descr.tin);
    fprintf(rgt_ctx.out_fd, "|- Date: ");
    print_ts(node->start_ts);
    fprintf(rgt_ctx.out_fd, "\n");

    if (node->descr.objective != NULL)
        fprintf(rgt_ctx.out_fd, "|- Objective: %s\n",
                node->descr.objective);
    if (node->descr.authors)
        fprintf(rgt_ctx.out_fd, "|- Authors: %s\n", node->descr.authors);

    print_params(node->params);

    fprintf(rgt_ctx.out_fd, "\n");

    return 1;
}

static inline int
live_process_end_event(node_info_t *node, const char *node_name,
                       GQueue *verdicts)
{
    const char *result;
    
    UNUSED(verdicts);

    switch (node->result.status)
    {
#define NODE_RES_CASE(res_) \
        case RES_STATUS_ ## res_: \
            result = #res_;       \
            break

        NODE_RES_CASE(PASSED);
        NODE_RES_CASE(KILLED);
        NODE_RES_CASE(CORED);
        NODE_RES_CASE(SKIPPED);
        NODE_RES_CASE(FAKED);
        NODE_RES_CASE(FAILED);
        NODE_RES_CASE(EMPTY);

#undef NODE_RES_CASE
        default:
            assert(0);
    }

    fprintf(rgt_ctx.out_fd, "| %s complited %-55s %s\n", node_name,
            node->descr.name, result);
    fprintf(rgt_ctx.out_fd, "|- Date: ");
    print_ts(node->end_ts);
    fprintf(rgt_ctx.out_fd, "\n\n");

    return 1;
}

static int
live_process_test_start(node_info_t *node, GQueue *verdicts)
{
    return live_process_start_event(node, "test", verdicts);
}

static int
live_process_test_end(node_info_t *node, GQueue *verdicts)
{
    return live_process_end_event(node, "Test", verdicts);
}

static int
live_process_pkg_start(node_info_t *node, GQueue *verdicts)
{
    return live_process_start_event(node, "package", verdicts);
}

static int
live_process_pkg_end(node_info_t *node, GQueue *verdicts)
{
    return live_process_end_event(node, "Package", verdicts);
}

static int
live_process_sess_start(node_info_t *node, GQueue *verdicts)
{
    return live_process_start_event(node, "session", verdicts);
}

static int
live_process_sess_end(node_info_t *node, GQueue *verdicts)
{
    return live_process_end_event(node, "Session", verdicts);
}

static int
live_process_branch_end(node_info_t *node, GQueue *verdicts)
{
    UNUSED(node);
    UNUSED(verdicts);
    return 1;
}

static int
live_process_branch_start(node_info_t *node, GQueue *verdicts)
{
    UNUSED(node);
    UNUSED(verdicts);
    return 1;
}

static int
live_process_regular_msg(log_msg *msg)
{
    rgt_expand_log_msg(msg);

    fprintf(rgt_ctx.out_fd, "%s %s %s ",
            msg->level_str, msg->entity, msg->user);
    print_ts(msg->timestamp);
    fprintf(rgt_ctx.out_fd, "\n  %s\n\n", msg->txt_msg);

    return 1;
}

