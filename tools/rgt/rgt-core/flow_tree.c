/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Test Environment: Interface for test execution flow.
 *
 * The module is responsible for keeping track of occured events and
 * checking if new events are legal.
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#include "rgt_common.h"

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include <glib.h>
#include <obstack.h>

#include <limits.h>

#ifndef PATH_MAX
#define PATH_MAX 1024
#endif

#include "flow_tree.h"
#include "log_msg.h"
#include "filter.h"
#include "log_format.h"
#include "memory.h"
#include "logger_defs.h"

#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

/* Define to 1 to enable rgt duration filter */
#define TE_RGT_USE_DURATION_FILTER 0

/**
 * Timestamp used the last time for message pointers offloading
 * into files (i.e. all the message pointers having timestamp no
 * greater than it were offloaded into files).
 */
static uint32_t last_offload_ts[2];

/*
 * Number of seconds (as measured by log messages timestamps) to
 * wait before repeating offloading of old message pointers to files
 * to reduce memory consumption.
 */
#define OFFLOAD_TIMEOUT    5

/*
 * Number of new messages which should be processed before we
 * repeat offloading old message pointers to files.
 */
#define OFFLOAD_MSG_NUM    1000

/*
 * All the message pointers having timestamp no greater than
 * current message timestamp - OFFLOAD_INTERVAL seconds should
 * be offloaded to files when we repeat offloading.
 * It should be less than OFFLOAD_TIMEOUT.
 */
#define OFFLOAD_INTERVAL   3

#ifdef RGT_PROF_STAT
/** Counter for the number of messages added into the queue tail. */
static unsigned long msg_put_to_tail;
/** Counter for the number of messages added by using cache information. */
static unsigned long msg_use_cache;
/**
 * Counter for the number of messages that added immediately after
 * cached message.
 */
static unsigned long msg_put_after_cache_quick;
/**
 * Counter for the number of messages that added after cached message,
 * but not immediately after it.
 */
static unsigned long msg_put_after_cache_slow;
/**
 * Counter for the number of messages added before cached message.
 */
static unsigned long msg_put_before_cache;
/**
 * Counter for the number of messages added with no cache information
 * available.
 */
static unsigned long msg_nocache;
/**
 * The number of times timestamp compare function was called
 */
static unsigned long timestamp_cmp_cnt;
#endif /* RGT_PROF_STAT */


/* Forward declaration */
struct node_t;

/**
 * Queue of message pointer queues which still have some entries not
 * offloaded into files.
 */
static GQueue *offload_queue = NULL;

/**
 * Status of the session branch
 *
 * @todo Remove this field from all sources.
 */
enum branch_status {
    BSTATUS_ACTIVE, /**< Branch is active:
                         There is at least one non closed node. */
    BSTATUS_IDLE,   /**< Branch is idle. */
};

/**< Session branch-specific information */
typedef struct branch_info {
    struct node_t      *first_el; /**< Pointer to the first element in
                                       the branch */
    struct node_t      *last_el;  /**< Pointer to the last element
                                       in the branch */
    enum branch_status  status;   /**< Status of the branch */
    uint32_t           *start_ts; /**< Branch start timestamp */
    uint32_t           *end_ts;   /**< Branch end timestamp */
} branch_info;

/** Node of the execution flow tree */
typedef struct node_t {
    struct node_t *parent; /**< Pointer to the parent node */
    struct node_t *prev;   /**< Pointer to the previous node
                                in execution order */
    struct node_t *next;   /**< For package node used as a pointer
                                to the session underneath */

    node_id_t      id;     /**< Node id (key that is used by g_hash
                                outines) */
    struct node_t *self;   /**< Pointer to this structure (self pointer) */
    char          *name;   /**< Node name */

    node_type_t         type; /**< Type of the node */
    enum node_fltr_mode fmode; /**< Filter mode for current node */
    uint32_t            start_ts[2]; /**< Node start timestamp */
    uint32_t            end_ts[2];   /**< Node end timestamp */

    msg_queue     msg_att;       /**< The queue of pointers to messages
                                      attached to the node */
    msg_queue     msg_after_att; /**< The queue of pointers to messages
                                      following the node */
    ctrl_msg_data ctrl_data;     /**< Data for callbacks processing
                                      control messages */

    int n_branches;        /**< Number of branches under the node */
    int n_active_branches; /**< Number of active branches */
    int more_branches;     /**< Flag that indicates if the node can append
                                more branches. It is set to false just
                                after first close event for any children
                                node arrives. */
    branch_info *branches; /**< Array of branches */

    void *user_data;  /**< User-specific data associated with a node */
} node_t;

/** Obstack structure that is used to allocation nodes of the tree */
static struct obstack *obstk;

static uint32_t zero_timestamp[2] = {0, 0};
static uint32_t max_timestamp[2]  = {UINT32_MAX, UINT32_MAX};

#define DEF_FILTER_MODE NFMODE_INCLUDE

#define FILL_BRANCH_INFO(node_ptr) \
    do {                                            \
        (node_ptr)->n_active_branches = 0;          \
        (node_ptr)->n_branches = 0;                 \
        (node_ptr)->more_branches = TRUE;           \
        (node_ptr)->branches = NULL;                \
    } while (0)

/* Function that is used for comparision of two timestamps */
static gint timestamp_cmp(gconstpointer a, gconstpointer b,
                          gpointer user_data);

/** Set of nodes that can accept a new child node */
static GHashTable *new_set;

/** Set of nodes that are waiting to be closed */
static GHashTable *close_set;

/**
 * Pointer to the root of the tree.
 * This node is created in flow_tree_init routine.
 */
static node_t *root = NULL;

/**
 * Initialize queue of message pointers.
 *
 * @param q     Queue of message pointers.
 */
static void
msg_queue_init(msg_queue *q)
{
    q->queue = g_queue_new();
    assert(q->queue != NULL);
    q->cache = NULL;
    q->offloaded = FALSE;
    memcpy(q->offload_ts, zero_timestamp, sizeof(q->offload_ts));
}

/**
 * Destroy queue of message pointers.
 *
 * @param q     Queue of message pointers.
 */
static void
msg_queue_destroy(msg_queue *q)
{
    log_msg_ptr *msg_ptr;

    if (q->queue != NULL)
    {
        while ((msg_ptr = g_queue_pop_head(q->queue)) != NULL)
        {
            free_log_msg_ptr(msg_ptr);
        }
    }
    g_queue_free(q->queue);
    q->queue = NULL;
    q->cache = NULL;
    memcpy(q->offload_ts, zero_timestamp, sizeof(q->offload_ts));
}

