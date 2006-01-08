/** @file
 * @brief TAD Receiver
 *
 * Traffic Application Domain Command Handler.
 * Receive module. 
 *
 * Copyright (C) 2003-2006 Test Environment authors (see file AUTHORS
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
 * @author Konstantin Abramenko <Konstantin.Abramenko@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER     "TAD Recv"

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <unistd.h>

#include "comm_agent.h"
#include "logger_api.h"
#include "logger_ta_fast.h"
#include "rcf_ch_api.h"
#include "rcf_pch.h"
#include "ndn.h" 

#include "tad_csap_inst.h"
#include "tad_csap_support.h"
#include "tad_utils.h"
#include "tad_send_recv.h"



#define SEND_ANSWER(_fmt...) \
    do {                                                             \
        int r_c;                                                     \
                                                                     \
        if (snprintf(answer_buffer + ans_len,                        \
                     ANS_BUF - ans_len, _fmt) >= ANS_BUF - ans_len)  \
        {                                                            \
            VERB("answer is truncated");                             \
        }                                                            \
        rcf_ch_lock();                                               \
        r_c = rcf_comm_agent_reply(context->rcfc, answer_buffer,     \
                                   strlen(answer_buffer) + 1);       \
        rcf_ch_unlock();                                             \
    } while (0)


#define ANS_BUF 100
#define RBUF 0x4000

/**
 * Process action for received packet.
 *
 * @param csap    CSAP descriptor
 * @param raw_pkt       binary data with original packet received
 * @param raw_len       length of original packet
 * @param payload       binary data with payload after match
 * @param payload_len   length of payload
 * @param action        ASN value of Packet-Action type
 *
 * @return status of operation
 */
int
tad_perform_action(csap_p csap,
                   uint8_t *raw_pkt, size_t raw_len,
                   uint8_t *payload, size_t payload_len,
                   const asn_value *action)
{
    const asn_value *action_ch_val;
    asn_tag_class    t_class;
    uint16_t         t_val; 
    int              rc = 0;

    if (csap == NULL || action == NULL || raw_pkt == NULL)
    {
        ERROR("%s(): NULL parameters passed", __FUNCTION__);
        return TE_EWRONGPTR;
    }

    rc = asn_get_choice_value(action, &action_ch_val,
                              &t_class, &t_val);
    VERB("%s(): get action choice rc %r, class %d, tag %d", 
         __FUNCTION__, rc, (int)t_class, (int)t_val);

    switch (t_val)
    {
#if 0
        case NDN_ACT_ECHO:
            if (csap->echo_cb != NULL)
            {
                rc = csap->echo_cb(csap, raw_pkt, raw_len);
                if (rc)
                    ERROR("csap #%d, echo_cb returned %r code.", 
                          csap->id, rc);
                /* Have no reason to stop receiving. */
                rc = 0;
            }
            break;
#endif

        case NDN_ACT_FUNCTION: 
            {
                tad_processing_pkt_method method_addr;

                char  buffer[200] = {0,};
                char *usr_place;
                size_t buf_len = sizeof(buffer);

                rc = asn_read_value_field(action_ch_val, buffer,
                                          &buf_len, "");
                if (rc != 0)
                    ERROR("csap #%d, ASN read value error %r", 
                          csap->id, rc); 
                else
                { 
                    /* 
                     * If there is no user string for function after colon,
                     * valid pointer to zero-length string will be passed.
                     */
                    for (usr_place = buffer; *usr_place; usr_place++)
                        if (*usr_place == ':')
                        {
                            *usr_place = 0;
                            usr_place++;
                            break;
                        }

                    VERB("function name: \"%s\"", buffer);

                    method_addr = (tad_processing_pkt_method) 
                        rcf_ch_symbol_addr((char *)buffer, 1);

                    if (method_addr == NULL)
                    {
                        ERROR("No funcion named '%s' found", buffer);
                        rc = TE_RC(TE_TAD_CH, TE_ENOENT); 
                    }
                    else
                    {
                        rc = method_addr(csap, usr_place,
                                         raw_pkt, raw_len);
                        if (rc != 0)
                            WARN("rc from user method %r", rc);
                        rc = 0;
                    }
                }
            }
            break;

        case NDN_ACT_FORWARD_PLD:
            {
                int32_t         target_csap_id;
                csap_p          target_csap;
                csap_spt_type_p cbs;

                asn_read_int32(action_ch_val, &target_csap_id, "");
                if ((target_csap = csap_find(target_csap_id)) != NULL)
                {
                    tad_pkt     pkt;
                    tad_pkt_seg seg;

                    tad_pkt_init(&pkt, NULL, NULL, NULL);
                    tad_pkt_init_seg_data(&seg, payload, payload_len, NULL);
                    tad_pkt_append_seg(&pkt, &seg);

                    cbs = csap_get_proto_support(target_csap,
                              csap_get_rw_layer(target_csap));
                    rc = cbs->write_cb(target_csap, &pkt);
                    VERB("action 'forward payload' processed");
                } 
            }
            break; 

        default:
            WARN("%s(CSAP %d) unsupported action tag %d",
                 __FUNCTION__, csap->id, t_val);
            break;
    }

    return rc;
}
 
