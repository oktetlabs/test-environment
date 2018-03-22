/** @file
 * @brief IP rule Configuration Model TAPI
 *
 * @defgroup tapi_conf_ip_rule IP rules configuration
 * @ingroup tapi_conf
 * @{
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights served.
 *
 * 
 *
 *
 * @author Oleg Sadakov <Oleg.Sadakov@oktetlabs.ru>
 */

#ifndef __TE_TAPI_CFG_IP_RULE_H_
#define __TE_TAPI_CFG_IP_RULE_H_

#include "conf_api.h"
#include "conf_ip_rule.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Entry to keep ip rule data */
typedef struct tapi_rt_ip_rule_entry {
    te_conf_ip_rule entry;      /**< Context of ip rule */
    cfg_handle hndl;            /**< Handle of the entry in configurator */
} tapi_rt_ip_rule_entry;

/**
 * Get routing policy database by the specified Test Agent
 *
 * @param [in] ta           Test Agent name
 * @param [in] addr_family  Address family of the rules (@c AF_INET)
 * @param [out]tbl          Pointer to the routing policy database
 * @param [out]n            The number of entries in the @p tbl
 *
 * @return  Status code
 *
 * @note Function allocates memory with @b malloc(), which should be free
 *       by the caller.
 */
extern te_errno tapi_cfg_get_rule_table(const char *ta, int addr_family,
                                        tapi_rt_ip_rule_entry **tbl,
                                        unsigned int *n);

/**
 * Add new rule into routing policy database
 *
 * @param [in]ta            Test Agent name
 * @param [in]addr_family   Address family of the rules (@c AF_INET)
 * @param [in]ip_rule       Rule for adding
 *
 * @return  Status code
 */
extern te_errno tapi_cfg_add_rule(const char *ta, int addr_family,
                                  te_conf_ip_rule *ip_rule);

/**
 * Delete the first existing rule from routing policy database
 *
 * @param [in]ta            Test Agent name
 * @param [in]addr_family   Address family of the rules (@c AF_INET)
 * @param [in]required      Required fields for a rule search
 * @param [in]ip_rule       Data to find and delete rule
 *
 * @return  Status code
 */
extern te_errno tapi_cfg_del_rule(const char *ta, int addr_family,
                                  uint32_t required,
                                  te_conf_ip_rule *ip_rule);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* !__TE_TAPI_CFG_IP_RULE_H_ */

/**@} <!-- END tapi_conf_ip_rule --> */
