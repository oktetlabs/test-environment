/** @file
 * @brief Test Environment: RGT Core
 * 
 * Implementation of high level functions of message processing.
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

#include <obstack.h>
#include <stdio.h>

#ifdef HAVE_CTYPE_H
#include <ctype.h>
#endif

#include "logger_defs.h"
#include "log_msg.h"
#include "flow_tree.h"
#include "log_format.h"
#include "filter.h"
#include "rgt_common.h"
#include "memory.h"

f_process_ctrl_log_msg ctrl_msg_proc[CTRL_EVT_LAST][NT_LAST];
f_process_reg_log_msg  reg_msg_proc;

/* External declaration */
static node_info_t *create_node_by_msg(log_msg *msg, node_type_t type, 
                                       int node_id, int parent_id);

/* See the description in log_msg.h */
int
rgt_process_tester_control_message(log_msg *msg)
{
    int          node_id;
    int          parent_id;
    msg_arg     *arg;
    node_info_t *node;
    node_type_t  node_type;
    int          err_code = ESUCCESS;
    const char  *fmt_str = msg->fmt_str;

    enum ctrl_event_type evt_type;
    enum result_status   res;
    size_t               len;


    /* Get Parent ID and Node ID */
    if (sscanf(fmt_str, "%u %u", &parent_id, &node_id) < 2)
    {
        /* Unrecognized format */
        FMT_TRACE("Unrecognized message format (%s)", msg->fmt_str);
        THROW_EXCEPTION;
    }
    while (isdigit(*fmt_str) || isspace(*fmt_str))
    {
        fmt_str++;
    }

    /*
     * Determine type of message:
     * All control messages start from "%d %d " character sequence,
     * then message type is followed.
     * For more information see OKT-HLD-0000095-TE_TS.
     */
    if ((node_type = NT_TEST,
         strncmp(fmt_str, CNTR_MSG_TEST,
                 strlen(CNTR_MSG_TEST)) == 0) ||
        (node_type = NT_PACKAGE,
         strncmp(fmt_str, CNTR_MSG_PACKAGE,
                 strlen(CNTR_MSG_PACKAGE)) == 0) ||
        (node_type = NT_SESSION,
         strncmp(fmt_str, CNTR_MSG_SESSION,
                 strlen(CNTR_MSG_SESSION)) == 0))
    {
        if ((node = create_node_by_msg(msg, node_type,
                                       node_id, parent_id)) == NULL)
        {
            THROW_EXCEPTION;
        }

        if (flow_tree_add_node(parent_id, node_id, node_type,
                               node->descr.name, node->start_ts,
                               node, &err_code) == NULL)
        {
            free_node_info(node);
            free_log_msg(msg);
            if (err_code != ESUCCESS)
                THROW_EXCEPTION;
            return ESUCCESS;
        }

        evt_type = CTRL_EVT_START;
    }
#define RGT_CMP_RESULT(_result) \
    (res = RES_STATUS_##_result, len = strlen(#_result), \
     strncmp(fmt_str, #_result, len) == 0)
    else if ((RGT_CMP_RESULT(PASSED) || RGT_CMP_RESULT(KILLED) ||
              RGT_CMP_RESULT(CORED) || RGT_CMP_RESULT(SKIPPED) ||
              RGT_CMP_RESULT(FAKED) || RGT_CMP_RESULT(EMPTY) ||
              RGT_CMP_RESULT(FAILED) || RGT_CMP_RESULT(INCOMPLETE) ||
              (res = (enum result_status)-1, len = strlen("%s"),
               strncmp(fmt_str, "%s", len) == 0)) &&
             (fmt_str[len] == '\0' || isspace(fmt_str[len])))
#undef RGT_CMP_RESULT
    {
        fmt_str += len;

        if (res == (enum result_status)-1)
        {
            if ((arg = get_next_arg(msg)) == NULL)
            {
                FMT_TRACE("Missing argument with test status as "
                          "string (format string is '%s')", msg->fmt_str);
                THROW_EXCEPTION;
            }
#define RGT_CMP_RESULT(_result) \
    (res = RES_STATUS_##_result, len = strlen(#_result), \
     (size_t)arg->len != len || memcmp(arg->val, #_result, len) != 0)
            if (RGT_CMP_RESULT(PASSED) && RGT_CMP_RESULT(KILLED) &&
                RGT_CMP_RESULT(CORED) && RGT_CMP_RESULT(SKIPPED) &&
                RGT_CMP_RESULT(FAKED) && RGT_CMP_RESULT(EMPTY) &&
                RGT_CMP_RESULT(FAILED) && RGT_CMP_RESULT(INCOMPLETE))
            {
                FMT_TRACE("Unexpected test status '%s'", arg->val);
                THROW_EXCEPTION;
            }
#undef RGT_CMP_RESULT
        }

        while (*fmt_str != '\0' && isspace(*fmt_str))
            fmt_str++;

        if ((node = flow_tree_close_node(parent_id, node_id, 
                                         msg->timestamp,
                                         &err_code)) == NULL)
        {
            free_log_msg(msg);
            if (err_code != ESUCCESS)
                THROW_EXCEPTION;

            return ESUCCESS;
        }

        memcpy(node->end_ts, msg->timestamp, sizeof(node->end_ts));
        node->result.status = res;

        if (*fmt_str != '\0')
        {
            if (strcmp(fmt_str, "%s") != 0)
            {
                FMT_TRACE("Unrecognized message format (%s) - "
                          "only %%s is expected after test status",
                          msg->fmt_str);
                THROW_EXCEPTION;
            }
            if ((arg = get_next_arg(msg)) == NULL)
            {
                FMT_TRACE("Missing argument with test error string "
                          "for format string '%s'", msg->fmt_str);
                THROW_EXCEPTION;
            }
            node->result.err = node_info_obstack_copy0(arg->val,
                                                       arg->len);
        }

        evt_type = CTRL_EVT_END;
    }
    else
    {
        /* Unrecognized format */
        FMT_TRACE("Unrecognized message format (%s)", msg->fmt_str);
        THROW_EXCEPTION;
    }

    free_log_msg(msg);

    if (rgt_ctx.op_mode == RGT_OP_MODE_LIVE)
        ctrl_msg_proc[evt_type][node->type](node, NULL);

    return ESUCCESS;
}

/* See the description in log_msg.h */
void
rgt_process_regular_message(log_msg *msg)
{
    if (rgt_ctx.op_mode == RGT_OP_MODE_LIVE)
    {
        /* 
         * We should only check if there is at least one node 
         * message is linked with
         */
        if (flow_tree_filter_message(msg) == NFMODE_INCLUDE)
        {
            /*
             * Check filter by level, entity name, user name and
             * timestamp.
             */
            if (rgt_filter_check_message(msg->entity, msg->user,
                                         msg->level, msg->timestamp,
                                         &msg->flags) == NFMODE_INCLUDE)
            {
                reg_msg_proc(msg);
            }
        }
    }
    else
    {
        /* 
         * At first we should check filter by entity name, user name and 
         * timestamp. Then we can attach message to execution flow tree.
         */
        if (rgt_filter_check_message(msg->entity, msg->user,
                                     msg->level, msg->timestamp,
                                     &msg->flags) == NFMODE_INCLUDE)
        {
            /* Don't expand message, but just attach it to the flow tree */
            flow_tree_attach_message(msg);
            return;
        }
    }

    free_log_msg(msg);

    return;
}

/* See the description in log_msg.h */
void
rgt_emulate_accurate_close(uint32_t *latest_ts)
{
    node_id_t id;
    node_id_t parent_id;
    log_msg *msg;
    int      n;
    char     fmt_str[128];

    while (flow_tree_get_close_node(&id, &parent_id) == 0)
    {
        msg = alloc_log_msg();

        /*
         * Fill in all the necessary fields to pretend being
         * the message from Tester.
         */
        n = snprintf(fmt_str, sizeof(fmt_str), "%u %u %s",
                     parent_id, id, "INCOMPLETE");
        assert(n < (int)sizeof(fmt_str));

        msg->id = id;
        msg->entity = obstack_copy(msg->obstk, TE_LOG_CMSG_ENTITY_TESTER,
                                 strlen(TE_LOG_CMSG_ENTITY_TESTER) + 1);
        msg->user = obstack_copy(msg->obstk, TE_LOG_CMSG_USER,
                                 strlen(TE_LOG_CMSG_USER) + 1);

        memcpy(&(msg->timestamp), latest_ts, sizeof(msg->timestamp));
        msg->fmt_str = obstack_copy(msg->obstk, fmt_str, n + 1);

        msg->level = TE_LL_RING;
        msg->level_str = te_log_level2str(msg->level);
        assert(msg->level_str != NULL);

        rgt_process_tester_control_message(msg);
    }
}

void
log_msg_init_arg(log_msg *msg)
{
    msg->cur_arg = msg->args;
}

/**
 * Return pointer to the log message argument. The first call of the
 * function returns pointer to the first argument. The second call
 * returns pointer to the second argument and so on.
 *
 * @param msg       Message which argument we are going to obtain
 *
 * @return Pointer to an argument of a message
 */
msg_arg *
get_next_arg(log_msg *msg)
{
    msg_arg *arg;
    
    arg = msg->cur_arg;
    
    if (arg != NULL)
    {
        msg->cur_arg = arg->next;
    }

    return arg;
}

/**
 * Processes event occured on a node of the flow tree.
 * Currently the only event that is actually processed is MORE_BRANCHES.
 *
 * @param type      Type of a node on which an event has occured.
 * @param evt       Type of an event.
 * @param node      User-specific data that is passed on creation
 *                  of the node.
 */
void
rgt_process_event(node_type_t type, enum event_type evt, node_info_t *node)
{
    if (node == NULL)
        return;

    switch (type)
    {
        case NT_SESSION:
            if (evt == MORE_BRANCHES)
                node->descr.n_branches += 1;
            break;

        default:
            break;
    }
}

/*************************************************************************/
/*           Static routines                                             */
/*************************************************************************/

/**
 * Creates node_info_t structure for an appropriate control log message.
 * 
 * @param  msg        Control log message
 * @param  type       Type of control event
 * @param  node_id    ID of the processed node
 * @param  parent_id  In of the parent node of the processed node
 *
 * @return Pointer to created node_info_t structure
 */
static node_info_t *
create_node_by_msg(log_msg *msg, node_type_t type,
                   int node_id, int parent_id)
{
    node_info_t *node;
    msg_arg     *arg;
    param      **p_prm;
    const char  *node_type_str;
    const char  *fmt_str;

    if ((node = alloc_node_info()) == NULL)
    {
        TRACE("Not enough memory");
        return NULL;
    }
    memset(node, 0, sizeof(*node));

    memcpy(node->start_ts, msg->timestamp, sizeof(node->start_ts));
    node->type = type;
    node->result.err = NULL;

    node_type_str = CNTR_BIN2STR(type);
    if ((fmt_str = strstr(msg->fmt_str, node_type_str)) == NULL)
    {
        assert(0);
        return NULL;
    }
    fmt_str += strlen(node_type_str);

#define SKIP_SPACES(_str) \
    do {                            \
        /* Skip all the spaces */   \
        while (isspace(*(_str)))    \
            (_str)++;               \
    } while (0)

    SKIP_SPACES(fmt_str);

    if (strncmp(fmt_str, "%s", strlen("%s")) == 0)
    {
        /* Process "name" clause */
        if ((arg = get_next_arg(msg)) == NULL)
        {
            FMT_TRACE("Missing \"name\" argument in control message "
                      "%s (%d %d)", msg->fmt_str, node_id, parent_id);
            return NULL;
        }
        node->descr.name = (char *)node_info_obstack_copy0(arg->val,
                                                           arg->len);

        fmt_str += strlen("%s");

        SKIP_SPACES(fmt_str);
    }


    if (strncmp(fmt_str, "\"%s\"", strlen("\"%s\"")) == 0)
    {
        /* Process "objective" clause */
        if ((arg = get_next_arg(msg)) == NULL)
        {
            FMT_TRACE("Missing \"objective\" argument in control message "
                      "%s (%d %d)", msg->fmt_str, node_id, parent_id);
            return NULL;
        }
        if (arg->len > 0)
        {
            node->descr.objective = 
                (char *)node_info_obstack_copy0(arg->val, arg->len);
        }

        fmt_str += strlen("\"%s\"");

        SKIP_SPACES(fmt_str);
    }

    if (strncmp(fmt_str, "PAGE", strlen("PAGE")) == 0)
    {
        /* Process "page" clause */
        fmt_str += strlen("PAGE");

        SKIP_SPACES(fmt_str);

        if (strncmp(fmt_str, "%s", strlen("%s")) != 0)
        {
            FMT_TRACE("Missing \"%%s\" after PAGE clause in "
                      "control message %s (%d %d)",
                      msg->fmt_str, node_id, parent_id);
            return NULL;
        }

        if ((arg = get_next_arg(msg)) == NULL)
        {
            FMT_TRACE("Missing \"page\" argument in control message "
                      "%s (%d %d)", msg->fmt_str, node_id, parent_id);
            return NULL;
        }
        node->descr.page =
            (char *)node_info_obstack_copy0(arg->val, arg->len);

        SKIP_SPACES(node->descr.page);

        fmt_str += strlen("%s");

        SKIP_SPACES(fmt_str);
    }

    if (strncmp(fmt_str, "AUTHORS", strlen("AUTHORS")) == 0)
    {
        /* Process "authors" clause */
        fmt_str += strlen("AUTHORS");

        SKIP_SPACES(fmt_str);

        if (strncmp(fmt_str, "%s", strlen("%s")) != 0)
        {
            FMT_TRACE("Missing \"%%s\" after AUTHORS clause in "
                      "control message %s (%d %d)",
                      msg->fmt_str, node_id, parent_id);
            return NULL;
        }

        if ((arg = get_next_arg(msg)) == NULL)
        {
            FMT_TRACE("Missing \"authors\" argument in control message "
                      "%s (%d %d)", msg->fmt_str, node_id, parent_id);
            return NULL;
        }
        node->descr.authors =
            (char *)node_info_obstack_copy0(arg->val, arg->len);

        SKIP_SPACES(node->descr.authors);

        fmt_str += strlen("%s");

        SKIP_SPACES(fmt_str);
    }

    p_prm = &(node->params);
    if (strncmp(fmt_str, "ARGs", strlen("ARGs")) == 0)
    {
        char     *param_lst;
        size_t    seg_len;

        /* Process "args" clause */
        fmt_str += strlen("ARGs");

        SKIP_SPACES(fmt_str);

        if (strncmp(fmt_str, "%s", strlen("%s")) != 0)
        {
            FMT_TRACE("Missing \"%%s\" after ARGs clause in "
                      "control message %s (%d %d)",
                      msg->fmt_str, node_id, parent_id);
            return NULL;
        }

        if ((arg = get_next_arg(msg)) == NULL)
        {
            FMT_TRACE("Missing \"args\" argument in control message "
                      "%s (%d %d)", msg->fmt_str, node_id, parent_id);
            return NULL;
        }

        param_lst = (char *)node_info_obstack_copy0(arg->val, arg->len);

        while (*param_lst != '\0')
        {
            SKIP_SPACES(param_lst);
            
            *p_prm = (param *)node_info_obstack_alloc(sizeof(param));
            (*p_prm)->name = param_lst;
            if ((param_lst = strchr(param_lst, '=')) == NULL)
            {
                FMT_TRACE("The value of %s \"%s\" parameters is incorrect "
                          "in control message %s (%d %d)",
                          node_type_str,
                          (node->descr.name != NULL) ?
                                node->descr.name : "<unnamed>",
                          msg->fmt_str, node_id, parent_id);
                return NULL;
            }
            *(param_lst++) = '\0';
            /* Parameter value should start with quote mark */
            if (*(param_lst++) != '"')
            {
                FMT_TRACE("Missing quote mark at the beginning of "
                          "%s parameter value in control message %s "
                          "(%d %d)",
                          (*p_prm)->name, msg->fmt_str, node_id,
                          parent_id);
                return NULL;
            }
            (*p_prm)->val = param_lst;

            /* Search for back slash or quotation mark */
            while ((seg_len = strcspn(param_lst, "\\\""),
                    param_lst += seg_len,
                    *param_lst != '\"'))
            {
                if (*param_lst == '\0')
                {
                    FMT_TRACE("The value of %s \"%s\" parameters is "
                              "incorrect in control message %s (%d %d): "
                              "[there is no trailing quotation mark]",
                              node_type_str,
                              (node->descr.name != NULL) ?
                                    node->descr.name : "<unnamed>",
                               msg->fmt_str, node_id, parent_id);
                    return NULL;
                }
                assert(*param_lst == '\\');

                /*
                 * After back slash it is only possible to meet
                 * a quotation mark or a back slash.
                 * We should strip an extra back slash, which is 
                 * used to perceive '"' and '\' as a characters in 
                 * attribute's value.
                 */
                if (*(param_lst + 1) != '\\' && *(param_lst + 1) != '\"')
                {
                    FMT_TRACE("The value of %s \"%s\" parameters is "
                              "incorrect in control message %s (%d %d): "
                              "[back slash is followed by '%c' character]",
                              node_type_str,
                              (node->descr.name != NULL) ?
                                    node->descr.name : "<unnamed>",
                               msg->fmt_str, node_id, parent_id,
                               *(param_lst + 1));
                    return NULL;
                }

                /*
                 * We found an auxiliary back slash, which should not be
                 * included as a part of parameter value.
                 * The simplest but not efficient way to delete it in
                 * a stream of characters is to zap it by shifting the rest
                 * of the string to the left on one position.
                 *
                 * @todo think of some more efficient algorithm if that
                 * annoys you.
                 */
                memmove(param_lst, param_lst + 1,
                        strlen(param_lst + 1) + 1);
                /*
                 * I intentionally used "strlen(param_lst + 1) + 1" 
                 * construction instead of simpler "strlen(param_lst)"
                 * in order to be more understandable.
                 */

                param_lst++;
            }
            assert(*param_lst == '\"');

            *(param_lst++) = '\0';
            p_prm = &((*p_prm)->next);
        }
    }
    *p_prm = NULL;

#undef SKIP_SPACES

    return node;
}
