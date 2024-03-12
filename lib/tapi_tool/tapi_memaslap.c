/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief TAPI to manage memaslap
 *
 * TAPI to manage memaslap.
 *
 * Copyright (C) 2022-2022 OKTET Labs Ltd. All rights reserved.
 */

#define TE_LGR_USER "TAPI MEMASLAP"

#include <signal.h>
#include <arpa/inet.h>

#include "tapi_memaslap.h"
#include "tapi_job_opt.h"
#include "tapi_file.h"
#include "te_mi_log.h"
#include "te_alloc.h"
#include "te_str.h"
#include "te_defs.h"

#define TAPI_MEMASLAP_TIMEOUT_MS 10000

#define CFG_OPT_KEY_LEN_MIN_LIM 16
#define CFG_OPT_KEY_LEN_MAX_LIM 250
#define CFG_OPT_VALUE_LEN_MIN_LIM 1
#define CFG_OPT_VALUE_LEN_MAX_LIM 1048576

/**
 * Path to memaslap exec in the case of
 * tapi_memaslap_opt::memaslap_path is @c NULL.
 */
static const char *memaslap_path = "memaslap";

/* Possible memaslap command line arguments */
static const tapi_job_opt_bind memaslap_binds[] = TAPI_JOB_OPT_SET(
    TAPI_JOB_OPT_EMBED_ARRAY("--servers=", TRUE, ",", NULL,
                             tapi_memaslap_opt, n_servers, servers,
                             TAPI_JOB_OPT_ADDR_PORT_PTR(NULL, FALSE,
                                                        tapi_memaslap_opt,
                                                        servers[0])),
    TAPI_JOB_OPT_UINT_T("--threads=", TRUE, NULL, tapi_memaslap_opt, threads),
    TAPI_JOB_OPT_UINT_T("--concurrency=", TRUE, NULL, tapi_memaslap_opt,
                        concurrency),
    TAPI_JOB_OPT_UINT_T("--conn_sock=", TRUE, NULL, tapi_memaslap_opt,
                        conn_sock),
    TAPI_JOB_OPT_UINT_T("--execute_number=", TRUE, NULL, tapi_memaslap_opt,
                        execute_number),
    TAPI_JOB_OPT_UINT_T("--time=", TRUE, "s", tapi_memaslap_opt,
                        time),
    TAPI_JOB_OPT_UINT_T("--win_size=", TRUE, "k", tapi_memaslap_opt,
                        win_size),
    TAPI_JOB_OPT_UINT_T("--fixed_size=", TRUE, NULL, tapi_memaslap_opt,
                        fixed_size),
    TAPI_JOB_OPT_DOUBLE("--verify=", TRUE, NULL, tapi_memaslap_opt,
                        verify),
    TAPI_JOB_OPT_UINT_T("--division=", TRUE, NULL, tapi_memaslap_opt,
                        division),
    TAPI_JOB_OPT_UINT_T("--stat_freq=", TRUE, "s", tapi_memaslap_opt,
                        stat_freq),
    TAPI_JOB_OPT_DOUBLE("--exp_verify=", TRUE, NULL, tapi_memaslap_opt,
                        expire_verify),
    TAPI_JOB_OPT_DOUBLE("--overwrite=", TRUE, NULL, tapi_memaslap_opt,
                        overwrite),
    TAPI_JOB_OPT_BOOL("--reconnect", tapi_memaslap_opt, reconnect),
    TAPI_JOB_OPT_BOOL("--udp", tapi_memaslap_opt, udp),
    TAPI_JOB_OPT_BOOL("--facebook", tapi_memaslap_opt, facebook),
    TAPI_JOB_OPT_BOOL("--binary", tapi_memaslap_opt, bin_protocol),
    TAPI_JOB_OPT_UINT_T("--tps=", TRUE, "k", tapi_memaslap_opt,
                        expected_tps),
    TAPI_JOB_OPT_UINT_T("--rep_write=", TRUE, NULL, tapi_memaslap_opt,
                        rep_write),
    TAPI_JOB_OPT_STRING("--cfg_cmd=", TRUE, tapi_memaslap_opt, cfg_cmd),
    TAPI_JOB_OPT_BOOL("--verbose", tapi_memaslap_opt, verbose)
);

