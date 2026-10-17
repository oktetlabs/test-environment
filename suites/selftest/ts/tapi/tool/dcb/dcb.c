/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2026 Advanced Micro Devices, Inc. All rights reserved. */
/** @file
 * @brief TAPI DCB test
 *
 * Test the usage of DCB TAPI.
 */

/** @page tapi-tool-dcb Check the dcb tool TAPI
 *
 * @objective Check that the dcb tool TAPI reads the DCB configuration of
 *            an interface, and applies a configuration which is then read
 *            back.
 *
 * A set always changes the configuration. The test skips if the dcb tool
 * is not found on the IUT or the interface does not provide the DCB data.
 * A changed configuration is restored with the tool and read back.
 *
 * @par Scenario:
 */

#define TE_TEST_NAME "dcb"

#include "dcb.h"
#include "tapi_test.h"
#include "tapi_env.h"
#include "tapi_dcb.h"
#include "tapi_cfg_dcbx.h"
#include "tapi_cfg_ets.h"
#include "tapi_cfg_pfc.h"
#include "tapi_job.h"
#include "tapi_job_factory_rpc.h"
#include "tapi_rpc_misc.h"

/**
 * Get mapped value of a DCB command: string -> enum.
 *
 * @param[in] cmd_  Variable whose name is the same as the name of
 *                  DCB command name we get the value.
 *
 * @return Corresponding DCB command value.
 *
 * @note The function aborts if @p cmd_ does not match a supported command or
 *        variable does not exist.
 */
#define TEST_GET_DCB_CMD_PARAM(cmd_) \
    (cmd_) = tapi_dcb_str2cmd(TEST_STRING_PARAM(cmd_))

/** DCBX mode set by the test. */
#define DCB_TEST_DCBX (TAPI_DCB_DCBX_HOST | TAPI_DCB_DCBX_VER_IEEE)

/** DCBX mode set by the test if @ref DCB_TEST_DCBX is already set. */
#define DCB_TEST_DCBX_ALT (TAPI_DCB_DCBX_HOST | TAPI_DCB_DCBX_VER_CEE)

/** First priority mapped to Traffic Class 1 by the test. */
#define DCB_TEST_TC1_PRIO 4

/** Priority whose PFC is enabled if it is disabled on all priorities. */
#define DCB_TEST_PFC_PRIO 3

/** Bandwidth, in percent, of a Traffic Class that has the whole link. */
#define DCB_TEST_BW_ALL 100

/** Bandwidth, in percent, of each of the two Traffic Classes. */
#define DCB_TEST_BW_HALF 50

/**
 * Bandwidths, in percent, of Traffic Classes 0 and 1 if the original
 * configuration already has @ref DCB_TEST_BW_HALF for both.
 */
#define DCB_TEST_BW_TC0_ALT 60
#define DCB_TEST_BW_TC1_ALT (DCB_TEST_BW_ALL - DCB_TEST_BW_TC0_ALT)

/**
 * Get the command which reads the configuration changed by a command.
 *
 * @param cmd   dcb command.
 *
 * @return Show command.
 */
static tapi_dcb_cmd
show_cmd(tapi_dcb_cmd cmd)
{
    switch (cmd)
    {
        case TAPI_DCB_CMD_SET_PFC:
            return TAPI_DCB_CMD_SHOW_PFC;

        case TAPI_DCB_CMD_SET_ETS:
            return TAPI_DCB_CMD_SHOW_ETS;

        case TAPI_DCB_CMD_SET_DCBX:
            return TAPI_DCB_CMD_SHOW_DCBX;

        default:
            return cmd;
    }
}

/**
 * Check whether the interface provides the data a command works with.
 *
 * @param ta        Test Agent.
 * @param if_name   Interface name.
 * @param cmd       dcb command.
 * @param present   Whether the interface provides the data.
 *
 * @return Status code.
 */
static te_errno
dcb_data_is_present(const char *ta, const char *if_name, tapi_dcb_cmd cmd,
                    bool *present)
{
    switch (show_cmd(cmd))
    {
        case TAPI_DCB_CMD_SHOW_PFC:
            return tapi_cfg_pfc_is_present(ta, if_name, present);

        case TAPI_DCB_CMD_SHOW_ETS:
            return tapi_cfg_ets_is_present(ta, if_name, present);

        case TAPI_DCB_CMD_SHOW_DCBX:
            return tapi_cfg_dcbx_is_present(ta, if_name, present);

        default:
            return TE_RC(TE_TAPI, TE_EINVAL);
    }
}

/**
 * Make an ETS configuration which differs from the original one.
 *
 * With at least two Traffic Classes, priorities 0-3 go to TC 0 and 4-7 to
 * TC 1, both scheduled by ETS with 50% each (60% and 40% if that is the
 * original configuration). With one Traffic Class, all priorities go to
 * TC 0, and its TSA switches between ETS (100%) and strict priority.
 * Other Traffic Classes are strict priority with no bandwidth.
 *
 * @param origin    Original ETS state.
 * @param target    Where to store the new configuration.
 */
