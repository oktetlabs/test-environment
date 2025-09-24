/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Test API for SPDK iostat.py tool routine
 *
 * Test API to control SPDK 'iostat.py' tool.
 *
 * Copyright (C) 2025 OKTET Labs Ltd. All rights reserved.
 */

#define TE_LGR_USER "TAPI SPDK IOSTAT"

#include <stddef.h>

#include "tapi_spdk_iostat.h"
#include "te_alloc.h"
#include "te_str.h"
#include "te_string.h"
#include "logger_api.h"
#include "te_enum.h"
#include "tapi_test_log.h"

typedef struct tapi_spdk_iostat_app {
    tapi_job_t         *job; /**< Job handle */
    char               *rpc_path;
    tapi_job_channel_t *out_chs[2];
    tapi_job_channel_t *filter;
} tapi_spdk_iostat_app;

const tapi_spdk_iostat_opt tapi_spdk_iostat_default_opt = {.server = NULL,
                                                           .bdev_name = NULL,
                                                           .extended = false,
                                                           .verbose = false};

/** Server option bindings */
static const tapi_job_opt_bind iostat_binds[] = TAPI_JOB_OPT_SET(
    TAPI_JOB_OPT_STRING("-s", false, tapi_spdk_iostat_opt, server),
    TAPI_JOB_OPT_STRING("-b", false, tapi_spdk_iostat_opt, bdev_name),
    TAPI_JOB_OPT_BOOL("-x", tapi_spdk_iostat_opt, extended),
    TAPI_JOB_OPT_BOOL("-v", tapi_spdk_iostat_opt, verbose)
);

static te_errno
attach_filter(tapi_spdk_iostat_app *app)
{
    te_string re_buf = TE_STRING_INIT;
    te_errno  rc;

    te_string_append(&re_buf,
                     "^(\\S+)\\s+([\\d.]+)\\s+([\\d.]+)\\s+([\\d.]+)"
                     "\\s+([\\d.]+)\\s+([\\d.]+)\\s+([\\d.]+)\\s+([\\d.]+)");

    rc = tapi_job_attach_filter(TAPI_JOB_CHANNEL_SET(app->out_chs[0]),
                                "Stat-filter", true, 0, &app->filter);
    if (rc != 0)
        return rc;

    return tapi_job_filter_add_regexp(app->filter, te_string_value(&re_buf),
                                      0);
}


static te_errno
create_iostat_job(tapi_job_factory_t *factory, const char *iostat_path,
                  const tapi_spdk_iostat_opt *opt, tapi_spdk_iostat_app **app)
{
    te_vec                args = TE_VEC_INIT(char *);
    te_errno              rc;
    tapi_spdk_iostat_app *result = NULL;

    result = TE_ALLOC(sizeof(*result));

    rc = tapi_job_opt_build_args(iostat_path, iostat_binds, opt, &args);
    if (rc != 0)
    {
        ERROR("Failed to build iostat arguments: %r", rc);
        return rc;
    }


    rc = tapi_job_simple_create(
        factory,
        &(tapi_job_simple_desc_t){.program = iostat_path,
                                  .argv = (const char **)args.data.ptr,
                                  .job_loc = &result->job,
                                  .stdout_loc = &result->out_chs[0],
                                  .stderr_loc = &result->out_chs[1],
                                  .filters = TAPI_JOB_SIMPLE_FILTERS(
                                      {
                                          .use_stderr = true,
                                          .log_level = TE_LL_ERROR,
                                          .readable = true,
                                          .filter_name = "stderr",
                                      },
                                      {
                                          .use_stdout = true,
                                          .log_level = TE_LL_INFO,
                                          .readable = true,
                                          .filter_name = "stdout",
                                      })});
    if (rc != 0)
    {
        ERROR("Failed to create RPC job: %r", rc);
        if (result->job != NULL)
            tapi_job_destroy(result->job, 0);
        te_vec_deep_free(&args);
        free(result);
        return rc;
    }

    rc = attach_filter(result);
    if (rc != 0)
    {
        ERROR("Failed to attach a new filter");
        if (result->job != NULL)
            tapi_job_destroy(result->job, 0);
        te_vec_deep_free(&args);
        free(result);
        return rc;
    }

    *app = result;
    return 0;
}

#define EXPECTED_FIELDS_NUM_IN_REPORT 8

