/** @file
 * @brief Generic test API to storage client routines
 *
 * Generic client functions for storage service.
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

#define TE_LGR_USER     "TAPI Storage Client"

#include "tapi_storage_client.h"
#include "tapi_storage_client_ftp.h"


/* See description in tapi_storage_client.h. */
te_errno
tapi_storage_client_init(tapi_storage_service_type          type,
                         rcf_rpc_server                    *rpcs,
                         const tapi_storage_client_methods *methods,
                         tapi_storage_auth_params          *auth,
                         void                              *context,
                         tapi_storage_client               *client)
{
    switch (type)
    {
        case TAPI_STORAGE_SERVICE_FTP:
            return tapi_storage_client_ftp_init(rpcs, methods, auth,
                                                context, client);

        case TAPI_STORAGE_SERVICE_SAMBA:
        case TAPI_STORAGE_SERVICE_DLNA:
            ERROR("%s:%d: Service type %u is not supported yet",
                  __FUNCTION__, __LINE__, type);
            return TE_RC(TE_TAPI, TE_ENOSYS);

        default:
            /* Service is not supported */
            ERROR("Unknown service type %u", type);
            return TE_RC(TE_TAPI, TE_EINVAL);
    }
}

/* See description in tapi_storage_client.h. */
void
tapi_storage_client_fini(tapi_storage_client *client)
{
    switch (client->type)
    {
        case TAPI_STORAGE_SERVICE_FTP:
            tapi_storage_client_ftp_fini(client);
            break;

        case TAPI_STORAGE_SERVICE_SAMBA:
        case TAPI_STORAGE_SERVICE_DLNA:
            ERROR("%s:%d: Service type %u is not supported yet",
                  __FUNCTION__, __LINE__, client->type);
            break;

        case TAPI_STORAGE_SERVICE_UNSPECIFIED:
            break;
    }
}
