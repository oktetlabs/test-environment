/** @file
 * @brief Test Environment: Interface for test execution flow.
 *
 * The module is responsible for keeping track of occured events and 
 * checking if new events are legal.
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

#ifdef FLOW_TREE_LIBRARY_DEBUG
#include <stdio.h>

#endif /* FLOW_TREE_LIBRARY_DEBUG */

#include <glib.h>
#include <obstack.h>

#include "rgt_common.h"
#include "flow_tree.h"
#include "log_msg.h"
#include "filter.h"
#include "log_format.h"
#include "memory.h"

/* Forward declaration */
struct node_t;

/**< Status of the session branch [TODO: Remove this field from all sources] */
enum branch_status {
    BSTATUS_ACTIVE, /**< Branch is active: 
                         There is at least one non closed node. */
    BSTATUS_IDLE,   /**< Branch is idle. */
};

/**< Session branch-specific information */
typedef struct branch_info {
    struct node_t      *next;     /**< Pointer to the first element in
                                       the branch */
    struct node_t      *last_el;  /**< Pointer to the last element 
                                       in the branch */
    enum branch_status  status;   /**< Status of the branch */
    uint32_t           *start_ts; /**< Branch start timestamp */
    uint32_t           *end_ts;   /**< Branch end timestamp */
} branch_info;

/** Session specific part of information */
typedef struct session_spec_part {
   int n_branches;        /**< Number of branches in the session */
   int n_active_branches; /**< Number of active branches */
   int more_branches;     /**< Flag that indicates if session can append more
                               branches. It is set to false just after first
                               close event for any children node arrives. */
   branch_info *branches; /**< Array of branches */
} session_spec_part;

/** Node of the execution flow tree */
typedef struct node_t {
    struct node_t *parent; /**< Pointer to the parent node */
    struct node_t *prev;   /**< Pointer to the previous node 
                                in execution order */
    struct node_t *next;   /**< For package node used as a pointer 
                                to the session underneath */

    node_id_t      id;     /**< Node id (key that is used by g_hash routines) */
    struct node_t *self;   /**< Pointer to this structure (self pointer) */
    char          *name;   /**< Node name */
    
    enum node_type      ntype; /**< Type of the node */
    enum node_fltr_mode fmode; /**< Filter mode for current node */
    uint32_t            start_ts[2]; /**< Node start timestamp */
    uint32_t            end_ts[2];   /**< Node end timestamp */

    GSList *msg_att;        /**< Messages attachement for the node */
    GSList *msg_after_att;  /**< Messages that are followed by the node */

    session_spec_part *sess_info; /**< Pointer to the session specific data */

    void *user_data;  /**< User-specific data associated with a node */
} node_t;

/** Obstack structure that is used to allocation nodes of the tree */
static struct obstack *obstk;

static uint32_t zero_timestamp[2] = {0, 0};
static uint32_t max_timestamp[2]  = {UINT32_MAX, UINT32_MAX};

#define ROOT_ID 0
#define DEF_FILTER_MODE NFMODE_INCLUDE

#define CREATE_SESSION_SPEC_PART(node_ptr) \
    do {                                                       \
        (node_ptr)->ntype = NT_SESSION;                        \
        (node_ptr)->sess_info = (session_spec_part *)          \
            obstack_alloc(obstk, sizeof(session_spec_part));   \
        (node_ptr)->sess_info->n_active_branches = 0;          \
        (node_ptr)->sess_info->n_branches = 0;                 \
        (node_ptr)->sess_info->more_branches = TRUE;           \
        (node_ptr)->sess_info->branches = NULL;                \
    } while (0)

