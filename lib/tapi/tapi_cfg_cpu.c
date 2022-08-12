/** @file
 * @brief Test API to configure CPUs.
 *
 * Implementation of API to configure CPUs.
 *
 * Copyright (C) 2018-2022 OKTET Labs. All rights reserved.
 */

#define TE_LGR_USER      "Conf CPUs TAPI"

#include "te_config.h"

#include <stdio.h>
#ifdef STDC_HEADERS
#include <stdlib.h>
#include <string.h>
#endif

#include "te_defs.h"
#include "te_errno.h"
#include "te_stdint.h"
#include "te_alloc.h"
#include "rcf_api.h"
#include "logger_api.h"
#include "conf_api.h"
#include "te_str.h"

#include "tapi_cfg.h"
#include "tapi_cfg_base.h"
#include "tapi_cfg_cpu.h"

static te_errno
format_cpu_rsrc_oid(const char *ta, const tapi_cpu_index_t *cpu_id, size_t length,
                    char *buf)
{
    int rc = snprintf(buf, length, "/agent:%s/rsrc:cpu:%lu:%lu:%lu:%lu", ta,
                      cpu_id->node_id, cpu_id->package_id, cpu_id->core_id,
                      cpu_id->thread_id);

    if (rc < 0)
    {
        ERROR("Failed to format CPU resource OID");
        return TE_OS_RC(TE_TAPI, errno);
    }

    if ((size_t)rc >= length)
    {
        ERROR("Too long CPU resource OID");
        return TE_RC(TE_TAPI, TE_ENAMETOOLONG);
    }

    return 0;
}

static te_errno
format_cpu_value(const char *ta, const tapi_cpu_index_t *cpu_id,
                 size_t length, char *buf)
{
    int rc;

    rc = snprintf(buf, length,
                  "/agent:%s/hardware:/node:%lu/cpu:%lu/core:%lu/thread:%lu",
                  ta, cpu_id->node_id, cpu_id->package_id, cpu_id->core_id,
                  cpu_id->thread_id);

    if (rc < 0)
    {
        ERROR("Failed to format CPU path in configuration tree");
        return TE_OS_RC(TE_TAPI, errno);
    }

    if ((size_t)rc >= length)
    {
        ERROR("Too long CPU path in configuration tree");
        return TE_RC(TE_TAPI, TE_ENAMETOOLONG);
    }

    return 0;
}

static te_errno
get_cpu_id_generic(const cfg_oid *oid, int depth, unsigned long *id)
{
    te_errno rc;

    if (oid->len > depth)
    {
        if ((rc = te_strtoul(CFG_OID_GET_INST_NAME(oid, depth), 10, id)) != 0)
        {
            ERROR("Failed to get %dth CPU index from OID", depth);
            return rc;
        }
    }
    else
    {
        *id = TAPI_CPU_ID_UNSPEC;
    }

    return 0;
}

static te_errno
find_cpu_generic(const char *pattern, const char *err_msg, size_t *n_indices,
                 tapi_cpu_index_t **indices)
{
    cfg_handle *match_cpus = NULL;
    unsigned int number_of_cpus = 0;
    tapi_cpu_index_t *result = NULL;
    cfg_oid *oid = NULL;
    unsigned int i;
    te_errno rc = 0;

    rc = cfg_find_pattern(pattern, &number_of_cpus, &match_cpus);
    if (rc != 0 || number_of_cpus == 0)
    {
        if (number_of_cpus == 0)
            rc = TE_RC(TE_TAPI, TE_ENOENT);

        ERROR("Failed to find %s", err_msg);
        goto out;
    }

    result = TE_ALLOC(sizeof(*result) * number_of_cpus);
    if (result == NULL)
    {
        rc = TE_RC(TE_TAPI, TE_ENOMEM);
        goto out;
    }

    for (i = 0; i < number_of_cpus; i++)
    {
        if ((rc = cfg_get_oid(match_cpus[i], &oid)) != 0)
        {
            ERROR("Failed to get OID from %s handle", err_msg);
            goto out;
        }
        if ((rc = get_cpu_id_generic(oid, 3, &result[i].node_id)) != 0)
            goto out;
        if ((rc = get_cpu_id_generic(oid, 4, &result[i].package_id)) != 0)
            goto out;
        if ((rc = get_cpu_id_generic(oid, 5, &result[i].core_id)) != 0)
            goto out;
        if ((rc = get_cpu_id_generic(oid, 6, &result[i].thread_id)) != 0)
            goto out;
    }

    *n_indices = number_of_cpus;

    if (indices != NULL)
    {
        *indices = result;
    }
    else
    {
        free(result);
        result = NULL;
    }

out:
    cfg_free_oid(oid);
    free(match_cpus);
    if (rc != 0)
        free(result);

    return rc;
}