/**
 * Try match binary data with Traffic-Pattern-Unit and prepare ASN value 
 * with packet if it satisfies to the pattern unit.
 *
 * @param data          binary data to be matched
 * @param d_len         length of data
 * @param csap    CSAP instance
 * @param pattern_unit  ASN value of 'Traffic-Pattern-Unit' type
 * @param packet        parsed packet (OUT)
 *
 * @return zero on success, otherwise error code.  
 */
int
tad_tr_recv_match_with_unit(uint8_t *data, int d_len, csap_p csap, 
                            const asn_value *pattern_unit,
                            asn_value_p *packet)
{
    const asn_value *action_seq = NULL;
    int              act_num = 0, i;
    te_bool          action_result = FALSE;

    int  layer;
    int  rc;
    char label[20] = "pdus";

    struct timeval current;

    gettimeofday(&current, NULL);

    csap_pkts data_to_check;
    csap_pkts rest_payload;


    memset(&rest_payload,  0, sizeof(rest_payload));
    memset(&data_to_check, 0, sizeof(data_to_check));


    rc = asn_get_child_value(pattern_unit, &action_seq,
                             PRIVATE, NDN_PU_ACTIONS);

    /* Check if there is 'result' in actions */
    if (rc == 0)
    {
        const asn_value *action_val;
        const asn_value *action_ch_val;
        asn_tag_class    t_class;
        uint16_t         t_val; 

        act_num = asn_get_length(action_seq, ""); 

        for (i = 0; i < act_num; i++)
        { 
            rc = asn_get_indexed(action_seq, &action_val, i);
            if (rc != 0)
            {
                ERROR("%s(): get %d action failed: %r",
                      __FUNCTION__, i, rc);
                break;
            }
            rc = asn_get_choice_value(action_val, &action_ch_val,
                                      &t_class, &t_val);
            if (rc == 0 && t_val == NDN_ACT_REPORT)
            {
                action_result = TRUE;
                break;
            }
        }
    }
    rc = 0;

    *packet = NULL;
    if ((csap->state & TAD_STATE_RESULTS) || action_result)
    {
        asn_value_p pdus = asn_init_value(ndn_generic_pdu_sequence);

        *packet = asn_init_value(ndn_raw_packet);
        asn_write_int32(*packet, current.tv_sec, "received.seconds");
        asn_write_int32(*packet, current.tv_usec, "received.micro-seconds");

        asn_write_component_value(*packet, pdus, "pdus");
        asn_free_value(pdus);
    }

    data_to_check.data = malloc(d_len);
    data_to_check.len  = d_len;
    
    memcpy(data_to_check.data, data, d_len);

    for (layer = csap->depth; layer-- > 0; )
    {
        csap_spt_type_p  csap_spt_descr; 
        const asn_value *layer_pdu = NULL; 
        asn_value_p      parsed_pdu = NULL;

        if ((csap->state & TAD_STATE_RESULTS) || action_result)
            parsed_pdu = asn_init_value(ndn_generic_pdu);

        sprintf(label + sizeof("pdus") - 1, ".%d", layer);
        rc = asn_get_subvalue(pattern_unit, &layer_pdu, label); 
        VERB("get subval with pattern unit for label %s rc %r",
             label, rc);

        csap_spt_descr = csap_get_proto_support(csap, layer);

        rc = csap_spt_descr->match_do_cb(csap, layer, layer_pdu, 
                                         &data_to_check, &rest_payload,
                                         parsed_pdu); 

        INFO("match cb 0x%x for lev %d returned %r",
             csap_spt_descr->match_do_cb, layer, rc);

        if (data_to_check.free_data_cb) 
            data_to_check.free_data_cb(data_to_check.data);
        else
            free(data_to_check.data);

        memset(&data_to_check, 0, sizeof(data_to_check));
        if (rc != 0)
        {
            asn_free_value(*packet);
            asn_free_value(parsed_pdu);
            *packet = NULL;
            return rc;
        }

        if ((csap->state & TAD_STATE_RESULTS) || action_result)
        {
            rc = asn_insert_indexed(*packet, parsed_pdu, 0, "pdus");
            asn_free_value(parsed_pdu);
            if (rc != 0)
            {
                ERROR("ASN error in add next pdu %r", rc);
                asn_free_value(*packet);
                *packet = NULL;
                return rc;
            } 

#ifdef TALOGDEBUG 
            {
                char buf[RBUF];
                asn_sprint_value(*packet, buf, 1000, 0);
                printf("packet: %s\n", buf);
            }
#endif
        }

        if (rest_payload.len)
        {
            memcpy(&data_to_check, &rest_payload, sizeof(rest_payload));
            memset(&rest_payload, 0, sizeof(rest_payload));
        }
        else break;
    }

    /* match payload */
    rc = asn_get_choice(pattern_unit, "payload", label, sizeof(label));
    if (rc == 0)
    {
        if (strcmp(label, "mask") == 0)
        {
            const asn_value *mask_pat;
            rc = asn_get_subvalue(pattern_unit, &mask_pat,
                                  "payload.#mask");
            if (rc != 0)
            {
                ERROR("%s(): get mask failed %r", __FUNCTION__, rc);
                asn_free_value(*packet);
                *packet = NULL;
                return rc;
            }
            rc = ndn_match_mask(mask_pat,
                                data_to_check.data, data_to_check.len);
            VERB("CSAP %d, rc from ndn_match_mask %r",
                 csap->id, rc);
        }

        if (rc != 0)
        {
            F_VERB("CSAP %d, Error matching pattern, rc %r",
                   csap->id, rc);
            asn_free_value(*packet);
            *packet = NULL;
        }
    }
    else if (TE_RC_GET_ERROR(rc) == TE_EASNINCOMPLVAL)
        rc = 0;


    if (rc == 0 && ((csap->state & TAD_STATE_RESULTS) || 
                    action_result))
    {
        if (data_to_check.len)
        {
            /* There are non-parsed payload rest */
            rc = asn_write_value_field(*packet, data_to_check.data, 
                                       data_to_check.len, "payload.#bytes");
            if (rc)
            {
                ERROR( "ASN error in add rest payload %r", rc);
                asn_free_value(*packet);
                *packet = NULL;
                return rc;
            }
        } 
    }


    /* process action, if it present. */
    if (rc == 0) 
    { 
        const asn_value *action_val;
        for (i = 0; i < act_num; i++)
        { 
            rc = asn_get_indexed(action_seq, &action_val, i);

            if (rc != 0)
            {
                ERROR("%s(): get %d action failed: %r",
                      __FUNCTION__, i, rc);
                break;
            }

            rc = tad_perform_action(csap, data, d_len, 
                                    data_to_check.data,
                                    data_to_check.len,
                                    action_val);
            if (rc != 0)
            {
                ERROR("%s(): perform %d action failed: %r",
                      __FUNCTION__, i, rc);
                break;
            }
        }
    }

    F_VERB("Packet to RX CSAP #%d matches", csap->id);

    if (data_to_check.free_data_cb)
        data_to_check.free_data_cb(data_to_check.data);
    else
        free(data_to_check.data); 

    if (rc != 0)
    {
        asn_free_value(*packet);
        *packet = NULL;
    }

    return rc; 
}


