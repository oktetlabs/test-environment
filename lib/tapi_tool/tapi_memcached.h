/** @file
 * @brief TAPI to manage memcached
 *
 * @defgroup tapi_memcached TAPI to manage memcached
 * @ingroup te_ts_tapi
 * @{
 *
 * TAPI to manage *memcached*.
 *
 * Copyright (C) 2022-2022 OKTET Labs. All rights reserved.
 *
 */

#ifndef __TE_TAPI_MEMCACHED_H__
#define __TE_TAPI_MEMCACHED_H__

#include <sys/socket.h>

#include "te_errno.h"
#include "tapi_job_opt.h"
#include "tapi_job.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Constant for sockaddr "0.0.0.0:0" initialization.
 *
 * @note Can be used when there is a need to use localhost or zero port.
 */
extern const struct sockaddr_in zero_sockaddr;

/** memcached tool information. */
typedef struct tapi_memcached_app {
    /** TAPI job handle. */
    tapi_job_t *job;
    /** Output channel handles. */
    tapi_job_channel_t *out_chs[2];
} tapi_memcached_app;

/** Representation of possible values for memcached::protocol option. */
typedef enum tapi_mamcached_proto {
    TAPI_MEMCACHED_PROTO_AUTO,
    TAPI_MEMCACHED_PROTO_ASCII,
    TAPI_MEMCACHED_PROTO_BINARY,
} tapi_mamcached_proto_t;

/** Specific memcached options. */
typedef struct tapi_memcached_opt {
    /** Unix socket path to listen on (disables network support). */
    const char                         *unix_socket;
    /** Enable ascii "shutdown" command. */
    te_bool                             enable_ascii_shutdown;
    /** Permissions (in octal form) for Unix socket created with -s option. */
    tapi_job_opt_uint_t                 unix_mask;
    /** Listen on <ip_addr>. */
    const struct sockaddr              *listen_ipaddr;
    /** Assume the identity of <username>. */
    const char                         *username;
    /** Memory usage in MB. */
    tapi_job_opt_uint_t                 memory_limit;
    /** Max simultaneous connections. */
    tapi_job_opt_uint_t                 conn_limit;
    /**
     * Once a connection exceeds this number of consecutive requests,
     * the server will try to process I/O on other connections before
     * processing any further requests from that connection.
     */
    tapi_job_opt_uint_t                 max_reqs_per_event;
    /**
     * Lock down all paged memory.
     * This is a somewhat dangerous option with large caches.
     */
    te_bool                             lock_memory;
    /**
     * TCP port to listen on (0 by default, 0 to turn off).
     *
     * @note To set 0 use @c zero_sockaddr.
     */
    const struct sockaddr              *tcp_port;
    /**
     * UDP port to listen on (0 by default, 0 to turn off).
     *
     * @note To set 0 use @c zero_sockaddr.
    */
    const struct sockaddr              *udp_port;
    /**
     * Disable automatic removal of items from the cache when out of memory.
     * Additions will not be possible until adequate space is freed up.
     */
    te_bool                             disable_evictions;
    /** Raise the core file size limit to the maximum allowable. */
    te_bool                             enable_coredumps;
    /**
     * A lower value may result in less wasted memory depending on the total
     * amount of memory available and the distribution of item sizes.
     */
    tapi_job_opt_double_t               slab_growth_factor;
    /**
     * Allocate a minimum of <size> bytes for the item key, value,
     * and flags.
     */
    tapi_job_opt_uint_t                 slab_min_size;
    /** Disable the use of CAS (and reduce the per-item size by 8 bytes). */
    te_bool                             disable_cas;
    /** Be verbose during the event loop, print out errors and warnings. */
    te_bool                             verbose;
    /** Number of threads to use to process incoming requests. */
    tapi_job_opt_uint_t                 threads;
    /**
     * One char delimiter between key prefixes and IDs.
     * This is used for per-prefix stats reporting.
     */
    const char                         *delimiter;
    /**
     * Try to use large memory pages (if available).
     * Increasing the memory page size could reduce the number of TLB misses
     * and improve the performance.
     */
    te_bool                             enable_largepages;
    /** Set the backlog queue limit to number of connections. */
    tapi_job_opt_uint_t                 listen_backlog;
    /** Specify the binding protocol to use ("auto" by default). */
    tapi_mamcached_proto_t              protocol;
    /** Override the default size of each slab page in Kilobytes. */
    tapi_job_opt_uint_t                 max_item_size;
    /**
     * Turn on SASL authentication. This option is only meaningful
     * if memcached was compiled with SASL support enabled.
     */
    te_bool                             enable_sasl;
    /**
     * Disable the "flush_all" command. The cmd_flush counter will
     * increment, but clients will receive an error message and the
     * flush will not occur.
     */
    te_bool                             disable_flush_all;
    /** Disable the "stats cachedump" and "lru_crawler metadump" commands. */
    te_bool                             disable_dumping;
    /** Disable watch commands (live logging). */
    te_bool                             disable_watch;
    /**
     * @defgroup tapi_memcached_extended_opts
     * @ingroup tapi_memcached
     * @{
     */
    /** Immediately close new connections after limit. */
    te_bool                             maxconns_fast;
    /** Cancel maxconns_fast option. */
    te_bool                             no_maxconns_fast;
    /**
     * An integer multiplier for how large the hash
     * table should be. Normally grows at runtime.
     * Set based on "STAT hash_power_level".
     */
    tapi_job_opt_uint_t                 hashpower;
    /**
     * Time in seconds for how long to wait before
     * forcefully killing LRU tail item.
     * Very dangerous option!
     */
    tapi_job_opt_uint_t                 tail_repair_time;
    /** Disable LRU Crawler background thread. */
    te_bool                             no_lru_crawler;
    /** Microseconds to sleep between items. */
    tapi_job_opt_uint_t                 lru_crawler_sleep;
    /** Max items to crawl per slab per run (if 0 then unlimited). */
    tapi_job_opt_uint_t                 lru_crawler_tocrawl;
    /**  Disable new LRU system + background thread. */
    te_bool                             no_lru_maintainer;
    /** pct of slab memory to reserve for hot lru. Requires lru_maintainer. */
    tapi_job_opt_uint_t                 hot_lru_pct;
    /** pct of slab memory to reserve for warm lru. Requires lru_maintainer. */
    tapi_job_opt_uint_t                 warm_lru_pct;
    /** Items idle > cold lru age * drop from hot lru. */
    tapi_job_opt_double_t               hot_max_factor;
    /** Items idle > cold lru age * this drop from warm. */
    tapi_job_opt_double_t               warm_max_factor;
    /**
     * TTL's below get separate LRU, can't be evicted.
     * Requires lru_maintainer.
     */
    tapi_job_opt_uint_t                 temporary_ttl;
    /** Timeout for idle connections (if 0 then no timeout). */
    tapi_job_opt_uint_t                 idle_timeout;
    /** Size in kilobytes of per-watcher write buffer. */
    tapi_job_opt_uint_t                 watcher_logbuf_size;
    /**
     * Size in kilobytes of per-worker-thread buffer
     * read by background thread, then written to watchers.
     */
    tapi_job_opt_uint_t                 worker_logbuf_size;
    /** Enable dynamic reports for 'stats sizes' command. */
    te_bool                             track_sizes;
    /** Disables hash table expansion. Dangerous! */
    te_bool                             no_hashexpand;
    /** @} */
    /** Path to memcached exec (if @c NULL then "memcached"). */
    const char                         *memcached_path;
} tapi_memcached_opt;

