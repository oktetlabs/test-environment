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

#ifndef __TS_TAPI_ENV_ENV_H__
#define __TS_TAPI_ENV_ENV_H__

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
#define TAPI_ENV_NAME_MAX       32


/**
 * Test suite specific variables with 'env' support.
 */
#define TEST_START_ENV_VARS \
    tapi_env    env;

/**
 * Test suite specific the first actions of the test with 'env' support.
 */
#define TEST_START_ENV \
    do {                                                                \
        if (argc < 1)                                                   \
        {                                                               \
            TEST_FAIL("Incorrect number of arguments for the test");    \
            return EXIT_FAILURE;                                        \
        }                                                               \
        memset(&env, 0, sizeof(env));                                   \
        {                                                               \
            const char *str_;                                           \
                                                                        \
            str_ = test_get_param(argc, argv, "env");                   \
            if (str_ == NULL)                                           \
            {                                                           \
                TEST_FAIL("'env' is mandatory parameter");              \
            }                                                           \
            rc = tapi_env_get(str_, &env);                              \
            if (rc != 0)                                                \
            {                                                           \
                TEST_FAIL("tapi_env_get() failed: %s : %r", str_, rc);  \
            }                                                           \
        }                                                               \
    } while (0)

/**
 * Test suite specific part of the last action with 'env' support.
 */
#define TEST_END_ENV \
    do {                                                \
        rc = tapi_env_free(&env);                       \
        if (rc != 0)                                    \
        {                                               \
            ERROR("tapi_env_free() failed: %r", rc);    \
            result = EXIT_FAILURE;                      \
        }                                               \
    } while (0)


/**
 * Get network. Name of the variable must match name of the network 
 * in environment configuration string.
 *
 * @param net_      Pointer to tapi_env_net (OUT)
 */
