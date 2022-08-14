/** @file
 * @brief API to configure some system options via /proc/sys
 *
 * @defgroup tapi_conf_proc TA system options configuration (OBSOLETE)
 * @ingroup tapi_conf
 * @{
 *
 * ###########################################################################
 * NOTE! This API is obsolete!
 *
 * @ref tapi_cfg_sys must be used in modern tests.
 * ###########################################################################
 *
 * API declaration for access to some system options via /proc/sys
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_TAPI_PROC_H__
#define __TE_TAPI_PROC_H__

#include "te_errno.h"
#include "rcf_rpc.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Flush network routes.
 *
 * @param rpcs      RPC server
 *
 * @return @c 0 on success or @c -1 in case of failure
 */
extern int tapi_cfg_net_route_flush(rcf_rpc_server *rpcs);

/**
 * Set a new TCP syncookies value.
 *
 * @param ta        Test agent name
 * @param value     Value to be set
 * @param old_value Location for previous value or @c NULL
 *
 * @return Status code
 */
extern te_errno tapi_cfg_tcp_syncookies_set(const char *ta, int value,
                                            int *old_value);

/**
 * Get TCP syncookies value.
 *
 * @param ta        Test agent name
 * @param value     Location for the value
 *
 * @return Status code
 */
extern te_errno tapi_cfg_tcp_syncookies_get(const char *ta,
                                            int *value);

/**
 * Set a new TCP timestamps value.
 *
 * @param ta        Test agent name
 * @param value     Value to be set
 * @param old_value Location for previous value or @c NULL
 *
 * @return Status code
 */
extern te_errno tapi_cfg_tcp_timestamps_set(const char *ta, int value,
                                            int *old_value);

/**
 * Get TCP timestamps value.
 *
 * @param ta        Test agent name
 * @param value     Location for the value
 *
 * @return Status code
 */
extern te_errno tapi_cfg_tcp_timestamps_get(const char *ta,
                                            int *value);

/**
 * Get RPF filtering value of TA interface.
 *
 * @param ta        Test agent name
 * @param ifname    Interface name
 * @param rp_filter location for RPF filtering value
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_if_rp_filter_get(const char *ta,
                                          const char *ifname,
                                          int *rp_filter);

/**
 * Set RPF filtering value of TA interface.
 *
 * @param ta        Test agent name
 * @param ifname    Interface name
 * @param rp_filter New RPF filtering value
 * @param old_value Location for previous RPF filtering value or @c NULL
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_if_rp_filter_set(const char *ta,
                                          const char *ifname,
                                          int rp_filter, int *old_value);

/**
 * Get RPF filtering value of TA interface @c "all".
 *
 * @param ta        Test agent name
 * @param rp_filter location for RPF filtering value
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_if_all_rp_filter_get(const char *ta,
                                              int *rp_filter);

/**
 * Set RPF filtering value of TA interface @c "all".
 *
 * @param ta        Test agent name
 * @param rp_filter New RPF filtering value
 * @param old_value Location for previous RPF filtering value or @c NULL
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_if_all_rp_filter_set(
        const char *ta, int rp_filter, int *old_value);


/**
 * Get IP4 forwarding state of TA interface.
 *
 * @param ta           Test agent name
 * @param ifname       Interface name
 * @param iface_ip4_fw Location for forwarding state value
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_if_iface_ip4_fw_get(const char *ta,
                                             const char *ifname,
                                             int *iface_ip4_fw);

/**
 * Change IP4 forwarding state of TA interface.
 *
 * @param ta           Test agent name
 * @param ifname       Interface name
 * @param iface_ip4_fw Location for forwarding state value
 * @param old_value    Location for forwarding state value or @c NULL
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_if_iface_ip4_fw_set(const char *ta,
                                             const char *ifname,
                                             int iface_ip4_fw,
                                             int *old_value);

/**
 * Get arp_ignore value of TA interface.
 *
 * @param ta          Test agent name
 * @param ifname      Interface name
 * @param arp_ignore  Location for arp_ignore value
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_if_arp_ignore_get(const char *ta,
                                           const char *ifname,
                                           int *arp_ignore);

/**
 * Set arp_ignore value of TA interface.
 *
 * @param ta            Test agent name
 * @param ifname        Interface name
 * @param arp_ignore    New arp_ignore value
 * @param old_value     Location for previous arp_ignore value or @c NULL
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_if_arp_ignore_set(const char *ta,
                                           const char *ifname,
                                           int arp_ignore, int *old_value);

/**
 * Get arp_ignore value of TA interface @c "all".
 *
 * @param ta            Test agent name
 * @param arp_ignore    Location for arp_ignore value
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_if_all_arp_ignore_get(const char *ta,
                                               int *arp_ignore);

/**
 * Set arp_ignore value of TA interface @c "all".
 *
 * @param ta            Test agent name
 * @param arp_ignore    New arp_ignore value
 * @param old_value     Location for previous arp_ignore value or @c NULL
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_if_all_arp_ignore_set(
        const char *ta, int arp_ignore, int *old_value);

/**
 * Set a new tcp_keepalive_time value.
 *
 * @param ta        Test agent name
 * @param value     Value to be set
 * @param old_value Location for previous value or @c NULL
 *
 * @return Status code
 */
