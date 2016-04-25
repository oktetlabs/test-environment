/** @file
 * @brief Test API to storage server routines
 *
 * Generic server functions for storage server.
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

#define TE_LGR_USER     "TAPI Storage Server"

#include "tapi_storage_server.h"

#ifdef HAVE_ASSERT_H
#include <assert.h>
#endif


/* See description in tapi_storage_server.h. */
te_errno
tapi_storage_server_init(tapi_storage_service_type          type,
                         rcf_rpc_server                    *rpcs,
                         const tapi_storage_server_methods *methods,
                         tapi_storage_auth_params          *auth,
                         void                              *context,
                         tapi_storage_server               *server)
{
    assert(server != NULL);

    if (auth == NULL)
        return TE_RC(TE_TAPI, TE_EINVAL);

    server->type = type;
    server->rpcs = rpcs;
    server->methods = methods;
    server->context = context;
    return tapi_storage_auth_params_copy(&server->auth, auth);
}

/* See description in tapi_storage_server.h. */
void
tapi_storage_server_fini(tapi_storage_server *server)
{
    server->type = TAPI_STORAGE_SERVICE_UNSPECIFIED;
    server->rpcs = NULL;
    server->methods = NULL;
    server->context = NULL;
    tapi_storage_auth_params_fini(&server->auth);
}
