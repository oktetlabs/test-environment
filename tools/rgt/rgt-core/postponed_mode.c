/** @file 
 * @brief Test Environment: Postponed mode specific routines.
 *
 * Interface for output control message events and regular messages 
 * into the XML file.
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

#if HAVE_CONFIG_H
#include <config.h>
#endif

/* To get obstack_printf() definition */
#define _GNU_SOURCE 1
#include <stdio.h>

#if HAVE_TIME_H
#include <time.h>
#endif

#if HAVE_CTYPE_H
#include <ctype.h>
#endif

#include "log_msg.h"
#include "rgt_common.h"
#include "postponed_mode.h"
#include "memory.h"

static int logs_opened = 0;
static int logs_closed = 1;
struct obstack *log_obstk = NULL;

static int postponed_process_test_start(node_info_t *node);
static int postponed_process_test_end(node_info_t *node);
static int postponed_process_pkg_start(node_info_t *node);
static int postponed_process_pkg_end(node_info_t *node);
static int postponed_process_sess_start(node_info_t *node);
static int postponed_process_sess_end(node_info_t *node);
static int postponed_process_branch_start(node_info_t *node);
static int postponed_process_branch_end(node_info_t *node);
static int postponed_process_regular_msg(log_msg *msg);

static void output_regular_log_msg(log_msg *msg);

void
postponed_mode_init(f_process_ctrl_log_msg
                        ctrl_proc[CTRL_EVT_LAST][NT_LAST],
                    f_process_reg_log_msg *reg_proc)
{
    ctrl_proc[CTRL_EVT_START][NT_SESSION] = postponed_process_sess_start;
    ctrl_proc[CTRL_EVT_END][NT_SESSION] = postponed_process_sess_end;
    ctrl_proc[CTRL_EVT_START][NT_TEST] = postponed_process_test_start;
    ctrl_proc[CTRL_EVT_END][NT_TEST] = postponed_process_test_end;
    ctrl_proc[CTRL_EVT_START][NT_PACKAGE] = postponed_process_pkg_start;
    ctrl_proc[CTRL_EVT_END][NT_PACKAGE] = postponed_process_pkg_end;
    ctrl_proc[CTRL_EVT_START][NT_BRANCH] = postponed_process_branch_start;
    ctrl_proc[CTRL_EVT_END][NT_BRANCH] = postponed_process_branch_end;
    
    *reg_proc = postponed_process_regular_msg;
}

static void
print_ts(FILE *fd, uint32_t *ts)
{ 
#define TIME_BUF_LEN 40

    char       time_buf[TIME_BUF_LEN];
    struct tm  tm, *tm_tmp;
    size_t     res;
    
    if ((tm_tmp = localtime((time_t *)&(ts[0]))) == NULL)
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
    fprintf(fd, "%s %u ms", time_buf, ts[1] / 1000);

#undef TIME_BUF_LEN
}

/**
 * Processes string with expanding XML special characters
 *
 * @param obstk  Obstack structure for string output
 * @param str    String to process and output
 */
