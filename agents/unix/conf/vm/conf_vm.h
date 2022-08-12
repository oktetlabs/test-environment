/** @file
 * @brief Unix Test Agent
 *
 * Unix TA virtual machines support
 *
 * Copyright (C) 2019-2022 OKTET Labs. All rights reserved.
 */

#ifndef __TE_AGENTS_UNIX_CONF_VM_H_
#define __TE_AGENTS_UNIX_CONF_VM_H_

#include "te_errno.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize virtual machines configuration.
 *
 * @return Status code.
 */
extern te_errno ta_unix_conf_vm_init(void);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_AGENTS_UNIX_CONF_VM_H_ */
