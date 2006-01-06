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

#include "te_stdint.h"

/**
 * Infinitive timeout to wait forever.
 *
 * @todo Put it in appropriate place.
 */
#define TAD_TIMEOUT_INF     (unsigned int)(-1)


#define CSAP_PARAM_STATUS               "status"
#define CSAP_PARAM_TOTAL_BYTES          "total_bytes"
#define CSAP_PARAM_TOTAL_SENT           "total_sent"
#define CSAP_PARAM_TOTAL_RECEIVED       "total_received"
#define CSAP_PARAM_FIRST_PACKET_TIME    "first_pkt_time"
#define CSAP_PARAM_LAST_PACKET_TIME     "last_pkt_time"


/** 
 * Type for CSAP handle, should have semantic unsigned integer, 
 * because TAD Users Guide specify CSAP ID as positive integer, and
 * zero used as mark of invalid/uninitialized CSAP handle. 
 */ 
typedef unsigned int csap_handle_t;
enum {
    CSAP_INVALID_HANDLE = 0, /**< Constant for invalid CSAP handle */
};


typedef enum {
    CSAP_IDLE,          /**< CSAP is ready for traffic operations or 
                             destroy */
    CSAP_BUSY,          /**< CSAP is busy with some traffic processing */
    CSAP_COMPLETED,     /**< Last traffic processing completed and CSAP
                             waiting for *_stop command from Test. */
    CSAP_ERROR,         /**< Error was occured during processing, CSAP
                             waiting for *_stop command from Test. */
} tad_csap_status_t;
    
/**
 * Constants for ASN.1 tags of protocol choices in PDUs and CSAPs 
 * and for marks in CSAP instance and CSAP support descriptors.
 */
typedef enum { 
    TE_PROTO_INVALID = 0, /**< Invalid protocol, for error and undef */

    TE_PROTO_ARP,
    TE_PROTO_BRIDGE,
    TE_PROTO_CLI,
    TE_PROTO_DHCP,
    TE_PROTO_ETH,
    TE_PROTO_FILE,
    TE_PROTO_ICMP4,
    TE_PROTO_IP4,
    TE_PROTO_ISCSI,
    TE_PROTO_PCAP,
    TE_PROTO_SNMP,
    TE_PROTO_TCP,
    TE_PROTO_UDP,
    TE_PROTO_SOCKET,
} te_tad_protocols_t;



/**
 * Calculate 16-bit checksum: 16-bit one's complement of the one's
 * complement sum of all 16 bit words.
 * Function works correctly with length less then 64k.
 *
 * @param data    pointer to the data which checksum should be calculated
 * @param length  length of the data
 *
 * @return calculated checksum.
 */
static inline uint16_t
calculate_checksum(const void *data, size_t length)
{
    uint32_t  checksum;
    uint16_t *ch_p;

    for (ch_p = (uint16_t *)data, checksum = 0; 
         length >= 2;
         length -= 2, checksum += *(ch_p++)); 
    if (length == 1)
    {
        union {uint8_t bytes[2]; uint16_t num;} a;
        a.bytes[0] = *((uint8_t *)ch_p);
        a.bytes[1] = 0;
        checksum += a.num;
    }

    return (checksum & 0xffff) + (checksum >> 16);
}

/**
 * Callback type for methods generating fully determined stream of data,
 * depending only from length and offset.
 *
 * @param offset        offset in stream of first byte in buffer
 * @param length        length of buffer
 * @param buffer        location for generated data (OUT)
 *
 * @return status code
 */
typedef int (*tad_stream_callback)(uint64_t offset, uint32_t length,
                                   uint8_t *buffer);

#endif /* !__TE_TAD_COMMON_H__ */
