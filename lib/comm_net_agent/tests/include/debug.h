/** @file 
 * @brief Test Environment
 * Network Communication Library Tests - Test Agent side - Library
 * Connections API
 *
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 * Author: Pavel A. Bolokhov <Pavel.Bolokhov@oktetlabs.ru>
 * 
 * @(#) $Id$
 */
#ifndef __TE_COMM_NET_AGENT_TESTS_LIB_DEBUG_H__
#define __TE_COMM_NET_AGENT_TESTS_LIB_DEBUG_H__
#ifdef __cplusplus
extern "C" {
#endif
#include "config.h"

#define DEBUG(x...)                             \
    do {                                   \
        fprintf(stderr, x);                     \
    } while (0)

#ifdef __cplusplus
}
#endif
#endif /* __TE_COMM_NET_AGENT_TESTS_LIB_DEBUG_H__ */
