/** @file
 * @brief Configuration TAPI to work with /local/host subtree
 *
 * @defgroup tapi_host_ns Agents, namespaces and interfaces relations
 * @ingroup tapi_conf
 * @{
 *
 * Definition of test API to manage the configurator subtree `/local/host`.
 * (@path{storage/cm/cm_local.xml})
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 * @author Andrey Dmitrov <Andrey.Dmitrov@oktetlabs.ru>
 */

#ifndef __TE_TAPI_HOST_NS_H__
#define __TE_TAPI_HOST_NS_H__

#include "te_defs.h"
#include "te_errno.h"
#include "conf_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Consider this API is enabled if Configurator object @b /local/host is
 * registered.
 *
 * @return @c TRUE if the test API is enabled.
 */
extern te_bool tapi_host_ns_enabled(void);

/**
 * Get hostname of test agent @p ta.
 *
 * @param ta    Test agent name
 * @param host  Hostname
 *
 * @return Status code.
 */
extern te_errno tapi_host_ns_get_host(const char *ta, char **host);

/**
 * Register test agent in the configuration tree @b /local/host.
 *
 * @param host  Hostname
 * @param ta    Test agent name
 * @param netns Network namespace name
 *
 * @return Status code.
 */
extern te_errno tapi_host_ns_agent_add(const char *host, const char *ta,
                                       const char *netns);

/**
 * Delete test agent from the configuration tree @b /local/host.
 *
 * @param ta    Test agent name
 *
 * @return Status code.
 */
extern te_errno tapi_host_ns_agent_del(const char *ta);

/**
 * Add interface to the agent subtree.
 *
 * @param ta            Test agent name
 * @param ifname        Interface name
 * @param parent_ifname Parent interface name or @c NULL
 *
 * @return Status code.
 */
extern te_errno tapi_host_ns_if_add(const char *ta, const char *ifname,
                                    const char *parent_ifname);

/**
 * Delete interface from the agent subtree.
 *
 * @param ta        Test agent name
 * @param ifname    Interface name
 * @param del_refs  Delete all parent referencesto this interface if @c TRUE
 *
 * @return Status code.
 */
extern te_errno tapi_host_ns_if_del(const char *ta, const char *ifname,
                                    te_bool del_refs);

/**
 * Add reference to a parent interface.
 *
 * @param ta            Test agent name
 * @param ifname        Interface name
 * @param parent_ta     Owner agent of the parent interface
 * @param parent_ifname Parent interface name
 *
 * @return Status code.
 */
extern te_errno tapi_host_ns_if_parent_add(const char *ta, const char *ifname,
                                           const char *parent_ta,
                                           const char *parent_ifname);

/**
 * Delete reference to a parent interface.
 *
 * @param ta            Test agent name
 * @param ifname        Interface name
 * @param parent_ta     Owner agent of the parent interface
 * @param parent_ifname Parent interface name
 *
 * @return Status code.
 */
extern te_errno tapi_host_ns_if_parent_del(const char *ta, const char *ifname,
                                           const char *parent_ta,
                                           const char *parent_ifname);

/**
 * Change interface net namespace and update all parents references
 * accordingly.
 *
 * @param ta        Test agent name
 * @param ifname    Interface name
 * @param ns_name   Network namespace name
 * @param ns_ta     Test agent in the namespace - new owner of the interface
 *
 * @return Status code.
 */
extern te_errno tapi_host_ns_if_change_ns(const char *ta, const char *ifname,
                                          const char *ns_name,
                                          const char *ns_ta);

/**
 * Type of callback function which can be passed to
 * tapi_host_ns_if_child_iter() or tapi_host_ns_if_parent_iter().
 *
 * @param ta        Test agent name
 * @param ifname    Interface name
 * @param opaque    Opaque user data
 *
 * @return Status code. Also see tapi_host_ns_if_child_iter() and
 *         tapi_host_ns_if_parent_iter() description for details.
 */
typedef te_errno (*tapi_host_ns_if_cb_func)(const char *ta,
                                            const char *ifname,
                                            void *opaque);

/**
 * Iterate by child interfaces.
 *
 * @param ta        Test agent name
 * @param ifname    Interface name
 * @param opaque    Opaque user data
 * @param cb        A callback function
 *
 * @return Status code. The function stops iterating if @p cb returns non-zero
 *         value.
 */
extern te_errno tapi_host_ns_if_child_iter(const char *ta, const char *ifname,
                                           tapi_host_ns_if_cb_func cb,
                                           void *opaque);

/**
 * Iterate by parent interfaces.
 *
 * @param ta        Test agent name
 * @param ifname    Interface name
 * @param opaque    Opaque user data
 * @param cb        A callback function
 *
 * @return Status code. The function stops iterating if @p cb returns non-zero
 *         value.
 */
extern te_errno tapi_host_ns_if_parent_iter(const char *ta,
                                            const char *ifname,
                                            tapi_host_ns_if_cb_func cb,
                                            void *opaque);

/**
 * Iterate by all grabbed interfaces on @p ta.
 *
 * @param ta        Test agent name
 * @param opaque    Opaque user data
 * @param cb        A callback function
 *
 * @return Status code. The function stops iterating if @p cb returns non-zero
 *         value.
 */
extern te_errno tapi_host_ns_if_ta_iter(const char *ta,
                                        tapi_host_ns_if_cb_func cb,
                                        void *opaque);

/**
 * Iterate by all grabbed interfaces on the @p host.
 *
 * @param host      Host name
 * @param opaque    Opaque user data
 * @param cb        A callback function
 *
 * @return Status code. The function stops iterating if @p cb returns non-zero
 *         value.
 */
extern te_errno tapi_host_ns_if_host_iter(const char *host,
                                          tapi_host_ns_if_cb_func cb,
                                          void *opaque);

/**
 * Get name of test agent which is in default net namespace on the same host
 * where @p ta is located.
 *
 * @param ta            Test agent name
 * @param ta_default    Name of the agent in default net namespace
 *
 * @return Status code.
 */
extern te_errno tapi_host_ns_agent_default(const char *ta, char **ta_default);

/**@} <!-- END tapi_host_ns --> */

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_HOST_NS_H__ */
