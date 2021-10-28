/** @file
 * @brief ethtool tool TAPI
 *
 * Implementation of TAPI to handle ethtool tool.
 *
 * Copyright (C) 2021 OKTET Labs. All rights reserved.
 *
 * @author Dmitry Izbitsky <Dmitry.Izbitsky@oktetlabs.ru>
 */

#include <stddef.h>
#include <signal.h>

#include "te_defs.h"
#include "te_str.h"
#include "te_alloc.h"
#include "tapi_ethtool.h"
#include "tapi_job_opt.h"

/** How long to wait for ehtool termination, in ms */
#define TAPI_ETHTOOL_TERM_TIMEOUT_MS 1000

const tapi_ethtool_opt tapi_ethtool_default_opt = {
    .cmd = TAPI_ETHTOOL_CMD_NONE,
    .if_name = NULL,
};

const tapi_ethtool_report tapi_ethtool_default_report = {
    .cmd = TAPI_ETHTOOL_CMD_NONE,
};

/** Filters used to parse ethtool output when no command is supplied */
typedef struct if_props_filters {
    tapi_job_channel_t *autoneg; /**< Get auto-negotiation state */
    tapi_job_channel_t *link_status; /**< Get link status */
} if_props_filters;

/** Main structure describing ethtool command */
struct tapi_ethtool_app {
    tapi_ethtool_cmd cmd; /**< Ethtool command */

    tapi_job_t *job; /**< Job handle */
    tapi_job_channel_t *out_chs[2]; /**< Channels for stdout and stderr */

    tapi_job_channel_t *err_filter; /**< Filter for reading stderr */

    union {
        if_props_filters if_props; /**< Filters for interface properties */
    } out_filters; /**< Filters for parsing stdout */
};

static te_errno fill_cmd_arg(const void *value, te_vec *args);

/*
 * These option bindings are enough for all commands requiring
 * just interface name as a parameter, and also will be included
 * in bindings for more complex commands.
 */

#define BASIC_BINDS \
    { &fill_cmd_arg, NULL, FALSE, NULL, \
      offsetof(tapi_ethtool_opt, cmd) }, \
    TAPI_JOB_OPT_STRING(NULL, FALSE, tapi_ethtool_opt, if_name)

static const tapi_job_opt_bind basic_binds[] = TAPI_JOB_OPT_SET(
    BASIC_BINDS
);

/**
 * Fill ethtool command in command line.
 *
 * @param value     Ethtool command from tapi_ethtool_cmd
 * @param args      Where to add command string
 *
 * @return Status code.
 */
static te_errno
fill_cmd_arg(const void *value, te_vec *args)
{
    tapi_ethtool_cmd cmd = *(const tapi_ethtool_cmd *)value;
    const char *cmd_str = NULL;

    switch (cmd)
    {
        case TAPI_ETHTOOL_CMD_NONE:
            return TE_ENOENT;

        default:
            ERROR("%s(): unknown command code %d", __FUNCTION__, cmd);
            return TE_EINVAL;
    }

    return te_vec_append_str_fmt(args, "%s", cmd_str);
}

/**
 * Get option bindings for the requested ethtool command.
 *
 * @param cmd     Ethtool command
 *
 * @return Pointer to option bindings.
 */
static const tapi_job_opt_bind *
get_binds_by_cmd(tapi_ethtool_cmd cmd)
{
    switch (cmd)
    {
        default:
            return basic_binds;
    }

    return NULL;
}

/**
 * Add filter to extract a single value from ethtool output.
 *
 * @param app             Structure describing ethtool command
 * @param filter_name     Name of the filter
 * @param filter          Where to save pointer to the created filter
 * @param prefix_regexp   Regular expression for the prefix with which
 *                        this value is printed. It is assumed that
 *                        every value is printed on its own line like
 *                        "Prefix: value"
 *
 * @return Status code.
 */
static te_errno
add_value_filter(tapi_ethtool_app *app,
                 const char *filter_name,
                 tapi_job_channel_t **filter,
                 const char *prefix_regexp)
{
    char re_buf[TAPI_ETHTOOL_MAX_STR] = "";
    te_errno rc;

    rc = te_snprintf(re_buf, sizeof(re_buf),
                     "^\\s*%s:\\s*(.*)\\s*$", prefix_regexp);
    if (rc != 0)
        return 0;

    rc = tapi_job_attach_filter(TAPI_JOB_CHANNEL_SET(app->out_chs[0]),
                                filter_name, TRUE, 0, filter);
    if (rc != 0)
        return rc;

    return tapi_job_filter_add_regexp(*filter, re_buf, 1);
}

/**
 * Attach filters used to parse ethtool output when it is run
 * with no command (and prints interface properties).
 *
 * @param app     Ethtool application structure
 *
 * @return Status code.
 */
static te_errno
attach_if_props_filters(tapi_ethtool_app *app)
{
    te_errno rc;

    rc = add_value_filter(app, "Link status",
                          &app->out_filters.if_props.link_status,
                          "Link detected");
    if (rc != 0)
        return rc;

    return add_value_filter(app, "Auto-negotiation",
                            &app->out_filters.if_props.autoneg,
                            "Auto-negotiation");
}

/**
 * Attach filters used to parse ethtool output.
 * Filters are chosen depending on specific ethtool command.
 *
 * @param app     Ethtool application structure
 *
 * @return Status code.
 */
static te_errno
attach_filters(tapi_ethtool_app *app)
{
    switch (app->cmd)
    {
        case TAPI_ETHTOOL_CMD_NONE:
            return attach_if_props_filters(app);

        default:
            return 0;
    }
}

