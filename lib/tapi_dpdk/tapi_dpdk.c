/** @file
 * @brief Test API for DPDK
 *
 * Test API for DPDK helper functions (implementation)
 *
 * Copyright (C) 2019 OKTET Labs. All rights reserved.
 *
 *
 * @author Igor Romanov <Igor.Romanov@oktetlabs.ru>
 */

#define TE_LGR_USER     "TAPI DPDK"

#include "te_config.h"

#include <stdio.h>

#include "tapi_dpdk.h"
#include "tapi_mem.h"
#include "tapi_test.h"
#include "tapi_rpc_rte_eal.h"
#include "tapi_file.h"
#include "te_ethernet.h"
#include "tapi_job_factory_rpc.h"

#define COMMANDS_FILE_NAME "testpmd_commands"
#define TESTPMD_MAX_PARAM_LEN 64
#define MAKE_TESTPMD_CMD(_cmd) (TAPI_DPDK_TESTPMD_COMMAND_PREFIX _cmd)
#define MAKE_TESTPMD_ARG(_arg) (TAPI_DPDK_TESTPMD_ARG_PREFIX _arg)
#define TESTPMD_CMD_POST_SETUP "port start all\nshow port info all\n"
#define TESTPMD_TOTAL_MBUFS_MIN 2048
#define MBUF_OVERHEAD 256
#define TESTPMD_MIN_N_CORES 2

typedef enum testpmd_param_type {
    TESTPMD_PARAM_TYPE_UINT64,
    TESTPMD_PARAM_TYPE_STRING
} testpmd_param_type;

typedef struct testpmd_param {
    const char *key;
    testpmd_param_type type;
    union {
        uint64_t val;
        char str_val[TESTPMD_MAX_PARAM_LEN];
    };
} testpmd_param;

typedef enum testpmd_param_enum {
    TESTPMD_PARAM_MBUF_SIZE,
    TESTPMD_PARAM_MBUF_COUNT,
    TESTPMD_PARAM_MBCACHE,
    TESTPMD_PARAM_TXPKTS,
    TESTPMD_PARAM_BURST,
    TESTPMD_PARAM_TXQ,
    TESTPMD_PARAM_RXQ,
    TESTPMD_PARAM_TXD,
    TESTPMD_PARAM_RXD,
    TESTPMD_PARAM_MTU,
    TESTPMD_PARAM_LPBK_MODE,
    TESTPMD_PARAM_START_TX_FIRST,
    TESTPMD_PARAM_START,
    TESTPMD_PARAM_FLOW_CTRL_AUTONEG,
    TESTPMD_PARAM_FLOW_CTRL_RX,
    TESTPMD_PARAM_FLOW_CTRL_TX,
} testpmd_param_enum;

/*
 * The default values of testpmd parameters.
 * Note that the numbers of descriptors are set to 512 to be able
 * to calculate required total number of mbufs, though they might
 * be different.
 */
static const testpmd_param default_testpmd_params[] = {
    [TESTPMD_PARAM_MBUF_SIZE] = {
        .key = MAKE_TESTPMD_ARG("mbuf_size"),
        .type = TESTPMD_PARAM_TYPE_UINT64, .val = 2176},
    [TESTPMD_PARAM_MBUF_COUNT] = {
        .key = MAKE_TESTPMD_ARG("total_num_mbufs"),
        .type = TESTPMD_PARAM_TYPE_UINT64, .val = 0},
    [TESTPMD_PARAM_MBCACHE] = {
        .key = MAKE_TESTPMD_ARG("mbcache"),
        .type = TESTPMD_PARAM_TYPE_UINT64, .val = 250},
    [TESTPMD_PARAM_TXPKTS] = {
        .key = MAKE_TESTPMD_CMD("txpkts"),
        .type = TESTPMD_PARAM_TYPE_STRING, .str_val = "64"},
    [TESTPMD_PARAM_BURST] = { .key = MAKE_TESTPMD_ARG("burst"),
        .type = TESTPMD_PARAM_TYPE_UINT64, .val = 32},
    [TESTPMD_PARAM_TXQ] = { .key = MAKE_TESTPMD_ARG("txq"),
        .type = TESTPMD_PARAM_TYPE_UINT64, .val = 1},
    [TESTPMD_PARAM_RXQ] = { .key = MAKE_TESTPMD_ARG("rxq"),
        .type = TESTPMD_PARAM_TYPE_UINT64, .val = 1},
    [TESTPMD_PARAM_TXD] = { .key = MAKE_TESTPMD_ARG("txd"),
        .type = TESTPMD_PARAM_TYPE_UINT64, .val = 512},
    [TESTPMD_PARAM_RXD] = { .key = MAKE_TESTPMD_ARG("rxd"),
        .type = TESTPMD_PARAM_TYPE_UINT64, .val = 512},
    [TESTPMD_PARAM_MTU] = { .key = MAKE_TESTPMD_CMD("mtu"),
        .type = TESTPMD_PARAM_TYPE_UINT64, .val = 0},
    [TESTPMD_PARAM_LPBK_MODE] = {
        .key = MAKE_TESTPMD_CMD("loopback_mode"),
        .type = TESTPMD_PARAM_TYPE_UINT64, .val = 0},
    [TESTPMD_PARAM_START_TX_FIRST] = {
        .key = MAKE_TESTPMD_CMD("start_tx_first"),
        .type = TESTPMD_PARAM_TYPE_UINT64, .val = 0},
    [TESTPMD_PARAM_START] = {
        .key = MAKE_TESTPMD_CMD("start"),
        .type = TESTPMD_PARAM_TYPE_STRING, .str_val = "FALSE"},
    [TESTPMD_PARAM_FLOW_CTRL_AUTONEG] = {
        .key = MAKE_TESTPMD_CMD("flow_ctrl_autoneg"),
        .type = TESTPMD_PARAM_TYPE_STRING, .str_val = "on"},
    [TESTPMD_PARAM_FLOW_CTRL_RX] = {
        .key = MAKE_TESTPMD_CMD("flow_ctrl_rx"),
        .type = TESTPMD_PARAM_TYPE_STRING, .str_val = "on"},
    [TESTPMD_PARAM_FLOW_CTRL_TX] = {
        .key = MAKE_TESTPMD_CMD("flow_ctrl_tx"),
        .type = TESTPMD_PARAM_TYPE_STRING, .str_val = "on"},
};

static size_t
copy_default_testpmd_params(testpmd_param **param)
{
    testpmd_param *result;
    unsigned int i;

    result = tapi_malloc(sizeof(default_testpmd_params));
    for (i = 0; i < TE_ARRAY_LEN(default_testpmd_params); i++)
        result[i] = default_testpmd_params[i];

    *param = result;

    return TE_ARRAY_LEN(default_testpmd_params);
}

