/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Network Configuration Model TAPI
 *
 * Implementation of test API for network configuration model
 * (doc/cm/cm_net.xml).
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#define TE_LGR_USER      "Configuration TAPI"

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
#include "te_alloc.h"
#include "te_sockaddr.h"
#include "te_str.h"
#include "rcf_api.h"
#include "logger_api.h"
#include "conf_api.h"

#include "tapi_cfg.h"
#include "tapi_cfg_base.h"
#include "tapi_cfg_iptables.h"
#include "tapi_cfg_net.h"
#include "tapi_host_ns.h"

#define TAPI_CFG_NET_OID_INTERFACE \
                "/agent/interface"
#define TAPI_CFG_NET_OID_PCI_INSTANCE \
                "/agent/hardware/pci/vendor/device/instance"
#define TAPI_CFG_NET_OID_NETDEV \
                "/agent/hardware/pci/vendor/device/instance/netdev"

/* See description in tapi_cfg_net.h */
enum net_node_rsrc_type
tapi_cfg_net_get_node_rsrc_type(cfg_net_node_t *node)
{
    te_errno        rc;
    cfg_val_type    type;
    char           *inst_oid = NULL;
    char            obj_oid[CFG_OID_MAX];

    if (node->rsrc_type != NET_NODE_RSRC_TYPE_UNKNOWN)
        return node->rsrc_type;

    type = CVT_STRING;
    rc = cfg_get_instance(node->handle, &type, &inst_oid);
    if (rc != 0)
    {
        ERROR("Failed to get Configurator instance by handle "
              "0x%x: %r", node->handle, rc);
        return rc;
    }

    cfg_oid_inst2obj(inst_oid, obj_oid);

    if (strcmp(obj_oid, TAPI_CFG_NET_OID_INTERFACE) == 0)
    {
        node->rsrc_type = NET_NODE_RSRC_TYPE_INTERFACE;
    }
    else if (strcmp(obj_oid, TAPI_CFG_NET_OID_PCI_INSTANCE) == 0)
    {
        node->rsrc_type = NET_NODE_RSRC_TYPE_PCI_FN;
    }
    else if (strcmp(obj_oid, TAPI_CFG_NET_OID_NETDEV) == 0)
    {
        node->rsrc_type = NET_NODE_RSRC_TYPE_PCI_FN_NETDEV;
    }
    else if (strcmp(obj_oid, "/local/dpdk/vdev") == 0)
    {
        node->rsrc_type = NET_NODE_RSRC_TYPE_RTE_VDEV;
    }

    free(inst_oid);

    return node->rsrc_type;
}

/* See description in tapi_cfg_net.h */
te_errno
tapi_cfg_net_get_net(cfg_handle net_handle, cfg_net_t *net)
{
    te_errno        rc;
    char           *net_oid;
    char           *nat_setup;
    cfg_handle     *gateway_handles;
    unsigned int    n_gateways;
    cfg_handle     *node_handles;
    unsigned int    n_nodes;
    unsigned int    i;


    if (net_handle == CFG_HANDLE_INVALID || net == NULL)
        return TE_RC(TE_TAPI, TE_EINVAL);

    memset(net, 0, sizeof(*net));

    /* Save cfg handle of the net */
    net->handle = net_handle;

    /* Get net OID as string */
    rc = cfg_get_oid_str(net_handle, &net_oid);
    if (rc != 0)
    {
        ERROR("cfg_get_oid_str() failed %r", rc);
        return rc;
    }

    net->name = cfg_oid_str_get_inst_name(net_oid, -1);
    if (net->name == NULL)
    {
        ERROR("Failed to get the last instance name from OID '%s'", net_oid);
        return TE_RC(TE_TAPI, TE_EFAULT);
    }

    rc = cfg_get_bool(&net->is_virtual, "%s/virtual:", net_oid);
    if (rc != 0)
    {
        ERROR("Failed to get the virtual instance for network '%s'", net_oid);
        free(net->name);
        return rc;
    }

    rc = cfg_find_pattern_fmt(&n_gateways, &gateway_handles,
                              "%s/gateway:*", net_oid);
    if (rc != 0)
    {
        ERROR("Failed to get the gateways for network %s: %r", net_oid, rc);
        free(net->name);
        return rc;
    }

    te_kvpair_init(&net->gateways);
    for (i = 0; i < n_gateways; i++)
    {
        cfg_val_type  cvt = CVT_STRING;
        char         *target_network = NULL;
        char         *gateway_node = NULL;

        rc = cfg_get_inst_name(gateway_handles[i], &target_network);
        if (rc != 0)
        {
            ERROR("Failed to get target network of one of the gateways of network %s: %r",
                  net_oid, rc);
            free(net->name);
            te_kvpair_fini(&net->gateways);
            return rc;
        }

        rc = cfg_get_instance(gateway_handles[i], &cvt, &gateway_node);
        if (rc != 0)
        {
            ERROR("Failed to read gateway node of one of the gateways of network %s: %r",
                  net_oid, rc);
            free(net->name);
            te_kvpair_fini(&net->gateways);
            return rc;
        }

        rc = te_kvpair_add(&net->gateways, target_network, "%s", gateway_node);
        free(target_network);
        free(gateway_node);
        if (rc != 0)
        {
            ERROR("Failed to add gateway node of network %s: %r", net_oid, rc);
            free(net->name);
            te_kvpair_fini(&net->gateways);
            return rc;
        }
    }

    rc = cfg_get_bool(&net->nat, "%s/nat:", net_oid);
    if (rc != 0)
    {
        ERROR("Failed to get the nat instance for network '%s'", net_oid);
        free(net->name);
        return rc;
    }

    if (net->nat && net->is_virtual)
    {
        ERROR("Only non-virtual networks may be behind NAT", net_oid);
        free(net->name);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    rc = cfg_get_string(&nat_setup, "%s/nat:/setup:", net_oid);
    if (rc != 0 && TE_RC_GET_ERROR(rc) != TE_ENOENT)
    {
        ERROR("Failed to get the nat setup instance for network '%s'", net_oid);
        free(nat_setup);
        free(net->name);
        return rc;
    }
    if (TE_RC_GET_ERROR(rc) == TE_ENOENT ||
        strcmp(nat_setup, "none") == 0 ||
        strcmp(nat_setup, "") == 0)
        net->nat_setup = NET_NAT_SETUP_NONE;
    else if (strcmp(nat_setup, "iptables") == 0)
        net->nat_setup = NET_NAT_SETUP_IPTABLES;
    free(nat_setup);

    /* Find all nodes in this net */
    rc = cfg_find_pattern_fmt(&n_nodes, &node_handles,
                              "%s/node:*", net_oid);
    free(net_oid);
    if (rc != 0)
    {
        ERROR("cfg_find_pattern() failed %r", rc);
        free(net->name);
        te_kvpair_fini(&net->gateways);
        net->name = NULL;
        return rc;
    }

    net->n_nodes = n_nodes;
    if (n_nodes == 0)
    {
        net->nodes = NULL;
    }
    else
    {
        net->nodes = (cfg_net_node_t *)calloc(net->n_nodes,
                                              sizeof(*(net->nodes)));
        if (net->nodes == NULL)
        {
            ERROR("Memory allocation failure");
            free(net->name);
            te_kvpair_fini(&net->gateways);
            net->name = NULL;
            return rc;
        }
    }

    for (i = 0; i < net->n_nodes; ++i)
    {
        char *node_oid;
        int   val;

        /* Save cfg handle of the net node */
        net->nodes[i].handle = node_handles[i];

        /* Get net node OID as string */
        rc = cfg_get_oid_str(node_handles[i], &node_oid);
        if (rc != 0)
        {
            ERROR("cfg_get_oid_str() failed %r", rc);
            break;
        }

        /* Get node type */
        rc = cfg_get_instance_int_fmt(&val, "%s/type:", node_oid);
        free(node_oid);
        if (rc != 0)
        {
            ERROR("cfg_get_instance_fmt() failed %r", rc);
            break;
        }
        net->nodes[i].type = val;
    }
    free(node_handles);

    if (rc != 0)
        tapi_cfg_net_free_net(net);

    return rc;
}

/* See description in tapi_cfg_net.h */
te_errno
tapi_cfg_net_get_nets(cfg_nets_t *nets)
{
    te_errno        rc;
    unsigned int    n_nets;
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
        rc = tapi_cfg_net_get_net(net_handles[i], nets->nets + i);
        if (rc != 0)
        {
            ERROR("tapi_cfg_net_get_net() failed %r", rc);
            break;
        }
    }
    free(net_handles);

    if (rc != 0)
        tapi_cfg_net_free_nets(nets);

    return rc;
}

/* See description in tapi_cfg_net.h */
void
tapi_cfg_net_free_net(cfg_net_t *net)
{
    if (net != NULL)
    {
        free(net->name);
        free(net->nodes);
    }
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
            tapi_cfg_net_free_net(nets->nets + i);
        }
        free(nets->nets);
    }
}

/* See description in tapi_cfg_net.h */
te_errno
tapi_cfg_net_register_net(const char *name, cfg_net_t *net, ...)
{
    cfg_net_node_t *node;
    const char     *node_val;
    int             node_num = 1;
    te_errno        rc;
    va_list         ap;

    rc = cfg_add_instance_fmt(&net->handle, CFG_VAL(NONE, 0),
                              "/net:%s", name);
    if (rc != 0)
        return rc;

    net->nodes = NULL;
    net->n_nodes = 0;

    va_start(ap, net);

    while ((node_val = va_arg(ap, const char *)) != NULL)
    {
        net->nodes = realloc(net->nodes, sizeof(*net->nodes) * node_num);
        node = &net->nodes[node_num - 1];

        node->type = va_arg(ap, enum net_node_type);

        rc = cfg_add_instance_fmt(&node->handle, CFG_VAL(STRING, node_val),
                                  "/net:%s/node:%d", name, node_num);
        if (rc != 0)
        {
            ERROR("Failed to add node!");
            break;
        }
        rc = cfg_set_instance_fmt(CFG_VAL(INT32, node->type),
                                  "/net:%s/node:%d/type:", name, node_num);
        if (rc != 0)
            break;

        node_num++;
    }

    if (rc == 0)
        net->n_nodes = (node_num - 1);

    va_end(ap);

    return rc;
}

/* See description in tapi_cfg_net.h */
te_errno
tapi_cfg_net_unregister_net(const char *name, cfg_net_t *net)
{
    free(net->nodes);
    return cfg_del_instance_fmt(true, "/net:%s", name);
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
    te_errno        rc;
    unsigned int    net_num;
    cfg_handle     *node_handles = NULL;
    cfg_handle     *net_handles = NULL;
    char           *net_name = NULL;
    unsigned int    i;


    /* Get handles of all nodes in all networks */
    rc = cfg_find_pattern("/net:*", &net_num, &net_handles);
    if (rc != 0)
    {
        ERROR("Failed(%x) to find all nodes", rc);
        return rc;
    }

    for (i = 0, rc = TE_ESRCH; i < net_num; ++i, rc = TE_ESRCH)
    {
        unsigned int    node_num;
        unsigned int    j;

        free(node_handles);
        free(net_name);

        node_handles = NULL;
        net_name = NULL;

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
                goto exit;
            }

            res = strcmp(oid, val);
            free(val);
            if (res == 0)
            {
                strcpy(net, net_name);
                goto exit;
            }
        }
    }

exit:
    free(node_handles);
    free(net_handles);
    free(net_name);

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
    char  *oid_name = NULL;

#define THROW_EXCEPTION(err_msg...) \
    do {                \
        ERROR(err_msg);   \
        goto clean_all; \
    } while (0)

