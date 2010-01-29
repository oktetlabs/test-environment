/** @file
 * @brief Test API for TAD. IP stack CSAP
 *
 * Implementation of Test API
 * 
 * Copyright (C) 2006 Test Environment authors (see file AUTHORS
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
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER "TAPI ICMPv4"

#include <netinet/ip_icmp.h>

#include "te_config.h"

#include "te_errno.h"
#include "logger_api.h"
#include "asn_usr.h"
#include "tapi_ndn.h"
#include "ndn_ipstack.h"
#include "tapi_icmp4.h"

#include "tapi_test.h"


/* See the description in tapi_icmp4.h */
te_errno
tapi_icmp4_add_csap_layer(asn_value **csap_spec)
{
    return tapi_tad_csap_add_layer(csap_spec, ndn_icmp4_csap, "#icmp4",
                                   NULL);
}

te_errno
tapi_tcp_ip4_icmp_ip4_eth_csap_create(const char    *ta_name,
                                      int            sid,
                                      const char    *eth_dev,
                                      unsigned int   receive_mode,
                                      const uint8_t *eth_src,
                                      const uint8_t *eth_dst, 
                                      in_addr_t      src_addr,
                                      in_addr_t      dst_addr,
                                      in_addr_t      msg_src_addr,
                                      in_addr_t      msg_dst_addr,
                                      int            msg_src_port,
                                      int            msg_dst_port,
                                      csap_handle_t *tcp_csap)
{
    te_errno    rc;
    asn_value  *csap_spec = NULL;

    /*
     * Incapsulated IP packet header has rem_addr as local address and
     * loc_addr as remote address
     */
    rc = tapi_tcp_add_csap_layer(&csap_spec, msg_src_port, msg_dst_port);
    if (rc != 0)
    {
        asn_free_value(csap_spec);
        WARN("%s(): add TCP csap layer failed %r", __FUNCTION__, rc);
        return rc;
    }

    /*
     * Incapsulated IP packet header has rem_addr as local address and
     * loc_addr as remote address
     */
    rc = tapi_ip4_add_csap_layer(&csap_spec, msg_src_addr, msg_dst_addr,
                                 -1 /* default proto */,
                                 -1 /* default ttl */,
                                 -1 /* default tos */);
    if (rc != 0)
    {
        asn_free_value(csap_spec);
        WARN("%s(): add IP4 csap layer failed %r", __FUNCTION__, rc);
        return rc;
    }

    rc = tapi_icmp4_add_csap_layer(&csap_spec);
    if (rc != 0)
    {
        asn_free_value(csap_spec);
        WARN("%s(): add ICMP csap layer failed %r", __FUNCTION__, rc);
        return rc;
    }

    rc = tapi_ip4_add_csap_layer(&csap_spec, src_addr, dst_addr,
                                 -1 /* default proto */,
                                 -1 /* default ttl */,
                                 -1 /* default tos */);
    if (rc != 0)
    {
        asn_free_value(csap_spec);
        WARN("%s(): add IP4 csap layer failed %r", __FUNCTION__, rc);
        return rc;
    }

    rc = tapi_eth_add_csap_layer(&csap_spec, eth_dev, receive_mode,
                                 eth_dst, eth_src,
                                 NULL /* automatic length/type */,
                                 TE_BOOL3_ANY /* untagged/tagged */,
                                 TE_BOOL3_ANY /* Ethernet2/LLC+SNAP */);
    if (rc != 0)
    {
        asn_free_value(csap_spec);
        WARN("%s(): add ETH csap layer failed %r", __FUNCTION__, rc);
        return rc;
    }


    rc = tapi_tad_csap_create(ta_name, sid, "tcp.ip4.icmp4.ip4.eth",
                              csap_spec, tcp_csap);

    asn_free_value(csap_spec);

    return TE_RC(TE_TAPI, rc);
}

