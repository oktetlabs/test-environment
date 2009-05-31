/** @file
 * @brief Test API for TAD IGMPv2 CSAP
 *
 * Implementation of Test API
 * 
 * Copyright (C) 2006 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/ or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 * @author Alexander Kukuta <Alexander.Kukuta@oktetlabs.ru>
 *
 * $Id: $
 */

#define TE_LGR_USER "TAPI IGMPv2"

#include "te_config.h"

#include "te_errno.h"
#include "logger_api.h"
#include "asn_usr.h"
#include "tapi_ndn.h"
#include "ndn_ipstack.h"
#include "ndn_igmp.h"
#include "tapi_igmp.h"

#include "tapi_test.h"


/* See the description in tapi_igmp2.h */
te_errno
tapi_igmp2_add_csap_layer(asn_value **csap_spec)
{
    asn_value  *layer = NULL;

    CHECK_RC(tapi_tad_csap_add_layer(csap_spec, ndn_igmp_csap, "#igmp",
                                     &layer));

    CHECK_RC(asn_write_int32(layer, TAPI_IGMP_VERSION_2,
                             "version.#plain"));

    return 0;
}


/* See the description in tapi_igmp2.h */
te_errno
tapi_igmp2_add_pdu(asn_value          **tmpl_or_ptrn,
                   asn_value          **pdu,
                   te_bool              is_pattern,
                   tapi_igmp_msg_type   type,
                   int                  max_resp_time,
                   in_addr_t            group_addr)
{
    asn_value  *tmp_pdu;

    if (type > 0xff || max_resp_time > 0xff)
        return TE_RC(TE_TAPI, TE_EINVAL);

    CHECK_RC(tapi_tad_tmpl_ptrn_add_layer(tmpl_or_ptrn, is_pattern,
                                          ndn_igmp_message, "#igmp",
                                          &tmp_pdu));

    if (type > 0)
        CHECK_RC(asn_write_int32(tmp_pdu, type, "type.#plain"));
    if (max_resp_time >= 0)
        CHECK_RC(asn_write_int32(tmp_pdu, max_resp_time,
                                 "max-resp-time.#plain"));

    ERROR("Fill Group Address: 0x%08x", (uint32_t)group_addr);
    if (group_addr != htonl(INADDR_ANY))
        CHECK_RC(asn_write_value_field(tmp_pdu,
                                       &group_addr, sizeof(group_addr),
                                       "group-address.#plain"));

    if (pdu != NULL)
        *pdu = tmp_pdu;

    return 0;
}

/* See the description in tapi_igmp2.h */
te_errno
tapi_igmp2_ip4_eth_csap_create(const char    *ta_name,
                               int            sid,
                               const char    *eth_dev,
                               unsigned int   receive_mode,
                               const uint8_t *eth_src,
                               in_addr_t      src_addr,
                               csap_handle_t *igmp_csap)
{
    te_errno    rc;
    asn_value  *csap_spec = NULL;

    do {
        if ((rc = tapi_igmp2_add_csap_layer(&csap_spec)) != 0)
        {
            WARN("%s(): add IGMPv2 csap layer failed %r", __FUNCTION__, rc);
            break;
        }

        if ((rc = tapi_ip4_add_csap_layer(&csap_spec,
                                          src_addr,
                                          htonl(INADDR_ANY),
                                          IPPROTO_IGMP,
                                          -1 /* default ttl */,
                                          -1 /* default tos */)) != 0)
        {
            WARN("%s(): add IP4 csap layer failed %r", __FUNCTION__, rc);
            break;
        }

        if ((rc = tapi_eth_add_csap_layer(&csap_spec, eth_dev,
                                          receive_mode,
                                          NULL,
                                          eth_src,
                                          NULL, TE_BOOL3_ANY,
                                          TE_BOOL3_ANY)) != 0)
        {
            WARN("%s(): add ETH csap layer failed %r", __FUNCTION__, rc);
            break;
        }


        rc = tapi_tad_csap_create(ta_name, sid, "igmp.ip4.eth",
                                  csap_spec, igmp_csap);
    } while (0);

    asn_free_value(csap_spec);

    return TE_RC(TE_TAPI, rc);
}

/* See header */
void
tapi_ip4_to_mac(in_addr_t ip4_addr, uint8_t *eth_addr)
{
    uint8_t *ip_addr_p = &ip4_addr;

    /* Map IP address into MAC one */
    eth_addr[0] = 0x01;
    eth_addr[1] = 0x00;
    eth_addr[2] = 0x5e;
    eth_addr[3] = ip_addr_p[1] & 0x7f;
    eth_addr[4] = ip_addr_p[2];
    eth_addr[5] = ip_addr_p[3];
}

/* See the description in tapi_igmp2.h */
te_errno
tapi_igmp2_ip4_eth_add_pdu(asn_value             **tmpl_or_ptrn,
                           asn_value             **pdu,
                           te_bool                 is_pattern,
                           tapi_igmp_msg_type      type,
                           tapi_igmp_query_type    query_type,
                           int                     max_resp_time,
                           in_addr_t               group_addr,
                           in_addr_t               src_addr,
                           uint8_t                *eth_src)
{
    te_errno       rc     = 0;
    const uint16_t ip_eth = ETHERTYPE_IP;
    in_addr_t      dst_addr = htonl(INADDR_ANY);
    uint8_t        eth_dst[ETHER_ADDR_LEN];

    /* Add IGMPv2 layer message to PDU template/pattern */
    CHECK_RC(tapi_igmp2_add_pdu(tmpl_or_ptrn, pdu, is_pattern,
                                type, max_resp_time, group_addr));

    switch (type)
    {
        case TAPI_IGMP_TYPE_QUERY:
            dst_addr = (query_type == TAPI_IGMP_QUERY_TYPE_GROUP) ?
                group_addr : htonl(TAPI_MCAST_ADDR_ALL_HOSTS);
            break;

        case TAPI_IGMP_TYPE_REPORT_V1:
        case TAPI_IGMP_TYPE_REPORT:
            dst_addr = group_addr;
            break;

        case TAPI_IGMP_TYPE_LEAVE:
            dst_addr = htonl(TAPI_MCAST_ADDR_ALL_ROUTERS);
            break;

        default:
            dst_addr = htonl(INADDR_ANY);
            break;
    }

    /* Add IPv4 layer header to PDU template/pattern */
    CHECK_RC(tapi_ip4_add_pdu(tmpl_or_ptrn, pdu, is_pattern,
                              src_addr, dst_addr,
                              IPPROTO_IGMP,
                              TAPI_IGMP2_IP4_TTL_DEFAULT,
                              -1 /* default ToS */ ));

    /* Calculate MAC multicast address also */
    if (dst_addr != htonl(INADDR_ANY))
        tapi_ip4_to_mac(dst_addr, eth_dst);

    /* Add Ethernet layer header to PDU template/pattern */
    CHECK_RC(tapi_eth_add_pdu(tmpl_or_ptrn, pdu, is_pattern,
                              (dst_addr == htonl(INADDR_ANY)) ? NULL : eth_dst,
                              eth_src,
                              &ip_eth,
                              TE_BOOL3_ANY,
                              TE_BOOL3_ANY));

    return rc;
}
