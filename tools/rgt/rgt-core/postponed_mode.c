/** @file
 * @brief Test Environment: Postponed mode specific routines.
 *
 * Interface for output control message events and regular messages
 * into the XML file.
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 * @author Oleg N. Kravtsov  <Oleg.Kravtsov@oktetlabs.ru>
 *
 * $Id$
 */

#include "rgt_common.h"

#if HAVE_TIME_H
#include <time.h>
#endif

#if HAVE_CTYPE_H
#include <ctype.h>
#endif

#include "log_msg.h"
#include "postponed_mode.h"
#include "memory.h"

#include "te_errno.h"


static int logs_opened = 0;
static int logs_closed = 1;

static struct obstack *log_obstk = NULL;

static int postponed_process_test_start(node_info_t *node,
                                        ctrl_msg_data *data);
static int postponed_process_test_end(node_info_t *node,
                                      ctrl_msg_data *data);
static int postponed_process_pkg_start(node_info_t *node,
                                       ctrl_msg_data *data);
static int postponed_process_pkg_end(node_info_t *node,
                                     ctrl_msg_data *data);
static int postponed_process_sess_start(node_info_t *node,
                                        ctrl_msg_data *data);
static int postponed_process_sess_end(node_info_t *node,
                                      ctrl_msg_data *data);
static int postponed_process_branch_start(node_info_t *node,
                                          ctrl_msg_data *data);
static int postponed_process_branch_end(node_info_t *node,
                                        ctrl_msg_data *data);
static int postponed_process_regular_msg(log_msg *msg);

static int postponed_process_open(void);
static int postponed_process_close(void);

static void output_regular_log_msg(log_msg *msg);

void
postponed_mode_init(f_process_ctrl_log_msg
                        ctrl_proc[CTRL_EVT_LAST][NT_LAST],
                    f_process_reg_log_msg *reg_proc,
                    f_process_log_root root_proc[CTRL_EVT_LAST])
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

    root_proc[CTRL_EVT_START] = postponed_process_open;
    root_proc[CTRL_EVT_END] = postponed_process_close;
}

static void
print_ts(FILE *fd, uint32_t *ts)
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
    fprintf(fd, "%s.%u", time_buf, ts[1] / 1000);

#undef TIME_BUF_LEN
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
    fprintf(rgt_ctx.out_fd, "%u:%u:%u.%u",
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
            fprintf(rgt_ctx.out_fd, "<param name=\"%s\" value=\"",
                    prm->name);
            write_xml_string(NULL, prm->val, TRUE);
            fprintf(rgt_ctx.out_fd, "\"/>\n");
            prm = prm->next;
        }
        fprintf(rgt_ctx.out_fd, "</params>\n");
    }
}

/**
 * Process verdict or artifact message.
 *
 * @param data        Pointer to log_msg_ptr.
 * @param user_data   Not used.
 * @param tag         Tag in which to print message.
 */
static void
process_result_msg(gpointer data, gpointer user_data,
                   const char *tag)
{
    log_msg_ptr *msg_ptr = (log_msg_ptr *)data;
    log_msg     *msg = NULL;

    UNUSED(user_data);

    fprintf(rgt_ctx.out_fd, "<%s>", tag);
    msg = log_msg_read(msg_ptr);
    output_regular_log_msg(msg);
    free_log_msg(msg);
    fprintf(rgt_ctx.out_fd, "</%s>\n", tag);
}

/*
 * Callback to process verdict message.
 */
static void
process_verdict_cb(gpointer data, gpointer user_data)
{
    process_result_msg(data, user_data, "verdict");
}

/*
 * Callback to process artifact message.
 */
static void
process_artifact_cb(gpointer data, gpointer user_data)
{
    process_result_msg(data, user_data, "artifact");
}