/** Default memcached options initializer. */
extern const tapi_memcached_opt tapi_memcached_default_opt;

/**
 * Create memcached app.
 *
 * @param[in]  factory      Job factory.
 * @param[in]  opt          memcached options.
 * @param[out] app          memcached app handle.
 *
 * @return Status code.
 */
extern te_errno tapi_memcached_create(tapi_job_factory_t *factory,
                                      const tapi_memcached_opt *opt,
                                      tapi_memcached_app **app);

/**
 * Start memcached.
 *
 * @param[in]  app          memcached app handle.
 *
 * @return Status code.
 */
extern te_errno tapi_memcached_start(const tapi_memcached_app *app);

/**
 * Wait for memcached completion.
 *
 * @param[in]  app          memcached app handle.
 * @param[in]  timeout_ms   Wait timeout in milliseconds.
 *
 * @return Status code.
 */
extern te_errno tapi_memcached_wait(const tapi_memcached_app *app,
                                    int timeout_ms);

/**
 * Stop memcached. It can be started over with tapi_memcached_start().
 *
 * @param[in]  app          memcached app handle.
 *
 * @return Status code.
 */
extern te_errno tapi_memcached_stop(const tapi_memcached_app *app);

/**
 * Send a signal to memcached.
 *
 * @param[in]  app          memcached app handle.
 * @param[in]  signum       Signal to send.
 *
 * @return Status code.
 */
extern te_errno tapi_memcached_kill(const tapi_memcached_app *app,
                                    int signum);

/**
 * Destroy memcached.
 *
 * @param[in]  app          memcached app handle.
 *
 * @return Status code.
 */
extern te_errno tapi_memcached_destroy(tapi_memcached_app *app);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_MEMCACHED_H__ */
/**@} <!-- END tapi_memcached --> */