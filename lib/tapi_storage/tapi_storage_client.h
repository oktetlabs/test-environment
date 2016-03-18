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

#ifndef __TAPI_STORAGE_CLIENT_H__
#define __TAPI_STORAGE_CLIENT_H__

#include "tapi_local_file.h"
#include "tapi_storage_common.h"


#ifdef __cplusplus
extern "C" {
#endif

/**
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
 * @param recursive     Perform recursive copying if @c TRUE and specified
 *                      local file is directory.
 * @param force         Force to replace existent content by the same. On
 *                      default (if @c FALSE) the existent contend will not
 *                      be rewriten by the same (lazy behaviour).
 *
 * @return Status code.
 */
typedef te_errno (* tapi_storage_client_method_put)(
                                        tapi_storage_client *client,
                                        const char          *local_file,
                                        const char          *remote_file,
                                        te_bool              recursive,
                                        te_bool              force);

/**
 * Get a file from remote storage.
 *
 * @param client        Client handle.
 * @param remote_file   Remote file name.
 * @param local_file    Local file name or @c NULL to use the same.
 * @param recursive     Perform recursive copying if @c TRUE and specified
 *                      remote file is directory.
 *
 * @return Status code.
 */
typedef te_errno (* tapi_storage_client_method_get)(
                                        tapi_storage_client *client,
                                        const char          *remote_file,
                                        const char          *local_file,
                                        te_bool              recursive);

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
    rcf_rpc_server                    *rpcs;    /**< RPC server handle. */
    tapi_storage_service_type          type;    /**< Type of client. */
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
    return (client->methods->open != NULL ? client->methods->open(client)
                                          : TE_EOPNOTSUPP);
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
    return (client->methods->close != NULL ? client->methods->close(client)
                                           : TE_EOPNOTSUPP);
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
    return (client->methods->pwd != NULL
            ? client->methods->pwd(client, directory) : TE_EOPNOTSUPP);
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
    return (client->methods->ls != NULL
            ? client->methods->ls(client, path, files) : TE_EOPNOTSUPP);
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
    return (client->methods->cd != NULL
            ? client->methods->cd(client, remote_directory)
            : TE_EOPNOTSUPP);
}


/**
 * Put a local file to remote storage.
 *
 * @param client        Client handle.
 * @param local_file    Local file name.
 * @param remote_file   Remote file name or @c NULL to use the same.
 * @param recursive     Perform recursive copying if @c TRUE and specified
 *                      local file is directory.
 * @param force         Force to replace existent content by the same. On
 *                      default (if @c FALSE) the existent contend will not
 *                      be rewriten by the same (lazy behaviour).
 *
 * @return Status code.
 */
static inline te_errno
tapi_storage_client_put(tapi_storage_client *client,
                        const char          *local_file,
                        const char          *remote_file,
                        te_bool              recursive,
                        te_bool              force)
{
    return (client->methods->put != NULL
            ? client->methods->put(client, local_file, remote_file,
                                   recursive, force)
            : TE_EOPNOTSUPP);
}

/**
 * Get a file from remote storage.
 *
 * @param client        Client handle.
 * @param remote_file   Remote file name.
 * @param local_file    Local file name or @c NULL to use the same.
 * @param recursive     Perform recursive copying if @c TRUE and specified
 *                      remote file is directory.
 *
 * @return Status code.
 */
static inline te_errno
tapi_storage_client_get(tapi_storage_client *client,
                        const char          *remote_file,
                        const char          *local_file,
                        te_bool              recursive)
{
    return (client->methods->get != NULL
            ? client->methods->get(client, remote_file, local_file,
                                   recursive)
            : TE_EOPNOTSUPP);
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
    return (client->methods->rm != NULL
            ? client->methods->rm(client, filename, recursive)
            : TE_EOPNOTSUPP);
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
                          const char          *directory_name,
                          te_bool              recursive)
{
    return (client->methods->mkdir != NULL
            ? client->methods->mkdir(client, directory_name, recursive)
            : TE_EOPNOTSUPP);
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
    return (client->methods->rmdir != NULL
            ? client->methods->rmdir(client, directory_name)
            : TE_EOPNOTSUPP);
}


/**
 * Initialize client handle.
 * Client should be released with @b tapi_storage_client_fini when it is no
 * longer needed.
 *
 * @param[in]  type         Back-end client type.
 * @param[in]  rpcs         RPC server handle.
 * @param[in]  methods      Back-end client scpecific methods.
 * @param[in]  auth         Back-end client specific authorization
 *                          parameters.
 * @param[in]  context      Back-end client specific context.
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
 * Release client that was initialized with @b tapi_storage_client_init.
 *
 * @param client        Client handle.
 *
 * @return Status code.
 *
 * @sa tapi_storage_client_init
 *
 * @todo How to free context?
 */
extern void tapi_storage_client_fini(tapi_storage_client *client);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TAPI_STORAGE_CLIENT_H__ */
