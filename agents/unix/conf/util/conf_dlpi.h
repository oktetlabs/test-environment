/** @file
 * @brief Unix Test Agent
 *
 * Unix TA configuring support using DLPI (definitions).
 *
 * Copyright (C) 2004-2018 OKTET Labs. All rights reserved.
 *
 *
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 */

#ifndef __TE_TA_UNIX_CONF_DLPI_H__
#define __TE_TA_UNIX_CONF_DLPI_H__

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

/**
 * Get physical address using DLPI.
 *
 * @param name          Interface name
 * @param addr          Buffer for address
 * @param addrlen       Buffer size (in), address length (out)
 *
 * @return Status code.
 */
extern te_errno ta_unix_conf_dlpi_phys_addr_get(const char *name,
                                                void       *addr,
                                                size_t     *addrlen);

/**
 * Set physical address using DLPI.
 *
 * @param name          Interface name
 * @param addr          Address
 * @param addrlen       Address length
 *
 * @return Status code.
 */
extern te_errno ta_unix_conf_dlpi_phys_addr_set(const char *name,
                                                void       *addr,
                                                size_t      addrlen);

/**
 * Get physical broadcast address using DLPI.
 *
 * @param name          Interface name
 * @param addr          Buffer for address
 * @param addrlen       Buffer size (in), address length (out)
 *
 * @return Status code.
 */
extern te_errno ta_unix_conf_dlpi_phys_bcast_addr_get(const char *name,
                                                      void       *addr,
                                                      size_t     *addrlen);

#endif /* !__TE_TA_UNIX_CONF_DLPI_H__ */
