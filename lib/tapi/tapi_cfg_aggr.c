/** @file
 * @brief Test API to configure bonding and bridging.
 *
 * Implementation of API to configure linux trunks
 * (IEEE 802.3ad) and bridges.
 *
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
 *
 *
 * @author Dmitry Izbitsky <Dmitry.Izbitsky@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER     "Interface aggregation TAPI"

#include "te_config.h"
#include "te_defs.h"
#include "logger_api.h"
#include "conf_api.h"
#include "tapi_cfg_base.h"
#include "tapi_host_ns.h"
#include "te_ethernet.h"

#define CHECK_BOND(ta_, name_) \
    do {                                                            \
        int   rc_;                                                  \
        char *aggr_type_ = NULL;                                    \
        rc_ = cfg_get_instance_string_fmt(&aggr_type_,              \
                        "/agent:%s/aggregation:%s",                 \
                        ta_, name_);                                \
        if (rc_ != 0 || aggr_type_ == NULL)                         \
        {                                                           \
            ERROR("Failed to obtain type of aggregation node");     \
            return rc_;                                             \
        }                                                           \
        if (strcmp(aggr_type_, "bond") != 0 &&                      \
            strcmp(aggr_type_, "team") != 0 )                       \
        {                                                           \
            ERROR("Aggregation %s is not bond or team interface",   \
                  name_);                                           \
            free(aggr_type_);                                       \
            return -1;                                              \
        }                                                           \
        free(aggr_type_);                                           \
    } while (0)

/* See the description in tapi_cfg_aggr.h */
int
tapi_cfg_aggr_create_bond(const char *ta, const char *name,
                          char **ifname, const char *type)
{
    int             rc = 0;
    char           *bond_ifname = NULL;
    cfg_handle      aggr_handle = CFG_HANDLE_INVALID;
    cfg_handle      rsrc_handle = CFG_HANDLE_INVALID;
    char            oid[CFG_OID_MAX];

    rc = cfg_add_instance_fmt(&aggr_handle, CVT_STRING, type,
                              "/agent:%s/aggregation:%s",
                              ta, name);
    if (rc != 0)
    {
        ERROR("Failed to create new aggregation node");
        return rc;
    }

    rc = cfg_get_instance_string_fmt(&bond_ifname,
                                     "/agent:%s/aggregation:%s/interface:",
                                     ta, name);
    if (rc != 0 || bond_ifname == NULL)
    {
        ERROR("Failed to obtain name of created bond interface");
        return rc;
    }

    snprintf(oid, CFG_OID_MAX, "/agent:%s/interface:%s",
             ta, bond_ifname);
    rc = cfg_add_instance_fmt(&rsrc_handle, CVT_STRING, oid,
                              "/agent:%s/rsrc:%s", ta,
                              bond_ifname);
    if (rc != 0)
    {
        ERROR("Failed to set rsrc node for created bond interface");
        return rc;
    }

    rc = tapi_cfg_base_if_up(ta, bond_ifname);
    if (rc != 0)
    {
        ERROR("Failed to bring created interface up");
        return rc;
    }

    if (tapi_host_ns_enabled())
        rc = tapi_host_ns_if_add(ta, bond_ifname, NULL);

    if (ifname != NULL)
        *ifname = bond_ifname;
    else
        free(bond_ifname);

    return rc;
}

