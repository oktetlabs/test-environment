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

#ifdef HAVE_TIME_H
#include <time.h>
#endif

#include <stdio.h>

#include "log_msg.h"
#include "rgt_common.h"
#include "live_mode.h"

static int live_process_test_start(node_info *node);
static int live_process_test_end(node_info *node);
static int live_process_pkg_start(node_info *node);
static int live_process_pkg_end(node_info *node);
static int live_process_sess_start(node_info *node);
static int live_process_sess_end(node_info *node);
static int live_process_branch_start(node_info *node);
static int live_process_branch_end(node_info *node);
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

    char       time_buf[TIME_BUF_LEN];
    struct tm  tm, *tm_tmp;
    size_t     res;

    if ((tm_tmp = gmtime((time_t *)&(ts[0]))) == NULL)
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
    fprintf(output_fd, "%s %u ms", time_buf, ts[1] / 1000);

#undef TIME_BUF_LEN
}

static void
print_params(param *prms)
{
    param *prm = prms;
    
    if (prm != NULL)
    {
        fprintf(output_fd, " - Parameters:\n");
    }
    
    while (prm != NULL)
    {
        fprintf(output_fd, "     + %s = %s\n", prm->name, prm->val);
        prm = prm->next;
    }
}

static int
live_process_test_start(node_info *node)
{
    test_info *test = &(node->node_specific.test);

    fprintf(output_fd, "| Starting test: %s\n", test->name);
    fprintf(output_fd, "|- Date: ");
    print_ts(node->start_ts);

    fprintf(output_fd, "\n|- Objective: %s", test->objective);
    fprintf(output_fd, "\n|- Author: %s\n", test->author);
    print_params(node->params);

    fprintf(output_fd, "\n");
        
    return 1;
}

static int
live_process_test_end(node_info *node)
{
    test_info *test = &(node->node_specific.test);
    
    fprintf(output_fd, "| Test complited %-55s %s\n", test->name,
            (node->result.status == RES_STATUS_PASS) ? "PASS" : "FAIL");
    fprintf(output_fd, "|- Date: ");
    print_ts(node->end_ts);
    fprintf(output_fd, "\n\n");

    return 1;
}

static int
live_process_pkg_start(node_info *node)
{
    pkg_info *pkg = &(node->node_specific.pkg);

    fprintf(output_fd, "| Starting package: %s\n", pkg->name);
    fprintf(output_fd, "|- Date: ");
    print_ts(node->start_ts);

    fprintf(output_fd, "\n|- Title: %s", pkg->title);
    fprintf(output_fd, "\n|- Author: %s\n", pkg->author);
    
    print_params(node->params);

    fprintf(output_fd, "\n");

    return 1;
}

static int
live_process_pkg_end(node_info *node)
{
    pkg_info *pkg = &(node->node_specific.pkg);

    fprintf(output_fd, "| Package complited %-52s %s\n", pkg->name,
            (node->result.status == RES_STATUS_PASS) ? "PASS" : "FAIL");
    fprintf(output_fd, "|- Date: ");
    print_ts(node->end_ts);
    fprintf(output_fd, "\n\n");

    return 1;
}

static int
live_process_sess_start(node_info *node)
{
    session_info *sess = &(node->node_specific.sess);

    fprintf(output_fd, "| Starting session: \"%s\"\n", sess->objective);
    fprintf(output_fd, "|- Date: ");
    print_ts(node->start_ts);

    fprintf(output_fd, "\n|- Objective: %s\n", sess->objective);
    print_params(node->params);

    fprintf(output_fd, "\n");

    return 1;
}

static int
live_process_sess_end(node_info *node)
{
    session_info *sess = &(node->node_specific.sess);
    
    fprintf(output_fd, "| Session complited %-52s %s\n", sess->objective,
            (node->result.status == RES_STATUS_PASS) ? "PASS" : "FAIL");
    fprintf(output_fd, "|- Date: ");
    print_ts(node->end_ts);
    fprintf(output_fd, "\n\n");    

    return 1;
}

static int
live_process_branch_end(node_info *node)
{
    UNUSED(node);
    return 1;
}

static int
live_process_branch_start(node_info *node)
{
    UNUSED(node);
    return 1;
}

static int
live_process_regular_msg(log_msg *msg)
{
    rgt_expand_regular_log_msg(msg);

    fprintf(output_fd, "%s %s %s ", msg->level, msg->entity, msg->user);
    print_ts(msg->timestamp);
    fprintf(output_fd, "\n  %s\n\n", msg->txt_msg);

    return 1;
}

static void
rgt_expand_regular_log_msg(log_msg *msg)
{
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
                    char  format[3] = {'%', msg->fmt_str[i + 1], '\0'};
                    char *tmp;

                    if ((arg = get_next_arg(msg)) == NULL)
                    {
                        fprintf(stderr, "Too few arguments in the message");
                        free_log_msg(msg);
                        THROW_EXCEPTION;
                    }

                    tmp = (char *)malloc(40);

                    *((uint32_t *)arg->val) = 
                        ntohl(*(uint32_t *)arg->val);

                    sprintf(tmp, format, *((uint32_t *)arg->val));
                    obstack_grow(msg->obstk, tmp, strlen(tmp));
                    i++;
                    free(tmp);
                    
                    continue;
                }
                case 's':
                {
                    if ((arg = get_next_arg(msg)) == NULL)
                    {
                        fprintf(stderr, "Too few arguments in the message");
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

/*
 *  %tm[[n].[w]] - memory dump, n - the number of elements after
 *                 which "\n" is to be inserted , w - width (in bytes) of 
 *                 the element.
 */
                    
                    if (strstr(msg->fmt_str + i, "%tm") != (msg->fmt_str + i))
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
                    i += 3;

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
