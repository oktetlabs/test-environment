/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2004-2023 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief Test API for FIO tool
 *
 * Test API to control 'fio' tool.
 */

#define TE_LGR_USER "TAPI FIO"

#include "tapi_fio.h"
#include "tapi_mem.h"
#include "tapi_file.h"
#include "te_alloc.h"
#include "te_units.h"
#include "tapi_test.h"
#include "tapi_cfg_cpu.h"
#include "fio.h"

/** Default path to FIO tool */
#define TAPI_FIO_TOOL_PATH_DEFAULT "fio"

static void
app_init(tapi_fio_app *app, const tapi_fio_opts *opts,
         tapi_job_factory_t *factory, const char *path)
{
    const char *ta = tapi_job_factory_ta(factory);

    app->factory = factory;
    app->running = FALSE;
    app->args = TE_VEC_INIT(char *);
    app->path = (te_string)TE_STRING_INIT;

    te_string_append(&app->path, path == NULL ?
                                 TAPI_FIO_TOOL_PATH_DEFAULT : path);

    if (opts == NULL)
        tapi_fio_opts_init(&app->opts);
    else
        app->opts = *opts;

    if (app->opts.output_path.ptr == NULL)
        tapi_file_make_custom_pathname(&app->opts.output_path, NULL, ".json");

    if (app->opts.numjobs.expr != NULL)
    {
        CHECK_RC(tapi_cfg_cpu_calculate_numjobs(ta, app->opts.numjobs.expr,
                                                &app->opts.numjobs.value));
    }
}

static void
app_fini(tapi_fio_app *app)
{
    te_vec_deep_free(&app->args);
    te_string_free(&app->path);
    tapi_job_destroy(app->job, -1);
}

/* See description in tapi_fio.h */
void
tapi_fio_opts_init(tapi_fio_opts *opts)
{
    *opts = TAPI_FIO_OPTS_DEFAULTS;
}