/* Function that is used for comparision of two timestamps */
static gint timestamp_cmp(gconstpointer a, gconstpointer b);

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
flow_tree_init()
{
    obstk = obstack_initialize();
    
    root = (node_t *)obstack_alloc(obstk, sizeof(node_t));
    root->parent = root->prev = root->next = NULL;
    root->id   = ROOT_ID;
    root->self = root;
    root->fmode = DEF_FILTER_MODE;
    root->name = "";
    root->msg_att = NULL;
    root->msg_after_att = NULL;
    root->user_data = NULL;

    memcpy(root->start_ts, zero_timestamp, sizeof(root->start_ts));
    memcpy(root->end_ts, max_timestamp, sizeof(root->end_ts));

    close_set = g_hash_table_new(g_int_hash, g_int_equal);
    new_set   = g_hash_table_new(g_int_hash, g_int_equal);

    g_hash_table_insert(new_set, &root->id, &root->self);
    CREATE_SESSION_SPEC_PART(root);
}

/**
 * Frees control messages linked with each node and regular messages that 
 * belong to the node and goes after it.
 *
 * @param  cur_node  The root of the flow tree subtree for which the operation
 *                   is applied.
 *
 * @return  Nothing.
 */
static void
flow_tree_free_attachments(node_t *cur_node)
{
    if (cur_node == NULL)
        return;

    if (cur_node->msg_att != NULL)
    {
        g_slist_free(cur_node->msg_att);
    }

    if (cur_node->msg_after_att != NULL)
    {
        g_slist_free(cur_node->msg_after_att);
    }

    if (cur_node->ntype == NT_SESSION)
    {
        int                i;
        session_spec_part *sess = cur_node->sess_info;

        for (i = 0; i < sess->n_branches; i++)
        {
            flow_tree_free_attachments(sess->branches[i].next);
        }
    }

    flow_tree_free_attachments(cur_node->next);
    /* We don't need to free user_data. This should be done be user */
    
    /* Free branch_info in sessions */
    if (cur_node->ntype == NT_SESSION)
    {
        free(cur_node->sess_info->branches);
    }
}

/**
 * Free all resources used by flow tree library.
 *
 * @return Nothing
 *
 * @se Add session node with id equals to ROOT_ID into the tree and insert 
 *     it into set of patential parent nodes (so called "new set").
 */