/**
 * Deallocates memory specified in "ptr_" parameter by using "func_" function
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

        oid_name = cfg_convert_oid(oid);
        if (oid_name == NULL)
            THROW_EXCEPTION("Not enough memory to get name of OID.");

        VERB("Net/Node: %s", oid_name);

        rc = cfg_get_instance_int_fmt(&cfg_node_type, "%s/type:", oid_name);
        if (rc != 0)
            THROW_EXCEPTION("Error while getting type of node %s", oid_name);

        VERB("Node type: %d (expected %d)", cfg_node_type, node_type);

        if (cfg_node_type != node_type)
        {
            SAFE_FREE(oid_name, free);
            SAFE_FREE(oid, cfg_free_oid);
            continue;
        }

        VERB("Node %s has expected type", oid_name);

        /*
         * Processing of the node of specified type:
         * Check if node value has format we are expecting and that
         * agent name the same as specified.
         */

        val_type = CVT_STRING;
        if ((rc = cfg_get_instance(handles[i], &val_type, &val)) != 0)
            THROW_EXCEPTION("Error while getting value of %s", oid_name);

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
        SAFE_FREE(oid_name, free);
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
    free(oid_name);
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
    int             type;
    cfg_oid        *ta_node_oid;
    char          **oids = NULL;
    cfg_oid        *nut_oid = NULL;
    char           *port_name;
    char           *end;
    long int        port;

    rc = cfg_get_instance_int_fmt(&type, "%s/type:", ta_node);
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
        ERROR("Invalid port number '%s' format", port_name);
        cfg_free_oid(nut_oid);
        return TE_EFMT;
    }
    cfg_free_oid(nut_oid);
    *p_port = port;

    return 0;
}

typedef struct {
    char         **rsrc_names;
    char         **rsrc_vals;
    unsigned int   nb_rsrcs;
} net_node_rsrc_desc_t;

static net_node_rsrc_desc_t *
tapi_cfg_net_node_rsrc_desc_alloc(unsigned int nb_rsrcs)
{
    net_node_rsrc_desc_t *d = NULL;

    d = TE_ALLOC(sizeof(*d));

    d->rsrc_names = TE_ALLOC(nb_rsrcs * sizeof(*(d->rsrc_names)));

    d->rsrc_vals = TE_ALLOC(nb_rsrcs * sizeof(*(d->rsrc_vals)));

    d->nb_rsrcs = nb_rsrcs;

    return d;
}

static void
tapi_cfg_net_node_rsrc_desc_free(net_node_rsrc_desc_t *d)
{
    unsigned int i;

    for (i = 0; i < d->nb_rsrcs; ++i)
    {
        free(d->rsrc_vals[i]);
        free(d->rsrc_names[i]);
    }

    free(d->rsrc_vals);
    free(d->rsrc_names);
    free(d);
}

static te_errno
tapi_cfg_net_mk_node_rsrc_desc_iface(const cfg_oid         *oid,
                                     net_node_rsrc_desc_t **rsrc_descp)
{
    net_node_rsrc_desc_t *d = tapi_cfg_net_node_rsrc_desc_alloc(1);

    if (d == NULL)
        return TE_ENOMEM;

    d->rsrc_vals[0] = cfg_convert_oid(oid);
    if (d->rsrc_vals[0] == NULL)
    {
        tapi_cfg_net_node_rsrc_desc_free(d);
        return TE_ENOMEM;
    }

    /*
     * Make it it makes sense to add 'if:' prefix, but keep just
     * interface name which is used before.
     */
    d->rsrc_names[0] = strdup(CFG_OID_GET_INST_NAME(oid, 2));
    if (d->rsrc_names[0] == NULL)
    {
        tapi_cfg_net_node_rsrc_desc_free(d);
        return TE_ENOMEM;
    }

    *rsrc_descp = d;

    return 0;
}

static te_errno
tapi_cfg_net_mk_node_rsrc_desc_pci_fn(const cfg_oid         *oid,
                                      enum net_node_rsrc_type rsrc_type,
                                      net_node_rsrc_desc_t **rsrc_descp)
{
    net_node_rsrc_desc_t *d = tapi_cfg_net_node_rsrc_desc_alloc(1);

    if (d == NULL)
        return TE_ENOMEM;

    d->rsrc_vals[0] = cfg_convert_oid(oid);
    if (d->rsrc_vals[0] == NULL)
    {
        tapi_cfg_net_node_rsrc_desc_free(d);
        return TE_ENOMEM;
    }

    switch (rsrc_type)
    {
        case NET_NODE_RSRC_TYPE_PCI_FN:
            d->rsrc_names[0] = tapi_cfg_pci_rsrc_name(oid);
            break;

        case NET_NODE_RSRC_TYPE_PCI_FN_NETDEV:
            d->rsrc_names[0] = tapi_cfg_pci_fn_netdev_rsrc_name(oid);
            break;

        default:
            ERROR("%s() unexpected rsrc_type value: %d", __func__, rsrc_type);
            tapi_cfg_net_node_rsrc_desc_free(d);
            return TE_EINVAL;
    }

    if (d->rsrc_names[0] == NULL)
    {
        tapi_cfg_net_node_rsrc_desc_free(d);
        return TE_ENOMEM;
    }

    *rsrc_descp = d;

    return 0;
}

static te_errno
tapi_cfg_net_mk_node_rsrc_desc_rte_vdev(const cfg_oid         *oid,
                                        net_node_rsrc_desc_t **rsrc_descp)
{
    char                 *oid_str = NULL;
    unsigned int          nb_slaves = 0;
    cfg_handle           *slave_handles = NULL;
    net_node_rsrc_desc_t *d = NULL;
    te_errno              rc = 0;
    unsigned int          i;

    oid_str = cfg_convert_oid(oid);
    if (oid_str == NULL)
        return TE_ENOMEM;

    rc = cfg_find_pattern_fmt(&nb_slaves, &slave_handles,
                              "%s/slave:*", oid_str);
    free(oid_str);
    if (rc != 0)
        return rc;

    d = tapi_cfg_net_node_rsrc_desc_alloc(nb_slaves);
    if (d == NULL)
    {
        free(slave_handles);
        return TE_ENOMEM;
    }

    for (i = 0; i < nb_slaves; ++i)
    {
        cfg_val_type  val_type = CVT_STRING;
        cfg_oid      *pci_oid = NULL;

        rc = cfg_get_instance(slave_handles[i], &val_type, &d->rsrc_vals[i]);
        if (rc != 0)
            goto out;

        pci_oid = cfg_convert_oid_str(d->rsrc_vals[i]);
        if (pci_oid == NULL) {
            rc = TE_ENOMEM;
            goto out;
        }

        /*
         * Frankly, RTE vdev slave device is not necessarily a PCI function.
         * However, considering other possible options is hardly useful for
         * the most applications of this code.
         */
        d->rsrc_names[i] = tapi_cfg_pci_rsrc_name(pci_oid);
        cfg_free_oid(pci_oid);
        if (d->rsrc_names[i] == NULL)
        {
            rc = TE_ENOMEM;
            goto out;
        }
    }

    *rsrc_descp = d;

out:
    if (rc != 0)
        tapi_cfg_net_node_rsrc_desc_free(d);

    free(slave_handles);

    return rc;
}

/**
 * Derive indirect values of the given node and generate unique resource
 * names for them. Almost all resource types imply that only direct node
 * value is passed to the caller and only one resource name is generated.
 *
 * @param rsrc_type      Resource type
 * @param oid            OID provided by the primary node value
 * @param rsrc_descp     Location for resulting resource descriptor
 *
 * @return Status code.
 */
static te_errno
tapi_cfg_net_mk_node_rsrc_names_vals(enum net_node_rsrc_type    rsrc_type,
                                     const cfg_oid             *oid,
                                     net_node_rsrc_desc_t     **rsrc_descp)
{
    te_errno rc = 0;

    switch (rsrc_type)
    {
        case NET_NODE_RSRC_TYPE_INTERFACE:
            rc = tapi_cfg_net_mk_node_rsrc_desc_iface(oid, rsrc_descp);
            break;

        case NET_NODE_RSRC_TYPE_PCI_FN:
        case NET_NODE_RSRC_TYPE_PCI_FN_NETDEV:
            rc = tapi_cfg_net_mk_node_rsrc_desc_pci_fn(oid, rsrc_type, rsrc_descp);
            break;

        case NET_NODE_RSRC_TYPE_RTE_VDEV:
            rc = tapi_cfg_net_mk_node_rsrc_desc_rte_vdev(oid, rsrc_descp);
            break;

        case NET_NODE_RSRC_TYPE_UNKNOWN:
        default:
            rc = TE_EINVAL;
    }

    return rc;
}

/* See description in tapi_cfg_net.h */
te_errno
tapi_cfg_net_foreach_node(tapi_cfg_net_node_cb *cb, void *cookie)
{
    int             rc;
    cfg_nets_t      nets;
    unsigned int    i, j;

    if (cb == NULL)
        return TE_RC(TE_TAPI, TE_EINVAL);

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
            char           *oid_str;
            cfg_oid        *oid;

            type = CVT_STRING;
            rc = cfg_get_instance(net->nodes[j].handle, &type, &oid_str);
            if (rc != 0)
            {
                ERROR("Failed to get Configurator instance by handle "
                      "0x%x: %r", net->nodes[j].handle, rc);
                break;
            }
            oid = cfg_convert_oid_str(oid_str);
            if (oid == NULL)
            {
                ERROR("Failed to convert OID '%s' to structure", oid_str);
                free(oid_str);
                rc = TE_RC(TE_TAPI, TE_EINVAL);
                break;
            }

            rc = cb(net, &net->nodes[j], oid_str, oid, cookie);

            cfg_free_oid(oid);
            free(oid_str);

            if (rc != 0)
                break;
        }
        if (rc != 0)
            break;
    }

    tapi_cfg_net_free_nets(&nets);

    return rc;
}

static te_errno
tapi_cfg_net_pci_fn_by_dpdk_vdev_ref(const char *vdev_str, size_t *n_pci_fns,
                                     char ***pci_fns)
{
    unsigned int n_vdev_slaves;
    cfg_handle *vdev_slaves;
    char **result = NULL;
    unsigned int i;
    te_errno rc = 0;

    rc = cfg_find_pattern_fmt(&n_vdev_slaves, &vdev_slaves, "%s/slave:*",
                              vdev_str);
    if (rc != 0)
    {
        ERROR("Failed to get DPDK vdev slaves by DPDK vdev reference");
        goto out;
    }

    result = TE_ALLOC(sizeof(*result) * n_vdev_slaves);

    for (i = 0; i < n_vdev_slaves; i++)
    {
        cfg_oid *vdev_slave_oid;
        char *vdev_slave_str;
        char *pci_inst_name;

        rc = cfg_get_oid(vdev_slaves[i], &vdev_slave_oid);
        if (rc != 0)
        {
            ERROR("Failed to get DPDK vdev slave oid by handle");
            goto out;
        }

        vdev_slave_str = cfg_convert_oid(vdev_slave_oid);
        cfg_free_oid(vdev_slave_oid);
        if (vdev_slave_str == NULL)
        {
            ERROR("Failed to get DPDK vdev name by oid");
            rc = TE_RC(TE_CONF_API, TE_ENOMEM);
            goto out;
        }

        rc = cfg_get_instance_str(NULL, &pci_inst_name, vdev_slave_str);
        free(vdev_slave_str);
        if (rc != 0)
        {
            ERROR("Failed to get PCI instance name by DPDK vdev slave");
            goto out;
        }

        rc = cfg_get_instance_str(NULL, &result[i], pci_inst_name);
        free(pci_inst_name);
        if (rc != 0)
        {
            ERROR("Failed to get PCI function by PCI instance");
            goto out;
        }
    }

    *pci_fns = result;
    *n_pci_fns = n_vdev_slaves;

out:
    if (rc != 0)
    {
        if (result != NULL)
        {
            for (i = 0; i < n_vdev_slaves; i++)
                free(result[i]);
        }
        free(result);
    }

    return rc;
}