static te_errno
get_txpkts_size(const char *txpkts, uint64_t *txpkts_size)
{
    uint64_t result = 0;
    uint64_t value;
    char **str_array = NULL;
    te_errno rc = 0;
    int i;
    int param_len = test_split_param_list(txpkts, ',', &str_array);

    if (param_len == 0)
    {
        ERROR("Failed to parse txpkts parameter");
        rc = TE_RC(TE_TAPI, TE_EINVAL);
        goto out;
    }

    for (i = 0; i < param_len; i++)
    {
        if ((rc = te_strtoul(str_array[i], 0, &value)) != 0)
        {
            ERROR("Failed to get txpkts length");
            rc = TE_RC(TE_TAPI, TE_EINVAL);
            goto out;
        }
        result += value;
    }

    *txpkts_size = result;

out:
    if (str_array != NULL)
    {
        free(str_array[0]);
        free(str_array);
    }

    return rc;
}

static const char *
empty_string_if_null(const char *string)
{
    return string == NULL ? "" : string;
}

static char *
test_arg2testpmd_arg(const char *test_arg)
{
    te_string result = TE_STRING_INIT;
    const char testpmd_arg_prefix[] = "--";
    size_t i;

    CHECK_RC(te_string_append(&result, "%s%s", testpmd_arg_prefix,
                              test_arg +
                              (sizeof(TAPI_DPDK_TESTPMD_ARG_PREFIX) - 1)));

    for (i = 0; i < result.len; i++)
    {
        if (result.ptr[i] == '_')
            result.ptr[i] = '-';
    }

    return result.ptr;
}

void
tapi_dpdk_append_argument(const char *argument, int *argc_p, char ***argv_p)
{
    int argc = *argc_p;
    char **argv;

    argv = tapi_realloc(*argv_p, (argc + 1) * sizeof(*argv));
    if (argument == NULL)
    {
        argv[argc] = NULL;
    }
    else
    {
        if (te_asprintf(&(argv[argc]), "%s", argument) < 0)
            TEST_FAIL("Failed to append an argument");
    }

    *argc_p = argc + 1;
    *argv_p = argv;
}

static te_bool
is_prefixed(const char *str, const char *prefix)
{
    return (strncmp(str, prefix, strlen(prefix)) == 0);
}

static te_bool
is_testpmd_arg(const char *arg)
{
    return is_prefixed(arg, TAPI_DPDK_TESTPMD_ARG_PREFIX);
}

static te_bool
is_testpmd_command(const char *arg, testpmd_param_enum *param)
{
    testpmd_param_enum i;

    if (!is_prefixed(arg, TAPI_DPDK_TESTPMD_COMMAND_PREFIX))
        return FALSE;

    for (i = 0; i < TE_ARRAY_LEN(default_testpmd_params); i++)
    {
        if (strcmp(arg, default_testpmd_params[i].key) == 0)
        {
            *param = i;
            return TRUE;
        }
    }

    return FALSE;
}

static void
append_testpmd_arguments_from_test_args(te_kvpair_h *test_args, int *argc_out,
                                        char ***argv_out)
{
    const te_kvpair *pair;

    TAILQ_FOREACH(pair, test_args, links)
    {
        if (is_testpmd_arg(pair->key))
        {
            if (strcmp(pair->value, "FALSE") == 0)
                continue;

            /* Append key */
            tapi_dpdk_append_argument(NULL, argc_out, argv_out);
            (*argv_out)[*argc_out - 1] = test_arg2testpmd_arg(pair->key);

            /* Append value, if not boolean */
            if (strcmp(pair->value, "TRUE") != 0)
                tapi_dpdk_append_argument(pair->value, argc_out, argv_out);
        }
    }
}

static void
build_coremask_eal_arg(unsigned int n_cores, tapi_cpu_index_t *cpu_ids,
                       lcore_mask_t *lcore_mask)
{
    lcore_mask_t result = {{0}};
    unsigned int i;

    for (i = 0; i < n_cores; i++)
        CHECK_RC(tapi_rte_lcore_mask_set_bit(&result, cpu_ids[i].thread_id));

    *lcore_mask = result;
}

te_errno
tapi_dpdk_build_eal_arguments(rcf_rpc_server *rpcs,
                              tapi_env *env, unsigned int n_cpus,
                              tapi_cpu_index_t *cpu_ids,
                              const char *program_name,
                              int *argc_out, char ***argv_out)
{
    int extra_argc = 0;
    char **extra_argv = NULL;
    lcore_mask_t lcore_mask;
    te_errno rc;
    int i;

    build_coremask_eal_arg(n_cpus, cpu_ids, &lcore_mask);

    rc = tapi_rte_make_eal_args(env, rpcs, program_name, &lcore_mask,
                                extra_argc, (const char **)extra_argv,
                                argc_out, argv_out);
    if (rc != 0)
        ERROR("Failed to initialize EAL arguments for testpmd");

    for (i = 0; i < extra_argc; i++)
        free(extra_argv[i]);
    free(extra_argv);

    return rc;
}

static void
append_testpmd_command(unsigned int port_number, te_string *setup_cmd,
                       te_string *start_cmd, testpmd_param_enum param,
                       const char *cmd_val_fmt, ...)
{
    te_bool start = FALSE;
    te_bool add_val = TRUE;
    te_bool add_port = FALSE;

    switch (param)
    {
        case TESTPMD_PARAM_FLOW_CTRL_AUTONEG:
            CHECK_RC(te_string_append(setup_cmd, "set flow_ctrl autoneg "));
            add_port = TRUE;
            break;

        case TESTPMD_PARAM_FLOW_CTRL_RX:
            CHECK_RC(te_string_append(setup_cmd, "set flow_ctrl rx "));
            add_port = TRUE;
            break;

        case TESTPMD_PARAM_FLOW_CTRL_TX:
            CHECK_RC(te_string_append(setup_cmd, "set flow_ctrl tx "));
            add_port = TRUE;
            break;

        case TESTPMD_PARAM_LPBK_MODE:
            CHECK_RC(te_string_append(setup_cmd, "port config all loopback "));
            break;

        case TESTPMD_PARAM_MTU:
            CHECK_RC(te_string_append(setup_cmd, "port config mtu %u ",
                                      port_number));
            break;

        case TESTPMD_PARAM_START_TX_FIRST:
            CHECK_RC(te_string_append(start_cmd, "start tx_first "));
            start = TRUE;
            break;

        case TESTPMD_PARAM_START:
            CHECK_RC(te_string_append(start_cmd, "start"));
            start = TRUE;
            add_val = FALSE;
            break;

        case TESTPMD_PARAM_TXPKTS:
            CHECK_RC(te_string_append(start_cmd, "set txpkts "));
            start = TRUE;
            break;

        default:
            return;
    }

    if (add_val)
    {
        va_list  ap;

        va_start(ap, cmd_val_fmt);
        CHECK_RC(te_string_append_va(start ? start_cmd : setup_cmd,
                                     cmd_val_fmt, ap));
        va_end(ap);
    }

    if (add_port)
        CHECK_RC(te_string_append(start ? start_cmd : setup_cmd,
                                  " %u", port_number));

    CHECK_RC(te_string_append(start ? start_cmd : setup_cmd, "\n"));
}

