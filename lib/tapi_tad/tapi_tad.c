/** @file
 * @brief Test API
 *
 * Implementation of Test API for Traffic Application Domain
 * features.
 *
 *
 * Copyright (C) 2004 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 * @author Konstantin Abramenko <Konstantin.Abramenko@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER     "TAPI TAD"

#include "te_config.h"

#ifdef STDC_HEADERS
#include <stdlib.h>
#include <stdio.h>
#endif
#ifdef HAVE_ASSERT_H
#include <assert.h>
#endif
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#include "te_defs.h"
#include "te_errno.h"
#include "tad_common.h"

#include "logger_api.h"
#include "rcf_api.h"

#include "tapi_tad.h"
#include "ndn.h"


#define SEC_USEC_SEPARATOR  '.'

/**
 * Log exit point and return from function.
 *
 * @param _rc   - return code
 */
#define RETURN_RC(_rc) \
    do {                        \
        EXIT("%d", (_rc));      \
        return (_rc);           \
    } while (FALSE)



/* See description in tapi_tad.h */
int
tapi_csap_param_get_llint(const char *ta_name, int ta_sid,
                          csap_handle_t csap_id,
                          const char *param_name,
                          long long int *p_llint)
{
    int     rc;
    char    buf[RCF_MAX_VAL] = { 0, };
    char   *end;


    ENTRY("TA=%s, SID=%d, CSAP=%d, param=%s, location=0x%08x",
            ta_name, ta_sid, csap_id, param_name, p_llint);

    rc = rcf_ta_csap_param(ta_name, ta_sid, csap_id, param_name,
                           sizeof(buf), buf);
    if (rc != 0)
    {
        ERROR("Failed(%r) to get CSAP #%d parameter '%s' from "
            "TA %s:%d", rc, csap_id, param_name, ta_name, ta_sid);
        RETURN_RC(rc);
    }

    *p_llint = strtoll(buf, &end, 10);
    if ((end == buf) || (*end != '\0'))
    {
        ERROR("Conversion of string %s to number failed", buf);
        RETURN_RC(TE_EFMT);
    }

    RETURN_RC(0);

} /* tapi_csap_param_get_llint */


/* See description in tapi_tad.h */
int
tapi_csap_param_get_timestamp(const char *ta_name, int ta_sid,
                              csap_handle_t csap_id,
                              const char *timestamp_name,
                              struct timeval *p_timestamp)
{
    int         rc;
    char        buf[RCF_MAX_VAL] = { 0, };
    char       *start;
    char       *end;
    long int    tmp;


    ENTRY("TA=%s, SID=%d, CSAP=%d, timestamp=%s, location=0x%08x",
            ta_name, ta_sid, csap_id, timestamp_name, p_timestamp);

    rc = rcf_ta_csap_param(ta_name, ta_sid, csap_id, timestamp_name,
                           sizeof(buf), buf);
    if (rc != 0)
    {
        ERROR("Failed(%d) to get CSAP #%d parameter '%s' from TA %s:%d",
            rc, csap_id, timestamp_name, ta_name, ta_sid);
        RETURN_RC(rc);
    }

    /* Get seconds */
    start = buf;
    tmp = strtol(start, &end, 10);
    if ((start == end) || (*end != SEC_USEC_SEPARATOR))
    {
        ERROR("Conversion of string %s to number failed", start);
        RETURN_RC(TE_EFMT);
    }
    p_timestamp->tv_sec = tmp;
    /* Get microseconds */
    start = end + 1;
    tmp = strtol(start, &end, 10);
    if ((start == end) || (*end != '\0'))
    {
        ERROR("Conversion of string %s to number failed", start);
        RETURN_RC(TE_EFMT);
    }
    p_timestamp->tv_usec = tmp;

    RETURN_RC(0);

} /* tapi_csap_param_get_timestamp */


