/** @file
 * @brief Unix Test Agent
 *
 * Unix TA rules configuration declarations
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 */

#ifndef __TE_AGENTS_UNIX_CONF_RULE_H_
#define __TE_AGENTS_UNIX_CONF_RULE_H_

#include "conf_ip_rule.h"
#include "te_errno.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize rules configuration.
 *
 * @return Status code.
 */
extern te_errno ta_unix_conf_rule_init(void);


/**
 * Find a rule and fill the optional fields.
 *
 * @param [in]   required   Required fields for comparison
 * @param [in,out]rule      Rule related information
 *
 * @return Status code.
 */
extern te_errno ta_unix_conf_rule_find(
        uint32_t required, te_conf_ip_rule *rule);

/**
 * Get instances list for object "/agent/rule".
 *
 * @param [out]list         Location for the list pointer
 *
 * @return Status code.
 */
extern te_errno ta_unix_conf_rule_list(char **list);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* !__TE_AGENTS_UNIX_CONF_RULE_H_ */
