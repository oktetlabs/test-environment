/** @file
 * @brief NPtcp tool TAPI
 *
 * @defgroup tapi_nptcp NPtcp tool tapi (tapi_nptcp)
 * @ingroup te_ts_tapi
 * @{
 *
 * TAPI to handle NPtcp tool.
 *
 * Copyright (C) 2020 OKTET Labs. All rights reserved.
 *
 * @author Andrey Izrailev <Andrey.Izrailev@oktetlabs.ru>
 */

#ifndef __TE_TAPI_NPTCP_H__
#define __TE_TAPI_NPTCP_H__

#include "te_errno.h"
#include "tapi_job.h"
#include "tapi_job_opt.h"
#include "rcf_rpc.h"
#include "te_mi_log.h"

#ifdef __cplusplus
extern "C" {
#endif

/** NPtcp tool specific command line options */
typedef struct tapi_nptcp_opt {
    /** Send and receive TCP buffer size (in bytes) */
    unsigned int tcp_buffer_size;
    /** Receiver host to connect to */
    const char *host;
    /** Invalidate cache */
    te_bool invalidate_cache;
    /** Lower bound for the size of message to be tested (in bytes) */
    unsigned int starting_msg_size;
    /** The number of repeats for each test */
    unsigned int nrepeats;
    /**
     * Transmit and receive buffer offsets from perfect page alignment.
     * Use one integer to set both offsets to the value or two integeres
     * separated by comma (e.g. "5,10") to set transmit and receive buffer
     * offsets correspondingly.
     */
    const char *offsets;
    /** Output filename (default is np.out) */
    const char *output_filename;
    /** Perturbation size (in bytes) */
    unsigned int perturbation_size;
    /** Reset the TCP sockets */
    te_bool reset_sockets;
    /** Set streaming mode */
    te_bool streaming_mode;
    /** Upper bound for the size of message to be tested (in bytes) */
    unsigned int upper_bound;
    /** Set bi-directional mode */
    te_bool bi_directional_mode;
} tapi_nptcp_opt;

/** Default options initializer */
extern const tapi_nptcp_opt tapi_nptcp_default_opt;

/** Entry (row) of NPtcp statistics report */
typedef struct tapi_nptcp_report_entry {
    /** Entry number */
    unsigned int number;
    /** Entry bytes */
    unsigned int bytes;
    /** Entry number if times */
    unsigned int times;
    /** Entry throughput */
    double throughput;
    /** Entry rtt */
    double rtt;
} tapi_nptcp_report_entry;

/** Information of a NPtcp tool */
typedef struct tapi_nptcp_app {
    /** Receiver's job handle */
    tapi_job_t *job_receiver;
    /** Transmitter's job handle */
    tapi_job_t *job_transmitter;
    /** Output channel handles */
    tapi_job_channel_t *out_chs[2];
    /** Entry number filter */
    tapi_job_channel_t *num_filter;
    /** Sent bytes filter */
    tapi_job_channel_t *bytes_filter;
    /** The number of times this amount of bytes was sent filter */
    tapi_job_channel_t *times_filter;
    /** Throughput filter */
    tapi_job_channel_t *throughput_filter;
    /** RTT filter */
    tapi_job_channel_t *rtt_filter;
} tapi_nptcp_app;

/**
 * Create NPtcp app.
 *
 * @param[in]  factory_receiver     Job factory for receiver.
 * @param[in]  factory_transmitter  Job factory for transmitter.
 * @param[in]  opt_receiver         Options to run NPtcp with on
 *                                  receiver's side.
 * @param[in]  opt_transmitter      Options to run NPtcp with on
 *                                  transmitter's side.
 * @param[out] app                  NPtcp app handle.
 *
 * @return Status code.
 */
extern te_errno tapi_nptcp_create(tapi_job_factory_t *factory_receiver,
                                  tapi_job_factory_t *factory_transmitter,
                                  const tapi_nptcp_opt *opt_receiver,
                                  const tapi_nptcp_opt *opt_transmitter,
                                  tapi_nptcp_app **app);

/**
 * Start NPtcp.
 *
 * @param      app                  NPtcp app handle.
 *
 * @return Status code.
 */
extern te_errno tapi_nptcp_start(tapi_nptcp_app *app);

/**
 * Wait for NPtcp completion.
 *
 * @param      app                  NPtcp app handle.
 * @param      timeout_ms           Wait timeout in milliseconds.
 *
 * @return Status code.
 */
extern te_errno tapi_nptcp_wait(tapi_nptcp_app *app, int timeout_ms);

/**
 * Wait for NPtcp receiver completion. It should be used only if
 * tapi_nptcp_kill_receiver() or tapi_nptcp_kill_transmitter() was called.
 * If you want to wait for the tool completion, use tapi_nptcp_wait().
 * Same applies to tapi_nptcp_wait_transmitter().
 *
 * @param      app                  NPtcp app handle.
 * @param      timeout_ms           Wait timeout in milliseconds.
 *
 * @return Status code.
 *
 * @sa tapi_nptcp_wait_transmitter
 */
extern te_errno tapi_nptcp_wait_receiver(tapi_nptcp_app *app, int timeout_ms);

/**
 * Wait for NPtcp transmitter completion.
 *
 * @param      app                  NPtcp app handle.
 * @param      timeout_ms           Wait timeout in milliseconds.
 *
 * @return Status code.
 *
 * @sa tapi_nptcp_wait_receiver
 */
extern te_errno tapi_nptcp_wait_transmitter(tapi_nptcp_app *app,
                                            int timeout_ms);

/**
 * Get NPtcp report.
 *
 * @param[in]  app                  NPtcp app handle.
 * @param[out] report               NPtcp report handle. May be passed
 *                                  uninitialized, should be freed
 *                                  with te_vec_free().
 *
 * @return Status code.
 */
extern te_errno tapi_nptcp_get_report(tapi_nptcp_app *app,
                                      te_vec *report);

/**
 * Send a signal to NPtcp running on receiver's side.
 *
 * @param      app                  NPtcp app handle.
 * @param      signum               Signal to send.
 *
 * @return Status code.
 */
extern te_errno tapi_nptcp_kill_receiver(tapi_nptcp_app *app, int signum);

/**
 * Send a signal to NPtcp running on transmitter's side.
 *
 * @param      app                  NPtcp app handle.
 * @param      signum               Signal to send.
 *
 * @return Status code.
 */
extern te_errno tapi_nptcp_kill_transmitter(tapi_nptcp_app *app, int signum);

/**
 * Stop NPtcp. It can be started over with tapi_nptcp_start().
 *
 * @param      app                  NPtcp app handle.
 *
 * @return Status code.
 */
extern te_errno tapi_nptcp_stop(tapi_nptcp_app *app);

/**
 * Destroy NPtcp job running on receiver's side.
 *
 * @param      app                  NPtcp app handle.
 *
 * @return Status code.
 */
extern te_errno tapi_nptcp_destroy(tapi_nptcp_app *app);

/**
 * Add NPtcp report to MI logger.
 *
 * @param[out] logger               MI logger entity.
 * @param[in]  report               NPtcp report.
 */
extern void tapi_nptcp_report_mi_log(te_mi_logger *logger,
                                     te_vec *report);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __TE_TAPI_NPTCP_H__ */

/** @} <!-- END tapi_nptcp --> */
