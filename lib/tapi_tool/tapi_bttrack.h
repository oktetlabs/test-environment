/** @file
 * @brief TAPI to manage bttrack torrent tracker
 *
 * @defgroup tapi_bttrack TAPI to manage bttrack torrent tracker (tapi_bttrack)
 * @ingroup tapi_ctorrent
 * @{
 *
 * TAPI to manage *bttrack* torrent tracker (from *bittornado* package).
 *
 * Copyright (C) 2021-2022 OKTET Labs. All rights reserved.
 */

#ifndef __TE_TAPI_BTTRACK_H__
#define __TE_TAPI_BTTRACK_H__

#include "te_errno.h"
#include "tapi_job.h"

#ifdef __cplusplus
extern "C" {
#endif

/** bttrack insance handle */
struct tapi_bttrack_app {
    /** TAPI job handle */
    tapi_job_t *job;
    /** IP address associated with the tracker (accessed by peers) */
    const char *ip;
    /** TCP port */
    unsigned int port;
    /** Output channel handles */
    tapi_job_channel_t *out_chs[2];
};

typedef struct tapi_bttrack_app tapi_bttrack_app;

/** bttrack specific options */
typedef struct tapi_bttrack_opt {
    /**
     * File to store recent downloader information (it will be created
     * if it does not exist). This option must be set (even if you use
     * tapi_bttrack_default_opt), use tapi_file_generate_name() or
     * anything in /tmp (like /tmp/dfile) if you do not care about
     * the file's content.
     */
    char *dfile;
    /** TCP port to listen on (default is 80) */
    unsigned int port;
} tapi_bttrack_opt;

/** Default bttrack options initializer */
extern const tapi_bttrack_opt tapi_bttrack_default_opt;

/**
 * Create bttrack app.
 *
 * @param[in]  factory       Job factory.
 * @param[in]  ip            IP address of the tracker.
 * @param[in]  opt           bttrack options.
 * @param[out] app           bttrack app handle.
 *
 * @return     Status code.
 */
extern te_errno tapi_bttrack_create(tapi_job_factory_t *factory,
                                    const char         *ip,
                                    tapi_bttrack_opt   *opt,
                                    tapi_bttrack_app  **app);

/**
 * Start bttrack.
 *
 * @param      app           bttrack app handle.
 *
 * @return     Status code.
 */
extern te_errno tapi_bttrack_start(tapi_bttrack_app *app);

/**
 * Wait for bttrack completion.
 *
 * @param      app           bttrack app handle.
 * @param      timeout_ms    Wait timeout in milliseconds.
 *
 * @return     Status code.
 */
extern te_errno tapi_bttrack_wait(tapi_bttrack_app *app, int timeout_ms);

/**
 * Send a signal to bttrack.
 *
 * @param      app           bttrack app handle.
 * @param      signum        Signal to send.
 *
 * @return     Status code.
 */
extern te_errno tapi_bttrack_kill(tapi_bttrack_app *bttrack,
                                  int signum);

/**
 * Stop bttrack. It can be started over with tapi_bttrack_start().
 *
 * @param      app           bttrack app handle.
 *
 * @return     Status code.
 */
extern te_errno tapi_bttrack_stop(tapi_bttrack_app *bttrack);

/**
 * Destroy bttrack app (free memory, etc.).
 *
 * @param      app           bttrack app handle.
 *
 * @return     Status code.
 */
extern te_errno tapi_bttrack_destroy(tapi_bttrack_app *bttrack);


#ifdef __cplusplus
}
#endif

#endif /* __TE_TAPI_BTTRACK_H__ */

/** @} <!-- END tapi_bttrack --> */
