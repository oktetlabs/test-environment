/** @file
 * @brief Test API for TAD. Common functions for IP CSAP.
 *
 * Implementation of common functions for IPv4/IPv6 CSAP.
 *
 * Copyright (C) 2019 OKTET Labs. All rights reserved.
 *
 * @author Dmitry Izbitsky <Dmitry.Izbitsky@oktetlabs.ru>
 */

#include "te_config.h"

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#include "logger_api.h"
#include "tapi_ndn.h"
#include "ndn_ipstack.h"
#include "tapi_ip4.h"
#include "tapi_ip6.h"
#include "tapi_tcp.h"
#include "tapi_udp.h"
#include "tapi_ip_common.h"
#include "tapi_test.h"

/* See description in tapi_ip_common.h */
te_errno
tapi_ip_eth_csap_create(const char *ta_name, int sid,
                        const char *eth_dev, unsigned int receive_mode,
                        const uint8_t *loc_mac_addr,
                        const uint8_t *rem_mac_addr,
                        int af,
                        const void *loc_ip_addr,
                        const void *rem_ip_addr,
                        int ip_proto,
                        csap_handle_t *ip_csap)
{
    switch (af)
    {
        case AF_INET:
        {
            in_addr_t loc_ip4_addr = htonl(INADDR_ANY);
            in_addr_t rem_ip4_addr = htonl(INADDR_ANY);

            if (loc_ip_addr != NULL)
                loc_ip4_addr = *(in_addr_t *)loc_ip_addr;
            if (rem_ip_addr != NULL)
                rem_ip4_addr = *(in_addr_t *)rem_ip_addr;

            return tapi_ip4_eth_csap_create(
                        ta_name, sid, eth_dev, receive_mode,
                        loc_mac_addr, rem_mac_addr,
                        loc_ip4_addr, rem_ip4_addr,
                        ip_proto, ip_csap);
        }

        case AF_INET6:
            return tapi_ip6_eth_csap_create(
                        ta_name, sid, eth_dev, receive_mode,
                        loc_mac_addr, rem_mac_addr,
                        (const uint8_t *)loc_ip_addr,
                        (const uint8_t *)rem_ip_addr,
                        ip_proto, ip_csap);
    }

    ERROR("%s(): address family %d is not supported",
          __FUNCTION__, af);
    return TE_RC(TE_TAPI, TE_EINVAL);
}

/* See description in tapi_ip_common.h */
te_errno
tapi_tcp_udp_ip_eth_csap_create(
                        const char *ta_name, int sid,
                        const char *eth_dev, unsigned int receive_mode,
                        const uint8_t *loc_mac_addr,
                        const uint8_t *rem_mac_addr,
                        int af,
                        int ip_proto,
                        const void *loc_ip_addr,
                        const void *rem_ip_addr,
                        int loc_port,
                        int rem_port,
                        csap_handle_t *csap)
{
    switch (ip_proto)
    {
        case IPPROTO_TCP:
            return tapi_tcp_ip_eth_csap_create(
                            ta_name, sid, eth_dev, receive_mode,
                            loc_mac_addr, rem_mac_addr,
                            af, loc_ip_addr, rem_ip_addr,
                            loc_port, rem_port, csap);

        case IPPROTO_UDP:
            return tapi_udp_ip_eth_csap_create(
                            ta_name, sid, eth_dev, receive_mode,
                            loc_mac_addr, rem_mac_addr,
                            af, loc_ip_addr, rem_ip_addr,
                            loc_port, rem_port, csap);

        default:
            ERROR("%s(): not supported protocol %d",
                  __FUNCTION__, ip_proto);
            return TE_RC(TE_TAPI, TE_EINVAL);
    }
}

