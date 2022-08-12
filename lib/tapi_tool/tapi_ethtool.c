/** @file
 * @brief ethtool tool TAPI
 *
 * Implementation of TAPI to handle ethtool tool.
 *
 * Copyright (C) 2021-2022 OKTET Labs. All rights reserved.
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

    .err_out = FALSE,
    .err_data = TE_STRING_INIT,
    .err_code = TE_EOK,
};

/** Filters used to parse ethtool output when no command is supplied */
typedef struct if_props_filters {
    tapi_job_channel_t *autoneg; /**< Get auto-negotiation state */
    tapi_job_channel_t *link_status; /**< Get link status */
} if_props_filters;

/**
 * Filters used to parse ethtool output when --show-pause command
 * is supplied
 */
typedef struct pause_filters {
    tapi_job_channel_t *autoneg; /**< Pause auto-negotiation state */
    tapi_job_channel_t *rx; /**< Whether Rx pause frames are enabled */
    tapi_job_channel_t *tx; /**< Whether Tx pause frames are enabled */
} pause_filters;

/**
 * Filters used to parse ethtool output when --show-ring command
 * is supplied
 */
typedef struct ring_filters {
    tapi_job_channel_t *rx; /**< Get RX ring size */
    tapi_job_channel_t *tx; /**< Get TX ring size */
} ring_filters;

/** Main structure describing ethtool command */
typedef struct tapi_ethtool_app {
    tapi_ethtool_cmd cmd; /**< Ethtool command */

    tapi_job_t *job; /**< Job handle */
    tapi_job_channel_t *out_chs[2]; /**< Channels for stdout and stderr */

    tapi_job_channel_t *err_filter; /**< Filter for reading stderr */

    union {
        if_props_filters if_props; /**< Filters for interface properties */
        tapi_job_channel_t *line_filter; /**< Filter extracting output
                                              line by line */
        pause_filters pause; /**< Filters for pause parameters */
        ring_filters ring; /**< Filters for ring parameters */
    } out_filters; /**< Filters for parsing stdout */
} tapi_ethtool_app;