extern te_errno tapi_cfg_tcp_keepalive_time_set(const char *ta,
                                                int value, int *old_value);

/**
 * Get tcp_keepalive_time value.
 *
 * @param ta        Test agent name
 * @param value     Location for the value
 *
 * @return Status code
 */
extern te_errno tapi_cfg_tcp_keepalive_time_get(const char *ta,
                                                int *value);

/**
 * Set a new tcp_keepalive_probes value.
 *
 * @param ta        Test agent name
 * @param value     Value to be set
 * @param old_value Location for previous value or @c NULL
 *
 * @return Status code
 */
extern te_errno tapi_cfg_tcp_keepalive_probes_set(const char *ta,
                                                  int value,
                                                  int *old_value);

/**
 * Get tcp_keepalive_probes value.
 *
 * @param ta        Test agent name
 * @param value     Location for the value
 *
 * @return Status code
 */
extern te_errno tapi_cfg_tcp_keepalive_probes_get(const char *ta,
                                                  int *value);

/**
 * Set a new tcp_keepalive_intvl value.
 *
 * @param ta        Test agent name
 * @param value     Value to be set
 * @param old_value Location for previous value or @c NULL
 *
 * @return Status code
 */
extern te_errno tapi_cfg_tcp_keepalive_intvl_set(const char *ta,
                                                 int value, int *old_value);

/**
 * Get tcp_keepalive_intvl value.
 *
 * @param ta        Test agent name
 * @param value     Location for the value
 *
 * @return Status code
 */
extern te_errno tapi_cfg_tcp_keepalive_intvl_get(const char *ta,
                                                 int *value);

/**
 * Set a new tcp_retries2 value.
 *
 * @param ta        Test agent name
 * @param value     Value to be set
 * @param old_value Location for previous value or @c NULL
 *
 * @return Status code
 */
extern te_errno tapi_cfg_tcp_retries2_set(const char *ta, int value,
                                          int *old_value);

/**
 * Get tcp_retries2 value.
 *
 * @param ta        Test agent name
 * @param value     Location for the value
 *
 * @return Status code
 */
extern te_errno tapi_cfg_tcp_retries2_get(const char *ta, int *value);

/**
 * Set a new tcp_orphan_retries value.
 *
 * @param ta        Test agent name
 * @param value     Value to be set
 * @param old_value Location for previous value or @c NULL
 *
 * @return Status code
 */
extern te_errno tapi_cfg_tcp_orphan_retries_set(const char *ta,
                                                int value,
                                                int *old_value);

/**
 * Get tcp_orphan_retries value.
 *
 * @param ta        Test agent name
 * @param value     Location for the value
 *
 * @return Status code
 */
extern te_errno tapi_cfg_tcp_orphan_retries_get(const char *ta,
                                                int *value);

/**
 * Set a new tcp_fin_timeout value.
 *
 * @param ta        Test agent name
 * @param value     Value to be set
 * @param old_value Location for previous value or @c NULL
 *
 * @return Status code
 */
extern te_errno tapi_cfg_tcp_fin_timeout_set(const char *ta,
                                             int value,
                                             int *old_value);

/**
 * Get tcp_fin_timeout value.
 *
 * @param ta        Test agent name
 * @param value     Location for the value
 *
 * @return Status code
 */
extern te_errno tapi_cfg_tcp_fin_timeout_get(const char *ta,
                                             int *value);

/**
 * Set a new tcp_synack_retries value.
 *
 * @param ta        Test agent name
 * @param value     Value to be set
 * @param old_value Location for previous value or @c NULL
 *
 * @return Status code
 */
extern te_errno tapi_cfg_tcp_synack_retries_set(const char *ta,
                                                int value, int *old_value);

