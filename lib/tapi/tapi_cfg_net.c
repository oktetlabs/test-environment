/** @file
 * @brief Network Configuration Model TAPI
 *
 * Implementation of test API for network configuration model
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

#include "te_config.h"

#include <stdio.h>
#ifdef STDC_HEADERS
#include <stdlib.h>
#include <string.h>
#endif
#if HAVE_ASSERT_H
#include <assert.h>
#endif
#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#if HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

#include "te_defs.h"
#include "te_errno.h"
#include "te_stdint.h"
#include "conf_api.h"

#include "tapi_cfg.h"
#include "tapi_cfg_base.h"
#include "tapi_cfg_net.h"

#define TE_LGR_USER      "Configuration TAPI"
#include "logger_api.h"


/* See description in tapi_cfg_net.h */
int
tapi_cfg_net_get_nets(cfg_nets_t *nets)
{
    int             rc;
    int             n_nets;
    cfg_handle     *net_handles;
    unsigned int    i;

    rc = cfg_find_pattern("/net:*", &n_nets, &net_handles);
    if (rc != 0)
    {
        ERROR("cfg_find_pattern() failed %r", rc);
        return rc;
    }
    nets->n_nets = n_nets;
    if (n_nets == 0)
    {
        nets->nets = NULL;
    }
    else
    {
        nets->nets = (cfg_net_t *)calloc(nets->n_nets,
                                         sizeof(*(nets->nets)));
        if (nets->nets == NULL)
        {
            ERROR("Memory allocation failure");
            rc = TE_ENOMEM;
        }
    }

    for (i = 0; rc == 0 && i < nets->n_nets; ++i)
    {
        cfg_net_t      *net = nets->nets + i;
        char           *net_oid;
        int             n_nodes;
        cfg_handle     *node_handles;
        unsigned int    j;

        /* Save cfg handle of the net */
        net->handle = net_handles[i];

        /* Get net OID as string */
        rc = cfg_get_oid_str(net_handles[i], &net_oid);
        if (rc != 0)
        {
            ERROR("cfg_get_oid_str() failed %r", rc);
            break;
        }
        /* Find all nodes in this net */
        rc = cfg_find_pattern_fmt(&n_nodes, &node_handles,
                                  "%s/node:*", net_oid);
        free(net_oid);
        if (rc != 0)
        {
            ERROR("cfg_find_pattern() failed %r", rc);
            break;
        }
        net->n_nodes = n_nodes;
        if (n_nodes == 0)
        {
            net->nodes = NULL;
        }
        else
        {
            net->nodes =
                (cfg_net_node_t *)calloc(net->n_nodes,
                                         sizeof(*(net->nodes)));
            if (net->nodes == NULL)
            {
                ERROR("Memory allocation failure");
                break;
            }
        }
        
        for (j = 0; j < net->n_nodes; ++j)
        {
            char           *node_oid;
            cfg_val_type    val_type = CVT_INTEGER;
            int             val;

            /* Save cfg handle of the net node */
            net->nodes[j].handle = node_handles[j];

            /* Get net node OID as string */
            rc = cfg_get_oid_str(node_handles[j], &node_oid);
            if (rc != 0)
            {
                ERROR("cfg_get_oid_str() failed %r", rc);
                break;
            }
            /* Get node type */
            rc = cfg_get_instance_fmt(&val_type, &val,
                                      "%s/type:", node_oid);
            free(node_oid);
            if (rc != 0)
            {
                ERROR("cfg_get_instance_fmt() failed %r", rc);
                break;
            }
            net->nodes[j].type = val;
        }
        free(node_handles);
    }
    free(net_handles);

    if (rc != 0)
        tapi_cfg_net_free_nets(nets);

    return rc;
}

/* See description in tapi_cfg_net.h */
void
tapi_cfg_net_free_nets(cfg_nets_t *nets)
{
    if (nets != NULL)
    {
        unsigned int    i;

        for (i = 0; i < nets->n_nets; ++i)
        {
            free(nets->nets[i].nodes);
        }
        free(nets->nets);
    }
}


