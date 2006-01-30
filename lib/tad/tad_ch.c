/** @file
 * @brief TAD Command Handler
 *
 * Traffic Application Domain Command Handler.
 * Definition of routines provided for Portable Command Handler.
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

#define TE_LGR_USER     "TAD CH"

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

#if HAVE_UNISTD_H
#include <unistd.h> 
#endif

#include <string.h>
#include <pthread.h>

#include "logger_api.h"
#include "logger_ta_fast.h"
#include "comm_agent.h"
#include "rcf_ch_api.h"
#include "rcf_pch.h"
#include "te_defs.h"

#ifndef DUMMY_TAD

#include "ndn.h" 

#include "csap_id.h"
#include "tad_csap_inst.h"
#include "tad_csap_support.h"
#include "tad_utils.h"
#include "tad_send_recv.h"


static struct timeval tv_zero = {0,0};

#define SEND_ANSWER(_fmt...) \
    do {                                                                   \
        int r_c;                                                           \
                                                                           \
        if ((size_t)snprintf(cbuf + answer_plen, buflen - answer_plen,     \
                             _fmt) >= (buflen - answer_plen))              \
        {                                                                  \
            VERB("answer is truncated\n");                                 \
        }                                                                  \
        rcf_ch_lock();                                                     \
        r_c = rcf_comm_agent_reply(rcfc, cbuf, strlen(cbuf) + 1);          \
        rcf_ch_unlock();                                                   \
        if (r_c)                                                           \
            fprintf(stderr, "rc from rcf_comm_agent_reply: 0x%X\n", r_c);  \
    } while (0)

#endif

static int is_initialized = 0;

#ifdef WITH_ATM
extern te_errno csap_support_atm_register(void);
#endif

#ifdef WITH_ETH
extern te_errno csap_support_eth_register(void);
#endif

#ifdef WITH_ARP
extern te_errno csap_support_arp_register(void);
#endif

#ifdef WITH_IPSTACK
extern te_errno csap_support_ipstack_register(void);
#endif

#ifdef WITH_FILE
extern te_errno csap_support_file_register(void);
#endif

#ifdef WITH_SNMP
extern te_errno csap_support_snmp_register(void);
#endif

#ifdef WITH_DHCP
extern te_errno csap_support_dhcp_register(void);
#endif

#ifdef WITH_BRIDGE
extern te_errno csap_support_bridge_register(void);
#endif

#ifdef WITH_CLI
extern te_errno csap_support_cli_register(void);
#endif

#ifdef WITH_PCAP
extern te_errno csap_support_pcap_register(void);
#endif

#ifdef WITH_ISCSI
extern te_errno csap_support_iscsi_register(void);
#endif

#ifdef WITH_SOCKET
extern te_errno csap_support_socket_register(void);
#endif


static void
check_init(void)
{
    if (is_initialized) return;

#ifndef DUMMY_TAD
    csap_id_init();
    csap_spt_init();
#ifdef WITH_ATM
    csap_support_atm_register();
#endif
#ifdef WITH_ETH
    csap_support_eth_register();
#endif 
#ifdef WITH_ARP
    csap_support_arp_register();
#endif
#ifdef WITH_IPSTACK
    csap_support_ipstack_register();
#endif
#ifdef WITH_FILE
    csap_support_file_register();
#endif
#ifdef WITH_SNMP
    csap_support_snmp_register();
#endif
#ifdef WITH_CLI
    csap_support_cli_register();
#endif
#ifdef WITH_DHCP
    csap_support_dhcp_register();
#endif
#ifdef WITH_BRIDGE
    csap_support_bridge_register();
#endif
#ifdef WITH_PCAP
    csap_support_pcap_register();
#endif
#ifdef WITH_ISCSI
    csap_support_iscsi_register();
#endif
#ifdef WITH_SOCKET
    csap_support_socket_register();
#endif
#endif /* DUMMY_TAD */

    is_initialized = 1;
}

void
tad_ch_init(void)
{
    check_init();
}

#ifndef DUMMY_TAD 

/**
 * Safe compare two strings. Almost equivalent to standard "strcmp", but
 * correct work if one or both arguments are NULL. 
 * If both arguments are empty strings or NULL (in any combination), they 
 * are considered as equal and zero is returned. 
 * 
 *
 * @param l        first string
 * @param r        second string
 *
 * @return same value as strcmp. 
 */
