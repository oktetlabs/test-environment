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

#include "log_msg.h"
#include "flow_tree.h"
#include "log_format.h"
#include "filter.h"
#include "rgt_common.h"
#include "memory.h"

f_process_ctrl_log_msg ctrl_msg_proc[CTRL_EVT_LAST][NT_LAST];
f_process_reg_log_msg  reg_msg_proc;

/* External declaration */
static node_info *create_node_by_msg(log_msg *msg, enum node_type ntype, 
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
    int         node_id;
    int         parent_id;
    msg_arg    *arg;
    node_info  *node;
    int         err_code = ESUCCESS;

    enum ctrl_event_type evt_type = CTRL_EVT_START;
    enum result_status   res;

    if (msg->args_count < 2)
    {
        TRACE("Control message should contain at least two arguments");
        THROW_EXCEPTION;
    }

    /* Get parent id and test id */
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
     * All control messages start from "%T %T " character sequence,
     * then message type is followed.
     * For more information see OKT-HLD-0000095-TE_TS.
     */
#define PROCESS_NODE(_lable, _ntype, _name) \
    if (strncmp(msg->fmt_str, "%T %T " _lable " ",           \
                strlen("%T %T " _lable " ")) == 0)           \
    {                                                        \
        if ((node = create_node_by_msg(msg, _ntype, node_id, \
                                       parent_id)) == NULL)  \
            THROW_EXCEPTION;                                 \
                                                             \
        if (flow_tree_add_node(parent_id, node_id, _ntype,   \
                               _name, node->start_ts,        \
                               node, &err_code) == NULL)     \
        {                                                    \
            free_node_info(node);                            \
            free_log_msg(msg);                               \
            if (err_code != ESUCCESS)                        \
                THROW_EXCEPTION;                             \
            return ESUCCESS;                                 \
        }                                                    \
    }
    
    PROCESS_NODE("test", NT_TEST, node->node_specific.test.name)
    else 
    PROCESS_NODE("package", NT_PACKAGE, node->node_specific.pkg.name)
    else
    PROCESS_NODE("session", NT_SESSION, NULL)
    else if ((res = RES_STATUS_PASS, 
              strncmp(msg->fmt_str, "%T %T pass", strlen("%T %T pass")) == 0) ||
             (res = RES_STATUS_FAIL, 
              strncmp(msg->fmt_str, "%T %T fail", strlen("%T %T fail")) == 0))
    {
        if ((node = flow_tree_close_node(parent_id, node_id, 
                            msg->timestamp, &err_code)) == NULL)
        {
            free_log_msg(msg);
            if (err_code != ESUCCESS)
                THROW_EXCEPTION;

            return ESUCCESS;
        }

        evt_type = CTRL_EVT_END;

        memcpy(node->end_ts, msg->timestamp, sizeof(node->end_ts));
        node->result.status = res;
        if (res == RES_STATUS_FAIL)
        {
            if ((arg = get_next_arg(msg)) != NULL)
                node->result.err = node_info_obstack_copy0(arg->val, arg->len);
        }
    }
    else
    {
        /* Unrecognized format */
        FMT_TRACE("Unrecognized message format (%s)", msg->fmt_str);
        THROW_EXCEPTION;
    }

    free_log_msg(msg);

    if (rgt_op_mode == RGT_OP_MODE_LIVE)
        ctrl_msg_proc[evt_type][node->ntype](node);

    return ESUCCESS;
}

/**
 * Filters log message and in the case of successful filtering processes it.
 * In the live mode it calls log message processing function and frees message.
 * In postponed mode it inserts message in the flow tree object.
 *
 * @param  msg   Log message to be processed
 *
 * @return  Nothing
 *
 * @todo  Don't free log message but rather use it for storing the next one.
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
            /* Check filter by level, entity name, user name and timestamp */
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
 * Return pointer to the log message argument. The first call of the function
 * returns pointer to the first argument. The second call returns pointer to 
 * the second argument and so on.
 *
 * @param  msg  Message which argument we are going to obtain
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
 * @param ntype  Type of a node on which an event has occured.
 * @param evt    Type of an event.
 * @param node   User-specific data that is passed on  creation of the node.
 *
 * @return  Nothing.
 */
