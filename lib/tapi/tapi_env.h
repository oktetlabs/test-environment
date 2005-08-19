/** @file
 * @brief Socket API Test Suite
 *
 * Common includes and definitions.
 *
 * Copyright (C) 2004 OKTET Labs Ltd., St.-Petersburg, Russia
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TS_SOCKTS_ENV_H__
#define __TS_SOCKTS_ENV_H__

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#if HAVE_SYS_QUEUE_H
#include <sys/queue.h>
#endif
#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#if HAVE_NET_IF_H
#include <net/if.h>
#endif

#include "te_defs.h"
#include "conf_api.h"
#include "tapi_cfg_net.h"
#include "tapi_rpc.h"


/** Maximum length name used in configuration string */
#define SOCKTS_NAME_MAX         32


/** Types of Points of Control and Observation */
typedef enum {
    SOCKTS_PCO_IUT,             /**< Implementation under testing */
    SOCKTS_PCO_TESTER,          /**< Auxiluary tester */
} sockts_pco_type;

/** Types of addresses */
typedef enum {
    SOCKTS_ADDR_LOOPBACK,       /**< Loopback */
    SOCKTS_ADDR_UNICAST,        /**< Unicast */
    SOCKTS_ADDR_FAKE_UNICAST,   /**< Unicast from the same subnet, but
                                     does not assigned */
    SOCKTS_ADDR_MULTICAST,      /**< Multicast */
    SOCKTS_ADDR_BROADCAST,      /**< Broadcast */
    SOCKTS_ADDR_WILDCARD,       /**< Wildcard */
    SOCKTS_ADDR_ALIEN,          /**< Address not assigned to any 
                                     interface of the host */
} sockts_addr_type;


/** Tail queue of Cfgr handles */
typedef struct cfg_handle_tqe {
    TAILQ_ENTRY(cfg_handle_tqe) links;  /**< List links */
    cfg_handle                  handle; /**< Cfgr handle */
} cfg_handle_tqe;

/** Head of the tail queue with Cfgr handles */
typedef TAILQ_HEAD(cfg_handle_tqh, cfg_handle_tqe) cfg_handle_tqh;


/** Network entry */
typedef struct sockts_env_net {
    LIST_ENTRY(sockts_env_net)  links;  /**< Links of the networks list */
    LIST_ENTRY(sockts_env_net)  inhost; /**< Links of the host networks list */
    
    char name[SOCKTS_NAME_MAX];   /**< Name of the net */

    unsigned int        n_hosts;  /**< Number of hosts in network */

    unsigned int        i_net;    /**< Index of the associated net */
    cfg_net_t          *cfg_net;  /**< Configuration net */

    cfg_handle          ip4net;   /**< Handle of IPv4 addresses pool */
    struct sockaddr    *ip4addr;  /**< IPv4 address of the net */
    unsigned int        ip4pfx;   /**< IPV4 address prefix length */
    struct sockaddr_in  ip4bcast; /**< IPv4 broadcast address of the net */
    cfg_handle_tqh      ip4addrs; /**< List of additional addresses */

} sockts_env_net;

/** List of required networks in environment */
typedef LIST_HEAD(sockts_env_nets, sockts_env_net) sockts_env_nets;


/* Forward */
struct sockts_env_process;
/** List of processes on a host */
typedef LIST_HEAD(sockts_env_processes, sockts_env_process)
            sockts_env_processes;

/** Host entry in environment */
typedef struct sockts_env_host {
    CIRCLEQ_ENTRY(sockts_env_host)  links;  /**< Links */

    char    name[SOCKTS_NAME_MAX];  /**< Name of the host */
    char   *ta;                     /**< Name of TA located on the host */
    char   *libname;                /**< Name of dynamic library to be
                                         used on the host as IUT */

    unsigned int arp_sync;          /**< Time to wait ARP changes
                                         propogation */
    unsigned int route_sync;        /**< Time to wait routing table
                                         changes propogation */
    
    unsigned int n_nets;        /**< Number of nets the host belongs to */

    /** List of networks the host belongs to */
    sockts_env_nets         nets;
    /** List of processes on a host */
    sockts_env_processes    processes;
    
    unsigned int i_net;         /**< Index of the associated net */
    unsigned int i_node;        /**< Index of the associated node */
    
    te_bool ip4_unicast_used;   /**< Is IPv4 address assigned to the
                                     host in this net used? */

} sockts_env_host;

