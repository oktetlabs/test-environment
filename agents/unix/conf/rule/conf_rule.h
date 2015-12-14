/** @file
 * @brief Unix Test Agent
 *
 * Unix TA rules configuration declarations
 *
 *
 * Copyright (C) 2015 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 *
 * @author Oleg Sadakov <Oleg.Sadakov@oktetlabs.ru>
 *
 * $Id$
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
 * @param [inout]rule       Rule related information
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