/* Default values of memaslap command line arguments */
const tapi_memaslap_opt tapi_memaslap_default_opt = {
    .n_servers          = 0,
    .servers            = { NULL },
    .threads            = TAPI_JOB_OPT_UINT_UNDEF,
    .concurrency        = TAPI_JOB_OPT_UINT_UNDEF,
    .conn_sock          = TAPI_JOB_OPT_UINT_UNDEF,
    .execute_number     = TAPI_JOB_OPT_UINT_UNDEF,
    .time               = TAPI_JOB_OPT_UINT_UNDEF,
    .win_size           = TAPI_JOB_OPT_UINT_UNDEF,
    .fixed_size         = TAPI_JOB_OPT_UINT_UNDEF,
    .verify             = TAPI_JOB_OPT_DOUBLE_UNDEF,
    .division           = TAPI_JOB_OPT_UINT_UNDEF,
    .stat_freq          = TAPI_JOB_OPT_UINT_UNDEF,
    .expire_verify      = TAPI_JOB_OPT_DOUBLE_UNDEF,
    .overwrite          = TAPI_JOB_OPT_DOUBLE_UNDEF,
    .reconnect          = FALSE,
    .udp                = FALSE,
    .facebook           = FALSE,
    .bin_protocol       = FALSE,
    .expected_tps       = TAPI_JOB_OPT_UINT_UNDEF,
    .rep_write          = TAPI_JOB_OPT_UINT_UNDEF,
    .verbose            = FALSE,
    .memaslap_path      = NULL
};

/* Default values of memaslap configuration file options */
const tapi_memaslap_cfg_opt tapi_memaslap_default_cfg_opt = {
    .key_len_min = 64,
    .key_len_max = 64,
    .value_len_min = 1024,
    .value_len_max = 1024,
    .set_share = 0.1
};

static te_errno
cfg_opts_check_lens(tapi_memaslap_cfg_opt *cfg_opt)
{
    te_errno rc = 0;

    if (cfg_opt->key_len_min > cfg_opt->key_len_max)
    {
        ERROR("Incorrect key_len: min %zu > max %zu", cfg_opt->key_len_min,
              cfg_opt->key_len_max);
        rc = TE_RC(TE_TAPI, TE_EINVAL);
    }

    if (cfg_opt->key_len_min < CFG_OPT_KEY_LEN_MIN_LIM)
    {
        ERROR("Incorrect key_len_min: %zu, must be >= %d", cfg_opt->key_len_min,
              CFG_OPT_KEY_LEN_MIN_LIM);
        rc = TE_RC(TE_TAPI, TE_EINVAL);
    }

    if (cfg_opt->key_len_max > CFG_OPT_KEY_LEN_MAX_LIM)
    {
        ERROR("Incorrect key_len_max: %zu, must be <= %d", cfg_opt->key_len_max,
              CFG_OPT_KEY_LEN_MAX_LIM);
        rc = TE_RC(TE_TAPI, TE_EINVAL);
    }

    if (cfg_opt->value_len_min > cfg_opt->value_len_max)
    {
        ERROR("Incorrect value_len: min %zu > max %zu", cfg_opt->value_len_min,
              cfg_opt->value_len_max);
        rc = TE_RC(TE_TAPI, TE_EINVAL);
    }

    if (cfg_opt->value_len_min < CFG_OPT_VALUE_LEN_MIN_LIM)
    {
        ERROR("Incorrect value_len_min: %zu, must be >= %d",
              cfg_opt->value_len_min, CFG_OPT_VALUE_LEN_MIN_LIM);
        rc = TE_RC(TE_TAPI, TE_EINVAL);
    }

    if (cfg_opt->value_len_max > CFG_OPT_VALUE_LEN_MAX_LIM)
    {
        ERROR("Incorrect value_len_max: %zu, must be <= %d",
              cfg_opt->value_len_max, CFG_OPT_VALUE_LEN_MAX_LIM);
        rc = TE_RC(TE_TAPI, TE_EINVAL);
    }

    return rc;
}