/* See description in tapi_tad.h */
int
tapi_csap_get_total_bytes(const char *ta_name, int ta_sid,
                          csap_handle_t csap_id,
                          unsigned long long int *p_bytes)
{
    int             rc;
    long long int   tmp;


    ENTRY("TA=%s, SID=%d, CSAP=%d, location=0x%08x",
            ta_name, ta_sid, csap_id, p_bytes);

    rc = tapi_csap_param_get_llint(ta_name, ta_sid, csap_id,
                                   CSAP_PARAM_TOTAL_BYTES, &tmp);
    if (rc != 0)
    {
        RETURN_RC(rc);
    }

    assert(tmp >= 0);
    *p_bytes = tmp;

    RETURN_RC(0);

} /* tapi_csap_get_total_bytes */


/* See description in tapi_tad.h */
int
tapi_csap_get_duration(const char *ta_name, int ta_sid,
                       csap_handle_t csap_id, struct timeval *p_dur)
{
    int             rc;
    struct timeval  first_pkt_time;
    struct timeval  last_pkt_time;


    ENTRY("TA=%s, SID=%d, CSAP=%d, location=0x%08x",
            ta_name, ta_sid, csap_id, p_dur);

    /* Get the first time stamp */
    rc = tapi_csap_param_get_timestamp(ta_name, ta_sid, csap_id,
                                       CSAP_PARAM_FIRST_PACKET_TIME,
                                       &first_pkt_time);
    if (rc != 0)
    {
        RETURN_RC(rc);
    }
    VERB("First packet time %d sec %d usec",
        first_pkt_time.tv_sec, first_pkt_time.tv_usec);
    /* Get the last packet time stamp */
    rc = tapi_csap_param_get_timestamp(ta_name, ta_sid, csap_id,
                                       CSAP_PARAM_LAST_PACKET_TIME,
                                       &last_pkt_time);
    if (rc != 0)
    {
        RETURN_RC(rc);
    }
    VERB("Last packet time %d sec %d usec",
        last_pkt_time.tv_sec, last_pkt_time.tv_usec);

    p_dur->tv_sec  = last_pkt_time.tv_sec  - first_pkt_time.tv_sec;
    p_dur->tv_usec = last_pkt_time.tv_usec - first_pkt_time.tv_usec;
    if (p_dur->tv_usec < 0)
    {
        --(p_dur->tv_sec);
        p_dur->tv_usec += 1000000;
        assert(p_dur->tv_usec > 0);
    }
    assert(p_dur->tv_usec < 1000000);
    VERB("Dutation between the first and the last timestamp %d sec "
        "%d usec", p_dur->tv_sec, p_dur->tv_usec);

    RETURN_RC(0);

} /* tapi_csap_get_duration */


/* Description in tapi_tad.h */
int 
tapi_csap_get_status(const char *ta_name, int ta_sid, csap_handle_t csap_id,
                                tad_csap_status_t *status)
{
    int             rc;
    long long int   tmp;


    ENTRY("TA=%s, SID=%d, CSAP=%d, location=0x%08x",
            ta_name, ta_sid, csap_id, status);

    rc = tapi_csap_param_get_llint(ta_name, ta_sid, csap_id,
                                   CSAP_PARAM_STATUS, &tmp);

    if (rc != 0)
    {
        RETURN_RC(rc);
    }

    *status = (int)tmp;

    RETURN_RC(0); 
}

/* Description in tapi_tad.h */
int 
tapi_tad_csap_create(const char *ta_name, int session,
                     const char *stack_id, 
                     const asn_value *csap_spec, int *handle)
{
    int  rc;
    char tmp_name[] = "/tmp/te_tapi_tad_csap_create.XXXXXX";

    if ((rc = te_make_tmp_file(tmp_name)) != 0)
        return TE_RC(TE_TAPI, rc);

    asn_save_to_file(csap_spec, tmp_name); 

    rc = rcf_ta_csap_create(ta_name, session, stack_id, tmp_name, handle);

    unlink(tmp_name);
    if (rc != 0)
        ERROR("Csap create failed with rc %r", rc);
    return rc;
}