/** List of hosts required in environment */
typedef CIRCLEQ_HEAD(sockts_env_hosts, sockts_env_host) sockts_env_hosts;


/* Forward */
struct sockts_env_pco;
/** List of PCOs */
typedef TAILQ_HEAD(sockts_env_pcos, sockts_env_pco) sockts_env_pcos;

/** Process entry on a host */
typedef struct sockts_env_process {
    LIST_ENTRY(sockts_env_process)  links;  /**< Links */

    sockts_env_pcos pcos;       /**< Tail queue of PCOs in process */
} sockts_env_process;


/** Entry of PCO name to RPC server mapping */
typedef struct sockts_env_pco {
    TAILQ_ENTRY(sockts_env_pco) links;  /**< Links in the process */

    char name[SOCKTS_NAME_MAX];         /**< Name of the PCO */

    sockts_pco_type     type;           /**< Type of PCO */
    sockts_env_process *process;        /**< Parent process */

    rcf_rpc_server     *rpcs;           /**< RPC server handle */
    te_bool             created;        /**< Is created by this test */
} sockts_env_pco;


/** Entry of address name to real address mapping */
typedef struct sockts_env_addr {
    CIRCLEQ_ENTRY(sockts_env_addr) links;       /**< Links */

    sockts_env_net     *net;    /**< Net the address belongs to */
    sockts_env_host    *host;   /**< Host the address belongs to */

    char    name[SOCKTS_NAME_MAX];      /**< Name of the address */

    rpc_socket_addr_family  family;     /**< Address family */
    sockts_addr_type        type;       /**< Address type */
    
    socklen_t               addrlen;    /**< Length of assigned address */
    struct sockaddr        *addr;       /**< Assigned address */
    struct sockaddr_storage addr_st;    /**< Address storage */

    cfg_handle  handle; /**< Handle of the added instance in configurator */

} sockts_env_addr;

/** List of addresses in environment */
typedef CIRCLEQ_HEAD(sockts_env_addrs, sockts_env_addr) sockts_env_addrs;


/** Entry of interface nick name to interface info mapping */
typedef struct sockts_env_if {
    LIST_ENTRY(sockts_env_if)   links;  /**< Links */

    char    name[SOCKTS_NAME_MAX];  /**< Name of the interface
                                         in configuration string */

    sockts_env_net     *net;    /**< Net the interface belongs to */
    sockts_env_host    *host;   /**< Host the interface belongs to */

    struct if_nameindex info;   /**< Interface info */

} sockts_env_if;

/** List of interfaces in environment */
typedef LIST_HEAD(sockts_env_ifs, sockts_env_if) sockts_env_ifs;


/** Alias in Socket API testing environment */
typedef struct sockts_env_alias {
    LIST_ENTRY(sockts_env_alias)    links;  /**< Links */

    char    alias[SOCKTS_NAME_MAX]; /**< Alias */
    char    name[SOCKTS_NAME_MAX];  /**< Real name */

} sockts_env_alias;

/** List of aliases in environment */
typedef LIST_HEAD(sockts_env_aliases, sockts_env_alias) sockts_env_aliases;


/** Environment for the test */
typedef struct sockts_env {
    unsigned int        n_nets;     /**< Total number of networks */

    sockts_env_nets     nets;       /**< List of networks */
    sockts_env_hosts    hosts;      /**< List of hosts */
    sockts_env_addrs    addrs;      /**< List of addresses */
    sockts_env_ifs      ifs;        /**< List of interfaces */
    sockts_env_aliases  aliases;    /**< List of aliases */

    cfg_nets_t          cfg_nets;   /**< Configuration networks */
} sockts_env;


