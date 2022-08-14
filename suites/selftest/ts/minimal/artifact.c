/** @file
 * @brief Minimal test
 *
 * Minial test scenario that yields an artifact.
 *
 * Copyright (C) 2020-2022 OKTET Labs Ltd. All rights reserved.
 */

/** @page minimal_artifact Artifact test
 *
 * @objective Demo of artifact registering
 *
 * @par Test sequence:
 *
 */

#ifndef DOXYGEN_TEST_SPEC

/** Logging subsystem entity name */
#define TE_TEST_NAME    "artifact"

#include "te_config.h"
#include "tapi_test.h"

#include "te_mi_log.h"

int
main(int argc, char **argv)
{
    TEST_START;

    TEST_STEP("Print RING artifact");
    RING_ARTIFACT("RING artifact");

    TEST_STEP("Print MI artifact");
    te_mi_log_meas("mytool",
                 TE_MI_MEAS_V(TE_MI_MEAS(PPS, "pps", MIN, 42.4, PLAIN),
                              TE_MI_MEAS(PPS, "pps", MEAN, 300, PLAIN),
                              TE_MI_MEAS(PPS, "pps", CV, 10, PLAIN),
                              TE_MI_MEAS(PPS, "pps", MAX, 5.4, KILO),
                              TE_MI_MEAS(TEMP, "mem", SINGLE, 42.4, PLAIN),
                              TE_MI_MEAS(LATENCY, "low", MEAN, 54, MICRO)),
                 TE_MI_MEAS_KEYS({"key", "value"}),
                 TE_MI_COMMENTS({"comment", "comment_value"}));

    TEST_SUCCESS;

cleanup:

    TEST_END;
}

#endif /* !DOXYGEN_TEST_SPEC */
