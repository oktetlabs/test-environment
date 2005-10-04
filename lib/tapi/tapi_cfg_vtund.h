/** @file
 * @brief Test API to configure VTund.
 *
 * Definition of API to configure VTund.
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

#ifndef __TE_TAPI_CFG_VTUND_H__
#define __TE_TAPI_CFG_VTUND_H__

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#include "te_errno.h"
#include "conf_api.h"


#ifdef __cplusplus
extern "C" {
#endif


/**
 * Create a tunnel between two hosts.
 *
 * @param ta_srv        Test Agent with VTund server
 * @param ta_clnt       Test Agent with VTund client
 * @param srv_addr      Address and port for VTund server to listen to
 *                      and for VTund client to connect to (if port is 0,
 *                      default VTund port is used)
 * @param ta_srv_if     Configurator handle of the created interface 
 *                      on the Test Agent with VTund server
 * @param ta_clnt_if    Configurator handle of the created interface 
 *                      on the Test Agent with VTund client
 * 
 * @return Status code
 */
extern te_errno tapi_cfg_vtund_create_tunnel(
                    const char            *ta_srv,
                    const char            *ta_clnt,
                    const struct sockaddr *srv_addr,
                    cfg_handle            *ta_srv_if,
                    cfg_handle            *ta_clnt_if);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_CFG_VTUND_H__ */