/* Description in tapi_tad.h */
int 
tapi_tad_trsend_start(const char *ta_name, int session, 
                      csap_handle_t csap, const asn_value *templ,
                      rcf_call_mode_t blk_mode)
{
    int  rc;
    char tmp_name[] = "/tmp/te_tapi_tad_trsend_start.XXXXXX";

    if ((rc = te_make_tmp_file(tmp_name)) != 0)
        return TE_RC(TE_TAPI, rc);

    asn_save_to_file(templ, tmp_name);

    rc = rcf_ta_trsend_start(ta_name, session, csap, tmp_name, blk_mode);
    unlink(tmp_name);
    if (rc != 0)
        WARN("trsend_start failed with rc %r", rc);
    return rc;
}

/* See description in tapi_tad.h */
te_errno
tapi_tad_trrecv_start(const char *ta_name, int session,
                      csap_handle_t handle, const asn_value *pattern,
                      unsigned int timeout, unsigned int num,
                      rcf_trrecv_mode mode)
{
    te_errno rc = 0;
    char     tmp_name[] = "/tmp/te_tapi_tad_trrecv_start.XXXXXX";

    if ((rc = te_make_tmp_file(tmp_name)) != 0)
        return TE_RC(TE_TAPI, rc);

    rc = asn_save_to_file(pattern, tmp_name);
    if (rc != 0)
    {
        (void)unlink(tmp_name);
        ERROR("Failed to save pattern to file: %r", rc);
        return rc;
    }

    rc = rcf_ta_trrecv_start(ta_name, session, handle, 
                             tmp_name, timeout, num, mode);
    unlink(tmp_name);
    if (rc != 0)
        WARN("trrecv_start failed with rc %r", rc);
    return rc;
}


/**
 * Packet handler which parse received packet from file into ASN value
 * and pass it together with user data to user callback.
 *
 * This function complies with rcf_pkt_handler prototype.
 *
 * @param filename      Name of the file with received packet data
 * @param my_data       Pointer to tapi_tad_trrecv_cb_data structure
 */
static void
tapi_tad_trrecv_pkt_handler(const char *filename, void *my_data)
{
    te_errno    rc;
    int         syms = 0;
    asn_value  *packet;

    tapi_tad_trrecv_cb_data *cb_data =
        (tapi_tad_trrecv_cb_data *)my_data;


    /* Parse file in any case to check that it is OK */
    rc = asn_parse_dvalue_in_file(filename, ndn_raw_packet,
                                  &packet, &syms);
    if (rc != 0)
    {
        ERROR("Parse packet from file failed on symbol %d : %r\n%Tf",
              syms, rc, filename);
        return;
    }

    if (cb_data != NULL && cb_data->callback != NULL)
    {
        cb_data->callback(packet, cb_data->user_data);
        /* Packet is owned by callback */
    }
    else
    {
        asn_free_value(packet);
    }
}

/* See the description in tapi_eth.h */
tapi_tad_trrecv_cb_data *
tapi_tad_trrecv_make_cb_data(tapi_tad_trrecv_cb  callback,
                             void               *user_data)
{
    tapi_tad_trrecv_cb_data *res;

    res = (tapi_tad_trrecv_cb_data *)calloc(1, sizeof(*res));
    if (res == NULL)
    {
        ERROR("%s(): failed to allocate memory", __FUNCTION__);
        return NULL;
    }
    res->callback = callback;
    res->user_data = user_data;
    
    return res;
}


/* See description in tapi_tad.h */
te_errno
tapi_tad_trrecv_wait(const char              *ta_name,
                     int                      session,
                     csap_handle_t            handle,
                     tapi_tad_trrecv_cb_data *cb_data,
                     unsigned int            *num)
{
    return rcf_ta_trrecv_wait(ta_name, session, handle,
                              cb_data == NULL ? NULL :
                                  tapi_tad_trrecv_pkt_handler,
                              cb_data, num);
}

