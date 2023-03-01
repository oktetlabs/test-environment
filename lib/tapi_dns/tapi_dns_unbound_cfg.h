/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2023 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief Unbound DNS server config file generator tool TAPI.
 *
 * TAPI to handle unbound DNS server config file generator tool.
 */

#ifndef __TE_TAPI_DNS_UNBOUND_CFG_H__
#define __TE_TAPI_DNS_UNBOUND_CFG_H__

#include "tapi_job.h"
#include "tapi_job_opt.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Unbound DNS server config file options. */
typedef struct tapi_dns_unbound_cfg_opt tapi_dns_unbound_cfg_opt;

/**
 * Generate config file for unbound DNS server app.
 *
 * @note @p result_pathname value malloc'd, so must be free'd.
 *
 * @param[in]  ta                 Test Agent name.
 * @param[in]  opt                Configs for Unbound server tool.
 * @param[in]  base_dir           Path to directory where the file will be
 *                                generated. If @c NULL, the default base
 *                                directory will be used.
 * @param[in]  pathname           Path to the config file. If @c NULL, the file
 *                                name will be randomly generated. If not an
 *                                absolute path, @c base_dir will be used.
 * @param[out] result_pathname    Resulting path to the file. Maybe @c NULL.
 *
 * @return    Status code.
 */
extern te_errno tapi_dns_unbound_cfg_create(const char *ta,
                                            const tapi_dns_unbound_cfg_opt *opt,
                                            const char *base_dir,
                                            const char *filename,
                                            char **result_pathname);

/**
 * Destroy generated config file for unbound DNS server app.
 *
 * @param[in] ta          Test Agent name.
 * @param[in] cfg_file    Path to generated config file.
 */
extern void tapi_dns_unbound_cfg_destroy(const char *ta, const char *cfg_file);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __TE_TAPI_DNS_UNBOUND_CFG_H__ */
