/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Test API to control IRQ interrupt configurator tree.
 *
 * Definition of TAPI to configure IRQ interrupts.
 *
 * Copyright (C) 2026 OKTET Ltd.
 */

#ifndef __TE_TAPI_CFG_IRQ_H__
#define __TE_TAPI_CFG_IRQ_H__

/**
 * Set affinity mask for the specified IRQ.
 *
 * @param ta            Test agent.
 * @param interface     Interface name.
 * @param irq           IRQ number.
 * @param mask          Affinity mask.
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_irq_affinity_set(const char *ta,
                                          const char *interface,
                                          unsigned int irq,
                                          uint64_t mask);

/**
 * Set affinity mask for all IRQs on the given interface.
 *
 * @param ta            Test agent.
 * @param interface     Interface name.
 * @param mask          Affinity mask.
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_irq_affinity_set_all(const char *ta,
                                              const char *interface,
                                              uint64_t mask);

#endif /* !__TE_TAPI_CFG_IRQ_H__ */
