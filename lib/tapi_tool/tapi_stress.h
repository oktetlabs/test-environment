/** @file
 * @brief stress tool TAPI
 *
 * @defgroup tapi_stress tool functions TAPI
 * @ingroup te_ts_tapi
 * @{
 *
 * TAPI to handle stress tool.
 *
 * Copyright (C) 2020-2022 OKTET Labs. All rights reserved.
 */

#ifndef __TE_TAPI_STRESS_H__
#define __TE_TAPI_STRESS_H__

#include "te_errno.h"
#include "te_string.h"
#include "tapi_job.h"
#include "tapi_job_opt.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Default stress tool termination timeout applicable in most cases */
#define TAPI_STRESS_DEFAULT_TERM_TIMEOUT_MS 100

/**
 * stress tool specific command line options. At least one of the
 * stress targets (CPU, IO, VM) should be specified (by setting a value other
 * than @ref TAPI_JOB_OPT_OMIT_UINT).
 */
typedef struct tapi_stress_opt {
    /* Spawn N workers spinning on sqrt(). 0 - set N to number of all CPUs */
    unsigned int cpu;
    /* Spawn N workers spinning on sync() */
    unsigned int io;
    /* Spawn N workers spinning on malloc()/free() */
    unsigned int vm;
    /*
     * Run stress test for specified number of seconds.
     * Set to @ref TAPI_JOB_OPT_OMIT_UINT to run forever.
     */
    unsigned int timeout_s;
} tapi_stress_opt;

/** Default options initializer */
static const tapi_stress_opt tapi_stress_default_opt = {
    .cpu = TAPI_JOB_OPT_OMIT_UINT,
    .io = TAPI_JOB_OPT_OMIT_UINT,
    .vm = TAPI_JOB_OPT_OMIT_UINT,
    .timeout_s = TAPI_JOB_OPT_OMIT_UINT,
};

/** Information of a stress tool */
struct tapi_stress_app;

/**
 * Create stress app. All needed information to run stress is in @p opt
 *
 * @param[in]  factory      Job factory.
 * @param[in]  opt          Options of stress tool.
 * @param[out] app          stress app handle.
 *
 * @return Status code.
 */
extern te_errno tapi_stress_create(tapi_job_factory_t *factory,
                                   const tapi_stress_opt *opt,
                                   struct tapi_stress_app **app);

/**
 * Start stress app.
 *
 * @param[in]  app          stress app handle.
 *
 * @return Status code.
 */
extern te_errno tapi_stress_start(struct tapi_stress_app *app);

/**
 * Stop stress app.
 *
 * @param[in]  app          stress app handle.
 * @param[in]  timeout_ms   Wait timeout in milliseconds.
 *
 * @return Status code.
 */
extern te_errno tapi_stress_stop(struct tapi_stress_app *app, int timeout_ms);

/**
 * Destroy stress app.
 *
 * @param[in]  app          stress app handle.
 */
extern void tapi_stress_destroy(struct tapi_stress_app *app);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __TE_TAPI_STRESS_H__ */

/**@} <!-- END tapi_stress --> */
