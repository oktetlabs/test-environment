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
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
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
 * @param ap        List of arguments for format string.
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_sys_get_int(const char *ta, int *val,
                                     const char *fmt, ...)
                                  __attribute__((format(printf, 3, 4)));


/**
 * Set value of integer parameter in /sys: subtree.
 *
 * @param ta        Test agent name.
 * @param val       Value to set.
 * @param old_val   Where to save previous value (may be @c NULL).
 * @param fmt       Format string of the path.
 * @param ap        List of arguments for format string.
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_sys_set_int(const char *ta, int val, int *old_val,
                                     const char *fmt, ...)
                                  __attribute__((format(printf, 4, 5)));

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
                                  __attribute__((format(printf, 3, 4)));


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
                                  __attribute__((format(printf, 4, 5)));

/**
 * Get value of string parameter in /sys: subtree.
 *
 * @param ta        Test agent name.
 * @param val       Where to save pointer to allocated string
 *                  containing requested value.
 * @param fmt       Format string of the path.
 * @param ap        List of arguments for format string.
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_sys_get_str(const char *ta, char **val,
                                     const char *fmt, ...)
                                  __attribute__((format(printf, 3, 4)));

/**
 * Set value of string parameter in /sys: subtree.
 *
 * @param ta        Test agent name.
 * @param val       Value to set.
 * @param old_val   Where to save pointer to allocated string
 *                  containing the previous value (may be @c NULL).
 * @param fmt       Format string of the path.
 * @param ap        List of arguments for format string.
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_sys_set_str(const char *ta, const char *val,
                                     char **old_val, const char *fmt, ...)
                                  __attribute__((format(printf, 4, 5)));


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
 * @param ap        List of arguments for format string.
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_sys_ns_get_int(const char *ta, int *val,
                                        const char *fmt, ...)
                                        __attribute__((format(printf, 3, 4)));


/**
 * The same as tapi_cfg_sys_set_int() but try to set the option value in
 * default net namespace if it does not exist in current namespace.
 *
 * @param ta        Test agent name.
 * @param val       Value to set.
 * @param old_val   Where to save previous value (may be @c NULL).
 * @param fmt       Format string of the path.
 * @param ap        List of arguments for format string.
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_sys_ns_set_int(const char *ta, int val, int *old_val,
                                        const char *fmt, ...)
                                        __attribute__((format(printf, 4, 5)));

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
                                        __attribute__((format(printf, 3, 4)));


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
                                        __attribute__((format(printf, 4, 5)));

/**
 * The same as tapi_cfg_sys_get_str() but try to get the option value in
 * default net namespace if it does not exist in current namespace.
 *
 * @param ta        Test agent name.
 * @param val       Where to save pointer to allocated string
 *                  containing requested value.
 * @param fmt       Format string of the path.
 * @param ap        List of arguments for format string.
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_sys_ns_get_str(const char *ta, char **val,
                                        const char *fmt, ...)
                                        __attribute__((format(printf, 3, 4)));

/**
 * The same as tapi_cfg_sys_set_str() but try to set the option value in
 * default net namespace if it does not exist in current namespace.
 *
 * @param ta        Test agent name.
 * @param val       Value to set.
 * @param old_val   Where to save pointer to allocated string
 *                  containing the previous value (may be @c NULL).
 * @param fmt       Format string of the path.
 * @param ap        List of arguments for format string.
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_sys_ns_set_str(const char *ta, const char *val,
                                        char **old_val, const char *fmt, ...)
                                        __attribute__((format(printf, 4, 5)));

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_CFG_PROC_SYS_H__ */

/**@} <!-- END tapi_cfg_sys --> */
