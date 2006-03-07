/** @file
 * @brief Unix Test Agent
 *
 * Unix TA routing configuring support using routing sockets interface.
 *
 * Copyright (C) 2006 Test Environment authors (see file AUTHORS
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
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id: conf.c 24823 2006-03-05 07:24:43Z arybchik $
 */

#ifdef USE_ROUTE_SOCKET

#define TE_LGR_USER     "Unix Conf Route Socket"

#include "te_config.h"
#if HAVE_CONFIG_H
#include <config.h>
#endif

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#if HAVE_NET_IF_H
#include <net/if.h>
#endif
#if HAVE_NET_ROUTE_H
#include <net/route.h>
#endif


/* See the description in conf_route.h */
te_errno
ta_unix_conf_route_find(ta_rt_info_t *rt_info)
{
    UNUSED(rt_info);
    return TE_RC(TE_TA_UNIX, TE_ENOSYS);
}

/* See the description in conf_route.h. */
te_errno
ta_unix_conf_route_change(ta_cfg_obj_action_e  action,
                          ta_rt_info_t        *rt_info)
{
    UNUSED(action);
    UNUSED(rt_info);
    return TE_RC(TE_TA_UNIX, TE_ENOSYS);
}

/* See the description in conf_route.h */
te_errno
ta_unix_conf_route_blackhole_list(char **list)
{
    *list = NULL;
    return 0;
}

/* See the description in conf_route.h */
te_errno
ta_unix_conf_route_blackhole_add(ta_rt_info_t *rt_info)
{
    UNUSED(rt_info);
    return TE_RC(TE_TA_UNIX, TE_ENOSYS);
}

/* See the description in conf_route.h */
te_errno 
ta_unix_conf_route_blackhole_del(ta_rt_info_t *rt_info)
{
    UNUSED(rt_info);
    return TE_RC(TE_TA_UNIX, TE_ENOSYS);
}


#endif /* USE_ROUTE_SOCKET */
