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

#include "te_config.h"

#include <stdio.h>

#include "tapi_dpdk.h"
#include "tapi_mem.h"
#include "tapi_test.h"
#include "tapi_rpc_rte_eal.h"
#include "tapi_file.h"

#define COMMANDS_FILE_NAME "testpmd_commands"
#define TESTPMD_MAX_PARAM_LEN 64
#define MAKE_TESTPMD_CMD(_cmd) (TAPI_DPDK_TESTPMD_COMMAND_PREFIX _cmd)
#define MAKE_TESTPMD_ARG(_arg) (TAPI_DPDK_TESTPMD_ARG_PREFIX _arg)
#define TESTPMD_CMD_PRE_SETUP "show port info all\nport stop all\n"
#define TESTPMD_CMD_POST_SETUP "port start all\n"
#define TESTPMD_TOTAL_MBUFS_MIN 2048
#define MBUF_OVERHEAD 256
#define ETHER_MIN_MTU 68
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
    TESTPMD_PARAM_TXPKTS,
    TESTPMD_PARAM_TXQ,
    TESTPMD_PARAM_RXQ,
    TESTPMD_PARAM_TXD,
    TESTPMD_PARAM_RXD,
    TESTPMD_PARAM_MTU,
    TESTPMD_PARAM_LPBK_MODE,
    TESTPMD_PARAM_START_TX_FIRST,
    TESTPMD_PARAM_START,
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
    [TESTPMD_PARAM_TXPKTS] = {
        .key = MAKE_TESTPMD_ARG("txpkts"),
        .type = TESTPMD_PARAM_TYPE_STRING, .str_val = "64"},
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

static void
append_argument(const char *argument, int *argc_p, char ***argv_p)
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
            append_argument(NULL, argc_out, argv_out);
            (*argv_out)[*argc_out - 1] = test_arg2testpmd_arg(pair->key);

            /* Append value, if not boolean */
            if (strcmp(pair->value, "TRUE") != 0)
                append_argument(pair->value, argc_out, argv_out);
        }
    }
}

static void
append_corelist_eal_arg(size_t n_cores, tapi_cpu_index_t *cpu_ids,
                        int *argc_out, char ***argv_out)
{
    te_string corelist = TE_STRING_INIT;
    size_t i;

    for (i = 0; i < n_cores; i++)
    {
        CHECK_RC(te_string_append(&corelist, i == 0 ? "%lu" : ",%lu",
                                  cpu_ids[i].thread_id));
    }

    append_argument("-l", argc_out, argv_out);
    append_argument(corelist.ptr, argc_out, argv_out);
    te_string_free(&corelist);
}

