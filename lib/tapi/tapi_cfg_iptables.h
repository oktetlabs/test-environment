/** @file
 * @brief iptables Configuration Model TAPI
 *
 * Definition of test API for iptables configuration model
 * (storage/cm/cm_iptables.xml).
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_TAPI_CFG_IPTABLES_H__
#define __TE_TAPI_CFG_IPTABLES_H__

#include "te_defs.h"
#include "te_stdint.h"
#include "te_queue.h"
#include "te_ethernet.h"
#include "tapi_cfg_iptables.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup tapi_conf_iptable iptables configuration
 * @ingroup tapi_conf
 * @{
 */

/* Maximum length of iptables command string */
#define TAPI_CFG_IPTABLES_CMD_LEN_MAX    512

/**
 * Execute an iptables rule for the specific chain
 *
 * @param ta            - Test agent name
 * @param ifname        - Interface name
 * @param table         - Table to operate with (raw, filter, mangle, nat)
 * @param chain         - Chain name to operate with (without prefix)
 * @param rule          - Rule to add
 *
 * @return              Status of the operation
 */
extern te_errno tapi_cfg_iptables_cmd(const char *ta,
                                      const char *ifname,
                                      const char *table,
                                      const char *chain,
                                      const char *rule);

/**
 * Execute an iptables rule for the specific chain. The rule is specified
 * using a format string with arguments.
 *
 * @param ta            - Test agent name
 * @param ifname        - Interface name
 * @param table         - Table to operate with (raw, filter, mangle, nat)
 * @param chain         - Chain name to operate with (without prefix)
 * @param rule          - Formatted argument to combine the rule
 *
 * @return              Status of the operation
 */
extern te_errno tapi_cfg_iptables_cmd_fmt(const char *ta, const char *ifname,
                                          const char *table, const char *chain,
                                          const char *rule, ...);

/**
 * Install or delete jumping rule for the per-interface chain
 *
 * @param ta            - Test agent name
 * @param ifname        - Interface name
 * @param table         - Table to operate with (raw, filter, mangle, nat)
 * @param chain         - Chain name to operate with (without prefix)
 * @param enable        - Install or delete jumping rule
 *
 * @return              Status of the operation
 */
extern te_errno tapi_cfg_iptables_chain_set(const char *ta,
                                            const char *ifname,
                                            const char *table,
                                            const char *chain,
                                            te_bool enable);

/**
 * Add per-interface chain to the system
 *
 * @param ta            - Test agent name
 * @param ifname        - Interface name
 * @param table         - Table to operate with (raw, filter, mangle, nat)
 * @param chain         - Chain name to operate with (without prefix)
 * @param enable        - Install or not jumping rule to the built-in chain
 *
 * @return              Status of the operation
 */
extern te_errno tapi_cfg_iptables_chain_add(const char *ta,
                                            const char *ifname,
                                            const char *table,
                                            const char *chain,
                                            te_bool enable);

/**
 * Delete per-interface chain from the system
 *
 * @param ta            - Test agent name
 * @param ifname        - Interface name
 * @param table         - Table to operate with (raw, filter, mangle, nat)
 * @param chain         - Chain name to operate with (without prefix)
 *
 * @return              Status of the operation
 */
extern te_errno tapi_cfg_iptables_chain_del(const char *ta,
                                            const char *ifname,
                                            const char *table,
                                            const char *chain);

/**@} <!-- END tapi_conf_iptable --> */

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_CFG_IPTABLES_H__ */
