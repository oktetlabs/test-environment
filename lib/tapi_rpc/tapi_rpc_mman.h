/** @file
 * @brief Test API - RPC
 *
 * Definition of TAPI for remote calls of memory management operations
 * (mmap(), munmap())
 *
 *
 * Copyright (C) 2020 OKTET Labs. All rights reserved.
 *
 * @author Dmitry Izbitsky <Dmitry.Izbitsky@oktetlabs.ru>
 */

#ifndef __TE_TAPI_RPC_MMAN_H__
#define __TE_TAPI_RPC_MMAN_H__

#include "rcf_rpc.h"
#include "te_rpc_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Create memory mapping on TA.
 *
 * @note If you need actual address on TA, not RPC pointer, you
 *       can use rpc_get_addr_by_id().
 *
 * @param rpcs        RPC server handle.
 * @param addr        Hint where to create mapping.
 * @param length      Length of the mapping.
 * @param prot        Memory protection flags (see @ref rpc_prot_flags).
 * @param flags       Memory mapping flags (see @ref rpc_map_flags).
 * @param fd          File descriptor for which to create mapping
 *                    (may be @c -1 if it is anonymous mapping).
 * @param offset      Offset in the file.
 *
 * @return RPC pointer to address of the mapping on success,
 *         @c RPC_NULL on failure.
 */
extern rpc_ptr rpc_mmap(rcf_rpc_server *rpcs,
                        uint64_t addr, uint64_t length,
                        unsigned int prot, unsigned int flags,
                        int fd, off_t offset);

/**
 * Destroy memory mapping on TA.
 *
 * @param rpcs        RPC server handle.
 * @param addr        RPC pointer previously returned by rpc_mmap().
 * @param length      Length of the mapping.
 *
 * @return @c 0 on success, @c -1 on failure
 */
extern int rpc_munmap(rcf_rpc_server *rpcs,
                      rpc_ptr addr, uint64_t length);

/**
 * Give advise about use of memory.
 *
 * @param rpcs        RPC server handle.
 * @param addr        RPC pointer to the start of a memory range.
 * @param length      Length of the range.
 * @param advise      Advise (see @ref rpc_madv_value).
 *
 * @return @c 0 on success, @c -1 on failure
 */
extern int rpc_madvise(rcf_rpc_server *rpcs,
                       rpc_ptr addr, uint64_t length,
                       rpc_madv_value advise);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_RPC_MMAN_H__ */
