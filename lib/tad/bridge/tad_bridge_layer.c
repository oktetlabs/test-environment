/** @file
 * @brief Test Environment: 
 *
 * Traffic Application Domain Command Handler
 * Ethernet CSAP layer-related callbacks.
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
 * Author: Konstantin Abramenko <konst@oktetlabs.ru>
 *
 * @(#) $Id$
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include "tad_bridge_impl.h"
#include "te_defs.h"

#define LLC_BPDU_DSAP_SSAP (0x42)
#define LLC_BPDU_CONTROL (0x03)

/**
 * Callback for read parameter value of bridge CSAP.
 *
 * @param csap_id       identifier of CSAP.
 * @param level         Index of level in CSAP stack, which param is wanted.
 * @param param         Protocol-specific name of parameter.
 *
 * @return 
 *     String with textual presentation of parameter value, or NULL 
 *     if error occured. User have to free memory at returned pointer.
 */ 
char* bridge_get_param_cb (int csap_id, int level, const char *param)
{
    UNUSED(csap_id);
    UNUSED(level);
    UNUSED(param);
    return NULL;
}

/**
 * Callback for confirm PDU with ethernet CSAP parameters and possibilities.
 *
 * @param csap_id       identifier of CSAP
 * @param layer         numeric index of layer in CSAP type to be processed.
 * @param tmpl_pdu      asn_value with PDU (IN/OUT)
 *
 * @return zero on success or error code.
 */ 
int bridge_confirm_pdu_cb (int csap_id, int layer, asn_value_p tmpl_pdu)
{
    int    rc = 0; 
    csap_p csap_descr;
    char   buffer[8]; /* maximum length of field in Config BPDU*/

    UNUSED (layer);

    memset(buffer, 0, sizeof(buffer));

    if ((csap_descr = csap_find(csap_id)) == NULL)
    {
        return TE_ETADCSAPNOTEX;
    } 

    VERB("bridge confirm called\n");
    if (csap_descr->command == TAD_OP_RECV)
    {
        VERB("Noting to do with RX CSAP\n");
        return 0;

    } 

#define CHECK_FIELD(label, val, len)    \
    do if (rc == 0) {                                           \
        char _buffer[100];                                      \
        size_t _len = sizeof(_buffer);                          \
                                                                \
        rc = asn_read_value_field(tmpl_pdu, _buffer, &_len,     \
                                  label ".#plain");             \
        VERB("CHECK field %s, asn_read rc %x", label, rc);      \
        switch (rc)                                             \
        {                                                       \
            case 0:                                             \
            /* TODO: compare with internal CSAP settings! */    \
                break;                                          \
                                                                \
            case TE_EASNOTHERCHOICE:                            \
            /* TODO: process complex tmpl should be here*/      \
                asn_free_subvalue (tmpl_pdu, label);            \
            /* fall through! */                                 \
            case TE_EASNINCOMPLVAL:                             \
                rc = asn_write_value_field(tmpl_pdu, val, len,  \
                                           label ".#plain");    \
                VERB("rc from asn write %x\n", rc);             \
        }                                                       \
    } while (0)

    CHECK_FIELD("proto-id", buffer, 2);
    CHECK_FIELD("version-id", buffer, 1);
    CHECK_FIELD("bpdu-type", buffer, 1);
    rc = asn_get_choice(tmpl_pdu, "content", buffer, sizeof(buffer));
    /* if there are no content, we assume CFG PBDU ??? */

    if (rc && TE_RC_GET_ERROR(rc) != TE_EASNINCOMPLVAL)
    {
        VERB("bridge confirm, get choice failed %x\n", rc);
        return rc;
    }
    if (TE_RC_GET_ERROR(rc) == TE_EASNINCOMPLVAL || 
        strcmp(buffer, "cfg") == 0)
    {
        rc = 0;

        CHECK_FIELD("content.#cfg.flags", buffer, 1);
        CHECK_FIELD("content.#cfg.root-id", buffer, 8);
        CHECK_FIELD("content.#cfg.root-path-cost", buffer, 4);
        CHECK_FIELD("content.#cfg.bridge-id", buffer, 8);
        CHECK_FIELD("content.#cfg.port-id", buffer, 2);
        CHECK_FIELD("content.#cfg.message-age", buffer, 2);
        CHECK_FIELD("content.#cfg.max-age", buffer, 2);
        CHECK_FIELD("content.#cfg.hello-time", buffer, 2);
        CHECK_FIELD("content.#cfg.forward-delay", buffer, 2);
    }
    VERB("bridge confirm return, %x\n", rc);
    
#undef CHECK_FIELD
    return rc;
}

