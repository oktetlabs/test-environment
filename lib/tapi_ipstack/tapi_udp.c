/** @file
 * @brief Test API for TAD. ipstack CSAP
 *
 * Implementation of Test API
 * 
 * Copyright (C) 2004 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
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
 * @author: Konstantin Abramenko <konst@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER "TAPI UDP" 

#include "te_config.h"

#include <stdio.h>
#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#endif
#if HAVE_STRINGS_H
#include <strings.h>
#endif
#if HAVE_ASSERT_H
#include <assert.h>
#endif

#include <netinet/in.h>
#include <netinet/ether.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include "rcf_api.h"
#include "conf_api.h"
#include "logger_api.h"

#include "tapi_udp.h"
#include "tapi_tad.h"
#include "ndn_ipstack.h"
#include "ndn_eth.h"


/**
 * data for udp callback 
 */
typedef struct {
    udp4_datagram  *dgram;
    void           *callback_data;
    udp4_callback   callback;
} udp4_cb_data_t;


/**
 * Read field from packet.
 *
 * @param _dir    direction of field: src or dst
 * @param _field  label of desired field: port or addr
 */
#define READ_PACKET_FIELD(_dir, _field) \
    do {                                                        \
        len = sizeof((*udp_dgram)-> _dir ## _ ##_field ); \
        if (rc == 0)                                            \
            rc = asn_read_value_field(pdu,                      \
                        &((*udp_dgram)-> _dir ##_## _field ), \
                        &len, # _dir "-" # _field);   \
    } while (0)


/**
 * Convert UDP packet ASN value to plain C structure
 *
 * @param pkt           ASN value of type DHCPv4 message or Generic-PDU with
 *                      choice "dhcp"
 * @param udp_dgram     converted structure (OUT)
 *
 * @return zero on success or error code
 *
 * @note Function allocates memory under dhcp_message data structure, which
 * should be freed with dhcpv4_message_destroy
 */
int
ndn_udp4_dgram_to_plain(asn_value_p pkt, udp4_datagram **udp_dgram)
{
    int         rc = 0;
    size_t      len;
    asn_value  *pdu;

    *udp_dgram = (struct udp4_datagram *)malloc(sizeof(**udp_dgram));
    if (*udp_dgram == NULL)
        return TE_ENOMEM;

    memset(*udp_dgram, 0, sizeof(**udp_dgram));

    asn_save_to_file(pkt, "/tmp/asn_file.asn");

    if ((rc = ndn_get_timestamp(pkt, &((*udp_dgram)->ts))) != 0)
    {
        free(*udp_dgram);
        return TE_RC(TE_TAPI, rc);
    }

    pdu = asn_read_indexed(pkt, 0, "pdus"); /* this should be UDP PDU */

    if (pdu == NULL)
        rc = TE_EASNINCOMPLVAL;

    READ_PACKET_FIELD(src, port);
    READ_PACKET_FIELD(dst, port);

    pdu = asn_read_indexed(pkt, 1, "pdus"); /* this should be Ip4 PDU */
    if (pdu == NULL)
        rc = TE_EASNINCOMPLVAL;

    READ_PACKET_FIELD(src, addr);
    READ_PACKET_FIELD(dst, addr);

    if (rc)
    {
        free(*udp_dgram);
        return TE_RC(TE_TAPI, rc);
    }

    len = asn_get_length(pkt, "payload");
    if (len <= 0)
        return 0;

    (*udp_dgram)->payload_len = len;
    (*udp_dgram)->payload = malloc(len);

    rc = asn_read_value_field(pkt, (*udp_dgram)->payload, &len, "payload");

    return TE_RC(TE_TAPI, rc);
}

#undef READ_PACKET_FIELD

/**
 * Create Traffic-Pattern-Unit for udp.ip4.eth CSAP
 *
 * @param src_addr      IPv4 source address (or NULL)
 * @param dst_addr      IPv4 destination address (or NULL)
 * @param src_port      UDP source port (or 0)
 * @param dst_port      UDP destination port (or 0)
 * @param result_value  Location for resulting ASN pattern
 *
 * @return Zero on success or error code
 */
int
tapi_udp_ip4_eth_pattern_unit(const uint8_t *src_addr,
                              const uint8_t *dst_addr,
                              uint16_t src_port, uint16_t dst_port,
                              asn_value **result_value)
{
    int        rc;
    int        num;
    asn_value *pattern = NULL;

    if (result_value == NULL)
        return TE_RC(TE_TAPI, TE_EWRONGPTR);

    do {
        rc = asn_parse_value_text("{ pdus { udp:{}, ip4:{}, eth:{}}}",
                                  ndn_traffic_pattern_unit, &pattern, &num);
        if (rc != 0) break;

        if (src_addr != NULL)
        {
            rc = asn_write_value_field(pattern, src_addr, sizeof(in_addr_t),
                                       "pdus.1.#ip4.src-addr.#plain");
            if (rc != 0) break;
        }
        if (dst_addr != NULL)
        {
            rc = asn_write_value_field(pattern, dst_addr, sizeof(in_addr_t),
                                       "pdus.1.#ip4.dst-addr.#plain");
            if (rc != 0) break;
        }
        if (src_port != 0)
        {
            rc = asn_write_value_field(pattern, &src_port, sizeof(src_port),
                                       "pdus.0.#udp.src-port.#plain");
            if (rc != 0) break;
        }
        if (dst_port != 0)
        {
            rc = asn_write_value_field(pattern, &dst_port, sizeof(dst_port),
                                       "pdus.0.#udp.dst-port.#plain");
            if (rc != 0) break;
        }
    } while(0);

    if (rc != 0)
    {
        ERROR("%s: error %X", __FUNCTION__, rc);
        asn_free_value(pattern);
        return TE_RC(TE_TAPI, rc);
    }
    *result_value = pattern;
    return 0;
}

/**
 * Create file with udp.ip4 Traffic-Pattern-Unit from UDP datagram
 * parameters
 *
 * @param fname     Name of file to create
 * @param dgram     Parameters of UDP endpoints
 *
 * @return Zero on success or error code
 */
static int
tapi_udp4_prepare_tmpl_file(const char *fname, const udp4_datagram *dgram)
{
    FILE *f;
    uint8_t *ip_addr_p;
    int i;

    if (fname == NULL || dgram == NULL)
        return TE_RC(TE_TAPI, TE_EWRONGPTR);

    f = fopen(fname, "w");
    if (f == NULL)
    {
        ERROR("%s: failed to open file '%s' for UDP pattern writing",
              __FUNCTION__, fname);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    fprintf(f, "{ pdus { udp: {src-port plain:%d, dst-port plain:%d}\n",
            (int)dgram->src_port, (int)dgram->dst_port);

    ip_addr_p = (uint8_t*) &(dgram->src_addr.s_addr);
    fprintf(f, "         ip4: {src-addr plain:{%02x, %02x, %02x, %02x},\n",
            ip_addr_p[0], ip_addr_p[1], ip_addr_p[2], ip_addr_p[3]);

    ip_addr_p = (uint8_t*) &(dgram->dst_addr.s_addr);
    fprintf(f,
            "               dst-addr plain:{%02x, %02x, %02x, %02x} } }",
            ip_addr_p[0], ip_addr_p[1], ip_addr_p[2], ip_addr_p[3]);

    if (dgram->payload_len > 0)
    {
        fprintf (f, ",\n  payload bytes:{");
        for (i = 0; i < dgram->payload_len; i++)
        {
            fprintf (f, " %02x", dgram->payload[i]);
            if (i != 0)
                fprintf (f, ",");
        }
        fprintf(f, "}");
    }
    fprintf(f, "\n}\n");

    fclose(f);
    return 0;
}


/* see description in tapi_udp.h */
int
tapi_udp4_csap_create(const char *ta_name, int sid,
                      const char *loc_addr_str, const char *rem_addr_str,
                      uint16_t loc_port, uint16_t rem_port,
                      csap_handle_t *udp_csap)
{
    int         rc;
    char        csap_fname[100] = "/tmp/te_udp4_csap.XXXXXX";
    asn_value_p csap_udp_level;
    asn_value_p csap_ip4_level;
    asn_value_p csap_spec;
    asn_value_p csap_level_spec;

    struct in_addr loc_addr = {0};
    struct in_addr rem_addr = {0};

    if ((loc_addr_str != NULL && inet_aton(loc_addr_str, &loc_addr) == 0) ||
        (rem_addr_str != NULL && inet_aton(rem_addr_str, &rem_addr) == 0))
    {
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    csap_spec       = asn_init_value(ndn_csap_spec);
    csap_level_spec = asn_init_value(ndn_generic_csap_level);
    csap_udp_level  = asn_init_value(ndn_udp_csap);

    asn_write_value_field(csap_udp_level, &loc_port, sizeof(loc_port),
                          "local-port.#plain");
    asn_write_value_field(csap_udp_level, &rem_port, sizeof(rem_port),
                          "remote-port.#plain");

    asn_write_component_value(csap_level_spec, csap_udp_level, "#udp");

    rc = asn_insert_indexed(csap_spec, csap_level_spec, 0, "");

    rc = asn_free_subvalue(csap_level_spec, "#udp");
    csap_level_spec = asn_init_value(ndn_generic_csap_level);
    rc = 0;

    if (rc == 0)
    {
        csap_ip4_level   = asn_init_value(ndn_ip4_csap);

        rc = asn_write_value_field(csap_ip4_level,
                                   &loc_addr, sizeof(loc_addr),
                                    "local-addr.#plain");
        rc = asn_write_value_field(csap_ip4_level,
                                   &rem_addr, sizeof(rem_addr),
                                    "remote-addr.#plain");
        rc = asn_write_component_value(csap_level_spec,
                                       csap_ip4_level, "#ip4");
    }

    if (rc == 0)
        rc = asn_insert_indexed(csap_spec, csap_level_spec, 1, "");

    if (rc == 0)
    {
        mktemp(csap_fname);
        rc = asn_save_to_file(csap_spec, csap_fname);
        VERB("TAPI: udp create csap, save to file %s, rc: %x\n",
                csap_fname, rc);
    }

    asn_free_value(csap_spec);
    asn_free_value(csap_udp_level);
    asn_free_value(csap_ip4_level);

    if (rc == 0)
        rc = rcf_ta_csap_create(ta_name, sid, "data.udp", 
                                csap_fname, udp_csap);

    return TE_RC(TE_TAPI, rc);
}

/* See description in tapi_udp.h */
int
tapi_udp_ip4_eth_csap_create(const char *ta_name, int sid,
                             const char *eth_dev,
                             const uint8_t *loc_mac,
                             const uint8_t *rem_mac,
                             in_addr_t loc_addr, in_addr_t rem_addr,
                             uint16_t loc_port, uint16_t rem_port,
                             csap_handle_t *udp_csap)
{
    int        rc;
    int        num = 0;
    asn_value *csap_spec = NULL;

    rc = asn_parse_value_text("{ udp:{}, ip4:{}, eth:{}}",
                              ndn_csap_spec, &csap_spec, &num);
    if (rc != 0)
        return rc;

    do {
        if (eth_dev != NULL)
        {
            rc = asn_write_value_field(csap_spec, eth_dev,
                                       strlen(eth_dev),
                                       "2.#eth.device-id.#plain");
            if (rc != 0) break;
        }
        if (loc_mac != NULL)
        {
            rc = asn_write_value_field(csap_spec, loc_mac, ETH_ALEN,
                                       "2.#eth.local-addr.#plain");
            if (rc != 0) break;
        }
        if (rem_mac != NULL)
        {
            rc = asn_write_value_field(csap_spec, rem_mac, ETH_ALEN,
                                       "2.#eth.remote-addr.#plain");
            if (rc != 0) break;
        }
        if (loc_addr != INADDR_ANY)
        {
            rc = asn_write_value_field(csap_spec, &loc_addr,
                                       sizeof(loc_addr),
                                       "1.#ip4.local-addr.#plain");
            if (rc != 0) break;
        }
        if (rem_addr != INADDR_ANY)
        {
            rc = asn_write_value_field(csap_spec, &rem_addr,
                                       sizeof(rem_addr),
                                       "1.#ip4.remote-addr.#plain");
            if (rc != 0) break;
        }
        if (loc_port != 0)
        {
            rc = asn_write_value_field(csap_spec, &loc_port,
                                       sizeof(loc_port),
                                       "0.#udp.local-port.#plain");
            if (rc != 0) break;
        }
        if (rem_port != 0)
        {
            rc = asn_write_value_field(csap_spec, &rem_port,
                                       sizeof(rem_port),
                                       "0.#udp.remote-port.#plain");
            if (rc != 0) break;
        }

        rc = tapi_tad_csap_create(ta_name, sid, "udp.ip4.eth",
                                  csap_spec, udp_csap);
    } while (0);

    asn_free_value(csap_spec);

    return TE_RC(TE_TAPI, rc);
}

/* see description in tapi_udp.h */
int
tapi_udp4_dgram_send(const char *ta_name, int sid,
                     csap_handle_t csap, const udp4_datagram *udp_dgram)
{
    int         rc;
    const char  templ_fname[] = "/tmp/te_udp4_send.XXXXXX";

    /* TODO! */

    UNUSED(udp_dgram);


    if ((rc = rcf_ta_trsend_start(ta_name, sid, csap, templ_fname,
                                  RCF_MODE_BLOCKING)) != 0)
    {
        printf("rcf_ta_trsend_start returns %d\n", rc);
    }
    return rc;
}

/* see description in tapi_udp.h */
static void
udp4_pkt_handler(char *pkt_fname, void *user_param)
{
    udp4_cb_data_t *cb_data = (udp4_cb_data_t *)user_param;
    asn_value      *pkt = NULL;
    int s_parsed;
    int rc;

    if ((rc = asn_parse_dvalue_in_file(pkt_fname, ndn_raw_packet,
                                       &pkt, &s_parsed)) != 0)
    {
        fprintf(stderr, "asn_parse_dvalue_in_file fails, rc = %x"
                ", s_parsed=%d\n", rc, s_parsed);
        return;
    }

    rc = ndn_udp4_dgram_to_plain(pkt, &cb_data->dgram);
    if (rc != 0)
    {
        fprintf(stderr, "ndn_udp4_dgram_to_plain fails, rc = %x\n", rc);
        return;
    }
    if (cb_data->callback != NULL)
    {
        cb_data->callback(cb_data->dgram, cb_data->callback_data);
        if (cb_data->dgram->payload)
            free(cb_data->dgram->payload);
        free(cb_data->dgram);
        cb_data->dgram = NULL;
    }
}

/* see description in tapi_udp.h */
int
tapi_udp4_dgram_start_recv(const char *ta_name,  int sid,
                           csap_handle_t csap,
                           const udp4_datagram *udp_dgram,
                           udp4_callback callback, void *user_data)
{
    char pattern_fname[] = "/tmp/te_udp4_pattern.XXXXXX";
    udp4_cb_data_t  *cb_data = calloc(1, sizeof(*cb_data));
    int              rc;
    unsigned int     timeout = 0; /** @todo Fix me */

    cb_data->callback = callback;
    cb_data->callback_data = user_data;

    mktemp(pattern_fname);

    /* TODO: here should be creation of pattern */
    UNUSED(udp_dgram);

    /** Recevie unlimited number of packets */
    rc = rcf_ta_trrecv_start(ta_name, sid, csap, pattern_fname,
                             udp4_pkt_handler, &cb_data, timeout, 0);
    unlink(pattern_fname);
    return rc;
}

/* See description in tapi_udp.h */
int
tapi_udp_ip4_eth_recv_start(const char *ta_name,  int sid,
                            csap_handle_t csap,
                            const udp4_datagram *udp_dgram,
                            udp4_callback callback, void *user_data)
{
    udp4_cb_data_t  *cb_data = calloc(1, sizeof(*cb_data));
    int              rc;
    unsigned int     timeout = 0; /** @todo Fix me */
    asn_value       *pattern;
    asn_value       *pattern_unit;

    cb_data->callback = callback;
    cb_data->callback_data = user_data;

    if (udp_dgram != NULL)
    {
        rc = tapi_udp_ip4_eth_pattern_unit((uint8_t *)&udp_dgram->src_addr,
                                           (uint8_t *)&udp_dgram->dst_addr,
                                           udp_dgram->src_port,
                                           udp_dgram->dst_port,
                                           &pattern_unit);
    }
    else
    {
        rc = tapi_udp_ip4_eth_pattern_unit(NULL, NULL, 0, 0, &pattern_unit);
    }
    if (rc != 0)
    {
        ERROR("%s: pattern unit creation error: %r", __FUNCTION__, rc);
        return rc;
    }
    pattern = asn_init_value(ndn_traffic_pattern);
    if ((rc = asn_insert_indexed(pattern, pattern_unit, 0, "")) != 0)
    {
        asn_free_value(pattern_unit);
        asn_free_value(pattern);
        ERROR("%s: pattern unit insertion error: %r", __FUNCTION__, rc);
        return rc;
    }
    asn_free_value(pattern_unit);

    rc = tapi_tad_trrecv_start(ta_name, sid, csap, pattern,
                               udp4_pkt_handler, cb_data, timeout, 0);
    asn_free_value(pattern);

    return rc;
}



/* see description in tapi_udp.h */
int
tapi_udp4_dgram_send_recv(const char *ta_name, int sid, csap_handle_t csap,
                          unsigned int timeout,
                          const udp4_datagram *dgram_sent,
                          udp4_datagram *dgram_recv)
{
    char            template_fname[] = "/tmp/te_udp4_send_recv.XXXXXX";
    udp4_cb_data_t *cb_data = calloc(1, sizeof(*cb_data));
    int             rc;

    mktemp(template_fname);

    tapi_udp4_prepare_tmpl_file(template_fname, dgram_sent);

    rc = rcf_ta_trsend_recv(ta_name, sid, csap, template_fname,
                                udp4_pkt_handler, cb_data, timeout, NULL);

    unlink(template_fname);

    /* TODO usage */
    UNUSED(dgram_recv);

    return rc;
}


