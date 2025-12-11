/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2025 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief Test API to allocate L4 port
 *
 * @defgroup tapi_conf_l4_port Test API to allocate L4 port
 * @ingroup tapi_conf
 * @{
 */

#ifndef __TE_TAPI_CFG_L4_PORT_H__
#define __TE_TAPI_CFG_L4_PORT_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Allocate free L4 port.
 *
 * @param[in] family    Address family.
 * @param[in] type      @c SOCK_STREAM or @c SOCK_DGRAM.
 * @param[out] port     Location for allocated port.
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_l4_port_alloc(const char *ta, int family, int type,
                                       uint16_t *port);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_CFG_L4_PORT_H__ */

/**@} <!-- END tapi_conf_l4_port --> */
