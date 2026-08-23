/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2026 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief Test API to control IRQ interrupt configuration model
 *
 * Definition of test API to configure IRQ interrupts
 * (doc/cm/cm_if_irq.yml).
 */

#ifndef __TE_TAPI_CFG_IRQ_H__
#define __TE_TAPI_CFG_IRQ_H__

#include "te_defs.h"
#include "te_errno.h"
#include "te_stdint.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup tapi_conf_irq Interface interrupts configuration
 * @ingroup tapi_conf_link
 * @{
 *
 * Query and change the SMP affinity of the interrupts of a network
 * interface.
 *
 * Affinity is expressed in two interchangeable ways:
 * - as a CPU list in the Linux kernel syntax, e.g. @c "0-3,8", which
 *   places no limit on the number of CPUs;
 * - as a 64-bit mask, which is easier to compute in a test but cannot
 *   address CPUs numbered 64 and above.
 */

/**
 * Get the affinity of a single IRQ as a CPU list.
 *
 * @param[in]  ta        Test agent name.
 * @param[in]  if_name   Interface name.
 * @param[in]  irq       IRQ number.
 * @param[out] cpu_list  CPU list in the Linux kernel syntax,
 *                       e.g. @c "0-3,8". The caller owns the string
 *                       and should free() it.
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_irq_affinity_get(const char *ta,
                                          const char *if_name,
                                          unsigned int irq,
                                          char **cpu_list);

/**
 * Set the affinity of a single IRQ from a CPU list.
 *
 * @param ta        Test agent name.
 * @param if_name   Interface name.
 * @param irq       IRQ number.
 * @param cpu_list  CPU list in the Linux kernel syntax,
 *                  e.g. @c "0-3,8". Must not be empty.
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_irq_affinity_set(const char *ta,
                                          const char *if_name,
                                          unsigned int irq,
                                          const char *cpu_list);

/**
 * Set the affinity of every IRQ of an interface from a CPU list.
 *
 * @param ta        Test agent name.
 * @param if_name   Interface name.
 * @param cpu_list  CPU list in the Linux kernel syntax,
 *                  e.g. @c "0-3,8". Must not be empty.
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_irq_affinity_set_all(const char *ta,
                                              const char *if_name,
                                              const char *cpu_list);

/**
 * Get the affinity of a single IRQ as a 64-bit mask.
 *
 * @param[in]  ta        Test agent name.
 * @param[in]  if_name   Interface name.
 * @param[in]  irq       IRQ number.
 * @param[out] mask      Affinity mask, bit @c N set iff CPU @c N may
 *                       handle the interrupt.
 *
 * @return Status code.
 * @retval TE_ERANGE  The IRQ is allowed on a CPU numbered 64 or above,
 *                    so the affinity does not fit into a 64-bit mask.
 *                    Use tapi_cfg_irq_affinity_get() instead.
 */
extern te_errno tapi_cfg_irq_affinity_mask_get(const char *ta,
                                               const char *if_name,
                                               unsigned int irq,
                                               uint64_t *mask);

/**
 * Set the affinity of a single IRQ from a 64-bit mask.
 *
 * @param ta        Test agent name.
 * @param if_name   Interface name.
 * @param irq       IRQ number.
 * @param mask      Affinity mask, bit @c N set iff CPU @c N may handle
 *                  the interrupt. Must not be zero.
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_irq_affinity_mask_set(const char *ta,
                                               const char *if_name,
                                               unsigned int irq,
                                               uint64_t mask);

/**
 * Set the affinity of every IRQ of an interface from a 64-bit mask.
 *
 * @param ta        Test agent name.
 * @param if_name   Interface name.
 * @param mask      Affinity mask, bit @c N set iff CPU @c N may handle
 *                  the interrupt. Must not be zero.
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_irq_affinity_mask_set_all(const char *ta,
                                                   const char *if_name,
                                                   uint64_t mask);

/**@} <!-- END tapi_conf_irq --> */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* !__TE_TAPI_CFG_IRQ_H__ */
