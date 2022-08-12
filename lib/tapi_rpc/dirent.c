/** @file
 * @brief Test API for RPC
 *
 * TAPI for remote calls of standard directory operations
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 * @author Oleg Kravtsov <Oleg.Kravtsov@oktetlabs.ru>
 *
 */

#include "te_config.h"

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#endif
#if HAVE_STRINGS_H
#include <strings.h>
#endif

#include "tapi_rpc_internal.h"
#include "tapi_rpc_dirent.h"
#include "tapi_rpc_misc.h"
#include "te_printf.h"

void
rpc_struct_dirent_props(rcf_rpc_server *rpcs, size_t *props)
{
    tarpc_struct_dirent_props_in  in;
    tarpc_struct_dirent_props_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_VOID(struct_dirent_props);
    }

    rcf_rpc_call(rpcs, "struct_dirent_props", &in, &out);

    TAPI_RPC_LOG(rpcs, struct_dirent_props, "", "{%s}",
                 dirent_props_rpc2str(out.retval));

    *props = out.retval;

    RETVAL_VOID(struct_dirent_props);
}

rpc_dir_p
rpc_opendir(rcf_rpc_server *rpcs, const char *path)
{
    tarpc_opendir_in  in;
    tarpc_opendir_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_RPC_PTR(opendir, RPC_NULL);
    }

    if (path != NULL)
    {
        in.path.path_len = strlen(path) + 1;
        in.path.path_val = (char *)strdup(path);
    }

    rcf_rpc_call(rpcs, "opendir", &in, &out);

    if (path != NULL)
        free(in.path.path_val);

    CHECK_RETVAL_VAR(opendir, out.mem_ptr, FALSE, RPC_NULL);
    TAPI_RPC_LOG(rpcs, opendir, "%s", "0x%x",
                 path, (unsigned)out.mem_ptr);

    RETVAL_RPC_PTR(opendir, out.mem_ptr);
}

int
rpc_closedir(rcf_rpc_server *rpcs, rpc_dir_p dirp)
{
    tarpc_closedir_in  in;
    tarpc_closedir_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(opendir, -1);
    }

    in.mem_ptr = dirp;

    rcf_rpc_call(rpcs, "closedir", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(closedir, out.retval);
    TAPI_RPC_LOG(rpcs, closedir, "0x%x", "%d",
                 dirp, out.retval);

    RETVAL_INT(closedir, out.retval);
}

rpc_dirent *
rpc_dirent_alloc(void)
{
    rpc_dirent *dent;

    if ((dent = calloc(1, sizeof(*dent))) == NULL)
        ERROR("Failed to allocate 'dirent' structure");

    return dent;
}

rpc_dirent *
rpc_readdir(rcf_rpc_server *rpcs, rpc_dir_p dirp)
{
    rpc_dirent        *dent = NULL;
    tarpc_readdir_in   in;
    tarpc_readdir_out  out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_PTR(readdir, NULL);
    }

    in.mem_ptr = dirp;

    rcf_rpc_call(rpcs, "readdir", &in, &out);

    if (RPC_IS_CALL_OK(rpcs) && out.dent.dent_val != NULL)
    {
        if ((dent = rpc_dirent_alloc()) == NULL)
            rpcs->_errno = TE_RC(TE_RCF, TE_ENOMEM);
        else
        {
            struct tarpc_dirent *rpc_dent;

            rpc_dent = out.dent.dent_val;
            dent->d_name = rpc_dent->d_name.d_name_val;
            rpc_dent->d_name.d_name_val = NULL;
            rpc_dent->d_name.d_name_len = 0;
            dent->d_ino = rpc_dent->d_ino;
            dent->d_off = rpc_dent->d_off;
            dent->d_type = rpc_dent->d_type;
            dent->d_namelen = rpc_dent->d_namelen;
        }
    }

    if (dent == NULL)
    {
        TAPI_RPC_LOG(rpcs, readdir, "0x%x", "NULL", dirp);
    }
    else
    {
        TAPI_RPC_LOG(rpcs, readdir, "0x%x", "{%s}", dirp, dent->d_name);

    }

    /*
     * There is nothing wrong when readdir() returns NULL,
     * it means we reached the end of the directory stream.
     */
    if (out.dent.dent_val == NULL)
    {
        TAPI_RPC_OUT(readdir, FALSE);
        return NULL;
    }
    else
        RETVAL_PTR(readdir, dent);
}

