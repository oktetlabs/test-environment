/** @file
 * @brief Test API to configure bonding and bridging.
 *
 * Implementation of API to configure linux trunks
 * (IEEE 802.3ad) and bridges.
 *
 *
 * Copyright (C) 2005 Test Environment authors (see file AUTHORS
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
#include "te_ethernet.h"

#define CHECK_BOND(ta_, name_) \
    do {                                                            \
        int             rc_;                                        \
        cfg_val_type    val_type_ = CVT_STRING;                     \
        char           *aggr_type_ = NULL;                          \
        rc_ = cfg_get_instance_fmt(                                 \
                        &val_type_, &aggr_type_,                    \
                        "/agent:%s/aggregation:%s",                 \
                        ta_, name_);                                \
        if (rc_ != 0 || aggr_type_ == NULL)                         \
        {                                                           \
            ERROR("Failed to obtain type of aggregation node");     \
            return rc_;                                             \
        }                                                           \
        if (strcmp(aggr_type_, "802.3ad") != 0)                     \
        {                                                           \
            ERROR("Aggregation %s is not bond interface",           \
                  name_);                                           \
            free(aggr_type_);                                       \
            return -1;                                              \
        }                                                           \
        free(aggr_type_);                                           \
    } while (0)

/* See the description in tapi_cfg_aggr.h */
int
tapi_cfg_aggr_create_bond(const char *ta, const char *name,
                          char **ifname)
{
    int             rc = 0;
    cfg_val_type    val_type;
    char           *bond_ifname = NULL;
    cfg_handle      aggr_handle = CFG_HANDLE_INVALID;
    cfg_handle      rsrc_handle = CFG_HANDLE_INVALID;
    char            oid[CFG_OID_MAX];

    rc = cfg_add_instance_fmt(&aggr_handle, CVT_STRING, "802.3ad",
                              "/agent:%s/aggregation:%s",
                              ta, name);
    if (rc != 0)
    {
        ERROR("Failed to create new aggregation node");
        return rc;
    }

    val_type = CVT_STRING;
    rc = cfg_get_instance_fmt(
                        &val_type, &bond_ifname,
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

    if (ifname != NULL)
        *ifname = bond_ifname;
    else
        free(bond_ifname);

    return 0;
}

/* See the description in tapi_cfg_aggr.h */
int
tapi_cfg_aggr_destroy_bond(const char *ta, const char *name)
{
    int             rc = 0;
    cfg_val_type    val_type;
    char           *bond_ifname = NULL;
    cfg_handle      aggr_handle = CFG_HANDLE_INVALID;
    cfg_handle      rsrc_handle = CFG_HANDLE_INVALID;

    CHECK_BOND(ta, name);

    val_type = CVT_STRING;
    rc = cfg_get_instance_fmt(
                        &val_type, &bond_ifname,
                        "/agent:%s/aggregation:%s/interface:",
                        ta, name);
    if (rc != 0 || bond_ifname == NULL)
    {
        ERROR("Failed to obtain name of bond interface");
        return rc;
    }

    rc = tapi_cfg_base_if_down(ta, bond_ifname);
    if (rc != 0)
    {
        ERROR("Failed to bring bond interface down");
        free(bond_ifname);
        return rc;
    }

    rc = cfg_find_fmt(&aggr_handle, "/agent:%s/aggregation:%s",
                      ta, name);
    if (rc != 0)
    {
        ERROR("Failed to find aggregation node");
        free(bond_ifname);
        return rc;
    }

    cfg_del_instance(aggr_handle, FALSE);

    rc = cfg_find_fmt(&rsrc_handle, "/agent:%s/rsrc:%s",
                      ta, bond_ifname);
    if (rc != 0)
    {
        ERROR("Failed to find rsrc node");
        free(bond_ifname);
        return rc;
    }

    cfg_del_instance(rsrc_handle, FALSE);

    free(bond_ifname);
    return 0;
}

/* See the description in tapi_cfg_aggr.h */
int
tapi_cfg_aggr_bond_enslave(const char *ta, const char *name,
                           const char *slave_if)
{
    int             rc = 0;
    cfg_val_type    val_type;
    char           *bond_ifname = NULL;
    cfg_handle      slave_handle = CFG_HANDLE_INVALID;
    char            oid[CFG_OID_MAX];

    CHECK_BOND(ta, name);

    val_type = CVT_STRING;
    rc = cfg_get_instance_fmt(
                        &val_type, &bond_ifname,
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

    free(bond_ifname);
    return 0;
}

/* See the description in tapi_cfg_aggr.h */
int
tapi_cfg_aggr_bond_free_slave(const char *ta, const char *name,
                              const char *slave_if)
{
    int             rc = 0;

    CHECK_BOND(ta, name);

    rc = cfg_del_instance_fmt(FALSE,
                              "/agent:%s/aggregation:%s/member:%s",
                              ta, name, slave_if);
    if (rc != 0)
    {
        ERROR("Failed to release slave interface");
        return rc;
    }

    return 0;
}