static inline int
postponed_process_start_event(node_info_t *node, const char *node_name,
                              ctrl_msg_data *data)
{
    if (!logs_closed)
    {
        fprintf(rgt_ctx.out_fd, "</logs>\n");
        logs_opened = 0;
        logs_closed = 1;
    }

    fprintf(rgt_ctx.out_fd, "<%s", node_name);
    if (node->descr.tin != TE_TIN_INVALID)
        fprintf(rgt_ctx.out_fd, " tin=\"%u\"", node->descr.tin);
    fprintf(rgt_ctx.out_fd, " test_id=\"%d\"", node->node_id);
    if (node->descr.name)
        fprintf(rgt_ctx.out_fd, " name=\"%s\"", node->descr.name);
    if (node->descr.hash != NULL)
        fprintf(rgt_ctx.out_fd, " hash=\"%s\"", node->descr.hash);

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
        NODE_RES_CASE(EMPTY);
        NODE_RES_CASE(INCOMPLETE);

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
        write_xml_string(NULL, node->descr.objective, FALSE);
        fputs("</objective>\n", rgt_ctx.out_fd);
    }
    if (node->descr.page != NULL)
    {
        fputs("<page>", rgt_ctx.out_fd);
        write_xml_string(NULL, node->descr.page, FALSE);
        fputs("</page>\n", rgt_ctx.out_fd);
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
            write_xml_string(NULL, author, TRUE);
            fputs("\"/>", rgt_ctx.out_fd);
            author = ptr;
        } while (ptr != NULL);

        fputs("</authors>\n", rgt_ctx.out_fd);
    }

    if (data != NULL)
    {
        if (!msg_queue_is_empty(&data->verdicts))
        {
            fputs("<verdicts>", rgt_ctx.out_fd);
            msg_queue_foreach(&data->verdicts, process_verdict_cb, NULL);
            fputs("</verdicts>\n", rgt_ctx.out_fd);
        }

        if (!msg_queue_is_empty(&data->artifacts))
        {
            fputs("<artifacts>", rgt_ctx.out_fd);
            msg_queue_foreach(&data->artifacts, process_artifact_cb, NULL);
            fputs("</artifacts>\n", rgt_ctx.out_fd);
        }
    }

    print_params(node);
    fprintf(rgt_ctx.out_fd, "</meta>\n");
    logs_opened = 0;

    return 1;
}

