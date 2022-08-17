/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief mke2fs tool TAPI
 *
 * @defgroup tapi_mke2fs mke2fs tool tapi (tapi_mke2fs)
 * @ingroup te_ts_tapi
 * @{
 *
 * TAPI to handle mke2fs tool.
 *
 * Copyright (C) 2020-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_TAPI_MKE2FS_H__
#define __TE_TAPI_MKE2FS_H__

#include "tapi_job.h"

#ifdef __cplusplus
extern "C" {
#endif

/** mke2fs tool specific command line options. */
typedef struct tapi_mke2fs_opt {
    /**
     * Size of blocks in bytes. If omitted with TAPI_JOB_OPT_OMIT_UINT,
     * block-size is heuristically determined by the filesystem size and
     * the expected usage of the filesystem.
     */
    unsigned int block_size;
    /** Create the filesystem with an ext3 journal. */
    te_bool use_journal;
    /** The filesystem type that is to be created. */
    const char *fs_type;
    /** The device name on which to create the filesystem (mandatory). */
    const char *device;
} tapi_mke2fs_opt;

/** Default options initializer. */
extern const tapi_mke2fs_opt tapi_mke2fs_default_opt;

/** mke2fs tool information. */
typedef struct tapi_mke2fs_app tapi_mke2fs_app;

/**
 * Create mke2fs app.
 *
 * @param[in]  factory         Job factory.
 * @param[in]  opt             mke2fs tool options.
 * @param[out] app             mke2fs app handle.
 *
 * @return     Status code.
 */
extern te_errno tapi_mke2fs_create(tapi_job_factory_t *factory,
                                   const tapi_mke2fs_opt *opt,
                                   tapi_mke2fs_app **app);

/**
 * Start mke2fs tool.
 *
 * @param      app             mke2fs app handle.
 *
 * @return     Status code.
 */
extern te_errno tapi_mke2fs_start(tapi_mke2fs_app *app);

/**
 * Wait for mke2fs tool completion.
 *
 * @param      app             mke2fs app handle.
 * @param      timeout_ms      Wait timeout in milliseconds.
 *
 * @return     Status code.
 * @retval     TE_EINPROGRESS  mke2fs is still running.
 * @retval     TE_ESHCMD       mke2fs was never started or returned
 *                             non-zero exit status.
 */
extern te_errno tapi_mke2fs_wait(tapi_mke2fs_app *app, int timeout_ms);

/**
 * Send a signal to mke2fs tool.
 *
 * @param      app             mke2fs app handle.
 * @param      signum          Signal to send.
 *
 * @return     Status code.
 */
extern te_errno tapi_mke2fs_kill(tapi_mke2fs_app *app, int signum);

/**
 * Stop mke2fs tool. It can be started over with tapi_mke2fs_start().
 *
 * @param      app             mke2fs app handle.
 *
 * @return     Status code.
 */
extern te_errno tapi_mke2fs_stop(tapi_mke2fs_app *app);

/**
 * Destroy mke2fs app. The app cannot be used after calling this function.
 *
 * @param      app             mke2fs app handle.
 *
 * @return     Status code.
 */
extern te_errno tapi_mke2fs_destroy(tapi_mke2fs_app *app);

/**
 * Check if the filesystem was created with ext3 journal.
 * The function should be called after tapi_mke2fs_wait().
 *
 * @param      app             mke2fs app handle.
 *
 * @return     Status code.
 * @retval     0               tapi_mke2fs_opt::use_journal was not specified
 *                             or it was specified and the filesystem was
 *                             created with the journal
 * @retval     TE_EPROTO       tapi_mke2fs_opt::use_journal was specified
 *                             but the journal was not created.
 */
extern te_errno tapi_mke2fs_check_journal(tapi_mke2fs_app *app);

/**
 * A convenience wrapper for tapi_mke2fs_create/start/wait.
 *
 * @param      factory         Job factory.
 * @param      opt             mke2fs tool options.
 * @param      app             mke2fs app handle.
 * @param      timeout_ms      Wait timeout in milliseconds.
 *
 * @return     Status code.
 */
extern te_errno tapi_mke2fs_do(tapi_job_factory_t *factory,
                               const tapi_mke2fs_opt *opt,
                               tapi_mke2fs_app **app,
                               int timeout_ms);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __TE_TAPI_MKE2FS_H__ */

/** @} <!-- END tapi_mke2fs --> */
