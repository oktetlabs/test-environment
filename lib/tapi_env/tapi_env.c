/** @file
 * @brief Socket API Test Suite
 *
 * Environment allocation and destruction.
 *
 * Copyright (C) 2004 OKTET Labs Ltd., St.-Petersburg, Russia
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
 */

/** Logger subsystem user of the library */
#define TE_LGR_USER "Environment LIB"

#include "te_config.h"

#ifdef STDC_HEADERS
#include <stdlib.h>
#include <string.h>
#endif
#if HAVE_TIME_H 
#include <time.h>
#endif
#if HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#if HAVE_NET_ETHERNET_H
#include <net/ethernet.h>
#endif
#if HAVE_PTHREAD_H
#include <pthread.h>
#endif

#include "tapi_env.h"

#include "logger_api.h"
#include "conf_api.h"
#include "rcf_rpc.h"
#include "tapi_cfg.h"
#include "tapi_cfg_net.h"
#include "tapi_cfg_base.h"
#include "tapi_rpc.h"
#include "tapi_sockaddr.h"



/**
 * Function provided by FLEX.
 *
 * @param e     Pointer to environment.
 * @param cfg   Configuration.
 *
 * @return Status code.
 */
extern int env_cfg_parse(tapi_env *e, const char *cfg);


/** Entry of the list with network node indexes */
typedef struct node_index {
    LIST_ENTRY(node_index)  links;  /**< Links */
    unsigned int            net;    /**< Net index */
    unsigned int            node;   /**< Node in the net index */
} node_index;

/** List of the network node indexes */
typedef LIST_HEAD(node_indexes, node_index) node_indexes;


static int prepare_nets(tapi_env_nets *nets, cfg_nets_t *cfg_nets);
static int prepare_hosts(tapi_env_hosts *hosts, cfg_nets_t *cfg_nets);
static int prepare_addresses(tapi_env_addrs *addrs, cfg_nets_t *cfg_nets);
static int prepare_interfaces(tapi_env_ifs *ifs, cfg_nets_t *cfg_nets);
static int prepare_pcos(tapi_env_hosts *hosts);

static int add_address(tapi_env_addr *env_addr, cfg_nets_t *cfg_nets,
                       const struct sockaddr *addr);

static int bind_env_hosts_to_cfg_nets(tapi_env_hosts *hosts,
                                      cfg_nets_t *cfg_nets);
static te_bool bind_host(tapi_env_host *host, tapi_env_hosts *hosts,
                         cfg_nets_t *cfg_nets, node_indexes *used_nodes);

static int cmp_agent_names(cfg_handle node1, cfg_handle node2);

static int node_value_get_ith_inst_name(cfg_handle node, unsigned int i,
                                        char **p_str);

static int node_mark_used(node_indexes *used_nodes,
                          unsigned int net, unsigned int node);
static void node_unmark_used(node_indexes *used_nodes,
                             unsigned int net, unsigned int node);
static te_bool node_is_used(node_indexes *used_nodes,
                            unsigned int net, unsigned int node);

static te_bool check_node_type_vs_net(cfg_net_t *net, unsigned int node_i,
                                      tapi_env_type net_type);

static te_bool check_node_type_vs_pcos(enum net_node_type node_type,
                                       tapi_env_host *host);


/* See description in tapi_env.h */
te_errno
tapi_env_allocate_addr(tapi_env_net *net, int af,
                     struct sockaddr **addr, socklen_t *addrlen)
{
    int             rc;
    cfg_handle_tqe *handle;


    if (net == NULL)
        return TE_EINVAL;
    if (addr == NULL)
    {
        ERROR("Invalid location for address pointer");
        return TE_EINVAL;
    }
    if (af != AF_INET)
    {
        ERROR("Address family %d is not supported", af);
        return TE_EINVAL;
    }

    handle = calloc(1, sizeof(*handle));
    if (handle == NULL)
    {
        ERROR("calloc(1, %u) failed", sizeof(handle));
        return TE_ENOMEM;
    }

    rc = tapi_cfg_alloc_ip4_addr(net->ip4net, &handle->handle,
                                 (struct sockaddr_in **)addr);
    if (rc != 0)
    {
        ERROR("Failed to allocate IPv4 address in subnet 0x%x: %r",
              net->ip4net, rc);
        free(handle);
        return rc;
    }
    if (addrlen != NULL)
        *addrlen = sizeof(struct sockaddr_in);

    TAILQ_INSERT_TAIL(&net->ip4addrs, handle, links);

    return 0;
}


/* See description in tapi_env.h */
te_errno
tapi_env_get(const char *cfg, tapi_env *env)
{
    int         rc;


    if (cfg == NULL || env == NULL)
    {
        ERROR("Invalid argument: %s(0x%x, 0x%x)",
              __FUNCTION__, cfg, env);
        return TE_EINVAL;
    }

    /* Initialize lists */
    env->n_nets = 0;
    LIST_INIT(&env->nets);
    CIRCLEQ_INIT(&env->hosts);
    CIRCLEQ_INIT(&env->addrs);
    LIST_INIT(&env->ifs);
    LIST_INIT(&env->aliases);

    /* Parse environment configuration string */
    rc = env_cfg_parse(env, cfg);
    if (rc != 0)
    {
        ERROR("Invalid environment configuration string: %s", cfg);
        return TE_EFMT;
    }
    VERB("Environment configuration string '%s' successfully parsed", cfg);

    /* canonicalize_hosts(env); */

    /* Get available networks configuration */
    rc = tapi_cfg_net_get_nets(&env->cfg_nets);
    if (rc != 0)
    {
        ERROR("Failed to get networks from Configurator: %r", rc);
        return rc;
    }

    if (env->cfg_nets.n_nets < env->n_nets)
    {
        ERROR("Too few networks in available configuration (%u) in "
              "comparison with required (%u)",
              env->cfg_nets.n_nets, env->n_nets);
        return TE_EENV;
    }
    
    rc = bind_env_hosts_to_cfg_nets(&env->hosts, &env->cfg_nets);
    if (rc != 0)
    {
        /* ERROR is logged in bind_env_hosts_to_cfg_nets function */
        return rc;
    }

    rc = prepare_nets(&env->nets, &env->cfg_nets);
    if (rc != 0)
    {
        ERROR("Failed to prepare networks");
        return rc;
    }

    rc = prepare_hosts(&env->hosts, &env->cfg_nets);
    if (rc != 0)
    {
        ERROR("Failed to prepare hosts");
        return rc;
    }

    rc = prepare_addresses(&env->addrs, &env->cfg_nets);
    if (rc != 0)
    {
        ERROR("Failed to prepare addresses");
        return rc;
    }

    rc = prepare_interfaces(&env->ifs, &env->cfg_nets);
    if (rc != 0)
    {
        ERROR("Failed to prepare interfaces");
        return rc;
    }

    rc = prepare_pcos(&env->hosts);
    if (rc != 0)
    {
        ERROR("Failed to prepare PCOs");
        return rc;
    }

    return 0;
}


