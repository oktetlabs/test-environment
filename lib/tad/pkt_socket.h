/** @file
 * @brief Test Environment: 
 *
 * Traffic Application Domain Command Handler
 *
 * Declarations of types and functions, used in common and 
 * protocol-specific modules implemnting TAD.
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
 * @author Konstantin Abramenko <konst@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_TAD_PKT_SOCKET_H__
#define __TE_TAD_PKT_SOCKET_H__ 

#ifndef PACKAGE_VERSION
#include "config.h"
#endif

#if HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_PTHREAD_H
#include <pthread.h>
#endif

#include "te_stdint.h"
#include "te_errno.h"
#include "asn_usr.h" 
#include "tad_common.h"
#include "rcf_ch_api.h"

#ifdef __cplusplus
extern "C" {
#endif





/* ============= Types and structures definitions =============== */


/* 
 * Ethernet interface related data
 * 
 */
struct eth_csap_interface;
typedef struct eth_csap_interface *eth_csap_interface_p;
typedef struct eth_csap_interface
{
    char  name[IFNAME_SIZE];    /**< Etherner interface name (e.g. eth0) */
    int   if_index;             /**< Interface index                     */
    
    char  local_addr[ETH_ALEN]; /**< Hardware address of the home interface
                                     Extracted through ioctls if not defined
                                     by the user in the configuration param.
                                     (may differ from real hardware address)
                                 */
    
} eth_csap_interface_t;

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /*  __TE_TAD_PKT_SOCKET_H__ */