void
flow_tree_destroy()
{
    if (root == NULL)
        return;
 
    g_hash_table_destroy(new_set);
    g_hash_table_destroy(close_set);

    flow_tree_free_attachments(root);

    obstack_destroy(obstk);

    root = NULL;
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
 * @retval NULL  The function returns NULL if the node is rejected by filters
 *               or if an error occures. In the last case it also updates 
 *               err_code parameter with appropriate code.
 */
void *
flow_tree_add_node(node_id_t parent_id, node_id_t node_id, 
                   enum node_type new_node_type,
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
    cur_node->ntype = new_node_type;
    cur_node->user_data = user_data;

    cur_node->msg_att = NULL;
    cur_node->msg_after_att = NULL;

    memcpy(cur_node->start_ts, timestamp, sizeof(cur_node->start_ts));
    memcpy(cur_node->end_ts, max_timestamp, sizeof(cur_node->end_ts));

    /* This will be overwritten if necessary below */
    cur_node->fmode = par_node->fmode;
    
    if (new_node_type == NT_SESSION)
    {
        CREATE_SESSION_SPEC_PART(cur_node);

        /* Session just inherit filter mode from the parent */
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

        if ((fmode = rgt_filter_check_branch(cur_node->name)) != NFMODE_DEFAULT)
        {
            cur_node->fmode = fmode;
        }
    }

    /* Fill in auxiliary for g_hash fields */
    cur_node->id = node_id;
    cur_node->self = cur_node;
    cur_node->next = NULL;

    if (par_node->ntype == NT_SESSION)
    {
        session_spec_part *sess = par_node->sess_info;
        
        if (sess->more_branches == TRUE)
        {
            branch_info *old_ptr = sess->branches;
            
            /* Create new branch */
            sess->branches = realloc(old_ptr,
                                     sizeof(branch_info) * (sess->n_branches + 1));
            if (sess->branches == NULL)
            {
                sess->branches = old_ptr;
                *err_code = ENOMEM;
                TRACE("No memory available");
                return NULL;
            }
            sess->n_branches++;
            sess->n_active_branches++;
            
            /* Update user-specific info with new number of messages */
            rgt_process_event(NT_SESSION, MORE_BRANCHES, par_node->user_data);

            cur_node->prev = par_node;

            sess->branches[sess->n_branches - 1].next = cur_node;

            /* Commmon part */
            sess->branches[sess->n_branches - 1].last_el = cur_node;
            sess->branches[sess->n_branches - 1].status = BSTATUS_ACTIVE;
            sess->branches[sess->n_branches - 1].start_ts = cur_node->start_ts;
            sess->branches[sess->n_branches - 1].end_ts = cur_node->end_ts;

            if (new_node_type != NT_TEST)
            {
                g_hash_table_insert(new_set, &cur_node->id, &cur_node->self);
            }
            g_hash_table_remove(close_set, &par_node->id);
            g_hash_table_insert(close_set, &cur_node->id, &cur_node->self);
        }
        else if (sess->n_branches == 1)
        {
            /* Linear session with at least one node */
#ifdef FLOW_TREE_LIBRARY_DEBUG
            assert(sess->n_active_branches == 0);
#endif
            
            sess->n_active_branches++;
            cur_node->prev = sess->branches[0].last_el;

            cur_node->prev->next = cur_node;

            /* Common part */
            sess->branches[0].last_el = cur_node;
            sess->branches[0].status = BSTATUS_ACTIVE;
            sess->branches[sess->n_branches - 1].end_ts = max_timestamp;

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
            FMT_TRACE("Session with node_id equals to %d can't spawn "
                      "new branches", parent_id);
            return NULL;
        }

        /* Fill in some other fields */
    }
    else if (par_node->ntype == NT_PACKAGE)
    {
        /* Current node can be only session! */
        if (new_node_type != NT_SESSION)
        {
            *err_code = EINVAL;
            TRACE("Child for package node can be only session");
            return NULL;
        }

        cur_node->prev = par_node;
        par_node->next = cur_node;

        g_hash_table_remove(close_set, &par_node->id);
        g_hash_table_remove(new_set, &par_node->id);
        g_hash_table_insert(close_set, &cur_node->id, &cur_node->self);
        g_hash_table_insert(new_set, &cur_node->id, &cur_node->self);
    }
    else
    {
        /* This can't be reached */
#ifdef FLOW_TREE_LIBRARY_DEBUG
        assert(0);
#endif /* FLOW_TREE_LIBRARY_DEBUG */
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
 * @retval  NULL     The node was rejected by filters on creation or an error
 *                   occurs
 */
void *
flow_tree_close_node(node_id_t parent_id, node_id_t node_id,
                     uint32_t *timestamp, int *err_code)
{
    node_t **p_cur_node;
    node_t  *cur_node;
    node_t  *par_node;

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
    
    if (par_node->ntype == NT_SESSION)
    {
        int                i;
        session_spec_part *sess = par_node->sess_info;

        sess->more_branches = FALSE;
        sess->n_active_branches--;
        
        /* 
         * This operation actually deletes node only once, a first child 
         * is going to be closed 
         */
        g_hash_table_remove(new_set, &par_node->id);

        if (sess->n_active_branches == 0)
        {
            g_hash_table_insert(close_set, &par_node->id, &par_node->self);
            if (sess->n_branches == 1)
            {
                g_hash_table_insert(new_set, &par_node->id, &par_node->self);
            }
        }

        /* Set idle status in closed branch */
        for (i = 0; i < sess->n_branches; i++)
        {
            if (sess->branches[i].last_el == cur_node)
            {
                sess->branches[i].status = BSTATUS_IDLE;
                sess->branches[i].end_ts = cur_node->end_ts;

                break;
            }
        }
#ifdef FLOW_TREE_LIBRARY_DEBUG
        assert(i != sess->n_branches);
#endif /* FLOW_TREE_LIBRARY_DEBUG */
    }
    else if (par_node->ntype == NT_PACKAGE)
    {
        /* 
         * Just insert it in close set.
         * Package can be parent only for session 
         */
        g_hash_table_insert(close_set, &par_node->id, &par_node->self);
    }
    else
    {
        /* Only package and session can be parent for some node */
#ifdef FLOW_TREE_LIBRARY_DEBUG
        assert(0);
#endif /* FLOW_TREE_LIBRARY_DEBUG */
    }

    return cur_node->user_data;
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

    if (node->ntype == NT_TEST)
    {
        return node->fmode;
    }
    else if (node->ntype == NT_SESSION)
    {
        session_spec_part *sess;
        node_t            *cur_node = NULL;
        int                i;

        sess = node->sess_info;

        for (i = 0; i < sess->n_branches; i++)
        {

            if ((TIMESTAMP_CMP(ts, sess->branches[i].start_ts) < 0) ||
                (TIMESTAMP_CMP(ts, sess->branches[i].end_ts) > 0))
            {
                continue;
            }

            /*
             * If we never be here while "for" performs then cur_node is equal
             * to NULL after the "for" finishes. Otherwise "res" variable will
             * be updated correctly.
             */

            cur_node = sess->branches[i].last_el;

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
             * message is outside of the all branches but inside the SESSION,
             * so we have to return session filtering mode.
             */
            return node->fmode;
        }
        else
            return res;
    }
    else if (node->ntype == NT_PACKAGE)
    {
        res = closed_tree_get_mode(node->next, ts);

        if (res == NFMODE_DEFAULT)
        {
            return node->fmode;
        } 

        return res;
    }

    /* Dummy code */
#ifdef FLOW_TREE_LIBRARY_DEBUG
    assert(0);
#endif /* FLOW_TREE_LIBRARY_DEBUG */

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

int
flow_tree_attach_from_node(node_t *node, log_msg *msg)
{
    uint32_t *ts = msg->timestamp;

#ifdef FLOW_TREE_LIBRARY_DEBUG
    assert(node != NULL);
#endif /* FLOW_TREE_LIBRARY_DEBUG */

    /* If the node out of message timestamp */
    if (TIMESTAMP_CMP(ts, node->start_ts) < 0)
        return -1;

    if (TIMESTAMP_CMP(ts, node->end_ts) > 0)
    {
        /* Append message to the "after" list of messages */
        node->msg_after_att = 
            g_slist_insert_sorted(node->msg_after_att, msg, timestamp_cmp);
        return 0;
    }

    if (node->ntype == NT_TEST)
    {
        /* Attach message to the node */
        node->msg_att = 
            g_slist_insert_sorted(node->msg_att, msg, timestamp_cmp);
        return 0;
    }
    else if (node->ntype == NT_SESSION)
    {
        session_spec_part *sess;
        node_t            *cur_node = NULL;
        int                i;

        sess = node->sess_info;

        for (i = 0; i < sess->n_branches; i++)
        {
            if (TIMESTAMP_CMP(ts, sess->branches[i].start_ts) <= 0)
            {
                continue;
            }

            cur_node = sess->branches[i].last_el;

            while (cur_node != node)
            {
                if (flow_tree_attach_from_node(cur_node, msg) != 0)
                {
                    cur_node = cur_node->prev;
                }

                break;
            }

            /* It's impossible that we haven't find a node */
#ifdef FLOW_TREE_LIBRARY_DEBUG
            assert(cur_node != node);
#endif /* FLOW_TREE_LIBRARY_DEBUG */
        }

        if (cur_node == NULL)
        {
            /* message came before starting any branch of the session */
            node->msg_att = 
                g_slist_insert_sorted(node->msg_att, msg, timestamp_cmp);
        }

        return 0;
    }
    else if (node->ntype == NT_PACKAGE)
    {
        if (flow_tree_attach_from_node(node->next, msg) < 0)
        {
            /* Insert in "att" list */
            node->msg_att = 
                g_slist_insert_sorted(node->msg_att, msg, timestamp_cmp);
        }
        return 0;
    }

    /* Dummy code */
#ifdef FLOW_TREE_LIBRARY_DEBUG
    assert(0);
#endif /* FLOW_TREE_LIBRARY_DEBUG */

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
    flow_tree_attach_from_node(root, msg);
}

static void
wrapper_process_regular_msg(gpointer data, gpointer user_data)
{
    UNUSED(user_data);
    reg_msg_proc((log_msg *)data);
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
    if (cur_node == NULL)
        return;

    if (cur_node->fmode == NFMODE_INCLUDE && cur_node->user_data != NULL)
    {
        ctrl_msg_proc[CTRL_EVT_START][cur_node->ntype](cur_node->user_data);
        
        /* Output messages that belongs to the node */
        if (cur_node->msg_att != NULL)
            g_slist_foreach(cur_node->msg_att, wrapper_process_regular_msg, NULL);
    }

    if (cur_node->ntype == NT_SESSION)
    {
        int                i;
        session_spec_part *sess = cur_node->sess_info;

        /* For each branch output it */
        for (i = 0; i < sess->n_branches; i++)
        {
            /* Call branch-start routine */
            if (cur_node->fmode == NFMODE_INCLUDE && cur_node->user_data != NULL)
                ctrl_msg_proc[CTRL_EVT_START][NT_BRANCH](NULL);
            
            flow_tree_wander(sess->branches[i].next);
            
            /* Call branch-end routine */
            if (cur_node->fmode == NFMODE_INCLUDE && cur_node->user_data != NULL)
                ctrl_msg_proc[CTRL_EVT_END][NT_BRANCH](NULL);
        }

        /* Output messages that were after the session */
    }
    else if (cur_node->ntype == NT_TEST)
    {
        /* Do nothing */
    }
    else if (cur_node->ntype == NT_PACKAGE)
    {
        flow_tree_wander(cur_node->next);
    }

    if (cur_node->fmode == NFMODE_INCLUDE && cur_node->user_data != NULL)
    {
        ctrl_msg_proc[CTRL_EVT_END][cur_node->ntype](cur_node->user_data);
    }

    /* Output messages that were after the node */
    if (cur_node->msg_after_att != NULL && 
        cur_node->parent->fmode == NFMODE_INCLUDE)
    {
        g_slist_foreach(cur_node->msg_after_att, 
                        wrapper_process_regular_msg, NULL);
    }

    if (cur_node->ntype != NT_PACKAGE)
        flow_tree_wander(cur_node->next);
}

/**
 * Goes through the flow tree and calls callback functions for each node.
 * First it calls start node callback, then calls message processing callback
 * for all messages attached to the node and goes to the subtree.
 * After that it calls end node callback and message processing callback
 * for all messages attached after the node.
 *
 * @return  Nothing.
 */
void
flow_tree_trace()
{
    /* Output messages that belongs to the root node */
    if (root->msg_att != NULL && root->fmode == NFMODE_INCLUDE)
        g_slist_foreach(root->msg_att, wrapper_process_regular_msg, NULL);

    /* Usually n_branches of the root session is equlas to 1 */
    if (root->sess_info->n_branches > 0)
    {
        flow_tree_wander(root->sess_info->branches[0].next);
    }

    /* Output messages that were after the root node */
    if (root->msg_after_att != NULL && 
        root->fmode == NFMODE_INCLUDE)
    {
        g_slist_foreach(root->msg_after_att, 
                        wrapper_process_regular_msg, NULL);
    }
}

static gint
timestamp_cmp(gconstpointer a, gconstpointer b)
{
    return TIMESTAMP_CMP((((log_msg *)a)->timestamp), (((log_msg *)b)->timestamp));
}


#ifdef FLOW_TREE_LIBRARY_DEBUG

/* 
 * These routines are auxiluary in debugging of the library and should not be
 * compiled while it's build for working binaries.
 */

/**
 * Verifies if paritular set of nodes (close or new) equals to user specified
 * set of nodes.
 *
 * @param   set_name    Name of the set that will be verified.
 * @param   user_set    User prepared set of nodes that is compared with actual
 *                      set. It has to be in the following format:
 *                      "node_id:node_id: ... :node_id".
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
        if ((user_set == cur_ptr) || ((*cur_ptr) != '\0' && (*cur_ptr) != ':'))
            return -1;

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
        if ((par_list == cur_ptr) || ((*cur_ptr) != '\0' && (*cur_ptr) != ':'))
            return -1;

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
    char            *buf;    /**< Pointer to the current position in buffer */
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

