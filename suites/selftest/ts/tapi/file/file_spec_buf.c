/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2025 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief TAPI File Test Suite
 *
 * Generate a file according to a given spec.
 */

/** @page file_spec_buf Test for creating a file with randomized content.
 *
 * @objective Test that generating files by a given spec works properly.
 *
 * @param minlen     Minimum length of a random chunk
 * @param maxlen     Maximum length of a random chunk
 * @param minrepeat  Minimum number of chunks
 * @param maxrepeat  Maximum number of chunks
 *
 * @par Scenario:
 *
 */

#define TE_TEST_NAME    "file_spec_buf"

#include "file_suite.h"
#include <ctype.h>

#define CUT_LINE "-8<-"
#define LINE_PREFIX ">>> "
#define GARBAGE "--- should be omitted ---"

static void
make_chunks(const char *ta, const char *filename,
            size_t minlen, size_t maxlen,
            size_t minrepeat, size_t maxrepeat)
{
    tapi_file_chunk_spec subchunks[] = {
        {
            .kind = TAPI_FILE_CHUNK_SPEC_KIND_LITERAL,
            .maxlen = sizeof(LINE_PREFIX) - 1,
            .u = {.spec = LINE_PREFIX GARBAGE},
        },
        {
            .kind = TAPI_FILE_CHUNK_SPEC_KIND_PATTERN,
            .minlen = minlen,
            .maxlen = maxlen,
            .u = {.spec = "([0-9a-fA-F])\n"},
        },
        {.kind = TAPI_FILE_CHUNK_SPEC_KIND_END}
    };
    tapi_file_chunk_spec chunks[] = {
        {
            .kind = TAPI_FILE_CHUNK_SPEC_KIND_LITERAL,
            .minlen = maxlen,
            .u = {.spec = CUT_LINE},
        },
        {
            .kind = TAPI_FILE_CHUNK_SPEC_KIND_LITERAL,
            .u = {.spec = "\n"},
        },
        {
            .kind = TAPI_FILE_CHUNK_SPEC_KIND_COMPOUND,
            .minlen = minrepeat,
            .maxlen = maxrepeat,
            .u = {.nested = subchunks},
        },
        {.kind = TAPI_FILE_CHUNK_SPEC_KIND_END}
    };

    CHECK_RC(tapi_file_create_by_spec_ta(ta, filename, chunks));
}

static void
validate_chunks(char *content, size_t minlen, size_t maxlen,
                size_t minrepeat, size_t maxrepeat)
{
    char *line;
    size_t n;

    strtok(content, "\n");

    if (!te_compare_bufs(CUT_LINE, sizeof(CUT_LINE) - 1,
                         maxlen / (sizeof(CUT_LINE) - 1),
                         content, strlen(content), 0))
        TEST_VERDICT("Wrong first line: %s", content);

    for (line = strtok(NULL, "\n"), n = 0; line != NULL;
         line = strtok(NULL, "\n"), n++)
    {
        const char *no_prefix;
        size_t line_len;

        if (n > maxrepeat)
            TEST_VERDICT("Too many lines");
        no_prefix = te_str_strip_prefix(line, LINE_PREFIX);
        if (no_prefix == NULL)
            TEST_VERDICT("Invalid line prefix");
        if (te_str_strip_prefix(no_prefix, GARBAGE) != NULL)
            TEST_VERDICT("Garbage at the beginning");
        line_len = strlen(no_prefix) + 1;
        if (line_len < minlen)
            TEST_VERDICT("Line too short");
        if (line_len > maxlen)
            TEST_VERDICT("Line too long");

        for (; *no_prefix != '\0'; no_prefix++)
        {
            if (!isxdigit(*no_prefix))
                TEST_VERDICT("Illegal character: %x", *no_prefix);
        }
    }
    if (n < minrepeat)
        TEST_VERDICT("Too few lines");
}

int
main(int argc, char **argv)
{
    te_string rfile = TE_STRING_INIT;
    char *content = NULL;
    rcf_rpc_server *pco_iut = NULL;
    size_t minlen;
    size_t maxlen;
    size_t minrepeat;
    size_t maxrepeat;

    TEST_START;

    TEST_GET_PCO(pco_iut);
    TEST_GET_UINT_PARAM(minlen);
    TEST_GET_UINT_PARAM(maxlen);
    TEST_GET_UINT_PARAM(minrepeat);
    TEST_GET_UINT_PARAM(maxrepeat);

    TEST_STEP("Generate the file and put it onto TA");
    tapi_file_make_name(&rfile);
    make_chunks(pco_iut->ta, rfile.ptr, minlen, maxlen, minrepeat, maxrepeat);

    TEST_STEP("Verifying the file");
    CHECK_RC(tapi_file_read_ta(pco_iut->ta, rfile.ptr, &content));
    validate_chunks(content, minlen, maxlen, minrepeat, maxrepeat);

    TEST_SUCCESS;

cleanup:

    CLEANUP_CHECK_RC(tapi_file_ta_unlink_fmt(pco_iut->ta, "%s", rfile.ptr));

    free(content);
    te_string_free(&rfile);

    TEST_END;
}
