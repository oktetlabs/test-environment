/** @file
 * @brief Test Environment: Index mode specific routines.
 *
 * Interface for creating an index for raw log with information about
 * which parts of raw log represent which test iterations, packages and
 * sessions.
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 *
 */

#include "rgt_common.h"

#ifdef HAVE_TIME_H
#include <time.h>
#endif

#include "log_msg.h"
#include "index_mode.h"

#include "te_errno.h"

static int index_process_test_start(node_info_t *node, ctrl_msg_data *data);
static int index_process_test_end(node_info_t *node, ctrl_msg_data *data);
static int index_process_pkg_start(node_info_t *node, ctrl_msg_data *data);
static int index_process_pkg_end(node_info_t *node, ctrl_msg_data *data);
static int index_process_sess_start(node_info_t *node, ctrl_msg_data *data);
static int index_process_sess_end(node_info_t *node, ctrl_msg_data *data);
static int index_process_branch_start(node_info_t *node,
                                      ctrl_msg_data *data);
static int index_process_branch_end(node_info_t *node, ctrl_msg_data *data);
static int index_process_regular_msg(log_msg *msg);

/** Offset of the previous message in raw log */
static off_t   prev_rawlog_fpos;
/** Is the first message in raw log being processed? */
static te_bool first_message;

/* See the description in index_mode.h */
void
index_mode_init(f_process_ctrl_log_msg ctrl_proc[CTRL_EVT_LAST][NT_LAST],
                f_process_reg_log_msg  *reg_proc,
                f_process_log_root root_proc[CTRL_EVT_LAST])
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

    root_proc[CTRL_EVT_START] = NULL;
    root_proc[CTRL_EVT_END] = NULL;

    prev_rawlog_fpos = 0;
    first_message = TRUE;
}

/** Print the length of previous message, finishing its description  */
static void
print_prev_length(void)
{
    if (first_message)
    {
        fprintf(rgt_ctx.out_fd, "0.0 0 0 0 FIRST %u ROOT",
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
    fprintf(rgt_ctx.out_fd, "%u.%.6u %lld %d %d START %u %s",
            node->start_ts[0], node->start_ts[1],
            (long long int)rgt_ctx.rawlog_fpos,
            node->parent_id, node->node_id,
            node->descr.tin, node_type2str(node->type));

    return 1;
}

/** Process control message for log node termination */
static int
print_node_end(node_info_t *node)
{
    print_prev_length();
    fprintf(rgt_ctx.out_fd, "%u.%.6u %lld %d %d END -1 %s",
            node->end_ts[0], node->end_ts[1],
            (long long int)rgt_ctx.rawlog_fpos,
            node->parent_id, node->node_id,
            node_type2str(node->type));

    return 1;
}

/** Process "test started" control message */
static int
index_process_test_start(node_info_t *node, ctrl_msg_data *data)
{
    UNUSED(data);

    return print_node_start(node);
}

/** Process "test finished" control message */
static int
index_process_test_end(node_info_t *node, ctrl_msg_data *data)
{
    UNUSED(data);

    return print_node_end(node);
}

/** Process "package started" control message */
static int
index_process_pkg_start(node_info_t *node, ctrl_msg_data *data)
{
    UNUSED(data);

    return print_node_start(node);
}

/** Process "package finished" control message */
static int
index_process_pkg_end(node_info_t *node, ctrl_msg_data *data)
{
    UNUSED(data);

    return print_node_end(node);
}

/** Process "session started" control message */
static int
index_process_sess_start(node_info_t *node, ctrl_msg_data *data)
{
    UNUSED(data);

    return print_node_start(node);
}

/** Process "session finished" control message */
static int
index_process_sess_end(node_info_t *node, ctrl_msg_data *data)
{
    UNUSED(data);

    return print_node_end(node);
}

/** Process "branch start" event */
static int
index_process_branch_start(node_info_t *node, ctrl_msg_data *data)
{
    UNUSED(node);
    UNUSED(data);

    /* Do nothing - this is "generation event", not raw log message */
    return 0;
}

/** Process "branch end" event */
static int
index_process_branch_end(node_info_t *node, ctrl_msg_data *data)
{
    UNUSED(node);
    UNUSED(data);

    /* Do nothing - this is "generation event", not raw log message */
    return 0;
}

/** Process regular raw log message */
static int
index_process_regular_msg(log_msg *msg)
{
    unsigned int to_start_frag = 0;

    UNUSED(msg);

    if ((msg->flags & (RGT_MSG_FLG_VERDICT | RGT_MSG_FLG_ARTIFACT)) ||
        strcmp(msg->user, "TRC tags") == 0)
        to_start_frag = 1;

    print_prev_length();
    fprintf(rgt_ctx.out_fd, "%u.%.6u %lld %u -1 REGULAR %u UNDEF",
            msg->timestamp[0], msg->timestamp[1],
            (long long int)rgt_ctx.rawlog_fpos, msg->id,
            to_start_frag);

    return 1;
}
