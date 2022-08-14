/** @file
 * @brief TAPI to manage ctorrent
 *
 * @defgroup tapi_ctorrent TAPI to manage ctorrent (tapi_ctorrent)
 * @ingroup te_ts_tapi
 * @{
 *
 * TAPI to manage ctorrent – a BitTorrent client.
 *
 * Copyright (C) 2021-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_TAPI_CTORRENT_H__
#define __TE_TAPI_CTORRENT_H__

#include "te_errno.h"
#include "tapi_bttrack.h"

#ifdef __cplusplus
extern "C" {
#endif

/** ctorrent instance handle */
typedef struct tapi_ctorrent_app tapi_ctorrent_app;

/** ctorrent specific options */
typedef struct tapi_ctorrent_opt {
    /** Seeding duration (in hours, 72 by default) */
    unsigned int hours_to_seed;
    /** IP to listen on (all by default) */
    const char *ip;
    /** TCP port to listen on */
    unsigned int port;
    /**
     * Save downloaded data to the specified file. By default, the data will
     * be saved to a file with the same name as an original file had.
     */
    const char *save_to_file;
    /** Max peers count (100 by default) */
    unsigned int max_peers;
    /** Min peers count (1 by default) */
    unsigned int min_peers;
    /** Max download bandwidth (in KBps (not Kbps)) */
    unsigned int download_rate;
    /** Max upload bandwidth (in KBps) */
    unsigned int upload_rate;
    /** Metainfo file to download/seed (required) */
    const char *metainfo_file;
} tapi_ctorrent_opt;

/** Default ctorrent's options initializer */
extern const tapi_ctorrent_opt tapi_ctorrent_default_opt;

/**
 * Create metainfo (.torrent) file.
 *
 * @param      factory        Job factory.
 * @param      tracker        Torrent tracker handle.
 * @param      metainfo_file  Name (or pathname) of a file that will be created.
 *                            The file will contain all necessary information
 *                            to share data stored in target.
 * @param      target         Name (or pathname) of a file or a directory
 *                            that will be shared.
 * @param      timeout_ms     Timeout to wait for creation to complete. Note
 *                            that the bigger file is, the bigger amount of
 *                            time is required. Use a negative number for
 *                            a default timeout.
 *
 * @return     Status code.
 * @retval     TE_EEXIST      *metainfo_file* already exists on TA.
 */
extern te_errno tapi_ctorrent_create_metainfo_file(
                                            tapi_job_factory_t *factory,
                                            tapi_bttrack_app   *tracker,
                                            const char         *metainfo_file,
                                            const char         *target,
                                            int                 timeout_ms);

/**
 * Create ctorrent app.
 *
 * @param[in]  factory        Job factory.
 * @param[in]  opt            ctorrent options.
 * @param[out] app            ctorrent app handle.
 *
 * @return     Status code.
 *
 * @note       It is always better to specify tapi_ctorrent_opt::save_to_file
 *             option even when creating an app for an original seeder.
 *             Otherwise, it won't find a file to seed if it is not
 *             in the current directory.
 */
extern te_errno tapi_ctorrent_create_app(tapi_job_factory_t  *factory,
                                         tapi_ctorrent_opt   *opt,
                                         tapi_ctorrent_app  **app);

/**
 * Start ctorrent.
 * All required data will be downloaded, seeding will be initiated (i.e.
 * the host will become a peer).
 *
 * @param      app            ctorrent app handle.
 *
 * @return     Status code.
 */
extern te_errno tapi_ctorrent_start(tapi_ctorrent_app *app);

/**
 * Send a signal to ctorrent.
 *
 * @param      app            ctorrent app handle.
 * @param      signum         Signal to send.
 *
 * @return     Status code.
 */
extern te_errno tapi_ctorrent_kill(tapi_ctorrent_app *app, int signum);

/**
 * Stop ctorrent. It can be started over with tapi_ctorrent_start().
 * Before termination, ctorrent app will try to send its last report
 * to a tracker, it might require some time.
 *
 * @param      app            ctorrent app handle.
 * @param      timeout_ms     Timeout of graceful termination. After
 *                            the timeout expiration the job will be killed
 *                            with @c SIGKILL. Negative means a default timeout.
 *
 * @return     Status code.
 */
extern te_errno tapi_ctorrent_stop(tapi_ctorrent_app *app, int timeout_ms);

/**
 * Destroy ctorrent.
 *
 * @param      app            ctorrent app handle.
 * @param      timeout_ms     Timeout of graceful termination. After
 *                            the timeout expiration the job will be killed
 *                            with @c SIGKILL. Negative means a default timeout.
 *
 * @return     Status code.
 */
extern te_errno tapi_ctorrent_destroy(tapi_ctorrent_app *app, int timeout_ms);

/**
 * Check if the download is completed.
 * The download is considered completed if the file specified by
 * tapi_ctorrent_opt::save_to_file contains all required pieces, so for original
 * seeder the download is always completed.
 *
 * @param[in]  app                 ctorrent app handle.
 * @param[in]  receive_timeout_ms  Timeout to receive status data from ctorrent.
 * @param[out] completed           @c TRUE if the download is completed.
 *
 * @return     Status code.
 * @retval     TE_ETIMEDOUT        ctorrent has not displayed its status line
 *                                 for too long.
 *
 * @note       Sometimes ctorrent may check its pieces integrity and do not
 *             display its status line for some time. The bigger torrent is,
 *             the more time is required to check the integrity, so be sure
 *             to set big enough timeout (or use a negative value for
 *             a default timeout).
 */
extern te_errno tapi_ctorrent_check_completion(tapi_ctorrent_app *app,
                                               int receive_timeout_ms,
                                               te_bool *completed);

/**
 * Wait for the download to complete.
 *
 * @param      app                 ctorrent app handle.
 * @param      receive_timeout_ms  Timeout to receive status data from ctorrent.
 *
 * @return     Status code.
 * @retval     TE_ETIMEDOUT        ctorrent has not displayed its status line
 *                                 for too long.
 *
 * @sa         tapi_ctorrent_check_completion()
 */
extern te_errno tapi_ctorrent_wait_completion(tapi_ctorrent_app *app,
                                              int receive_timeout_ms);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __TE_TAPI_CTORRENT_H__ */

/** @} <!-- END tapi_ctorrent --> */
