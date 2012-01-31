/** @file
 * @brief Test API for TAD. IPv6 stack CSAP
 *
 * Implementation of Test API
 *
 * Copyright (C) 2012 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
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
 * @author Konstantin Petrov <Konstantin.Petrov@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER "TAPI ICMPv6"

#include <netinet/ip_icmp.h>

#include "te_config.h"

#include "te_errno.h"
#include "logger_api.h"
#include "asn_usr.h"
#include "tapi_ndn.h"
#include "ndn_ipstack.h"
#include "tapi_icmp6.h"

#include "tapi_test.h"

te_errno
tapi_icmp_ip6_eth_csap_create(const char    *ta_name,
                              int            sid,
                              const char    *eth_dev,
                              unsigned int   receive_mode,
                              const uint8_t *loc_hwaddr,
                              const uint8_t *rem_hwaddr,
                              const uint8_t *loc_addr,
                              const uint8_t *rem_addr,
                              csap_handle_t *icmp_csap)
{
    te_errno    rc;
    asn_value  *csap_spec = NULL;

    do {
        if ((rc = tapi_tad_csap_add_layer(&csap_spec,
                                          ndn_icmp4_csap,
                                          "#icmp6", NULL)) != 0)
        {
            WARN("%s(): add ICMP csap layer failed %r", __FUNCTION__, rc);
            break;
        }

        if ((rc = tapi_ip6_add_csap_layer(&csap_spec,
                                     loc_addr, rem_addr,
                                     IPPROTO_ICMPV6)) != 0)
        {
            WARN("%s(): add IP6 csap layer failed %r", __FUNCTION__, rc);
            break;
        }

        if ((rc = tapi_eth_add_csap_layer(&csap_spec, eth_dev, receive_mode,
                                          rem_hwaddr, loc_hwaddr,
                                          NULL, TE_BOOL3_ANY,
                                          TE_BOOL3_ANY)) != 0)
        {
            WARN("%s(): add ETH csap layer failed %r", __FUNCTION__, rc);
            break;
        }

        rc = tapi_tad_csap_create(ta_name, sid, "icmp6.ip6.eth",
                                  csap_spec, icmp_csap);
    } while (0);

    asn_free_value(csap_spec);

    return TE_RC(TE_TAPI, rc);
}

te_errno
tapi_icmp6_add_pdu(asn_value **tmpl_or_ptrn, asn_value **pdu,
                   te_bool is_pattern,
                   int type, int code,
                   icmp6_msg_body *body,
                   icmp6_msg_option *optlist)
{
    asn_value          *tmp_pdu;
    asn_value          *option_pdu;
    icmp6_msg_option   *option = optlist;
    int                 syms;
    int                 i = 0;
    uint8_t             optlen;
    char                name_buf[256];

    if (type > 0xff || code > 0xff)
        return TE_RC(TE_TAPI, TE_EINVAL);

    CHECK_RC(tapi_tad_tmpl_ptrn_add_layer(tmpl_or_ptrn, is_pattern,
                                          ndn_icmp6_message, "#icmp6",
                                          &tmp_pdu));

    if (type >= 0)
        CHECK_RC(asn_write_int32(tmp_pdu, type, "type.#plain"));
    if (code >= 0)
        CHECK_RC(asn_write_int32(tmp_pdu, code, "code.#plain"));

    if (body != NULL)
    {
        if (body->msg_type == ICMP6_ROUTER_SOL)
        {
            CHECK_RC(asn_write_int32(tmp_pdu,
                                     body->msg_body.rsol_reserved,
                                     "body.#router-sol.reserved.#plain"));
        }
        else if (body->msg_type == ICMP6_ROUTER_ADV)
        {
            CHECK_RC(asn_write_value_field(tmp_pdu,
                                &body->msg_body.radv.cur_hop_limit, 1,
                                "body.#router-adv.cur-hop-limit.#plain"));

            CHECK_RC(asn_write_value_field(tmp_pdu,
                                &body->msg_body.radv.flags, 1,
                                "body.#router-adv.flags.#plain"));

            CHECK_RC(asn_write_value_field(tmp_pdu,
                                &body->msg_body.radv.lifetime, 2,
                                "body.#router-adv.lifetime.#plain"));

            CHECK_RC(asn_write_int32(tmp_pdu,
                                body->msg_body.radv.reachable_time,
                                "body.#router-adv.reachable-time.#plain"));

            CHECK_RC(asn_write_int32(tmp_pdu,
                                body->msg_body.radv.retrans_timer,
                                "body.#router-adv.retrans-timer.#plain"));
        }
        else if (body->msg_type == ICMP6_NEIGHBOR_SOL)
        {
            CHECK_RC(asn_write_int32(tmp_pdu,
                                     body->msg_body.nsol.nsol_reserved,
                                     "body.#neighbor-sol.reserved.#plain"));

            CHECK_RC(asn_write_value_field(tmp_pdu,
                                body->msg_body.nsol.tgt_addr, 16,
                                "body.#neighbor-sol.target-addr.#plain"));
        }
        else if (body->msg_type == ICMP6_NEIGHBOR_ADV)
        {
            CHECK_RC(asn_write_int32(tmp_pdu,
                                     body->msg_body.nadv.flags,
                                     "body.#neighbor-adv.flags.#plain"));

            CHECK_RC(asn_write_value_field(tmp_pdu,
                                body->msg_body.nadv.tgt_addr, 16,
                                "body.#neighbor-adv.target-addr.#plain"));
        }
        else if (body->msg_type == ICMP6_ECHO_REQUEST ||
                 body->msg_type == ICMP6_ECHO_REPLY)
        {
            CHECK_RC(asn_write_int32(tmp_pdu,
                                     body->msg_body.echo.id,
                                     "body.#echo.id.#plain"));

            CHECK_RC(asn_write_int32(tmp_pdu,
                                     body->msg_body.echo.seq,
                                     "body.#echo.seq.#plain"));
        }
        else
            ERROR("%s(): ICMPv6 message type "
                  "is not supported", __FUNCTION__);
    }

    while (option != NULL) {
        if (option->opt_type == ICMP6_OPT_SOURCE_LL_ADDR ||
            option->opt_type == ICMP6_OPT_TARGET_LL_ADDR)
        {
            optlen = 1; /* One 8-octets block */
            sprintf(name_buf,
                    "{type plain:%d, length plain:%d}",
                    option->opt_type, optlen);

            CHECK_RC(asn_parse_value_text(name_buf,
                                      ndn_icmp6_opt,
                                      &option_pdu, &syms));

            CHECK_RC(asn_write_value_field(option_pdu,
                                option->opt_body.source_ll_addr,
                                ETHER_ADDR_LEN,
                                "body.#ll-addr.mac.#plain"));
        }
        else if (option->opt_type == ICMP6_OPT_PREFIX_INFO)
        {
            optlen = 4; /* Four 8-octets blocks */
            sprintf(name_buf,
                    "{type plain:%d, length plain:%d}",
                    option->opt_type, optlen);

            CHECK_RC(asn_parse_value_text(name_buf,
                                      ndn_icmp6_opt,
                                      &option_pdu, &syms));

            CHECK_RC(asn_write_value_field(option_pdu,
                        &option->opt_body.prefix_info.prefix_length, 1,
                        "body.#prefix.prefix-length.#plain"));

            CHECK_RC(asn_write_value_field(option_pdu,
                        &option->opt_body.prefix_info.flags, 1,
                        "body.#prefix.flags.#plain"));

            CHECK_RC(asn_write_int32(tmp_pdu,
                        option->opt_body.prefix_info.valid_lifetime,
                        "body.#prefix.valid-lifetime.#plain"));

            CHECK_RC(asn_write_int32(tmp_pdu,
                        option->opt_body.prefix_info.preferred_lifetime,
                        "body.#prefix.preferred-lifetime.#plain"));

            CHECK_RC(asn_write_value_field(option_pdu,
                        option->opt_body.prefix_info.prefix, 16,
                        "body.#prefix.prefix.#plain"));
        }
        else
            ERROR("%s(): ICMPv6 option type "
                  "is not supported", __FUNCTION__);

        CHECK_RC(asn_insert_indexed(tmp_pdu, option_pdu, i, "options"));

        i++;
        option = option->next;
    }

    if (pdu != NULL)
        *pdu = tmp_pdu;

    return 0;
}
