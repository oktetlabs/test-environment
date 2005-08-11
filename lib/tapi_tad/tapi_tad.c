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

#include "rcf_api.h"

#include "tapi_tad.h"
#include "ndn.h"

#define SEC_USEC_SEPARATOR  '.'

#define TE_LGR_USER     "TAPI TAD"
#include "logger_api.h"

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
        ERROR("Failed(%d) to get CSAP #%d parameter '%s' from "
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
        WARN("Csap create failed with rc %X", rc);
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
        WARN("trsend_start failed with rc %X", rc);
    return rc;
}

/* Description in tapi_tad.h */
int 
tapi_tad_trrecv_start(const char *ta_name, int session, int handle, 
                      const asn_value *pattern, rcf_pkt_handler handler, 
                      void *user_param, unsigned int timeout, int num)
{
    int rc = 0;
    char tmp_name[] = "/tmp/te_tapi_tad_trsend_start.XXXXXX";

    if ((rc = te_make_tmp_file(tmp_name)) != 0)
        return TE_RC(TE_TAPI, rc);

    asn_save_to_file(pattern, tmp_name); 

    rc = rcf_ta_trrecv_start(ta_name, session, handle, 
                             tmp_name, handler, user_param, 
                             timeout, num);
    unlink(tmp_name);
    if (rc != 0)
        WARN("trrecv_start failed with rc %X", rc);
    return rc;
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
    if (rc == TE_EASNINCOMPLVAL)
    {
        iterators = asn_init_value(ndn_template_params_seq);
        rc = asn_put_child_value(templ, iterators,
                                 PRIVATE, NDN_TMPL_ARGS);
    }
    if (rc != 0)
    {
        ERROR("%s(): error of init iterators ASN value: %X",
              __FUNCTION__, rc);
        return TE_RC(TE_TAPI, rc);
    } 

    snprintf(buf, sizeof(buf), "simple-for:{begin %d, end %d, step %d}",
             begin, end, step);
    rc = asn_parse_value_text(buf, ndn_template_parameter, 
                              &simple_for, &syms);
    if (rc != 0)
    {
        ERROR("%s(): parse simple-for failed %X on sym %d", 
              __FUNCTION__, rc, syms);
        return TE_RC(TE_TAPI, rc); 
    }

    rc = asn_insert_indexed(iterators, simple_for, -1, "");
    if (rc != 0)
    {
        ERROR("%s(): insert iterator failed %X", __FUNCTION__, rc);
        return TE_RC(TE_TAPI, rc); 
    }

    return 0;
}