static void
ets_target_make(const tapi_dcb_ets_state *origin, tapi_dcb_ets_config *target)
{
    unsigned int i;

    *target = origin->config;

    memset(target->tc_bw, 0, sizeof(target->tc_bw));
    memset(target->tc_tsa, TAPI_DCB_TSA_STRICT, sizeof(target->tc_tsa));
    memset(target->prio_tc, 0, sizeof(target->prio_tc));

    if (origin->ets_cap >= 2)
    {
        for (i = DCB_TEST_TC1_PRIO; i < TAPI_DCB_NUM_PRIORITIES; i++)
            target->prio_tc[i] = 1;

        target->tc_tsa[0] = TAPI_DCB_TSA_ETS;
        target->tc_tsa[1] = TAPI_DCB_TSA_ETS;
        target->tc_bw[0] = DCB_TEST_BW_HALF;
        target->tc_bw[1] = DCB_TEST_BW_HALF;

        if (tapi_dcb_ets_config_equal(target, &origin->config))
        {
            target->tc_bw[0] = DCB_TEST_BW_TC0_ALT;
            target->tc_bw[1] = DCB_TEST_BW_TC1_ALT;
        }
    }
    else if (origin->config.tc_tsa[0] != TAPI_DCB_TSA_ETS)
    {
        target->tc_tsa[0] = TAPI_DCB_TSA_ETS;
        target->tc_bw[0] = DCB_TEST_BW_ALL;
    }
}

/**
 * Check whether two PFC configurations are the same.
 *
 * @param a     PFC configuration.
 * @param b     PFC configuration.
 *
 * @return @c true if they are the same.
 */
static bool
pfc_config_equal(const tapi_dcb_pfc_config *a, const tapi_dcb_pfc_config *b)
{
    unsigned int i;

    if (a->mbc != b->mbc || a->delay != b->delay)
        return false;

    for (i = 0; i < TAPI_DCB_NUM_PRIORITIES; i++)
    {
        if (a->pfc_en[i] != b->pfc_en[i])
            return false;
    }

    return true;
}

/**
 * Check whether a configuration read by a show command is the one
 * applied by a set command.
 *
 * @param cmd       Set command.
 * @param applied   Arguments of the set command.
 * @param report    Report of the show command.
 *
 * @return @c true if they are the same.
 */
static bool
config_equal(tapi_dcb_cmd cmd, const tapi_dcb_opt *applied,
             const tapi_dcb_report *report)
{
    switch (cmd)
    {
        case TAPI_DCB_CMD_SET_PFC:
            return pfc_config_equal(&report->data.pfc.config,
                                    &applied->args.pfc);

        case TAPI_DCB_CMD_SET_ETS:
            return tapi_dcb_ets_config_equal(&report->data.ets.config,
                                             &applied->args.ets);

        case TAPI_DCB_CMD_SET_DCBX:
            return report->data.dcbx == applied->args.dcbx;

        default:
            return false;
    }
}

/**
 * Get the arguments of a set command which applies the configuration
 * read by a show command.
 *
 * @param cmd       Set command.
 * @param report    Report of the show command.
 * @param opt       Options whose arguments to fill.
 */
static void
config_from_report(tapi_dcb_cmd cmd, const tapi_dcb_report *report,
                   tapi_dcb_opt *opt)
{
    switch (cmd)
    {
        case TAPI_DCB_CMD_SET_PFC:
            opt->args.pfc = report->data.pfc.config;
            break;

        case TAPI_DCB_CMD_SET_ETS:
            opt->args.ets = report->data.ets.config;
            break;

        case TAPI_DCB_CMD_SET_DCBX:
            opt->args.dcbx = report->data.dcbx;
            break;

        default:
            break;
    }
}