/*
 * Adjust the testpmd parameters. It is performed in two steps:
 *
 * 1) Get parameters from the test, if a parameter is not set, get it from
 *    defaults (default_testpmd_params).
 *
 * 2) Modify parameters that are not set by the test, but must be modified
 *    in order to run testpmd packet forwarding, based on parameters that were
 *    obtained in the first step.
 */
static te_errno
adjust_testpmd_defaults(te_kvpair_h *test_args, unsigned int port_number,
                        unsigned int n_fwd_cpus,
                        te_string *cmdline_setup, te_string *cmdline_start,
                        int *argc_out, char ***argv_out)
{
    const te_kvpair *pair;
    testpmd_param *params;
    te_bool *param_is_set;
    uint64_t txpkts_size;
    unsigned int mbuf_size;
    unsigned int mtu;
    size_t params_len;
    te_errno rc;
    size_t i;

    params_len = copy_default_testpmd_params(&params);
    param_is_set = tapi_calloc(params_len, sizeof(*param_is_set));

    TAILQ_FOREACH(pair, test_args, links)
    {
        for (i = 0; i < params_len; i++)
        {
            if (strcmp(pair->key, params[i].key) == 0)
            {
                switch (params[i].type)
                {
                    case TESTPMD_PARAM_TYPE_STRING:
                        if (strlen(pair->value) + 1 > TESTPMD_MAX_PARAM_LEN)
                        {
                            free(params);
                            free(param_is_set);
                            return rc;
                        }
                        strcpy(params[i].str_val, pair->value);
                        param_is_set[i] = TRUE;
                        break;

                    case TESTPMD_PARAM_TYPE_UINT64:
                        rc = te_strtoul(pair->value, 0, &params[i].val);
                        if (rc != 0)
                        {
                            free(params);
                            free(param_is_set);
                            return rc;
                        }
                        param_is_set[i] = TRUE;
                        break;

                    default:
                        ERROR("Unknown testpmd parameter type %d",
                              params[i].type);
                        return TE_RC(TE_TAPI, TE_EINVAL);
                }
                break;
            }
        }
    }

    if ((rc = get_txpkts_size(params[TESTPMD_PARAM_TXPKTS].str_val,
                              &txpkts_size)) != 0)
    {
        free(params);
        free(param_is_set);
        return rc;
    }

    if (!param_is_set[TESTPMD_PARAM_MBUF_COUNT])
    {
        uint64_t needed_mbuf_count = (params[TESTPMD_PARAM_TXQ].val *
                                      (params[TESTPMD_PARAM_TXD].val +
                                       params[TESTPMD_PARAM_BURST].val)) +
                                     (params[TESTPMD_PARAM_RXQ].val *
                                      params[TESTPMD_PARAM_RXD].val) +
                                     params[TESTPMD_PARAM_MBCACHE].val *
                                     n_fwd_cpus;

        char *value;

        if (needed_mbuf_count < TESTPMD_TOTAL_MBUFS_MIN)
            needed_mbuf_count = TESTPMD_TOTAL_MBUFS_MIN;

        if (te_asprintf(&value, "%lu", needed_mbuf_count) < 0)
            TEST_FAIL("Failed to build total-num-mbufs testpmd parameter");

        tapi_dpdk_append_argument("--total-num-mbufs", argc_out, argv_out);
        tapi_dpdk_append_argument(value, argc_out, argv_out);
        free(value);
    }
    if (!param_is_set[TESTPMD_PARAM_MBUF_SIZE] &&
        tapi_dpdk_mbuf_size_by_pkt_size(txpkts_size, &mbuf_size))
    {
        char *value;

        if (te_asprintf(&value, "%u", mbuf_size) < 0)
            TEST_FAIL("Failed to build mbuf-size testpmd parameter");

        tapi_dpdk_append_argument("--mbuf-size", argc_out, argv_out);
        tapi_dpdk_append_argument(value, argc_out, argv_out);
        free(value);
    }
    if (!param_is_set[TESTPMD_PARAM_MTU] &&
        tapi_dpdk_mtu_by_pkt_size(txpkts_size, &mtu))
    {
        append_testpmd_command(port_number, cmdline_setup, cmdline_start,
                               TESTPMD_PARAM_MTU, "%u", mtu);
    }

    free(params);
    free(param_is_set);

    return 0;
}

static void
generate_cmdline_filename(const char *dir, char **cmdline_filename)
{
    char *path;

    if (te_asprintf(&path, "%s/%s_%s", dir, tapi_file_generate_name(),
                    COMMANDS_FILE_NAME) < 0)
    {
        TEST_FAIL("Failed to create testpmd commands file name");
    }

    *cmdline_filename = path;
}

static void
append_testpmd_cmdline_from_args(te_kvpair_h *test_args,
                                 unsigned int port_number,
                                 te_string *cmdline_setup,
                                 te_string *cmdline_start)
{
    const te_kvpair *pair;
    testpmd_param_enum param;

    TAILQ_FOREACH(pair, test_args, links)
    {
        if (is_testpmd_command(pair->key, &param) &&
            strcmp(pair->value, "FALSE") != 0)
        {
            append_testpmd_command(port_number, cmdline_setup, cmdline_start,
                                   param, "%s", pair->value);
        }
    }
}

static te_errno
tapi_dpdk_attach_filters(tapi_job_simple_desc_t *desc)
{
    te_errno rc;
    tapi_job_simple_filter_t *filter;

    for (filter = desc->filters;
         filter->use_stdout || filter->use_stderr;
         filter++)
    {
        if ((rc = tapi_job_attach_simple_filter(desc, filter)) != 0)
        {
            tapi_job_destroy(*desc->job_loc, -1);
            *desc->job_loc = NULL;
            return rc;
        }
    }

    return 0;
}

static te_errno
tapi_dpdk_add_dbells_params(te_kvpair_h *test_params, const char *q_num,
                            const char *pref)
{
    te_errno rc;
    unsigned int q, qnum;
    te_string dbells = TE_STRING_INIT;
    const char *display_xstats_arg;
    const char *display_xstats;

    rc = te_strtoui(q_num, 0, &qnum);
    if (rc != 0)
        return rc;

    rc = te_string_append(&dbells, "%s_dbells,", pref);
    if (rc != 0)
        goto out;

    for (q = 0; q < qnum; q++)
    {
        rc = te_string_append(&dbells, "%s_q%u_dbells,", pref, q);
        if (rc != 0)
            goto out;
    }

    display_xstats_arg = TAPI_DPDK_TESTPMD_ARG_PREFIX "display-xstats";
    display_xstats = te_kvpairs_get(test_params, display_xstats_arg);

    rc = te_string_append(&dbells, "%s", empty_string_if_null(display_xstats));
    if (rc != 0)
        goto out;

    if (display_xstats != NULL)
    {
        rc = te_kvpairs_del(test_params, display_xstats_arg);
        if (rc != 0)
        {
            ERROR("Failed to remove key for display xstats argument "
                  "that was reported as present in a list.");
            goto out;
        }
    }

    rc = te_kvpair_add(test_params, display_xstats_arg, dbells.ptr);

out:
    te_string_free(&dbells);
    return rc;
}

