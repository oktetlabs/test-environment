/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2026 Advanced Micro Devices, Inc. All rights reserved. */
/** @file
 * @brief dcb tool TAPI
 *
 * Implementation of TAPI to handle dcb tool.
 */

#include "te_defs.h"
#include "te_str.h"
#include "te_alloc.h"
#include "tapi_dcb.h"

#define CHECK(_exp) \
    do {                \
        rc = (_exp);    \
        if (rc != 0)    \
            return rc;  \
    } while (0)

/** How long to wait for DCB termination, in ms. */
#define TAPI_DCB_TERM_TIMEOUT_MS 1000

/**
 * Default time for a dcb command to complete, in ms. A set may make the
 * driver reconfigure and restart the interface.
 */
#define TAPI_DCB_TIMEOUT_MS 10000

/** How long to wait for the rest of dcb output after it exits, in ms. */
#define TAPI_DCB_OUTPUT_TIMEOUT_MS 1000

/** Maximum length of a regular expression used to parse dcb output. */
#define TAPI_DCB_MAX_STR 256

const tapi_dcb_opt tapi_dcb_default_opt = {
    .cmd = TAPI_DCB_CMD_SHOW_PFC,
    .timeout_ms = TAPI_DCB_TIMEOUT_MS,
    .if_name = NULL,
    .args = { },
};

const tapi_dcb_report tapi_dcb_default_report = {
    .cmd = TAPI_DCB_CMD_SHOW_PFC,

    .err_out = false,
    .err_data = TE_STRING_INIT,

    .out = false,
    .out_data = TE_STRING_INIT,

    .data = { },
};

/** Filters used to parse DCB output when `pfc show` command is supplied. */
typedef struct pfc_filters {
    /** Number of TCs that may simultaneously have PFC enabled. */
    tapi_job_channel_t *pfc_cap;
    /** MACsec bypass capability. */
    tapi_job_channel_t *mbc;
    /** Signal propagation delay, in link bits. */
    tapi_job_channel_t *delay;
    /** Bitmap indicating PFC enabled TCs. */
    tapi_job_channel_t *pfc_en;
    /** Transmitted PFC frames count. */
    tapi_job_channel_t *requests;
    /** Received PFC frames count. */
    tapi_job_channel_t *indications;
} pfc_filters;

/** Filters used to parse dcb output when `ets show` command is supplied. */
typedef struct ets_filters {
    /** Readonly host config or not. */
    tapi_job_channel_t *willing;
    /** Number of supported ETS TCs. */
    tapi_job_channel_t *ets_cap;
    /** CBS TSA support. */
    tapi_job_channel_t *cbs;
    /** Percentage of bandwidth allocated to each TC. */
    tapi_job_channel_t *tc_bw;
    /** Percentage of bandwidth allocated to each PG. */
    tapi_job_channel_t *pg_bw;
    /** TC <-> TSA mapping. */
    tapi_job_channel_t *tc_tsa;
    /** Priority <-> TC mapping. */
    tapi_job_channel_t *prio_tc;
    /** Recommended TC bandwidth. */
    tapi_job_channel_t *reco_tc_bw;
    /** Recommended TC TSA. */
    tapi_job_channel_t *reco_tc_tsa;
    /** Recommended Priority <-> TC mapping. */
    tapi_job_channel_t *reco_prio_tc;
} ets_filters;

/** Filters used to parse dcb output when `dcbx show` command is supplied. */
typedef struct dcbx_filters {
    /** DCBX negotiation mode host. */
    tapi_job_channel_t *neg_host;
    /** DCBX negotiation mode lld-managed. */
    tapi_job_channel_t *neg_lld;
    /** DCBX engine mode CEE. */
    tapi_job_channel_t *engine_cee;
    /** DCBX engine mode IEEE. */
    tapi_job_channel_t *engine_ieee;
    /** DCBX engine static config. */
    tapi_job_channel_t *static_conf;
} dcbx_filters;

/** Main structure describing dcb command. */
typedef struct tapi_dcb_app {
    /** DCB command. */
    tapi_dcb_cmd cmd;

    /** Job handle. */
    tapi_job_t *job;
    /** Channels for stdout and stderr. */
    tapi_job_channel_t *out_chs[2];

    /** Filter for reading stderr. */
    tapi_job_channel_t *err_filter;
    /** Filter for reading stdout. */
    tapi_job_channel_t *out_filter;

    union {
        /** Filters for interface PFC information. */
        pfc_filters pfc;
        /** Filters for interface ETS information. */
        ets_filters ets;
        /** Filters for interface DCBX information. */
        dcbx_filters dcbx;
    } out_filters; /**< Filters for parsing stdout. */
} tapi_dcb_app;

static te_errno fill_cmd_arg(const void *value, const void *, te_vec *args);

static const tapi_job_opt_bind dcb_binds[] = TAPI_JOB_OPT_SET(
    { &fill_cmd_arg, NULL, false, NULL, 0 }
);

/**
 * Append PFC configuration arguments to the dcb command line.
 *
 * @param[in] opt       DCB command line options.
 * @param[in,out] args  Command-line argument vector to update.
 */
