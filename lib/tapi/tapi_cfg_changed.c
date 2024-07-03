/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2022 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief  Test API to track data changes
 */

#define TE_LGR_USER     "Data Change Tracking TAPI"

#include "te_config.h"
#include "te_defs.h"
#include "logger_api.h"
#include "conf_api.h"
#include "tapi_cfg_changed.h"
#include "te_vector.h"
#include "te_str.h"

#define CFG_CHANGED_OID_PFX "/local:/changed:"

static te_errno
add_changeset_tag(const char *tag, cfg_handle *h)
{
    te_errno rc;

    rc = cfg_find_fmt(h, CFG_CHANGED_OID_PFX "%s", tag);
    if (rc == 0)
        return 0;

    if (TE_RC_GET_ERROR(rc) != TE_ENOENT)
        return rc;

    return cfg_add_instance_fmt(h, CFG_VAL(NONE, NULL),
                                CFG_CHANGED_OID_PFX "%s", tag);
}

/* See description in tapi_cfg_changed.h */
te_errno
tapi_cfg_changed_add_region(const char *tag, size_t start, size_t len)
{
    cfg_handle cs_handle;
    cfg_handle r_handle;
    te_errno rc;
    cfg_val_type valtype = CVT_STRING;
    char *val;
    char len_str[RCF_MAX_VAL];
    uintmax_t oldlen;

    TE_SPRINTF(len_str, "%zu", len);

    rc = add_changeset_tag(tag, &cs_handle);
    if (rc != 0)
        return rc;

    rc = cfg_find_fmt(&r_handle,
                      CFG_CHANGED_OID_PFX "%s/region:%zu",
                      tag, start);
    if (TE_RC_GET_ERROR(rc) == TE_ENOENT)
    {
        return cfg_add_instance_child_fmt(NULL, CFG_VAL(STRING, len_str),
                                          cs_handle, "/region:%zu", start);
    }
    else if (rc != 0)
    {
        return rc;
    }

    rc = cfg_get_instance(r_handle, &valtype, &val);
    if (rc != 0)
        return rc;

    rc = te_strtoumax(val, 10, &oldlen);
    free(val);
    if (rc != 0)
        return TE_RC_UPSTREAM(TE_TAPI, rc);

    if (oldlen > len)
        return 0;

    return cfg_set_instance(r_handle, CFG_VAL(STRING, len_str));
}

typedef struct changed_region {
    cfg_handle h;
    size_t start;
    size_t len;
} changed_region;

static te_errno
get_region(cfg_handle handle, void *data)
{
    te_vec *regions = data;
    te_errno rc;
    cfg_val_type type = CVT_STRING;
    char *strval = NULL;
    uintmax_t intval;
    changed_region region;

    region.h = handle;

    rc = cfg_get_instance(handle, &type, &strval);
    if (rc != 0)
        return rc;

    rc = te_strtoumax(strval, 10, &intval);
    free(strval);
    if (rc != 0)
        return rc;
    region.len = intval;

    rc = cfg_get_inst_name(handle, &strval);
    if (rc != 0)
        return rc;

    rc = te_strtoumax(strval, 10, &intval);
    free(strval);
    if (rc != 0)
        return rc;
    region.start = intval;

    return TE_VEC_APPEND(regions, region);
}

static int
compare_region(const void *obj1, const void *obj2)
{
    const changed_region *r1 = obj1;
    const changed_region *r2 = obj2;

    if (r1->start > r2->start)
        return 1;
    else if (r1->start < r2->start)
        return -1;
    else
        return 0;
}

static te_errno
get_regions(const char *tag, te_vec *regions)
{
    changed_region *r;
    te_errno rc = cfg_find_pattern_iter_fmt(get_region, regions,
                                            CFG_CHANGED_OID_PFX "%s/region:*",
                                            tag);

    if (rc != 0)
        return rc;

    te_vec_sort(regions, compare_region);

    /* Fix unsigned overflows for too large lengths */
    TE_VEC_FOREACH(regions, r)
    {
        if (r->start + r->len < r->start)
            r->len = SIZE_MAX - r->start;
    }

    /* Adjust lengths to exclude overlap */
    TE_VEC_FOREACH(regions, r)
    {
        if (te_vec_get_index(regions, r) + 1 != te_vec_size(regions))
        {
            if (r->start + r->len > r[1].start)
                r->len = r[1].start - r->start;
        }
    }

    return 0;
}

static int
compare_overlap_region(const void *obj1, const void *obj2)
{
    const changed_region *r1 = obj1;
    const changed_region *r2 = obj2;

    if (r1->start >= r2->start + r2->len)
        return 1;
    else if (r1->start + r1->len < r2->start)
        return -1;
    else
        return 0;
}

