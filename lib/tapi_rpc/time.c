/** @file
 * @brief Test API - RPC
 *
 * Implementation of RPC calls and auxiliary functions related
 * to time.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 */

#include "te_config.h"

#include <stdio.h>
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include "te_defs.h"
#include "tapi_rpc_internal.h"
#include "tapi_rpc_time.h"
#include "tapi_test.h"
#include "tarpc.h"

#include "log_bufs.h"

/* See description in tapi_rpc_time.h */
const char *
tarpc_timeval2str(const struct tarpc_timeval *tv)
{

/* Number of buffers used in the function */
#define N_BUFS 10
#define BUF_SIZE 32

    static char buf[N_BUFS][BUF_SIZE];
    static char (*cur_buf)[BUF_SIZE] = (char (*)[BUF_SIZE])buf[0];

    char *ptr;

    /*
     * Firt time the function is called we start from the second buffer, but
     * then after a turn we'll use all N_BUFS buffer.
     */
    if (cur_buf == (char (*)[BUF_SIZE])buf[N_BUFS - 1])
        cur_buf = (char (*)[BUF_SIZE])buf[0];
    else
        cur_buf++;

    ptr = *cur_buf;

    if (tv == NULL)
    {
        snprintf(ptr, BUF_SIZE, "(nil)");
    }
    else
    {
        snprintf(ptr, BUF_SIZE, "{%ld,%ld}",
                 (long)tv->tv_sec, (long)tv->tv_usec);
    }

#undef BUF_SIZE
#undef N_BUFS

    return ptr;
}

/* See description in tapi_rpc_time.h */
const char *
timespec2str(const struct timespec *tv)
{
    static char buf[32];

    if (tv == NULL)
    {
        strcpy(buf, "(nil)");
    }
    else
    {
        sprintf(buf, "{%lld,%lld}", (long long)tv->tv_sec,
                (long long)tv->tv_nsec);
    }
    return buf;
}

/* See description in tapi_rpc_time.h */
const char *
tarpc_timespec2str(const struct tarpc_timespec *tv)
{
    static char buf[32];

    if (tv == NULL)
    {
        strcpy(buf, "(nil)");
    }
    else
    {
        sprintf(buf, "{%lld,%lld}", (long long)tv->tv_sec,
                (long long)tv->tv_nsec);
    }
    return buf;
}

/* See description in tapi_rpc_time.h */
const char *
tarpc_hwtstamp_config2str(const tarpc_hwtstamp_config *hw_cfg)
{
    static char buf[256];

    if (hw_cfg == NULL)
    {
        strcpy(buf, "(nil)");
    }
    else
    {
        TE_SPRINTF(buf, "{ .flags=0x%x, .tx_type=%s, .rx_filter=%s }",
                   hw_cfg->flags, hwtstamp_tx_types_rpc2str(hw_cfg->tx_type),
                   hwtstamp_rx_filters_rpc2str(hw_cfg->rx_filter));
    }

    return buf;
}

/* See description in tapi_rpc_time.h */
int
rpc_gettimeofday(rcf_rpc_server *rpcs,
                 tarpc_timeval *tv, tarpc_timezone *tz)
{
    tarpc_gettimeofday_in  in;
    tarpc_gettimeofday_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(gettimeofday, -1);
    }

    if (tv != NULL)
    {
        in.tv.tv_len = 1;
        in.tv.tv_val = tv;
    }
    if (tz != NULL)
    {
        in.tz.tz_len = 1;
        in.tz.tz_val = tz;
    }

    rcf_rpc_call(rpcs, "gettimeofday", &in, &out);

    if (RPC_IS_CALL_OK(rpcs))
    {
        if (tv != NULL && out.tv.tv_val != NULL)
        {
            tv->tv_sec  = out.tv.tv_val->tv_sec;
            tv->tv_usec = out.tv.tv_val->tv_usec;
        }
        if (tz != NULL && out.tz.tz_val != NULL)
        {
            tz->tz_minuteswest = out.tz.tz_val->tz_minuteswest;
            tz->tz_dsttime     = out.tz.tz_val->tz_dsttime;
        }
    }

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(gettimeofday, out.retval);
    TAPI_RPC_LOG(rpcs, gettimeofday, "%p %p", "%d tv=%s tz={%d,%d}",
                 tv, tz, out.retval,
                 tarpc_timeval2str(tv),
                 (out.retval != 0 || tz == NULL) ? 0 :
                     (int)tz->tz_minuteswest,
                 (out.retval != 0 || tz == NULL) ? 0 :
                     (int)tz->tz_dsttime);
    RETVAL_INT(gettimeofday, out.retval);
}

