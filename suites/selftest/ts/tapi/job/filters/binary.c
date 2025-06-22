/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2025 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief TAPI Job filters test: binary data
 *
 * TAPI Job filters test: binary data
 */

/** @page filters-binary Testing filters on binary data
 *
 * @objective Verify that TAPI Job can handle binary data properly
 *
 * @param minlen                 Mininum length of a random chunk.
 * @param maxlen                 Maximum length of a random chunk.
 * @param block_size             Block size of the output.
 * @param binary_needle          Use binary zeroes in the needle string
 *                               as well as in context.
 *
 * @par Scenario:
 *
 */

#define TE_TEST_NAME "filters/binary"

#include "filters_suite.h"
#include "tapi_file.h"

#define NEEDLE_PFX "<<< Find Me: "
#define NEEDLE_SFX ">>>"

#define NEEDLE_RE NEEDLE_PFX "[^>]+" NEEDLE_SFX

int
main(int argc, char **argv)
{
    rcf_rpc_server *pco_iut = NULL;
    tapi_job_factory_t *factory = NULL;
    size_t minlen;
    size_t maxlen;
    size_t block_size;
    bool binary_needle;
    tapi_job_t *dd_job = NULL;
    tapi_job_channel_t *filter_handle = NULL;
    tapi_job_channel_t *output_channel = NULL;
    size_t needle_len;
    char *needle = NULL;

    tapi_job_status_t status;
    te_string rfile = TE_STRING_INIT;
    te_string dd_if_param = TE_STRING_INIT;
    te_string dd_bs_param = TE_STRING_INIT;
    te_string matched = TE_STRING_INIT;

    TEST_START;

    TEST_GET_PCO(pco_iut);
    TEST_GET_UINT_PARAM(minlen);
    TEST_GET_UINT_PARAM(maxlen);
    TEST_GET_UINT_PARAM(block_size);
    TEST_GET_BOOL_PARAM(binary_needle);

    TEST_STEP("Initialize factory");
    CHECK_RC(tapi_job_factory_rpc_create(pco_iut, &factory));

    TEST_STEP("Create data dumping job");
    tapi_file_make_name(&rfile);

    te_string_append(&dd_if_param, "if=%s", rfile.ptr);
    te_string_append(&dd_bs_param, "bs=%zu", block_size);

    CHECK_RC(tapi_job_simple_create(factory, &(tapi_job_simple_desc_t){
                .program = "/usr/bin/dd",
                .argv = (const char *[]){"dd",
                                         dd_if_param.ptr,
                                         dd_bs_param.ptr,
                                         NULL},
                .job_loc = &dd_job,
                .stdout_loc = &output_channel,
                .filters = TAPI_JOB_SIMPLE_FILTERS(
                    {.readable = true,
                     .use_stdout = true,
                     .re = NEEDLE_RE,
                     .extract = 0,
                     .filter_var = &filter_handle,
                    }),
            }));

    TEST_STEP("Create the data file");
    needle = te_make_spec_buf(minlen, maxlen, &needle_len,
                              binary_needle ?
                              NEEDLE_PFX "([^>])" NEEDLE_SFX :
                              NEEDLE_PFX  "([^`\0>])" NEEDLE_PFX);

    CHECK_RC(tapi_file_create_by_spec_ta(
                 pco_iut->ta, rfile.ptr,
                 (const tapi_file_chunk_spec[]){
                     {
                         .kind = TAPI_FILE_CHUNK_SPEC_KIND_PATTERN,
                         .minlen = minlen,
                         .maxlen = maxlen,
                         .u = {.spec = "[]"},
                     },
                     {
                         .kind = TAPI_FILE_CHUNK_SPEC_KIND_LITERAL,
                         .maxlen = needle_len,
                         .u = {.spec = needle},
                     },
                     {
                         .kind = TAPI_FILE_CHUNK_SPEC_KIND_PATTERN,
                         .minlen = minlen,
                         .maxlen = maxlen,
                         .u = {.spec = "[]"},
                     },
                     {.kind = TAPI_FILE_CHUNK_SPEC_KIND_END}
                 }));

    TEST_STEP("Start the job");
    CHECK_RC(tapi_job_start(dd_job));

    TEST_STEP("Get the matching output");
    CHECK_RC(tapi_job_receive_single(filter_handle, &matched, -1));

    if (!te_compare_bufs(needle, needle_len, 1, matched.ptr, matched.len,
                         TE_LL_ERROR))
    {
        TEST_VERDICT("Invalid matched string");
    }

    CHECK_RC(tapi_job_wait(dd_job, -1, &status));

    if (status.type != TAPI_JOB_STATUS_EXITED || status.value != 0)
        TEST_VERDICT("The script did not terminate correctly");

    TEST_SUCCESS;

cleanup:
    CLEANUP_CHECK_RC(tapi_job_destroy(dd_job, -1));
    tapi_job_factory_destroy(factory);
    free(needle);
    te_string_free(&matched);
    te_string_free(&dd_if_param);
    te_string_free(&dd_bs_param);
    te_string_free(&rfile);

    TEST_END;
}
