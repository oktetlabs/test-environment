/** @file
 * @brief Test API to configure PPPoE.
 *
 * Implementation of API to configure PPPoE.
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
 * @author Igor Labutin <Igor.Labutin@oktetlabs.ru>
 *
 * $Id: tapi_cfg_dhcp.c 32571 2006-10-17 13:43:12Z edward $
 */

#define TE_LGR_USER     "TAPI CFG PPPOE"

#include "te_config.h"

#ifdef STDC_HEADERS
#include <stdlib.h>
#include <string.h>
#endif

#include "te_defs.h"
#include "conf_api.h"
#include "logger_api.h"
#include "tapi_cfg_base.h"
#include "tapi_cfg_pppoe.h"


/** Format string for DHCP server object instance */
#define TE_CFG_TA_PPPOE_SERVER_FMT   "/agent:%s/pppoeserver:"

/* See description in tapi_cfg_pppoe.h */
te_errno
tapi_cfg_pppoe_server_if_add(const char *ta, const char *ifname)
{
    cfg_handle      handle;

    /* Add subnet configuration entry */
    return cfg_add_instance_fmt(&handle, CFG_VAL(NONE, NULL),
                                TE_CFG_TA_PPPOE_SERVER_FMT "/interface:%s",
                                ta, ifname);
}

/* See description in tapi_cfg_pppoe.h */
te_errno
tapi_cfg_pppoe_server_if_del(const char *ta, const char *ifname)
{
    /* Delete subnet configuration entry */
    return cfg_del_instance_fmt(FALSE, TE_CFG_TA_PPPOE_SERVER_FMT
                                "/interface:%s", ta, ifname);
}


/* See description in tapi_cfg_pppoe.h */
te_errno
tapi_cfg_pppoe_server_subnet_set(const char *ta, const char *subnet)
{
    if (ta == NULL || subnet == NULL)
    {
        ERROR("%s(): Invalid argument", __FUNCTION__);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    /* Add subnet configuration entry */
    return cfg_set_instance_fmt(CFG_VAL(STRING, subnet),
                                TE_CFG_TA_PPPOE_SERVER_FMT "/subnet:", ta);
}

/* See description in tapi_cfg_pppoe.h */
te_errno
tapi_cfg_pppoe_server_subnet_get(const char *ta, const char **subnet_p)
{
    cfg_val_type type = CVT_STRING;
    int          rc;

    /* Add subnet configuration entry */
    if ((rc = cfg_get_instance_fmt(&type, subnet_p,
                                   TE_CFG_TA_PPPOE_SERVER_FMT "/subnet:",
                                   ta)) != 0)
    {
        ERROR("Failed to get pppoe server subnet: %r", rc);
    }

    return rc;
}