static te_errno
tapi_dpdk_dbells_available(tapi_job_channel_t *dbells_skip_filter, te_bool *res)
{
    tapi_job_buffer_t buf = TAPI_JOB_BUFFER_INIT;
    te_errno rc;

    rc = tapi_job_receive(TAPI_JOB_CHANNEL_SET(dbells_skip_filter), 1000, &buf);
    if (rc != 0)
    {
        if (rc != TE_ETIMEDOUT)
            goto out;

        *res = true;
        rc = 0;
        goto out;
    }

    if (buf.eos)
    {
        rc = TE_EINVAL;
        goto out;
    }

    if (buf.filter == dbells_skip_filter)
    {
        *res = false;
        rc = 0;
    }
    else
    {
        ERROR("Received buf from a job contains invalid filter pointer");
        rc = TE_EINVAL;
    }

out:
    te_string_free(&buf.data);
    return rc;
}

static te_errno
tapi_dpdk_stats_log_dbells(tapi_job_channel_t *dbells_filter,
                           tapi_job_channel_t *dbells_skip_filter,
                           const te_meas_stats_t *meas_stats_pps,
                           const char *prefix)
{
    tapi_job_buffer_t buf = TAPI_JOB_BUFFER_INIT;
    te_meas_stats_t meas_stats_dbells = {0};
    te_mi_logger *logger = NULL;
    unsigned int num_datapoints;
    te_bool dbells_available;
    unsigned long dbells_ps;
    unsigned long pps_mean;
    unsigned int i;
    te_errno rc;

    rc = tapi_dpdk_dbells_available(dbells_skip_filter, &dbells_available);
    if (rc != 0)
        goto out;

    if (!dbells_available)
    {
        WARN("%s doorbells statistics is unavailable", prefix);
        rc = 0;
        goto out;
    }

    rc = te_mi_logger_meas_create(TAPI_DPDK_TESTPMD_NAME, &logger);
    if (rc != 0)
    {
        ERROR("Failed to create logger");
        goto out;
    }

    num_datapoints = meas_stats_pps->stab_required ?
                     meas_stats_pps->stab.correct_data.num_datapoints +
                                                meas_stats_pps->num_zeros :
                     meas_stats_pps->data.num_datapoints;

    rc = te_meas_stats_init(&meas_stats_dbells, num_datapoints,
                            0, 0, 0, 0, 0);
    if (rc != 0)
        goto out;

    for (i = 0; i < num_datapoints; i++)
    {
        rc = tapi_job_receive(TAPI_JOB_CHANNEL_SET(dbells_filter),
                              TAPI_DPDK_TESTPMD_RECEIVE_TIMEOUT_MS, &buf);
        if (rc != 0)
            goto out;

        if (buf.eos)
            break;

        if (buf.filter == dbells_filter)
        {
            if (meas_stats_pps->stab_required &&
                i < meas_stats_pps->num_zeros)
            {
                te_string_reset(&buf.data);
                continue;
            }

            if ((rc = te_strtoul(buf.data.ptr, 0, &dbells_ps)) != 0)
                goto out;

            if (te_meas_stats_update(&meas_stats_dbells, (double)dbells_ps) ==
                TE_MEAS_STATS_UPDATE_NOMEM)
            {
                rc = TE_ENOMEM;
                goto out;
            }

            if (buf.dropped > 0)
                WARN("Dropped messages count: %lu", buf.dropped);

            te_string_reset(&buf.data);
        }
        else
        {
            ERROR("Received buf from a job contains invalid filter pointer");
            rc = TE_EINVAL;
            goto out;
        }
    }

    te_mi_logger_add_meas(logger, NULL, TE_MI_MEAS_FREQ, NULL,
                          TE_MI_MEAS_AGGR_MEAN, meas_stats_dbells.data.mean,
                          TE_MI_MEAS_MULTIPLIER_PLAIN);

    pps_mean = meas_stats_pps->stab_required ?
               meas_stats_pps->stab.correct_data.mean :
               meas_stats_pps->data.mean;

    if (meas_stats_dbells.data.mean != 0)
        te_mi_logger_add_meas(logger, NULL, TE_MI_MEAS_EPE, NULL,
                              TE_MI_MEAS_AGGR_MEAN,
                              (double)pps_mean / meas_stats_dbells.data.mean,
                              TE_MI_MEAS_MULTIPLIER_PLAIN);

out:
    te_meas_stats_free(&meas_stats_dbells);
    te_mi_logger_destroy(logger);
    te_string_free(&buf.data);
    return rc;
}

te_errno
tapi_dpdk_grab_cpus(const char *ta,
                    unsigned int n_cpus_preferred,
                    unsigned int n_cpus_required,
                    int numa_node,
                    const tapi_cpu_prop_t *prop,
                    unsigned int *n_cpus_grabbed,
                    tapi_cpu_index_t *cpu_ids)
{
    tapi_cpu_index_t topology = {
        .node_id = numa_node < 0 ? TAPI_CPU_ID_UNSPEC : numa_node,
        .package_id = TAPI_CPU_ID_UNSPEC,
        .core_id = TAPI_CPU_ID_UNSPEC,
        .thread_id = TAPI_CPU_ID_UNSPEC,
    };
    size_t n_threads;
    unsigned int to_grab;
    te_errno rc;

    rc = tapi_cfg_get_all_threads(ta, &n_threads, NULL);
    if (rc != 0)
        return rc;

    to_grab = MIN(n_cpus_preferred, n_threads);

    rc = tapi_cfg_cpu_grab_multiple_with_id(ta, prop, &topology,
                                            to_grab, cpu_ids);
    if (rc == 0)
    {
        *n_cpus_grabbed = to_grab;
        return 0;
    }

    if (to_grab == n_cpus_required)
        return rc;

    rc = tapi_cfg_cpu_grab_multiple_with_id(ta, prop, &topology,
                                            n_cpus_required, cpu_ids);
    if (rc == 0)
    {
        *n_cpus_grabbed = n_cpus_required;
        return 0;
    }

    return rc;
}