/* See description in tapi_cfg_net.h */
int
tapi_cfg_net_get_pairs(enum net_node_type first,
                       enum net_node_type second,
                       unsigned int *p_n_pairs,
                       cfg_handle (**p_pairs)[2])
{
    int             rc;
    cfg_nets_t      nets;
    unsigned int    n_pairs = 0;
    cfg_handle    (*pairs)[2] = NULL;

    /* Get testing network configuration in C structures */
    rc = tapi_cfg_net_get_nets(&nets);
    if (rc != 0)
    {
        ERROR("tapi_cfg_net_get_nets() failed %r", rc);
        return rc;
    }

    if (nets.n_nets > 0)
    {
        unsigned int    i, j;

        /* Allocate memory for maximum number of pairs */
        pairs = malloc(nets.n_nets * sizeof(*pairs));
        if (pairs == NULL)
        {
            tapi_cfg_net_free_nets(&nets);
            return TE_ENOMEM;
        }
        for (i = 0; i < nets.n_nets; ++i)
        {
            cfg_net_t  *net = nets.nets + i;

            pairs[n_pairs][0] = CFG_HANDLE_INVALID;
            pairs[n_pairs][1] = CFG_HANDLE_INVALID;

            for (j = 0; j < net->n_nodes; ++j)
            {
                if ((pairs[n_pairs][0] == CFG_HANDLE_INVALID) &&
                    (net->nodes[j].type == first))
                {
                    pairs[n_pairs][0] = net->nodes[j].handle;
                }
                else if ((pairs[n_pairs][1] == CFG_HANDLE_INVALID) &&
                         (net->nodes[j].type == second))
                {
                    pairs[n_pairs][1] = net->nodes[j].handle;
                }
            }

            if ((pairs[n_pairs][0] != CFG_HANDLE_INVALID) &&
                (pairs[n_pairs][1] != CFG_HANDLE_INVALID))
            {
                ++n_pairs;
            }
        }
    }
    tapi_cfg_net_free_nets(&nets);

    *p_n_pairs = n_pairs;
    *p_pairs = pairs;

    return 0;
}


/* See description in tapi_cfg_net.h */
int
tapi_cfg_net_find_net_by_node(const char *oid, char *net)
{
    int         rc;
    int         net_num;
    cfg_handle *net_handles = NULL;
    int         i;


    /* Get handles of all nodes in all networks */
    rc = cfg_find_pattern("/net:*", &net_num, &net_handles);
    if (rc != 0)
    {
        ERROR("Failed(%x) to find all nodes", rc);
        return rc;
    }

    for (i = 0, rc = TE_ESRCH; i < net_num; ++i, rc = TE_ESRCH)
    {
        char       *net_name;
        int         node_num;
        cfg_handle *node_handles = NULL;
        int         j;

        rc = cfg_get_inst_name(net_handles[i], &net_name);
        if (rc != 0)
        {
            ERROR("Failed(%x) to get cfg OID instance name by handle", rc);
            break;
        }

        rc = cfg_find_pattern_fmt(&node_num, &node_handles,
                                  "/net:%s/node:*", net_name);
        if (rc != 0)
        {
            ERROR("Failed(%x) to find nodes in the net '%s'", net_name);
            break;
        }
        
        for (j = 0, rc = ESRCH; j < node_num; ++j, rc = ESRCH)
        {
            cfg_val_type  type;
            char         *val;
            int           res;

            type = CVT_STRING;
            rc = cfg_get_instance(node_handles[j], &type, &val);
            if (rc != 0)
            {
                ERROR("Failed(%x) to get value by cfg handle", rc);
                break;
            }

            res = strcmp(oid, val);
            free(val);
            if (res == 0)
            {
                strcpy(net, net_name);
                free(net_name);
                free(node_handles);
                free(net_handles);
                return 0;
            }
        }
        free(node_handles);
        free(net_name);
    }
    free(net_handles);

    return rc;

} /* tapi_cfg_net_find_net_by_node */


