/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2004-2023 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief Test API for FIO tool
 *
 * @defgroup tapi_fio FIO tool
 * @ingroup te_ts_tapi
 * @{
 *
 * Test API to control 'fio' tool.
 */

#ifndef __TAPI_FIO_H__
#define __TAPI_FIO_H__

#include "te_errno.h"
#include "te_defs.h"
#include "te_string.h"
#include "te_queue.h"
#include "tapi_job.h"
#include "te_vector.h"
#include "te_mi_log.h"
#include "tapi_job_opt.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Estimate timeout from input FIO parameters */
#define TAPI_FIO_TIMEOUT_DEFAULT    (-1)

/** FIO report max size */
#define TAPI_FIO_MAX_REPORT (10 * 1024 * 1024)

/** Latency FIO report */
typedef struct tapi_fio_report_lat
{
    int min_ns;         /**< Minimal latency in nanoseconds */
    int max_ns;         /**< Maximum latency in nanoseconds */
    double mean_ns;     /**< Latency mean in nanoseconds */
    double stddev_ns;   /**< Latency standard deviation in nanoseconds */
} tapi_fio_report_lat;

/** Bandwidth FIO report */
typedef struct tapi_fio_report_bw
{
    int min;          /**< Minimal bandwidth */
    int max;          /**< Maximal bandwidth */
    double mean;      /**< Bandwidth mean */
    double stddev;    /**< Bandwidth standard deviation */
} tapi_fio_report_bw;

/** Latency percentiles */
typedef struct tapi_fio_report_percentiles {
    int percent_99_00; /**< 99.00 percentile */
    int percent_99_50; /**< 99.50 percentile */
    int percent_99_90; /**< 99.90 percentile */
    int percent_99_95; /**< 99.95 percentile */
} tapi_fio_report_percentiles;

/** Completion latency FIO report */
typedef struct tapi_fio_report_clat {
    int min_ns;                              /**< Minimal completion latency
                                                  in nanoseconds */
    int max_ns;                              /**< Maximum completion latency
                                                  in nanoseconds */
    double mean_ns;                          /**< Completion latency mean
                                                  in nanoseconds */
    double stddev_ns;                        /**< Completion latency standard
                                                  deviation in nanoseconds */
    tapi_fio_report_percentiles percentiles; /**< Latency percentiles */
} tapi_fio_report_clat;

/** IOPS FIO report */
typedef struct tapi_fio_report_iops {
    int min;          /**< Minimal iops */
    int max;          /**< Maximal iops */
    double mean;      /**< iops mean */
    double stddev;    /**< iops standard deviation */
} tapi_fio_report_iops;

/** FIO reports */
typedef struct tapi_fio_report_io
{
    tapi_fio_report_lat latency;
    tapi_fio_report_clat clatency;
    tapi_fio_report_bw bandwidth;
    tapi_fio_report_iops iops;
} tapi_fio_report_io;

/** FIO test tool report. */
typedef struct tapi_fio_report {
    tapi_fio_report_io read;    /**< Read FIO report */
    tapi_fio_report_io write;   /**< Write FIO report */
} tapi_fio_report;

/** List of possible IO engines to use. */
typedef enum {
    TAPI_FIO_IOENGINE_SYNC,      /**< Use read/write */
    TAPI_FIO_IOENGINE_PSYNC,     /**< Use pread/pwrite */
    TAPI_FIO_IOENGINE_VSYNC,     /**< Use readv/writev */
    TAPI_FIO_IOENGINE_PVSYNC,    /**< Use preadv/pwritev */
    TAPI_FIO_IOENGINE_PVSYNC2,   /**< Use preadv2/pwritev2 */
    TAPI_FIO_IOENGINE_LIBAIO,    /**< Use Kernel Asynchronous I/O */
    TAPI_FIO_IOENGINE_POSIXAIO,  /**< Use POSIX asynchronous IO */
    TAPI_FIO_IOENGINE_RBD,       /**< I/O engine supporting direct access
                                      to Ceph Rados Block Devices */
} tapi_fio_ioengine;

#define TAPI_FIO_IOENGINE_MAPPING_LIST \
    {"sync", TAPI_FIO_IOENGINE_SYNC},         \
    {"psync", TAPI_FIO_IOENGINE_PSYNC},       \
    {"vsync", TAPI_FIO_IOENGINE_VSYNC},       \
    {"pvsync", TAPI_FIO_IOENGINE_PVSYNC},     \
    {"pvsync2", TAPI_FIO_IOENGINE_PVSYNC2},   \
    {"libaio", TAPI_FIO_IOENGINE_LIBAIO},     \
    {"posixaio", TAPI_FIO_IOENGINE_POSIXAIO}, \
    {"rbd", TAPI_FIO_IOENGINE_RBD}

/**
 * Get the value of parameter of type 'tapi_fio_ioengine'
 *
 * @param var_name_  Name of the parameter
 *
 * @return value of parameter of type 'tapi_fio_ioengine'
 */
#define TEST_FIO_IOENGINE_PARAM(var_name_) \
    TEST_ENUM_PARAM(var_name_, TAPI_FIO_IOENGINE_MAPPING_LIST)

/**
 * Get test parameter ioengine
 *
 * @sa TEST_FIO_IOENGINE_PARAM
 */
#define TEST_GET_FIO_IOENGINE_PARAM(var_name_) \
    (var_name_) = TEST_FIO_IOENGINE_PARAM(var_name_)

/** Support IO direction */
typedef enum {
    TAPI_FIO_RWTYPE_RAND,       /**< Random read/write */
    TAPI_FIO_RWTYPE_SEQ,        /**< Sequential read/write */
    TAPI_FIO_RWTYPE_READ,       /**< Sequential reads */
    TAPI_FIO_RWTYPE_WRITE,      /**< Sequential writes */
    TAPI_FIO_RWTYPE_TRIM,       /**< Sequential trims */
    TAPI_FIO_RWTYPE_RANDREAD,   /**< Random reads */
    TAPI_FIO_RWTYPE_RANDWRITE,  /**< Random writes */
    TAPI_FIO_RWTYPE_RANDTRIM,   /**< Random trims */
    TAPI_FIO_RWTYPE_TRIMWRITE,  /**< Sequential trim+write sequences */
} tapi_fio_rwtype;

/** List of values allowed for parameter of type 'tapi_fio_rwtype'. */
#define TAPI_FIO_RWTYPE_MAPPING_LIST \
    {"rand", TAPI_FIO_RWTYPE_RAND},             \
    {"seq", TAPI_FIO_RWTYPE_SEQ},               \
    {"read", TAPI_FIO_RWTYPE_READ},             \
    {"write", TAPI_FIO_RWTYPE_WRITE},           \
    {"trim", TAPI_FIO_RWTYPE_TRIM},             \
    {"randread", TAPI_FIO_RWTYPE_RANDREAD},     \
    {"randwrite", TAPI_FIO_RWTYPE_RANDWRITE},   \
    {"randtrim", TAPI_FIO_RWTYPE_RANDTRIM},     \
    {"trimwrite", TAPI_FIO_RWTYPE_TRIMWRITE}

/**
 * Get the value of parameter of type 'tapi_fio_rwtype'
 *
 * @param var_name_  Name of the parameter
 *
 * @return value of parameter of type 'tapi_fio_rwtype'
 */
#define TEST_FIO_RWTYPE_PARAM(var_name_) \
    TEST_ENUM_PARAM(var_name_, TAPI_FIO_RWTYPE_MAPPING_LIST)

/**
 * Get test parameter rwtype
 *
 * @sa TEST_FIO_RWTYPE_PARAM
 */
#define TEST_GET_FIO_RWTYPE_PARAM(var_name_) \
    (var_name_) = TEST_FIO_RWTYPE_PARAM(var_name_)

/** The maximum value of numjobs, used to estimate the timeout */
#define TAPI_FIO_MAX_NUMJOBS    (1024)


/**
 * Contain FIO numjobs.
 *
 * @note numjobs.expr is used to calculate the actual number of jobs if it is
 *       not @c NULL, otherwise numjobs.value is used.
*/
typedef struct tapi_fio_numjobs_t {
    const char *expr;   /**< Expression to calculate the number of jobs. */
    unsigned int value; /**< Number of jobs. */
} tapi_fio_numjobs_t;

/**
 * The syntax of the parameter is an expression accepted by
 * tapi_cfg_cpu_calculate_numjobs().
 *
 * @param var_name_  Name of the parameter
 *
 * @return value of parameter of type 'tapi_fio_numjobs_t'
 */
#define TEST_FIO_NUMJOBS_PARAM(var_name_) \
    (tapi_fio_numjobs_t){                     \
        .expr = TEST_STRING_PARAM(var_name_), \
        .value = 0,                           \
    }

/**
 * Get test parameter numjobs
 *
 * @sa TEST_FIO_NUMJOBS_PARAM
 */
#define TEST_GET_FIO_NUMJOBS_PARAM(var_name_) \
    (var_name_) = TEST_FIO_NUMJOBS_PARAM(var_name_)

/** List of possible FIO test tool options. */
typedef struct tapi_fio_opts {
    const char *name;            /**< Test name */
    const char *filename;        /**< Files to use for the workload */
    unsigned int blocksize;      /**< Block size unit */
    tapi_fio_numjobs_t numjobs;  /**< Duplicate this job many times */
    unsigned int iodepth;        /**< Number of IO buffers to keep in flight */
    int runtime_sec;             /**< Stop workload when the time expires */
    unsigned int rwmixread;      /**< Percentage of mixed workload that is
                                      reads */
    tapi_fio_rwtype rwtype;      /**< Read or write type */
    tapi_fio_ioengine ioengine;  /**< I/O Engine type */
    te_string output_path;       /**< File name where store FIO result */
    bool direct;              /**< Use O_DIRECT I/O */
    bool scramble_buffers;    /**< Use random data for every I/O */
    bool exit_on_error;       /**< Terminate all jobs when one exits in
                                    * error */
    const char *rand_gen;        /**< Random generator type */
    const char *user;            /**< Raw string passed to fio */
    const char *prefix;          /**< Raw string used as a prefix before fio */
    const char *rbdname;         /**< Specifies the name of the RBD */
    const char *pool;            /**< Specifies the name of the Ceph pool
                                      containing RBD or RADOS data. */
    tapi_job_opt_uintmax_t size; /**< The total size of file I/O for
                                      each thread of this job */
} tapi_fio_opts;

/** Macro to initialize default value. */
#define TAPI_FIO_OPTS_DEFAULTS ((struct tapi_fio_opts) { \
    .name = "default.fio",                              \
    .filename = NULL,                                   \
    .blocksize = 512,                                   \
    .numjobs = {.expr = NULL, .value = 1},              \
    .iodepth = 1,                                       \
    .runtime_sec = 0,                                   \
    .rwmixread = 50,                                    \
    .rwtype = TAPI_FIO_RWTYPE_SEQ,                      \
    .ioengine = TAPI_FIO_IOENGINE_SYNC,                 \
    .output_path = TE_STRING_INIT,                      \
    .direct = true,                                     \
    .scramble_buffers = true,                           \
    .exit_on_error = true,                              \
    .rand_gen = NULL,                                   \
    .user = NULL,                                       \
    .prefix = NULL,                                     \
    .rbdname = NULL,                                    \
    .pool = NULL,                                       \
    .size = TAPI_JOB_OPT_UINTMAX_UNDEF,                 \
})

