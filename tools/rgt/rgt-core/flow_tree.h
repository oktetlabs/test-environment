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

#ifndef __TE_RGT_FILTER_FLOW_TREE_H__
#define __TE_RGT_FILTER_FLOW_TREE_H__
#ifdef __cplusplus
extern "C" {
#endif

#include <glib.h>

#include "rgt_common.h"
#include "log_msg.h"

/* Define type that is used to node identification */
typedef gint node_id_t;

/**
 * Initialize flow tree library.
 *
 * @return Nothing
 *
 * @se Add session node with id equals to ROOT_ID into the tree and insert 
 *     it into set of patential parent nodes (so called "new set").
 */
void
flow_tree_init();

/**
 * Free all resources used by flow tree library.
 *
 * @return Nothing
 */
void
flow_tree_destroy();

/**
 * Try to add a new node into the execution flow tree.
 *
 * @param  parent_id   Parent node id.
 * @param  node_id     Id of the node to be created.
 *
 * @return Status of operation.
 *
 * @retval 1 New node was successfully added.
 * @retval 0 The parent node can't get children.
 */
void *
flow_tree_add_node(node_id_t parent_id, node_id_t node_id, 
                   node_type_t new_node_type,
                   char *node_name, uint32_t *timestamp,
                   void *user_data, int *err_code);

/**
 * Try to close the node in execution flow tree.
 *
 * @param  parent_id   Parent node id.
 * @param  node_id     Id of the node to be closed.
 *
 * @return Pointer to the user data.
 *
 * @retval Pointer if the node was successfully closed.
 * @retval NULL The node is unexpected.
 */
void *flow_tree_close_node(node_id_t parent_id, node_id_t node_id,
                           uint32_t *timestamp, int *err_code);


/**
 * Filters message according to package/test filtering.
 *
 * @param msg A message to be checked 
 *
 * @return Returns a filtering mode of a node which the message links with
 */
enum node_fltr_mode flow_tree_filter_message(log_msg *msg);

/**
 * Attaches message to the nodes of the flow tree.
 *
 * @param  msg   Message to be attached.
 *
 * @return  Nothing.
 */
void flow_tree_attach_message(log_msg *msg);

/**
 * Goes through the flow tree and calls callback functions for each node.
 * First it calls start node callback, then calls message processing
 * callback for all messages attached to the node and goes to the subtree.
 * After that it calls end node callback and message processing callback
 * for all messages attached after the node.
 *
 * @return  Nothing.
 */
void flow_tree_trace();


#ifdef FLOW_TREE_LIBRARY_DEBUG

/* 
 * These routines are auxiluary in debugging of the library and should
 * not be compiled while it's build for working binaries.
 */

enum flow_tree_set_name {
    FLOW_TREE_SET_NEW,
    FLOW_TREE_SET_CLOSE,
};

/**
 * Verifies if paritular set of nodes (close or new) equals to user
 * specified set of nodes.
 *
 * @param   set_name    Name of the set that will be verified.
 * @param   user_set    User prepared set of nodes that is compared
 *                      with actual set. It have to be in the following
 *                      format: "node_id:node_id: ... :node_id".
 *
 * @return  Verdict of the verification.
 *
 * @retval  1  Actual set of nodes are the same as it was expected.
 * @retval  0  Actual set of nodes and user specified set are different.
 * @retval -1  Invalid format of the user specified set of nodes.
 */
int
flow_tree_check_set(enum flow_tree_set_name set_name, const char *user_set);

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
flow_tree_get_set(enum flow_tree_set_name set_name, char *buf, gint len);

int
flow_tree_check_parent_list(enum flow_tree_set_name set_name, 
                            node_id_t node_id, const char *par_list);

#endif /* FLOW_TREE_LIBRARY_DEBUG */

#ifdef __cplusplus
}
#endif

#endif /* __TE_RGT_FILTER_FLOW_TREE_H__ */