/* See description in tapi_tad.h */
te_errno
tapi_tad_trrecv_stop(const char              *ta_name,
                     int                      session,
                     csap_handle_t            handle,
                     tapi_tad_trrecv_cb_data *cb_data,
                     unsigned int            *num)
{
    return rcf_ta_trrecv_stop(ta_name, session, handle,
                              cb_data == NULL ? NULL :
                                  tapi_tad_trrecv_pkt_handler,
                              cb_data, num);
}

/* See description in tapi_tad.h */
te_errno
tapi_tad_trrecv_get(const char              *ta_name,
                    int                      session,
                    csap_handle_t            handle,
                    tapi_tad_trrecv_cb_data *cb_data,
                    unsigned int            *num)
{
    return rcf_ta_trrecv_get(ta_name, session, handle,
                             cb_data == NULL ? NULL :
                                 tapi_tad_trrecv_pkt_handler,
                             cb_data, num);
}


/* Description in tapi_tad.h */
int
tapi_tad_add_iterator_for(asn_value *templ, int begin, int end, int step)
{
    asn_value *iterators;
    asn_value *simple_for;

    char buf[1000] = {0,};
    int rc, syms;

    if (templ == NULL)
        return TE_RC(TE_TAPI, TE_EWRONGPTR);

    rc = asn_get_child_value(templ, (const asn_value **)&iterators,
                                  PRIVATE, NDN_TMPL_ARGS);
    if (TE_RC_GET_ERROR(rc) == TE_EASNINCOMPLVAL)
    {
        iterators = asn_init_value(ndn_template_params_seq);
        rc = asn_put_child_value(templ, iterators,
                                 PRIVATE, NDN_TMPL_ARGS);
    }
    if (rc != 0)
    {
        ERROR("%s(): error of init iterators ASN value: %r",
              __FUNCTION__, rc);
        return TE_RC(TE_TAPI, rc);
    } 

    snprintf(buf, sizeof(buf), "simple-for:{begin %d, end %d, step %d}",
             begin, end, step);
    rc = asn_parse_value_text(buf, ndn_template_parameter, 
                              &simple_for, &syms);
    if (rc != 0)
    {
        ERROR("%s(): parse simple-for failed %r on sym %d", 
              __FUNCTION__, rc, syms);
        return TE_RC(TE_TAPI, rc); 
    }

    rc = asn_insert_indexed(iterators, simple_for, -1, "");
    if (rc != 0)
    {
        ERROR("%s(): insert iterator failed %r", __FUNCTION__, rc);
        return TE_RC(TE_TAPI, rc); 
    }

    return 0;
}



/* Description in tapi_tad.h */
int
tapi_tad_forward_all(const char *ta_name, int session,
                     csap_handle_t csap_rcv, csap_handle_t csap_fwd,
                     asn_value *pattern, 
                     unsigned int timeout, int *forwarded)
{
    int rc;

    if (ta_name == NULL || csap_rcv == CSAP_INVALID_HANDLE ||
        pattern == NULL)
    {
        ERROR("%s(): Invalid input", __FUNCTION__);
        return TE_RC(TE_TAPI, EINVAL);
    } 

    if (csap_fwd != CSAP_INVALID_HANDLE)
    {
        rc = asn_write_int32(pattern, csap_fwd, "0.actions.0.#forw-pld");
        if (rc != 0)
        {
            ERROR("%s():wrіte forward action failed %r", __FUNCTION__, rc);
            return TE_RC(TE_TAPI, rc);
        }
    }

    rc = tapi_tad_trrecv_start(ta_name, session, csap_rcv, pattern,
                               TAD_TIMEOUT_INF, 0, RCF_TRRECV_COUNT);
    if (rc != 0)
    {
        ERROR("%s(%s:%d): trrecv_start failed %r", 
              __FUNCTION__, ta_name, csap_rcv, rc);
        return TE_RC(TE_TAPI, rc);
    } 
    MSLEEP(timeout);

    rc = rcf_ta_trrecv_stop(ta_name, session, csap_rcv, NULL, NULL,
                            forwarded);

    return TE_RC(TE_TAPI, rc);
}
