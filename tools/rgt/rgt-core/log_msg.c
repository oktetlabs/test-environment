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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <obstack.h>
#include <stdio.h>

#ifdef HAVE_CTYPE_H
#include <ctype.h>
#endif

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

/**
 * Defines type of control message.
 *
 * @param  msg   Pointer to control message to be processed.
 *
 * @se
 *    In the case of errors it frees log message and calls longjmp.
 */
int
rgt_process_control_message(log_msg *msg)
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

    if (msg->args_count < 2)
    {
        TRACE("Control message should contain at least two arguments");
        THROW_EXCEPTION;
    }

    /* Get Parent ID and Node ID */
    arg = get_next_arg(msg);
    assert(arg->len == sizeof(uint32_t));

    parent_id = *((int *)arg->val);
    parent_id = ntohl(parent_id);

    arg = get_next_arg(msg);
    assert(arg->len == sizeof(uint32_t));

    node_id = *((int *)arg->val);
    node_id = ntohl(node_id);

    /*
     * Determine type of message:
     * All control messages start from "%d %d " character sequence,
     * then message type is followed.
     * For more information see OKT-HLD-0000095-TE_TS.
     */
    if (strncmp(fmt_str, "%u %u ", strlen("%u %u ")) != 0)
    {
        /* Unrecognized format */
        FMT_TRACE("Unrecognized message format (%s)", msg->fmt_str);
        THROW_EXCEPTION;
    }
    fmt_str += strlen("%u %u ");

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
    else if ((res = RES_STATUS_PASSED, 
              strncmp(fmt_str, "PASSED", strlen("PASSED")) == 0) ||
             (res = RES_STATUS_KILLED, 
              strncmp(fmt_str, "KILLED", strlen("KILLED")) == 0) ||
             (res = RES_STATUS_DUMPED, 
              strncmp(fmt_str, "DUMPED", strlen("DUMPED")) == 0) ||
             (res = RES_STATUS_SKIPPED, 
              strncmp(fmt_str, "SKIPPED", strlen("SKIPPED")) == 0) ||
             (res = RES_STATUS_FAKED, 
              strncmp(fmt_str, "FAKED", strlen("FAKED")) == 0) ||
#if 1
             (res = RES_STATUS_PASSED, 
              strncmp(fmt_str, "EMPTY", strlen("EMPTY")) == 0) ||
#endif
             (res = RES_STATUS_FAILED, 
              strncmp(fmt_str, "FAILED", strlen("FAILED")) == 0))
    {
        /** @todo Process trailing "%s" */

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
        if (res == RES_STATUS_FAILED)
        {
            if ((arg = get_next_arg(msg)) != NULL)
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

    if (rgt_op_mode == RGT_OP_MODE_LIVE)
        ctrl_msg_proc[evt_type][node->type](node);

    return ESUCCESS;
}

/**
 * Filters log message and in the case of successful filtering processes
 * it. In the live mode it calls log message processing function and
 * frees message. In postponed mode it inserts message in the flow tree
 * object.
 *
 * @param msg   Log message to be processed
 *
 * @todo Don't free log message but rather use it for storing the next one.
 */
void
rgt_process_regular_message(log_msg *msg)
{
    if (rgt_op_mode == RGT_OP_MODE_LIVE)
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
            if (rgt_filter_check_message(msg->level, msg->entity, 
                    msg->user, msg->timestamp) == NFMODE_INCLUDE)
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
        if (rgt_filter_check_message(msg->level, msg->entity, 
                msg->user, msg->timestamp) == NFMODE_INCLUDE)
        {
            /* Don't expand message, but just attach it to the flow tree */
            flow_tree_attach_message(msg);
            return;
        }
    }

    free_log_msg(msg);

    return;
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
    
    /* Skip all the spaces */
    while (isspace(*fmt_str))
        fmt_str++;

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
    }

    /* Skip all the spaces */
    while (isspace(*fmt_str))
        fmt_str++;

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
    }
    
    /* Skip all the spaces */
    while (isspace(*fmt_str))
        fmt_str++;

    if (strncmp(fmt_str, "AUTHORS", strlen("AUTHORS")) == 0)
    {
        /* Process "authors" clause */
        fmt_str += strlen("AUTHORS");

        /* Skip all the spaces */
        while (isspace(*fmt_str))
            fmt_str++;

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
        node->descr.author =
            (char *)node_info_obstack_copy0(arg->val, arg->len);
        while (isspace(*node->descr.author))
            node->descr.author++;

        fmt_str += strlen("%s");
    }
    
    /* Skip all the spaces */
    while (isspace(*fmt_str))
        fmt_str++;

    p_prm = &(node->params);
    if (strncmp(fmt_str, "ARGs", strlen("ARGs")) == 0)
    {
        char *param_lst;

        /* Process "args" clause */
        fmt_str += strlen("ARGs");

        /* Skip all the spaces */
        while (isspace(*fmt_str))
            fmt_str++;

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
            /* Skip spaces */
            while (isspace(*param_lst))
                param_lst++;
            
            *p_prm = (param *)node_info_obstack_alloc(sizeof(param));
            (*p_prm)->name = param_lst;
            if ((param_lst = strchr(param_lst, '=')) == NULL)
            {
                FMT_TRACE("The value of %s parameters is incorrect "
                          "in control message %s (%d %d)",
                          node_type_str, msg->fmt_str, node_id, parent_id);
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
            /*
             * Go to the end of the parameter value - trailing quote mark.
             * @todo Currently, it is not allowed to have quote mark in the
             * parameter value, so be carefull with the parameter value.
             */
            if ((param_lst = strchr(param_lst, '"')) == NULL)
            {
                FMT_TRACE("The value of %s parameters is incorrect "
                          "in control message %s (%d %d)",
                          node_type_str, msg->fmt_str, node_id, parent_id);
                return NULL;
            }
            *(param_lst++) = '\0';
            p_prm = &((*p_prm)->next);
        }
    }
    *p_prm = NULL;

    return node;
}