te_errno
tapi_udp_ip4_icmp_ip4_eth_csap_create(const char    *ta_name,
                                      int            sid,
                                      const char    *eth_dev,
                                      unsigned int   receive_mode,
                                      const uint8_t *eth_src,
                                      const uint8_t *eth_dst, 
                                      in_addr_t      src_addr,
                                      in_addr_t      dst_addr,
                                      in_addr_t      msg_src_addr,
                                      in_addr_t      msg_dst_addr,
                                      int            msg_src_port,
                                      int            msg_dst_port,
                                      csap_handle_t *udp_csap)
{
    te_errno    rc;
    asn_value  *csap_spec = NULL;

    /*
     * Incapsulated IP packet header has rem_addr as local address and
     * loc_addr as remote address
     */
    rc = tapi_udp_add_csap_layer(&csap_spec, msg_src_port, msg_dst_port);
    if (rc != 0)
    {
        asn_free_value(csap_spec);
        WARN("%s(): add UDP csap layer failed %r", __FUNCTION__, rc);
        return rc;
    }

    /*
     * Incapsulated IP packet header has rem_addr as local address and
     * loc_addr as remote address
     */
    rc = tapi_ip4_add_csap_layer(&csap_spec, msg_src_addr, msg_dst_addr,
                                 -1 /* default proto */,
                                 -1 /* default ttl */,
                                 -1 /* default tos */);
    if (rc != 0)
    {
        asn_free_value(csap_spec);
        WARN("%s(): add IP4 csap layer failed %r", __FUNCTION__, rc);
        return rc;
    }

    rc = tapi_icmp4_add_csap_layer(&csap_spec);
    if (rc != 0)
    {
        asn_free_value(csap_spec);
        WARN("%s(): add ICMP csap layer failed %r", __FUNCTION__, rc);
        return rc;
    }

    rc = tapi_ip4_add_csap_layer(&csap_spec, src_addr, dst_addr,
                                 -1 /* default proto */,
                                 -1 /* default ttl */,
                                 -1 /* default tos */);
    if (rc != 0)
    {
        asn_free_value(csap_spec);
        WARN("%s(): add IP4 csap layer failed %r", __FUNCTION__, rc);
        return rc;
    }

    rc = tapi_eth_add_csap_layer(&csap_spec, eth_dev, receive_mode,
                                 eth_dst, eth_src,
                                 NULL /* automatic length/type */,
                                 TE_BOOL3_ANY /* untagged/tagged */,
                                 TE_BOOL3_ANY /* Ethernet2/LLC+SNAP */);
    if (rc != 0)
    {
        asn_free_value(csap_spec);
        WARN("%s(): add ETH csap layer failed %r", __FUNCTION__, rc);
        return rc;
    }


    rc = tapi_tad_csap_create(ta_name, sid, "udp.ip4.icmp4.ip4.eth",
                              csap_spec, udp_csap);

    asn_free_value(csap_spec);

    return TE_RC(TE_TAPI, rc);
}

/* See the description in tapi_icmp4.h */
te_errno
tapi_icmp4_add_pdu(asn_value **tmpl_or_ptrn, asn_value **pdu,
                   te_bool is_pattern,
                   int type, int code)
{
    asn_value  *tmp_pdu;

    if (type > 0xff || code > 0xff)
        return TE_RC(TE_TAPI, TE_EINVAL);

    CHECK_RC(tapi_tad_tmpl_ptrn_add_layer(tmpl_or_ptrn, is_pattern,
                                          ndn_icmp4_message, "#icmp4",
                                          &tmp_pdu));

    if (type >= 0)
        CHECK_RC(asn_write_int32(tmp_pdu, type, "type.#plain"));
    if (code >= 0)
        CHECK_RC(asn_write_int32(tmp_pdu, code, "code.#plain"));

    if (type == ICMP_PARAMETERPROB)
    {
        int ptr = 0;
        CHECK_RC(asn_write_int32(tmp_pdu, ptr, "ptr.#plain"));
    }

    if (pdu != NULL)
        *pdu = tmp_pdu;

    return 0;
}

