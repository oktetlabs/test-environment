/** @file
 * @brief Test Environment: 
 *
 * Traffic Application Domain
 * ICMP messages generating routines
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
 * Author: Konstantin Abramenko <Konstantin.Abramenko@oktetlabs.ru>
 *
 * @(#) $Id$
 */

#include <string.h>
#include <stdlib.h>

#define TE_LGR_USER "TAD ICMP" 


#include "tad_ipstack_impl.h"

/* for ETH_ALEN */
#include "ndn_eth.h"


/**
 * Function for make ICMP error by IP packet, catched by '*.ip4.eth'
 * raw CSAP.
 * Prototype made according with 'tad_processing_pkt_method' function type.
 * This method uses write_cb callback of passed 'eth' CSAP for send reply.
 * User parameter should contain integer numbers, separated by colon: 
 * "<type>:<code>[:<unused>[:<rate>]]". 
 * <unused> contains number to be placed in the 'unused' 32-bit field of
 * ICMP error (in host order). Default value is zero.
 * <rate> contains number of original packets per one ICMP error. 
 * Default value is 1.
 *
 * @param csap_descr  CSAP descriptor structure.
 * @param usr_param   String passed by user.
 * @param orig_pkt    Packet binary data, as it was caught from net.
 * @param pkt_len     Length of pkt data.
 *
 * @return zero on success or error code.
 */
int 
tad_icmp_error(csap_p csap_descr, const char *usr_param, 
               const uint8_t *orig_pkt, size_t pkt_len)
{
    uint8_t type, 
            code;
    int     rc = 0;
    char   *endptr; 
    size_t  msg_len;

    uint8_t *msg, *p;
    uint32_t unused = 0;
    int      rate = 1;

    if (csap_descr == NULL || usr_param == NULL ||
        orig_pkt == NULL || pkt_len == 0)
        return TE_EWRONGPTR;


    type = strtol(usr_param, &endptr, 10);
    if ((endptr == NULL) || (*endptr != ':'))
    {
        ERROR("%s(): wrong usr_param, should be <type>:<code>",
              __FUNCTION__);
        return EINVAL;
    }
    endptr++;
    code = strtol(endptr, &endptr, 10);

    if (endptr != NULL)
    {
        if (*endptr != ':')
        {
            ERROR("%s(): wrong usr_param, shouls be colon.", __FUNCTION__);
            return EINVAL;
        }
        endptr++;
        unused = strtol(endptr, &endptr, 10);
        if (endptr != NULL)
        {
            if (*endptr != ':')
            {
                ERROR("%s(): wrong usr_param, shouls be colon.",
                      __FUNCTION__);
                return EINVAL;
            }
            endptr++;
            rate = atoi(endptr);
            if (rate == 0)
            {
                ERROR("%s(): wrong rate in usr_param, shouls be non-zero",
                      __FUNCTION__);
                return EINVAL;
            }
        }
    }

    if ((rand() % rate) != 0)
        return 0;

    if (csap_descr->prepare_send_cb != NULL && 
        (rc = csap_descr->prepare_send_cb(csap_descr)) != 0)
    {
        ERROR("%s(): prepare for recv failed %x", __FUNCTION__, rc);
        return rc;
    }

#define ICMP_PLD_SIZE 28
    msg_len = 14 /* eth */ + 20 /* IP */ + 8 /* ICMP */
            + ICMP_PLD_SIZE /* IP header + 8 bytes of orig pkt */;

    p = msg = calloc(1, msg_len); 

    /* Ethernet header */
    memcpy(p, orig_pkt + ETH_ALEN, ETH_ALEN); 
    memcpy(p + ETH_ALEN, orig_pkt, ETH_ALEN); 
    memcpy(p + 2 * ETH_ALEN, orig_pkt + 2 * ETH_ALEN, 2); 
    p += 14; orig_pkt += 14;

    /* IP header, now leave orig_pkt unchanged */
    memcpy(p, orig_pkt, 2); /* vers, hlen, tos */
    p += 2;
    *(uint16_t *)p = htons(msg_len - 14); /* ip len */
    p += 2;
    *(uint16_t *)p = (uint16_t)rand(); /* ip ident */
    p += 2;
    *(uint16_t *)p = 0; /* flags & offset */
    p += 2;
    *p++ = 64;
    *p++ = IPPROTO_ICMP;
    p += 2; /* leave place for header checksum */
    memcpy(p, orig_pkt + 16, 4);
    p += 4;
    memcpy(p, orig_pkt + 12, 4);
    p += 4;

    /* set header checksum */
    *(uint16_t *)(msg + 14 + 10) = ~calculate_checksum(msg + 14, 20);

    /* ICMP header */

    *p++ = type;
    *p++ = code;
    p += 2; /* leave place for ICMP checksum */
    *(uint32_t *)p = htonl(unused);
    p += 4; 

    memcpy(p, orig_pkt, ICMP_PLD_SIZE);

    /* set ICMP checksum */
    *(uint16_t *)(msg + 14 + 20 + 2) = 
        ~calculate_checksum(msg + 14 + 20, ICMP_PLD_SIZE + 8);

    rc = csap_descr->write_cb(csap_descr, msg, msg_len);
    INFO("%s(): sent %d bytes", __FUNCTION__, rc);
    if (rc < 0)
    {
        ERROR("%s() write error", __FUNCTION__);
        return csap_descr->last_errno;
    }

    return 0;
}
