/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2025 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief Auxiliary library for interacting with test environment.
 *
 * @defgroup tapi_net_env Auxiliary library for interacting with test environment.
 * @ingroup tapi_net
 * @{
 *
 * Definition of helpers for interacting with test environment.
 */

#ifndef __TAPI_NET_ENV_H__
#define __TAPI_NET_ENV_H__

#include "tapi_env.h"
#include "tapi_net.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Get the full interface stack for a given base interface.
 *
 * Name of the base interface must match name of the
 * interface in environment configuration parameter.
 *
 * @param env            Test environment.
 * @param net_ctx        Context with logical interfaces.
 * @param base_iface     Name of the base interface.
 *
 * @return Pointer to the head of the interface stack or @c NULL when missing.
 */
extern const tapi_net_iface_head *tapi_net_env_get_iface_stack(
                                      tapi_env *env, tapi_net_ctx *net_ctx,
                                      const char *base_iface);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* !__TAPI_NET_ENV_H__ */

/**@} <!-- END tapi_net_env --> */
