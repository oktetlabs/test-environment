/** @file
 * @brief Test for te_units.h functions
 *
 * Testing unit conversions
 *
 * Copyright (C) 2022-2022 OKTET Labs. All rights reserved.
 */

/** @page tools_units te_units.h test
 *
 * @objective Testing unit conversion correctness
 *
 * Do some sanity checks that units are being correctly
 * converted
 *
 * @par Test sequence:
 *
 * @author Artem Andreev <Artem.Andreev@oktetlabs.ru>
 */

/** Logging subsystem entity name */
#define TE_TEST_NAME    "units"

#include "te_config.h"

#include <float.h>

#include <string.h>
#ifdef HAVE_MATH_H
#include <math.h>
#endif

#include "tapi_test.h"
#include "te_units.h"

typedef struct expectation {
    const char *input;
    double value;
} expectation;

static void
check_expectations(const expectation *exps, const te_unit_list *units)
{
    unsigned i;

    for (i = 0; exps[i].input != NULL; i++)
    {
        double actual;

        CHECK_RC(te_unit_list_value_from_string(exps[i].input,
                                                units,
                                                &actual));
        if (fabs(actual - exps[i].value) > DBL_EPSILON)
        {
            TEST_VERDICT("Conversion mismatch for '%s': expected %g, got %g",
                         exps[i].input, exps[i].value, actual);
        }
    }
}

int
main(int argc, char **argv)
{
    const te_unit_list lengths = {
        .scale = 1000,
        .start_pow = -3,
        .units = (const char *[]){
            "nm", "μm", "mm", "m", "km", NULL
        }
    };
    const expectation length_expectations[] = {
        {"5nm", 5e-9},
        {"10μm", 10e-6},
        {"15mm", 15e-3},
        {"20m", 20.0},
        {"30km", 30000.0},
        {NULL, 0.0}
    };

    const te_unit_list times = {
        .scale = 1,
        .start_pow = 0,
        .non_uniform_scale = (double[]){
            1e-9, 1e-3, 1.0, 60.0, 3600.0, 86400.0
        },
        .units = (const char *[]){
            "ns", "ms", "s", "m", "h", "d", NULL
        }
    };
    const expectation time_expectations[] = {
        {"5ns", 5e-9},
        {"15ms", 15e-3},
        {"20s", 20.0},
        {"30m", 1800},
        {"40h", 144000.0},
        {"50d", 4320000.0},
        {NULL, 0.0}
    };
    te_errno bad_rc;
    double tmp;

    TEST_START;

    TEST_STEP("Checking uniform scaling");
    check_expectations(length_expectations, &lengths);

    TEST_STEP("Checking non-uniform scaling");
    check_expectations(time_expectations, &times);

    TEST_STEP("Mixing uniform and non-uniform scales is disallowed");
    bad_rc = te_unit_list_value_from_string("1m",
                                            &(te_unit_list){
                                                .scale = 1000,
                                                .start_pow = -3,
                                                .non_uniform_scale = (double[]){
                                                    1.0, 1.0, 1.0, 60.0,
                                                    3600.0, 86400.0
                                                },
                                                .units = (const char *[]){
                                                    "ns", "ms", "s",
                                                    "m", "h", "d", NULL
                                                }
                                            }, &tmp);
    if (TE_RC_GET_ERROR(bad_rc) != TE_EINVAL)
        TEST_VERDICT("Unexpected error: %r", bad_rc);

    TEST_SUCCESS;

cleanup:
    TEST_END;
}
