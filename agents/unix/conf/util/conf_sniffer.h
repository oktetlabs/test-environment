/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Unix Test Agent sniffers support.
 *
 * Defenition of unix TA sniffers configuring support.
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_CONF_SNIFFER_H__
#define __TE_CONF_SNIFFER_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize sniffers configuration.
 *
 * @return Status code.
 */
extern te_errno ta_unix_conf_sniffer_init(void);

/**
 * Cleanup sniffers function.
 *
 * @return Status code.
 */
extern te_errno ta_unix_conf_sniffer_cleanup(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ndef __TE_CONF_SNIFFER_H__ */
