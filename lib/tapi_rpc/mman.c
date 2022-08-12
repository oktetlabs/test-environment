/** @file
 * @brief Test API for RPC
 *
 * Definition of TAPI for remote calls of memory management operations
 * (mmap(), munmap())
 *
 *
 * Copyright (C) 2020-2022 OKTET Labs. All rights reserved.
 *
 * @author Dmitry Izbitsky <Dmitry.Izbitsky@oktetlabs.ru>
 */

#include "te_config.h"

#include "rcf_rpc.h"
#include "te_rpc_types.h"
#include "tarpc.h"
#include "tapi_rpc_internal.h"
#include "logger_api.h"
#include "tapi_rpc_mman.h"

/* See description in tapi_rpc_mman.h */
rpc_ptr
rpc_mmap(rcf_rpc_server *rpcs, uint64_t addr, uint64_t length,
         unsigned int prot, unsigned int flags, int fd, off_t offset)
{
    tarpc_mmap_in     in;
    tarpc_mmap_out    out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): invalid RPC server handle", __FUNCTION__);
        RETVAL_RPC_PTR(mmap, RPC_NULL);
    }

    in.addr = addr;
    in.length = length;
    in.prot = prot;
    in.flags = flags;
    in.fd = fd;
    in.offset = offset;

    rcf_rpc_call(rpcs, "mmap", &in, &out);

    CHECK_RETVAL_VAR_RPC_PTR(mmap, out.retval);
    TAPI_RPC_LOG(rpcs, mmap, "%" TE_PRINTF_64 "u, "
                 "%" TE_PRINTF_64 "u, %s, %s, %d, "
                 "%" TE_PRINTF_64 "d", RPC_PTR_FMT,
                 addr, length, prot_flags_rpc2str(prot),
                 map_flags_rpc2str(flags), fd,
                 (int64_t)offset, RPC_PTR_VAL(out.retval));
    RETVAL_RPC_PTR(mmap, (rpc_ptr)out.retval);
}

/* See description in tapi_rpc_mman.h */
int
rpc_munmap(rcf_rpc_server *rpcs, rpc_ptr addr, uint64_t length)
{
    tarpc_munmap_in     in;
    tarpc_munmap_out    out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(munmap, -1);
    }

    in.addr = addr;
    in.length = length;

    rcf_rpc_call(rpcs, "munmap", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(munmap, out.retval);
    TAPI_RPC_LOG(rpcs, munmap, RPC_PTR_FMT ", "
                 "%" TE_PRINTF_64 "u", "%d",
                 RPC_PTR_VAL(addr), length, out.retval);
    RETVAL_INT(munmap, out.retval);
}

/* See description in tapi_rpc_mman.h */
int
rpc_madvise(rcf_rpc_server *rpcs,
            rpc_ptr addr, uint64_t length,
            rpc_madv_value advise)
{
    tarpc_madvise_in     in;
    tarpc_madvise_out    out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(madvise, -1);
    }

    in.addr = addr;
    in.length = length;
    in.advise = advise;

    rcf_rpc_call(rpcs, "madvise", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(madvise, out.retval);
    TAPI_RPC_LOG(rpcs, madvise, RPC_PTR_FMT ", "
                 "%" TE_PRINTF_64 "u, %s", "%d",
                 RPC_PTR_VAL(addr), length, madv_value_rpc2str(advise),
                 out.retval);
    RETVAL_INT(madvise, out.retval);
}
