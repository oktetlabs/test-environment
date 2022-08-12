/** @file
 * @brief API for converting various types in RPC calls
 *
 * Implementation of API used to convert between TARPC types and
 * native types.
 *
 * Copyright (C) 2020-2022 OKTET Labs. All rights reserved.
 */

#include "rpcs_conv.h"

void
rpcs_iovec_tarpc2h(const struct tarpc_iovec *tarpc_iov, struct iovec *iov,
                   size_t count, te_bool may_change,
                   checked_arg_list *arglist)
{
    size_t i;

    memset(iov, 0, sizeof(*iov) * count);
    for (i = 0; i < count; i++)
    {
        iov[i].iov_base = tarpc_iov[i].iov_base.iov_base_val;
        iov[i].iov_len = tarpc_iov[i].iov_len;

        INIT_CHECKED_ARG(iov[i].iov_base,
                         tarpc_iov[i].iov_base.iov_base_len,
                         (may_change ? iov[i].iov_len : 0));
    }
    INIT_CHECKED_ARG(iov, sizeof(*iov) * count, 0);
}
