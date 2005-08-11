/** @file
 * @brief Test Environment: 
 *
 * Traffic Application Domain Command Handler
 * Receive module. 
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
 * $Id$
 */


#include <assert.h>

#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <unistd.h>

#include "comm_agent.h"
#include "rcf_ch_api.h"
#include "rcf_pch.h"


#include "tad_csap_inst.h"
#include "tad_csap_support.h"
#include "tad_utils.h"
#include "ndn.h" 

#define TE_LGR_USER     "TAD Recv"

#include "logger_ta.h"

#define SEND_ANSWER(_fmt...) \
    do {                                                             \
        struct rcf_comm_connection *handle = context->rcf_handle;    \
        int r_c;                                                     \
                                                                     \
        if (snprintf(answer_buffer + ans_len,                        \
                     ANS_BUF - ans_len, _fmt) >= ANS_BUF - ans_len)  \
        {                                                            \
            VERB("answer is truncated\n");                           \
        }                                                            \
        rcf_ch_lock();                                               \
        r_c = rcf_comm_agent_reply(handle, answer_buffer,            \
                                   strlen(answer_buffer) + 1);       \
        rcf_ch_unlock();                                             \
    } while (0)


#define ANS_BUF 100
#define RBUF 0x4000

/**
 * Try match binary data with Traffic-Pattern-Unit and prepare ASN value 
 * with packet if it satisfies to the pattern unit.
 *
 * @param data          binary data to be matched
 * @param d_len         length of data
 * @param csap_descr    CSAP instance
 * @param pattern_unit  ASN value of 'Traffic-Pattern-Unit' type
 * @param packet        parsed packet (OUT)
 *
 * @return zero on success, otherwise error code.  
 */
