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
#ifndef __TAPI_STORAGE_SERVER_H__
#define __TAPI_STORAGE_SERVER_H__

#include "tapi_storage_common.h"


#ifdef __cplusplus
extern "C" {
#endif

/**
 * Forward declaration of generic server structure.
 */
struct tapi_storage_server;
typedef struct tapi_storage_server tapi_storage_server;


/**
 * Enable a storage server.
 *
 * @param server       Server handle.
 *
 * @return Status code.
 */
typedef te_errno (* tapi_storage_server_method_enable)(
                                        tapi_storage_server *server);

/**
 * Disable a storage server.
 *
 * @param server       Server handle.
 *
 * @return Status code.
 */
typedef te_errno (* tapi_storage_server_method_disable)(
                                       tapi_storage_server *server);

/**
 * Check if server enabled or not.
 *
 * @param server       Server handle.
 *
 * @return @c TRUE if server is enabled.
 */
typedef te_bool (* tapi_storage_server_method_is_enabled)(
                                       tapi_storage_server *server);

/**
 * Add a storage to the share, i.e. it looks for storage with specified
 * name, gets appropriate mount point and adds the last one to the share
 * list.
 *
 * @param server        Server handle.
 * @param storage_name  Name of the storage.
 *
 * @return Status code.
 */
typedef te_errno (* tapi_storage_server_method_add_storage)(
                                       tapi_storage_server *server,
                                       const char          *storage_name);

/**
 * Add a directory to storage share.
 *
 * @param server        Server handle.
 * @param directory     Directory pathname to be shared.
 *
 * @return Status code.
 */
typedef te_errno (* tapi_storage_server_method_add_share)(
                                       tapi_storage_server *server,
                                       const char          *directory);

/**
 * Delete directory from storage sharing.
 *
 * @param server    Server handle.
 * @param directory Directory pathname to be removed from the sharing list.
 *
 * @return Status code.
 */
typedef te_errno (* tapi_storage_server_method_del_share)(
                                       tapi_storage_server *server,
                                       const char          *directory);

/**
 * Get shared directories list. @p directories allocated by this function
 * and should be freed by user when it is no longer needed.
 *
 * @param[in]  server       Server handle.
 * @param[out] directories  Allocated array with directories list.
 * @param[out] len          Array length.
 *
 * @return Status code.
 */
typedef te_errno (* tapi_storage_server_method_get_share)(
                                       tapi_storage_server  *server,
                                       const char          **directories,
                                       size_t               *len);

/**
 * Methods to operate the server.
 */
typedef struct tapi_storage_server_methods {
    tapi_storage_server_method_enable      enable;
    tapi_storage_server_method_disable     disable;
    tapi_storage_server_method_is_enabled  is_enabled;
    tapi_storage_server_method_add_storage add_storage;
    tapi_storage_server_method_add_share   add_share;
    tapi_storage_server_method_del_share   del_share;
    tapi_storage_server_method_get_share   get_share;
} tapi_storage_server_methods;

/**
 * Generic structure which provides set of operations to manage a storage
 * service independently on agent back-end.
 */
struct tapi_storage_server {
    rcf_rpc_server                    *rpcs;    /**< RPC server handle. */
    tapi_storage_service_type          type;    /**< Type of server. */
    const tapi_storage_server_methods *methods; /**< Methods to operate the
                                                     server. */
    tapi_storage_auth_params           auth;    /**< Authorization
                                                     parameters. */
    void                              *context; /**< Server context. */
};


/**
 * Enable a storage server.
 * Server should be disabled with @b tapi_storage_server_disable when it is
 * no longer needed.
 *
 * @param server        Server handle.
 *
 * @return Status code.
 *
 * @sa tapi_storage_server_disable
 */
static inline te_errno
tapi_storage_server_enable(tapi_storage_server *server)
{
    return (server->methods->enable != NULL
            ? server->methods->enable(server) : TE_EOPNOTSUPP);
}

/**
 * Disable a storage server that was enabled with
 * @b tapi_storage_server_enable.
 *
 * @param server        Server handle.
 *
 * @return Status code.
 *
 * @sa tapi_storage_server_enable
 */
static inline te_errno
tapi_storage_server_disable(tapi_storage_server *server)
{
    return (server->methods->disable != NULL
            ? server->methods->disable(server) : TE_EOPNOTSUPP);
}

/**
 * Check if server enabled or not.
 *
 * @param server       Server handle.
 *
 * @return @c TRUE if server is enabled.
 */
static inline te_bool
tapi_storage_server_is_enabled(tapi_storage_server *server)
{
    return (server->methods->is_enabled != NULL
            ? server->methods->is_enabled(server) : TE_EOPNOTSUPP);
}

/**
 * Add a storage to the share, i.e. it looks for storage with specified
 * name, gets appropriate mount point and adds the last one to the share
 * list.
 *
 * @param server        Server handle.
 * @param storage_name  Name of the storage.
 *
 * @return Status code.
 */
static inline te_errno
tapi_storage_server_add_storage(tapi_storage_server *server,
                                const char          *storage_name)
{
    return (server->methods->add != NULL
            ? server->methods->add_storage(server, storage_name)
            : TE_EOPNOTSUPP);
}

/**
 * Add a directory to storage share.
 *
 * @param server        Server handle.
 * @param directory     Directory pathname to be shared.
 *
 * @return Status code.
 */
static inline te_errno
tapi_storage_server_add_share(tapi_storage_server *server,
                              const char          *directory)
{
    return (server->methods->add_share != NULL
            ? server->methods->add_share(server, directory)
            : TE_EOPNOTSUPP);
}

/**
 * Delete directory from storage sharing.
 *
 * @param server    Server handle.
 * @param directory Directory pathname to be removed from the sharing list.
 *
 * @return Status code.
 */
static inline te_errno
tapi_storage_server_del_share(tapi_storage_server *server,
                              const char          *directory)
{
    return (server->methods->del_share != NULL
            ? server->methods->del_share(server, directory)
            : TE_EOPNOTSUPP);
}

/**
 * Get shared directories list. @p directories allocated by this function
 * and should be freed by user when it is no longer needed.
 *
 * @param[in]  server       Server handle.
 * @param[out] directories  Allocated array with directories list.
 * @param[out] len          Array length.
 *
 * @return Status code.
 */
static inline te_errno
tapi_storage_server_get_share(tapi_storage_server  *server,
                              const char          **directories,
                              size_t               *len)
{
    return (server->methods->get_share != NULL
            ? server->methods->get_share(server, directories, len)
            : TE_EOPNOTSUPP);
}

/**
 * Initialize server handle.
 * Server should be released with @b tapi_storage_server_fini when it is no
 * longer needed.
 *
 * @param[in]  type         Back-end server type.
 * @param[in]  rpcs         RPC server handle.
 * @param[in]  methods      Back-end server specific methods.
 * @param[in]  auth         Back-end server specific authorization
 *                          parameters.
 * @param[in]  context      Back-end server specific context.
 * @param[out] server       Server handle.
 *
 * @return Status code.
 *
 * @sa tapi_storage_server_fini
 */
extern te_errno tapi_storage_server_init(
                                tapi_storage_service_type          type,
                                rcf_rpc_server                    *rpcs,
                                const tapi_storage_server_methods *methods,
                                tapi_storage_auth_params          *auth,
                                void                              *context,
                                tapi_storage_server               *server);

/**
 * Release server that was initialized with @b tapi_storage_server_init.
 *
 * @param server        Server handle.
 *
 * @sa tapi_storage_server_init
 *
 * @todo How to free context?
 */
extern void tapi_storage_server_fini(tapi_storage_server *server);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TAPI_STORAGE_SERVER_H__ */