static te_errno
check_thread(const char *ta, const tapi_cpu_index_t *cpu_id,
             const tapi_cpu_prop_t *prop, te_bool *suitable)
{
    char cpu_value[RCF_MAX_VAL] = {0};
    te_bool is_isolated;
    int isolated;
    te_errno rc;

    if ((rc = format_cpu_value(ta, cpu_id, sizeof(cpu_value), cpu_value)) != 0)
        return rc;

    rc = tapi_cfg_get_int_fmt(&isolated, "%s/isolated:", cpu_value);
    if (rc != 0)
    {
        ERROR("Failed to get isolated property of CPU %lu", cpu_id->thread_id);
        return rc;
    }

    is_isolated = (isolated != 0);
    *suitable = (is_isolated == prop->isolated);

    return 0;
}

te_errno
tapi_cfg_cpu_grab_by_id(const char *ta, const tapi_cpu_index_t *cpu_id)
{
    char cpu_rsrc_oid[CFG_OID_MAX] = {0};
    char cpu_value[RCF_MAX_VAL] = {0};
    char *old_cpu_value = NULL;
    te_errno rc = 0;

    rc = format_cpu_rsrc_oid(ta, cpu_id, sizeof(cpu_rsrc_oid), cpu_rsrc_oid);
    if (rc != 0)
        return rc;

    /* Check that the CPU is not already grabbed by this process */
    rc = cfg_get_instance_str(NULL, &old_cpu_value, cpu_rsrc_oid);
    free(old_cpu_value);

    switch (TE_RC_GET_ERROR(rc))
    {
        case TE_ENOENT:
            break;
        case 0:
            return TE_RC(TE_TAPI, TE_EEXIST);
        default:
            return rc;
    }

    if ((rc = format_cpu_value(ta, cpu_id, sizeof(cpu_value), cpu_value)) != 0)
        return rc;

    rc = cfg_add_instance_str(cpu_rsrc_oid, NULL, CVT_STRING, cpu_value);
    if (TE_RC_GET_ERROR(rc) == TE_EPERM)
        return TE_RC(TE_TAPI, TE_EBUSY);

    return rc;
}

te_errno
tapi_cfg_cpu_release_by_id(const char *ta, const tapi_cpu_index_t *cpu_id)
{
    char cpu_rsrc_oid[CFG_OID_MAX] = {0};
    te_errno rc = 0;

    rc = format_cpu_rsrc_oid(ta, cpu_id, sizeof(cpu_rsrc_oid), cpu_rsrc_oid);
    if (rc != 0)
        return rc;

    /* Check that the CPU is grabbed by this process */
    rc = cfg_get_instance_str(NULL, NULL, cpu_rsrc_oid);

    switch (TE_RC_GET_ERROR(rc))
    {
        case TE_ENOENT:
            return 0;
        case 0:
            break;
        default:
            return rc;
    }

    return cfg_del_instance_fmt(FALSE, "%s", cpu_rsrc_oid);
}