static inline int
strcmp_imp(const char *l, const char *r)
{
    if (l == NULL)
    {
        if ((r == NULL) || (*r == '\0'))
            return 0;
        else 
            return -1;
    } 
    if (r == NULL)
    {
        if (*l == '\0')
            return 0;
        else
            return 1;
    }

    return strcmp(l, r);
}

#endif /* DUMMY_TAD */

/* See description in rcf_ch_api.h */
int
rcf_ch_csap_create(struct rcf_comm_connection *rcfc,
                   char *cbuf, size_t buflen, size_t answer_plen,
                   const uint8_t *ba, size_t cmdlen,
                   const char *stack, const char *params)
{
#ifdef DUMMY_TAD 
    UNUSED(rcfc);
    UNUSED(cbuf);
    UNUSED(buflen);
    UNUSED(answer_plen);
    UNUSED(ba);
    UNUSED(cmdlen);

    VERB("CSAP create: stack <%s> params <%s>\n", stack, params);
    return -1;
#else
    csap_p          new_csap;
    asn_value_p     csap_nds;

    csap_handle_t   new_csap_id; 
    unsigned int    layer;
    int             syms; 
    te_errno        rc; 

    UNUSED(cmdlen);
    UNUSED(params);


    check_init();

    new_csap_id = csap_create(stack);
    if (new_csap_id == CSAP_INVALID_HANDLE)
    {
        rc = TE_RC(TE_TAD_CH, errno);
        SEND_ANSWER("%d CSAP creation internal error", rc);
        ERROR("CSAP '%s' creation internal error %r", stack, rc);
        return 0;
    }

    INFO("CSAP '%s' created, new id: %d", stack, new_csap_id);

    if (ba == NULL)
    {
        SEND_ANSWER("%d missing attached NDS", TE_ETADMISSNDS);
        ERROR("Missing attached NDS");
        csap_destroy(new_csap_id);
        return 0;
    }

    new_csap = csap_find(new_csap_id); 
    assert(new_csap != NULL);

    rc = asn_parse_value_text(ba, ndn_csap_spec, &csap_nds, &syms);
    if (rc != 0)
    {
        ERROR("CSAP NDS parse error sym=%d: %r", syms, rc);
        SEND_ANSWER("%d", TE_RC(TE_TAD_CH, rc));
        csap_destroy(new_csap_id); 
        return 0;
    }
    assert(csap_nds != NULL);

    /* FIXME: Where is it used? */
    syms = asn_get_length(csap_nds, "");
    INFO("Length of PDU list in NDS: %d, csap depth %u", 
          syms, new_csap->depth); 

    /*
     * Init csap layers from lower to upper 
     */
    for (layer = new_csap->depth; layer-- > 0; )
    {
        asn_value       *nds_pdu;
        csap_spt_type_p  csap_spt_descr; 

        nds_pdu = asn_read_indexed(csap_nds, layer, "");
        if (nds_pdu == NULL)
        {
            ERROR("Copy %u layer PDU from NDS ASN value failed", layer);
            rc = TE_EASNGENERAL;
            break;
        }

        new_csap->layers[layer].csap_layer_pdu = nds_pdu;
        csap_spt_descr = csap_get_proto_support(new_csap, layer);

        if ((csap_spt_descr->init_cb != NULL) &&
            (rc = csap_spt_descr->init_cb(new_csap, layer)) != 0)
        {
            ERROR(CSAP_LOG_FMT "Initialization of layer #%u '%s' "
                  "failed: %r", CSAP_LOG_ARGS(new_csap), layer,
                  csap_spt_descr->proto, rc);
            break;
        }
    }

    if (rc == 0)
    {
        /*
         * Initialize read/write layer (the lowest)
         */
        new_csap->read_write_layer = new_csap->depth - 1;
        if (csap_get_proto_support(new_csap, csap_get_rw_layer(new_csap))
                ->rw_init_cb == NULL)
        {
            ERROR("The lowest CSAP layer does not have read/write "
                  "initialization routine");
            rc = TE_RC(TE_TAD_CH, TE_EPROTONOSUPPORT);
        }
        else if ((rc = csap_get_proto_support(new_csap,
                                              csap_get_rw_layer(new_csap))
                           ->rw_init_cb(new_csap, csap_nds)) != 0)
        {
            ERROR("Initialization of the lowest layer to read/write "
                  "failed: %r", rc);
        }
    }

    asn_free_value(csap_nds);

    if (rc == 0)
    {
        SEND_ANSWER("0 %u", new_csap_id); 
    }
    else
    {
        csap_destroy(new_csap_id); 
        SEND_ANSWER("%u Csap creation error", TE_RC(TE_TAD_CH, rc));
    }
    return 0; 
#endif
}