/* See description in tapi_ethtool.h */
te_errno
tapi_ethtool_create(tapi_job_factory_t *factory,
                    const tapi_ethtool_opt *opt,
                    tapi_ethtool_app **app)
{
    tapi_ethtool_app *result = NULL;
    te_vec ethtool_args = TE_VEC_INIT(char *);
    te_errno rc;

    tapi_job_simple_desc_t desc;

    result = TE_ALLOC(sizeof(*result));
    if (result == NULL)
    {
        ERROR("Failed to allocate memory for ethtool app");
        return TE_RC(TE_TAPI, TE_ENOMEM);
    }

    result->cmd = opt->cmd;

    rc = tapi_job_opt_build_args("ethtool", get_binds_by_cmd(opt->cmd),
                                 opt, &ethtool_args);
    if (rc != 0)
    {
        ERROR("Failed to build ethtool command line arguments");
        goto out;
    }

    memset(&desc, 0, sizeof(desc));

    desc.program = "ethtool";
    desc.argv = (const char **)(ethtool_args.data.ptr);
    desc.job_loc = &result->job;
    desc.stdout_loc = &result->out_chs[0];
    desc.stderr_loc = &result->out_chs[1];
    desc.filters = TAPI_JOB_SIMPLE_FILTERS(
        {
            .use_stdout  = TRUE,
            .readable    = TRUE,
            .log_level   = TE_LL_RING,
            .filter_name = "out",
        },
        {
            .use_stderr  = TRUE,
            .readable    = TRUE,
            .log_level   = TE_LL_ERROR,
            .filter_name = "err",
            .filter_var = &result->err_filter,
        });

    rc = tapi_job_simple_create(factory, &desc);
    if (rc != 0)
    {
        ERROR("Failed to create job instance for ethtool app");
        goto out;
    }

    rc = attach_filters(result);
    if (rc != 0)
    {
        ERROR("Failed to attach command-specific filters");
        goto out;
    }

    *app = result;

out:

    te_vec_deep_free(&ethtool_args);

    if (rc != 0)
    {
        if (result->job != NULL)
            tapi_job_destroy(result->job, 0);

        free(result);
    }

    return rc;
}

/* See description in tapi_ethtool.h */
te_errno
tapi_ethtool_start(tapi_ethtool_app *app)
{
    return tapi_job_start(app->job);
}

/* See description in tapi_ethtool.h */
te_errno
tapi_ethtool_wait(tapi_ethtool_app *app, int timeout_ms)
{
    te_errno rc;
    tapi_job_status_t status;

    rc = tapi_job_wait(app->job, timeout_ms, &status);
    if (rc != 0)
        return rc;

    TAPI_JOB_CHECK_STATUS(status);

    return 0;
}

/* See description in tapi_ethtool.h */
te_errno
tapi_ethtool_kill(tapi_ethtool_app *app, int signum)
{
    return tapi_job_kill(app->job, signum);
}

/* See description in tapi_ethtool.h */
te_errno
tapi_ethtool_stop(tapi_ethtool_app *app)
{
    return tapi_job_stop(app->job, SIGINT, TAPI_ETHTOOL_TERM_TIMEOUT_MS);
}

/* See description in tapi_ethtool.h */
te_errno
tapi_ethtool_destroy(tapi_ethtool_app *app)
{
    te_errno rc;

    if (app == NULL)
        return 0;

    rc = tapi_job_destroy(app->job, TAPI_ETHTOOL_TERM_TIMEOUT_MS);
    if (rc != 0)
        ERROR("Failed to destroy ethtool job");

    free(app);

    return rc;
}

/* See description in tapi_ethtool.h */
te_errno
tapi_ethtool_check_stderr(tapi_ethtool_app *app)
{
    return tapi_job_filters_have_data(TAPI_JOB_CHANNEL_SET(app->err_filter),
                                      0);
}

/**
 * Obtain interface properties from ethtool output via filters.
 *
 * @param app       Ethtool application structure
 * @param props     Where to save parsed properties
 *
 * @return Status code.
 */
static te_errno
get_if_props(tapi_ethtool_app *app,
             tapi_ethtool_if_props *props)
{
    te_errno rc = 0;
    te_string val = TE_STRING_INIT;

    memset(props, 0, sizeof(*props));

    rc = tapi_job_receive_single(app->out_filters.if_props.autoneg,
                                 &val, 0);
    if (rc != 0)
        goto out;

    if (strcasecmp(val.ptr, "on") == 0)
        props->autoneg = TRUE;

    te_string_free(&val);

    rc = tapi_job_receive_single(app->out_filters.if_props.link_status,
                                 &val, 0);
    if (rc != 0)
        goto out;

    if (strcasecmp(val.ptr, "yes") == 0)
        props->link = TRUE;

out:

    te_string_free(&val);
    return rc;
}

/* See description in tapi_ethtool.h */
te_errno
tapi_ethtool_get_report(tapi_ethtool_app *app,
                        tapi_ethtool_report *report)
{
    switch (app->cmd)
    {
        case TAPI_ETHTOOL_CMD_NONE:
            return get_if_props(app, &report->data.if_props);

        default:
            ERROR("%s(): no report is defined for the current command",
                  __FUNCTION__);
            return TE_RC(TE_TAPI, TE_ENOENT);
    }

    report->cmd = app->cmd;
}

/* See description in tapi_ethtool.h */
void
tapi_ethtool_destroy_report(tapi_ethtool_report *report)
{
    UNUSED(report);
}
