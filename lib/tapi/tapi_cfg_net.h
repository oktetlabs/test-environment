/** @file
 * @brief Network Configuration Model TAPI
 *
 * Definition of test API for network configuration model
 * (storage/cm/cm_net.xml).
 *
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 * @author Oleg Kravtsov <Oleg.Kravtsov@oktetlabs.ru>
 *
 */

#ifndef __TE_TAPI_CFG_NET_H__
#define __TE_TAPI_CFG_NET_H__

#include "conf_api.h"
#include "tapi_cfg_pci.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup tapi_conf_net Network topology configuration of Test Agents
 * @ingroup tapi_conf
 * @{
 */

/** All possible node types of /net/node configuration model */
enum net_node_type {
    NET_NODE_TYPE_INVALID = -1, /**< Node has invalid configuration */
    NET_NODE_TYPE_AGENT = 0,    /**< Node is an Agent */
    NET_NODE_TYPE_NUT = 1,      /**< Node is a NUT */
    NET_NODE_TYPE_NUT_PEER = 2, /**< Node is a NUT peer, i.e. something that
                                 * might have IUT-components. This is a
                                 * half-measure before proper typization of
                                 * nodes is implemented. */
};

/** All possible node types of /net/node resources */
enum net_node_rsrc_type {
    NET_NODE_RSRC_TYPE_UNKNOWN = 0, /**< Unknown type of the resource */
    NET_NODE_RSRC_TYPE_INTERFACE,   /**< Network interface */
    NET_NODE_RSRC_TYPE_PCI_FN,      /**< PCI function */
    NET_NODE_RSRC_TYPE_RTE_VDEV,    /**< RTE vdev */
};

/** Node description structure */
typedef struct cfg_net_node_t {
    cfg_handle              handle;     /**< Cfg instance handle */
    enum net_node_type      type;       /**< Node type */
    enum net_node_rsrc_type rsrc_type;  /**< Node resource type */
} cfg_net_node_t;

/** Net description structure */
typedef struct cfg_net_t {
    char           *name;       /**< Network instance name */
    cfg_handle      handle;     /**< Cfg instance handle */
    unsigned int    n_nodes;    /**< Number of nodes in the net */
    cfg_net_node_t *nodes;      /**< Array with net nodes */
} cfg_net_t;

/** Nets description structure */
typedef struct cfg_nets_t {
    unsigned int    n_nets;     /**< Number of nets */
    cfg_net_t      *nets;       /**< Array with nets */
} cfg_nets_t;

/** PCI device of node description structure */
typedef struct cfg_net_pci_info_t {
    enum net_node_type  node_type;
    char               *pci_addr;
    char               *ta_driver;
} cfg_net_pci_info_t;

/**
 * Initialize PCI device info structure.
 *
 * @param pci_info      Pointer to PCI info structure.
 */
extern void tapi_cfg_net_init_pci_info(cfg_net_pci_info_t *pci_info);

/**
 * Free resources allocated for info of PCI device.
 *
 * @param pci_info      Pointer to PCI info structure.
 */
extern void tapi_cfg_net_free_pci_info(cfg_net_pci_info_t *pci_info);

/**
 * Get type of the network node resource.
 *
 * Returns cached value if it is already known and tries to fill in
 * cache otherwise.
 */
extern enum net_node_rsrc_type tapi_cfg_net_get_node_rsrc_type(
                                   cfg_net_node_t *node);

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
 * Register a new network in '/net' CM tree.
 * The function could be useful when you need to build
 * a network dynamically (for example if you want to build some
 * L3 network with a set of nodes, which you later want to assign
 * IP addresses or do some other stuff available for 'cfg_net_t'
 * data types).
 *
 * @param name  network name to assign
 * @param net   placeholder for a network data to be filled in
 *
 * Other parameters are pairs of type:
 * - "const char *node_oid_str" - the value of '/net/node' instance
 *   to be added into a newly created network;
 * - "enum net_node_type node_type" - node type value of
 *   '/net/node/type' instance.
 * .
 * Trailing NULL tells there is no more pairs.
 *
 * @return Status of the operation.
 */
extern te_errno tapi_cfg_net_register_net(const char *name,
                                          cfg_net_t *net, ...);

/**
 * Unregister net previously registered with
 * tapi_cfg_net_register_net() function.
 *
 * @param name  network name to unregister
 * @param net   network data structure (previously filled in with
 *              tapi_cfg_net_register_net() function)
 *
 * @return Status of the operation.
 */
extern te_errno tapi_cfg_net_unregister_net(const char *name,
                                            cfg_net_t *net);

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
 * Remove networks with empty interface names from the CS database.
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_net_remove_empty(void);


/**
 * Prototype of the function to be called for each node.
 *
 * @param net           Network the node belongs to
 * @param node          Node itself
 * @param oid_str       Node OID value in string format
 * @param oid           Parsed node OID
 * @param cookie        Callback opaque data
 *
 * @return Status code.
 *
 * @note Iterator terminates if callback returns non zero.
 */
typedef te_errno tapi_cfg_net_node_cb(cfg_net_t *net, cfg_net_node_t *node,
                                      const char *str, cfg_oid *oid,
                                      void *cookie);

/**
 * Execute callback for each node.
 *
 * @param cb            Callback function
 * @param cookie        Callback opaque data
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_net_foreach_node(tapi_cfg_net_node_cb *cb,
                                          void *cookie);

/**
 * Bind a driver (with specified type) on a PCI function referenced by
 * every net node matching @p node_type. The driver names for specific
 * types are discovered from "/local" configurator tree
 * (e.g. "/local/net_driver").
 *
 * @param node_type     Node type of nets on which bind is performed
 * @param driver        Driver type to bind
 *
 * @note    Net nodes are expected to reference PCI functions,
 *          in case when a node references net interface:
 *          - if @c NET_DRIVER_TYPE_NET is passed, the function succeeds
 *          without performing bind,
 *          - if passed @p driver is not @c NET_DRIVER_TYPE_NET bind fails
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_net_bind_driver_by_node(enum net_node_type node_type,
                                            enum tapi_cfg_driver_type driver);

/**
 * Fill PCI device info of IUT node.
 *
 * @param iut_if_pci_info       Pointer to PCI info structure of IUT node.
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_net_get_iut_if_pci_info(
                                    cfg_net_pci_info_t *iut_if_pci_info);
/**
 * Update network nodes specified in terms of PCI function and bound to
 * network driver to refer to corresponding network interfaces.
 *
 * The interface is reserved as a resource.
 *
 * @sa tapi_cfg_net_bind_driver_by_node()
 * @sa tapi_cfg_net_nodes_switch_pci_fn_to_interface()
 *
 * @param type  Network node type of the nodes to update,
 *              pass @c NET_NODE_TYPE_INVALID to update all network nodes
 *
 * @return Status code.
 *
 * @note Partially done changes are not rolled back in the case of failure.
 */
extern te_errno tapi_cfg_net_nodes_update_pci_fn_to_interface(
                                                    enum net_node_type type);

/**
 * Switch network nodes specified in terms of PCI function to network
 * interfaces incluing rebind the PCI function to network driver.
 *
 * @param type  Network node type of the nodes to switch,
 *              pass @c NET_NODE_TYPE_INVALID to switch all network nodes
 *
 * @return Status code.
 *
 * @note Partially done changes are not rolled back in the case of failure.
 */
extern te_errno tapi_cfg_net_nodes_switch_pci_fn_to_interface(
                                                    enum net_node_type type);

/**
 * Reserve resources for all interfaces mentioned in networks
 * configuration.
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_net_reserve_all(void);

/**
 * Set UP state for all interfaces (nodes) of networks configuration.
 *
 * @param force         If interface is already UP, push it down and up
 *                      again
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_net_all_up(te_bool force);

/**
 * Delete IPv4 addresses from all nodes in networks configuration.
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_net_delete_all_ip4_addresses(void);

/**
 * Delete IPv6 addresses (except link-local) from all nodes in networks
 * configuration.
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_net_delete_all_ip6_addresses(void);


/** Information about made assignments */
typedef struct tapi_cfg_net_assigned {
    cfg_handle  pool;      /**< subnet handle, i.e. handle of the object
                                instance "/net_pool:X/entry:Y" */
    cfg_handle *entries;   /**< assigned pool entries */
} tapi_cfg_net_assigned;


/**
 * Check all configuration networks that MTU values on the ends
 * of each network does not differ
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_net_all_check_mtu(void);


/**
 * Assign IP subnet to specified configuration network.
 *
 * @param af            Address family
 * @param net           Configuration network
 * @param assigned      Information about done assignments
 *
 * If assigned is NULL or assigned->pool is CFG_HANDLE_INVALID, IP
 * subnet is allocated using #tapi_cfg_alloc_entry from /net_pool:ip4 or
 * /net_pool:ip6. Its handle is returned in assigned->pool. Pool entries
 * assigned to net nodes are returned as array assigned->entries.
 * Order of elements in the same as order of nodes in net->nodes array.
 *
 * @return Status code.
 */
extern int tapi_cfg_net_assign_ip(unsigned int af, cfg_net_t *net,
                                  tapi_cfg_net_assigned *assigned);

/**
 * Reset previously assigned IP addresses from a network.
 *
 * @param af        Address family
 * @param net       Configuration network
 * @param assigned  Information about done assignments (to be unregistered)
 *
 * @return Status of the operation.
 */
extern te_errno tapi_cfg_net_unassign_ip(unsigned int af, cfg_net_t *net,
                                         tapi_cfg_net_assigned *assigned);

/**
 * Assign IP subnets for all configuration network.
 *
 * @param af            Address family
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_net_all_assign_ip(unsigned int af);

/**
 * Dirty hack, must be removed after sockts_get_host_addr()
 * will be repaired.
 */
extern int tapi_cfg_net_assign_ip_one_end(unsigned int af, cfg_net_t *net,
                                          tapi_cfg_net_assigned *assigned);

/**
 * Obtain information about subnet address and prefix length.
 *
 * @param assigned    subnet assignments handle
 * @param addr        subnet address (OUT)
 * @param prefix_len  subnet prefix length (OUT)
 *
 * @return Status of the operation
 */
extern te_errno tapi_cfg_net_assigned_get_subnet_ip(
        tapi_cfg_net_assigned *assigned,
        struct sockaddr **addr,
        unsigned int *prefix_len);

/**
 * Delete all nets from network configuration.
 *
 * @return Status code (see te_errno.h)
 */
extern te_errno tapi_cfg_net_delete_all(void);

/**
 * Get PCI devices associated with a network node.
 *
 * @param[in]  node         Network node configuration
 * @param[out] n_pci        Number of PCI devices
 * @param[out] pci_oids     PCI device object identifiers
 *                          (/agent/hardware/pci/device)
 *
 * @return Status code
 */
extern te_errno tapi_cfg_net_node_get_pci_oids(const cfg_net_node_t *node,
                                               unsigned int *n_pci,
                                               char ***pci_oids);

/**@} <!-- END tapi_conf_net --> */

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_CFG_NET_H__ */
