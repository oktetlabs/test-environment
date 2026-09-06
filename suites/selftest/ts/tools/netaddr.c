/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Test for network address string conversions
 *
 * Testing te_netaddr_from_te_str() and te_netaddr2te_str()
 *
 * Copyright (C) 2026 OKTET Ltd. All rights reserved.
 */

/** @page tools_netaddr Network address string conversion test
 *
 * @objective Testing te_netaddr_from_te_str() correctness
 *
 * Parse every textual form Configurator uses for an address value
 * and check that printing it back yields the original string, then
 * check that malformed input is rejected.
 *
 * @par Test sequence:
 *
 */

/** Logging subsystem entity name */
#define TE_TEST_NAME    "netaddr"

#include "te_config.h"

#include <sys/socket.h>

#include "tapi_test.h"
#include "te_sockaddr.h"
#include "te_string.h"

/** An address that must survive a parse-print round trip */
typedef struct roundtrip_case {
    const char *str; /**< Textual form */
    int af;          /**< Address family it denotes */
} roundtrip_case;

static const roundtrip_case roundtrips[] = {
    { "192.0.2.1", AF_INET },
    { "0.0.0.0", AF_INET },
    { "255.255.255.255", AF_INET },
    { "2001:db8::1", AF_INET6 },
    { "::1", AF_INET6 },
    { "aa:bb:cc:dd:ee:ff", AF_LOCAL },
    { "00:00:00:00:00:00", AF_LOCAL },
    { "", AF_UNSPEC },
};

static const char *const invalid[] = {
    "192.0.2.256",
    "1.2.3.4.5",
    "aa:bb:cc:dd:ee",
    "aa:bb:cc:dd:ee:ff:00",
    "2001:db8::1::2",
    "not an address",
};

int
main(int argc, char **argv)
{
    unsigned int i;

    TEST_START;

    TEST_STEP("Round-trip every supported address form");
    for (i = 0; i < TE_ARRAY_LEN(roundtrips); i++)
    {
        const roundtrip_case *c = &roundtrips[i];
        struct sockaddr_storage addr;
        te_string str = TE_STRING_INIT;

        CHECK_RC(te_netaddr_from_te_str(c->str, &addr));

        if (addr.ss_family != c->af)
        {
            TEST_VERDICT("'%s' parsed as family %d, expected %d",
                         c->str, (int)addr.ss_family, c->af);
        }

        CHECK_RC(te_netaddr2te_str(SA(&addr), &str));
        if (strcmp(str.ptr == NULL ? "" : str.ptr, c->str) != 0)
        {
            TEST_VERDICT("'%s' printed back as '%s'", c->str,
                         str.ptr == NULL ? "" : str.ptr);
        }
        te_string_free(&str);
    }

    TEST_STEP("Reject malformed addresses");
    for (i = 0; i < TE_ARRAY_LEN(invalid); i++)
    {
        struct sockaddr_storage addr;

        if (te_netaddr_from_te_str(invalid[i], &addr) == 0)
            TEST_VERDICT("'%s' was accepted", invalid[i]);
    }

    TEST_STEP("Reject NULL arguments");
    {
        struct sockaddr_storage addr;

        if (te_netaddr_from_te_str(NULL, &addr) == 0)
            TEST_VERDICT("A NULL string was accepted");
        if (te_netaddr_from_te_str("192.0.2.1", NULL) == 0)
            TEST_VERDICT("A NULL address was accepted");
    }

    TEST_SUCCESS;

cleanup:

    TEST_END;
}
