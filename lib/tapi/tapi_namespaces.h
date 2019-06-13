/** @file
 * @brief Linux namespaces configuration test API
 *
 * @defgroup tapi_namespaces Agent namespaces configuration
 * @ingroup tapi_conf
 * @{
 *
 * Definition of test API for linux namespaces configuration model
 * (@path{doc/cm/cm_namespace.xml}).
 *
 *
 * Copyright (C) 2003-2019 OKTET Labs. All rights reserved.
 *
 * 
 *
 *
 * @author Andrey Dmitrov <Andrey.Dmitrov@oktetlabs.ru>
 */

#ifndef __TE_TAPI_NAMESPACES_H__
#define __TE_TAPI_NAMESPACES_H__

#include "te_errno.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Add network namespace @p ns_name to the agent @p ta resources.
 *
 * @return Status code.
 */
extern te_errno tapi_netns_add_rsrc(const char *ta, const char *ns_name);

/**
 * Delete network namespace @p ns_name from the agent @p ta resources.
 *
 * @return Status code.
 */
extern te_errno tapi_netns_del_rsrc(const char *ta, const char *ns_name);

/**
 * Add network namespace @p ns_name to agent @p ta and grab it as the
 * resource.
 *
 * @return Status code.
 */
extern te_errno tapi_netns_add(const char *ta, const char *ns_name);

/**
 * Delete network namespace @p ns_name from the agent @p ta and from its
 * reources.
 *
 * @return Status code.
 */
extern te_errno tapi_netns_del(const char *ta, const char *ns_name);

/**
 * Move network interface @p if_name to namespace @p ns_name.
 *
 * @param ta        Test agent.
 * @param ns_name   The namespace name.
 * @param if_name   The interface name.
 *
 * Status code.
 */
extern te_errno tapi_netns_if_set(const char *ta, const char *ns_name,
                                  const char *if_name);

/**
 * Move network interface @p if_name from namespace @p ns_name.
 *
 * @param ta        Test agent.
 * @param ns_name   The namespace name.
 * @param if_name   The interface name.
 *
 * Status code.
 */
extern te_errno tapi_netns_if_unset(const char *ta, const char *ns_name,
                                    const char *if_name);

/**
 * Create network namespace and configure control network channel using veth
 * interfaces and iptables to route control traffic.
 *
 * @param ta        Test agent name
 * @param ns_name   The network namespace name
 * @param veth1     Veth interface name
 * @param veth2     Veth interface peer name
 * @param ctl_if    Control interface name on the test agent
 * @param rcfport   Port number to communicate with RCF
 *
 * @return Status code.
 */
extern te_errno tapi_netns_create_ns_with_net_channel(const char *ta,
                                                      const char *ns_name,
                                                      const char *veth1,
                                                      const char *veth2,
                                                      const char *ctl_if,
                                                      int rcfport);

/**
 * Add new test agent located in the specified network namespace @p ns_name.
 *
 * @param host      The target hostname
 * @param ns_name   The network namespace name
 * @param ta_name   The test agent name
 * @param ta_type   The test agent type
 * @param rcfport   Port number to communicate with RCF
 * @param ta_conn   Connection hostname or address or @c NULL
 *
 * @return Status code.
 */
extern te_errno tapi_netns_add_ta(const char *host, const char *ns_name,
                                  const char *ta_name, const char *ta_type,
                                  int rcfport, const char *ta_conn);

/**
 * Create network namespace and configure control network channel using
 * auxiliary macvlan interface. IP address is obtained using @b dhclient.
 *
 * @param ta            Test agent name
 * @param ns_name       The network namespace name
 * @param ctl_if        Control interface name on the test agent
 * @param macvlan_if    MAC VLAN interface name
 * @param addr          Obtained IP address
 * @param addr_len      Length of the buffer @p addr
 *
 * @return Status code.
 */
extern te_errno tapi_netns_create_ns_with_macvlan(const char *ta,
                                                  const char *ns_name,
                                                  const char *ctl_if,
                                                  const char *macvlan_if,
                                                  char *addr,
                                                  size_t addr_len);

/**
 * Destroy network namespace and undo other configurations applied by
 * function tapi_netns_create_ns_with_macvlan().
 *
 * @param ta            Test agent name
 * @param ns_name       The network namespace name
 * @param ctl_if        Control interface name on the test agent
 * @param macvlan_if    MAC VLAN interface name
 *
 * @return Status code.
 */
extern te_errno tapi_netns_destroy_ns_with_macvlan(const char *ta,
                                                   const char *ns_name,
                                                   const char *ctl_if,
                                                   const char *macvlan_if);

/**
 * Create network namespace and configure control network channel using
 * auxiliary ipvlan interface. IP address is obtained using @b dhclient.
 *
 * @param ta            Test agent name
 * @param ns_name       The network namespace name
 * @param ctl_if        Control interface name on the test agent
 * @param ipvlan_if     IP VLAN interface name
 * @param addr          Obtained IP address
 * @param addr_len      Length of the buffer @p addr
 *
 * @return Status code.
 */
extern te_errno tapi_netns_create_ns_with_ipvlan(const char *ta,
                                                 const char *ns_name,
                                                 const char *ctl_if,
                                                 const char *ipvlan_if,
                                                 char *addr,
                                                 size_t addr_len);

/**
 * Destroy network namespace and undo other configurations applied by
 * function tapi_netns_create_ns_with_ipvlan().
 *
 * @param ta            Test agent name
 * @param ns_name       The network namespace name
 * @param ctl_if        Control interface name on the test agent
 * @param ipvlan_if     IP VLAN interface name
 *
 * @return Status code.
 */
extern te_errno tapi_netns_destroy_ns_with_ipvlan(const char *ta,
                                                  const char *ns_name,
                                                  const char *ctl_if,
                                                  const char *ipvlan_if);

/**@} <!-- END tapi_namespaces --> */

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_NAMESPACES_H__ */
