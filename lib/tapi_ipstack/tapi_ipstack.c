
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif

#include <stdio.h>
#include <assert.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include "rcf_api.h"
#include "conf_api.h"
#include "logger_api.h"

#include "tapi_ipstack.h"
#include "ndn_ipstack.h"


typedef struct {
    udp4_datagram  *dgram;
    void           *callback_data;
    udp4_callback   callback;
} udp4_cb_data_t;

#define DEBUG


/**
 * Convert UDP packet ASN value to plain C structure
 *
 * @param pkt           ASN value of type DHCPv4 message or Generic-PDU with
 *                      choice "dhcp"
 * @param udp_dgrm      converted structure (OUT)
 *
 * @return zero on success or error code
 *
 * @se Function allocates memory under dhcp_message data structure, which
 * should be freed with dhcpv4_message_destroy
 */
int
ndn_udp4_dgram_to_plain(asn_value_p pkt, udp4_datagram **udp_dgram)
{
    int         rc = 0;
    int         len;
    asn_value  *pdu;

    *udp_dgram = (struct udp4_datagram *)malloc(sizeof(**udp_dgram));
    if (*udp_dgram == NULL)
        return ENOMEM;

    memset(*udp_dgram, 0, sizeof(**udp_dgram));

    pdu = asn_read_indexed(pkt, 0, "pdus"); /* this should be UDP PDU */

    if (pdu == NULL)
        rc = EASNINCOMPLVAL;

#define READ_PACKET_FIELD(_dir, _field) \
    do {                                                        \
        len = sizeof((*udp_dgram)-> _dir ## _ ##_field ); \
        if (rc == 0)                                            \
            rc = asn_read_value_field(pdu,                      \
                        &((*udp_dgram)-> _dir ##_## _field ), \
                        &len, "_dir ## - ## _field");   \
    } while (0)

    READ_PACKET_FIELD(src, port);
    READ_PACKET_FIELD(dst, port);

    pdu = asn_read_indexed(pkt, 1, "pdus"); /* this should be Ip4 PDU */
    if (pdu == NULL)
        rc = EASNINCOMPLVAL;

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


static int
tapi_udp4_prepare_tmpl_file(const char *fname, const udp4_datagram *dgram)
{
    FILE *f;
    uint8_t *ip_addr_p;
    int i;

    if (fname == NULL || dgram == NULL)
        return TE_RC(TE_TAPI, ETEWRONGPTR);

    f = fopen (fname, "w");
    if (f == NULL)
        return TE_RC(TE_TAPI, EINVAL);
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
            if (i)
                fprintf (f, ",");
        }
        fprintf (f, "}");
    }
    fprintf (f, "\n}\n");

    fclose (f);
    return 0;
}



/**
 * Creates 'data.udp.ip4' CSAP
 *
 * @param ta_name    Test Agent name
 * @param
 * @param dhcp_csap  Location for the DHCPv4 CSAP handle (OUT)
 *
 * @return  Status of the operation
 */
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

    if ((loc_addr_str && inet_aton(loc_addr_str, &loc_addr) == 0) ||
        (rem_addr_str && inet_aton(rem_addr_str, &rem_addr) == 0))
    {
        return TE_RC(TE_TAPI, EINVAL);
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


/**
 * Send UDP datagram via 'data.udp.ip4' CSAP.
 *
 * @param ta            Test Agent name.
 * @param sid           RCF SID
 * @param csap          identifier of an SNMP CSAP (OUT).
 * @param udp_dgram     UDP datagram to be sent.
 *
 * @return zero on success or error code.
 */
int
tapi_udp4_dgram_send(const char *ta_name, int sid,
                     csap_handle_t csap, const udp4_datagram *udp_dgram)
{
    int         rc;
    const char  templ_fname[] = "/tmp/te_udp4_send.XXXXXX";


    if ((rc = rcf_ta_trsend_start(ta_name, sid, csap, templ_fname,
                                  RCF_MODE_BLOCKING)) != 0)
    {
        printf("rcf_ta_trsend_start returns %d\n", rc);
    }
    return rc;
}

/**
 * Handler that is used as a callback routine for processing incoming
 * packets
 *
 * @param pkt_fname   File name with ASN.1 representation of the received
 *                    packet
 * @param user_param  Pointer to the placeholder of the DHCP message handler
 *
 * @se It allocates a DHCP message
 */
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
        fprintf(stderr, "asn_parse_dvalue_in_file fails, rc = %x\n", rc);
        return;
    }

    rc = ndn_udp4_dgram_to_plain(pkt, &cb_data->dgram);
    if (rc != 0)
    {
        fprintf(stderr, "ndn_udp4_dgram_to_plain fails, rc = %x\n", rc);
        return;
    }
    if (cb_data->callback)
    {
        cb_data->callback(cb_data->dgram, cb_data->callback_data);
        if (cb_data->dgram->payload)
            free(cb_data->dgram->payload);
        free(cb_data->dgram);
        cb_data->dgram = NULL;
    }
}

/**
 * Start receiving of UDP datagrams via 'data.udp.ip4' CSAP.
 *
 * @param ta            Test Agent name.
 * @param sid           RCF SID
 * @param csap          identifier of an SNMP CSAP (OUT).
 * @param udp_dgram     UDP datagram with pattern for filter.
 * @param callback      callback function, which will be call for each
 *                      received packet
 * @param userdata      opaque data to be passed into the callback function.
 *
 * @return zero on success or error code.
 */
int
tapi_udp4_dgram_start_recv(const char *ta_name,  int sid,
            csap_handle_t csap, const  udp4_datagram *udp_dgram,
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

    /** Recevie unlimited number of packets */
    rc = rcf_ta_trrecv_start(ta_name, sid, csap, pattern_fname,
                             udp4_pkt_handler, &cb_data,
                             timeout, 0);


    unlink(pattern_fname);
    return rc;
}



/* see description in tapi_ipstack.h */
int
tapi_udp4_dgram_send_recv(const char *ta_name, int sid, csap_handle_t csap,
                          unsigned int timeout,
                          const udp4_datagram *dgram_send,
                          udp4_datagram *dgram_recv)
{
    char            template_fname[] = "/tmp/te_udp4_send_recv.XXXXXX";
    udp4_cb_data_t *cb_data = calloc(1, sizeof(*cb_data));
    int             rc;
    int             sid;

    mktemp(template_fname);

    tapi_udp4_prepare_tmpl_file(template_fname, dgram_send);

    rc = rcf_ta_create_session(ta_name, &sid);
    if (rc == 0)
        rc = rcf_ta_trsend_recv(ta_name, sid, csap, template_fname,
                                udp4_pkt_handler, cb_data, timeout, NULL);

    unlink(template_fname);

    return rc;
}


