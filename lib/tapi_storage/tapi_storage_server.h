/** @file
 * @brief Test API to storage server routines
 *
 * @defgroup tapi_storage_server Test API to control the storage server
 * @ingroup tapi_storage
 * @{
 *
 * Generic server functions for storage server.
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
 *
 *
 * @author Ivan Melnikov <Ivan.Melnikov@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TAPI_STORAGE_SERVER_H__
#define __TAPI_STORAGE_SERVER_H__

#include "tapi_storage_common.h"
#include "tapi_storage_share.h"
#include "tapi_local_storage_device.h"
#include "rcf_rpc.h"


#ifdef __cplusplus
extern "C" {
#endif


/** On-stack tapi_storage_server structure initializer. */
#define TAPI_STORAGE_SERVER_INIT { \
    .type = TAPI_STORAGE_SERVICE_UNSPECIFIED,   \
    .rpcs = NULL,                               \
    .methods = NULL,                            \
    .auth = { 0 },                              \
    .context = NULL                             \
}

/*
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
 * @param storage       Storage device can be represented by name, mount
 *                      point, etc. depends on server implementation.
 *                      May be @c NULL.
 * @param path          Directory pathname on the @p storage to be shared if
 *                      storage is not @c NULL, or full path if storage is
 *                      @c NULL. May be @c NULL if it is required to add all
 *                      storage data to share.
 *
 * @return Status code.
 */
typedef te_errno (* tapi_storage_server_method_add_share)(
                                        tapi_storage_server *server,
                                        const char          *storage,
                                        const char          *path);

/**
 * Delete directory from storage sharing.
 *
 * @param server        Server handle.
 * @param storage       Storage device can be represented by name, mount
 *                      point, etc. depends on server implementation.
 *                      May be @c NULL.
 * @param path          Directory pathname on the @p storage to be removed
 *                      from the sharing list if storage is not @c NULL, or
 *                      full path if storage is @c NULL. May be @c NULL if
 *                      trere are all storage data is shared.
 *
 * @return Status code.
 */
typedef te_errno (* tapi_storage_server_method_del_share)(
                                        tapi_storage_server *server,
                                        const char          *storage,
                                        const char          *path);

/**
 * Get shared directories list. @p share should be freed by user with
 * @p tapi_storage_share_list_free when it is no longer needed.
 *
 * @param[in]  server       Server handle.
 * @param[out] share        List of shared directories.
 *
 * @return Status code.
 *
 * @sa tapi_storage_share_list_free
 */
typedef te_errno (* tapi_storage_server_method_get_share)(
                                       tapi_storage_server     *server,
                                       tapi_storage_share_list *share);

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
    tapi_storage_service_type          type;    /**< Type of server. */
    rcf_rpc_server                    *rpcs;    /**< RPC server handle. */
    const tapi_storage_server_methods *methods; /**< Methods to operate the
                                                     server. */
    tapi_storage_auth_params           auth;    /**< Authorization
                                                     parameters. */
    void                              *context; /**< Server context. */
};


/**
 * Enable a storage server.
 * Server should be disabled with @p tapi_storage_server_disable when it is
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
    return (server->methods != NULL && server->methods->enable != NULL
            ? server->methods->enable(server) : TE_EOPNOTSUPP);
}

/**
 * Disable a storage server that was enabled with
 * @p tapi_storage_server_enable.
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
    return (server->methods != NULL && server->methods->disable != NULL
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
    return (server->methods != NULL && server->methods->is_enabled != NULL
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
    return (server->methods != NULL && server->methods->add_storage != NULL
            ? server->methods->add_storage(server, storage_name)
            : TE_EOPNOTSUPP);
}

/**
 * Add a directory to storage share.
 *
 * @param server        Server handle.
 * @param storage       Storage device can be represented by name, mount
 *                      point, etc. depends on server implementation.
 *                      May be @c NULL.
 * @param path          Directory pathname on the @p storage to be shared if
 *                      storage is not @c NULL, or full path if storage is
 *                      @c NULL. May be @c NULL if it is required to add all
 *                      storage data to share.
 *
 * @return Status code.
 */
static inline te_errno
tapi_storage_server_add_share(tapi_storage_server *server,
                              const char          *storage,
                              const char          *path)
{
    return (server->methods != NULL && server->methods->add_share != NULL
            ? server->methods->add_share(server, storage, path)
            : TE_EOPNOTSUPP);
}

/**
 * Delete directory from storage sharing.
 *
 * @param server        Server handle.
 * @param storage       Storage device can be represented by name, mount
 *                      point, etc. depends on server implementation.
 *                      May be @c NULL.
 * @param path          Directory pathname on the @p storage to be removed
 *                      from the sharing list if storage is not @c NULL, or
 *                      full path if storage is @c NULL. May be @c NULL if
 *                      trere are all storage data is shared.
 *
 * @return Status code.
 */
static inline te_errno
tapi_storage_server_del_share(tapi_storage_server *server,
                              const char          *storage,
                              const char          *path)
{
    return (server->methods != NULL && server->methods->del_share != NULL
            ? server->methods->del_share(server, storage, path)
            : TE_EOPNOTSUPP);
}

/**
 * Get shared directories list. @p share should be freed by user with
 * @p tapi_storage_share_list_free when it is no longer needed.
 *
 * @param[in]  server       Server handle.
 * @param[out] share        List of shared directories.
 *
 * @return Status code.
 *
 * @sa tapi_storage_share_list_free
 */
static inline te_errno
tapi_storage_server_get_share(tapi_storage_server     *server,
                              tapi_storage_share_list *share)
{
    return (server->methods != NULL && server->methods->get_share != NULL
            ? server->methods->get_share(server, share)
            : TE_EOPNOTSUPP);
}

/**
 * Initialize server handle.
 * Server should be released with @p tapi_storage_server_fini when it is no
 * longer needed.
 *
 * @param[in]  type         Back-end server type.
 * @param[in]  rpcs         RPC server handle.
 * @param[in]  methods      Back-end server specific methods.
 * @param[in]  auth         Back-end server specific authorization
 *                          parameters. May be @c NULL.
 * @param[in]  context      Back-end server specific context. Don't free
 *                          the @p context before finishing work with
                            @p server.
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
 * Release server that was initialized with @p tapi_storage_server_init.
 *
 * @param server        Server handle.
 *
 * @sa tapi_storage_server_init
 */
extern void tapi_storage_server_fini(tapi_storage_server *server);

/**@} <!-- END tapi_storage_server --> */

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TAPI_STORAGE_SERVER_H__ */