te_errno
tapi_dpdk_grab_cpus_nonstrict_prop(const char *ta,
                                   unsigned int n_cpus_preferred,
                                   unsigned int n_cpus_required,
                                   int numa_node,
                                   const tapi_cpu_prop_t *prop,
                                   unsigned int *n_cpus_grabbed,
                                   tapi_cpu_index_t *cpu_ids)
{
    te_errno rc;

    if (numa_node >= 0)
    {
        /*
         * When grabbing CPUs with required property, set also a strict
         * constraint on CPUs quantity (n_cpus_required is set to
         * n_cpus_preferred)
         */
        if (tapi_dpdk_grab_cpus(ta, n_cpus_preferred, n_cpus_preferred,
                                numa_node, prop, n_cpus_grabbed, cpu_ids) == 0)
        {
            return 0;
        }

        WARN("Fallback to grab any available CPUs on a single NUMA node");
    }

    rc = tapi_cfg_cpu_grab_multiple_on_single_node(ta, prop,
                                                   n_cpus_preferred,
                                                   cpu_ids);
    if (rc == 0)
    {
        *n_cpus_grabbed = n_cpus_preferred;
        return 0;
    }

    WARN("Fallback to grab any available CPUs on any NUMA node");

    return tapi_dpdk_grab_cpus(ta, n_cpus_preferred, n_cpus_required, -1,
                               NULL, n_cpus_grabbed, cpu_ids);
}

const char *
tapi_dpdk_get_vdev_eal_argument(int eal_argc, char **eal_argv)
{
    int i;

    for (i = 0; i < eal_argc; i++)
    {
        if (strcmp(eal_argv[i], "--vdev") == 0 && i + 1 < eal_argc)
        {
            return eal_argv[i + 1];
        }
    }

    return NULL;
}