static void
fwrite_string(struct obstack *obstk, const char *str)
{
    int i = 0;
    
    while (str[i] != '\0')
    {
        switch (str[i])
        {
            case '\r':
                if (i > 0 && str[i - 1] == '\n')
                {
                    /*
                     * Skip \r after \n, because it does not bring any
                     * formating, but just follows after ]n on some systems
                     */
                    break;
                }
                /* Process it as an ordinary new line character */
                /* FALLTHROUGH */

            case '\n':
                if (obstk != NULL)
                    obstack_grow(log_obstk, "<br/>", 5);
                else
                    fputs("<br/>", rgt_ctx.out_fd);
                break;

            case '<':
                if (obstk != NULL)
                    obstack_grow(log_obstk, "&lt;", 4);
                else
                    fputs("&lt;", rgt_ctx.out_fd);
                break;
            
            case '>':
                if (obstk != NULL)
                    obstack_grow(log_obstk, "&gt;", 4);
                else
                    fputs("&gt;", rgt_ctx.out_fd);
                break;
            
            case '&':
                if (obstk != NULL)
                    obstack_grow(log_obstk, "&amp;", 5);
                else
                    fputs("&amp;", rgt_ctx.out_fd);
                break;
                
            default:
                if (str[i] == '\t' || isprint(str[i]))
                {
                    if (obstk != NULL)
                        obstack_1grow(log_obstk, str[i]);
                    else
                        fputc(str[i], rgt_ctx.out_fd);
                }
                else
                {
                    if (obstk != NULL)
                        obstack_printf(log_obstk, "&lt;0x%02x&gt;",
                                       (unsigned char)str[i]);
                    else
                        fprintf(rgt_ctx.out_fd, "&lt;0x%02x&gt;",
                                (unsigned char)str[i]);
                }
                break;
        }
        i++;
    }
}

static void
print_ts_info(node_info_t *node)
{
    uint32_t duration[2];

    fprintf(rgt_ctx.out_fd, "<start-ts>");
    print_ts(rgt_ctx.out_fd, node->start_ts);
    fprintf(rgt_ctx.out_fd, "</start-ts>\n");
    fprintf(rgt_ctx.out_fd, "<end-ts>");
    print_ts(rgt_ctx.out_fd, node->end_ts);
    fprintf(rgt_ctx.out_fd, "</end-ts>\n");
    
    /*
     * This information is surplus but it could be useful to get it
     * without additional processing "start-ts" and "end-ts" tags.
     */
    fprintf(rgt_ctx.out_fd, "<duration>");
    TIMESTAMP_SUB(duration, node->end_ts, node->start_ts);
    fprintf(rgt_ctx.out_fd, "%u:%u:%u %u ms",
            duration[0] / (60 * 60),
            (duration[0] % (60 * 60)) / 60,
            (duration[0] % (60 * 60)) % 60,
            duration[1] / 1000);
    fprintf(rgt_ctx.out_fd, "</duration>\n");
}

int
postponed_process_open()
{
    if (log_obstk == NULL)
        log_obstk = obstack_initialize();

    fprintf(rgt_ctx.out_fd, "<?xml version=\"1.0\"?>\n");
    fprintf(rgt_ctx.out_fd, "<proteos:log_report "
            "xmlns:proteos=\"http://www.oktetlabs.ru/proteos\">\n");

    return 0;
}

int
postponed_process_close()
{
    if (log_obstk != NULL)
        obstack_destroy(log_obstk);

    if (!logs_closed)
    {
        fprintf(rgt_ctx.out_fd, "</logs>\n");
        logs_opened = 0;
        logs_closed = 1;
    }

    fprintf(rgt_ctx.out_fd, "</proteos:log_report>\n");
    return 0;
}

static void
print_params(node_info_t *node)
{
    if (node->params != NULL)
    {
        param *prm = node->params;

        fprintf(rgt_ctx.out_fd, "<params>\n");
        while (prm != NULL)
        {
            fprintf(rgt_ctx.out_fd, "<param name=\"%s\" value=\"%s\"/>\n",
                    prm->name, prm->val);
            prm = prm->next;
        }
        fprintf(rgt_ctx.out_fd, "</params>\n");
    }
}