/** FIO tool context. Based on tapi_perf_app */
typedef struct tapi_fio_app {
    tapi_job_factory_t *factory; /**< Factory to create the job */
    te_string path; /** Path to fio tool */
    bool running; /**< Is the app running */
    tapi_job_t *job; /**< TAPI job handle */
    tapi_job_channel_t *out_chs[2]; /**< Output channel handles */
    tapi_fio_opts opts; /**< Tool's options */
    te_vec args; /**< Arguments that are used when running the tool */
} tapi_fio_app;

typedef struct tapi_fio tapi_fio;

/**
 * Method to start FIO. Call from tapi_fio_start().
 *
 * @param fio      FIO context
 *
 * @return Status code.
 * @retval 0               No errors.
 * @retval TE_EOPNOTSUPP   If FIO control structure not valid.
 *
 * @sa tapi_fio_start
 */
typedef te_errno (*tapi_fio_method_start)(tapi_fio *fio);

/**
 * Method wait certain time for FIO to complete.
 *
 * @param fio           FIO context
 * @param timeout_sec   Timeout in seconds
 *
 * @return Status code.
 * @retval 0               No errors.
 * @retval TE_EOPNOTSUPP   If FIO control structure not valid.
 * @retval TE_ETIMEDOUT    If timeout is expired.
 *
 * @sa tapi_fio_wait
 */