/* See description in tapi_cfg_net.h */
int
tapi_cfg_net_get_nodes_values(const char *net_name,
                              enum net_node_type node_type,
                              const char *ta_name, char ***oids)
{
    te_errno            rc = 0;
    cfg_handle         *handles = NULL;
    unsigned int        num;
    unsigned int        i;
    cfg_oid            *oid = NULL;
    cfg_val_type        val_type;
    enum net_node_type  cfg_node_type;

    char  *val;
    int    ret_array_len = 1; /* The last entry always equals to NULL */
    char **ret_array = NULL;

#define THROW_EXCEPTION(err_msg...) \
    do {                \
        ERROR(err_msg);   \
        goto clean_all; \
    } while (0)

/** 
 * Deallocates memory specified in "ptr_" prameter by using "func_" function
 * and then sets the variable to NULL.
 */
#define SAFE_FREE(ptr_, func_) \
    do {                       \
        func_(ptr_);           \
        ptr_ = NULL;           \
    } while (0)

    VERB("In get_node_inst_value:\n"
         "\tnet name   : %s\n"
         "\tnode type  : %d\n"
         "\tagent name : %s\n",
         ((net_name == NULL) ? "NULL" : net_name), node_type,
         ((ta_name == NULL) ? "NULL" : ta_name));

    if (net_name == NULL)
        net_name = "*";

    /* Get the first Agent on the specified net */

    if ((rc = cfg_find_pattern_fmt(&num, &handles,
                                   "/net:%s/node:*", net_name)) != 0)
        THROW_EXCEPTION("Error while obtaining /net:%s/node:* instances",
                        net_name);

    for (i = 0; i < num; i++)
    {
        int str_len;

        rc = cfg_get_oid(handles[i], &oid);
        if (rc != 0)
            THROW_EXCEPTION("Error while getting OID by handle");
        assert(oid->inst);

        VERB("Net/Node: %s", oid);

        val_type = CVT_INTEGER;
        rc = cfg_get_instance_fmt(&val_type, &cfg_node_type, 
                                  "%s/type:", oid);
        if (rc != 0)
            THROW_EXCEPTION("Error while getting type of node %s", oid);

        VERB("Node type: %d (expected %d)", cfg_node_type, node_type);

        if (cfg_node_type != node_type)
        {
            SAFE_FREE(oid, cfg_free_oid);
            continue;
        }


        VERB("Node %s has expected type", oid);

        /* 
         * Processing of the node of specified type:
         * Check if node value has format we are expecting and that 
         * agent name the same as specified.
         */

        val_type = CVT_STRING;
        if ((rc = cfg_get_instance(handles[i], &val_type, &val)) != 0)
            THROW_EXCEPTION("Error while getting value of %s", oid);

        VERB("Node value: %s", val);

        str_len = strlen("/agent:");

        if (strncmp(val, "/agent:", str_len) == 0)
        {
            VERB("A[], ta_name = %s", ta_name == NULL ?  "NULL" : ta_name);

            if (ta_name == NULL ||
                (strncmp(val + str_len, ta_name, strlen(ta_name)) == 0 &&
                 *(val + str_len + strlen(ta_name)) == '/'))
            {
                char **tmp_ptr = ret_array;

                if ((ret_array = (char **)realloc(ret_array,
                           sizeof(char *) * (ret_array_len + 1))) == NULL)
                {
                    rc = TE_ENOMEM;
                    free(val);
                    ret_array = tmp_ptr;
                    THROW_EXCEPTION("Not enough memory");
                }
                ret_array[ret_array_len - 1] = val;
                ret_array[ret_array_len] = NULL;
                ret_array_len++;
            }
            else
            {
                free(val);
            }
        }
        else
        {
            free(val);
        }
        SAFE_FREE(oid, cfg_free_oid);
    }

clean_all:
    
    if (rc != 0)
    {
        tapi_cfg_net_free_nodes_values(ret_array);
    }
    else if (ret_array_len == 1)
    {
        rc = TE_ENOENT;
    }
    else
    {
        *oids = ret_array;
    }
    free(handles);
    cfg_free_oid(oid);

    return rc;

} /* tapi_cfg_net_get_nodes_values */


