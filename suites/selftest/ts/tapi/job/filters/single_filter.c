/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2025 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief TAPI Job filters test: single filter
 *
 * TAPI Job filters test: single filter
 */

/** @page filters-single-filter Test for a single filter in TAPI Job filters.
 *
 * @objective Validate tapi_job_receive() in various circumstances.
 *
 * @param filter_chunks        List of output segments.
 * @param filter_newline       Separate output segments with newline if true.
 * @param filter_regexp        Regexp to look for.
 * @param filter_extract       Regexp group to extract
 *                             (0 means the whole mathed substring).
 * @param use_stdout           Use stdout if true, otherwise stderr.
 * @param delay                Delay in seconds between output segments.
 * @param wait_before_receive  Wait for the job completion before
 *                             trying to get matches.
 *
 * @par Scenario:
 *
 */

#define TE_TEST_NAME "filters/match_single"

#include "filters_suite.h"

static void
build_script(te_string *dest, size_t n_lines, char **lines,
             unsigned int delay, bool use_stdout, bool newline)
{
    size_t i;

    for (i = 0; i < n_lines; i++)
    {
        if (delay > 0)
            te_string_append(dest, "sleep %u; ", delay);
        te_string_append(dest, "echo %s",
                         newline || i + 1 == n_lines ? "" : "-n ");
        te_string_append_shell_arg_as_is(dest, lines[i]);
        te_string_append(dest, "%s\n", use_stdout ? "" : " >&2");
    }
}

int
main(int argc, char **argv)
{
    rcf_rpc_server *pco_iut = NULL;
    tapi_job_factory_t *factory = NULL;
    char **filter_chunks;
    size_t n_filter_chunks;
    bool filter_newline;
    const char *filter_regexp;
    unsigned int filter_extract;
    char **filter_expected;
    size_t n_filter_expected;
    bool use_stdout;

    unsigned int delay;
    bool wait_before_receive;

    tapi_job_t *shell_job = NULL;
    tapi_job_channel_t *filter_handle = NULL;
    tapi_job_channel_t *output_channel = NULL;
    tapi_job_simple_filter_t filter_desc[] = {
        {.readable = true,
         .log_level = TE_LL_RING,
         .filter_var = &filter_handle,
        },
        {.use_stdout = false, .use_stderr = false},
    };
    tapi_job_simple_desc_t desc = {
        .program = "/bin/sh",
        .job_loc = &shell_job,
        .filters = filter_desc,
    };
    tapi_job_status_t status;
    te_string script = TE_STRING_INIT;
    int timeout_ms;
    size_t i;
    tapi_job_buffer_t buffer = TAPI_JOB_BUFFER_INIT;

    TEST_START;

    TEST_GET_PCO(pco_iut);
    TEST_GET_STRING_LIST_PARAM(filter_chunks, n_filter_chunks);
    TEST_GET_BOOL_PARAM(filter_newline);
    TEST_GET_STRING_PARAM(filter_regexp);
    TEST_GET_UINT_PARAM(filter_extract);
    TEST_GET_STRING_LIST_PARAM(filter_expected, n_filter_expected);
    TEST_GET_BOOL_PARAM(use_stdout);
    TEST_GET_INT_PARAM(delay);
    TEST_GET_BOOL_PARAM(wait_before_receive);

    timeout_ms = TE_SEC2MS(delay * n_filter_chunks + 1);

    build_script(&script, n_filter_chunks, filter_chunks, delay,
                 use_stdout, filter_newline);

    desc.argv = (const char *[]){"sh", "-c", script.ptr, NULL};
    filter_desc[0].use_stdout = use_stdout;
    filter_desc[0].use_stderr = !use_stdout;
    filter_desc[0].re = filter_regexp;
    filter_desc[0].extract = filter_extract;
    if (use_stdout)
        desc.stdout_loc = &output_channel;
    else
        desc.stderr_loc = &output_channel;

    TEST_STEP("Initialize factory");
    CHECK_RC(tapi_job_factory_rpc_create(pco_iut, &factory));

    TEST_STEP("Create scripting job");
    CHECK_RC(tapi_job_simple_create(factory, &desc));

    TEST_STEP("Start the job");
    CHECK_RC(tapi_job_start(shell_job));

    if (wait_before_receive)
    {
        TEST_STEP("Waiting for the job to complete");
        CHECK_RC(tapi_job_wait(shell_job, timeout_ms, &status));
    }

    TEST_STEP("Get the matching output");
    for (i = 0; i < n_filter_expected; i++)
    {
        tapi_job_simple_receive(TAPI_JOB_CHANNEL_SET(filter_handle),
                                wait_before_receive ? 0 : timeout_ms,
                                &buffer);
        if (buffer.eos)
            TEST_VERDICT("Not enough messages");

        if (strcmp(filter_expected[i], buffer.data.ptr) != 0)
        {
            TEST_VERDICT("The %zu'th  matched string differs "
                         "from the expected one", i);
        }
    }

    if (!wait_before_receive)
    {
        TEST_STEP("Waiting for the job to complete");
        CHECK_RC(tapi_job_wait(shell_job, timeout_ms, &status));
    }

    tapi_job_simple_receive(TAPI_JOB_CHANNEL_SET(filter_handle), 0, &buffer);
    if (!buffer.eos)
        TEST_VERDICT("Too many messages");

    if (status.type != TAPI_JOB_STATUS_EXITED || status.value != 0)
        TEST_VERDICT("The script did not terminate correctly");

    TEST_SUCCESS;

cleanup:
    CLEANUP_CHECK_RC(tapi_job_destroy(shell_job, -1));
    tapi_job_factory_destroy(factory);
    te_string_free(&buffer.data);
    te_string_free(&script);

    /* FIXME: there should be a more elegant way */
    free(filter_chunks[0]);
    free(filter_chunks);

    TEST_END;
}