typedef te_errno (*tapi_fio_method_wait)(tapi_fio *fio,
                                         int16_t timeout_sec);

/**
 * Method for stop FIO.
 *
 * @param fio      FIO context
 *
 * @return Status code.
 * @retval 0               No errors.
 * @retval TE_EOPNOTSUPP   If FIO control structure not valid.
 *
 * @sa tapi_fio_stop
 */
typedef te_errno (*tapi_fio_method_stop)(tapi_fio *fio);


/**
 * Generate report from fio output.
 *
 * @param fio      FIO context
 * @param report   FIO test tool report
 *
 * @return Status code.
 * @retval 0               No errors.
 * @retval TE_EOPNOTSUPP   If FIO control structure not valid.
 *
 * @sa tapi_fio_get_report
 */
typedef te_errno (*tapi_fio_method_get_report)
        (tapi_fio *fio, tapi_fio_report *report);

/** Methods to operate FIO test tool. */
typedef struct tapi_fio_methods {
    tapi_fio_method_start       start;          /**< Method to start */
    tapi_fio_method_stop        stop;           /**< Method to stop */
    tapi_fio_method_wait        wait;           /**< Method to wait */
    tapi_fio_method_get_report  get_report;     /**< Method to get report */
} tapi_fio_methods;

/** FIO tool context */
typedef struct tapi_fio {
    LIST_ENTRY(tapi_fio) list;          /**< A way to put FIO into lists  */
    tapi_fio_app app;                   /**< Tool context */
    const tapi_fio_methods *methods;    /**< Methods to operate FIO */
} tapi_fio;