/* See the description in tapi_ip_common.h */
te_errno
tapi_ip_pdu_tmpl_fragments(asn_value **tmpl, asn_value **pdu,
                           te_bool ipv4,
                           tapi_ip_frag_spec *fragments,
                           unsigned int num_frags)
{
    asn_value         *tmp_pdu;
    te_errno           rc;
    unsigned int       i;
    tapi_ip_frag_spec *frag;
    asn_value         *frag_seq = NULL;

    if (tmpl != NULL)
    {
        CHECK_RC(tapi_tad_tmpl_ptrn_add_layer(tmpl,
                                              FALSE /* template */,
                                              (ipv4 ?
                                                  ndn_ip4_header :
                                                  ndn_ip6_header),
                                              (ipv4 ?
                                                  "#ip4" : "#ip6"),
                                              &tmp_pdu));
    }
    else if (pdu == NULL)
    {
        ERROR("%s(): Neither template nor PDU location is specified",
              __FUNCTION__);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }
    else if (*pdu == NULL)
    {
        ERROR("%s(): PDU location has to have some PDU when parent "
              "template is not specified", __FUNCTION__);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }
    else
    {
        tmp_pdu = *pdu;
    }

    if (num_frags == 0)
    {
        /* Nothing to do */
    }
    else if (fragments == NULL)
    {
        ERROR("%s(): Fragments specification pointer is NULL, but "
              "number of fragments is positive", __FUNCTION__);
        return TE_RC(TE_TAPI, TE_EWRONGPTR);
    }
    else
    {
        rc = tapi_tad_init_asn_value(&frag_seq,
                                     (ipv4 ?
                                          ndn_ip4_frag_seq :
                                          ndn_ip6_frag_seq));
        if (rc != 0)
            return rc;

        rc = asn_put_child_value(tmp_pdu, frag_seq,
                                 PRIVATE,
                                 (ipv4 ?
                                    NDN_TAG_IP4_FRAGMENTS :
                                    NDN_TAG_IP6_FRAGMENTS));
        if (rc != 0)
        {
            ERROR("%s(): Failed to put 'fragment-spec' in IP PDU: %r",
                  __FUNCTION__, rc);
            asn_free_value(frag_seq);
            return rc;
        }

        for (i = 0, frag = fragments; i < num_frags; i++, frag++)
        {
#define CHECK_ASN_RC(_expr) \
    do {                                          \
        rc = _expr;                               \
        if (rc != 0)                              \
        {                                         \
            ERROR("%s: %s failed with %r",        \
                  __FUNCTION__, #_expr, rc);      \
            return rc;                            \
        }                                         \
    } while (0)

            asn_value *frag_val = asn_init_value(ipv4 ?
                                                    ndn_ip4_frag_spec :
                                                    ndn_ip6_frag_spec);
            if (frag_val == NULL)
            {
                ERROR("%s(): Failed to allocate fragment specification: %r",
                      __FUNCTION__, rc);
            }

            CHECK_ASN_RC(asn_write_int32(frag_val, frag->hdr_offset,
                                         "hdr-offset"));
            CHECK_ASN_RC(asn_write_int32(frag_val, frag->real_offset,
                                         "real-offset"));
            CHECK_ASN_RC(asn_write_int32(frag_val, frag->hdr_length,
                                         "hdr-length"));
            CHECK_ASN_RC(asn_write_int32(frag_val, frag->real_length,
                                         "real-length"));
            CHECK_ASN_RC(asn_write_bool(frag_val,
                                        frag->more_frags, "more-frags"));
            if (ipv4)
            {
                CHECK_ASN_RC(asn_write_bool(frag_val, frag->dont_frag,
                                            "dont-frag"));
            }

            if (frag->id >= 0)
            {
                CHECK_ASN_RC(asn_write_uint32(frag_val,
                                              (uint32_t)(frag->id), "id"));
            }

            CHECK_ASN_RC(asn_insert_indexed(frag_seq, frag_val, i, ""));
        }
#undef CHECK_ASN_RC
    }

    if (pdu != NULL)
        *pdu = tmp_pdu;

    return 0;
}

/* See the description in tapi_ip_common.h */
void
tapi_ip_frag_specs_init(tapi_ip_frag_spec *frags,
                        unsigned int num)
{
    unsigned int i;

    memset(frags, 0, sizeof(*frags) * num);
    for (i = 0; i < num; i++)
        frags[i].id = -1;
}