/**
 * Initializer for ctrl_msg_data.
 *
 * @param data      Pointer to ctrl_msg_data.
 */
static void
ctrl_msg_data_init(ctrl_msg_data *data)
{
    msg_queue_init(&data->verdicts);
    msg_queue_init(&data->artifacts);
}

/**
 * Release memory allocated for members of ctrl_msg_data structure.
 *
 * @param data      Pointer to ctrl_msg_data.
 */
static void
ctrl_msg_data_destroy(ctrl_msg_data *data)
{
    msg_queue_destroy(&data->verdicts);
    msg_queue_destroy(&data->artifacts);
}

/**
 * Initialize flow tree library:
 * Initializes obstack data structure, two hashes for set of nodes
 * ("new" set and "close" set) and creates root session node.
 *
 * @return Nothing
 *
 * @se Add session node with id equals to ROOT_ID into the tree and insert
 *     it into set of patential parent nodes (so-called "new set").
 */
void
flow_tree_init(void)
{
    obstk = obstack_initialize();

    root = (node_t *)obstack_alloc(obstk, sizeof(node_t));
    root->parent = root->prev = root->next = NULL;
    root->type = NT_SESSION;
    root->id   = FLOW_TREE_ROOT_ID;
    root->self = root;
    root->fmode = DEF_FILTER_MODE;
    root->name = "";
    msg_queue_init(&root->msg_att);
    msg_queue_init(&root->msg_after_att);
    ctrl_msg_data_init(&root->ctrl_data);
    root->user_data = NULL;

    memcpy(root->start_ts, zero_timestamp, sizeof(root->start_ts));
    memcpy(root->end_ts, max_timestamp, sizeof(root->end_ts));

    close_set = g_hash_table_new(g_int_hash, g_int_equal);
    new_set   = g_hash_table_new(g_int_hash, g_int_equal);

    g_hash_table_insert(new_set, &root->id, &root->self);
    FILL_BRANCH_INFO(root);

    offload_queue = g_queue_new();
    assert(offload_queue != NULL);
}

/**
 * Frees control messages linked with each node and regular messages that
 * belong to the node and goes after it.
 *
 * @param  cur_node  The root of the flow tree subtree for which
 *                   the operation is applied.
 *
 * @return  Nothing.
 */
static void
flow_tree_free_attachments(node_t *cur_node)
{
    if (cur_node == NULL)
        return;

    msg_queue_destroy(&cur_node->msg_att);
    msg_queue_destroy(&cur_node->msg_after_att);
    ctrl_msg_data_destroy(&cur_node->ctrl_data);

    if (cur_node->type != NT_TEST)
    {
        int i;

        for (i = 0; i < cur_node->n_branches; i++)
        {
            flow_tree_free_attachments(cur_node->branches[i].first_el);
        }
    }

    flow_tree_free_attachments(cur_node->next);
    /* We don't need to free user_data. This should be done be user */

    /* Free branch_info in sessions */
    if (cur_node->type != NT_TEST)
    {
        free(cur_node->branches);
    }
}

/**
 * Free all resources used by flow tree library.
 *
 * @return Nothing
 */
void
flow_tree_destroy(void)
{
    if (root == NULL)
        return;

    g_hash_table_destroy(new_set);
    g_hash_table_destroy(close_set);

    flow_tree_free_attachments(root);

    obstack_destroy(obstk);

    root = NULL;

    if (offload_queue != NULL)
    {
        g_queue_free(offload_queue);
        offload_queue = NULL;
    }
}

/**
 * Try to add a new node into the execution flow tree.
 *
 * @param  parent_id      Parent node id
 * @param  node_id        Id of the node to be created
 * @param  new_node_type  Type of the node to be created
 * @param  node_name      Name of the node
 * @param  timestamp      Timestamp
 * @param  user_data      User-specific data
 * @param  err_code       If an error occures in the function, it sets
 *                        *err_code into the code of the error.
 *
 * @return Pointer to the user data structure.
 *
 * @retval NULL  The function returns NULL if the node is rejected
 *               by filters or if an error occures. In the last case
 *               it also updates err_code parameter with appropriate code.
 */
