/** @file
 * @brief Test API to configure DHCP.
 *
 * Implementation of API to configure DHCP.
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
 *
 * $Id$
 */

#define TE_LGR_USER     "TAPI CFG DHCP"

#if HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef STDC_HEADERS
#include <stdlib.h>
#include <string.h>
#endif

#include "te_defs.h"
#include "logger_api.h"
#include "tapi_cfg_dhcp.h"


/** Format string for DHCP server object instance */
#define TE_CFG_TA_DHCP_SERVER_FMT   "/agent:%s/dhcpserver:"


/* See description in tapi_cfg_dhcp.h */
int
tapi_cfg_dhcps_add_host(const char            *ta,
                        const char            *name,
                        const char            *group,
                        const struct sockaddr *chaddr,
                        const void            *client_id,
                        int                    client_id_len,
                        const struct sockaddr *fixed_ip,
                        const char            *next_server,
                        const char            *filename,
                        cfg_handle            *handle)
{
    int             rc;
    char            autoname[16];
    char           *str;
    unsigned int    i;

    if (ta == NULL || chaddr == NULL ||
        (client_id != NULL && client_id_len != -1 && client_id_len <= 0))
    {
        ERROR("%s(): Invalid argument", __FUNCTION__);
        return TE_RC(TE_TAPI, EINVAL);
    }

    /* Generate unique name */
    if (name == NULL)
    {
        unsigned int    n;
        cfg_handle     *hs = NULL;
        unsigned int    host_i = 0;

        rc = cfg_find_pattern_fmt(&n, &hs,
                                  TE_CFG_TA_DHCP_SERVER_FMT "/host:*",
                                  ta);
        if (rc != 0)
        {
            ERROR("%s(): Failed to find by pattern '%s' for TA '%s': %X",
                  __FUNCTION__, TE_CFG_TA_DHCP_SERVER_FMT "/host:*",
                  ta, rc);
            return rc;
        }
        
        do {
            snprintf(autoname, sizeof(autoname), "host%u", ++host_i);

            for (i = 0; i < n; ++i)
            {
                rc = cfg_get_inst_name(hs[i], &str);
                if (rc != 0)
                {
                    ERROR("%s(): Failed to get instance name by handle "
                          "0x%x: %X", __FUNCTION__, hs[i], rc);
                    free(hs);
                    return rc;
                }
                if (strcmp(str, autoname) == 0)
                {
                    free(str);
                    break;
                }
                free(str);
            }
        } while (i < n);

        name = autoname;
        INFO("%s(): Automatically selected DHCP host configuration "
             "name is '%s'", __FUNCTION__, name);
    }

    /* Add host configuration entry */
    rc = cfg_add_instance_fmt(handle, CVT_NONE, NULL,
                              TE_CFG_TA_DHCP_SERVER_FMT "/host:%s",
                              ta, name);
    if (rc != 0)
    {
        /* Failure is logged in Configurator */
        return rc;
    }

    /* Do local set of specified parameters */

    if (group != NULL)
    {
        rc = cfg_set_instance_local_fmt(CVT_STRING, group,
                 TE_CFG_TA_DHCP_SERVER_FMT "/host:%s/group:",
                 ta, name);
        if (rc != 0)
            return rc;
    }

    rc = cfg_set_instance_local_fmt(CVT_ADDRESS, chaddr,
             TE_CFG_TA_DHCP_SERVER_FMT "/host:%s/chaddr:",
             ta, name);
    if (rc != 0)
        return rc;

    if (client_id != NULL)
    {
        if (client_id_len == -1)
        {
            rc = cfg_set_instance_local_fmt(CVT_STRING, client_id,
                     TE_CFG_TA_DHCP_SERVER_FMT "/host:%s/client-id:",
                     ta, name);
            if (rc != 0)
                return rc;
        }
        else
        {
            char cid_str[client_id_len * 3];

            assert(client_id_len > 0);
            for (i = 0, str = cid_str;
                 i < (unsigned int)client_id_len;
                 ++i, str += 3)
            {
                sprintf(str, "%02x%s", ((uint8_t *)client_id)[i],
                        (i == (unsigned int)(client_id_len - 1)) ?
                            "" : ":");
            }

            rc = cfg_set_instance_local_fmt(CVT_STRING, cid_str,
                     TE_CFG_TA_DHCP_SERVER_FMT "/host:%s/client-id:",
                     ta, name);
            if (rc != 0)
                return rc;
        }
    }

    if (fixed_ip != NULL)
    {
        rc = cfg_set_instance_local_fmt(CVT_ADDRESS, chaddr,
                 TE_CFG_TA_DHCP_SERVER_FMT "/host:%s/ip-address:",
                 ta, name);
        if (rc != 0)
            return rc;
    }

    if (next_server != NULL)
    {
        rc = cfg_set_instance_local_fmt(CVT_STRING, next_server,
                 TE_CFG_TA_DHCP_SERVER_FMT "/host:%s/next:",
                 ta, name);
        if (rc != 0)
            return rc;
    }

    if (filename != NULL)
    {
        rc = cfg_set_instance_local_fmt(CVT_STRING, filename,
                 TE_CFG_TA_DHCP_SERVER_FMT "/host:%s/file:",
                 ta, name);
        if (rc != 0)
            return rc;
    }

    rc = cfg_commit_fmt(TE_CFG_TA_DHCP_SERVER_FMT "/host:%s", ta, name);
    if (rc != 0)
    {
        ERROR("%s(): Failed to commit changes: %X", __FUNCTION__, rc);
        return rc;
    }

    return 0;
}