/* See description in tapi_env.h */
te_errno
tapi_env_free(tapi_env *env)
{
    int               result = 0;
    int               rc;
    tapi_env_host    *host;
    tapi_env_process *proc;
    tapi_env_pco     *pco;
    tapi_env_addr    *addr;
    tapi_env_net     *net;
    cfg_handle_tqe   *addr_hndl;
    tapi_env_if      *iface;
    tapi_env_alias   *alias;
    cfg_val_type      type;
    int               n_entries;
    int               n_deleted;
    cfg_handle        ip4_net_hndl;
    char             *ip4_net_oid;

    if (env == NULL)
        return 0;

    /* Destroy list of hosts */
    while ((host = env->hosts.cqh_first) != (void *)&env->hosts)
    {
        /* Destroy list of processes */
        while ((proc = host->processes.lh_first) != NULL)
        {
            /* Destroy PCOs */
            while ((pco = proc->pcos.tqh_first) != NULL)
            {
                if (pco->rpcs != NULL)
                {
                    VERB("Destroy RPC Server (%s,%s)",
                         pco->rpcs->ta, pco->rpcs->name);
                         
                    if (pco->created)
                    {
                        rc = rcf_rpc_server_destroy(pco->rpcs);
                        if (rc != 0)
                        {
                            ERROR("rcf_rpc_server_destroy() failed: %r", 
                                  rc);
                            TE_RC_UPDATE(result, rc);
                        }
                    }
                    else
                    {
                        if (pco->rpcs->timed_out)
                            rcf_rpc_server_restart(pco->rpcs);
                        free(pco->rpcs);
                    }
                }
                TAILQ_REMOVE(&proc->pcos, pco, links);
                free(pco);
            }
            LIST_REMOVE(proc, links);
            free(proc);
        }
        CIRCLEQ_REMOVE(&env->hosts, host, links);
        free(host);
    }
    /* Destroy list of addresses in reverse order */
    while ((addr = env->addrs.cqh_last) != (void *)&env->addrs)
    {
        if (addr->handle != CFG_HANDLE_INVALID)
        {
            rc = cfg_del_instance(addr->handle, FALSE);
            if (rc != 0)
            {
                ERROR("%s(): cfg_del_instance() failed: %r",
                      __FUNCTION__, rc);
                TE_RC_UPDATE(result, rc);
            }
        }
        CIRCLEQ_REMOVE(&env->addrs, addr, links);
        free(addr);
    }
    /* Destroy list of nets */
    while ((net = env->nets.lh_first) != NULL)
    {
        LIST_REMOVE(net, links);
        n_deleted = 0;
        ip4_net_oid = NULL;
        while ((addr_hndl = net->ip4addrs.tqh_first) != NULL)
        {
            TAILQ_REMOVE(&net->ip4addrs, addr_hndl, links);
            if (n_deleted == 0)
            {
                if (((rc = cfg_get_father(addr_hndl->handle,
                                          &ip4_net_hndl)) != 0) ||
                    ((rc = cfg_get_father(ip4_net_hndl,
                                          &ip4_net_hndl)) != 0))

                {
                    ERROR("cfg_get_father() failed: %r", rc);
                    TE_RC_UPDATE(result, rc);
                }
                else if ((rc = cfg_get_oid_str(ip4_net_hndl,
                                               &ip4_net_oid)) != 0)
                {
                    ERROR("cfg_get_oid_str() failed: %r", rc);
                    TE_RC_UPDATE(result, rc);
                }
            }
            rc = cfg_del_instance(addr_hndl->handle, FALSE);
            if (rc != 0)
            {
                ERROR("Failed to delete IPv4 address pool entry: %r", rc);
                TE_RC_UPDATE(result, rc);
            }
            free(addr_hndl);
            ++n_deleted;
        }
        if (ip4_net_oid != NULL)
        {
            type = CVT_INTEGER;
            rc = cfg_get_instance_fmt(&type, &n_entries,
                                      "%s/n_entries:", ip4_net_oid);
            if (rc != 0)
            {
                ERROR("Failed to get number of entries in the pool: %r",
                      rc);
                TE_RC_UPDATE(result, rc);
            }
            else
            {
                n_entries -= n_deleted;
                rc = cfg_set_instance_fmt(CFG_VAL(INTEGER, n_entries),
                                          "%s/n_entries:", ip4_net_oid);
                if (rc != 0)
                {
                    ERROR("Failed to set number of entries in the pool: %r",
                          rc);
                    TE_RC_UPDATE(result, rc);
                }
            }
            free(ip4_net_oid);
        }
        free(net);
    }
    /* Destroy list of interfaces */
    while ((iface = env->ifs.lh_first) != NULL)
    {
        LIST_REMOVE(iface, links);
        free(iface);
    }
    /* Destroy list of aliases */
    while ((alias = env->aliases.lh_first) != NULL)
    {
        LIST_REMOVE(alias, links);
        free(alias);
    }

    tapi_cfg_net_free_nets(&env->cfg_nets);
    
    return result;
}


