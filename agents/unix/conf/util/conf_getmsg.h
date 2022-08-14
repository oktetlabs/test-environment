/** @file
 * @brief Unix Test Agent configuration using getmsg()/putmsg()
 *
 * Definitions for Unix TA configuration with getmsg()/putmsg()
 *
 * Copyright (C) 2019-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_CONF_GETMSG_H__
#define __TE_CONF_GETMSG_H__

#include "te_config.h"

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include "te_errno.h"
#include "te_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

#if HAVE_INET_MIB2_H

/**
 * Get list of neighbor table entries for a given interface.
 *
 * @param iface         Interface name.
 * @param is_static     If @c TRUE, list static (permanent) entries,
 *                      otherwise list dynamic entries.
 * @param list          Where to save list of entries names (addresses)
 *                      as string of names separated by spaces.
 *
 * @return Status code.
 */
extern te_errno ta_unix_conf_neigh_list_getmsg(const char *iface,
                                               te_bool is_static,
                                               char **list);
#endif /* HAVE_INET_MIB2_H */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ndef __TE_CONF_GETMSG_H__ */
