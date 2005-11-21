/** @file
 * @brief Test API to access configuration model
 *
 * Implementation of IPv6-related API
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
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER     "Configuration TAPI"

#include "te_config.h"

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_ASSERT_H
#include <assert.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#endif
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#include "te_defs.h"
#include "te_errno.h"
#include "te_stdint.h"
#include "logger_api.h"
#include "conf_api.h"
#include "rcf_api.h"

#include "tapi_sockaddr.h"
#include "tapi_cfg_ip6.h"


/* See description in tapi_cfg_ip6.h */
te_errno
tapi_cfg_ip6_get_linklocal_addr(const char *ta, const char *iface,
                                struct sockaddr_in6 *p_addr)
{
    te_errno        rc;
    unsigned int    num;
    cfg_handle     *handles = NULL;
    unsigned int    i;
    cfg_oid        *oid = NULL;

    struct sockaddr_in6 addr;


    if (ta == NULL || iface == NULL)
    {
        ERROR("%s(): Invalid argument", __FUNCTION__);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    rc = cfg_find_pattern_fmt(&num, &handles,
                              "/agent:%s/interface:%s/net_addr:*",
                              ta, iface);
    if (rc != 0)
        return rc;

    memset(&addr, 0, sizeof(addr));
    addr.sin6_family = AF_INET6;

    for (i = 0; i < num; ++i)
    {
        rc = cfg_get_oid(handles[i], &oid);
        if (rc != 0)
        {
            ERROR("%s(): cfg_get_oid() failed for #%u: %r",
                  __FUNCTION__, i, rc);
            break;
        }

        if (inet_pton(AF_INET6, CFG_OID_GET_INST_NAME(oid, 3),
                      &addr.sin6_addr) > 0 &&
            IN6_IS_ADDR_LINKLOCAL(&addr.sin6_addr))
        {
            break;
        }

        cfg_free_oid(oid); oid = NULL;
    }

    cfg_free_oid(oid);
    free(handles);

    if (i < num)
    {
        cfg_val_type    type = CVT_INTEGER;
        int             index;

        rc = cfg_get_instance_fmt(&type, &index,
                                  "/agent:%s/interface:%s/index:",
                                  ta, iface);
        if (rc != 0)
        {
            ERROR("Failed to get index of the interface '%s' on "
                  "TA '%s': %r", ta, iface, rc);
        }
        else
        {
            addr.sin6_scope_id = index;
            if (p_addr != NULL)
                memcpy(p_addr, &addr, sizeof(*p_addr));
            else
                rc = TE_RC(TE_TAPI, TE_EFAULT);
        }
    }
    else
    {
        rc = TE_RC(TE_TAPI, TE_EINVAL);
    }

    return rc;
}