/* See description in tapi_env.h */
tapi_env_net *
tapi_env_get_net(tapi_env *env, const char *name)
{
    tapi_env_net     *p;
    tapi_env_alias   *a;

    for (a = env->aliases.lh_first; a != NULL; a = a->links.le_next)
    {
        if (strcmp(a->alias, name) == 0)
        {
            VERB("'%s' is alias of '%s'", name, a->name);
            name = a->name;
            break;
        }
    }
    for (p = env->nets.lh_first; p != NULL; p = p->links.le_next)
    {
        if (strcmp(p->name, name) == 0)
            return p;
    }

    WARN("Net '%s' does not exist in environment", name);

    return NULL;
}

/* See description in tapi_env.h */
tapi_env_host *
tapi_env_get_host(tapi_env *env, const char *name)
{
    tapi_env_host    *p;
    tapi_env_alias   *a;

    for (a = env->aliases.lh_first; a != NULL; a = a->links.le_next)
    {
        if (strcmp(a->alias, name) == 0)
        {
            VERB("'%s' is alias of '%s'", name, a->name);
            name = a->name;
            break;
        }
    }
    for (p = env->hosts.cqh_first;
         p != (void *)&(env->hosts);
         p = p->links.cqe_next)
    {
        if (strcmp(p->name, name) == 0)
            return p;
    }

    WARN("Host '%s' does not exist in environment", name);

    return NULL;
}

/* See description in tapi_env.h */
rcf_rpc_server *
tapi_env_get_pco(tapi_env *env, const char *name)
{
    tapi_env_alias   *a;
    tapi_env_host    *host;
    tapi_env_process *proc;
    tapi_env_pco     *pco;

    for (a = env->aliases.lh_first; a != NULL; a = a->links.le_next)
    {
        if (strcmp(a->alias, name) == 0)
        {
            VERB("'%s' is alias of '%s'", name, a->name);
            name = a->name;
            break;
        }
    }

    for (host = env->hosts.cqh_first;
         host != (void *)&env->hosts;
         host = host->links.cqe_next)
    {
        for (proc = host->processes.lh_first;
             proc != NULL;
             proc = proc->links.le_next)
        {
            for (pco = proc->pcos.tqh_first;
                 pco != NULL;
                 pco = pco->links.tqe_next)
            {
                if (strcmp(pco->name, name) == 0)
                    return pco->rpcs;
            }
        }
    }

    WARN("PCO '%s' does not exist in environment", name);

    return NULL;
}


/* See description in tapi_env.h */
const struct sockaddr *
tapi_env_get_addr(tapi_env *env, const char *name, socklen_t *addrlen)
{
    tapi_env_addr    *p;
    tapi_env_alias   *a;

    for (a = env->aliases.lh_first; a != NULL; a = a->links.le_next)
    {
        if (strcmp(a->alias, name) == 0)
        {
            VERB("'%s' is alias of '%s'", name, a->name);
            name = a->name;
            break;
        }
    }
    for (p = env->addrs.cqh_first;
         p != (void *)&env->addrs;
         p = p->links.cqe_next)
    {
        if (strcmp(p->name, name) == 0)
        {
            *addrlen = p->addrlen;
            return p->addr;
        }
    }

    WARN("Address '%s' does not exist in environment", name);

    return NULL;
}


/* See description in tapi_env.h */
const struct if_nameindex *
tapi_env_get_if(tapi_env *env, const char *name)
{
    tapi_env_if      *p;
    tapi_env_alias   *a;

    for (a = env->aliases.lh_first; a != NULL; a = a->links.le_next)
    {
        if (strcmp(a->alias, name) == 0)
        {
            VERB("'%s' is alias of '%s'", name, a->name);
            name = a->name;
            break;
        }
    }
    for (p = env->ifs.lh_first; p != NULL; p = p->links.le_next)
    {
        if (strcmp(p->name, name) == 0)
            return &p->info;
    }

    WARN("Interface '%s' does not exist in environment", name);

    return NULL;
}


/**
 * Prepare environment networks.
 *
 * @param ifs           list of environment networks
 *
 * @return Status code.
 */
static int
prepare_nets(tapi_env_nets *nets, cfg_nets_t *cfg_nets)
{
    int             rc = 0;
    tapi_env_net *env_net;
    cfg_val_type    val_type;
    unsigned int    i;
    char           *net_oid;
    unsigned int    n_ip4_nets;
    cfg_handle     *ip4_nets;
    char           *ip4_net_oid;


    for (env_net = nets->lh_first, i = 0;
         env_net != NULL;
         env_net = env_net->links.le_next, ++i)
    {
        env_net->cfg_net = cfg_nets->nets + env_net->i_net;

        /* Get string OID of the associated net in networks configuration */
        rc = cfg_get_oid_str(env_net->cfg_net->handle, &net_oid);
        if (rc != 0)
        {
            ERROR("Failed to string OID by handle: %r", rc);
            break;
        }

        /* Get IPv4 subnet for the network */
        rc = cfg_find_pattern_fmt(&n_ip4_nets, &ip4_nets,
                                  "%s/ip4_subnet:*", net_oid);
        if (rc != 0)
        {
            ERROR("Failed to find IPv4 subnets assigned to net '%s': %r",
                  net_oid, rc);
            free(net_oid);
            break;
        }
        if (n_ip4_nets <= 0)
        {
            ERROR("No IPv4 networks are assigned to net '%s'", net_oid);
            free(net_oid);
            rc = TE_EENV;
            break;
        }
        
        /* Get IPv4 subnet address */
        val_type = CVT_ADDRESS;
        rc = cfg_get_instance(ip4_nets[0], &val_type, &env_net->ip4addr);
        if (rc != 0)
        {
            ERROR("Failed to get IPv4 subnet for net '%s': %r",
                  net_oid, rc);
            free(ip4_nets);
            free(net_oid);
            break;
        }
        free(net_oid);

        /* Get IPv4 subnet handle */
        rc = cfg_get_inst_name_type(ip4_nets[0], CVT_INTEGER,
                                    CFG_IVP(&env_net->ip4net));
        if (rc != 0)
        {
            ERROR("Failed to get IPv4 subnet handle: %r", rc);
            free(ip4_nets);
            break;
        }
        /* Get IPv4 subnet OID */
        rc = cfg_get_oid_str(env_net->ip4net, &ip4_net_oid);
        if (rc != 0)
        {
            ERROR("cfg_get_oid_str() failed: %r", rc);
            free(ip4_nets);
            break;
        }
        free(ip4_nets);

        /* Get IPv4 subnet for the network */
        val_type = CVT_INTEGER;
        rc = cfg_get_instance_fmt(&val_type, &env_net->ip4pfx,
                                  "%s/prefix:", ip4_net_oid);
        if (rc != 0)
        {
            ERROR("Failed to get IPv4 prefix length for configuration "
                  "network %s: %r", ip4_net_oid, rc);
            free(ip4_net_oid);
            break;
        }
        free(ip4_net_oid);

        /*
         * Prepare IPv4 broadcast address in accordance with got
         * IPv4 subnet.
         */
        memcpy(&env_net->ip4bcast, env_net->ip4addr,
               sizeof(env_net->ip4bcast));
        {
            in_addr_t addr = ntohl(env_net->ip4bcast.sin_addr.s_addr);

            addr |= (1 << ((sizeof(in_addr_t) << 3) - env_net->ip4pfx)) - 1;
            env_net->ip4bcast.sin_addr.s_addr = htonl(addr);
        }
    }

    return rc;
}

