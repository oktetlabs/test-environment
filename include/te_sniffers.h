/** @file
 * @brief Unix Test Agent sniffers support.
 *
 * Defenition of sniffers support.
 *
 * Copyright (C) 2004-2012 Test Environment authors (see file AUTHORS
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
 * @author Andrey Dmitrov <Andrey.Dmitrov@oktetlabs.ru>
 *
 * $Id: 
 */

#ifndef __TE_SNIFFERS_H__
#define __TE_SNIFFERS_H__

 /** Sniffer identifier */
typedef struct sniffer_id {
    char        *ifname;    /**< Interface name. */
    char        *snifname;  /**< Sniffer name. */
    int          ssn;       /**< Sniffer session sequence number. */

    unsigned long long    abs_offset;  /**< Absolute offset of the first byte
                                            of the first packet in a packets 
                                            portion. */
} sniffer_id;


/* Size of a PCAP file header. */
#define SNIF_PCAP_HSIZE 24


/* Size of the sniffer marker packet protocol. */
#define SNIF_MARK_PSIZE 34

/**
 * Initialization of a header for a marker packet.
 * 
 * @param _proto    Pointer to the allocated header location.
 * @param _msglen   Length of the user message for the packet.
 */
#define SNIFFER_MARK_H_INIT(_proto, _msglen) \
{ \
    memset(_proto, 0, SNIF_MARK_PSIZE); \
    _proto[12] = 0x08;       /* Ether Type */ \
    _proto[14] = 0x45;       /* Version 4, Header length 20 bytes */ \
    /* Total length 52 */ \
    _proto[17] = SNIF_MARK_PSIZE + _msglen; \
    _proto[20] = 0x40;       /* Flags: Don't fragment */ \
    _proto[23] = 0x3D;       /* Transport layer _protocol: Any host \
                                internal protocol */ \
}

typedef struct te_ts_t{
        uint32_t tv_sec;
        uint32_t tv_usec;
} te_ts_t;

/** PCAP packet header */
typedef struct te_pcap_pkthdr {
    te_ts_t  ts;     /**< time stamp */
    uint32_t caplen; /**< length of portion present */
    uint32_t len;    /**< length this packet (off wire) */
} te_pcap_pkthdr;

/**
 * Safe copy of the time stamp
 * 
 * @param dest  32-bits struct of the time stamp
 * @param src   32/64-bits struct of the timestamp
 */
#define SNIFFER_TS_CPY(_dest, _src)\
{\
    (_dest).tv_sec = (uint32_t)(_src).tv_sec;\
    (_dest).tv_usec = (uint32_t)(_src).tv_usec;\
}

#endif /* ndef __TE_SNIFFERS_H__ */
