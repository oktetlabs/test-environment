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

static char *
test_arg2testpmd_arg(const char *test_arg)
{
    te_string result = TE_STRING_INIT;
    const char testpmd_arg_prefix[] = "--";
    size_t i;

    if (te_string_append(&result, "%s%s", testpmd_arg_prefix,
                         test_arg +
                         (sizeof(TAPI_DPDK_TESTPMD_ARG_PREFIX) - 1)) != 0)
    {
        TEST_FAIL("Failed to build testpmd cli argument");
    }

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
is_testpmd_command(const char *arg)
{
    return is_prefixed(arg, TAPI_DPDK_TESTPMD_COMMAND_PREFIX);
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
        if (te_string_append(&corelist, i == 0 ? "%lu" : ",%lu",
                             cpu_ids[i].thread_id) != 0)
        {
            TEST_FAIL("Failed to append corelist argument");
        }
    }

    append_argument("-l", argc_out, argv_out);
    append_argument(corelist.ptr, argc_out, argv_out);
    te_string_free(&corelist);
}

static te_errno
build_testpmd_arguments(rcf_rpc_server *rpcs, tapi_env *env, size_t n_cpus,
                        tapi_cpu_index_t *cpu_ids, const char *cmdline_file,
                        te_kvpair_h *test_args, int *argc_out, char ***argv_out)
{
    int extra_argc = 0;
    char **extra_argv = NULL;
    te_errno rc;
    int i;

    append_corelist_eal_arg(n_cpus, cpu_ids, &extra_argc, &extra_argv);
    append_argument("--", &extra_argc, &extra_argv);
    append_testpmd_arguments_from_test_args(test_args, &extra_argc,
                                            &extra_argv);
    if (cmdline_file != NULL)
    {
        append_argument("--cmdline-file", &extra_argc, &extra_argv);
        append_argument(cmdline_file, &extra_argc, &extra_argv);
    }
    append_argument(NULL, &extra_argc, &extra_argv);

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
append_testpmd_command(const te_kvpair *cmd_pair, te_string *buf)
{
    te_errno rc = 0;

    if (strcmp(cmd_pair->key, "testpmd_command_loopback_mode") == 0)
    {
        rc = te_string_append(buf, "port stop all\nport config all loopback %s\n"
                             "port start all\n", cmd_pair->value);
    }
    else if (strcmp(cmd_pair->key, "testpmd_command_start_tx_first") == 0)
    {
        rc = te_string_append(buf, "start tx_first %s\n", cmd_pair->value);
    }
    else if (strcmp(cmd_pair->key, "testpmd_command_start") == 0)
    {
        rc = te_string_append(buf, "start\n");
    }

    if (rc != 0)
        TEST_FAIL("Failed to append testpmd command");
}

static void
create_cmdline_file(te_kvpair_h *test_args, const char *dir,
                    char **cmdline_file, char **cmdline_buf)
{
    te_string buf = TE_STRING_INIT;
    const te_kvpair *pair;
    char *path;

    TAILQ_FOREACH(pair, test_args, links)
    {
        if (is_testpmd_command(pair->key) && strcmp(pair->value, "FALSE") != 0)
            append_testpmd_command(pair, &buf);
    }

    if (buf.len == 0)
    {
        te_string_free(&buf);
        *cmdline_file = NULL;
        *cmdline_buf = NULL;
        return;
    }

    if (te_asprintf(&path, "%s/%s_%s", dir, tapi_file_generate_name(),
                    COMMANDS_FILE_NAME) < 0)
    {
        TEST_FAIL("Failed to create testpmd commands file name");
    }

    *cmdline_file = path;
    *cmdline_buf = buf.ptr;

    return;
}

static te_errno
grab_cpus(const char *ta, size_t n_cpus, const tapi_cpu_prop_t *prop,
          tapi_cpu_index_t *cpu_ids)
{
    te_errno rc;
    size_t i;

    for (i = 0; i < n_cpus; i++)
    {
        rc = tapi_cfg_cpu_grab_by_prop(ta, prop, &cpu_ids[i]);
        if (rc != 0)
        {
            size_t j;

            WARN("Could not grab required CPUs, error: %s", te_rc_err2str(rc));
            for (j = 0; j < i; j++)
            {
                if (tapi_cfg_cpu_release_by_id(ta, &cpu_ids[i]) != 0)
                    WARN("Failed to release grabbed CPUs");
            }

            return rc;
        }
    }

    return 0;
}

static te_errno
grab_cpus_nonstrict_prop(const char *ta, size_t n_cpus,
                         const tapi_cpu_prop_t *prop, tapi_cpu_index_t *cpu_ids)
{
    tapi_cpu_prop_t fallback_prop;

    if (grab_cpus(ta, n_cpus, prop, cpu_ids) == 0)
        return 0;

    WARN("Fallback to grab not isolated CPUs");
    fallback_prop = *prop;
    fallback_prop.isolated = FALSE;

    return grab_cpus(ta, n_cpus, &fallback_prop, cpu_ids);
}

te_errno
tapi_dpdk_create_testpmd_job(rcf_rpc_server *rpcs, tapi_env *env,
                             size_t n_cpus, const tapi_cpu_prop_t *prop,
                             te_kvpair_h *test_args,
                             tapi_dpdk_testpmd_job_t *testpmd_job)
{
    te_string testpmd_path = TE_STRING_INIT;
    tapi_cpu_index_t *cpu_ids = NULL;
    int testpmd_argc = 0;
    char **testpmd_argv = NULL;
    char *cmdline_file = NULL;
    char *cmdline_buf = NULL;
    char *working_dir = NULL;
    te_errno rc = 0;
    int i;

    cpu_ids = tapi_calloc(n_cpus, sizeof(*cpu_ids));
    if ((rc = grab_cpus_nonstrict_prop(rpcs->ta, n_cpus, prop, cpu_ids)) != 0)
        goto out;

    rc = cfg_get_instance_fmt(NULL, &working_dir, "/agent:%s/dir:", rpcs->ta);
    if (rc != 0)
    {
        ERROR("Failed to get working directory");
        goto out;
    }

    create_cmdline_file(test_args, working_dir, &cmdline_file, &cmdline_buf);

    rc = build_testpmd_arguments(rpcs, env, n_cpus, cpu_ids, cmdline_file,
                                 test_args, &testpmd_argc, &testpmd_argv);
    if (rc != 0)
        goto out;

    if (te_string_append(&testpmd_path, "%sdpdk-testpmd", working_dir) != 0)
        TEST_FAIL("Failed to append testpmd path");


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
    testpmd_job->cmdline_buf = cmdline_buf;
    testpmd_job->ta = tapi_strdup(rpcs->ta);

out:
    if (rc != 0)
    {
        free(cmdline_file);
        free(cmdline_buf);
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
                                "%s", testpmd_job->cmdline_buf) != 0)
        {
            ERROR("Failed to create comand file on TA for testpmd");
            return TE_RC(TE_TAPI, TE_EFAIL);
        }

        RING("testpmd cmdline-file content:\n%s", testpmd_job->cmdline_buf);
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
    free(testpmd_job->cmdline_buf);

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
