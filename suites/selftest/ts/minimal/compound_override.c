/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2026 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief Plain argument overriding a field of a compound argument.
 *
 * Check that a plain argument named @c stem_field replaces the field
 * @c field of the compound argument @c stem.
 */

/** @page minimal_compound_override Plain argument overriding a field
 *
 * @objective Test that a plain argument `link_mtu` overrides the field
 *            `mtu` of the compound argument `link` and is seen once.
 *
 * @par Test sequence:
 *
 */

/** Logging subsystem entity name */
#define TE_TEST_NAME    "minimal/compound_override"

#include "te_config.h"
#include "tapi_test.h"

int
main(int argc, char **argv)
{
    const char *link_speed;
    const char *link_mtu;
    const char *expect_mtu;
    const char prefix[] = "link_mtu=";
    unsigned int count = 0;
    int i;

    TEST_START;

    TEST_STEP("Getting parameters");

    TEST_GET_STRING_PARAM(link_speed);
    TEST_GET_STRING_PARAM(link_mtu);
    TEST_GET_STRING_PARAM(expect_mtu);

    RING("link_speed = %s, link_mtu = %s, expect_mtu = %s",
         link_speed, link_mtu, expect_mtu);

    if (strcmp(link_speed, "25G") != 0)
        TEST_FAIL("link_speed is '%s', expected '25G'", link_speed);

    if (strcmp(link_mtu, expect_mtu) != 0)
        TEST_FAIL("link_mtu is '%s', expected '%s'", link_mtu, expect_mtu);

    for (i = 0; i < argc; i++)
    {
        if (strncmp(argv[i], prefix, strlen(prefix)) == 0)
            count++;
    }

    if (count != 1)
        TEST_FAIL("link_mtu is passed %u times, expected once", count);

    TEST_SUCCESS;

cleanup:
    TEST_END;
}