/**
 * Struct for element in queue of received packets. 
 */
typedef struct received_packets_queue_t {
    struct received_packets_queue_t *next;
    struct received_packets_queue_t *prev;

    asn_value_p pkt;
} received_packets_queue_t;

/**
 * Send received packet to the test via RCF
 *
 * @param packet        ASN value with received packets; 
 * @param rcfc          handle of RCF connection;
 * @param answer_buffer buffer with begin of answer;
 * @param ans_len       index of first significant symbols in answer_buffer;
 *
 * @return zero on success, otherwise error code.  
 */
int
tad_report_packet(asn_value_p packet, struct rcf_comm_connection *rcfc, 
                  char *answer_buffer, int ans_len) 

{
    int   rc;
    int   attach_len;
    char *buffer;
    char *attach;

    if (packet == NULL)
    {
        WARN("%s(): NULL packet passed", __FUNCTION__);
        return 0;
    }

    attach_len = asn_count_txt_len(packet, 0);

    /* 
     * 20 is upper estimation for "attach" and decimal presentation
     * of attach length 
     */
    if ((buffer = calloc(1, ans_len + 20 + attach_len)) == NULL)
        return TE_ENOMEM;

    memcpy(buffer, answer_buffer, ans_len);
    rc = sprintf(buffer + ans_len, " attach %d", attach_len);

    attach = buffer + strlen(buffer) + 1; 
    if (asn_sprint_value(packet, attach, attach_len + 1, 0) > attach_len)
    {
        ERROR("asn_sprint_value returns greater than expected!");
    } 

    VERB("%s():  attach len %d", __FUNCTION__, attach_len);

    rcf_ch_lock();
    rc = rcf_comm_agent_reply(rcfc, buffer,
                              strlen(buffer) + 1 + attach_len);
    rcf_ch_unlock(); 
    free(buffer);
    return rc;
}