void *
flow_tree_add_node(node_id_t parent_id, node_id_t node_id,
                   node_type_t new_node_type,
                   char *node_name, uint32_t *timestamp,
                   void *user_data, int *err_code)
{
    node_t **p_par_node;
    node_t  *par_node;
    node_t  *cur_node;

    /* Find parent node */
    if ((p_par_node = (node_t **)
                g_hash_table_lookup(new_set, &parent_id)) == NULL)
    {
        *err_code = EINVAL;
        FMT_TRACE("Unexpected start node (node_id %d)", node_id);
        return NULL;
    }
    par_node = *p_par_node;

    cur_node = (node_t *)obstack_alloc(obstk, sizeof(node_t));

    cur_node->parent = par_node;
    cur_node->type = new_node_type;
    cur_node->user_data = user_data;

    msg_queue_init(&cur_node->msg_att);
    msg_queue_init(&cur_node->msg_after_att);
    ctrl_msg_data_init(&cur_node->ctrl_data);

    memcpy(cur_node->start_ts, timestamp, sizeof(cur_node->start_ts));
    memcpy(cur_node->end_ts, max_timestamp, sizeof(cur_node->end_ts));

    /* This will be overwritten if necessary below */
    cur_node->fmode = par_node->fmode;

    FILL_BRANCH_INFO(cur_node);

    /* Form node name */
    if (node_name == NULL)
    {
        /* Use parent name */
        assert(new_node_type == NT_SESSION);
        cur_node->name = par_node->name;
    }
    else
    {
        int                 str_len;
        int                 node_name_len;
        enum node_fltr_mode fmode;

        str_len = strlen(par_node->name);
        node_name_len = strlen(node_name);

        /* TODO: Clarification (comments) */
        /* Define filtering mode by filter module */
        cur_node->name = (char *)obstack_alloc(obstk,
                                     str_len + 1 + node_name_len + 1);
        memcpy(cur_node->name, par_node->name, str_len);
        cur_node->name[str_len] = '/';
        memcpy(cur_node->name + 1 + str_len, node_name, node_name_len + 1);

        if ((fmode = rgt_filter_check_branch(cur_node->name)) !=
                 NFMODE_DEFAULT)
        {
            cur_node->fmode = fmode;
        }
    }

    /* Fill in auxiliary for g_hash fields */
    cur_node->id = node_id;
    cur_node->self = cur_node;
    cur_node->next = NULL;

    if (par_node->more_branches == TRUE)
    {
        branch_info *old_ptr = par_node->branches;

        /* Create new branch */
        par_node->branches =
            realloc(old_ptr, sizeof(branch_info) *
                             (par_node->n_branches + 1));
        if (par_node->branches == NULL)
        {
            par_node->branches = old_ptr;
            *err_code = ENOMEM;
            TRACE("No memory available");
            return NULL;
        }
        par_node->n_branches++;
        par_node->n_active_branches++;

        /* Update user-specific info with new number of messages */
        rgt_process_event(NT_SESSION, MORE_BRANCHES, par_node->user_data);

        cur_node->prev = par_node;

        par_node->branches[par_node->n_branches - 1].first_el = cur_node;

        /* Commmon part */
        par_node->branches[par_node->n_branches - 1].last_el = cur_node;
        par_node->branches[par_node->n_branches - 1].status =
            BSTATUS_ACTIVE;
        par_node->branches[par_node->n_branches - 1].start_ts =
            cur_node->start_ts;
        par_node->branches[par_node->n_branches - 1].end_ts =
            cur_node->end_ts;

        if (new_node_type != NT_TEST)
        {
            g_hash_table_insert(new_set, &cur_node->id, &cur_node->self);
        }
        g_hash_table_remove(close_set, &par_node->id);
        g_hash_table_insert(close_set, &cur_node->id, &cur_node->self);
    }
    else if (par_node->n_branches == 1)
    {
#ifdef FLOW_TREE_LIBRARY_DEBUG
        assert(par_node->n_active_branches == 0);
#endif

        par_node->n_active_branches++;
        cur_node->prev = par_node->branches[0].last_el;

        cur_node->prev->next = cur_node;

        /* Common part */
        par_node->branches[0].last_el = cur_node;
        par_node->branches[0].status = BSTATUS_ACTIVE;
        par_node->branches[par_node->n_branches - 1].end_ts = max_timestamp;
        if (new_node_type != NT_TEST)
        {
            g_hash_table_insert(new_set, &cur_node->id, &cur_node->self);
        }
        g_hash_table_remove(close_set, &par_node->id);
        g_hash_table_insert(close_set, &cur_node->id, &cur_node->self);

        /*
         * more_branches equals to false, so we have to remove
         * session from new set.
         */
        g_hash_table_remove(new_set, &par_node->id);
    }
    else
    {
        /* Error: Attemp to add a parallel node in already closed session */
        *err_code = EINVAL;
        FMT_TRACE("%s with node_id equals to %d can't spawn "
                  "new branches", CNTR_BIN2STR(par_node->type), parent_id);
        return NULL;
    }

    if (cur_node->fmode != NFMODE_INCLUDE)
    {
        cur_node->user_data = NULL;
    }

    return cur_node->user_data;
}

/**
 * Try to close the node in the execution flow tree.
 *
 * @param  parent_id   Parent node id
 * @param  node_id     Id of the node to be closed
 * @param  timestamp   Timestamp of the end message
 * @param  err_code    Placeholder for return status code
 *
 * @return  Pointer to the user specific data (passed in add_node routine)
 *
 * @retval  pointer  The node was successfully closed
 * @retval  NULL     The node was rejected by filters on creation or
 *                   an error occurs
 */
void *
flow_tree_close_node(node_id_t parent_id, node_id_t node_id,
                     uint32_t *timestamp, int *err_code)
{
    node_t **p_cur_node;
    node_t  *cur_node;
    node_t  *par_node;
    int      i;

    /* Find current node */
    if ((p_cur_node = (node_t **)
                g_hash_table_lookup(close_set, &node_id)) == NULL)
    {
        *err_code = EINVAL;
        FMT_TRACE("Unexpected end node (node_id %d)", node_id);
        return NULL;
    }

    cur_node = *p_cur_node;
    memcpy(cur_node->end_ts, timestamp, sizeof(cur_node->end_ts));
    par_node = cur_node->parent;

    /* Only package and session can be parent for some node */
    assert(par_node->type != NT_TEST);

    if (par_node->id != parent_id)
    {
        /* Incorrect parent_id value */
        *err_code = EINVAL;
        FMT_TRACE("Incorrect parent id for the end node: "
                  "received %d, expected %d", parent_id, par_node->id);
        return NULL;
    }

    /* Only for non test nodes */
    g_hash_table_remove(new_set, &node_id);
    g_hash_table_remove(close_set, &node_id);


    par_node->more_branches = FALSE;
    par_node->n_active_branches--;

    /*
     * This operation actually deletes node only once, a first child
     * is going to be closed
     */
    g_hash_table_remove(new_set, &par_node->id);

    if (par_node->n_active_branches == 0)
    {
        g_hash_table_insert(close_set, &par_node->id, &par_node->self);
        if (par_node->n_branches == 1)
        {
            g_hash_table_insert(new_set, &par_node->id, &par_node->self);
        }
    }

    /* Set idle status in closed branch */
    for (i = 0; i < par_node->n_branches; i++)
    {
        if (par_node->branches[i].last_el == cur_node)
        {
            par_node->branches[i].status = BSTATUS_IDLE;
            par_node->branches[i].end_ts = cur_node->end_ts;
            break;
        }
    }
    assert(i != par_node->n_branches);

    return cur_node->user_data;
}

/**
 * Callback function that copies node ID into user provided memory.
 *
 * @param key        Pointer to node key (node ID in our case)
 * @param value      Pointer to node value
 * @param user_data  Pointer to user data (placeholder for node ID)
 */
static void
get_node_id_callback(gpointer key, gpointer value, gpointer user_data)
{
    node_t **p_node = (node_t **)user_data;

    UNUSED(key);

    *p_node = *(node_t **)value;
}

/* See the decription in flow_tree.h */
te_errno
flow_tree_get_close_node(node_id_t *id, node_id_t *parent_id)
{
    node_t *node = NULL;

    g_hash_table_foreach(close_set, get_node_id_callback, &node);

    if (node == NULL || node->id == FLOW_TREE_ROOT_ID)
    {
        /*
         * All the nodes are closed:
         * Root node can present in "close" set, as a result of
         * common close processing algorithm, so do not care about it.
         */
        return TE_ENOENT;
    }

    assert(node->parent != NULL);

    *id = node->id;
    *parent_id = node->parent->id;

    return 0;
}