static te_errno fill_cmd_arg(const void *value, const void *priv, te_vec *args);

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
fill_cmd_arg(const void *value, const void *priv, te_vec *args)
{
    tapi_ethtool_cmd cmd = *(const tapi_ethtool_cmd *)value;
    const char *cmd_str = NULL;

    UNUSED(priv);

    switch (cmd)
    {
        case TAPI_ETHTOOL_CMD_NONE:
            return TE_ENOENT;

        case TAPI_ETHTOOL_CMD_STATS:
            cmd_str = "--statistics";
            break;

        case TAPI_ETHTOOL_CMD_SHOW_PAUSE:
            cmd_str = "--show-pause";
            break;

        case TAPI_ETHTOOL_CMD_SHOW_RING:
            cmd_str = "--show-ring";
            break;

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
 * Attach a filter which gets output line by line, removing spaces
 * at the beginning and at the end of every line.
 *
 * @param app     Ethtool application structure
 *
 * @return Status code.
 */
static te_errno
attach_line_filter(tapi_ethtool_app *app)
{
    te_errno rc;

    rc = tapi_job_attach_filter(TAPI_JOB_CHANNEL_SET(app->out_chs[0]),
                                "Line", TRUE, 0,
                                &app->out_filters.line_filter);
    if (rc != 0)
        return rc;

    return tapi_job_filter_add_regexp(app->out_filters.line_filter,
                                      "^\\s*(.*)\\s*$", 1);
}

/**
 * Attach filters used to parse ethtool output when it is run
 * with --show-pause command.
 *
 * @param app     Ethtool application structure
 *
 * @return Status code.
 */
static te_errno
attach_pause_filters(tapi_ethtool_app *app)
{
    te_errno rc;

    rc = add_value_filter(app, "Autonegotiate",
                          &app->out_filters.pause.autoneg,
                          "Autonegotiate");
    if (rc != 0)
        return rc;

    rc = add_value_filter(app, "Rx pause",
                          &app->out_filters.pause.rx, "RX");
    if (rc != 0)
        return rc;

    return add_value_filter(app, "Tx pause",
                            &app->out_filters.pause.tx, "TX");
}

/**
 * Attach filters used to parse ethtool output when it is run
 * with --show-ring command.
 *
 * @param app     Ethtool application structure
 *
 * @return Status code.
 */
static te_errno
attach_ring_filters(tapi_ethtool_app *app)
{
    te_errno rc;

    rc = add_value_filter(app, "Rx size",
                          &app->out_filters.ring.rx, "RX");
    if (rc != 0)
        return rc;

    return add_value_filter(app, "Tx size",
                            &app->out_filters.ring.tx, "TX");
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

        case TAPI_ETHTOOL_CMD_STATS:
            return attach_line_filter(app);

        case TAPI_ETHTOOL_CMD_SHOW_PAUSE:
            return attach_pause_filters(app);

        case TAPI_ETHTOOL_CMD_SHOW_RING:
            return attach_ring_filters(app);

        default:
            return 0;
    }
}

/**
 * Create a job to run ethtool application.
 *
 * @param factory       Job factory
 * @param opt           Ethtool options
 * @param app           Where to save pointer to application structure
 *
 * @return Status code.
 */
static te_errno
create_app(tapi_job_factory_t *factory,
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

/**
 * Obtain interface statistics from ethtool output via filters.
 *
 * @param app       Ethtool application structure
 * @param stats     Where to save parsed statistics
 *
 * @return Status code.
 */
static te_errno
get_stats(tapi_ethtool_app *app, te_kvpair_h *stats)
{
    te_errno rc;
    tapi_job_buffer_t *bufs = NULL;
    unsigned int count = 0;
    unsigned int i;
    char *colon = NULL;

    te_kvpair_init(stats);

    rc = tapi_job_receive_many(
                    TAPI_JOB_CHANNEL_SET(app->out_filters.line_filter),
                    0, &bufs, &count);
    if (rc != 0)
        goto out;

    for (i = 0; i < count; i++)
    {
        if (bufs[i].eos)
            break;

        colon = strrchr(bufs[i].data.ptr, ':');
        if (colon == NULL)
        {
            ERROR("%s(): wrong format of a line:\n%s",
                  __FUNCTION__, bufs[i].data.ptr);
            rc = TE_RC(TE_TAPI, TE_EINVAL);
            break;
        }

        *colon = '\0';

        rc = te_kvpair_add(stats, bufs[i].data.ptr, colon + 1);
        if (rc != 0)
            break;
    }

out:
    if (rc != 0)
        te_kvpair_fini(stats);

    tapi_job_buffers_free(bufs, count);

    return rc;
}

/**
 * Get a single value from a filter which can be either on or off.
 *
 * @param filter      Filter to read from
 * @param value       Will be set to @c TRUE in case of 'on' and
 *                    to @c FALSE in case of 'off'
 *
 * @return Status code.
 */
static te_errno
get_on_off_value(tapi_job_channel_t *filter, te_bool *value)
{
    te_errno rc = 0;
    te_string str_val = TE_STRING_INIT;

    rc = tapi_job_receive_single(filter, &str_val, 0);
    if (rc != 0)
        goto out;

    if (strcasecmp(str_val.ptr, "on") == 0)
    {
        *value = TRUE;
    }
    else if (strcasecmp(str_val.ptr, "off") == 0)
    {
        *value = FALSE;
    }
    else
    {
        ERROR("%s(): cannot parse value '%s'", __FUNCTION__,
              str_val.ptr);
        rc = TE_RC(TE_TAPI, TE_EINVAL);
    }

out:

    te_string_free(&str_val);
    return rc;
}

/**
 * Obtain pause parameters from ethtool output via filters.
 *
 * @param app       Ethtool application structure
 * @param pause     Where to save parsed parameters
 *
 * @return Status code.
 */
static te_errno
get_pause(tapi_ethtool_app *app, tapi_ethtool_pause *pause)
{
    te_errno rc;

    memset(pause, 0, sizeof(*pause));

    rc = get_on_off_value(app->out_filters.pause.autoneg,
                          &pause->autoneg);
    if (rc != 0)
        return rc;

    rc = get_on_off_value(app->out_filters.pause.rx,
                          &pause->rx);
    if (rc != 0)
        return rc;

    return get_on_off_value(app->out_filters.pause.tx,
                            &pause->tx);
}

/**
 * Get current value and preset maximum value from a filter
 *
 * @param filter      Filter to read from
 * @param value       Will be set to current ring size
 * @param max_value   Will be set to maximum ring size
 *
 * @return Status code.
 */
static te_errno
get_ring_size_value(tapi_job_channel_t *filter,
                    int *value, int *max_value)
{
    te_errno rc = 0;
    tapi_job_buffer_t val_max = TAPI_JOB_BUFFER_INIT;
    te_string str_val = TE_STRING_INIT;

    tapi_job_simple_receive(TAPI_JOB_CHANNEL_SET(filter),
                            TAPI_ETHTOOL_TERM_TIMEOUT_MS, &val_max);

    rc = tapi_job_receive_single(filter, &str_val,
                                 TAPI_ETHTOOL_TERM_TIMEOUT_MS);
    if (rc != 0)
        goto out;

    rc = te_strtoi(str_val.ptr, 10, value);
    rc = te_strtoi(val_max.data.ptr, 10, max_value);

out:

    te_string_free(&val_max.data);
    te_string_free(&str_val);
    return rc;
}

/**
 * Obtain ring sizes from ethtool output via filters.
 *
 * @param app      Ethtool application structure
 * @param ring     Where to save parsed parameters
 *
 * @return Status code.
 */
static te_errno
get_ring(tapi_ethtool_app *app, tapi_ethtool_ring *ring)
{
    te_errno rc;

    memset(ring, 0, sizeof(*ring));

    rc = get_ring_size_value(app->out_filters.ring.rx,
                             &ring->rx, &ring->rx_max);
    if (rc != 0)
        return rc;

    rc = get_ring_size_value(app->out_filters.ring.tx,
                             &ring->tx, &ring->tx_max);
    return rc;
}

/**
 * Check and parse stderr output.
 *
 * @param app           Pointer to application structure
 * @param report        Where to save parsed data
 *
 * @return Status code.
 */
static te_errno
get_error(tapi_ethtool_app *app,
          tapi_ethtool_report *report)
{
    tapi_job_buffer_t *buffers = NULL;
    unsigned int count = 0;
    unsigned int i;
    te_errno rc = 0;

    rc = tapi_job_receive_many(TAPI_JOB_CHANNEL_SET(app->err_filter),
                               0, &buffers, &count);
    if (rc != 0)
        return rc;

    report->err_code = TE_EOK;
    for (i = 0; i < count; i++)
    {
        if (buffers[i].eos)
            break;

        report->err_out = TRUE;

        rc = te_string_append(&report->err_data, "%s",
                              te_string_value(&buffers[i].data));
        if (rc != 0)
            goto cleanup;

        if (strcasestr(te_string_value(&buffers[i].data),
                       "Operation not supported") != NULL)
            report->err_code = TE_EOPNOTSUPP;
    }

cleanup:

    tapi_job_buffers_free(buffers, count);

    return rc;
}

/**
 * Get data parsed from ethtool output.
 *
 * @param app           Pointer to application structure
 * @param report        Where to save parsed data
 * @param parse_stdout  If @c TRUE, parse stdout
 *
 * @return Status code.
 */
static te_errno
get_report(tapi_ethtool_app *app,
           tapi_ethtool_report *report,
           te_bool parse_stdout)
{
    te_errno rc;

    tapi_ethtool_destroy_report(report);

    report->cmd = app->cmd;

    rc = get_error(app, report);
    if (rc != 0)
        return TE_RC(TE_TAPI, rc);

    if (!parse_stdout)
        return 0;

    switch (report->cmd)
    {
        case TAPI_ETHTOOL_CMD_NONE:
            return get_if_props(app, &report->data.if_props);

        case TAPI_ETHTOOL_CMD_STATS:
            return get_stats(app, &report->data.stats);

        case TAPI_ETHTOOL_CMD_SHOW_PAUSE:
            return get_pause(app, &report->data.pause);

        case TAPI_ETHTOOL_CMD_SHOW_RING:
            return get_ring(app, &report->data.ring);

        default:
            ERROR("%s(): no report is defined for the current command",
                  __FUNCTION__);
            return TE_RC(TE_TAPI, TE_ENOENT);
    }
}

/* See description in tapi_ethtool.h */
te_errno
tapi_ethtool(tapi_job_factory_t *factory,
             const tapi_ethtool_opt *opt,
             tapi_ethtool_report *report)
{
    te_errno rc;
    te_errno rc_aux;
    tapi_ethtool_app *app = NULL;
    tapi_job_status_t status;

    rc = create_app(factory, opt, &app);
    if (rc != 0)
        return rc;

    rc = tapi_job_start(app->job);
    if (rc != 0)
        goto out;

    rc = tapi_job_wait(app->job, TAPI_ETHTOOL_TERM_TIMEOUT_MS,
                       &status);
    if (rc != 0)
        goto out;

    if (status.type != TAPI_JOB_STATUS_EXITED ||
        status.value != 0)
    {
        rc = TE_RC(TE_TAPI, TE_ESHCMD);
        if (report != NULL)
            get_report(app, report, FALSE);

        goto out;
    }

    if (report != NULL)
        rc = get_report(app, report, TRUE);

out:

    rc_aux = tapi_job_destroy(app->job, TAPI_ETHTOOL_TERM_TIMEOUT_MS);
    if (rc == 0)
        rc = rc_aux;

    free(app);
    return rc;
}

/* See description in tapi_ethtool.h */
void
tapi_ethtool_destroy_report(tapi_ethtool_report *report)
{
    switch (report->cmd)
    {
        case TAPI_ETHTOOL_CMD_STATS:
            te_kvpair_fini(&report->data.stats);
            break;

        default:
            return;
    }

    te_string_free(&report->err_data);

    *report = (tapi_ethtool_report)tapi_ethtool_default_report;
}

/* See description in tapi_ethtool.h */
te_errno
tapi_ethtool_get_stat(tapi_ethtool_report *report,
                      const char *name, long int *value)
{
    const char *value_str;

    value_str = te_kvpairs_get(&report->data.stats, name);
    if (value_str == NULL)
    {
        ERROR("%s(): there is no statistic named '%s'",
              __FUNCTION__, name);
        return TE_RC(TE_TAPI, TE_ENOENT);
    }

    return te_strtol(value_str, 10, value);
}
