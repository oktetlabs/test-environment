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

#ifdef HAVE_CONFIG_H
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
#include "rcf_ch_api.h"
#include "rcf_pch.h"

#define TE_LGR_USER     "TAD CH"
#include "logger_ta.h"

#include "tad.h" 
#include "ndn.h" 


#define SEND_ANSWER(_fmt...) \
    do {                                                             \
        struct rcf_comm_connection *handle = context->rcf_handle;    \
        int r_c;                                                     \
        if (snprintf(answer_buffer + ans_len,                        \
                     ANS_BUF - ans_len, _fmt) >= ANS_BUF - ans_len)  \
        {                                                            \
            VERB("answer is truncated\n");       \
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
 * @param data          binary data to be matched.
 * @param d_len         length of data.
 * @param csap_descr    CSAP instance.
 * @param pattern_unit  'Traffic-Pattern-Unit' ASN value.
 * @param packet        parsed packet.
 *
 * @return zero on success, otherwise error code.  
 */
int
tad_tr_recv_match_with_unit(uint8_t *data, int d_len, csap_p csap_descr, 
                            asn_value_p pattern_unit, asn_value_p *packet)
{ 
    int level;
    int rc;
    struct timeval current;
    char label[20] = "pdus";

    gettimeofday(&current, NULL);

    csap_pkts data_to_check;
    csap_pkts rest_payload;


    memset(&rest_payload,  0, sizeof(csap_pkts));
    memset(&data_to_check, 0, sizeof(csap_pkts));


    if (csap_descr->command & TAD_COMMAND_RESULTS)
    {
        asn_value_p pdus = asn_init_value(ndn_generic_pdu_sequence);

        *packet = asn_init_value(ndn_raw_packet);
        asn_write_value_field(*packet, &current.tv_sec, sizeof(current.tv_sec),
                                "received.seconds");
        asn_write_value_field(*packet, &current.tv_usec,sizeof(current.tv_sec),
                                "received.micro-seconds");

        asn_write_component_value (*packet, pdus, "pdus");
        asn_free_value (pdus);
    }

    data_to_check.data = malloc(d_len);
    data_to_check.len  = d_len;
    
    memcpy (data_to_check.data, data, d_len);

    for (level = csap_descr->depth - 1; level >= 0; level --)
    {
        csap_spt_type_p csap_spt_descr; 
        const asn_value  *level_pdu = NULL; 
        asn_value_p parsed_pdu = NULL;

        if (csap_descr->command & TAD_COMMAND_RESULTS)
        {
            parsed_pdu = asn_init_value(ndn_generic_pdu);
        }

        sprintf(label + sizeof("pdus") - 1, ".%d", level);
        rc = asn_get_subvalue(pattern_unit, &level_pdu, label); 
        VERB(
            "get subval with pattern unit for label %s rc %x", label, rc);

        csap_spt_descr = find_csap_spt (csap_descr->proto[level]);

        rc = csap_spt_descr->match_cb(csap_descr->id, level, level_pdu, 
                        &data_to_check, &rest_payload, parsed_pdu); 

        VERB("match cb for lev %d returned %d", level, rc);

        if (data_to_check.free_data_cb) 
            data_to_check.free_data_cb(data_to_check.data);
        else
            free(data_to_check.data);

        memset(&data_to_check, 0, sizeof(csap_pkts));
        if (rc)
        {
            asn_free_value(*packet);
            asn_free_value(parsed_pdu);
            return rc;
        }

        if (csap_descr->command & TAD_COMMAND_RESULTS)
        {
            rc = asn_insert_indexed(*packet, parsed_pdu, 0, "pdus");
            if (rc)
            {
                ERROR("ASN error in add next pdu 0x%x\n", rc);
                return rc;
            } 
            asn_free_value(parsed_pdu);

#ifdef TALOGDEBUG 
            {
                char buf[RBUF];
                asn_sprint_value(*packet, buf, 1000, 0);
                printf ("packet: %s\n", buf);
            }
#endif
        }

        if (rest_payload.len)
        {
            memcpy(&data_to_check, &rest_payload, sizeof(csap_pkts));
            memset(&rest_payload, 0, sizeof(csap_pkts));
        }
        else break;
    }

    /* match payload */
    rc = asn_get_choice(pattern_unit, "payload", label, sizeof(label));
    if (rc == 0)
    {
        if (strcmp(label, "mask") == 0)
        {
            const uint8_t *mask = NULL;
            const uint8_t *pat = NULL;
            int mask_len;


            rc = asn_get_field_data(pattern_unit, &mask, "payload.#mask.m");

            if (rc == 0)
                rc = asn_get_field_data(pattern_unit, &pat, "payload.#mask.v"); 

            if (rc != 0 || mask == NULL || pat == NULL)
            {
                if (rc == 0) 
                    rc = ETEWRONGPTR;
                ERROR("Error getting mask of payload, %X", rc);
                return rc;
            }
            mask_len = asn_get_length(pattern_unit, "payload.#mask.m");
            if (mask_len != data_to_check.len)
            {
                F_RING("Income pld length %d != mask len %d", 
                        data_to_check.len, mask_len);
                rc = ETADNOTMATCH;
            }
            else 
            {
                int i;
                const uint8_t *d = data_to_check.data; 

                for (i = 0; i < mask_len && i < data_to_check.len; 
                        i++, mask++, pat++, d++)
                    if ((*d & *mask) != (*pat & *mask))
                    {
                        rc = ETADNOTMATCH;
                        break;
                    }
                if (rc != 0)
                    F_RING("pld byte [%d] is %02x; dont match", i, *d);
            }
        }

        if (rc != 0)
        {
            F_INFO("Error matching pattern, rc %X", rc);
            if (*packet && csap_descr->command & TAD_COMMAND_RESULTS)
                asn_free_value(*packet);
        }
    }

    if (rc == 0 && csap_descr->command & TAD_COMMAND_RESULTS)
    {
        if (data_to_check.len)
        { /* There are non-parsed payload rest */
            rc = asn_write_value_field(*packet, data_to_check.data, 
                                    data_to_check.len, "payload.#bytes");
            if (rc)
            {
                ERROR( "ASN error in add rest payload 0x%x\n", rc);
                asn_free_value(*packet);
                return rc;
            }
        } 
    }

    /* call echo callback, if it present and requested. */
    if (rc == 0)
    {
        rc = asn_get_choice(pattern_unit, "action", label, sizeof(label));

        if (rc == 0 && (strcmp(label, "echo") == 0) && 
                    csap_descr->echo_cb != NULL)
        {
            rc = csap_descr->echo_cb (csap_descr, data, d_len);
            if (rc)
                ERROR( "csap #%d, echo_cb returned %x code.", 
                            csap_descr->id, rc);
                /* Have no reason to stop receiving. */
            rc = 0;
        }
        else if (rc)
        {
            INFO("asn read action rc %x", rc);
            rc = 0;
        }
    }

    F_VERB("Packet to RX CSAP #%d matches", csap_descr->id);

    if (data_to_check.free_data_cb)
        data_to_check.free_data_cb(data_to_check.data);
    else
        free(data_to_check.data); 

    return rc; 
 }


typedef struct received_packets_queue
{
    struct received_packets_queue *next;
    struct received_packets_queue *prev;

    asn_value_p pkt;
} received_packets_queue;

/**
 * Send received packet to the test via RCF
 *
 * @param packet        ASN value with received packets; 
 * @param handle        handle of RCF connection;
 * @param answer_buffer buffer with begin of answer;
 * @param ans_len       number of first significant symbols in answer_buffer;
 *
 * @return zero on success, otherwise error code.  
 */
int
tad_report_packet( asn_value_p packet, struct rcf_comm_connection *handle, 
                          char * answer_buffer, int ans_len) 

{
    int rc;
    int attach_len;
    char * buffer;
    char * attach;

    attach_len = asn_count_txt_len(packet, 0);

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
 * @param queue_root    queue with received packets; 
 * @param handle        handle of RCF connection;
 * @param answer_buffer buffer with begin of answer;
 * @param ans_len       number of first significant symbols in answer_buffer;
 *
 * @return zero on success, otherwise error code.  
 */
int
tad_tr_recv_send_results( received_packets_queue * queue_root,
                          struct rcf_comm_connection *handle, 
                          char * answer_buffer, int ans_len) 
{
    int rc;
    received_packets_queue * pkt_qelem;

    for (pkt_qelem = queue_root->next; pkt_qelem != queue_root; 
         pkt_qelem = queue_root->next)
    {
        if(pkt_qelem->pkt)
        {
            rc = tad_report_packet(pkt_qelem->pkt, handle, 
                                    answer_buffer, ans_len);
            if (rc) return rc;

            asn_free_value(pkt_qelem->pkt);
        } 
        REMQUE(pkt_qelem);
        free(pkt_qelem);
    }

    return 0; 
}



/**
 * Generate Traffic Pattern NDS by template for trsend_recv command
 *
 * @param csap_descr    structure with CSAP parameters;
 * @param template      Traffic Template;
 * @param pattern       generated Traffic Pattern (OUT);
 *
 * @return zero on success, otherwise error code.  
 */
static int
tad_tr_sr_generate_pattern(csap_p csap_descr, asn_value_p template, 
                            asn_value_p * pattern)
{
    int rc = 0;
    int level;

    asn_value_p pattern_unit = asn_init_value(ndn_traffic_pattern_unit);
    asn_value_p pdus         = asn_init_value(ndn_generic_pdu_sequence);

    VERB("%s called for csap # %d", __FUNCTION__, csap_descr->id);

    rc = asn_write_component_value(pattern_unit, pdus, "pdus");
    if (rc) 
        return rc;
    asn_free_value(pdus);

    for (level = 0; level < csap_descr->depth; level ++)
    {
        csap_spt_type_p csap_spt_descr; 
        asn_value_p level_tmpl_pdu; 
        asn_value_p level_pattern; 
        asn_value_p gen_pattern_pdu = asn_init_value(ndn_generic_pdu);

        csap_spt_descr = find_csap_spt (csap_descr->proto[level]);

        level_tmpl_pdu = asn_read_indexed(template, level, "pdus"); 

        rc = csap_spt_descr->generate_pattern_cb
                    (csap_descr->id, level, level_tmpl_pdu, &level_pattern);

        VERB("%s, lev %d, generate pattern cb rc %X", 
                __FUNCTION__, level, rc);

        if (rc == 0) 
            rc = asn_write_component_value(gen_pattern_pdu, level_pattern, "");

        if (rc == 0) 
            rc = asn_insert_indexed (pattern_unit, gen_pattern_pdu, level, "pdus");

        asn_free_value(gen_pattern_pdu);
        asn_free_value(level_pattern);
        asn_free_value(level_tmpl_pdu);

        if (rc) 
            break;
    } 

    if (rc == 0)
    {
        *pattern = asn_init_value(ndn_traffic_pattern);
        rc = asn_insert_indexed (*pattern, pattern_unit, 0, "");
    }
    VERB("%s, returns %X", __FUNCTION__, rc);
    return rc;
}

/**
 * Prepare CSAP for operations 
 */
int 
tad_prepare_csap(csap_p csap_descr, const asn_value *pattern)
{
    int rc;
    int pu_num, i;
    char label[20];
    const asn_value *pu;
    int echo_need = 0;

    if (csap_descr->prepare_recv_cb)
    {
        rc = csap_descr->prepare_recv_cb(csap_descr);
        if (rc)
        {
            ERROR("prepare for recv failed %x", rc);
            return rc;
        }
    } 
    
    pu_num = asn_get_length(pattern, ""); 

    for (i = 0; i < pu_num; i++)
    {
        sprintf (label, "%d", i);
        rc = asn_get_subvalue(pattern, &pu, label);
        if (rc)
            break;
        rc = asn_get_choice(pu, "action", label, sizeof(label));
        if(rc == 0 && (strcmp(label, "echo") == 0))
        {
            echo_need = 1;
            break;
        }
    }

    if (csap_descr->prepare_send_cb && 
        ((csap_descr->command & TAD_OP_SEND) || echo_need))
    {
        rc = csap_descr->prepare_send_cb(csap_descr);
        if (rc)
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
 * @param arg      start argument, should be pointer to tad_task_context struct.
 *
 * @return nothing. 
 */
void * 
tad_tr_recv_thread(void * arg)
{
    struct rcf_comm_connection *handle; 
    tad_task_context *context = arg;


    int             rc = 0;
    csap_p          csap_descr;
    char            answer_buffer[ANS_BUF];  
    int             ans_len;
    asn_value_p     result = NULL;
    asn_value_p     nds = NULL; 
    int             d_len = 0;
    unsigned int    pkt_count = 0;
    char           *read_buffer;

    received_packets_queue received_packets; 

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

        if (rc)
            break;
        

        strcpy(answer_buffer, csap_descr->answer_prefix);
        ans_len = strlen (answer_buffer); 

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

    if (rc)
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
         (csap_descr->command & TAD_OP_SEND))
    { 
        do {
            csap_pkts   packets_root;
            asn_value_p pattern = NULL;
            const asn_value *pdus;

            if (csap_descr->prepare_send_cb)
            {
                rc = csap_descr->prepare_send_cb(csap_descr);
                if (rc)
                {
                    ERROR("prepare for send failed %x", rc);
                    break;
                }
            }

            rc = asn_get_subvalue (nds, &pdus, "pdus");
            if (rc)
                break;

            rc = tad_confirm_pdus(csap_descr, (asn_value *)pdus); 
            if (rc)
                break;

            rc = tad_tr_send_prepare_bin(csap_descr, nds, handle, 
                    NULL, 0, TAD_PLD_UNKNOWN, NULL, &packets_root);
            if (rc)
                break;

            rc = tad_tr_sr_generate_pattern(csap_descr, nds, &pattern);
            INFO("generate pattern rc %X", rc);
            if (rc)
                break;
            
            asn_free_value(nds);
            nds = NULL;

            d_len = csap_descr->write_read_cb
                                   (csap_descr, csap_descr->timeout,
                                    packets_root.data, packets_root.len,
                                    read_buffer, RBUF); 

            /* only single packet processed in send_recv command */
            VERB("write_read cb return d_len %d", d_len);
            gettimeofday (&csap_descr->first_pkt, NULL);
            csap_descr->last_pkt = csap_descr->first_pkt;

            if (d_len < 0)
            {
                rc = csap_descr->last_errno;
                F_ERROR(
                         "CSAP #%d internal write_read error %x", 
                         csap_descr->id, csap_descr->last_errno);
                break;
            }

            if (d_len == 0)
                break;

            nds = pattern; 

            asn_value_p pattern_unit = asn_read_indexed(pattern, 0, "");
            rc = tad_tr_recv_match_with_unit(read_buffer, d_len, csap_descr,
                                             pattern_unit, &result); 

            VERB("match_with_unit returned %d", rc);

            if (rc)
            {
                if (rc == ETADNOTMATCH)
                    rc = 0;
                break;
            }

            if (csap_descr->command & TAD_COMMAND_RESULTS)
            { 
                received_packets_queue * new_qelem = 
                            malloc(sizeof(received_packets_queue));
                new_qelem->pkt = result;
                INSQUE(new_qelem, received_packets.prev); 
                VERB("insert packet in queue");
            }
            csap_descr->state |= TAD_STATE_COMPLETE;
            csap_descr->total_bytes += d_len;
            asn_free_value(pattern_unit);

        } while (0); 
    } /* finish of 'trsend_recv' special actions */

    if (rc) 
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

        asn_value_p pattern_unit;

        CSAP_DA_LOCK(csap_descr);
        if (csap_descr->command & TAD_COMMAND_STOP)
        {
            CSAP_DA_UNLOCK(csap_descr);
            VERB("trrecv_stop flag detected"); 
            break;
        }

        if ((csap_descr->command & TAD_COMMAND_WAIT) || 
            (csap_descr->command & TAD_COMMAND_GET))
        {
            if (csap_descr->command & TAD_COMMAND_WAIT)
            {
                csap_descr->state |= TAD_STATE_FOREGROUND;
                csap_descr->command &= ~TAD_COMMAND_WAIT;
            }

            if ((rc = tad_tr_recv_send_results (&received_packets, handle, 
                                                answer_buffer, ans_len)))
            {
                ERROR(
                       "send results for 'trrecv_get' failed with code 0x%x\n", 
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

            if (csap_descr->command & TAD_COMMAND_GET)
            {
                csap_descr->command &= ~TAD_COMMAND_GET;
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
                    VERB(
                            "CSAP %d status complete by timeout, " 
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
                pattern_unit = asn_read_indexed(nds, unit, "");
                rc = tad_tr_recv_match_with_unit (read_buffer, d_len, csap_descr,
                                                  pattern_unit, &result); 
                F_VERB("Match pkt return %x, unit %d", rc, unit);
                asn_free_value(pattern_unit);
                switch (rc)
                {
                    case 0: /* received data matches to this pattern unit */
                        csap_descr->last_pkt = pkt_caught;
                        if (!pkt_count)
                            csap_descr->first_pkt = csap_descr->last_pkt;
                        unit = num_pattern_units; /* to break from 'for' also. */
                        csap_descr->total_bytes += d_len;
                        pkt_count++;
                        F_VERB("Match pkt, d_len %d, total %d, pkts %u",
                               d_len, csap_descr->total_bytes, pkt_count);
                        break;

                    case ETADLESSDATA: /* @todo correct processing of 
                                          this case should be done */
                        rc = 0;
                    case ETADNOTMATCH:
                        continue;

                    default: 
                        ERROR(
                           "Match with patter-unit failed with code: 0x%x\n", rc);
                        break;
                }
                if (rc)
                    break;
            }

            if (rc == ETADNOTMATCH)
            {
                rc = 0;
                continue;
            }

            /* Here packet is successfully received, parsed and matched */

            if ((rc == 0) && (csap_descr->command & TAD_COMMAND_RESULTS))
            { 
                F_VERB("result wanted, will be sent\n");
                if (csap_descr->state & TAD_STATE_FOREGROUND)
                {
                    F_VERB("in foreground mode\n");
                    rc = tad_report_packet(result, handle, answer_buffer, ans_len);
                }
                else
                {
                    F_VERB("Put packet into the queue\n");
                    received_packets_queue * new_qelem = 
                                malloc(sizeof(received_packets_queue));
                    new_qelem->pkt = result;
                    INSQUE(new_qelem, received_packets.prev);
                }
            } 

            if (rc) 
            {
                csap_descr->last_errno = rc;
                csap_descr->state |= TAD_STATE_COMPLETE;
                rc = 0;
                continue;
            }

            if (csap_descr->num_packets)
            {
                F_VERB("check for num_pkts.");
                if (csap_descr->num_packets <= pkt_count)
                {
                    csap_descr->state |= TAD_STATE_COMPLETE;
                    VERB("CSAP %d status complete",
                                 csap_descr->id); 
                }
            }
        } /* if (need packets) */
    } /* while (background mode or not completed) */

    /* either stop got or foreground operation completed */ 

    rc = tad_tr_recv_send_results(&received_packets, handle, 
                                      answer_buffer, ans_len);
    if (rc)
    {
        ERROR(
                   "trrecv thread: send results failed with code 0x%x\n", rc);
        if (csap_descr->last_errno == 0)
            csap_descr->last_errno = rc;
        rc = 0;
    } 

    if (csap_descr->release_cb)
        csap_descr->release_cb(csap_descr);

    memset (&csap_descr->wait_for, 0, sizeof (struct timeval)); 
    asn_free_value(nds);
    free(read_buffer);

    CSAP_DA_LOCK(csap_descr);
    csap_descr->command = 0;
    csap_descr->state   = 0;
    SEND_ANSWER("%d %u", 
                TE_RC(TE_TAD_CSAP, csap_descr->last_errno), pkt_count); 

    csap_descr->answer_prefix[0] = '\0';
    csap_descr->num_packets      = 0;
    csap_descr->last_errno       = 0;
    CSAP_DA_UNLOCK(csap_descr);

    F_VERB("CSAP %d recv process finished", csap_descr->id);

    free(context);
    return NULL;
}