static te_errno
build_eal_testpmd_arguments(rcf_rpc_server *rpcs, tapi_env *env, size_t n_cpus,
                            tapi_cpu_index_t *cpu_ids, int *argc_out,
                            char ***argv_out)
{
    int extra_argc = 0;
    char **extra_argv = NULL;
    te_errno rc;
    int i;

    append_corelist_eal_arg(n_cpus, cpu_ids, &extra_argc, &extra_argv);

    rc = tapi_rte_make_eal_args(env, rpcs, extra_argc,
                                (const char **)extra_argv,
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
    va_list  ap;
    te_bool start = FALSE;
    te_bool setup = FALSE;

    switch (param)
    {
        case TESTPMD_PARAM_LPBK_MODE:
            CHECK_RC(te_string_append(setup_cmd, "port config all loopback "));
            setup = TRUE;
            break;

        case TESTPMD_PARAM_MTU:
            CHECK_RC(te_string_append(setup_cmd, "port config mtu %u ", port_number));
            setup = TRUE;
            break;

        case TESTPMD_PARAM_START_TX_FIRST:
            CHECK_RC(te_string_append(start_cmd, "start tx_first "));
            start = TRUE;
            break;

        case TESTPMD_PARAM_START:
            CHECK_RC(te_string_append(start_cmd, "start\n"));
            break;

        default:
            return;
    }

    va_start(ap, cmd_val_fmt);
    if (setup)
    {
        CHECK_RC(te_string_append_va(setup_cmd, cmd_val_fmt, ap));
        CHECK_RC(te_string_append(setup_cmd, "\n"));
    }
    else if (start)
    {
        CHECK_RC(te_string_append_va(start_cmd, cmd_val_fmt, ap));
        CHECK_RC(te_string_append(start_cmd, "\n"));
    }
    va_end(ap);
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
                        te_string *cmdline_setup, te_string *cmdline_start,
                        int *argc_out, char ***argv_out)
{
    const te_kvpair *pair;
    testpmd_param *params;
    te_bool *param_is_set;
    uint64_t txpkts_size;
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
                if (params[i].type == TESTPMD_PARAM_TYPE_STRING)
                {
                    if (strlen(pair->value) + 1 > TESTPMD_MAX_PARAM_LEN)
                    {
                        free(params);
                        free(param_is_set);
                        return rc;
                    }
                    strcpy(params[i].str_val, pair->value);
                    param_is_set[i] = TRUE;
                    break;
                }
                else if (params[i].type == TESTPMD_PARAM_TYPE_UINT64)
                {
                    if ((rc = te_strtoul(pair->value, 0, &params[i].val)) != 0)
                    {
                        free(params);
                        free(param_is_set);
                        return rc;
                    }
                    param_is_set[i] = TRUE;

                    break;
                }
                else
                {
                    ERROR("Unknown testpmd parameter type %d", params[i].type);
                    return TE_RC(TE_TAPI, TE_EINVAL);
                }
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
                                      params[TESTPMD_PARAM_TXD].val) +
                                     (params[TESTPMD_PARAM_RXQ].val *
                                      params[TESTPMD_PARAM_RXD].val);

        char *value;

        if (needed_mbuf_count < TESTPMD_TOTAL_MBUFS_MIN)
            needed_mbuf_count = TESTPMD_TOTAL_MBUFS_MIN;

        if (te_asprintf(&value, "%lu", needed_mbuf_count) < 0)
            TEST_FAIL("Failed to build total-num-mbufs testpmd parameter");

        append_argument("--total-num-mbufs", argc_out, argv_out);
        append_argument(value, argc_out, argv_out);
        free(value);
    }
    if (!param_is_set[TESTPMD_PARAM_MBUF_SIZE] &&
        (txpkts_size + MBUF_OVERHEAD) > params[TESTPMD_PARAM_MBUF_SIZE].val)
    {
        char *value;

        if (te_asprintf(&value, "%lu", (txpkts_size + MBUF_OVERHEAD)) < 0)
            TEST_FAIL("Failed to build mbuf-size testpmd parameter");

        append_argument("--mbuf-size", argc_out, argv_out);
        append_argument(value, argc_out, argv_out);
        free(value);
    }
    if (!param_is_set[TESTPMD_PARAM_MTU] && txpkts_size >= ETHER_MIN_MTU)
    {
        append_testpmd_command(port_number, cmdline_setup, cmdline_start,
                               TESTPMD_PARAM_MTU, "%lu", txpkts_size);
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
grab_cpus(const char *ta, size_t n_cpus_preferred, size_t n_cpus_required,
          const tapi_cpu_prop_t *prop, size_t *n_cpus_grabbed,
          tapi_cpu_index_t *cpu_ids)
{
    te_errno rc;
    size_t i;

    for (i = 0; i < n_cpus_preferred; i++)
    {
        rc = tapi_cfg_cpu_grab_by_prop(ta, prop, &cpu_ids[i]);
        if (rc != 0)
        {
            size_t j;

            WARN("Could not grab preferred quantity of CPUs, error: %s",
                 te_rc_err2str(rc));
            if (TE_RC_GET_ERROR(rc) == TE_ENOENT && i >= n_cpus_required)
            {
                WARN_ARTIFACT("Only %lu CPUs were grabbed", i);
                *n_cpus_grabbed = i;
                return 0;
            }

            for (j = 0; j < i; j++)
            {
                if (tapi_cfg_cpu_release_by_id(ta, &cpu_ids[j]) != 0)
                    WARN("Failed to release grabbed CPUs");
            }

            return rc;
        }
    }

    *n_cpus_grabbed = n_cpus_preferred;
    return 0;
}

static te_errno
grab_cpus_nonstrict_prop(const char *ta, size_t n_cpus_preferred,
                         size_t n_cpus_required, const tapi_cpu_prop_t *prop,
                         size_t *n_cpus_grabbed, tapi_cpu_index_t *cpu_ids)
{
    /*
     * When grabbing CPUs with required property, set also a strict
     * constraint on CPUs quantity (n_cpus_required is set to n_cpus_preferred)
     */
    if (grab_cpus(ta, n_cpus_preferred, n_cpus_preferred, prop,
                  n_cpus_grabbed, cpu_ids) == 0)
    {
        return 0;
    }

    WARN("Fallback to grab any available CPUs");

    return grab_cpus(ta, n_cpus_preferred, n_cpus_required, NULL,
                     n_cpus_grabbed, cpu_ids);
}

const char *
get_vdev_eal_argument(int eal_argc, char **eal_argv)
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
get_vdev_port_number(const char *vdev, unsigned int *port_number)
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
     * devices. It might fail if --vdev and --pci-whitelist have
     * different devices specified.
     */
    *port_number = dev_count;

    return 0;
}