/**
 * Send received packets in queue to the test via RCF and clear queue. 
 *
 * @param queue_root    queue with received packets
 * @param rcfc          handle of RCF connection
 * @param answer_buffer buffer with begin of answer
 * @param ans_len       index of first significant symbols in answer_buffer
 *
 * @return zero on success, otherwise error code
 */
int
tad_tr_recv_send_results(received_packets_queue_t *queue_root,
                         struct rcf_comm_connection *rcfc, 
                         char *answer_buffer, int ans_len) 
{
    int rc;
    int pkt_num = 0;

    received_packets_queue_t *pkt_qelem;

    for (pkt_qelem = queue_root->next; pkt_qelem != queue_root; 
         pkt_qelem = queue_root->next)
   {
        if(pkt_qelem->pkt != NULL)
        {
            rc = tad_report_packet(pkt_qelem->pkt, rcfc, 
                                    answer_buffer, ans_len);
            if (rc != 0) 
                return rc;

            pkt_num++;
            asn_free_value(pkt_qelem->pkt);
        } 
        REMQUE(pkt_qelem);
        free(pkt_qelem);
    }
    
    VERB("The number of reported packets is %d", pkt_num);

    return 0; 
}



void
tad_tr_recv_clear_results(received_packets_queue_t *queue_root)
{
    received_packets_queue_t *pkt_qelem;

    for (pkt_qelem = queue_root->next; pkt_qelem != queue_root; 
         pkt_qelem = queue_root->next)
   {
        if(pkt_qelem->pkt != NULL)
        {
            asn_free_value(pkt_qelem->pkt);
        } 
        REMQUE(pkt_qelem);
        free(pkt_qelem);
    }
}

/**
 * Generate Traffic Pattern NDS by template for trsend_recv command
 *
 * @param csap    structure with CSAP parameters
 * @param template      traffic template
 * @param pattern       generated Traffic Pattern (OUT)
 *
 * @return zero on success, otherwise error code.  
 */
