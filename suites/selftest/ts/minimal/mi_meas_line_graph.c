/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Minimal test
 *
 * Minimal test scenario that yields a MI measurement artifact
 * which can be displayed as a graph in HTML log.
 *
 * Copyright (C) 2020-2022 OKTET Labs Ltd. All rights reserved.
 */

/** @page minimal_mi_meas_line_graph Test for line-graph views of MI measurement artifacts
 *
 * @objective Demo of using line-graph views with MI measurement artifacts
 *
 * @par Test sequence:
 *
 */

#ifndef DOXYGEN_TEST_SPEC

#define TE_TEST_NAME    "mi_meas_line_graph"

#include "te_config.h"

#include <math.h>

#include "tapi_test.h"
#include "te_mi_log.h"

/** Number of values added for a parameter */
#define VALUES_NUM 100

int
main(int argc, char **argv)
{
    te_mi_logger *logger;
    int i;

    TEST_START;

    TEST_STEP("Create a MI logger.");
    CHECK_RC(te_mi_logger_meas_create("Some application", &logger));

    TEST_STEP("Add values for four measured parameters:");
    TEST_SUBSTEP("For each measured parameter specify a different type "
                 "and use a different function of sequence number "
                 "to compute its value.");
    TEST_SUBSTEP("Let the first parameter have empty name to check usage "
                 "of type name for parameter identification.");
    TEST_SUBSTEP("Add less values for the third parameter to check that "
                 "warning is printed in HTML log when graph axes have "
                 "different numbers of values.");
    for (i = 0; i < VALUES_NUM; i++)
    {
        te_mi_logger_add_meas(logger, NULL, TE_MI_MEAS_TEMP, NULL,
                              TE_MI_MEAS_AGGR_SINGLE, (i * i),
                              TE_MI_MEAS_MULTIPLIER_MILLI);

        te_mi_logger_add_meas(logger, NULL, TE_MI_MEAS_PPS, "B-parameter",
                              TE_MI_MEAS_AGGR_SINGLE,
                              2 * i * sin(i / 10.0),
                              TE_MI_MEAS_MULTIPLIER_MILLI);

        /*
         * Add less values for the last parameter to check how
         * warnings are printed in HTML log.
         */
        if (i < VALUES_NUM - 1)
        {
            te_mi_logger_add_meas(logger, NULL, TE_MI_MEAS_LATENCY,
                                  "C-parameter",
                                  TE_MI_MEAS_AGGR_SINGLE, i,
                                  TE_MI_MEAS_MULTIPLIER_MILLI);
        }

        te_mi_logger_add_meas(logger, NULL, TE_MI_MEAS_PPS, "D-parameter",
                              TE_MI_MEAS_AGGR_SINGLE,
                              1.5 * i * cos(i / 8.0),
                              TE_MI_MEAS_MULTIPLIER_MILLI);
    }

    TEST_STEP("Add the first line-graph view which assigns the first "
              "parameter to axis X and does not specify axis Y, so "
              "that all the rest parameters are assigned to it by "
              "default.");
    te_mi_logger_add_meas_view(logger, NULL, TE_MI_MEAS_VIEW_LINE_GRAPH,
                               "graph1",
                               "How B, C and D depend on temperature");
    te_mi_logger_meas_graph_axis_add_type(logger, NULL,
                                          TE_MI_MEAS_VIEW_LINE_GRAPH, "graph1",
                                          TE_MI_GRAPH_AXIS_X,
                                          TE_MI_MEAS_TEMP);

    TEST_STEP("Add the second line-graph view which will show how "
              "the first parameter depends on the second parameter.");
    te_mi_logger_add_meas_view(logger, NULL, TE_MI_MEAS_VIEW_LINE_GRAPH,
                               "graph2", "How temperature depends on B");
    te_mi_logger_meas_graph_axis_add(logger, NULL,
                                     TE_MI_MEAS_VIEW_LINE_GRAPH, "graph2",
                                     TE_MI_GRAPH_AXIS_X,
                                     TE_MI_MEAS_PPS, "B-parameter");
    te_mi_logger_meas_graph_axis_add_type(
                                      logger, NULL,
                                      TE_MI_MEAS_VIEW_LINE_GRAPH, "graph2",
                                      TE_MI_GRAPH_AXIS_Y,
                                      TE_MI_MEAS_TEMP);


    TEST_STEP("Add the third line-graph view which will show how "
              "the second and the fourth parameters depend on the third "
              "one.");
    te_mi_logger_add_meas_view(logger, NULL, TE_MI_MEAS_VIEW_LINE_GRAPH,
                               "graph3", "How B and D depend on C");

    te_mi_logger_meas_graph_axis_add_name(
                                      logger, NULL,
                                      TE_MI_MEAS_VIEW_LINE_GRAPH, "graph3",
                                      TE_MI_GRAPH_AXIS_X, "C-parameter");

    te_mi_logger_meas_graph_axis_add(logger, NULL,
                                     TE_MI_MEAS_VIEW_LINE_GRAPH, "graph3",
                                     TE_MI_GRAPH_AXIS_Y,
                                     TE_MI_MEAS_PPS, "B-parameter");
    te_mi_logger_meas_graph_axis_add(logger, NULL,
                                     TE_MI_MEAS_VIEW_LINE_GRAPH, "graph3",
                                     TE_MI_GRAPH_AXIS_Y,
                                     TE_MI_MEAS_PPS, "D-parameter");

    TEST_STEP("Add the fourth line-graph view which will show how value "
              "of the third parameter depends on its sequence number.");
    te_mi_logger_add_meas_view(logger, NULL, TE_MI_MEAS_VIEW_LINE_GRAPH,
                               "graph4", "Values of C");
    te_mi_logger_meas_graph_axis_add_name(
                                      logger, NULL,
                                      TE_MI_MEAS_VIEW_LINE_GRAPH, "graph4",
                                      TE_MI_GRAPH_AXIS_X,
                                      TE_MI_GRAPH_AUTO_SEQNO);
    te_mi_logger_meas_graph_axis_add_name(
                                      logger, NULL,
                                      TE_MI_MEAS_VIEW_LINE_GRAPH, "graph4",
                                      TE_MI_GRAPH_AXIS_Y, "C-parameter");

    TEST_STEP("Call te_mi_logger_destroy() to log MI measurement artifact "
              "together with views and release resources.");
    te_mi_logger_destroy(logger);

    TEST_SUCCESS;

cleanup:

    TEST_END;
}

#endif /* !DOXYGEN_TEST_SPEC */