te_errno
tapi_dpdk_get_vdev_port_number(const char *vdev, unsigned int *port_number)
{
    unsigned int dev_count = 0;
    const char *tmp = vdev;

    if (vdev == NULL)
    {
        ERROR("Failed to parse NULL vdev argument");
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    while ((tmp = strstr(tmp, "dev(")) != NULL)
    {
       dev_count++;
       tmp++;
    }

    /*
     * Hack: assume that port number of vdev is right after all slave
     * devices. It might fail if --vdev and --allow have
     * different devices specified.
     */
    *port_number = dev_count;

    return 0;
}

void
append_testpmd_nb_cores_arg(unsigned int n_fwd_cpus,
                            int *argc_out, char ***argv_out)
{
    te_string nb_cores = TE_STRING_INIT;

    tapi_dpdk_append_argument("--nb-cores", argc_out, argv_out);

    CHECK_RC(te_string_append(&nb_cores, "%lu", n_fwd_cpus));

    tapi_dpdk_append_argument(nb_cores.ptr, argc_out, argv_out);

    te_string_free(&nb_cores);
}

typedef struct tapi_dpdk_testpmd_prep_eal {
    te_string testpmd_path;
    int testpmd_argc;
    char **testpmd_argv;
    unsigned int port_number;
    unsigned int nb_cores;
    tapi_cpu_index_t *grabbed_cpu_ids;
    unsigned int nb_cpus_grabbed;
} tapi_dpdk_testpmd_prep_eal;

static void
tapi_dpdk_testpmd_prepare_eal_init(tapi_dpdk_testpmd_prep_eal *prep_eal)
{
    memset(prep_eal, 0, sizeof(*prep_eal));
    prep_eal->testpmd_path = (te_string)TE_STRING_INIT;
    prep_eal->testpmd_argv = NULL;
    prep_eal->grabbed_cpu_ids = NULL;
}

static void
tapi_dpdk_testpmd_prepare_eal_cleanup(tapi_dpdk_testpmd_prep_eal *prep_eal,
                                      te_bool release_cpus_rsrc, const char *ta)
{
    int i;

    if (release_cpus_rsrc && ta != NULL)
    {
        for (i = 0; i < prep_eal->nb_cpus_grabbed; i++)
        {
            (void)tapi_cfg_cpu_release_by_id(ta,
                                             &prep_eal->grabbed_cpu_ids[i]);
        }
    }
    free(prep_eal->grabbed_cpu_ids);
    prep_eal->nb_cpus_grabbed = 0;
    prep_eal->grabbed_cpu_ids = NULL;

    prep_eal->nb_cores = 0;
    prep_eal->port_number = 0;
    for (i = 0; i < prep_eal->testpmd_argc; i++)
        free(prep_eal->testpmd_argv[i]);
    free(prep_eal->testpmd_argv);
    prep_eal->testpmd_argc = 0;
    prep_eal->testpmd_argv = NULL;
    te_string_free(&prep_eal->testpmd_path);
}

static te_errno
tapi_dpdk_prepare_and_build_eal_args(rcf_rpc_server *rpcs, tapi_env *env,
                                     unsigned int n_fwd_cpus,
                                     const tapi_cpu_prop_t *prop,
                                     te_kvpair_h *test_args,
                                     tapi_dpdk_testpmd_job_t *testpmd_job,
                                     tapi_dpdk_testpmd_prep_eal *prep_eal)
{
    char *working_dir = NULL;
    const char *vdev_arg = NULL;
    /* The first CPU is reserved by testpmd for command-line processing */
    unsigned int n_cpus = n_fwd_cpus + 1;
    unsigned int service_cores_count;
    int numa_node;
    te_errno rc = 0;

    tapi_dpdk_testpmd_prepare_eal_init(prep_eal);

    if (n_fwd_cpus == 0)
    {
        ERROR("Testpmd cannot be run with 0 forwarding cores");
        rc = TE_RC(TE_TAPI, TE_EINVAL);
        goto wrong_fwd_cpus;
    }

    rc = tapi_rte_get_numa_node(env, rpcs, &numa_node);
    if (rc != 0)
        goto fail_get_numa_node;

    rc = tapi_eal_get_nb_required_service_cores_rpcs(env, rpcs,
                                                     &service_cores_count);
    if (rc != 0)
        goto fail_get_service_cores;

    rc = cfg_get_instance_fmt(NULL, &working_dir, "/agent:%s/dir:", rpcs->ta);
    if (rc != 0)
    {
        ERROR("Failed to get working directory");
        goto fail_get_working_dir;
    }

    /*
     * Adjust the preferred count of CPUs to grab, include required service
     * cores count.
     */
    n_cpus += service_cores_count;

    prep_eal->grabbed_cpu_ids = tapi_calloc(n_cpus,
                                          sizeof(*(prep_eal->grabbed_cpu_ids)));
    if ((rc = tapi_dpdk_grab_cpus_nonstrict_prop(rpcs->ta, n_cpus,
                                             TESTPMD_MIN_N_CORES +
                                             service_cores_count,
                                             numa_node,
                                             prop,
                                             &prep_eal->nb_cpus_grabbed,
                                             prep_eal->grabbed_cpu_ids)) != 0)
    {
        goto fail_grab_cpus;
    }

    prep_eal->nb_cores = prep_eal->nb_cpus_grabbed - 1 - service_cores_count;

    rc = te_string_append(&prep_eal->testpmd_path, "%s/dpdk-testpmd",
                          working_dir);
    if (rc != 0)
        goto fail_get_testpmd_path;

    rc = tapi_dpdk_build_eal_arguments(rpcs, env, prep_eal->nb_cpus_grabbed,
                                       prep_eal->grabbed_cpu_ids,
                                       prep_eal->testpmd_path.ptr,
                                       &prep_eal->testpmd_argc,
                                       &prep_eal->testpmd_argv);
    if (rc != 0)
        goto fail_build_eal_args;

    vdev_arg = tapi_dpdk_get_vdev_eal_argument(prep_eal->testpmd_argc,
                                               prep_eal->testpmd_argv);
    if (vdev_arg != NULL)
    {
        if ((rc = tapi_dpdk_get_vdev_port_number(vdev_arg,
                                                 &prep_eal->port_number)) != 0)
            goto fail_get_port_number;
    }

    return 0;

fail_get_port_number:
fail_build_eal_args:
fail_get_testpmd_path:
fail_grab_cpus:
    tapi_dpdk_testpmd_prepare_eal_cleanup(prep_eal, TRUE, rpcs->ta);
    free(working_dir);
fail_get_working_dir:
fail_get_service_cores:
fail_get_numa_node:
wrong_fwd_cpus:
    return rc;
}

te_errno
tapi_dpdk_create_testpmd_job(rcf_rpc_server *rpcs, tapi_env *env,
                             unsigned int n_fwd_cpus,
                             const tapi_cpu_prop_t *prop,
                             te_kvpair_h *test_args,
                             tapi_dpdk_testpmd_job_t *testpmd_job)
{
    int testpmd_argc = 0;
    char **testpmd_argv = NULL;
    unsigned int port_number;
    tapi_dpdk_testpmd_prep_eal prep_eal;

    char *cmdline_file = NULL;
    te_string cmdline_setup = TE_STRING_INIT;
    te_string cmdline_start = TE_STRING_INIT;
    char *tmp_dir = NULL;
    te_errno rc = 0;
    tapi_job_factory_t *factory = NULL;
    int i;

    rc = tapi_dpdk_prepare_and_build_eal_args(rpcs, env, n_fwd_cpus, prop,
                                              test_args, testpmd_job,
                                              &prep_eal);
    if (rc != 0)
        goto out;

    testpmd_argc = prep_eal.testpmd_argc;
    testpmd_argv = prep_eal.testpmd_argv;
    prep_eal.testpmd_argc = 0;
    prep_eal.testpmd_argv = NULL;
    port_number = prep_eal.port_number;
    n_fwd_cpus = prep_eal.nb_cores;

    /* Separate EAL arguments from testpmd arguments */
    tapi_dpdk_append_argument("--", &testpmd_argc, &testpmd_argv);
    rc = adjust_testpmd_defaults(test_args, port_number, n_fwd_cpus,
                                 &cmdline_setup, &cmdline_start,
                                 &testpmd_argc, &testpmd_argv);
    if (rc != 0)
        goto out;

    rc = cfg_get_instance_fmt(NULL, &tmp_dir,
                              "/agent:%s/tmp_dir:", rpcs->ta);
    if (rc != 0)
    {
        ERROR("Failed to get temporary directory");
        goto out;
    }

    generate_cmdline_filename(tmp_dir, &cmdline_file);
    append_testpmd_cmdline_from_args(test_args, port_number, &cmdline_setup,
                                     &cmdline_start);
    /*
     * Disable device start to execute setup commands first and then start the
     * device.
     */
    tapi_dpdk_append_argument("--disable-device-start",
                              &testpmd_argc, &testpmd_argv);

    append_testpmd_nb_cores_arg(n_fwd_cpus, &testpmd_argc, &testpmd_argv);
    tapi_dpdk_append_argument("--cmdline-file", &testpmd_argc, &testpmd_argv);
    tapi_dpdk_append_argument(cmdline_file, &testpmd_argc, &testpmd_argv);

    append_testpmd_arguments_from_test_args(test_args, &testpmd_argc,
                                            &testpmd_argv);

    /* Terminate argv with NULL */
    tapi_dpdk_append_argument(NULL, &testpmd_argc, &testpmd_argv);

    rc = tapi_job_factory_rpc_create(rpcs, &factory);
    if (rc != 0)
    {
        ERROR("Failed to create factory for testpmd job");
        goto out;
    }

    rc = tapi_job_simple_create(factory,
                          &(tapi_job_simple_desc_t){
                                .program = prep_eal.testpmd_path.ptr,
                                .argv = (const char **)testpmd_argv,
                                .job_loc = &testpmd_job->job,
                                .stdin_loc = &testpmd_job->in_channel,
                                .stdout_loc = &testpmd_job->out_channels[0],
                                .stderr_loc = &testpmd_job->out_channels[1],
                                .filters = TAPI_JOB_SIMPLE_FILTERS(
                                    {.use_stdout = TRUE,
                                     .readable = TRUE,
                                     .re = "(?m)Tx-pps:\\s*([0-9]+)",
                                     .extract = 1,
                                     .filter_var = &testpmd_job->tx_pps_filter,
                                    },
                                    {.use_stdout = TRUE,
                                     .readable = TRUE,
                                     .re = "(?m)Rx-pps:\\s*([0-9]+)",
                                     .extract = 1,
                                     .filter_var = &testpmd_job->rx_pps_filter,
                                    },
                                    {.use_stdout = TRUE,
                                     .readable = TRUE,
                                     .re = "(?m)^Link speed: ([0-9]+ [MG])bps$",
                                     .extract = 1,
                                     .filter_var = &testpmd_job->link_speed_filter,
                                    },
                                    {.use_stderr = TRUE,
                                     .log_level = TE_LL_ERROR,
                                     .readable = TRUE,
                                     .filter_var = &testpmd_job->err_filter,
                                     .filter_name = "err",
                                    },
                                    {.use_stdout = TRUE,
                                     .log_level = TE_LL_RING,
                                     .readable = FALSE,
                                     .filter_name = "out",
                                    }
                                 )
                          });
    if (rc != 0)
        goto out;

    testpmd_job->cmdline_file = cmdline_file;
    testpmd_job->cmdline_setup = cmdline_setup;
    testpmd_job->cmdline_start = cmdline_start;
    testpmd_job->ta = tapi_strdup(rpcs->ta);
    testpmd_job->port_number = port_number;

out:
    if (rc != 0)
    {
        free(cmdline_file);
        te_string_free(&cmdline_setup);
        te_string_free(&cmdline_start);
    }

    for (i = 0; i < testpmd_argc; i++)
        free(testpmd_argv[i]);
    free(testpmd_argv);

    tapi_dpdk_testpmd_prepare_eal_cleanup(&prep_eal, FALSE, NULL);
    tapi_job_factory_destroy(factory);

    return rc;
}

te_errno
tapi_dpdk_testpmd_is_opt_supported(rcf_rpc_server *rpcs, tapi_env *env,
                                   te_kvpair_h *opt, te_bool *opt_supported)
{
    int testpmd_argc = 0;
    char **testpmd_argv = NULL;
    tapi_dpdk_testpmd_prep_eal prep_eal;

    tapi_job_factory_t *factory = NULL;
    tapi_dpdk_testpmd_job_t testpmd_job_s = {0};
    tapi_dpdk_testpmd_job_t *testpmd_job = &testpmd_job_s;
    te_string stop_testpmd_cmd = TE_STRING_INIT;
    tapi_job_status_t status;
    int current_wait_time_ms;
    int max_wait_timeout_ms;
    int wait_timeout_ms;
    te_errno rc;
    int i;

    rc = tapi_dpdk_prepare_and_build_eal_args(rpcs, env, TESTPMD_MIN_N_CORES,
                                              NULL, opt, testpmd_job,
                                              &prep_eal);
    if (rc != 0)
        goto out;

    testpmd_argc = prep_eal.testpmd_argc;
    testpmd_argv = prep_eal.testpmd_argv;
    prep_eal.testpmd_argc = 0;
    prep_eal.testpmd_argv = NULL;

    /* Separate EAL arguments from testpmd arguments */
    tapi_dpdk_append_argument("--", &testpmd_argc, &testpmd_argv);

    append_testpmd_arguments_from_test_args(opt, &testpmd_argc,
                                            &testpmd_argv);
    tapi_dpdk_append_argument(NULL, &testpmd_argc, &testpmd_argv);

    rc = tapi_job_factory_rpc_create(rpcs, &factory);
    if (rc != 0)
    {
        ERROR("Failed to create factory for testpmd job");
        goto out;
    }

    rc = tapi_job_simple_create(factory,
                          &(tapi_job_simple_desc_t){
                                .program = prep_eal.testpmd_path.ptr,
                                .argv = (const char **)testpmd_argv,
                                .job_loc = &testpmd_job->job,
                                .stdin_loc = &testpmd_job->in_channel,
                                .stdout_loc = &testpmd_job->out_channels[0],
                                .stderr_loc = &testpmd_job->out_channels[1],
                          });
    if (rc != 0)
        goto out;

    testpmd_job->ta = tapi_strdup(rpcs->ta);
    testpmd_job->port_number = prep_eal.port_number;

    *opt_supported = tapi_job_start(testpmd_job->job) == 0 ? TRUE : FALSE;
    if (*opt_supported)
    {
        *opt_supported = FALSE;
        rc = te_string_append(&stop_testpmd_cmd, "\r");
        if (rc != 0)
        {
            ERROR("Failed to create stop testpmd command string");
            goto out;
        }

        wait_timeout_ms = 100;
        max_wait_timeout_ms = 60000;
        current_wait_time_ms = 0;
        while (((rc = tapi_job_wait(testpmd_job->job, wait_timeout_ms,
                                    NULL)) != 0) && (rc != TE_ECHILD))
        {
            current_wait_time_ms += wait_timeout_ms;
            if (current_wait_time_ms > max_wait_timeout_ms)
            {
                ERROR("Job didn't terminate for too long, but it had to "
                      "either by stop command or unsupported option");
                rc = TE_ETIMEDOUT;
                goto out;
            }
            tapi_job_send(testpmd_job->in_channel, &stop_testpmd_cmd);
        }

        rc = tapi_job_wait(testpmd_job->job, 0, &status);
        if (rc != 0)
        {
            ERROR("Failed to get a status of the supposedly terminated job");
            goto out;
        }

        if ((status.type == TAPI_JOB_STATUS_EXITED) && (status.value == 0))
            *opt_supported = TRUE;
    }

    tapi_dpdk_testpmd_destroy(testpmd_job);

out:
    for (i = 0; i < testpmd_argc; i++)
        free(testpmd_argv[i]);
    free(testpmd_argv);

    tapi_dpdk_testpmd_prepare_eal_cleanup(&prep_eal, TRUE, rpcs->ta);
    te_string_free(&stop_testpmd_cmd);

    tapi_job_factory_destroy(factory);

    return rc;
}

te_errno
tapi_dpdk_testpmd_start(tapi_dpdk_testpmd_job_t *testpmd_job)
{
    if (testpmd_job->cmdline_file != NULL)
    {
        if (tapi_file_create_ta(testpmd_job->ta, testpmd_job->cmdline_file,
                "%s%s%s",
                empty_string_if_null(testpmd_job->cmdline_setup.ptr),
                TESTPMD_CMD_POST_SETUP,
                empty_string_if_null(testpmd_job->cmdline_start.ptr)) != 0)
        {
            ERROR("Failed to create comand file on TA for testpmd");
            return TE_RC(TE_TAPI, TE_EFAIL);
        }

        RING("testpmd cmdline-file content:\n%s%s%s",
             empty_string_if_null(testpmd_job->cmdline_setup.ptr),
             TESTPMD_CMD_POST_SETUP,
             empty_string_if_null(testpmd_job->cmdline_start.ptr));
    }

    return tapi_job_start(testpmd_job->job);
}

te_errno
tapi_dpdk_testpmd_destroy(tapi_dpdk_testpmd_job_t *testpmd_job)
{
    te_errno rc;

    if (testpmd_job == NULL)
        return 0;

    rc = tapi_job_destroy(testpmd_job->job, TAPI_DPDK_TESTPMD_TERM_TIMEOUT_MS);
    if (rc != 0)
        return rc;

    free(testpmd_job->ta);
    free(testpmd_job->cmdline_file);
    te_string_free(&testpmd_job->cmdline_setup);
    te_string_free(&testpmd_job->cmdline_start);

    return 0;
}

te_errno
tapi_dpdk_testpmd_get_link_speed(tapi_dpdk_testpmd_job_t *testpmd_job,
                                 unsigned int *link_speed)
{
    tapi_job_buffer_t buf = TAPI_JOB_BUFFER_INIT;
    unsigned int multiplier;
    te_errno rc = 0;

    rc = tapi_job_receive(TAPI_JOB_CHANNEL_SET(testpmd_job->link_speed_filter),
                          TAPI_DPDK_TESTPMD_RECEIVE_TIMEOUT_MS, &buf);

    if (rc != 0)
    {
        ERROR("Failed to get link speed from testpmd job");
        return rc;
    }

    if (buf.eos)
    {
        ERROR("End of stream before link speed message");
        te_string_free(&buf.data);
        return TE_RC(TE_TAPI, TE_EFAIL);
    }

    if (buf.dropped > 0)
        WARN("Dropped messages count: %lu", buf.dropped);

    switch (buf.data.ptr[buf.data.len - 1])
    {
        case 'G':
            multiplier = 1000;
            break;

        case 'M':
            multiplier = 1;
            break;

        default:
            ERROR("Invalid bps prefix in the link speed");
            te_string_free(&buf.data);
            return TE_RC(TE_TAPI, TE_EINVAL);
    }

    /* Remove bps prefix and a space after the link speed value */
    te_string_cut(&buf.data, 2);

    if ((rc = te_strtoui(buf.data.ptr, 0, link_speed)) != 0)
    {
        ERROR("Failed to parse link speed");
        te_string_free(&buf.data);
        return TE_RC(TE_TAPI, TE_EFAIL);
    }

    *link_speed *= multiplier;

    return 0;
}

te_errno
tapi_dpdk_testpmd_get_stats(tapi_dpdk_testpmd_job_t *testpmd_job,
                            te_meas_stats_t *tx,
                            te_meas_stats_t *rx)
{
    tapi_job_buffer_t buf = TAPI_JOB_BUFFER_INIT;
    unsigned long tx_pps = 0;
    unsigned long rx_pps = 0;
    te_errno rc = 0;

    do {
        rc = tapi_job_receive(TAPI_JOB_CHANNEL_SET(testpmd_job->tx_pps_filter,
                                                   testpmd_job->rx_pps_filter,
                                                   testpmd_job->err_filter),
                              TAPI_DPDK_TESTPMD_RECEIVE_TIMEOUT_MS, &buf);
        if (rc != 0)
            goto out;

        if (buf.eos)
            break;

        if (buf.filter == testpmd_job->tx_pps_filter)
        {
            if ((rc = te_strtoul(buf.data.ptr, 0, &tx_pps)) != 0)
                goto out;

            if (tx != NULL &&
                te_meas_stats_update(tx, (double)tx_pps) ==
                TE_MEAS_STATS_UPDATE_NOMEM)
            {
                rc = TE_ENOMEM;
                goto out;
            }
        }
        else if (buf.filter == testpmd_job->rx_pps_filter)
        {
            if ((rc = te_strtoul(buf.data.ptr, 0, &rx_pps)) != 0)
                goto out;

            if (rx != NULL &&
                te_meas_stats_update(rx, (double)rx_pps) ==
                TE_MEAS_STATS_UPDATE_NOMEM)
            {
                rc = TE_ENOMEM;
                goto out;
            }
        }
        else if (buf.filter == testpmd_job->err_filter)
        {
            WARN("Error message: %s", buf.data.ptr);
        }
        else
        {
            ERROR("Received buf from a job contains invalid filter pointer");
        }

        if (buf.dropped > 0)
            WARN("Dropped messages count: %lu", buf.dropped);

        te_string_reset(&buf.data);
    } while (TE_MEAS_STATS_CONTINUE(tx) ||
             TE_MEAS_STATS_CONTINUE(rx));

    if (TE_MEAS_STATS_CONTINUE(tx) ||
        TE_MEAS_STATS_CONTINUE(rx))
    {
        ERROR("Channel had been closed before required number of stats were received");
        rc = TE_RC(TE_TAPI, TE_EFAIL);
        goto out;
    }

out:
    te_string_free(&buf.data);

    return rc;
}

te_bool
tapi_dpdk_mtu_by_pkt_size(unsigned int packet_size, unsigned int *mtu)
{
    unsigned int sufficient_mtu = packet_size -
                                  MIN(ETHER_HDR_LEN, packet_size);

    *mtu = sufficient_mtu;
    return (sufficient_mtu > ETHER_DATA_LEN) ? TRUE : FALSE;
}

te_bool
tapi_dpdk_mbuf_size_by_pkt_size(unsigned int packet_size,
                                unsigned int *mbuf_size)
{
    unsigned int minimal_mbuf_size = packet_size + MBUF_OVERHEAD;

    if (minimal_mbuf_size <=
        default_testpmd_params[TESTPMD_PARAM_MBUF_SIZE].val)
    {
        return FALSE;
    }

    *mbuf_size = minimal_mbuf_size;

    return TRUE;
}

te_errno
tapi_dpdk_attach_dbells_filter_rx(tapi_dpdk_testpmd_job_t *testpmd_job)
{
    tapi_job_simple_desc_t desc = {
        .job_loc = &testpmd_job->job,
        .stdout_loc = &testpmd_job->out_channels[0],
        .stderr_loc = &testpmd_job->out_channels[1],
        .filters = TAPI_JOB_SIMPLE_FILTERS(
            {.use_stdout = TRUE,
             .readable = TRUE,
             .re = "(?m)rx_dbells\\s*([0-9]+)\\s*([0-9]+)",
             .extract = 2,
             .filter_var = &testpmd_job->rx_dbells_filter,
            },
            {.use_stderr = TRUE,
             .readable = TRUE,
             .re = "(?m)No\\sxstat\\s'rx_dbells'",
             .extract = 0,
             .filter_var = &testpmd_job->rx_dbells_skip_filter,
            }
        )
    };

    return tapi_dpdk_attach_filters(&desc);
}

te_errno
tapi_dpdk_attach_dbells_filter_tx(tapi_dpdk_testpmd_job_t *testpmd_job)
{
    tapi_job_simple_desc_t desc = {
        .job_loc = &testpmd_job->job,
        .stdout_loc = &testpmd_job->out_channels[0],
        .stderr_loc = &testpmd_job->out_channels[1],
        .filters = TAPI_JOB_SIMPLE_FILTERS(
            {.use_stdout = TRUE,
             .readable = TRUE,
             .re = "(?m)tx_dbells\\s*([0-9]+)\\s*([0-9]+)",
             .extract = 2,
             .filter_var = &testpmd_job->tx_dbells_filter,
            },
            {.use_stderr = TRUE,
             .readable = TRUE,
             .re = "(?m)No\\sxstat\\s'tx_dbells'",
             .extract = 0,
             .filter_var = &testpmd_job->tx_dbells_skip_filter,
            }
        )
    };

    return tapi_dpdk_attach_filters(&desc);
}

te_errno
tapi_dpdk_add_rx_dbells_display(te_kvpair_h *test_params, const char *q_num)
{
    return tapi_dpdk_add_dbells_params(test_params, q_num, "rx");
}

te_errno
tapi_dpdk_add_tx_dbells_display(te_kvpair_h *test_params, const char *q_num)
{
    return tapi_dpdk_add_dbells_params(test_params, q_num, "tx");
}

te_errno
tapi_dpdk_stats_log_rx_dbells(const tapi_dpdk_testpmd_job_t *testpmd_job,
                              const te_meas_stats_t *meas_stats_pps)
{
    return tapi_dpdk_stats_log_dbells(testpmd_job->rx_dbells_filter,
                                      testpmd_job->rx_dbells_skip_filter,
                                      meas_stats_pps, "rx");
}

te_errno
tapi_dpdk_stats_log_tx_dbells(const tapi_dpdk_testpmd_job_t *testpmd_job,
                              const te_meas_stats_t *meas_stats_pps)
{
    return tapi_dpdk_stats_log_dbells(testpmd_job->tx_dbells_filter,
                                      testpmd_job->tx_dbells_skip_filter,
                                      meas_stats_pps, "tx");
}
