/** @file
 * @brief Unix Test Agent sniffers support.
 *
 * Defenition of unix TA sniffers configuring support.
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
 *
 *
 * @author Andrey Dmitrov <Andrey.Dmitrov@oktetlabs.ru>
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
