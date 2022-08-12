/** @file
 * @brief Generic test API to storage client routines
 *
 * Generic client functions for storage service.
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

/* See description in tapi_storage_client.h. */
te_errno
tapi_storage_client_mput(tapi_storage_client   *client,
                         const tapi_local_file *local_file,
                         const char            *remote_file,
                         te_bool                recursive,
                         te_bool                force)
{
    te_errno  rc;
    char     *real_local_pathname;
    char     *remote_pathname = NULL;
    size_t    remote_pathname_len = 0;
    const char *basename;
    tapi_local_file_list  files;
    tapi_local_file_le   *f;
    tapi_local_file      *file;

    if (remote_file == NULL)
        remote_file = local_file->pathname;
    if (local_file->type == TAPI_FILE_TYPE_FILE)
    {
        if (!force)
        {
            rc = tapi_storage_client_ls(client, remote_file, &files);
            if (rc == 0)
            {
                /*
                 * FIXME May be also needed to check that file is the only
                 * one in returned list and its name is the same as
                 * @p remote_file. It should be true for regular file.
                 */
                VERB("File \"%s\" is already presents on server",
                     remote_file);
                tapi_local_fs_list_free(&files);
                return 0;
            }
            /* @c TE_ENODATA - particular file is not found. */
            if (TE_RC_GET_ERROR(rc) != TE_ENODATA)
                return rc;
        }
        real_local_pathname =
            tapi_local_fs_get_file_real_pathname(local_file, NULL);
        if (real_local_pathname == NULL)
            return TE_RC(TE_TAPI, TE_EINVAL);
        VERB("put file: \"%s\" to \"%s\"",
             real_local_pathname, remote_file);
        rc = tapi_storage_client_put(client, real_local_pathname,
                                     remote_file);
        free(real_local_pathname);
        return rc;
    }
    /* local_file is directory. */
    VERB("mkdir: \"%s\"", remote_file);
    rc = tapi_storage_client_mkdir(client, remote_file);
    /* @c TE_EFAIL - it seems the certain directory is already existed. */
    if (rc != 0 && TE_RC_GET_ERROR(rc) != TE_EFAIL)
        return rc;
    rc = tapi_local_fs_ls(local_file->pathname, &files);
    if (rc != 0)
        return rc;
    TAPI_LOCAL_FS_FOREACH(f, &files, file)
    {
        /* Prepare a new remote file name. */
        basename = tapi_local_file_get_name(file);
        if (basename == NULL)
        {
            rc = TE_RC(TE_TAPI, TE_EINVAL);
            break;
        }
        if (remote_pathname_len <
            strlen(remote_file) + strlen(basename) + 1 + 1) /* +'/'+'\0' */
        {
            char *p;

            remote_pathname_len =
                strlen(remote_file) + strlen(basename) + 1 + 1;
            p = realloc(remote_pathname, remote_pathname_len);
            if (p == NULL)
            {
                ERROR("%s:%d: Failed to allocate memory",
                      __FUNCTION__, __LINE__);
                rc = TE_RC(TE_TAPI, TE_ENOMEM);
                break;
            }
            remote_pathname = p;
        }
        strcpy(remote_pathname, remote_file);
        strcat(remote_pathname, "/");
        strcat(remote_pathname, basename);
        if (recursive || file->type == TAPI_FILE_TYPE_FILE)
        {
            /* Put files recursively. */
            rc = tapi_storage_client_mput(client, file, remote_pathname,
                                          recursive, force);
        }
        else
        {
            VERB("mkdir: \"%s\" (create an empty dir in non-recursive mode",
                 remote_pathname);
            rc = tapi_storage_client_mkdir(client, remote_pathname);
            /* @c TE_EFAIL - it seems the certain directory is existed. */
            if (TE_RC_GET_ERROR(rc) == TE_EFAIL)
                rc = 0;
        }
        if (rc != 0)
            break;
    }
    free(remote_pathname);
    tapi_local_fs_list_free(&files);
    return rc;
}

/* See description in tapi_storage_client.h. */
te_errno
tapi_storage_client_mget(tapi_storage_client *client,
                         const char          *remote_file,
                         const char          *local_file,
                         te_bool              recursive)
{
    UNUSED(client);
    UNUSED(remote_file);
    UNUSED(local_file);
    UNUSED(recursive);

    ERROR("%s:%d: Not implemented yet", __FUNCTION__, __LINE__);

    return TE_RC(TE_TAPI, TE_ENOSYS);
}
