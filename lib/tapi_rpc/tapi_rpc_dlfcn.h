/** @file
 * @brief Test API - RPC
 *
 * Definition of TAPI for remote calls of dynamic linking loader.
 *
 *
 * Copyright (C) 2010 Test Environment authors (see file AUTHORS in
 * the root directory of the distribution).
 * Copyright (c) 2010 Level5 Networks Corp.
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
 * @author Nikita Rastegaev <Nikita.Rastegaev@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_TAPI_RPC_DLFCN_H__
#define __TE_TAPI_RPC_DLFCN_H__

#include "rcf_rpc.h"
#include "te_rpc_types.h"


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup te_lib_rpc_dlfcn TAPI for remote calls of dynamic linking loader
 * @ingroup te_lib_rpc_tapi
 * @{
 */

typedef int64_t     rpc_dlhandle;
typedef int64_t     rpc_dlsymaddr;

#define RPC_DLHANDLE_NULL   ((rpc_dlhandle)(RPC_NULL))
#define RPC_DLSYM_NULL      ((rpc_dlsymaddr)(RPC_NULL))

/**
 * Loads the dynamic labrary file.
 *
 * @param rpcs      RPC server handle
 * @param filename  the name of the file to load
 * @param flags     dlopen flags
 *
 * @return dynamic library handle on success or NULL in the case of failure
 */
extern rpc_dlhandle rpc_dlopen(rcf_rpc_server *rpcs, const char *filename,
                               int flags);

/**
 * Returns a human readable string describing the most recent error
 * that occured from dlopen(), dlsym() or dlclose().
 *
 * @param rpcs      RPC server handle
 *
 * @return a pointer to string or NULL if no errors occured.
 */
extern char *rpc_dlerror(rcf_rpc_server *rpcs);

/**
 * Returns the address where a certain symbol from dynamic library
 * is loaded into memory.
 *
 * @param rpcs      RPC server handle
 * @param handle    handle of a dynamic library returned by dlopen()
 * @param symbol    null-terminated symbol name
 *
 * @return address of the symbol or NULL if symbol is not found.
 */
extern rpc_dlsymaddr rpc_dlsym(rcf_rpc_server *rpcs, rpc_dlhandle handle,
                               const char *symbol);

/**
 * Calls a certain function without arguments from dynamic labrary.
 *
 * @param rpcs      RPC server handle
 * @param handle    handle of a dynamic library returned by dlopen()
 * @param symbol    null-terminated symbol name
 *
 * @return return code of function.
 */
extern int rpc_dlsym_call(rcf_rpc_server *rpcs, rpc_dlhandle handle,
                          const char *symbol);

/**
 * Decrements the reference count on the dynamic library handle.
 * If the reference count drops to zero and no other loaded libraries
 * use symbols in it, then the dynamic library is unloaded.
 *
 * @param rpcs      RPC server handle
 * @param handle    handle of a dynamic library returned by dlopen()
 *
 * @return 0 on success, and non-zero on error.
 */
extern int rpc_dlclose(rcf_rpc_server *rpcs, rpc_dlhandle handle);

/**@} <!-- END te_lib_rpc_dlfcn --> */

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_RPC_DLFCN_H__ */