#define TEST_GET_NET(net_) \
    do {                                        \
        (net_) = tapi_env_get_net(&env, #net_); \
        if ((net_) == NULL)                     \
        {                                       \
            TEST_STOP;                          \
        }                                       \
    } while (0)

/**
 * Get named host from environment. Name of the variable must match 
 * name of the host in environment configuration string.
 *
 * @param host_     Pointer to tapi_env_host (OUT)
 */
#define TEST_GET_HOST(host_) \
    do {                                            \
        (host_) = tapi_env_get_host(&env, #host_);  \
        if ((host_) == NULL)                        \
        {                                           \
            TEST_STOP;                              \
        }                                           \
    } while (0)

/**
 * Get PCO (RPC server) handle. Name of the variable must match
 * name of the PCO in environment configuration string.
 *
 * @param rpcs_     RPC server handle (OUT)
 */
#define TEST_GET_PCO(rpcs_) \
    do {                                                            \
        (rpcs_) = tapi_env_get_pco(&env, #rpcs_);                   \
        if ((rpcs_) == NULL)                                        \
        {                                                           \
            TEST_STOP;                                              \
        }                                                           \
    } while (0)

/**
 * Get address. Name of the variable must match name of the address
 * in environment configuration string.
 *
 * @param addr_     address (const struct sockaddr *) (OUT)
 * @param addrlen_  address length (OUT)
 */
#define TEST_GET_ADDR(addr_, addrlen_) \
    do {                                                            \
        (addr_) = tapi_env_get_addr(&env, #addr_, &(addrlen_));     \
        if ((addr_) == NULL)                                        \
        {                                                           \
            TEST_STOP;                                              \
        }                                                           \
    } while (0)

/**
 * Get the value of link-layer address parameter.
 * Name of the variable must match name of the address
 * in environment configuration string.
 *
 * @param addr_     address (const unsigned char *) (OUT)
 */
#define TEST_GET_LINK_ADDR(addr_) \
    do {                                                            \
        const struct sockaddr *sa_addr_;                            \
        socklen_t              sa_addr_len_;                        \
                                                                    \
        sa_addr_ = tapi_env_get_addr(&env, #addr_, &(sa_addr_len_));\
        if ((sa_addr_) == NULL)                                     \
        {                                                           \
            TEST_STOP;                                              \
        }                                                           \
        if ((sa_addr_)->sa_family != AF_LOCAL)                      \
        {                                                           \
            TEST_FAIL("'" #addr_ "' parameter is not "              \
                      "a link layer address family: %s",            \
                      addr_family_rpc2str(addr_family_h2rpc(        \
                          (sa_addr_)->sa_family)));                 \
        }                                                           \
        addr_ = (sa_addr_)->sa_data;                                \
    } while (0)

/**
 * Get interface. Name of the variable must match name of the interface
 * in environment configuration string.
 *
 * @param if_     interface (const struct if_nameindex *) (OUT)
 */
#define TEST_GET_IF(if_) \
    do {                                                            \
        (if_) = tapi_env_get_if(&env, #if_);                        \
        if ((if_) == NULL)                                          \
        {                                                           \
            TEST_STOP;                                              \
        }                                                           \
    } while (0)


/** Types of entities in the Environment */
typedef enum {
    TAPI_ENV_IUT,       /**< Implementation Under Testing */
    TAPI_ENV_TESTER,    /**< Auxiluary tester */
} tapi_env_type;

/** Types of addresses */
typedef enum {
    TAPI_ENV_ADDR_LOOPBACK,     /**< Loopback */
    TAPI_ENV_ADDR_UNICAST,      /**< Unicast */
    TAPI_ENV_ADDR_FAKE_UNICAST, /**< Unicast from the same subnet, but
                                     does not assigned */
    TAPI_ENV_ADDR_MULTICAST,    /**< Multicast */
    TAPI_ENV_ADDR_BROADCAST,    /**< Broadcast */
    TAPI_ENV_ADDR_WILDCARD,     /**< Wildcard */
    TAPI_ENV_ADDR_ALIEN,        /**< Address not assigned to any 
                                     interface of the host */
} tapi_env_addr_type;


/** Tail queue of Cfgr handles */
typedef struct cfg_handle_tqe {
    TAILQ_ENTRY(cfg_handle_tqe) links;  /**< List links */
    cfg_handle                  handle; /**< Cfgr handle */
} cfg_handle_tqe;

/** Head of the tail queue with Cfgr handles */
typedef TAILQ_HEAD(cfg_handle_tqh, cfg_handle_tqe) cfg_handle_tqh;


/** Network entry */
typedef struct tapi_env_net {
    LIST_ENTRY(tapi_env_net)    links;  /**< Links of the networks list */
    LIST_ENTRY(tapi_env_net)    inhost; /**< Links of the host networks
                                             list */
    
    char name[TAPI_ENV_NAME_MAX];   /**< Name of the net */

    unsigned int        n_hosts;    /**< Number of hosts in network */

    unsigned int        i_net;      /**< Index of the associated net */
    cfg_net_t          *cfg_net;    /**< Configuration net */

    cfg_handle          ip4net;     /**< Handle of IPv4 addresses pool */
    struct sockaddr    *ip4addr;    /**< IPv4 address of the net */
    unsigned int        ip4pfx;     /**< IPV4 address prefix length */
    struct sockaddr_in  ip4bcast;   /**< IPv4 broadcast address
                                         of the net */
    cfg_handle_tqh      ip4addrs;   /**< List of additional addresses */

} tapi_env_net;

/** List of required networks in environment */
typedef LIST_HEAD(tapi_env_nets, tapi_env_net) tapi_env_nets;


/* Forward */
struct tapi_env_process;
/** List of processes on a host */
typedef LIST_HEAD(tapi_env_processes, tapi_env_process)
            tapi_env_processes;

/** Host entry in environment */
typedef struct tapi_env_host {
    CIRCLEQ_ENTRY(tapi_env_host)    links;  /**< Links */

    char    name[TAPI_ENV_NAME_MAX];    /**< Name of the host */

    char   *ta;                 /**< Name of TA located on the host */
    char   *libname;            /**< Name of dynamic library to be
                                     used on the host as IUT */

    unsigned int arp_sync;      /**< Time to wait ARP changes
                                     propogation */
    unsigned int route_sync;    /**< Time to wait routing table
                                     changes propogation */
    
    unsigned int n_nets;        /**< Number of nets the host belongs to */

    /** List of networks the host belongs to */
    tapi_env_nets         nets;
    /** List of processes on a host */
    tapi_env_processes    processes;
    
    unsigned int i_net;         /**< Index of the associated net */
    unsigned int i_node;        /**< Index of the associated node */
    
    te_bool ip4_unicast_used;   /**< Is IPv4 address assigned to the
                                     host in this net used? */

} tapi_env_host;

/** List of hosts required in environment */
typedef CIRCLEQ_HEAD(tapi_env_hosts, tapi_env_host) tapi_env_hosts;


/* Forward */
struct tapi_env_pco;
/** List of PCOs */
typedef TAILQ_HEAD(tapi_env_pcos, tapi_env_pco) tapi_env_pcos;

/** Process entry on a host */
typedef struct tapi_env_process {
    LIST_ENTRY(tapi_env_process)    links;  /**< Links */

    tapi_env_pcos pcos; /**< Tail queue of PCOs in process */
} tapi_env_process;


/** Entry of PCO name to RPC server mapping */
typedef struct tapi_env_pco {
    TAILQ_ENTRY(tapi_env_pco) links;    /**< Links in the process */

    char name[TAPI_ENV_NAME_MAX];       /**< Name of the PCO */

    tapi_env_type       type;           /**< Type of PCO */
    tapi_env_process   *process;        /**< Parent process */

    rcf_rpc_server     *rpcs;           /**< RPC server handle */
    te_bool             created;        /**< Is created by this test */
} tapi_env_pco;


/** Entry of address name to real address mapping */
typedef struct tapi_env_addr {
    CIRCLEQ_ENTRY(tapi_env_addr)    links;  /**< Links */

    tapi_env_net   *net;    /**< Net the address belongs to */
    tapi_env_host  *host;   /**< Host the address belongs to */

    char name[TAPI_ENV_NAME_MAX];       /**< Name of the address */

    rpc_socket_addr_family  family;     /**< Address family */
    tapi_env_addr_type      type;       /**< Address type */
    
    socklen_t               addrlen;    /**< Length of assigned address */
    struct sockaddr        *addr;       /**< Assigned address */
    struct sockaddr_storage addr_st;    /**< Address storage */

    cfg_handle  handle; /**< Handle of the added instance in configurator */

} tapi_env_addr;

/** List of addresses in environment */
typedef CIRCLEQ_HEAD(tapi_env_addrs, tapi_env_addr) tapi_env_addrs;


/** Entry of interface nick name to interface info mapping */
typedef struct tapi_env_if {
    LIST_ENTRY(tapi_env_if) links;  /**< Links */

    char name[TAPI_ENV_NAME_MAX];   /**< Name of the interface
                                         in configuration string */

    tapi_env_net   *net;    /**< Net the interface belongs to */
    tapi_env_host  *host;   /**< Host the interface belongs to */

    struct if_nameindex info;   /**< Interface info */

} tapi_env_if;

/** List of interfaces in environment */
typedef LIST_HEAD(tapi_env_ifs, tapi_env_if) tapi_env_ifs;


/** Alias in Socket API testing environment */
typedef struct tapi_env_alias {
    LIST_ENTRY(tapi_env_alias)  links;  /**< Links */

    char    alias[TAPI_ENV_NAME_MAX];   /**< Alias */
    char    name[TAPI_ENV_NAME_MAX];    /**< Real name */

} tapi_env_alias;

/** List of aliases in environment */
typedef LIST_HEAD(tapi_env_aliases, tapi_env_alias) tapi_env_aliases;


/** Environment for the test */
typedef struct tapi_env {
    unsigned int        n_nets;     /**< Total number of networks */

    tapi_env_nets       nets;       /**< List of networks */
    tapi_env_hosts      hosts;      /**< List of hosts */
    tapi_env_addrs      addrs;      /**< List of addresses */
    tapi_env_ifs        ifs;        /**< List of interfaces */
    tapi_env_aliases    aliases;    /**< List of aliases */

    cfg_nets_t          cfg_nets;   /**< Configuration networks */
} tapi_env;


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
extern te_errno tapi_env_get(const char *cfg, tapi_env *env);


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
extern te_errno tapi_env_allocate_addr(tapi_env_net *net, int af,
                                       struct sockaddr **addr,
                                       socklen_t *addrlen);


/**
 * Free Socket API test suite environment.
 *
 * @param env       Environment
 *
 * @return Status code.
 */
extern te_errno tapi_env_free(tapi_env *env);


/**
 * Get handle of the net from environment by name.
 * 
 * @param env       Environment
 * @param name      Name of the net
 *
 * @return Handle of the net.
 */
extern tapi_env_net * tapi_env_get_net(tapi_env *env, const char *name);

/**
 * Get handle of the host from environment by name.
 *
 * @param env       Environment
 * @param name      Name of the host
 *
 * @return Handle of the host.
 */
extern tapi_env_host * tapi_env_get_host(tapi_env *env, const char *name);

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
extern rcf_rpc_server *tapi_env_get_pco(tapi_env *env, const char *name);

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
extern const struct sockaddr * tapi_env_get_addr(tapi_env *env,
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
extern const struct if_nameindex * tapi_env_get_if(tapi_env *env,
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
extern te_errno tapi_env_get_net_host_addr(const tapi_env_net      *net,
                                           const tapi_env_host     *host,
                                           tapi_cfg_net_assigned   *assigned,
                                           struct sockaddr        **addr,
                                           socklen_t               *addrlen);
                                    
#ifdef __cplusplus
} /* extern "C" */
#endif
#endif
