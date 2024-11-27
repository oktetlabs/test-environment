/** @file
 * @brief
 * Check distributed TRC.
 *
 * Copyright (C) 2025 OKTET Labs Ltd., St. Petersburg, Russia
 */

/** @page test_superset Trivial test for checking TRC.
 *
 * @objective Check distributed TRC when merged iterations are
 *            supersets for existing ones.
 *
 * @param a     The first argument.
 * @param b     The second argument.
 *
 * @par Scenario:
 *
 * @author Dmitry Izbitsky <dmitry.izbitsky@oktetlabs.ru>
 */

#define TE_TEST_NAME "distributed/test_superset"

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

    UNUSED(a);
    UNUSED(b);

    TEST_SUCCESS;

cleanup:

    TEST_END;
}
