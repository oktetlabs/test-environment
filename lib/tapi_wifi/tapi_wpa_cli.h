/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Control WPA client
 *
 * @defgroup tapi_wifi_wpa_cli Control WPA client
 * @ingroup tapi_wifi
 * @{
 *
 * Test API to control WPA client tool.
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TAPI_WPA_CLI_H__
#define __TAPI_WPA_CLI_H__

#include "te_defs.h"
#include "te_errno.h"
#include "rcf_rpc.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Invoke wpa_cli command for certain Wi-Fi client.
 *
 * @param rpcs          RPC server handle.
 * @param ifname        Client's wireless interface name.
 * @param command       Array representing wpa_cli command with its arguments,
 *                      must be terminated with @c NULL. For example,
 *                      `{ "wps_pbc", NULL }`.
 * @param timeout_ms    Command execution maximum time (in milliseconds).
 *
 * @return Status code.
 */
extern te_errno tapi_wpa_cli(rcf_rpc_server *rpcs,
                             const char *ifname,
                             const char *command[],
                             int timeout_ms);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TAPI_WPA_CLI_H__ */

/**@} <!-- END tapi_wifi_wpa_cli --> */