static te_errno
tapi_cfg_net_bind_driver_on_pci_fn(const char *ta,
                                   enum tapi_cfg_driver_type driver,
                                   size_t n_pci_fns, char **pci_fns)
{
    char *driver_name = NULL;
    te_errno rc = 0;
    size_t i;

    rc = tapi_cfg_pci_get_ta_driver(ta, driver, &driver_name);
    if (rc != 0)
        goto out;

    if (driver_name == NULL)
    {
        WARN("Driver is not set on agent %s, do not perform bind", ta);
        rc = 0;
        goto out;
    }

    for (i = 0; i < n_pci_fns; i++)
    {
        char *driver_old = NULL;
        int cmp_rc;

        rc = tapi_cfg_pci_get_driver(pci_fns[i], &driver_old);
        if (rc != 0)
            goto out;

        cmp_rc = strcmp(driver_old, driver_name);
        free(driver_old);
        if (cmp_rc != 0)
        {
            rc = tapi_cfg_pci_bind_driver(pci_fns[i], driver_name);
            if (rc != 0)
                goto out;
        }
    }

out:
    free(driver_name);

    return rc;
}

struct bind_cfg_t {
    enum net_node_type node_type;
    enum tapi_cfg_driver_type driver;
};

static char *
make_pci_fn_oid_str_by_pci_fn_netdev_oid(cfg_oid *oid)
{
    unsigned int i;
    te_string str_inst_oid = TE_STRING_INIT;

    /*
     * Build an inst_oid for pci instance without the last ('netdev') item.
     */
    for (i = 1; i < oid->len - 1; i++)
    {
        te_string_append(&str_inst_oid, "/%s:%s",
                         cfg_oid_inst_subid(oid, i),
                         CFG_OID_GET_INST_NAME(oid, i));
    }
    return str_inst_oid.ptr;
}

static tapi_cfg_net_node_cb tapi_cfg_net_node_bind_driver;
static te_errno
tapi_cfg_net_node_bind_driver(cfg_net_t *net, cfg_net_node_t *node,
                              const char *oid_str, cfg_oid *oid, void *cookie)
{
    struct bind_cfg_t *cfg;
    char **pci_fns = NULL;
    size_t n_pci_fns;
    const char *agent;
    te_errno rc = 0;
    size_t i;

    if (net->is_virtual)
        return 0;

    if (cookie == NULL)
    {
        ERROR("Invalid cookie passed to bind driver");
        rc = TE_RC(TE_CONF_API, TE_EINVAL);
        goto out;
    }
    cfg = (struct bind_cfg_t *)cookie;

    if (cfg->node_type != node->type)
        goto out;

    if (strcmp(cfg_oid_inst_subid(oid, 1), "agent") == 0)
    {
        if (strcmp(cfg_oid_inst_subid(oid, 2), "interface") == 0)
        {
            if (cfg->driver == NET_DRIVER_TYPE_NET)
            {
                WARN("Net node is linked to a net interface, do not bind net driver");
                goto out;
            }
            else
            {
                ERROR("Cannot bind non 'kernel net driver' "
                      "for a net node linked to a net interface");
                rc = TE_RC(TE_CONF_API, TE_EINVAL);
                goto out;
            }
        }
        else if (strcmp(cfg_oid_inst_subid(oid, 2), "hardware") == 0)
        {
            char obj_oid[CFG_OID_MAX];

            n_pci_fns = 1;
            pci_fns = TE_ALLOC(sizeof(*pci_fns));

            cfg_oid_inst2obj(oid_str, obj_oid);
            if (strcmp(obj_oid, TAPI_CFG_NET_OID_NETDEV) == 0)
            {
                char *pci_fn_oid_str;

                pci_fn_oid_str = make_pci_fn_oid_str_by_pci_fn_netdev_oid(oid);
                rc = cfg_get_instance_str(NULL, &pci_fns[0], pci_fn_oid_str);
                free(pci_fn_oid_str);
            }
            else
            {
                rc = cfg_get_instance_str(NULL, &pci_fns[0], oid_str);
            }
            if (rc != 0)
            {
                ERROR("Failed to get PCI device path of an agent");
                goto out;
            }
        }
        else
        {
            ERROR("Invalid agent reference in a network node");
            rc = TE_RC(TE_CONF_API, TE_EINVAL);
            goto out;
        }
    }
    else if (strcmp(cfg_oid_inst_subid(oid, 1), "local") == 0)
    {
        rc = tapi_cfg_net_pci_fn_by_dpdk_vdev_ref(oid_str, &n_pci_fns,
                                                  &pci_fns);
        if (rc != 0)
            goto out;
    }
    else
    {
        ERROR("Net node is linked to neither a test agent nor DPDK vdev");
        rc = TE_RC(TE_CONF_API, TE_EINVAL);
        goto out;
    }

    agent = CFG_OID_GET_INST_NAME(oid, 1);

    rc = tapi_cfg_net_bind_driver_on_pci_fn(agent, cfg->driver,
                                            n_pci_fns, pci_fns);
    if (rc != 0)
        goto out;

out:
    if (pci_fns != NULL)
    {
        for (i = 0; i < n_pci_fns; i++)
            free(pci_fns[i]);
    }
    free(pci_fns);

    return rc;
}

te_errno
tapi_cfg_net_bind_driver_by_node(enum net_node_type node_type,
                                 enum tapi_cfg_driver_type driver)
{
    struct bind_cfg_t cfg = {
        .node_type = node_type,
        .driver = driver
    };

    return tapi_cfg_net_foreach_node(tapi_cfg_net_node_bind_driver, &cfg);
}

te_errno
tapi_cfg_net_node_interface(const char *node_value, char **iface_name)
{
    char node_value_obj[CFG_OID_MAX];
    bool with_netdev = false;
    cfg_oid *node_value_oid = NULL;
    char *pci_fn_oid_str = NULL;
    te_errno rc = 0;

    node_value_oid = cfg_convert_oid_str(node_value);
    if (node_value_oid == NULL)
    {
        ERROR("Cannot parse '%s' as OID", node_value);
        rc = TE_EINVAL;
        goto out;
    }

    cfg_oid_inst2obj(node_value, node_value_obj);
    if (strcmp(node_value_obj, TAPI_CFG_NET_OID_NETDEV) == 0)
    {
        char *pci_fn_node_value;

        with_netdev = true;
        pci_fn_node_value =
            make_pci_fn_oid_str_by_pci_fn_netdev_oid(node_value_oid);
        rc = cfg_get_instance_str(NULL, &pci_fn_oid_str, pci_fn_node_value);
        free(pci_fn_node_value);
    }
    else if (strcmp(node_value_obj, TAPI_CFG_NET_OID_PCI_INSTANCE) == 0)
    {
        rc = cfg_get_instance_str(NULL, &pci_fn_oid_str, node_value);
    }
    else if (strcmp(node_value_obj, TAPI_CFG_NET_OID_INTERFACE) == 0)
    {
        *iface_name = cfg_oid_get_inst_name(node_value_oid,
                                            node_value_oid->len - 1);
    }
    else
    {
        ERROR("Unsupported resource: %s", node_value);
        rc = TE_EINVAL;
        goto out;
    }

    if (rc != 0)
    {
        ERROR("Failed to get PCI device path: %r", rc);
        goto out;
    }

    if (with_netdev)
    {
        rc = tapi_cfg_pci_fn_netdev_get_net_if(pci_fn_oid_str,
                CFG_OID_GET_INST_NAME(node_value_oid, node_value_oid->len - 1),
                iface_name);
    }
    else
    {
        rc = tapi_cfg_pci_get_net_if(pci_fn_oid_str, iface_name);
    }

out:
    free(pci_fn_oid_str);
    free(node_value_oid);

    return TE_RC(TE_TAPI, rc);
}

/**
 * Callback to switch network node specified using PCI function to
 * network interface.
 */
static tapi_cfg_net_node_cb switch_agent_pci_fn_to_interface;
static te_errno
switch_agent_pci_fn_to_interface(cfg_net_t *net, cfg_net_node_t *node,
                                 const char *oid_str, cfg_oid *oid,
                                 void *cookie)
{
    char interface_path[RCF_MAX_PATH];
    enum net_node_type *type = cookie;
    char *interface = NULL;
    const char *agent;
    te_errno rc = 0;

    UNUSED(net);

    if (*type != NET_NODE_TYPE_INVALID && node->type != *type)
        goto out;

    if (strcmp(cfg_oid_inst_subid(oid, 1), "agent") != 0 ||
        strcmp(cfg_oid_inst_subid(oid, 2), "hardware") != 0)
    {
        INFO("Network node '%s' is not a PCI function", oid_str);
        goto out;
    }

    rc = tapi_cfg_net_node_interface(oid_str, &interface);
    if (rc != 0)
    {
        if (TE_RC_GET_ERROR(rc) == TE_ENOENT)
            ERROR("No network interfaces found by '%s'", oid_str);
        goto out;
    }

    agent = CFG_OID_GET_INST_NAME(oid, 1);
    rc = tapi_cfg_base_if_add_rsrc(agent, interface);
    if (rc != 0)
    {
        ERROR("Failed to reserve network interface '%s' resource on TA '%s': %r",
              interface, agent, rc);
        goto out;
    }

    rc = te_snprintf(interface_path, sizeof(interface_path),
                     "/agent:%s/interface:%s", agent, interface);
    if (rc != 0)
    {
        ERROR("Failed to make interface OID in provided buffer: %r", rc);
        goto out;
    }

    rc = cfg_set_instance(node->handle, CFG_VAL(STRING, interface_path));
    if (rc != 0)
        ERROR("Failed to assign network node to interface");

out:
    free(interface);

    return TE_RC(TE_TAPI, rc);
}

/* See the description in tapi_cfg_net.h */
te_errno
tapi_cfg_net_nodes_update_pci_fn_to_interface(enum net_node_type type)
{
    te_errno rc;

    rc = tapi_cfg_net_foreach_node(switch_agent_pci_fn_to_interface, &type);
    if (rc != 0)
    {
        ERROR("Failed to configure interfaces mentioned in networks "
              "configuration: %r", rc);
    }

    return rc;
}

/* See the description in tapi_cfg_net.h */
te_errno
tapi_cfg_net_nodes_switch_pci_fn_to_interface(enum net_node_type type)
{
    te_errno rc;
    enum net_node_type types[] = {NET_NODE_TYPE_AGENT, NET_NODE_TYPE_NUT};
    unsigned int n_types = TE_ARRAY_LEN(types);
    unsigned int i;

    if (type != NET_NODE_TYPE_INVALID)
    {
        n_types = 1;
        types[0] = type;
    }

    for (i = 0; i < n_types; i++)
    {
        rc = tapi_cfg_net_bind_driver_by_node(types[i], NET_DRIVER_TYPE_NET);
        if (rc != 0)
        {
            ERROR("Failed to bind net driver on network nodes: %r", rc);
            return rc;
        }
    }

    /*
     * If a net driver was rebound, synchronize configuration tree to discover
     * network interfaces that are associated with that driver.
     */
    rc = cfg_synchronize("/:", true);
    if (rc != 0)
    {
        ERROR("Configurator synchronize failed after interfaces bind: %r", rc);
        return rc;
    }

    return tapi_cfg_net_nodes_update_pci_fn_to_interface(type);
}

