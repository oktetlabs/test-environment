/** @file
 * @brief API to configure some system options via /proc/sys
 *
 * Implementation of API for access to some system options via /proc/sys.
 *
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
 *
 *
 * @author Andrey Dmitrov <Andrey.Dmitrov@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER     "TAPI Proc"

#include "te_config.h"
#include "conf_api.h"
#include "tapi_proc.h"
#include "tapi_cfg.h"

/* See description in tapi_proc.h */
te_errno
tapi_cfg_if_rp_filter_get(const char *ta, const char *ifname,
                          int *rp_filter)
{
    return tapi_cfg_get_int_fmt(rp_filter,
                                "/agent:%s/interface:%s/rp_filter:",
                                ta, ifname);
}

/* See description in tapi_proc.h */
te_errno
tapi_cfg_if_rp_filter_set(const char *ta, const char *ifname,
                          int rp_filter, int *old_value)
{

    return tapi_cfg_set_int_fmt(rp_filter, old_value,
                                "/agent:%s/interface:%s/rp_filter:",
                                ta, ifname);
}

/* See description in tapi_proc.h */
te_errno
tapi_cfg_if_arp_ignore_get(const char *ta, const char *ifname,
                          int *arp_ignore)
{
    return tapi_cfg_get_int_fmt(arp_ignore,
                                "/agent:%s/interface:%s/arp_ignore:",
                                ta, ifname);
}

/* See description in tapi_proc.h */
te_errno
tapi_cfg_if_arp_ignore_set(const char *ta, const char *ifname,
                          int arp_ignore, int *old_value)
{

    return tapi_cfg_set_int_fmt(arp_ignore, old_value,
                                "/agent:%s/interface:%s/arp_ignore:",
                                ta, ifname);
}

/* See description in tapi_proc.h */
te_errno
tapi_cfg_if_iface_ip4_fw_get(const char *ta, const char *ifname,
                             int *iface_ip4_fw)
{
    return tapi_cfg_get_int_fmt(iface_ip4_fw,
                                "/agent:%s/interface:%s/iface_ip4_fw:",
                                ta, ifname);
}

/* See description in tapi_proc.h */
te_errno
tapi_cfg_if_iface_ip4_fw_set(const char *ta, const char *ifname,
                             int iface_ip4_fw, int *old_value)
{

    return tapi_cfg_set_int_fmt(iface_ip4_fw, old_value,
                                "/agent:%s/interface:%s/iface_ip4_fw:",
                                ta, ifname);
}

/**
 * Macros to define similar functions to get and set system values in /proc
 *
 * @param _name     Field name to be used in functions name.
 * @param _path     Path in configurator tree, it should include '%s'
 *                  modifier for agent name.
 */
#define DEFINE_API_FUNC_TA_ONLY(_name, _path) \
te_errno                                                                \
tapi_cfg_##_name##_set(const char *ta, int value, int *old_value)       \
{                                                                       \
    return tapi_cfg_set_int_fmt(value, old_value, _path, ta);           \
}                                                                       \
                                                                        \
te_errno                                                                \
tapi_cfg_##_name##_get(const char *ta, int *value)                      \
{                                                                       \
    return tapi_cfg_get_int_fmt(value, _path, ta);                      \
}

DEFINE_API_FUNC_TA_ONLY(tcp_syncookies, "/agent:%s/sys:/tcp_syncookies:")
DEFINE_API_FUNC_TA_ONLY(tcp_keepalive_time,
                        "/agent:%s/sys:/tcp_keepalive_time:")
DEFINE_API_FUNC_TA_ONLY(tcp_keepalive_probes,
                        "/agent:%s/sys:/tcp_keepalive_probes:")
DEFINE_API_FUNC_TA_ONLY(tcp_keepalive_intvl,
                        "/agent:%s/sys:/tcp_keepalive_intvl:")
DEFINE_API_FUNC_TA_ONLY(tcp_retries2,
                        "/agent:%s/sys:/tcp_retries2:")
DEFINE_API_FUNC_TA_ONLY(tcp_orphan_retries,
                        "/agent:%s/sys:/tcp_orphan_retries:")
DEFINE_API_FUNC_TA_ONLY(tcp_synack_retries,
                        "/agent:%s/sys:/tcp_synack_retries:")
DEFINE_API_FUNC_TA_ONLY(tcp_syn_retries,
                        "/agent:%s/sys:/tcp_syn_retries:")
DEFINE_API_FUNC_TA_ONLY(tcp_fin_timeout,
                        "/agent:%s/sys:/tcp_fin_timeout:")
DEFINE_API_FUNC_TA_ONLY(core_somaxconn, "/agent:%s/sys:/somaxconn:")
DEFINE_API_FUNC_TA_ONLY(neigh_gc_thresh3,
                        "/agent:%s/sys:/neigh_gc_thresh3:")
DEFINE_API_FUNC_TA_ONLY(igmp_max_memberships,
                        "/agent:%s/sys:/igmp_max_memberships:")
DEFINE_API_FUNC_TA_ONLY(core_optmem_max, "/agent:%s/sys:/optmem_max:")
DEFINE_API_FUNC_TA_ONLY(tcp_max_syn_backlog,
                        "/agent:%s/sys:/tcp_max_syn_backlog:")
DEFINE_API_FUNC_TA_ONLY(tcp_timestamps,
                        "/agent:%s/sys:/tcp_timestamps:")
DEFINE_API_FUNC_TA_ONLY(route_mtu_expires,
                        "/agent:%s/sys:/route_mtu_expires:")
DEFINE_API_FUNC_TA_ONLY(if_all_rp_filter,
                        "/agent:%s/rp_filter_all:")
DEFINE_API_FUNC_TA_ONLY(if_all_arp_ignore,
                        "/agent:%s/arp_ignore_all:")

#undef DEFINE_API_FUNC_TA_ONLY