void
append_testpmd_nb_cores_arg(size_t n_fwd_cpus, int *argc_out, char ***argv_out)
{
    te_string nb_cores = TE_STRING_INIT;

    append_argument("--nb-cores", argc_out, argv_out);

    CHECK_RC(te_string_append(&nb_cores, "%lu", n_fwd_cpus));

    append_argument(nb_cores.ptr, argc_out, argv_out);

    te_string_free(&nb_cores);
}

te_errno
tapi_dpdk_create_testpmd_job(rcf_rpc_server *rpcs, tapi_env *env,
                             size_t n_fwd_cpus, const tapi_cpu_prop_t *prop,
                             te_kvpair_h *test_args,
                             tapi_dpdk_testpmd_job_t *testpmd_job)
{
    te_string testpmd_path = TE_STRING_INIT;
    tapi_cpu_index_t *cpu_ids = NULL;
    int testpmd_argc = 0;
    char **testpmd_argv = NULL;
    char *cmdline_file = NULL;
    te_string cmdline_setup = TE_STRING_INIT;
    te_string cmdline_start = TE_STRING_INIT;
    char *working_dir = NULL;
    const char *vdev_arg = NULL;
    unsigned int port_number = 0;
    te_errno rc = 0;
    /* The first CPU is reserved by testpmd for command-line processing */
    size_t n_cpus = n_fwd_cpus + 1;
    size_t n_cpus_grabbed;
    int i;

    if (n_fwd_cpus == 0)
    {
        ERROR("Testpmd cannot be run with 0 forwarding cores");
        rc = TE_RC(TE_TAPI, TE_EINVAL);
        goto out;
    }

    cpu_ids = tapi_calloc(n_cpus, sizeof(*cpu_ids));
    if ((rc = grab_cpus_nonstrict_prop(rpcs->ta, n_cpus, TESTPMD_MIN_N_CORES,
                                       prop, &n_cpus_grabbed, cpu_ids)) != 0)
    {
        goto out;
    }

    rc = cfg_get_instance_fmt(NULL, &working_dir, "/agent:%s/dir:", rpcs->ta);
    if (rc != 0)
    {
        ERROR("Failed to get working directory");
        goto out;
    }

    rc = build_eal_testpmd_arguments(rpcs, env, n_cpus_grabbed, cpu_ids,
                                     &testpmd_argc, &testpmd_argv);
    if (rc != 0)
        goto out;

    vdev_arg = get_vdev_eal_argument(testpmd_argc, testpmd_argv);
    if (vdev_arg != NULL)
    {
        if ((rc = get_vdev_port_number(vdev_arg, &port_number)) != 0)
            goto out;
    }

    /* Separate EAL arguments from testpmd arguments */
    append_argument("--", &testpmd_argc, &testpmd_argv);

    rc = adjust_testpmd_defaults(test_args, port_number, &cmdline_setup,
                                 &cmdline_start, &testpmd_argc, &testpmd_argv);
    if (rc != 0)
        goto out;

    generate_cmdline_filename(working_dir, &cmdline_file);
    append_testpmd_cmdline_from_args(test_args, port_number, &cmdline_setup,
                                     &cmdline_start);
    append_testpmd_nb_cores_arg(n_cpus_grabbed - 1, &testpmd_argc, &testpmd_argv);
    append_argument("--cmdline-file", &testpmd_argc, &testpmd_argv);
    append_argument(cmdline_file, &testpmd_argc, &testpmd_argv);

    append_testpmd_arguments_from_test_args(test_args, &testpmd_argc,
                                            &testpmd_argv);

    /* Terminate argv with NULL */
    append_argument(NULL, &testpmd_argc, &testpmd_argv);

    CHECK_RC(te_string_append(&testpmd_path, "%sdpdk-testpmd", working_dir));

    rc = tapi_job_rpc_simple_create(rpcs,
                          &(tapi_job_simple_desc_t){
                                .program = testpmd_path.ptr,
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
                                     .re = "(?m)^Link speed: ([0-9]+) Mbps$",
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
    free(cpu_ids);
    free(working_dir);

    for (i = 0; i < testpmd_argc; i++)
        free(testpmd_argv[i]);
    free(testpmd_argv);
    te_string_free(&testpmd_path);

    return rc;
}

te_errno
tapi_dpdk_testpmd_start(tapi_dpdk_testpmd_job_t *testpmd_job)
{
    if (testpmd_job->cmdline_file != NULL)
    {
        if (tapi_file_create_ta(testpmd_job->ta, testpmd_job->cmdline_file,
                "%s%s%s%s", TESTPMD_CMD_PRE_SETUP,
                empty_string_if_null(testpmd_job->cmdline_setup.ptr),
                TESTPMD_CMD_POST_SETUP,
                empty_string_if_null(testpmd_job->cmdline_start.ptr)) != 0)
        {
            ERROR("Failed to create comand file on TA for testpmd");
            return TE_RC(TE_TAPI, TE_EFAIL);
        }

        RING("testpmd cmdline-file content:\n%s%s%s%s", TESTPMD_CMD_PRE_SETUP,
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

    if ((rc = te_strtoui(buf.data.ptr, 0, link_speed)) != 0)
    {
        ERROR("Failed to parse link speed");
        te_string_free(&buf.data);
        return TE_RC(TE_TAPI, TE_EFAIL);
    }

    return 0;
}

te_errno
tapi_dpdk_testpmd_get_stats(tapi_dpdk_testpmd_job_t *testpmd_job,
                            unsigned int n_stats,
                            tapi_dpdk_testpmd_stats_t *stats)
{
    tapi_job_buffer_t buf = TAPI_JOB_BUFFER_INIT;
    unsigned int tx_pps_received = 0;
    unsigned int rx_pps_received = 0;
    unsigned long tx_pps = 0;
    unsigned long rx_pps = 0;
    te_errno rc = 0;

    while (tx_pps_received < n_stats || rx_pps_received < n_stats)
    {
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
            tx_pps_received++;
        }
        else if (buf.filter == testpmd_job->rx_pps_filter)
        {
            if ((rc = te_strtoul(buf.data.ptr, 0, &rx_pps)) != 0)
                goto out;
            rx_pps_received++;
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
    }

    if (tx_pps_received < n_stats || rx_pps_received < n_stats)
    {
        ERROR("Channel had been closed before required number of stats were received");
        rc = TE_RC(TE_TAPI, TE_EFAIL);
        goto out;
    }

    stats->tx_pps = tx_pps;
    stats->rx_pps = rx_pps;

out:
    te_string_free(&buf.data);

    return rc;
}
