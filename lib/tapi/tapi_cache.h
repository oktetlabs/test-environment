/** @file
 * @brief API to deal with cached data
 *
 * @defgroup tapi_cache Test API to handle a cache
 * @ingroup tapi_conf
 * @{
 *
 * Copyright (C) 2003-2019 OKTET Labs. All rights reserved.
 *
 * @author Ivan Melnikov <Ivan.Melnikov@oktetlabs.ru>
 *
 *
 * Generic API to operate the Configurator subtree /volatile/cache.
 * Configurator subtree (@path{doc/cm/cm_volatile.xml}).
 *
 * @section tapi_cache_synopsis Synopsis
 *
 * Test API is intended to manage data which could be obtained with different
 * ways. The main idea is to reduce a number of expensive read operations of
 * data which are changed rarely, and keep the cache up to date independently on
 * what way is used to obtain data.
 *
 * @section tapi_cache_terms Terminology
 *
 * @attr_name{area} - Subtree handled by same actualization callbacks set.
 * For example, there are two areas in @conf_path{/volatile/cache/foo/bar}:
 * @attr_val{foo} and @attr_val{foo/bar}
 * Note, it is not mandatory to register callbacks for each area
 *
 * @attr_name{method} - The way to gather data to be cached. Do not mistake it
 * with callback functions! For example, @attr_val{crm}, @attr_val{cwmp}, etc.
 * It <b>MUST</b> be a leaf of Configurator tree.
 * For example, @conf_path{/volatile:/cache:/foo:FOO/bar:BAR/baz:crm}
 *
 * @section tapi_cache_example Example
 *
 * To use this feature, you need to register @attr_name{cache} node
 * and its children in Configurator
 *
 * Lets assume for instance, we need to operate @conf_path{wifi/ssid/path}
 * cache area
 *
 * @code
 * <?xml version="1.0"?>
 * <history>
 *   <register>
 *     <object oid="/volatile"
 *         access="read_create" type="none" volatile="true"/>
 *     <object oid="/volatile/cache"
 *         access="read_create" type="none"/>
 *     <object oid="/volatile/cache/wifi"
 *         access="read_create" type="none"/>
 *     <object oid="/volatile/cache/wifi/ssid"
 *         access="read_create" type="none"/>
 *     <object oid="/volatile/cache/wifi/ssid/path"
 *         access="read_create" type="string"/>
 *   </register>
 *
 *   <add>
 *     <instance oid="/volatile:"/>
 *     <instance oid="/volatile:/cache:"/>
 *   </add>
 * </history>
 * @endcode
 *
 * Example of usage in the test
 *
 * @code
 * #include "tapi_cache.h"
 * ...
 *
 * CHECK_RC(tapi_cache_register("crm", "wifi", wifi_crm_cb));
 * CHECK_RC(tapi_cache_register("cwmp", "wifi", wifi_cwmp_cb));
 *
 * CHECK_RC(tapi_cache_actualize("crm", &wifi_crm_opaque, "wifi:%s", band));
 * CHECK_RC(tapi_cache_actualize("cwmp", &wifi_cwmp_opaque, "wifi:*"));
 * ...
 *
 * # Get all SSIDs of band 5G from the cache
 * struct {
 *     char *ssid[];
 * } ap;
 * CHECK_RC(tapi_cache_find(get_ssids, &ap, "wlan:5G/ssid:*"));
 *
 * # Get a CRM path to the first 5G SSID from the cache
 * char *crm_path;
 * CHECK_RC(tapi_cache_get_string(&crm_path,
 *                                "wifi:5G/ssid:%s/path:crm", ap.ssid[0]));
 *
 * # Get a CWMP path to the first 5G SSID from the cache
 * char *cwmp_path;
 * CHECK_RC(tapi_cache_get_string(&cwmp_path,
 *                                "wifi:5G/ssid:%s/path:cwmp", ap.ssid[0]));
 * ...
 *
 * # Add a new 5G SSID "test" via CRM:
 * crm_path = some_tapi_add_ssid_via_crm(...);
 * # Register this newly added SSID to cache
 * CHECK_RC(tapi_cache_add_string(crm_path, "wifi:5G/ssid:test/path:crm"));
 * ...
 *
 * CHECK_RC(tapi_cache_invalidate(NULL, "wifi:*"));
 * @endcode
 */

#ifndef __TE_TAPI_CACHE_H__
#define __TE_TAPI_CACHE_H__

