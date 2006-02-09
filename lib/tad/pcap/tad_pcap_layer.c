/** @file
 * @brief TAD PCAP
 *
 * Traffic Application Domain Command Handler.
 * Ethernet-PCAP CSAP layer-related callbacks.
 *
 * Copyright (C) 2005 Test Environment authors (see file AUTHORS
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
 * @author Konstantin Abramenko <konst@oktetlabs.ru>
 * @author Alexander Kukuta <Alexander.Kukuta@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER     "TAD Ethernet-PCAP"

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

#include "te_defs.h"
#include "te_printf.h"

#include "logger_api.h"
#include "logger_ta_fast.h"

#include "tad_pcap_impl.h"


#define PCAP_LINKTYPE_DEFAULT           DLT_EN10MB  /* See pcap-bpf.h */

#define PCAP_COMPILED_BPF_PROGRAMS_MAX  1024

#define TAD_PCAP_SNAPLEN                0xffff


typedef struct tad_pcap_layer_data {
    int                 iftype;     /**< Default link type
                                         (see man 3 pcap) */

    /** Array of pre-compiled BPF programs */
    struct bpf_program *bpfs[PCAP_COMPILED_BPF_PROGRAMS_MAX];

    int                 bpf_count;  /**< Total count of pre-compiled
                                         BPF programs */
} tad_pcap_layer_data;


/* See description tad_pcap_impl.h */
te_errno
tad_pcap_init_cb(csap_p csap, unsigned int layer)
{
    tad_pcap_layer_data    *layer_data;

    layer_data = calloc(1, sizeof(*layer_data));
    if (layer_data == NULL)
    {
        ERROR("Init, not memory for layer_data");
        return TE_RC(TE_TAD_CSAP, TE_ENOMEM);
    }
    csap_set_proto_spec_data(csap, layer, layer_data);

    layer_data->iftype = PCAP_LINKTYPE_DEFAULT;

    return 0;
}

/* See description tad_pcap_impl.h */
te_errno
tad_pcap_destroy_cb(csap_p csap, unsigned int layer)
{
    tad_pcap_layer_data    *layer_data;

    layer_data = csap_get_proto_spec_data(csap, layer);
    if (layer_data == NULL)
    {
        WARN("No PCAP CSAP %d special data found!", csap->id);
        return 0;
    }
    csap_set_proto_spec_data(csap, layer, NULL);

    free(layer_data);

    return 0;
}


/* See description in tad_pcap_impl.h */
te_errno
tad_pcap_confirm_ptrn_cb(csap_p csap, unsigned int layer,
                         asn_value_p layer_pdu, void **p_opaque)
{
    tad_pcap_layer_data    *layer_data;

    size_t                  val_len;
    char                   *pcap_str;
    int                     rc;

    struct bpf_program     *bpf_program;

    UNUSED(p_opaque);

    layer_data = csap_get_proto_spec_data(csap, layer);

    rc = asn_get_length(layer_pdu, "filter");
    if (rc < 0)
    {
        ERROR("%s(): asn_get_length() failed, rc=%r", __FUNCTION__, rc);
        return rc;
    }

    val_len = rc;

    pcap_str = (char *)malloc(val_len + 1);
    if (pcap_str == NULL)
    {
        return TE_RC(TE_TAD_CSAP, TE_ENOMEM);
    }

    rc = asn_read_value_field(layer_pdu, pcap_str, &val_len, "filter");
    if (rc < 0)
    {
        ERROR("%s(): asn_read_value_field() failed, rc=%r", __FUNCTION__, rc);
        return rc;
    }

    VERB("%s: Try to compile filter string \"%s\"", __FUNCTION__, pcap_str);

    bpf_program = (struct bpf_program *)malloc(sizeof(struct bpf_program));
    if (bpf_program == NULL)
    {
        return TE_RC(TE_TAD_CSAP, TE_ENOMEM);
    }

    rc = pcap_compile_nopcap(TAD_PCAP_SNAPLEN, layer_data->iftype,
                             bpf_program, pcap_str, TRUE, 0);
    if (rc != 0)
    {
        ERROR("%s(): pcap_compile_nopcap() failed, rc=%d", __FUNCTION__, rc);
        return TE_RC(TE_TAD_CSAP, TE_EINVAL);
    }
    VERB("%s: pcap_compile_nopcap() returns %d", __FUNCTION__, rc);

    layer_data->bpfs[++layer_data->bpf_count] = bpf_program;

    val_len = sizeof(int);
    rc = asn_write_value_field(layer_pdu, &layer_data->bpf_count,
                               val_len, "bpf-id");
    if (rc < 0)
    {
        ERROR("%s(): asn_write_value_field() failed, rc=%r",
              __FUNCTION__, rc);
        return rc;
    }

    VERB("%s: filter string compiled, bpf-id %d", __FUNCTION__,
         layer_data->bpf_count);

    VERB("exit, return 0");

    return 0;
}

