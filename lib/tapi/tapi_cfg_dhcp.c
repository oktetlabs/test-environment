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

#include "te_config.h"

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
tapi_cfg_dhcps_add_subnet(const char            *ta,
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
                              TE_CFG_TA_DHCP_SERVER_FMT "/subnet:%s",
                              ta, str);
    free(str);
    if (rc != 0)
    {
        /* Failure is logged in Configurator */
        return rc;
    }

    return 0;
}

/* See description in tapi_cfg_dhcp.h */
int
tapi_cfg_dhcps_add_host_gen(const char            *ta,
                            const char            *name,
                            const char            *group,
                            const struct sockaddr *chaddr,
                            const void            *client_id,
                            int                    client_id_len,
                            const struct sockaddr *fixed_ip,
                            const char            *next_server,
                            const char            *filename,
                            const char            *flags,
                            const void            *host_id,
                            int                    host_id_len,
                            const char            *prefix6,
                            cfg_handle            *handle)
{
    int             rc;
    char            autoname[16];
    char           *str;
    unsigned int    i;

    if (ta == NULL ||
        (client_id != NULL && client_id_len != -1 && client_id_len <= 0) ||
        (host_id != NULL && host_id_len != -1 && host_id_len <= 0))
    {
        ERROR("%s(): Invalid argument", __FUNCTION__);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    /* Generate unique name 'hostXX' */
    if (name == NULL)
    {
        unsigned int    n;
        cfg_handle     *hs = NULL;
        unsigned int    host_i = 0;

        if ((rc = cfg_find_pattern_fmt(&n, &hs,
                                       TE_CFG_TA_DHCP_SERVER_FMT "/host:*",
                                       ta)) != 0)
        {
            ERROR("%s(): Failed to find by pattern '%s' for TA '%s': %r",
                  __FUNCTION__, TE_CFG_TA_DHCP_SERVER_FMT "/host:*",
                  ta, rc);
            return rc;
        }

        do {
            snprintf(autoname, sizeof(autoname), "host%u", ++host_i);

            for (i = 0; i < n; ++i)
            {
                if ((rc = cfg_get_inst_name(hs[i], &str)) != 0)
                {
                    ERROR("%s(): Failed to get instance name by handle "
                          "0x%x: %r", __FUNCTION__, hs[i], rc);
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
        free(hs);

        name = autoname;
        INFO("%s(): Automatically selected DHCP host configuration "
             "name is '%s'", __FUNCTION__, name);
    }

    /* Add host configuration entry */
    if ((rc = cfg_add_instance_fmt(handle, CFG_VAL(NONE, NULL),
                                   TE_CFG_TA_DHCP_SERVER_FMT "/host:%s",
                                   ta, name)) != 0)
    {
        /* Failure is logged in Configurator */
        return rc;
    }

    /* Do local set of specified parameters */
    if (group != NULL)
    {
        if ((rc = cfg_set_instance_fmt(CFG_VAL(STRING, group),
                            TE_CFG_TA_DHCP_SERVER_FMT "/host:%s/group:",
                            ta, name)) != 0)
        {
            return rc;
        }
    }

    if (chaddr != NULL)
    {
        if ((rc = cfg_types[CVT_ADDRESS].val2str(
                 (cfg_inst_val)(struct sockaddr *)chaddr, &str)) != 0)
        {
            ERROR("%s(): Failed to convert IP address to string: %r",
                  __FUNCTION__, rc);
            return rc;
        }
        rc = cfg_set_instance_fmt(CFG_VAL(STRING, str),
                 TE_CFG_TA_DHCP_SERVER_FMT "/host:%s/chaddr:",
                 ta, name);
        free(str);
        if (rc != 0)
            return rc;
    }

#define FILL_ID_PARAM(__param_name) \
    if (__param_name##_id != NULL)                                      \
    {                                                                   \
        if (__param_name##_id_len == -1)                                \
        {                                                               \
            if ((rc = cfg_set_instance_fmt(CFG_VAL(STRING,              \
                                              __param_name##_id),       \
                                      TE_CFG_TA_DHCP_SERVER_FMT         \
                                      "/host:%s/" #__param_name "-id:", \
                                      ta, name)) != 0)                  \
            {                                                           \
                return rc;                                              \
            }                                                           \
        }                                                               \
        else                                                            \
        {                                                               \
            char cid_str[__param_name##_id_len * 3];                    \
                                                                        \
            assert(__param_name##_id_len > 0);                          \
            for (i = 0, str = cid_str;                                  \
                 i < (unsigned int)__param_name##_id_len;               \
                 ++i, str += 3)                                         \
            {                                                           \
                sprintf(str, "%02x%s",                                  \
                        ((uint8_t *)__param_name##_id)[i],              \
                        (i ==                                           \
                            (unsigned int)(__param_name##_id_len - 1)) ?\
                                "" :                                    \
                                    ":");                               \
            }                                                           \
                                                                        \
            if ((rc = cfg_set_instance_fmt(CFG_VAL(STRING, cid_str),    \
                                    TE_CFG_TA_DHCP_SERVER_FMT           \
                                    "/host:%s/" #__param_name "-id:",   \
                                    ta, name)) != 0)                    \
            {                                                           \
                return rc;                                              \
            }                                                           \
        }                                                               \
    }
    FILL_ID_PARAM(client)
    FILL_ID_PARAM(host)
#undef FILL_ID_PARAM

    if (prefix6 != NULL)
    {
        if ((rc = cfg_set_instance_fmt(CFG_VAL(STRING, prefix6),
                 TE_CFG_TA_DHCP_SERVER_FMT "/host:%s/prefix6:",
                 ta, name)) != 0)
        {
            return rc;
        }
    }

    if (fixed_ip != NULL)
    {
        if ((rc = cfg_types[CVT_ADDRESS].val2str(
                 (cfg_inst_val)(struct sockaddr *)fixed_ip, &str)) != 0)
        {
            ERROR("%s(): Failed to convert IP address to string: %r",
                  __FUNCTION__, rc);
            return rc;
        }
        rc = cfg_set_instance_fmt(CFG_VAL(STRING, str),
                 TE_CFG_TA_DHCP_SERVER_FMT "/host:%s/ip-address:",
                 ta, name);
        free(str);
        if (rc != 0)
            return rc;
    }

    if (next_server != NULL)
    {
        if ((rc = cfg_set_instance_fmt(CFG_VAL(STRING, next_server),
                 TE_CFG_TA_DHCP_SERVER_FMT "/host:%s/next:",
                 ta, name)) != 0)
        {
            return rc;
        }
    }

    if (filename != NULL)
    {
        if ((rc = cfg_set_instance_fmt(CFG_VAL(STRING, filename),
                 TE_CFG_TA_DHCP_SERVER_FMT "/host:%s/file:",
                 ta, name)) != 0)
        {
            return rc;
        }
    }

    if (flags != NULL)
    {
        if ((rc = cfg_set_instance_fmt(CFG_VAL(STRING, flags),
                 TE_CFG_TA_DHCP_SERVER_FMT "/host:%s/flags:",
                 ta, name)) != 0)
            return rc;
    }

    return 0;
}

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
                        const char            *flags,
                        cfg_handle            *handle)
{
    return tapi_cfg_dhcps_add_host_gen(ta, name, group, chaddr,
                                       client_id, client_id_len,
                                       fixed_ip, next_server,
                                       filename, flags,
                                       NULL, 0,
                                       NULL, handle);
}
