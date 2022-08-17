/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Generic test API to storage client routines
 *
 * @defgroup tapi_storage_client Test API to control the storage client
 * @ingroup tapi_storage
 * @{
 *
 * Generic client functions for storage service.
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TAPI_STORAGE_CLIENT_H__
#define __TAPI_STORAGE_CLIENT_H__

#include "tapi_storage_common.h"
#include "tapi_local_fs.h"
#include "rcf_rpc.h"


#ifdef __cplusplus
extern "C" {
#endif


/** On-stack tapi_storage_client structure initializer. */
#define TAPI_STORAGE_CLIENT_INIT { \
    .type = TAPI_STORAGE_SERVICE_UNSPECIFIED,   \
    .rpcs = NULL,                               \
    .methods = NULL,                            \
    .auth = { 0 },                              \
    .context = NULL                             \
}

/*
 * Forward declaration of generic client structure.
 */
struct tapi_storage_client;
typedef struct tapi_storage_client tapi_storage_client;


/**
 * Open a connection.
 *
 * @param client        Client handle.
 *
 * @return Status code.
 */
typedef te_errno (* tapi_storage_client_method_open)(
                                            tapi_storage_client *client);

/**
 * Close the connection.
 *
 * @param client        Client handle.
 *
 * @return Status code.
 */
typedef te_errno (* tapi_storage_client_method_close)(
                                            tapi_storage_client *client);

/**
 * Get current work directory name.
 *
 * @param client        Client handle.
 * @param directory     File context where the directory name will be saved.
 *
 * @return Status code.
 */
typedef te_errno (* tapi_storage_client_method_pwd)(
                                        tapi_storage_client *client,
                                        tapi_local_file     *directory);

/**
 * Get files list.
 *
 * @param[in]  service      Service handle.
 * @param[in]  path         Path to the files.
 * @param[out] files        Files list.
 *
 * @return Status code.
 */
typedef te_errno (* tapi_storage_client_method_ls)(
                                            tapi_storage_client  *client,
                                            const char           *path,
                                            tapi_local_file_list *files);

/**
 * Change remote work directory.
 *
 * @param client            Client handle.
 * @param remote_directory  Remote directory.
 *
 * @return Status code.
 */
typedef te_errno (* tapi_storage_client_method_cd)(
                                    tapi_storage_client *client,
                                    const char          *remote_directory);


/**
 * Put a local file to remote storage.
 *
 * @param client        Client handle.
 * @param local_file    Local file name.
 * @param remote_file   Remote file name or @c NULL to use the same.
 *
 * @return Status code.
 */
typedef te_errno (* tapi_storage_client_method_put)(
                                        tapi_storage_client *client,
                                        const char          *local_file,
                                        const char          *remote_file);

/**
 * Get a file from remote storage.
 *
 * @param client        Client handle.
 * @param remote_file   Remote file name.
 * @param local_file    Local file name or @c NULL to use the same.
 *
 * @return Status code.
 */
typedef te_errno (* tapi_storage_client_method_get)(
                                        tapi_storage_client *client,
                                        const char          *remote_file,
                                        const char          *local_file);

/**
 * Remove a file(s) from the current working directory on the remote
 * storage.
 *
 * @param client        Client handle.
 * @param filename      Remote file name to remove.
 * @param recursive     Perform recursive removing if @c TRUE and specified
 *                      file is directory.
 *
 * @return Status code.
 */
typedef te_errno (* tapi_storage_client_method_rm)(
                                        tapi_storage_client *client,
                                        const char          *filename,
                                        te_bool              recursive);

/**
 * Make a new directory in the current working directory on the remote
 * storage. It behaves like a 'mkdir -p' i.e. it creates a parent
 * directories as needed.
 *
 * @param client            Client handle.
 * @param directory_name    Directory name to create on remote storage.
 *
 * @return Status code.
 */
typedef te_errno (* tapi_storage_client_method_mkdir)(
                                    tapi_storage_client *client,
                                    const char          *directory_name);

/**
 * Remove the directory from the remote storage.
 *
 * @param client            Client handle.
 * @param directory_name    Directory name to remove it from the remote
 *                          storage.
 *
 * @return Status code.
 */
typedef te_errno (* tapi_storage_client_method_rmdir)(
                                    tapi_storage_client *client,
                                    const char          *directory_name);

/**
 * Methods to operate the client.
 */
typedef struct tapi_storage_client_methods {
    tapi_storage_client_method_open   open;
    tapi_storage_client_method_close  close;
    tapi_storage_client_method_pwd    pwd;
    tapi_storage_client_method_ls     ls;
    tapi_storage_client_method_cd     cd;
    tapi_storage_client_method_put    put;
    tapi_storage_client_method_get    get;
    tapi_storage_client_method_rm     rm;
    tapi_storage_client_method_mkdir  mkdir;
    tapi_storage_client_method_rmdir  rmdir;
} tapi_storage_client_methods;

/**
 * Generic structure which provides set of operations to work with a storage
 * independently on back-end service.
 */
struct tapi_storage_client {
    tapi_storage_service_type          type;    /**< Type of client. */
    rcf_rpc_server                    *rpcs;    /**< RPC server handle. */
    const tapi_storage_client_methods *methods; /**< Methods to operate the
                                                     client. */
    tapi_storage_auth_params           auth;    /**< Authorization
                                                     parameters. */
    void                              *context; /**< Client context. */
};


/**
 * Open a connection.
 *
 * @param client        Client handle.
 *
 * @return Status code.
 */
static inline te_errno
tapi_storage_client_connect(tapi_storage_client *client)
{
    if (client->methods == NULL ||
        client->methods->open == NULL)
        return TE_EOPNOTSUPP;

    return client->methods->open(client);
}

/**
 * Close the connection.
 *
 * @param client        Client handle.
 *
 * @return Status code.
 */
static inline te_errno
tapi_storage_client_disconnect(tapi_storage_client *client)
{
    if (client->methods == NULL ||
        client->methods->close == NULL)
        return TE_EOPNOTSUPP;

    return client->methods->close(client);
}

/**
 * Get current work directory name.
 *
 * @param client        Client handle.
 * @param directory     File context where the directory name will be saved.
 *
 * @return Status code.
 */
static inline te_errno
tapi_storage_client_pwd(tapi_storage_client *client,
                        tapi_local_file     *directory)
{
    if (client->methods == NULL ||
        client->methods->pwd == NULL)
        return TE_EOPNOTSUPP;

    return client->methods->pwd(client, directory);
}

/**
 * Get files list.
 *
 * @param[in]  service      Service handle.
 * @param[in]  path         Path to the files.
 * @param[out] files        Files list.
 *
 * @return Status code.
 */
static inline te_errno
tapi_storage_client_ls(tapi_storage_client  *client,
                       const char           *path,
                       tapi_local_file_list *files)
{
    if (client->methods == NULL ||
        client->methods->ls == NULL)
        return TE_EOPNOTSUPP;

    return client->methods->ls(client, path, files);
}

/**
 * Change remote work directory.
 *
 * @param client            Client handle.
 * @param remote_directory  Remote directory.
 *
 * @return Status code.
 */
static inline te_errno
tapi_storage_client_cd(tapi_storage_client *client,
                       const char          *remote_directory)
{
    if (client->methods == NULL ||
        client->methods->cd == NULL)
        return TE_EOPNOTSUPP;

    return client->methods->cd(client, remote_directory);
}


/**
 * Put a local file to remote storage.
 *
 * @param client        Client handle.
 * @param local_file    Local file name.
 * @param remote_file   Remote file name or @c NULL to use the same.
 *
 * @return Status code.
 */
static inline te_errno
tapi_storage_client_put(tapi_storage_client *client,
                        const char          *local_file,
                        const char          *remote_file)
{
    if (client->methods == NULL ||
        client->methods->put == NULL)
        return TE_EOPNOTSUPP;

    return client->methods->put(client, local_file, remote_file);
}

/**
 * Get a file from remote storage.
 *
 * @param client        Client handle.
 * @param remote_file   Remote file name.
 * @param local_file    Local file name or @c NULL to use the same.
 *
 * @return Status code.
 */
static inline te_errno
tapi_storage_client_get(tapi_storage_client *client,
                        const char          *remote_file,
                        const char          *local_file)
{
    if (client->methods == NULL ||
        client->methods->get == NULL)
        return TE_EOPNOTSUPP;

    return client->methods->get(client, remote_file, local_file);
}

/**
 * Remove a file(s) from the current working directory on the remote
 * storage.
 *
 * @param client        Client handle.
 * @param filename      Remote file name to remove.
 * @param recursive     Perform recursive removing if @c TRUE and specified
 *                      file is directory.
 *
 * @return Status code.
 */
static inline te_errno
tapi_storage_client_rm(tapi_storage_client *client,
                       const char          *filename,
                       te_bool              recursive)
{
    if (client->methods == NULL ||
        client->methods->rm == NULL)
        return TE_EOPNOTSUPP;

    return client->methods->rm(client, filename, recursive);
}

/**
 * Make a new directory in the current working directory on the remote
 * storage.
 *
 * @param client            Client handle.
 * @param directory_name    Directory name to create on remote storage.
 * @param recursive         Make parent directories as needed if @c TRUE.
 *
 * @return Status code.
 */
static inline te_errno
tapi_storage_client_mkdir(tapi_storage_client *client,
                          const char          *directory_name)
{
    if (client->methods == NULL ||
        client->methods->mkdir == NULL)
        return TE_EOPNOTSUPP;

    return client->methods->mkdir(client, directory_name);
}

/**
 * Remove the directory from the remote storage.
 *
 * @param client            Client handle.
 * @param directory_name    Directory name to remove it from the remote
 *                          storage.
 *
 * @return Status code.
 */
static inline te_errno
tapi_storage_client_rmdir(tapi_storage_client *client,
                          const char          *directory_name)
{
    if (client->methods == NULL ||
        client->methods->rmdir == NULL)
        return TE_EOPNOTSUPP;

    return client->methods->rmdir(client, directory_name);
}


/**
 * Initialize client handle.
 * Client should be released with @p tapi_storage_client_fini when it is no
 * longer needed.
 *
 * @param[in]  type         Back-end client type.
 * @param[in]  rpcs         RPC server handle.
 * @param[in]  methods      Back-end client scpecific methods.
 * @param[in]  auth         Back-end client specific authorization
 *                          parameters. May be @c NULL.
 * @param[in]  context      Back-end client specific context. Don't free
 *                          the @p context before finishing work with
                            @p client.
 * @param[out] client       Client handle.
 *
 * @return Status code.
 *
 * @sa tapi_storage_client_fini
 */
extern te_errno tapi_storage_client_init(
                                tapi_storage_service_type          type,
                                rcf_rpc_server                    *rpcs,
                                const tapi_storage_client_methods *methods,
                                tapi_storage_auth_params          *auth,
                                void                              *context,
                                tapi_storage_client               *client);

/**
 * Release client that was initialized with @p tapi_storage_client_init.
 *
 * @param client        Client handle.
 *
 * @return Status code.
 *
 * @sa tapi_storage_client_init
 */
extern void tapi_storage_client_fini(tapi_storage_client *client);

/**
 * Copy local files to the storage server location. Can be used for
 * recursive copying of directories.
 *
 * @param client        Client handle.
 * @param local_file    Local file name.
 * @param remote_file   Remote file name or @c NULL to use the same.
 * @param recursive     It has effect only if @p local_file is directory.
 *                      If @c FALSE then only files from mentioned directory
 *                      will be copied. If @c TRUE then additionally the
 *                      all files from subdirectories will be copied.
 * @param force         Force to replace existent content by the same. If
 *                      @c FALSE the existent content will not be rewriten
 *                      by the same (lazy behaviour).
 *
 * @return Status code.
 */
extern te_errno tapi_storage_client_mput(tapi_storage_client   *client,
                                         const tapi_local_file *local_file,
                                         const char            *remote_file,
                                         te_bool                recursive,
                                         te_bool                force);

/**
 * Copy files from the storage server to local location. Can be used for
 * recursive copying of directories.
 *
 * @param client        Client handle.
 * @param remote_file   Remote file name.
 * @param local_file    Local file name or @c NULL to use the same.
 * @param recursive     It has effect only if @p remote_file is directory.
 *                      If @c FALSE then only files from mentioned directory
 *                      will be copied. If @c TRUE then additionally the
 *                      all files from subdirectories will be copied.
 *
 * @return Status code.
 */
extern te_errno tapi_storage_client_mget(tapi_storage_client *client,
                                         const char          *remote_file,
                                         const char          *local_file,
                                         te_bool              recursive);

/**@} <!-- END tapi_storage_client --> */

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TAPI_STORAGE_CLIENT_H__ */