/* See description in tapi_memaslap.h */
te_errno
tapi_memaslap_create(tapi_job_factory_t *factory,
                     tapi_memaslap_opt *opt,
                     tapi_memaslap_app **app)
{
    te_errno            rc;
    tapi_memaslap_app  *new_app;
    const char         *exec_path = memaslap_path;

    const char *ta = NULL;
    char *tmp_fn = NULL;

    if (factory == NULL)
    {
        rc = TE_EFAULT;
        ERROR("Memaslap factory to create job can't be NULL: %r", rc);
        return TE_RC(TE_TAPI, rc);
    }

    if (opt == NULL)
    {
        rc = TE_EFAULT;
        ERROR("Memaslap opt to create job can't be NULL: %r", rc);
        return TE_RC(TE_TAPI, rc);
    }

    if (app == NULL)
    {
        rc = TE_EFAULT;
        ERROR("Memaslap app to create job can't be NULL: %r", rc);
        return TE_RC(TE_TAPI, rc);
    }

    ta = tapi_job_factory_ta(factory);
    if (ta == NULL)
    {
        ERROR("Failed to get TA of memaslap app");
        return TE_RC(TE_TAPI, TE_ENOENT);
    }

    new_app = TE_ALLOC(sizeof(tapi_memaslap_app));

    new_app->ta = ta;

    if (opt->memaslap_path != NULL)
        exec_path = opt->memaslap_path;

    if (opt->cfg_opts != NULL)
    {
        rc = cfg_opts_check_lens(opt->cfg_opts);
        if (rc != 0)
            return rc;

        te_string cfg_txt = TE_STRING_INIT;

        tmp_fn = tapi_file_make_name(NULL);
        new_app->tmp_cfg_fn = tmp_fn;

        /*
         * Here default values are added for key and value
         * sections; these sections must be filled, or memaslap
         * will not run correctly.
         */
        te_string_append(&cfg_txt,
                         "key\n"
                         "%zu %zu 1\n"
                         "value\n"
                         "%zu %zu 1\n"
                         "cmd\n"
                         "0    %.2f\n"
                         "1    %.2f\n",
                         opt->cfg_opts->key_len_min,
                         opt->cfg_opts->key_len_max,
                         opt->cfg_opts->value_len_min,
                         opt->cfg_opts->value_len_max,
                         opt->cfg_opts->set_share,
                         1.0 - opt->cfg_opts->set_share);
        RING("The following contents will be passed "
             "in configuration file %s to memaslap:\n%s",
             tmp_fn, te_string_value(&cfg_txt));

        rc = tapi_file_create_ta(ta, tmp_fn, "%s",
                                 te_string_value(&cfg_txt));
        te_string_free(&cfg_txt);
        if (rc != 0)
        {
            ERROR("Failed to create file %s on TA %s for memaslap: %r",
                  tmp_fn, ta, rc);
            free(tmp_fn);
            free(new_app);
            return TE_RC(TE_TAPI, rc);
        }

        opt->cfg_cmd = tmp_fn;
    }

    rc = tapi_job_opt_build_args(exec_path, memaslap_binds,
                                 opt, &new_app->cmd);
    if (opt->cfg_opts != NULL)
        opt->cfg_cmd = NULL;
    if (rc != 0)
    {
        ERROR("Failed to build memaslap job command line arguments: %r", rc);
        te_vec_deep_free(&new_app->cmd);
        free(tmp_fn);
        free(new_app);
        return rc;
    }

    rc = tapi_job_simple_create(factory,
                                &(tapi_job_simple_desc_t){
                                    .program    = exec_path,
                                    .argv       = (const char **)new_app->cmd.data.ptr,
                                    .job_loc    = &new_app->job,
                                    .stdout_loc = &new_app->out_chs[0],
                                    .stderr_loc = &new_app->out_chs[1],
                                    .filters    = TAPI_JOB_SIMPLE_FILTERS(
                                        {
                                            .use_stdout = TRUE,
                                            .readable = TRUE,
                                            .re = "TPS:\\s*([0-9]+)\\s",
                                            .extract = 1,
                                            .filter_var = &new_app->tps_filter,
                                        },
                                        {
                                            .use_stdout = TRUE,
                                            .readable = TRUE,
                                            .re = "Net_rate:\\s*([0-9]+.[0-9]+)M",
                                            .extract = 1,
                                            .filter_var = &new_app->net_rate_filter,
                                        },
                                        {
                                            .use_stdout  = TRUE,
                                            .readable    = TRUE,
                                            .log_level   = TE_LL_RING,
                                            .filter_name = "memaslap stdout"
                                        },
                                        {
                                            .use_stderr  = TRUE,
                                            .readable    = FALSE,
                                            .log_level   = TE_LL_WARN,
                                            .filter_name = "memaslap stderr"
                                        }
                                    )
                                });
    if (rc != 0)
    {
        ERROR("Failed to create %s job: %r", exec_path, rc);
        te_vec_deep_free(&new_app->cmd);
        free(tmp_fn);
        free(new_app);
        return rc;
    }

    *app = new_app;
    return 0;
}

