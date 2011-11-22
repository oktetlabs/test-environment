/** @file
 * @brief Environment Library
 *
 * Environment allocation and destruction.
 *
 * Copyright (C) 2004-2006 Test Environment authors (see file AUTHORS
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
#include "tapi_cfg_ip6.h"
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
    SLIST_ENTRY(node_index) links;  /**< Links */
    unsigned int            net;    /**< Net index */
    unsigned int            node;   /**< Node in the net index */
} node_index;

/** List of the network node indexes */
typedef SLIST_HEAD(node_indexes, node_index) node_indexes;

static te_errno prepare_nets(tapi_env_nets *nets,
                             cfg_nets_t    *cfg_nets);
static te_errno prepare_hosts(tapi_env *env);
static te_errno prepare_addresses(tapi_env_addrs *addrs,
                                  cfg_nets_t     *cfg_nets);
static te_errno prepare_interfaces(tapi_env_ifs *ifs,
                                   cfg_nets_t   *cfg_nets);
static te_errno prepare_pcos(tapi_env_hosts *hosts);

static te_errno add_address(tapi_env_addr         *env_addr,
                            cfg_nets_t            *cfg_nets,
                            const struct sockaddr *addr);

static te_errno bind_env_to_cfg_nets(tapi_env_ifs *ifs,
                                     cfg_nets_t   *cfg_nets);
static te_bool bind_host_if(tapi_env_if  *iface,
                            tapi_env_ifs *ifs,
                            cfg_nets_t   *cfg_nets,
                            node_indexes *used_nodes);

static int cmp_agent_names(cfg_handle node1, cfg_handle node2);

static te_errno node_value_get_ith_inst_name(cfg_handle     node,
                                             unsigned int   i,
                                             char         **p_str);

static te_errno node_mark_used(node_indexes *used_nodes,
                               unsigned int net, unsigned int node);
static void node_unmark_used(node_indexes *used_nodes,
                             unsigned int net, unsigned int node);
static te_bool node_is_used(node_indexes *used_nodes,
                            unsigned int net, unsigned int node);

static te_bool check_net_type_cfg_vs_env(cfg_net_t *net,
                                         tapi_env_type net_type);

static te_bool check_node_type_vs_pcos(cfg_nets_t         *cfg_nets,
                                       cfg_net_node_t     *node,
                                       tapi_env_processes *processes);

/* Resolve object name. If it's an alias name - return the actual object
 * name. If it's an object name - return it. */
static inline const char* env_resolve(tapi_env* env,
                                      const char* name)
{
  const char*     out_name = name;
  tapi_env_alias *a;

  SLIST_FOREACH(a, &env->aliases, links)
  {
    if (strcmp(a->alias, name) == 0)
    {
      VERB("'%s' is alias of '%s'", name, a->name);
      out_name = a->name;
      break;
    }
  }

  return out_name;
}


/* See description in tapi_env.h */
te_errno
tapi_env_allocate_addr(tapi_env_net *net, int af,
                       struct sockaddr **addr, socklen_t *addrlen)
{
    te_errno        rc;
    cfg_handle_tqe *handle;


    if (net == NULL)
        return TE_EINVAL;
    if (addr == NULL)
    {
        ERROR("Invalid location for address pointer");
        return TE_EINVAL;
    }
    if (af != AF_INET && af != AF_INET6)
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

    rc = tapi_cfg_alloc_net_addr(af == AF_INET ? net->ip4net : net->ip6net,
                                 &handle->handle, addr);
    if (rc != 0)
    {
        ERROR("Failed to allocate address in subnet 0x%x: %r",
              af == AF_INET ? net->ip4net : net->ip6net, rc);
        free(handle);
        return rc;
    }
    if (addrlen != NULL)
        *addrlen = te_sockaddr_get_size_by_af(af); 

    STAILQ_INSERT_TAIL(&net->net_addrs, handle, links);

    return 0;
}



/* See description in tapi_env.h */
te_errno
tapi_env_init(tapi_env *env)
{
    if (env == NULL)
    {
        ERROR("Invalid argument: %s(%p)", __FUNCTION__, env);
        return TE_EINVAL;
    }

    /* Initialize lists */
    env->n_nets = 0;
    SLIST_INIT(&env->nets);
    SLIST_INIT(&env->hosts);
    CIRCLEQ_INIT(&env->ifs);
    CIRCLEQ_INIT(&env->addrs);
    SLIST_INIT(&env->aliases);
    memset(&env->cfg_nets, 0, sizeof(env->cfg_nets));

    return 0;
}