enum node_fltr_mode
closed_tree_get_mode(node_t *node, uint32_t *ts)
{
    enum node_fltr_mode res = NFMODE_DEFAULT;

    if (node == NULL)
        return NFMODE_DEFAULT;

    /* If the node out of message timestamp */
    if ((TIMESTAMP_CMP(ts, node->start_ts) < 0) ||
        (TIMESTAMP_CMP(ts, node->end_ts) > 0))
    {
        return NFMODE_DEFAULT;
    }

    if (node->type == NT_TEST)
    {
        return node->fmode;
    }
    else
    {
        node_t *cur_node = NULL;
        int     i;

        for (i = 0; i < node->n_branches; i++)
        {
            if ((TIMESTAMP_CMP(ts, node->branches[i].start_ts) < 0) ||
                (TIMESTAMP_CMP(ts, node->branches[i].end_ts) > 0))
            {
                continue;
            }

            /*
             * If we never be here while "for" performs then cur_node is
             * equal to NULL after the "for" finishes. Otherwise "res"
             * variable will be updated correctly.
             */

            cur_node = node->branches[i].last_el;

            while (cur_node != node)
            {
                if (TIMESTAMP_CMP(ts, cur_node->start_ts) < 0)
                {
                    cur_node = cur_node->prev;
                    continue;
                }

                if (TIMESTAMP_CMP(ts, cur_node->end_ts) > 0)
                {
                    /* Message is between nodes */
                    res = node->fmode;
                    break;
                }

                /*
                 * cur_node is a pointer to the node in which
                 * context message is received
                 */
                res = closed_tree_get_mode(cur_node, ts);
                break;
            }

            if (cur_node == node)
                res = node->fmode;

            if (res == NFMODE_INCLUDE)
            {
                return NFMODE_INCLUDE;
            }

            /*
             * Continue with the next branch until at least one node
             * has NFMODE_INCLUDE filtering mode for the message timestamp.
             */
        }

        if (cur_node == NULL)
        {
            /*
             * message is outside of the all branches but inside
             * the SESSION, so we have to return session filtering mode.
             */
            return node->fmode;
        }
        else
            return res;
    }

    /* We can't be here */
    assert(0);

    return NFMODE_DEFAULT;
}

/**
 * Filters message according to package/test filtering.
 *
 * @param msg A message to be checked
 *
 * @return Returns a filtering mode of a node which the message links with
 */
enum node_fltr_mode
flow_tree_filter_message(log_msg *msg)
{
    return closed_tree_get_mode(root, msg->timestamp);
}

/** Attach message pointer to GQueue */
static void
attach_msg_to_gqueue(GQueue *queue, GList **cache, log_msg_ptr *msg)
{
    log_msg_ptr *tail_msg = g_queue_peek_tail(queue);

    if (tail_msg != NULL &&
        TIMESTAMP_CMP(msg->timestamp, tail_msg->timestamp) >= 0)
    {
        /*
         * Just add to the tail - do not need to traverse
         * through the list.
         */
        g_queue_push_tail(queue, msg);

#ifdef RGT_PROF_STAT
        msg_put_to_tail++;
#endif
    }
    else
    {
        /*
         * First try to add message just after cached message.
         */
        if (*cache != NULL)
        {
            GList       *after_cache_elem;
            log_msg_ptr *after_cache_msg;

#ifdef RGT_PROF_STAT
            msg_use_cache++;
#endif

            tail_msg = (log_msg_ptr *)((*cache)->data);

            if (TIMESTAMP_CMP(msg->timestamp, tail_msg->timestamp) >= 0 &&
                (after_cache_elem = g_list_next(*cache)) != NULL &&
                (after_cache_msg =
                    (log_msg_ptr *)after_cache_elem->data) != NULL)
            {
                /*
                 * The message we want to put into the queue should go
                 * after cached message (as its timestamp is bigger than
                 * one of the cached message):
                 * ... -> / cached msg / -> /after cache msg / -> ...
                 *
                 * Now try to find out if it should go just immediately
                 * after chached message:
                 * ... ->/ cached msg / -> / MSG / -> /after cache msg / ->
                 *
                 * or we should go further the queue.
                 * ... ->/ cached msg / ->/after cache msg / -> ... / MSG /
                 */

                if (TIMESTAMP_CMP(msg->timestamp,
                                  after_cache_msg->timestamp) <= 0)
                {
#ifdef RGT_PROF_STAT
                    msg_put_after_cache_quick++;
#endif

                    /*
                     * Insert the message just after cached and
                     * update cache with this new message.
                     */
                    g_queue_insert_after(queue, *cache, msg);

                    *cache = g_list_next(*cache);
                    assert((*cache)->data == msg);
                }
                else
                {
                    GList *queue_elem = g_list_next(after_cache_elem);

#ifdef RGT_PROF_STAT
                    msg_put_after_cache_slow++;
#endif

                    /*
                     * Find the place in the queue where the message
                     * should be inserted, starting from the cached message.
                     */

                    /*
                     * We can be sure that queue_elem won't be NULL
                     * on any iteration, because the message timestamp
                     * less than tail's timestamp.
                     */
                    while (TIMESTAMP_CMP(msg->timestamp,
                        ((log_msg_ptr *)queue_elem->data)->timestamp) > 0)
                    {
                        queue_elem = g_list_next(queue_elem);
                    }

                    g_queue_insert_before(queue, queue_elem, msg);

                    *cache = g_list_previous(queue_elem);
                    assert((*cache)->data == msg);
                }
            }
            else
            {
                GList   *queue_elem = g_list_previous(*cache);

#ifdef RGT_PROF_STAT
                msg_put_before_cache++;
#endif

                /*
                 * The message we want to put into the queue should go
                 * before cached message (as its timestamp is less than
                 * one of the cached message):
                 * go in backward directon to find the place
                 * for the message.
                 */

                while (queue_elem != NULL &&
                    TIMESTAMP_CMP(msg->timestamp,
                       ((log_msg_ptr *)queue_elem->data)->timestamp) < 0)
                {
                    queue_elem = g_list_previous(queue_elem);
                }

                if (queue_elem == NULL)
                {
                    /* Add into the head of the queue */
                    g_queue_push_head(queue, msg);
                }
                else
                {
                    /* Append after the element */
                    g_queue_insert_after(queue, queue_elem, msg);

                    /*
                     * Do not update cache here:
                     * for some samples this might make productivity worse.
                     */
                }
            }

            return;
        }

        /*
         * The message was delayed, and there is no cache for the message.
         * Go through the queue to find out the right place for the message.
         */
        g_queue_insert_sorted(queue, msg, timestamp_cmp, NULL);

        /* Find the entry we've just put into the queue */
        *cache = g_queue_find(queue, msg);

#ifdef RGT_PROF_STAT
        msg_nocache++;
#endif
    }
}

