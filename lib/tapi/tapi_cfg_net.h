/** @file
 * @brief Network Configuration Model TAPI
 *
 * Definition of test API for network configuration model
 * (storage/cm/cm_net.xml).
 *
 *
 * Copyright (C) 2004 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 * @author Oleg Kravtsov <Oleg.Kravtsov@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_TAPI_CFG_NET_H__
#define __TE_TAPI_CFG_NET_H__

#include "conf_api.h"

#ifdef __cplusplus
extern "C" {
#endif


/** All possible node types of /net/node configuration model */
enum net_node_type {
    NET_NODE_TYPE_AGENT = 0,    /**< Node is an Agent */
    NET_NODE_TYPE_NUT = 1,      /**< Node is a NUT */
};

/** Node description structure */
typedef struct cfg_net_node_t {
    cfg_handle          handle; /**< Cfg instance handle */
    enum net_node_type  type;   /**< Node type */
} cfg_net_node_t;

/** Net description structure */
typedef struct cfg_net_t {
    cfg_handle      handle;     /**< Cfg instance handle */
    unsigned int    n_nodes;    /**< Number of nodes in the net */
    cfg_net_node_t *nodes;      /**< Array with net nodes */
} cfg_net_t;

/** Nets description structure */
typedef struct cfg_nets_t {
    unsigned int    n_nets;     /**< Number of nets */
    cfg_net_t      *nets;       /**< Array with nets */
} cfg_nets_t;


/**
 * Get configuration of the net.
 *
 * @param net_handle    Configurator handle of the net to get
 * @param net           Location for net data (OUT)
 *
 * @@retval Status code.
 */
extern te_errno tapi_cfg_net_get_net(cfg_handle  net_handle,
                                     cfg_net_t  *net);

/**
 * Get nets configuration.
 *
 * @param nets          Pointer to nets description structure (OUT)
 *
 * @retval Status code.
 */
extern te_errno tapi_cfg_net_get_nets(cfg_nets_t *nets);

/**
 * Free network dump structure.
 *
 * @param net           Pointer to net description structure
 */
extern void tapi_cfg_net_free_net(cfg_net_t *net);

/**
 * Free networks dump structure.
 *
 * @param nets          Pointer to nets description structure
 */
extern void tapi_cfg_net_free_nets(cfg_nets_t *nets);


/**
 * Get pair (if available) from each configured network.
 *
 * @param first     - type of the first node
 * @param second    - type of the second node
 * @param p_n_pairs - location for number of found pairs
 * @param p_pairs   - location for array of found pairs
 *
 * @return Status code.
 */
extern int tapi_cfg_net_get_pairs(enum net_node_type first,
                                  enum net_node_type second,
                                  unsigned int *p_n_pairs,
                                  cfg_handle (**p_pairs)[2]);


/**
 * Find 'net' with node value equal to provided OID.
 *
 * @param oid       - search value
 * @param net       - location for net name (at least CFG_INST_NAME_MAX)
 *
 * @return Status code.
 */
extern int tapi_cfg_net_find_net_by_node(const char *oid, char *net);

/**
 * For the given network, it returns the list of values gathered on
 * the nodes of specified type.
 *
 * The function allocates memory under an array of pointers to the 
 * values.  The last element of this array is set to NULL.  Deallocate
 * this memory with tapi_cfg_net_free_nodes_values.
 *
 * @param net_name  - name of the network
 * @param node_type - type of the node to find
 * @param ta_name   - name of the agent or NULL if it does not matter
 * @param oids      - location for the list of values (OUT)
 * 
 * @retval Status of the operation
 * @retval TE_ENOENT  The network does not have any node of 
 *                 specified type attached
 *
 * @sa tapi_cfg_net_free_nodes_values
 */
extern int tapi_cfg_net_get_nodes_values(const char *net_name,
                                         enum net_node_type node_type,
                                         const char *ta_name,
                                         char ***oids);

/**
 * Frees memory allocated with tapi_cfg_net_get_nodes_values function.
 *
 * @param oids   - Pointer returned with tapi_cfg_net_get_nodes_values
 *
 * @sa tapi_cfg_net_get_nodes_values
 */
extern void tapi_cfg_net_free_nodes_values(char **oids);


/**
 * Get number of the switch port to which specified TA connected.
 *
 * @param ta_node   - OID of TA node in net configuration model
 * @param p_port    - location for port number
 *
 * @return Status code.
 */
extern int tapi_cfg_net_get_switch_port(const char *ta_node,
                                        unsigned int *p_port);


/**
 * Set UP state for all interfaces (nodes) of networks configuration.
 *
 * @return Status code.
 */
extern int tapi_cfg_net_all_up(void);

/** Information about made assignments */
typedef struct tapi_cfg_net_assigned {
    cfg_handle  pool;      /**< subnet handle, i.e. handle of the object
                                instance "/ip4_net_pool:/entry:X" */
    cfg_handle *entries;   /**< assigned pool entries */
} tapi_cfg_net_assigned;

/**
 * Assign IPv4 subnet to specified configuration network.
 *
 * @param net       Configuration network
 * @param assigned  Information about done assignments
 *
 * If assigned is NULL or assigned->pool is CFG_HANDLE_INVALID, IPv4
 * subnet is allocated using #tapi_cfg_alloc_ip4_net. Its handle is
 * returned in assigned->pool. Pool entries assigned to net nodes are
 * returned as array assigned->entries. Order of elements in the same
 * as order of nodes in net->nodes array.
 *
 * @return Status code.
 */
extern int tapi_cfg_net_assign_ip4(cfg_net_t *net,
                                   tapi_cfg_net_assigned *assigned);

/**
 * Assign IPv4 subnets for all configuration network.
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_net_all_assign_ip4(void);

/**
 * Dirty hack, must be removed after sockts_get_host_addr() 
 * will be repaired.
 */
extern int tapi_cfg_net_assign_ip4_one_end(cfg_net_t *net, 
                                           tapi_cfg_net_assigned *assigned);

/**
 * Delete all nets from network configuration.
 *
 * @return Status code (see te_errno.h)
 */
extern te_errno tapi_cfg_net_delete_all(void);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_CFG_NET_H__ */
