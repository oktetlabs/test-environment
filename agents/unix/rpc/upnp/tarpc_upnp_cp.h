/** @file
 * @brief UPnP Control Point process
 *
 * UPnP Control Point proxy functions definition.
 *
 *
 * Copyright (C) 2016 Test Environment authors (see file AUTHORS
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
 * @author Ivan Melnikov <Ivan.Melnikov@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TARPC_UPNP_CP_H__
#define __TARPC_UPNP_CP_H__

#include "tarpc.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Create the UNIX socket and perform connection with UPnP Control Point.
 *
 * @param in    TA RPC parameter containing no input value.
 *
 * @return On success, @c 0. On error, @c -1, and errno is set
 *         appropriately.
 */
extern int upnp_cp_connect(tarpc_upnp_cp_connect_in *in);

/**
 * Perform disconnection from UPnP Control Point and destroy the UNIX
 * socket.
 *
 * @param in    TA RPC parameter containing no input value.
 *
 * @return On success, @c 0. On error, @c -1, and errno is set
 *         appropriately.
 */
extern int upnp_cp_disconnect(tarpc_upnp_cp_disconnect_in *in);

/**
 * Transmit a message from TEN to UPnP Control Point over UNIX socket and
 * back.
 *
 * @param[in]  in       TA RPC parameter containing request.
 * @param[out] out      TA RPC parameter containing reply.
 *
 * @return Status code.
 */
extern int upnp_cp_action(tarpc_upnp_cp_action_in  *in,
                          tarpc_upnp_cp_action_out *out);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TARPC_UPNP_CP_H__ */