/**
 * Get tcp_synack_retries value.
 *
 * @param ta        Test agent name
 * @param value     Location for the value
 *
 * @return Status code
 */
extern te_errno tapi_cfg_tcp_synack_retries_get(const char *ta,
                                                int *value);

/**
 * Set a new tcp_syn_retries value.
 *
 * @param ta        Test agent name
 * @param value     Value to be set
 * @param old_value Location for previous value or @c NULL
 *
 * @return Status code
 */
extern te_errno tapi_cfg_tcp_syn_retries_set(const char *ta,
                                             int value, int *old_value);

/**
 * Get tcp_syn_retries value.
 *
 * @param ta        Test agent name
 * @param value     Location for the value
 *
 * @return Status code
 */
extern te_errno tapi_cfg_tcp_syn_retries_get(const char *ta,
                                             int *value);

/**
 * Set a new somaxconn value.
 *
 * @param ta        Test agent name
 * @param value     Value to be set
 * @param old_value Location for previous value or @c NULL
 *
 * @return Status code
 */
extern te_errno tapi_cfg_core_somaxconn_set(const char *ta,
                                       int value, int *old_value);

/**
 * Get somaxconn value.
 *
 * @param ta        Test agent name
 * @param value     Location for the value
 *
 * @return Status code
 */
extern te_errno tapi_cfg_core_somaxconn_get(const char *ta,
                                            int *value);

/**
 * Set a new value to /proc/sys/net/ipv4/neigh/default/gc_thresh3.
 *
 * @param ta        Test agent name
 * @param value     Value to be set
 * @param old_value Location for previous value or @c NULL
 *
 * @return Status code
 */
extern te_errno tapi_cfg_neigh_gc_thresh3_set(const char *ta,
                                              int value, int *old_value);

/**
 * Get value from /proc/sys/net/ipv4/neigh/default/gc_thresh3.
 *
 * @param ta        Test agent name
 * @param value     Location for the value
 *
 * @return Status code
 */
extern te_errno tapi_cfg_neigh_gc_thresh3_get(const char *ta,
                                              int *value);

/**
 * Set a new igmp_max_memberships value.
 *
 * @param ta        Test agent name
 * @param value     Value to be set
 * @param old_value Location for previous value or @c NULL
 *
 * @return Status code
 */
extern te_errno tapi_cfg_igmp_max_memberships_set(const char *ta,
                                                 int value, int *old_value);

/**
 * Get igmp_max_memberships value.
 *
 * @param ta        Test agent name
 * @param value     Location for the value
 *
 * @return Status code
 */
extern te_errno tapi_cfg_igmp_max_memberships_get(const char *ta,
                                                  int *value);

/**
 * Set a new /proc/sys/net/core/optmem_max value.
 *
 * @param ta        Test agent name
 * @param value     Value to be set
 * @param old_value Location for previous value or @c NULL
 *
 * @return Status code
 */
extern te_errno tapi_cfg_core_optmem_max_set(const char *ta,
                                             int value, int *old_value);

/**
 * Get /proc/sys/net/core/optmem_max value.
 *
 * @param ta        Test agent name
 * @param value     Location for the value
 *
 * @return Status code
 */
extern te_errno tapi_cfg_core_optmem_max_get(const char *ta,
                                             int *value);

/**
 * Set a new tcp_max_syn_backlog value.
 *
 * @param ta        Test agent name
 * @param value     Value to be set
 * @param old_value Location for previous value or @c NULL
 *
 * @return Status code
 */
extern te_errno tapi_cfg_tcp_max_syn_backlog_set(const char *ta,
                                                 int value, int *old_value);

/**
 * Get tcp_max_syn_backlog value.
 *
 * @param ta        Test agent name
 * @param value     Location for the value
 *
 * @return Status code
 */
extern te_errno tapi_cfg_tcp_max_syn_backlog_get(const char *ta,
                                                  int *value);

/**
 * Set a new route_mtu_expires value.
 *
 * @param ta          Test agent name
 * @param value       Value to be set
 * @param old_value   Location for the previous value or @c NULL
 *
 * @return Status code
 */
extern te_errno tapi_cfg_route_mtu_expires_set(const char *ta,
                                               int value, int *old_value);

/**
 * Get route_mtu_expires value.
 *
 * @param ta        Test agent name
 * @param value     Location for the value
 *
 * @return Status code
 */
extern te_errno tapi_cfg_route_mtu_expires_get(const char *ta,
                                               int *value);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_PROC_H__ */

/**@} <!-- END tapi_conf_proc --> */
