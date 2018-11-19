/** @file
 * @brief FIO Test API for fio tool routine
 *
 * Test API to control 'fio' tool.
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * @author Nikita Somenkov <Nikita.Somenkov@oktetlabs.ru>
 */


#include "tapi_test_log.h"
#include "tapi_test.h"
#include "tapi_rpc_misc.h"
#include "tapi_file.h"
#include "fio_internal.h"
#include "fio.h"

#include <jansson.h>

typedef void (*set_opt_t)(te_string *, const tapi_fio_opts *);

static const char *
fio_ioengine_str(tapi_fio_ioengine ioengine)
{
    static const char *ioengines[] = {
        [TAPI_FIO_IOENGINE_LIBAIO] = "libaio",
        [TAPI_FIO_IOENGINE_PSYNC] = "psync",
        [TAPI_FIO_IOENGINE_SYNC] = "sync",
        [TAPI_FIO_IOENGINE_POSIXAIO] = "posixaio"
    };
    if (ioengine < 0 || ioengine > TE_ARRAY_LEN(ioengines))
        return NULL;
    return ioengines[ioengine];
}

static const char*
fio_rwtype_str(tapi_fio_rwtype rwtype)
{
    static const char *rwtypes[] = {
        [TAPI_FIO_RWTYPE_SEQ] = "rw",
        [TAPI_FIO_RWTYPE_RAND] = "randrw"
    };
    if (rwtype < 0 || rwtype > TE_ARRAY_LEN(rwtypes))
        return NULL;
    return rwtypes[rwtype];
}

static void
set_opt_name(te_string *cmd, const tapi_fio_opts *opts)
{
    const char *name = opts->name ? opts->name: FIO_DEFAULT_NAME;
    CHECK_RC(te_string_append(cmd, " --name=%s", name));
}

static void
set_opt_filename(te_string *cmd, const tapi_fio_opts *opts)
{
    if (opts->filename == NULL)
        TEST_FAIL("Filename must be specify");
    CHECK_RC(te_string_append(cmd, " --filename=%s", opts->filename));
}

static void
set_opt_iodepth(te_string *cmd, const tapi_fio_opts *opts)
{
    CHECK_RC(te_string_append(cmd, " --iodepth=%d", opts->iodepth));
}

static void
set_opt_runtime(te_string *cmd, const tapi_fio_opts *opts)
{
    if (opts->runtime_sec < 0)
        return;
    CHECK_RC(te_string_append(cmd, " --runtime=%ds --time_based",
                              opts->runtime_sec));
}

static void
set_opt_rwmixread(te_string *cmd, const tapi_fio_opts *opts)
{
    CHECK_RC(te_string_append(cmd, " --rwmixread=%d", opts->rwmixread));
}

static void
set_opt_rwtype(te_string *cmd, const tapi_fio_opts *opts)
{
    const char *str_rwtype = fio_rwtype_str(opts->rwtype);
    if (str_rwtype == NULL)
        return;
    CHECK_RC(te_string_append(cmd, " --readwrite=%s", str_rwtype));
}

static void
set_opt_ioengine(te_string *cmd, const tapi_fio_opts *opts)
{
    const char *ioengine = fio_ioengine_str(opts->ioengine);
    if (ioengine == NULL)
        TEST_FAIL("I/O Engine not supported");
    CHECK_RC(te_string_append(cmd, " --ioengine=%s", ioengine));
}

static void
set_opt_blocksize(te_string *cmd, const tapi_fio_opts *opts)
{
    CHECK_RC(te_string_append(cmd, " --blocksize=%d", opts->blocksize));
}

static void
set_opt_numjobs(te_string *cmd, const tapi_fio_opts *opts)
{
    CHECK_RC(te_string_append(
        cmd, " --numjobs=%d --thread", opts->numjobs.value));
}

