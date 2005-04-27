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
 * @author Konstantin Abramenko <konst@oktetlabs.ru>
 * @author Alexander Kukuta <Alexander.Kukuta@oktetlabs.ru>
 *
 * $Id: $
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>

#if 0
#define TE_LOG_LEVEL    0xff
#endif

#define TE_LGR_USER     "TAD Ethernet-PCAP"
#include "logger_ta.h"

#include "tad_pcap_impl.h"
#include "te_defs.h"

#if 1
#define PCAP_DEBUG(args...) \
    do {                                        \
        fprintf(stdout, "\nTAD PCAP stack " args);    \
        INFO("TAD PCAP stack " args);                 \
    } while (0)

#undef ERROR
#define ERROR(args...) PCAP_DEBUG("ERROR: " args)

#undef RING
#define RING(args...) PCAP_DEBUG("RING: " args)

#undef WARN
#define WARN(args...) PCAP_DEBUG("WARN: " args)

#undef VERB
#define VERB(args...) PCAP_DEBUG("VERB: " args)
#endif

/**
 * Callback for read parameter value of Ethernet-PCAP CSAP.
 *
 * @param csap_id       identifier of CSAP.
 * @param level         Index of level in CSAP stack, which param is wanted.
 * @param param         Protocol-specific name of parameter.
 *
 * @return 
 *     String with textual presentation of parameter value, or NULL 
 *     if error occured. User have to free memory at returned pointer.
 */ 
char* pcap_get_param_cb (csap_p csap_descr, int level, const char *param)
{
    char   *param_buf;            
    pcap_csap_specific_data_p spec_data;

    VERB("%s() started", __FUNCTION__);

    if (csap_descr == NULL)
    {
        VERB("error in pcap_get_param %s: wrong csap descr passed\n", param);
        return NULL;
    }

    spec_data =
        (pcap_csap_specific_data_p) csap_descr->layers[level].specific_data; 

    if (strcmp (param, "total_bytes") == 0)
    {
        param_buf  = malloc(20);
        sprintf(param_buf, "%u", spec_data->total_bytes);
        return param_buf;
    }

    VERB("error in pcap_get_param %s: not supported parameter\n", param);

    return NULL;
}


/**
 * Callback for confirm PDU with Ethernet-PCAP CSAP
 * parameters and possibilities.
 *
 * @param csap_id       identifier of CSAP
 * @param layer         numeric index of layer in CSAP type to be processed.
 * @param tmpl_pdu      asn_value with PDU (IN/OUT)
 *
 * @return zero on success or error code.
 */ 
