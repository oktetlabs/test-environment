/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2022 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief TE RCF Engine - parser of TCE configuration
 *
 * Internal definitions to process the TCE configuration.
 *
 * The TCE information is generated on TA agents within the same directories
 * the TA agents have been built. To fetch this information the RCF controller
 * has to be provided with paths to this directories and to the directory,
 * the TCE workspace directory, to save this information into. This information
 * is provided within the TCE configuration file.
 *
 * The TCE configuration is an auto-generated YAML document.
 * Do not create it manually!
 *
 * The root node of this document must provide the @p te_local node and zero
 * or more @p ta_type nodes.
 *
 * The @p te_local node provides the information on the directories of the TE
 * host. It must contain the following mappings:
 * - @p tebin the directory where TE environment executables are located,
 * - @p tcews the directory where to save the TCE information fetched.
 *
 * The @p ta_type node provides the information on build directories of TA
 * agent that are expected to generate the TCE information. It must contain
 * the following mappings:
 * - @p name the type of the TA agent,
 * - @p base the base of build directories of TA components.
 * Also, it contains zero or mode @p ta_comp nodes that provide the
 * information on TA components of the TA agent considered.
 *
 * The @p ta_comp node must contain the following mappings:
 * - @p name the name of a TA component,
 * - @p build the build directory of this component.
 *
 * There is an example:
 * @code{.yaml}
 * ---
 *
 * te_local:
 *   tebin: "/home/ivanov/te.run/inst/default/bin"
 *   tcews: "/home/ivanov/te.run/tce_workspace"
 *
 * ta_type:
 *    name: "linux64"
 *    base: "/tmp/te_ws_d92b8d8df337303d7028ee7f760b949c"
 *
 *    ta_comp:
 *      name:  "dpdk"
 *      build: "ext/dpdk/build"
 * @endcode
 */

#ifndef __TE_RCF_TCE_PARSER_H__
#define __TE_RCF_TCE_PARSER_H__

#include "rcf_tce_conf.h"

/**
 * Parse an RCF TCE configuration.
 *
 * @param[out] conf The variable to store the configuration.
 * @param      file The path to the TCE configuration file.
 *
 * @return 0 on success or TE errno otherwise.
 */
extern te_errno rcf_tce_conf_parse(rcf_tce_conf_t **conf, const char *file);

/** Destroy an RCF TCE configuration. */
extern void rcf_tce_conf_free(rcf_tce_conf_t *conf);

#endif /* __TE_RCF_TCE_PARSER_H__ */
