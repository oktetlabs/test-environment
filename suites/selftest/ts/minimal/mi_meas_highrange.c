/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2024 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief Plotting a graph with high-range values.
 *
 * The test causes a graph to be plotted where both small- and large-scale
 * measurements are recorded.
 */
/** @page minimal_mi_meas_highrange Test for representing high-range measurements.
 *
 * @objective Demo of using line-graph views with high-range measurements.
 *
 * @par Test sequence:
 *
 */

#ifndef DOXYGEN_TEST_SPEC

#define TE_TEST_NAME    "mi_meas_highrange"

#include "te_config.h"

#include <math.h>

#include "tapi_test.h"
#include "te_mi_log.h"

int
main(int argc, char **argv)
{
    te_mi_logger *logger;
    unsigned int n_values = 0;
    unsigned int i;

    TEST_START;
    TEST_GET_UINT_PARAM(n_values);

    TEST_STEP("Create a MI logger.");
    CHECK_RC(te_mi_logger_meas_create("High range", &logger));

    TEST_STEP("Add measurements");
    for (i = 0; i < n_values; i++)
    {
        double v = exp(i - n_values / 2);
        te_mi_logger_add_meas(logger, NULL, TE_MI_MEAS_PPS, "Positive",
                              TE_MI_MEAS_AGGR_SINGLE,
                              v, TE_MI_MEAS_MULTIPLIER_PLAIN);

        te_mi_logger_add_meas(logger, NULL, TE_MI_MEAS_PPS, "Negative",
                              TE_MI_MEAS_AGGR_SINGLE,
                              -v, TE_MI_MEAS_MULTIPLIER_PLAIN);
    }

    TEST_STEP("Add a line-graph view to show high-range values");
    te_mi_logger_add_meas_view(logger, NULL, TE_MI_MEAS_VIEW_LINE_GRAPH,
                               "graph1", "High-range values");
    te_mi_logger_meas_graph_axis_add_name(
                                      logger, NULL,
                                      TE_MI_MEAS_VIEW_LINE_GRAPH, "graph1",
                                      TE_MI_GRAPH_AXIS_X,
                                      TE_MI_GRAPH_AUTO_SEQNO);
    te_mi_logger_meas_graph_axis_add_name(
                                      logger, NULL,
                                      TE_MI_MEAS_VIEW_LINE_GRAPH, "graph1",
                                      TE_MI_GRAPH_AXIS_Y, "Positive");
    te_mi_logger_meas_graph_axis_add_name(
                                      logger, NULL,
                                      TE_MI_MEAS_VIEW_LINE_GRAPH, "graph1",
                                      TE_MI_GRAPH_AXIS_Y, "Negative");

    TEST_STEP("Log MI measurement artifact");
    te_mi_logger_destroy(logger);

    TEST_SUCCESS;

cleanup:

    TEST_END;
}

#endif /* !DOXYGEN_TEST_SPEC */
