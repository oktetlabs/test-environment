/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief API to modify target requirements from prologues
 *
 * @defgroup ts_tapi_reqs Target requirements modification
 * @ingroup te_ts_tapi
 * @{
 *
 * Declaration of API to modify target requirements from prologues.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_TAPI_REQS_H__
#define __TE_TAPI_REQS_H__

#include "te_errno.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Modify the set of defined requirements for tests
 *
 * @param reqs          Requirements expression
 *
 * @return Status code.
 */
extern te_errno tapi_reqs_modify(const char *reqs);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_REQS_H__ */

/**@} <!-- END ts_tapi_reqs --> */
