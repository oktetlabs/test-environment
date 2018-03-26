/** @file
 * @brief Test API for FIO tool
 *
 * @defgroup tapi_fio FIO tool
 * @ingroup te_ts_tapi
 * @{
 *
 * Test API to control 'fio' tool.
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * @author Nikita Somenkov <Nikita.Somenkov@oktetlabs.ru>
 */

#ifndef __TAPI_FIO_H__
#define __TAPI_FIO_H__

#include "te_errno.h"
#include "te_defs.h"
#include "te_string.h"
#include "rcf_rpc.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Estimate timeout from input FIO parameters */
#define TAPI_FIO_TIMEOUT_DEFAULT    (-1)

/** FIO test tool report. */
typedef struct tapi_fio_report {
    double bandwidth;   /**< Average data rate */
    double latency;     /**< I/O latency */
    int threads;        /**< Count of thread */
} tapi_fio_report;

/** List of possible IO engines to use. */
typedef enum {
    TAPI_FIO_IOENGINE_SYNC,      /**< Use read/write */
    TAPI_FIO_IOENGINE_PSYNC,     /**< Use pread/pwrite */
    TAPI_FIO_IOENGINE_LIBAIO,    /**< Use Kernel Asynchronous I/O */
    TAPI_FIO_IOENGINE_POSIXAIO,  /**< Use POSIX asynchronous IO */
} tapi_fio_ioengine;

#define TAPI_FIO_IOENGINE_MAPPING_LIST \
    {"sync", TAPI_FIO_IOENGINE_SYNC},        \
    {"psync", TAPI_FIO_IOENGINE_PSYNC},      \
    {"libaio", TAPI_FIO_IOENGINE_LIBAIO},    \
    {"posixaio", TAPI_FIO_IOENGINE_POSIXAIO}

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
    TAPI_FIO_RWTYPE_RAND,     /**< Random read/write */
    TAPI_FIO_RWTYPE_SEQ       /**< Sequential read/write */
} tapi_fio_rwtype;

#define TAPI_FIO_RWTYPE_MAPPING_LIST \
    {"rand", TAPI_FIO_RWTYPE_RAND},     \
    {"seq", TAPI_FIO_RWTYPE_SEQ}

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

/** Multiplication factor for numjobs */
typedef enum tapi_fio_numjobs_factor {
    TAPI_FIO_NUMJOBS_NPROC_FACTOR,      /**< Multiply value on CPU Cores */
    TAPI_FIO_NUMJOBS_WITHOUT_FACTOR     /**< No multiply */
};

/** Contain FIO numjobs */
typedef struct tapi_fio_numjobs_t {
    unsigned int value;                     /**< Number of jobs */
    enum tapi_fio_numjobs_factor factor;    /**< Type of multiply */
} tapi_fio_numjobs_t;

/**
 * Get the value of parameter of type 'tapi_fio_numjobs_t'
 * @note Support format [number]n | <number>, where n - count of cores on TA,
 * input example: 2n, n, 42
 *
 * @param var_name_  Name of the parameter
 *
 * @return value of parameter of type 'tapi_fio_numjobs_t'
 */
#define TEST_FIO_NUMJOBS_PARAM(var_name_) \
    test_get_fio_numjobs_param(argc, argv, #var_name_)

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
    int runtime_ms;              /**< Stop workload when the time expires */
    unsigned int rwmixread;      /**< Percentage of mixed workload that is
                                      reads */
    tapi_fio_rwtype rwtype;      /**< Read or write type */
    tapi_fio_ioengine ioengine;  /**< I/O Engine type */
    const char *user;            /**< Raw string passed to fio */
} tapi_fio_opts;

/** Macro to initialize default value. */
#define TAPI_FIO_OPTS_DEFAULTS ((struct tapi_fio_opts) { \
    .name = NULL,                                       \
    .filename = NULL,                                   \
    .blocksize = 512,                                   \
    .numjobs = {                                        \
        .value = 1,                                     \
        .factor = TAPI_FIO_NUMJOBS_WITHOUT_FACTOR,      \
    },                                                  \
    .iodepth = 1,                                       \
    .runtime_ms = 0,                                    \
    .rwmixread = 50,                                    \
    .rwtype = TAPI_FIO_RWTYPE_SEQ,                      \
    .ioengine = TAPI_FIO_IOENGINE_SYNC,                 \
    .user = "",                                         \
})

/** FIO tool context. Based on tapi_perf_app */
typedef struct tapi_fio_app {
    tapi_fio_opts opts;     /**< Tool's options */
    rcf_rpc_server *rpcs;   /**< RPC handle */
    tarpc_pid_t pid;        /**< PID */
    int fd_stdout;          /**< File descriptor to read from stdout stream */
    int fd_stderr;          /**< File descriptor to read from stderr stream */
    char *cmd;              /**< Command line string to run the application */
    te_string stdout;       /**< Buffer to save tool's stdout message */
    te_string stderr;       /**< Buffer to save tool's stderr message */
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
    tapi_fio_app app;                   /**< Tool context */
    const tapi_fio_methods *methods;    /**< Methods to operate FIO */
} tapi_fio;

/**
 * Get parameters of type 'tapi_fio_numjobs_t'
 * @note Support format [number]n | <number>, where n - count of cores on TA,
 * input example: 2n, n, 42
 *
 * @param argc       Count of arguments
 * @param argv       List of arguments
 * @param name       Name of parameter
 *
 * @return value of parameter of type 'tapi_fio_numjobs_t'
 */
extern tapi_fio_numjobs_t test_get_fio_numjobs_param(int argc, char **argv,
                                                     const char *name);

/**
 * Create FIO context based on options.
 * @note Never return NULL, test failed if error.
 *
 * @param options       FIO options
 * @param rpcs          RPC handle
 *
 * @return Initialization created fio.
 *
 * @sa tapi_fio_destroy
 */
extern tapi_fio * tapi_fio_create(const tapi_fio_opts *options,
                                  rcf_rpc_server *rpcs);

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

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __TAPI_FIO_H__ */

/**@} <!-- END tapi_fio --> */