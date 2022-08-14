/** @file
 * @brief Test API NVMe internal
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#define TE_LGR_USER "NVME TAPI"

#include "tapi_rpc_unistd.h"
#include "tapi_rpc_dirent.h"
#include "tapi_nvme_internal.h"

#define NVME_TE_STRING_INIT_STATIC(_size) \
    TE_STRING_BUF_INIT((char[_size]){'\0'})

/* See description in tapi_nvme_internal.h */
te_errno
tapi_nvme_internal_file_append(rcf_rpc_server *rpcs, unsigned int timeout_sec,
                               const char *string, const char *fmt, ...)
{
    int fd;
    te_errno rc;
    va_list arguments;
    te_string path = NVME_TE_STRING_INIT_STATIC(NAME_MAX);

    va_start(arguments, fmt);
    if ((rc = te_string_append_va(&path, fmt, arguments)) != 0)
        return rc;
    va_end(arguments);

    RPC_AWAIT_IUT_ERROR(rpcs);
    fd = rpc_open(rpcs, path.ptr, RPC_O_CREAT | RPC_O_APPEND | RPC_O_WRONLY,
                  TAPI_NVME_INTERNAL_MODE);

    if (fd == -1)
    {
        ERROR("Cannot open file %s", path);
        return rpcs->_errno;
    }

    RPC_AWAIT_IUT_ERROR(rpcs);
    if (timeout_sec != 0)
        rpcs->timeout = TE_SEC2MS(timeout_sec);
    if (rpc_write(rpcs, fd, string, strlen(string)) == -1)
        return rpcs->_errno;

    RPC_AWAIT_IUT_ERROR(rpcs);
    if (rpc_close(rpcs, fd) == -1)
        return rpcs->_errno;

    return 0;
}

/* See description in tapi_nvme_internal.h */
int
tapi_nvme_internal_file_read(rcf_rpc_server *rpcs, char *buffer, size_t size,
    const char *fmt, ...)
{
    te_errno rc;
    int read, fd;
    va_list arguments;

    te_string path = NVME_TE_STRING_INIT_STATIC(NAME_MAX);

    va_start(arguments, fmt);
    if ((rc = te_string_append_va(&path, fmt, arguments)) != 0)
        return rc;
    va_end(arguments);

    RPC_AWAIT_IUT_ERROR(rpcs);
    if ((fd = rpc_open(rpcs, path.ptr, RPC_O_RDONLY,
                       TAPI_NVME_INTERNAL_MODE)) == -1)
    {
        ERROR("Cannot open file %s", path.ptr);
        return -1;
    }

    RPC_AWAIT_IUT_ERROR(rpcs);
    if ((read = rpc_read(rpcs, fd, buffer, size)) == -1)
        ERROR("Cannot read file %s", path.ptr);

    RPC_AWAIT_IUT_ERROR(rpcs);
    if (rpc_close(rpcs, fd) == -1)
    {
        ERROR("Cannot close file %s", path.ptr);
        read = -1;
    }

    return read;
}

/* See description in tapi_nvme_internal.h */
te_bool
tapi_nvme_internal_isdir_exist(rcf_rpc_server *rpcs, const char *path)
{
    rpc_dir_p dir;

    RPC_AWAIT_IUT_ERROR(rpcs);
    dir = rpc_opendir(rpcs, path);
    if (dir != RPC_NULL)
    {
        RPC_AWAIT_IUT_ERROR(rpcs);
        rpc_closedir(rpcs, dir);
        return TRUE;
    }

    return FALSE;
}

/* See description in tapi_nvme_internal.h */
te_bool
tapi_nvme_internal_mkdir(rcf_rpc_server *rpcs, const char *fmt, ...)
{
    te_errno rc;
    va_list arguments;

    te_string path = NVME_TE_STRING_INIT_STATIC(NAME_MAX);

    va_start(arguments, fmt);
    if (te_string_append_va(&path, fmt, arguments) != 0)
    {
        ERROR("%s: Cannot create path with format %s", fmt);
        return FALSE;
    }
    va_end(arguments);

    RPC_AWAIT_IUT_ERROR(rpcs);
    rc = rpc_mkdir(rpcs, path.ptr, TAPI_NVME_INTERNAL_MODE);

    if (rc == -1 && rpcs->_errno == TE_EEXIST)
        return TRUE;

    return rc == 0;
}

/* See description in tapi_nvme_internal.h */
te_bool
tapi_nvme_internal_rmdir(rcf_rpc_server *rpcs, const char *fmt, ...)
{
    va_list arguments;

    te_string path = NVME_TE_STRING_INIT_STATIC(NAME_MAX);

    va_start(arguments, fmt);
    if (te_string_append_va(&path, fmt, arguments) != 0)
    {
        ERROR("%s: Cannot create path with format %s", fmt);
        return FALSE;
    }
    va_end(arguments);

    if (!tapi_nvme_internal_isdir_exist(rpcs, path.ptr))
        return FALSE;

    RPC_AWAIT_IUT_ERROR(rpcs);
    return rpc_rmdir(rpcs, path.ptr) == 0;
}

/* See description in tapi_nvme_internal.h */
te_errno
tapi_nvme_internal_filterdir(rcf_rpc_server *rpcs, const char *path,
                             const char *start_from, te_vec *result)
{
    te_errno rc = 0;
    rpc_dir_p dir;
    rpc_dirent *dirent = NULL;
    te_vec temp_result = TE_VEC_INIT(tapi_nvme_internal_dirinfo);

    RPC_AWAIT_IUT_ERROR(rpcs);
    if ((dir = rpc_opendir(rpcs, path)) == RPC_NULL)
    {
        ERROR("Cannot open directory %s", path);
        return RPC_ERRNO(rpcs);
    }

    RPC_AWAIT_IUT_ERROR(rpcs);
    while ((dirent = rpc_readdir(rpcs, dir)) != NULL)
    {
        tapi_nvme_internal_dirinfo dirinfo;

        if (strstr(dirent->d_name, start_from) == dirent->d_name)
        {
            strcpy(dirinfo.name, dirent->d_name);
            if ((rc = TE_VEC_APPEND(&temp_result, dirinfo)) != 0)
            {
                te_vec_free(&temp_result);
                return rc;
            }
        }

        RPC_AWAIT_IUT_ERROR(rpcs);
    }

    if (rpc_closedir(rpcs, dir) == -1)
    {
        ERROR("Cannot close directory %s", path);
        te_vec_free(&temp_result);
        return RPC_ERRNO(rpcs);
    }

    *result = temp_result;
    return 0;
}