/**
 * Callback for generate binary data to be sent to media.
 *
 * @param csap_descr    CSAP instance
 * @param layer         numeric index of layer in CSAP type to be processed.
 * @param tmpl_pdu      asn_value with PDU. 
 * @param up_payload    pointer to data which is already generated for upper 
 *                      layers and is payload for this protocol level. 
 *                      May be zero.  Presented as list of packets. 
 *                      Almost always this list will contain only one element, 
 *                      but need in fragmentation sometimes may occur. 
 *                      Of cause, on up level only one PDU is passed, 
 *                      but upper layer (if any present) may perform 
 *                      fragmentation, and current layer may have possibility 
 *                      to de-fragment payload.
 * @param pkts          Callback have to fill this structure with list of 
 *                      generated packets. Almost always this list will 
 *                      contain only one element, but need 
 *                      in fragmentation sometimes may occur. (OUT)
 *
 * @return zero on success or error code.
 */ 
int
bridge_gen_bin_cb(csap_p csap_descr, int layer, const asn_value *tmpl_pdu,
                  const tad_tmpl_arg_t *args, size_t  arg_num, 
                  const csap_pkts_p  up_payload, csap_pkts_p pkts)
{
    int rc;
    int frame_size = 50;/* TODO: correct, rather dummy */

    uint8_t *data;
    uint8_t *p;

    ndn_stp_bpdu_t bridge_pdu; 


    UNUSED(csap_descr); /* not necessary for this method currently. */
    UNUSED(layer); /* not necessary for this method currently. */
    UNUSED(up_payload); /* N/A for this CSAP. */
    UNUSED(args); 
    UNUSED(arg_num); 


    /* At this moment only #plain choices should leave in template */
    rc = ndn_bpdu_asn_to_plain (tmpl_pdu, &bridge_pdu);

    F_VERB("ndn_bpdu_asn_to_plain return %r", rc);

    if (rc) 
        return rc;

    if ((data = malloc(frame_size)) == NULL)
    {
        return TE_ENOMEM; /* can't allocate memory for frame data */
    }
    
    /* filling with zeroes */
    memset(data, 0, frame_size); 
    p = data;

    /* fill LLC */ 

    *p++ = LLC_BPDU_DSAP_SSAP;
    *p++ = LLC_BPDU_DSAP_SSAP;
    *p++ = LLC_BPDU_CONTROL;

    /* BPDU itself */
    *(uint16_t *)p = (uint16_t)0;
    p += 2; 
    *p++ = bridge_pdu.version;
    *p++ = bridge_pdu.bpdu_type;
    if (bridge_pdu.bpdu_type == STP_BPDU_CFG_TYPE)
    {
        int len;


        *p++ = bridge_pdu.cfg.flags;

        len = sizeof(bridge_pdu.cfg.root_id);
        memcpy (p, bridge_pdu.cfg.root_id, len);
        p += len;

        *(uint32_t *)p = htonl(bridge_pdu.cfg.root_path_cost);
        p += 4;

        len = sizeof(bridge_pdu.cfg.bridge_id);
        memcpy (p, bridge_pdu.cfg.bridge_id, len);
        p += len;

        *(uint16_t *)p = htons(bridge_pdu.cfg.port_id);
        p += 2;
        *(uint16_t *)p = htons(bridge_pdu.cfg.msg_age);
        p += 2;
        *(uint16_t *)p = htons(bridge_pdu.cfg.max_age);
        p += 2;
        *(uint16_t *)p = htons(bridge_pdu.cfg.hello_time);
        p += 2;
        *(uint16_t *)p = htons(bridge_pdu.cfg.fwd_delay);
        p += 2;
        VERB(   "BDPU frame to be sent:\n rpc %d, port_id 0x%x,"
                " root_id/prio 0x%x; bridge_id/prio: 0x%x", 
                bridge_pdu.cfg.root_path_cost,
                bridge_pdu.cfg.port_id,
                (int)ntohs(*((uint16_t *)bridge_pdu.cfg.root_id)),
                (int)ntohs(*((uint16_t *)bridge_pdu.cfg.bridge_id))
               );

    } 

    pkts->data = data;
    pkts->len  = p - data;
    pkts->next = NULL;

    return 0;
}