#include "te_defs.h"
#include "conf_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Cache area configurator root object */
#define TAPI_CACHE_ROOT_OID     "/volatile/cache"
/** Cache area configurator root instance */
#define TAPI_CACHE_ROOT_INST    "/volatile:/cache:"
/**
 * Cache instance pointing to all cache data (cache root). Useful if you need
 * to actualize all cache instances at once
 */
#define TAPI_CACHE_ALL          ""


/**
 * Type of callback function which can be called by tapi_cache_actualize()
 * and registered by tapi_cache_register()
 *
 * @param oid       Object instance identifier (root of subtree to be
 *                  actualized)
 * @param opaque    Opaque data containing arguments of certain callback
 *
 * @return Status code
 */
typedef te_errno (*tapi_cache_cb)(const char *oid, void *opaque);


/**
 * Register a callback function for particular cache actualization @p method
 * of certain @p area
 *
 * @param method        Method
 * @param area          Area
 * @param cb_func       Callback function
 *
 * @return Status code
 */
extern te_errno tapi_cache_register(const char *method, const char *area,
                                    tapi_cache_cb cb_func);

/**
 * Actualize certain cache area instances. It calls a suitable function
 * registered with tapi_cache_register() to gather data required to fill
 * the cache area with particular @p method
 *
 * @note It invalidates the area instances of certain @p method before
 * actualization. Also it is possible to call the function on parent instance
 * which does not have registered callback but whose children have ones.
 * Caution: keep in mind in this case it can call different callbacks with the
 * same opaque.
 * For instance, if we had the following areas with registered callbacks:
 * @conf_path{foo/bar}, @conf_path{foo/bar/baz}, @conf_path{qux},
 * the function on area instance @ref TAPI_CACHE_ALL would invoke callbacks
 * of @conf_path{foo/bar} and @conf_path{qux}
 *
 *
 * @param method        Method
 * @param opaque        Opaque argument to pass to the callback function
 *                      registered with tapi_cache_register()
 * @param area_ptrn     Format string of area instance, can contain wildcards
 *                      to actualize several instances of area.
 *                      For example, "foo:*", "foo:FOO", or @ref TAPI_CACHE_ALL
 * @param ...           Format string arguments
 *
 * @return Status code
 * @retval 0            Instance has been actualized completely
 * @retval TE_ECHILD    Some of subareas have not been actualized due to they
 *                      do not have actualization callbacks
 * @retval TE_ENOENT    Instance has not been actualized completely
 * @retval Any other    Another errors which should be handled as critical
 *
 * @sa tapi_cache_invalidate, tapi_cache_invalidate_all
 */
extern te_errno tapi_cache_actualize(const char *method, void *opaque,
                                     const char *area_ptrn, ...)
                                     __attribute__((format(printf, 3, 4)));

/**
 * Invalidate certain cache area instances.
 *
 * @param method        Method, can be @c NULL to invalidate all instances
 *                      indepenently on actualization method
 * @param area_ptrn     Format string of area instance, can contain wildcards
 *                      to invalidate several instances of area.
 *                      For example, "foo:*", or "foo:FOO"
 * @param ...           Format string arguments
 *
 * @return Status code
 *
 * @sa tapi_cache_actualize
 */
extern te_errno tapi_cache_invalidate(const char *method,
                                      const char *area_ptrn, ...)
                                      __attribute__((format(printf, 2, 3)));

/**
 * Add a new instance to cache area, add parent instances (with value
 * CFG_VAL(NONE, NULL)) as needed
 *
 * @note It is important for proper work of acrialize and invalidate actions
 * to follow agreement that leaf is a method
 *
 * @param type          Value type
 * @param value         Value to add
 * @param area_inst     Format string of area instance to add.
 *                      For example, "foo:%s/bar:%s/baz:METHOD", it is allowed
 *                      to use "foo:%s/bar:%s" if further a leaf instance of
 *                      method will be added according to agreement
 * @param ...           Format string arguments
 *
 * @return Status code
 *
 * @sa tapi_cache_add_string, tapi_cache_add_int, tapi_cache_add_addr
 */
extern te_errno tapi_cache_add(cfg_val_type type, const void *value,
                               const char *area_inst, ...)
                               __attribute__((format(printf, 3, 4)));

/**
 * Add a new string instance to cache area
 *
 * @param value         String value to add
 * @param area_inst     Format string of area instance to add
 * @param ...           Format string arguments
 *
 * @return Status code
 *
 * @sa tapi_cache_add
 */