static inline int
postponed_process_end_event(node_info_t *node, const char *node_name,
                            ctrl_msg_data *data)
{
    UNUSED(node);
    UNUSED(data);

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
postponed_process_test_start(node_info_t *node, ctrl_msg_data *data)
{
    return postponed_process_start_event(node, "test", data);
}

static int
postponed_process_test_end(node_info_t *node, ctrl_msg_data *data)
{
    return postponed_process_end_event(node, "test", data);
}

static int
postponed_process_pkg_start(node_info_t *node, ctrl_msg_data *data)
{
    return postponed_process_start_event(node, "pkg", data);
}

static int
postponed_process_pkg_end(node_info_t *node, ctrl_msg_data *data)
{
    return postponed_process_end_event(node, "pkg", data);
}

static int
postponed_process_sess_start(node_info_t *node, ctrl_msg_data *data)
{
    return postponed_process_start_event(node, "session", data);
}

static int
postponed_process_sess_end(node_info_t *node, ctrl_msg_data *data)
{
    return postponed_process_end_event(node, "session", data);
}

static int
postponed_process_branch_start(node_info_t *node, ctrl_msg_data *data)
{
    UNUSED(node);
    UNUSED(data);

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
postponed_process_branch_end(node_info_t *node, ctrl_msg_data *data)
{
    UNUSED(node);
    UNUSED(data);

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
            "<msg level=\"%s\" entity=\"%s\" user=\"%s\" ts_val=\"%u.%06u\" "
            "ts=\"", msg->level_str, msg->entity, msg->user,
            msg->timestamp[0], msg->timestamp[1]);
    print_ts(rgt_ctx.out_fd, msg->timestamp);
    fprintf(rgt_ctx.out_fd, "\" nl=\"%d\">", msg->nest_lvl);
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

    if (msg->txt_msg != NULL)
    {
        write_xml_string(log_obstk, msg->txt_msg, FALSE);
    }
    else
    {
        for (i = 0; msg->fmt_str[i] != '\0'; i++)
        {
            if (msg->fmt_str[i] == '%' && msg->fmt_str[i + 1] != '\0')
            {
                if (msg->fmt_str[i + 1] == '%')
                {
                    obstack_1grow(log_obstk, '%');
                    i++;
                    continue;
                }

                if ((arg = get_next_arg(msg)) == NULL)
                {
                    /* Too few arguments in the message */
                    /* Simply write the rest of format string to the log */
                    write_xml_string(log_obstk, msg->fmt_str + i, FALSE);
                    break;
                }

                /* Parse output format */
                switch (msg->fmt_str[i + 1])
                {
                    case 'c':
                    {
                        uint32_t val;
                        char     c_buf[2] = {};

                        val = ntohl(*(uint32_t *)arg->val);
                        if (val > UCHAR_MAX)
                        {
                            obstack_printf(log_obstk, "&lt;0x%08x&gt;",
                                           val);
                        }
                        else
                        {
                            c_buf[0] = (char)val;
                            write_xml_string(log_obstk, c_buf, FALSE);
                        }

                        i++;

                        continue;
                    }

                    case 'd':
                    case 'u':
                    case 'o':
                    case 'x':
                    case 'X':
                    {
                        char  format[3] = {'%', msg->fmt_str[i + 1], '\0'};

                        *((uint32_t *)arg->val) =
                        ntohl(*(uint32_t *)arg->val);

                        obstack_printf(log_obstk, format,
                                       *((uint32_t *)arg->val));
                        i++;

                        continue;
                    }

                    case 'p':
                    {
                        uint32_t val;
                        int      j;

                        /* Address should be 4 bytes aligned */
                        assert(arg->len % 4 == 0);

                        obstack_grow(log_obstk, "0x", strlen("0x"));
                        for (j = 0; j < arg->len / 4; j++)
                        {
                            val = *(((uint32_t *)arg->val) + j);

                            /* Skip not trailing zero words */
                            if (val == 0 && (j + 1) < arg->len / 4)
                            {
                                continue;
                            }
                            val = ntohl(val);

                            obstack_printf(log_obstk, "%08x", val);
                        }

                        i++;

                        continue;
                    }

                    case 's':
                    {
                        write_xml_string(log_obstk, (const char *)arg->val,
                                         FALSE);
                        i++;

                        continue;
                    }

                    case 'r':
                    {
                        te_errno    err;
                        const char *src;

                        err = *((uint32_t *)arg->val) =
                        ntohl(*(uint32_t *)arg->val);

                        src = te_rc_mod2str(err);
                        if (strlen(src) > 0)
                        {
                            write_xml_string(log_obstk, src, FALSE);
                            obstack_1grow(log_obstk, '-');
                        }
                        write_xml_string(log_obstk, te_rc_err2str(err),
                                         FALSE);
                        i++;

                        continue;
                    }

                    case 'T':
                    {
                        int  j;
                        int  n_tuples;
                        int  tuple_width;
                        int  cur_pos = 0;
                        char one_byte_str[3];
                        int  default_format = FALSE;
                        int  k;

                        /* @todo think of better way to implement this! */
                        if (strstr(msg->fmt_str + i, "%Tf") ==
                            (msg->fmt_str + i))
                        {
                            /* Strart file tag */
                            obstack_printf(log_obstk,
                                           "<file name=\"%s\">", "TODO");
                            write_xml_string(log_obstk,
                                             (const char *)arg->val, FALSE);
                            /* End file tag */
                            obstack_grow(log_obstk, "</file>",
                                         strlen("</file>"));

                            /* shift to the end of "%Tf" */
                            i += 2;
                            continue;
                        }


                        /*
                         * %Tm[[n].[w]] - memory dump, n - the number of
                         * elements after which "\n" is to be inserted,
                         * w - width (in bytes) of the element.
                         */
                        if (strstr(msg->fmt_str + i, "%Tm") !=
                            (msg->fmt_str + i))
                        {
                            /* Invalid format just output as it is */
                            fprintf(stderr, "WARNING: Invalid format for "
                                    "%%T specificator\n");
                            print_message_info(msg);
                            break;
                        }

                        obstack_grow(log_obstk,
                                     "<mem-dump>", strlen("<mem-dump>"));
                        if (sscanf(msg->fmt_str + i, "%%Tm[[%d].[%d]]",
                                   &n_tuples, &tuple_width) != 2)
                        {
                            default_format = TRUE;
                            tuple_width = 1;
                            n_tuples = 16; /* @todo remove hardcode */
                        }

                        while (cur_pos < arg->len)
                        {
                            obstack_grow(log_obstk, "<row>",
                                         strlen("<row>"));
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
                                    snprintf(one_byte_str,
                                             sizeof(one_byte_str),
                                             "%02X", *(arg->val + cur_pos));
                                    obstack_grow(log_obstk,
                                                 one_byte_str, 2);
                                }
                                /* End a block in a row */
                                obstack_grow(log_obstk, "</elem>",
                                             strlen("</elem>"));
                            }
                            /* End a memory table row */
                            obstack_grow(log_obstk, "</row>",
                                         strlen("</row>"));
                        }
                        obstack_grow(log_obstk, "</mem-dump>",
                                     strlen("</mem-dump>"));

                        /* shift to the end of "%Tm" */
                        i += 2;

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
                } /* switch */
            }

            switch (msg->fmt_str[i])
            {
                case '\r':
                    if (i > 0 && msg->fmt_str[i - 1] == '\n')
                    {
                        /*
                         * Skip \r after \n, because it does not bring any
                         * formating, but just follows after ]n on some
                         * systems
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
                    if (msg->fmt_str[i] == '\t' || isprint(msg->fmt_str[i]))
                    {
                        obstack_1grow(log_obstk, msg->fmt_str[i]);
                    }
                    else
                    {
                        obstack_printf(log_obstk, "&lt;0x%02x&gt;",
                                       (unsigned char)msg->fmt_str[i]);
                    }
                    break;
            }

        } /* for */
    } /* if (msg->txt_msg != NULL) */

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
