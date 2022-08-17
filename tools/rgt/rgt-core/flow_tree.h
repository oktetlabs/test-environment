/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Test Environment: Interface for test execution flow.
 *
 * The module is responsible for keeping track of occured events and
 * checking if new events are legal.
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_RGT_FILTER_FLOW_TREE_H__
#define __TE_RGT_FILTER_FLOW_TREE_H__
#ifdef __cplusplus
extern "C" {
#endif

#include <glib.h>

#include "rgt_common.h"
#include "log_msg.h"
#include "te_errno.h"

/* Define type that is used to node identification */
typedef gint node_id_t;

#define FLOW_TREE_ROOT_ID ((node_id_t)(0))

/**
 * Initialize flow tree library.
 *
 * @return Nothing
 *
 * @se Add session node with id equals to ROOT_ID into the tree and insert
 *     it into set of patential parent nodes (so called "new set").
 */
void
flow_tree_init(void);

/**
 * Free all resources used by flow tree library.
 *
 * @return Nothing
 */
extern void flow_tree_destroy(void);

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
extern void *flow_tree_add_node(node_id_t parent_id, node_id_t node_id,
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
extern void *flow_tree_close_node(node_id_t parent_id, node_id_t node_id,
                                  uint32_t *timestamp, int *err_code);

/**
 * Returns ID (and parent ID) of a node that is waiting for close
 * operation.
 * Note that there could be more than one node in "close" set,
 * this function returns an arbitrary one because they all are
 * not distinguishable
 *
 * @param id         ID of a node waiting for close
 * @param parent_id  ID of parent node
 *
 * @return 0 on success, TE_ENOENT if all the nodes are closed.
 */
extern te_errno flow_tree_get_close_node(node_id_t *id,
                                         node_id_t *parent_id);

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
void flow_tree_trace(void);


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

