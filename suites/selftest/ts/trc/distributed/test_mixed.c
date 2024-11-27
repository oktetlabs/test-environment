/** @file
 * @brief
 * Check distributed TRC.
 *
 * Copyright (C) 2025 OKTET Labs Ltd., St. Petersburg, Russia
 */

/** @page test_mixed Trivial test for checking TRC.
 *
 * @objective Check distributed TRC with different kinds of iteration
 *            matches.
 *
 * @param a     The first argument.
 * @param b     The second argument.
 * @param c     The third argument.
 *
 * @par Scenario:
 *
 * @author Dmitry Izbitsky <dmitry.izbitsky@oktetlabs.ru>
 */

#define TE_TEST_NAME "distributed/test_mixed"

#include "tapi_test.h"
#include "tapi_env.h"

int
main(int argc, char **argv)
{
    int a;
    int b;
    int c;

    TEST_START;
    TEST_GET_INT_PARAM(a);
    TEST_GET_INT_PARAM(b);
    TEST_GET_INT_PARAM(c);

    if (b == 1)
    {
        RING_VERDICT("b is equal to 1");
    }
    else if (a == 1)
    {
        if (b == 2)
            RING_VERDICT("a is equal to 1, b is equal to 2");
        else
            RING_VERDICT("a is equal to 1");
    }
    else if (a == 2 && c == 2)
    {
        RING_VERDICT("a and c are equal to 2");
    }
    else if (a == 2 && b == 2)
    {
        RING_VERDICT("a and b are equal to 2");
    }
    else if (a == 3 && b == 3 && c == 3)
    {
        RING_VERDICT("All parameters are equal to 3");
    }
    else if (a == 4 && b == 4 && c == 4)
    {
        RING_VERDICT("All parameters are equal to 4");
    }

    TEST_SUCCESS;

cleanup:

    TEST_END;
}