/* See description in tapi_cfg_changed.h */
te_errno
tapi_cfg_changed_add_region_overlap(const char *tag, size_t start, size_t len)
{
    te_errno rc = 0;
    te_vec regions = TE_VEC_INIT(changed_region);
    unsigned int overlap;
    unsigned int max_overlap;
    changed_region r;

    if (start + len < start)
        len = SIZE_MAX - start;

    rc = get_regions(tag, &regions);
    if (rc != 0)
        goto cleanup;

    r.start = start;
    r.len = len;
    if (!te_vec_search(&regions, &r, compare_overlap_region,
                       &overlap, &max_overlap))
    {
        te_vec_free(&regions);
        return tapi_cfg_changed_add_region(tag, start, len);
    }

    r = TE_VEC_GET(changed_region, &regions, overlap);
    if (start < r.start)
    {
        rc = tapi_cfg_changed_add_region(tag, start, r.start - start);
        if (rc != 0)
            goto cleanup;
    }

    while (overlap <= max_overlap)
    {
        size_t cur_start = r.start;
        size_t cur_len = len - (cur_start - start);

        overlap++;
        if (overlap != te_vec_size(&regions))
        {
            r = TE_VEC_GET(changed_region, &regions, overlap);
            if (cur_start + cur_len > r.start)
                cur_len = r.start - cur_start;
        }
        rc = tapi_cfg_changed_add_region(tag, cur_start, cur_len);
        if (rc != 0)
            goto cleanup;
    }

cleanup:
    te_vec_free(&regions);
    return rc;
}

/* See description in tapi_cfg_changed.h */
te_errno
tapi_cfg_changed_remove_region(const char *tag, size_t start)
{
    te_errno rc = cfg_del_instance_fmt(false,
                                       CFG_CHANGED_OID_PFX "%s/region:%zu",
                                       tag, start);

    if (TE_RC_GET_ERROR(rc) == TE_ENOENT)
        rc = 0;

    return rc;
}

/* See description in tapi_cfg_changed.h */
te_errno
tapi_cfg_changed_remove_region_overlap(const char *tag, size_t start,
                                       size_t len)
{
    te_errno rc = 0;
    te_vec regions = TE_VEC_INIT(changed_region);
    unsigned int min_overlap;
    unsigned int max_overlap;
    changed_region r;

    if (start + len < start)
        len = SIZE_MAX - start;

    rc = get_regions(tag, &regions);
    if (rc != 0)
        goto cleanup;

    r.start = start;
    r.len = len;
    if (!te_vec_search(&regions, &r, compare_overlap_region,
                       &min_overlap, &max_overlap))
    {
        te_vec_free(&regions);
        return tapi_cfg_changed_add_region(tag, start, len);
    }

    for (; min_overlap <= max_overlap; min_overlap++)
    {
        r = TE_VEC_GET(changed_region, &regions, min_overlap);

        if (start <= r.start)
        {
            rc = tapi_cfg_changed_remove_region(tag, r.start);
            if (rc != 0)
                goto cleanup;
        }
        else
        {
            char len_str[RCF_MAX_VAL];

            TE_SPRINTF(len_str, "%zu", start - r.start);
            rc = cfg_set_instance(r.h, CFG_VAL(STRING, len_str));
            if (rc != 0)
                goto cleanup;
        }

        if (r.start + r.len > start + len)
        {
            rc = tapi_cfg_changed_add_region(tag, start + len,
                                             r.len - (start + len - r.start));
            if (rc != 0)
                goto cleanup;
        }
    }

cleanup:
    te_vec_free(&regions);

    return rc;
}

/* See description in tapi_cfg_changed.h */
te_errno
tapi_cfg_changed_process_regions(const char *tag, tapi_cfg_changed_callback *cb,
                                 void *ctx)
{
    te_errno rc = 0;
    te_vec regions = TE_VEC_INIT(changed_region);
    changed_region *r;

    rc = get_regions(tag, &regions);
    if (rc != 0)
        goto cleanup;

    TE_VEC_FOREACH(&regions, r)
    {
        rc = cb(tag, r->start, r->len, ctx);
        if (TE_RC_GET_ERROR(rc) == TE_EAGAIN)
        {
            rc = 0;
        }
        else
        {
            if (rc == 0)
                rc = cfg_del_instance(r->h, false);
            if (rc != 0)
                goto cleanup;
        }
    }

cleanup:
    te_vec_free(&regions);
    return rc;
}

/* See description in tapi_cfg_changed.h */
te_errno
tapi_cfg_changed_clear_tag(const char *tag)
{
    te_errno rc = cfg_del_instance_fmt(true,
                                       CFG_CHANGED_OID_PFX "%s", tag);

    if (TE_RC_GET_ERROR(rc) == TE_ENOENT)
        rc = 0;

    return rc;
}