#ifdef __cplusplus
extern "C" {
#endif

/**
 * Get Socket API test suite environment for the test.
 *
 * @param cfg       Environment configuration string
 * @param env       Location for environment
 *
 * @return Status code.
 */
extern int sockts_get_env(const char *cfg, sockts_env *env);


/**
 * Retrieve unused in system port in host order.
 *
 * @param p_port    Location for allocated port
 *
 * @return Status code.
 */
extern int sockts_allocate_port(uint16_t *p_port);

/**
 * Retrieve unused in system port in network order.
 *
 * @param p_port    Location for allocated port
 *
 * @return Status code.
 */
static inline int 
sockts_allocate_port_htons(uint16_t *p_port)
{
    uint16_t port;
    int      rc;
    
    if ((rc = sockts_allocate_port(&port)) != 0)
        return rc;
        
    *p_port = htons(port);
    
    return 0;
}

/**
 * Allocate new address from specified net.
 *
 * @param net       Net
 * @param af        Family of the address to be allocated
 * @param addr      Location for address pointer (allocated from heap)
 * @param addrlen   Location for address length of NULL
 *
 * @return Status code.
 */
extern int sockts_allocate_addr(sockts_env_net *net, int af,
                                struct sockaddr **addr,
                                socklen_t *addrlen);


/**
 * Free Socket API test suite environment.
 *
 * @param env       Environment
 *
 * @return Status code.
 */
extern int sockts_free_env(sockts_env *env);


/**
 * Get handle of the net from environment by name.
 * 
 * @param env       Environment
 * @param name      Name of the net
 *
 * @return Handle of the net.
 */
extern sockts_env_net * sockts_get_net(sockts_env *env, const char *name);

/**
 * Get handle of the host from environment by name.
 *
 * @param env       Environment
 * @param name      Name of the host
 *
 * @return Handle of the host.
 */
extern sockts_env_host * sockts_get_host(sockts_env *env, const char *name);

/**
 * Get handle of PCO (RPC server) from environment by name.
 *
 * RPC server is automatically closed and handle becomes not valid
 * when environment is freed.
 * 
 * @param env       Environment
 * @param name      Name of the PCO
 *
 * @return Handle of the RPC server.
 */
extern rcf_rpc_server *sockts_get_pco(sockts_env *env, const char *name);

/**
 * Get address from environment by name.
 *
 * Valid port is set in the structure as well.  The structure should
 * be copied, if user wants to modify its content.  Pointer becomes
 * not valid when environment is freed.
 *
 * @param env       Environment
 * @param name      Name of the address in configuration string
 * @param addrlen   Address length
 *
 * @return Pointer to address structure.
 */
extern const struct sockaddr * sockts_get_addr(sockts_env *env,
                                               const char *name,
                                               socklen_t *addrlen);

/**
 * Get system name of the interface named in configuration string
 * as 'name' argument.
 *
 * Pointer becomes not valid when environment is freed.
 *
 * @param env       Environment handle
 * @param name      Name of the interface in environment
 *
 * @return Pointer to the structure with interface name and index.
 */
extern const struct if_nameindex * sockts_get_if(sockts_env *env,
                                                 const char *name);

/**
 * Get address assigned to the host in specified HW net and address
 * space.
 *
 * @param net           Environment net
 * @param host          Environment host
 * @param assigned      Information about assigned addresses
 * @param addr          Location for address pointer (OUT)
 * @param addrlen       Address length (OUT)
 *
 * @return Status code.
 */
extern int sockts_get_net_host_addr(const sockts_env_net   *net,
                                    const sockts_env_host  *host,
                                    tapi_cfg_net_assigned  *assigned,
                                    struct sockaddr       **addr,
                                    socklen_t              *addrlen);
                                    
/**
 * Delete all addresses on a given interface, except of addr_to_save.
 *
 * @param ta            Test Agent name
 * @param if_name       interface name
 * @param addr_to_save  address to save on interface.
 *                      If this parameter is NULL, then save
 *                      the first address in address list returned by
 *                      'ip addr list' output.
 *
 * @return Status code
 */ 
extern int sockts_del_if_addresses(const char *ta,
                                   const char *if_name,
                                   const struct sockaddr *addr_to_save);
                                    
#ifdef __cplusplus
} /* extern "C" */
#endif
#endif