te_errno
tapi_cfg_get_all_threads(const char *ta,  size_t *size,
                         tapi_cpu_index_t **indices)
{
    char pattern[CFG_OID_MAX] = {0};
    int rc;

    rc = snprintf(pattern, sizeof(pattern),
                  "/agent:%s/hardware:/node:*/cpu:*/core:*/thread:*", ta);

    if (rc < 0)
    {
        ERROR("Failed to format CPU configuration path query");
        return TE_OS_RC(TE_TAPI, errno);
    }

    if ((size_t)rc >= sizeof(pattern))
    {
        ERROR("Too long CPU configuration path query");
        return TE_RC(TE_TAPI, TE_ENAMETOOLONG);
    }

    return find_cpu_generic(pattern, "CPU thread", size, indices);
}

te_errno
tapi_cfg_cpu_get_nodes(const char *ta, size_t *n_nodes,
                       tapi_cpu_index_t **nodes)
{
    char pattern[CFG_OID_MAX] = {0};
    int rc;

    rc = snprintf(pattern, sizeof(pattern), "/agent:%s/hardware:/node:*", ta);
    if (rc < 0)
    {
        ERROR("Failed to format CPU configuration path query");
        return TE_OS_RC(TE_TAPI, errno);
    }

    if ((size_t)rc >= sizeof(pattern))
    {
        ERROR("Too long CPU configuration path query");
        return TE_RC(TE_TAPI, TE_ENAMETOOLONG);
    }

    return find_cpu_generic(pattern, "NUMA node", n_nodes, nodes);
}

te_errno
tapi_cfg_cpu_grab_by_prop(const char *ta, const tapi_cpu_prop_t *prop,
                          tapi_cpu_index_t *cpu_id)
{
    tapi_cpu_index_t *indices = NULL;
    size_t thread_count = 0;
    te_bool suitable = FALSE;
    te_errno rc = 0;
    size_t i;

    if ((rc = tapi_cfg_get_all_threads(ta, &thread_count, &indices)) != 0)
        goto out;

    for (i = 0; i < thread_count; i++)
    {
        /* Don't check thread suitability if prop is not specified */
        if (prop != NULL)
        {
            if ((rc = check_thread(ta, &indices[i], prop, &suitable)) != 0)
                goto out;

            if (!suitable)
                continue;
        }

        rc = tapi_cfg_cpu_grab_by_id(ta, &indices[i]);
        if (rc == 0) /* Approptiate CPU is grabbed */
        {
            memcpy(cpu_id, &indices[i], sizeof(tapi_cpu_index_t));
            goto out;
        }
        else if (TE_RC_GET_ERROR(rc) == TE_EBUSY ||
                 TE_RC_GET_ERROR(rc) == TE_EEXIST) /* CPU is unavailable */
        {
            continue;
        }
        else /* An error occured */
        {
            goto out;
        }
    }

    rc = TE_RC(TE_TAPI, TE_ENOENT);

out:
    free(indices);

    return rc;
}

static te_bool
cpu_id_matches(unsigned long id, unsigned long required)
{
    return required == TAPI_CPU_ID_UNSPEC || id == required;
}

static te_bool
cpu_index_matches(const tapi_cpu_index_t *id, const tapi_cpu_index_t *required)
{
    return cpu_id_matches(id->node_id, required->node_id) &&
        cpu_id_matches(id->package_id, required->package_id) &&
        cpu_id_matches(id->core_id, required->core_id) &&
        cpu_id_matches(id->thread_id, required->thread_id);
}

static unsigned int
get_cpu_count_by_topology(unsigned int n_cpus, const tapi_cpu_index_t *cpu_ids,
                          const tapi_cpu_index_t *topology)
{
    unsigned int count = 0;
    unsigned int i;

    if (topology == NULL)
        return n_cpus;

    for (i = 0; i < n_cpus; i++)
    {
        if (cpu_index_matches(&cpu_ids[i], topology))
            count++;
    }

    return count;
}

