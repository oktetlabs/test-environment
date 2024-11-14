/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2024 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief Test API to configure tunnel.
 *
 * @defgroup tapi_conf_tunnel Tunnel configuration
 * @ingroup tapi_conf
 * @{
 *
 * Definition of TAPI to configure tunnel.
 */

#ifndef __TE_TAPI_CFG_TUNNEL_H__
#define __TE_TAPI_CFG_TUNNEL_H__

#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#include "te_defs.h"
#include "te_errno.h"
#include "te_stdint.h"

#ifdef __cplusplus
extern "C" {
#endif

/** List of supported tunnels */
typedef enum tapi_cfg_tunnel_type {
    TAPI_CFG_TUNNEL_TYPE_VXLAN,
} tapi_cfg_tunnel_type;

typedef struct tapi_cfg_tunnel_vxlan {
    /** Source IP address (can't be @c NULL) */
    struct sockaddr *local;
    /** Unicast or multicast destination IP address (can't be @c NULL) */
    struct sockaddr *remote;
    /** Physical interface (can't be @c NULL) */
    char *if_name;
    /** Virtual Network Identifier */
    int32_t vni;
    /** UDP destination port (@c 4789 by default) */
    uint16_t port;
} tapi_cfg_tunnel_vxlan;

/** Tunnel configuration */
typedef struct tapi_cfg_tunnel {
    /** Tunnel name (can't be @c NULL) */
    char *tunnel_name;
    /** Tunnel type (@sa tapi_cfg_tunnel_type) */
    enum tapi_cfg_tunnel_type type;
    /** Tunnel status (if @c 1 created or @c 0) */
    bool status;
    /** Tunnel specific information */
    union {
        tapi_cfg_tunnel_vxlan vxlan;
    };
} tapi_cfg_tunnel;

/**
 * Add tunnel interface.
 *
 * @param ta            Test Agent.
 * @param conf          Tunnel configuration.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_tunnel_add(const char *ta,
                                    const tapi_cfg_tunnel *conf);

/**
 * Enable tunnel.
 *
 * @param ta            Test Agent.
 * @param conf          Tunnel configuration.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_tunnel_enable(const char *ta,
                                       const tapi_cfg_tunnel *conf);

/**
 * Disable tunnel.
 *
 * @param ta            Test Agent.
 * @param conf          Tunnel configuration.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_tunnel_disable(const char *ta,
                                        const tapi_cfg_tunnel *conf);

/**
 * Delete tunnel.
 *
 * @param ta            Test Agent.
 * @param tunnel_name   Tunnel name.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_tunnel_del(const char *ta, const char *tunnel_name);

/**
 * Get tunnel configuration information.
 *
 * @param ta            Test Agent.
 * @param tunnel_name   Tunnel name.
 * @param conf          Pointer of tunnel configuration to store.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_tunnel_get(const char *ta,
                                    const char *tunnel_name,
                                    tapi_cfg_tunnel *conf);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_CFG_TUNNEL_H__ */