static inline int
postponed_process_start_event(node_info_t *node, const char *node_name)
{
    if (!logs_closed)
    {
        fprintf(rgt_ctx.out_fd, "</logs>\n");
        logs_opened = 0;
        logs_closed = 1;
    }

    fprintf(rgt_ctx.out_fd, "<%s", node_name);
    if (node->descr.name)
        fprintf(rgt_ctx.out_fd, " name=\"%s\"", node->descr.name);

    switch (node->result.status)
    {
#define NODE_RES_CASE(res_) \
        case RES_STATUS_ ## res_:                \
            fprintf(rgt_ctx.out_fd, " result=\"" #res_ "\""); \
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

    if (node->result.err)
    {
        fprintf(rgt_ctx.out_fd, " err=\"%s\"", node->result.err);
    }
    fprintf(rgt_ctx.out_fd, ">\n");

    if (node->descr.n_branches > 1)
    {
        fprintf(rgt_ctx.out_fd, "<meta nbranches=\"%d\">\n",
                node->descr.n_branches);
    }
    else
    {
        fprintf(rgt_ctx.out_fd, "<meta>\n");
    }

    print_ts_info(node);

    if (node->descr.objective != NULL)
    {
        fputs("<objective>", rgt_ctx.out_fd);
        fwrite_string(NULL, node->descr.objective);
        fputs("</objective>\n", rgt_ctx.out_fd);
    }
    if (node->descr.authors)
    {
        char *author = node->descr.authors;
        char *ptr;

        fputs("<authors>", rgt_ctx.out_fd);

        /* Authors are separated with a space */
        do {
            if ((ptr = strchr(author, ' ')) != NULL)
            {
                *ptr = '\0';
                ptr++;
            }

            fputs("<author email=\"", rgt_ctx.out_fd);
            author += strlen("mailto:");
            fwrite_string(NULL, author);
            fputs("\"/>", rgt_ctx.out_fd);
            author = ptr;
        } while (ptr != NULL);

        fputs("</authors>\n", rgt_ctx.out_fd);
    }

    print_params(node);
    fprintf(rgt_ctx.out_fd, "</meta>\n");
    logs_opened = 0;

    return 1;
}

static inline int
postponed_process_end_event(node_info_t *node, const char *node_name)
{
    UNUSED(node);

    if (!logs_closed)
    {
        fprintf(rgt_ctx.out_fd, "</logs>\n");
        logs_opened = 0;
        logs_closed = 1;
    }
    
    fprintf(rgt_ctx.out_fd, "</%s>\n", node_name);
    return 1;
}

static int
postponed_process_test_start(node_info_t *node)
{
    return postponed_process_start_event(node, "test");
}

static int
postponed_process_test_end(node_info_t *node)
{
    return postponed_process_end_event(node, "test");
}

static int
postponed_process_pkg_start(node_info_t *node)
{
    return postponed_process_start_event(node, "pkg");
}

static int
postponed_process_pkg_end(node_info_t *node)
{
    return postponed_process_end_event(node, "pkg");
}

static int
postponed_process_sess_start(node_info_t *node)
{
    return postponed_process_start_event(node, "session");
}

static int
postponed_process_sess_end(node_info_t *node)
{
    return postponed_process_end_event(node, "session");
}

static int
postponed_process_branch_start(node_info_t *node)
{
    UNUSED(node);

    if (!logs_closed)
    {
        fprintf(rgt_ctx.out_fd, "</logs>\n");
        logs_opened = 0;
        logs_closed = 1;
    }
    fprintf(rgt_ctx.out_fd, "<branch>\n");
    return 1;
}

static int
postponed_process_branch_end(node_info_t *node)
{
    UNUSED(node);

    if (!logs_closed)
    {
        fprintf(rgt_ctx.out_fd, "</logs>\n");
        logs_opened = 0;
        logs_closed = 1;
    }
    fprintf(rgt_ctx.out_fd, "</branch>\n");
    return 1;
}

static int
postponed_process_regular_msg(log_msg *msg)
{
    if (!logs_opened)
    {
        fprintf(rgt_ctx.out_fd, "<logs>");
        logs_opened = 1;
        logs_closed = 0;
    }
    fprintf(rgt_ctx.out_fd,
            "<msg level=\"%s\" entity=\"%s\" user=\"%s\" ts=\"",
            msg->level, msg->entity, msg->user);
    print_ts(rgt_ctx.out_fd, msg->timestamp);
    fprintf(rgt_ctx.out_fd, "\">");
    output_regular_log_msg(msg);
    fprintf(rgt_ctx.out_fd, "</msg>\n");

    return 1;
}

static void
print_message_info(log_msg *msg)
{
    fprintf(stderr, "entity name: %s\nuser name: %s\ntimestmp: ", 
            msg->entity, msg->user);
    print_ts(stderr, msg->timestamp);
    fprintf(stderr, "\nformat string: %s\n", msg->fmt_str);
    fprintf(stderr, "\n");
}

static void
output_regular_log_msg(log_msg *msg)
{
    msg_arg *arg;
    int      i;
    void    *obstk_base;

/* @todo - It should be removed in the future! */
#define OBSTACK_FLUSH() \
    do {                                              \
        char *out_str;                                \
                                                      \
        obstack_1grow(log_obstk, '\0');               \
        out_str = (char *)obstack_finish(log_obstk);  \
        fprintf(rgt_ctx.out_fd, "%s", out_str);            \
        obstack_free(log_obstk, out_str);             \
    } while (0)

    if (log_obstk == NULL)
        return;

    obstk_base = obstack_base(log_obstk);
    log_msg_init_arg(msg);

    for (i = 0; msg->fmt_str[i] != '\0'; i++)
    {
        if (msg->fmt_str[i] == '%' && msg->fmt_str[i + 1] != '\0')
        {
            /* Parse output format */
            switch (msg->fmt_str[i + 1])
            {
                case 'c':
                case 'd':
                case 'u':
                case 'o':
                case 'x':
                case 'X':
                case 'p': /* FIXME: Add 0x before %p automatically */
                {
                    char  format[3] = {'%', msg->fmt_str[i + 1], '\0'};

                    if ((arg = get_next_arg(msg)) == NULL)
                    {
                        fprintf(stderr,
                                "Too few arguments in the message:\n");
                        print_message_info(msg);
                        THROW_EXCEPTION;
                    }

                    *((uint32_t *)arg->val) = 
                        ntohl(*(uint32_t *)arg->val);

                    obstack_printf(log_obstk, format,
                                   *((uint32_t *)arg->val));
                    i++;

                    continue;
                }
                case 's':
                {
                    if ((arg = get_next_arg(msg)) == NULL)
                    {
                        fprintf(stderr,
                                "Too few arguments in the message\n");
                        print_message_info(msg);
                        THROW_EXCEPTION;
                    }

                    fwrite_string(log_obstk, arg->val);
                    i++;

                    continue;
                }
                case '%':
                {
                    obstack_1grow(log_obstk, msg->fmt_str[i + 1]);
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
                   
                    /* @todo think of better way to implement this! */
                    if (strstr(msg->fmt_str + i, "%tf") ==
                            (msg->fmt_str + i))
                    {
                        FILE *fd;
                        char  str[500];

                        if (obstack_next_free(log_obstk) != obstk_base)
                            OBSTACK_FLUSH();

                        if ((arg = get_next_arg(msg)) == NULL)
                        {
                            fprintf(stderr,
                                    "Too few arguments in the message\n");
                            print_message_info(msg);
                            THROW_EXCEPTION;
                        }
                        if ((fd = fopen(arg->val, "r")) == NULL)
                        {
                            perror("Error during the processing of the log "
                                   "message");
                            print_message_info(msg);
                            THROW_EXCEPTION;
                        }

                        /* Strart file tag */
                        fprintf(rgt_ctx.out_fd, "<file name=\"%s\">",
                                arg->val);
                        while (fgets(str, sizeof(str), fd) != NULL)
                        {
                            fwrite_string(NULL, str);
                        }
                        /* End file tag */
                        fputs("</file>", rgt_ctx.out_fd);
                        fclose(fd);

                        /* shift to the end of "%tf" */
                        i += 3;
                        break;
                    }
                   

                   /*
                    * %tm[[n].[w]] - memory dump, n - the number of
                    * elements after which "\n" is to be inserted,
                    * w - width (in bytes) of the element.
                    */
                    if (strstr(msg->fmt_str + i, "%tm") !=
                            (msg->fmt_str + i))
                    {
                        /* Invalid format just output as it is */
                        fprintf(stderr, "WARNING: Invalid format for "
                                "%%t specificator\n");
                        print_message_info(msg);
                        break;
                    }

                    if ((arg = get_next_arg(msg)) == NULL)
                    {
                        fprintf(stderr,
                                "Too few arguments in the message\n");
                        print_message_info(msg);
                        THROW_EXCEPTION;
                    }

                    obstack_grow(log_obstk,
                                 "<mem-dump>", strlen("<mem-dump>"));
                    if (sscanf(msg->fmt_str + i, "%%tm[[%d].[%d]]", 
                               &n_tuples, &tuple_width) != 2)
                    {
                        default_format = TRUE;
                        tuple_width = 1;
                        n_tuples = 16; /* @todo remove hardcode */
                    }

                    while (cur_pos < arg->len)
                    {
                        obstack_grow(log_obstk, "<row>", strlen("<row>"));
                        /* Start a memory table row */
                        for (j = 0;
                             j < n_tuples && cur_pos < arg->len;
                             j++)
                        {
                            /* Start a block in a row */
                            obstack_grow(log_obstk, "<elem>",
                                         strlen("<elem>"));
                            for (k = 0; 
                                 k < tuple_width && cur_pos < arg->len;
                                 k++, cur_pos++)
                            {
                                snprintf(one_byte_str, sizeof(one_byte_str),
                                         "%02X", *(arg->val + cur_pos));
                                obstack_grow(log_obstk, one_byte_str, 2);
                            }
                            /* End a block in a row */
                            obstack_grow(log_obstk, "</elem>",
                                         strlen("</elem>"));
                        }
                        /* End a memory table row */
                        obstack_grow(log_obstk, "</row>", strlen("</row>"));
                    }
                    obstack_grow(log_obstk, "</mem-dump>",
                                 strlen("</mem-dump>"));

                    /* shift to the end of "%tm" */
                    i += 3;

                    if (!default_format)
                    {
                        j = 0;
                        i += 2; /* shift to the end of "[[" */
                        while (*(msg->fmt_str + i++) != ']' || ++j != 3)
                            ;

                        i--; 
                    }

                    continue;
                }
            }
        }
        
        switch (msg->fmt_str[i])
        {
            case '\r':
                if (i > 0 && msg->fmt_str[i - 1] == '\n')
                {
                    /*
                     * Skip \r after \n, because it does not bring any
                     * formating, but just follows after ]n on some systems
                     */
                    break;
                }
                /* Process it as ordinary new line character */
                /* FALLTHROUGH */

            case '\n':
                obstack_grow(log_obstk, "<br/>", 5);
                break;
            
            case '<':
                obstack_grow(log_obstk, "&lt;", 4);
                break;
            
            case '>':
                obstack_grow(log_obstk, "&gt;", 4);
                break;
            
            case '&':
                obstack_grow(log_obstk, "&amp;", 5);
                break;
            
            default:
                obstack_1grow(log_obstk, msg->fmt_str[i]);
                break;
        }
    }

    if (obstack_next_free(log_obstk) != obstk_base)
    {
        char *out_str;
        int   str_len;
        int   br_len = strlen("<br/>");

        obstack_1grow(log_obstk, '\0');
        str_len = obstack_object_size(log_obstk);
        out_str = (char *)obstack_finish(log_obstk);

        /* 
         * Truncate trailing end of line characters:
         * @todo - maybe it's better not to make this by default.
         */
        i = br_len + 1;
        while ((str_len >= i) &&
                strncmp((out_str + str_len - i), "<br/>", br_len) == 0)
        {
            i += br_len;
        }
        *(out_str + str_len + br_len - i) = '\0';

        fprintf(rgt_ctx.out_fd, "%s", out_str);
        obstack_free(log_obstk, out_str);
    }

    return;
}
