/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Test API for SPDK iostat.py
 *
 * @defgroup tapi_spdk_iostat Test API for SPDK iostat tool
 * @ingroup te_ts_tapi
 * @{
 *
 * Test API to control SPDK 'iostat.py' tool
 *
 * Copyright (C) 2025 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_TAPI_SPDK_IOSTAT_H__
#define __TE_TAPI_SPDK_IOSTAT_H__

#include "te_defs.h"
#include "te_errno.h"
#include "te_string.h"
#include "tapi_job.h"
#include "tapi_job_opt.h"
#include "te_units.h"

#include "te_vector.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Forward declaration of SPDK iostat handle */
typedef struct tapi_spdk_iostat_app tapi_spdk_iostat_app;

/** SPDK iostat options */
typedef struct tapi_spdk_iostat_opt {
    /** Server socket path (default: /var/tmp/spdk.sock) or IP address */
    const char *server;
    /** Bdev name to get stat. use @c NULL to print stats for all bdevs */
    const char *bdev_name;
    /** Get extended stats */
    bool extended;
    /** Use verbose mode */
    bool verbose;
} tapi_spdk_iostat_opt;

extern const tapi_spdk_iostat_opt tapi_spdk_iostat_default_opt;

typedef struct tapi_spdk_iostat_dev_report_t {
    char    name[64];
    double  tps;
    te_unit kb_read_s;
    te_unit kb_wrtn_s;
    te_unit kb_dscd_s;
    te_unit kb_read;
    te_unit kb_wrtn;
    te_unit kb_dscd;
} tapi_spdk_iostat_dev_report_t;

typedef struct tapi_spdk_iostat_report_t {
    te_vec devices;
} tapi_spdk_iostat_report_t;

/**
 * Run SPDK iostat command.
 *
 * @param[in]  factory     Job factory
 * @param[in]  iostat_path Path to iostat.py script
 * @param[in]  opt         Server connection options.
 * @param[out] report      iostat.py report
 *
 * @return Status code
 */
extern te_errno tapi_spdk_iostat(tapi_job_factory_t         *factory,
                                 const char                 *iostat_path,
                                 const tapi_spdk_iostat_opt *opt,
                                 tapi_spdk_iostat_report_t  *report);

/**
 * Get diff between two reports
 *
 * @param first_report  The first report
 * @param second_report The second report
 * @param diff_report   Diff between the second and the first
 */
extern void
tapi_spdk_iostat_get_diff_report(tapi_spdk_iostat_report_t *first_report,
                                 tapi_spdk_iostat_report_t *second_report,
                                 tapi_spdk_iostat_report_t *diff_report);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __TE_TAPI_SPDK_IOSTAT__H__ */

/**@} <!-- END tapi_spdk_iostat --> */
