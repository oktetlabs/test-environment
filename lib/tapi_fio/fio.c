/** @file
 * @brief FIO Test API for fio tool routine
 *
 * Test API to control 'fio' tool.
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#define TE_LGR_USER "TAPI FIO"

#include "tapi_test_log.h"
#include "tapi_test.h"
#include "tapi_file.h"
#include "fio_internal.h"
#include "fio.h"

#include <jansson.h>

static te_errno
fio_start(tapi_fio *fio)
{
    te_errno errno;

    return fio_app_start(&fio->app);
}

static te_errno
fio_stop(tapi_fio *fio)
{
    return fio_app_stop(&fio->app);
}

static te_errno
fio_wait(tapi_fio *fio, int16_t timeout_sec)
{
    te_errno errno;

    ENTRY("FIO waiting %d sec", timeout_sec);

    errno = fio_app_wait(&fio->app, timeout_sec);

    EXIT();
    return errno;
}

#define JSON_ERROR(_type, _key)                             \
    do {                                                    \
        ERROR("%s(%d): JSON %s is expected by key %s",      \
              __FUNCTION__, __LINE__, _type, #_key);        \
        return TE_RC(TE_TAPI, TE_EINVAL);                   \
    } while (0)

#define TRY_GET_JSON_OBJ(_json_obj, _key, _type, _result)   \
    do {                                                    \
        json_t *jobj;                                       \
                                                            \
        jobj = json_##_type##_get(_json_obj, _key);         \
        if (!jobj)                                          \
            JSON_ERROR(#_type, _key);                       \
                                                            \
        _result = jobj;                                     \
    } while (0)

#define TRY_GET_JSON_VALUE(_json_obj, _key, _type, _result) \
    do {                                                    \
        json_t *jval;                                       \
                                                            \
        TRY_GET_JSON_OBJ(_json_obj, _key, object, jval);    \
        if (!json_is_##_type(jval))                         \
            JSON_ERROR(#_type, _key);                       \
                                                            \
        _result = json_##_type##_value(jval);               \
    } while(0)

static te_errno
get_bandwidth_report(const json_t *jrpt, tapi_fio_report_bw *bw)
{
    tapi_fio_report_bw temp_report;

    TRY_GET_JSON_VALUE(jrpt, "bw_max", integer, temp_report.max);
    TRY_GET_JSON_VALUE(jrpt, "bw_min", integer, temp_report.min);
    TRY_GET_JSON_VALUE(jrpt, "bw_mean", real, temp_report.mean);
    TRY_GET_JSON_VALUE(jrpt, "bw_dev", real, temp_report.stddev);

    *bw = temp_report;
    return 0;
}

static te_errno
get_latency_report(const json_t *jrpt, tapi_fio_report_lat *lat)
{
    tapi_fio_report_lat temp_report;

    TRY_GET_JSON_VALUE(jrpt, "min", integer, temp_report.min_ns);
    TRY_GET_JSON_VALUE(jrpt, "max", integer, temp_report.max_ns);
    TRY_GET_JSON_VALUE(jrpt, "mean", real, temp_report.mean_ns);
    TRY_GET_JSON_VALUE(jrpt, "stddev", real, temp_report.stddev_ns);

    *lat = temp_report;
    return 0;
}

static te_errno
get_clatency_percentile_report(const json_t *jrpt,
                               tapi_fio_report_percentiles *percentile)
{
    tapi_fio_report_percentiles temp_report;

    TRY_GET_JSON_VALUE(jrpt, "99.000000", integer, temp_report.percent_99_00);
    TRY_GET_JSON_VALUE(jrpt, "99.500000", integer, temp_report.percent_99_50);
    TRY_GET_JSON_VALUE(jrpt, "99.900000", integer, temp_report.percent_99_90);
    TRY_GET_JSON_VALUE(jrpt, "99.950000", integer, temp_report.percent_99_95);

    *percentile = temp_report;
    return 0;
}

static te_errno
get_clatency_report(const json_t *jrpt, tapi_fio_report_clat *clat)
{
    tapi_fio_report_clat temp_report;
    te_errno rc;
    json_t *jperc;

    TRY_GET_JSON_VALUE(jrpt, "min", integer, temp_report.min_ns);
    TRY_GET_JSON_VALUE(jrpt, "max", integer, temp_report.max_ns);
    TRY_GET_JSON_VALUE(jrpt, "mean", real, temp_report.mean_ns);
    TRY_GET_JSON_VALUE(jrpt, "stddev", real, temp_report.stddev_ns);

    jperc = json_object_get(jrpt, "percentile");
    if (jperc != NULL)
    {
        rc = get_clatency_percentile_report(jperc, &temp_report.percentiles);
        if (rc != 0)
            return rc;
    }
    else
    {
        memset(&temp_report.percentiles, 0, sizeof(temp_report.percentiles));
    }

    *clat = temp_report;
    return 0;
}

static te_errno
get_iops_report(const json_t *jrpt, tapi_fio_report_iops *iops)
{
    tapi_fio_report_iops temp_report;

    TRY_GET_JSON_VALUE(jrpt, "iops_min", integer, temp_report.min);
    TRY_GET_JSON_VALUE(jrpt, "iops_max", integer, temp_report.max);
    TRY_GET_JSON_VALUE(jrpt, "iops_mean", real, temp_report.mean);
    TRY_GET_JSON_VALUE(jrpt, "iops_stddev", real, temp_report.stddev);

    *iops = temp_report;
    return 0;
}

static te_errno
get_report_io(const json_t *jrpt, tapi_fio_report_io *rio)
{
    te_errno rc;
    json_t *jlat, *jclat;
    tapi_fio_report_io temp_report;

    if ((rc = get_bandwidth_report(jrpt, &temp_report.bandwidth)) != 0)
        return rc;

    if ((rc = get_iops_report(jrpt, &temp_report.iops)) != 0)
        return rc;

    TRY_GET_JSON_OBJ(jrpt, "lat_ns", object, jlat);
    if ((rc = get_latency_report(jlat, &temp_report.latency)) != 0)
        return rc;

    TRY_GET_JSON_OBJ(jrpt, "clat_ns", object, jclat);
    if ((rc = get_clatency_report(jclat, &temp_report.clatency)) != 0)
        return rc;

    *rio = temp_report;
    return 0;
}

static te_errno
get_report(const json_t *jrpt, tapi_fio_report *report)
{
    te_errno rc;
    json_t *jjobs, *jfirst_job, *jwrite, *jread;
    tapi_fio_report temp_report;

    TRY_GET_JSON_OBJ(jrpt, "jobs", object, jjobs);
    TRY_GET_JSON_OBJ(jjobs, 0, array, jfirst_job);

    TRY_GET_JSON_OBJ(jfirst_job, "write", object, jwrite);
    if ((rc = get_report_io(jwrite, &temp_report.write)) != 0)
        return rc;

    TRY_GET_JSON_OBJ(jfirst_job, "read", object, jread);
    if ((rc = get_report_io(jread, &temp_report.read)) != 0)
        return rc;

    *report = temp_report;
    return 0;
}

static te_errno
fio_get_report(tapi_fio *fio, tapi_fio_report *report)
{
    json_t *jrpt;
    te_errno rc;
    char *json_output = NULL;
    const char *ta;

    ENTRY("FIO get reporting");

    ta = tapi_job_factory_ta(fio->app.factory);
    if (ta == NULL)
        return TE_RC(TE_TAPI, TE_EINVAL);

    rc = tapi_file_read_ta(ta, fio->app.opts.output_path.ptr,
                           &json_output);
    if (rc != 0)
        return rc;


    RING("FIO result.json:\n%s", json_output);
    jrpt = json_loads(json_output, 0, 0);
    if (!json_is_object(jrpt))
    {
        free(json_output);
        ERROR("Cannot parse FIO output");
        EXIT();
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    rc = get_report(jrpt, report);
    json_decref(jrpt);

    free(json_output);
    EXIT();
    return rc;
}

tapi_fio_methods methods = {
    .start = fio_start,
    .stop = fio_stop,
    .wait = fio_wait,
    .get_report = fio_get_report,
};