/**
 * Offload to the file corresponging to a given queue all the message
 * pointers whose timestamp is no greater than end_ts.
 *
 * @param q       Queue of message pointers
 * @param end_ts  Finishing timestamp
 */
static void
msg_queue_offload(msg_queue *q, uint32_t *end_ts)
{
    FILE *f;
    char  path[PATH_MAX];

    log_msg_ptr *msg_ptr;

    if (q->queue == NULL || g_queue_is_empty(q->queue))
        return;

    if (rgt_ctx.tmp_dir == NULL)
        return;

    if (TIMESTAMP_CMP(q->offload_ts, end_ts) >= 0)
        return;

    snprintf(path, sizeof(path), "%s/%p", rgt_ctx.tmp_dir, q);
    f = fopen(path, "a");
    if (f == NULL)
    {
        fprintf(stderr, "Failed to open %s for appending: errno %d (%s)\n",
                path, errno, strerror(errno));
        THROW_EXCEPTION;
    }

    while ((msg_ptr = g_queue_peek_head(q->queue)) != NULL)
    {
        if (TIMESTAMP_CMP(msg_ptr->timestamp, end_ts) > 0)
            break;

        if (fwrite(msg_ptr, 1,
                   sizeof(log_msg_ptr), f) != sizeof(log_msg_ptr))
        {
            fprintf(stderr, "Failed to write log_msg_ptr to %s\n", path);
            THROW_EXCEPTION;
        }

        q->offload_ts[0] = msg_ptr->timestamp[0];
        q->offload_ts[1] = msg_ptr->timestamp[1];

        g_queue_pop_head(q->queue);
        free_log_msg_ptr(msg_ptr);
        q->offloaded = TRUE;
    }

    q->cache = NULL;

    fclose(f);
}

/**
 * Reload from the file corresponging to a given queue all the message
 * pointers whose timestamp is no less than start_ts.
 *
 * @param q         Queue of message pointers
 * @param start_ts  Starting timestamp
 */
static void
msg_queue_reload(msg_queue *q, uint32_t *start_ts)
{
    FILE *f;
    char  path[PATH_MAX];

    off_t file_len;
    off_t n_recs;
    off_t req_start;
    off_t req_end;
    off_t req_middle;
    off_t truncate_length = -1;

    size_t rc;

    log_msg_ptr   msg_ptr;
    log_msg_ptr  *msg_ptr_new = NULL;
    GList        *insert_after_elem = NULL;

    if (!q->offloaded)
        return;

    if (rgt_ctx.tmp_dir == NULL)
        return;

    snprintf(path, sizeof(path), "%s/%p", rgt_ctx.tmp_dir, q);
    f = fopen(path, "r");
    if (f == NULL)
    {
        fprintf(stderr, "Failed to open %s for reading: errno %d (%s)\n",
                path, errno, strerror(errno));
        THROW_EXCEPTION;
    }

    fseeko(f, 0LL, SEEK_END);
    file_len = ftello(f);

    if (file_len == 0)
    {
        fclose(f);
        return;
    }

    n_recs = file_len / sizeof(log_msg_ptr);

    req_start = 0;
    req_end = n_recs - 1;
    if (req_end < 0)
        req_end = 0;

    fseeko(f, 0LL, SEEK_SET);
    if (fread(&msg_ptr, 1, sizeof(msg_ptr), f) != sizeof(msg_ptr))
    {
        fprintf(stderr, "Failed to read the first record from %s\n", path);
        THROW_EXCEPTION;
    }

    if (start_ts != NULL &&
        TIMESTAMP_CMP(msg_ptr.timestamp, start_ts) < 0)
    {
        while (req_end - req_start > 1)
        {
            req_middle = (req_start + req_end) / 2;

            fseeko(f, req_middle * sizeof(msg_ptr), SEEK_SET);
            if (fread(&msg_ptr, 1, sizeof(msg_ptr), f) != sizeof(msg_ptr))
            {
                fprintf(stderr, "Failed to read record %llu from %s\n",
                        (long long unsigned)req_middle, path);
                THROW_EXCEPTION;
            }

            if (TIMESTAMP_CMP(msg_ptr.timestamp, start_ts) >= 0)
                req_end = req_middle;
            else
                req_start = req_middle;
        }
    }

    fseeko(f, req_start * sizeof(msg_ptr), SEEK_SET);
    while (!feof(f))
    {
        msg_ptr_new = alloc_log_msg_ptr();
        rc = fread(msg_ptr_new, 1, sizeof(log_msg_ptr), f);
        if (rc <= 0 && feof(f))
        {
            free_log_msg_ptr(msg_ptr_new);
            break;
        }
        else if (rc != sizeof(log_msg_ptr))
        {

            fprintf(stderr, "Failed to read full record from %s\n", path);
            free_log_msg_ptr(msg_ptr_new);
            THROW_EXCEPTION;
        }

        if (start_ts == NULL ||
            TIMESTAMP_CMP(msg_ptr_new->timestamp, start_ts) >= 0)
        {
            if (truncate_length < 0)
            {
                truncate_length = ftello(f) - sizeof(log_msg_ptr);
                q->offload_ts[0] = msg_ptr_new->timestamp[0];
                q->offload_ts[1] = msg_ptr_new->timestamp[1];
            }

            if (insert_after_elem == NULL)
            {
                g_queue_push_head(q->queue, msg_ptr_new);
                insert_after_elem = g_queue_peek_head_link(q->queue);
            }
            else
            {
                g_queue_insert_after(q->queue, insert_after_elem,
                                     msg_ptr_new);
                insert_after_elem = g_list_next(insert_after_elem);
            }
        }
        else
        {
            free_log_msg_ptr(msg_ptr_new);
            if (truncate_length >= 0)
            {
                fprintf(stderr, "%s\n",
                        "Out-of-order message encountered "
                        "in offloaded queue");
                THROW_EXCEPTION;
            }
        }
    }

    fclose(f);

    if (truncate_length == 0)
        q->offloaded = FALSE;

    if (truncate_length >= 0 &&
        truncate(path, truncate_length) < 0)
    {
        fprintf(stderr, "Failed to truncate %s\n", path);
        THROW_EXCEPTION;
    }
}

