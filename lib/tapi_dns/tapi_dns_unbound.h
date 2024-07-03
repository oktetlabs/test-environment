/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2023 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief Unbound DNS server tool TAPI.
 *
 * @defgroup tapi_dns_unbound unbound DNS server tool TAPI (tapi_dns_unbound)
 * @ingroup te_ts_tapi
 * @{
 *
 * TAPI to handle unbound DNS server tool.
 */

#ifndef __TE_TAPI_DNS_UNBOUND_H__
#define __TE_TAPI_DNS_UNBOUND_H__

#include "tapi_job.h"
#include "tapi_job_opt.h"
#include "te_sockaddr.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Representation of possible values for unbound::verbose option. */
typedef enum tapi_dns_unbound_verbose {
    /** No verbosity, only errors. */
    TAPI_DNS_UNBOUND_NOT_VERBOSE = TAPI_JOB_OPT_ENUM_UNDEF,

    /** Gives operational information. */
    TAPI_DNS_UNBOUND_VERBOSE = 1,

    /**
     * Gives detailed operational information including short information per
     * query.
     */
    TAPI_DNS_UNBOUND_MORE_VERBOSE,

    /** Gives query level information, output per query. */
    TAPI_DNS_UNBOUND_VERBOSE_LL_QUERY,

    /** Gives algorithm level information. */
    TAPI_DNS_UNBOUND_VERBOSE_LL_ALGO,

    /** Logs client identification for cache misses. */
    TAPI_DNS_UNBOUND_VERBOSE_LL_CACHE,
} tapi_dns_unbound_verbose;

/** Representation of pair of address and port. */
typedef struct tapi_dns_unbound_cfg_address {
    /** Address, interface or host name. */
    const char *addr;
    /** Port number. */
    tapi_job_opt_uint_t port;
} tapi_dns_unbound_cfg_address;

/** Representation of possible values of action for access_control option. */
typedef enum tapi_dns_unbound_cfg_ac_action {
    /** Stops queries from hosts from that netblock. */
    TAPI_DNS_UNBOUND_CFG_AC_DENY,

    /** Stops queries too, but sends a DNS rcode REFUSED error message back. */
    TAPI_DNS_UNBOUND_CFG_AC_REFUSE,

    /**
     * Gives access to clients from that netblock. It gives only access for
     * recursion clients (which is what almost all clients need). Non-recursive
     * queries are refused.
     */
    TAPI_DNS_UNBOUND_CFG_AC_ALLOW,

    /**
     * Ignores the recursion desired (RD) bit and treats all requests as if the
     * recursion desired bit is set.
     */
    TAPI_DNS_UNBOUND_CFG_AC_ALLOW_SETRD,

    /** Give both recursive and non recursive access. */
    TAPI_DNS_UNBOUND_CFG_AC_ALLOW_SNOOP,

    /**
     * Messages that are disallowed to query for the authoritative local-data
     * are dropped.
     */
    TAPI_DNS_UNBOUND_CFG_AC_DENY_NON_LOCAL,

    /**
     * Messages that are disallowed to query for the authoritative local-data
     * are refused.
     */
    TAPI_DNS_UNBOUND_CFG_AC_REFUSE_NON_LOCAL,
} tapi_dns_unbound_cfg_ac_action;

/** Representation of unbound::access-control option. */
typedef struct tapi_dns_unbound_cfg_ac {
    /* Subnet to apply access control. */
    te_sockaddr_subnet subnet;

    /* Action to apply to requests in the subnet. */
    tapi_dns_unbound_cfg_ac_action action;
} tapi_dns_unbound_cfg_ac;

/** Representation of unbound::auth-zone option element. */
typedef struct tapi_dns_unbound_cfg_auth_zone {
    /** Name of the authority zone. */
    const char *name;

    /** The source of the zone to fetch with AXFR and IXFR. */
    struct {
        size_t n;
        tapi_dns_unbound_cfg_address *addr;
    } primaries;

    /** The source of the zone to fetch with HTTP or HTTPS. */
    struct {
        size_t n;
        const char **url;
    } primary_urls;

    /** The filename where the zone is stored. */
    const char *zonefile;
} tapi_dns_unbound_cfg_auth_zone;

/**
 * Unbound DNS server config file options.
 *
 * @note @c use-syslog option disabled by default.
 */
typedef struct tapi_dns_unbound_cfg_opt {
    /** Increase verbosity. */
    tapi_dns_unbound_verbose verbosity;

    /** Array of file names to include. */
    struct {
        size_t n;
        const char **filename;
    } includes;

    /**
     * If given, after binding the port the user privileges are dropped.
     * If username is set as an empty string or @c NULL, then no user change is
     * performed.
     */
    const char *username;

    /** Change root directory to the new one. */
    const char *chroot;

    /** Sets the working directory for the program. */
    const char *directory;

    /**
     * Array of interfaces to use to connect to the network. These interfaces
     * are listened to for queries from clients, and answers to clients are
     * given from this.
     */
    struct {
        size_t n;
        tapi_dns_unbound_cfg_address *addr;
    } interfaces;

    /**
     * Array of interfaces to use to connect to the network. These interfaces
     * are used to send queries to authoritative servers and receive their
     * replies.
     */
    struct {
        size_t n;
        const struct sockaddr **addr;
    } outgoing_interfaces;

    /** Array of access control rules for given netblocks and actions. */
    struct {
        size_t n;
        tapi_dns_unbound_cfg_ac *rule;
    } access_controls;

    /**
     * Array of addresses on private network, and are not allowed to be
     * returned for public internet names.
     */
    struct {
        size_t n;
        te_sockaddr_subnet *addr;
    } private_addresses;

    /**
     * Allow this domain, and all its subdomains to contain private addresses.
     */
    const char *private_domain;

    /** The port number on which the server responds to queries. */
    tapi_job_opt_uint_t port;

    /**
     * If @c true, then open dedicated listening sockets for incoming queries
     * for each thread and try to set the @c SO_REUSEPORT socket option on each
     * socket.
     */
    bool so_reuseport;

    /** Authority zones. */
    struct {
        size_t n;
        tapi_dns_unbound_cfg_auth_zone *zone;
    } auth_zones;

    /** The number of threads to create to serve clients. */
    tapi_job_opt_uint_t num_threads;

    /** The number of queries that every thread will service simultaneously. */
    tapi_job_opt_uint_t num_queries_per_thread;

    /**
     * Timeout used when the server is very busy. Set to a value that usually
     * results in one roundtrip to the authority servers.
     */
    tapi_job_opt_uint_t jostle_timeout;

    /**
     * If @c true, Unbound does not insert authority/additional sections into
     * response messages when those sections are not required. This reduces
     * response size significantly, and may avoid TCP fallback for some
     * responses. This may cause a slight speedup.
     */
    bool minimal_responses;

    /** Enable or disable whether IPv4 queries are answered or issued. */
    bool do_ip4;

    /** Enable or disable whether IPv6 queries are answered or issued. */
    bool do_ip6;

    /** Enable or disable whether UDP queries are answered or issued. */
    bool do_udp;

    /** Enable or disable whether TCP queries are answered or issued. */
    bool do_tcp;

    /** Number of incoming TCP buffers to allocate per thread. */
    tapi_job_opt_uint_t incoming_num_tcp;

    /** Number of outgoing TCP buffers to allocate per thread. */
    tapi_job_opt_uint_t outgoing_num_tcp;

    /** Time to live maximum for RRsets and messages in the cache. */
    tapi_job_opt_uint_t cache_max_ttl;

    /** Time to live minimum for RRsets and messages in the cache. */
    tapi_job_opt_uint_t cache_min_ttl;

    /**
     * If not @c 0, then set the @c SO_RCVBUF socket option to get more buffer
     * space on UDP port 53 incoming queries.
     */
    tapi_job_opt_uint_t so_rcvbuf;

    /**
     * If not @c 0, then set the @c SO_SNDBUF socket option to get more buffer
     * space on UDP port 53 outgoing queries.
     */
    tapi_job_opt_uint_t so_sndbuf;
} tapi_dns_unbound_cfg_opt;

/** Default options initializer. */
extern const tapi_dns_unbound_cfg_opt tapi_dns_unbound_cfg_default_opt;

/** Unbound DNS server specific command line options. */
typedef struct tapi_dns_unbound_opt {
    /** Path to Unbound executable. */
    const char *unbound_path;

    /**
     * Config file with settings for unbound to read instead of reading the file
     * at the default location.
     * Set to @c NULL to generate config file from
     * tapi_dns_unbound_opt::cfg_opt.
     */
    const char *cfg_file;

    /**
     * Configuration options for unbound DNS server.
     * This field is ignored if @ref tapi_dns_unbound_opt.cfg_file is not
     * @c NULL.
     */
    const tapi_dns_unbound_cfg_opt *cfg_opt;

    /** Increase verbosity. */
    tapi_dns_unbound_verbose verbose;
} tapi_dns_unbound_opt;

/** Default options initializer. */
extern const tapi_dns_unbound_opt tapi_dns_unbound_default_opt;

/** Information of a unbound DNS server tool. */
typedef struct tapi_dns_unbound_app {
    /** TAPI job handle. */
    tapi_job_t *job;
    /** Output channel handles. */
    tapi_job_channel_t *out_chs[2];
    /** Filter for out channel. */
    tapi_job_channel_t *out_filter;
    /** Filter for error messages. */
    tapi_job_channel_t *err_filter;
    /** Filter for debug info messages. */
    tapi_job_channel_t *info_filter;
    /** Path to generated config file. */
    char *generated_cfg_file;
} tapi_dns_unbound_app;

/**
 * Create unbound DNS server app.
 *
 * @param[in]  factory    Job factory.
 * @param[in]  opt        Unbound server tool options.
 * @param[out] app        Unbound server app handle.
 *
 * @return    Status code.
 */
extern te_errno tapi_dns_unbound_create(tapi_job_factory_t *factory,
                                        const tapi_dns_unbound_opt *opt,
                                        tapi_dns_unbound_app **app);

/**
 * Start unbound DNS server tool.
 *
 * @param app    Unbound DNS server app handle.
 *
 * @return    Status code.
 */
extern te_errno tapi_dns_unbound_start(tapi_dns_unbound_app *app);

/**
 * Wait for unbound DNS server tool completion.
 *
 * @param app           Unbound DNS server app handle.
 * @param timeout_ms    Wait timeout in milliseconds.
 *
 * @return    Status code.
 * @retval    TE_EINPROGRESS unbound DNS server is still running.
 */
extern te_errno tapi_dns_unbound_wait(tapi_dns_unbound_app *app,
                                      int timeout_ms);

/**
 * Send a signal to unbound DNS server tool.
 *
 * @param app       Unbound DNS server app handle.
 * @param signum    Signal to send.
 *
 * @return    Status code.
 */
extern te_errno tapi_dns_unbound_kill(tapi_dns_unbound_app *app, int signum);

/**
 * Stop unbound DNS server tool. It can be started over with
 * tapi_dns_unbound_start().
 *
 * @param app    Unbound DNS server app handle.
 *
 * @return    Status code.
 * @retval    TE_EPROTO unbound DNS server tool is stopped, but report is
 *            unavailable.
 */
extern te_errno tapi_dns_unbound_stop(tapi_dns_unbound_app *app);

/**
 * Destroy unbound DNS server app. The app cannot be used after calling this
 * function.
 *
 * @param app    Unbound DNS server app handle.
 *
 * @return    Status code.
 */
extern te_errno tapi_dns_unbound_destroy(tapi_dns_unbound_app *app);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __TE_TAPI_DNS_UNBOUND_H__ */

/** @} <!-- END tapi_dns_unbound --> */
