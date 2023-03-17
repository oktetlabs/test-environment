/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2023 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief DNS zone file generation tool TAPI.
 *
 * @defgroup tapi_dns_zone_file DNS zone file generation tool TAPI
 *           (tapi_dns_zone_file).
 * @ingroup te_ts_tapi
 * @{
 *
 * TAPI to handle DNS zone file generation tool.
 */

#ifndef __TE_TAPI_DNS_ZONE_FILE_H__
#define __TE_TAPI_DNS_ZONE_FILE_H__

#include "tapi_job.h"
#include "tapi_job_opt.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Possible types of resource record in zone file. */
typedef enum tapi_dns_zone_file_rr_type {
    /** Host name address. */
    TAPI_DNS_ZONE_FILE_RRT_A,

    /** Canonical name of an alias. */
    TAPI_DNS_ZONE_FILE_RRT_CNAME,

    /** CPU and OS used by a host. */
    TAPI_DNS_ZONE_FILE_RRT_HINFO,

    /** Mail exchange for the domain. */
    TAPI_DNS_ZONE_FILE_RRT_MX,

    /** Authoritative name server for the domain. */
    TAPI_DNS_ZONE_FILE_RRT_NS,

    /** Pointer to another part of the domain name space. */
    TAPI_DNS_ZONE_FILE_RRT_PTR,

    /** Start of a zone of authority. */
    TAPI_DNS_ZONE_FILE_RRT_SOA,

    /** IPv6 address. */
    TAPI_DNS_ZONE_FILE_RRT_AAAA,
} tapi_dns_zone_file_rr_type;

/** Possible classes of resource record in zone file. */
typedef enum tapi_dns_zone_file_rr_class {
    /** Internet system. */
    TAPI_DNS_ZONE_FILE_RRC_IN,

    /** Chaos system. */
    TAPI_DNS_ZONE_FILE_RRC_CH,
} tapi_dns_zone_file_rr_class;

/**
 * Describe @c RDATA field of resource record of @c A type.
 *
 * @note At the moment, only the @c IN class is supported.
 */
typedef struct tapi_dns_zone_file_rr_a {
    const struct sockaddr *addr;
} tapi_dns_zone_file_rr_a;

/** Describe @c RDATA field of resource record of @c SOA type. */
typedef struct tapi_dns_zone_file_rr_soa {
    /**
     * The domain name of the name server that was the original or primary
     * source of data for this zone.
     */
    const char *primary_name_server;

    /**
     * A domain name which specifies the mailbox of the person responsible for
     * this zone.
     */
    const char *hostmaster_email;

    /** Version number of the original copy of the zone. */
    unsigned int serial;

    /** Time interval before the zone should be refreshed.*/
    unsigned int refresh;

    /**
     * Time interval that should elapse before a failed refresh should be
     * retried.
     */
    unsigned int retry;

    /**
     * Time value that specifies the upper limit on the time interval that can
     * elapse before the zone is no longer authoritative.
     */
    unsigned int expire;

    /** Minimum TTL field that should be exported with any RR from this zone.*/
    unsigned int minimum;
} tapi_dns_zone_file_rr_soa;

/** Describe @c RDATA field of resource record of @c NS type. */
typedef struct tapi_dns_zone_file_rr_ns {
    /**
     * A domain name which specifies a host which should be authoritative for
     * the specified class and domain.
     */
    const char *nsdname;
} tapi_dns_zone_file_rr_ns;

/**
 * Describe pair of @c TYPE and corresponding @c RDATA fields of resource
 * record.
 */
typedef struct tapi_dns_zone_file_rr_data {
    /** Type of @c RDATA field. */
    tapi_dns_zone_file_rr_type type;

    /** @c RDATA field corresponding to @c TYPE. */
    union {
        tapi_dns_zone_file_rr_a a;
        tapi_dns_zone_file_rr_a aaaa;
        tapi_dns_zone_file_rr_soa soa;
        tapi_dns_zone_file_rr_ns ns;
    } u;
} tapi_dns_zone_file_rr_data;

/** Analog of resource record in zone file. */
typedef struct tapi_dns_zone_file_rr {
    /** The domain name where the RR is found. */
    const char *owner;

    /** Protocol family or instance of a protocol. */
    tapi_dns_zone_file_rr_class class;

    /** Time to live of the RR. */
    tapi_job_opt_uint_t ttl;

    /** Pair of type of the resource in RR and corresponding resource data. */
    tapi_dns_zone_file_rr_data rdata;
} tapi_dns_zone_file_rr;

/**
 * Generate zone file for DNS server.
 *
 * @param[in]  ta                    Test Agent name.
 * @param[in]  resource_records      Array of resource records.
 * @param[in]  resource_records_n    Number of resource records.
 * @param[in]  base_dir              Path to directory where the file will be
 *                                   generated. If @c NULL, the default base
 *                                   directory will be used.
 * @param[in]  pathname              Path to the config file. If @c NULL, the
 *                                   file name will be randomly generated. If
 *                                   not an absolute path, @c base_dir will be
 *                                   used.
 * @param[out] result_pathname       Resulting path to the file. Maybe @c NULL.
 *
 * @return    Status code.
 */
extern te_errno tapi_dns_zone_file_create(const char *ta,
                                      tapi_dns_zone_file_rr *resource_records,
                                      size_t resource_records_n,
                                      const char *base_dir,
                                      const char *pathname,
                                      char **result_pathname);

/**
 * Destroy zone file for DNS server.
 *
 * @param[in] ta          Test Agent name.
 * @param[in] pathname    Path to zone file.
 *
 * @return    Status code.
 */
extern te_errno tapi_dns_zone_file_destroy(const char *ta,
                                           const char *pathname);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __TE_TAPI_DNS_ZONE_FILE_H__ */

/** @} <!-- END tapi_dns_zone_file --> */