int
tad_tr_recv_match_with_unit(uint8_t *data, int d_len, csap_p csap_descr, 
                            const asn_value *pattern_unit,
                            asn_value_p *packet)
{ 
    int  level;
    int  rc;
    char label[20] = "pdus";

    struct timeval current;

    gettimeofday(&current, NULL);

    csap_pkts data_to_check;
    csap_pkts rest_payload;


    memset(&rest_payload,  0, sizeof(rest_payload));
    memset(&data_to_check, 0, sizeof(data_to_check));


    *packet = NULL;
    if (csap_descr->state & TAD_STATE_RESULTS)
    {
        asn_value_p pdus = asn_init_value(ndn_generic_pdu_sequence);

        *packet = asn_init_value(ndn_raw_packet);
        asn_write_value_field(*packet, &current.tv_sec,
                              sizeof(current.tv_sec),
                              "received.seconds");
        asn_write_value_field(*packet, &current.tv_usec,
                              sizeof(current.tv_sec),
                              "received.micro-seconds");

        asn_write_component_value(*packet, pdus, "pdus");
        asn_free_value(pdus);
    }

    data_to_check.data = malloc(d_len);
    data_to_check.len  = d_len;
    
    memcpy(data_to_check.data, data, d_len);

    for (level = csap_descr->depth - 1; level >= 0; level --)
    {
        csap_spt_type_p csap_spt_descr; 
        const asn_value  *level_pdu = NULL; 
        asn_value_p parsed_pdu = NULL;

        if (csap_descr->state & TAD_STATE_RESULTS)
        {
            parsed_pdu = asn_init_value(ndn_generic_pdu);
        }

        sprintf(label + sizeof("pdus") - 1, ".%d", level);
        rc = asn_get_subvalue(pattern_unit, &level_pdu, label); 
        VERB("get subval with pattern unit for label %s rc 0x%x",
             label, rc);

        csap_spt_descr = csap_descr->layers[level].proto_support;

        rc = csap_spt_descr->match_cb(csap_descr->id, level, level_pdu, 
                                      &data_to_check, &rest_payload,
                                      parsed_pdu); 

        VERB("match cb 0x%x for lev %d returned 0x%x",
             csap_spt_descr->match_cb, level, rc);

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

        if (csap_descr->state & TAD_STATE_RESULTS)
        {
            rc = asn_insert_indexed(*packet, parsed_pdu, 0, "pdus");
            asn_free_value(parsed_pdu);
            if (rc != 0)
            {
                ERROR("ASN error in add next pdu 0x%x\n", rc);
                asn_free_value(*packet);
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
                ERROR("%s(): get mask failed %X", __FUNCTION__, rc);
                asn_free_value(*packet);
                return rc;
            }
            rc = ndn_match_mask(mask_pat,
                                data_to_check.data, data_to_check.len);
            VERB("CSAP %d, rc from ndn_match_mask %X",
                 csap_descr->id, rc);
        }

        if (rc != 0)
        {
            F_VERB("CSAP %d, Error matching pattern, rc %X, COUNT %d",
                   csap_descr->id, rc);
            asn_free_value(*packet);
            *packet = NULL;
        }
    }
    else if (rc == TE_EASNINCOMPLVAL)
        rc = 0;

    if (rc == 0 && csap_descr->state & TAD_STATE_RESULTS)
    {
        if (data_to_check.len)
        { /* There are non-parsed payload rest */
            rc = asn_write_value_field(*packet, data_to_check.data, 
                                       data_to_check.len, "payload.#bytes");
            if (rc)
            {
                ERROR( "ASN error in add rest payload 0x%x\n", rc);
                asn_free_value(*packet);
                *packet = NULL;
                return rc;
            }
        } 
    }

    /* process action, if it present. */
    if (rc == 0) do
    { 
        const asn_value *action_val;
        const asn_value *action_ch_val;
        asn_tag_class    t_class;
        uint16_t         t_val;

        rc = asn_get_child_value(pattern_unit, &action_val,
                                 PRIVATE, NDN_PU_ACTION);
        if (rc != 0)
        {
            INFO("asn read action rc %x", rc);
            rc = 0;
            break;
        }
        asn_get_choice_value(action_val, &action_ch_val,
                             &t_class, &t_val);
        switch (t_val)
        {
            case NDN_ACT_ECHO:
                if (csap_descr->echo_cb != NULL)
                {
                    rc = csap_descr->echo_cb(csap_descr, data, d_len);
                    if (rc)
                        ERROR("csap #%d, echo_cb returned %x code.", 
                              csap_descr->id, rc);
                        /* Have no reason to stop receiving. */
                    rc = 0;
                }
                break;

            case NDN_ACT_FUNCTION: 
            {
                tad_processing_pkt_method method_addr;

                char  buffer[200] = {0,};
                char *usr_place;
                size_t buf_len = sizeof(buffer);

                rc = asn_read_value_field(action_ch_val, buffer,
                                          &buf_len, "");
                if (rc != 0)
                    ERROR("csap #%d, ASN read value error %X", 
                          csap_descr->id, rc); 
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
                        rc = method_addr(csap_descr, usr_place,
                                         data, d_len);
                        if (rc != 0)
                            WARN("rc from user method %X", rc);
                        rc = 0;
                    }
                }
            }
            break;

            case NDN_ACT_FORWARD:
            {
                int32_t target_csap;
                csap_p  target_csap_descr;

                asn_read_int32(action_ch_val, &target_csap, "");
                if ((target_csap_descr = csap_find(target_csap)) != NULL)
                {
                    int b = target_csap_descr->write_cb(target_csap_descr,
                                                        data, d_len);
                    RING("action forward processed, %d sent", b);
                } 
            }
            break; 
        }
    } while (0);

    F_VERB("Packet to RX CSAP #%d matches", csap_descr->id);

    if (data_to_check.free_data_cb)
        data_to_check.free_data_cb(data_to_check.data);
    else
        free(data_to_check.data); 

    if ((rc != 0) && (csap_descr->state & TAD_STATE_RESULTS))
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
 * @param handle        handle of RCF connection;
 * @param answer_buffer buffer with begin of answer;
 * @param ans_len       index of first significant symbols in answer_buffer;
 *
 * @return zero on success, otherwise error code.  
 */
int
tad_report_packet( asn_value_p packet, struct rcf_comm_connection *handle, 
                          char *answer_buffer, int ans_len) 

{
    int   rc;
    int   attach_len;
    char *buffer;
    char *attach;

    attach_len = asn_count_txt_len(packet, 0);

    /* 
     * 20 is upper estimation for "attach" and decimal presentation
     * of attach length 
     */
    if ((buffer = calloc(1, ans_len + 20 + attach_len)) == NULL)
        return ENOMEM;

    memcpy(buffer, answer_buffer, ans_len);
    rc = sprintf(buffer + ans_len, " attach %d", attach_len);

    attach = buffer + strlen(buffer) + 1; 
    if (asn_sprint_value(packet, attach, attach_len + 1, 0) > attach_len)
    {
        ERROR("asn_sprint_value returns greater than expected!\n");
    } 

    VERB("report about packet to test, attach len: %d", attach_len);

    rcf_ch_lock();
    rc = rcf_comm_agent_reply(handle, buffer,
                              strlen(buffer) + 1 + attach_len);
    rcf_ch_unlock(); 
    free(buffer);
    return rc;
}