/* See description in tapi_env.h */
te_errno
tapi_env_get(const char *cfg, tapi_env *env)
{
    te_errno    rc;

    if (cfg == NULL || env == NULL)
    {
        ERROR("Invalid argument: %s(0x%x, 0x%x)",
              __FUNCTION__, cfg, env);
        return TE_EINVAL;
    }

    rc = tapi_env_init(env);
    if (rc != 0)
    {
        ERROR("%s(): tapi_env_init() failed: %r", __FUNCTION__, rc);
        return rc;
    }

    /* Parse environment configuration string */
    rc = env_cfg_parse(env, cfg);
    if (rc != 0)
    {
        ERROR("Invalid environment configuration string: %s", cfg);
        return rc;
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
    
    rc = bind_env_to_cfg_nets(&env->ifs, &env->cfg_nets);
    if (rc != 0)
    {
        /* ERROR is logged in bind_env_to_cfg_nets function */
        return rc;
    }

    rc = prepare_nets(&env->nets, &env->cfg_nets);
    if (rc != 0)
    {
        ERROR("Failed to prepare networks");
        return rc;
    }

    rc = prepare_hosts(env);
    if (rc != 0)
    {
        ERROR("Failed to prepare hosts/interfaces");
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
    te_errno          rc;
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
    cfg_handle        ip_net_hndl;
    char             *ip_net_oid;

    if (env == NULL)
        return 0;

    /* Destroy list of hosts */
    while ((host = SLIST_FIRST(&env->hosts)) != NULL)
    {
        /* Destroy list of processes */
        while ((proc = SLIST_FIRST(&host->processes)) != NULL)
        {
            /* Destroy PCOs */
            while ((pco = STAILQ_FIRST(&proc->pcos)) != NULL)
            {
                if (pco->rpcs != NULL)
                {
                    VERB("Destroy RPC Server (%s,%s)",
                         pco->rpcs->ta, pco->rpcs->name);
                         
                    if (pco->created && 
                        !rcf_rpc_server_has_children(pco->rpcs))
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
                STAILQ_REMOVE(&proc->pcos, pco, tapi_env_pco, links);
                free(pco->name);
                free(pco);
            }
            SLIST_REMOVE(&host->processes, proc, tapi_env_process, links);
            free(proc);
        }
        SLIST_REMOVE(&env->hosts, host, tapi_env_host, links);
        free(host->ta);
        free(host->libname);
        free(host->name);
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
        if (addr->addr != SA(&addr->addr_st))
            free(addr->addr);
        free(addr->name);
        free(addr);
    }
    /* Destroy list of nets */
    while ((net = SLIST_FIRST(&env->nets)) != NULL)
    {
        SLIST_REMOVE(&env->nets, net, tapi_env_net, links);
        n_deleted = 0;
        ip_net_oid = NULL;
        while ((addr_hndl = STAILQ_FIRST(&net->net_addrs)) != NULL)
        {
            STAILQ_REMOVE(&net->net_addrs, addr_hndl, cfg_handle_tqe,
                          links);
            if (n_deleted == 0)
            {
                if (((rc = cfg_get_father(addr_hndl->handle,
                                          &ip_net_hndl)) != 0) ||
                    ((rc = cfg_get_father(ip_net_hndl,
                                          &ip_net_hndl)) != 0))

                {
                    ERROR("cfg_get_father() failed: %r", rc);
                    TE_RC_UPDATE(result, rc);
                }
                else if ((rc = cfg_get_oid_str(ip_net_hndl,
                                               &ip_net_oid)) != 0)
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
        if (ip_net_oid != NULL)
        {
            type = CVT_INTEGER;
            rc = cfg_get_instance_fmt(&type, &n_entries,
                                      "%s/n_entries:", ip_net_oid);
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
                                          "%s/n_entries:", ip_net_oid);
                if (rc != 0)
                {
                    ERROR("Failed to set number of entries in the pool: %r",
                          rc);
                    TE_RC_UPDATE(result, rc);
                }
            }
            free(ip_net_oid);
        }
        free(net->ip4addr);
        free(net->ip6addr);
        free(net->name);
        free(net);
    }
    /* Destroy list of interfaces */
    while ((iface = env->ifs.cqh_first) != (void *)&env->ifs)
    {
        CIRCLEQ_REMOVE(&env->ifs, iface, links);
        free(iface->if_info.if_name);
        free(iface->br_info.if_name);
        free(iface->ph_info.if_name);
        free(iface->name);
        free(iface);
    }
    /* Destroy list of aliases */
    while ((alias = SLIST_FIRST(&env->aliases)) != NULL)
    {
        SLIST_REMOVE(&env->aliases, alias, tapi_env_alias, links);
        free(alias->alias);
        free(alias->name);
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

    if (env == NULL || name == NULL)
    {
        ERROR("%s(): Invalid arguments", __FUNCTION__);
        return NULL;
    }

    name = env_resolve(env, name);

    SLIST_FOREACH(p, &env->nets, links)
    {
        if (p->name != NULL && strcmp(p->name, name) == 0)
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

    if (env == NULL || name == NULL)
    {
        ERROR("%s(): Invalid arguments", __FUNCTION__);
        return NULL;
    }

    name = env_resolve(env, name);

    SLIST_FOREACH(p, &env->hosts, links)
    {
        if (p->name != NULL && strcmp(p->name, name) == 0)
            return p;
    }

    WARN("Host '%s' does not exist in environment", name);

    return NULL;
}

/* See description in tapi_env.h */
rcf_rpc_server *
tapi_env_get_pco(tapi_env *env, const char *name)
{
    tapi_env_host    *host;
    tapi_env_process *proc;
    tapi_env_pco     *pco;

    if (env == NULL || name == NULL)
    {
        ERROR("%s(): Invalid arguments", __FUNCTION__);
        return NULL;
    }


    name = env_resolve(env, name);

    SLIST_FOREACH(host, &env->hosts, links)
    {
        SLIST_FOREACH(proc, &host->processes, links)
        {
            STAILQ_FOREACH(pco, &proc->pcos, links)
            {
                if (pco->name != NULL && strcmp(pco->name, name) == 0)
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
    const char       *aname;

    if (env == NULL || name == NULL)
    {
        ERROR("%s(): Invalid arguments", __FUNCTION__);
        return NULL;
    }

    /* do we have an address with such name? if yes - just return it */
    for (p = env->addrs.cqh_first;
         p != (void *)&env->addrs;
         p = p->links.cqe_next)
    {
        if (p->name != NULL && strcmp(p->name, name) == 0)
        {
            if (addrlen != NULL)
                *addrlen = p->addrlen;
            return p->addr;
        }
    }

    /* so this is probably an alias */
    aname = env_resolve(env, name);

    for (p = env->addrs.cqh_first;
         p != (void *)&env->addrs || ((p = NULL), 0);
         p = p->links.cqe_next)
    {
        if (p->name != NULL && strcmp(p->name, aname) == 0)
          break;
    }

    if (p != NULL) {
      tapi_env_addr* addr = calloc(1, sizeof(*addr));

      assert(addr != NULL);
      /* fixme: memory leak, but it's a test application - it will die in
       * any case */
      addr->name = strdup(name);
      addr->iface = p->iface;
      addr->family = p->family;
      addr->type = p->type;
      addr->handle = p->handle;
      addr->addrlen = p->addrlen;
      memcpy(&(addr->addr_st), &(p->addr_st), sizeof(p->addr_st));
      addr->addr = calloc(1, addr->addrlen);
      memcpy(addr->addr, p->addr, addr->addrlen);
      CIRCLEQ_INSERT_TAIL(&env->addrs, addr, links);

      if(addrlen != NULL)
          *addrlen = addr->addrlen;

      /* so we should not get here if we've already added the address */
      return addr->addr;
    }

    WARN("Address '%s' does not exist in environment", name);

    return NULL;
}


/* See description in tapi_env.h */
const struct if_nameindex *
tapi_env_get_if(tapi_env *env, const char *name)
{
    tapi_env_if      *p;

    if (env == NULL || name == NULL || *name == '\0')
    {
        ERROR("%s(): Invalid arguments", __FUNCTION__);
        return NULL;
    }

    name = env_resolve(env, name);

    for (p = env->ifs.cqh_first;
         p != (void *)&env->ifs;
         p = p->links.cqe_next)
    {
        if (p->name != NULL && strcmp(p->name, name) == 0)
            return &p->if_info;
    }

    WARN("Interface '%s' does not exist in environment", name);

    return NULL;
}


/* See description in tapi_env.h */
const struct if_nameindex *
tapi_env_get_br(tapi_env *env, const char *name)
{
    tapi_env_if      *p;

    if (env == NULL || name == NULL || *name == '\0')
    {
        ERROR("%s(): Invalid arguments", __FUNCTION__);
        return NULL;
    }

    name = env_resolve(env, name);

    for (p = env->ifs.cqh_first;
         p != (void *)&env->ifs;
         p = p->links.cqe_next)
    {
        if (p->name != NULL && strcmp(p->name, name) == 0)
            return &p->br_info;
    }

    WARN("XEN bridge '%s' does not exist in environment", name);

    return NULL;
}


/* See description in tapi_env.h */
const struct if_nameindex *
tapi_env_get_ph(tapi_env *env, const char *name)
{
    tapi_env_if      *p;

    if (env == NULL || name == NULL || *name == '\0')
    {
        ERROR("%s(): Invalid arguments", __FUNCTION__);
        return NULL;
    }

    name = env_resolve(env, name);

    for (p = env->ifs.cqh_first;
         p != (void *)&env->ifs;
         p = p->links.cqe_next)
    {
        if (p->name != NULL && strcmp(p->name, name) == 0)
            return &p->ph_info;
    }

    WARN("XEN physical interface '%s' does not exist in environment",
         name);

    return NULL;
}


/**
 * Prepare environment networks.
 *
 * @param ifs           list of environment networks
 *
 * @return Status code.
 */
static te_errno
prepare_nets(tapi_env_nets *nets, cfg_nets_t *cfg_nets)
{
    te_errno        rc = 0;
    tapi_env_net   *env_net;
    cfg_val_type    val_type;
    unsigned int    i;
    char           *net_oid;
    unsigned int    n_ip_nets;
    cfg_handle     *ip_nets;
    char           *ip_net_oid;


    for (env_net = SLIST_FIRST(nets), i = 0;
         env_net != NULL;
         env_net = SLIST_NEXT(env_net, links), ++i)
    {
        env_net->cfg_net = cfg_nets->nets + env_net->i_net;

        /* Get string OID of the associated net in networks configuration */
        rc = cfg_get_oid_str(env_net->cfg_net->handle, &net_oid);
        if (rc != 0)
        {
            ERROR("Failed to string OID by handle: %r", rc);
            break;
        }


        /*
         * IPv4 prepare
         */

        /* Get IPv4 subnet for the network */
        rc = cfg_find_pattern_fmt(&n_ip_nets, &ip_nets,
                                  "%s/ip4_subnet:*", net_oid);
        if (rc != 0)
        {
            ERROR("Failed to find IPv4 subnets assigned to net '%s': %r",
                  net_oid, rc);
            free(net_oid);
            break;
        }
        if (n_ip_nets <= 0)
        {
            ERROR("No IPv4 networks are assigned to net '%s'", net_oid);
            free(net_oid);
            rc = TE_EENV;
            break;
        }
        
        /* Get IPv4 subnet address */
        val_type = CVT_ADDRESS;
        rc = cfg_get_instance(ip_nets[0], &val_type, &env_net->ip4addr);
        if (rc != 0)
        {
            ERROR("Failed to get IPv4 subnet for net '%s': %r",
                  net_oid, rc);
            free(ip_nets);
            free(net_oid);
            break;
        }

        /* Get IPv4 subnet handle */
        rc = cfg_get_inst_name_type(ip_nets[0], CVT_INTEGER,
                                    CFG_IVP(&env_net->ip4net));
        if (rc != 0)
        {
            ERROR("Failed to get IPv4 subnet handle: %r", rc);
            free(ip_nets);
            free(net_oid);
            break;
        }
        /* Get IPv4 subnet OID */
        rc = cfg_get_oid_str(env_net->ip4net, &ip_net_oid);
        if (rc != 0)
        {
            ERROR("cfg_get_oid_str() failed: %r", rc);
            free(ip_nets);
            free(net_oid);
            break;
        }
        free(ip_nets); ip_nets = NULL;

        /* Get IPv4 subnet for the network */
        val_type = CVT_INTEGER;
        rc = cfg_get_instance_fmt(&val_type, &env_net->ip4pfx,
                                  "%s/prefix:", ip_net_oid);
        if (rc != 0)
        {
            ERROR("Failed to get IPv4 prefix length for configuration "
                  "network %s: %r", ip_net_oid, rc);
            free(ip_net_oid);
            free(net_oid);
            break;
        }
        free(ip_net_oid);

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


        /*
         * IPv6 prepare
         */

        /* Get IPv6 subnet for the network */
        rc = cfg_find_pattern_fmt(&n_ip_nets, &ip_nets,
                                  "%s/ip6_subnet:*", net_oid);
        if (rc != 0)
        {
            ERROR("Failed to find IPv6 subnets assigned to net '%s': %r",
                  net_oid, rc);
            free(net_oid);
            break;
        }
        if (n_ip_nets <= 0)
        {
            INFO("No IPv6 networks are assigned to net '%s'", net_oid);
            free(ip_nets);
            free(net_oid);
            continue;
        }
        
        /* Get IPv6 subnet address */
        val_type = CVT_ADDRESS;
        rc = cfg_get_instance(ip_nets[0], &val_type, &env_net->ip6addr);
        if (rc != 0)
        {
            ERROR("Failed to get IPv6 subnet for net '%s': %r",
                  net_oid, rc);
            free(ip_nets);
            free(net_oid);
            break;
        }

        /* Get IPv6 subnet handle */
        rc = cfg_get_inst_name_type(ip_nets[0], CVT_INTEGER,
                                    CFG_IVP(&env_net->ip6net));
        if (rc != 0)
        {
            ERROR("Failed to get IPv6 subnet handle: %r", rc);
            free(ip_nets);
            free(net_oid);
            break;
        }
        /* Get IPv6 subnet OID */
        rc = cfg_get_oid_str(env_net->ip6net, &ip_net_oid);
        if (rc != 0)
        {
            ERROR("cfg_get_oid_str() failed: %r", rc);
            free(ip_nets);
            free(net_oid);
            break;
        }
        free(ip_nets);

        /* Get IPv6 subnet for the network */
        val_type = CVT_INTEGER;
        rc = cfg_get_instance_fmt(&val_type, &env_net->ip6pfx,
                                  "%s/prefix:", ip_net_oid);
        if (rc != 0)
        {
            ERROR("Failed to get IPv6 prefix length for configuration "
                  "network %s: %r", ip_net_oid, rc);
            free(ip_net_oid);
            free(net_oid);
            break;
        }
        free(ip_net_oid);
    }

    return rc;
}

/**
 * Prepare required hosts in accordance with bound network
 * configuration.
 *
 * @param env           current state of environment
 *
 * @return Status code.
 */
static te_errno
prepare_hosts(tapi_env *env)
{
    tapi_env_host  *host;
    tapi_env_if    *iface;
    te_errno        rc = 0;
    cfg_handle      handle;
    cfg_val_type    type;

    SLIST_FOREACH(host, &env->hosts, links)
    {
        /* Find any interface instance which refers to the host */
        for (iface = env->ifs.cqh_first;
             iface != (void *)&env->ifs && iface->host != host;
             iface = iface->links.cqe_next);

        if (iface == (void *)&env->ifs)
        {
            ERROR("%s(): Failed to find any interface which refer to "
                  "the host 0x%x", __FUNCTION__, host);
            return TE_RC(TE_TAPI, TE_EFAULT);
        }

        /* Get name of the Test Agent */
        rc = node_value_get_ith_inst_name(
                 env->cfg_nets.nets[iface->net->i_net].
                     nodes[iface->i_node].handle,
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
prepare_unicast(unsigned int af, tapi_env_addr *env_addr,
                cfg_nets_t *cfg_nets, struct sockaddr **addr)
{
    te_errno rc;

    assert(addr != NULL);
    assert(af == AF_INET || af == AF_INET6);

    if (af == AF_INET ? env_addr->iface->ip4_unicast_used :
                        env_addr->iface->ip6_unicast_used)
    {
        rc = tapi_env_allocate_addr(env_addr->iface->net,
                                    af, addr, NULL);
        if (rc != 0)
        {
            ERROR("Failed to allocate additional address: %r", rc);
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
        unsigned int    ip_addrs_num;
        cfg_handle     *ip_addrs;
        cfg_val_type    val_type;

        /* Handle of the assosiated network node */
        handle = cfg_nets->nets[env_addr->iface->net->i_net].
                     nodes[env_addr->iface->i_node].handle;

        /* Get address assigned to the node */
        rc = cfg_get_oid_str(handle, &node_oid);
        if (rc != 0)
        {
            ERROR("Failed to string OID by handle: %r", rc);
            return rc;
        }

        /* Get IP addresses of the node */
        rc = cfg_find_pattern_fmt(&ip_addrs_num, &ip_addrs,
                                  "%s/ip%u_address:*", node_oid,
                                  af == AF_INET ? 4 : 6);
        if (rc != 0)
        {
            ERROR("Failed to find IP addresses assigned "
                  "to node '%s': %r", node_oid, rc);
            free(node_oid);
            return rc;
        }
        if (ip_addrs_num == 0)
        {
            ERROR("No IP%u addresses are assigned to node '%s'",
                  af == AF_INET ? 4 : 6, node_oid);
            free(ip_addrs);
            free(node_oid);
            return TE_EENV;
        }
        free(node_oid);

        /* Get IPv4 address */
        val_type = CVT_ADDRESS;
        rc = cfg_get_instance(ip_addrs[0], &val_type, addr);
        free(ip_addrs);
        if (rc != 0)
        {
            ERROR("Failed to get node IP address: %r", rc);
            return rc;
        }

        if (af == AF_INET)
            env_addr->iface->ip4_unicast_used = TRUE;
        else
            env_addr->iface->ip6_unicast_used = TRUE;
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
static te_errno
prepare_addresses(tapi_env_addrs *addrs, cfg_nets_t *cfg_nets)
{
    te_errno        rc;
    tapi_env_addr  *env_addr;
    tapi_env_if    *iface;
    cfg_handle      handle;
    cfg_val_type    val_type;

    for (env_addr = addrs->cqh_first;
         env_addr != (void *)addrs;
         env_addr = env_addr->links.cqe_next)
    {
        env_addr->handle = CFG_HANDLE_INVALID;
        env_addr->addr   = SA(&env_addr->addr_st);

        iface = env_addr->iface;

        /* Handle of the assosiated network node */
        handle = cfg_nets->nets[iface->net->i_net].
                     nodes[iface->i_node].handle;

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
                         (uint8_t *)env_addr->addr->sa_data);
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
                rc = prepare_unicast(AF_INET, env_addr, cfg_nets,
                                     &env_addr->addr);
                if (rc != 0)
                    break;
            }
            else if (env_addr->type == TAPI_ENV_ADDR_FAKE_UNICAST)
            {
                rc = tapi_env_allocate_addr(env_addr->iface->net, AF_INET,
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
                static uint32_t alien_addr = 0;
    
                if (alien_addr == 0)
                {
                    struct sockaddr *tmp;
                    cfg_val_type     type = CVT_ADDRESS;

                    rc = cfg_get_instance_fmt(&type, &tmp,
                                              "/local:/ip4_alien:");
                    if (rc != 0)
                        break;
                    alien_addr = ntohl(SIN(tmp)->sin_addr.s_addr);
                    free(tmp);
                }
                SIN(env_addr->addr)->sin_addr.s_addr = htonl(alien_addr);
                alien_addr += 0x01000000;
            }
            else if (env_addr->type == TAPI_ENV_ADDR_MULTICAST)
            {
                SIN(env_addr->addr)->sin_addr.s_addr =
                    htonl(rand_range(0xe0000100, 0xefffffff));
            }
            else if (env_addr->type == TAPI_ENV_ADDR_MCAST_ALL_HOSTS)
            {
                SIN(env_addr->addr)->sin_addr.s_addr =
                    htonl(INADDR_ALLHOSTS_GROUP);
            }
            else if (env_addr->type == TAPI_ENV_ADDR_BROADCAST)
            {
                memcpy(env_addr->addr, &env_addr->iface->net->ip4bcast,
                       sizeof(env_addr->iface->net->ip4bcast));
            }
            else
            {
                ERROR("Unsupported IPv4 address type");
                return TE_EINVAL;
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

                rc = prepare_unicast(AF_INET, env_addr, cfg_nets, &ip4);
                if (rc != 0)
                    break;

                SIN6(env_addr->addr)->sin6_addr.s6_addr16[5] = 0xffff;
                SIN6(env_addr->addr)->sin6_addr.s6_addr32[3] =
                    SIN(ip4)->sin_addr.s_addr;

                free(ip4);
            }
            else if (env_addr->type == TAPI_ENV_ADDR_UNICAST)
            {
                rc = prepare_unicast(AF_INET6, env_addr, cfg_nets,
                                     &env_addr->addr);
                if (rc != 0)
                    break;
            }
            else if (env_addr->type == TAPI_ENV_ADDR_LINKLOCAL)
            {
                char       *oid_string;
                cfg_oid    *oid_struct;

                val_type = CVT_STRING;
                rc = cfg_get_instance(handle, &val_type, &oid_string);
                if (rc != 0)
                {
                    ERROR("Failed to get instance value by handle "
                          "0x%x: %r", handle, rc);
                    return rc;
                }
                oid_struct = cfg_convert_oid_str(oid_string);
                if (oid_struct == NULL)
                {
                    ERROR("Failed to convert OID '%s' to structure",
                          oid_string);
                    free(oid_string);
                    return TE_RC(TE_TAPI, TE_EINVAL);
                }

                rc = tapi_cfg_ip6_get_linklocal_addr(
                         CFG_OID_GET_INST_NAME(oid_struct, 1),
                         CFG_OID_GET_INST_NAME(oid_struct, 2),
                         SIN6(env_addr->addr));
                if (rc != 0)
                {
                    ERROR("Failed to get link-local address for '%s': %r",
                          oid_string, rc);
                    free(oid_string);
                }
                free(oid_string);
                cfg_free_oid(oid_struct);
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
        }
        else
        {
            ERROR("Unsupported address family");
            return TE_EINVAL;
        }
    }
    return 0;
}


static te_errno
add_address(tapi_env_addr *env_addr, cfg_nets_t *cfg_nets,
            const struct sockaddr *addr)
{
    te_errno        rc;
    cfg_handle      handle;
    char           *str;
    cfg_val_type    val_type = CVT_STRING;


    handle = cfg_nets->nets[env_addr->iface->net->i_net].
                       nodes[env_addr->iface->i_node].handle;
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
                                    (addr->sa_family == AF_INET) ?
                                        env_addr->iface->net->ip4pfx :
                                        env_addr->iface->net->ip6pfx,
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
static te_errno
prepare_interfaces(tapi_env_ifs *ifs, cfg_nets_t *cfg_nets)
{
    te_errno        rc;
    tapi_env_if    *p;
    cfg_val_type    val_type;
    char           *oid = NULL;

    for (p = ifs->cqh_first; p != (void *)ifs; p = p->links.cqe_next)
    {
        if (p->name == NULL)
            continue;

        if (strcmp(p->name, "lo") != 0)
        {
            /* Get name of the interface from network node value */
            rc = node_value_get_ith_inst_name(
                     cfg_nets->nets[p->net->i_net].nodes[p->i_node].handle,
                     2, &p->if_info.if_name);
            if (rc != 0)
            {
                ERROR("Failed to get interface name");
                return rc;
            }

            val_type = CVT_STRING;
            rc = cfg_get_instance(cfg_nets->nets[p->net->i_net].
                                      nodes[p->i_node].handle,
                                  &val_type, &oid);
            if (rc != 0)
            {
                ERROR("Failed to get OID of the network node");
                return rc;
            }
            else /** XEN-specific part */
            {
                char const  xen[]    = "xen:/";
                char const  bridge[] = "/bridge:";
                char       *xen_oid  = malloc(strlen(oid) +
                                              strlen(xen) +
                                              strlen(bridge) + 1);
                char const *slash    = strrchr(oid, '/');

                if (slash == NULL)
                {
                    rc = TE_EFAIL;
                    goto cleanup0;
                }

                memcpy(xen_oid, oid, ++slash - oid);
                xen_oid[slash - oid] = '\0';
                strcat(xen_oid, xen);
                strcat(xen_oid, slash);
                val_type = CVT_STRING;

                if ((rc = cfg_get_instance_fmt(&val_type,
                                               &p->ph_info.if_name,
                                               xen_oid)) != 0)
                {
                    if (rc != TE_RC(TE_CS, TE_ENOENT))
                    {
                        ERROR("Failed to get '%s' OID value", xen_oid);
                        goto cleanup0;
                    }

                    p->ph_info.if_name = strdup("");
                }

                strcat(xen_oid, bridge);
                val_type = CVT_STRING;

                if ((rc = cfg_get_instance_fmt(&val_type,
                                               &p->br_info.if_name,
                                               xen_oid)) != 0)
                {
                    if (rc != TE_RC(TE_CS, TE_ENOENT))
                    {
                        ERROR("Failed to get '%s' OID value", xen_oid);
                        goto cleanup0;
                    }

                    p->br_info.if_name = strdup("");
                }

                rc = 0;
cleanup0:
                free(xen_oid);

                if (rc != 0)
                    return rc;
            }
        }
        else
        {
            const char *oid_fmt = "/agent:%s/interface:%s";
            char       *ta;

            /* Get name of the test agent */
            rc = node_value_get_ith_inst_name(
                     cfg_nets->nets[p->net->i_net].nodes[p->i_node].handle,
                     1, &ta);
            if (rc != 0)
            {
                ERROR("Failed to get test agent name");
                return rc;
            }

            if (cfg_find_fmt(NULL, oid_fmt, ta, "lo") == 0)
            {
                p->if_info.if_name = strdup("lo");
            }
            else if (cfg_find_fmt(NULL, oid_fmt, ta, "lo0") == 0)
            {
                p->if_info.if_name = strdup("lo0");
            }
            /* FIXME: Dirty hack for Windows */
            else if (cfg_find_fmt(NULL, oid_fmt, ta, "intf1") == 0)
            {
                p->if_info.if_name = strdup("intf1");
            }
            else
            {
                ERROR("Unable to get loopback interface");
                return TE_ESRCH;
            }
            if (p->if_info.if_name == NULL)
            {
                ERROR("strdup() failed");
                return te_rc_os2te(errno);
            }

            oid = malloc(strlen(oid_fmt) - 2 + strlen(ta) -
                         2 + strlen(p->if_info.if_name) + 1);
            if (oid == NULL)
            {
                ERROR("malloc() failed");
                return TE_ENOMEM;
            }
            sprintf(oid, oid_fmt, ta, p->if_info.if_name);
            
            free(ta);
        }

        val_type = CVT_INTEGER;
        rc = cfg_get_instance_fmt(&val_type, &(p->if_info.if_index),
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
 * @param hosts         list of hosts
 * @param cfg_nets      networks configuration
 *
 * @return Status code.
 */
static te_errno
prepare_pcos(tapi_env_hosts *hosts)
{
    te_errno            rc = 0;
    tapi_env_host      *host;
    tapi_env_process   *proc;
    tapi_env_pco       *pco;
    te_bool             main_thread;
    cfg_val_type        val_type;
    int                 iut_errno_change_no_check = 0;

    val_type = CVT_INTEGER;
    rc = cfg_get_instance_fmt(&val_type, &iut_errno_change_no_check,
                              "/local:/iut_errno_change_no_check:");
    if (rc != 0 && rc != TE_RC(TE_CS, TE_ENOENT))
    {
        ERROR("Failed to get '/local:/iut_errno_change_no_check:': %r",
              rc);
        return rc;
    }
    rc = 0;

    for (host = SLIST_FIRST(hosts);
         host != NULL && rc == 0;
         host = SLIST_NEXT(host, links))
    {
        for (proc = SLIST_FIRST(&host->processes);
             proc != NULL && rc == 0;
             proc = SLIST_NEXT(proc, links))
        {
            main_thread = TRUE;

            STAILQ_FOREACH(pco, &proc->pcos, links)
            {
                if (main_thread)
                {
                    if (rcf_rpc_server_get(host->ta, pco->name, NULL,
                                           RCF_RPC_SERVER_GET_EXISTING,
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
                    if (pco->type == TAPI_ENV_IUT)
                    {
                        pco->rpcs->errno_change_check =
                            !iut_errno_change_no_check;

                        if (host->libname != NULL)
                        {
                            rc = rcf_rpc_setlibname(pco->rpcs,
                                                    host->libname);
                            if (rc != 0)
                            {
                                rc = pco->rpcs->_errno;
                                ERROR("Failed to set RPC server '%s' "
                                      "dynamic library name '%s': %r", 
                                      pco->rpcs->name,
                                      host->libname ? : "(NULL)", rc);
                                break;
                            }
                        }
                    }
                }
                else
                {
                    rc = rcf_rpc_server_thread_create(
                             STAILQ_FIRST(&proc->pcos)->rpcs,
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
                (STAILQ_NEXT(pco = STAILQ_FIRST(&proc->pcos),
                             links) != NULL))
            {
                STAILQ_REMOVE(&proc->pcos, pco, tapi_env_pco, links);
                STAILQ_INSERT_TAIL(&proc->pcos, pco, links);
            }
        }
    }
    return rc;
}


static te_errno
bind_env_to_cfg_nets(tapi_env_ifs *ifs, cfg_nets_t *cfg_nets)
{
    te_errno        rc = 0;
    node_indexes    used_nodes;
    node_index     *p;

    /* Initialize empty list with used nodes */
    SLIST_INIT(&used_nodes);

    /* Recursively bind all hosts */
    if (!bind_host_if(ifs->cqh_first, ifs, cfg_nets, &used_nodes))
    {
        ERROR("Failed to bind requested environment configuration to "
              "available network configuration");
        rc = TE_EENV;
    }

    /* Free list of used indexes */
    while ((p = SLIST_FIRST(&used_nodes)) != NULL)
    {
        SLIST_REMOVE(&used_nodes, p, node_index, links);
        free(p);
    }
    
    return rc;
}


/**
 * Bind host to the node in network model.
 *
 * @param iface         host interface to be bound
 * @param ifs           list with all hosts/interfaces
 * @param cfg_nets      network model configuration
 * @param used_nodes    list with used (net, node) indexes
 *
 * @retval TRUE         success
 * @retval FALSE        failure
 */
static te_bool
bind_host_if(tapi_env_if *iface, tapi_env_ifs *ifs,
             cfg_nets_t *cfg_nets, node_indexes *used_nodes)
{
    unsigned int    i, j;
    tapi_env_if    *p;

    if (iface == (void *)ifs)
        return TRUE;

    assert(iface != NULL);
    assert(iface->net != NULL);
    assert(iface->host != NULL);

    VERB("Try to bind host '%s' interface '%s'",
         iface->host->name, iface->name);

    for (i = 0; i < cfg_nets->n_nets; ++i)
    {
        for (j = 0; j < cfg_nets->nets[i].n_nodes; ++j)
        {
            if (node_is_used(used_nodes, i, j))
            {
                VERB("Node (%u,%u) is already used", i, j);
                continue;
            }

            if (!check_net_type_cfg_vs_env(cfg_nets->nets + i,
                                           iface->net->type))
            {
                VERB("Node (%u,%u) type=%u is not suitable for the host "
                     "in the net with type=%u", i, j,
                     cfg_nets->nets[i].nodes[j].type, iface->net->type);
                continue;
            }

            if (!check_node_type_vs_pcos(cfg_nets,
                                         cfg_nets->nets[i].nodes + j,
                                         &iface->host->processes))
            {
                VERB("Node (%u,%u) type=%u is not suitable for the host",
                     i, j, cfg_nets->nets[i].nodes[j].type);
                continue;
            }
            VERB("Node (%u,%u) match PCOs type", i, j);

            /* Check that there is no conflicts with already bound nodes */
            p = iface;
            while ((p = p->links.cqe_prev) != (void *)ifs)
            {
                te_bool one_host;
                te_bool one_ta;

                if (iface->net == p->net)
                {
                    if (i != p->net->i_net)
                    {
                        VERB("Hosts '%s/%s'(0x%x) and '%s/%s'(0x%x) "
                             "must be in one net", iface->host->name,
                             iface->name, iface, p->host->name,
                             p->name, p);
                        break;
                    }
                }
                one_host = (iface->host == p->host);
                one_ta = (cmp_agent_names(
                              cfg_nets->nets[i].nodes[j].handle,
                              cfg_nets->nets[p->net->i_net].
                                  nodes[p->i_node].handle) == 0);
                /* 
                 * If host is the same, it implies that names are
                 * specified. If both names are not specified, allow
                 * any binding.
                 */
                if ((one_host != one_ta) &&
                    ((iface->host->name != NULL) ||
                     (p->host->name != NULL)))
                {
                    VERB("Hosts with %s names ('%s/%s' vs '%s/%s') "
                         "can't be bound to nodes %s",
                         one_host ? "the same" : "different",
                         iface->host->name, iface->name,
                         p->host->name, p->name,
                         one_ta ? "with the same test agent" :
                                  "on different agents");
                    break;
                }
            }
            if (p == (void *)ifs)
            {
                /* No conflicts discovered */
                iface->net->i_net = i;
                iface->i_node = j;
                if (node_mark_used(used_nodes, i, j) != 0)
                    return FALSE;
                VERB("Mark (%u,%u) as used by '%s/%s'", i, j,
                     iface->host->name, iface->name);
                /* Try to bind the next host/interface */
                if (bind_host_if(iface->links.cqe_next, ifs, cfg_nets,
                                 used_nodes))
                {
                    return TRUE;
                }
                VERB("Failed to bind host '%s/%s', unmark (%u,%u)",
                     iface->host->name, iface->name, i, j);
                /* Failed to bind the host */
                node_unmark_used(used_nodes, i, j);
                iface->net->i_net = iface->i_node = UINT_MAX;
            }
        }
    }

    VERB("Failed to bind host '%s/%s'", iface->host->name, iface->name);

    return FALSE;
}

static te_errno
node_value_get_ith_inst_name(cfg_handle node, unsigned int i, char **p_str)
{
    te_errno        rc;
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
    te_errno    rc;
    int         retval;
    char       *agt1;
    char       *agt2;

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
 * @retval TE_ENOMEM    memory allocation failure
 */
static te_errno
node_mark_used(node_indexes *used_nodes, unsigned int net, unsigned int node)
{
    node_index *p = calloc(1, sizeof(*p));
    
    if (p == NULL)
        return TE_ENOMEM;
        
    p->net = net;
    p->node = node;
    
    SLIST_INSERT_HEAD(used_nodes, p, links);
    
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

    for (p = SLIST_FIRST(used_nodes);
         (p != NULL) && ((net != p->net) || (node != p->node));
         p = SLIST_NEXT(p, links));

    if (p != NULL)    
        SLIST_REMOVE(used_nodes, p, node_index, links);
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

    for (p = SLIST_FIRST(used_nodes);
         (p != NULL) && ((net != p->net) || (node != p->node));
         p = SLIST_NEXT(p, links));

    return (p != NULL);
}

/**
 * Get type of PCOs.
 *
 * @param procs         List of processes
 *
 * @return Environment node type.
 * @retval TAPI_ENV_IUT     At least one process should have IUT
 * @retval TAPI_ENV_TESTER  All processes are Testers
 */
static tapi_env_type
get_pcos_type(tapi_env_processes *procs)
{
    tapi_env_process *proc;
    tapi_env_pco     *pco;

    SLIST_FOREACH(proc, procs, links)
    {
        STAILQ_FOREACH(pco, &proc->pcos, links)
        {
            if (pco->type == TAPI_ENV_IUT)
            {
                VERB("%s(): PCOs are IUT", __FUNCTION__);
                return TAPI_ENV_IUT;
            }
        }
    }
    VERB("%s(): PCOs are Tester", __FUNCTION__);
    return TAPI_ENV_TESTER;
}

/**
 * Get TA type associated with specified configuration network node.
 *
 * @param cfg_nets      All information about configuration networks
 * @param node          Configuration network node
 *
 * @return Configuration network node type.
 */
static enum net_node_type
get_ta_type(cfg_nets_t *cfg_nets, cfg_net_node_t *node)
{
    enum net_node_type  type = node->type;
    unsigned int        i;
    unsigned int        j;

    for (i = 0; i < cfg_nets->n_nets && type == NET_NODE_TYPE_AGENT; ++i)
    {
        for (j = 0;
             j < cfg_nets->nets[i].n_nodes && type == NET_NODE_TYPE_AGENT;
             ++j)
        {
            if ((node->handle != cfg_nets->nets[i].nodes[j].handle) &&
                cmp_agent_names(node->handle,
                    cfg_nets->nets[i].nodes[j].handle) == 0 &&
                cfg_nets->nets[i].nodes[j].type != NET_NODE_TYPE_AGENT)
            {
                type = cfg_nets->nets[i].nodes[j].type;
            }
        }
    }
    VERB("%s(): TA type is %u", __FUNCTION__, type);

    return type;
}

/**
 * Check that network node type matches type of requested PCOs.
 *
 * @param cfg_nets      All information about configuration networks
 * @param node          Configuration network node
 * @param processes     Processes with PCOs on the host to be bound
 *
 * @return Whether types match?
 */
static te_bool
check_node_type_vs_pcos(cfg_nets_t         *cfg_nets,
                        cfg_net_node_t     *node,
                        tapi_env_processes *processes)
{
    return (get_pcos_type(processes) != TAPI_ENV_IUT) ||
           (get_ta_type(cfg_nets, node) == NET_NODE_TYPE_NUT);
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
check_net_type_cfg_vs_env(cfg_net_t *net, tapi_env_type net_type)
{
    enum net_node_type  node_type;
    unsigned int        i;

    /* Network is considered as IUT, if it has at least one NUT */
    for (node_type = NET_NODE_TYPE_AGENT, i = 0; i < net->n_nodes; ++i)
    {
        if (net->nodes[i].type == NET_NODE_TYPE_NUT)
        {
            node_type = NET_NODE_TYPE_NUT;
            break;
        }
    }

    switch (net_type)
    {
        case TAPI_ENV_UNSPEC:
            return TRUE;

        case TAPI_ENV_IUT:
            return (node_type == NET_NODE_TYPE_NUT);

        case TAPI_ENV_TESTER:
            return (node_type == NET_NODE_TYPE_AGENT);

        default:
            assert(0);
            return FALSE;
    }
}


/* See description in tapi_env.h */
te_errno
tapi_env_get_net_host_addr(const tapi_env          *env,
                           const tapi_env_net      *net,
                           const tapi_env_host     *host,
                           tapi_cfg_net_assigned   *assigned,
                           struct sockaddr        **addr,
                           socklen_t               *addrlen)
{
    te_errno            rc;
    cfg_val_type        val_type;
    char               *node_oid;
    const tapi_env_if  *iface;


    if (net == NULL || host == NULL || assigned == NULL || addr == NULL)
    {
        ERROR("%s(): Invalid parameter", __FUNCTION__);
        return TE_EINVAL;
    }

    for (iface = env->ifs.cqh_first;
         iface != (void *)&env->ifs &&
            (iface->net != net || iface->host != host);
         iface = iface->links.cqe_next);
    
    if (iface == (void *)&env->ifs)
    {
        ERROR("Host '%s' does not belong to the net", host->name);
        return TE_RC(TE_TAPI, TE_ESRCH);
    }

    /* Get string OID of the associated net in networks configuration */
    rc = cfg_get_oid_str(net->cfg_net->nodes[iface->i_node].handle,
                         &node_oid);
    if (rc != 0)
    {
        ERROR("Failed to string OID by handle 0x%x: %r",
              net->cfg_net->nodes[iface->i_node], rc);
        return rc;
    }

    /* Get IPv4 subnet address */
    val_type = CVT_ADDRESS;
    rc = cfg_get_instance_fmt(&val_type, addr,
                              "%s/ip4_address:%u", node_oid,
                              assigned->entries[iface->i_node]);
    if (rc != 0)
    {
        ERROR("Failed to get IPv4 address assigned to the node '%s' "
              "with handle 0x%x: %r", node_oid,
              assigned->entries[iface->i_node], rc);
        free(node_oid);
        return rc;
    }
    free(node_oid);

    if (addrlen != NULL)
        *addrlen = te_sockaddr_get_size(*addr);

    return 0;
}