static te_errno
tapi_cfg_net_node_get_pci_info(cfg_net_t *net, cfg_net_node_t *node,
                               const char *oid_str, cfg_oid *oid, void *cookie)
{
    cfg_net_pci_info_t *pci_info;
    char **pci_oids;
    unsigned int n_pci = 0;
    unsigned int i;
    te_errno rc;
    const char *agent;
    char *domain = NULL;
    char *bus = NULL;
    char *slot = NULL;
    int fn;

    UNUSED(net);
    UNUSED(oid_str);

    if (cookie == NULL)
    {
        ERROR("Invalid cookie passed to get node PCI info");
        return TE_RC(TE_CONF_API, TE_EINVAL);
    }
    pci_info = (cfg_net_pci_info_t *)cookie;

    if (pci_info->node_type != node->type)
        return 0;

    tapi_cfg_net_get_node_rsrc_type(node);
    rc = tapi_cfg_net_node_get_pci_oids(node, &n_pci, &pci_oids);
    if (rc != 0)
        goto fail_get_pci_oids;

    rc = cfg_get_instance_string_fmt(&domain, "%s/domain:", *pci_oids);
    if (rc != 0)
        goto fail_get_domain;

    rc = cfg_get_instance_string_fmt(&bus, "%s/bus:", *pci_oids);
    if (rc != 0)
        goto fail_get_bus;

    rc = cfg_get_instance_string_fmt(&slot, "%s/slot:", *pci_oids);
    if (rc != 0)
        goto fail_get_slot;

    rc = cfg_get_instance_int_fmt(&fn, "%s/fn:", *pci_oids);
    if (rc != 0)
        goto fail_get_fn;

    pci_info->pci_addr = TE_ALLOC(16);

    snprintf(pci_info->pci_addr, 16, "%s:%s:%s.%d", domain, bus, slot, fn);

    rc = cfg_get_string(&pci_info->bound_driver, "%s/driver:", *pci_oids);
    if (rc != 0)
        goto fail_get_driver;

    agent = CFG_OID_GET_INST_NAME(oid, 1);
    rc = tapi_cfg_pci_get_ta_driver(agent, NET_DRIVER_TYPE_NET,
                                    &pci_info->net_driver);
    if (rc != 0)
        goto fail_get_net_driver;

    rc = tapi_cfg_pci_get_ta_driver(agent, NET_DRIVER_TYPE_DPDK,
                                    &pci_info->dpdk_driver);
    if (rc != 0)
        goto fail_get_dpdk_driver;

    return 0;

fail_get_dpdk_driver:
fail_get_net_driver:
fail_get_driver:
    tapi_cfg_net_free_pci_info(pci_info);
fail_get_fn:
    free(slot);
fail_get_slot:
    free(bus);
fail_get_bus:
    free(domain);
fail_get_domain:
    for (i = 0; i < n_pci; i++)
        free(pci_oids[i]);
fail_get_pci_oids:
    return rc;
}

te_errno
tapi_cfg_net_get_iut_if_pci_info(cfg_net_pci_info_t *iut_if_pci_info)
{
    tapi_cfg_net_init_pci_info(iut_if_pci_info);
    iut_if_pci_info->node_type = NET_NODE_TYPE_NUT;
    return tapi_cfg_net_foreach_node(tapi_cfg_net_node_get_pci_info,
                                     iut_if_pci_info);
}

static tapi_cfg_net_node_cb tapi_cfg_net_node_reserve;

static te_errno
tapi_cfg_net_node_rsrc_reserve(const char *oid_str,
                               cfg_oid    *oid,
                               const char *rsrc_name)
{
    te_errno rc = 0;

    /* Check if resource is already reserved, reserve it if not. */
    rc = cfg_get_instance_fmt(NULL, NULL, "/agent:%s/rsrc:%s",
                              CFG_OID_GET_INST_NAME(oid, 1), rsrc_name);
    if (rc != 0)
    {
        rc = cfg_add_instance_fmt(NULL, CFG_VAL(STRING, oid_str),
                                  "/agent:%s/rsrc:%s",
                                  CFG_OID_GET_INST_NAME(oid, 1), rsrc_name);
        if (rc != 0)
            ERROR("Failed to reserve resource '%s': %r", oid_str, rc);
        else if (tapi_host_ns_enabled())
            rc = tapi_host_ns_if_add(CFG_OID_GET_INST_NAME(oid, 1),
                                     CFG_OID_GET_INST_NAME(oid, 2), NULL);
   }

   return rc;
}

static te_errno
tapi_cfg_net_node_reserve(cfg_net_t *net, cfg_net_node_t *node,
                          const char *oid_str, cfg_oid *oid, void *cookie)
{
    net_node_rsrc_desc_t *rsrc_desc = NULL;
    char                  oid_object_str[CFG_OID_MAX];
    int                   rc = 0;
    unsigned int          i;

    UNUSED(net);
    UNUSED(cookie);

    cfg_oid_inst2obj(oid_str, oid_object_str);

    /*
     * We should reserve resource only for OIDs that point to
     * "agent" subtree. Apart from "agent" we may have some
     * user-designed nodes, like "nut" subtree.
     */
    if (strcmp((((cfg_inst_subid *)(oid->ids))[1].subid), "agent") != 0 &&
        strcmp(oid_object_str, "/local/dpdk/vdev") != 0)
        return 0;

    rc = tapi_cfg_net_mk_node_rsrc_names_vals(
                    tapi_cfg_net_get_node_rsrc_type(node),
                    oid, &rsrc_desc);
    if (rc == 0)
    {
        for (i = 0; i < rsrc_desc->nb_rsrcs; ++i)
        {
            rc = tapi_cfg_net_node_rsrc_reserve(rsrc_desc->rsrc_vals[i], oid,
                                                rsrc_desc->rsrc_names[i]);
            if (rc != 0)
                break;
        }

        tapi_cfg_net_node_rsrc_desc_free(rsrc_desc);
    }

    return rc;
}

/* See description in tapi_cfg_net.h */
te_errno
tapi_cfg_net_reserve_all(void)
{
    return tapi_cfg_net_foreach_node(tapi_cfg_net_node_reserve, NULL);
}

/* See description in tapi_cfg_net.h */
te_errno
tapi_cfg_net_all_up(bool force)
{
    int             rc;
    cfg_nets_t      nets;
    unsigned int    i, j, k;
    char          **nodes;
    unsigned int    n_nodes;

    /* Get available networks configuration */
    rc = tapi_cfg_net_get_nets(&nets);
    if (rc != 0)
    {
        ERROR("Failed to get networks from Configurator: %r", rc);
        return rc;
    }

    n_nodes = 0;
    for (i = 0; i < nets.n_nets; ++i)
        n_nodes += nets.nets[i].n_nodes;
    nodes = calloc(sizeof(char *), n_nodes);
    if (nodes == NULL)
    {
        ERROR("Out of memory");
        return TE_OS_RC(TE_TAPI, errno);
    }

    /* Get the list of interfaces to be brought up */
    k = 0;
    for (i = 0; i < nets.n_nets; ++i)
    {
        cfg_net_t  *net = nets.nets + i;

        for (j = 0; j < net->n_nodes; ++j, k++)
        {
            cfg_val_type    type;
            char           *oid = NULL;

            if (tapi_cfg_net_get_node_rsrc_type(&net->nodes[j]) !=
                NET_NODE_RSRC_TYPE_INTERFACE)
                continue;

            type = CVT_STRING;
            rc = cfg_get_instance(net->nodes[j].handle, &type, &oid);
            if (rc != 0)
            {
                ERROR("Failed to get Configurator instance by handle "
                      "0x%x: %r", net->nodes[j].handle, rc);
                break;
            }
            nodes[k] = oid;
        }
    }

    /* Check interfaces status and bring them down if "force" */
    for (k = 0; k < n_nodes; k++)
    {
        int status;

        if (nodes[k] == NULL)
            continue; /* Not an interface */

        rc = cfg_get_instance_int_fmt(&status, "%s/status:", nodes[k]);
        if (rc != 0)
        {
            ERROR("Failed to get status of %s: %r", nodes[k], rc);
            goto error;
        }
        if (status == 1)
        {
            if (force)
            {
                rc = cfg_set_instance_fmt(CFG_VAL(INT32, 0),
                                          "%s/status:", nodes[k]);
                if (rc != 0)
                {
                    ERROR("Failed to set status of %s to DOWN: %r",
                          nodes[k], rc);
                    goto error;
                }

            }
            else
            {
                INFO("Node (interface) %s is already UP", nodes[k]);
                free(nodes[k]);
                nodes[k] = NULL;
            }
        }
    }
    if (force)
        CFG_WAIT_CHANGES;

    /* Bring interfaces up */
    for (k = 0; k < n_nodes; k++)
    {
        if (nodes[k] == NULL)
            continue; /* Not an interface or the interface was already up */
        rc = cfg_set_instance_fmt(CFG_VAL(INT32, 1),
                                  "%s/status:", nodes[k]);
        if (rc != 0)
        {
            ERROR("Failed to set status of %s to UP: %r",
                  nodes[k], rc);
            goto error;
        }
        else
        {
            INFO("Node (interface) %s is already UP", nodes[k]);
        }
        free(nodes[k]);
        nodes[k] = NULL;
    }

error:
    if (rc != 0)
    {
        for (k = 0; k < n_nodes; k++)
        {
            if (nodes[k] != NULL)
                free(nodes[k]);
        }
    }
    free(nodes);
    tapi_cfg_net_free_nets(&nets);

    return rc;
}


static tapi_cfg_net_node_cb tapi_cfg_net_node_delete_all_ip_addresses;
static te_errno
tapi_cfg_net_node_delete_all_ip_addresses(cfg_net_t *net, cfg_net_node_t *node,
                                          const char *oid_str, cfg_oid *oid,
                                          void *cookie)
{
    int      rc;
    char     ta_type[RCF_MAX_NAME];
    char    *def_route_if;
    bool ipv6 = false;

    struct sockaddr_storage   dummy_addr;

    UNUSED(net);
    UNUSED(node);

    if (net->is_virtual)
        return 0;

    if (cookie != NULL && *(bool *)cookie)
        ipv6 = true;

    memset(&dummy_addr, 0, sizeof(dummy_addr));
    dummy_addr.ss_family = (ipv6 ? AF_INET6 : AF_INET);

    /* Do not delete addresses from Win32 hosts */
    rc = rcf_ta_name2type(CFG_OID_GET_INST_NAME(oid, 1), ta_type);
    if (rc != 0)
    {
        ERROR("Failed to get type of TA '%s': %r",
              CFG_OID_GET_INST_NAME(oid, 1), rc);
        return rc;
    }
    if (strcmp(ta_type, "win32") == 0 ||
        /* The following types have issues in /agent/route */
        strcmp(ta_type, "freebsd6") == 0 ||
        strcmp(ta_type, "netbsd") == 0)
    {
        return 0;
    }

    if (!ipv6)
    {
        /*
         * Do not delete IPv4 addresses from interfaces used by default
         * route.
         */
        rc = cfg_get_instance_string_fmt(&def_route_if,
                                         "/agent:%s/ip4_rt_default_if:",
                                         CFG_OID_GET_INST_NAME(oid, 1));
        if (TE_RC_GET_ERROR(rc) == TE_ENOENT)
        {
            def_route_if = NULL;
        }
        else if (rc != 0)
        {
            ERROR("Failed to get /agent:%s/ip4_rt_default_if: %r",
                  CFG_OID_GET_INST_NAME(oid, 1), rc);
            return rc;
        }

        if ((def_route_if != NULL) &&
            strcmp(def_route_if, CFG_OID_GET_INST_NAME(oid, 2)) == 0)
        {
            WARN("Do not remove any IPv4 addresses from %s, since "
                 "the interface is used by default route", oid_str);
            free(def_route_if);
            return 0;
        }
        free(def_route_if);
    }

    if (ipv6)
    {
        rc = tapi_cfg_del_if_ip6_addresses(
                  CFG_OID_GET_INST_NAME(oid, 1),
                  CFG_OID_GET_INST_NAME(oid, 2),
                  SA(&dummy_addr));
    }
    else
    {
        rc = tapi_cfg_del_if_ip4_addresses(
                  CFG_OID_GET_INST_NAME(oid, 1),
                  CFG_OID_GET_INST_NAME(oid, 2),
                  SA(&dummy_addr));
    }
    if (rc != 0)
    {
        ERROR("Failed to delete %s addresses from %s: %r",
              (ipv6 ? "IPv6" : "IPv4"), oid_str, rc);
        return rc;
    }

    return 0;
}