/**
 * Prepare required hosts in accordance with bound network configuration.
 *
 * @param hosts         list of hosts
 * @param cfg_nets      networks configuration
 *
 * @return Status code.
 */
static int
prepare_hosts(tapi_env_hosts *hosts, cfg_nets_t *cfg_nets)
{
    int                 rc = 0;
    tapi_env_host    *host;
    cfg_handle          handle;
    cfg_val_type        type;

    for (host = hosts->cqh_first;
         host != (void *)hosts;
         host = host->links.cqe_next)
    {
        /* Get name of the Test Agent */
        rc = node_value_get_ith_inst_name(
                 cfg_nets->nets[host->i_net].nodes[host->i_node].handle,
                 1, &host->ta);
        if (rc != 0)
            break;
        
        /* Get name of the dynamic library with IUT */
        rc = cfg_find_fmt(&handle, "/local:%s/socklib:", host->ta);
        if (rc != 0)
        {
            if (TE_RC_GET_ERROR(rc) == TE_ENOENT)
            {
                host->libname = NULL;
                rc = 0;
            }
            else
            {
                ERROR("Unexpected Configurator failure: %r", rc);
                break;
            }
        }
        else
        {
            type = CVT_STRING;
            rc = cfg_get_instance(handle, &type, &host->libname);
            if (rc != 0)
            {
                ERROR("Failed to get instance by handle 0x%x: %r",
                      handle, rc);
                break;
            }
        }
    }

    return rc;
}


te_errno
prepare_ip4_unicast(tapi_env_addr *env_addr, cfg_nets_t *cfg_nets,
                    struct sockaddr **addr)
{
    int rc;

    assert(addr != NULL);

    if (env_addr->host->ip4_unicast_used)
    {
        rc = tapi_env_allocate_addr(env_addr->net, AF_INET, addr, NULL);
        if (rc != 0)
        {
            ERROR("Failed to allocate additional IPv4 "
                  "address: %r", rc);
            return rc;
        }

        rc = add_address(env_addr, cfg_nets, *addr);
        if (rc != 0)
        {
            ERROR("Failed to add address");
            return rc;
        }
    }
    else
    {
        cfg_handle      handle;
        char           *node_oid;
        int             ip4_addrs_num;
        cfg_handle     *ip4_addrs;
        cfg_val_type    val_type;

        /* Handle of the assosiated network node */
        handle = cfg_nets->nets[env_addr->host->i_net].
                     nodes[env_addr->host->i_node].handle;

        /* Get IPv4 address assigned to the node */
        rc = cfg_get_oid_str(handle, &node_oid);
        if (rc != 0)
        {
            ERROR("Failed to string OID by handle: %r", rc);
            return rc;
        }

        /* Get IPv4 addresses of the node */
        rc = cfg_find_pattern_fmt(&ip4_addrs_num, &ip4_addrs,
                                  "%s/ip4_address:*", node_oid);
        if (rc != 0)
        {
            ERROR("Failed to find IPv4 addresses assigned "
                  "to node '%s': %r", node_oid, rc);
            free(node_oid);
            return rc;
        }
        if (ip4_addrs_num <= 0)
        {
            ERROR("No IPv4 addresses are assigned to node '%s'",
                  node_oid);
            free(node_oid);
            return TE_EENV;
        }

        /* Get IPv4 address */
        val_type = CVT_ADDRESS;
        rc = cfg_get_instance(ip4_addrs[0], &val_type, addr);
        free(node_oid);
        if (rc != 0)
        {
            ERROR("Failed to get node IPv4 address: %r", rc);
            return rc;
        }

        env_addr->host->ip4_unicast_used = TRUE;
    }

    return 0;
}

/**
 * Prepare required addresses in accordance with bound network
 * configuration.
 *
 * @param addrs         list of environment addresses
 * @param cfg_nets      networks configuration
 *
 * @return Status code.
 */
