/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2023 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief Common methods for tapi_dns lib.
 *
 * Common methods for tapi_dns lib.
 */

#ifndef __TAPI_DNS_COMMON_H__
#define __TAPI_DNS_COMMON_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Generate absolute path to file on TA side.
 *
 * @param ta          Test Agent.
 * @param base_dir    Path to directory where the file will be generated.
 *                    If @c NULL, the default base directory will be used.
 * @param filename    Path to the config file. If @c NULL, the file name will be
 *                    randomly generated. If not an absolute path, @c base_dir
 *                    will be used.
 *
 * @return    Resulting path to the file.
 */
extern char *tapi_dns_gen_filepath(const char *ta,
                                   const char *base_dir,
                                   const char *filename);

#ifdef __cplusplus
}
#endif
#endif /* __TAPI_DNS_COMMON_H__ */