/* See description in tapi_cfg_net.h */
void
tapi_cfg_net_free_nodes_values(char **oids)
{
    char **ptr = oids;
 
    if (ptr == NULL)
        return;

    while (*ptr != NULL)
    {
        free(*ptr);
        ptr++;
    }
    free(oids);
} /* tapi_cfg_net_free_nodes_values */


/* See description in tapi_cfg_net.h */
int
tapi_cfg_net_get_switch_port(const char *ta_node, unsigned int *p_port)
{
    int             rc;
    cfg_val_type    val_type = CVT_INTEGER;
    int             type;
    cfg_oid        *ta_node_oid;
    char          **oids = NULL;
    cfg_oid        *nut_oid = NULL;
    char           *port_name;
    char           *end;
    long int        port;

    rc = cfg_get_instance_fmt(&val_type,  &type, "%s/type:", ta_node);
    if (rc != 0)
    {
        ERROR("cfg_get_instance_fmt() failed %r", rc);
        return rc;
    }
    if (type != NET_NODE_TYPE_AGENT)
    {
        ERROR("Node of inappropriate type %d provided as argument", type);
        return TE_EINVAL;
    }

    ta_node_oid = cfg_convert_oid_str(ta_node);
    if (ta_node_oid == NULL)
    {
        ERROR("Failed to parse OID %s", ta_node);
        return TE_EFMT;
    }
    assert(ta_node_oid->inst);
    assert(ta_node_oid->len == 3);

    /* Find all NUTs in TA net */
    rc = tapi_cfg_net_get_nodes_values(
             ((cfg_inst_subid *)(ta_node_oid->ids))[1].name,
             NET_NODE_TYPE_NUT, NULL, &oids);
    if (rc != 0)
    {
        ERROR("Failed to find NUT nodes in net %s",
            ((cfg_inst_subid *)(ta_node_oid->ids))[1].name);
        cfg_free_oid(ta_node_oid);
        return rc;
    }
    cfg_free_oid(ta_node_oid);
    if (oids == NULL || oids[0] == NULL || oids[1] != NULL)
    {
        ERROR("Invalid network configuration");
        tapi_cfg_net_free_nodes_values(oids);
        return TE_EENV;
    }
    
    /* Parse NUT OID and get port */
    nut_oid = cfg_convert_oid_str(oids[0]);
    if ((nut_oid == NULL) || !(nut_oid->inst) || (nut_oid->len != 3))
    {
        ERROR("Invalid NUT node value '%s'", oids[0]);
        tapi_cfg_net_free_nodes_values(oids);
        return TE_EINVAL;
    }
    tapi_cfg_net_free_nodes_values(oids);

    port_name = ((cfg_inst_subid *)(nut_oid->ids))[2].name;
    port = strtol(port_name, &end, 10);
    if (end == port_name || *end != '\0' || port < 0)
    {
        ERROR("Invlaid port number '%s' format", port_name);
        cfg_free_oid(nut_oid);
        return TE_EFMT;
    }
    cfg_free_oid(nut_oid);
    *p_port = port;

    return 0;
}

/* See description in tapi_cfg_net.h */
int
tapi_cfg_net_all_up(void)
{
    int             rc;
    cfg_nets_t      nets;
    unsigned int    i, j;

    /* Get available networks configuration */
    rc = tapi_cfg_net_get_nets(&nets);
    if (rc != 0)
    {
        ERROR("Failed to get networks from Configurator: %r", rc);
        return rc;
    }

    for (i = 0; i < nets.n_nets; ++i)
    {
        cfg_net_t  *net = nets.nets + i;

        for (j = 0; j < net->n_nodes; ++j)
        {
            cfg_val_type    type;
            char           *oid = NULL;
            int             status;

            type = CVT_STRING;
            rc = cfg_get_instance(net->nodes[j].handle, &type, &oid);
            if (rc != 0)
            {
                ERROR("Failed to get Configurator instance by handle "
                      "0x%x: %r", net->nodes[j].handle, rc);
                break;
            }

            type = CVT_INTEGER;
            rc = cfg_get_instance_fmt(&type, &status, "%s/status:", oid);
            if (rc != 0)
            {
                ERROR("Failed to get status of %s: %r", oid, rc);
                free(oid);
                break;
            }
            if (status != 1)
            {
                rc = cfg_set_instance_fmt(CFG_VAL(INTEGER, 1),
                                          "%s/status:", oid);
                if (rc != 0)
                {
                    ERROR("Failed to set status of %s to UP: %r",
                          oid, rc);
                    free(oid);
                    break;
                }
            }
            else
            {
                INFO("Node (interface) %s is already UP", oid);
            }
            free(oid);
        }
        if (rc != 0)
            break;
    }

    tapi_cfg_net_free_nets(&nets);

    return rc;
}


