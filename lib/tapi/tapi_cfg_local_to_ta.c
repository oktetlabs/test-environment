/** @file
 * @brief Test API to access Configurator
 *
 * Implementation
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

#define TE_LGR_USER     "Configuration TAPI"

#include "te_config.h"

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#endif
#if HAVE_ASSERT_H
#include <assert.h>
#endif

#include "te_defs.h"
#include "te_errno.h"
#include "te_stdint.h"
#include "conf_api.h"
#include "logger_api.h"

#include "tapi_cfg.h"


/* See the description in tapi_cfg.h */
te_errno
tapi_cfg_env_local_to_agent(void)
{
    const char * const  pattern = "/local:*/env:*";

    te_errno        rc;
    unsigned int    num;
    cfg_handle     *handles = NULL;
    unsigned int    i;
    cfg_oid        *oid = NULL;
    cfg_val_type    type = CVT_STRING;
    char           *new_value = NULL;
    char           *old_value = NULL;

    rc = cfg_find_pattern(pattern, &num, &handles);
    if (TE_RC_GET_ERROR(rc) == TE_ENOENT)
    {
        return 0;
    }
    else if (rc != 0)
    {
        ERROR("Failed to find by pattern '%s': %r", pattern, rc);
        return rc;
    }

    for (i = 0; i < num; ++i)
    {
        rc = cfg_get_instance(handles[i], &type, &new_value);
        if (rc != 0)
        {
            ERROR("%s(): cfg_get_instance() failed for #%u: %r",
                  __FUNCTION__, i, rc);
            break;
        }
        rc = cfg_get_oid(handles[i], &oid);
        if (rc != 0)
        {
            ERROR("%s(): cfg_get_oid() failed for #%u: %r",
                  __FUNCTION__, i, rc);
            break;
        }
        rc = cfg_get_instance_fmt(&type, &old_value,
                                  "/agent:%s/env:%s",
                                  CFG_OID_GET_INST_NAME(oid, 1),
                                  CFG_OID_GET_INST_NAME(oid, 2));
        if (rc == 0)
        {
            if (strcmp(new_value, "") == 0)
                rc = cfg_del_instance_fmt(FALSE, "/agent:%s/env:%s",
                                          CFG_OID_GET_INST_NAME(oid, 1),
                                          CFG_OID_GET_INST_NAME(oid, 2));

            else if (strcmp(new_value, old_value) != 0)
                rc = cfg_set_instance_fmt(type, new_value,
                                          "/agent:%s/env:%s",
                                          CFG_OID_GET_INST_NAME(oid, 1),
                                          CFG_OID_GET_INST_NAME(oid, 2));
        }
        else if (TE_RC_GET_ERROR(rc) == TE_ENOENT)
        {
            if (strcmp(new_value, "") != 0)
                rc = cfg_add_instance_fmt(NULL, type, new_value,
                                          "/agent:%s/env:%s",
                                          CFG_OID_GET_INST_NAME(oid, 1),
                                          CFG_OID_GET_INST_NAME(oid, 2));
            else
                rc = 0;
        }
        if (rc != 0)
        {
            ERROR("%s(): Failed on #%u: %r", __FUNCTION__, i, rc);
            break;
        }
        free(new_value); new_value = NULL;
        cfg_free_oid(oid); oid = NULL;
        free(old_value); old_value = NULL;
    }

    free(new_value);
    cfg_free_oid(oid);
    free(old_value);
    free(handles);

    return rc;
}

/* See the description in tapi_cfg.h */
te_errno
tapi_cfg_rpcs_local_to_agent(void)
{
    const char * const  pattern = "/local:*/rpcserver:*";

    te_errno        rc;
    unsigned int    num;
    cfg_handle     *handles = NULL;
    unsigned int    i;
    cfg_oid        *oid = NULL;
    cfg_val_type    type = CVT_STRING;
    char           *new_value = NULL;

    rc = cfg_find_pattern(pattern, &num, &handles);
    if (TE_RC_GET_ERROR(rc) == TE_ENOENT)
    {
        return 0;
    }
    else if (rc != 0)
    {
        ERROR("Failed to find by pattern '%s': %r", pattern, rc);
        return rc;
    }

    for (i = 0; i < num; ++i)
    {
        rc = cfg_get_instance(handles[i], &type, &new_value);
        if (rc != 0)
        {
            ERROR("%s(): cfg_get_instance() failed for #%u: %r",
                  __FUNCTION__, i, rc);
            break;
        }
        rc = cfg_get_oid(handles[i], &oid);
        if (rc != 0)
        {
            ERROR("%s(): cfg_get_oid() failed for #%u: %r",
                  __FUNCTION__, i, rc);
            break;
        }
        rc = cfg_add_instance_fmt(NULL, type, new_value,
                                  "/agent:%s/rpcserver:%s",
                                  CFG_OID_GET_INST_NAME(oid, 1),
                                  CFG_OID_GET_INST_NAME(oid, 2));
        if (rc != 0)
        {
            ERROR("%s(): Failed on #%u: %r", __FUNCTION__, i, rc);
            break;
        }
        free(new_value); new_value = NULL;
        cfg_free_oid(oid); oid = NULL;
    }

    free(new_value);
    cfg_free_oid(oid);
    free(handles);

    return rc;
}
