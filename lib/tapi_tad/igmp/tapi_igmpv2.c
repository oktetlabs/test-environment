/** @file
 * @brief Test API for TAD. IP stack CSAP
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

#include <netinet/ip_icmp.h>

#include "te_config.h"

#include "te_errno.h"
#include "logger_api.h"
#include "asn_usr.h"
#include "tapi_ndn.h"
#include "ndn_ipstack.h"
#include "ndn_igmpv2.h"
#include "tapi_igmpv2.h"

#include "tapi_test.h"


/* See the description in tapi_igmpv2.h */
te_errno
tapi_igmpv2_add_csap_layer(asn_value **csap_spec)
{
    return tapi_tad_csap_add_layer(csap_spec, ndn_igmpv2_csap, "#igmpv2",
                                   NULL);
}


/* See the description in tapi_igmpv2.h */
te_errno
tapi_igmpv2_add_pdu(asn_value **tmpl_or_ptrn, asn_value **pdu,
                    te_bool is_pattern,
                    int type, int max_resp_time, in_addr_t group_addr)
{
    asn_value  *tmp_pdu;

    if (type > 0xff || max_resp_time > 0xff)
        return TE_RC(TE_TAPI, TE_EINVAL);

    CHECK_RC(tapi_tad_tmpl_ptrn_add_layer(tmpl_or_ptrn, is_pattern,
                                          ndn_igmpv2_message, "#igmpv2",
                                          &tmp_pdu));

    if (type >= 0)
        CHECK_RC(asn_write_int32(tmp_pdu, type, "type.#plain"));
    if (max_resp_time >= 0)
        CHECK_RC(asn_write_int32(tmp_pdu, max_resp_time,
                                 "max-resp-time.#plain"));

    if (group_addr != htonl(INADDR_ANY))
        CHECK_RC(asn_write_value_field(tmp_pdu,
                                       &group_addr, sizeof(group_addr),
                                       "group-addr.#plain"));

    if (pdu != NULL)
        *pdu = tmp_pdu;

    return 0;
}

/* See the description in tapi_igmpv2.h */
te_errno
tapi_igmpv2_ip4_eth_csap_create(const char    *ta_name,
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
        if ((rc = tapi_tad_csap_add_layer(&csap_spec,
                                          ndn_igmpv2_csap,
                                          "#igmpv2", NULL)) != 0)
        {
            WARN("%s(): add IGMPv2 csap layer failed %r", __FUNCTION__, rc);
            break;
        }

        if ((rc = tapi_ip4_add_csap_layer(&csap_spec,
                                          src_addr,
                                          htonl(INADDR_ANY),
                                          -1 /* default proto */,
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


        rc = tapi_tad_csap_create(ta_name, sid, "icmp4.ip4.eth",
                                  csap_spec, icmp_csap);
    } while (0);

    asn_free_value(csap_spec);

    return TE_RC(TE_TAPI, rc);
}

/* See the description in tapi_igmpv2.h */
te_errno
tapi_igmpv2_ip4_eth_add_pdu(asn_value **tmpl_or_ptrn,
                            asn_value **pdu,
                            te_bool     is_pattern,
                            int         type,
                            int         max_resp_time,
                            in_addr_t   group_addr,
                            in_addr_t   src_addr,
                            uint8_t    *eth_src)
{
    te_errno        rc = 0;
    const uint16_t  ip_eth = ETHERTYPE_IP;

    CHECK_RC(tapi_igmpv2_add_pdu(tmpl_or_ptrn, pdu, is_pattern,
                                 type, max_resp_time, group_addr));

    CHECK_RC(tapi_ip4_add_pdu(tmpl_or_ptrn, pdu, is_pattern,
                              src_addr, htonl(INADDR_ANY),
                              TE_PROTO_IGMPV2,
                              TAPI_IGMPV2_IP4_TTL_DEFAULT,
                              -1 /* default ToS */ ));
    CHECK_RC(tapi_eth_add_pdu(tmpl_or_ptrn, pdu, is_pattern,
                              NULL, eth_src,
                              &ip_eth,
                              TE_BOOL3_ANY,
                              TE_BOOL3_ANY));

    return rc;
}
