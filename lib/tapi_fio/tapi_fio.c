/** @file
 * @brief Test API for FIO tool
 *
 * Test API to control 'fio' tool.
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * @author Nikita Somenkov <Nikita.Somenkov@oktetlabs.ru>
 */

#include "tapi_fio.h"
#include "tapi_mem.h"
#include "te_alloc.h"
#include "tapi_test.h"
#include "tapi_rpc_unistd.h"
#include "fio.h"

static void
numjobs_transform(rcf_rpc_server *rpcs, tapi_fio_numjobs_t *numjobs)
{
    if (numjobs->factor != TAPI_FIO_NUMJOBS_NPROC_FACTOR)
        return;

    numjobs->value *= rpc_sysconf(rpcs, RPC_SC_NPROCESSORS_ONLN);
    numjobs->factor = TAPI_FIO_NUMJOBS_WITHOUT_FACTOR;
}

static void
app_init(tapi_fio_app *app, const tapi_fio_opts *opts, rcf_rpc_server *rpcs)
{
    app->rpcs = rpcs;
    app->pid = -1;
    app->fd_stdout = -1;
    app->fd_stderr = -1;
    app->cmd = NULL;
    app->stdout = (te_string)TE_STRING_INIT;
    app->stderr = (te_string)TE_STRING_INIT;

    if (opts == NULL)
        tapi_fio_opts_init(&app->opts);
    else
        app->opts = *opts;

    numjobs_transform(app->rpcs, &app->opts.numjobs);
}

static void
app_fini(tapi_fio_app *app)
{
    free(app->cmd);
    te_string_free(&app->stdout);
    te_string_free(&app->stderr);
}

/* See description in tapi_fio.h */
void
tapi_fio_opts_init(tapi_fio_opts *opts)
{
    *opts = TAPI_FIO_OPTS_DEFAULTS;
}

/* See description in tapi_fio.h */
tapi_fio *
tapi_fio_create(const tapi_fio_opts *options, rcf_rpc_server *rpcs)
{
    tapi_fio *fio;

    fio = TE_ALLOC(sizeof(*fio));
    CHECK_NOT_NULL(fio);

    app_init(&fio->app, options, rpcs);
    fio->methods = &methods;

    return fio;
}

/* See description in tapi_fio.h */
void
tapi_fio_destroy(tapi_fio *fio)
{
    if (fio == NULL)
        return;

    tapi_fio_stop(fio);
    app_fini(&fio->app);
    free(fio);
}

/* See description in tapi_fio.h */
te_errno
tapi_fio_start(tapi_fio *fio)
{
    if (fio == NULL ||
        fio->methods == NULL ||
        fio->methods->start == NULL)
        return TE_RC(TE_TAPI, TE_EOPNOTSUPP);

    return fio->methods->start(fio);

}

/* See description in tapi_fio.h */
te_errno
tapi_fio_wait(tapi_fio *fio, int16_t timeout_sec)
{
    if (fio == NULL ||
        fio->methods == NULL ||
        fio->methods->wait == NULL)
        return TE_RC(TE_TAPI, TE_EOPNOTSUPP);

    return fio->methods->wait(fio, timeout_sec);
}

/* See description in tapi_fio.h */
te_errno
tapi_fio_get_report(tapi_fio *fio, tapi_fio_report *report)
{
    if (fio == NULL ||
        fio->methods == NULL ||
        fio->methods->get_report == NULL)
        return TE_RC(TE_TAPI, TE_EOPNOTSUPP);

    return fio->methods->get_report(fio, report);
}

/* See description in tapi_fio.h */
te_errno
tapi_fio_stop(tapi_fio *fio)
{
    if (fio == NULL ||
        fio->methods == NULL ||
        fio->methods->stop  == NULL)
        return TE_RC(TE_TAPI, TE_EOPNOTSUPP);

    return fio->methods->stop(fio);
}

/* See description in tapi_fio.h */
void
tapi_fio_log_report(tapi_fio_report *report)
{
    RING("FIO REPORT: threads=%d bandwidth=%f latency=%f",
         report->threads, report->bandwidth, report->latency);
}

/* See description in tapi_fio.h */
tapi_fio_numjobs_t
test_get_fio_numjobs_param(int argc, char **argv, const char *name)
{
    char *end_ptr = NULL;
    const char *str_val = NULL;
    unsigned long value = 0;
    tapi_fio_numjobs_t result = {
        .value = 0,
        .factor = TAPI_FIO_NUMJOBS_WITHOUT_FACTOR
    };

    str_val = test_get_param(argc, argv, name);
    CHECK_NOT_NULL(str_val);

    value = strtoul(str_val, &end_ptr, 0);

    if (strcmp(end_ptr, "nproc") == 0)
        result.factor = TAPI_FIO_NUMJOBS_NPROC_FACTOR;
    else if (*end_ptr != '\0')
        TEST_FAIL("Failed to convert '%s' to a numjobs", str_val);

    if (value > UINT_MAX) {
        TEST_FAIL("'%s' parameter value is greater"
                  " than %u and cannot be stored in"
                  " an 'unsigned int' variable",
                  name, UINT_MAX);
    }

    result.value = str_val != end_ptr ? (unsigned)value : 1;

    return result;
}
