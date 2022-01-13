/** @file
 * @brief Network Configuration Model TAPI
 *
 * Implementation of test API for network configuration model
 * (doc/cm/cm_net.xml).
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
 * $Id$
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
#include "te_str.h"
#include "rcf_api.h"
#include "logger_api.h"
#include "conf_api.h"

#include "tapi_cfg.h"
#include "tapi_cfg_base.h"
#include "tapi_cfg_net.h"
#include "tapi_host_ns.h"

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

    if (strcmp(obj_oid, "/agent/interface") == 0)
    {
        node->rsrc_type = NET_NODE_RSRC_TYPE_INTERFACE;
    }
    else if (strcmp(obj_oid,
                    "/agent/hardware/pci/vendor/device/instance") == 0)
    {
        node->rsrc_type = NET_NODE_RSRC_TYPE_PCI_FN;
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

    /* Find all nodes in this net */
    rc = cfg_find_pattern_fmt(&n_nodes, &node_handles,
                              "%s/node:*", net_oid);
    free(net_oid);
    if (rc != 0)
    {
        ERROR("cfg_find_pattern() failed %r", rc);
        free(net->name);
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
            net->name = NULL;
            return rc;
        }
    }

    for (i = 0; i < net->n_nodes; ++i)
    {
        char           *node_oid;
        cfg_val_type    val_type = CVT_INTEGER;
        int             val;

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
        rc = cfg_get_instance_fmt(&val_type, &val,
                                  "%s/type:", node_oid);
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
        rc = cfg_set_instance_fmt(CFG_VAL(INTEGER, node->type),
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
    return cfg_del_instance_fmt(TRUE, "/net:%s", name);
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
    cfg_handle     *net_handles = NULL;
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
        char           *net_name;
        unsigned int    node_num;
        cfg_handle     *node_handles = NULL;
        unsigned int    j;

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
    char  *oid_name = NULL;

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

        oid_name = cfg_convert_oid(oid);
        if (oid_name == NULL)
            THROW_EXCEPTION("Not enough memory to get name of OID.");

        VERB("Net/Node: %s", oid_name);

        val_type = CVT_INTEGER;
        rc = cfg_get_instance_fmt(&val_type, &cfg_node_type,
                                  "%s/type:", oid_name);
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
    if (d == NULL)
        return NULL;

    d->rsrc_names = TE_ALLOC(nb_rsrcs * sizeof(*(d->rsrc_names)));
    if (d->rsrc_names == NULL)
    {
        free(d);
        return NULL;
    }

    d->rsrc_vals = TE_ALLOC(nb_rsrcs * sizeof(*(d->rsrc_vals)));
    if (d->rsrc_vals == NULL)
    {
        free(d->rsrc_names);
        free(d);
        return NULL;
    }

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

    d->rsrc_names[0] = tapi_cfg_pci_rsrc_name(oid);
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
            rc = tapi_cfg_net_mk_node_rsrc_desc_pci_fn(oid, rsrc_descp);
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
    if (result == NULL)
    {
        rc = TE_RC(TE_CONF_API, TE_ENOMEM);
        goto out;
    }

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

    UNUSED(net);

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
            n_pci_fns = 1;
            pci_fns = TE_ALLOC(sizeof(*pci_fns));
            if (pci_fns == NULL)
            {
                rc = TE_RC(TE_CONF_API, TE_ENOMEM);
                goto out;
            }

            rc = cfg_get_instance_str(NULL, &pci_fns[0], oid_str);
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
    char *pci_path = NULL;
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

    rc = cfg_get_instance_str(NULL, &pci_path, oid_str);
    if (rc != 0)
    {
        ERROR("Failed to get PCI device path: %r", rc);
        goto out;
    }

    rc = tapi_cfg_pci_get_net_if(pci_path, &interface);
    if (rc != 0)
    {
        if (TE_RC_GET_ERROR(rc) == TE_ENOENT)
        {
            ERROR("No network interfaces provided by PCI function '%s'",
                  pci_path);
        }
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
    free(pci_path);

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
    rc = cfg_synchronize("/:", TRUE);
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
    cfg_val_type type;
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

    type = CVT_STRING;
    rc = cfg_get_instance_fmt(&type, &domain, "%s/domain:", *pci_oids);
    if (rc != 0)
        goto fail_get_domain;

    rc = cfg_get_instance_fmt(&type, &bus, "%s/bus:", *pci_oids);
    if (rc != 0)
        goto fail_get_bus;

    rc = cfg_get_instance_fmt(&type, &slot, "%s/slot:", *pci_oids);
    if (rc != 0)
        goto fail_get_slot;

    type = CVT_INTEGER;
    rc = cfg_get_instance_fmt(&type, &fn, "%s/fn:", *pci_oids);
    if (rc != 0)
        goto fail_get_fn;

    pci_info->pci_addr = TE_ALLOC(16);
    if (pci_info->pci_addr == NULL)
    {
        rc = TE_ENOMEM;
        goto fail_alloc_pci_addr;
    }
    snprintf(pci_info->pci_addr, 16, "%s:%s:%s.%d", domain, bus, slot, fn);

    agent = CFG_OID_GET_INST_NAME(oid, 1);
    rc = tapi_cfg_pci_get_ta_driver(agent, NET_DRIVER_TYPE_NET,
                                    &pci_info->ta_driver);
    if (rc != 0)
        goto fail_get_driver;

    return 0;

fail_get_driver:
    tapi_cfg_net_free_pci_info(pci_info);
fail_alloc_pci_addr:
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
tapi_cfg_net_all_up(te_bool force)
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
        cfg_val_type    type;
        int             status;

        if (nodes[k] == NULL)
            continue; /* Not an interface */

        type = CVT_INTEGER;
        rc = cfg_get_instance_fmt(&type, &status, "%s/status:", nodes[k]);
        if (rc != 0)
        {
            ERROR("Failed to get status of %s: %r", nodes[k], rc);
            goto error;
        }
        if (status == 1)
        {
            if (force)
            {
                rc = cfg_set_instance_fmt(CFG_VAL(INTEGER, 0),
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
        rc = cfg_set_instance_fmt(CFG_VAL(INTEGER, 1),
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
    int                 rc;
    cfg_val_type        type;
    char                ta_type[RCF_MAX_NAME];
    char               *def_route_if;
    te_bool             ipv6 = FALSE;

    struct sockaddr_storage   dummy_addr;

    UNUSED(net);
    UNUSED(node);

    if (cookie != NULL && *(te_bool *)cookie)
        ipv6 = TRUE;

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
        type = CVT_STRING;
        rc = cfg_get_instance_fmt(&type, &def_route_if,
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
    te_bool ipv6 = FALSE;

    return tapi_cfg_net_foreach_node(
                                tapi_cfg_net_node_delete_all_ip_addresses,
                                &ipv6);
}

/* See description in tapi_cfg_net.h */
te_errno
tapi_cfg_net_delete_all_ip6_addresses(void)
{
    te_bool ipv6 = TRUE;

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
            ERROR("%s(): cfg_get_inst_name_type(0x%x) failed: %r",
                  __FUNCTION__, net_hndl, rc);
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
        type = CVT_INTEGER;
        rc = cfg_get_instance_fmt(&type, &net_pfx, "%s/prefix:", net_oid);
        if (rc != 0)
        {
            ERROR("Failed to get subnet '%s' prefix: %r", net_oid, rc);
            break;
        }

        /* Add the subnet to the list of subnets of the net */
        rc = cfg_add_instance_child_fmt(NULL, CVT_ADDRESS, net_addr,
                                        net->handle, "/ip%u_subnet:%u",
                                        af == AF_INET ? 4 : 6, net_hndl);
        if (rc != 0)
        {
            ERROR("Failed to add '/ip%u_subnet:%u' child to "
                  "instance with handle 0x%x: %r",
                  af == AF_INET ? 4 : 6, net_hndl, net->handle, rc);
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

                rc = tapi_cfg_base_add_net_addr(str, addr, net_pfx, TRUE,
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
                                            "/ip%u_address:%u",
                                            af == AF_INET ? 4 : 6,
                                            entry_hndl);
            if (rc != 0)
            {
                ERROR("Failed to add 'ip%u_address:%u' child to "
                      "instance with handle 0x%x: %r",
                      af == AF_INET ? 4 : 6, entry_hndl,
                      net->nodes[i].handle, rc);
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
        rc = cfg_del_instance_fmt(FALSE, "%s/net_addr:%s",
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
        rc = cfg_del_instance(assigned->entries[i], TRUE);
        if (rc != 0)
        {
            ERROR("Failed to delete ip address instance from net/node");
        }
        else
        {
            char *pool_entry_oid_str;
            int   n_entries;

            type = CVT_INTEGER;
            if (cfg_get_oid_str(pool_entry_handle,
                                &pool_entry_oid_str) == 0 &&
                cfg_get_instance_fmt(&type, &n_entries,
                                     "%s/n_entries:",
                                     pool_entry_oid_str) == 0)
            {
                n_entries--;
                rc = cfg_set_instance_fmt(CFG_VAL(INTEGER, n_entries),
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
                                     CFG_VAL(INTEGER, 0));
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
        ERROR("Failed to retrive subnet address: %r", rc);
        free(net_oid);
        return rc;
    }

    /* Get prefix length */
    type = CVT_INTEGER;
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

            type = CVT_INTEGER;
            rc = cfg_get_instance_fmt(&type, &mtu, "%s/mtu:", oid);
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
            ERROR("%s(): cfg_get_inst_name_type(0x%x) failed: %r",
                  __FUNCTION__, net_hndl, rc);
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
        type = CVT_INTEGER;
        rc = cfg_get_instance_fmt(&type, &net_pfx,
                                  "%s/prefix:", net_oid);
        if (rc != 0)
        {
            ERROR("Failed to get IPv4 subnet '%s' prefix: %r",
                  net_oid, rc);
            break;
        }

        /* Add the subnet to the list of IPv4 subnets of the net */
        rc = cfg_add_instance_child_fmt(NULL, CVT_ADDRESS, net_addr,
                                        net->handle, "/ip%u_subnet:%u",
                                        af == AF_INET ? 4 : 6, net_hndl);
        if (rc != 0)
        {
            ERROR("Failed to add '/ip%u_subnet:%u' child to "
                  "instance with handle 0x%x: %r",
                  af == AF_INET ? 4 : 6, net_hndl, net->handle, rc);
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

            rc = tapi_cfg_base_add_net_addr(str, addr, net_pfx, TRUE,
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
                                            "/ip%u_address:%u",
                                            af == AF_INET ? 4 : 6,
                                            entry_hndl);
            if (rc != 0)
            {
                ERROR("Failed to add 'ip%u_address:%u' child to "
                      "instance with handle 0x%x: %r",
                      af == AF_INET ? 4 : 6, entry_hndl,
                      net->nodes[i].handle, rc);
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
        rc = cfg_del_instance(net_handles[i], TRUE);
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
                if ((rc = cfg_del_instance(net_handles[i], TRUE)) != 0)
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
            if (result == NULL)
            {
                rc = TE_RC(TE_TAPI, TE_ENOMEM);
                break;
            }

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
    pci_info->ta_driver = NULL;
}

void
tapi_cfg_net_free_pci_info(cfg_net_pci_info_t *pci_info)
{
    free(pci_info->pci_addr);
    free(pci_info->ta_driver);
}
