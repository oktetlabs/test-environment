/** @file
 * @brief Generic Test API to network throughput test tools
 *
 * @defgroup tapi_performance Test API to control a network throughput test tool
 * @ingroup te_ts_tapi
 * @{
 *
 * Generic high level test API to control a network throughput test tool.
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 *
 *
 * @section tapi_performance_notes Notes
 *
 * @note Throughput value should be obtained from receiver (usually it is
 * a server instance until it is changed by traffic direction options)
 *
 * @note You have to restart a server along with a client if you need to
 * perform a few measurements in a row, otherwise the server returns a report
 * of the first measurement all the time (until restarting or destroying)
 *
 * @section tapi_performance_example Example of usage
 *
 * Lets assume we need to send @attr_val{UDP} traffic with iperf.
 * We need to check the result throughput with the following input options:
 * @attr_name{total bandwidth} = @attr_val{1000 Mbit/s}, @attr_name{streams} =
 * @attr_val{5}, and @attr_name{test duration} = @attr_val{60 sec}. Server
 * iperf should use any free port on the host, and its host address is
 * @attr_val{192.168.1.1}.
 *
 * So, we have the following commands for both server and client:
 *
 * @cmd{iperf -s -u -p 60000}
 *
 * @cmd{iperf -c 192.168.1.1 -p 60000 -u -b 200 -P 5 -t 60}
 *
 * @code
 * #include "tapi_job.h"
 * #include "tapi_job_factory_rpc.h"
 * #include "tapi_performance.h"
 *
 * int main(int argc, char *argv[])
 * {
 *     tapi_perf_server    *perf_server = NULL;
 *     tapi_perf_client    *perf_client = NULL;
 *     tapi_perf_opts       perf_opts;
 *     tapi_perf_report     perf_server_report;
 *     tapi_perf_report     perf_client_report;
 *     rcf_rpc_server      *perf_server_rpcs;
 *     rcf_rpc_server      *perf_client_rpcs;
 *     tapi_job_factory_t  *client_factory = NULL;
 *     tapi_job_factory_t  *server_factory = NULL;
 *
 *     TEST_START;
 *
 *     // Set default perf options
 *     tapi_perf_opts_init(&perf_opts);
 *
 *     // Set test specific perf options
 *     perf_opts.host = "192.168.1.1";
 *     perf_opts.protocol = IPPROTO_UDP;
 *     perf_opts.port = tapi_get_port(perf_server_rpcs);
 *     perf_opts.streams = 5;
 *     perf_opts.bandwidth_bits = TE_UNITS_DEC_M2U(1000) / perf_opts.streams;
 *     perf_opts.duration_sec = 60;
 *     // To force server to print a report at the end of test even if it lost
 *     // connection with client (iperf tool issue, Bug 9714)
 *     perf_opts.interval_sec = perf_opts.duration_sec;
 *
 *     CHECK_RC(tapi_job_factory_rpc_create(perf_server_rpcs, &server_factory));
 *     CHECK_RC(tapi_job_factory_rpc_create(perf_client_rpcs, &client_factory));
 *     perf_server = tapi_perf_server_create(TAPI_PERF_IPERF, &perf_opts,
 *                                           server_factory);
 *     perf_client = tapi_perf_client_create(TAPI_PERF_IPERF, &perf_opts,
 *                                           client_factory);
 *     CHECK_RC(tapi_perf_server_start(perf_server));
 *     CHECK_RC(tapi_perf_client_start(perf_client));
 *     CHECK_RC(tapi_perf_client_wait(perf_client, TAPI_PERF_TIMEOUT_DEFAULT));
 *     // Time is relative and goes differently on different hosts.
 *     // Sometimes we need to wait for a few moments until report is ready.
 *     VSLEEP(1, "ensure perf server has printed its report");
 *
 *     CHECK_RC(tapi_perf_client_get_dump_check_report(perf_client, "client",
 *                                                     &perf_client_report));
 *     CHECK_RC(tapi_perf_server_get_dump_check_report(perf_server, "server",
 *                                                     &perf_server_report));
 *
 *     // Analyze the obtained reports
 *     ...
 *
 *     TEST_SUCCESS;
 *
 * cleanup:
 *     tapi_perf_client_destroy(perf_client);
 *     tapi_perf_server_destroy(perf_server);
 *     tapi_job_factory_destroy(client_factory);
 *     tapi_job_factory_destroy(server_factory);
 *     TEST_END;
 * }
 * @endcode
 */