/* See description in the rgt_common.h */
void
msg_queue_foreach(msg_queue *q, GFunc cb, void *user_data)
{
    FILE *f;
    char  path[PATH_MAX];

    log_msg_ptr   msg_ptr;
    size_t        rc;

    if (q == NULL)
        return;

    if (rgt_ctx.tmp_dir != NULL && q->offloaded)
    {
        snprintf(path, sizeof(path), "%s/%p", rgt_ctx.tmp_dir, q);
        f = fopen(path, "r");
        if (f == NULL)
        {
            fprintf(stderr,
                    "Failed to open %s for reading: errno %d (%s)\n",
                    path, errno, strerror(errno));
            THROW_EXCEPTION;
        }
        else
        {
            while (!feof(f))
            {
                rc = fread(&msg_ptr, 1, sizeof(log_msg_ptr), f);
                if (rc <= 0 && feof(f))
                    break;
                else if (rc != sizeof(log_msg_ptr))
                {

                    fprintf(stderr, "Failed to read full record from %s\n", path);
                    THROW_EXCEPTION;
                }
                cb(&msg_ptr, user_data);
            }

            fclose(f);
        }
    }

    if (q->queue != NULL)
        g_queue_foreach(q->queue, cb, user_data);
}

/* See description in the rgt_common.h */
te_bool
msg_queue_is_empty(msg_queue *q)
{
    if (q == NULL || q->queue == NULL)
        return TRUE;
    else
        return !(q->offloaded) && g_queue_is_empty(q->queue);
}

/**
 * Attach message pointer to a queue.
 *
 * @param q       Queue of message pointers
 * @param msg     Message pointer to be attached
 */
static void
msg_queue_attach(msg_queue *q, log_msg_ptr *msg)
{
    static te_bool  first_msg = TRUE;

    static uint64_t msg_counter = 0;

    msg_counter++;

    if (first_msg)
    {
        first_msg = FALSE;
        last_offload_ts[0] = msg->timestamp[0];
        last_offload_ts[1] = msg->timestamp[1];
    }
    else if (TIMESTAMP_CMP(msg->timestamp, last_offload_ts) > 0)
    {
        uint32_t diff_ts[2];

        TIMESTAMP_SUB(diff_ts, msg->timestamp, last_offload_ts);
        if (diff_ts[0] >= OFFLOAD_TIMEOUT && msg_counter >= OFFLOAD_MSG_NUM)
        {
            GList *elem;
            GList *next_elem;

            msg_counter = 0;

            last_offload_ts[0] = msg->timestamp[0];
            last_offload_ts[1] = msg->timestamp[1];
            if (last_offload_ts[0] >= OFFLOAD_INTERVAL)
                last_offload_ts[0] -= OFFLOAD_INTERVAL;
            else
                last_offload_ts[0] = 0;

            for (elem = g_queue_peek_head_link(offload_queue); elem != NULL;
                 elem = next_elem)
            {
                msg_queue *queue = (msg_queue *)elem->data;

                msg_queue_offload(queue, last_offload_ts);

                next_elem = g_list_next(elem);
                if (g_queue_is_empty(queue->queue))
                    g_queue_delete_link(offload_queue, elem);
            }
        }
    }

    if (g_queue_is_empty(q->queue))
        g_queue_push_tail(offload_queue, q);

    if (TIMESTAMP_CMP(msg->timestamp, q->offload_ts) < 0)
        msg_queue_reload(q, msg->timestamp);

    attach_msg_to_gqueue(q->queue, &(q->cache), msg);
}

int
flow_tree_attach_from_node(node_t *node, log_msg_ptr *msg)
{
    uint32_t *ts = msg->timestamp;

    assert(node != NULL);

    /* If the node out of message timestamp */
    if (TIMESTAMP_CMP(ts, node->start_ts) < 0)
        return -1;

    if (TIMESTAMP_CMP(ts, node->end_ts) > 0)
    {
        /* Append message to the "after" list of messages */
        msg_queue_attach(&node->msg_after_att, msg);
        return 0;
    }

    if (node->type == NT_TEST)
    {
        /* Attach message to the node */
        msg_queue_attach(&node->msg_att, msg);
        return 0;
    }
    else
    {
        node_t *cur_node = NULL;
        int     i;

        for (i = 0; i < node->n_branches; i++)
        {
            if (TIMESTAMP_CMP(ts, node->branches[i].start_ts) < 0)
            {
                continue;
            }

            /*
             * Start working from the latest entry in the list,
             * as in most cases messages go in time order.
             */
            cur_node = node->branches[i].last_el;

            while (cur_node != node)
            {
                if (flow_tree_attach_from_node(cur_node, msg) != 0)
                {
                    /*
                     * Try to find a place for the message in backward
                     * time order as the most probable case.
                     */
                    cur_node = cur_node->prev;
                    continue;
                }

                break;
            }

            /* It's impossible that we haven't find a node */
            assert(cur_node != node);
        }

        if (cur_node == NULL)
        {
            /* message came before starting any branch of the session */
            msg_queue_attach(&node->msg_att, msg);
        }

        return 0;
    }

    /* We can't be here */
    assert(0);

    return NFMODE_DEFAULT;
}


/**
 * Attaches message to the nodes of the flow tree.
 *
 * @param  msg   Message to be attached.
 *
 * @return  Nothing.
 */
void
flow_tree_attach_message(log_msg *msg)
{
    node_t **p_cur_node;
    node_t  *cur_node;

    assert(msg->flags != 0);

    if (msg->id == TE_LOG_ID_UNDEFINED)
    {
        /* There could be no verdicts until we start test or package */
        /* FIXME: may be something was actually wrong here */
        /* assert((msg->flags & RGT_MSG_FLG_VERDICT) == 0); */

        flow_tree_attach_from_node(root, log_msg_ref(msg));
        free_log_msg(msg);
        msg = NULL;
        return;
    }

    /*
     * Each message keeps "Log ID" value, which is used to differentiate
     * log messages came from tests. That feature is essential for
     * the case when we run many tests together (parallel execution).
     *
     * NOTE:
     * Currently this field has sense only for messages came from tests,
     * and keeps TE_LOG_ID_UNDEFINED value for messages came from
     * Engine processes and Test Agents.
     *
     * Here we expect that log messages from the particular test can't be
     * mixed in time, as we use a set of "expected to close" nodes.
     */
    if ((p_cur_node = (node_t **)
                g_hash_table_lookup(close_set, &(msg->id))) == NULL)
    {
        fprintf(stderr, "Cannot find test with ID equals to %d\n", msg->id);

        if (rgt_ctx.ignore_unknown_id)
        {
            free_log_msg(msg);
            return;
        }

        THROW_EXCEPTION;
    }
    cur_node = *p_cur_node;

    if ((msg->flags & RGT_MSG_FLG_NORMAL) != 0)
        flow_tree_attach_from_node(cur_node, log_msg_ref(msg));

    /* Check if we are processing Test Control message */
    if ((msg->flags & (RGT_MSG_FLG_VERDICT | RGT_MSG_FLG_ARTIFACT)) != 0)
    {
        int idlen = strlen(TE_TEST_OBJECTIVE_ID);

        /* Currently Control messages can be generated only for tests */
        assert(cur_node->type == NT_TEST);

        rgt_expand_log_msg(msg);
        if (strncmp(msg->fmt_str, TE_TEST_OBJECTIVE_ID, idlen) == 0)
        {
            node_info_t *info = cur_node->user_data;

            info->descr.objective =
            node_info_obstack_copy0(msg->txt_msg +
                                    idlen,
                                    strlen(msg->txt_msg + idlen));

            if ((msg->flags & RGT_MSG_FLG_NORMAL) == 0)
            {
                free_log_msg(msg);
                msg = NULL;
            }
        }
        else
        {
            if (msg->flags & RGT_MSG_FLG_ARTIFACT)
            {
                msg_queue_attach(&cur_node->ctrl_data.artifacts,
                                 log_msg_ref(msg));
            }
            else
            {
                msg_queue_attach(&cur_node->ctrl_data.verdicts,
                                 log_msg_ref(msg));
            }
        }
    }

    if (msg != NULL)
        free_log_msg(msg);
}

