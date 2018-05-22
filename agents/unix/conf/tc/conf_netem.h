/** @file
 * @brief Unix TA Traffic Control qdisc netem configuration support
 *
 * Implementation of get/set methods for qdisc netem node
 *
 * Copyright (C) 2018 OKTET Labs. All rights reserved.
 *
 * @author Nikita Somenkov <Nikita.Somenkov@oktetlabs.ru>
 */

#ifndef __TE_AGENTS_UNIX_CONF_CONF_NETEM_H_
#define __TE_AGENTS_UNIX_CONF_CONF_NETEM_H_

#include "te_errno.h"

/**
 * 'set' method implementation for TC qdisc netem
 *
 * @param gid       group identifier
 * @param oid       full object instance identifier
 * @param value     location for the value
 * @param if_name   interface name
 * @param tc        Trafic Control value (empty string)
 * @param qdisc     QDisc value (empty string)
 * @param param     parameter name for set value
 *
 * @return Status code
 */
extern te_errno conf_netem_param_set(unsigned int gid, const char *oid,
                                     const char *value, const char *if_name,
                                     const char *tc, const char *qdisc,
                                     const char *param);

/**
 * 'get' method implementation for TC qdisc netem
 *
 * @param gid       group identifier
 * @param oid       full object instance identifier
 * @param value     location for the value
 * @param if_name   interface name
 * @param tc        Trafic Control value (empty string)
 * @param qdisc     QDisc value (empty string)
 * @param param     parameter name for get value
 *
 * @return Status code
 */
extern te_errno conf_netem_param_get(unsigned int gid, const char *oid,
                                     char *value, const char *if_name,
                                     const char *tc, const char *qdisc,
                                     const char *param);

/**
 * 'add' method implementation for TC qdisc netem
 *
 * @param gid       group identifier
 * @param oid       full object instance identifier
 * @param value     parameter for add
 *
 * @return Status code
 */
extern te_errno conf_netem_param_add(unsigned int gid, const char *oid,
                                     const char *value);

/**
 * 'del' method implementation for TC qdisc netem
 *
 * @param gid       group identifier
 * @param oid       full object instance identifier
 * @param value     parameter for del
 *
 * @return Status codehg
 */
extern te_errno conf_netem_param_del(unsigned int gid, const char *oid,
                                     const char *value);

/**
 * 'list' method implementation for TC qdisc netem
 *
 * @param gid       group identifier
 * @param oid       full object instance identifier
 * @param sub_id    ID (name) of the object to be listed
 * @param list      location for the returned list pointer
 * @param if_name   interface name
 *
 * @return Status code
 */
extern te_errno conf_netem_param_list(unsigned int gid, const char *oid,
                                      const char *sub_id, char **list,
                                      const char *if_name);

#endif /* __TE_AGENTS_UNIX_CONF_CONF_NETEM_H_ */
