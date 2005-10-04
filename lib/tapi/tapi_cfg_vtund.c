/** @file
 * @brief Test API to configure VTund.
 *
 * Implementation of API to configure VTund.
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

#define TE_LGR_USER     "TAPI CFG VTund"

#include "te_config.h"

#ifdef STDC_HEADERS
#include <stdlib.h>
#include <string.h>
#endif

#include "te_defs.h"
#include "logger_api.h"
#include "tapi_sockaddr.h"
#include "tapi_cfg_vtund.h"


/** Default port for VTund server */
#define TAPI_CFG_VTUND_PORT_DEF     5000


/* See description in tapi_cfg_vtund.h */
te_errno
tapi_cfg_vtund_create_tunnel(const char            *ta_srv,
                             const char            *ta_clnt,
                             const struct sockaddr *srv_addr,
                             cfg_handle            *ta_srv_if,
                             cfg_handle            *ta_clnt_if)
{
    te_errno    rc;
    uint16_t    srv_port;

    srv_port = ntohs(sockaddr_get_port(srv_addr));
    if (srv_port == 0)
        srv_port = TAPI_CFG_VTUND_PORT_DEF;

    cfg_add_instance_fmt(NULL, CFG_VAL(INTEGER, 0),
                         "/agent:%s/vtund:/server:%u",
                         ta_srv, srv_port);

    cfg_add_instance_fmt(NULL, CFG_VAL(NONE, 0),
                         "/agent:%s/vtund:/server:%u/session:%s-%s",
                         ta_srv, srv_port, ta_srv, ta_clnt);

    cfg_add_instance_fmt(NULL, CFG_VAL(INTEGER, 0),
                         "/agent:%s/vtund:/client:%s-%s",
                         ta_clnt, ta_srv, ta_clnt);
    cfg_set_instance_fmt(CFG_VAL(ADDRESS, srv_addr),
                         "/agent:%s/vtund:/client:%s-%s/server:",
                         ta_clnt, ta_srv, ta_clnt);
    cfg_set_instance_fmt(CFG_VAL(INTEGER, srv_port),
                         "/agent:%s/vtund:/client:%s-%s/port:",
                         ta_clnt, ta_srv, ta_clnt);

    cfg_set_instance_fmt(CFG_VAL(INTEGER, 1),
                         "/agent:%s/vtund:/server:%u",
                         ta_srv, srv_port);
    cfg_set_instance_fmt(CFG_VAL(INTEGER, 1),
                         "/agent:%s/vtund:/client:%s-%s",
                         ta_clnt, ta_srv, ta_clnt);

    fprintf(stderr, "Sleeping...\n"); fflush(stderr);
    sleep(30);

    return rc;
}
