/** @file
 * @brief Test API to storage server routines
 *
 * Generic server functions for storage server.
 *
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 * @author Ivan Melnikov <Ivan.Melnikov@oktetlabs.ru>
 *
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
    te_errno rc = 0;

    assert(server != NULL);

    server->type = type;
    server->rpcs = rpcs;
    server->methods = methods;
    server->context = context;

    if (auth == NULL)
        server->auth = (tapi_storage_auth_params)TAPI_STORAGE_AUTH_INIT;
    else
        rc = tapi_storage_auth_params_copy(&server->auth, auth);

    return rc;
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
