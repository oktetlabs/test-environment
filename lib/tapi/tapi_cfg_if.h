/** @file
 * @brief Network interface configuration
 *
 * @defgroup tapi_conf_if Network interface configuration TAPI
 * @ingroup tapi_conf
 * @{
 *
 * Copyright (C) 2003-2021 OKTET Labs. All rights reserved.
 *
 *
 *
 * @author Andrey A. Dmitrov <Andrey.Dmitrov@oktetlabs.ru>
 */

#ifndef __TE_TAPI_CFG_IF_H__
#define __TE_TAPI_CFG_IF_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Check whether a given interface feature is read-only
 *
 * @param ta                Test agent name
 * @param ifname            Interface name
 * @param feature_name      Feature name
 * @param readonly          Will be set to @c TRUE if the feature
 *                          is read-only
 *
 * @return Status code
 */
extern te_errno tapi_cfg_if_feature_is_readonly(const char *ta,
                                                const char *ifname,
                                                const char *feature_name,
                                                te_bool *readonly);

/**
 * Check whether a given interface feature is present
 *
 * @param ta                Test agent name
 * @param ifname            Interface name
 * @param feature_name      Feature name
 * @param present           Will be set to @c TRUE if the feature
 *                          is present
 *
 * @return Status code
 */
extern te_errno tapi_cfg_if_feature_is_present(const char *ta,
                                               const char *ifname,
                                               const char *feature_name,
                                               te_bool *present);

/**
 * Get feature value of an ethernet interface
 *
 * @param ta                Test agent name
 * @param ifname            Interface name
 * @param feature_name      Feature name
 * @param feature_value_out Location for the value
 *
 * @return Status code
 */
extern te_errno tapi_cfg_if_feature_get(const char *ta,
                                        const char *ifname,
                                        const char *feature_name,
                                        int        *feature_value_out);

/**
 * Set feature value of an ethernet interface
 *
 * @param ta            Test agent name
 * @param ifname        Interface name
 * @param feature_name  Feature name
 * @param feature_value Location for the value
 *
 * @return Status code
 */
extern te_errno tapi_cfg_if_feature_set(const char *ta,
                                        const char *ifname,
                                        const char *feature_name,
                                        int         feature_value);

/**
 * Set feature value of an ethernet interface and all its parents if they are.
 * Ignore @c EOPNOTSUPP failures if it is successful at least for one
 * interface.
 *
 * @param ta            Test agent name
 * @param ifname        Interface name
 * @param feature_name  Feature name
 * @param feature_value Location for the value
 *
 * @return Status code
 */
extern te_errno tapi_cfg_if_feature_set_all_parents(const char *ta,
                                                    const char *ifname,
                                                    const char *feature_name,
                                                    int feature_value);

/**
 * Get driver name of a network interface.
 *
 * @param ta            Test agent name
 * @param ifname        Interface name
 * @param drivername    Driver name, obtained value should be released if
 *                      not required anymore.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_if_deviceinfo_drivername_get(const char *ta,
                                                      const char *ifname,
                                                      char **drivername);

/**
 * Get driver version of a network interface.
 *
 * @param ta            Test agent name
 * @param ifname        Interface name
 * @param driverversion Driver version, obtained value should be released if
 *                      not required anymore.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_if_deviceinfo_driverversion_get(const char *ta,
                                                         const char *ifname,
                                                         char **driverversion);

/**
 * Get firmware version of a network interface.
 *
 * @param ta                Test agent name
 * @param ifname            Interface name
 * @param firmwareversion   Firmware version, obtained value should be
 *                          released if not required anymore.
 *
 * @return Status code
 */
extern te_errno tapi_cfg_if_deviceinfo_firmwareversion_get(
                                                    const char *ta,
                                                    const char *ifname,
                                                    char **firmwareversion);

/**
 * Get GRO value of an ethernet interface
 *
 * @param ta        Test agent name
 * @param ifname    Interface name
 * @param gro       Location for the GRO value
 *
 * @return Status code
 */
extern te_errno tapi_cfg_if_gro_get(const char *ta, const char *ifname,
                                    int *gro);

/**
 * Get GSO value of an ethernet interface
 *
 * @param ta        Test agent name
 * @param ifname    Interface name
 * @param gso       Location for the GSO value
 *
 * @return Status code
 */
extern te_errno tapi_cfg_if_gso_get(const char *ta, const char *ifname,
                                    int *gso);

/**
 * Get TSO value of an ethernet interface
 *
 * @param ta        Test agent name
 * @param ifname    Interface name
 * @param tso       The TSO value location
 *
 * @return Status code
 */
extern te_errno tapi_cfg_if_tso_get(const char *ta, const char *ifname,
                                    int *tso);

/**
 * Get flags value of an ethernet interface
 *
 * @param ta        Test agent name
 * @param ifname    Interface name
 * @param flags     Location for the eth flags
 *
 * @return Status code
 */
extern te_errno tapi_cfg_if_flags_get(const char *ta, const char *ifname,
                                      int *flags);

/**
 * Set GRO value of an ethernet interface
 *
 * @param ta        Test agent name
 * @param ifname    Interface name
 * @param gro       A new GRO value
 *
 * @return Status code
 */
extern te_errno tapi_cfg_if_gro_set(const char *ta, const char *ifname,
                                    int gro);

/**
 * Set GSO value of an ethernet interface
 *
 * @param ta        Test agent name
 * @param ifname    Interface name
 * @param gso       A new GSO value
 *
 * @return Status code
 */
extern te_errno tapi_cfg_if_gso_set(const char *ta, const char *ifname,
                                 int gso);

/**
 * Set TSO value of an ethernet interface
 *
 * @param ta        Test agent name
 * @param ifname    Interface name
 * @param tso       The TSO value
 *
 * @return Status code
 */
extern te_errno tapi_cfg_if_tso_set(const char *ta, const char *ifname,
                                    int tso);

/**
 * Set flags value of an ethernet interface
 *
 * @param ta        Test agent name
 * @param ifname    Interface name
 * @param flags     A new eth flags value
 *
 * @return Status code
 */
extern te_errno tapi_cfg_if_flags_set(const char *ta, const char *ifname,
                                      int flags);

/**
 * Reset an ethernet interface
 *
 * @param ta        Test agent name
 * @param ifname    Interface name
 *
 * @return Status code
 */
extern te_errno tapi_cfg_if_reset(const char *ta, const char *ifname);


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_CFG_IF_H__ */

/**@} <!-- END tapi_conf_if --> */
