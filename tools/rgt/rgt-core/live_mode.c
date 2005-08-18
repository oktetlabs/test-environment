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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/* To get obstack_printf() definition */
#define _GNU_SOURCE 1
#include <stdio.h>

#ifdef HAVE_TIME_H
#include <time.h>
#endif

#include "log_msg.h"
#include "rgt_common.h"
#include "live_mode.h"

#include "te_errno.h"

static int live_process_test_start(node_info_t *node);
static int live_process_test_end(node_info_t *node);
static int live_process_pkg_start(node_info_t *node);
static int live_process_pkg_end(node_info_t *node);
static int live_process_sess_start(node_info_t *node);
static int live_process_sess_end(node_info_t *node);
static int live_process_branch_start(node_info_t *node);
static int live_process_branch_end(node_info_t *node);
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
live_process_start_event(node_info_t *node, const char *node_name)
{
    fprintf(rgt_ctx.out_fd, "| Starting %s: %s\n",
            node_name, node->descr.name);
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
live_process_end_event(node_info_t *node, const char *node_name)
{
    const char *result;

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
live_process_test_start(node_info_t *node)
{
    return live_process_start_event(node, "test");
}

static int
live_process_test_end(node_info_t *node)
{
    return live_process_end_event(node, "Test");
}

static int
live_process_pkg_start(node_info_t *node)
{
    return live_process_start_event(node, "package");
}

static int
live_process_pkg_end(node_info_t *node)
{
    return live_process_end_event(node, "Package");
}

static int
live_process_sess_start(node_info_t *node)
{
    return live_process_start_event(node, "session");
}

static int
live_process_sess_end(node_info_t *node)
{
    return live_process_end_event(node, "Session");
}

static int
live_process_branch_end(node_info_t *node)
{
    UNUSED(node);
    return 1;
}

static int
live_process_branch_start(node_info_t *node)
{
    UNUSED(node);
    return 1;
}

static int
live_process_regular_msg(log_msg *msg)
{
    rgt_expand_regular_log_msg(msg);

    fprintf(rgt_ctx.out_fd, "%s %s %s ",
            msg->level, msg->entity, msg->user);
    print_ts(msg->timestamp);
    fprintf(rgt_ctx.out_fd, "\n  %s\n\n", msg->txt_msg);

    return 1;
}

static void
rgt_expand_regular_log_msg(log_msg *msg)
{
    static char tmp[40];

    msg_arg *arg;
    int      str_len;
    int      i;

    if (msg->txt_msg != NULL)
        return;

    str_len = strlen(msg->fmt_str);
    
    for (i = 0; i < str_len; i++)
    {
        if (msg->fmt_str[i] == '%' && i < str_len - 1)
        {
            switch (msg->fmt_str[i + 1])
            {
                case 'c':
                case 'd':
                case 'u':
                case 'o':
                case 'x':
                case 'X':
                {
                    char format[3] = {'%', msg->fmt_str[i + 1], '\0'};

                    if ((arg = get_next_arg(msg)) == NULL)
                    {
                        fprintf(stderr,
                                "Too few arguments in the message");
                        free_log_msg(msg);
                        THROW_EXCEPTION;
                    }

                    *((uint32_t *)arg->val) = 
                        ntohl(*(uint32_t *)arg->val);

                    sprintf(tmp, format, *((uint32_t *)arg->val));
                    obstack_grow(msg->obstk, tmp, strlen(tmp));
                    i++;

                    continue;
                }

                case 'p':
                {
                    uint32_t val;
                    int      j;

                    if ((arg = get_next_arg(msg)) == NULL)
                    {
                        fprintf(stderr,
                                "Too few arguments in the message");
                        free_log_msg(msg);
                        THROW_EXCEPTION;
                    }

                    /* Address should be 4 bytes aligned */
                    assert(arg->len % 4 == 0);

                    obstack_grow(msg->obstk, "0x", strlen("0x"));
                    for (j = 0; j < arg->len / 4; j++)
                    {
                        val = *(((uint32_t *)arg->val) + j);

                        /* Skip not trailing zero words */
                        if (val == 0 && (j + 1) < arg->len / 4)
                        {
                            continue;
                        }
                        val = ntohl(val);

                        sprintf(tmp, "%08x", val);
                        obstack_grow(msg->obstk, tmp, strlen(tmp));
                    }

                    i++;

                    continue;
                }

                case 's':
                {
                    if ((arg = get_next_arg(msg)) == NULL)
                    {
                        fprintf(stderr,
                                "Too few arguments in the message");
                        free_log_msg(msg);
                        THROW_EXCEPTION;
                    }

                    obstack_grow(msg->obstk, arg->val, arg->len);
                    i++;

                    continue;
                }
                case '%':
                {
                    obstack_1grow(msg->obstk, msg->fmt_str[i]);
                    i++;
                    continue;
                }

                case 'r':
                {
                    te_errno    err;
                    const char *src;
                    const char *err_str;

                    if ((arg = get_next_arg(msg)) == NULL)
                    {
                        fprintf(stderr,
                                "Too few arguments in the message:\n");
                        free_log_msg(msg);
                        THROW_EXCEPTION;
                    }

                    err = *((uint32_t *)arg->val) = 
                        ntohl(*(uint32_t *)arg->val);

                    src = te_rc_mod2str(err);
                    err_str = te_rc_err2str(err);
                    if (strlen(src) > 0)
                    {
                        obstack_grow(msg->obstk, src, strlen(src));
                        obstack_1grow(msg->obstk, '-');
                    }
                    obstack_grow(msg->obstk, err_str, strlen(err_str));
                    i++;

                    continue;
                }

                case 't':
                {
                    int  j;
                    int  n_tuples;
                    int  tuple_width;
                    int  cur_pos = 0;
                    char one_byte_str[3];
                    int  default_format = FALSE;
                    int  k;
                    int rc = 10;

                    /* @todo think of better way to implement this! */
                    if (strstr(msg->fmt_str + i, "%tf") ==
                            (msg->fmt_str + i))
                    {
                        FILE *fd;
                        char  str[500];

                        if ((arg = get_next_arg(msg)) == NULL)
                        {
                            fprintf(stderr,
                                    "Too few arguments in the message\n");
                            free_log_msg(msg);
                            THROW_EXCEPTION;
                        }
                        if ((fd = fopen(arg->val, "r")) == NULL)
                        {
                            perror("Error during the processing of the log "
                                   "message");
                            free_log_msg(msg);
                            THROW_EXCEPTION;
                        }

                        obstack_printf(msg->obstk, "File '%s':\n",
                                       arg->val);

                        while (fgets(str, sizeof(str), fd) != NULL)
                        {
                            obstack_grow(msg->obstk, str, strlen(str));
                        }
                        fclose(fd);

                        /* shift to the end of "%tf" */
                        i += 2;
                        break;
                    }

/*
 *  %tm[[n].[w]] - memory dump, n - the number of elements after
 *                 which "\n" is to be inserted , w - width (in bytes) of 
 *                 the element.
 */
                    
                    if (strstr(msg->fmt_str + i, "%tm") !=
                            (msg->fmt_str + i))
                    {
                        /* Invalid format just output as it is */
                        fprintf(stderr, "WARNING: Invalid format for "
                                "%%t specificator\n");
                        break;
                    }

                    if ((arg = get_next_arg(msg)) == NULL)
                    {
                        fprintf(stderr, "Too few arguments in the message");
                        free_log_msg(msg);
                        THROW_EXCEPTION;
                    }

                    if (/* i > str_len - 10 || */
                        (rc = sscanf(msg->fmt_str + i, "%%tm[[%d].[%d]]", 
                               &n_tuples, &tuple_width)) != 2)
                    {
                        default_format = TRUE;
                        tuple_width = 1;
                        n_tuples = 16; /* @todo remove hardcode */
                    }

                    obstack_1grow(msg->obstk, '\n');
                    while (cur_pos < arg->len)
                    {
                        obstack_grow(msg->obstk, "\n   ", 4);
                        
                        for (j = 0;
                             j < n_tuples && cur_pos < arg->len;
                             j++)
                        {
                            for (k = 0; 
                                 (k < tuple_width) && (cur_pos < arg->len);
                                 k++, cur_pos++)
                            {
                                snprintf(one_byte_str, sizeof(one_byte_str),
                                         "%02X",
                                         *(arg->val + cur_pos));
                                obstack_grow(msg->obstk, one_byte_str, 2);
                            }
                            obstack_1grow(msg->obstk, ' ');
                        }
                    }
                    obstack_grow(msg->obstk, "\n\n", 2);

                    /* shift to the end of "%tm" */
                    i += 2;

                    if (!default_format)
                    {
                        j = 0;
                        i += 2; /* shift to the end of "[[" */
                        while (*(msg->fmt_str + i++) != ']' || ++j != 3)
                        {
                        }

                        i--; 
                    }

                    continue;
                }
            }
        }

        obstack_1grow(msg->obstk, msg->fmt_str[i]);
    }
    
    obstack_1grow(msg->obstk, '\0');

    str_len = obstack_object_size(msg->obstk);
    msg->txt_msg = (char *)obstack_finish(msg->obstk);

    /* Truncate trailing end of line characters */
    i = 2;
    while ((str_len >= i) && (*(msg->txt_msg + str_len - i) == '\n'))
        i++;

    *(msg->txt_msg + str_len + 1 - i) = '\0';

    return;
}