static void
wrapper_process_regular_msg(gpointer data, gpointer user_data)
{
    log_msg_ptr *msg_ptr = (log_msg_ptr *)data;
    log_msg     *msg = NULL;
    te_bool      msg_visible = TRUE;

    UNUSED(user_data);

    if (reg_msg_proc != NULL)
    {
        msg = log_msg_read(msg_ptr);
        if (msg->id != TE_LOG_ID_UNDEFINED)
        {
            if (~msg->level & TE_LL_CONTROL)
            {
                msg->nest_lvl = rgt_ctx.current_nest_lvl;
            }
            else
            {
                if (strcmp(msg->user, TE_USER_STEP) == 0)
                {
                    msg->nest_lvl = 0;
                    rgt_ctx.current_nest_lvl = 1;
                }
                else if (strcmp(msg->user, TE_USER_SUBSTEP) == 0)
                {
                    msg->nest_lvl = 1;
                    rgt_ctx.current_nest_lvl = 2;
                }
                else if (strcmp(msg->user, TE_USER_STEP_PUSH) == 0)
                {
                    msg->nest_lvl = rgt_ctx.current_nest_lvl;
                    rgt_ctx.current_nest_lvl++;
                }
                else if (strcmp(msg->user, TE_USER_STEP_POP) == 0)
                {
                    rgt_ctx.current_nest_lvl--;
                    msg->nest_lvl = rgt_ctx.current_nest_lvl;
                }
                else if (strcmp(msg->user, TE_USER_STEP_NEXT) == 0)
                {
                    msg->nest_lvl = rgt_ctx.current_nest_lvl - 1;
                }
                else if (strcmp(msg->user, TE_USER_STEP_RESET) == 0)
                {
                    msg_visible = FALSE;
                    rgt_ctx.current_nest_lvl = 0;
                }
                else
                {
                    msg->nest_lvl = rgt_ctx.current_nest_lvl;
                }
            }
        }
        if (msg_visible)
            reg_msg_proc(msg);
        free_log_msg(msg);
    }
}

/**
 * Performs wandering over the subtree started from cur_node.
 *
 * @param  cur_node   Root of the subtree to be wander.
 *
 * @return  Nothing.
 */
static void
flow_tree_wander(node_t *cur_node)
{
    enum node_fltr_mode duration_filter_res = NFMODE_INCLUDE;

    if (cur_node == NULL)
        return;

    if (cur_node->fmode == NFMODE_INCLUDE && cur_node->user_data != NULL)
    {
#if TE_RGT_USE_DURATION_FILTER
        duration_filter_res = rgt_filter_check_duration(
                                  node_type2str(cur_node->type),
                                  cur_node->start_ts,
                                  cur_node->end_ts);

        if (duration_filter_res == NFMODE_INCLUDE)
#endif
        {
            if (ctrl_msg_proc[CTRL_EVT_START][cur_node->type] != NULL)
                ctrl_msg_proc[CTRL_EVT_START][cur_node->type](
                    cur_node->user_data, &cur_node->ctrl_data);

            /* Output messages that belongs to the node */
            msg_queue_foreach(&cur_node->msg_att,
                              wrapper_process_regular_msg, NULL);
        }
    }

    if (cur_node->type != NT_TEST)
    {
        int i;

        /* For each branch output it */
        for (i = 0; i < cur_node->n_branches; i++)
        {
            /* Call branch-start routine */
            if (cur_node->fmode == NFMODE_INCLUDE &&
                cur_node->user_data != NULL)
            {
                if (ctrl_msg_proc[CTRL_EVT_START][NT_BRANCH] != NULL)
                    ctrl_msg_proc[CTRL_EVT_START][NT_BRANCH](
                        cur_node->user_data, &cur_node->ctrl_data);
            }

            flow_tree_wander(cur_node->branches[i].first_el);

            /* Call branch-end routine */
            if (cur_node->fmode == NFMODE_INCLUDE &&
                cur_node->user_data != NULL)
            {
                if (ctrl_msg_proc[CTRL_EVT_END][NT_BRANCH] != NULL)
                    ctrl_msg_proc[CTRL_EVT_END][NT_BRANCH](
                        cur_node->user_data, &cur_node->ctrl_data);
            }
        }
    }

    if (cur_node->fmode == NFMODE_INCLUDE &&
        cur_node->user_data != NULL &&
        duration_filter_res == NFMODE_INCLUDE)
    {
        if (ctrl_msg_proc[CTRL_EVT_END][cur_node->type] != NULL)
            ctrl_msg_proc[CTRL_EVT_END][cur_node->type](
                cur_node->user_data, &cur_node->ctrl_data);
        rgt_ctx.current_nest_lvl = 0;
    }

    /* Output messages that were after the node */
    if (cur_node->parent->fmode == NFMODE_INCLUDE)
    {
        msg_queue_foreach(&cur_node->msg_after_att,
                          wrapper_process_regular_msg, NULL);
    }

    flow_tree_wander(cur_node->next);
}

/**
 * Goes through the flow tree and calls callback functions for each node.
 * First it calls start node callback, then calls message processing
 * callback for all messages attached to the node and goes to the subtree.
 * After that it calls end node callback and message processing callback
 * for all messages attached after the node.
 *
 * @return  Nothing.
 */