#ifndef __TAPI_PERFORMANCE_H__
#define __TAPI_PERFORMANCE_H__

#include "te_errno.h"
#include "rcf_rpc.h"
#include "te_string.h"
#include "tapi_job.h"
#include "te_vector.h"


#ifdef __cplusplus
extern "C" {
#endif

/**
 * Default timeout to client wait method.
 * It means the real timeout will be calculated according to tool's options.
 */
#define TAPI_PERF_TIMEOUT_DEFAULT   (-1)

/**
 * Disable periodic bandwidth reports.
 */
#define TAPI_PERF_INTERVAL_DISABLED (-1)

/**
 * Supported network throughput test tools list.
 */
typedef enum tapi_perf_bench {
    TAPI_PERF_IPERF,        /**< iperf tool */
    TAPI_PERF_IPERF3,       /**< iperf3 tool */
} tapi_perf_bench;

/**
 * The list of values allowed for parameter of type 'tapi_perf_bench'
 */
#define TAPI_PERF_BENCH_MAPPING_LIST    \
    { "iperf",    TAPI_PERF_IPERF },    \
    { "iperf3",   TAPI_PERF_IPERF3 }

/**
 * Get the value of parameter of type 'tapi_perf_bench'
 *
 * @param var_name_  Name of the variable used to get the value of
 *                   "var_name_" parameter of type 'tapi_perf_bench' (OUT)
 */
#define TEST_GET_PERF_BENCH(var_name_) \
    TEST_GET_ENUM_PARAM(var_name_, TAPI_PERF_BENCH_MAPPING_LIST)


/**
 * List of possible network throughput test tool errors.
 */
typedef enum tapi_perf_error {
    TAPI_PERF_ERROR_FORMAT,     /**< Wrong report format. */
    TAPI_PERF_ERROR_READ,       /**< Read failed. */
    TAPI_PERF_ERROR_WRITE_CONN_RESET, /**< Write failed. Connection reset. */
    TAPI_PERF_ERROR_CONNECT,    /**< Connect failed. */
    TAPI_PERF_ERROR_NOROUTE,    /**< No route to host. */
    TAPI_PERF_ERROR_BIND,       /**< Bind failed. */
    TAPI_PERF_ERROR_SOCKET_CLOSED,  /**< Socket closed unexpectedly. */
    TAPI_PERF_ERROR_MAX,        /**< Not error, but elements number. */
} tapi_perf_error;

/**
 * List of possible report kinds.
 */
typedef enum tapi_perf_report_kind {
    TAPI_PERF_REPORT_KIND_DEFAULT,   /**< Specific default report kind */
    TAPI_PERF_REPORT_KIND_SENDER,    /**< Sender's report */
    TAPI_PERF_REPORT_KIND_RECEIVER,  /**< Receiver's report */
} tapi_perf_report_kind;

/**
 * Network throughput test tool report.
 */
typedef struct tapi_perf_report {
    uint64_t bytes;         /**< Number of bytes was transmitted */
    double seconds;         /**< Number of seconds was expired during test */
    double bits_per_second; /**< Throughput */
    size_t zero_intervals;  /**< Number of zero intervals */
    double min_bps_per_stream;  /**< Minimal rate observed for any stream at the
                                     end of report */
    uint32_t errors[TAPI_PERF_ERROR_MAX];  /**< Errors counters. */
} tapi_perf_report;


/* Forward declaration of network throughput test server tool */
struct tapi_perf_server;
typedef struct tapi_perf_server tapi_perf_server;
typedef struct tapi_perf_opts tapi_perf_opts;

/**
 * Build command string to run server tool.
 *
 * @param args          List of built commands line arguments.
 * @param options       Tool server options.
 */
typedef void (* tapi_perf_server_method_build_args)(te_vec *args,
                                                 const tapi_perf_opts *options);

/**
 * Get server report. The function reads client output (stdout, stderr).
 *
 * @param[in]  server       Server context.
 * @param[out] report       Report with results.
 *
 * @return Status code.
 */
typedef te_errno (* tapi_perf_server_method_get_report)(
                                                tapi_perf_server     *server,
                                                tapi_perf_report_kind kind,
                                                tapi_perf_report     *report);

/**
 * Methods to operate the server network throughput test tool.
 */
typedef struct tapi_perf_server_methods {
    tapi_perf_server_method_build_args build_args;
    tapi_perf_server_method_get_report get_report;
} tapi_perf_server_methods;


/* Forward declaration of network throughput test client tool */
struct tapi_perf_client;
typedef struct tapi_perf_client tapi_perf_client;

/**
 * Build command string to run client tool.
 *
 * @param args          List of built commands line arguments.
 * @param options       Tool client options.
 */
typedef void (* tapi_perf_client_method_build_args)(te_vec *args,
                                                 const tapi_perf_opts *options);

/**
 * Wait while client finishes his work. Note, function jumps to cleanup if
 * timeout is expired.
 *
 * @param client        Client context.
 * @param timeout       Time to wait for client results (seconds). It MUST be
 *                      big enough to finish client normally (it depends
 *                      on client's options), otherwise the function will be
 *                      failed. Use @ref TAPI_PERF_TIMEOUT_DEFAULT to coerce
 *                      the function to calculate the required timeout value.
 *
 * @return Status code.
 */
typedef te_errno (* tapi_perf_client_method_wait)(tapi_perf_client *client,
                                                  int16_t timeout);

/**
 * Get client report. The function reads client output (stdout, stderr).
 *
 * @param[in]  client       Client context.
 * @param[out] report       Report with results.
 *
 * @return Status code.
 */
typedef te_errno (* tapi_perf_client_method_get_report)(
                                                tapi_perf_client     *client,
                                                tapi_perf_report_kind kind,
                                                tapi_perf_report     *report);

/**
 * Methods to operate the client network throughput test tool.
 */
typedef struct tapi_perf_client_methods {
    tapi_perf_client_method_build_args build_args;
    tapi_perf_client_method_wait       wait;
    tapi_perf_client_method_get_report get_report;
} tapi_perf_client_methods;


/**
 * Network throughput test tool options
 */
typedef struct tapi_perf_opts {
    char *host;             /**< Destination host (server) */
    char *src_host;         /**< Source host (client) */
    int port;               /**< Port to listen on/connect to */
    rpc_socket_proto ipversion;     /**< IP version */
    rpc_socket_proto protocol;      /**< Transport protocol */
    int64_t bandwidth_bits; /**< Target bandwidth (bits/sec) */
    int64_t num_bytes;      /**< Number of bytes to transmit (instead of time) */
    int32_t duration_sec;   /**< Time in seconds to transmit for */
    int32_t interval_sec;   /**< Pause in seconds between periodic bandwidth
                             *   reports. Warning! It can affect report
                             *   processing */
    int32_t length;         /**< Length of buffer to read or write */
    int16_t streams;        /**< Number of parallel client streams */
    te_bool reverse;        /**< Whether run in reverse mode (server sends,
                                 client receives), or not */
    te_bool dual;           /**< Bidirectional mode */
} tapi_perf_opts;

/**
 * Network throughput test tool context (common for both server and client)
 */
typedef struct tapi_perf_app {
    tapi_perf_bench bench;              /**< Tool's sort */
    tapi_perf_opts opts;                /**< Tool's options */
    const tapi_job_factory_t *factory;  /**< RPC server handle */
    tapi_job_t *job;                    /**< Agent job control */
    tapi_job_channel_t *out_filter;     /**< Filters of stdout message */
    tapi_job_channel_t *err_filter;     /**< Filters of stderr message */
    te_string stdout;                   /**< Buffer to save tool's stdout message */
    te_string stderr;                   /**< Buffer to save tool's stderr message */
    char *cmd;                          /**< Command line string to run the application */
} tapi_perf_app;

/**
 * Network throughput test server tool context
 */
typedef struct tapi_perf_server {
    tapi_perf_app app;      /**< Tool context */
    const tapi_perf_server_methods *methods; /**< Methods to operate the tool */
} tapi_perf_server;

/**
 * Network throughput test client tool context
 */
typedef struct tapi_perf_client {
    tapi_perf_app app;      /**< Tool context */
    const tapi_perf_client_methods *methods; /**< Methods to operate the tool */
} tapi_perf_client;


/**
 * Initialize options with default values (from point of view of perf tool).
 *
 * @param opts             Network throughput test tool options.
 */
extern void tapi_perf_opts_init(tapi_perf_opts *opts);

/**
 * Create server network throughput test tool proxy.
 *
 * @param bench             Sort of tool, see @ref tapi_perf_bench to get a list
 *                          of supported tools.
 * @param options           Server tool specific options, may be @c NULL, to set
 *                          them to default, further you can edit them using
 *                          return value.
 *
 * @return Status code.
 *
 * @sa tapi_perf_server_destroy
 */
extern tapi_perf_server *tapi_perf_server_create(tapi_perf_bench bench,
                                                 const tapi_perf_opts *options,
                                                 tapi_job_factory_t *factory);

/**
 * Destroy server network throughput test tool proxy.
 *
 * @param server            Server context.
 *
 * @sa tapi_perf_server_create
 */
extern void tapi_perf_server_destroy(tapi_perf_server *server);

/**
 * Start perf server. It returns immediately after run the command starting the
 * server. It can be unreliable to call tapi_perf_client_start() just after this
 * function because server can not be ready to accept clients by this time,
 * especially on slow machine. It is recommended to use this function only if
 * there will be some delay before starting a client, otherwise use
 * tapi_perf_server_start() instead.
 *
 * @param server            Server context.
 *
 * @return Status code.
 *
 * @sa tapi_perf_server_start, tapi_perf_server_stop
 */
extern te_errno tapi_perf_server_start_unreliable(tapi_perf_server *server);

/**
 * Start perf server "reliably". It calls tapi_perf_server_start_unreliable()
 * and wait until it is ready to accept clients. Note, it is not true reliable
 * because it doesn't check whether server is ready, or not, it just waits for
 * some time.
 *
 * @param server            Server context.
 *
 * @return Status code.
 *
 * @sa tapi_perf_server_start_unreliable, tapi_perf_server_stop
 */
extern te_errno tapi_perf_server_start(tapi_perf_server *server);

/**
 * Stop perf server.
 *
 * @param server            Server context.
 *
 * @return Status code.
 *
 * @sa tapi_perf_server_start
 */
extern te_errno tapi_perf_server_stop(tapi_perf_server *server);

/**
 * Get server report. The function reads server output (stdout, stderr).
 *
 * @param[in]  server       Server context.
 * @param[out] report       Report with results.
 *
 * @return Status code.
 */
extern te_errno tapi_perf_server_get_report(tapi_perf_server *server,
                                            tapi_perf_report *report);

/**
 * Get server report of specified kind. The function reads server
 * output (stdout, stderr).
 *
 * @param[in]  server       Server context.
 * @param[in]  kind         Report kind, e.g. default or receiver's,
 *                          or sender's.
 * @param[out] report       Report with results.
 *
 * @return Status code.
 */
extern te_errno tapi_perf_server_get_specific_report(tapi_perf_server *server,
    tapi_perf_report_kind kind, tapi_perf_report *report);

/**
 * Check server report for errors. The function prints verdicts in case of
 * errors are presents in the @p report.
 *
 * @param server            Server context.
 * @param report            Server report.
 * @param tag               Tag to print in verdict message.
 *
 * @return Status code. It returns non-zero code if there are errors in the
 * report.
 */
extern te_errno tapi_perf_server_check_report(tapi_perf_server *server,
                                              tapi_perf_report *report,
                                              const char *tag);

/**
 * Get server report and check it for errors. The function is a wrapper which
 * calls tapi_perf_server_get_report() and tapi_perf_server_check_report()
 *
 * @param[in]  server           Server context.
 * @param[in]  tag              Tag to print in verdict message.
 * @param[out] report           Report with results, it may be @c NULL if you
 *                              don't care about results, but only errors.
 *
 * @return Status code.
 *
 * @sa tapi_perf_server_get_report, tapi_perf_server_check_report,
 * tapi_perf_server_get_dump_check_report
 */
extern te_errno tapi_perf_server_get_check_report(tapi_perf_server *server,
                                                  const char *tag,
                                                  tapi_perf_report *report);

/**
 * Get server report, dump it to log and check for errors.
 *
 * @param[in]  server           Server context.
 * @param[in]  tag              Tag to print in both verdict and dump messages.
 * @param[out] report           Report with results, it may be @c NULL if you
 *                              don't care about results, but only errors.
 *
 * @return Status code.
 *
 * @sa tapi_perf_server_get_report, tapi_perf_server_check_report,
 * tapi_perf_server_get_check_report
 */
extern te_errno tapi_perf_server_get_dump_check_report(tapi_perf_server *server,
                                                       const char *tag,
                                                       tapi_perf_report *report);

/**
 * Create client network throughput test tool proxy.
 *
 * @param bench             Sort of tool, see @ref tapi_perf_bench to get a list
 *                          of supported tools.
 * @param options           Client tool specific options, may be @c NULL, to set
 *                          them to default, further you can edit them using
 *                          return value.
 *
 * @return Status code.
 *
 * @sa tapi_perf_client_destroy
 */
extern tapi_perf_client *tapi_perf_client_create(tapi_perf_bench bench,
                                                 const tapi_perf_opts *options,
                                                 tapi_job_factory_t *factory);

/**
 * Destroy client network throughput test tool proxy.
 *
 * @param client            Client context.
 *
 * @sa tapi_perf_client_create
 */
extern void tapi_perf_client_destroy(tapi_perf_client *client);

/**
 * Start perf client.
 *
 * @param client            Client context.
 *
 * @return Status code.
 *
 * @sa tapi_perf_client_stop
 */
extern te_errno tapi_perf_client_start(tapi_perf_client *client);

/**
 * Stop perf client.
 *
 * @param client            Client context.
 *
 * @return Status code.
 *
 * @sa tapi_perf_client_start
 */
extern te_errno tapi_perf_client_stop(tapi_perf_client *client);

/**
 * Wait while client finishes his work. Note, function jumps to cleanup if
 * timeout is expired.
 *
 * @param client        Client context.
 * @param timeout       Time to wait for client results (seconds). It MUST be
 *                      big enough to finish client normally (it depends
 *                      on client's options), otherwise the function will be
 *                      failed. Use @ref TAPI_PERF_TIMEOUT_DEFAULT to coerce
 *                      the function to calculate the required timeout value.
 *
 * @return Status code.
 */
extern te_errno tapi_perf_client_wait(tapi_perf_client *client,
                                      int16_t timeout);

/**
 * Get client report. The function reads client output (stdout, stderr).
 *
 * @param[in]  client       Client context.
 * @param[out] report       Report with results.
 *
 * @return Status code.
 */
extern te_errno tapi_perf_client_get_report(tapi_perf_client *client,
                                            tapi_perf_report *report);

/**
 * Get client report of specified kind. The function reads client
 * output (stdout, stderr).
 *
 * @param[in]  client       Client context.
 * @param[in]  kind         Report kind, e.g. default or receiver's,
 *                          or sender's.
 * @param[out] report       Report with results.
 *
 * @return Status code.
 */
extern te_errno tapi_perf_client_get_specific_report(tapi_perf_client *client,
    tapi_perf_report_kind kind, tapi_perf_report *report);

/**
 * Check client report for errors. The function prints verdicts in case of
 * errors are presents in the @p report.
 *
 * @param client            Client context.
 * @param report            Client report.
 * @param tag               Tag to print in verdict message.
 *
 * @return Status code. It returns non-zero code if there are errors in the
 * report.
 */
extern te_errno tapi_perf_client_check_report(tapi_perf_client *client,
                                              tapi_perf_report *report,
                                              const char *tag);

/**
 * Get client report and check it for errors. The function is a wrapper which
 * calls tapi_perf_client_get_report() and tapi_perf_client_check_report()
 *
 * @param[in]  client           Client context.
 * @param[in]  tag              Tag to print in verdict message.
 * @param[out] report           Report with results, it may be @c NULL if you
 *                              don't care about results, but only errors.
 *
 * @return Status code.
 *
 * @sa tapi_perf_client_get_report, tapi_perf_client_check_report,
 * tapi_perf_client_get_dump_check_report
 */
extern te_errno tapi_perf_client_get_check_report(tapi_perf_client *client,
                                                  const char *tag,
                                                  tapi_perf_report *report);

/**
 * Get client report, dump it to log and check for errors.
 *
 * @param[in]  client           Client context.
 * @param[in]  tag              Tag to print in both verdict and dump messages.
 * @param[out] report           Report with results, it may be @c NULL if you
 *                              don't care about results, but only errors.
 *
 * @return Status code.
 *
 * @sa tapi_perf_client_get_report, tapi_perf_client_check_report,
 * tapi_perf_client_get_check_report
 */
extern te_errno tapi_perf_client_get_dump_check_report(tapi_perf_client *client,
                                                       const char *tag,
                                                       tapi_perf_report *report);

/**
 * Get error description.
 *
 * @param error             Error code.
 *
 * @return Error code string representation.
 */
extern const char *tapi_perf_error2str(tapi_perf_error error);

/**
 * Get string representation of @p bench.
 *
 * @param bench             Tool's sort.
 *
 * @return Tool's sort name.
 */
extern const char *tapi_perf_bench2str(tapi_perf_bench bench);

/**
 * Get server network throughput test tool name.
 *
 * @param server            Server context.
 *
 * @return Server tool name.
 */
static inline const char *
tapi_perf_server_get_name(const tapi_perf_server *server)
{
    return tapi_perf_bench2str(server->app.bench);
}

/**
 * Get client network throughput test tool name.
 *
 * @param client            Client context.
 *
 * @return Client tool name.
 */
static inline const char *
tapi_perf_client_get_name(const tapi_perf_client *client)
{
    return tapi_perf_bench2str(client->app.bench);
}

/**
 * Print a network throughput test tool report.
 *
 * @param server            Server context.
 * @param client            Client context.
 * @param report            Report.
 * @param test_params       Test specific params; It should be represented in
 *                          the form of comma-separated pairs "param=value".
 */
extern void tapi_perf_log_report(const tapi_perf_server *server,
                                 const tapi_perf_client *client,
                                 const tapi_perf_report *report,
                                 const char *test_params);

/**
 * Print a network throughput test tool report by adding throughput of all
 * server/client pairs. Note, that we expect server/client pairs to run
 * roughly the same traffic, see perf_opts_cmp() for details.
 *
 * @param server              List of server contexts.
 * @param client              List of client contexts.
 * @param report              List of reports (user decides which one
 *                            is taken where).
 * @param number_of_instances Number of instances in the above 3 lists
 * @param test_params         Test specific params; It should be represented
 *                            in the form of comma-separated pairs
 *                            "param=value".
 */
extern void tapi_perf_log_cumulative_report(const tapi_perf_server *server[],
                                            const tapi_perf_client *client[],
                                            const tapi_perf_report *report[],
                                            int number_of_instances,
                                            const char *test_params);

/**
 * Compare important parts of the run.
 *
 * @param opts_a First object for comparison
 * @param opts_b Second object for comparison
 *
 * @return @c TRUE if objects' important properties are equal, @c FALSE if not
 */
extern te_bool tapi_perf_opts_cmp(const tapi_perf_opts *opts_a,
                                  const tapi_perf_opts *opts_b);

/**@} <!-- END tapi_performance --> */

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TAPI_PERFORMANCE_H__ */