static void
fill_set_pfc_cmd_args(const tapi_dcb_opt *opt, te_vec *args)
{
    const tapi_dcb_pfc_config *pfc = &opt->args.pfc;
    uint8_t i;

    te_vec_append_str_fmt(args, "prio-pfc");
    for (i = 0; i < TAPI_DCB_NUM_PRIORITIES; i++)
        te_vec_append_str_fmt(args, "%u:%s",
                              i, pfc->pfc_en[i] ? "on" : "off");

    te_vec_append_str_fmt(args, "macsec-bypass");
    te_vec_append_str_fmt(args, "%s", pfc->mbc ? "on" : "off");

    te_vec_append_str_fmt(args, "delay");
    te_vec_append_str_fmt(args, "%u", pfc->delay);
}

/**
 * Append ETS configuration arguments to the dcb command line.
 *
 * @param[in] opt       DCB command line options.
 * @param[in,out] args  Command-line argument vector to update.
 *
 * @return Status code.
 * @retval TE_EINVAL    A TSA value has no name the dcb tool accepts.
 */
static te_errno
fill_set_ets_cmd_args(const tapi_dcb_opt *opt, te_vec *args)
{
    const tapi_dcb_ets_config *ets = &opt->args.ets;
    uint8_t i;

    /* The dcb tool accepts only TSA names, not numeric values. */
    for (i = 0; i < TAPI_DCB_NUM_TCS_MAX; i++)
    {
        if (te_enum_map_from_any_value(tapi_dcb_tsa_map, ets->tc_tsa[i],
                                       NULL) == NULL ||
            te_enum_map_from_any_value(tapi_dcb_tsa_map,
                                       ets->reco_tc_tsa[i], NULL) == NULL)
        {
            ERROR("TSA of TC %u (%u, recommended %u) cannot be passed to "
                  "the dcb tool: it accepts only strict, cbs, ets and "
                  "vendor", i, ets->tc_tsa[i], ets->reco_tc_tsa[i]);
            return TE_RC(TE_TAPI, TE_EINVAL);
        }
    }

    te_vec_append_str_fmt(args, "willing");
    te_vec_append_str_fmt(args, "%s", ets->willing ? "on" : "off");

    te_vec_append_str_fmt(args, "tc-tsa");
    for (i = 0; i < TAPI_DCB_NUM_TCS_MAX; i++)
        te_vec_append_str_fmt(args, "%u:%s",
                              i, tapi_dcb_tsa2str(ets->tc_tsa[i]));

    te_vec_append_str_fmt(args, "reco-tc-tsa");
    for (i = 0; i < TAPI_DCB_NUM_TCS_MAX; i++)
        te_vec_append_str_fmt(args, "%u:%s",
                              i, tapi_dcb_tsa2str(ets->reco_tc_tsa[i]));

    te_vec_append_str_fmt(args, "pg-bw");
    for (i = 0; i < TAPI_DCB_NUM_TCS_MAX; i++)
        te_vec_append_str_fmt(args, "%u:%u", i, ets->pg_bw[i]);

    te_vec_append_str_fmt(args, "tc-bw");
    for (i = 0; i < TAPI_DCB_NUM_TCS_MAX; i++)
        te_vec_append_str_fmt(args, "%u:%u", i, ets->tc_bw[i]);

    te_vec_append_str_fmt(args, "reco-tc-bw");
    for (i = 0; i < TAPI_DCB_NUM_TCS_MAX; i++)
        te_vec_append_str_fmt(args, "%u:%u", i, ets->reco_tc_bw[i]);

    te_vec_append_str_fmt(args, "prio-tc");
    for (i = 0; i < TAPI_DCB_NUM_PRIORITIES; i++)
        te_vec_append_str_fmt(args, "%u:%u", i, ets->prio_tc[i]);

    te_vec_append_str_fmt(args, "reco-prio-tc");
    for (i = 0; i < TAPI_DCB_NUM_PRIORITIES; i++)
        te_vec_append_str_fmt(args, "%u:%u", i, ets->reco_prio_tc[i]);

    return 0;
}

/**
 * Append DCBX configuration arguments to the dcb command line.
 *
 * @param[in] opt       DCB command line options.
 * @param[in,out] args  Command-line argument vector to update.
 *
 * @return Status code.
 * @retval TE_EINVAL    The mode is 0 or has unknown flags.
 */
