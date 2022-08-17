/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Test API
 *
 * Implementation of Test API for Traffic Application Domain
 * features.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
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
#include "te_str.h"
#include "tad_common.h"

#include "logger_api.h"
#include "rcf_api.h"
#include "conf_api.h"
#include "ndn.h"
#include "ndn_socket.h"

#include "tapi_tad.h"
#include "tapi_ndn.h"

#include "asn_usr.h"

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
                          int64_t *p_llint)
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
    int     rc;
    int64_t tmp;


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
    int     rc;
    int64_t tmp;


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
te_errno
tapi_tad_csap_create(const char *ta_name, int session,
                     const char *stack_id,
                     const asn_value *csap_spec, csap_handle_t *handle)
{
    te_errno    rc;
    char        tmp_name[] = "/tmp/te_tapi_tad_csap_create.XXXXXX";
    char       *stack_id_by_spec = NULL;

    if ((rc = te_make_tmp_file(tmp_name)) != 0)
        return TE_RC(TE_TAPI, rc);

    rc = asn_save_to_file(csap_spec, tmp_name);
    if (rc != 0)
    {
        ERROR("%s(): asn_save_to_file() failed: %r", __FUNCTION__, rc);
        (void)unlink(tmp_name);
        return rc;
    }

    if (stack_id == NULL)
    {
        stack_id_by_spec = ndn_csap_stack_by_spec(csap_spec);
        if (stack_id_by_spec == NULL)
        {
            ERROR("%s(): ndn_csap_stack_by_spec() failed: %r",
            __FUNCTION__, rc);
            (void)unlink(tmp_name);
            return rc;
        }
    }

    rc = rcf_ta_csap_create(ta_name, session,
                            (stack_id == NULL) ? stack_id_by_spec : stack_id,
                            tmp_name, handle);
    if ((rc == 0) &&
        ((rc = cfg_synchronize_fmt(TRUE, "/agent:%s/csap:*",
                                   ta_name)) != 0))
    {
        ERROR("%s(): cfg_synchronize_fmt(/agent:%s/csap:*) failed: %r",
              __FUNCTION__, ta_name, rc);
    }

    free(stack_id_by_spec);
    unlink(tmp_name);
    if (rc != 0)
        ERROR("%s(): CSAP create failed with rc %r", __FUNCTION__, rc);
    return rc;
}

/* Description in tapi_tad.h */
te_errno
tapi_tad_csap_destroy(const char *ta_name, int session,
                      csap_handle_t handle)
{
    te_errno    rc;

    rc = rcf_ta_csap_destroy(ta_name, session, handle);
    if ((rc == 0) &&
        ((rc = cfg_synchronize_fmt(TRUE, "/agent:%s/csap:*",
                                   ta_name)) != 0))
    {
        ERROR("%s(): cfg_synchronize_fmt(/agent:%s/csap:*) failed: %r",
              __FUNCTION__, ta_name, rc);
    }

    return rc;
}

/* Description in tapi_tad.h */
te_errno
tapi_tad_trsend_start(const char *ta_name, int session,
                      csap_handle_t csap, const asn_value *templ,
                      rcf_call_mode_t blk_mode)
{
    te_errno    rc;
    char        tmp_name[] = "/tmp/te_tapi_tad_trsend_start.XXXXXX";

    if ((rc = te_make_tmp_file(tmp_name)) != 0)
        return TE_RC(TE_TAPI, rc);

    rc = asn_save_to_file(templ, tmp_name);
    if (rc != 0)
    {
        ERROR("%s(): asn_save_to_file() failed: %r", __FUNCTION__, rc);
        (void)unlink(tmp_name);
        return rc;
    }

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
                      unsigned int mode)
{
    te_errno    rc = 0;
    char        tmp_name[] = "/tmp/te_tapi_tad_trrecv_start.XXXXXX";
    asn_value  *my_ptrn = NULL;

    if ((rc = te_make_tmp_file(tmp_name)) != 0)
        return TE_RC(TE_TAPI, rc);

    if (pattern == NULL)
    {
        rc = tapi_tad_new_ptrn_unit(&my_ptrn, NULL);
        if (rc != 0)
        {
            ERROR("Failed to create wildcard pattern with single "
                  "pattern unit: %r", rc);
            return rc;
        }
        pattern = my_ptrn;
    }

    rc = asn_save_to_file(pattern, tmp_name);
    if (rc != 0)
    {
        ERROR("Failed to save pattern to file: %r", rc);
        (void)unlink(tmp_name);
        asn_free_value(my_ptrn);
        return rc;
    }

    asn_free_value(my_ptrn);

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

/* See the description in tapi_tad.h */
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
                     unsigned int timeout, unsigned int *forwarded)
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
            ERROR("%s():write forward action failed %r", __FUNCTION__, rc);
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
    usleep(timeout * 1000);

    rc = rcf_ta_trrecv_stop(ta_name, session, csap_rcv, NULL, NULL,
                            forwarded);

    return TE_RC(TE_TAPI, rc);
}