int pcap_confirm_pdu_cb (int csap_id, int layer, asn_value_p tmpl_pdu)
{
    pcap_csap_specific_data_p   spec_data; 
    csap_p                      csap_descr;

    int                         val_len;
    char                       *pcap_str;
    int                         rc; 

    struct bpf_program         *bpf_program;
    
    VERB("%s() started", __FUNCTION__);
    
    if ((csap_descr = csap_find(csap_id)) == NULL)
    {
        ERROR("null csap_descr for csap id %d", csap_id);
        return ETADCSAPNOTEX;
    }
    
    spec_data = (pcap_csap_specific_data_p)
        csap_descr->layers[layer].specific_data; 

    rc = asn_get_length(tmpl_pdu, "filter");
    if (rc < 0)
    {
        ERROR("%s(): asn_get_length() failed, rc=%d", __FUNCTION__, rc);
        return rc;
    }
    
    val_len = rc;

    pcap_str = (char *)malloc(val_len + 1);
    if (pcap_str == NULL)
    {
        return TE_RC(TE_TAD_CSAP, ENOMEM);
    }
    
    rc = asn_read_value_field(tmpl_pdu, pcap_str, &val_len, "filter");
    if (rc < 0)
    {
        ERROR("%s(): asn_read_value_field() failed, rc=%d", __FUNCTION__, rc);
        return rc;
    }

    VERB("%s: Try to compile filter string \"%s\"", __FUNCTION__, pcap_str);    

    bpf_program = (struct bpf_program *)malloc(sizeof(struct bpf_program));
    if (bpf_program == NULL)
    {
        return TE_RC(TE_TAD_CSAP, ENOMEM);
    }

    rc = pcap_compile_nopcap(TAD_PCAP_SNAPLEN, spec_data->iftype,
                             bpf_program, pcap_str, TRUE, 0);
    if (rc != 0)
    {
        ERROR("%s(): pcap_compile_nopcap() failed, rc=%d", __FUNCTION__, rc);
        return TE_RC(TE_TAD_CSAP, EINVAL);
    }
    VERB("%s: pcap_compile_nopcap() returns %d", __FUNCTION__, rc);

    spec_data->bpfs[++spec_data->bpf_count] = bpf_program;

    val_len = sizeof(int);
    rc = asn_write_value_field(tmpl_pdu, &spec_data->bpf_count,
                               val_len, "bpf-id");
    if (rc < 0)
    {
        ERROR("%s(): asn_write_value_field() failed, rc=%d",
              __FUNCTION__, rc);
        return rc;
    }

    VERB("%s: filter string compiled, bpf-id %d", __FUNCTION__,
         spec_data->bpf_count);

    VERB("exit, return 0");
    
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
int 
pcap_match_bin_cb(int csap_id, int layer, const asn_value *pattern_pdu,
                  const csap_pkts *pkt, csap_pkts *payload, 
                  asn_value_p parsed_packet)
{
    csap_p                      csap_descr;
    pcap_csap_specific_data_p   spec_data;
    int                         rc;
    uint8_t                    *data;
    int                         bpf_id;
    struct bpf_program         *bpf_program;
	struct bpf_insn            *bpf_code;
    int                         tmp_len;

    VERB("%s() started", __FUNCTION__);

    if ((csap_descr = csap_find(csap_id)) == NULL)
    {
        ERROR("null csap_descr for csap id %d", csap_id);
        return ETADCSAPNOTEX;
    }

    spec_data = (pcap_csap_specific_data_p)
        csap_descr->layers[layer].specific_data;
    data = pkt->data; 

    if (pattern_pdu == NULL)
    {
        VERB("pattern pdu is NULL, packet matches");
    }

    tmp_len = sizeof(int);
    rc = asn_read_value_field(pattern_pdu, &bpf_id, &tmp_len, "bpf-id");
    if (rc != 0)
    {
        ERROR("%s(): Cannot read \"bpf-id\" field from PDU pattern",
              __FUNCTION__);
        return rc;
    }

    /* bpf_id == 0 means that filter string is not compiled yet */
    if ((bpf_id <= 0) || (bpf_id > spec_data->bpf_count))
    {
        ERROR("%s(): Invalid bpf_id value in PDU pattern", __FUNCTION__);
        return TE_RC(TE_TAD_CSAP, EINVAL);
    }

    bpf_program = spec_data->bpfs[bpf_id];
    if (bpf_program == NULL)
    {
        ERROR("%s(): Invalid bpf_id value in PDU pattern", __FUNCTION__);
        return TE_RC(TE_TAD_CSAP, EINVAL);
    }
    
    bpf_code = bpf_program->bf_insns;
    
    rc = bpf_filter(bpf_code, pkt->data, pkt->len, pkt->len);
    VERB("bpf_filter() returns 0x%x (%d)", rc, rc);
    if (rc <= 0)
    {
        return ETADNOTMATCH;
    }

#if 1
    do {
        int filter_id;
        int filter_len;
        char *filter = NULL;

        VERB("Packet matches, try to get filter string");

        filter_len = sizeof(int);
        
        if (asn_read_value_field(pattern_pdu, &filter_id, 
                                 &filter_len, "filter-id") < 0)
        {
            ERROR("Cannot get filter-id");
            filter_id = -1;
        }
        
        filter_len = asn_get_length(pattern_pdu, "filter");
        if (filter_len < 0)
        {
            ERROR("Cannot get length of filter string");
            break;
        }
        
        filter = (char *) malloc(filter_len + 1);
        if (filter == NULL)
        {
            ERROR("Cannot allocate memory for filter string");
            break;
        }
        
        if (asn_read_value_field(pattern_pdu, filter,
                                 &filter_len, "filter") < 0)
        {
            ERROR("Cannot get filter string");
            free(filter);
            break;
        }
        
        filter[filter_len] = '\0';
        
        VERB("Received packet matches to filter: \"%s\", filter-id=%d",
             filter, filter_id);
        
        free(filter);
    } while (0);
#endif

    /* Fill parsed packet value */
    if (parsed_packet)
    {
        rc = asn_write_component_value(parsed_packet, pattern_pdu, "#pcap"); 
        if (rc)
            ERROR("write pcap filter to packet rc %x\n", rc);
    }

    VERB("Try to copy payload of %d bytes", pkt->len);

    /* passing payload to upper layer */
    memset(payload, 0 , sizeof(*payload));
    payload->len = pkt->len;

    payload->data = malloc(payload->len);
    memcpy(payload->data, pkt->data, payload->len);

    F_VERB("PCAP csap N %d, packet matches, pkt len %d, pld len %d", 
           csap_id, pkt->len, payload->len);
    
    return 0;
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
int
pcap_gen_pattern_cb(int csap_id, int layer, const asn_value *tmpl_pdu, 
                    asn_value_p   *pattern_pdu)
{ 
    UNUSED(csap_id); 
    UNUSED(layer);
    UNUSED(tmpl_pdu);

    VERB("%s() started", __FUNCTION__);

    if (pattern_pdu)
        *pattern_pdu = NULL;

    return ETENOSUPP;
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
int pcap_gen_bin_cb(csap_p csap_descr, int layer, const asn_value *tmpl_pdu,
                    const tad_tmpl_arg_t *args, size_t arg_num, 
                    const csap_pkts_p up_payload, csap_pkts_p pkts)
{
    UNUSED(csap_descr);
    UNUSED(layer);
    UNUSED(tmpl_pdu);
    UNUSED(args);
    UNUSED(arg_num);
    UNUSED(up_payload);
    UNUSED(pkts);
    
    return EOPNOTSUPP;
}

