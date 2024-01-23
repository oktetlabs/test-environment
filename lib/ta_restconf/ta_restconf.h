/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2024 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief RESTCONF agent library
 *
 * The library provides types, functions and configuration tree
 * for RESTCONF management.
 */

#ifndef __TA_RESTCONF_H__
#define __TA_RESTCONF_H__

#include "te_errno.h"
#include "te_defs.h"
#include "logger_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize the RESTCONF agent configuration subtrees and default settings.
 *
 * @return Status code.
 */
extern te_errno ta_restconf_conf_init(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __TA_RESTCONF_H__ */
