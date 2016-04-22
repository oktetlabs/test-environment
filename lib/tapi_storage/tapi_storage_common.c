/** @file
 * @brief Common test API to both client and server storage routines
 *
 * Common routines for both client and server storage services.
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

#define TE_LGR_USER     "TAPI Storage Common"

#include "tapi_storage_common.h"

#include "te_defs.h"
#include "logger_api.h"
#include "te_alloc.h"
#include "tapi_sockaddr.h"


/* See description in tapi_storage_common.h. */
te_errno
tapi_storage_auth_params_init(const struct sockaddr    *server_addr,
                              uint16_t                  port,
                              const char               *user,
                              const char               *password,
                              tapi_storage_auth_params *auth_params)
{
    te_errno rc = 0;

    auth_params->server_addr = NULL;
    auth_params->user = NULL;
    auth_params->password = NULL;
    do {
        /* Server address. */
        if (server_addr != NULL)
        {
            auth_params->server_addr =
                TE_ALLOC(sizeof(struct sockaddr_storage));
            if (auth_params->server_addr == NULL)
            {
                rc = TE_RC(TE_TAPI, TE_ENOMEM);
                break;
            }
            tapi_sockaddr_clone_exact(server_addr,
                                      SS(auth_params->server_addr));
            /* Save the port. */
            te_sockaddr_set_port(auth_params->server_addr, htons(port));
        }
        /* Server service port. */
        auth_params->port = port;
        /* User name. */
        if (user != NULL)
        {
            auth_params->user = strdup(user);
            if (auth_params->user == NULL)
            {
                rc = TE_RC(TE_TAPI, TE_ENOMEM);
                break;
            }
        }
        /* User password. */
        if (password != NULL)
        {
            auth_params->password = strdup(password);
            if (auth_params->password == NULL)
                rc = TE_RC(TE_TAPI, TE_ENOMEM);
        }
    } while (0);

    if (rc != 0)
        tapi_storage_auth_params_fini(auth_params);
    return rc;
}

/* See description in tapi_storage_common.h. */
void
tapi_storage_auth_params_fini(tapi_storage_auth_params *auth_params)
{
    free(auth_params->server_addr);
    free(auth_params->user);
    free(auth_params->password);
    auth_params->server_addr = NULL;
    auth_params->user = NULL;
    auth_params->password = NULL;
}

/* See description in tapi_storage_common.h. */
te_errno
tapi_storage_auth_params_copy(tapi_storage_auth_params *to,
                              const tapi_storage_auth_params *from)
{
    return tapi_storage_auth_params_init(from->server_addr, from->port,
                                         from->user, from->password, to);
}
