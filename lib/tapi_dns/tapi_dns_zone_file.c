/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2023 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief DNS zone file generation tool TAPI.
 *
 * TAPI to handle DNS zone file generation tool.
 */

#define TE_LGR_USER "TAPI UNBOUND"

#include "tapi_dns_common.h"
#include "tapi_dns_zone_file.h"
#include "tapi_job_opt.h"
#include "te_alloc.h"
#include "te_str.h"
#include "te_sockaddr.h"
#include "te_enum.h"
#include "tapi_file.h"

/* Zone file specific options. */
typedef struct tapi_dns_zone_file_data {
    size_t resource_records_n;
    tapi_dns_zone_file_rr *resource_records;
} tapi_dns_zone_file_data;

static const te_enum_map tapi_dns_zone_file_rr_type_mapping[] = {
    {.name = "A",     .value = TAPI_DNS_ZONE_FILE_RRT_A},
    {.name = "CNAME", .value = TAPI_DNS_ZONE_FILE_RRT_CNAME},
    {.name = "HINFO", .value = TAPI_DNS_ZONE_FILE_RRT_HINFO},
    {.name = "MX",    .value = TAPI_DNS_ZONE_FILE_RRT_MX},
    {.name = "NS",    .value = TAPI_DNS_ZONE_FILE_RRT_NS},
    {.name = "PTR",   .value = TAPI_DNS_ZONE_FILE_RRT_PTR},
    {.name = "SOA",   .value = TAPI_DNS_ZONE_FILE_RRT_SOA},
    {.name = "AAAA",  .value = TAPI_DNS_ZONE_FILE_RRT_AAAA},
    TE_ENUM_MAP_END
};

static te_errno
job_opt_create_rdata(const void *value, const void *priv, te_vec *args)
{
    const tapi_dns_zone_file_rr_data *rdata = value;
    const char *type_str = NULL;

    if (rdata == NULL)
        return TE_ENOENT;

    type_str = te_enum_map_from_value(tapi_dns_zone_file_rr_type_mapping,
                                      rdata->type);

    switch (rdata->type)
    {
        case TAPI_DNS_ZONE_FILE_RRT_A:
            return te_vec_append_str_fmt(args, "%s %s",
                       type_str,
                       te_sockaddr_get_ipstr(rdata->u.a.addr));

        case TAPI_DNS_ZONE_FILE_RRT_AAAA:
            return te_vec_append_str_fmt(args, "%s %s",
                       type_str,
                       te_sockaddr_get_ipstr(rdata->u.aaaa.addr));

        case TAPI_DNS_ZONE_FILE_RRT_SOA:
            return te_vec_append_str_fmt(args, "%s %s %s (%u %u %u %u %u)",
                                         type_str,
                                         rdata->u.soa.primary_name_server,
                                         rdata->u.soa.hostmaster_email,
                                         rdata->u.soa.serial,
                                         rdata->u.soa.refresh,
                                         rdata->u.soa.retry,
                                         rdata->u.soa.expire,
                                         rdata->u.soa.minimum);

        case TAPI_DNS_ZONE_FILE_RRT_NS:
            return te_vec_append_str_fmt(args, "%s %s",
                       type_str, rdata->u.ns.nsdname);

        default:
            return TE_EOPNOTSUPP;
    }
}

/**
 * Bind tapi_dns_zone_file_rr argument.
 *
 * @param[in] prefix_           Argument prefix.
 * @param[in] concat_prefix_    Concatenate prefix with argument if @c TRUE.
 * @param[in] struct_           Option struct.
 * @param[in] rr_data_          Field name of the string in option struct.
 */
#define TAPI_DNS_ZONE_OPT_RDATA(prefix_, concat_prefix_, struct_, rr_data_) \
    { job_opt_create_rdata, prefix_, concat_prefix_, NULL,                  \
      TAPI_JOB_OPT_OFFSETOF_CHK_SIZE(struct_, rr_data_,                     \
                                     tapi_dns_zone_file_rr_data),           \
      NULL }

static const te_enum_map tapi_dns_zone_file_rr_class_mapping[] = {
    {.name = "IN", .value = TAPI_DNS_ZONE_FILE_RRC_IN},
    {.name = "CH", .value = TAPI_DNS_ZONE_FILE_RRC_CH},
    TE_ENUM_MAP_END
};

static const tapi_job_opt_bind zone_file_binds[] = TAPI_JOB_OPT_SET(
    TAPI_JOB_OPT_ARRAY_PTR(tapi_dns_zone_file_data,
        resource_records_n, resource_records,
        TAPI_JOB_OPT_STRUCT(NULL, FALSE, " ", NULL,
                            TAPI_JOB_OPT_STRING(NULL, FALSE,
                                tapi_dns_zone_file_rr, owner),
                            TAPI_JOB_OPT_UINT_T(NULL, FALSE, NULL,
                                tapi_dns_zone_file_rr, ttl),
                            TAPI_JOB_OPT_ENUM(NULL, FALSE,
                                tapi_dns_zone_file_rr, class,
                                tapi_dns_zone_file_rr_class_mapping),
                            TAPI_DNS_ZONE_OPT_RDATA(NULL, FALSE,
                                tapi_dns_zone_file_rr, rdata)
                            ))
);

te_errno
tapi_dns_zone_file_create(const char *ta,
                          tapi_dns_zone_file_rr *resource_records,
                          size_t resource_records_n,
                          const char *base_dir,
                          const char *pathname,
                          char **result_pathname)
{
    te_vec zone_file_args = TE_VEC_INIT(char *);
    te_string zone_file_data = TE_STRING_INIT;
    char *res_path = NULL;
    te_errno rc;
    tapi_dns_zone_file_data opt = {
        .resource_records = resource_records,
        .resource_records_n = resource_records_n,
    };

    if (opt.resource_records == NULL)
    {
        ERROR("Empty resource records array");
        return TE_EINVAL;
    }

    rc = tapi_job_opt_build_args("", zone_file_binds, &opt,
                                 &zone_file_args);
    if (rc != 0)
    {
        ERROR("Failed to build zone file options: %r", rc);
        return rc;
    }

    te_string_join_vec(&zone_file_data, &zone_file_args, "\n");
    te_vec_deep_free(&zone_file_args);

    res_path = tapi_dns_gen_filepath(ta, base_dir, pathname);
    rc = tapi_file_create_ta(ta, res_path, "%s", zone_file_data.ptr);
    te_string_free(&zone_file_data);
    if (rc != 0)
    {
        free(res_path);
        ERROR("Failed to create zone file: %r", rc);
        return rc;
    }

    if (result_pathname != NULL)
        *result_pathname = res_path;
    else
        free(res_path);

    return 0;
}

te_errno
tapi_dns_zone_file_destroy(const char *ta, const char *pathname)
{
    if (pathname != NULL)
        return tapi_file_ta_unlink_fmt(ta, "%s", pathname);

    return 0;
}