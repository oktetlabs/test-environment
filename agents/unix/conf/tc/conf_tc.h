/** @file
 * @brief Unix TA Traffic Control configuration support
 *
 * Implementation of configuration nodes traffic control support
 *
 * Copyright (C) 2018-2022 OKTET Labs. All rights reserved.
 *
 */

#ifndef __TE_AGENTS_UNIX_CONF_CONF_TC_H_
#define __TE_AGENTS_UNIX_CONF_CONF_TC_H_

#include "te_errno.h"

/**
 * Initialization of Traffic Control configuration
 *
 * @return Status code
 */
extern te_errno ta_unix_conf_tc_init(void);

/**
 * Clean up of Traffic Control configuration
 */
extern void ta_unix_conf_tc_fini(void);

#endif /* __TE_AGENTS_UNIX_CONF_CONF_TC_H_ */
