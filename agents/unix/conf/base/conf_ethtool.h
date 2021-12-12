/** @file
 * @brief Unix Test Agent
 *
 * Common API for SIOCETHTOOL usage in Unix TA configuration
 *
 *
 * Copyright (C) 2021 OKTET Labs. All rights reserved.
 *
 *
 * @author Dmitry Izbitsky <Dmitry.Izbitsky@oktetlabs.ru>
 */

#ifndef __TE_AGENTS_UNIX_CONF_BASE_CONF_ETHTOOL_H_
#define __TE_AGENTS_UNIX_CONF_BASE_CONF_ETHTOOL_H_

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

#include "te_defs.h"
#include "te_errno.h"

/**
 * Call @c SIOCETHTOOL ioctl() to get or set some values.
 *
 * @param if_name     Name of the interface
 * @param cmd         Ethtool command number
 * @param value       Pointer to Ethtool command structure
 *
 * @return Status code.
 */
extern te_errno call_ethtool_ioctl(const char *if_name, int cmd,
                                   void *value);

/**
 * Get a pointer to Ethtool command structure to work with.
 * It the structure is needed for set request, pointer will be to
 * a dynamically allocated structure associated with a given interface.
 * Otherwise - to locally declared structure.
 * Structure fields are filled with help of related Ethtool get command.
 * Dynamically allocated structure is filled only when it is requested
 * the first time.
 *
 * @param if_name         Interface name
 * @param gid             Group ID
 * @param cmd             Ethtool command number
 * @param ecmd_local      Pointer to locally declared
 *                        command structure. Must not be @c NULL
 * @param ecmd_out        Here requested pointer will be saved
 * @param do_set          If @c TRUE, it is a set request
 *
 * @return Status code.
 */
extern te_errno get_ethtool_value(const char *if_name, unsigned int gid,
                                  int cmd, void *val_local, void **ptr_out,
                                  te_bool do_set);

/**
 * Commit configuration changes via @c SIOCETHTOOL.
 * It is assumed that changes made by one or more set operations are
 * saved in an object stored for a given interface which can be
 * retrieved with ta_obj_find().
 *
 * @param if_name         Interface name
 * @param cmd             Ethtool command telling what to commit
 *                        (@c ETHTOOL_SCOALESCE, @c ETHTOOL_SPAUSEPARAM)
 *
 * @return Status code.
 */
extern te_errno commit_ethtool_value(const char *if_name, int cmd);

#endif /* !__TE_AGENTS_UNIX_CONF_BASE_CONF_ETHTOOL_H_ */
