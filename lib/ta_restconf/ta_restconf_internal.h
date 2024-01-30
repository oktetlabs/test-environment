/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2024 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief RESTCONF agent library
 *
 * Types and functions for internal use.
 */

#ifndef __TA_RESTCONF_INTERNAL_H__
#define __TA_RESTCONF_INTERNAL_H__

#include <inttypes.h>

#include "te_errno.h"
#include "rcf_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Structure for RESTCONF configuration settings. */
typedef struct restconf_settings {
    /**< Server IP address or hostname. */
    char host[RCF_MAX_VAL];
    /**< Server port. */
    uint16_t port;
    /**< Whether use https or http when connecting to server. */
    te_bool https;
    /**< Username to use when connecting to server. */
    char username[RCF_MAX_VAL];
    /**< Password to use when connecting to server. */
    char password[RCF_MAX_VAL];
} restconf_settings;

/** RESTCONF configuration settings. */
extern restconf_settings restconf;

/**
 * Initialize the RESTCONF agent /config/server configuration subtree.
 *
 * @return Status code.
 */
extern te_errno ta_restconf_conf_server_init(void);

/**
 * Initialize the RESTCONF agent /config/search configuration subtree.
 *
 * @return Status code.
 */
extern te_errno ta_restconf_conf_search_init(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __TA_RESTCONF_INTERNAL_H__ */
