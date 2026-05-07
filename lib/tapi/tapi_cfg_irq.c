/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Test API to control IRQ interrupt configurator tree.
 *
 * Implementation of TAPI to configure IRQ interrupts.
 *
 * Copyright (C) 2026 OKTET Ltd.
 */

#include "conf_api.h"
#include "tapi_cfg_irq.h"
#include "te_str.h"

te_errno
tapi_cfg_irq_affinity_set(const char *ta, const char *interface,
                          unsigned int irq, uint64_t mask)
{

    return cfg_set_instance_fmt(CFG_VAL(UINT64, mask),
                                "/agent:%s/interface:%s/irq:%u/smp_affinity:",
                                ta, interface, irq);
}

te_errno
tapi_cfg_irq_affinity_set_all(const char *ta, const char *interface,
                              uint64_t mask)
{
    te_errno rc;
    cfg_handle *irqs;
    unsigned int irqs_len;
    unsigned int i;
    unsigned int irq;
    char *inst_name;

    rc = cfg_find_pattern_fmt(&irqs_len, &irqs, "/agent:%s/interface:%s/irq:*",
                              ta, interface);
    if (rc != 0)
        return rc;

    for (i = 0; i < irqs_len; i++)
    {
        rc = cfg_get_inst_name(irqs[i], &inst_name);
        if (rc != 0)
            break;

        rc = te_strtoui(inst_name, 10, &irq);
        free(inst_name);
        if (rc != 0)
            break;

        rc = tapi_cfg_irq_affinity_set(ta, interface, irq, mask);
        if (rc != 0)
            break;
    }

    free(irqs);
    return rc;
}
