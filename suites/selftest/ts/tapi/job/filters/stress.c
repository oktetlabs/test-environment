/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2025 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief TAPI Job filters test: stress testing
 *
 * TAPI Job filters test: stress testing
 */

/** @page filters-stress Stress testing TAPI Job filters
 *
 * @objective Verify that TAPI Job does not break on large random data.
 *
 * @param minlen                 Minimum length of a single line.
 * @param maxlen                 Maximum length of a single line.
 * @param n_false_starts         Number of partial matches that do not end
 *                               in a full match before a full match.
 * @param num_matches            Number of matches.
 * @param extract                Subexpression to extract (must be 0, 1 or 2).
 * @param lookbehind             Use a regexp with a lookbehind constraint.
 * @param wait_before_receive    Wait for the job completion before trying
 *                               to receive matches.
 * @param ascii                  Use only ASCII characters if true, or
 *                               the full 8-bit range if false.
 *
 * @par Scenario:
 *
 */

#define TE_TEST_NAME "filters/stress"

#include "filters_suite.h"
#include "tapi_file.h"
#include "te_rand.h"

#define NEEDLE_PFX "<<< "
#define NEEDLE_1 "Find"
#define NEEDLE_SEP
#define NEEDLE_2 "This"
#define NEEDLE_SFX ">>>"
#define NEEDLE NEEDLE_PFX NEEDLE_1 NEEDLE_SEP NEEDLE_2 NEEDLE_SFX
#define CONTEXT "After That --->"

#define NEEDLE_RE \
    NEEDLE_PFX "(" NEEDLE_1 ")" NEEDLE_SEP "(" NEEDLE_2 ")" NEEDLE_SFX

#define FAKE_NEEDLE \
    CONTEXT NEEDLE_PFX NEEDLE_1 NEEDLE_SEP

static void
make_chunks(const char *ta, const char *filename,
            size_t minlen, size_t maxlen,
            size_t n_false_starts, size_t num_matches,
            bool ascii)
{
    const char *spec = ascii ? "[\x1-\x7F^\n]" : "[^`\0\n]";
    tapi_file_chunk_spec body[] = {
        {
            .kind = TAPI_FILE_CHUNK_SPEC_KIND_COMPOUND,
            .minlen = num_matches,
            .maxlen = num_matches,
            .u = {
                .nested = (const tapi_file_chunk_spec[]){
                    {
                        .kind = TAPI_FILE_CHUNK_SPEC_KIND_COMPOUND,
                        .minlen = n_false_starts,
                        .maxlen = n_false_starts,
                        .u = {
                            .nested = (const tapi_file_chunk_spec[]){
                                {
                                    .kind = TAPI_FILE_CHUNK_SPEC_KIND_PATTERN,
                                    .minlen = minlen,
                                    .maxlen = maxlen,
                                    .u = {.spec = spec},
                                },
                                {
                                    .kind = TAPI_FILE_CHUNK_SPEC_KIND_LITERAL,
                                    .u = {.spec = FAKE_NEEDLE},
                                },
                                {.kind = TAPI_FILE_CHUNK_SPEC_KIND_END}
                            }
                        }
                    },
                    {
                        .kind = TAPI_FILE_CHUNK_SPEC_KIND_PATTERN,
                        .minlen = minlen,
                        .maxlen = maxlen,
                        .u = {.spec = spec},
                    },
                    {
                        .kind = TAPI_FILE_CHUNK_SPEC_KIND_LITERAL,
                        .u = {.spec = CONTEXT NEEDLE},
                    },
                    {.kind = TAPI_FILE_CHUNK_SPEC_KIND_END}
                }
            },
        },
        {.kind = TAPI_FILE_CHUNK_SPEC_KIND_END}
    };

    CHECK_RC(tapi_file_create_by_spec_ta(ta, filename, body));
}

