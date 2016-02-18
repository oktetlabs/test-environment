/** @file
 * @brief ip rule Configuration Model TAPI
 *
 *
 * Copyright (C) 2015 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
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
 * @author Oleg Sadakov <Oleg.Sadakov@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER     "Configuration TAPI"

#include "tapi_cfg_ip_rule.h"

#include "logger_api.h"
#include "te_errno.h"

/**
 * Get instance name from configuration tree and parse it into @p rule
 *
 * @param [in] handle       Object of instance handle
 * @param [out]required     Pointer to required fields
 * @param [out]rule         Pointer to structure @b te_conf_ip_rule
 *
 * @return                  Status code
 *
 * @note                    In special case when instance name is equal "*",
 *                          function returns a field @p required equal to
 *                          @c 0 and status code equal to @c 0.
 */
static te_errno
get_ip_rule(cfg_handle handle, uint32_t *required, te_conf_ip_rule *rule)
{
    te_errno    rc;
    char       *data;

    *required = 0;
    if ((rc = cfg_get_inst_name(handle, &data)) != 0)
        return rc;
    if (strcmp(data, "*") != 0)
        rc = te_conf_ip_rule_from_str(data, required, rule);
    free(data);
    return rc;
}


/* See the description in tapi_cfg.h */
te_errno tapi_cfg_get_rule_table(const char *ta, int addr_family,
                                 tapi_rt_ip_rule_entry **tbl,
                                 unsigned int *n)
{
    te_errno                rc;
    cfg_handle             *handles;
    unsigned int            i;
    unsigned int            size;
    uint32_t                required;
    tapi_rt_ip_rule_entry  *tbl_ptr;
    tapi_rt_ip_rule_entry  *local_tbl;

    UNUSED(addr_family);

    *tbl = NULL;
    *n = 0;

    if (ta == NULL || tbl == NULL || n == NULL)
        return TE_RC(TE_TAPI, TE_EINVAL);

    rc = cfg_find_pattern_fmt(&size, &handles, "/agent:%s/rule:*", ta);
    if (rc != 0)
    {
        ERROR("%s: Cannot get the list of all rules on the test agent %s",
              __FUNCTION__, ta);
        return rc;
    }

    if (size == 0)
        return 0;

    local_tbl = (tapi_rt_ip_rule_entry *)calloc(size, sizeof(*local_tbl));
    if (local_tbl == NULL)
    {
        free(handles);
        return TE_RC(TE_TAPI, TE_ENOMEM);
    }

    tbl_ptr = local_tbl;
    for (i = 0; i < size; i++)
    {
        rc = get_ip_rule(handles[i], &required, &tbl_ptr->entry);
        if (rc)
        {
            ERROR("%s: Cannot obtain rule instance name (%r)",
                  __FUNCTION__, rc);
            break;
        }
        if (required == 0)
            continue;

        tbl_ptr->hndl = handles[i];
        tbl_ptr++;
    }
    free(handles);

    if (rc != 0)
    {
        free(local_tbl);
        return rc;
    }

    *tbl = local_tbl;
    *n = tbl_ptr - local_tbl;

    return 0;
}

/* See the description in tapi_cfg.h */
te_errno tapi_cfg_add_rule(const char *ta, int addr_family,
                           te_conf_ip_rule *ip_rule)
{
    te_errno     rc;
    char        *name;
    cfg_handle   handle;

    UNUSED(addr_family);

    rc = te_conf_ip_rule_to_str(ip_rule, &name);
    if (rc != 0)
    {
        ERROR("%s: Cannot convert ip_rule to string (%r)",
              __FUNCTION__, rc);
        return rc;
    }

    rc = cfg_add_instance_fmt(&handle, CFG_VAL(NONE, NULL),
                              "/agent:%s/rule:%s", ta, name);
    if (rc != 0)
    {
        ERROR("%s: Failed to add rule node '/agent:%s/rule:%s' (%r)",
              __FUNCTION__, ta, name, rc);
        free(name);
        return rc;
    }
    free(name);

    rc = cfg_synchronize_fmt(TRUE, "/agent:%s/rule:*", ta);
    if (rc != 0)
        ERROR("%s: Failed to synchronize '/agent:%s/rule:*' (%r)",
              __FUNCTION__, ta, rc);

    return rc;
}

/* See the description in tapi_cfg.h */
te_errno
tapi_cfg_del_rule(const char *ta, int addr_family, uint32_t required,
                  te_conf_ip_rule *ip_rule)
{
    te_errno                    rc;
    te_conf_obj_compare_result  cmp;
    te_conf_ip_rule             local_rule;
    unsigned int                i;
    unsigned int                num;
    cfg_handle                 *handles;
    uint32_t                    local_required;

    UNUSED(addr_family);

    rc = cfg_find_pattern_fmt(&num, &handles, "/agent:%s/rule:*", ta);
    if (rc != 0)
    {
        ERROR("%s: Cannot get the list of all rules on the test agent %s",
              __FUNCTION__, ta);
        return rc;
    }

    for (i = 0; i < num; i++)
    {
        rc = get_ip_rule(handles[i], &local_required, &local_rule);
        if (rc != 0)
        {
            ERROR("%s: Cannot obtain rule instance name (%r)",
                  __FUNCTION__, rc);
            break;
        }
        if (local_required == 0)
            continue;

        cmp = te_conf_ip_rule_compare(required, ip_rule, &local_rule);
        if (cmp == TE_CONF_OBJ_COMPARE_EQUAL ||
            cmp == TE_CONF_OBJ_COMPARE_CONTAINS)
        {
            rc = cfg_del_instance(handles[i], FALSE);
            if (rc != 0)
                ERROR("%s: Cannot delete a rule (%r)", __FUNCTION__, rc);
            break;
        }
    }
    free(handles);

    if (i == num)
    {
        ERROR("%s: Cannot find a rule for removal", __FUNCTION__);
        return TE_RC(TE_TAPI, TE_ENOENT);
    }
    return rc;
}
