/** @file
 * @brief Test API - RPC
 *
 * Definition of TAPI for remote calls of standard directory operations
 * (including opendir(), readdir() and closedir()).
 *
 *
 * Copyright (C) 2013 Test Environment authors (see file AUTHORS in
 * the root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
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
 * @author Oleg Kravtsov <Oleg.Kravtsov@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_TAPI_RPC_DIRENT_H__
#define __TE_TAPI_RPC_DIRENT_H__

#include "rcf_rpc.h"

#include "te_rpc_types.h"
#include "te_rpc_dirent.h"

#include "tarpc.h"

#include "tapi_jmp.h"


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup te_lib_rpc_dirent TAPI for directory operation calls
 * @ingroup te_lib_rpc_tapi
 * @{
 */

/** 'DIR *' equivalent */
typedef rpc_ptr rpc_dir_p;

/** 'struct dirent' equivalent */
typedef struct rpc_dirent {
    char       *d_name; /**< Directory entry name */
    size_t      d_namelen; /**< The size of 'd_name' string
                                (not including trailing zero) */
    uint64_t    d_ino; /**< Directory entry inode value */
    uint64_t    d_off; /**< Offset to the next dirent */
    rpc_d_type  d_type; /**< Directory entry type */
    size_t      d_props; /**< Direntry properties
                              (RPC_DIRENT_HAVE_* flags) */
} rpc_dirent;

/**
 * Get properties of 'struct dirent' on target system
 * (mainly information on which fields exist in 'struct dirent').
 *
 * @param rpcs      RPC server handle
 * @param props     properties of 'struct dirent' (OUT)
 *
 * @return N/A
 */
extern void rpc_struct_dirent_props(rcf_rpc_server *rpcs, size_t *props);

/**
 * Open a directory stream.
 *
 * @param rpcs      RPC server handle
 * @param path      Path to a directory
 * 
 * @return A pointer to the directory stream or NULL on failure
 */
extern rpc_dir_p rpc_opendir(rcf_rpc_server *rpcs,
                             const char *path);

/**
 * Close a directory stream.
 *
 * @param rpcs     RPC server handle
 * @param dirp     Directory stream to close
 *
 * @return  0 on success or -1 on failure
 */
extern int rpc_closedir(rcf_rpc_server *rpcs,
                        rpc_dir_p dirp);

/**
 * Read next directory entry from a directory stream.
 *
 * @param rpcs     RPC server handle
 * @param dirp     Directory stream handle
 *
 * @return A pointer to dirent structure or
 *         NULL if the end of the directory stream reached or
 *         an error occurred
 */
extern rpc_dirent *rpc_readdir(rcf_rpc_server *rpcs,
                               rpc_dir_p dirp);

/**@} <!-- END te_lib_rpc_dirent --> */

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_RPC_DIRENT_H__ */