static int
tad_tr_sr_generate_pattern(csap_p csap, asn_value_p template, 
                           asn_value_p *pattern)
{
    te_errno     rc = 0;
    unsigned int layer;

    asn_value_p pattern_unit = asn_init_value(ndn_traffic_pattern_unit);
    asn_value_p pdus         = asn_init_value(ndn_generic_pdu_sequence);

    VERB("%s called for csap # %d", __FUNCTION__, csap->id);

    rc = asn_write_component_value(pattern_unit, pdus, "pdus");
    if (rc != 0) 
        return rc;
    asn_free_value(pdus);

    for (layer = 0; layer < csap->depth; layer++)
    {
        csap_spt_type_p csap_spt_descr; 

        asn_value_p layer_tmpl_pdu; 
        asn_value_p layer_pattern; 
        asn_value_p gen_pattern_pdu = asn_init_value(ndn_generic_pdu);

        csap_spt_descr = csap_get_proto_support(csap, layer);

        layer_tmpl_pdu = asn_read_indexed(template, layer, "pdus"); 

        rc = csap_spt_descr->generate_pattern_cb(csap, layer,
                                                 layer_tmpl_pdu,
                                                 &layer_pattern);

        VERB("%s, lev %d, generate pattern cb rc %r", 
                __FUNCTION__, layer, rc);

        if (rc == 0) 
            rc = asn_write_component_value(gen_pattern_pdu, 
                                           layer_pattern, "");

        if (rc == 0) 
            rc = asn_insert_indexed(pattern_unit, gen_pattern_pdu, 
                                    layer, "pdus");

        asn_free_value(gen_pattern_pdu);
        asn_free_value(layer_pattern);
        asn_free_value(layer_tmpl_pdu);

        if (rc != 0) 
            break;
    } 

    if (rc == 0)
    {
        *pattern = asn_init_value(ndn_traffic_pattern);
        rc = asn_insert_indexed(*pattern, pattern_unit, 0, "");
    }
    VERB("%s, returns %r", __FUNCTION__, rc);
    asn_free_value(pattern_unit);
    return rc;
}

/**
 * Prepare CSAP for operations 
 *
 * @param csap    structure with CSAP parameters
 * @param pattern       patter of operation, need for check actions
 *
 * @return zero on success, otherwise error code
 */
int 
tad_prepare_csap(csap_p csap, const asn_value *pattern)
{
    int  rc;
    int  pu_num, i;
    int  echo_need = 0;
    char label[20];

    const asn_value *pu;

    csap_low_resource_cb_t  prepare_send_cb;
    csap_low_resource_cb_t  prepare_recv_cb;


    prepare_recv_cb = csap_get_proto_support(csap,
                          csap_get_rw_layer(csap))->prepare_recv_cb;
    prepare_send_cb = csap_get_proto_support(csap,
                          csap_get_rw_layer(csap))->prepare_send_cb;

    if (prepare_recv_cb)
    {
        rc = prepare_recv_cb(csap);
        if (rc != 0)
        {
            ERROR("prepare for recv failed %r", rc);
            return rc;
        }
    } 
    
    pu_num = asn_get_length(pattern, ""); 

    for (i = 0; i < pu_num; i++)
    {
        sprintf(label, "%d", i);
        rc = asn_get_subvalue(pattern, &pu, label);
        if (rc != 0)
            break;
        rc = asn_get_choice(pu, "action", label, sizeof(label));
        if(rc == 0 && (strcmp(label, "echo") == 0))
        {
            echo_need = 1;
            break;
        }
    }

    if (prepare_send_cb && 
        ((csap->state & TAD_STATE_SEND) || echo_need))
    {
        rc = prepare_send_cb(csap);
        if (rc != 0)
        {
            ERROR("prepare for recv failed %r", rc);
            return rc;
        }
    }

    return 0;
}

/**
 * Start routine for tr_recv thread. 
 *
 * @param arg      start argument, should be pointer to tad_task_context.
 *
 * @return NULL 
 */
