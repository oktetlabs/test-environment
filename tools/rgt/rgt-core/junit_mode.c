/** @file
 * @brief Test Environment: JUnit mode specific routines.
 *
 * Implementation of output of control and regular messages
 * into the JUnit XML file.
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 */

#include "te_config.h"

#if HAVE_STDIO_H
#include <stdio.h>
#endif

#if HAVE_TIME_H
#include <time.h>
#endif

#if HAVE_CTYPE_H
#include <ctype.h>
#endif

#include "rgt_common.h"
#include "log_msg.h"
#include "junit_mode.h"
#include "memory.h"

#include "te_errno.h"
#include "tq_string.h"

/**
 * Compute time difference in seconds.
 *
 * @param ts_end_     End timestamp.
 * @param ts_start    Start timestamp.
 */
#define RGT_TIME_DIFF(ts_end_, ts_start_) \
    (((long int)ts_end_[0] - (long int)ts_start_[0]) + \
     ((long int)ts_end_[1] - (long int)ts_start_[1]) / 1000000.0)

static int junit_process_pkg_start(node_info_t *node,
                                   ctrl_msg_data *data);
static int junit_process_pkg_end(node_info_t *node,
                                 ctrl_msg_data *data);
static int junit_process_test_start(node_info_t *node,
                                    ctrl_msg_data *data);
static int junit_process_test_end(node_info_t *node,
                                  ctrl_msg_data *data);
static int junit_process_regular_msg(log_msg *log);
static int junit_process_open(void);
static int junit_process_close(void);

/** List of currently processed packages, from top to bottom. */
static tqh_strings pkg_names;

/** Collect all error and warning logs */
static struct obstack *ew_log_obstk = NULL;

/* See the description in junit_mode.h */
void
junit_mode_init(f_process_ctrl_log_msg
                        ctrl_proc[CTRL_EVT_LAST][NT_LAST],
                f_process_reg_log_msg *reg_proc,
                f_process_log_root root_proc[CTRL_EVT_LAST])
{
    ctrl_proc[CTRL_EVT_START][NT_PACKAGE] = junit_process_pkg_start;
    ctrl_proc[CTRL_EVT_END][NT_PACKAGE] = junit_process_pkg_end;
    ctrl_proc[CTRL_EVT_START][NT_TEST] = junit_process_test_start;
    ctrl_proc[CTRL_EVT_END][NT_TEST] = junit_process_test_end;
    ctrl_proc[CTRL_EVT_START][NT_SESSION] = NULL;
    ctrl_proc[CTRL_EVT_END][NT_SESSION] = NULL;
    ctrl_proc[CTRL_EVT_START][NT_BRANCH] = NULL;
    ctrl_proc[CTRL_EVT_END][NT_BRANCH] = NULL;

    *reg_proc = junit_process_regular_msg;

    root_proc[CTRL_EVT_START] = junit_process_open;
    root_proc[CTRL_EVT_END] = junit_process_close;
}

/**
 * Start log processing.
 */
static int
junit_process_open(void)
{
    fputs("<?xml version=\"1.0\"?>\n", rgt_ctx.out_fd);
    fputs("<testsuites>\n", rgt_ctx.out_fd);

    TAILQ_INIT(&pkg_names);

    return 0;
}

/**
 * Finish log processing.
 */
int
junit_process_close(void)
{
    tqe_string            *tqe_str;

    fputs("</testsuites>\n", rgt_ctx.out_fd);

    while ((tqe_str = TAILQ_FIRST(&pkg_names)) != NULL)
    {
        TAILQ_REMOVE(&pkg_names, tqe_str, links);
        free(tqe_str->v);
        free(tqe_str);
    }

    return 0;
}

/** Process "package started" control message. */
static int
junit_process_pkg_start(node_info_t *node, ctrl_msg_data *data)
{
    tqe_string  *tqe_str;
    double       time_val;

    UNUSED(data);

    time_val = RGT_TIME_DIFF(node->end_ts, node->start_ts);

    fprintf(rgt_ctx.out_fd, "<testsuite name=\"%s\" time=\"%.3f\">\n",
            node->descr.name, time_val);

    tqe_str = calloc(1, sizeof(*tqe_str));
    if (tqe_str == NULL)
    {
        fputs("Out of memory\n", stderr);
        return -1;
    }

    tqe_str->v = strdup(node->descr.name);
    if (tqe_str->v == NULL)
    {
        fputs("Out of memory\n", stderr);
        return -1;
    }

    TAILQ_INSERT_TAIL(&pkg_names, tqe_str, links);

    return 0;
}

/**
 * Callback for printing all verdicts in single attribute.
 *
 * @param data        Pointer to log_msg_ptr.
 * @param user_data   Pointer to boolean value which should be
 *                    set to @c TRUE initially.
 */
static void
print_verdicts_in_attr_cb(gpointer data, gpointer user_data)
{
    log_msg_ptr *msg_ptr = (log_msg_ptr *)data;
    log_msg     *msg = NULL;

    te_bool     *first = (te_bool *)user_data;

    msg = log_msg_read(msg_ptr);
    rgt_expand_log_msg(msg);
    if (!*first)
        fputs("; ", rgt_ctx.out_fd);
    write_xml_string(NULL, msg->txt_msg, TRUE);
    free_log_msg(msg);

    *first = FALSE;
}

/**
 * Log <skipped> node.
 *
 * @param data        Control message data.
 */
static void
process_skipped(ctrl_msg_data *data)
{
    if (!msg_queue_is_empty(&data->verdicts))
    {
        te_bool first = TRUE;

        fputs("<skipped message=\"", rgt_ctx.out_fd);
        msg_queue_foreach(&data->verdicts, print_verdicts_in_attr_cb,
                          &first);
        fputs("\"/>\n", rgt_ctx.out_fd);
    }
    else
    {
        fputs("<skipped/>\n", rgt_ctx.out_fd);
    }
}

