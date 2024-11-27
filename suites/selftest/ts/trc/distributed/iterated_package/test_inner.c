/** @file
 * @brief
 * Check distributed TRC.
 *
 * Copyright (C) 2025 OKTET Labs Ltd., St. Petersburg, Russia
 */

/** @page test_inner Trivial test for checking TRC.
 *
 * @objective Check distributed TRC when test is inside iterated package.
 *
 * @param a     The first argument.
 * @param b     The second argument.
 *
 * @par Scenario:
 *
 * @author Dmitry Izbitsky <dmitry.izbitsky@oktetlabs.ru>
 */

#define TE_TEST_NAME "distributed/iterated_package/test_inner"

#include "tapi_test.h"
#include "tapi_env.h"

int
main(int argc, char **argv)
{
    int a;
    int b;

    TEST_START;
    TEST_GET_INT_PARAM(a);
    TEST_GET_INT_PARAM(b);

    if (b == 2)
        RING_VERDICT("b is equal to 2");
    else
        RING_VERDICT("a is equal to %d", a);

    TEST_SUCCESS;

cleanup:

    TEST_END;
}