/* See description in tapi_memaslap.h */
te_errno
tapi_memaslap_start(const tapi_memaslap_app *app)
{
    te_errno rc;

    if (app == NULL)
    {
        rc = TE_EFAULT;
        ERROR("Memaslap app to start job can't be NULL: %r", rc);
        return TE_RC(TE_TAPI, rc);
    }

    return tapi_job_start(app->job);
}

/* See description in tapi_memaslap.h */
te_errno
tapi_memaslap_wait(const tapi_memaslap_app *app, int timeout_ms)
{
    te_errno          rc;
    tapi_job_status_t status;

    if (app == NULL)
    {
        rc = TE_EFAULT;
        ERROR("Memaslap app to wait job can't be NULL: %r", rc);
        return TE_RC(TE_TAPI, rc);
    }

    rc = tapi_job_wait(app->job, timeout_ms, &status);
    if (rc != 0)
    {
        if (TE_RC_GET_ERROR(rc) == TE_EINPROGRESS)
            RING("Job was still in process at the end of the wait");

        return rc;
    }

    TAPI_JOB_CHECK_STATUS(status);
    return 0;
}

/* See description in tapi_memaslap.h */
te_errno
tapi_memaslap_stop(const tapi_memaslap_app *app)
{
    te_errno rc;

    if (app == NULL)
    {
        rc = TE_EFAULT;
        ERROR("Memaslap app to stop job can't be NULL: %r", rc);
        return TE_RC(TE_TAPI, rc);
    }

    return tapi_job_stop(app->job, SIGTERM, TAPI_MEMASLAP_TIMEOUT_MS);
}

/* See description in tapi_memaslap.h */
te_errno
tapi_memaslap_kill(const tapi_memaslap_app *app, int signum)
{
    te_errno rc;

    if (app == NULL)
    {
        rc = TE_EFAULT;
        ERROR("Memaslap app to kill job can't be NULL: %r", rc);
        return TE_RC(TE_TAPI, rc);
    }

    return tapi_job_kill(app->job, signum);
}

/* See description in tapi_memaslap.h */
te_errno
tapi_memaslap_destroy(tapi_memaslap_app *app)
{
    te_errno rc;

    if (app == NULL)
        return 0;

    rc = tapi_job_destroy(app->job, TAPI_MEMASLAP_TIMEOUT_MS);
    if (rc != 0)
    {
        ERROR("Failed to destroy memaslap job: %r", rc);
        return rc;
    }

    te_vec_deep_free(&app->cmd);

    if (app->tmp_cfg_fn != NULL)
    {
        rc = tapi_file_ta_unlink_fmt(app->ta, "%s", app->tmp_cfg_fn);
        if (rc != 0)
        {
            ERROR("Failed to remove memaslap configuration file %s "
                  "on TA %s: %r", app->tmp_cfg_fn, app->ta, rc);
            return TE_RC(TE_TAPI, rc);
        }

        free(app->tmp_cfg_fn);
        app->tmp_cfg_fn = NULL;
    }
    free(app);
    return 0;
}

/**
 * Read data from filter.
 *
 * @param[in]  filter Filter for reading.
 * @param[out] out    String with message from filter.
 *
 * @return Status code.
 */
static te_errno
read_filter(tapi_job_channel_t *filter, te_string *out)
{
    te_errno          rc;
    tapi_job_buffer_t buf = TAPI_JOB_BUFFER_INIT;

    rc = tapi_job_receive(TAPI_JOB_CHANNEL_SET(filter), -1, &buf);
    if (rc != 0)
    {
        ERROR("Failed to read data from filter: %r", rc);
        return rc;
    }

    *out = buf.data;
    return 0;
}