/* See the description in tapi_cfg_aggr.h */
int
tapi_cfg_aggr_destroy_bond(const char *ta, const char *name)
{
    te_errno     rc = 0;
    te_errno     rc2;
    char        *bond_ifname = NULL;

    CHECK_BOND(ta, name);

    rc = cfg_get_instance_string_fmt(&bond_ifname,
                                     "/agent:%s/aggregation:%s/interface:",
                                     ta, name);
    if (rc != 0 || bond_ifname == NULL)
    {
        ERROR("Failed to obtain name of bond interface");
    }
    else
    {
        cfg_handle  rsrc_handle = CFG_HANDLE_INVALID;

        rc = cfg_find_fmt(&rsrc_handle, "/agent:%s/rsrc:%s",
                          ta, bond_ifname);
        if (rc != 0)
        {
            if (rc == TE_RC(TE_CS, TE_ENOENT))
                rc = 0;
            else
                ERROR("Failed to get rsrc node: %r", rc);
        }
        else
        {
            rc = tapi_cfg_base_if_down(ta, bond_ifname);
            if (rc != 0)
                ERROR("Failed to bring bond interface down: %r", rc);

            rc2 = cfg_del_instance(rsrc_handle, FALSE);
            if (rc == 0)
                rc = rc2;
        }

        if (tapi_host_ns_enabled())
        {
            rc2 = tapi_host_ns_if_del(ta, bond_ifname, TRUE);
            if (rc == 0)
                rc = rc2;
        }

        free(bond_ifname);
    }

    rc2 = cfg_del_instance_fmt(FALSE, "/agent:%s/aggregation:%s", ta, name);
    if (rc == 0)
        rc = rc2;

    return rc;
}

/* See the description in tapi_cfg_aggr.h */
int
tapi_cfg_aggr_bond_enslave(const char *ta, const char *name,
                           const char *slave_if)
{
    int             rc = 0;
    char           *bond_ifname = NULL;
    cfg_handle      slave_handle = CFG_HANDLE_INVALID;

    CHECK_BOND(ta, name);

    rc = cfg_get_instance_string_fmt(&bond_ifname,
                                     "/agent:%s/aggregation:%s/interface:",
                                     ta, name);
    if (rc != 0 || bond_ifname == NULL)
    {
        ERROR("Failed to obtain name of bond interface");
        return rc;
    }

    rc = tapi_cfg_base_if_down(ta, slave_if);
    if (rc != 0)
    {
        ERROR("Failed to bring down interface to be enslaved");
        free(bond_ifname);
        return rc;
    }

    rc = tapi_cfg_base_if_up(ta, bond_ifname);
    if (rc != 0)
    {
        ERROR("Failed to bring bond interface up");
        free(bond_ifname);
        return rc;
    }

    rc = cfg_add_instance_fmt(&slave_handle, CVT_NONE, NULL,
                              "/agent:%s/aggregation:%s/member:%s",
                              ta, name, slave_if);
    if (rc != 0)
    {
        ERROR("Failed to enslave interface");
        free(bond_ifname);
        return rc;
    }

    rc = tapi_cfg_base_if_up(ta, slave_if);
    if (rc != 0)
    {
        ERROR("Failed to bring enslaved interface up");
        free(bond_ifname);
        return rc;
    }

    if (tapi_host_ns_enabled())
        rc = tapi_host_ns_if_parent_add(ta, bond_ifname, ta, slave_if);

    free(bond_ifname);
    return rc;
}

/* See the description in tapi_cfg_aggr.h */
int
tapi_cfg_aggr_bond_free_slave(const char *ta, const char *name,
                              const char *slave_if)
{
    te_errno rc = 0;
    te_errno rc2;

    CHECK_BOND(ta, name);

    if (tapi_host_ns_enabled())
    {
        char *bond_ifname;

        rc = cfg_get_instance_fmt(NULL, &bond_ifname,
                            "/agent:%s/aggregation:%s/interface:",
                            ta, name);
        if (rc != 0)
            ERROR("Failed to obtain name of bond interface: %r");
        else
        {
            rc = tapi_host_ns_if_parent_del(ta, bond_ifname, ta, slave_if);
            free(bond_ifname);
            if (rc != 0)
                ERROR("Failed to delete parent interface reference: %r", rc);
        }
    }

    rc2 = cfg_del_instance_fmt(FALSE,
                              "/agent:%s/aggregation:%s/member:%s",
                              ta, name, slave_if);
    if (rc2 != 0)
        ERROR("Failed to release slave interface");
    if (rc == 0)
        rc = rc2;

    return rc;
}
