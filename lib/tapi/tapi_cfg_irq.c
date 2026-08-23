/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2026 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief Test API to control IRQ interrupt configuration model
 *
 * Implementation of test API to configure IRQ interrupts.
 */

#define TE_LGR_USER     "TAPI CFG IRQ"

#include "te_config.h"

#include "conf_api.h"
#include "logger_api.h"
#include "tapi_cfg_irq.h"
#include "te_intset.h"
#include "te_str.h"

te_errno
tapi_cfg_irq_affinity_get(const char *ta, const char *if_name,
                          unsigned int irq, char **cpu_list)
{
    return cfg_get_instance_string_fmt(cpu_list,
                                       "/agent:%s/interface:%s/irq:%u"
                                       "/smp_affinity:", ta, if_name, irq);
}

te_errno
tapi_cfg_irq_affinity_set(const char *ta, const char *if_name,
                          unsigned int irq, const char *cpu_list)
{
    if (te_str_is_null_or_empty(cpu_list))
    {
        ERROR("%s(): empty CPU list", __FUNCTION__);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    return cfg_set_instance_fmt(CFG_VAL(STRING, cpu_list),
                                "/agent:%s/interface:%s/irq:%u"
                                "/smp_affinity:", ta, if_name, irq);
}

te_errno
tapi_cfg_irq_affinity_set_all(const char *ta, const char *if_name,
                              const char *cpu_list)
{
    cfg_handle *handles = NULL;
    unsigned int n_handles;
    unsigned int i;
    te_errno rc;

    if (te_str_is_null_or_empty(cpu_list))
    {
        ERROR("%s(): empty CPU list", __FUNCTION__);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    rc = cfg_find_pattern_fmt(&n_handles, &handles,
                              "/agent:%s/interface:%s/irq:*/smp_affinity:",
                              ta, if_name);
    if (rc != 0)
        return rc;

    if (n_handles == 0)
        WARN("%s(): no IRQs found for %s on %s", __FUNCTION__, if_name, ta);

    for (i = 0; i < n_handles; i++)
    {
        rc = cfg_set_instance(handles[i], CFG_VAL(STRING, cpu_list));
        if (rc != 0)
            break;
    }

    free(handles);
    return rc;
}

te_errno
tapi_cfg_irq_affinity_mask_get(const char *ta, const char *if_name,
                               unsigned int irq, uint64_t *mask)
{
    char *cpu_list = NULL;
    te_errno rc;

    rc = tapi_cfg_irq_affinity_get(ta, if_name, irq, &cpu_list);
    if (rc != 0)
        return rc;

    rc = te_bits_parse(cpu_list, mask);
    if (rc != 0)
    {
        ERROR("%s(): affinity '%s' of IRQ %u does not fit into a 64-bit "
              "mask: %r", __FUNCTION__, cpu_list, irq, rc);
        rc = TE_RC(TE_TAPI, rc);
    }

    free(cpu_list);
    return rc;
}

/**
 * Convert a 64-bit affinity mask into a CPU list.
 *
 * @param[in]  mask      Affinity mask, must not be zero.
 * @param[out] cpu_list  CPU list, to be free()d by the caller.
 *
 * @return Status code.
 */
static te_errno
affinity_mask2cpu_list(uint64_t mask, char **cpu_list)
{
    char *result;

    if (mask == 0)
    {
        ERROR("%s(): empty affinity mask", __FUNCTION__);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    result = te_bits2string(mask);
    if (result == NULL)
        return TE_RC(TE_TAPI, TE_ENOMEM);

    *cpu_list = result;
    return 0;
}

te_errno
tapi_cfg_irq_affinity_mask_set(const char *ta, const char *if_name,
                               unsigned int irq, uint64_t mask)
{
    char *cpu_list = NULL;
    te_errno rc;

    rc = affinity_mask2cpu_list(mask, &cpu_list);
    if (rc != 0)
        return rc;

    rc = tapi_cfg_irq_affinity_set(ta, if_name, irq, cpu_list);
    free(cpu_list);
    return rc;
}

te_errno
tapi_cfg_irq_affinity_mask_set_all(const char *ta, const char *if_name,
                                   uint64_t mask)
{
    char *cpu_list = NULL;
    te_errno rc;

    rc = affinity_mask2cpu_list(mask, &cpu_list);
    if (rc != 0)
        return rc;

    rc = tapi_cfg_irq_affinity_set_all(ta, if_name, cpu_list);
    free(cpu_list);
    return rc;
}