/* See the description in tapi_tad.h */
te_errno
tapi_tad_socket_add_csap_layer(asn_value **csap_spec, int fd)
{
    te_errno    rc;
    asn_value  *layer;

    rc = tapi_tad_csap_add_layer(csap_spec, ndn_socket_csap,
                                 "#socket", &layer);
    if (rc != 0)
    {
        ERROR("Failed to add 'socket' layer in CSAP parameters: %r",
              rc);
        return TE_RC(TE_TAPI, rc);
    }

    rc = asn_write_int32(layer, fd, "type.#file-descr");
    if (rc != 0)
    {
        ERROR("Failed to write file descriptor to 'socket' layer in "
              "CSAP parameters: %r", rc);
        return TE_RC(TE_TAPI, rc);
    }

    return 0;
}

/* See the description in tapi_tad.h */
te_errno
tapi_tad_csap_get_no_match_pkts(const char *ta_name, int session,
                                csap_handle_t csap_id, unsigned int *val)
{
    int         rc;
    int64_t     tmp;

    ENTRY("TA=%s, SID=%d, CSAP=%d, location=0x%08x",
          ta_name, session, csap_id, val);

    rc = tapi_csap_param_get_llint(ta_name, session, csap_id,
                                   CSAP_PARAM_NO_MATCH_PKTS, &tmp);
    if (rc != 0)
    {
        RETURN_RC(rc);
    }

    *val = (unsigned int)tmp;

    RETURN_RC(0);
}

/**
 * Destroy CSAP by its Configurator handle using RCF.
 *
 * @param ta_name         Test Agent name
 * @param session         RCF session ID
 * @param csap_cfg_handle Configurator handle of the given CSAP
 *
 * @note The function will not use Configurator itself for CSAP removal.
 *
 * @return Status code.
 */
static te_errno
tapi_tad_csap_destroy_by_cfg_handle(const char *ta_name,
                                    int         session,
                                    cfg_handle  csap_cfg_handle)
{
    char              *cfg_inst_name;
    csap_handle_t      id;
    te_errno           rc;

    rc = cfg_get_inst_name(csap_cfg_handle, &cfg_inst_name);
    if (rc != 0)
        return rc;

    rc = te_strtoui(cfg_inst_name, 0, &id);
    if (rc == 0)
        rc = rcf_ta_csap_destroy(ta_name, session, id);

    free(cfg_inst_name);

    return rc;
}

/**
 * Destroy all CSAP instances on a Test Agent using RCF.
 *
 * @param ta_cfg_handle Configurator handle of the given Test Agent
 * @param session       RCF session ID
 *
 * @note The function will not use Configurator itself for CSAP removal.
 *
 * @return Status code.
 */
static te_errno
tapi_tad_csap_destroy_all_by_ta(cfg_handle ta_cfg_handle,
                                int        session)
{
    char         *ta_name;
    unsigned int  nb_csap_cfg_handles;
    cfg_handle   *csap_cfg_handles = NULL;
    te_errno      rc;
    unsigned int  i;

    rc = cfg_get_inst_name(ta_cfg_handle, &ta_name);
    if (rc != 0)
        goto out;

    rc = cfg_find_pattern_fmt(&nb_csap_cfg_handles, &csap_cfg_handles,
                              "/agent:%s/csap:*", ta_name);
    if (rc != 0)
        goto out;

    for (i = 0; i < nb_csap_cfg_handles; ++i)
    {
        rc = tapi_tad_csap_destroy_by_cfg_handle(ta_name, session,
                                                 csap_cfg_handles[i]);
        if (rc != 0)
            break;
    }

    if (rc == 0)
        rc = cfg_synchronize_fmt(TRUE, "/agent:%s/csap:*", ta_name);

out:
    free(csap_cfg_handles);
    free(ta_name);

    return rc;
}

/* See description in 'tapi_tad.h' */
te_errno
tapi_tad_csap_destroy_all(int session)
{
    unsigned int  nb_ta_cfg_handles;
    cfg_handle   *ta_cfg_handles;
    te_errno      rc;
    unsigned int  i;

    rc = cfg_find_pattern_fmt(&nb_ta_cfg_handles, &ta_cfg_handles, "/agent:*");
    if (rc != 0)
       return rc;

    for (i = 0; i < nb_ta_cfg_handles; ++i)
    {
        rc = tapi_tad_csap_destroy_all_by_ta(ta_cfg_handles[i], session);
        if (rc != 0)
            break;
    }

    free(ta_cfg_handles);

    return rc;
}

/* See description in tap_tad.h */
te_errno
tapi_tad_get_pkt_rx_ts(asn_value *pkt, struct timeval *tv)
{
    uint32_t secs = 0;
    uint32_t usecs = 0;
    int rc = 0;

    if (pkt == NULL || tv == NULL)
    {
        ERROR("%s(): NULL arguments were passed", __FUNCTION__);
        return TE_EINVAL;
    }

    rc = asn_read_uint32(pkt, &secs, "received.seconds");
    if (rc != 0)
    {
        ERROR("%s(): failed to get seconds from CSAP packet: %r",
              __FUNCTION__, rc);
        return rc;
    }
    tv->tv_sec = secs;

    rc = asn_read_uint32(pkt, &usecs, "received.micro-seconds");
    if (rc != 0)
    {
        ERROR("%s(): failed to get microseconds from CSAP packet: %r",
              __FUNCTION__, rc);
        return rc;
    }
    tv->tv_usec = usecs;

    return 0;
}
