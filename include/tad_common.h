/** @file
 * @brief Traffic Application Domain definitions
 *
 * Common RCF Traffic Application Domain definitions.
 * 
 * 
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS
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
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 * 
 * $Id$
 */

#ifndef __TE_TAD_COMMON_H__
#define __TE_TAD_COMMON_H__

/**
 * Infinitive timeout to wait forever.
 *
 * @todo Put it in appropriate place.
 */
#define TAD_TIMEOUT_INF     (unsigned int)(-1)


#define CSAP_PARAM_STATUS               "status"
#define CSAP_PARAM_TOTAL_BYTES          "total_bytes"
#define CSAP_PARAM_FIRST_PACKET_TIME    "first_pkt_time"
#define CSAP_PARAM_LAST_PACKET_TIME     "last_pkt_time"


/** Type for CSAP handle */
typedef int csap_handle_t;


typedef enum {
    CSAP_IDLE,          /**< CSAP is ready for traffic operations or 
                             destroy */
    CSAP_BUSY,          /**< CSAP is busy with some traffic processing */
    CSAP_COMPLETED,     /**< Last traffic processing completed and CSAP
                             waiting for *_stop command from Test. */
    CSAP_ERROR,         /**< Error was occured during processing, CSAP
                             waiting for *_stop command from Test. */
} tad_csap_status_t;
    

#endif /* !__TE_TAD_COMMON_H__ */