/* See description in tapi_fio.h */
tapi_fio *
tapi_fio_create(const tapi_fio_opts *options, tapi_job_factory_t *factory,
                const char *path)
{
    tapi_fio *fio;

    fio = TE_ALLOC(sizeof(*fio));
    CHECK_NOT_NULL(fio);

    app_init(&fio->app, options, factory, path);
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

#define REPORT(...)                                         \
    do {                                                    \
        te_errno rc;                                        \
        if ((rc = te_string_append(log, __VA_ARGS__)) != 0) \
            return rc;                                      \
    } while (0)

#define KBYTE2MBIT(_val)    ((_val) * 8 / 1000.)

static te_errno
log_report_bw(te_string *log, const tapi_fio_report_bw *rbw)
{
    REPORT("\tbandwidth\n");
    REPORT("\t\tmin:\t%f MB/sec  %f Mbit/sec\n",
           TE_UNITS_BIN_U2K(rbw->min), KBYTE2MBIT(rbw->min));
    REPORT("\t\tmax:\t%f MB/sec  %f Mbit/sec\n",
           TE_UNITS_BIN_U2K(rbw->max), KBYTE2MBIT(rbw->max));
    REPORT("\t\tmean:\t%f MB/sec  %f Mbit/sec\n",
           TE_UNITS_BIN_U2K(rbw->mean), KBYTE2MBIT(rbw->mean));
    REPORT("\t\tstddev:\t%f MB/sec  %f Mbit/sec\n",
           TE_UNITS_BIN_U2K(rbw->stddev), KBYTE2MBIT(rbw->stddev));

    return 0;
}

static te_errno
log_report_lat(te_string *log, const tapi_fio_report_lat *rlat)
{
    REPORT("\tlatency\n");
    REPORT("\t\tmin:\t%f us\n", TE_NS2US((double)rlat->min_ns));
    REPORT("\t\tmax:\t%f us\n", TE_NS2US((double)rlat->max_ns));
    REPORT("\t\tmean:\t%f us\n", TE_NS2US(rlat->mean_ns));
    REPORT("\t\tstddev:\t%f us\n", TE_NS2US(rlat->stddev_ns));

    return 0;
}

static te_errno
log_report_iops(te_string *log, const tapi_fio_report_iops *riops)
{
    REPORT("\tiops\n");
    REPORT("\t\tmin:\t%d iops \n", riops->min);
    REPORT("\t\tmax:\t%d iops \n", riops->max);
    REPORT("\t\tmean:\t%f iops\n", riops->mean);
    REPORT("\t\tstddev:\t%f iops \n", riops->stddev);

    return 0;
}

static te_errno
log_report_percentile(te_string *log,
                      const tapi_fio_report_percentiles *rpercentile)
{
    REPORT("\tlatency percentiles\n");
    REPORT("\t\t99.00:\t%f us\n",
           TE_NS2US((double)rpercentile->percent_99_00));
    REPORT("\t\t99.50:\t%f us\n",
           TE_NS2US((double)rpercentile->percent_99_50));
    REPORT("\t\t99.90:\t%f us\n",
           TE_NS2US((double)rpercentile->percent_99_90));
    REPORT("\t\t99.95:\t%f us\n",
           TE_NS2US((double)rpercentile->percent_99_95));

    return 0;
}

static te_errno
log_report_clat(te_string *log, const tapi_fio_report_clat *rclat)
{
    te_errno rc;

    REPORT("\tcompletion latency\n");
    REPORT("\t\tmin:\t%f us\n", TE_NS2US((double)rclat->min_ns));
    REPORT("\t\tmax:\t%f us\n", TE_NS2US((double)rclat->max_ns));
    REPORT("\t\tmean:\t%f us\n", TE_NS2US(rclat->mean_ns));
    REPORT("\t\tstddev:\t%f us\n", TE_NS2US(rclat->stddev_ns));

    if ((rc = log_report_percentile(log, &rclat->percentiles)) != 0)
        return rc;

    return 0;
}

#undef REPORT

static te_errno
log_report_io(te_string *log, const tapi_fio_report_io *rio)
{
    te_errno rc;

    if ((rc = log_report_lat(log, &rio->latency)) != 0)
        return rc;

    if ((rc = log_report_bw(log, &rio->bandwidth)) != 0)
        return rc;

    if ((rc = log_report_clat(log, &rio->clatency)) != 0)
        return rc;

    if ((rc = log_report_iops(log, &rio->iops)) != 0)
        return rc;

    return 0;
}

/* See description in tapi_fio.h */
void
tapi_fio_log_report(tapi_fio_report *rp)
{
    te_errno rc;
    te_string report = TE_STRING_INIT;

    te_string_append(&report, "read\n");
    if ((rc = log_report_io(&report, &rp->read)) != 0)
        ERROR("Log report exit with error %r", rc);

    te_string_append(&report, "write\n");
    if ((rc = log_report_io(&report, &rp->write)) != 0)
        ERROR("Log report exit with error %r", rc);

    RING("SHORT FIO REPORT:\n%s", report.ptr);
    te_string_free(&report);
}

te_errno
tapi_fio_mi_report(te_mi_logger *logger, const tapi_fio_report *report)
{
    if (logger == NULL || report == NULL)
        return TE_EINVAL;

    te_mi_logger_add_meas_vec(logger, NULL, TE_MI_MEAS_V(
        TE_MI_MEAS(THROUGHPUT, "Read throughput", MEAN,
                   KBYTE2MBIT(report->read.bandwidth.mean), MEBI),
        TE_MI_MEAS(IOPS, "Read iops", MEAN, report->read.iops.mean, PLAIN),
        TE_MI_MEAS(LATENCY, "Read clat 99.00 percentile", PERCENTILE,
                   report->read.clatency.percentiles.percent_99_00 / 1000, MICRO),
        TE_MI_MEAS(THROUGHPUT, "Write throughput", MEAN,
                   KBYTE2MBIT(report->write.bandwidth.mean), MEBI),
        TE_MI_MEAS(IOPS, "Write iops", MEAN, report->write.iops.mean, PLAIN),
        TE_MI_MEAS(LATENCY, "Write clat 99.00 percentile", PERCENTILE,
                   report->write.clatency.percentiles.percent_99_00 / 1000, MICRO)));

    return 0;
}