static int
prepare_addresses(tapi_env_addrs *addrs, cfg_nets_t *cfg_nets)
{
    int             rc;
    tapi_env_addr  *env_addr;
    tapi_env_host  *host;
    cfg_handle      handle;
    cfg_val_type    val_type;

    for (env_addr = addrs->cqh_first;
         env_addr != (void *)addrs;
         env_addr = env_addr->links.cqe_next)
    {
        env_addr->handle = CFG_HANDLE_INVALID;
        env_addr->addr   = SA(&env_addr->addr_st);

        host = env_addr->host;

        /* Handle of the assosiated network node */
        handle = cfg_nets->nets[host->i_net].
                     nodes[host->i_node].handle;

        if (env_addr->family == RPC_AF_ETHER)
        {
            env_addr->addrlen = sizeof(struct sockaddr);
            /* It's not right family, but it's used in Configurator */
            env_addr->addr->sa_family = AF_LOCAL;
            if (env_addr->type == TAPI_ENV_ADDR_ALIEN)
            {
                static uint8_t counter;

                /* TODO - get it from Configurator */
                env_addr->addr->sa_data[0] = 0x00;
                env_addr->addr->sa_data[1] = 0x10;
                env_addr->addr->sa_data[2] = 0x29;
                env_addr->addr->sa_data[3] = 0x38;
                env_addr->addr->sa_data[4] = 0x47;
                env_addr->addr->sa_data[5] = 0x56 + counter++;
            }
            else if (env_addr->type == TAPI_ENV_ADDR_UNICAST)
            {
                char *str;

                val_type = CVT_STRING;
                rc = cfg_get_instance(handle, &val_type, &str);
                if (rc != 0)
                {
                    ERROR("Failed to get instance value by handle "
                          "0x%x: %r", handle, rc);
                    return rc;
                }

                rc = tapi_cfg_base_if_get_mac(str,
                                              env_addr->addr->sa_data);
                if (rc != 0)
                {
                    ERROR("Failed to get link layer address of '%s': %r",
                          str, rc);
                    free(str);
                    return rc;
                }
                free(str);
            }
            else if (env_addr->type == TAPI_ENV_ADDR_MULTICAST)
            {
                env_addr->addr->sa_data[0] = random() | 0x01;
                env_addr->addr->sa_data[1] = random();
                env_addr->addr->sa_data[2] = random();
                env_addr->addr->sa_data[3] = random();
                env_addr->addr->sa_data[4] = random();
                env_addr->addr->sa_data[5] = random();
            }
            else if (env_addr->type == TAPI_ENV_ADDR_BROADCAST)
            {
                memset(env_addr->addr->sa_data, 0xff, ETHER_ADDR_LEN);
            }
            else
            {
                ERROR("Unsupported Ethernet address type");
                return TE_EINVAL;
            }
        }
        else if (env_addr->family == RPC_AF_INET)
        {
            /* Set address length */
            env_addr->addrlen = sizeof(struct sockaddr_in);

            env_addr->addr->sa_family = AF_INET;
            if (env_addr->type == TAPI_ENV_ADDR_UNICAST)
            {
                rc = prepare_ip4_unicast(env_addr, cfg_nets,
                                         &env_addr->addr);
                if (rc != 0)
                    break;
            }
            else if (env_addr->type == TAPI_ENV_ADDR_FAKE_UNICAST)
            {
                rc = tapi_env_allocate_addr(env_addr->net, AF_INET,
                                          &env_addr->addr, NULL);
                if (rc != 0)
                {
                    ERROR("Failed to allocate additional IPv4 "
                          "address: %r", rc);
                    break;
                }
            }
            else if (env_addr->type == TAPI_ENV_ADDR_LOOPBACK)
            {
                SIN(env_addr->addr)->sin_addr.s_addr = htonl(INADDR_LOOPBACK);
            }
            else if (env_addr->type == TAPI_ENV_ADDR_WILDCARD)
            {
                SIN(env_addr->addr)->sin_addr.s_addr = htonl(INADDR_ANY);
            }
            else if (env_addr->type == TAPI_ENV_ADDR_ALIEN)
            {
                static uint32_t alien_addr = 0x21212121;
    
                /* TODO - get it from Configurator */
                SIN(env_addr->addr)->sin_addr.s_addr = htonl(alien_addr);
                alien_addr += 0x01000000;
            }
            else if (env_addr->type == TAPI_ENV_ADDR_MULTICAST)
            {
                SIN(env_addr->addr)->sin_addr.s_addr =
                    htonl(rand_range(0xe0000000, 0xefffffff));
            }
            else if (env_addr->type == TAPI_ENV_ADDR_BROADCAST)
            {
                memcpy(env_addr->addr, &env_addr->net->ip4bcast,
                       sizeof(env_addr->net->ip4bcast));
            }
            else
            {
                ERROR("Unsupported IPv4 address type");
                return TE_EINVAL;
            }

            /* Set allocated port */
            rc = tapi_allocate_port_htons(
                     &(SIN(env_addr->addr)->sin_port));
            if (rc != 0)
            {
                ERROR("Failed to allocate a port for address: %r", rc);
                return rc;
            }
        }
        else if (env_addr->family == RPC_AF_INET6)
        {
            /* Set address length */
            env_addr->addrlen = sizeof(struct sockaddr_in6);

            env_addr->addr->sa_family = AF_INET6;
            if (env_addr->type == TAPI_ENV_ADDR_IP4MAPPED_UC)
            {
                struct sockaddr *ip4;

                rc = prepare_ip4_unicast(env_addr, cfg_nets, &ip4);
                if (rc != 0)
                    break;

                SIN6(env_addr->addr)->sin6_addr.s6_addr16[5] = 0xffff;
                SIN6(env_addr->addr)->sin6_addr.s6_addr32[3] =
                    SIN(ip4)->sin_addr.s_addr;

                free(ip4);
            }
            else if (env_addr->type == TAPI_ENV_ADDR_WILDCARD)
            {
                SIN6(env_addr->addr)->sin6_addr = in6addr_any;
            }
            else
            {
                ERROR("Unsupported IPv6 address type");
                return TE_EINVAL;
            }

            /* Set allocated port */
            rc = tapi_allocate_port_htons(
                     &(SIN6(env_addr->addr)->sin6_port));
            if (rc != 0)
            {
                ERROR("Failed to allocate a port for address: %r", rc);
                return rc;
            }
        }
        else
        {
            ERROR("Unsupported address family");
            return TE_EINVAL;
        }
    }
    return 0;
}


static int
add_address(tapi_env_addr *env_addr, cfg_nets_t *cfg_nets,
            const struct sockaddr *addr)
{
    int             rc;
    cfg_handle      handle;
    char           *str;
    cfg_val_type    val_type = CVT_STRING;


    handle = cfg_nets->nets[env_addr->host->i_net].
                       nodes[env_addr->host->i_node].handle;
    rc = cfg_get_instance(handle, &val_type, &str);
    if (rc != 0)
    {
        ERROR("Failed to get instance value by handle 0x%x: %r",
              handle, rc);
        return rc;
    }

    /* All is logged inside the function */
    rc = tapi_cfg_base_add_net_addr(str,
                                    addr,
                                    env_addr->net->ip4pfx,
                                    TRUE,
                                    &env_addr->handle);
    if (TE_RC_GET_ERROR(rc) == TE_EEXIST)
    {
        /* Address already assigned - continue */
        rc = 0;
    }

    free(str);
    
    return rc;
}