/* See description in tapi_cfg_net.h */
int
tapi_cfg_net_assign_ip4(cfg_net_t *net, tapi_cfg_net_assigned *assigned)
{
    int                 rc;
    cfg_val_type        type;
    char               *str;
    unsigned int        i;
    cfg_handle          ip4_net_hndl;
    char               *ip4_net_oid = NULL;
    unsigned int        ip4_net_pfx;
    struct sockaddr    *ip4_net_addr;
    cfg_handle          ip4_entry_hndl;
    cfg_handle          ip4_addr_hndl;
    struct sockaddr_in *ip4_addr;

    if (net == NULL)
    {
        ERROR("%s: Net pointer is NULL", __FUNCTION__);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    /* Allocate or use passed IPv4 subnet */
    if (assigned == NULL ||
        assigned->pool == CFG_HANDLE_INVALID)
    {
        rc = tapi_cfg_alloc_ip4_net(&ip4_net_hndl);
        if (rc != 0)
        {
            ERROR("%s: Failed to allocate IPv4 net to assign: %r",
                  __FUNCTION__, rc);
            return rc;
        }
    }
    else
    {
        ip4_net_hndl = assigned->pool;
    }

    do { /* Fake loop */

        /* 
         * Get all information about this IPv4 subnet
         */
        rc = cfg_get_inst_name_type(ip4_net_hndl, CVT_ADDRESS,
                                    CFG_IVP(&ip4_net_addr));
        if (rc != 0)
        {
            ERROR("%s(): cfg_get_inst_name_type(0x%x) failed",
                  __FUNCTION__, ip4_net_hndl);
            break;
        }

        /* Get OID as string */
        rc = cfg_get_oid_str(ip4_net_hndl, &ip4_net_oid);
        if (rc != 0)
        {
            ERROR("%s(): cfg_get_oid_str(0x%x) failed",
                  __FUNCTION__, ip4_net_hndl);
            break;
        }
        /* Get prefix length */
        type = CVT_INTEGER;
        rc = cfg_get_instance_fmt(&type, &ip4_net_pfx,
                                  "%s/prefix:", ip4_net_oid);
        if (rc != 0)
        {
            ERROR("Failed to get IPv4 subnet '%s' prefix: %r",
                  ip4_net_oid, rc);
            break;
        }

        /* Add the subnet to the list of IPv4 subnets of the net */
        rc = cfg_add_instance_child_fmt(NULL, CVT_ADDRESS, ip4_net_addr,
                                        net->handle, "/ip4_subnet:%u",
                                        ip4_net_hndl);
        if (rc != 0)
        {
            ERROR("Failed to add '/ip4_subnet:%u' child to "
                  "instance with handle 0x%x: %r",
                  ip4_net_hndl, net->handle, rc);
            break;
        }

        if ((assigned != NULL) && (net->n_nodes > 0))
        {
            assigned->entries = calloc(net->n_nodes,
                                       sizeof(*(assigned->entries)));
            if (assigned->entries == NULL)
            {
                ERROR("calloc(%u, %u) failed", net->n_nodes,
                      sizeof(*(assigned->entries)));
                rc = TE_ENOMEM;
                break;
            }
        }

        /*
         * Assign IPv4 addresses to each node of the net.
         */
        for (i = 0; i < net->n_nodes; ++i)
        {
            rc = tapi_cfg_alloc_ip4_addr(ip4_net_hndl, &ip4_entry_hndl,
                                         &ip4_addr);
            if (rc != 0)
            {
                ERROR("Failed to allocate address for node #%u: %r",
                      i, rc);
                break;
            }

            /* Get interface OID */
            type = CVT_STRING;
            rc = cfg_get_instance(net->nodes[i].handle, &type,
                                  &str);
            if (rc != 0)
            {
                ERROR("Failed to get Configurator instance by handle "
                      "0x%x: %r", net->nodes[i].handle, rc);
                free(ip4_addr);
                break;
            }

            rc = tapi_cfg_base_add_net_addr(str, SA(ip4_addr),
                                            ip4_net_pfx, TRUE,
                                            &ip4_addr_hndl);
            if (TE_RC_GET_ERROR(rc) == TE_EEXIST)
            {
                /* Address already assigned - continue */
                rc = 0;
            } else if (rc != 0)
            {
                free(ip4_addr);
                break;
            }

            rc = cfg_add_instance_child_fmt(NULL, CVT_ADDRESS,
                                            SA(ip4_addr),
                                            net->nodes[i].handle,
                                            "/ip4_address:%u",
                                            ip4_entry_hndl);
            if (rc != 0)
            {
                ERROR("Failed to add 'ip4_address:%u' child to "
                      "instance with handle 0x%x: %r",
                      ip4_entry_hndl, net->nodes[i].handle, rc);
                free(ip4_addr);
                break;
            }
            free(ip4_addr);

            if (assigned != NULL)
                assigned->entries[i] = ip4_entry_hndl;
        }

    } while (0); /* End of fake loop */

    if ((rc == 0) &&
        (assigned != NULL) && (assigned->pool == CFG_HANDLE_INVALID))
    {
        assigned->pool= ip4_net_hndl;
    }

    free(ip4_net_oid);

    return rc;
}


/* See description in tapi_cfg_net.h */
te_errno
tapi_cfg_net_all_assign_ip4(void)
{
    te_errno        rc;
    cfg_nets_t      nets;
    unsigned int    i;

    /* Get available networks configuration */
    rc = tapi_cfg_net_get_nets(&nets);
    if (rc != 0)
    {
        ERROR("Failed to get networks from Configurator: %r", rc);
        return rc;
    }

    for (i = 0; i < nets.n_nets; ++i)
    {
        rc = tapi_cfg_net_assign_ip4(nets.nets + i, NULL);
        if (rc != 0)
        {
            ERROR("Failed to assign IPv4 subnet to net #%u: %r", i, rc);
            break;
        }
    }

    tapi_cfg_net_free_nets(&nets);

    return rc;
}


/* See description in tapi_cfg_net.h */
int
tapi_cfg_net_assign_ip4_one_end(cfg_net_t *net, 
                                tapi_cfg_net_assigned *assigned)
{
    int                 rc;
    cfg_val_type        type;
    char               *str;
    unsigned int        i;
    cfg_handle          ip4_net_hndl;
    char               *ip4_net_oid = NULL;
    unsigned int        ip4_net_pfx;
    struct sockaddr    *ip4_net_addr;
    cfg_handle          ip4_entry_hndl;
    cfg_handle          ip4_addr_hndl;
    struct sockaddr_in *ip4_addr;

    if (net == NULL)
    {
        ERROR("%s: Net pointer is NULL", __FUNCTION__);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    /* Allocate or use passed IPv4 subnet */
    if (assigned == NULL ||
        assigned->pool == CFG_HANDLE_INVALID)
    {
        rc = tapi_cfg_alloc_ip4_net(&ip4_net_hndl);
        if (rc != 0)
        {
            ERROR("%s: Failed to allocate IPv4 net to assign: %r",
                  __FUNCTION__, rc);
            return rc;
        }
    }
    else
    {
        ip4_net_hndl = assigned->pool;
    }

    do { /* Fake loop */

        /* 
         * Get all information about this IPv4 subnet
         */
        rc = cfg_get_inst_name_type(ip4_net_hndl, CVT_ADDRESS,
                                    CFG_IVP(&ip4_net_addr));
        if (rc != 0)
        {
            ERROR("%s(): cfg_get_inst_name_type(0x%x) failed",
                  __FUNCTION__, ip4_net_hndl);
            break;
        }

        /* Get OID as string */
        rc = cfg_get_oid_str(ip4_net_hndl, &ip4_net_oid);
        if (rc != 0)
        {
            ERROR("%s(): cfg_get_oid_str(0x%x) failed",
                  __FUNCTION__, ip4_net_hndl);
            break;
        }
        /* Get prefix length */
        type = CVT_INTEGER;
        rc = cfg_get_instance_fmt(&type, &ip4_net_pfx,
                                  "%s/prefix:", ip4_net_oid);
        if (rc != 0)
        {
            ERROR("Failed to get IPv4 subnet '%s' prefix: %r",
                  ip4_net_oid, rc);
            break;
        }

        /* Add the subnet to the list of IPv4 subnets of the net */
        rc = cfg_add_instance_child_fmt(NULL, CVT_ADDRESS, ip4_net_addr,
                                        net->handle, "/ip4_subnet:%u",
                                        ip4_net_hndl);
        if (rc != 0)
        {
            ERROR("Failed to add '/ip4_subnet:%u' child to "
                  "instance with handle 0x%x: %r",
                  ip4_net_hndl, net->handle, rc);
            break;
        }

        if ((assigned != NULL) && (net->n_nodes > 0))
        {
            assigned->entries = calloc(net->n_nodes,
                                       sizeof(*(assigned->entries)));
            if (assigned->entries == NULL)
            {
                ERROR("calloc(%u, %u) failed", net->n_nodes,
                      sizeof(*(assigned->entries)));
                rc = TE_ENOMEM;
                break;
            }
        }

        /*
         * Assign IPv4 addresses to each node of the net.
         */
        for (i = 1; i < net->n_nodes; ++i)
        {
            rc = tapi_cfg_alloc_ip4_addr(ip4_net_hndl, &ip4_entry_hndl,
                                         &ip4_addr);
            if (rc != 0)
            {
                ERROR("Failed to allocate address for node #%u: %r",
                      i, rc);
                break;
            }

            /* Get interface OID */
            type = CVT_STRING;
            rc = cfg_get_instance(net->nodes[i].handle, &type,
                                  &str);
            if (rc != 0)
            {
                ERROR("Failed to get Configurator instance by handle "
                      "0x%x: %r", net->nodes[i].handle, rc);
                free(ip4_addr);
                break;
            }

            rc = tapi_cfg_base_add_net_addr(str, SA(ip4_addr),
                                            ip4_net_pfx, TRUE,
                                            &ip4_addr_hndl);
            if (TE_RC_GET_ERROR(rc) == TE_EEXIST)
            {
                /* Address already assigned - continue */
                rc = 0;
            } else if (rc != 0)
            {
                free(ip4_addr);
                break;
            }

            rc = cfg_add_instance_child_fmt(NULL, CVT_ADDRESS,
                                            SA(ip4_addr),
                                            net->nodes[i].handle,
                                            "/ip4_address:%u",
                                            ip4_entry_hndl);
            if (rc != 0)
            {
                ERROR("Failed to add 'ip4_address:%u' child to "
                      "instance with handle 0x%x: %r",
                      ip4_entry_hndl, net->nodes[i].handle, rc);
                free(ip4_addr);
                break;
            }
            free(ip4_addr);

            if (assigned != NULL)
                assigned->entries[i] = ip4_entry_hndl;
        }

    } while (0); /* End of fake loop */

    if ((rc == 0) &&
        (assigned != NULL) && (assigned->pool == CFG_HANDLE_INVALID))
    {
        assigned->pool= ip4_net_hndl;
    }

    free(ip4_net_oid);

    return rc;
}