int
main(int argc, char **argv)
{
    rcf_rpc_server *iut_rpcs = NULL;
    const struct if_nameindex *iut_if = NULL;
    tapi_job_factory_t *factory = NULL;
    tapi_dcb_opt opt = tapi_dcb_default_opt;
    tapi_dcb_report origin = tapi_dcb_default_report;
    tapi_dcb_report current = tapi_dcb_default_report;
    tapi_dcb_cmd cmd;
    bool restore = false;
    unsigned int prio;
    bool present;

    TEST_START;
    TEST_PARAM_DOC(env, "IUT host and its interface");
    TEST_GET_PCO(iut_rpcs);
    TEST_GET_IF(iut_if);
    TEST_PARAM_DOC(cmd,
        "dcb command to check:",
        "- SHOW_PFC, SHOW_ETS, SHOW_DCBX: read the configuration",
        "- SET_PFC: switch PFC of one priority",
        "- SET_ETS: change the Traffic Class bandwidths, the TSA and the "
        "Priority to Traffic Class mapping",
        "- SET_DCBX: switch to host-managed IEEE mode, or to host-managed CEE "
        "mode if IEEE is already set");
    TEST_GET_DCB_CMD_PARAM(cmd);

    TEST_STEP("Check that the dcb tool is available on IUT");
    if (rpc_te_file_check_executable(iut_rpcs, "dcb") != 0)
        TEST_SKIP("dcb tool is not found on the IUT");

    TEST_STEP("Check that the interface provides the DCB data");
    CHECK_RC(dcb_data_is_present(iut_rpcs->ta, iut_if->if_name, cmd,
                                 &present));
    if (!present)
        TEST_SKIP("The interface does not provide the DCB data");

    CHECK_RC(tapi_job_factory_rpc_create(iut_rpcs, &factory));
    opt.if_name = iut_if->if_name;

    TEST_STEP("Read the current configuration");
    opt.cmd = show_cmd(cmd);
    rc = tapi_dcb(factory, &opt, &origin);
    if (rc != 0)
        TEST_VERDICT("The dcb tool failed to read the configuration: %r", rc);

    TEST_STEP("Prepare a configuration to apply");
    opt.cmd = cmd;
    switch (cmd)
    {
        case TAPI_DCB_CMD_SHOW_PFC:
        case TAPI_DCB_CMD_SHOW_ETS:
        case TAPI_DCB_CMD_SHOW_DCBX:
            TEST_SUCCESS;
            break;

        case TAPI_DCB_CMD_SET_PFC:
            /*
             * Disabling PFC is always valid; enabling it is limited by
             * pfc_cap.
             */
            opt.args.pfc = origin.data.pfc.config;
            for (prio = 0; prio < TAPI_DCB_NUM_PRIORITIES; prio++)
            {
                if (opt.args.pfc.pfc_en[prio])
                    break;
            }
            if (prio < TAPI_DCB_NUM_PRIORITIES)
            {
                opt.args.pfc.pfc_en[prio] = false;
            }
            else if (origin.data.pfc.pfc_cap > 0)
            {
                opt.args.pfc.pfc_en[DCB_TEST_PFC_PRIO] = true;
            }
            else
            {
                TEST_SKIP("PFC cannot be enabled on any priority");
            }
            break;

        case TAPI_DCB_CMD_SET_ETS:
            if (origin.data.ets.ets_cap == 0)
                TEST_SKIP("The driver reports no ETS Traffic Classes");

            ets_target_make(&origin.data.ets, &opt.args.ets);
            break;

        case TAPI_DCB_CMD_SET_DCBX:
            /* The dcb tool cannot set mode 0, so it could not restore it. */
            if (origin.data.dcbx == 0)
                TEST_SKIP("DCBX mode 0 cannot be restored with the dcb tool");

            opt.args.dcbx = origin.data.dcbx == DCB_TEST_DCBX ?
                            DCB_TEST_DCBX_ALT : DCB_TEST_DCBX;
            break;

        default:
            TEST_FAIL("Unsupported command: %d", cmd);
    }

    TEST_STEP("Apply the configuration");
    restore = true;
    rc = tapi_dcb(factory, &opt, NULL);
    if (rc != 0)
        TEST_VERDICT("The dcb tool failed to apply the configuration: %r",
                     rc);

    TEST_STEP("Read the configuration back and compare");
    opt.cmd = show_cmd(cmd);
    rc = tapi_dcb(factory, &opt, &current);
    if (rc != 0)
        TEST_VERDICT("The dcb tool failed to read the configuration back: "
                     "%r", rc);

    if (!config_equal(cmd, &opt, &current))
        TEST_VERDICT("The configuration read back differs from the applied "
                     "one");

    TEST_SUCCESS;

cleanup:
    if (restore)
    {
        opt.cmd = cmd;
        config_from_report(cmd, &origin, &opt);

        rc = tapi_dcb(factory, &opt, NULL);
        if (rc != 0)
        {
            ERROR_VERDICT("The dcb tool failed to restore the "
                          "configuration: %r", rc);
            result = EXIT_FAILURE;
        }
        else
        {
            opt.cmd = show_cmd(cmd);
            tapi_dcb_destroy_report(&current);
            rc = tapi_dcb(factory, &opt, &current);
            if (rc != 0)
            {
                ERROR_VERDICT("The dcb tool failed to read the restored "
                              "configuration: %r", rc);
                result = EXIT_FAILURE;
            }
            else
            {
                opt.cmd = cmd;
                config_from_report(cmd, &origin, &opt);
                if (!config_equal(cmd, &opt, &current))
                {
                    ERROR_VERDICT("The configuration is not restored");
                    result = EXIT_FAILURE;
                }
            }
        }
    }

    tapi_dcb_destroy_report(&origin);
    tapi_dcb_destroy_report(&current);
    tapi_job_factory_destroy(factory);

    TEST_END;
}