/* See description in rcf_ch_api.h */
int
rcf_ch_csap_destroy(struct rcf_comm_connection *rcfc,
                    char *cbuf, size_t buflen, size_t answer_plen,
                    csap_handle_t csap_id)
{
#ifdef DUMMY_TAD 
    UNUSED(rcfc);
    UNUSED(cbuf);
    UNUSED(buflen);
    UNUSED(answer_plen);

    VERB("CSAP destroy: CSAP %u\n", csap_id);
    return -1;
#else
    csap_p          csap;
    csap_spt_type_p csap_spt_descr; 

    unsigned int layer;
    te_errno     rc; 
    
    check_init();

    VERB("%s: CSAP %d\n", __FUNCTION__, csap_id); 
    VERB("%s(CSAP %d), answer prefix %s", __FUNCTION__, csap_id,  cbuf);

    if ((csap = csap_find(csap_id)) == NULL)
    {
        WARN("%s: CSAP %d not exists", __FUNCTION__, csap_id);
        SEND_ANSWER("%d CSAP not exists", 
                    TE_RC(TE_TAD_CH, TE_ETADCSAPNOTEX));
        return 0;
    }

    CSAP_DA_LOCK(csap);
    if (csap->command != TAD_OP_IDLE)
    {
        csap->command = TAD_OP_DESTROY;
        CSAP_DA_UNLOCK(csap);
#if 0
        void *th_return;

        rc = pthread_join(csap->traffic_thread, &th_return);
        if (rc != 0)
        {
            ERROR("%s(): thread join failed, rc %d, errno %d", 
                  __FUNCTION__, rc, errno);
        }
#else
        while (csap->command != TAD_OP_IDLE)
            usleep(10*1000);
#endif
    } 
    else
        CSAP_DA_UNLOCK(csap);

    csap_spt_descr = csap_get_proto_support(csap, csap_get_rw_layer(csap));
    if (csap_spt_descr->rw_destroy_cb != NULL &&
        (rc = csap_spt_descr->rw_destroy_cb(csap)) != 0)
    {
        ERROR(CSAP_LOG_FMT "Destruction of the read/write layer #%u '%s' "
              "failed: %r", CSAP_LOG_ARGS(csap), csap_get_rw_layer(csap),
              csap_spt_descr->proto, rc);
    }

    for (layer = 0; layer < csap->depth; layer++)
    {
        csap_spt_descr = csap_get_proto_support(csap, layer);
        if (csap_spt_descr->destroy_cb != NULL)
        {
            rc = csap_spt_descr->destroy_cb(csap, layer);
            if (rc != 0)
            {
                ERROR(CSAP_LOG_FMT "Destruction of the layer #%u '%s' "
                      "failed: %r", CSAP_LOG_ARGS(csap), layer,
                      csap_spt_descr->proto, rc);
            }
        }
    }
    csap_destroy(csap_id);

    cbuf[answer_plen] = 0;
    VERB("%s(CSAP %d), answer prefix %s", __FUNCTION__, csap_id,  cbuf);

    usleep(100*1000);
    
    SEND_ANSWER("0");

    return 0;
#endif
}