/**
 * Prepare required interfaces data in accordance with bound
 * network configuration.
 *
 * @param ifs           list of environment interfaces
 * @param cfg_nets      networks configuration
 *
 * @return Status code.
 */
static int
prepare_interfaces(tapi_env_ifs *ifs, cfg_nets_t *cfg_nets)
{
    int             rc;
    tapi_env_if  *p;
    cfg_val_type    val_type;
    char           *oid = NULL;

    for (p = ifs->lh_first; p != NULL; p = p->links.le_next)
    {
        if (strcmp(p->name, "lo") != 0)
        {
            /* Get name of the interface from network node value */
            rc = node_value_get_ith_inst_name(
                     cfg_nets->nets[p->host->i_net].
                               nodes[p->host->i_node].handle,
                     2, &p->info.if_name);
            if (rc != 0)
            {
                ERROR("Failed to get interface name");
                return rc;
            }

            val_type = CVT_STRING;
            rc = cfg_get_instance(cfg_nets->nets[p->host->i_net].
                                      nodes[p->host->i_node].handle,
                                  &val_type, &oid);
            if (rc != 0)
            {
                ERROR("Failed to get OID of the network node");
                return rc;
            }
        }
        else
        {
            const char *oid_fmt = "/agent:%s/interface:%s";
            char       *ta;

            /* 
             * It's assumed here that name of the loopback interface
             * on the test agent is equal to 'lo'.
             */
            p->info.if_name = strdup(p->name);
            if (p->info.if_name == NULL)
            {
                ERROR("strdup() failed");
                return errno;
            }

            /* Get name of the test agent */
            rc = node_value_get_ith_inst_name(
                     cfg_nets->nets[p->host->i_net].
                               nodes[p->host->i_node].handle,
                     1, &ta);
            if (rc != 0)
            {
                ERROR("Failed to get test agent name");
                return rc;
            }

            oid = malloc(strlen(oid_fmt) - 2 + strlen(ta) -
                         2 + strlen(p->info.if_name) + 1);
            if (oid == NULL)
            {
                ERROR("malloc() failed");
                return TE_ENOMEM;
            }
            sprintf(oid, oid_fmt, ta, p->info.if_name);
            
            free(ta);
        }

        val_type = CVT_INTEGER;
        rc = cfg_get_instance_fmt(&val_type, &(p->info.if_index),
                                  "%s/index:", oid);
        if (rc != 0)
        {
            ERROR("Failed to get interface index of the %s via "
                  "Configurator: %r", oid, rc);
            free(oid);
            return rc;
        }
        free(oid);
        oid = NULL;
    }
    return 0;
}


/**
 * Prepare required PCOs in accordance with bound network configuration.
 *
 * @param pcos          list of PCOs
 * @param cfg_nets      networks configuration
 *
 * @return Status code.
 */
static int
prepare_pcos(tapi_env_hosts *hosts)
{
    int                 rc = 0;
    tapi_env_host    *host;
    tapi_env_process *proc;
    tapi_env_pco     *pco;
    te_bool             main_thread;

    for (host = hosts->cqh_first;
         host != (void *)hosts && rc == 0;
         host = host->links.cqe_next)
    {
        for (proc = host->processes.lh_first;
             proc != NULL && rc == 0;
             proc = proc->links.le_next)
        {
            main_thread = TRUE;

            for (pco = proc->pcos.tqh_first;
                 pco != NULL;
                 pco = pco->links.tqe_next)
            {
                if (main_thread)
                {
                    if (rcf_rpc_server_get(host->ta, pco->name,
                                           NULL, FALSE, TRUE, FALSE, 
                                           &(pco->rpcs)) != 0)
                    {
                        rc = rcf_rpc_server_create(host->ta, pco->name,
                                                   &(pco->rpcs));
                        if (rc != 0)
                        {
                            ERROR("rcf_rpc_server_get() failed: %r", rc);
                            break;
                        }
                        pco->created = TRUE;
                    }
                    else
                        pco->created = FALSE;

                    main_thread = FALSE;
                    if (pco->type == TAPI_ENV_IUT && host->libname != NULL)
                    {
                        rc = rcf_rpc_setlibname(pco->rpcs, host->libname);
                        if (rc != 0)
                        {
                            ERROR("Failed to set RPC server '%s' dynamic "
                                  "library name '%s': %r", 
                                  pco->rpcs->name,
                                  host->libname ? : "(NULL)", rc);
                            break;
                        }
                    }
                }
                else
                {
                    rc = rcf_rpc_server_thread_create(
                             proc->pcos.tqh_first->rpcs,
                             pco->name, &(pco->rpcs));
                    if (rc != 0)
                    {
                        ERROR("rcf_rpc_server_thread_create() failed: %r", 
                              rc);
                        break;
                    }
                    pco->created = TRUE;
                }
            }
            /*
             * If more than one threads in a process, move main
             * thread in the tail for correct destruction.
             */
            if (!main_thread &&
                ((pco = proc->pcos.tqh_first)->links.tqe_next != NULL))
            {
                TAILQ_REMOVE(&proc->pcos, pco, links);
                TAILQ_INSERT_TAIL(&proc->pcos, pco, links);
            }
        }
    }
    return rc;
}


static int
bind_env_hosts_to_cfg_nets(tapi_env_hosts *hosts,
                           cfg_nets_t *cfg_nets)
{
    int             rc = 0;
    node_indexes    used_nodes;
    node_index     *p;

    /* Initialize empty list with used nodes */
    LIST_INIT(&used_nodes);

    /* Recursively bind all hosts */
    if (!bind_host(hosts->cqh_first, hosts, cfg_nets, &used_nodes))
    {
        ERROR("Failed to bind requested environment configuration to "
              "available network configuration");
        rc = TE_EENV;
    }

    /* Free list of used indexes */
    while ((p = used_nodes.lh_first) != NULL)
    {
        LIST_REMOVE(p, links);
        free(p);
    }
    
    return rc;
}


