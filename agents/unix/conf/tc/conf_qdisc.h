/** @file
 * @brief Unix TA Traffic Control qdisc configuration support
 *
 * Implementation of get/set methods for qdisc node
 *
 * Copyright (C) 2018 OKTET Labs. All rights reserved.
 *
 * @author Nikita Somenkov <Nikita.Somenkov@oktetlabs.ru>
 */

#ifndef __TE_AGENTS_UNIX_CONF_CONF_QDISC_H_
#define __TE_AGENTS_UNIX_CONF_CONF_QDISC_H_

#include "te_errno.h"

/**
 * 'get' method implementation for TC parent
 *
 * @param gid       group identifier
 * @param oid       full object instance identifier
 * @param value     location for the value
 *
 * @return Status code
 */
extern te_errno conf_qdisc_parent_get(unsigned int gid, const char *oid,
                                      char *value);

/**
 * 'get' method implementation for TC handle
 *
 * @param gid       group identifier
 * @param oid       full object instance identifier
 * @param value     location for the value
 *
 * @return Status code
 */
extern te_errno conf_qdisc_handle_get(unsigned int gid, const char *oid,
                                      char *value);

/**
 * 'set' method implementation for TC qdisc enabled
 *
 * @param gid       group identifier
 * @param oid       full object instance identifier
 * @param value     location for the value
 * @param if_name   interface name
 *
 * @return Status code
 */
extern te_errno conf_qdisc_enabled_set(unsigned int gid, const char *oid,
                                       const char *value, const char *if_name);

/**
 * 'get' method implementation for TC qdisc enabled
 *
 * @param gid       group identifier
 * @param oid       full object instance identifier
 * @param value     location for the value
 * @param if_name   interface name
 *
 * @return Status code
 */
extern te_errno conf_qdisc_enabled_get(unsigned int gid, const char *oid,
                                       char *value, const char *if_name);

/**
 * 'get' method implementation for TC kind
 *
 * @param gid       group identifier
 * @param oid       full object instance identifier
 * @param value     location for the value
 * @param if_name   interface name
 *
 * @return Status code
 */
extern te_errno conf_qdisc_kind_get(unsigned int gid, const char *oid,
                                    char *value, const char *if_name);

/**
 * 'set' method implementation for TC kind
 *
 * @param gid       group identifier
 * @param oid       full object instance identifier
 * @param value     location for the value
 * @param if_name   interface name
 *
 * @return Status code
 */
extern te_errno conf_qdisc_kind_set(unsigned int gid, const char *oid,
                                    const char *value, const char *if_name);

#endif /* __TE_AGENTS_UNIX_CONF_CONF_QDISC_H_ */
