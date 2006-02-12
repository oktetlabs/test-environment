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

#ifndef TAD_DUMMY

#include "ndn.h" 

#include "tad_types.h"
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


/**
 * Check TAD initialization. It is intended to be called from RCF
 * Command Handler routines only. If initialization is not done or
 * failed, the macro calls return with -1 (no support).
 */
#define TAD_CHECK_INIT \
    do {                            \
        if (!tad_is_initialized)    \
            return -1;              \
    } while (0)


static te_bool tad_is_initialized = FALSE;


/* See description in rcf_ch_api.h */
te_errno
rcf_ch_tad_init(void)
{
    if (tad_is_initialized)
        return TE_RC(TE_TAD_CH, TE_EALREADY);

    /*
     * Set initialized flag in any case, if error occur it will be
     * unsed, by shutdown routine.
     */
    tad_is_initialized = TRUE;
    
#ifdef TAD_DUMMY

    return TE_RC(TE_TAD_CH, TE_ENOSYS);

#else /* !TAD_DUMMY */

#define CHECK_RC(_expr) \
    do {                                            \
        te_errno rc_ = (_expr);                     \
                                                    \
        if (rc_ != 0)                               \
        {                                           \
            ERROR("%s failed: %r",  #_expr, rc_);   \
            return rc_;                             \
        }                                           \
    } while (0)

    csap_id_init();
    CHECK_RC(csap_spt_init());

#ifdef WITH_ATM
    extern te_errno csap_support_atm_register(void);
    CHECK_RC(csap_support_atm_register());
#endif
#ifdef WITH_ETH
    extern te_errno csap_support_eth_register(void);
    CHECK_RC(csap_support_eth_register());
#endif 
#ifdef WITH_ARP
    extern te_errno csap_support_arp_register(void);
    CHECK_RC(csap_support_arp_register());
#endif
#ifdef WITH_IPSTACK
    extern te_errno csap_support_ipstack_register(void);
    CHECK_RC(csap_support_ipstack_register());
#endif
#ifdef WITH_SNMP
    extern te_errno csap_support_snmp_register(void);
    CHECK_RC(csap_support_snmp_register());
#endif
#ifdef WITH_CLI
    extern te_errno csap_support_cli_register(void);
    CHECK_RC(csap_support_cli_register());
#endif
#ifdef WITH_DHCP
    extern te_errno csap_support_dhcp_register(void);
    CHECK_RC(csap_support_dhcp_register());
#endif
#ifdef WITH_BRIDGE
    extern te_errno csap_support_bridge_register(void);
    CHECK_RC(csap_support_bridge_register());
#endif
#ifdef WITH_PCAP
    extern te_errno csap_support_pcap_register(void);
    CHECK_RC(csap_support_pcap_register());
#endif
#ifdef WITH_ISCSI
    extern te_errno csap_support_iscsi_register(void);
    CHECK_RC(csap_support_iscsi_register());
#endif
#ifdef WITH_SOCKET
    extern te_errno csap_support_socket_register(void);
    CHECK_RC(csap_support_socket_register());
#endif

#undef CHECK_RC

    return 0;
    
#endif /* !TAD_DUMMY */
}

/* See description in rcf_ch_api.h */
te_errno
rcf_ch_tad_shutdown(void)
{
    te_errno    rc = 0;

    if (!tad_is_initialized)
        return TE_RC(TE_TAD_CH, TE_EINVAL);

    /* The function continues shutdown even in the case of failures */
    tad_is_initialized = FALSE;


#ifndef TAD_DUMMY 
    csap_spt_destroy();
    csap_id_destroy();
#endif

    return rc;
}

#ifndef TAD_DUMMY 

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

#endif /* TAD_DUMMY */

/* See description in rcf_ch_api.h */
int
rcf_ch_csap_create(struct rcf_comm_connection *rcfc,
                   char *cbuf, size_t buflen, size_t answer_plen,
                   const uint8_t *ba, size_t cmdlen,
                   const char *stack, const char *params)
{
#ifdef TAD_DUMMY 
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


    TAD_CHECK_INIT;

    rc = csap_create(stack, &new_csap);
    if (rc != 0)
    {
        ERROR("CSAP '%s' creation internal error %r", stack, rc);
        SEND_ANSWER("%u", rc);
        return 0;
    }
    new_csap_id = new_csap->id;

    INFO("CSAP '%s' created, new id: %u", stack, new_csap_id);

    if (ba == NULL)
    {
        ERROR("Missing attached NDS with CSAP parameters");
        csap_destroy(new_csap_id);
        SEND_ANSWER("%u", TE_ETADMISSNDS);
        return 0;
    }

    rc = asn_parse_value_text(ba, ndn_csap_spec, &csap_nds, &syms);
    if (rc != 0)
    {
        ERROR("CSAP NDS parse error sym=%d: %r", syms, rc);
        SEND_ANSWER("%u", TE_RC(TE_TAD_CH, rc));
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
        new_csap->rw_layer = new_csap->depth - 1;
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

#define PRINT(msg...) \
    do { printf(msg); printf("\n"); fflush(stdout); } while (0)

/* See description in rcf_ch_api.h */
int
rcf_ch_csap_destroy(struct rcf_comm_connection *rcfc,
                    char *cbuf, size_t buflen, size_t answer_plen,
                    csap_handle_t csap_id)
{
#ifdef TAD_DUMMY 
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
    
    TAD_CHECK_INIT;

    VERB("%s: CSAP %u\n", __FUNCTION__, csap_id); 
    VERB("%s(CSAP %u), answer prefix %s", __FUNCTION__, csap_id,  cbuf);

    if ((csap = csap_find(csap_id)) == NULL)
    {
        WARN("%s: CSAP %u not exists", __FUNCTION__, csap_id);
        SEND_ANSWER("%u", TE_RC(TE_TAD_CH, TE_ETADCSAPNOTEX));
        return 0;
    }

    rc = csap_command(csap, TAD_OP_DESTROY);
    if (rc != 0)
    {
        SEND_ANSWER("%u", rc);
        return 0;
    }

    if (~csap->state & CSAP_STATE_IDLE)
    {
        rc = csap_wait(csap, CSAP_STATE_DONE);
    }

    INFO(CSAP_LOG_FMT "Starting destruction", CSAP_LOG_ARGS(csap));

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
    VERB("%s(CSAP %u), answer prefix %s", __FUNCTION__, csap_id,  cbuf);

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
#ifdef TAD_DUMMY 
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


    UNUSED(cmdlen);

    VERB("%s(CSAP %u)", __FUNCTION__, csap_id);
    cbuf[answer_plen] = 0;

    TAD_CHECK_INIT;

    if ((csap = csap_find(csap_id)) == NULL)
    {
        WARN("CSAP %u does not exist", csap_id);
        SEND_ANSWER("%u", TE_RC(TE_TAD_CH, TE_ETADCSAPNOTEX));
        return 0;
    }

    rc = csap_command(csap, TAD_OP_SEND);
    if (rc != 0)
    {
        SEND_ANSWER("%u", rc);
        return 0;
    }

    if (ba == NULL)
    {
        ERROR(CSAP_LOG_FMT "No NDS attached to traffic send start "
              "command", CSAP_LOG_ARGS(csap));
        (void)csap_command(csap, TAD_OP_IDLE);
        SEND_ANSWER("%u", TE_RC(TE_TAD_CH, TE_ETADMISSNDS));
        return 0;
    }

    rc = asn_parse_value_text(ba, ndn_traffic_template, &nds, &syms);
    if (rc != 0)
    {
        ERROR(CSAP_LOG_FMT "Parse error in attached NDS on symbol %d: %r",
              CSAP_LOG_ARGS(csap), syms, rc);
        (void)csap_command(csap, TAD_OP_IDLE);
        SEND_ANSWER("%u", TE_RC(TE_TAD_CH, rc));
        return 0;
    } 

    CSAP_LOCK(csap);

    if (postponed)
        csap->state |= CSAP_STATE_FOREGROUND;

    csap->first_pkt = tv_zero;
    csap->last_pkt  = tv_zero;

    CSAP_UNLOCK(csap);

    rc = tad_send_prepare(csap, nds, rcfc, cbuf, answer_plen);
    if (rc != 0)
    {
        (void)csap_command(csap, TAD_OP_IDLE);
        SEND_ANSWER("%u", rc);
        return 0;
    }
    
    rc = tad_pthread_create(NULL, tad_send_thread, csap);
    if (rc != 0)
    {
        (void)tad_send_release(csap, csap_get_send_context(csap));
        (void)csap_command(csap, TAD_OP_IDLE);
        SEND_ANSWER("%u", rc);
        return 0;
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
#ifdef TAD_DUMMY 
    UNUSED(rcfc);
    UNUSED(cbuf);
    UNUSED(buflen);
    UNUSED(answer_plen);

    VERB("TRSEND stop CSAP %u\n", csap_id);
    return -1;
#else
    csap_p      csap;
    te_errno    rc;
    te_errno    status = 0;

    VERB("trsend_stop CSAP %u", csap_id);

    TAD_CHECK_INIT; 

    if ((csap = csap_find(csap_id)) == NULL)
    {
        WARN("%s: CSAP %u not exists", __FUNCTION__, csap_id);
        SEND_ANSWER("%u 0", TE_RC(TE_TAD_CH, TE_ETADCSAPNOTEX));
        return 0;
    }

    rc = csap_command(csap, TAD_OP_STOP);
    if (rc != 0)
    {
        SEND_ANSWER("%u 0", rc);
        return 0;
    }

    if (csap->state & CSAP_STATE_SEND)
    {
        rc = csap_wait(csap, CSAP_STATE_DONE);
        if (rc == 0)
        {
            status = csap_get_send_context(csap)->status;
        }
    }
    else
    {
        rc = TE_RC(TE_TAD_CH, TE_ETADCSAPSTATE);
    }

    if ((rc == 0) && (~csap->state & CSAP_STATE_FOREGROUND))
    {
        rc = csap_command(csap, TAD_OP_IDLE);
    }
    TE_RC_UPDATE(rc, status);

    SEND_ANSWER("%u %u", rc, csap_get_send_context(csap)->sent_pkts);

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
#ifndef TAD_DUMMY 
    csap_p      csap;
    te_errno    rc;
    int         syms;
    asn_value_p nds = NULL; 
#endif

    INFO("%s: csap %u, num %u, timeout %u ms, %s", __FUNCTION__,
         csap_id, num, timeout, results ? "results" : "");

#ifdef TAD_DUMMY 
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

    TAD_CHECK_INIT; 

    if ((csap = csap_find(csap_id)) == NULL)
    {
        ERROR("%s: CSAP %u not exists", __FUNCTION__, csap_id);
        SEND_ANSWER("%u", TE_RC(TE_TAD_CH, TE_ETADCSAPNOTEX));
        return 0;
    }

    rc = csap_command(csap, TAD_OP_RECV);
    if (rc != 0)
    {
        SEND_ANSWER("%u", rc);
        return 0;
    }

    if (ba == NULL)
    {
        ERROR(CSAP_LOG_FMT "No NDS attached to traffic receive start "
              "command", CSAP_LOG_ARGS(csap));
        (void)csap_command(csap, TAD_OP_IDLE);
        SEND_ANSWER("%u", TE_RC(TE_TAD_CH, TE_ETADMISSNDS));
        return 0;
    }

    rc = asn_parse_value_text(ba, ndn_traffic_pattern, &nds, &syms);
    if (rc != 0)
    { 
        ERROR(CSAP_LOG_FMT "Parse error in attached NDS on symbol %d: %r",
              CSAP_LOG_ARGS(csap), syms, rc);
        (void)csap_command(csap, TAD_OP_IDLE);
        SEND_ANSWER("%u", TE_RC(TE_TAD_CH, rc));
        return 0;
    } 

    
    CSAP_LOCK(csap);

    if (results)
        csap->state |= CSAP_STATE_RESULTS;

    csap->first_pkt = csap->last_pkt = tv_zero;

    CSAP_UNLOCK(csap);


    rc = tad_recv_prepare(csap, nds, num, timeout, rcfc, cbuf, answer_plen);
    if (rc != 0)
    {
        (void)csap_command(csap, TAD_OP_IDLE);
        SEND_ANSWER("%u", rc);
        return 0;
    }
    
    rc = tad_pthread_create(NULL, tad_recv_thread, csap);
    if (rc != 0)
    {
        (void)tad_recv_release(csap, csap_get_recv_context(csap));
        (void)csap_command(csap, TAD_OP_IDLE);
        SEND_ANSWER("%u", rc);
        return 0;
    }

    return 0;
#endif
}


#ifndef TAD_DUMMY 
/**
 * Generic implementation of trrecv_stop/wait/get.
 *
 * @param rcfc          RCF communication handle
 * @param cbuf          Buffer with request
 * @param answer_plen   Answer prefix length
 * @param csap_id       CSAP ID
 * @param op            Operation (stop/wait/get)
 * 
 * @return Status code.
 */
static te_errno
tad_trrecv_op(rcf_comm_connection *rcfc,
              char *cbuf, size_t buflen, size_t answer_plen,
              csap_handle_t csap_id, tad_traffic_op_t op)
{
    csap_p      csap;
    te_errno    rc;

    VERB("%s: CSAP %u OP %u", __FUNCTION__, csap_id, op);

    TAD_CHECK_INIT;

    if ((csap = csap_find(csap_id)) == NULL)
    {
        WARN("%s: CSAP %u not exists", __FUNCTION__, csap_id);
        SEND_ANSWER("%u 0", TE_RC(TE_TAD_CH, TE_ETADCSAPNOTEX));
        return 0;
    }

    rc = csap_command(csap, op);
    if (rc != 0)
    {
        SEND_ANSWER("%u 0", rc);
        return 0;
    }

    rc = tad_recv_op_enqueue(csap, op, rcfc, cbuf, answer_plen);
    if (rc != 0)
    {
        /* Keep set flags, there is only one way now - destroy */
        SEND_ANSWER("%u 0", rc);
        return 0;
    }

    return 0;
}
#endif

/* See description in rcf_ch_api.h */
int
rcf_ch_trrecv_stop(rcf_comm_connection *rcfc,
                   char *cbuf, size_t buflen, size_t answer_plen,
                   csap_handle_t csap_id)
{
#ifdef TAD_DUMMY 
    UNUSED(rcfc);
    UNUSED(cbuf);
    UNUSED(buflen);
    UNUSED(answer_plen);

    VERB("TRRECV stop CSAP %u\n", csap_id);
    return -1;
#else
    return tad_trrecv_op(rcfc, cbuf, buflen, answer_plen, csap_id,
                         TAD_OP_STOP);
#endif
}

/* See description in rcf_ch_api.h */
int
rcf_ch_trrecv_wait(struct rcf_comm_connection *rcfc,
                   char *cbuf, size_t buflen, size_t answer_plen,
                   csap_handle_t csap_id)
{
#ifdef TAD_DUMMY 
    UNUSED(rcfc);
    UNUSED(cbuf);
    UNUSED(buflen);
    UNUSED(answer_plen);
    UNUSED(csap_id);

    VERB("%s: CSAP %u", __FUNCTION__, csap_id);
    
    return -1;
#else
    return tad_trrecv_op(rcfc, cbuf, buflen, answer_plen, csap_id,
                         TAD_OP_WAIT);
#endif
}

/* See description in rcf_ch_api.h */
int
rcf_ch_trrecv_get(struct rcf_comm_connection *rcfc,
                  char *cbuf, size_t buflen, size_t answer_plen,
                  csap_handle_t csap_id)
{
#ifdef TAD_DUMMY 
    UNUSED(rcfc);
    UNUSED(cbuf);
    UNUSED(buflen);
    UNUSED(answer_plen);

    VERB("TRRECV get CSAP %u\n", csap_id);
    return -1;
#else
    return tad_trrecv_op(rcfc, cbuf, buflen, answer_plen, csap_id,
                         TAD_OP_GET);
#endif
}


/* See description in rcf_ch_api.h */
int
rcf_ch_trsend_recv(struct rcf_comm_connection *rcfc,
                   char *cbuf, size_t buflen, size_t answer_plen,
                   const uint8_t *ba, size_t cmdlen, csap_handle_t csap_id,
                   te_bool results, unsigned int timeout)
{ 
#ifndef TAD_DUMMY 
    csap_p      csap;
    te_errno    rc;
    int         syms;
    asn_value_p tmpl = NULL; 
    asn_value_p ptrn = NULL;
#endif

    INFO("%s: csap %u, timeout %u ms, %s", __FUNCTION__,
         csap_id, timeout, results ? "results" : "");

#ifdef TAD_DUMMY 
    UNUSED(rcfc);
    UNUSED(cbuf);
    UNUSED(buflen);
    UNUSED(answer_plen);
    UNUSED(ba);
    UNUSED(cmdlen);

    return -1;
#else

    UNUSED(cmdlen);

    TAD_CHECK_INIT; 

    if ((csap = csap_find(csap_id)) == NULL)
    {
        ERROR("%s: CSAP %u not exists", __FUNCTION__, csap_id);
        SEND_ANSWER("%u", TE_RC(TE_TAD_CH, TE_ETADCSAPNOTEX));
        return 0;
    }

    rc = csap_command(csap, TAD_OP_SEND_RECV);
    if (rc != 0)
    {
        SEND_ANSWER("%u", rc);
        return 0;
    }

    if (ba == NULL)
    {
        ERROR(CSAP_LOG_FMT "No NDS attached to traffic send/receive start "
              "command", CSAP_LOG_ARGS(csap));
        (void)csap_command(csap, TAD_OP_IDLE);
        SEND_ANSWER("%u", TE_RC(TE_TAD_CH, TE_ETADMISSNDS));
        return 0;
    }

    rc = asn_parse_value_text(ba, ndn_traffic_template, &tmpl, &syms);
    if (rc != 0)
    { 
        ERROR(CSAP_LOG_FMT "Parse error in attached NDS on symbol %d: %r",
              CSAP_LOG_ARGS(csap), syms, rc);
        (void)csap_command(csap, TAD_OP_IDLE);
        SEND_ANSWER("%u", TE_RC(TE_TAD_CH, rc));
        return 0;
    } 

    CSAP_LOCK(csap);

    if (results)
        csap->state |= CSAP_STATE_RESULTS;

    csap->first_pkt = tv_zero;
    csap->last_pkt  = tv_zero;

    CSAP_UNLOCK(csap);

    rc = tad_send_prepare(csap, tmpl, rcfc, cbuf, answer_plen);
    if (rc != 0)
    {
        goto exit;
    }
    if (csap_get_send_context(csap)->tmpl_data.n_units != 1)
    {
        rc = TE_RC(TE_TAD_CH, TE_ETADWRONGNDS);
        ERROR(CSAP_LOG_FMT "Invalid number of units in send/recv template",
              CSAP_LOG_ARGS(csap));
        (void)tad_send_release(csap, csap_get_send_context(csap));
        goto exit;
    }

    rc = tad_send_recv_generate_pattern(csap, tmpl, &ptrn);
    if (rc != 0)
    {
        ERROR(CSAP_LOG_FMT "Failed to generate pattern by template: %r",
              CSAP_LOG_ARGS(csap), rc);
        (void)tad_send_release(csap, csap_get_send_context(csap));
        goto exit;
    }

    rc = tad_recv_prepare(csap, ptrn, 1, timeout, rcfc, cbuf, answer_plen);
    if (rc != 0)
    {
        (void)tad_send_release(csap, csap_get_send_context(csap));
        goto exit;
    }

    /*
     * Start threads in reverse order.
     */

    /* Emulate 'trrecv_wait' operation */
    rc = tad_recv_op_enqueue(csap, TAD_OP_WAIT, rcfc, cbuf, answer_plen);
    if (rc != 0)
    {
        (void)tad_recv_release(csap, csap_get_recv_context(csap));
        (void)tad_send_release(csap, csap_get_send_context(csap));
        goto exit;
    }
    /* 
     * Don't care about Receiver context any more, since 'trrecv_wait'
     * operation is responsible for destruction. However, now it is
     * necessary to care about stop of 'trrecv_wait' operation.
     */

    if ((rc = tad_pthread_create(NULL, tad_recv_thread, csap)) != 0)
    {
        /* Set status of the Receiver */
        csap_get_recv_context(csap)->status = rc;
        /* We can release Sender context, since it has not been started */
        (void)tad_send_release(csap, csap_get_send_context(csap));
        /* Unblock 'trrecv_wait' operation */
        (void)csap_command(csap, TAD_OP_RECV_DONE);
    }
    else if ((rc = tad_pthread_create(NULL, tad_send_thread, csap)) != 0)
    {
        /* Set status of the Sender */
        csap_get_send_context(csap)->status = rc;
        /* We can release Sender context, since it has not been started */
        (void)tad_send_release(csap, csap_get_send_context(csap));
        /* Notify Receiver that send has been finished */
        (void)csap_command(csap, TAD_OP_SEND_DONE);
    }

    /* 
     * Emulated 'trrecv_wait' is responsible for send of answer and 
     * transition to IDLE state.
     */
    return 0;

exit:
    (void)csap_command(csap, TAD_OP_IDLE);
    SEND_ANSWER("%u", rc);
    return 0;
#endif
}

/* See description in rcf_ch_api.h */
int
rcf_ch_trpoll(struct rcf_comm_connection *rcfc,
              char *cbuf, size_t buflen, size_t answer_plen,
              csap_handle_t csap_id, unsigned int timeout)
{
#ifdef TAD_DUMMY 
    UNUSED(rcfc);
    UNUSED(cbuf);
    UNUSED(buflen);
    UNUSED(answer_plen);
    UNUSED(csap_id);

    return -1;
#else
    csap_p      csap;
    te_errno    rc;

    TAD_CHECK_INIT; 

    /*
     * Answers are send with 0 in poll ID, since no request to be
     * canceled.
     */

    if ((csap = csap_find(csap_id)) == NULL)
    {
        ERROR("%s: CSAP %u not exists", __FUNCTION__, csap_id);
        SEND_ANSWER("%u 0", TE_RC(TE_TAD_CH, TE_ETADCSAPNOTEX));
        return 0;
    }

    if (csap->state == CSAP_STATE_IDLE)
    {
        /* Just created CSAP */
        rc = TE_ETADCSAPSTATE;
    }
    else if (csap->state & CSAP_STATE_DONE)
    {
        /* send and/or receive is done */
        rc = 0;
    }
    else
    {
        /* send and/or receive is in progress */
        rc = TE_ETIMEDOUT;
    }

    if ((rc != TE_ETIMEDOUT) || (timeout == 0))
    {
        SEND_ANSWER("%u 0", TE_RC(TE_TAD_CH, rc));
        return 0;
    }

    rc = tad_poll_enqueue(csap, timeout, rcfc, cbuf, answer_plen);
    if (rc != 0)
    {
        ERROR(CSAP_LOG_FMT "Failed to enqueue poll request: %r",
              CSAP_LOG_ARGS(csap));
        SEND_ANSWER("%u 0", rc);
        return 0;
    }

    return 0;
#endif
}

/* See description in rcf_ch_api.h */
int
rcf_ch_trpoll_cancel(struct rcf_comm_connection *rcfc,
                     char *cbuf, size_t buflen, size_t answer_plen,
                     csap_handle_t csap_id, unsigned int poll_id)
{
#ifdef TAD_DUMMY 
    UNUSED(rcfc);
    UNUSED(cbuf);
    UNUSED(buflen);
    UNUSED(answer_plen);
    UNUSED(csap_id);
    UNUSED(poll_id);

    return -1;
#else
    csap_p              csap;
    tad_poll_context   *p;
    te_errno            rc = TE_ENOENT;
    int                 ret;

    TAD_CHECK_INIT; 

    if ((csap = csap_find(csap_id)) == NULL)
    {
        SEND_ANSWER("%u", TE_RC(TE_TAD_CH, TE_ETADCSAPNOTEX));
        return 0;
    }

    CSAP_LOCK(csap);
    for (p = csap->poll_ops.lh_first; p != NULL; p = p->links.le_next)
    {
        if (p->id == poll_id)
        {
            ret = pthread_cancel(p->thread);
            if (ret != 0)
            {
                /* FIXME: */
                rc = te_rc_os2te(errno);
            }
            else
            {
                rc = 0;
            }
            break;
        }
    }
    CSAP_UNLOCK(csap);

    SEND_ANSWER("%u", TE_RC(TE_TAD_CH, rc));
    return 0;

#endif
}

/* See description in rcf_ch_api.h */
int
rcf_ch_csap_param(struct rcf_comm_connection *rcfc,
                  char *cbuf, size_t buflen, size_t answer_plen,
                  csap_handle_t csap_id, const char *param)
{
#ifdef TAD_DUMMY
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
                                0 : csap->rw_layer;

    TAD_CHECK_INIT;
    VERB("CSAP param: CSAP %u param <%s>\n", csap_id, param);
    if ((csap = csap_find(csap_id)) == NULL)
    {
        WARN("%s: CSAP %u not exists", __FUNCTION__, csap_id);
        SEND_ANSWER("%u", TE_RC(TE_TAD_CH, TE_ETADCSAPNOTEX));
    }
    else if (strcmp(param, CSAP_PARAM_STATUS) == 0)
    {
        tad_csap_status_t status = 0;

        CSAP_LOCK(csap);
        if (csap->state & CSAP_STATE_IDLE)
        {
            status = CSAP_IDLE;
        }
        else if (csap->state & CSAP_STATE_DONE)
        {
            te_errno    rc;

            if (csap->state & CSAP_STATE_RECV)
                rc = csap_get_recv_context(csap)->status;
            else
                rc = csap_get_send_context(csap)->status;

            if (rc != 0)
                status = CSAP_ERROR;
            else
                status = CSAP_COMPLETED;
        }
        else
            status = CSAP_BUSY;

        F_VERB("CSAP get_param, state 0x%x, status %d\n", 
               csap->state, (int)status);
        CSAP_UNLOCK(csap);

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
        SEND_ANSWER("%u", TE_RC(TE_TAD_CH, TE_EOPNOTSUPP));
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
            SEND_ANSWER("%u", TE_RC(TE_TAD_CH, TE_EOPNOTSUPP));
        }
    }

    return 0;
#endif
}