/* See description in rcf_ch_api.h */
int
rcf_ch_trsend_start(struct rcf_comm_connection *rcfc,
                    char *cbuf, size_t buflen, size_t answer_plen,
                    const uint8_t *ba, size_t cmdlen,
                    csap_handle_t csap_id, te_bool postponed)
{
#ifdef DUMMY_TAD 
    UNUSED(rcfc);
    UNUSED(cbuf);
    UNUSED(buflen);
    UNUSED(answer_plen);
    UNUSED(ba);
    UNUSED(cmdlen);

    VERB("TRSEND start: CSAP %u %s\n",
         csap_id, postponed ? "postponed" : "");
    return -1;
#else
    int            rc;
    int            syms;
    asn_value     *nds; 
    csap_p         csap;
    pthread_attr_t pthread_attr;

    tad_sender_context *send_context;

    UNUSED(cmdlen);

    VERB("%s(CSAP %d)", __FUNCTION__, csap_id);
    cbuf[answer_plen] = 0;

    check_init();

    if (ba == NULL)
    {
        INFO("No NDS attached to traffic send start command");
        SEND_ANSWER("%d missing attached NDS", 
                    TE_RC(TE_TAD_CH, TE_ETADMISSNDS));
        return 0;
    }

    if ((csap = csap_find(csap_id)) == NULL)
    {
        WARN("CSAP %u does not exist", csap_id);
        SEND_ANSWER("%d Wrong CSAP id", TE_RC(TE_TAD_CH, TE_ETADCSAPNOTEX));
        return 0;
    }

    CSAP_DA_LOCK(csap);
    if (csap->command != TAD_OP_IDLE)
    {
        CSAP_DA_UNLOCK(csap);
        WARN("%s(CSAP %d) is busy", __FUNCTION__, csap_id);
        SEND_ANSWER("%d", TE_RC(TE_TAD_CH, TE_ETADCSAPSTATE));
        return 0;
    }
    CSAP_DA_UNLOCK(csap);

    csap->first_pkt = tv_zero;
    csap->last_pkt  = tv_zero;


    rc = asn_parse_value_text(ba, ndn_traffic_template, &nds, &syms);
    if (rc != 0)
    {
        ERROR("parse error in attached NDS, code %r, symbol: %d",
              rc, syms);
        SEND_ANSWER("%d", TE_RC(TE_TAD_CH, rc));
        return 0;
    } 

    CSAP_DA_LOCK(csap);

    strncpy(csap->answer_prefix, cbuf, MAX_ANS_PREFIX - 1);
    csap->answer_prefix[answer_plen] = 0; 

    csap->command = TAD_OP_SEND;

    csap->state = TAD_STATE_SEND;

    if (postponed)
        csap->state |= TAD_STATE_FOREGROUND;

    CSAP_DA_UNLOCK(csap);

    rc = tad_sender_prepare(csap, nds, rcfc, &send_context);
    if (rc != 0)
    {
        SEND_ANSWER("%d", rc);
        csap->command = TAD_OP_IDLE;
        csap->state = 0;
        return 0;
    }
    
    if ((rc = pthread_attr_init(&pthread_attr)) != 0 ||
        (rc = pthread_attr_setdetachstate(&pthread_attr,
                                          PTHREAD_CREATE_DETACHED)) != 0)
    {
        ERROR("Cannot initialize pthread attribute variable: %d", rc);
        SEND_ANSWER("%d cannot initialize pthread attribute variable",
                    TE_RC(TE_TAD_CH, rc));
        (void)tad_sender_release(send_context);
        csap->command = TAD_OP_IDLE;
        csap->state = 0;
        return 0;
    }

    /*
     * FIXME: Ideally it should be done after successfull start-up of
     * the Sender thread only, however it requires carefull CSAP state
     * protection.
     */
    if (csap->state & TAD_STATE_FOREGROUND) 
        SEND_ANSWER("%d", TE_RC(TE_TAD_CH, TE_EACK));
    else
        SEND_ANSWER("0 0"); 

    rc = pthread_create(&csap->traffic_thread, &pthread_attr, 
                        tad_sender_thread, send_context);
    if (rc != 0)
    {
        ERROR("Cannot create a new SEND thread: %d", rc);
        SEND_ANSWER("%d cannot create a new SEND thread", 
                    TE_RC(TE_TAD_CH, rc));
        (void)pthread_attr_destroy(&pthread_attr);
        (void)tad_sender_release(send_context);
        csap->command = TAD_OP_IDLE;
        csap->state = 0;
    }

    return 0;
#endif
}


/* See description in rcf_ch_api.h */
int
rcf_ch_trsend_stop(struct rcf_comm_connection *rcfc,
                   char *cbuf, size_t buflen, size_t answer_plen,
                   csap_handle_t csap_id)
{
#ifdef DUMMY_TAD 
    UNUSED(rcfc);
    UNUSED(cbuf);
    UNUSED(buflen);
    UNUSED(answer_plen);

    VERB("TRSEND stop CSAP %u\n", csap_id);
    return -1;
#else
    csap_p csap;

    VERB("trsend_stop CSAP %d", csap_id);

    check_init(); 

    if ((csap = csap_find(csap_id)) == NULL)
    {
        WARN("%s: CSAP %d not exists", __FUNCTION__, csap_id);
        SEND_ANSWER("%d Wrong CSAP id", TE_RC(TE_TAD_CH, TE_ETADCSAPNOTEX));
        return 0;
    }

    CSAP_DA_LOCK(csap);

    if (csap->command == TAD_OP_SEND)
    {
        strncpy(csap->answer_prefix, cbuf, answer_plen);
        csap->answer_prefix[answer_plen] = 0; 
        csap->command = TAD_OP_STOP;
        CSAP_DA_UNLOCK(csap);
    }
    else
    {
        CSAP_DA_UNLOCK(csap);
        F_VERB("Inappropriate command, CSAP is not sending ");
        SEND_ANSWER("%d Inappropriate command in current state", 
                    TE_RC(TE_TAD_CH, TE_ETADCSAPSTATE));
    }

    return 0;
#endif
}