extern te_errno tapi_cache_add_string(const char *value,
                                      const char *area_inst, ...)
                                      __attribute__((format(printf, 2, 3)));

/**
 * Add a new integer instance to cache area
 *
 * @param value         Integer value to add
 * @param area_inst     Format string of area instance to add
 * @param ...           Format string arguments
 *
 * @return Status code
 *
 * @sa tapi_cache_add
 */
extern te_errno tapi_cache_add_int(int value, const char *area_inst, ...)
                                   __attribute__((format(printf, 2, 3)));

/**
 * Add a new network address instance to cache area
 *
 * @param addr          Network address to add
 * @param area_inst     Format string of area instance to add
 * @param ...           Format string arguments
 *
 * @return Status code
 *
 * @sa tapi_cache_add
 */
extern te_errno tapi_cache_add_addr(const struct sockaddr *addr,
                                    const char *area_inst, ...)
                                    __attribute__((format(printf, 2, 3)));

/**
 * Delete instance(s) of cache area by pattern
 *
 * @note Prefer tapi_cache_invalidate() to this function since this one
 * knows nothing about methods and, hence, ignores them. It simply removes
 * area instances of all methods.
 *
 * @param area_ptrn     Format string of area instance, can contain wildcards
 *                      to delete several instances of area.
 *                      For example, "foo:*", or "foo:FOO"
 * @param ...           Format string arguments
 *
 * @return Status code
 */
extern te_errno tapi_cache_del(const char *area_ptrn, ...)
                               __attribute__((format(printf, 1, 2)));

/**
 * Find all area instances matching a pattern and call @p cb_func for each of
 * found items
 *
 * @param cb_func       Pointer to callback function
 * @param opaque        Opaque argument to pass to @p cb_func
 * @param area_ptrn     Format string of area instance, can contain wildcards
 *                      to find several instances of area.
 *                      For example, "foo:*", or "foo:FOO"
 * @param ...           Format string arguments
 *
 * @return Status code
 *
 * @sa cfg_find_pattern_iter_fmt
 */
extern te_errno tapi_cache_find(cfg_handle_cb_func cb_func, void *opaque,
                                const char *area_ptrn, ...)
                                __attribute__((format(printf, 3, 4)));

/**
 * Get a value of certain instance of cache area
 *
 * @param[inout] type       Value type location, may be @c NULL. If it
 *                          is @ref CVT_UNSPECIFIED, it will be updated with
 *                          obtained value type
 * @param[out]   value      Value location
 * @param[in]    area_inst  Area instance format string
 * @param[in]    ...        Format string arguments
 *
 * @return Status code
 *
 * @sa tapi_cache_get_string, tapi_cache_get_int, tapi_cache_get_addr
 */
extern te_errno tapi_cache_get(cfg_val_type *type, void *value,
                               const char *area_inst, ...)
                               __attribute__((format(printf, 3, 4)));

/**
 * Get a string value of certain instance of cache area
 *
 * @note Return value should be freed with free(3) when it is no longer needed
 *
 * @param[out]  value       Pointer to value location
 * @param[in]   area_inst   Area instance format string
 * @param[in]   ...         Format string arguments
 *
 * @return Status code
 *
 * @sa tapi_cache_get
 */
extern te_errno tapi_cache_get_string(char **value, const char *area_inst, ...)
                                      __attribute__((format(printf, 2, 3)));

/**
 * Get an integer value of certain instance of cache area
 *
 * @param[out]  value       Value location
 * @param[in]   area_inst   Area instance format string
 * @param[in]   ...         Format string arguments
 *
 * @return Status code
 *
 * @sa tapi_cache_get
 */
extern te_errno tapi_cache_get_int(int *value, const char *area_inst, ...)
                                   __attribute__((format(printf, 2, 3)));

/**
 * Get a network address value of certain instance of cache area
 *
 * @note Return value should be freed with free(3) when it is no longer needed
 *
 * @param[out]  addr        Pointer to address location
 * @param[in]   area_inst   Area instance format string
 * @param[in]   ...         Format string arguments
 *
 * @return Status code
 *
 * @sa tapi_cache_get
 */
extern te_errno tapi_cache_get_addr(struct sockaddr **addr,
                                    const char *area_inst, ...)
                                    __attribute__((format(printf, 2, 3)));

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_CACHE_H__ */
/** @} <!-- END tapi_cache --> */
