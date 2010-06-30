/** @file
 * @brief Test GateWay network configuring API
 *
 * Macros to be used in tests. The header must be included from test
 * sources only. It is allowed to use the macros only from @b main()
 * function of the test.
 *
 *
 * Copyright (C) 2004 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
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
 * @author Konstantin Abramenko <Konstantin Abramenko@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_TAPI_GW_H__
#define __TE_TAPI_GW_H__

/**
 * Get tagged network address from Configurator.
 *
 * @param _addr   variable for address.
 * @param _tag    name in CS of address. 
 */
#define TEST_GET_TAG_ADDR(_addr, _tag) \
    do {                                                            \
        te_errno rc = tapi_cfg_get_tagged_addr(#_tag, &(_addr));    \
        if (rc != 0)                                                \
            TEST_FAIL("Failed to get tagged address: %r", rc);      \
    } while (0)



/**
 * Get name of tagged network interface from Configurator.
 *
 * @param _if     variable for interface name.
 * @param _tag    name in CS of interface. 
 */
#define TEST_GET_TST_IF(_if, _tag)  \
    do {                                                                \
        char *string;                                                   \
        te_errno rc = cfg_get_instance_fmt(NULL, &(string),             \
                                           "/local:/tst_if:" # _tag);   \
        if (rc != 0)                                                    \
            TEST_FAIL("Failed to get name of tester "                   \
                      "interface: %r", rc);                             \
        if (tapi_is_cfg_link(string))                                   \
        {                                                               \
            (_if) = strdup(strrchr(string, ':') + 1);                   \
        }                                                               \
        else                                                            \
            (_if) = string;                                             \
    } while (0)

#endif /* !__TE_TAPI_GW_H__ */