/* Append string representation of clock ID to TE string */
static te_errno
append_clock_id(te_string *str, tarpc_clock_id_type id_type, int id)
{
    if (id_type == TARPC_CLOCK_ID_NAMED)
    {
        return te_string_append(str, "clock_id=%s", clock_id_rpc2str(id));
    }
    else
    {
        return te_string_append(str, "clock_fd=%d", id);
    }
}

/* See description in tapi_rpc_sys_time.h */
int
rpc_clock_gettime(rcf_rpc_server *rpcs,
                  tarpc_clock_id_type id_type,
                  int id, tarpc_timespec *ts)
{
    tarpc_clock_gettime_in in;
    tarpc_clock_gettime_out out;

    te_string params_str = TE_STRING_INIT_STATIC(1024);

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(clock_gettime, -1);
    }

    in.id_type = id_type;
    in.id = id;

    rcf_rpc_call(rpcs, "clock_gettime", &in, &out);

    if (RPC_IS_CALL_OK(rpcs) && ts != NULL)
    {
        ts->tv_sec = out.ts.tv_sec;
        ts->tv_nsec = out.ts.tv_nsec;
    }

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(clock_gettime, out.retval);

    append_clock_id(&params_str, id_type, id);
    TAPI_RPC_LOG(rpcs, clock_gettime, "%s", "%d ts=%s",
                 params_str.ptr, out.retval, tarpc_timespec2str(&out.ts));

    RETVAL_INT(clock_gettime, out.retval);
}

/* See description in tapi_rpc_time.h */
int
rpc_clock_settime(rcf_rpc_server *rpcs,
                  tarpc_clock_id_type id_type,
                  int id, const tarpc_timespec *ts)
{
    tarpc_clock_settime_in in;
    tarpc_clock_settime_out out;

    te_string params_str = TE_STRING_INIT_STATIC(1024);

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(clock_settime, -1);
    }
    if (ts == NULL)
    {
        ERROR("%s(): ts cannot be NULL", __FUNCTION__);
        rpcs->_errno = TE_RC(TE_TAPI, TE_EINVAL);
        RETVAL_INT(clock_settime, -1);
    }

    in.id_type = id_type;
    in.id = id;
    in.ts.tv_sec = ts->tv_sec;
    in.ts.tv_nsec = ts->tv_nsec;

    rcf_rpc_call(rpcs, "clock_settime", &in, &out);

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(clock_settime, out.retval);

    append_clock_id(&params_str, id_type, id);
    TAPI_RPC_LOG(rpcs, clock_settime, "%s, ts=%s", "%d",
                 params_str.ptr, tarpc_timespec2str(ts), out.retval);

    RETVAL_INT(clock_settime, out.retval);
}

/* See description in tapi_rpc_time.h */
int
rpc_clock_adjtime(rcf_rpc_server *rpcs,
                  tarpc_clock_id_type id_type,
                  int id, tarpc_timex *params)
{
    tarpc_clock_adjtime_in in;
    tarpc_clock_adjtime_out out;

    te_string *params_str = NULL;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(clock_adjtime, -1);
    }
    if (params == NULL)
    {
        ERROR("%s(): params cannot be NULL", __FUNCTION__);
        rpcs->_errno = TE_RC(TE_TAPI, TE_EINVAL);
        RETVAL_INT(clock_adjtime, -1);
    }

    params_str = te_log_str_alloc();
    if (params_str == NULL)
    {
        ERROR("%s(): failed to allocate logging buffer", __FUNCTION__);
        rpcs->_errno = TE_RC(TE_TAPI, TE_ENOMEM);
        RETVAL_INT(clock_adjtime, -1);
    }

    in.id_type = id_type;
    in.id = id;
    memcpy(&in.params, params, sizeof(*params));

    rcf_rpc_call(rpcs, "clock_adjtime", &in, &out);

    if (RPC_IS_CALL_OK(rpcs))
        memcpy(params, &out.params, sizeof(*params));

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(clock_adjtime, out.retval);

    append_clock_id(params_str, id_type, id);
    te_string_append(params_str, ", ");
    timex_tarpc2te_str(params, params_str);

    TAPI_RPC_LOG(rpcs, clock_adjtime, "%s", "%d", te_string_value(params_str),
                 out.retval);

    te_string_free(params_str);
    RETVAL_INT(clock_adjtime, out.retval);
}
