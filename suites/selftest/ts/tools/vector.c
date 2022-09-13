/** @file
 * @brief Test for te_vector.h functions
 *
 * Testing vector manipulating routines
 *
 * Copyright (C) 2022 OKTET Labs. All rights reserved.
 */

/** @page tools_vector te_vector.h test
 *
 * @objective Testing vector manipulating routines
 *
 * Test the correctness of dynamic vector implementation.
 *
 * @par Test sequence:
 */

/** Logging subsystem entity name */
#define TE_TEST_NAME    "tools/vector"

#include "te_config.h"

#include "tapi_test.h"
#include "te_vector.h"

static void
check_split(const char *input, te_bool empty_is_none,
            unsigned int n_chunks, const char **chunks)
{
    te_vec strvec = TE_VEC_INIT(char *);
    unsigned i;

    CHECK_RC(te_vec_split_string(input, &strvec, ':', empty_is_none));
    if (te_vec_size(&strvec) != n_chunks)
    {
        TEST_VERDICT("'%s' split into %zu chunks, expected %u",
                     input == NULL ? "NULL" : input,
                     te_vec_size(&strvec), n_chunks);
    }

    for (i = 0; i < n_chunks; i++)
    {
        const char *ptr = TE_VEC_GET(const char *, &strvec, i);

        CHECK_NOT_NULL(ptr);
        if (strcmp(chunks[i], ptr) != 0)
        {
            TEST_VERDICT("%u'th chunk of '%s' is '%s', but expected '%s'",
                         i, input, ptr, chunks[i]);
        }
    }

    /* Ensure that the chunks are appended */
    CHECK_RC(te_vec_split_string(input, &strvec, ':', empty_is_none));
    if (te_vec_size(&strvec) != n_chunks * 2)
    {
        TEST_VERDICT("Second split did not preserve contents: "
                     "%u chunks expected, %u observed",
                     n_chunks * 2, te_vec_size(&strvec));
    }

    for (i = 0; i < n_chunks; i++)
    {
        const char *ptr = TE_VEC_GET(const char *, &strvec, i);

        CHECK_NOT_NULL(ptr);
        if (strcmp(chunks[i], ptr) != 0)
        {
            TEST_VERDICT("Existing %u'th chunk changed: "
                         "got '%s', but expected '%s'",
                         i, ptr, chunks[i]);
        }

        for (i = 0; i < n_chunks; i++)
        {
            const char *ptr = TE_VEC_GET(const char *, &strvec, i + n_chunks);

            CHECK_NOT_NULL(ptr);
            if (strcmp(chunks[i], ptr) != 0)
            {
                TEST_VERDICT("New %u'th chunk of '%s' is '%s', "
                             "but expected '%s'",
                             i, input, ptr, chunks[i]);
            }
        }
    }

    te_vec_deep_free(&strvec);
}

int
main(int argc, char **argv)
{
    TEST_START;

    TEST_STEP("Test splitting of an empty string");
    check_split(NULL, FALSE, 0, NULL);
    check_split("", TRUE, 0, NULL);

    TEST_STEP("Test splitting of non-empty string");
    check_split("a:b:c:d", FALSE, 4, (const char *[]){"a", "b", "c", "d"});
    check_split("abcd", FALSE, 1, (const char *[]){"abcd"});

    TEST_STEP("Test splitting string with empty chunks");
    check_split("a:b:c:", FALSE, 4, (const char *[]){"a", "b", "c", ""});
    check_split(":a:b:c", FALSE, 4, (const char *[]){"", "a", "b", "c"});
    check_split(":::", FALSE, 4, (const char *[]){"", "", "", ""});
    check_split("a:b:c:", TRUE, 4, (const char *[]){"a", "b", "c", ""});
    check_split(":a:b:c", TRUE, 4, (const char *[]){"", "a", "b", "c"});
    check_split(":::", TRUE, 4, (const char *[]){"", "", "", ""});

    TEST_STEP("Test splitting an empty string as a single chunk");
    check_split("", FALSE, 1, (const char *[]){""});

    TEST_SUCCESS;

cleanup:

    TEST_END;
}