/* See description in tapi_cfg_net.h */
te_errno
tapi_cfg_net_delete_all_ip4_addresses(void)
{
    bool ipv6 = false;

    return tapi_cfg_net_foreach_node(
                                tapi_cfg_net_node_delete_all_ip_addresses,
                                &ipv6);
}

/* See description in tapi_cfg_net.h */
te_errno
tapi_cfg_net_delete_all_ip6_addresses(void)
{
    bool ipv6 = true;

    return tapi_cfg_net_foreach_node(
                                tapi_cfg_net_node_delete_all_ip_addresses,
                                &ipv6);
}

/* See description in tapi_cfg_net.h */
int
tapi_cfg_net_assign_ip(unsigned int af, cfg_net_t *net,
                       tapi_cfg_net_assigned *assigned)
{
    int                 rc;
    cfg_val_type        type;
    char               *str;
    unsigned int        i;
    cfg_handle          net_hndl;
    char               *net_oid = NULL;
    unsigned int        net_pfx;
    struct sockaddr    *net_addr = NULL;
    cfg_handle          entry_hndl;
    cfg_handle          addr_hndl;
    struct sockaddr    *addr;

    if (net == NULL)
    {
        ERROR("%s: Net pointer is NULL", __FUNCTION__);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }
    if (af != AF_INET && af != AF_INET6)
    {
        ERROR("%s: Address family %u is not supported yet",
              __FUNCTION__, af);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    /* Allocate or use passed IPv4 subnet */
    if (assigned == NULL ||
        assigned->pool == CFG_HANDLE_INVALID)
    {
        rc = tapi_cfg_alloc_entry(af == AF_INET ? "/net_pool:ip4" :
                                                  "/net_pool:ip6",
                                  &net_hndl);
        if (rc != 0)
        {
            ERROR("%s: Failed to allocate %s net to assign: %r",
                  __FUNCTION__, (af == AF_INET) ? "ip4" : "ip6", rc);
            return rc;
        }
    }
    else
    {
        net_hndl = assigned->pool;
    }

    do { /* Fake loop */

        /*
         * Get all information about this IPv4 subnet
         */
        rc = cfg_get_inst_name_type(net_hndl, CVT_ADDRESS,
                                    CFG_IVP(&net_addr));
        if (rc != 0)
        {
            ERROR("%s(): cfg_get_inst_name_type(0x%jx) failed: %r",
                  __FUNCTION__, (uintmax_t)net_hndl, rc);
            break;
        }

        /* Get OID as string */
        rc = cfg_get_oid_str(net_hndl, &net_oid);
        if (rc != 0)
        {
            ERROR("%s(): cfg_get_oid_str(0x%x) failed: %r",
                  __FUNCTION__, net_hndl, rc);
            break;
        }
        /* Get prefix length */
        type = CVT_INT32;
        rc = cfg_get_instance_fmt(&type, &net_pfx, "%s/prefix:", net_oid);
        if (rc != 0)
        {
            ERROR("Failed to get subnet '%s' prefix: %r", net_oid, rc);
            break;
        }

        /* Add the subnet to the list of subnets of the net */
        rc = cfg_add_instance_child_fmt(NULL, CVT_ADDRESS, net_addr,
                                        net->handle, "/ip%u_subnet:0x%jx",
                                        af == AF_INET ? 4 : 6,
                                        (uintmax_t)net_hndl);
        if (rc != 0)
        {
            ERROR("Failed to add '/ip%u_subnet:0x%jx' child to "
                  "instance with handle 0x%jx: %r",
                  af == AF_INET ? 4 : 6, (uintmax_t)net_hndl,
                  (uintmax_t)net->handle, rc);
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
         * Assign addresses to each node of the net.
         */
        for (i = 0; i < net->n_nodes; ++i)
        {
            rc = tapi_cfg_alloc_net_addr(net_hndl, &entry_hndl, &addr);
            if (rc != 0)
            {
                ERROR("Failed to allocate address for node #%u: %r",
                      i, rc);
                break;
            }

            if (tapi_cfg_net_get_node_rsrc_type(&net->nodes[i]) ==
                NET_NODE_RSRC_TYPE_INTERFACE)
            {
                /* Get interface OID */
                type = CVT_STRING;
                rc = cfg_get_instance(net->nodes[i].handle, &type,
                                      &str);
                if (rc != 0)
                {
                    ERROR("Failed to get Configurator instance by handle "
                          "0x%x: %r", net->nodes[i].handle, rc);
                    free(addr);
                    break;
                }

                rc = tapi_cfg_base_add_net_addr(str, addr, net_pfx, true,
                                                &addr_hndl);
                if (TE_RC_GET_ERROR(rc) == TE_EEXIST)
                {
                    /* Address already assigned - continue */
                    rc = 0;
                }
                else if (rc != 0)
                {
                    free(str);
                    free(addr);
                    break;
                }
                free(str);
            }

            rc = cfg_add_instance_child_fmt(NULL, CVT_ADDRESS, addr,
                                            net->nodes[i].handle,
                                            "/ip%u_address:0x%jx",
                                            af == AF_INET ? 4 : 6,
                                            (uintmax_t)entry_hndl);
            if (rc != 0)
            {
                ERROR("Failed to add 'ip%u_address:0x%jx' child to "
                      "instance with handle 0x%jx: %r",
                      af == AF_INET ? 4 : 6, (uintmax_t)entry_hndl,
                      (uintmax_t)net->nodes[i].handle, rc);
                free(addr);
                break;
            }
            free(addr);

            if (assigned != NULL)
                assigned->entries[i] = entry_hndl;
        }

    } while (0); /* End of fake loop */

    if ((rc == 0) &&
        (assigned != NULL) && (assigned->pool == CFG_HANDLE_INVALID))
    {
        assigned->pool= net_hndl;
    }

    free(net_oid);
    free(net_addr);

    return rc;
}

/* See description in tapi_cfg_net.h */
te_errno
tapi_cfg_net_unassign_ip(unsigned int af, cfg_net_t *net,
                         tapi_cfg_net_assigned *assigned)
{
    cfg_val_type  type;
    cfg_handle    pool_handle;
    cfg_handle    pool_entry_handle;
    char         *if_oid;
    char         *net_addr_str;
    size_t        i;
    te_errno      rc;

    UNUSED(af);

    /*
     * Unassign addresses to each node of the net.
     */
    for (i = 0; i < net->n_nodes; ++i)
    {
        /* instance name is an IP address */
        if ((rc = cfg_get_inst_name(assigned->entries[i],
                                    &net_addr_str)) != 0)
        {
            ERROR("Failed to get network address value");
            return rc;
        }

        /* Get interface OID */
        type = CVT_STRING;
        rc = cfg_get_instance(net->nodes[i].handle, &type, &if_oid);
        if (rc != 0)
        {
            ERROR("Failed to get node instance value, err %r", rc);
            return rc;
        }

        /* Delete network address from TA */
        rc = cfg_del_instance_fmt(false, "%s/net_addr:%s",
                                  if_oid, net_addr_str);
        if (rc != 0)
        {
            ERROR("Failed to delete network address '%s' "
                  "from '%s', err %r",
                  net_addr_str, if_oid, rc);
            return rc;
        }

        if ((rc = cfg_get_father(assigned->entries[i],
                                 &pool_handle)) != 0 ||
            (rc = cfg_get_father(pool_handle,
                                 &pool_entry_handle)) != 0)
        {
            ERROR("Failed to get '/net_pool/entry/' instance handle");
            return rc;
        }

        /* Delete "/net_pool/entry/pool/entry" instance  */
        rc = cfg_del_instance(assigned->entries[i], true);
        if (rc != 0)
        {
            ERROR("Failed to delete ip address instance from net/node");
        }
        else
        {
            char *pool_entry_oid_str;
            int   n_entries;

            if (cfg_get_oid_str(pool_entry_handle,
                                &pool_entry_oid_str) == 0 &&
                cfg_get_instance_int_fmt(&n_entries, "%s/n_entries:",
                                     pool_entry_oid_str) == 0)
            {
                n_entries--;
                rc = cfg_set_instance_fmt(CFG_VAL(INT32, n_entries),
                                          "%s/n_entries:",
                                          pool_entry_oid_str);
                if (rc != 0)
                {
                    ERROR("Failed to update '%s/n_entries:' "
                          "instance value, err %r",
                          pool_entry_oid_str, rc);
                    /* Continue processing anyway */
                }

                if (n_entries == 0)
                {
                    /* Reset pool entry "in use" value */
                    cfg_set_instance(pool_entry_handle,
                                     CFG_VAL(INT32, 0));
                }
            }
        }

        /*
         * "/net/node/ip[4|6]_address" instance is deleted when we
         * unregister a network (with tapi_cfg_net_unregister_net()
         * function), so there is no need to do anything special here
         * for the '/net' subtree.
         */
    }
    return 0;
}

te_errno
tapi_cfg_net_assigned_get_subnet_ip(tapi_cfg_net_assigned *assigned,
                                    struct sockaddr **addr,
                                    unsigned int *prefix_len)
{
    int              rc;
    cfg_val_type     type;
    cfg_handle       net_hndl;
    char            *net_oid = NULL;
    struct sockaddr *net_addr = NULL;

    /* Check if subnet handle is valid */
    if (assigned == NULL ||
        assigned->pool == CFG_HANDLE_INVALID)
    {
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    net_hndl = assigned->pool;
    rc = cfg_get_oid_str(net_hndl, &net_oid);
    if (rc != 0)
    {
        ERROR("Failed to get subnet instance name: %r", rc);
        return rc;
    }
    rc = cfg_get_inst_name_type(net_hndl, CVT_ADDRESS, CFG_IVP(&net_addr));
    if (rc != 0)
    {
        ERROR("Failed to retrieve subnet address: %r", rc);
        free(net_oid);
        return rc;
    }

    /* Get prefix length */
    type = CVT_INT32;
    rc = cfg_get_instance_fmt(&type, prefix_len, "%s/prefix:", net_oid);
    if (rc != 0)
    {
        ERROR("Failed to get subnet '%s' prefix: %r", net_oid, rc);
        free(net_addr);
    }
    else
    {
        *addr = net_addr;
    }
    free(net_oid);

    return rc;
}

/* See description in tapi_cfg_net.h */
te_errno
tapi_cfg_net_all_assign_ip(unsigned int af)
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
        if (nets.nets[i].is_virtual)
            continue;

        rc = tapi_cfg_net_assign_ip(af, nets.nets + i, NULL);
        if (rc != 0)
        {
            ERROR("Failed to assign IPsubnet to net #%u: %r", i, rc);
            break;
        }
    }

    tapi_cfg_net_free_nets(&nets);

    return rc;
}

/* See description in tapi_cfg_net.h */
te_errno
tapi_cfg_net_all_check_mtu(void)
{
    unsigned int    i, j;
    int             net_mtu, mtu;
    int             rc = 0;

    cfg_nets_t      nets;

    RING("Perform MTU compare check for all networks");

    /* Get testing network configuration in C structures */
    rc = tapi_cfg_net_get_nets(&nets);
    if (rc != 0)
    {
        ERROR("tapi_cfg_net_get_nets() failed %r", rc);
        return rc;
    }

    for (i = 0; i < nets.n_nets; ++i)
    {
        cfg_net_t  *net = nets.nets + i;

        net_mtu = 0;

        for (j = 0; j < net->n_nodes; ++j)
        {
            cfg_val_type    type;
            char           *oid = NULL;

            type = CVT_STRING;
            rc = cfg_get_instance(net->nodes[j].handle, &type, &oid);
            if (rc != 0)
            {
                ERROR("Failed to get Configurator instance by handle "
                      "0x%x: %r", net->nodes[j].handle, rc);
                goto cleanup;
            }

            rc = cfg_get_instance_int_fmt(&mtu, "%s/mtu:", oid);
            if (rc != 0)
            {
                ERROR("Failed to get MTU of %s: %r", oid, rc);
                goto cleanup;
            }

            RING("%s/mtu: = %d", oid, mtu);

            /* Store MTU value of the first node in the network */
            if (net_mtu == 0)
            {
                net_mtu = mtu;
                continue;
            }

            /* Fail if MTU value differs */
            if (mtu != net_mtu)
            {
                ERROR("Different MTU values on the ends of network");
                rc = TE_RC(TE_TAPI, TE_EINVAL);
                goto cleanup;
            }
        }
    }

cleanup:
    tapi_cfg_net_free_nets(&nets);

    return rc;
}

/* See description in tapi_cfg_net.h */
int
tapi_cfg_net_assign_ip_one_end(unsigned int af, cfg_net_t *net,
                               tapi_cfg_net_assigned *assigned)
{
    int                 rc;
    cfg_val_type        type;
    char               *str;
    unsigned int        i;
    cfg_handle          net_hndl;
    char               *net_oid = NULL;
    unsigned int        net_pfx;
    struct sockaddr    *net_addr;
    cfg_handle          entry_hndl;
    cfg_handle          addr_hndl;
    struct sockaddr    *addr;

    if (net == NULL)
    {
        ERROR("%s: Net pointer is NULL", __FUNCTION__);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    /* Allocate or use passed IPv4 subnet */
    if (assigned == NULL ||
        assigned->pool == CFG_HANDLE_INVALID)
    {
        rc = tapi_cfg_alloc_entry(af == AF_INET ? "/net_pool:ip4" :
                                                  "/net_pool:ip6",
                                  &net_hndl);
        if (rc != 0)
        {
            ERROR("%s: Failed to allocate net to assign: %r",
                  __FUNCTION__, rc);
            return rc;
        }
    }
    else
    {
        net_hndl = assigned->pool;
    }

    do { /* Fake loop */

        /*
         * Get all information about this IPv4 subnet
         */
        rc = cfg_get_inst_name_type(net_hndl, CVT_ADDRESS,
                                    CFG_IVP(&net_addr));
        if (rc != 0)
        {
            ERROR("%s(): cfg_get_inst_name_type(0x%jx) failed: %r",
                  __FUNCTION__, (uintmax_t)net_hndl, rc);
            break;
        }

        /* Get OID as string */
        rc = cfg_get_oid_str(net_hndl, &net_oid);
        if (rc != 0)
        {
            ERROR("%s(): cfg_get_oid_str(0x%x) failed: %r",
                  __FUNCTION__, net_hndl, rc);
            break;
        }
        /* Get prefix length */
        type = CVT_INT32;
        rc = cfg_get_instance_fmt(&type, &net_pfx, "%s/prefix:", net_oid);
        if (rc != 0)
        {
            ERROR("Failed to get IPv4 subnet '%s' prefix: %r",
                  net_oid, rc);
            break;
        }

        /* Add the subnet to the list of IPv4 subnets of the net */
        rc = cfg_add_instance_child_fmt(NULL, CVT_ADDRESS, net_addr,
                                        net->handle, "/ip%u_subnet:0x%jx",
                                        af == AF_INET ? 4 : 6,
                                        (uintmax_t)net_hndl);
        if (rc != 0)
        {
            ERROR("Failed to add '/ip%u_subnet:0x%jx' child "
                  "to instance with handle 0x%jx: %r",
                  af == AF_INET ? 4 : 6, (uintmax_t)net_hndl,
                  (uintmax_t)net->handle, rc);
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
            rc = tapi_cfg_alloc_net_addr(net_hndl, &entry_hndl, &addr);
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
                free(addr);
                break;
            }

            rc = tapi_cfg_base_add_net_addr(str, addr, net_pfx, true,
                                            &addr_hndl);
            if (TE_RC_GET_ERROR(rc) == TE_EEXIST)
            {
                /* Address already assigned - continue */
                rc = 0;
            } else if (rc != 0)
            {
                free(addr);
                break;
            }

            rc = cfg_add_instance_child_fmt(NULL, CVT_ADDRESS,
                                            SA(addr),
                                            net->nodes[i].handle,
                                            "/ip%u_address:0x%jx",
                                            af == AF_INET ? 4 : 6,
                                            (uintmax_t)entry_hndl);
            if (rc != 0)
            {
                ERROR("Failed to add 'ip%u_address:0x%jx' child to "
                      "instance with handle 0x%jx: %r",
                      af == AF_INET ? 4 : 6, (uintmax_t)entry_hndl,
                      (uintmax_t)net->nodes[i].handle, rc);
                free(addr);
                break;
            }
            free(addr);

            if (assigned != NULL)
                assigned->entries[i] = entry_hndl;
        }

    } while (0); /* End of fake loop */

    if ((rc == 0) &&
        (assigned != NULL) && (assigned->pool == CFG_HANDLE_INVALID))
    {
        assigned->pool= net_hndl;
    }

    free(net_oid);

    return rc;
}

/**
 * For a given configuration network, find its mask and prefix.
 *
 * @param       net         Network descriptor.
 * @param       af          Mask address family.
 * @param[out]  mask        Network's mask.
 * @param[out]  pfx         Network's prefix.
 *
 * @return Status code.
 */
static te_errno
tapi_cfg_net_get_net_mask(cfg_net_t *net, unsigned int af,
                          struct sockaddr **mask, int32_t *pfx)
{
    te_errno         rc;
    unsigned int     n_nodes;
    cfg_handle      *node_handles;
    cfg_val_type     cvt;
    char            *mask_str;
    struct sockaddr *mask_tmp;

    rc = cfg_find_pattern_fmt(&n_nodes, &node_handles, "/net:%s/ip%u_subnet:*",
                              net->name, af == AF_INET ? 4 : 6);
    if (TE_RC_GET_ERROR(rc) != 0)
    {
        ERROR("Failed to find the IPv%u subnet in %s: %r",
              af == AF_INET ? 4 : 6, net->name, rc);
        return rc;
    }

    if (n_nodes != 1)
    {
        ERROR("Only one IPv%u subnet is allowed in %s, found %u: %r",
              af == AF_INET ? 4 : 6, net->name, n_nodes, rc);
        free(node_handles);
        return rc;
    }

    cvt = CVT_ADDRESS;
    rc = cfg_get_instance(node_handles[0], &cvt, &mask_tmp);
    free(node_handles);
    if (rc != 0)
    {
        ERROR("Failed to get mask for network '%s': %r", net->name, rc);
        return rc;
    }

    rc = te_sockaddr_h2str(mask_tmp, &mask_str);
    if (rc != 0)
    {
        ERROR("Failed to format mask for network '%s': %r", net->name, rc);
        free(mask_tmp);
        return rc;
    }

    rc = cfg_get_int32(pfx, "/net_pool:%s/entry:%s/prefix:",
                       af == AF_INET ? "ip4" : "ip6",
                       mask_str);
    free(mask_str);
    if (rc != 0)
    {
        ERROR("Failed to extract prefix for network '%s': %r", net->name, rc);
        return rc;
    }

    *mask = mask_tmp;

    return 0;
}

/* See description in tapi_cfg_net.h */
cfg_net_node_t *
tapi_cfg_net_get_gateway(const cfg_net_t *net_src, const cfg_net_t *net_tgt)
{
    te_errno      rc;
    unsigned int  i;
    const char   *gateway_name;

    gateway_name = te_kvpairs_get(&net_src->gateways, net_tgt->name);
    if (gateway_name == NULL)
        return NULL;

    for (i = 0; i < net_src->n_nodes; i++)
    {
        char *node_name;

        rc = cfg_get_inst_name(net_src->nodes[i].handle, &node_name);
        if (rc != 0)
        {
            ERROR("Failed to get instance name of network node with handle 0x%x: %r",
                  net_src->nodes[i].handle, rc);
            continue;
        }

        if (strcmp(node_name, gateway_name) == 0)
        {
            free(node_name);
            return &net_src->nodes[i];
        }

        free(node_name);
    }

    return NULL;
}

/**
 * Get information associated with a given network node.
 *
 * @param       node_handle         Network node.
 * @param       af                  Address family for the address.
 * @param[out]  addr                Node's network address.
 * @param[out]  ta                  On which Test Agent the node is located.
 * @param[out]  intf                The interface that the node uses.
 *
 * @return Status code.
 */
static te_errno
tapi_cfg_net_get_node_info(cfg_handle node_handle, unsigned int af,
                           struct sockaddr **addr, char **ta, char **intf)
{
    te_errno      rc;
    char         *node_oid = NULL;
    cfg_val_type  cvt = CVT_STRING;
    cfg_oid      *intf_oid = NULL;
    char         *intf_str = NULL;
    unsigned int  n_ips;
    cfg_handle   *ip_hndls = NULL;

    rc = cfg_get_oid_str(node_handle, &node_oid);
    if (TE_RC_GET_ERROR(rc) != 0)
    {
        ERROR("Failed to get OID of network node %u: %r",
             node_handle, rc);
        goto cleanup;
    }

    rc = cfg_get_string(&intf_str, "%s", node_oid);
    if (TE_RC_GET_ERROR(rc) != 0)
    {
        ERROR("Failed to get interface of network node %s: %r",
              node_oid, rc);
        goto cleanup;
    }

    intf_oid = cfg_convert_oid_str(intf_str);
    if (intf_oid == NULL)
    {
        ERROR("Failed to convert OID of network node %u: %r",
                  node_handle, rc);
        goto cleanup;
    }

    if (ta != NULL)
        *ta = cfg_oid_get_inst_name(intf_oid, 1);
    if (intf != NULL)
        *intf = cfg_oid_get_inst_name(intf_oid, 2);

    if (addr != NULL)
    {
        rc = cfg_find_pattern_fmt(&n_ips, &ip_hndls, "%s/ip%u_address:*",
                                  node_oid, af == AF_INET ? 4 : 6);
        if (TE_RC_GET_ERROR(rc) != 0)
        {
            ERROR("Failed to find IPv%u address of network node %s: %r",
                  af == AF_INET ? 4 : 6, node_oid, rc);
            goto cleanup;
        }

        if (n_ips != 1)
        {
            ERROR("Node %s has %d IPv%u addresses, unsupported",
                  node_oid, n_ips, af == AF_INET? 4 : 6);
            rc = TE_RC(TE_TAPI, TE_EINVAL);
        }
        else if (n_ips == 1)
        {
            cvt = CVT_ADDRESS;
            rc = cfg_get_instance(ip_hndls[0], &cvt, addr);
            if (TE_RC_GET_ERROR(rc) != 0)
            {
                ERROR("Failed to get IPv%u address of network node %s: %r",
                      af == AF_INET ? 4 : 6, node_oid, rc);
            }
        }
    }

cleanup:
    free(ip_hndls);
    free(intf_str);
    cfg_free_oid(intf_oid);
    free(node_oid);

    return rc;
}

/**
 * Create routes from one non-virtual network to another.
 *
 * @param af            Route address family.
 * @param net_src       Source network.
 * @param net_tgt       Target network.
 *
 * @return Status code.
 */
static te_errno
tapi_cfg_net_create_routes_to(unsigned int af, cfg_net_t *net_src, cfg_net_t *net_tgt)
{
    te_errno     rc;
    unsigned int i;

    cfg_net_node_t  *gw_src = NULL;
    cfg_net_node_t  *gw_tgt = NULL;
    struct sockaddr *gw_addr = NULL;
    struct sockaddr *dst_mask = NULL;
    char            *dst_mask_str = NULL;
    int32_t          dst_pfx;

    if (net_src->n_nodes == 0 || net_tgt->n_nodes == 0)
    {
        WARN("Tried to create routes between networks '%s' and '%s' one of which is empty",
             net_src->name, net_tgt->name);
        rc = TE_RC(TE_TAPI, TE_EINVAL);
        goto cleanup;
    }

    rc = tapi_cfg_net_get_net_mask(net_tgt, af, &dst_mask, &dst_pfx);
    if (rc != 0)
    {
        ERROR("Failed to extract mask from network '%s': %r", net_tgt->name, rc);
        goto cleanup;
    }

    dst_mask_str = te_ip2str(dst_mask);
    if (dst_mask_str == NULL)
    {
        ERROR("Failed to convert destination mask to string");
        rc = TE_RC(TE_TAPI, TE_ENOMEM);
        goto cleanup;
    }

    gw_src = tapi_cfg_net_get_gateway(net_src, net_tgt);
    if (gw_src != NULL)
    {
        rc = tapi_cfg_net_get_node_info(gw_src->handle, af, &gw_addr,
                                        NULL, NULL);
        if (rc != 0)
        {
            ERROR("Failed to extract gateway for network '%s': %r", net_src, rc);
            goto cleanup;
        }
    }

    for (i = 0; i < net_src->n_nodes; i++)
    {
        char            *ta;
        char            *intf;
        struct sockaddr *addr;
        unsigned int     j;
        unsigned int     n_routes;
        cfg_handle      *routes;

        rc = tapi_cfg_net_get_node_info(net_src->nodes[i].handle, af, &addr, &ta,
                                        &intf);
        if (rc != 0)
        {
            ERROR("Failed to extract node info for node with handle %u: %r",
                  net_src->nodes[i].handle, rc);
            break;
        }

        /*
         * If target network's gateway is on the same TA as the source network's,
         * then use target network's gateway interface and source address.
         */
        gw_tgt = tapi_cfg_net_get_gateway(net_tgt, net_src);
        if (&net_src->nodes[i] == gw_src && gw_tgt != NULL)
        {
            struct sockaddr *gw_tgt_addr;
            char            *gw_tgt_ta;
            char            *gw_tgt_intf;

            rc = tapi_cfg_net_get_node_info(gw_tgt->handle, af, &gw_tgt_addr,
                                            &gw_tgt_ta, &gw_tgt_intf);
            if (rc != 0)
            {
                ERROR("Failed to extract node info for node with handle %u: %r",
                      net_src->nodes[i].handle, rc);
                break;
            }

            if (strcmp(ta, gw_tgt_ta) == 0)
            {
                free(addr);
                addr = gw_tgt_addr;
                free(intf);
                intf = gw_tgt_intf;
            }
            else
            {
                free(gw_tgt_addr);
                free(gw_tgt_intf);
            }
            free(gw_tgt_ta);
        }

        rc = cfg_find_pattern_fmt(&n_routes, &routes, "/agent:%s/route:%s|%d*",
                                  ta, dst_mask_str, dst_pfx);
        if (TE_RC_GET_ERROR(rc) != 0)
        {
            ERROR("Failed to check if route already exists: %r", rc);
        }
        for (j = 0; j < n_routes; j++)
        {
            rc = tapi_cfg_del_route(&routes[j]);
            if (rc != 0)
            {
                ERROR("Failed to remove old route: %r", rc);
                free(ta);
                free(intf);
                free(addr);
                free(routes);
                goto cleanup;
            }
        }

        rc = tapi_cfg_add_route(ta, af,
                        te_sockaddr_get_netaddr(dst_mask), dst_pfx,
                        i == 0 ? NULL : te_sockaddr_get_netaddr(gw_addr),
                        intf, NULL,
                        0, af == AF_INET ? 0 : 1, 0, 0, 0, 0, NULL);
        free(ta);
        free(intf);
        free(addr);
        free(routes);

        if (rc != 0)
        {
            ERROR("Failed to set up routing rule: %r", rc);
            break;
        }
    }

cleanup:
    free(dst_mask_str);
    free(dst_mask);
    free(gw_addr);

    return rc;
}

/**
 * Find network with a node that has a given OID.
 *
 * @param      nets         All available networks.
 * @param      oid          OID of a node in the target network.
 * @param[out] net          Target network.
 * @param[out] index        Index of target network.
 *
 * @return Status code.
 */
static te_errno
tapi_cfg_net_find_net_by_node_oid(cfg_nets_t *nets, const char *oid,
                                  cfg_net_t **net, unsigned int *index)
{
    char         *net_name;
    unsigned int  i;

    net_name = cfg_oid_str_get_inst_name(oid, 1);
    if (net_name == NULL)
    {
        ERROR("Failed to extact network name from OID %s", oid);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    for (i = 0; i < nets->n_nets; i++)
    {
        if (strcmp(nets->nets[i].name, net_name) == 0)
        {
            *net = &nets->nets[i];
            *index = i;
            break;
        }
    }
    free(net_name);

    if (i == nets->n_nets)
        return TE_RC(TE_TAPI, TE_ENOENT);
    else
        return 0;
}

/* See description in tapi_cfg_net.h */
te_errno
tapi_cfg_net_create_routes(unsigned int af)
{
    te_errno      rc = 0;
    unsigned int  i, j, k;
    cfg_nets_t    nets;
    bool         *routed = NULL;

    /* Get testing network configuration in C structures */
    rc = tapi_cfg_net_get_nets(&nets);
    if (rc != 0)
    {
        ERROR("Failed to get cfg networks: %r", rc);
        return rc;
    }

    routed = TE_ALLOC(sizeof(bool) * nets.n_nets * nets.n_nets);

    for (i = 0; i < nets.n_nets; ++i)
    {
        cfg_net_t *net = nets.nets + i;

        if (!net->is_virtual)
            continue;

        for (j = 0; j < net->n_nodes - 1; ++j)
        {
            cfg_val_type  type;
            char         *oid = NULL;
            cfg_net_t    *net1;
            unsigned int  net1_ind;

            type = CVT_STRING;
            rc = cfg_get_instance(net->nodes[j].handle, &type, &oid);
            if (rc != 0)
            {
                ERROR("Failed to extract virtual node content: %r", rc);
                goto cleanup;
            }

            rc = tapi_cfg_net_find_net_by_node_oid(&nets, oid, &net1, &net1_ind);
            free(oid);
            if (rc != 0)
            {
                ERROR("Failed to find network from OID %s", oid);
                goto cleanup;
            }

            for (k = j + 1; k < net->n_nodes; ++k)
            {
                cfg_net_t *net2;
                unsigned int net2_ind;

                type = CVT_STRING;
                rc = cfg_get_instance(net->nodes[k].handle, &type, &oid);
                if (rc != 0)
                {
                    ERROR("Failed to extract virtual node content: %r", rc);
                    goto cleanup;
                }

                rc = tapi_cfg_net_find_net_by_node_oid(&nets, oid, &net2,
                                                       &net2_ind);
                free(oid);
                if (rc != 0)
                {
                    ERROR("Failed to find network from OID %s", oid);
                    goto cleanup;
                }

                if (routed[net1_ind * nets.n_nets + net2_ind])
                    continue;

                if (strcmp(net1->name, net2->name) != 0)
                {
                    routed[net1_ind * nets.n_nets + net2_ind] = true;
                    routed[net2_ind * nets.n_nets + net1_ind] = true;
                    rc = tapi_cfg_net_create_routes_to(af, net1, net2);
                    if (rc == 0)
                        rc = tapi_cfg_net_create_routes_to(af, net2, net1);
                }

                if (rc != 0)
                    break;
            }
            if (rc != 0)
                break;
        }
    }

cleanup:
    free(routed);
    tapi_cfg_net_free_nets(&nets);

    return rc;
}

/**
 * Given a network and a node instance name, find the associated node structure.
 *
 * @param      net          Network to search.
 * @param      name         Node name.
 * @param[out] node         Node description structure.
 *
 * @return Status code.
 */
static te_errno
tapi_cfg_net_find_node(cfg_net_t *net, const char *name, cfg_net_node_t **node)
{
    te_errno     rc;
    unsigned int i;

    for (i = 0; i < net->n_nodes; i++)
    {
        char *node_name;

        rc = cfg_get_inst_name(net->nodes[i].handle, &node_name);
        if (rc != 0)
        {
            ERROR("Failed to get instance name of node %u from net %s: %r",
                  i, net->name, rc);
            return rc;
        }

        if (strcmp(node_name, name) == 0)
        {
            free(node_name);
            *node = &net->nodes[i];
            return 0;
        }

        free(node_name);
    }

    return TE_RC(TE_TAPI, TE_ENOENT);
}

/**
 * Set up masquerading for a given network using iptables.
 *
 * @param nets                  All available networks.
 * @param net                   Network to set up.
 * @param af                    Address family.
 *
 * @return Status code.
 */
static te_errno
tapi_cfg_net_setup_masquerade(cfg_nets_t *nets, cfg_net_t *net, unsigned int af)
{
    te_errno     rc = 0;
    unsigned int i;

    for (i = 0; i < nets->n_nets; i++)
    {
        char           *ta;
        char           *intf;
        cfg_net_t      *net2;
        cfg_net_node_t *gw;

        net2 = &nets->nets[i];
        gw = tapi_cfg_net_get_gateway(net2, net);

        if (net2 == net || net2->is_virtual || net2->n_nodes == 0)
            continue;

        if (gw == NULL)
        {
            ERROR("Failed to set up masquerading between networks %s and %s: no gateway specified",
                  net->name, net2->name);
            return TE_RC(TE_TAPI, TE_EINVAL);
        }

        rc = tapi_cfg_net_get_node_info(gw->handle, af,
                                        NULL, &ta, &intf);
        if (rc != 0)
        {
            ERROR("Failed to extract node info of gateway in net %s: %r",
                  net2->name, rc);
            return rc;
        }

        rc = tapi_cfg_iptables_chain_add(ta, intf, af, "nat",
                                         "POSTROUTING", true);
        if (rc != 0)
        {
            ERROR("Failed to create POSTROUTING chain for %s on TA %s: %r",
                  intf, ta, rc);
            free(ta);
            free(intf);
            return rc;
        }

        rc = tapi_cfg_iptables_rules(ta, intf, af, "nat", "POSTROUTING",
                                     "-j MASQUERADE");
        if (rc != 0)
        {
            ERROR("Failed to add MASQUERADE action for %s on TA %s: %r",
                  intf, ta, rc);
            free(ta);
            free(intf);
            return rc;
        }

        free(ta);
        free(intf);
    }

    return 0;
}

/**
 * Set up DNAT within a given network using iptables.
 *
 * @param nets              All available networks.
 * @param net               Network to set up.
 * @param af                Address family.
 * @param rule              Forwarding rule to install.
 * @param target_name       Name of the target node.
 *
 * @return Status code.
 */
static te_errno
tapi_cfg_net_setup_dnat(cfg_nets_t *nets, cfg_net_t *net, unsigned int af,
                        const char *rule, const char *target_name)
{
    unsigned int     i;
    te_errno         rc;
    cfg_net_t       *source_net = NULL;
    cfg_net_node_t  *target_node;
    cfg_net_node_t  *gateway_node;
    struct sockaddr *target_addr = NULL;
    char            *target_addr_str = NULL;
    char            *source_ta = NULL;
    char            *source_intf = NULL;

    RING("Redirecting traffic \"%s\" to %s:%s", rule, net->name, target_name);

    if (strchr(rule, ':') != NULL)
    {
        ERROR("Unsupported or unimplemented forwarding rule format");
        rc = TE_RC(TE_TAPI, TE_EINVAL);
        return rc;
    }

    for (i = 0; i < nets->n_nets; i++)
    {
        if (strcmp(nets->nets[i].name, rule) == 0)
        {
            source_net = &nets->nets[i];
            break;
        }
    }
    if (source_net == NULL)
    {
        ERROR("Failed to find source network '%s'", rule);
        return TE_RC(TE_TAPI, TE_ENOENT);
    }

    if (source_net->n_nodes == 0)
    {
        ERROR("Source network does not have any nodes");
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    gateway_node = tapi_cfg_net_get_gateway(source_net, net);
    if (gateway_node == NULL)
    {
        ERROR("No gateway available");
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    rc = tapi_cfg_net_find_node(net, target_name, &target_node);
    if (rc != 0)
    {
        ERROR("Failed to find target node %s in net %s: %r",
              target_node, net->name, rc);
        return rc;
    }

    rc = tapi_cfg_net_get_node_info(target_node->handle, af, &target_addr,
                                    NULL, NULL);
    if (rc != 0)
    {
        ERROR("Failed to get target node (%s) address: %r", target_name, rc);
        return rc;
    }

    rc = tapi_cfg_net_get_node_info(gateway_node->handle, af, NULL,
                                    &source_ta, &source_intf);
    if (rc != 0)
    {
        ERROR("Failed to get source network (%s) gateway info: %r",
              source_net->name, rc);
        goto cleanup;
    }

    target_addr_str = te_ip2str(target_addr);
    if (target_addr_str == NULL)
    {
        ERROR("Failed to transform target node (%s) addres to string");
        rc = TE_RC(TE_TAPI, TE_ENOMEM);
        goto cleanup;
    }

    rc = tapi_cfg_iptables_chain_add(source_ta, source_intf, af, "nat",
                                     "PREROUTING", true);
    if (rc != 0)
    {
        ERROR("Failed to create PREROUTING chain for %s on TA %s: %r",
              source_intf, source_ta, rc);
        goto cleanup;
    }

    rc = tapi_cfg_iptables_rules_fmt(source_ta, source_intf, af, "nat",
                                     "PREROUTING",
                                     "-j DNAT --to %s", target_addr_str);
    if (rc != 0)
    {
        ERROR("Failed to add DNAT action for %s on TA %s: %r",
              source_intf, source_ta, rc);
        goto cleanup;
    }

cleanup:
    free(source_ta);
    free(source_intf);
    free(target_addr);
    free(target_addr_str);

    return rc;
}

/**
 * Set up NAT for a given network using iptables.
 *
 * @param   nets        All available networks.
 * @param   net         Network to set up.
 * @param   af          Address family.
 *
 * @return Status code.
 */
static te_errno
tapi_cfg_net_setup_iptables(cfg_nets_t *nets, cfg_net_t *net, unsigned int af)
{
    te_errno      rc;
    unsigned int  i;
    unsigned int  n_handles;
    cfg_handle   *handles = NULL;

    if (!net->nat)
        return TE_RC(TE_TAPI, TE_EINVAL);

    rc = tapi_cfg_net_setup_masquerade(nets, net, af);
    if (rc != 0)
    {
        ERROR("Failed to set up masquerading for network %s: %r",
              net->name, rc);
        goto cleanup;
    }

    rc = cfg_find_pattern_fmt(&n_handles, &handles, "/net:%s/nat:/forward:*",
                              net->name);
    if (rc != 0)
    {
        ERROR("Failed to find forwarding rules for network %s: %r",
              net->name, rc);
        goto cleanup;
    }

    for (i = 0; i < n_handles; i++)
    {
        cfg_val_type  cvt;
        char         *rule = NULL;
        char         *target = NULL;

        rc = cfg_get_inst_name(handles[i], &rule);
        if (rc != 0)
        {
            ERROR("Failed to extract forwarding rule %u from net %s: %r",
                  i, net->name, rc);
            break;
        }

        cvt = CVT_STRING;
        rc = cfg_get_instance(handles[i], &cvt, &target);
        if (rc != 0)
        {
            ERROR("Failed to extract forwarding rule %u from net %s: %r",
                  i, net->name, rc);
            free(rule);
            break;
        }

        rc = tapi_cfg_net_setup_dnat(nets, net, af, rule, target);
        free(rule);
        free(target);
        if (rc != 0)
        {
            ERROR("Failed to find %s in net %s: %r", target, net->name, rc);
            goto cleanup;
        }
    }

cleanup:
    free(handles);

    return rc;
}

/* See description in tapi_cfg_net.h */
te_errno
tapi_cfg_net_create_nat(unsigned int af)
{
    unsigned int    i;
    int             rc = 0;

    cfg_nets_t      nets;

    /* Get testing network configuration in C structures */
    rc = tapi_cfg_net_get_nets(&nets);
    if (rc != 0)
    {
        ERROR("tapi_cfg_net_get_nets() failed %r", rc);
        return rc;
    }

    for (i = 0; i < nets.n_nets && rc == 0; ++i)
    {
        cfg_net_t       *net = nets.nets + i;

        if (!net->nat)
            continue;

        switch(net->nat_setup)
        {
            case NET_NAT_SETUP_IPTABLES:
                rc = tapi_cfg_net_setup_iptables(&nets, net, af);
                if (rc != 0)
                {
                    ERROR("Failed to set iptables rules for network %s: %r",
                          net->name, rc);
                }
                break;
            default:
                break;
        }
    }

    tapi_cfg_net_free_nets(&nets);

    return rc;
}

/* See description in tapi_cfg_net.h */
te_errno
tapi_cfg_net_delete_all(void)
{
    te_errno        rc;
    unsigned int    n_nets;
    cfg_handle     *net_handles = NULL;
    unsigned int    i;

    rc = cfg_find_pattern("/net:*", &n_nets, &net_handles);
    if (rc != 0)
    {
        ERROR("cfg_find_pattern() failed %r", rc);
        return rc;
    }

    for (i = 0; i < n_nets; ++i)
    {
        rc = cfg_del_instance(net_handles[i], true);
        if (rc != 0)
        {
            ERROR("Failed to delete net with handle %#08x: %r",
                  net_handles[i], rc);
            break;
        }
    }

    free(net_handles);

    return rc;
}

/**
 * Remove networks with empty interface names from the CS database.
 *
 * @return Status code.
 */
te_errno
tapi_cfg_net_remove_empty(void)
{
    unsigned int n_nets, n_nodes, i, j;
    cfg_handle  *net_handles = NULL;
    cfg_handle  *node_handles = NULL;
    char        *net_name = NULL;
    char        *node_value = NULL;
    te_errno     rc = 0;

    if ((rc = cfg_find_pattern("/net:*", &n_nets, &net_handles)) != 0)
    {
        if (TE_RC_GET_ERROR(rc) == TE_ENOENT)
            return 0;

        return rc;
    }

    for (i = 0; i < n_nets && rc == 0; i++)
    {
        char *tmp;

        if ((rc = cfg_get_inst_name(net_handles[i], &net_name)) != 0)
        {
            ERROR("Failed to get /net name by its handle");
            break;
        }

        if ((rc = cfg_find_pattern_fmt(&n_nodes, &node_handles,
                                       "/net:%s/node:*", net_name)) != 0)
        {
            ERROR("Failed to get nodes of the net %s", net_name);
            break;
        }

        for (j = 0; j < n_nodes; j++)
        {
            if ((rc = cfg_get_instance(node_handles[j], NULL,
                                       &node_value)) != 0)
            {
                ERROR("Failed to get /net/node value");
                break;
            }

            if ((tmp = strrchr(node_value, ':')) == NULL)
            {
                ERROR("Unexpected /net/node value: %s", node_value);
                rc = TE_RC(TE_TAPI, TE_EINVAL);
                break;
            }

            if (*(tmp + 1) == 0)
            {
                if ((rc = cfg_del_instance(net_handles[i], true)) != 0)
                    ERROR("Failed to delete /net:%s", net_name);

                break;
            }
            free(node_value); node_value = NULL;
        }

        free(net_name); net_name = NULL;
        free(node_handles); node_handles = NULL;
        free(node_value); node_value = NULL;
    }

    free(net_handles);
    free(node_handles);
    free(net_name);
    free(node_value);

    return rc;
}

static te_errno
tapi_cfg_net_get_node_oid(const cfg_net_node_t *node, char **oid)
{
    cfg_val_type type = CVT_STRING;

    return cfg_get_instance(node->handle, &type, oid);
}

te_errno
tapi_cfg_net_node_get_pci_oids(const cfg_net_node_t *node, unsigned int *n_pci,
                               char ***pci_oids)
{
    char *node_oid;
    char *pci_oid_str;
    cfg_oid *pci_oid;
    te_errno rc;
    char **result;
    size_t n;

    rc = tapi_cfg_net_get_node_oid(node, &node_oid);
    if (rc != 0)
        return rc;

    switch (node->rsrc_type)
    {
        case NET_NODE_RSRC_TYPE_INTERFACE:
            ERROR("Failed to get PCI devices of a node bound to net interface");
            rc = TE_RC(TE_TAPI, TE_EINVAL);
            break;

        case NET_NODE_RSRC_TYPE_PCI_FN:
            result = TE_ALLOC(sizeof(*result));

            rc = cfg_get_instance_str(NULL, &result[0], node_oid);
            if (rc != 0)
            {
                free(result);
                ERROR("Failed to get PCI device");
                break;
            }

            *n_pci = 1;
            *pci_oids = result;
            break;

        case NET_NODE_RSRC_TYPE_PCI_FN_NETDEV:
            result = TE_ALLOC(sizeof(*result));

            pci_oid = cfg_convert_oid_str(node_oid);
            pci_oid_str = make_pci_fn_oid_str_by_pci_fn_netdev_oid(pci_oid);
            cfg_free_oid(pci_oid);

            rc = cfg_get_instance_str(NULL, &result[0], pci_oid_str);
            free(pci_oid_str);
            if (rc != 0)
            {
                free(result);
                ERROR("Failed to get PCI device");
                break;
            }

            *n_pci = 1;
            *pci_oids = result;
            break;

        case NET_NODE_RSRC_TYPE_RTE_VDEV:
            rc = tapi_cfg_net_pci_fn_by_dpdk_vdev_ref(node_oid, &n,
                                                      pci_oids);
            if (rc == 0)
                *n_pci = (unsigned int)n;
            break;

        default:
            ERROR("Failed to get PCI devices of an unknown node");
            rc = TE_RC(TE_TAPI, TE_EINVAL);
            break;
    }

    free(node_oid);

    return rc;
}

void
tapi_cfg_net_init_pci_info(cfg_net_pci_info_t *pci_info)
{
    pci_info->pci_addr = NULL;
    pci_info->bound_driver = NULL;
    pci_info->dpdk_driver = NULL;
    pci_info->net_driver = NULL;
}

void
tapi_cfg_net_free_pci_info(cfg_net_pci_info_t *pci_info)
{
    free(pci_info->pci_addr);
    free(pci_info->bound_driver);
    free(pci_info->net_driver);
    free(pci_info->dpdk_driver);
}