/* See description in rcf_ch_api.h */
int
rcf_ch_trrecv_start(struct rcf_comm_connection *rcfc,
                    char *cbuf, size_t buflen, size_t answer_plen,
                    const uint8_t *ba, size_t cmdlen, csap_handle_t csap_id,
                    unsigned int num, te_bool results, unsigned int timeout)
{
#ifndef DUMMY_TAD 
    int         syms;
    int         rc;
    asn_value_p nds = NULL; 
    csap_p      csap;
    int         i, num_units;

    tad_task_context *recv_context;
    pthread_attr_t    pthread_attr;
    char              srname[] = "trsend_recv";
    te_bool           sr_flag = FALSE;
#endif

    INFO("%s: csap %d, num %u, timeout %u ms, %s", __FUNCTION__,
         csap_id, num, timeout, results ? "results" : "");

#ifdef DUMMY_TAD 
    UNUSED(rcfc);
    UNUSED(cbuf);
    UNUSED(buflen);
    UNUSED(answer_plen);
    UNUSED(ba);
    UNUSED(cmdlen);
    UNUSED(csap_id);
    UNUSED(num);
    UNUSED(results);
    UNUSED(timeout);

    return -1;
#else

    UNUSED(cmdlen);

    check_init(); 

    if (strncmp(cbuf + answer_plen, srname, strlen(srname)) == 0)
        sr_flag = TRUE;

    if (ba == NULL)
    {
        WARN("missing attached NDS");
        SEND_ANSWER("%d", TE_RC(TE_TAD_CH, TE_ETADMISSNDS));
        return 0;
    }

    if ((csap = csap_find(csap_id)) == NULL)
    {
        WARN("%s: CSAP %d not exists", __FUNCTION__, csap_id);
        SEND_ANSWER("%d", TE_RC(TE_TAD_CH, TE_ETADCSAPNOTEX));
        return 0;
    }

    CSAP_DA_LOCK(csap);
    if (csap->command != TAD_OP_IDLE)
    {
        CSAP_DA_UNLOCK(csap);
        WARN("CSAP is busy");
        SEND_ANSWER("%d", TE_RC(TE_TAD_CH, TE_ETADCSAPSTATE));
        return 0;
    }
    CSAP_DA_UNLOCK(csap);

    csap->first_pkt = tv_zero;
    csap->last_pkt  = tv_zero;

    if (sr_flag)
        rc = asn_parse_value_text(ba, ndn_traffic_template, &nds, &syms);
    else
        rc = asn_parse_value_text(ba, ndn_traffic_pattern, &nds, &syms);

    if (rc)
    { 
        ERROR("parse error in NDS, rc: %r, sym: %d", rc, syms);
        INFO("sr_flag %d, NDS: <%s>", (int)sr_flag, ba);
        SEND_ANSWER("%d", TE_RC(TE_TAD_CH, rc));
        return 0;
    } 
    VERB("nds parsed, pointer: %x", nds);

    
    CSAP_DA_LOCK(csap);

    csap->state = TAD_STATE_RECV;

    if (sr_flag) 
    {
        csap->command = TAD_OP_SEND_RECV; 
        csap->state |= TAD_STATE_FOREGROUND | TAD_STATE_SEND;
    }
    else
        csap->command = TAD_OP_RECV; 

    strncpy(csap->answer_prefix, cbuf, answer_plen);
    csap->answer_prefix[answer_plen] = 0; 

    CSAP_DA_UNLOCK(csap);

    csap->num_packets = num;

    CSAP_DA_LOCK(csap);
    if (results)
        csap->state |= TAD_STATE_RESULTS;
    CSAP_DA_UNLOCK(csap);

    recv_context = calloc(1, sizeof(tad_task_context));
    recv_context->csap = csap;
    recv_context->nds  = nds;
    recv_context->rcfc = rcfc; 

    if (!sr_flag)
        num_units = asn_get_length(nds, "");
    else
        num_units = 1; /* There is a single unit */
    rc = 0;
    for (i = 0; i < num_units; i++)
    {
        asn_value *pattern_unit = NULL;
        asn_value *pdus = NULL;

        if (!sr_flag)
            asn_get_indexed(nds, &pattern_unit, i, NULL);
        else
            pattern_unit = nds;

        if (pattern_unit == NULL)
        {
            WARN("%s: NULL pattern unit #%d", __FUNCTION__, i); 
            rc = TE_ETADWRONGNDS;
            break;
        }

        asn_get_subvalue(pattern_unit, (const asn_value **)&pdus, "pdus");

        rc = tad_confirm_pdus(csap, TRUE, pdus, NULL);
        if (rc != 0)
            break; 
    } /* loop by pattern units */

    if (rc) 
    {
        WARN("pattern does not confirm to CSAP; rc %r", rc);
        asn_free_value(nds);
        SEND_ANSWER("%d", TE_RC(TE_TAD_CH, rc));
        csap->command = TAD_OP_IDLE;
        csap->state = 0;
        return 0;
    }

    if (timeout && timeout != TAD_TIMEOUT_INF)
    { 
        gettimeofday(&csap->wait_for, NULL);
        csap->wait_for.tv_usec += timeout * 1000;
        csap->wait_for.tv_sec += 
            (csap->wait_for.tv_usec / 1000000);
        csap->wait_for.tv_usec %= 1000000;

        VERB("%s(): csap %d, wait_for set to %u.%u", __FUNCTION__,
             csap->id,
             csap->wait_for.tv_sec,
             csap->wait_for.tv_usec);
    }
    else 
        memset(&csap->wait_for, 0, sizeof(struct timeval));

    if ((rc = pthread_attr_init(&pthread_attr)) != 0 ||
        (rc = pthread_attr_setdetachstate(&pthread_attr,
                                          PTHREAD_CREATE_DETACHED)) != 0)
    {
        ERROR("Cannot initialize pthread attribute variable: %d", rc);
        free(recv_context);
        csap->command = TAD_OP_IDLE;
        csap->state = 0;
        SEND_ANSWER("%d cannot initialize pthread attribute variable",
                    TE_RC(TE_TAD_CH, rc));
        return 0;
    }

    if ((rc = pthread_create(&csap->traffic_thread, &pthread_attr,
                             tad_receiver_thread, recv_context)) != 0)
    {
        ERROR("recv thread create error; rc %d", rc);
        asn_free_value(recv_context->nds);
        free(recv_context);
        csap->command = TAD_OP_IDLE;
        csap->state = 0;
        SEND_ANSWER("%d", TE_RC(TE_TAD_CH, rc));
    }

    return 0;
#endif
}


