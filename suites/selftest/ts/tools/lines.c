/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2023 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief Test for te_string_process_line() function
 *
 * Testing line processing routine.
 */

/** @page tools_lines te_string_process_line() test
 *
 * @objective Testing line processing function.
 *
 * Test the correctness of line processing function.
 *
 * @par Test sequence:
 */

/** Logging subsystem entity name */
#define TE_TEST_NAME    "tools/lines"

#include "te_config.h"

#include "tapi_test.h"
#include "te_string.h"
#include "te_str.h"

static te_errno
count_upward(char *line, void *data)
{
    unsigned int *counter = data;

    UNUSED(line);
    (*counter)++;

    return 0;
}

static te_errno
count_downward(char *line, void *data)
{
    unsigned int *counter = data;

    UNUSED(line);
    (*counter)--;

    return *counter == 0 ? TE_EOK : 0;
}

static te_errno
count_empty(char *line, void *data)
{
    unsigned int *counter = data;

    if (*line != '\0')
    {
        ERROR("Not empty line: %s", line);
        return TE_EINVAL;
    }

    (*counter)++;

    return 0;
}

static void
build_buffer(unsigned int max_lines, unsigned int max_line_size, bool crlf,
             te_string *buffer, char **last_line, unsigned int *n_lines)
{
    size_t actual_len;
    unsigned int i;

    *n_lines = rand_range(1, max_lines);

    for (i = 0; i < *n_lines; i++)
    {
        char *buf = te_make_printable_buf(1, max_line_size + 1, &actual_len);

        te_string_append_buf(buffer, buf, actual_len - 1);
        free(buf);

        if (crlf)
            te_string_append(buffer, "\r");
        te_string_append(buffer, "\n");
    }

    *last_line = te_make_printable_buf(1, max_line_size + 1, &actual_len);
    te_string_append_buf(buffer, *last_line, actual_len - 1);
}

int
main(int argc, char **argv)
{
    unsigned int n_iterations;
    unsigned int max_lines;
    unsigned int max_line_size;
    bool crlf;
    unsigned int i;
    char *last_line = NULL;
    unsigned int n_lines;
    unsigned int down_counter;
    unsigned int up_counter;

    TEST_START;
    TEST_GET_UINT_PARAM(n_iterations);
    TEST_GET_UINT_PARAM(max_lines);
    TEST_GET_UINT_PARAM(max_line_size);
    TEST_GET_BOOL_PARAM(crlf);

    TEST_STEP("Testing random buffers");
    for (i = 0; i < n_iterations; i++)
    {
        te_string str = TE_STRING_INIT;

        build_buffer(max_lines, max_line_size, crlf, &str,
                     &last_line, &n_lines);

        up_counter = down_counter = rand_range(1, n_lines);
        CHECK_RC(te_string_process_lines(&str, true, count_downward,
                                         &down_counter));

        if (down_counter != 0)
        {
            TEST_VERDICT("Line processing stopped early: %u lines unprocessed",
                         down_counter);
        }
        CHECK_RC(te_string_process_lines(&str, true, count_upward,
                                         &up_counter));
        if (up_counter != n_lines)
        {
            ERROR("%u lines should be processed, but actually only %u",
                  n_lines, up_counter);
            TEST_VERDICT("Unexpected number of lines processed");
        }
        if (strcmp(str.ptr, last_line) != 0)
        {
            ERROR("Unexpected line trail: '%s' instead of '%s'",
                  str.ptr, last_line);
            TEST_VERDICT("Unexpected line trail");
        }

        if (*last_line != '\0')
        {
            CHECK_RC(te_string_process_lines(&str, false, count_upward,
                                             &up_counter));
            if (up_counter != n_lines + 1)
                TEST_VERDICT("Trailing line unprocessed");

            if (str.len != 0)
            {
                ERROR("Remaining trail: '%s'", str.ptr);
                TEST_VERDICT("Unexpected line trail");
            }
        }

        free(last_line);
        te_string_free(&str);
    }

    TEST_STEP("Testing empty buffer");
    up_counter = 0;
    CHECK_RC(te_string_process_lines(&(te_string)TE_STRING_INIT, false,
                                     count_upward, &up_counter));
    if (up_counter != 0)
        TEST_VERDICT("Callback called on an empty buffer");

    TEST_STEP("Testing empty lines");
    for (i = 0; i < n_iterations; i++)
    {
        te_string str = TE_STRING_INIT;

        build_buffer(max_lines, 0, crlf, &str, &last_line, &n_lines);
        assert(*last_line == '\0');

        up_counter = 0;
        CHECK_RC(te_string_process_lines(&str, true, count_empty,
                                         &up_counter));
        if (up_counter != n_lines)
        {
            ERROR("Expected %u lines, actually %u lines processed",
                  n_lines, up_counter);
            TEST_VERDICT("Unexpected number of empty lines processed");
        }

        if (str.len > 0)
            TEST_VERDICT("Non-empty trail of empty lines (%zu chars)", str.len);

        free(last_line);
        te_string_free(&str);
    }

    TEST_SUCCESS;

cleanup:

    TEST_END;
}