/**
 * Bind host to the node in network model.
 *
 * @param host          host to be bound
 * @param hosts         list with all hosts
 * @param cfg_nets      network model configuration
 * @param used_nodes    list with used (net, node) indexes
 *
 * @retval TRUE         success
 * @retval FALSE        failure
 */
static te_bool
bind_host(tapi_env_host *host, tapi_env_hosts *hosts,
          cfg_nets_t *cfg_nets, node_indexes *used_nodes)
{
    unsigned int        i, j;
    tapi_env_host    *p;

    if (host == (void *)hosts)
        return TRUE;

    VERB("Try to bind host '%s'", host->name);

    for (i = 0; i < cfg_nets->n_nets; ++i)
    {
        for (j = 0; j < cfg_nets->nets[i].n_nodes; ++j)
        {
            if (node_is_used(used_nodes, i, j))
            {
                VERB("Node (%u,%u) is already used", i, j);
                continue;
            }

            if (!check_node_type_vs_net(cfg_nets->nets + i, j,
                                        host->nets.lh_first->type))
            {
                VERB("Node (%u,%u) type=%u is not suitable for the host "
                     "in the net with type=%u", i, j,
                     cfg_nets->nets[i].nodes[j].type,
                     host->nets.lh_first->type);
                continue;
            }

            if (!check_node_type_vs_pcos(cfg_nets->nets[i].nodes[j].type,
                                         host))
            {
                VERB("Node (%u,%u) type=%u is not suitable for the host",
                     i, j, cfg_nets->nets[i].nodes[j].type);
                continue;
            }

            /* Check that there is no conflicts with already bound nodes */
            p = host;
            while ((p = p->links.cqe_prev) != (void *)hosts)
            {
                if (host->nets.lh_first == p->nets.lh_first)
                {
                    if (i != p->i_net)
                    {
                        VERB("Hosts '%s'(0x%x) and '%s'(0x%x) must be "
                             "in one net", host->name, host, p->name, p);
                        break;
                    }
                }
                if ((strlen(host->name) > 0) &&
                    (strcmp(host->name, p->name) == 0))
                {
                    if (cmp_agent_names(
                            cfg_nets->nets[i].nodes[j].handle,
                             cfg_nets->nets[p->i_net].
                                       nodes[p->i_node].handle) != 0)
                    {
                        VERB("Hosts with the same name '%s' can't be "
                             "bound to nodes on different agents",
                             host->name);
                        break;
                    }
                }
            }
            if (p == (void *)hosts)
            {
                /* No conflicts discovered */
                host->nets.lh_first->i_net = i;
                host->i_net = i;
                host->i_node = j;
                if (node_mark_used(used_nodes, i, j) != 0)
                    return FALSE;
                VERB("Mark (%u,%u) as used by '%s'", i, j, host->name);
                /* Try to bind the next host */
                if (bind_host(host->links.cqe_next, hosts, cfg_nets,
                              used_nodes))
                {
                    return TRUE;
                }
                VERB("Failed to bind host '%s', unmark (%u,%u)",
                     host->name, i, j);
                /* Failed to bind the host */
                node_unmark_used(used_nodes, i, j);
                host->nets.lh_first->i_net =
                    host->i_net = host->i_node = UINT_MAX;
            }
        }
    }

    VERB("Failed to bind host '%s'", host->name);

    return FALSE;
}

static int
node_value_get_ith_inst_name(cfg_handle node, unsigned int i, char **p_str)
{
    int             rc;
    char           *str;
    cfg_val_type    val_type = CVT_STRING;
    
    rc = cfg_get_instance(node, &val_type, &str);
    if (rc != 0)
    {
        ERROR("Failed to get instance value by handle 0x%x: %r",
              node , rc);
        return rc;
    }

    rc = cfg_get_ith_inst_name(str, i, p_str);
    if (rc != 0)
    {
        free(str);
        return rc;
    }
    free(str);

    return 0;
}

/**
 * Compare names of the test agents in OID stored in network
 * configuration nodes.
 *
 * @param node1         handle of the first node
 * @param node1         handle of the second node
 *
 * @retval 0            equal
 * @retval != 0         not equal
 */
static int
cmp_agent_names(cfg_handle node1, cfg_handle node2)
{
    int   rc;
    int   retval;
    char *agt1;
    char *agt2;

    rc = node_value_get_ith_inst_name(node1, 1, &agt1);
    if (rc != 0)
        return -1;
    rc = node_value_get_ith_inst_name(node2, 1, &agt2);
    if (rc != 0)
    {
        free(agt1);
        return -1;
    }

    retval = strcmp(agt1, agt2);

    free(agt1);
    free(agt2);
    
    return retval;
}


/**
 * Mark node with indexes (net, node) as used.
 *
 * @param used_nodes    list of used indes pairs
 * @param net           net-index
 * @param node          node-index
 *
 * @retval 0            success
 * @retval ENOMEM       memory allocation failure
 */
static int
node_mark_used(node_indexes *used_nodes, unsigned int net, unsigned int node)
{
    node_index *p = calloc(1, sizeof(*p));
    
    if (p == NULL)
        return ENOMEM;
        
    p->net = net;
    p->node = node;
    
    LIST_INSERT_HEAD(used_nodes, p, links);
    
    return 0;
}

/**
 * Unmark node with indexes (net, node) as used.
 *
 * @param used_nodes    list of used indes pairs
 * @param net           net-index
 * @param node          node-index
 */
static void
node_unmark_used(node_indexes *used_nodes, unsigned int net, unsigned int node)
{
    node_index *p;

    for (p = used_nodes->lh_first;
         (p != NULL) && ((net != p->net) || (node != p->node));
         p = p->links.le_next);

    if (p != NULL)    
        LIST_REMOVE(p, links);
}

/**
 * Is node with indexes (net, node) marked as used.
 *
 * @param used_nodes    list of used indes pairs
 * @param net           net-index
 * @param node          node-index
 *
 * @retval TRUE         marked as used
 * @retval FALSE        not marked as used
 */