/* See description tad_pcap_impl.h */
void
tad_pcap_release_ptrn_cb(csap_p csap, unsigned int layer, void *opaque)
{
    tad_pcap_layer_data    *layer_data;
    int                     bpf_id;

    UNUSED(opaque);

    layer_data = csap_get_proto_spec_data(csap, layer); 

    for (bpf_id = 1; bpf_id <= layer_data->bpf_count; bpf_id++)
    {
        if (layer_data->bpfs[bpf_id] != NULL)
        {
            pcap_freecode(layer_data->bpfs[bpf_id]);
            free(layer_data->bpfs[bpf_id]);
            layer_data->bpfs[bpf_id] = NULL;
        }
    }

    layer_data->bpf_count = 0;
}


/* See description in tad_pcap_impl.h */
te_errno
tad_pcap_match_bin_cb(csap_p csap,
                        unsigned int     layer,
                        const asn_value *ptrn_pdu,
                        void            *ptrn_opaque,
                        tad_recv_pkt    *meta_pkt,
                        tad_pkt         *pdu,
                        tad_pkt         *sdu)
{
    tad_pcap_layer_data    *layer_data;
    te_errno                rc;
    int                     ret;
    uint8_t                *data;
    size_t                  data_len;
    int                     bpf_id;
    struct bpf_program     *bpf_program;
    struct bpf_insn        *bpf_code;
    size_t                  tmp_len;

    UNUSED(ptrn_opaque);
    UNUSED(meta_pkt);

    F_VERB("%s() started", __FUNCTION__);

    assert(tad_pkt_seg_num(pdu) == 1);
    assert(tad_pkt_first_seg(pdu) != NULL);
    data = tad_pkt_first_seg(pdu)->data_ptr;
    data_len = tad_pkt_first_seg(pdu)->data_len;

    layer_data = csap_get_proto_spec_data(csap, layer);
    assert(layer_data != NULL);

    tmp_len = sizeof(int);
    rc = asn_read_value_field(ptrn_pdu, &bpf_id, &tmp_len, "bpf-id");
    if (rc != 0)
    {
        ERROR("%s(): Cannot read \"bpf-id\" field from PDU pattern",
              __FUNCTION__);
        return rc;
    }

    /* bpf_id == 0 means that filter string is not compiled yet */
    if ((bpf_id <= 0) || (bpf_id > layer_data->bpf_count))
    {
        ERROR("%s(): Invalid bpf_id value in PDU pattern", __FUNCTION__);
        return TE_RC(TE_TAD_CSAP, TE_EINVAL);
    }

    bpf_program = layer_data->bpfs[bpf_id];
    if (bpf_program == NULL)
    {
        ERROR("%s(): Invalid bpf_id value in PDU pattern", __FUNCTION__);
        return TE_RC(TE_TAD_CSAP, TE_EINVAL);
    }

    bpf_code = bpf_program->bf_insns;

    ret = bpf_filter(bpf_code, data, data_len, data_len);
    F_VERB("bpf_filter() returns 0x%x (%d)", ret, ret);
    if (ret <= 0)
    {
        return TE_ETADNOTMATCH;
    }

#if 1
    do {
        int filter_id;
        size_t filter_len;
        char *filter = NULL;

        VERB("Packet matches, try to get filter string");

        filter_len = sizeof(int);

        if (asn_read_value_field(ptrn_pdu, &filter_id,
                                 &filter_len, "filter-id") < 0)
        {
            ERROR("Cannot get filter-id");
            filter_id = -1;
        }

        ret = asn_get_length(ptrn_pdu, "filter");
        if (ret < 0)
        {
            ERROR("Cannot get length of filter string");
            break;
        }
        filter_len = ret;

        filter = (char *) malloc(filter_len + 1);
        if (filter == NULL)
        {
            ERROR("Cannot allocate memory for filter string");
            break;
        }

        if (asn_read_value_field(ptrn_pdu, filter,
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

    rc = tad_pkt_get_frag(sdu, pdu, 0, data_len,
                          TAD_PKT_GET_FRAG_ERROR);
    if (rc != 0)
    {
        ERROR(CSAP_LOG_FMT "Failed to prepare Ethernet SDU: %r",
              CSAP_LOG_ARGS(csap), rc);
        return rc;
    }

    F_VERB(CSAP_LOG_FMT "PCAP packet (len=%u) matched",
           CSAP_LOG_ARGS(csap), (unsigned)data_len);

    return 0;
}
