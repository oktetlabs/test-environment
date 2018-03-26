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
#include "fio_internal.h"
#include "fio.h"

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
    if (opts->name == NULL)
        TEST_FAIL("Test must be specified");
    CHECK_RC(te_string_append(cmd, " --name=%s", opts->name));
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
set_opt_user(te_string *cmd, const tapi_fio_opts *opts)
{
    if (opts->user == NULL)
        return;
    CHECK_RC(te_string_append(cmd, " %s", opts->user));
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
        set_opt_user
    };

    CHECK_RC(te_string_append(cmd, "fio"));
    for (i = 0; i < TE_ARRAY_LEN(set_opt); i++)
        set_opt[i](cmd, opts);
    CHECK_RC(te_string_append(cmd, " --output-format=json"));
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

static te_errno
fio_fake_get_report(tapi_fio *fio, tapi_fio_report *report)
{
    UNUSED(fio);
    /* TODO: talk with Mikhail */
    ENTRY("FIO get reporting");

    report->bandwidth = 0.0;
    report->latency = 0.0;
    report->threads = 0;

    rpc_read_fd2te_string(fio->app.rpcs, fio->app.fd_stdout,
                          100, 0,  &fio->app.stdout);

    rpc_read_fd2te_string(fio->app.rpcs, fio->app.fd_stderr,
                          100, 0,  &fio->app.stderr);

    RING("CMD stdout:\n%s", fio->app.stdout.ptr);
    RING("CMD stderr:\n%s", fio->app.stderr.ptr);

    EXIT();
    return 0;
}

tapi_fio_methods methods = {
    .start = fio_start,
    .stop = fio_stop,
    .wait = fio_wait,
    .get_report = fio_fake_get_report,
};