/**
 * Create 'icmp.ip4.eth' CSAP on the specified Agent
 *
 * @param ta_name       Test Agent name
 * @param sid           RCF SID
 * @param eth_dev       Name of Ethernet interface
 * @param receive_mode  Bitmask with receive mode, see 'enum
 *                      tad_eth_recv_mode' in tad_common.h.
 *                      Use TAD_ETH_RECV_DEF by default.
 * @param eth_src       Local MAC address (or NULL)
 * @param eth_dst       Remote MAC address (or NULL)
 * @param src_addr      Local IP address in network byte order (or NULL)
 * @param dst_addr      Remote IP address in network byte order (or NULL)
 * @param icmp_csap     Location for the CSAP handle (OUT)
 *
 * @return Zero on success or error code
 */
te_errno
tapi_icmp_ip4_eth_csap_create(const char    *ta_name,
                              int            sid,
                              const char    *eth_dev,
                              unsigned int   receive_mode,
                              const uint8_t *eth_src,
                              const uint8_t *eth_dst,
                              in_addr_t      src_addr,
                              in_addr_t      dst_addr,
                              csap_handle_t *icmp_csap)
{
    te_errno    rc;
    asn_value  *csap_spec = NULL;

    do {
        if ((rc = tapi_tad_csap_add_layer(&csap_spec,
                                          ndn_icmp4_csap,
                                          "#icmp4", NULL)) != 0)
        {
            WARN("%s(): add ICMP csap layer failed %r", __FUNCTION__, rc);
            break;
        }

        if ((rc = tapi_ip4_add_csap_layer(&csap_spec,
                                          src_addr, dst_addr,
                                          -1 /* default proto */,
                                          -1 /* default ttl */,
                                          -1 /* default tos */)) != 0)
        {
            WARN("%s(): add IP4 csap layer failed %r", __FUNCTION__, rc);
            break;
        }

        if ((rc = tapi_eth_add_csap_layer(&csap_spec, eth_dev,
                                          receive_mode, eth_dst, eth_src,
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

/**
 * Create 'icmp.ip4' CSAP on the specified Agent
 *
 * @param ta_name       Test Agent name
 * @param sid           RCF SID
 * @param ifname        Name of interface
 * @param src_addr      Local IP address in network byte order (or NULL)
 * @param dst_addr      Remote IP address in network byte order (or NULL)
 * @param icmp_csap     Location for the CSAP handle (OUT)
 *
 * @return Zero on success or error code
 */
te_errno
tapi_icmp_ip4_csap_create(const char    *ta_name,
                          int            sid,
                          const char    *ifname,
                          in_addr_t      src_addr,
                          in_addr_t      dst_addr,
                          csap_handle_t *icmp_csap)
{
    te_errno    rc;
    asn_value  *csap_spec = NULL;

    do {
        if ((rc = tapi_tad_csap_add_layer(&csap_spec,
                                          ndn_icmp4_csap,
                                          "#icmp4", NULL)) != 0)
        {
            WARN("%s(): add ICMP csap layer failed %r", __FUNCTION__, rc);
            break;
        }

        if ((rc = tapi_ip4_add_csap_layer(&csap_spec,
                                          src_addr, dst_addr,
                                          -1 /* default proto */,
                                          -1 /* default ttl */,
                                          -1 /* default tos */)) != 0)
        {
            WARN("%s(): add IP4 csap layer failed %r", __FUNCTION__, rc);
            break;
        }

        rc = asn_write_string(csap_spec, ifname,
                              "layers.1.#ip4.ifname.#plain");
        if (rc != 0)
        {
            WARN("%s(): write IP4 layer value 'ifname' failed %r",
                 __FUNCTION__, rc);
            break;
        }

        rc = tapi_tad_csap_create(ta_name, sid, "icmp4.ip4",
                                  csap_spec, icmp_csap);
    } while (0);

    asn_free_value(csap_spec);

    return TE_RC(TE_TAPI, rc);
}
