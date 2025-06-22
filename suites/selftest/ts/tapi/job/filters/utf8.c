/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2025 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief TAPI Job filters test: UTF-8 strings
 *
 * TAPI Job filters test: UTF-8 strings
 */

/** @page filters-utf8 Testing filters on UTF-8 strings
 *
 * @objective Verify that TAPI Job can handle UTF-8 data properly.
 *
 * @param minlen                 Mininum length of a random chunk.
 * @param maxlen                 Maximum length of a random chunk.
 * @param block_size             Block size of the output.
 * @param utf8_len               Length of encoded UTF-8 sequences
 *                               (must be either 2, 3 or 4).
 *
 * @par Scenario:
 *
 */

#define TE_TEST_NAME "filters/utf8"

#include "filters_suite.h"
#include "tapi_file.h"

int
main(int argc, char **argv)
{
    rcf_rpc_server *pco_iut = NULL;
    tapi_job_factory_t *factory = NULL;
    size_t minlen;
    size_t maxlen;
    size_t block_size;
    unsigned int utf8_len;
    tapi_job_t *dd_job = NULL;
    tapi_job_channel_t *filter_handle = NULL;
    tapi_job_channel_t *output_channel = NULL;
    size_t pre_len;
    size_t needle_len;
    size_t post_len;
    char *needle = NULL;

    static const char * const utf8_specs[] = {
        [2] = "[\xC2-\xDF][\x80-\xBF]",
        [3] = "[\xE2-\xEC][\xA0-\xBF][\x80-\xBF]",
        [4] = "[\xF1-\xF4][\x80-\x8F][\x80-\xBF][\x80-\xBF]",
    };

    tapi_job_status_t status;
    te_string rfile = TE_STRING_INIT;
    te_string dd_if_param = TE_STRING_INIT;
    te_string dd_bs_param = TE_STRING_INIT;
    te_string matched = TE_STRING_INIT;
    te_string re = TE_STRING_INIT;

    TEST_START;

    TEST_GET_PCO(pco_iut);
    TEST_GET_UINT_PARAM(minlen);
    TEST_GET_UINT_PARAM(maxlen);
    TEST_GET_UINT_PARAM(block_size);
    TEST_GET_UINT_PARAM(utf8_len);

    TEST_STEP("Initialize factory");
    CHECK_RC(tapi_job_factory_rpc_create(pco_iut, &factory));


    TEST_STEP("Create the data file");

    tapi_file_make_name(&rfile);

    /*
     * All of the pre block, the needle and the post block must hold
     * a whole number of UTF-8 characters.
     */
    pre_len = te_rand_unsigned_div(minlen, maxlen, utf8_len, 0);
    needle_len = te_rand_unsigned_div(minlen, maxlen, utf8_len, 0);
    post_len = te_rand_unsigned_div(minlen, maxlen, utf8_len, 0);

    needle = te_make_spec_buf(needle_len, needle_len, NULL,
                              utf8_specs[utf8_len]);
    te_string_append(&re, "(*UTF)%.*s", (int)needle_len, needle);

    CHECK_RC(tapi_file_create_by_spec_ta(
                 pco_iut->ta, rfile.ptr,
                 (const tapi_file_chunk_spec[]){
                     {
                         .kind = TAPI_FILE_CHUNK_SPEC_KIND_PATTERN,
                         .minlen = pre_len,
                         .maxlen = pre_len,
                         .u = {.spec = utf8_specs[utf8_len]},
                     },
                     {
                         .kind = TAPI_FILE_CHUNK_SPEC_KIND_LITERAL,
                         .u = {.spec = needle},
                     },
                     {
                         .kind = TAPI_FILE_CHUNK_SPEC_KIND_PATTERN,
                         .minlen = post_len,
                         .maxlen = post_len,
                         .u = {.spec = utf8_specs[utf8_len]},
                     },
                     {.kind = TAPI_FILE_CHUNK_SPEC_KIND_END}
                 }));

    TEST_STEP("Create data dumping job");

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
                     .re = re.ptr,
                     .extract = 0,
                     .filter_var = &filter_handle,
                    }),
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
    te_string_free(&re);
    te_string_free(&matched);
    te_string_free(&dd_if_param);
    te_string_free(&dd_bs_param);
    te_string_free(&rfile);

    TEST_END;
}