/** Check whether string is empty. */
static te_bool string_empty(const char *str)
{
    return (str == NULL || str[0] == '\0');
}

/** Process "package ended" control message. */
static int
junit_process_pkg_end(node_info_t *node, ctrl_msg_data *data)
{
    tqe_string    *tqe_str;

    UNUSED(node);
    UNUSED(data);

    if (string_empty(node->result.err) &&
        node->result.status == RES_STATUS_SKIPPED)
        process_skipped(data);

    fputs("</testsuite>\n", rgt_ctx.out_fd);

    tqe_str = TAILQ_LAST(&pkg_names, tqh_strings);
    assert(tqe_str != NULL);
    TAILQ_REMOVE(&pkg_names, tqe_str, links);
    free(tqe_str->v);
    free(tqe_str);

    return 0;
}

/** Callback for printing verdicts and artifacts to file. */
static void
process_result_cb(gpointer data, gpointer user_data)
{
    log_msg_ptr *msg_ptr = (log_msg_ptr *)data;
    log_msg     *msg = NULL;

    UNUSED(user_data);

    msg = log_msg_read(msg_ptr);
    rgt_expand_log_msg(msg);
    write_xml_string(NULL, msg->txt_msg, FALSE);
    fputs("\n", rgt_ctx.out_fd);
    free_log_msg(msg);
}

/** Process "test started" control message. */
static int
junit_process_test_start(node_info_t *node, ctrl_msg_data *data)
{
    tqe_string    *tqe_str;
    double         time_val;

    UNUSED(data);

    if (ew_log_obstk == NULL)
        ew_log_obstk = obstack_initialize();

    fputs("<testcase classname=\"", rgt_ctx.out_fd);

    tqe_str = TAILQ_FIRST(&pkg_names);
    if (tqe_str != NULL)
    {
        fputs(tqe_str->v, rgt_ctx.out_fd);
        tqe_str = TAILQ_NEXT(tqe_str, links);
        if (tqe_str != NULL)
        {
            fprintf(rgt_ctx.out_fd, ".%s", tqe_str->v);
            tqe_str = TAILQ_NEXT(tqe_str, links);
        }
        else
        {
            /* This is done for top prologue/epilogue;
             * otherwise Jenkins will place them into separate
             * hierarchy having unnamed root. */
            fputs(".[top]", rgt_ctx.out_fd);
        }
    }

    fputs("\" name=\"", rgt_ctx.out_fd);

    while (tqe_str != NULL)
    {
        fprintf(rgt_ctx.out_fd, "%s.", tqe_str->v);
        tqe_str = TAILQ_NEXT(tqe_str, links);
    }

    fputs(node->descr.name, rgt_ctx.out_fd);

    if (!string_empty(node->descr.hash))
        fprintf(rgt_ctx.out_fd, "%%%s", node->descr.hash);

    time_val = RGT_TIME_DIFF(node->end_ts, node->start_ts);
    fprintf(rgt_ctx.out_fd, "\" time=\"%.3f\">\n", time_val);

    return 0;
}

/** Process "failure" node */
static void
process_failure(node_info_t *node, ctrl_msg_data *data)
{
    struct param *p;

    fprintf(rgt_ctx.out_fd, "<failure message=\"%s: %s\">\n",
            result_status2str(node->result.status), node->result.err);

    fprintf(rgt_ctx.out_fd, "Test parameters:\n");
    for (p = node->params; p != NULL; p = p->next)
    {
        fprintf(rgt_ctx.out_fd, "  %s = ", p->name);
        write_xml_string(NULL, p->val, FALSE);
        fputc('\n', rgt_ctx.out_fd);
    }

    fprintf(rgt_ctx.out_fd, "\nError and warning messages:\n");
    if (ew_log_obstk != NULL)
    {
        obstack_1grow(ew_log_obstk, '\0');
        fputs(obstack_finish(ew_log_obstk), rgt_ctx.out_fd);
    }

    if (data != NULL)
    {
        if (!msg_queue_is_empty(&data->verdicts))
        {
            fputs("\nVerdict: ", rgt_ctx.out_fd);
            msg_queue_foreach(&data->verdicts, process_result_cb, NULL);
        }
        if (!msg_queue_is_empty(&data->artifacts))
        {
            fputs("\nArtifacts: ", rgt_ctx.out_fd);
            msg_queue_foreach(&data->artifacts, process_result_cb, NULL);
        }
    }

    fputs("</failure>\n", rgt_ctx.out_fd);
}

/** Process "test ended" control message. */
static int
junit_process_test_end(node_info_t *node, ctrl_msg_data *data)
{
    if (!string_empty(node->result.err))
        process_failure(node, data);
    else if (node->result.status == RES_STATUS_SKIPPED)
        process_skipped(data);

    fputs("</testcase>\n", rgt_ctx.out_fd);

    if (ew_log_obstk != NULL) {
        obstack_destroy(ew_log_obstk);
        ew_log_obstk = NULL;
    }

    return 0;
}

/** Collect all error and warning logs */
static int
junit_process_regular_msg(log_msg *log)
{
    if (log->level != TE_LL_ERROR && log->level != TE_LL_WARN)
        return 0;

    if (ew_log_obstk == NULL)
        return 0;

    rgt_expand_log_msg(log);
    if (log->txt_msg == NULL)
        return 0;

    obstack_printf(ew_log_obstk, "%s %s %s\n",
                   log->level_str, log->entity, log->user);
    write_xml_string(ew_log_obstk, log->txt_msg, FALSE);
    obstack_grow(ew_log_obstk, "\n\n", strlen("\n\n"));

    return 0;
}
