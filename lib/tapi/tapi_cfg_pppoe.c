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
#include "logger_api.h"
#include "tapi_cfg_pppoe.h"


/** Format string for DHCP server object instance */
#define TE_CFG_TA_PPPOE_SERVER_FMT   "/agent:%s/pppoeserver:"


/* See description in tapi_cfg_pppoe.h */
int
tapi_cfg_pppoes_add_subnet(const char            *ta,
                           const struct sockaddr *subnet,
                           int                    prefix_len,
                           cfg_handle            *handle)
{
    int     rc;
    char   *str;

    if (ta == NULL || subnet == NULL)
    {
        ERROR("%s(): Invalid argument", __FUNCTION__);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    rc = cfg_types[CVT_ADDRESS].val2str(
             (cfg_inst_val)(struct sockaddr *)subnet, &str);
    if (rc != 0)
    {
        ERROR("%s(): Failed to convert subnet address to string: %r",
              __FUNCTION__, rc);
        return rc;
    }

    /* Add subnet configuration entry */
    rc = cfg_add_instance_fmt(handle, CFG_VAL(INTEGER, prefix_len),
                              TE_CFG_TA_PPPOE_SERVER_FMT "/subnet:%s",
                              ta, str);
    free(str);
    if (rc != 0)
    {
        /* Failure is logged in Configurator */
        return rc;
    }

    return 0;
}