static te_bool
node_is_used(node_indexes *used_nodes, unsigned int net, unsigned int node)
{
    node_index *p;

    for (p = used_nodes->lh_first;
         (p != NULL) && ((net != p->net) || (node != p->node));
         p = p->links.le_next);

    return (p != NULL);
}

/**
 * Check that network node type matches type of requested PCOs.
 *
 * @param host          Host to be bound
 * @param node_type     Type of the network node
 *
 * @return Whether types match?
 */
static te_bool
check_node_type_vs_pcos(enum net_node_type node_type, tapi_env_host *host)
{
#if 0
    tapi_env_process *proc;
    tapi_env_pco     *pco;
    tapi_env_type     matched1;
    tapi_env_type     matched2;

    switch (node_type)
    {
        case NET_NODE_TYPE_AGENT:
            matched1 = TAPI_ENV_TESTER;
            matched2 = TAPI_ENV_TESTER;
            break;

        case NET_NODE_TYPE_NUT:
            matched1 = TAPI_ENV_IUT;
            matched2 = TAPI_ENV_TESTER;
            break;

        default:
            ERROR("Unknown network node type %u", node_type);
            return FALSE;
    }

    for (proc = host->processes.lh_first;
         proc != NULL;
         proc = proc->links.le_next)
    {
        for (pco = proc->pcos.tqh_first;
             pco != NULL;
             pco = pco->links.tqe_next)
        {
            if ((pco->type != matched1) && (pco->type != matched2))
            {
                VERB("Type of PCO '%s' is %u, "
                     "it does not match to %u or %u",
                     pco->name, pco->type, matched1, matched2);
                return FALSE;
            }
        }
    }
#else
    UNUSED(node_type);
    UNUSED(host);
#endif
    return TRUE;
}

/**
 * Check that network node type matches type of requested net.
 *
 * @param net           Configuration model net
 * @param node_i        Index of the node in @a net
 * @param net_type      Requested type of the net
 *
 * @return Whether types match?
 */
static te_bool
check_node_type_vs_net(cfg_net_t *net, unsigned int node_i,
                       tapi_env_type net_type)
{
    enum net_node_type node_type = net->nodes[node_i].type;

    switch (net_type)
    {
        case TAPI_ENV_UNSPEC:
            return TRUE;

        case TAPI_ENV_IUT:
            if (node_type == NET_NODE_TYPE_NUT)
            {
                return TRUE;
            }
            else
            {
                unsigned int i;

                for (i = 0; i < net->n_nodes; ++i)
                {
                    if (net->nodes[i].type == NET_NODE_TYPE_NUT)
                        return TRUE;
                }
                return FALSE;
            }

        case TAPI_ENV_TESTER:
            return (node_type == NET_NODE_TYPE_AGENT);

        default:
            assert(0);
            return FALSE;
    }
}


#if 0
static void
substitute_host(tapi_env *env, tapi_env_host *old, tapi_env_host *new)
{
    tapi_env_addr    *addr;
    tapi_env_if      *iface;
    tapi_env_process *proc;
    tapi_env_net     *net;

    for (addr = env->addrs.cqh_first;
         addr != (void *)&env->addrs;
         addr = addr->links.cqe_next)
    {
        if (addr->host == old)
            addr->host = new;
    }
    for (iface = env->ifs.lh_first;
         iface != NULL;
         iface = iface->links.le_next)
    {
        if (iface->host == old)
            iface->host = new;
    }
#if 0
    while ((proc = old->processes.lh_first) != NULL)
    {
        LIST_REMOVE(proc, links);
        LIST_INSERT_HEAD(&new->processes, proc, links);
    }
#endif

    new->n_nets += old->n_nets;
    while ((net = old->nets.lh_first) != NULL)
    {
        LIST_REMOVE(net, inhost);
        LIST_INSERT_HEAD(&new->nets, net, inhost);
    }
}

static void
canonicalize_hosts(tapi_env *env)
{
    tapi_env_host *p, *q, *r;

    for (p = env->hosts.cqh_first; p != NULL; p = p->links.cqe_next)
    {
        if (strlen(p->name) > 0)
        {
            for (q = p->links.cqe_next; q != NULL; q = r)
            {
                r = q->links.cqe_next;
                if (strcmp(p->name, q->name) == 0)
                {
                    substitute_host(env, q, p);
                    CIRCLEQ_REMOVE(&env->hosts, q, links);
                    free(q);
                }
            }
        }
    }
}
#endif


/* See description in tapi_env.h */
te_errno
tapi_env_get_net_host_addr(const tapi_env_net   *net,
                         const tapi_env_host  *host,
                         tapi_cfg_net_assigned  *assigned,
                         struct sockaddr       **addr,
                         socklen_t              *addrlen)
{
    int             rc;
    cfg_val_type    val_type;
    char           *node_oid;


    if (net == NULL || host == NULL || assigned == NULL || addr == NULL)
    {
        ERROR("%s(): Invalid parameter", __FUNCTION__);
        return TE_EINVAL;
    }

    if (host->i_net != net->i_net)
    {
        ERROR("Such configuration is not supported yet!!!");
        return TE_EOPNOTSUPP;
    }

    /* Get string OID of the associated net in networks configuration */
    rc = cfg_get_oid_str(net->cfg_net->nodes[host->i_node].handle,
                         &node_oid);
    if (rc != 0)
    {
        ERROR("Failed to string OID by handle 0x%x: %r",
              net->cfg_net->nodes[host->i_node], rc);
        return rc;
    }

    /* Get IPv4 subnet address */
    val_type = CVT_ADDRESS;
    rc = cfg_get_instance_fmt(&val_type, addr,
                              "%s/ip4_address:%u", node_oid,
                              assigned->entries[host->i_node]);
    if (rc != 0)
    {
        ERROR("Failed to get IPv4 address assigned to the node '%s' "
              "with handle 0x%x: %r", node_oid,
              assigned->entries[host->i_node], rc);
        free(node_oid);
        return rc;
    }
    free(node_oid);

    if (addrlen != NULL)
        *addrlen = sockaddr_get_size(*addr);

    return 0;
}
