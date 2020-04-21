/** @file
 * @brief Test Environment: RGT Core
 *
 * Implementation of high level functions of message processing.
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

#include <obstack.h>
#include <stdio.h>

#include <jansson.h>

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

#include "te_string.h"

f_process_ctrl_log_msg ctrl_msg_proc[CTRL_EVT_LAST][NT_LAST];
f_process_reg_log_msg  reg_msg_proc;
f_process_log_root     log_root_proc[CTRL_EVT_LAST];

/* External declaration */
static node_info_t *create_node_by_msg_json(json_t *json, uint32_t *ts,
                                            node_type_t type);
static node_info_t *create_node_by_msg(log_msg *msg, node_type_t type,
                                       int node_id, int parent_id);

/* Process Tester Control messages in JSON format */
static int
rgt_process_tester_control_message_json(log_msg *msg)
{
    int          node_id;
    int          parent_id;
    msg_arg     *arg;
    node_info_t *node = NULL;
    node_type_t  node_type;
    int          err_code = ESUCCESS;

    enum ctrl_event_type evt_type;
    enum result_status   res;

    const char *type   = NULL;
    const char *status = NULL;
    const char *error = NULL;

    json_t       *msg_json;
    json_error_t  json_error;

    arg = get_next_arg(msg);
    if (arg == NULL)
    {
        TRACE("Message seems to be JSON-formatted, but no argument given\n");
        THROW_EXCEPTION;
    }

    /* Parse the message body */
    msg_json = json_loads((const char *)arg->val, 0, &json_error);
    if (msg_json == NULL)
    {
        FMT_TRACE("Error parsing JSON log message: %s (line %d, column %d)",
                  json_error.text, json_error.line, json_error.column);
        THROW_EXCEPTION;
    }

    /* Unpack the JSON object */
    err_code = json_unpack_ex(msg_json, &json_error, 0, "{s:i, s:i, s?s, s?s, s?s}",
                              "id",     &node_id,
                              "parent", &parent_id,
                              "type",   &type,
                              "status", &status,
                              "error",  &error);
    if (err_code != 0)
    {
        FMT_TRACE("Error unpacking JSON log message: %s (line %d, column %d)",
                  json_error.text, json_error.line, json_error.column);
        THROW_EXCEPTION;
    }

    if (status == NULL)
    {
        evt_type = CTRL_EVT_START;

        if (strcmp(type, CNTR_MSG_TEST) == 0)
        {
            node_type = NT_TEST;
        }
        else if (strcmp(type, CNTR_MSG_PACKAGE) == 0)
        {
            node_type = NT_PACKAGE;
        }
        else if (strcmp(type, CNTR_MSG_SESSION) == 0)
        {
            node_type = NT_SESSION;
        }
        else
        {
            FMT_TRACE("Unknown entity type: %s", type);
            free_log_msg(msg);
            json_decref(msg_json);
            THROW_EXCEPTION;
        }

        if ((node = create_node_by_msg_json(msg_json, msg->timestamp, node_type)) == NULL)
        {
            free_log_msg(msg);
            json_decref(msg_json);
            THROW_EXCEPTION;
        }

        if (flow_tree_add_node(parent_id, node_id, node_type,
                               node->descr.name, node->start_ts,
                               node, &err_code) == NULL)
        {
            free_node_info(node);
            free_log_msg(msg);
            json_decref(msg_json);
            if (err_code != ESUCCESS)
                THROW_EXCEPTION;
            return ESUCCESS;
        }
    }
    else
    {
        evt_type = CTRL_EVT_END;

#define RGT_CMP_RESULT(_result) \
    (res = RES_STATUS_##_result, strcmp(status, #_result) != 0)
        if (RGT_CMP_RESULT(PASSED) && RGT_CMP_RESULT(KILLED) &&
            RGT_CMP_RESULT(CORED) && RGT_CMP_RESULT(SKIPPED) &&
            RGT_CMP_RESULT(FAKED) && RGT_CMP_RESULT(EMPTY) &&
            RGT_CMP_RESULT(FAILED) && RGT_CMP_RESULT(INCOMPLETE))
        {
            FMT_TRACE("Unexpected test status '%s'", status);
            free_log_msg(msg);
            json_decref(msg_json);
            THROW_EXCEPTION;
        }
#undef RGT_CMP_RESULT

        if ((node = flow_tree_close_node(parent_id, node_id,
                                         msg->timestamp,
                                         &err_code)) == NULL)
        {
            free_log_msg(msg);
            json_decref(msg_json);
            if (err_code != ESUCCESS)
                THROW_EXCEPTION;

            return ESUCCESS;
        }

        memcpy(node->end_ts, msg->timestamp, sizeof(node->end_ts));
        node->result.status = res;

        if (error != NULL)
            node->result.err = node_info_obstack_copy0(error, strlen(error));
    }

    free_log_msg(msg);
    json_decref(msg_json);

    if (rgt_ctx.op_mode == RGT_OP_MODE_LIVE ||
        rgt_ctx.op_mode == RGT_OP_MODE_INDEX)
    {
        if (ctrl_msg_proc[evt_type][node->type] != NULL)
            ctrl_msg_proc[evt_type][node->type](node, NULL);
    }

    return ESUCCESS;
}

/* Process Tester Control messages in plain-text format */
static int
rgt_process_tester_control_message_text(log_msg *msg)
{
    int          node_id;
    int          parent_id;
    msg_arg     *arg;
    node_info_t *node = NULL;
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
     * For more information see OKTL-0000593.
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

    if (rgt_ctx.op_mode == RGT_OP_MODE_LIVE ||
        rgt_ctx.op_mode == RGT_OP_MODE_INDEX)
    {
        if (ctrl_msg_proc[evt_type][node->type] != NULL)
            ctrl_msg_proc[evt_type][node->type](node, NULL);
    }

    return ESUCCESS;
}

/* See the description in log_msg.h */
int
rgt_process_tester_control_message(log_msg *msg)
{
    if ((msg->level & TE_LL_MI) != 0)
        return rgt_process_tester_control_message_json(msg);
    else
        return rgt_process_tester_control_message_text(msg);
}

/* See the description in log_msg.h */
void
rgt_process_regular_message(log_msg *msg)
{
    if (rgt_ctx.op_mode == RGT_OP_MODE_LIVE ||
        rgt_ctx.op_mode == RGT_OP_MODE_INDEX)
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
                if (reg_msg_proc != NULL)
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
 * @param  json       Control log message body
 * @param  ts         Control log message timestamp
 * @param  type       Type of control event
 *
 * @return Pointer to created node_info_t structure
 */
static node_info_t *
create_node_by_msg_json(json_t *msg, uint32_t *ts, node_type_t type)
{
    node_info_t *node;
    param      **p_prm;
    int          ret = 0;
    json_error_t err;

    json_t     *authors = NULL;
    json_t     *args = NULL;
    const char *name = NULL;
    const char *objective = NULL;
    const char *page = NULL;
    const char *hash = NULL;
    int         tin = TE_TIN_INVALID;

    if ((node = alloc_node_info()) == NULL)
    {
        TRACE("Not enough memory");
        return NULL;
    }
    memset(node, 0, sizeof(*node));

    memcpy(node->start_ts, ts, sizeof(node->start_ts));
    node->type = type;
    node->result.err = NULL;

    ret = json_unpack_ex(msg, &err, 0, "{s:i, s:i, s?s, s?s, s?s, s?s, s?i,"
                                       " s?o, s?o}",
                         "id", &node->node_id,
                         "parent", &node->parent_id,
                         "name", &name,
                         "objective", &objective,
                         "page", &page,
                         "hash", &hash,
                         "tin", &tin,
                         "authors", &authors,
                         "args", &args);
    if (ret != 0)
    {
        FMT_TRACE("Error unpacking JSON log message: %s (line %d, column %d)",
                  err.text, err.line, err.column);
        free_node_info(node);
        return NULL;
    }

    if (name != NULL)
        node->descr.name = node_info_obstack_copy0(name, strlen(name));

    if (objective != NULL)
        node->descr.objective =
            node_info_obstack_copy0(objective, strlen(objective));

    node->descr.tin = tin;

    if (page != NULL)
        node->descr.page = node_info_obstack_copy0(page, strlen(page));

    if (authors != NULL)
    {
        size_t       index;
        json_t      *author;
        const char  *name = NULL;
        const char  *email = NULL;
        te_string    authors_str = TE_STRING_INIT;

        json_array_foreach(authors, index, author)
        {
            ret = json_unpack_ex(author, &err, 0, "{s?s, s?s !}",
                                 "name", &name,
                                 "email", &email);
            if (ret != 0)
                FMT_TRACE("Error unpacking authors: %s (line %d, column %d)",
                          err.text, err.line, err.column);
            else
                te_string_append(&authors_str, "%s mailto:%s", name, email);
        }

        node->descr.authors =
            node_info_obstack_copy0(authors_str.ptr, authors_str.len);

        te_string_free(&authors_str);
    }

    if (hash != NULL)
        node->descr.hash = node_info_obstack_copy0(hash, strlen(hash));

    if (args != NULL)
    {
        size_t      index;
        json_t     *pair;
        const char *name = NULL;
        const char *value = NULL;

        p_prm = &(node->params);
        json_array_foreach(args, index, pair)
        {
            ret = json_unpack_ex(pair, &err, 0, "[ss!]", &name, &value);
            if (ret != 0)
            {
                FMT_TRACE("Error unpacking authors: %s (line %d, column %d)",
                          err.text, err.line, err.column);
            }
            else
            {
                *p_prm = (param *)node_info_obstack_alloc(sizeof(param));
                (*p_prm)->name =
                    node_info_obstack_copy0(name, sizeof(name));
                (*p_prm)->val =
                    node_info_obstack_copy0(value, sizeof(value));
                p_prm = &((*p_prm)->next);
            }
        }
        *p_prm = NULL;
    }

    return node;
}

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

    node->parent_id = parent_id;
    node->node_id = node_id;

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

    if (strncmp(fmt_str, "TIN", strlen("TIN")) == 0)
    {
        /* Process "page" clause */
        fmt_str += strlen("TIN");

        SKIP_SPACES(fmt_str);

        if (sscanf(fmt_str, "%u ", &node->descr.tin) < 1)
        {
            FMT_TRACE("Missing test identification number after TIN "
                      "clause in control message '%s' (%d %d)",
                      msg->fmt_str, node_id, parent_id);
            return NULL;
        }

        while (isdigit(*fmt_str))
            fmt_str++;

        SKIP_SPACES(fmt_str);
    }
    else
    {
        node->descr.tin = TE_TIN_INVALID;
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

    if (strncmp(fmt_str, "HASH", strlen("HASH")) == 0)
    {
        /* Process "hash" clause */
        fmt_str += strlen("HASH");

        SKIP_SPACES(fmt_str);

        if (strncmp(fmt_str, "%s", strlen("%s")) != 0)
        {
            FMT_TRACE("Missing \"%%s\" after HASH clause in "
                      "control message %s (%d %d)",
                      msg->fmt_str, node_id, parent_id);
            return NULL;
        }

        if ((arg = get_next_arg(msg)) == NULL)
        {
            FMT_TRACE("Missing \"hash\" argument in control message "
                      "%s (%d %d)", msg->fmt_str, node_id, parent_id);
            return NULL;
        }
        node->descr.hash =
            (char *)node_info_obstack_copy0(arg->val, arg->len);

        SKIP_SPACES(fmt_str);

        fmt_str += strlen("%s");

        SKIP_SPACES(fmt_str);
    }
    else
    {
        node->descr.hash = NULL;
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


/* See description in log_msg.h */
void
rgt_expand_log_msg(log_msg *msg)
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
            if (msg->fmt_str[i + 1] == '%')
            {
                obstack_1grow(msg->obstk, msg->fmt_str[i]);
                i++;
                continue;
            }

            if ((arg = get_next_arg(msg)) == NULL)
            {
                /* Too few arguments in the message */
                /* Simply write the rest of format string to the log */
                obstack_grow(msg->obstk, msg->fmt_str + i,
                             strlen(msg->fmt_str + i));
                break;
            }

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
                    obstack_grow(msg->obstk, arg->val, arg->len);
                    i++;

                    continue;
                }

                case 'r':
                {
                    te_errno    err;
                    const char *src;
                    const char *err_str;

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

                case 'T':
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
                    if (strstr(msg->fmt_str + i, "%Tf") ==
                            (msg->fmt_str + i))
                    {
                        obstack_grow(msg->obstk, arg->val, arg->len);

                        /* shift to the end of "%Tf" */
                        i += 2;
                        continue;
                    }

/*
 *  %Tm[[n].[w]] - memory dump, n - the number of elements after
 *                 which "\n" is to be inserted , w - width (in bytes) of
 *                 the element.
 */

                    if (strstr(msg->fmt_str + i, "%Tm") !=
                            (msg->fmt_str + i))
                    {
                        /* Invalid format just output as it is */
                        fprintf(stderr, "WARNING: Invalid format for "
                                "%%t specificator\n");
                        break;
                    }

                    if (/* i > str_len - 10 || */
                        (rc = sscanf(msg->fmt_str + i, "%%Tm[[%d].[%d]]",
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

                    /* shift to the end of "%Tm" */
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

/* See description in the log_msg.h */
log_msg_ptr *
log_msg_ref(log_msg *msg)
{
    log_msg_ptr *ptr;

    ptr = alloc_log_msg_ptr();

    ptr->offset = rgt_ctx.rawlog_fpos;
    ptr->timestamp[0] = msg->timestamp[0];
    ptr->timestamp[1] = msg->timestamp[1];

    return ptr;
}

/* See description in the log_msg.h */
log_msg *
log_msg_read(log_msg_ptr *msg_ptr)
{
    log_msg *msg = NULL;

    fseeko(rgt_ctx.rawlog_fd, msg_ptr->offset, SEEK_SET);
    if (rgt_ctx.fetch_log_msg(&msg, &rgt_ctx) == 0)
    {
        FMT_TRACE("Failed to reload log message from %lld",
                  (long long int)msg_ptr->offset);
        THROW_EXCEPTION;
    }

    return msg;
}