/**
 * Send received packets in queue to the test via RCF and clear queue. 
 *
 * @param queue_root    queue with received packets
 * @param handle        handle of RCF connection
 * @param answer_buffer buffer with begin of answer
 * @param ans_len       index of first significant symbols in answer_buffer
 *
 * @return zero on success, otherwise error code
 */
int
tad_tr_recv_send_results(received_packets_queue_t *queue_root,
                         struct rcf_comm_connection *handle, 
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
            rc = tad_report_packet(pkt_qelem->pkt, handle, 
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



/**
 * Generate Traffic Pattern NDS by template for trsend_recv command
 *
 * @param csap_descr    structure with CSAP parameters
 * @param template      traffic template
 * @param pattern       generated Traffic Pattern (OUT)
 *
 * @return zero on success, otherwise error code.  
 */
static int
tad_tr_sr_generate_pattern(csap_p csap_descr, asn_value_p template, 
                           asn_value_p *pattern)
{
    int rc = 0;
    int level;

    asn_value_p pattern_unit = asn_init_value(ndn_traffic_pattern_unit);
    asn_value_p pdus         = asn_init_value(ndn_generic_pdu_sequence);

    VERB("%s called for csap # %d", __FUNCTION__, csap_descr->id);

    rc = asn_write_component_value(pattern_unit, pdus, "pdus");
    if (rc != 0) 
        return rc;
    asn_free_value(pdus);

    for (level = 0; level < csap_descr->depth; level ++)
    {
        csap_spt_type_p csap_spt_descr; 

        asn_value_p level_tmpl_pdu; 
        asn_value_p level_pattern; 
        asn_value_p gen_pattern_pdu = asn_init_value(ndn_generic_pdu);

        csap_spt_descr = csap_descr->layers[level].proto_support;

        level_tmpl_pdu = asn_read_indexed(template, level, "pdus"); 

        rc = csap_spt_descr->generate_pattern_cb(csap_descr->id, level,
                                                 level_tmpl_pdu, 
                                                 &level_pattern);

        VERB("%s, lev %d, generate pattern cb rc %X", 
                __FUNCTION__, level, rc);

        if (rc == 0) 
            rc = asn_write_component_value(gen_pattern_pdu, 
                                           level_pattern, "");

        if (rc == 0) 
            rc = asn_insert_indexed(pattern_unit, gen_pattern_pdu, 
                                    level, "pdus");

        asn_free_value(gen_pattern_pdu);
        asn_free_value(level_pattern);
        asn_free_value(level_tmpl_pdu);

        if (rc != 0) 
            break;
    } 

    if (rc == 0)
    {
        *pattern = asn_init_value(ndn_traffic_pattern);
        rc = asn_insert_indexed(*pattern, pattern_unit, 0, "");
    }
    VERB("%s, returns %X", __FUNCTION__, rc);
    asn_free_value(pattern_unit);
    return rc;
}

/**
 * Prepare CSAP for operations 
 *
 * @param csap_descr    structure with CSAP parameters
 * @param pattern       patter of operation, need for check actions
 *
 * @return zero on success, otherwise error code
 */
int 
tad_prepare_csap(csap_p csap_descr, const asn_value *pattern)
{
    int  rc;
    int  pu_num, i;
    int  echo_need = 0;
    char label[20];

    const asn_value *pu;

    if (csap_descr->prepare_recv_cb)
    {
        rc = csap_descr->prepare_recv_cb(csap_descr);
        if (rc != 0)
        {
            ERROR("prepare for recv failed %x", rc);
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

    if (csap_descr->prepare_send_cb && 
        ((csap_descr->state & TAD_STATE_SEND) || echo_need))
    {
        rc = csap_descr->prepare_send_cb(csap_descr);
        if (rc != 0)
        {
            ERROR("prepare for recv failed %x", rc);
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
tad_tr_recv_thread(void *arg)
{
    struct rcf_comm_connection *handle; 

    tad_task_context *context = arg; 

    int           rc = 0;
    csap_p        csap_descr;
    char          answer_buffer[ANS_BUF];  
    int           ans_len = 0;
    int           d_len = 0;
    unsigned int  pkt_count = 0;
    char         *read_buffer = NULL;

    asn_value_p   result = NULL;
    asn_value_p   nds = NULL; 

    received_packets_queue_t received_packets; 

    received_packets.pkt = NULL;
    received_packets.next = &received_packets;
    received_packets.prev = &received_packets; 

    if (arg == NULL)
    {
        ERROR("trrecv thread start point: null argument! exit");
        return NULL;
    }
    handle = context->rcf_handle;


    csap_descr = context->csap;
    nds        = context->nds; 
    context->nds = NULL;

    VERB("START %s() CSAP %d, timeout %u", __FUNCTION__,
         csap_descr->id, csap_descr->timeout);

    if (csap_descr == NULL)
    {
        ERROR("trrecv thread start point: null csap! exit.");
        asn_free_value(nds);
        return NULL;
    }
    asn_save_to_file(nds, "/tmp/nds.asn");

    do /* symbolic do {} while(0) for traffic operation init block */
    {
        rc = tad_prepare_csap(csap_descr, nds);

        if (rc != 0)
            break; 

        strcpy(answer_buffer, csap_descr->answer_prefix);
        ans_len = strlen(answer_buffer); 

        if ((read_buffer = malloc(RBUF)) == NULL)
        {
            rc = ENOMEM; 
            break;
        }

    } while (0); 

    if (!(csap_descr->state & TAD_STATE_FOREGROUND))
    {
        SEND_ANSWER("0 0"); 
    } 
    VERB("trrecv thread for CSAP %d started", csap_descr->id);

    if (rc != 0)
    {
        csap_descr->last_errno = rc;
        csap_descr->state |= TAD_STATE_COMPLETE;
        csap_descr->state |= TAD_STATE_FOREGROUND;
        rc = 0;
    }

    /*
     * trsend_recv processing. 
     */
    if (!(csap_descr->state   & TAD_STATE_COMPLETE) && 
         (csap_descr->command == TAD_OP_SEND_RECV))
    { 
        do {
            const asn_value *pattern_unit;
            csap_pkts   packets_root;

            asn_value_p pattern = NULL;

            const asn_value *pdus;

            if (csap_descr->prepare_send_cb)
            {
                rc = csap_descr->prepare_send_cb(csap_descr);
                if (rc != 0)
                {
                    ERROR("prepare for send failed %x", rc);
                    break;
                }
            }

            rc = asn_get_subvalue(nds, &pdus, "pdus");
            if (rc != 0)
                break;

            rc = tad_confirm_pdus(csap_descr, (asn_value *)pdus); 
            if (rc != 0)
                break;

            rc = tad_tr_send_prepare_bin(csap_descr, nds, 
                                         NULL, 0, NULL, &packets_root);
            if (rc != 0)
                break;

            rc = tad_tr_sr_generate_pattern(csap_descr, nds, &pattern);
            INFO("generate pattern rc %X", rc);
            if (rc != 0)
                break;
            
            asn_free_value(nds);
            nds = pattern; 

            d_len = csap_descr->write_read_cb(csap_descr, 
                                              csap_descr->timeout,
                                              packets_root.data, 
                                              packets_root.len,
                                              read_buffer, RBUF); 

            /* only single packet processed in send_recv command */
            VERB("write_read cb return d_len %d", d_len);
            gettimeofday(&csap_descr->first_pkt, NULL);
            csap_descr->last_pkt = csap_descr->first_pkt;

            if (d_len < 0)
            {
                rc = csap_descr->last_errno;
                F_ERROR("CSAP #%d internal write_read error %x", 
                         csap_descr->id, csap_descr->last_errno);
                break;
            }

            if (d_len == 0)
                break;

            asn_get_indexed(pattern, &pattern_unit, 0);
            rc = tad_tr_recv_match_with_unit(read_buffer, d_len, csap_descr,
                                             pattern_unit, &result); 

            VERB("match_with_unit returned 0x%X", rc);

            if (rc != 0)
            {
                if (rc == TE_ETADNOTMATCH)
                    rc = 0;
                break;
            }

            if (csap_descr->state & TAD_STATE_RESULTS)
            { 
                received_packets_queue_t *new_qelem = 
                            malloc(sizeof(*new_qelem));
                new_qelem->pkt = result;
                INSQUE(new_qelem, received_packets.prev); 
                VERB("insert packet in queue");
            }
            csap_descr->state |= TAD_STATE_COMPLETE;
            csap_descr->total_bytes += d_len;
        } while (0); 
    } /* finish of 'trsend_recv' special actions */

    if (rc != 0) 
    {
        /* non-zero rc may occure only from trsend_recv special block,
         * this is in foreground mode. */
        csap_descr->state |= TAD_STATE_COMPLETE;
        csap_descr->last_errno = rc;
        F_ERROR("generate binary data error: 0x%x", rc); 
        rc = 0;
    }

    /* Loop for receiving/matching packages and wait for STOP/GET/WAIT. 
     * This loop should be braked only if 'foreground' operation complete 
     * of "STOP" received for 'background' operation. */ 
    while(!(csap_descr->state & TAD_STATE_COMPLETE)  || 
          !(csap_descr->state & TAD_STATE_FOREGROUND)  )
    {
        int num_pattern_units;
        int unit;

        const asn_value *pattern_unit;

        CSAP_DA_LOCK(csap_descr);
        if (csap_descr->command == TAD_OP_STOP)
        {
            strcpy(answer_buffer, csap_descr->answer_prefix);
            ans_len = strlen(answer_buffer); 

            CSAP_DA_UNLOCK(csap_descr);
            VERB("trrecv_stop flag detected"); 
            break;
        }

        if ((csap_descr->command == TAD_OP_WAIT) || 
            (csap_descr->command == TAD_OP_GET))
        { 
            strcpy(answer_buffer, csap_descr->answer_prefix);
            ans_len = strlen(answer_buffer); 

            if (csap_descr->command == TAD_OP_WAIT)
            {
                csap_descr->state |= TAD_STATE_FOREGROUND;
                csap_descr->command = TAD_OP_RECV;
                F_VERB("%s: wait flag encountered. %d packets got",
                       __FUNCTION__, pkt_count);
            }

            rc = tad_tr_recv_send_results(&received_packets, handle, 
                                          answer_buffer, ans_len);
            if (rc != 0) 
            {
                ERROR("send 'trrecv_get' results failed with rc 0x%X\n",
                      rc);
                /**
                 * @todo fix it. 
                 * Really, I don't know yet what to do if this failed. 
                 * If there is not bugs ;) this failure means that happens 
                 * something awful, such as lost connection with RCF. 
                 * But some bug seems to be more probable variant, so 
                 * process it as other errors: try to transmit via RCF
                 **/
                csap_descr->last_errno = rc;
                csap_descr->state |= TAD_STATE_COMPLETE;
                rc = 0;
            }

            if (csap_descr->command == TAD_OP_GET)
            {
                csap_descr->command = TAD_OP_RECV;
                SEND_ANSWER("0 %u", pkt_count); /* trrecv_get is finished */
                VERB("trrecv_get #%d OK, pkts: %u, state %x",
                     csap_descr->id, pkt_count, (int)csap_descr->state);
            }
        }
        CSAP_DA_UNLOCK(csap_descr);

        if (!(csap_descr->state & TAD_STATE_COMPLETE))
        {
            struct timeval pkt_caught;

            if (csap_descr->wait_for.tv_sec)
            {
                struct timeval  current;
                int             s_diff;

                gettimeofday(&current, NULL);
                s_diff = current.tv_sec - csap_descr->wait_for.tv_sec; 

                if (s_diff > 0 || 
                    (s_diff == 0 &&
                     current.tv_usec > csap_descr->wait_for.tv_usec))
                {
                    csap_descr->last_errno = ETIMEDOUT; 
                    csap_descr->state |= TAD_STATE_COMPLETE;
                    VERB("CSAP %d status complete by timeout, " 
                         "wait for: %u.%u, current: %u.%u", 
                         csap_descr->id, 
                         (uint32_t)csap_descr->wait_for.tv_sec, 
                         (uint32_t)csap_descr->wait_for.tv_usec,
                         (uint32_t)current.tv_sec, 
                         (uint32_t)current.tv_usec);
                    continue;
                }
            }

            d_len = csap_descr->read_cb(csap_descr, csap_descr->timeout, 
                                        read_buffer, RBUF); 
            gettimeofday(&pkt_caught, NULL);

            if (d_len == 0)
                continue;

            if (d_len < 0)
            {
                csap_descr->state |= TAD_STATE_COMPLETE;
                ERROR("CSAP read callback failed; rc: 0x%x\n", rc);
                continue;
            } 
            
            num_pattern_units = asn_get_length(nds, ""); 

            for (unit = 0; unit < num_pattern_units; unit ++)
            {
                rc = asn_get_indexed(nds, &pattern_unit, unit);
                if (rc != 0)
                {
                    WARN("Get pattern unit fails %X", rc);
                    break;
                }
                rc = tad_tr_recv_match_with_unit(read_buffer, d_len, 
                                                 csap_descr,
                                                 pattern_unit, &result); 
                F_INFO("CSAP %d, Match pkt return %x, unit %d", 
                       csap_descr->id, rc, unit);
                switch (rc)
                {
                    case 0: /* received data matches to this pattern unit */
                        csap_descr->last_pkt = pkt_caught;
                        if (!pkt_count)
                            csap_descr->first_pkt = csap_descr->last_pkt;
                        unit = num_pattern_units; /* to break from 'for' */
                        csap_descr->total_bytes += d_len;
                        pkt_count++;
                        F_VERB("Match pkt, d_len %d, total %d, pkts %u",
                               d_len, csap_descr->total_bytes, pkt_count);
                        break;

                    case TE_ETADLESSDATA: /* @todo fragmentation */
                        rc = 0;
                    case TE_ETADNOTMATCH:
                        continue;

                    default: 
                        ERROR("Match with patter-unit failed "
                              "with code: 0x%x\n", rc);
                        break;
                }
                if (rc != 0)
                    break;
            }

            if (rc == TE_ETADNOTMATCH)
            {
                rc = 0;
                continue;
            }

            /* Here packet is successfully received, parsed and matched */

            if ((rc == 0) && (csap_descr->state & TAD_STATE_RESULTS))
            { 
                if (csap_descr->state & TAD_STATE_FOREGROUND)
                {
                    F_VERB("in foreground mode\n");
                    rc = tad_report_packet(result, handle, answer_buffer,
                                           ans_len);
                }
                else
                {
                    F_VERB("Put packet into the queue\n");
                    received_packets_queue_t *new_qelem = 
                                            malloc(sizeof(*new_qelem));
                    new_qelem->pkt = result;
                    INSQUE(new_qelem, received_packets.prev);
                }
            } 

            if (rc != 0) 
            {
                csap_descr->last_errno = rc;
                csap_descr->state |= TAD_STATE_COMPLETE;
                rc = 0;
                continue;
            }

            if (csap_descr->num_packets)
            {
                F_VERB("%s(): check for num_pkts, want %d, got %d",
                       __FUNCTION__, csap_descr->num_packets, pkt_count);
                if (csap_descr->num_packets <= pkt_count)
                {
                    csap_descr->state |= TAD_STATE_COMPLETE;
                    INFO("%s(): CSAP %d status complete", 
                         __FUNCTION__, csap_descr->id); 
                }
            }
        } /* if (need packets) */
    } /* while (background mode or not completed) */

    /* either stop got or foreground operation completed */ 

    rc = tad_tr_recv_send_results(&received_packets, handle, 
                                  answer_buffer, ans_len);
    if (rc != 0)
    {
        ERROR("trrecv thread: send results failed with code 0x%x\n", rc);
        if (csap_descr->last_errno == 0)
            csap_descr->last_errno = rc;
        rc = 0;
    } 

    /* 
     * Release resources, this should be done before clearing 
     * CSAP state fields, becouse zero state during release means 
     * CSAP destroy procedure. 
     */
    if (csap_descr->release_cb)
        csap_descr->release_cb(csap_descr);

    memset(&csap_descr->wait_for, 0, sizeof(csap_descr->wait_for)); 
    asn_free_value(nds);
    free(read_buffer);

    CSAP_DA_LOCK(csap_descr);
    csap_descr->command = TAD_OP_IDLE;
    csap_descr->state   = 0;
    SEND_ANSWER("%d %u", TE_RC(TE_TAD_CSAP, csap_descr->last_errno), 
                pkt_count);

    csap_descr->answer_prefix[0] = '\0';
    csap_descr->num_packets      = pkt_count;
    csap_descr->last_errno       = 0;
    CSAP_DA_UNLOCK(csap_descr);

    F_RING("CSAP %d recv process finished, %d pkts got",
           csap_descr->id, pkt_count);

    free(context);
    return NULL;
}



