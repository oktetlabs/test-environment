/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Unix Test Agent UPnP Control Point support.
 *
 * Definition of unix TA UPnP Control Point configuring support.
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __CONF_UPNP_CP_H__
#define __CONF_UPNP_CP_H__

#include "te_errno.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize the UPnP Control Point configuration subtrees and default
 * settings.
 *
 * @return Status code.
 */
extern te_errno ta_unix_conf_upnp_cp_init(void);

/**
 * Stop the UPnP Control Point process.
 *
 * @return Status code.
 */
extern te_errno ta_unix_conf_upnp_cp_release(void);

/**
 * Set the UPnP Control Point pathname for the UNIX socket which is need to
 * connect to the Control Point.
 *
 * @param ta_path       Path to the TA workspace.
 */
extern void ta_unix_conf_upnp_cp_set_socket_name(const char *ta_path);

/**
 * Get the UPnP Control Point pathname for the UNIX socket which is need to
 * connect to the Control Point.
 *
 * @return Pathname for the UNIX socket.
 */
extern const char * ta_unix_conf_upnp_cp_get_socket_name(void);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__CONF_UPNP_CP_H__ */
