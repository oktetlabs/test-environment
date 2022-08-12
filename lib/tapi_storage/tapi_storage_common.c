/** @file
 * @brief Common test API to both client and server storage routines
 *
 * Common routines for both client and server storage services.
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

#define TE_LGR_USER     "TAPI Storage Common"

#include "tapi_storage_common.h"

#include "te_defs.h"
#include "logger_api.h"
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
            /* Save address. */
            rc = tapi_sockaddr_clone2(server_addr,
                                      &auth_params->server_addr);
            if (rc != 0)
                break;
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
    assert(from != NULL);
    return tapi_storage_auth_params_init(from->server_addr, from->port,
                                         from->user, from->password, to);
}
