/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2024 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief Plotting a graph with different multiplier values.
 *
 * The test causes a graph to be plotted with all known measurement
 * multiplier values.
 */
/** @page minimal_mi_meas_multipliers Test for MI measurement multipliers.
 *
 * @objective Demo of using all possible measurement multipliers.
 *
 * @par Test sequence:
 *
 */

#ifndef DOXYGEN_TEST_SPEC

#define TE_TEST_NAME    "mi_meas_multipliers"

#include "te_config.h"

#include "tapi_test.h"
#include "te_str.h"
#include "te_mi_log.h"

int
main(int argc, char **argv)
{
    static const char *multi_names[] = {
        [TE_MI_MEAS_MULTIPLIER_NANO] = "Nano",
        [TE_MI_MEAS_MULTIPLIER_MICRO] = "Micro",
        [TE_MI_MEAS_MULTIPLIER_MILLI] = "Milli",
        [TE_MI_MEAS_MULTIPLIER_PLAIN] = "Plain",
        [TE_MI_MEAS_MULTIPLIER_KILO] = "Kilo",
        [TE_MI_MEAS_MULTIPLIER_KIBI] = "Kibi",
        [TE_MI_MEAS_MULTIPLIER_MEGA] = "Mega",
        [TE_MI_MEAS_MULTIPLIER_MEBI] = "Mebi",
        [TE_MI_MEAS_MULTIPLIER_GIGA] = "Giga",
        [TE_MI_MEAS_MULTIPLIER_GIBI] = "Gibi",
    };
    te_mi_logger *logger;
    unsigned int n_values = 0;
    te_mi_meas_multiplier multi;

    TEST_START;
    TEST_GET_UINT_PARAM(n_values);

    TEST_STEP("Create a MI logger.");
    CHECK_RC(te_mi_logger_meas_create("High range", &logger));

    TEST_STEP("Add measurements");
    for (multi = TE_MI_MEAS_MULTIPLIER_NANO;
         multi < TE_MI_MEAS_MULTIPLIER_END;
         multi++)
    {
        unsigned int i;
        for (i = 0; i < n_values; i++)
        {
            te_mi_logger_add_meas(logger, NULL, TE_MI_MEAS_PPS,
                                  multi_names[multi],
                                  TE_MI_MEAS_AGGR_SINGLE,
                                  (double)i, multi);
        }
    }

    TEST_STEP("Add a line-graph view to show different multipliers");
    te_mi_logger_add_meas_view(logger, NULL, TE_MI_MEAS_VIEW_LINE_GRAPH,
                               "graph1", "Kinds of multipliers");
    te_mi_logger_meas_graph_axis_add_name(
                                      logger, NULL,
                                      TE_MI_MEAS_VIEW_LINE_GRAPH, "graph1",
                                      TE_MI_GRAPH_AXIS_X,
                                      TE_MI_GRAPH_AUTO_SEQNO);
    for (multi = TE_MI_MEAS_MULTIPLIER_NANO;
         multi < TE_MI_MEAS_MULTIPLIER_END;
         multi++)
    {
        te_mi_logger_meas_graph_axis_add_name(logger, NULL,
                                              TE_MI_MEAS_VIEW_LINE_GRAPH,
                                              "graph1",
                                              TE_MI_GRAPH_AXIS_Y,
                                              multi_names[multi]);
    }

    TEST_STEP("Log MI measurement artifact");
    te_mi_logger_destroy(logger);

    TEST_SUCCESS;

cleanup:

    TEST_END;
}

#endif /* !DOXYGEN_TEST_SPEC */
