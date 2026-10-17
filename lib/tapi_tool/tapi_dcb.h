/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2026 Advanced Micro Devices, Inc. All rights reserved. */
/** @file
 * @brief dcb tool TAPI
 *
 * @defgroup tapi_dcb DCB tool TAPI (tapi_dcb)
 * @ingroup te_ts_tapi
 * @{
 *
 * Declarations for TAPI to handle dcb tool.
 */

#ifndef __TE_TAPI_DCB_H__
#define __TE_TAPI_DCB_H__

#include "te_errno.h"
#include "te_enum.h"
#include "tapi_job.h"
#include "tapi_job_opt.h"

#include "tapi_dcb_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Supported dcb commands. */
typedef enum tapi_dcb_cmd {
    /** Show PFC properties (`pfc show` command). */
    TAPI_DCB_CMD_SHOW_PFC,
    /** Show ETS properties (`ets show` command). */
    TAPI_DCB_CMD_SHOW_ETS,
    /** Show DCBX properties (`dcbx show` command). */
    TAPI_DCB_CMD_SHOW_DCBX,
    /** Set PFC properties (`pfc set` command). */
    TAPI_DCB_CMD_SET_PFC,
    /** Set ETS properties (`ets set` command). */
    TAPI_DCB_CMD_SET_ETS,
    /** Set DCBX properties (`dcbx set` command). */
    TAPI_DCB_CMD_SET_DCBX,
} tapi_dcb_cmd;

/** Command line options for dcb. */
typedef struct tapi_dcb_opt {
    /** DCB command. */
    tapi_dcb_cmd cmd;
    /** Command execution timeout in milliseconds. */
    int timeout_ms;
    /** Interface name. */
    const char *if_name;

    union {
        tapi_dcb_pfc_config pfc;
        tapi_dcb_ets_config ets;
        tapi_dcb_dcbx_config dcbx;
    } args; /**< Command-specific arguments. */
} tapi_dcb_opt;

/** Structure for storing parsed data from DCB output. */
typedef struct tapi_dcb_report {
    /** DCB command. */
    tapi_dcb_cmd cmd;
    /** @c true if something was printed to stderr. */
    bool err_out;
    /** Text printed to stderr. */
    te_string err_data;
    /** @c true if something was printed to stdout. */
    bool out;
    /** Text printed to stdout. */
    te_string out_data;

    union {
        tapi_dcb_pfc_state pfc;
        tapi_dcb_ets_state ets;
        tapi_dcb_dcbx_state dcbx;
    } data; /**< Parsed data. */
} tapi_dcb_report;

/** Mapping for dcb commands of dcb util. */
static const te_enum_map tapi_dcb_cmd_enum_map[] = {
    {.name = "SHOW_PFC", .value = TAPI_DCB_CMD_SHOW_PFC},
    {.name = "SHOW_ETS", .value = TAPI_DCB_CMD_SHOW_ETS},
    {.name = "SHOW_DCBX", .value = TAPI_DCB_CMD_SHOW_DCBX},
    {.name = "SET_PFC", .value = TAPI_DCB_CMD_SET_PFC},
    {.name = "SET_ETS", .value = TAPI_DCB_CMD_SET_ETS},
    {.name = "SET_DCBX", .value = TAPI_DCB_CMD_SET_DCBX},
    TE_ENUM_MAP_END
};

/** Default options initializer. */
extern const tapi_dcb_opt tapi_dcb_default_opt;

/** Empty report, as tapi_dcb_destroy_report() leaves it. */
extern const tapi_dcb_report tapi_dcb_default_report;

/**
 * Convert a DCB command name to its corresponding command value.
 *
 * @param[in] cmd   DCB command name.
 *
 * @return Corresponding DCB command value.
 *
 * @note The function aborts if @p cmd does not match a supported command.
 */
extern tapi_dcb_cmd tapi_dcb_str2cmd(const char *cmd);

/**
 * Convert a DCB command value to its corresponding name.
 *
 * @param[in] cmd   DCB command value.
 *
 * @return Corresponding DCB command name.
 *
 * @note The function aborts if @p cmd does not match a supported command.
 */
extern const char *tapi_dcb_cmd2str(const tapi_dcb_cmd cmd);

/**
 * Run DCB util, parse its output if required.
 *
 * @param[in] factory       Job factory.
 * @param[in] opt           DCB command options.
 * @param[out] report       If not @c NULL, parsed data from output will be
 *                          saved here. It is overwritten without releasing
 *                          what it holds, so release a report filled by a
 *                          previous call before passing it again.
 *
 * stderr of the tool is logged at RING level and kept in @p report; the
 * caller decides whether a failure is expected.
 *
 * @return Status code.
 * @retval 0                Success. For a set, this does not prove that the
 *                          driver applied the configuration (the kernel
 *                          reports success if the driver has no setter, and
 *                          the driver may adjust the values).
 * @retval TE_EINVAL        @p opt cannot be expressed with the tool: a TSA
 *                          without a name, DCBX mode 0 or unknown DCBX flags.
 * @retval TE_ETIMEDOUT     The tool did not complete in time.
 * @retval TE_EPROTO        The tool printed a TSA without a name (3..254)
 *                          as "???", so its value is unknown.
 * @retval TE_ENOBUFS       Part of the tool output was lost.
 * @retval TE_EFAIL         The tool succeeded but printed to stderr (e.g.
 *                          a warning about an abbreviated argument).
 * @retval TE_ESHCMD        The tool failed, or could not be run. The tool
 *                          reports why only as text on stderr (kept in
 *                          @p report), which is not interpreted: the
 *                          interface may not support DCB, the driver may
 *                          reject the request, or the tool may reject the
 *                          arguments itself.
 *
 * @note @p report is filled even if the call fails: its command is always
 *       set, and the output read so far is kept, including stderr of a
 *       tool that did not complete in time. Release it with
 *       tapi_dcb_destroy_report().
 */
extern te_errno tapi_dcb(tapi_job_factory_t *factory,
                         const tapi_dcb_opt *opt,
                         tapi_dcb_report *report);

/**
 * Release resources allocated for dcb output data.
 *
 * @param[in,out] report    Structure storing parsed data. Passing @c NULL is
 *                          allowed, but has no effect.
 *
 * @note @p report must be filled by tapi_dcb() or be
 *       @ref tapi_dcb_default_report. After this function returns, the
 *       report is reset to @ref tapi_dcb_default_report.
 */
extern void tapi_dcb_destroy_report(tapi_dcb_report *report);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __TE_TAPI_DCB_H__ */

/** @} <!-- END tapi_dcb --> */
