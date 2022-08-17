/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Environment Library
 *
 * @defgroup tapi_env Network environment
 * @ingroup te_ts_tapi_test
 * @{
 *
 * Optional library which can help to simplify test iteration and description
 * of the tested network environment schema. If the library is used then every
 * test in the test suite has to have argument @b env written in YACC which
 * describes location of IUT and Tester RPC servers, nets, interfaces and
 * addresses.
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TS_TAPI_ENV_ENV_H__
#define __TS_TAPI_ENV_ENV_H__

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#if HAVE_NET_IF_H
#include <net/if.h>
#endif

#include "te_defs.h"
#include "te_queue.h"
#include "conf_api.h"
#include "tapi_cfg_net.h"
#include "tapi_rpc.h"
#include "tapi_sniffer.h"


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
        memset(&env, 0, sizeof(env));                                   \
        if (argc < 1)                                                   \
        {                                                               \
            TEST_FAIL("Incorrect number of arguments for the test");    \
            return EXIT_FAILURE;                                        \
        }                                                               \
        {                                                               \
            const char *str_;                                           \
                                                                        \
            str_ = test_get_param(argc, argv, "env");                   \
            if (str_ == NULL)                                           \
            {                                                           \
                TEST_FAIL("'env' is mandatory parameter");              \
            }                                                           \
            rc = tapi_env_get(str_, &env);                              \
            if (TE_RC_GET_ERROR(rc) == TE_ETADEAD)                      \
            {                                                           \
                result = TE_EXIT_ERROR;                                 \
                ERROR("Stopping test execution because of "             \
                      "unsolvable problems");                           \
                TEST_STOP;                                              \
            }                                                           \
            else if (rc != 0)                                           \
            {                                                           \
                TEST_FAIL("tapi_env_get() failed: %s : %r", str_, rc);  \
            }                                                           \
        }                                                               \
        CFG_WAIT_CHANGES;                                               \
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
 */
#define TEST_GET_ADDR_NO_PORT(addr_) \
    do {                                                            \
        (addr_) = tapi_env_get_addr(&env, #addr_, NULL);            \
        if ((addr_) == NULL)                                        \
            TEST_STOP;                                              \
    } while (0)
/**
 * Get address and assign a free port. Name of the variable must
 * match name of the address in environment configuration string.
 *
 * @param pco_      RPC server to use when checking the port (IN)
 * @param addr_     address (const struct sockaddr *) (OUT)
 */
#define TEST_GET_ADDR(pco_, addr_) \
    do {                                                            \
        uint16_t *port_ptr;                                         \
        TEST_GET_ADDR_NO_PORT(addr_);                               \
        port_ptr = te_sockaddr_get_port_ptr(addr_);                 \
        if (port_ptr != NULL &&                                     \
            tapi_allocate_port_htons(pco_, port_ptr) != 0) {        \
            ERROR("Failed to allocate a port for address: %r", rc); \
            TEST_STOP;                                              \
        }                                                           \
    } while (0)

/**
 * Check that the address is fake. Name of the variable must
 * match name of the address in environment configuration string.
 *
 * @param addr_     address
 * @param fake_     Whether the address has FAKE type (OUT)
 */
#define CHECK_ADDR_FAKE(addr_, fake_)     \
    do {                                        \
        fake_ = FALSE;                          \
        if (tapi_get_addr_type(&env, #addr_) == \
            TAPI_ENV_ADDR_FAKE_UNICAST)         \
            fake_ = TRUE;                       \
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
        addr_ = sa_addr_;                                           \
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

/**
 * Get environment interface entity. Name of the variable must match name
 * of the interface in environment configuration string.
 *
 * @param env_if_     interface (const tapi_env_if *) (OUT)
 */
#define TEST_GET_ENV_IF(env_if_) \
    do {                                                            \
        (env_if_) = tapi_env_get_env_if(&env, #env_if_);            \
        if ((env_if_) == NULL)                                      \
        {                                                           \
            TEST_STOP;                                              \
        }                                                           \
    } while (0)

/**
 * Get XEN bridge. Name of the variable must match name
 * of the XEN bridge in environment configuration string.
 *
 * @param br_     XEN bridge (const struct if_nameindex *) (OUT)
 */
#define TEST_GET_BR(if_, br_) \
    do {                                                            \
        (br_) = tapi_env_get_br(&env, #if_);                        \
        if ((br_) == NULL)                                          \
        {                                                           \
            TEST_STOP;                                              \
        }                                                           \
    } while (0)

/**
 * Get XEN physical interface. Name of the variable must match name
 * of the XEN physical interface in environment configuration string.
 *
 * @param ph_ XEN physical interface (const struct if_nameindex *) (OUT)
 */
#define TEST_GET_PH(if_, ph_) \
    do {                                                            \
        (ph_) = tapi_env_get_ph(&env, #if_);                        \
        if ((ph_) == NULL)                                          \
        {                                                           \
            TEST_STOP;                                              \
        }                                                           \
    } while (0)


/** Types of entities in the Environment */
typedef enum {
    TAPI_ENV_UNSPEC = 0,    /**< Unspecified */
    TAPI_ENV_IUT,           /**< Implementation Under Testing */
    TAPI_ENV_TESTER,        /**< Auxiluary tester */
    TAPI_ENV_IUT_PEER,      /**< Peer of the IUT */
    TAPI_ENV_INVALID,       /**< Environment is inconsistent */
} tapi_env_type;

static inline const char *
tapi_env_type_str(tapi_env_type type)
{
    switch (type)
    {
        case TAPI_ENV_UNSPEC:
            return "Unspecified";
        case TAPI_ENV_IUT:
            return "IUT";
        case TAPI_ENV_TESTER:
            return "tester";
        case TAPI_ENV_IUT_PEER:
            return "IUT-Peer";
        case TAPI_ENV_INVALID:
            return "INVALID";
        default:
            return "Unknown";
    }
}

/** Types of addresses */
typedef enum {
    TAPI_ENV_ADDR_LOOPBACK,         /**< Loopback */
    TAPI_ENV_ADDR_UNICAST,          /**< Unicast */
    TAPI_ENV_ADDR_FAKE_UNICAST,     /**< Unicast from the same subnet,
                                         but does not assigned */
    TAPI_ENV_ADDR_MULTICAST,        /**< Multicast */
    TAPI_ENV_ADDR_MCAST_ALL_HOSTS,  /**< Multicast all hosts group */
    TAPI_ENV_ADDR_BROADCAST,        /**< Broadcast */
    TAPI_ENV_ADDR_WILDCARD,         /**< Wildcard */
    TAPI_ENV_ADDR_ALIEN,            /**< Address not assigned to any
                                         interface of the host */
    TAPI_ENV_ADDR_IP4MAPPED_UC,     /**< Unicast IPv4-mapped IPv6 address */
    TAPI_ENV_ADDR_LINKLOCAL,        /**< Link-local IPv6 address */
    TAPI_ENV_ADDR_SITELOCAL,        /**< Site-local IPv6 address */
    TAPI_ENV_ADDR_INVALID,
} tapi_env_addr_type;


/** Tail queue of Cfgr handles */
typedef struct cfg_handle_tqe {
    STAILQ_ENTRY(cfg_handle_tqe)    links;  /**< List links */
    cfg_handle                      handle; /**< Cfgr handle */
} cfg_handle_tqe;

/** Head of the tail queue with Cfgr handles */
typedef STAILQ_HEAD(cfg_handle_tqh, cfg_handle_tqe) cfg_handle_tqh;


/** Network entry */
typedef struct tapi_env_net {
    SLIST_ENTRY(tapi_env_net)   links;  /**< Links of the networks list */

    char               *name;       /**< Name of the net */

    tapi_env_type       type;       /**< Type of the net */

    unsigned int        i_net;      /**< Index of the associated net */
    cfg_net_t          *cfg_net;    /**< Configuration net */

    cfg_handle          ip4net;     /**< Handle of IPv4 addresses pool */
    struct sockaddr    *ip4addr;    /**< IPv4 address of the net */
    unsigned int        ip4pfx;     /**< IPV4 address prefix length */
    struct sockaddr_in  ip4bcast;   /**< IPv4 broadcast address
                                         of the net */

    cfg_handle          ip6net;     /**< Handle of IPv4 addresses pool */
    struct sockaddr    *ip6addr;    /**< IPv4 address of the net */
    unsigned int        ip6pfx;     /**< IPV4 address prefix length */

    cfg_handle_tqh      net_addrs;  /**< List of additional addresses */

} tapi_env_net;

/** List of required networks in environment */
typedef SLIST_HEAD(tapi_env_nets, tapi_env_net) tapi_env_nets;


/* Forward */
struct tapi_env_process;
/** List of processes on a host */
typedef SLIST_HEAD(tapi_env_processes, tapi_env_process)
            tapi_env_processes;


/** Host entry in environment */
typedef struct tapi_env_host {
    SLIST_ENTRY(tapi_env_host)  links;  /**< Links */

    tapi_env_processes  processes;  /** List of processes on a host */

    char   *name;       /**< Name of the host */
    char   *ta;         /**< Name of TA located on the host */
    char   *libname;    /**< Name of dynamic library to be used on
                             the host as IUT */

} tapi_env_host;

/** List of required hosts in environment */
typedef SLIST_HEAD(tapi_env_hosts, tapi_env_host) tapi_env_hosts;


/** Host/interface entry in environment */
typedef struct tapi_env_if {
    CIRCLEQ_ENTRY(tapi_env_if)  links;  /**< Links */

    char           *name;       /**< Name of the interface in
                                     configuration string */

    tapi_env_net   *net;        /**< Net the interface belongs to */
    tapi_env_host  *host;       /**< Host the interface is belongs to */

    unsigned int    i_node;     /**< Index of the associated node */

    enum net_node_rsrc_type rsrc_type;  /**< Type of the associated
                                             network node resource */

    struct if_nameindex if_info;/**< Interface info */
    struct if_nameindex br_info;/**< XEN bridge info */
    struct if_nameindex ph_info;/**< XEN physical interface info */

    te_bool ip4_unicast_used;   /**< Is IPv4 address assigned to
                                     the host in this net used? */
    te_bool ip6_unicast_used;   /**< Is IPv6 address assigned to
                                     the host in this net used? */

    tapi_sniffer_id    *sniffer_id;    /**< ID of the sniffer created by
                                            request via TE_ENV_SNIFF_ON */

} tapi_env_if;

/** List of host interfaces required in environment */
typedef CIRCLEQ_HEAD(tapi_env_ifs, tapi_env_if) tapi_env_ifs;


/** Process interfaces */
typedef struct tapi_env_ps_if {
    STAILQ_ENTRY(tapi_env_ps_if) links; /**< Links */

    tapi_env_if    *iface;          /**< Interface entry */
} tapi_env_ps_if;

/** List of process interfaces */
typedef STAILQ_HEAD(tapi_env_ps_ifs, tapi_env_ps_if) tapi_env_ps_ifs;


/* Forward */
struct tapi_env_pco;
/** List of PCOs */
typedef STAILQ_HEAD(tapi_env_pcos, tapi_env_pco) tapi_env_pcos;

/** Process entry on a host */
typedef struct tapi_env_process {
    SLIST_ENTRY(tapi_env_process)   links;  /**< Links */

    tapi_env_pcos       pcos; /**< Tail queue of PCOs in process */
    tapi_env_ps_ifs     ifs;  /**< List of process interfaces */

    tapi_env_net *net;  /**< Network handle */
} tapi_env_process;


/** Entry of PCO name to RPC server mapping */
typedef struct tapi_env_pco {
    STAILQ_ENTRY(tapi_env_pco)  links;  /**< Links in the process */

    char               *name;           /**< Name of the PCO */

    tapi_env_type       type;           /**< Type of PCO */
    tapi_env_process   *process;        /**< Parent process */

    rcf_rpc_server     *rpcs;           /**< RPC server handle */
    te_bool             created;        /**< Is created by this test */
} tapi_env_pco;


/** Entry of address name to real address mapping */
typedef struct tapi_env_addr {
    CIRCLEQ_ENTRY(tapi_env_addr)    links;  /**< Links */

    char                   *name;       /**< Name of the address */

    tapi_env_if            *iface;      /**< Host interface the address
                                             belongs to */

    rpc_socket_addr_family  family;     /**< Address family */
    tapi_env_addr_type      type;       /**< Address type */

    socklen_t               addrlen;    /**< Length of assigned address */
    struct sockaddr        *addr;       /**< Assigned address */
    struct sockaddr_storage addr_st;    /**< Address storage */

    cfg_handle  handle; /**< Handle of the added instance in configurator */

} tapi_env_addr;

/** List of addresses in environment */
typedef CIRCLEQ_HEAD(tapi_env_addrs, tapi_env_addr) tapi_env_addrs;


/** Alias in Socket API testing environment */
typedef struct tapi_env_alias {
    SLIST_ENTRY(tapi_env_alias) links;  /**< Links */

    char   *alias;  /**< Alias */
    char   *name;   /**< Real name */

} tapi_env_alias;

/** List of aliases in environment */
typedef SLIST_HEAD(tapi_env_aliases, tapi_env_alias) tapi_env_aliases;


/** Environment for the test */
typedef struct tapi_env {
    unsigned int        n_nets;     /**< Total number of networks */

    tapi_env_nets       nets;       /**< List of networks */
    tapi_env_hosts      hosts;      /**< List of hosts */
    tapi_env_ifs        ifs;        /**< List of interfaces */
    tapi_env_addrs      addrs;      /**< List of addresses */
    tapi_env_aliases    aliases;    /**< List of aliases */

    cfg_nets_t          cfg_nets;   /**< Configuration networks */
} tapi_env;


#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize environment variable.
 *
 * @param env       Location for environment
 *
 * @return Status code.
 */
extern te_errno tapi_env_init(tapi_env *env);

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
 * Get type address from environment by name.
 *
 * @param env       Environment
 * @param name      Name of the address in configuration string
 *
 * @return Type of address.
 */
extern tapi_env_addr_type tapi_get_addr_type(tapi_env *env,
                                             const char *name);

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
 * Get environment entity of the interface named in configuration string
 * as 'name' argument.
 *
 * Pointer becomes not valid when environment is freed.
 *
 * @param env       Environment handle
 * @param name      Name of the interface in environment
 *
 * @return Pointer to the environment interface entity.
 */
extern const tapi_env_if * tapi_env_get_env_if(tapi_env *env, const char *name);

/**
 * Get system name of the bridge named in configuration string
 * as 'name' argument.
 *
 * Pointer becomes not valid when environment is freed.
 *
 * @param env       Environment handle
 * @param name      Name of the bridge in environment
 *
 * @return Pointer to the structure with bridge name and index.
 */
extern const struct if_nameindex * tapi_env_get_br(tapi_env *env,
                                                   const char *name);

/**
 * Get system name of the physical interface  named in configuration
 * string as 'name' argument.
 *
 * Pointer becomes not valid when environment is freed.
 *
 * @param env       Environment handle
 * @param name      Name of the physical interface in environment
 *
 * @return Pointer to the structure with bridge name and index.
 */
extern const struct if_nameindex * tapi_env_get_ph(tapi_env *env,
                                                   const char *name);

/**
 * Get address assigned to the host in specified HW net and address
 * space.
 *
 * @param env           Environment
 * @param net           Environment net
 * @param host          Environment host
 * @param af            Address family of requested address
 * @param assigned      Information about assigned addresses
 * @param addr          Location for address pointer (OUT)
 * @param addrlen       Address length (OUT)
 *
 * @return Status code.
 */
extern te_errno tapi_env_get_net_host_addr(const tapi_env          *env,
                                           const tapi_env_net      *net,
                                           const tapi_env_host     *host,
                                           sa_family_t              af,
                                           tapi_cfg_net_assigned   *assigned,
                                           struct sockaddr        **addr,
                                           socklen_t               *addrlen);

/**
 * Find PCO by RPC server handle.
 */
extern const tapi_env_pco *tapi_env_rpcs2pco(const tapi_env       *env,
                                             const rcf_rpc_server *rpcs);

/**
 * Allocate and add a number of addresses to the specified interface.
 *
 * @param rpcs          RPC server handle.
 * @param net           Network addresses pool.
 * @param af            Address family.
 * @param iface         Interface handle.
 * @param addr_num      Addresses number to be added.
 *
 * @return Address pointers array or @c NULL in case of fail.
 *
 * @note The function does not roll back configuration changes in case of
 *       fail - the test execution should be aborted.
 */
extern struct sockaddr **tapi_env_add_addresses(rcf_rpc_server *rpcs,
                                           tapi_env_net *net, int af,
                                           const struct if_nameindex *iface,
                                           int addr_num);

typedef void tapi_env_foreach_if_fn(tapi_env_if *iface, void *opaque);

/**
 * Run over all interfaces mentioned in the environment and run a
 * given function for each one.
 *
 * Hook can call TEST_FAIL() if things go wrong.
 *
 * @param env         Environment
 * @param fn          Hook pointer
 * @param opaque      Hook-defined opaque
 */
extern void tapi_env_foreach_if(tapi_env *env, tapi_env_foreach_if_fn *fn,
                                void *opaque);

/**
 * Return total number of nets in the environment
 *
 * @param env     Environment
 *
 * @return Number of nets
 */
extern unsigned tapi_env_nets_count(tapi_env *env);

/**
 * Return network node of an interface.
 *
 * @param iface     Network interface
 *
 * @return Network node configuration
 */
extern const cfg_net_node_t *tapi_env_get_if_net_node(const tapi_env_if *iface);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif

/**@} <!-- END tapi_env --> */