int
main(int argc, char **argv)
{
    rcf_rpc_server *pco_iut = NULL;
    tapi_job_factory_t *factory = NULL;
    size_t minlen;
    size_t maxlen;
    size_t n_false_starts;
    size_t num_matches;
    unsigned int extract;
    bool lookbehind;
    bool wait_before_receive;
    bool ascii;
    bool file_created = false;
    static const char *expected[] = {
        [0] = NEEDLE,
        [1] = NEEDLE_1,
        [2] = NEEDLE_2,
    };

    tapi_job_t *sed_job = NULL;
    tapi_job_channel_t *filter_handle = NULL;
    tapi_job_channel_t *output_channel = NULL;

    tapi_job_status_t status;
    size_t i;
    te_string rfile = TE_STRING_INIT;
    tapi_job_buffer_t buffer = TAPI_JOB_BUFFER_INIT;

    TEST_START;

    TEST_GET_PCO(pco_iut);
    TEST_GET_UINT_PARAM(minlen);
    TEST_GET_UINT_PARAM(maxlen);
    TEST_GET_UINT_PARAM(n_false_starts);
    TEST_GET_UINT_PARAM(num_matches);
    TEST_GET_UINT_PARAM(extract);
    TEST_GET_BOOL_PARAM(lookbehind);
    TEST_GET_BOOL_PARAM(wait_before_receive);
    TEST_GET_BOOL_PARAM(ascii);

    TEST_STEP("Initialize factory");
    CHECK_RC(tapi_job_factory_rpc_create(pco_iut, &factory));

    TEST_STEP("Create data dumping job");
    tapi_file_make_name(&rfile);

    CHECK_RC(tapi_job_simple_create(factory, &(tapi_job_simple_desc_t){
                .program = "/usr/bin/sed",
                .argv = (const char *[]){"sed", "-e", "", "-u",
                                         rfile.ptr, NULL},
                .job_loc = &sed_job,
                .stdout_loc = &output_channel,
                .filters = TAPI_JOB_SIMPLE_FILTERS(
                    {.readable = true,
                     .use_stdout = true,
                     .re = (lookbehind ?
                            "(?<=" CONTEXT ")" NEEDLE_RE :
                            NEEDLE_RE),
                     .extract = extract,
                     .filter_var = &filter_handle,
                    }),
            }));

    TEST_STEP("Create the data file");
    make_chunks(pco_iut->ta, rfile.ptr, minlen, maxlen, n_false_starts,
                num_matches, ascii);
    file_created = true;

    TEST_STEP("Start the job");
    CHECK_RC(tapi_job_start(sed_job));

    if (wait_before_receive)
        CHECK_RC(tapi_job_wait(sed_job, -1, &status));

    TEST_STEP("Get the matching output");
    for (i = 0; i < num_matches; i++)
    {
        tapi_job_simple_receive(TAPI_JOB_CHANNEL_SET(filter_handle),
                                wait_before_receive ? 0 : -1,
                                &buffer);
        if (buffer.eos)
            TEST_VERDICT("Not enough messages");

        if (strcmp(expected[extract], buffer.data.ptr) != 0)
        {
            TEST_VERDICT("The %zu'th  matched string differs "
                         "from the expected one", i);
        }
    }

    tapi_job_simple_receive(TAPI_JOB_CHANNEL_SET(filter_handle), 0, &buffer);
    if (!buffer.eos)
        TEST_VERDICT("Too many messages");

    if (!wait_before_receive)
        CHECK_RC(tapi_job_wait(sed_job, -1, &status));

    if (status.type != TAPI_JOB_STATUS_EXITED || status.value != 0)
        TEST_VERDICT("The script did not terminate correctly");

    TEST_SUCCESS;

cleanup:
    if (file_created)
        CLEANUP_CHECK_RC(tapi_file_ta_unlink_fmt(pco_iut->ta, "%s", rfile.ptr));
    CLEANUP_CHECK_RC(tapi_job_destroy(sed_job, -1));
    tapi_job_factory_destroy(factory);
    te_string_free(&buffer.data);
    te_string_free(&rfile);

    TEST_END;
}