/* See description in rcf_ch_api.h */
int
rcf_ch_trrecv_stop(struct rcf_comm_connection *rcfc,
                   char *cbuf, size_t buflen, size_t answer_plen,
                   csap_handle_t csap_id)
{
#ifdef DUMMY_TAD 
    UNUSED(rcfc);
    UNUSED(cbuf);
    UNUSED(buflen);
    UNUSED(answer_plen);

    VERB("TRRECV stop CSAP %u\n", csap_id);
    return -1;
#else
    csap_p csap;

    VERB("%s: CSAP %d", __FUNCTION__, csap_id);

    check_init();

    if ((csap = csap_find(csap_id)) == NULL)
    {
        WARN("%s: CSAP %d not exists", __FUNCTION__, csap_id);
        SEND_ANSWER("%d 0", TE_RC(TE_TAD_CH, TE_ETADCSAPNOTEX));
        return 0;
    }

    CSAP_DA_LOCK(csap); 

    /* Note: watch carefully, that unlock made in every 
     * conditional branch. */
    if (csap->command == TAD_OP_RECV)
    {
        csap->command = TAD_OP_STOP; 

        if (csap->state & TAD_STATE_FOREGROUND)
        {
            CSAP_DA_UNLOCK(csap);
            /* With some probability number of received packets, 
             * reported here, may be wrong. */
            SEND_ANSWER("0 %u", csap->num_packets); 
        }
        else
        { 
            strncpy(csap->answer_prefix, cbuf, answer_plen);
            csap->answer_prefix[answer_plen] = 0; 
            CSAP_DA_UNLOCK(csap);
        }
    }
    else
    {
        WARN("%s: inappropriate command, CSAP %d is not receiving; "
             "command %d; state %x ", __FUNCTION__, csap->id, 
             (int)csap->command, (int)csap->state);
        CSAP_DA_UNLOCK(csap);
        SEND_ANSWER("%d 0", TE_RC(TE_TAD_CH, TE_ETADCSAPSTATE));
    }

    return 0;
#endif
}