void
flow_tree_trace(void)
{
#ifdef RGT_PROF_STAT
    fprintf(stderr,
           "Msg put to tail: %ld\n"
           "Msg use cache: %ld\n"
           "Msg put immediately after cache: %ld\n"
           "Msg put after cache (slow): %ld\n"
           "Msg put before cache: %ld\n"
           "Msg no cache: %ld\n"
           "Number of comparing timestamp : %ld\n",
           msg_put_to_tail,
           msg_use_cache,
           msg_put_after_cache_quick,
           msg_put_after_cache_slow,
           msg_put_before_cache,
           msg_nocache,
           timestamp_cmp_cnt);
#endif

    /* Output messages that belongs to the root node */
    if (root->fmode == NFMODE_INCLUDE)
        msg_queue_foreach(&root->msg_att,
                          wrapper_process_regular_msg, NULL);

    /* Usually n_branches of the root session is equlas to 1 */
    if (root->n_branches > 0)
    {
        /* @todo Add some more branches here ! just for cycle */
        flow_tree_wander(root->branches[0].first_el);
    }

    /* Output messages that were after the root node */
    if (root->fmode == NFMODE_INCLUDE)
    {
        msg_queue_foreach(&root->msg_after_att,
                          wrapper_process_regular_msg, NULL);
    }
}

static gint
timestamp_cmp(gconstpointer a, gconstpointer b, gpointer user_data)
{
    UNUSED(user_data);

#ifdef RGT_PROF_STAT
    timestamp_cmp_cnt++;
#endif
    return TIMESTAMP_CMP((((log_msg_ptr *)a)->timestamp),
                         (((log_msg_ptr *)b)->timestamp));
}


#ifdef FLOW_TREE_LIBRARY_DEBUG

/*
 * These routines are auxiluary in debugging of the library and should
 * not be compiled while it's build for working binaries.
 */

/**
 * Verifies if paritular set of nodes (close or new) equals to user
 * specified set of nodes.
 *
 * @param   set_name    Name of the set that will be verified.
 * @param   user_set    User prepared set of nodes that is compared
 *                      with actual set. It has to be in the following
 *                      format: "node_id:node_id: ... :node_id".
 *
 * @return  Verdict of the verification.
 *
 * @retval  1  Actual set of nodes are the same as it was expected.
 * @retval  0  Actual set of nodes and user specified set are different.
 * @retval -1  Invalid format of the user specified set of nodes.
 */
int
flow_tree_check_set(enum flow_tree_set_name set_name, const char *user_set)
{
    GHashTable *check_set;
    node_id_t   id;
    guint       n = 0;
    const char *cur_ptr = user_set;

    check_set = ((set_name == FLOW_TREE_SET_NEW) ? new_set : close_set);

    while ((*cur_ptr) != '\0')
    {
        id = (node_id_t)strtol(user_set, (char **)&cur_ptr, 0);
        if ((user_set == cur_ptr) ||
            ((*cur_ptr) != '\0' && (*cur_ptr) != ':'))
        {
            return -1;
        }

        if (g_hash_table_lookup(check_set, &id) == NULL)
            return 0;

        n++;
        user_set = cur_ptr + 1;
    }

    if (g_hash_table_size(check_set) == n)
        return 1;

    return 0;
}

int
flow_tree_check_parent_list(enum flow_tree_set_name set_name,
                            node_id_t node_id, const char *par_list)
{
    GHashTable  *check_set;
    node_t     **p_cur_node;
    node_t      *cur_node;
    node_id_t    id;
    const char  *cur_ptr = par_list;

    check_set = ((set_name == FLOW_TREE_SET_NEW) ? new_set : close_set);

    if ((p_cur_node = (node_t **)
                g_hash_table_lookup(check_set, &node_id)) == NULL)
        return 0;

    cur_node = (*p_cur_node)->parent;

    while ((*cur_ptr) != '\0' && cur_node != NULL)
    {
        id = (node_id_t)strtol(par_list, (char **)&cur_ptr, 0);
        if ((par_list == cur_ptr) ||
            ((*cur_ptr) != '\0' && (*cur_ptr) != ':'))
        {
            return -1;
        }

        if (cur_node->id != id)
            return 0;

        par_list = (cur_ptr + 1);
        cur_node = cur_node->parent;
    }

    if ((*cur_ptr) != '\0')
        return 0;

    return 1;
}

/** Status value for user prepared buffer */
enum buf_status {
    STATUS_OK,       /**< Normal status */
    STATUS_OVERFLOW, /**< User prepared buffer is too short */
};

/** Internal representation of buffer */
struct user_buf {
    char            *buf;    /**< Pointer to the current position
                                  in buffer */
    gint             len;    /**< Length of the rest of the buffer */
    enum buf_status  status; /**< Buffer status */
};


static void
set_info_callback(gpointer key, gpointer value, gpointer user_data)
{
    struct user_buf *ubuf = (struct user_buf *)user_data;
    node_id_t       *id   = (node_id_t *)key;
    char             tmp[20];
    gint             tmp_str_len;

    if (ubuf->status != STATUS_OK)
        return;

    sprintf(tmp, "%d", *id);
    tmp_str_len = strlen(tmp);
    tmp[tmp_str_len] = ':';
    tmp[tmp_str_len + 1] = '\0';

    if (ubuf->len < (tmp_str_len + 2))
    {
        ubuf->status = STATUS_OVERFLOW;
        return;
    }

    strcpy(ubuf->buf, tmp);
    /* Borrow 1 byte for ':' */
    ubuf->buf += (tmp_str_len + 1);
    ubuf->len -= (tmp_str_len + 1);
}

/**
 * Obtains set of nodes from specific category
 * (nodes waiting for child node or nodes waiting for close event).
 *
 * @param  set_name  Name of the desired category of the nodes.
 * @param  buf       User prepared buffer where information will be placed.
 * @param  len       Length of user specified buffer.
 *
 * @return  Status of operation.
 *
 * @retval  1  Operation has complited successfully.
 * @retval  0  Buffer overflow is occured.
 */
int
flow_tree_get_set(enum flow_tree_set_name set_name, char *buf, gint len)
{
    GHashTable      *set;
    struct user_buf  ubuf;

    ubuf.buf = buf;
    ubuf.len = len;
    ubuf.status = STATUS_OK;

    set = ((set_name == FLOW_TREE_SET_NEW) ? new_set : close_set);

    g_hash_table_foreach(set, set_info_callback, &ubuf);

    if (ubuf.status == STATUS_OVERFLOW)
        return 0;

    if (ubuf.len != len)
    {
        /* At least one node was written, so we should cut the ending ':' */
        buf[len - ubuf.len - 1] = '\0';
    }

    return 1;
}

#endif /* FLOW_TREE_LIBRARY_DEBUG */
