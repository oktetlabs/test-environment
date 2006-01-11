/** @file
 * @brief TAD IP Stack
 *
 * Traffic Application Domain Command Handler.
 * ICMP messages generating routines.
 *
 * Copyright (C) 2005-2006 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
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
 * @author Konstantin Abramenko <Konstantin.Abramenko@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER     "TAD ICMP" 

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include <stdlib.h>
#if HAVE_NET_ETHERNET_H
#include <net/ethernet.h>
#endif
#if HAVE_NET_IF_ETHER_H
#include <net/if_ether.h>
#endif

#include "tad_ipstack_impl.h"


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
 * @param csap  CSAP descriptor structure.
 * @param usr_param   String passed by user.
 * @param orig_pkt    Packet binary data, as it was caught from net.
 * @param pkt_len     Length of pkt data.
 *
 * @return zero on success or error code.
 */
int 
tad_icmp_error(csap_p csap, const char *usr_param, 
               const uint8_t *orig_pkt, size_t pkt_len)
{
    csap_spt_type_p rw_layer_cbs;

    tad_pkt    *pkt;

    uint8_t type, 
            code;
    int     rc = 0;
    char   *endptr; 
    size_t  msg_len;

    uint8_t *msg, *p;
    uint32_t unused = 0;
    int      rate = 1;

    if (csap == NULL || usr_param == NULL ||
        orig_pkt == NULL || pkt_len == 0)
        return TE_EWRONGPTR;


    type = strtol(usr_param, &endptr, 10);
    if ((endptr == NULL) || (*endptr != ':'))
    {
        ERROR("%s(): wrong usr_param, should be <type>:<code>",
              __FUNCTION__);
        return TE_EINVAL;
    }
    endptr++;
    code = strtol(endptr, &endptr, 10);

    if (endptr != NULL)
    {
        if (*endptr != ':')
        {
            ERROR("%s(): wrong usr_param, shouls be colon.", __FUNCTION__);
            return TE_EINVAL;
        }
        endptr++;
        unused = strtol(endptr, &endptr, 10);
        if (endptr != NULL)
        {
            if (*endptr != ':')
            {
                ERROR("%s(): wrong usr_param, shouls be colon.",
                      __FUNCTION__);
                return TE_EINVAL;
            }
            endptr++;
            rate = atoi(endptr);
            if (rate == 0)
            {
                ERROR("%s(): wrong rate in usr_param, shouls be non-zero",
                      __FUNCTION__);
                return TE_EINVAL;
            }
        }
    }

    if ((rand() % rate) != 0)
        return 0;

    rw_layer_cbs = csap_get_proto_support(csap, csap_get_rw_layer(csap));
    if (rw_layer_cbs->prepare_send_cb != NULL && 
        (rc = rw_layer_cbs->prepare_send_cb(csap)) != 0)
    {
        ERROR("%s(): prepare for recv failed %r", __FUNCTION__, rc);
        return rc;
    }

#define ICMP_PLD_SIZE 28
    msg_len = 14 /* eth */ + 20 /* IP */ + 8 /* ICMP */
            + ICMP_PLD_SIZE /* IP header + 8 bytes of orig pkt */;

    pkt = tad_pkt_alloc(1, msg_len);
    if (pkt == NULL)
    {
        ERROR("%s(): no memory!", __FUNCTION__);
        return TE_ENOMEM;
    }
    p = msg = tad_pkt_first_seg(pkt)->data_ptr;

    /* Ethernet header */
    memcpy(p, orig_pkt + ETHER_ADDR_LEN, ETHER_ADDR_LEN); 
    memcpy(p + ETHER_ADDR_LEN, orig_pkt, ETHER_ADDR_LEN); 
    memcpy(p + 2 * ETHER_ADDR_LEN, orig_pkt + 2 * ETHER_ADDR_LEN, 2); 
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

    rc = rw_layer_cbs->write_cb(csap, pkt);
    tad_pkt_free(pkt);

    if (rc != 0)
    {
        ERROR("%s() write error: %r", __FUNCTION__, rc);
    }

    return rc;
}