/* See description in rcf_ch_api.h */
int
rcf_ch_trrecv_wait(struct rcf_comm_connection *rcfc,
                   char *cbuf, size_t buflen, size_t answer_plen,
                   csap_handle_t csap_id)
{
#ifdef DUMMY_TAD 
    UNUSED(rcfc);
    UNUSED(cbuf);
    UNUSED(buflen);
    UNUSED(answer_plen);
    UNUSED(csap_id);

    VERB("%s: CSAP %d", __FUNCTION__, csap_id);
    
    return -1;
#else
    csap_p csap;

    VERB("%s: CSAP %d", __FUNCTION__, csap_id);

    check_init();

    if ((csap = csap_find(csap_id)) == NULL)
    {
        WARN("%s: CSAP %d not exists", __FUNCTION__, csap_id);
        SEND_ANSWER("%d 0", TE_RC(TE_TAD_CH, TE_ETADCSAPNOTEX));
        return 0;
    }

    CSAP_DA_LOCK(csap);

    if (csap->command == TAD_OP_RECV)
    {
        SEND_ANSWER("%d", TE_RC(TE_TAD_CH, TE_EACK));
        csap->command = TAD_OP_WAIT; 
        strncpy(csap->answer_prefix, cbuf, answer_plen);
        csap->answer_prefix[answer_plen] = 0; 
        CSAP_DA_UNLOCK(csap);
    }
    else
    {
        ERROR("Inappropriate command, CSAP %d is not receiving; "
              "command %d; state %x",
              csap->id, 
              (int)csap->command, (int)csap->state);
        CSAP_DA_UNLOCK(csap);
        SEND_ANSWER("%d 0", TE_RC(TE_TAD_CH, TE_ETADCSAPSTATE));
    }
    return 0;
#endif
}

/* See description in rcf_ch_api.h */
int
rcf_ch_trrecv_get(struct rcf_comm_connection *rcfc,
                  char *cbuf, size_t buflen, size_t answer_plen,
                  csap_handle_t csap_id)
{
#ifdef DUMMY_TAD 
    UNUSED(rcfc);
    UNUSED(cbuf);
    UNUSED(buflen);
    UNUSED(answer_plen);

    VERB("TRRECV get CSAP %u\n", csap_id);
    return -1;
#else
    csap_p      csap;

    check_init();

    VERB("trrecv_get for CSAP %d", csap_id);

    if ((csap = csap_find(csap_id)) == NULL)
    {
        WARN("%s: CSAP %d not exists", __FUNCTION__, csap_id);
        SEND_ANSWER("%d 0", TE_RC(TE_TAD_CH, TE_ETADCSAPNOTEX));
        return 0;
    }

    CSAP_DA_LOCK(csap);
    if (csap->command == TAD_OP_RECV)
    {
        csap->command = TAD_OP_GET;
        strncpy(csap->answer_prefix, cbuf, answer_plen);
        csap->answer_prefix[answer_plen] = 0; 
        CSAP_DA_UNLOCK(csap);
    }
    else
    {
        ERROR("Inappropriate command, CSAP %d is not receiving; "
              "command %d; state %x ", csap->id, 
              (int)csap->command, (int)csap->state);
        CSAP_DA_UNLOCK(csap);

        SEND_ANSWER("%d 0", TE_RC(TE_TAD_CH, TE_ETADCSAPSTATE));
    }

    return 0;
#endif
}


