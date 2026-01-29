/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Test API to get/set parameters in /proc/sys/
 *
 * @defgroup tapi_cfg_sys System parameters configuration
 * @ingroup tapi_conf
 * @{
 *
 * Definitions of TAPI to get/set system parameters using `/agent/sys/`
 * Configurator subtree (@path{doc/cm/cm_base.xml}).
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_TAPI_CFG_PROC_SYS_H__
#define __TE_TAPI_CFG_PROC_SYS_H__

#include "te_config.h"
#include "te_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * In the following functions path to parameter should be relative
 * to /proc/sys/. For example, "net/ipv4/tcp_congestion_control".
 * If parameter has multiple fields (like tcp_wmem), field number should
 * be specified after a colon at the end of the path: "net/ipv4/tcp_wmem:0".
 */

/**
 * Get value of integer parameter in /sys: subtree.
 *
 * @param ta        Test agent name.
 * @param val       Where to save value.
 * @param fmt       Format string of the path.
 * @param ...       List of arguments for format string.
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_sys_get_int(const char *ta, int *val,
                                     const char *fmt, ...)
                                  TE_LIKE_PRINTF(3, 4);


/**
 * Set value of integer parameter in /sys: subtree.
 *
 * @param ta        Test agent name.
 * @param val       Value to set.
 * @param old_val   Where to save previous value (may be @c NULL).
 * @param fmt       Format string of the path.
 * @param ...       List of arguments for format string.
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_sys_set_int(const char *ta, int val, int *old_val,
                                     const char *fmt, ...)
                                  TE_LIKE_PRINTF(4, 5);

/**
 * Get value of uint64_t parameter in /sys: subtree.
 *
 * @param ta        Test agent name.
 * @param val       Where to save value.
 * @param fmt       Format string of the path.
 * @param ...       Format arguments.
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_sys_get_uint64(const char *ta, uint64_t *val,
                                        const char *fmt, ...)
                                  TE_LIKE_PRINTF(3, 4);


/**
 * Set value of uint64_t parameter in /sys: subtree.
 *
 * @param ta        Test agent name.
 * @param val       Value to set.
 * @param old_val   Where to save previous value (may be @c NULL).
 * @param fmt       Format string of the path.
 * @param ...       Format arguments.
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_sys_set_uint64(const char *ta, uint64_t val,
                                        uint64_t *old_val,
                                        const char *fmt, ...)
                                  TE_LIKE_PRINTF(4, 5);

/**
 * Get value of string parameter in /sys: subtree.
 *
 * @param ta        Test agent name.
 * @param val       Where to save pointer to allocated string
 *                  containing requested value.
 * @param fmt       Format string of the path.
 * @param ...       List of arguments for format string.
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_sys_get_str(const char *ta, char **val,
                                     const char *fmt, ...)
                                  TE_LIKE_PRINTF(3, 4);

/**
 * Set value of string parameter in /sys: subtree.
 *
 * @param ta        Test agent name.
 * @param val       Value to set.
 * @param old_val   Where to save pointer to allocated string
 *                  containing the previous value (may be @c NULL).
 * @param fmt       Format string of the path.
 * @param ...       List of arguments for format string.
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_sys_set_str(const char *ta, const char *val,
                                     char **old_val, const char *fmt, ...)
                                  TE_LIKE_PRINTF(4, 5);


/*
 * Functions tapi_cfg_sys_ns_* try to get or set /proc/sys values at the
 * current net namespace first. But if an option does not exist in the current
 * net namespace it is get/set in the default namespace.
 * `/local/host` configurator subtree must be configured to use these
 * functions.
 */

/**
 * The same as tapi_cfg_sys_get_int() but try to get the option value in
 * default net namespace if it does not exist in current namespace.
 *
 * @param ta        Test agent name.
 * @param val       Where to save value.
 * @param fmt       Format string of the path.
 * @param ...       List of arguments for format string.
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_sys_ns_get_int(const char *ta, int *val,
                                        const char *fmt, ...)
                                        TE_LIKE_PRINTF(3, 4);


/**
 * The same as tapi_cfg_sys_set_int() but try to set the option value in
 * default net namespace if it does not exist in current namespace.
 *
 * @param ta        Test agent name.
 * @param val       Value to set.
 * @param old_val   Where to save previous value (may be @c NULL).
 * @param fmt       Format string of the path.
 * @param ...       List of arguments for format string.
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_sys_ns_set_int(const char *ta, int val, int *old_val,
                                        const char *fmt, ...)
                                        TE_LIKE_PRINTF(4, 5);

/**
 * The same as tapi_cfg_sys_get_uint64() but try to get the option value in
 * default net namespace if it does not exist in current namespace.
 *
 * @param ta        Test agent name.
 * @param val       Where to save value.
 * @param fmt       Format string of the path.
 * @param ...       Format arguments.
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_sys_ns_get_uint64(const char *ta, uint64_t *val,
                                           const char *fmt, ...)
                                        TE_LIKE_PRINTF(3, 4);


/**
 * The same as tapi_cfg_sys_set_uint64() but try to set the option value in
 * default net namespace if it does not exist in current namespace.
 *
 * @param ta        Test agent name.
 * @param val       Value to set.
 * @param old_val   Where to save previous value (may be @c NULL).
 * @param fmt       Format string of the path.
 * @param ...       Format arguments.
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_sys_ns_set_uint64(const char *ta, uint64_t val,
                                           uint64_t *old_val,
                                           const char *fmt, ...)
                                        TE_LIKE_PRINTF(4, 5);

/**
 * The same as tapi_cfg_sys_get_str() but try to get the option value in
 * default net namespace if it does not exist in current namespace.
 *
 * @param ta        Test agent name.
 * @param val       Where to save pointer to allocated string
 *                  containing requested value.
 * @param fmt       Format string of the path.
 * @param ...       List of arguments for format string.
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_sys_ns_get_str(const char *ta, char **val,
                                        const char *fmt, ...)
                                        TE_LIKE_PRINTF(3, 4);

/**
 * The same as tapi_cfg_sys_set_str() but try to set the option value in
 * default net namespace if it does not exist in current namespace.
 *
 * @param ta        Test agent name.
 * @param val       Value to set.
 * @param old_val   Where to save pointer to allocated string
 *                  containing the previous value (may be @c NULL).
 * @param fmt       Format string of the path.
 * @param ...       List of arguments for format string.
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_sys_ns_set_str(const char *ta, const char *val,
                                        char **old_val, const char *fmt, ...)
                                        TE_LIKE_PRINTF(4, 5);

/** Sysctl subtrees under /net/ipv4 and /net/ipv6. */
typedef enum tapi_cfg_sys_ip_net_subtree {
    /** Interface-related network settings. */
    TAPI_CFG_SYS_IP_SUBTREE_CONF,
    /** Neighbor and address resolution settings. */
    TAPI_CFG_SYS_IP_SUBTREE_NEIGH,
} tapi_cfg_sys_ip_net_subtree;

/** Specific instance inside a sysctl subtree. */
typedef enum tapi_cfg_sys_ip_instance_kind {
    /** Settings applied to all interfaces. */
    TAPI_CFG_SYS_IP_INST_ALL,
    /** Default settings for newly created interfaces. */
    TAPI_CFG_SYS_IP_INST_DEFAULT,
} tapi_cfg_sys_ip_instance_kind;

/**
 * Acquire a Configurator resource associated with an IP-related
 * sysctl subtree.
 *
 * The resource is managed by the Configurator framework and is released
 * automatically during configuration cleanup.
 *
 * @param ta        Test agent name.
 * @param af        Address family.
 * @param subtree   Sysctl subtree.
 * @param inst      Target instance.
 * @param shared    Lock type.
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_sys_ip_grab(const char *ta, int af,
                                     tapi_cfg_sys_ip_net_subtree subtree,
                                     tapi_cfg_sys_ip_instance_kind inst,
                                     cs_rsrc_lock_type lock_type);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_CFG_PROC_SYS_H__ */

/**@} <!-- END tapi_cfg_sys --> */