/**
 * Callback for parse received packet and match it with pattern. 
 *
 * @param csap_id       identifier of CSAP
 * @param layer         numeric index of layer in CSAP type to be processed.
 * @param pattern_pdu   pattern NDS 
 * @param pkt           recevied packet
 * @param payload       rest upper layer payload, if any exists. (OUT)
 * @param parsed_packet caller of method should pass here empty asn_value 
 *                      instance of ASN type 'Generic-PDU'. Callback 
 *                      have to fill this instance with values from 
 *                      parsed and matched packet
 *
 * @return zero on success or error code.
 */
int bridge_match_bin_cb (int csap_id, int layer, const asn_value *pattern_pdu,
                       const csap_pkts *pkt, csap_pkts *payload, 
                       asn_value_p  parsed_packet )
{
    int      rc = 0;
    uint8_t *data;
    int      f_len;
    asn_value *bridge_pdu = NULL;
   
    UNUSED(csap_id); 
    UNUSED(layer);
    UNUSED(payload);
 
    data = pkt->data; 

    /* TODO: check LLC header */

    data += 3;
    if (parsed_packet != NULL)
        bridge_pdu = asn_init_value(ndn_bridge_pdu);

    f_len = 0;

    /* match of BPDUs should not be very fast, convert from ASN pattern 
     * to data_unit structure just in match callback. */

#define MATCH_FIELD(len_, label_) \
    do {                                                                \
        if (rc == 0)                                                    \
        {                                                               \
            rc = ndn_match_data_units(pattern_pdu, bridge_pdu,          \
                                      data, len_, label_);              \
            data += len_;                                               \
            if (rc != 0)                                                \
                F_VERB("%s: match %s rc %r", __FUNCTION__, label_, rc); \
        }                                                               \
    } while (0)

        
    MATCH_FIELD(2, "proto-id");
    MATCH_FIELD(1, "version-id"); 
    MATCH_FIELD(1, "bpdu-type"); 
    MATCH_FIELD(1, "content.#cfg.flags"); 
    MATCH_FIELD(8, "content.#cfg.root-id"); 
    MATCH_FIELD(4, "content.#cfg.root-path-cost"); 
    MATCH_FIELD(8, "content.#cfg.bridge-id"); 
    MATCH_FIELD(2, "content.#cfg.port-id"); 
    MATCH_FIELD(2, "content.#cfg.message-age"); 
    MATCH_FIELD(2, "content.#cfg.max-age"); 
    MATCH_FIELD(2, "content.#cfg.hello-time"); 
    MATCH_FIELD(2, "content.#cfg.forward-delay"); 

#undef MATCH_FIELD

    if (rc == 0 && parsed_packet != NULL)
    {
        rc = asn_write_component_value(parsed_packet, bridge_pdu, "#bridge");
        VERB("rc from asn_write_comp_val: %x", rc);
    }

    asn_free_value(bridge_pdu); 
    return rc; 
}

/**
 * Callback for generating pattern to filter 
 * just one response to the packet which will be sent by this CSAP 
 * according to this template. 
 *
 * @param csap_id       identifier of CSAP
 * @param layer         numeric index of layer in CSAP type to be processed.
 * @param tmpl_pdu      ASN value with template PDU.
 * @param pattern_pdu   OUT: ASN value with pattern PDU, generated according 
 *                      to passed template PDU and CSAP parameters. 
 *
 * @return zero on success or error code.
 */
int bridge_gen_pattern_cb (int csap_id, int layer, const asn_value *tmpl_pdu, 
                                         asn_value_p   *pattern_pdu)
{

    UNUSED(csap_id); 
    UNUSED(layer); 
    UNUSED(tmpl_pdu); 
    UNUSED(pattern_pdu); 

    return TE_EOPNOTSUPP;
}


