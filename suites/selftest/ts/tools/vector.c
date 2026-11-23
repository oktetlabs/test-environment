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
check_split(const char *input, bool empty_is_none,
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

    te_vec_free(&strvec);
}

static void
check_find_str(const te_vec *strvec, const char *str, bool exp_found,
               unsigned int exp_index)
{
    const char *name = str == NULL ? "(null)" : str;
    unsigned int index = UINT_MAX;
    bool found = te_vec_find_str(strvec, str, &index);

    if (found != exp_found)
    {
        TEST_VERDICT("'%s' is %s, but it should%s be", name,
                     found ? "found" : "not found", exp_found ? "" : " not");
    }
    if (found && index != exp_index)
    {
        TEST_VERDICT("'%s' is found at index %u instead of %u", name, index,
                     exp_index);
    }
    if (!found && index != UINT_MAX)
        TEST_VERDICT("The index is changed though '%s' is not found", name);
    if (te_vec_find_str(strvec, str, NULL) != exp_found)
        TEST_VERDICT("The result for '%s' differs without the index", name);
}

int
main(int argc, char **argv)
{
    te_vec strvec = TE_VEC_INIT_AUTOPTR(char *);

    TEST_START;

    TEST_STEP("Test splitting of an empty string");
    check_split(NULL, false, 0, NULL);
    check_split("", true, 0, NULL);

    TEST_STEP("Test splitting of non-empty string");
    check_split("a:b:c:d", false, 4, (const char *[]){"a", "b", "c", "d"});
    check_split("abcd", false, 1, (const char *[]){"abcd"});

    TEST_STEP("Test splitting string with empty chunks");
    check_split("a:b:c:", false, 4, (const char *[]){"a", "b", "c", ""});
    check_split(":a:b:c", false, 4, (const char *[]){"", "a", "b", "c"});
    check_split(":::", false, 4, (const char *[]){"", "", "", ""});
    check_split("a:b:c:", true, 4, (const char *[]){"a", "b", "c", ""});
    check_split(":a:b:c", true, 4, (const char *[]){"", "a", "b", "c"});
    check_split(":::", true, 4, (const char *[]){"", "", "", ""});

    TEST_STEP("Test splitting an empty string as a single chunk");
    check_split("", false, 1, (const char *[]){""});

    TEST_STEP("Test finding a string in an empty vector");
    check_find_str(&strvec, "a", false, 0);

    TEST_STEP("Test finding a string in a vector of strings");
    CHECK_RC(te_vec_split_string("a:bc::bc:d", &strvec, ':', true));
    check_find_str(&strvec, "a", true, 0);
    check_find_str(&strvec, "d", true, 4);

    TEST_STEP("Test that the first of equal strings is found");
    check_find_str(&strvec, "bc", true, 1);

    TEST_STEP("Test that a prefix and an absent string are not found");
    check_find_str(&strvec, "b", false, 0);
    check_find_str(&strvec, "bcd", false, 0);

    TEST_STEP("Test that an empty string and NULL are never found");
    check_find_str(&strvec, "", false, 0);
    check_find_str(&strvec, NULL, false, 0);

    TEST_SUCCESS;

cleanup:
    te_vec_free(&strvec);

    TEST_END;
}
