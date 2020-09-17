/** @file
 * @brief ping tool TAPI
 *
 * @defgroup tapi_ping ping tool TAPI (tapi_ping)
 * @ingroup te_ts_tapi
 * @{
 *
 * TAPI to handle ping tool.
 *
 * Copyright (C) 2020 OKTET Labs. All rights reserved.
 *
 * @author Andrey Izrailev <Andrey.Izrailev@oktetlabs.ru>
 */

#ifndef __TE_TAPI_PING_H__
#define __TE_TAPI_PING_H__

#include "tapi_job.h"
#include "te_mi_log.h"

#ifdef __cplusplus
extern "C" {
#endif

/** ping tool specific command line options */
typedef struct tapi_ping_opt {
    /**
     * Number of packets to send. If it is @c TAPI_JOB_OPT_OMIT_UINT,
     * the option is omitted and ping sends packets until explicitly stopped
     */
    unsigned int packet_count;
    /**
     * Number of data bytes to send (default is 56). Note that the real
     * size of the packet will be 8 bytes more because of an ICMP header.
     */
    unsigned int packet_size;
    /** Address or interface name to send packets from */
    const char *interface;
    /** Ping destination address */
    const char *destination;
} tapi_ping_opt;

/** Default options initializer */
extern const tapi_ping_opt tapi_ping_default_opt;

/* RTT statistics report */
typedef struct tapi_ping_rtt_stats {
    /* Minimum value */
    double min;
    /* Average value */
    double avg;
    /* Maximum value */
    double max;
    /* Median deviation */
    double mdev;
} tapi_ping_rtt_stats;

/** Statistics report of ping tool */
typedef struct tapi_ping_report {
    /** Number of transmitted packets */
    unsigned int transmitted;
    /** Number of received packets */
    unsigned int received;
    /** Packet loss (percentage) */
    unsigned int lost_percentage;
    /**
     * @c TRUE if rtt stats were produced by ping tool.
     * @c FALSE if they were not (because the packet_size was too small),
     * in this case the rtt field contains irrelevant data
     * and should not be addressed.
     */
    te_bool with_rtt;
    /** RTT statistics */
    tapi_ping_rtt_stats rtt;
} tapi_ping_report;

/** Information of a ping tool */
typedef struct tapi_ping_app {
    /** TAPI job handle */
    tapi_job_t *job;
    /** Output channel handles */
    tapi_job_channel_t *out_chs[2];
    /* Number of packets transmitted filter */
    tapi_job_channel_t *trans_filter;
    /* Number of packets received filter */
    tapi_job_channel_t *recv_filter;
    /** Percentage of packets lost filter */
    tapi_job_channel_t *lost_filter;
    /** RTT filter*/
    tapi_job_channel_t *rtt_filter;
    /**
     * Number of data to send
     * (required here to check if rtt stats will be produced)
     */
    unsigned int packet_size;
} tapi_ping_app;

/**
 * Create ping app.
 *
 * @param[in]  factory         Job factory
 * @param[in]  opt             ping tool options
 * @param[out] app             ping app handle
 *
 * @return     Status code
 */
extern te_errno tapi_ping_create(tapi_job_factory_t *factory,
                                 const tapi_ping_opt *opt,
                                 tapi_ping_app **app);

/**
 * Start ping tool.
 *
 * @param      app             ping app handle
 *
 * @return     Status code
 */
extern te_errno tapi_ping_start(tapi_ping_app *app);

/**
 * Wait for ping tool completion.
 *
 * @param      app             ping app handle
 * @param      timeout_ms      Wait timeout in milliseconds
 *
 * @return     Status code
 * @retval     TE_EINPROGRESS  ping is still running
 */
extern te_errno tapi_ping_wait(tapi_ping_app *app, int timeout_ms);

/**
 * Send a signal to ping tool.
 *
 * @param      app             ping app handle
 * @param      signum          Signal to send
 *
 * @return     Status code
 */
extern te_errno tapi_ping_kill(tapi_ping_app *app, int signum);

/**
 * Stop ping tool. It can be started over with tapi_ping_start().
 *
 * @param      app             ping app handle
 *
 * @return     Status code
 * @retval     TE_EPROTO       ping tool is stopped, but report is unavailable
 */
extern te_errno tapi_ping_stop(tapi_ping_app *app);

/**
 * Destroy ping app. The app cannot be used after calling this function.
 *
 * @param      app             ping app handle
 *
 * @return     Status code
 */
extern te_errno tapi_ping_destroy(tapi_ping_app *app);

/**
 * Get ping tool report.
 *
 * @param[in]  app             ping app handle
 * @param[out] report          ping report handle
 *
 * @return     Status code
 */
extern te_errno tapi_ping_get_report(tapi_ping_app *app,
                                     tapi_ping_report *report);

/**
 * Add ping tool report to MI logger.
 *
 * @param      logger          MI logger entity
 * @param      report          ping tool report
 */
extern void tapi_ping_report_mi_log(te_mi_logger *logger,
                                    tapi_ping_report *report);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __TE_TAPI_PING_H__ */

/** @} <!-- END tapi_ping --> */
