/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2026 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief List of the named fields of compound arguments.
 *
 * Check that Tester passes a @c COMP.arg=fields entry for every
 * compound argument with named fields and for no other argument.
 * The prefix is spelled out rather than taken from te_param.h, so
 * that the test pins the format scripts rely on.
 */

/** @page minimal_compound_list Compound argument list
 *
 * @objective Test that Tester names the compound arguments and their
 *            fields.
 *
 * @par Test sequence:
 *
 */

/** Logging subsystem entity name */
#define TE_TEST_NAME    "minimal/compound_list"

#include "te_config.h"
#include "tapi_test.h"

/**
 * Count the entries of @p argv that start with @p prefix.
 *
 * @param argc      Number of entries in @p argv.
 * @param argv      Command line arguments.
 * @param prefix    Prefix to look for.
 * @param last      Location for the last matching entry
 *                  (may be @c NULL).
 *
 * @return Number of matching entries.
 */
static unsigned int
count_prefixed(int argc, char **argv, const char *prefix, const char **last)
{
    unsigned int count = 0;
    int i;

    for (i = 0; i < argc; i++)
    {
        if (strncmp(argv[i], prefix, strlen(prefix)) == 0)
        {
            count++;
            if (last != NULL)
                *last = argv[i];
        }
    }

    return count;
}

/**
 * Find the first entry of @p argv that starts with @p prefix.
 *
 * @param argc      Number of entries in @p argv.
 * @param argv      Command line arguments.
 * @param prefix    Prefix to look for.
 *
 * @return Index of the entry or @c -1 if there is none.
 */
static int
find_prefixed(int argc, char **argv, const char *prefix)
{
    int i;

    for (i = 0; i < argc; i++)
    {
        if (strncmp(argv[i], prefix, strlen(prefix)) == 0)
            return i;
    }

    return -1;
}

int
main(int argc, char **argv)
{
    const char *link_mtu;
    const char *expect_mtu;
    const char *link_entry = NULL;
    unsigned int count;
    int seed_idx;
    int comp_idx;
    int field_idx;

    TEST_START;

    TEST_STEP("Getting parameters");

    TEST_GET_STRING_PARAM(link_mtu);
    TEST_GET_STRING_PARAM(expect_mtu);

    TEST_STEP("Checking that the compound argument link is listed");

    count = count_prefixed(argc, argv, "COMP.link=", &link_entry);
    if (count != 1)
        TEST_FAIL("COMP.link= is passed %u times, expected once", count);

    if (strcmp(link_entry, "COMP.link=mtu,speed") != 0)
    {
        TEST_FAIL("'%s' is passed, expected 'COMP.link=mtu,speed'",
                  link_entry);
    }

    TEST_STEP("Checking that the list follows te_rand_seed and precedes the fields");

    seed_idx = find_prefixed(argc, argv, "te_rand_seed=");
    comp_idx = find_prefixed(argc, argv, "COMP.link=");
    field_idx = find_prefixed(argc, argv, "link_");
    if (seed_idx < 0 || field_idx < 0)
    {
        TEST_FAIL("te_rand_seed= is argv[%d] and the first link_ entry is argv[%d], expected both",
                  seed_idx, field_idx);
    }

    if (comp_idx <= seed_idx || comp_idx >= field_idx)
    {
        TEST_FAIL("COMP.link= is argv[%d], expected between te_rand_seed= (argv[%d]) and the first link_ entry (argv[%d])",
                  comp_idx, seed_idx, field_idx);
    }

    TEST_STEP("Checking that an array and a plain argument are not listed");

    count = count_prefixed(argc, argv, "COMP.sizes=", NULL);
    if (count != 0)
        TEST_FAIL("COMP.sizes= is passed %u times, expected none", count);

    count = count_prefixed(argc, argv, "COMP.plain=", NULL);
    if (count != 0)
        TEST_FAIL("COMP.plain= is passed %u times, expected none", count);

    TEST_STEP("Checking that the overriding value is flattened");

    if (strcmp(link_mtu, expect_mtu) != 0)
        TEST_FAIL("link_mtu is '%s', expected '%s'", link_mtu, expect_mtu);

    TEST_SUCCESS;

cleanup:
    TEST_END;
}