static te_errno
add_device_stat(const char *line, tapi_spdk_iostat_report_t *report)
{
    tapi_spdk_iostat_dev_report_t dev_report;
    int                           fields;
    double                        tps;
    double                        kb_read_s, kb_wrtn_s, kb_dscd_s;
    double                        kb_read, kb_wrtn, kb_dscd;

    fields = sscanf(line, "%63s %lf %lf %lf %lf %lf %lf %lf", dev_report.name,
                    &tps, &kb_read_s, &kb_wrtn_s, &kb_dscd_s, &kb_read,
                    &kb_wrtn, &kb_dscd);

    if (fields != EXPECTED_FIELDS_NUM_IN_REPORT)
    {
        ERROR("Failed to parse iostat output");
        return TE_EFAIL;
    }

    dev_report.kb_read_s = te_unit_bin_pack(TE_UNITS_BIN_K2U(kb_read_s));
    dev_report.kb_wrtn_s = te_unit_bin_pack(TE_UNITS_BIN_K2U(kb_wrtn_s));
    dev_report.kb_dscd_s = te_unit_bin_pack(TE_UNITS_BIN_K2U(kb_dscd_s));
    dev_report.kb_read = te_unit_bin_pack(TE_UNITS_BIN_K2U(kb_read));
    dev_report.kb_wrtn = te_unit_bin_pack(TE_UNITS_BIN_K2U(kb_wrtn));
    dev_report.kb_dscd = te_unit_bin_pack(TE_UNITS_BIN_K2U(kb_dscd));

    TE_VEC_APPEND(&report->devices, dev_report);
    return 0;
}

static te_errno
get_report(tapi_spdk_iostat_app *app, tapi_spdk_iostat_report_t *report)
{
    te_errno           rc;
    tapi_job_buffer_t *bufs = NULL;
    unsigned           bufs_n = 0;
    unsigned           num;

    rc = tapi_job_receive_many(TAPI_JOB_CHANNEL_SET(app->filter), 10000,
                               &bufs, &bufs_n);
    if (rc != 0)
    {
        ERROR("Failed to read data from filter: %r", rc);
        return rc;
    }

    for (num = 0; num < bufs_n; num++)
    {
        if (bufs[num].eos)
            break;

        add_device_stat(bufs[num].data.ptr, report);
    }

    return 0;
}

te_errno
tapi_spdk_iostat(tapi_job_factory_t *factory, const char *iostat_path,
                 const tapi_spdk_iostat_opt *opt,
                 tapi_spdk_iostat_report_t  *report)
{
    tapi_spdk_iostat_app *app = NULL;
    tapi_job_status_t     status;
    te_errno              rc;

    rc = create_iostat_job(factory, iostat_path, opt, &app);
    if (rc != 0)
        return rc;

    rc = tapi_job_start(app->job);
    if (rc != 0)
    {
        ERROR("Failed to start iostat job: %r", rc);
        goto out;
    }

    rc = tapi_job_wait(app->job, -1, &status);
    if (rc != 0)
    {
        ERROR("Failed to wait for RPC command completion: %r", rc);
        goto out;
    }

    if (status.type != TAPI_JOB_STATUS_EXITED || status.value != 0)
    {
        rc = TE_RC(TE_TAPI, TE_ESHCMD);
        goto out;
    }

    if (opt->extended)
    {
        ERROR("Report for iostat extended is not supported");
        rc = TE_RC(TE_TAPI, TE_EOPNOTSUPP);
        goto out;
    }

    report->devices = (te_vec)TE_VEC_INIT(tapi_spdk_iostat_dev_report_t);
    rc = get_report(app, report);
    if (rc != 0)
    {
        ERROR("Failed to get output");
        goto out;
    }

out:
    if (app->job != NULL)
    {
        te_errno rc2;

        rc2 = tapi_job_destroy(app->job, -1);
        if (rc2 != 0)
        {
            ERROR("Failed to destroy iostat job: %r", rc2);
            rc = rc2;
        }
    }
    free(app);

    return rc;
}

void
tapi_spdk_iostat_get_diff_report(tapi_spdk_iostat_report_t *first_report,
                                 tapi_spdk_iostat_report_t *second_report,
                                 tapi_spdk_iostat_report_t *diff_report)
{
    tapi_spdk_iostat_dev_report_t *iter1;
    tapi_spdk_iostat_dev_report_t *iter2;

    diff_report->devices = (te_vec)TE_VEC_INIT(tapi_spdk_iostat_dev_report_t);

    TE_VEC_FOREACH(&first_report->devices, iter1)
    {
        TE_VEC_FOREACH(&second_report->devices, iter2)
        {
            if (strcmp(iter1->name, iter2->name) == 0)
            {
                tapi_spdk_iostat_dev_report_t diff;

                strcpy(diff.name, iter1->name);
                diff.tps = iter2->tps - iter1->tps;
#define UNIT_DIFF(_result, _second, _first)                                  \
    _result = te_unit_bin_pack(te_unit_bin_unpack(_second) -                 \
                               te_unit_bin_unpack(_first));

                UNIT_DIFF(diff.kb_dscd, iter2->kb_dscd, iter1->kb_dscd);
                UNIT_DIFF(diff.kb_read, iter2->kb_read, iter1->kb_read);
                UNIT_DIFF(diff.kb_wrtn, iter2->kb_wrtn, iter1->kb_wrtn);
                UNIT_DIFF(diff.kb_dscd_s, iter2->kb_dscd_s, iter1->kb_dscd_s);
                UNIT_DIFF(diff.kb_read_s, iter2->kb_read_s, iter1->kb_read_s);
                UNIT_DIFF(diff.kb_wrtn_s, iter2->kb_wrtn_s, iter1->kb_wrtn_s);
#undef UNIT_DIFF
                TE_VEC_APPEND(&diff_report->devices, diff);
            }
        }
    }
}