/**
 * Create FIO context based on options.
 * @note Never return NULL, test failed if error.
 *
 * @param options       FIO options
 * @param factory       Job factory
 * @param path          Path to fio tool.
 *                      May be NULL if no special path desired.
 *
 * @return Initialization created fio.
 *
 * @sa tapi_fio_destroy
 */
extern tapi_fio * tapi_fio_create(const tapi_fio_opts *options,
                                  tapi_job_factory_t *factory,
                                  const char *path);

/**
 * Destroy FIO context.
 *
 * @param fio          what destroy
 *
 * @sa tapi_fio_create
 */
extern void tapi_fio_destroy(tapi_fio *fio);

/**
 * Initialize default value.
 *
 * @param options          fio options
 */
extern void tapi_fio_opts_init(tapi_fio_opts *options);

/**
 * Start FIO.
 *
 * @param fio      FIO context
 *
 * @return Status code.
 * @retval 0               No errors.
 * @retval TE_EOPNOTSUPP   If FIO control structure not valid.
 */
extern te_errno tapi_fio_start(tapi_fio *fio);

/**
 * Wait certain time for FIO to complete.
 *
 * @param fio           FIO context
 * @param timeout_sec   Timeout in seconds
 *
 * @return Status code.
 * @retval 0               No errors.
 * @retval TE_EOPNOTSUPP   If FIO control structure not valid.
 */
extern te_errno tapi_fio_wait(tapi_fio *fio, int16_t timeout_sec);

/**
 * Kill FIO execution.
 *
 * @param fio      FIO context
 *
 * @return Status code.
 * @retval 0               No errors.
 * @retval TE_EOPNOTSUPP   If FIO control structure not valid.
 */
extern te_errno tapi_fio_stop(tapi_fio *fio);

/**
 * Dump report in TE log
 *
 * @param fio         FIO context
 * @param report      FIO report structure
 *
 * @return Status code.
 * @retval 0                  No errors.
 * @retval TE_EOPNOTSUPP      If FIO control structure not valid.
 */
extern te_errno tapi_fio_get_report(tapi_fio *fio,
                                    tapi_fio_report *report);

/**
 * Log report to TE log
 *
 * @param report      report to log
 */
extern void tapi_fio_log_report(tapi_fio_report *report);

/**
 * Add FIO report to MI logger
 *
 * @param logger   MI logger
 * @param report   FIO report structure
 *
 * @return Status code
 */
extern te_errno tapi_fio_mi_report(te_mi_logger *logger,
                                   const tapi_fio_report *report);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __TAPI_FIO_H__ */

/**@} <!-- END tapi_fio --> */