void *
tad_receiver_thread(void *arg)
{
    tad_task_context *context = arg; 

    int           rc = 0;
    csap_p        csap;
    char          answer_buffer[ANS_BUF];  
    int           ans_len = 0;
    int           d_len = 0;
    unsigned int  pkt_count = 0;
    char         *read_buffer = NULL;

    asn_value_p   result = NULL;
    asn_value_p   nds = NULL; 

    csap_read_cb_t          read_cb;
    csap_write_read_cb_t    write_read_cb;

    received_packets_queue_t received_packets; 

    received_packets.pkt = NULL;
    received_packets.next = &received_packets;
    received_packets.prev = &received_packets; 

    if (arg == NULL)
    {
        ERROR("trrecv thread start point: null argument! exit");
        return NULL;
    }


    csap = context->csap;
    nds        = context->nds; 
    context->nds = NULL;

    VERB("START %s() CSAP %d, timeout %u", __FUNCTION__,
         csap->id, csap->timeout);

    if (csap == NULL)
    {
        ERROR("trrecv thread start point: null csap! exit.");
        asn_free_value(nds);
        return NULL;
    }
    asn_save_to_file(nds, "/tmp/nds.asn");

    read_cb = csap_get_proto_support(csap,
                  csap_get_rw_layer(csap))->read_cb;
    write_read_cb = csap_get_proto_support(csap,
                        csap_get_rw_layer(csap))->write_read_cb;

    do { /* symbolic do {} while(0) for traffic operation init block */

        rc = tad_prepare_csap(csap, nds);
        if (rc != 0)
            break; 

        strcpy(answer_buffer, csap->answer_prefix);
        ans_len = strlen(answer_buffer); 

        if ((read_buffer = malloc(RBUF)) == NULL)
        {
            rc = TE_ENOMEM; 
            break;
        }

    } while (0); 

    if (!(csap->state & TAD_STATE_FOREGROUND))
    {
        SEND_ANSWER("0 0");
    }
    VERB("trrecv thread for CSAP %d started", csap->id);

    if (rc != 0)
    {
        csap->last_errno = rc;
        csap->state |= TAD_STATE_COMPLETE;
        csap->state |= TAD_STATE_FOREGROUND;
        rc = 0;
    }

    /*
     * trsend_recv processing. 
     */
    if (!(csap->state   & TAD_STATE_COMPLETE) &&
         (csap->command == TAD_OP_SEND_RECV))
    {
        do {
            const asn_value *pattern_unit;
            tad_pkts        *pkts;

            asn_value_p pattern = NULL;

            tad_sender_context *send_context;

            rc = tad_sender_prepare(csap, nds, context->rcfc,
                                    &send_context);
            if (rc != 0)
            {
                ERROR("%s(): tad_sender_prepare failed: %r",
                      __FUNCTION__, rc);
                break;
            }
            if (send_context->tmpl_data.n_units != 1)
            {
                ERROR("%s(): invalid number of units in send/recv template",
                      __FUNCTION__);
                rc = TE_RC(TE_TAD_CH, TE_EINVAL);
                break;
            }

            pkts = malloc((csap->depth + 1) * sizeof(*pkts));
            if (pkts == NULL)
            {
                ERROR("%s(): memory allocation failure", __FUNCTION__);
                rc = TE_RC(TE_TAD_CH, TE_ENOMEM);
                break;
            }


            rc = tad_tr_send_prepare_bin(csap,
                     send_context->tmpl_data.units[0].nds, 
                     NULL, 0, NULL,
                     send_context->tmpl_data.units[0].layer_opaque,
                     pkts);
            if (rc != 0)
            {
                ERROR("%s(): tad_tr_send_prepare_bin failed: %r",
                      __FUNCTION__, rc);
                break;
            }

            rc = tad_tr_sr_generate_pattern(csap, nds, &pattern);
            INFO("generate pattern rc %r", rc);
            if (rc != 0)
                break;

            asn_free_value(nds);
            nds = pattern; 

            d_len = write_read_cb(csap, csap->timeout,
                                  pkts->pkts.cqh_first,
                                  read_buffer, RBUF); 

            /* only single packet processed in send_recv command */
            VERB("write_read cb return d_len %d", d_len);
            gettimeofday(&csap->first_pkt, NULL);
            csap->last_pkt = csap->first_pkt;

            if (d_len < 0)
            {
                rc = csap->last_errno;
                F_ERROR("CSAP #%d internal write_read error 0x%X", 
                         csap->id, csap->last_errno);
                break;
            }

            if (d_len == 0)
                break;

            asn_free_value(result);
            result = NULL;

            asn_get_indexed(pattern, &pattern_unit, 0);
            rc = tad_tr_recv_match_with_unit(read_buffer, d_len, csap,
                                             pattern_unit, &result); 

            INFO("CSAP %d: match_with_unit return %r", csap->id, rc);

            if (rc != 0)
            {
                if (TE_RC_GET_ERROR(rc) == TE_ETADNOTMATCH)
                    rc = 0;
                break;
            }

            if (result != NULL)
            { 
                received_packets_queue_t *new_qelem = 
                            malloc(sizeof(*new_qelem));
                new_qelem->pkt = result;
                INSQUE(new_qelem, received_packets.prev); 
                VERB("insert packet in queue");
            }
            csap->state |= TAD_STATE_COMPLETE;
        } while (0); 
    } /* finish of 'trsend_recv' special actions */

    if (rc != 0) 
    {
        /* non-zero rc may occure only from trsend_recv special block,
         * this is in foreground mode. */
        csap->state |= TAD_STATE_COMPLETE;
        csap->last_errno = rc;
        F_ERROR("generate binary data error: %r", rc); 
        rc = 0;
    }

    /* Loop for receiving/matching packages and wait for STOP/GET/WAIT. 
     * This loop should be braked only if 'foreground' operation complete 
     * of "STOP" received for 'background' operation. */ 
    while(!(csap->state & TAD_STATE_COMPLETE)  || 
          !(csap->state & TAD_STATE_FOREGROUND)  )
    {
        int num_pattern_units;
        int unit;

        const asn_value *pattern_unit;

        CSAP_DA_LOCK(csap);
        if (csap->command == TAD_OP_STOP || 
            csap->command == TAD_OP_DESTROY)
        {
            strcpy(answer_buffer, csap->answer_prefix);
            ans_len = strlen(answer_buffer); 

            CSAP_DA_UNLOCK(csap);
            VERB("trrecv_stop flag detected"); 
            break;
        }

        if ((csap->command == TAD_OP_WAIT) || 
            (csap->command == TAD_OP_GET))
        { 
            strcpy(answer_buffer, csap->answer_prefix);
            ans_len = strlen(answer_buffer); 

            if (csap->command == TAD_OP_WAIT)
            {
                csap->state |= TAD_STATE_FOREGROUND;
                csap->command = TAD_OP_RECV;
                F_VERB("%s: wait flag encountered. %d packets got",
                       __FUNCTION__, pkt_count);
            }

            rc = tad_tr_recv_send_results(&received_packets, context->rcfc, 
                                          answer_buffer, ans_len);
            if (rc != 0) 
            {
                ERROR("send 'trrecv_get' results failed with rc %r",
                      rc);
                /**
                 * @todo fix it. 
                 * Really, I don't know yet what to do if this failed. 
                 * If there is not bugs ;) this failure means that happens 
                 * something awful, such as lost connection with RCF. 
                 * But some bug seems to be more probable variant, so 
                 * process it as other errors: try to transmit via RCF
                 **/
                csap->last_errno = rc;
                csap->state |= TAD_STATE_COMPLETE;
                rc = 0;
            }

            if (csap->command == TAD_OP_GET)
            {
                csap->command = TAD_OP_RECV;
                SEND_ANSWER("0 %u", pkt_count); /* trrecv_get is finished */
                VERB("trrecv_get #%d OK, pkts: %u, state %x",
                     csap->id, pkt_count, (int)csap->state);
            }
        }
        CSAP_DA_UNLOCK(csap);

        if (!(csap->state & TAD_STATE_COMPLETE))
        {
            struct timeval pkt_caught;

            if (csap->wait_for.tv_sec)
            {
                struct timeval  current;
                int             s_diff;

                gettimeofday(&current, NULL);
                s_diff = current.tv_sec - csap->wait_for.tv_sec; 

                if (s_diff > 0 ||
                    (s_diff == 0 &&
                     current.tv_usec > csap->wait_for.tv_usec))
                {
                    csap->last_errno = ETIMEDOUT;
                    csap->state |= TAD_STATE_COMPLETE;
                    VERB("CSAP %d status complete by timeout, "
                         "wait for: %u.%u, current: %u.%u",
                         csap->id,
                         (uint32_t)csap->wait_for.tv_sec,
                         (uint32_t)csap->wait_for.tv_usec,
                         (uint32_t)current.tv_sec,
                         (uint32_t)current.tv_usec);
                    continue;
                }
            }

            d_len = read_cb(csap, csap->timeout, read_buffer, RBUF); 
            gettimeofday(&pkt_caught, NULL);

            if (d_len == 0)
                continue;

            if (d_len < 0)
            {
                csap->state |= TAD_STATE_COMPLETE;
                ERROR("CSAP read callback failed; rc: %r", rc);
                continue;
            } 
            
            num_pattern_units = asn_get_length(nds, ""); 

            for (unit = 0; unit < num_pattern_units; unit ++)
            {
                rc = asn_get_indexed(nds, &pattern_unit, unit);
                if (rc != 0)
                {
                    WARN("Get pattern unit fails %r", rc);
                    break;
                }
                rc = tad_tr_recv_match_with_unit(read_buffer, d_len, 
                                                 csap,
                                                 pattern_unit, &result); 
                INFO("CSAP %d, Match pkt return %x, unit %d", 
                     csap->id, rc, unit);
                switch (TE_RC_GET_ERROR(rc))
                {
                    case 0: /* received data matches to this pattern unit */
                        csap->last_pkt = pkt_caught;
                        if (!pkt_count)
                            csap->first_pkt = csap->last_pkt;
                        unit = num_pattern_units; /* to break from 'for' */
                        pkt_count++;
                        F_VERB("Match pkt, d_len %d, pkts %u",
                               d_len, pkt_count);
                        break;

                    case TE_ETADLESSDATA: /* @todo fragmentation */
                        unit = num_pattern_units; /* to break from 'for' */
                    case TE_ETADNOTMATCH:
                        continue;

                    default: 
                        ERROR("Match with pattern-unit failed "
                              "with code: %r", rc);
                        break;
                }
                if (rc != 0)
                    break;
            }

            if (TE_RC_GET_ERROR(rc) == TE_ETADNOTMATCH ||
                TE_RC_GET_ERROR(rc) == TE_ETADLESSDATA)
            {
                rc = 0;
                continue;
            }

            /* Here packet is successfully received, parsed and matched */

            if ((rc == 0) && (result != NULL))
            { 
                if (csap->state & TAD_STATE_FOREGROUND)
                {
                    F_VERB("in foreground mode");
                    rc = tad_report_packet(result, context->rcfc,
                                           answer_buffer, ans_len);
                    asn_free_value(result);
                }
                else
                {
                    F_VERB("Put packet into the queue");
                    received_packets_queue_t *new_qelem = 
                                            malloc(sizeof(*new_qelem));
                    new_qelem->pkt = result;
                    INSQUE(new_qelem, received_packets.prev);
                }
                result = NULL;
            } 

            if (rc != 0) 
            {
                csap->last_errno = rc;
                csap->state |= TAD_STATE_COMPLETE;
                rc = 0;
                continue;
            }

            if (csap->num_packets)
            {
                F_VERB("%s(): check for num_pkts, want %d, got %d",
                       __FUNCTION__, csap->num_packets, pkt_count);
                if (csap->num_packets <= pkt_count)
                {
                    csap->state |= TAD_STATE_COMPLETE;
                    INFO("%s(): CSAP %d status complete", 
                         __FUNCTION__, csap->id); 
                }
            }
        } /* if (need packets) */
    } /* while (background mode or not completed) */

    /* either stop got or foreground operation completed */ 

    if (csap->command == TAD_OP_DESTROY)
        tad_tr_recv_clear_results(&received_packets);
    else
        rc = tad_tr_recv_send_results(&received_packets, context->rcfc, 
                                      answer_buffer, ans_len);
    if (rc != 0)
    {
        ERROR("trrecv thread: send results failed with code %r", rc);
        if (csap->last_errno == 0)
            csap->last_errno = rc;
        rc = 0;
    } 

#if 0
    /* 
     * Release resources, this should be done before clearing 
     * CSAP state fields, becouse zero state during release means 
     * CSAP destroy procedure. 
     */
    if (csap->release_cb)
        csap->release_cb(csap);
#endif

    memset(&csap->wait_for, 0, sizeof(csap->wait_for)); 
    asn_free_value(nds);
    free(read_buffer);

    CSAP_DA_LOCK(csap);

    if (csap->command != TAD_OP_DESTROY ||
        csap->state & TAD_STATE_FOREGROUND)
    {
        csap->command = TAD_OP_IDLE;
        csap->state   = 0;
        SEND_ANSWER("%d %u", TE_OS_RC(TE_TAD_CSAP, csap->last_errno),
                    pkt_count);
    }
    else
    {
        RING("%s(): There was non-foreground operation, destroy CSAP", 
             __FUNCTION__);
        csap->command = TAD_OP_IDLE;
        csap->state   = 0;
    }

    csap->answer_prefix[0] = '\0';
    csap->num_packets      = pkt_count;
    csap->last_errno       = 0;
    CSAP_DA_UNLOCK(csap);

    INFO("CSAP %d (type '%s') recv process finished, %d pkts got",
         csap->id, csap->csap_type, pkt_count);

    free(context);
    return NULL;
}



