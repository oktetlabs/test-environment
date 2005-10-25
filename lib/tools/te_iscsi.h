/** @file
 * @brief TE iSCSI stuff
 *
 * Functions and constant declared here can be used bothe on the agent
 * and in tests.
 *
 * Copyright (C) 2005 Test Environment authors (see file AUTHORS
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
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id: te_tools.h 18504 2005-09-17 15:59:32Z arybchik $
 */

#ifndef __TE_TOOLS_H__
#define __TE_TOOLS_H__

/* 
 * List of default parameters, used during initialization of
 * the target_data
 */
#define DEFAULT_TARGET_NAME                  "iqn.2004-01.com:0"
#define DEFAULT_MAX_CONNECTIONS              1
#define DEFAULT_INITIAL_R2T                  "Yes"
#define DEFAULT_HEADER_DIGEST                "None"
#define DEFAULT_DATA_DIGEST                  "None"
#define DEFAULT_IMMEDIATE_DATA               "Yes"
#define DEFAULT_MAX_RECV_DATA_SEGMENT_LENGTH 8192
#define DEFAULT_FIRST_BURST_LENGTH           65536
#define DEFAULT_MAX_BURST_LENGTH             262144
#define DEFAULT_DEFAULT_TIME2WAIT            2
#define DEFAULT_DEFAULT_TIME2RETAIN          20
#define DEFAULT_MAX_OUTSTANDING_R2T          1
#define DEFAULT_DATA_PDU_IN_ORDER            "Yes"
#define DEFAULT_DATA_SEQUENCE_IN_ORDER       "Yes"
#define DEFAULT_ERROR_RECOVERY_LEVEL         0
#define DEFAULT_SESSION_TYPE                 "Normal"
#define DEFAULT_CHAP                         "None"
#define DEFAULT_CHALLENGE_LENGTH             256
#define DEFAULT_INITIATOR_NAME \
    "iqn.1999-11.edu.unh.iol.iscsi-initiator"
#define DEFAULT_INITIATOR_ALIAS             "UNH"

/**
 * Flags:
 * the following assumption holds:
 * if parameter of the local Initiator structure 
 * was untouched than it should not be synchronized
 * with the Initiator. Than the Initiator uses the
 * default parameter and MAY NOT offer the parameter
 * during the negotiations.
 */
#define OFFER_0                                 0
#define OFFER_MAX_CONNECTIONS                   (1 << 0)
#define OFFER_INITIAL_R2T                       (1 << 1)
#define OFFER_HEADER_DIGEST                     (1 << 2)
#define OFFER_DATA_DIGEST                       (1 << 3)
#define OFFER_IMMEDIATE_DATA                    (1 << 4)
#define OFFER_MAX_RECV_DATA_SEGMENT_LENGTH      (1 << 5)
#define OFFER_FIRST_BURST_LENGTH                (1 << 6)
#define OFFER_MAX_BURST_LENGTH                  (1 << 7)
#define OFFER_DEFAULT_TIME2WAIT                 (1 << 8)
#define OFFER_DEFAULT_TIME2RETAIN               (1 << 9)
#define OFFER_MAX_OUTSTANDING_R2T               (1 << 10)
#define OFFER_DATA_PDU_IN_ORDER                 (1 << 11)
#define OFFER_DATA_SEQUENCE_IN_ORDER            (1 << 12)
#define OFFER_ERROR_RECOVERY_LEVEL              (1 << 13)

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TOOLS_H__ */