void
rgt_process_event(enum node_type ntype, enum event_type evt, node_info *node)
{
    if (node == NULL)
        return;

    switch (ntype)
    {
        case NT_SESSION:
            if (evt == MORE_BRANCHES)
                node->node_specific.sess.n_branches += 1;
            break;

        default:
            break;
    }
}

/******************************************************************************/
/*           Static routines                                                  */
/******************************************************************************/

/**
 * Creates node_info structure for an appropriate control log message.
 * 
 * @param  msg        Control log message
 * @param  ntype      Type of control event
 * @param  node_id    Id of the processed node
 * @param  parent_id  In of the parent node of the processed node
 *
 * @return Pointer to created node_info structure
 */
static node_info *
create_node_by_msg(log_msg *msg, enum node_type ntype,
                   int node_id, int parent_id)
{
    node_info  *node;
    char      **p_ptr[5];
    int         i, n = 0;
    msg_arg    *arg;
    param     **p_prm;
    
    struct node {
        const char *node_type;
        char *arguments[3];
    } nodes [] = {
        { "test", 
          { "name", "objective", "author" } },
        { "package", 
          { "name", "title", "author" } },
        { "session", 
          { "objective" } }
    };
    struct node *cur_node = NULL;

    if ((node = alloc_node_info()) == NULL)
    {
        TRACE("Not enough memory");
        return NULL;
    }

    memcpy(node->start_ts, msg->timestamp, sizeof(node->start_ts));
    node->ntype = ntype;
    node->result.err = NULL;

    switch (ntype)
    {
        case NT_TEST:
        {
            p_ptr[0] = &(node->node_specific.test.name);
            p_ptr[1] = &(node->node_specific.test.objective);
            p_ptr[2] = &(node->node_specific.test.author);
            cur_node = &(nodes[0]);
            n = 3;

            break;
        }
        
        case NT_PACKAGE:
        {
            p_ptr[0] = &(node->node_specific.pkg.name);
            p_ptr[1] = &(node->node_specific.pkg.title);
            p_ptr[2] = &(node->node_specific.pkg.author);
            cur_node = &(nodes[1]);
            n = 3;

            break;
        }
        
        case NT_SESSION:
        {
            node->node_specific.sess.n_branches = 0;
            p_ptr[0] = &(node->node_specific.sess.objective);
            cur_node = &(nodes[2]);
            n = 1;

            break;
        }
        
        default:
        {
            assert(0);
            break;
        }
    }
    
    for (i = 0; i < n; i++)
    {
        if ((arg = get_next_arg(msg)) == NULL)
        {
            FMT_TRACE("Missing argument %s in %s (%d, %d)", 
                      cur_node->arguments[i], cur_node->node_type,
                      node_id, parent_id);
            return NULL;
        }

        *(p_ptr[i]) = (char *)node_info_obstack_copy0(arg->val, arg->len);
    }

    /* Extract the list of parameters */
    p_prm = &(node->params);
    while ((arg = get_next_arg(msg)) != NULL)
    {
        *p_prm = (param *)node_info_obstack_alloc(sizeof(param));
        (*p_prm)->name = node_info_obstack_copy0(arg->val, arg->len);
        if ((arg = get_next_arg(msg)) == NULL)
        {
            FMT_TRACE("The value of the %s parameter is missed in %s (%d, %d)",
                      (*p_prm)->name, cur_node->node_type, node_id, parent_id);
            return NULL;
        }

        (*p_prm)->val = node_info_obstack_copy0(arg->val, arg->len);
        p_prm = &((*p_prm)->next);
    }
    *p_prm = NULL;

    return node;
}