static te_errno
fill_set_dcbx_cmd_args(const tapi_dcb_opt *opt, te_vec *args)
{
    const te_enum_map *entry;
    tapi_dcb_dcbx_config dcbx = opt->args.dcbx;

    /*
     * The tool sends the flags given on the command line; with no flags
     * it prints help and exits with success without sending anything.
     */
    if (dcbx == 0)
    {
        ERROR("The dcb tool cannot set DCBX mode 0");
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    for (entry = tapi_dcb_dcbx_flag_map; entry->name != NULL; entry++)
    {
        if ((dcbx & entry->value) != 0)
            te_vec_append_str_fmt(args, "%s", entry->name);

        dcbx &= (tapi_dcb_dcbx_config)(~entry->value);
    }

    if (dcbx != 0)
        return TE_RC(TE_TAPI, TE_EINVAL);

    return 0;
}

/**
 * Fill dcb command in command line.
 *
 * @param[in] value     DCB command line options.
 * @param[in] priv      Unused param.
 * @param[out] args     Where to add command string.
 *
 * @return Status code.
 */
static te_errno
fill_cmd_arg(const void *value, const void *priv, te_vec *args)
{
    te_errno rc = 0;
    const tapi_dcb_opt *opt = (const tapi_dcb_opt *)value;

    UNUSED(priv);

    switch (opt->cmd)
    {
        case TAPI_DCB_CMD_SHOW_PFC:
            te_vec_append_str_fmt(args, "-s");
            te_vec_append_str_fmt(args, "-j");
            te_vec_append_str_fmt(args, "pfc");
            te_vec_append_str_fmt(args, "show");
            te_vec_append_str_fmt(args, "dev");
            te_vec_append_str_fmt(args, "%s", opt->if_name);
            break;

        case TAPI_DCB_CMD_SHOW_ETS:
            te_vec_append_str_fmt(args, "-j");
            te_vec_append_str_fmt(args, "ets");
            te_vec_append_str_fmt(args, "show");
            te_vec_append_str_fmt(args, "dev");
            te_vec_append_str_fmt(args, "%s", opt->if_name);
            break;

        case TAPI_DCB_CMD_SHOW_DCBX:
            te_vec_append_str_fmt(args, "-j");
            te_vec_append_str_fmt(args, "dcbx");
            te_vec_append_str_fmt(args, "show");
            te_vec_append_str_fmt(args, "dev");
            te_vec_append_str_fmt(args, "%s", opt->if_name);
            break;

        case TAPI_DCB_CMD_SET_PFC:
            te_vec_append_str_fmt(args, "pfc");
            te_vec_append_str_fmt(args, "set");
            te_vec_append_str_fmt(args, "dev");
            te_vec_append_str_fmt(args, "%s", opt->if_name);

            fill_set_pfc_cmd_args(opt, args);
            break;

        case TAPI_DCB_CMD_SET_ETS:
            te_vec_append_str_fmt(args, "ets");
            te_vec_append_str_fmt(args, "set");
            te_vec_append_str_fmt(args, "dev");
            te_vec_append_str_fmt(args, "%s", opt->if_name);

            rc = fill_set_ets_cmd_args(opt, args);
            break;

        case TAPI_DCB_CMD_SET_DCBX:
            te_vec_append_str_fmt(args, "dcbx");
            te_vec_append_str_fmt(args, "set");
            te_vec_append_str_fmt(args, "dev");
            te_vec_append_str_fmt(args, "%s", opt->if_name);

            rc = fill_set_dcbx_cmd_args(opt, args);
            break;

        default:
            ERROR("Unknown command code: %d", opt->cmd);
            rc = TE_RC(TE_TAPI, TE_EINVAL);
            break;
    }

    return rc;
}

/**
 * Add filter to extract a single uint value from DCB output.
 *
 * @param[in] app           Structure describing DCB command.
 * @param[in] filter_name   Name of the filter.
 * @param[out] filter       Where to save pointer to the created filter.
 *
 * @return Status code.
 */
static te_errno
add_uint_filter(tapi_dcb_app *app,
                const char *filter_name,
                tapi_job_channel_t **filter)
{
    char re_buf[TAPI_DCB_MAX_STR] = "";
    te_errno rc;

    rc = te_snprintf(re_buf, sizeof(re_buf), "\"%s\":([0-9]+)", filter_name);
    if (rc != 0)
        return rc;

    rc = tapi_job_attach_filter(TAPI_JOB_CHANNEL_SET(app->out_chs[0]),
                                filter_name, true, 0, filter);
    if (rc != 0)
        return rc;

    return tapi_job_filter_add_regexp(*filter, re_buf, 1);
}

/**
 * Add filter to extract a single bool value from DCB output.
 *
 * @param[in] app           Structure describing DCB command.
 * @param[in] filter_name   Name of the filter.
 * @param[out] filter       Where to save pointer to the created filter.
 *
 * @return Status code.
 */
static te_errno
add_bool_filter(tapi_dcb_app *app,
                const char *filter_name,
                tapi_job_channel_t **filter)
{
    char re_buf[TAPI_DCB_MAX_STR] = "";
    te_errno rc;

    rc = te_snprintf(re_buf, sizeof(re_buf), "\"%s\":(true|false)",
                     filter_name);
    if (rc != 0)
        return rc;

    rc = tapi_job_attach_filter(TAPI_JOB_CHANNEL_SET(app->out_chs[0]),
                                filter_name, true, 0, filter);
    if (rc != 0)
        return rc;

    return tapi_job_filter_add_regexp(*filter, re_buf, 1);
}

/**
 * Add filter to extract an array from DCB output.
 *
 * @param[in] app           Structure describing DCB command.
 * @param[in] filter_name   Name of the filter.
 * @param[out] filter       Where to save pointer to the created filter.
 *
 * @return Status code.
 */
static te_errno
add_array_filter(tapi_dcb_app *app,
                 const char *filter_name,
                 tapi_job_channel_t **filter)
{
    /* The tool prints "???" for a value without a name. */
    const char *arr_elem = "(?:true|false|[0-9]+|\"[a-z]+\"|\"\\?\\?\\?\")";
    char re_buf[TAPI_DCB_MAX_STR] = "";
    te_errno rc;

    rc = te_snprintf(re_buf, sizeof(re_buf),
                     "\"%s\":\\[(%s(?:,%s)*)\\]",
                     filter_name, arr_elem, arr_elem);
    if (rc != 0)
        return rc;

    rc = tapi_job_attach_filter(TAPI_JOB_CHANNEL_SET(app->out_chs[0]),
                                filter_name, true, 0, filter);
    if (rc != 0)
        return rc;

    return tapi_job_filter_add_regexp(*filter, re_buf, 1);
}

/**
 * Attach filters used to parse DCB output when it is run
 * with @c TAPI_DCB_CMD_SHOW_PFC command.
 *
 * @param[in] app   DCB application structure.
 *
 * @return Status code.
 */
static te_errno
attach_pfc_filters(tapi_dcb_app *app)
{
    te_errno rc;
    pfc_filters *pfc = &app->out_filters.pfc;

    CHECK(add_uint_filter(app, "pfc_cap", &pfc->pfc_cap));
    CHECK(add_bool_filter(app, "macsec_bypass", &pfc->mbc));
    CHECK(add_uint_filter(app, "delay", &pfc->delay));
    CHECK(add_array_filter(app, "prio_pfc", &pfc->pfc_en));
    CHECK(add_array_filter(app, "requests", &pfc->requests));
    CHECK(add_array_filter(app, "indications", &pfc->indications));

    return 0;
}

/**
 * Attach filters used to parse DCB output when it is run
 * with @c TAPI_DCB_CMD_SHOW_ETS command.
 *
 * @param[in] app   DCB application structure.
 *
 * @return Status code.
 */
static te_errno
attach_ets_filters(tapi_dcb_app *app)
{
    te_errno rc;
    ets_filters *ets = &app->out_filters.ets;

    CHECK(add_bool_filter(app, "willing", &ets->willing));
    CHECK(add_uint_filter(app, "ets_cap", &ets->ets_cap));
    CHECK(add_bool_filter(app, "cbs", &ets->cbs));
    CHECK(add_array_filter(app, "tc_bw", &ets->tc_bw));
    CHECK(add_array_filter(app, "pg_bw", &ets->pg_bw));
    CHECK(add_array_filter(app, "tc_tsa", &ets->tc_tsa));
    CHECK(add_array_filter(app, "prio_tc", &ets->prio_tc));
    CHECK(add_array_filter(app, "reco_tc_bw", &ets->reco_tc_bw));
    CHECK(add_array_filter(app, "reco_tc_tsa", &ets->reco_tc_tsa));
    CHECK(add_array_filter(app, "reco_prio_tc", &ets->reco_prio_tc));

    return 0;
}

/**
 * Attach filters used to parse DCB output when it is run
 * with @c TAPI_DCB_CMD_SHOW_DCBX command.
 *
 * @param[in] app   DCB application structure.
 *
 * @return Status code.
 */
static te_errno
attach_dcbx_filters(tapi_dcb_app *app)
{
    te_errno rc;
    dcbx_filters *dcbx = &app->out_filters.dcbx;

    CHECK(add_bool_filter(app, "host", &dcbx->neg_host));
    CHECK(add_bool_filter(app, "lld_managed", &dcbx->neg_lld));
    CHECK(add_bool_filter(app, "cee", &dcbx->engine_cee));
    CHECK(add_bool_filter(app, "ieee", &dcbx->engine_ieee));
    CHECK(add_bool_filter(app, "static", &dcbx->static_conf));

    return 0;
}

/**
 * Attach filters used to parse DCB output.
 * Filters are chosen depending on specific DCB command.
 *
 * @param[in] app   DCB application structure.
 *
 * @return Status code.
 */
static te_errno
attach_filters(tapi_dcb_app *app)
{
    te_errno rc;

    switch (app->cmd)
    {
        case TAPI_DCB_CMD_SHOW_PFC:
            rc = attach_pfc_filters(app);
            break;

        case TAPI_DCB_CMD_SHOW_ETS:
            rc = attach_ets_filters(app);
            break;

        case TAPI_DCB_CMD_SHOW_DCBX:
            rc = attach_dcbx_filters(app);
            break;

        case TAPI_DCB_CMD_SET_PFC:
        case TAPI_DCB_CMD_SET_ETS:
        case TAPI_DCB_CMD_SET_DCBX:
            rc = 0;
            break;

        default:
            rc = TE_RC(TE_TAPI, TE_EINVAL);
            ERROR("Unknown command code: %d", app->cmd);
            break;
    }

    return rc;
}

/**
 * Create a job to run DCB application.
 *
 * @param[in] factory   Job factory.
 * @param[in] opt       DCB options.
 * @param[out] app      Where to save pointer to application structure.
 *
 * @return Status code.
 */
static te_errno
create_app(tapi_job_factory_t *factory,
           const tapi_dcb_opt *opt,
           tapi_dcb_app **app)
{
    tapi_dcb_app *result = NULL;
    te_vec dcb_args = TE_VEC_INIT(char *);
    te_errno rc;

    tapi_job_simple_desc_t desc;

    result = TE_ALLOC(sizeof(*result));

    result->cmd = opt->cmd;

    rc = tapi_job_opt_build_args("dcb", dcb_binds, opt, &dcb_args);
    if (rc != 0)
    {
        ERROR("Failed to build dcb command line arguments");
        goto out;
    }

    memset(&desc, 0, sizeof(desc));

    desc.program = "dcb";
    desc.argv = (const char **)(dcb_args.data.ptr);
    desc.job_loc = &result->job;
    desc.stdout_loc = &result->out_chs[0];
    desc.stderr_loc = &result->out_chs[1];
    desc.filters = TAPI_JOB_SIMPLE_FILTERS(
        {
            .use_stdout  = true,
            .readable    = true,
            .log_level   = TE_LL_RING,
            .filter_name = "out",
            .filter_var = &result->out_filter,
        },
        {
            .use_stderr  = true,
            .readable    = true,
            .log_level   = TE_LL_RING,
            .filter_name = "err",
            .filter_var = &result->err_filter,
        });

    rc = tapi_job_simple_create(factory, &desc);
    if (rc != 0)
    {
        ERROR("Failed to create job instance for dcb app");
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
    te_vec_deep_free(&dcb_args);

    if (rc != 0)
    {
        if (result->job != NULL)
            tapi_job_destroy(result->job, 0);

        free(result);
    }

    return rc;
}

/**
 * Read a boolean value from a job filter.
 *
 * @param[in] filter    Job filter from which to receive the value.
 * @param[out] out      Where to store the parsed value.
 *
 * @return Status code.
 */
static te_errno
get_bool_report(tapi_job_channel_t *filter, bool *out)
{
    te_errno rc;
    te_string val = TE_STRING_INIT;

    rc = tapi_job_receive_single(filter, &val, 0);
    if (rc != 0)
        goto out;

    if (strcmp(val.ptr, "true") == 0)
        *out = true;
    else if (strcmp(val.ptr, "false") == 0)
        *out = false;
    else
        rc = TE_RC(TE_TAPI, TE_EINVAL);

out:
    te_string_free(&val);

    return rc;
}

/**
 * Read a optional boolean value from a job filter.
 *
 * @param[in] filter    Job filter from which to receive the value.
 * @param[out] out      Where to store the parsed value. Can be not changed
 *                      because a value may be absent.
 *
 * @return Status code.
 */
static te_errno
get_optional_bool_report(tapi_job_channel_t *filter, bool *out)
{
    if (!tapi_job_filters_have_data(TAPI_JOB_CHANNEL_SET(filter), 0))
        return 0;

    return get_bool_report(filter, out);
}

/**
 * Read an unsigned 16-bit integer value from a job filter.
 *
 * @param[in] filter    Job filter from which to receive the value.
 * @param[out] out      Where to store the parsed value.
 *
 * @return Status code.
 */
static te_errno
get_u16_report(tapi_job_channel_t *filter, uint16_t *out)
{
    te_errno rc;
    te_string val = TE_STRING_INIT;
    uint64_t res;

    rc = tapi_job_receive_single(filter, &val, 0);
    if (rc != 0)
        goto out;

    rc = te_str_to_uint64(val.ptr, 10, &res);
    if (rc != 0)
       goto out;

    if (res > UINT16_MAX)
    {
        rc = TE_RC(TE_TAPI, TE_ERANGE);
        goto out;
    }

    *out = (uint16_t)res;

out:
    te_string_free(&val);

    return rc;
}

/**
 * Read an unsigned 8-bit integer value from a job filter.
 *
 * @param[in] filter    Job filter from which to receive the value.
 * @param[out] out      Where to store the parsed value.
 *
 * @return Status code.
 */
static te_errno
get_u8_report(tapi_job_channel_t *filter, uint8_t *out)
{
    te_errno rc;
    uint16_t res;

    rc = get_u16_report(filter, &res);
    if (rc != 0)
        return rc;

    if (res > UINT8_MAX)
        return TE_RC(TE_TAPI, TE_ERANGE);

    *out = (uint8_t)res;

    return 0;
}

/**
 * Read an array of boolean values from a job filter.
 *
 * @param[in] filter    Job filter from which to receive the value.
 * @param[out] out      Where to store the parsed values.
 *
 * @return Status code.
 */
static te_errno
get_bool_array_report(tapi_job_channel_t *filter,
                      bool out[static TAPI_DCB_NUM_TCS_MAX])
{
    te_errno rc;
    te_string val = TE_STRING_INIT;
    char *token;
    char *saveptr;
    int i = 0;

    rc = tapi_job_receive_single(filter, &val, 0);
    if (rc != 0)
        goto out;

    token = strtok_r(val.ptr, ",", &saveptr);

    while (token != NULL && i < TAPI_DCB_NUM_TCS_MAX)
    {
        out[i++] = (strcmp(token, "true") == 0);
        token = strtok_r(NULL, ",", &saveptr);
    }

    if (token != NULL || i != TAPI_DCB_NUM_TCS_MAX)
        rc = TE_RC(TE_TAPI, TE_EINVAL);

out:
    te_string_free(&val);

    return rc;
}

/**
 * Read an array of unsigned 64-bit integer values from a job filter.
 *
 * @param[in] filter    Job filter from which to receive the value.
 * @param[out] out      Where to store the parsed values.
 *
 * @return Status code.
 */
static te_errno
get_u64_array_report(tapi_job_channel_t *filter,
                     uint64_t out[static TAPI_DCB_NUM_TCS_MAX])
{
    te_errno rc;
    te_string val = TE_STRING_INIT;
    char *token;
    char *saveptr;
    int i = 0;

    rc = tapi_job_receive_single(filter, &val, 0);
    if (rc != 0)
        goto out;

    token = strtok_r(val.ptr, ",", &saveptr);

    while (token != NULL && i < TAPI_DCB_NUM_TCS_MAX)
    {
        rc = te_str_to_uint64(token, 10, &out[i]);
        if (rc != 0)
            goto out;

        token = strtok_r(NULL, ",", &saveptr);
        i++;
    }

    if (token != NULL || i != TAPI_DCB_NUM_TCS_MAX)
        rc = TE_RC(TE_TAPI, TE_EINVAL);

out:
    te_string_free(&val);

    return rc;
}

/**
 * Read an array of unsigned 8-bit integer values from a job filter.
 *
 * @param[in] filter    Job filter from which to receive the value.
 * @param[out] out      Where to store the parsed values.
 *
 * @return Status code.
 */
static te_errno
get_u8_array_report(tapi_job_channel_t *filter,
                    uint8_t out[static TAPI_DCB_NUM_TCS_MAX])
{
    te_errno rc;
    te_string val = TE_STRING_INIT;
    char *token;
    char *saveptr;
    int i = 0;

    rc = tapi_job_receive_single(filter, &val, 0);
    if (rc != 0)
        goto out;

    token = strtok_r(val.ptr, ",", &saveptr);

    while (token != NULL && i < TAPI_DCB_NUM_TCS_MAX)
    {
        uint64_t res;

        rc = te_str_to_uint64(token, 10, &res);
        if (rc != 0)
            goto out;

        if (res > UINT8_MAX)
        {
            rc = TE_RC(TE_TAPI, TE_ERANGE);
            goto out;
        }

        out[i++] = res;
        token = strtok_r(NULL, ",", &saveptr);
    }

    if (token != NULL || i != TAPI_DCB_NUM_TCS_MAX)
        rc = TE_RC(TE_TAPI, TE_EINVAL);

out:
    te_string_free(&val);

    return rc;
}

/**
 * Read an array of Transmission Selection Algorithm values from a job filter.
 *
 * @param[in] filter    Job filter from which to receive the values.
 * @param[in] name      Field name for logging.
 * @param[out] out      Where to store the parsed TSA values.
 *
 * @return Status code.
 * @retval TE_EPROTO    The tool printed a TSA without a name.
 * @retval TE_EINVAL    The output cannot be parsed.
 */
static te_errno
get_tsa_array_report(tapi_job_channel_t *filter, const char *name,
                     tapi_dcb_tsa out[static TAPI_DCB_NUM_TCS_MAX])
{
    te_errno rc;
    te_string val = TE_STRING_INIT;
    char *token;
    char *saveptr;
    int i = 0;

    rc = tapi_job_receive_single(filter, &val, 0);
    if (rc != 0)
        goto out;

    token = strtok_r(val.ptr, ",", &saveptr);

    while (token != NULL && i < TAPI_DCB_NUM_TCS_MAX)
    {
        if (strcmp("\"strict\"", token) == 0)
            out[i++] = TAPI_DCB_TSA_STRICT;
        else if (strcmp("\"ets\"", token) == 0)
            out[i++] = TAPI_DCB_TSA_ETS;
        else if (strcmp("\"cbs\"", token) == 0)
            out[i++] = TAPI_DCB_TSA_CBS;
        else if (strcmp("\"vendor\"", token) == 0)
            out[i++] = TAPI_DCB_TSA_VENDOR;
        else if (strcmp("\"???\"", token) == 0)
        {
            ERROR("%s of TC %d has no name in the dcb output, so its value "
                  "is unknown", name, i);
            rc = TE_RC(TE_TAPI, TE_EPROTO);
            goto out;
        }
        else
        {
            ERROR("Unexpected %s of TC %d in the dcb output: %s",
                  name, i, token);
            rc = TE_RC(TE_TAPI, TE_EINVAL);
            goto out;
        }

        token = strtok_r(NULL, ",", &saveptr);
    }

    if (token != NULL || i != TAPI_DCB_NUM_TCS_MAX)
        rc = TE_RC(TE_TAPI, TE_EINVAL);

out:
    te_string_free(&val);

    return rc;
}

/**
 * Obtain interface PFC properties from dcb output via filters.
 *
 * @param[in] app               DCB application structure.
 * @param[in,out] state         Where to save parsed PFC properties.
 *
 * @return Status code.
 */
static te_errno
get_dcb_pfc_state(tapi_dcb_app *app, tapi_dcb_pfc_state *state)
{
    te_errno rc = 0;
    pfc_filters *pfc = &app->out_filters.pfc;

    memset(state, 0, sizeof(*state));

    CHECK(get_u8_report(pfc->pfc_cap, &state->pfc_cap));
    CHECK(get_bool_report(pfc->mbc, &state->config.mbc));
    CHECK(get_u16_report(pfc->delay, &state->config.delay));
    CHECK(get_bool_array_report(pfc->pfc_en, state->config.pfc_en));
    CHECK(get_u64_array_report(pfc->requests, state->requests));
    CHECK(get_u64_array_report(pfc->indications, state->indications));

    return rc;
}

/**
 * Obtain interface ETS properties from dcb output via filters.
 *
 * @param[in] app               DCB application structure.
 * @param[in,out] state         Where to save parsed ETS properties.
 *
 * @return Status code.
 */
static te_errno
get_dcb_ets_state(tapi_dcb_app *app, tapi_dcb_ets_state *state)
{
    te_errno rc = 0;
    ets_filters *ets = &app->out_filters.ets;

    memset(state, 0, sizeof(*state));

    CHECK(get_bool_report(ets->willing, &state->config.willing));
    CHECK(get_u8_report(ets->ets_cap, &state->ets_cap));
    CHECK(get_bool_report(ets->cbs, &state->cbs));
    CHECK(get_u8_array_report(ets->tc_bw, state->config.tc_bw));
    CHECK(get_u8_array_report(ets->pg_bw, state->config.pg_bw));
    CHECK(get_tsa_array_report(ets->tc_tsa, "tc_tsa",
                               state->config.tc_tsa));
    CHECK(get_u8_array_report(ets->prio_tc, state->config.prio_tc));
    CHECK(get_u8_array_report(ets->reco_tc_bw, state->config.reco_tc_bw));
    CHECK(get_tsa_array_report(ets->reco_tc_tsa, "reco_tc_tsa",
                               state->config.reco_tc_tsa));
    CHECK(get_u8_array_report(ets->reco_prio_tc, state->config.reco_prio_tc));

    return rc;
}

/**
 * Obtain interface DCBX properties from dcb output via filters.
 *
 * @param[in] app               DCB application structure.
 * @param[in,out] state         Where to save parsed DCBX properties.
 *
 * @return Status code.
 */
static te_errno
get_dcb_dcbx_state(tapi_dcb_app *app, tapi_dcb_dcbx_state *state)
{
    const dcbx_filters *dcbx = &app->out_filters.dcbx;
    const struct {
        tapi_job_channel_t *filter;
        tapi_dcb_dcbx_state flag;
    } filter_map[] = {
        { .filter = dcbx->neg_host, .flag = TAPI_DCB_DCBX_HOST, },
        { .filter = dcbx->neg_lld, .flag = TAPI_DCB_DCBX_LLD_MANAGED, },
        { .filter = dcbx->engine_cee, .flag = TAPI_DCB_DCBX_VER_CEE, },
        { .filter = dcbx->engine_ieee, .flag = TAPI_DCB_DCBX_VER_IEEE, },
        { .filter = dcbx->static_conf, .flag = TAPI_DCB_DCBX_STATIC, },
    };
    tapi_dcb_dcbx_state result = 0;
    size_t i;
    te_errno rc;

    for (i = 0; i < TE_ARRAY_LEN(filter_map); i++)
    {
        bool enabled = false;

        rc = get_optional_bool_report(filter_map[i].filter, &enabled);
        if (rc != 0)
            return rc;

        if (enabled)
            result |= filter_map[i].flag;
    }

    /* Mode 0 is valid: the tool prints no flags for it. */
    *state = result;
    return 0;
}

/**
 * Read the output of a dcb job from a filter up to the end of stream.
 *
 * The job has exited, but its output may still be on the way.
 *
 * @param[in] filter    Filter to read from.
 * @param[out] data     String to append the output to.
 * @param[out] any      Set to @c true if there is some output.
 *
 * @return Status code.
 */
static te_errno
read_to_eos(tapi_job_channel_t *filter, te_string *data, bool *any)
{
    bool lost = false;
    bool eos = false;
    te_errno rc = 0;

    while (!eos)
    {
        tapi_job_buffer_t *buffers = NULL;
        unsigned int count = 0;
        unsigned int i;

        rc = tapi_job_receive_many(TAPI_JOB_CHANNEL_SET(filter),
                                   TAPI_DCB_OUTPUT_TIMEOUT_MS,
                                   &buffers, &count);

        for (i = 0; i < count; i++)
        {
            if (buffers[i].dropped > 0)
            {
                ERROR("%u messages of dcb output were dropped",
                      buffers[i].dropped);
                lost = true;
            }

            if (buffers[i].eos)
            {
                eos = true;
                break;
            }

            *any = true;
            te_string_append(data, "%s", te_string_value(&buffers[i].data));
        }

        tapi_job_buffers_free(buffers, count);

        if (rc == 0 && lost)
            rc = TE_RC(TE_TAPI, TE_ENOBUFS);

        if (rc != 0)
            break;

        if (count == 0)
        {
            rc = TE_RC(TE_TAPI, TE_ETIMEDOUT);
            break;
        }
    }

    if (rc != 0)
        ERROR("Failed to read dcb output to the end: %r", rc);

    return rc;
}

/**
 * Read stderr output.
 *
 * @param[in] app       Pointer to application structure.
 * @param[out] report   Where to save parsed data.
 *
 * @return Status code.
 */
static te_errno
get_error(tapi_dcb_app *app, tapi_dcb_report *report)
{
    return read_to_eos(app->err_filter, &report->err_data, &report->err_out);
}

/**
 * Read stdout output.
 *
 * @param[in] app       Pointer to application structure.
 * @param[out] report   Where to save obtained data.
 *
 * @return Status code.
 */
static te_errno
get_stdout(tapi_dcb_app *app, tapi_dcb_report *report)
{
    return read_to_eos(app->out_filter, &report->out_data, &report->out);
}

/**
 * Get data parsed from dcb output.
 *
 * @param[in] app           Pointer to application structure.
 * @param[in,out] report    Where to save parsed data.
 * @param[in] parse_stdout  If @c true, parse stdout.
 *
 * @return Status code.
 */
static te_errno
get_report(tapi_dcb_app *app, tapi_dcb_report *report, bool parse_stdout)
{
    te_errno rc;

    tapi_dcb_destroy_report(report);

    report->cmd = app->cmd;

    rc = get_error(app, report);
    if (rc != 0)
        return TE_RC(TE_TAPI, rc);

    rc = get_stdout(app, report);
    if (rc != 0)
        return TE_RC(TE_TAPI, rc);

    if (!parse_stdout)
        return 0;

    switch (report->cmd)
    {
        case TAPI_DCB_CMD_SHOW_PFC:
            rc = get_dcb_pfc_state(app, &report->data.pfc);
            break;

        case TAPI_DCB_CMD_SHOW_ETS:
            rc = get_dcb_ets_state(app, &report->data.ets);
            break;

        case TAPI_DCB_CMD_SHOW_DCBX:
            rc = get_dcb_dcbx_state(app, &report->data.dcbx);
            break;

        case TAPI_DCB_CMD_SET_PFC:
        case TAPI_DCB_CMD_SET_ETS:
        case TAPI_DCB_CMD_SET_DCBX:
            break;

        default:
            ERROR("No report is defined for the command: %d", report->cmd);
            rc = TE_RC(TE_TAPI, TE_ENOENT);
            break;
    }

    if (rc == 0 && report->err_out)
    {
        ERROR("Output of stderr is not empty");
        rc = TE_RC(TE_TAPI, TE_EFAIL);
    }

    return rc;
}

/* See description in tapi_dcb.h */
tapi_dcb_cmd
tapi_dcb_str2cmd(const char *cmd)
{
    const int unknown = -1;
    tapi_dcb_cmd val;

    val = te_enum_map_from_str(tapi_dcb_cmd_enum_map, cmd, unknown);
    if (val == unknown)
        abort();

    return val;
}

/* See description in tapi_dcb.h */
const char *
tapi_dcb_cmd2str(const tapi_dcb_cmd cmd)
{
    return te_enum_map_from_value(tapi_dcb_cmd_enum_map, cmd);
}

/* See description in tapi_dcb.h */
te_errno
tapi_dcb(tapi_job_factory_t *factory,
         const tapi_dcb_opt *opt,
         tapi_dcb_report *report)
{
    te_errno rc;
    te_errno rc_aux;
    tapi_dcb_app *app = NULL;
    tapi_job_status_t status;
    tapi_dcb_report local = tapi_dcb_default_report;
    tapi_dcb_report *result = (report != NULL) ? report : &local;
    bool success = false;

    *result = tapi_dcb_default_report;
    result->cmd = opt->cmd;

    rc = create_app(factory, opt, &app);
    if (rc != 0)
        goto out;

    rc = tapi_job_start(app->job);
    if (rc != 0)
        goto out;

    rc = tapi_job_wait(app->job, opt->timeout_ms, &status);
    if (TE_RC_GET_ERROR(rc) == TE_EINPROGRESS)
    {
        tapi_job_buffer_t *buffers = NULL;
        unsigned int count = 0;
        unsigned int i;

        /* Keep what the job has printed to stderr before it is killed. */
        tapi_job_receive_many(TAPI_JOB_CHANNEL_SET(app->err_filter), 0,
                              &buffers, &count);
        for (i = 0; i < count; i++)
        {
            if (!buffers[i].eos)
            {
                result->err_out = true;
                te_string_append(&result->err_data, "%s",
                                 te_string_value(&buffers[i].data));
            }
        }
        tapi_job_buffers_free(buffers, count);

        ERROR("dcb did not complete in %d ms, stderr: '%s'",
              opt->timeout_ms, te_string_value(&result->err_data));
        rc = TE_RC(TE_TAPI, TE_ETIMEDOUT);
        goto out;
    }
    if (rc != 0)
        goto out;

    success = (status.type == TAPI_JOB_STATUS_EXITED && status.value == 0);

    /* Parse stdout only if the command completed successfully. */
    rc = get_report(app, result, success);

    if (!success)
    {
        /* A failure to read the output is reported over the exit status */
        if (rc == 0)
            rc = TE_RC(TE_TAPI, TE_ESHCMD);

        /* The caller decides whether the failure is expected. */
        RING("dcb %s failed: %r", tapi_dcb_cmd2str(opt->cmd), rc);
    }

out:
    if (app != NULL)
    {
        rc_aux = tapi_job_destroy(app->job, TAPI_DCB_TERM_TIMEOUT_MS);
        if (rc == 0)
            rc = rc_aux;

        free(app);
    }

    tapi_dcb_destroy_report(&local);
    return rc;
}

/* See description in tapi_dcb.h */
void
tapi_dcb_destroy_report(tapi_dcb_report *report)
{
    if (report == NULL)
        return;

    te_string_free(&report->err_data);
    te_string_free(&report->out_data);

    *report = (tapi_dcb_report)tapi_dcb_default_report;
}