static void
set_opt_output(te_string *cmd, const tapi_fio_opts *opts)
{
    CHECK_RC(te_string_append(cmd, " --output-format=json --group_reporting"
                                   " --output=%s", opts->output_path.ptr));
}

static void
set_opt_user(te_string *cmd, const tapi_fio_opts *opts)
{
    if (opts->user == NULL)
        return;
    CHECK_RC(te_string_append(cmd, " %s", opts->user));
}

static void
set_opt_random_generator(te_string *cmd, const tapi_fio_opts *opts)
{
    size_t i;
    static const char *generators[] = {
        "lfsr",
        "tausworthe",
        "tausworthe64",
    };

    if (opts->rand_gen == NULL)
        return;

    for (i = 0; i < TE_ARRAY_LEN(generators); i++)
    {
        if (strcmp(opts->rand_gen, generators[i]) == 0)
        {
            CHECK_RC(te_string_append(cmd, " --random_generator=%s",
                                      opts->rand_gen));
            return;
        }
    }

    TEST_FAIL("Random generator '%s' is not supported", opts->rand_gen);
}

static void
set_opt_generic(te_string *cmd, const tapi_fio_opts *opts)
{
    UNUSED(opts);

    CHECK_RC(te_string_append(cmd, " --direct=%d", opts->direct ? 1: 0));
    CHECK_RC(te_string_append(cmd, " --exitall_on_error=%d",
                              opts->exit_on_error ? 1: 0));
}

static void
build_command(te_string *cmd, const tapi_fio_opts *opts)
{
    size_t i;
    set_opt_t set_opt[] = {
        set_opt_name,
        set_opt_filename,
        set_opt_ioengine,
        set_opt_blocksize,
        set_opt_numjobs,
        set_opt_iodepth,
        set_opt_runtime,
        set_opt_rwmixread,
        set_opt_rwtype,
        set_opt_output,
        set_opt_random_generator,
        set_opt_generic,
        set_opt_user,
    };

    CHECK_RC(te_string_append(cmd, "fio"));
    for (i = 0; i < TE_ARRAY_LEN(set_opt); i++)
        set_opt[i](cmd, opts);
}

static te_errno
fio_start(tapi_fio *fio)
{
    te_errno errno;
    te_string cmd = TE_STRING_INIT;

    ENTRY("FIO starting on %s", RPC_NAME(fio->app.rpcs));

    build_command(&cmd, &fio->app.opts);
    errno = fio_app_start(cmd.ptr, &fio->app);

    EXIT();
    return errno;
}

static te_errno
fio_stop(tapi_fio *fio)
{
    te_errno errno;

    ENTRY("FIO stopping on %s", RPC_NAME(fio->app.rpcs));

    errno = fio_app_stop(&fio->app);

    EXIT();
    return errno;
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
get_report_io(const json_t *jrpt, tapi_fio_report_io *rio)
{
    te_errno rc;
    json_t *jlat;
    tapi_fio_report_io temp_report;

    if ((rc = get_bandwidth_report(jrpt, &temp_report.bandwidth)) != 0)
        return rc;

    TRY_GET_JSON_OBJ(jrpt, "lat_ns", object, jlat);
    if ((rc = get_latency_report(jlat, &temp_report.latency)) != 0)
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

    ENTRY("FIO get reporting");

    rpc_read_fd2te_string(fio->app.rpcs, fio->app.fd_stdout,
                          TAPI_FIO_MAX_REPORT, 0,
                          &fio->app.stdout);

    rpc_read_fd2te_string(fio->app.rpcs, fio->app.fd_stderr,
                          TAPI_FIO_MAX_REPORT, 0,
                          &fio->app.stderr);

    RING("FIO stdout:\n%s", fio->app.stdout.ptr);
    RING("FIO stderr:\n%s", fio->app.stderr.ptr);

    rc = tapi_file_read_ta(fio->app.rpcs->ta,
                           fio->app.opts.output_path.ptr,
                           &json_output);

    if (rc != 0)
        return TE_EFAIL;

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
