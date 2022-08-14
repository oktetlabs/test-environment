/** @file
 * @brief Unix TA Traffic Control qdisc netem configuration support
 *
 * Implementation of get/set methods for qdisc netem node
 *
 * Copyright (C) 2018-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_AGENTS_UNIX_CONF_CONF_QDISC_PARAMS_H_
#define __TE_AGENTS_UNIX_CONF_CONF_QDISC_PARAMS_H_

#include "te_errno.h"

/**
 * 'set' method implementation for TC qdisc netem/tbf
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
extern te_errno conf_qdisc_param_set(unsigned int gid, const char *oid,
                                     const char *value, const char *if_name,
                                     const char *tc, const char *qdisc_str,
                                     const char *param);

/**
 * 'get' method implementation for TC qdisc netem/tbf
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
extern te_errno conf_qdisc_param_get(unsigned int gid, const char *oid,
                                     char *value, const char *if_name,
                                     const char *tc, const char *qdisc,
                                     const char *param);

/**
 * 'add' method implementation for TC qdisc netem/tbf
 *
 * @param gid       group identifier
 * @param oid       full object instance identifier
 * @param value     parameter for add
 * @param if_name   interface name
 * @param tc        Trafic Control value (empty string)
 * @param qdisc     QDisc value (empty string)
 * @param param     parameter name
 *
 * @return Status code
 */
extern te_errno conf_qdisc_param_add(unsigned int gid, const char *oid,
                                     const char *value, const char *if_name,
                                     const char *tc, const char *qdisc,
                                     const char *param);

/**
 * 'del' method implementation for TC qdisc netem/tbf
 *
 * @param gid       group identifier
 * @param oid       full object instance identifier
 * @param if_name   interface name
 * @param tc        Trafic Control value (empty string)
 * @param qdisc     QDisc value (empty string)
 * @param param     Parameter name to delete
 *
 * @return Status code
 */
extern te_errno conf_qdisc_param_del(unsigned int gid, const char *oid,
                                     const char *if_name, const char *tc,
                                     const char *qdisc, const char *param);

/**
 * 'list' method implementation for TC qdisc netem/tbf
 *
 * @param gid       group identifier
 * @param oid       full object instance identifier
 * @param sub_id    ID (name) of the object to be listed
 * @param list      location for the returned list pointer
 * @param if_name   interface name
 *
 * @return Status code
 */
extern te_errno conf_qdisc_param_list(unsigned int gid, const char *oid,
                                      const char *sub_id, char **list,
                                      const char *if_name);

/**
 * Free TC qdisc tbf parameters objects
 */
extern void conf_qdisc_tbf_params_free(void);

/**
 * Free TC qdisc clsact parameters objects.
 */
extern void conf_qdisc_clsact_params_free(void);

#endif /* __TE_AGENTS_UNIX_CONF_CONF_QDISC_PARAMS_H_ */