te_errno
tapi_cfg_cpu_grab_multiple_with_id(const char *ta,
                                   const tapi_cpu_prop_t *prop,
                                   const tapi_cpu_index_t *topology,
                                   unsigned int n_cpus,
                                   tapi_cpu_index_t *cpu_ids)
{
    tapi_cpu_index_t *indices = NULL;
    tapi_cpu_index_t *grabbed = NULL;
    unsigned int n_grabbed = 0;
    size_t thread_count = 0;
    te_errno rc = 0;
    size_t i;

    if (n_cpus == 0)
        return 0;

    if ((rc = tapi_cfg_get_all_threads(ta, &thread_count, &indices)) != 0)
        goto out;

    if (get_cpu_count_by_topology(thread_count, indices, topology) < n_cpus)
    {
        rc = TE_RC(TE_TAPI, TE_ENOENT);
        goto out;
    }

    grabbed = TE_ALLOC(sizeof(*grabbed) * thread_count);
    if (grabbed == NULL)
    {
        rc = TE_RC(TE_TAPI, TE_ENOMEM);
        goto out;
    }

    for (i = 0; i < thread_count && n_grabbed < n_cpus; i++)
    {
        te_bool suitable;

        if (topology != NULL && !cpu_index_matches(&indices[i], topology))
            continue;

        /* Don't check thread suitability if prop is not specified */
        if (prop != NULL)
        {
            if ((rc = check_thread(ta, &indices[i], prop, &suitable)) != 0)
                goto out;

            if (!suitable)
                continue;
        }

        rc = tapi_cfg_cpu_grab_by_id(ta, &indices[i]);
        if (rc == 0) /* Approptiate CPU is grabbed */
        {
            memcpy(&grabbed[n_grabbed], &indices[i], sizeof(*grabbed));
            n_grabbed++;
        }
        else if (TE_RC_GET_ERROR(rc) == TE_EBUSY ||
                 TE_RC_GET_ERROR(rc) == TE_EEXIST) /* CPU is unavailable */
        {
            continue;
        }
        else /* An error occured */
        {
            goto out;
        }
    }

    if (n_grabbed < n_cpus)
    {
        rc = TE_RC(TE_TAPI, TE_ENOENT);
        goto out;
    }

    memcpy(cpu_ids, grabbed, sizeof(*cpu_ids) * n_cpus);

out:
    if (rc != 0)
    {
        for (i = 0; i < n_grabbed; i++)
            (void)tapi_cfg_cpu_release_by_id(ta, &grabbed[i]);
    }

    free(indices);
    free(grabbed);

    return rc;
}

te_errno
tapi_cfg_cpu_grab_multiple_on_single_node(const char *ta,
                                          const tapi_cpu_prop_t *prop,
                                          unsigned int n_cpus,
                                          tapi_cpu_index_t *cpu_ids)
{
    tapi_cpu_index_t *indices = NULL;
    tapi_cpu_index_t *nodes = NULL;
    size_t thread_count;
    size_t n_nodes;
    te_errno rc;
    size_t i;

    rc = tapi_cfg_cpu_get_nodes(ta, &n_nodes, &nodes);
    if (rc != 0)
        goto out;

    if ((rc = tapi_cfg_get_all_threads(ta, &thread_count, &indices)) != 0)
        goto out;

    for (i = 0; i < n_nodes; i++)
    {
        tapi_cpu_index_t topology = {
            .node_id = nodes[i].node_id,
            .package_id = TAPI_CPU_ID_UNSPEC,
            .core_id = TAPI_CPU_ID_UNSPEC,
            .thread_id = TAPI_CPU_ID_UNSPEC,
        };

        if (get_cpu_count_by_topology(thread_count, indices, &topology) <
            n_cpus)
        {
            continue;
        }

        rc = tapi_cfg_cpu_grab_multiple_with_id(ta, prop, &topology,
                                                n_cpus, cpu_ids);
        if (rc == 0)
            goto out;
    }

    rc = TE_RC(TE_TAPI, TE_ENOENT);

out:
    free(nodes);
    free(indices);

    return rc;
}
