/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief API to modify TRC tags from prologues
 *
 * @defgroup ts_tapi_tags TRC tags modification
 * @ingroup te_ts_tapi
 * @{
 *
 * Declaration of API to modify TRC tags from prologues.
 *
 *
 * Copyright (C) 2022-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_TAPI_TAGS_H__
#define __TE_TAPI_TAGS_H__

#include "te_errno.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TE_CFG_TRC_TAGS_FMT "/local:/trc_tags:%s"

/**
 * Modify the set of defined TRC tags for tests by adding a new tag to the
 * configurator database. This function should be used to pass the TRC tag from
 * the root prologue only to the tester.
 *
 * @param tag       Tag name.
 * @param value     Tag value for TRC tags expressions (can be @c NULL).
 *
 * @return          Status code.
 */
extern te_errno tapi_tags_add_tag(const char *tag, const char *value);

/**
 * Add Linux kernel TRC tag with major and minor numbers suitable for
 * comparison.
 *
 * This function should be used to pass the TRC tag from the root prologue
 * only to the tester.
 *
 * @param ta        Test agent name.
 * @param prefix    Tag name prefix or @c NULL
 *
 * @return          Status code.
 */
extern te_errno tapi_tags_add_linux_mm(const char *ta, const char *prefix);

/**
 * Add PCI device tags of the network interface.
 *
 * This function should be used to pass the TRC tag from the root prologue
 * only to the tester.
 *
 * @param ta        Test agent name.
 * @param if_name   Network interface name.
 *
 * @return          Status code.
 */
extern te_errno tapi_tags_add_net_pci_tags(const char *ta, const char *if_name);

/**
 * Add firmware version tag of the network interface.
 *
 * This function should be used to pass the TRC tag from the root prologue
 * only to the tester.
 *
 * @param ta            Test agent name.
 * @param if_name       Network interface name.
 * @param tag_prefix    Tag name prefix or @c NULL.
 *
 * @return          Status code.
 */
extern te_errno tapi_tags_add_firmwareversion_tag(const char *ta,
                                                  const char *if_name,
                                                  const char *tag_prefix);

/**
 * Add PHY tags of the network interface.
 *
 * Adds such PHY tags as:
 * - @c "sp-" - IUT speed state
 * - @c "port-" - IUT port mode status.
 *
 * This function should be used to pass the TRC tag from the root prologue
 * only to the tester.
 *
 * @param iut_ta        IUT agent name.
 * @param iut_if_name   IUT network interface name.
 * @param tst_ta        TST agent name or @c NULL.
 * @param tst_if_name   TST network interface name or @c NULL.
 * @param tag_prefix    Tag name prefix or @c NULL.
 *
 * @return              Status code.
 *
 * @note TST TA and interface name can be used to compare with IUT configuration
 *       to ensure that the TST and IUT configuration (speed and port mode
 *       parameters) values are synchronized correctly. If TST values are
 *       @c NULL, the comparison will be skipped. TST input data is optional.
 */
extern te_errno tapi_tags_add_phy_tags(const char *iut_ta,
                                       const char *iut_if_name,
                                       const char *tst_ta,
                                       const char *tst_if_name,
                                       const char *tag_prefix);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_TAGS_H__ */

/**@} <!-- END ts_tapi_tags --> */
