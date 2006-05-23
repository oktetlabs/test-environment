/** @file
 * @brief Unix Test Agent
 *
 * Unix TA configuring support using DLPI (definitions).
 *
 * Copyright (C) 2006 Test Environment authors (see file AUTHORS
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
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
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

#endif /* !__TE_TA_UNIX_CONF_DLPI_H__ */
