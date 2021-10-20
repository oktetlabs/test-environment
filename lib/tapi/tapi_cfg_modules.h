/** @file
 * @brief Test API to configure kernel modules
 *
 * @defgroup tapi_conf_modules Kernel modules configuration
 * @ingroup tapi_conf
 * @{
 *
 * Definition of API to configure kernel modules and their parameters.
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * @author Konstantin Ushakov <Konstantin.Ushakov@oktetlabs.ru>
 */

#ifndef __TE_TAPI_CFG_MODULES_H__
#define __TE_TAPI_CFG_MODULES_H__

#include "te_defs.h"
#include "te_errno.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Add module into the list of the agent modules. Load if asked.
 * No error is returned if module is already added or loaded. Check
 * it beforehand if needed.
 *
 * @param ta_name        Name of the agent
 * @param mod_name       Name of the module
 * @param load           Load it (applicable only if no parameters are needed.)
 *
 * @return 0 or error
 */
te_errno tapi_cfg_module_add(const char *ta_name, const char *mod_name,
                             te_bool load);

/**
 * Load module with pre-configured parameters.
 *
 * @param ta_name        Name of the agent
 * @param mod_name       Name of the module
 *
 * @return 0 or error
 */
te_errno tapi_cfg_module_load(const char *ta_name, const char *mod_name);

/**
 * Unload module with pre-configured parameters.
 *
 * @param ta_name        Name of the agent
 * @param mod_name       Name of the module
 *
 * @return 0 or error
 */
te_errno tapi_cfg_module_unload(const char *ta_name, const char *mod_name);

/**
 * Set filename of the module to be loaded
 *
 * @param ta_name        Name of the agent
 * @param mod_name       Name of the module
 * @param filename       Filesystem path
 *
 * @return 0 or error
 */
te_errno tapi_cfg_module_filename_set(const char *ta_name,
                                      const char *mod_name,
                                      const char *filename);
/**
 * Add module parameter with specified value.
 *
 * @param ta_name        Name of the agent
 * @param mod_name       Name of the module
 * @param param          Name of the parameter
 * @param param_value    Value of the parameter
 *
 * @return 0 or error
 */
te_errno tapi_cfg_module_param_add(const char *ta_name, const char *mod_name,
                                   const char *param ,const char *param_value);

/**
 * Add module parameter with specified integer value.
 * Convenient wrapper around tapi_cfg_module_param_add().
 *
 * @param ta_name        Name of the agent
 * @param mod_name       Name of the module
 * @param param          Name of the parameter
 * @param param_value    Value of the parameter
 *
 * @return 0 or error
 */
te_errno tapi_cfg_module_int_param_add(const char *ta_name,
                                       const char *mod_name,
                                       const char *param,
                                       int param_value);

/**
 * Add a number of string params that go in pairs terminated by NULL.
 *
 * @param ta_name        Name of the agent
 * @param mod_name       Name of the module
 * @param ...            Param name, param value, NULL list.
 *
 * @return 0 or error
 */
te_errno tapi_cfg_module_params_add(const char *ta_name,
                                    const char *mod_name, ...);

/**
 * Add a number of int params that go in pairs terminated by NULL.
 *
 * @param ta_name        Name of the agent
 * @param mod_name       Name of the module
 * @param ...            Param name, param value (int), NULL list.
 *
 * @return 0 or error
 */
te_errno tapi_cfg_module_int_params_add(const char *ta_name,
                                        const char *mod_name, ...);

/**
 * Get value of the module parameter (as string).
 *
 * @param ta_name        Name of the agent
 * @param mod_name       Name of the kernel module
 * @param param_name     Name of the module parameter
 * @param param_value    Where to save pointer to string value of
 *                       the parameter (to be released by the caller)
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_module_param_get(const char *ta_name,
                                          const char *mod_name,
                                          const char *param_name,
                                          char **param_value);

/**
 * Get value of the module parameter (as integer number).
 *
 * @param ta_name        Name of the agent
 * @param mod_name       Name of the kernel module
 * @param param_name     Name of the module parameter
 * @param param_value    Where to save parameter value
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_module_param_get_int(const char *ta_name,
                                              const char *mod_name,
                                              const char *param_name,
                                              int *param_value);

/**
 * Given a module file in a TA directory, insert the former.
 * Take care of the module dependencies if required.
 *
 * @param  ta_name           The TA name
 * @param  module_name       The filename without ".ko" extension
 * @param  load_dependencies Take care of dependencies
 *
 * @retval Status code.
 */
extern te_errno tapi_cfg_module_add_from_ta_dir(const char *ta_name,
                                                const char *module_name,
                                                te_bool     load_dependencies);

/**
 *
 * Same as tapi_cfg_module_add_from_ta_dir(), but in case of a module file
 * absence in a TA directory, module addition falls back on loading module
 * shipped with the currently running kernel.
 *
 * @param  ta_name           The TA name
 * @param  module_name       The filename without ".ko" extension
 * @param  load_dependencies Take care of dependencies
 *
 * @retval Status code.
 */
extern te_errno tapi_cfg_module_add_from_ta_dir_or_fallback(const char *ta_name,
                                                     const char *module_name,
                                                     te_bool load_dependencies);

/**
 * Finish changing a module. The resource is set to shared and other
 * agents can access the module in shared mode. After this, module
 * load/unload, parameters manipulation are prohibited.
 *
 * @param ta_name        Name of the agent
 * @param mod_name       Name of the module
 */
extern te_errno tapi_cfg_module_change_finish(const char *ta_name,
                                              const char *mod_name);

/**
 * Get version of a module.
 *
 * @param ta_name        Name of the agent
 * @param module_name    Name of the kernel module
 * @param version        Where to save pointer to the version string
 *                       (memory should be released by the caller)
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_module_version_get(const char *ta_name,
                                            const char *module_name,
                                            char **version);

/**
 * Check that all the devices related to a given module are grabbed.
 *
 * @param ta_name        Name of the agent
 * @param module_name    Name of the kernel module
 * @param all_grabbed    Will be set to @c TRUE if all devices are grabbed
 *                       and to @c FALSE otherwise.
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_module_check_devices(const char *ta_name,
                                              const char *module_name,
                                              te_bool *all_grabbed);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_CFG_MODULES_H__ */

/**@} <!-- END tapi_conf_modules --> */