/**
 * Converts memaslap arguments into string for mi_logger.
 *
 * @param[in]  vec  Vector of memaslap argumets.
 * @param[out] str  String for output converted arguments.
 *
 * @return Status code.
 */
static te_errno
tapi_memaslap_args2str(te_vec *vec, char **str)
{
    te_errno    rc        = 0;
    te_string   arguments = TE_STRING_INIT;
    const char *separator = " ";
    void      **arg;

    if (str == NULL)
    {
        rc = TE_EFAULT;
        ERROR("str to convert memaslap args can't be NULL: %r", rc);
        return TE_RC(TE_TAPI, rc);
    }

    if (te_vec_size(vec) == 0)
    {
        rc = TE_EFAULT;
        ERROR("vec to convert memaslap args can't be NULL: %r", rc);
        return TE_RC(TE_TAPI, rc);
    }

    TE_VEC_FOREACH(vec, arg)
    {
        if (rc == 0 && *arg != NULL)
            rc = te_string_append(&arguments, "%s%s", *arg, separator);
    }

    if (rc != 0)
    {
        te_string_free(&arguments);
        ERROR("Failed to write memaslap args into string: %r", rc);
        return rc;
    }

    te_string_cut(&arguments, strlen(separator));
    *str = arguments.ptr;
    return 0;
}

/* See description in tapi_memaslap.h */
te_errno
tapi_memaslap_get_report(tapi_memaslap_app *app,
                         tapi_memaslap_report *report)
{
    te_errno  rc;
    te_string buf  = TE_STRING_INIT;

    if (app == NULL || report == NULL)
    {
        rc = TE_EFAULT;
        ERROR("Memaslap app/report to get job report can't be NULL: %r", rc);
        return TE_RC(TE_TAPI, rc);
    }

    read_filter(app->tps_filter, &buf);
    rc = te_strtoui(buf.ptr, 10, &report->tps);
    te_string_free(&buf);
    if (rc != 0)
    {
        ERROR("Failed read data from app->tps_filter: %r", rc);
        return rc;
    }

    read_filter(app->net_rate_filter, &buf);
    rc = te_strtod(buf.ptr, &report->net_rate);
    te_string_free(&buf);
    if (rc != 0)
    {
        ERROR("Failed read data from app->net_rate_filter: %r", rc);
        return rc;
    }
    /* Convert from MiB/s to Mibit/s */
    report->net_rate *= 8;

    rc = tapi_memaslap_args2str(&app->cmd, &report->cmd);
    if (rc != 0)
    {
        ERROR("Failed read memaslap command line arguments: %r", rc);
        return rc;
    }

    return 0;
}

/* See description in tapi_memaslap.h */
te_errno
tapi_memaslap_report_mi_log(const tapi_memaslap_report *report)
{
    te_errno      rc;
    te_mi_logger *logger;

    if (report == NULL)
    {
        rc = TE_EFAULT;
        ERROR("Memaslap report to write log can't be NULL: %r", rc);
        return TE_RC(TE_TAPI, rc);
    }

    rc = te_mi_logger_meas_create("memaslap", &logger);
    if (rc != 0)
    {
        ERROR("Failed to create MI logger, error: %r", rc);
        return rc;
    }

    te_mi_logger_add_meas(logger, NULL, TE_MI_MEAS_RPS, "TPS",
                          TE_MI_MEAS_AGGR_SINGLE, report->tps,
                          TE_MI_MEAS_MULTIPLIER_PLAIN);
    te_mi_logger_add_meas(logger, NULL, TE_MI_MEAS_THROUGHPUT, "Net_rate",
                          TE_MI_MEAS_AGGR_SINGLE, report->net_rate,
                          TE_MI_MEAS_MULTIPLIER_MEBI);
    te_mi_logger_add_comment(logger, NULL, "command", "%s",
                             report->cmd);

    te_mi_logger_destroy(logger);
    return 0;
}

/* See description in tapi_memaslap.h */
te_errno
tapi_memaslap_destroy_report(tapi_memaslap_report *report)
{
    te_errno rc;

    if (report == NULL)
    {
        rc = TE_EFAULT;
        ERROR("Memaslap report to destroy can't be NULL: %r", rc);
        return TE_RC(TE_TAPI, rc);
    }

    free(report->cmd);
    return 0;
}