/* See description in rcf_ch_api.h */
int
rcf_ch_csap_param(struct rcf_comm_connection *rcfc,
                  char *cbuf, size_t buflen, size_t answer_plen,
                  csap_handle_t csap_id, const char *param)
{
#ifdef DUMMY_TAD
    UNUSED(rcfc);
    UNUSED(cbuf);
    UNUSED(buflen);
    UNUSED(answer_plen);

    VERB("CSAP param: CSAP %u param <%s>\n", csap_id, param);
    return -1;
#else
    csap_p              csap = csap_find(csap_id);
    csap_layer_get_param_cb_t get_param_cb;
    int                 layer = csap == NULL ?
                                0 : csap->read_write_layer;

    check_init();
    VERB("CSAP param: CSAP %u param <%s>\n", csap_id, param);
    if ((csap = csap_find(csap_id)) == NULL)
    {
        WARN("%s: CSAP %d not exists", __FUNCTION__, csap_id);
        SEND_ANSWER("%d Wrong CSAP id", TE_RC(TE_TAD_CH, TE_ETADCSAPNOTEX));
    }
    else if (strcmp(param, CSAP_PARAM_STATUS) == 0)
    {
        tad_csap_status_t status = 0;

        CSAP_DA_LOCK(csap);
        if (csap->command == TAD_OP_IDLE)
            status = CSAP_IDLE;
        else if (csap->state & TAD_STATE_COMPLETE)
        {
            if (csap->last_errno)
                status = CSAP_ERROR;
            else
                status = CSAP_COMPLETED;
        }
        else
            status = CSAP_BUSY;

        F_VERB("CSAP get_param, status %d, command flag %d\n", 
               (int)status, (int)csap->command);
        CSAP_DA_UNLOCK(csap);

        SEND_ANSWER("0 %d", (int)status);
    }
    else if (strcmp(param, CSAP_PARAM_FIRST_PACKET_TIME) == 0)
    {
        VERB("CSAP get_param, get first pkt, %u.%u\n",
                                (uint32_t)csap->first_pkt.tv_sec, 
                                (uint32_t)csap->first_pkt.tv_usec);
        SEND_ANSWER("0 %u.%u",  (uint32_t)csap->first_pkt.tv_sec, 
                                (uint32_t)csap->first_pkt.tv_usec);
    }
    else if (strcmp(param, CSAP_PARAM_LAST_PACKET_TIME) == 0)
    {
        VERB("CSAP get_param, get last pkt, %u.%u\n",
                                (uint32_t)csap->last_pkt.tv_sec, 
                                (uint32_t)csap->last_pkt.tv_usec);
        SEND_ANSWER("0 %u.%u",  (uint32_t)csap->last_pkt.tv_sec, 
                                (uint32_t)csap->last_pkt.tv_usec);
    }
    else if ((get_param_cb =
                  csap_get_proto_support(csap, layer)->get_param_cb)
             == NULL)
    {
        VERB("CSAP does not support get_param\n");
        SEND_ANSWER("%d CSAP does not support get_param", 
                    TE_RC(TE_TAD_CH, TE_EOPNOTSUPP));
    }
    else
    {
        char *param_value = get_param_cb(csap, layer, param);
        if (param_value)
        {
            VERB("got value: <%s>\n", param_value);
            SEND_ANSWER("0 %s", param_value);
            free(param_value);
        }
        else
        {
            VERB("CSAP return error for get_param\n");
            SEND_ANSWER("%d CSAP return error for get_param", 
                        TE_RC(TE_TAD_CH, TE_EOPNOTSUPP));
        }
    }

    return 0;
#endif
}


/* See description in rcf_ch_api.h */
int
rcf_ch_trsend_recv(struct rcf_comm_connection *rcfc,
                   char *cbuf, size_t buflen, size_t answer_plen,
                   const uint8_t *ba, size_t cmdlen, csap_handle_t csap_id,
                   te_bool results, unsigned int timeout)
{ 
#ifdef DUMMY_TAD 
    UNUSED(rcfc);
    UNUSED(cbuf);
    UNUSED(buflen);
    UNUSED(answer_plen);
    UNUSED(ba);
    UNUSED(cmdlen);

    VERB("TRSEND recv: CSAP %u timeout %u %s\n",
         csap_id, timeout, results ? "results" : "");
    return -1;
#else
    csap_p csap;

    UNUSED(cmdlen);

    VERB("CSAP %d", csap_id);

    check_init(); 

    if ((csap = csap_find(csap_id)) == NULL)
    {
        SEND_ANSWER("%d Wrong CSAP id", TE_RC(TE_TAD_CH, TE_ETADCSAPNOTEX));
        return 0;
    }
    CSAP_DA_LOCK(csap);
    if (csap->command != TAD_OP_IDLE)
    {
        CSAP_DA_UNLOCK(csap);
        SEND_ANSWER("%d Specified CSAP is busy", 
            TE_RC(TE_TAD_CH, TE_ETADCSAPSTATE));
        return 0;
    }
    CSAP_DA_UNLOCK(csap);

    return rcf_ch_trrecv_start(rcfc, cbuf, buflen, answer_plen, ba,
                               cmdlen, csap_id, 1 /* one packet */, 
                               results, timeout);
#endif
}


