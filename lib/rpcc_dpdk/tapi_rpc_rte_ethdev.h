/** @file
 * @brief RPC client API for DPDK Ethernet Device API
 *
 * RPC client API for DPDK Ethernet Device API functions.
 *
 *
 * Copyright (C) 2016 Test Environment authors (see file AUTHORS in
 * the root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 */

#ifndef __TE_TAPI_RPC_RTE_ETHDEV_H__
#define __TE_TAPI_RPC_RTE_ETHDEV_H__

#include "rcf_rpc.h"

#include "tapi_rpc_rte.h"
#include "te_kvpair.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup te_lib_rpc_rte_ethdev TAPI for RTE Ethernet Device API remote calls
 * @ingroup te_lib_rpc_tapi
 * @{
 */

#define RPC_RTE_ETH_NAME_MAX_LEN 32

#define RPC_RTE_RETA_GROUP_SIZE 64

#define RPC_RSS_HASH_KEY_LEN_DEF 40

/**
 * rte_eth_dev_info() RPC.
 *
 * Caller must free memory allocated for driver_name using free().
 */
extern void rpc_rte_eth_dev_info_get(rcf_rpc_server *rpcs,
                                     uint8_t port_id,
                                     struct tarpc_rte_eth_dev_info *dev_info);

/**
 * rte_eth_dev_configure() RPC.
 *
 * If failure is not expected, the function jumps out in the case of
 * negative return value.
 */
extern int rpc_rte_eth_dev_configure(rcf_rpc_server *rpcs,
                                     uint8_t port_id,
                                     uint16_t nb_rx_queue,
                                     uint16_t nb_tx_queue,
                                     const struct tarpc_rte_eth_conf *eth_conf);

/**
 * Fill in eth_conf with default settings.
 *
 * The function may call rte_eth_dev_info_get() to obtain device
 * facilities.
 */
extern struct tarpc_rte_eth_conf *
    tapi_rpc_rte_eth_make_eth_conf(rcf_rpc_server *rpcs, uint8_t port_id,
                                   struct tarpc_rte_eth_conf *eth_conf);

/**
 * Do rte_eth_dev_configure() RPC with default eth_conf.
 */
extern int tapi_rpc_rte_eth_dev_configure_def(rcf_rpc_server *rpcs,
                                              uint8_t port_id,
                                              uint16_t nb_rx_queue,
                                              uint16_t nb_tx_queue);

/**
 * @b rte_eth_dev_close() RPC.
 */
extern void rpc_rte_eth_dev_close(rcf_rpc_server *rpcs, uint8_t port_id);

/**
 * @b rte_eth_dev_start() RPC.
 *
 * If error is not expected, the function jumps out in the case
 * of start failure.
 */
extern int rpc_rte_eth_dev_start(rcf_rpc_server *rpcs, uint8_t port_id);

/**
 * @b rte_eth_dev_stop() RPC.
 */
extern void rpc_rte_eth_dev_stop(rcf_rpc_server *rpcs, uint8_t port_id);

/**
 * @b rte_eth_tx_queue_setup() RPC.
 *
 * If failure is not expected, the function jumps out in the case of
 * non-zero return value.
 */
extern int rpc_rte_eth_tx_queue_setup(rcf_rpc_server *rpcs,
                                      uint8_t port_id,
                                      uint16_t tx_queue_id,
                                      uint16_t nb_tx_desc,
                                      unsigned int socket_id,
                                      struct tarpc_rte_eth_txconf *tx_conf);

/**
 * @b rte_eth_rx_queue_setup() RPC.
 *
 * If failure is not expected, the function jumps out in the case of
 * non-zero return value.
 */
extern int rpc_rte_eth_rx_queue_setup(rcf_rpc_server *rpcs,
                                      uint8_t port_id,
                                      uint16_t rx_queue_id,
                                      uint16_t nb_rx_desc,
                                      unsigned int socket_id,
                                      struct tarpc_rte_eth_rxconf *rx_conf,
                                      rpc_rte_mempool_p mp);

/**
 * @b rte_eth_tx_burst() RPC.
 *
 * The function jumps out in the case of actual number of transmitted packets
 * more than @p nb_pkts
 */
extern uint16_t rpc_rte_eth_tx_burst(rcf_rpc_server *rpcs, uint8_t  port_id,
                                     uint16_t queue_id,
                                     rpc_rte_mbuf_p *tx_pkts,
                                     uint16_t nb_pkts);

/**
 * @b rte_eth_rx_burst() RPC.
 *
 * The function jumps out in the case of the actual number of received packets
 * more than @p nb_pkts.
 */
extern uint16_t rpc_rte_eth_rx_burst(rcf_rpc_server *rpcs, uint8_t  port_id,
                                     uint16_t queue_id,
                                     rpc_rte_mbuf_p *rx_pkts,
                                     uint16_t nb_pkts);

/**
 * @b rte_eth_dev_set_link_up() RPC.
 *
 * If failure is not expected, the function jumps out in the case of
 * non-zero return value.
 */
extern int rpc_rte_eth_dev_set_link_up(rcf_rpc_server *rpcs, uint8_t port_id);

/**
 * @b rte_eth_dev_set_link_down() RPC.
 *
 * If failure is not expected, the function jumps out in the case of
 * non-zero return value.
 */
extern int rpc_rte_eth_dev_set_link_down(rcf_rpc_server *rpcs,
                                         uint8_t port_id);

/**
 * @b rte_eth_promiscuous_enable() RPC.
 */
extern void rpc_rte_eth_promiscuous_enable(rcf_rpc_server *rpcs,
                                           uint8_t port_id);

/**
 * @b rte_eth_promiscuous_disable() RPC.
 */
extern void rpc_rte_eth_promiscuous_disable(rcf_rpc_server *rpcs,
                                            uint8_t port_id);

/**
 * @b rte_eth_promiscuous_get() RPC
 *
 * If failure is not expected, the function jumps out in the case of
 * negative return value.
 */
extern int rpc_rte_eth_promiscuous_get(rcf_rpc_server *rpcs, uint8_t port_id);

/**
 * @b rte_eth_allmulticast_enable() RPC
 */
extern void rpc_rte_eth_allmulticast_enable(rcf_rpc_server *rpcs,
                                            uint8_t port_id);

/**
 * @b rte_eth_allmulticast_disable() RPC
 */
extern void rpc_rte_eth_allmulticast_disable(rcf_rpc_server *rpcs,
                                             uint8_t port_id);

/**
 * rte_eth_allmulticast_get() RPC
 *
 * If failure is not expected, the function jumps out in the case of
 * negative return value.
 */
extern int rpc_rte_eth_allmulticast_get(rcf_rpc_server *rpcs,
                                        uint8_t port_id);

/**
 * rte_eth_dev_get_mtu() RPC
 *
 * If failure is not expected, the function jumps out in the case of
 * negative return value.
 */
extern int rpc_rte_eth_dev_get_mtu(rcf_rpc_server *rpcs,
                                   uint8_t port_id,
                                   uint16_t *mtu);

/**
 * @b rte_eth_dev_set_mtu() RPC.
 *
 * If failure is not expected, the function jumps out in the case of
 * non-zero return value.
 */
extern int rpc_rte_eth_dev_set_mtu(rcf_rpc_server *rpcs, uint8_t port_id,
                                   uint16_t mtu);

/**
 * @b rte_eth_dev_vlan_filter() RPC.
 *
 * If failure is not expected, the function jumps out in the case of
 * non-zero return value.
 */
extern int rpc_rte_eth_dev_vlan_filter(rcf_rpc_server *rpcs, uint8_t port_id,
                                       uint16_t vlan_id, int on);

/**
 * @b rte_eth_dev_set_vlan_strip_on_queue() RPC.
 *
 * If failure is not expected, the function jumps out in the case of
 * non-zero return value.
 */
extern int rpc_rte_eth_dev_set_vlan_strip_on_queue(rcf_rpc_server *rpcs,
                                                   uint8_t port_id,
                                                   uint16_t rx_queue_id,
                                                   int on);

/**
 * @b rte_eth_dev_set_vlan_ether_type() RPC.
 *
 * If failure is not expected, the function jumps out in the case of
 * non-zero return value or vlan type is unknown.
 */
extern int rpc_rte_eth_dev_set_vlan_ether_type(rcf_rpc_server *rpcs,
                                               uint8_t port_id,
                                               enum tarpc_rte_vlan_type vlan_type,
                                               uint16_t tag_type);

/**
 * @b rte_eth_dev_set_vlan_offload() RPC.
 *
 * If failure is not expected, the function jumps out in the case of
 * non-zero return value.
 */
extern int rpc_rte_eth_dev_set_vlan_offload(rcf_rpc_server *rpcs, uint8_t port_id,
                                            tarpc_int offload_mask);

/**
 * @b rte_eth_dev_get_vlan_offload() RPC.
 *
 * If failure is not expected, the function jumps out in the case of
 * negative return value or invalid mask.
 */
extern int rpc_rte_eth_dev_get_vlan_offload(rcf_rpc_server *rpcs, uint8_t port_id);

/**
 * @b rte_eth_dev_set_vlan_pvid() RPC.
 *
 * If failure is not expected, the function jumps out in the case of
 * non-zero return value.
 */
extern int rpc_rte_eth_dev_set_vlan_pvid(rcf_rpc_server *rpcs, uint8_t port_id,
                                         uint16_t pvid, int on);

/**
 * @b rte_eth_rx_descriptor_done() RPC
 *
 * If failure is not expected, the function jumps out in the case of
 * return value is not equal to 0 or 1.
 */
extern int rpc_rte_eth_rx_descriptor_done(rcf_rpc_server *rpcs, uint8_t port_id,
                                          uint16_t queue_id, uint16_t offset);

/**
 * @b rte_eth_rx_queue_count() RPC
 *
 * If failure is not expected, the function jumps out in the case of
 * negative return value.
 */
extern int rpc_rte_eth_rx_queue_count(rcf_rpc_server *rpcs, uint8_t port_id,
                                      uint16_t queue_id);

/**
 * @b rte_eth_dev_socket_id() RPC
 *
 * If failure is not expected, the function jumps out in the case of
 * negative return value.
 */
extern int rpc_rte_eth_dev_socket_id(rcf_rpc_server *rpcs, uint8_t port_id);

/** @b rte_eth_dev_is_valid_port() RPC */
extern int rpc_rte_eth_dev_is_valid_port(rcf_rpc_server *rpcs, uint8_t port_id);

/**
 * @b rte_eth_dev_rx_queue_start() RPC
 *
 * If failure is not expected, the function jumps out in the case of
 * non-zero return value.
 */
extern int rpc_rte_eth_dev_rx_queue_start(rcf_rpc_server *rpcs,
                                          uint8_t port_id,
                                          uint16_t queue_id);

/**
 * @b rte_eth_dev_rx_queue_stop() RPC
 *
 * If failure is not expected, the function jumps out in the case of
 * non-zero return value.
 */
extern int rpc_rte_eth_dev_rx_queue_stop(rcf_rpc_server *rpcs,
                                         uint8_t port_id,
                                         uint16_t queue_id);

/**
 * @b rte_eth_dev_rx_queue_start() RPC
 *
 * If failure is not expected, the function jumps out in the case of
 * non-zero return value.
 */
extern int rpc_rte_eth_dev_tx_queue_start(rcf_rpc_server *rpcs,
                                          uint8_t port_id,
                                          uint16_t queue_id);

/**
 * @b rte_eth_dev_tx_queue_stop() RPC
 *
 * If failure is not expected, the function jumps out in the case of
 * non-zero return value.
 */
extern int rpc_rte_eth_dev_tx_queue_stop(rcf_rpc_server *rpcs,
                                         uint8_t port_id,
                                         uint16_t queue_id);

/**
 * @b rte_eth_macaddr_get() RPC
 */
extern void rpc_rte_eth_macaddr_get(rcf_rpc_server *rpcs, uint8_t port_id,
                                    struct tarpc_ether_addr *mac_addr);

/**
 * @b rte_eth_dev_default_mac_addr_set() RPC
 *
 * If failure is not expected, the function jumps out in the case of
 * non-zero return value.
 */
extern int rpc_rte_eth_dev_default_mac_addr_set(rcf_rpc_server *rpcs,
                                                uint8_t port_id,
                                                struct tarpc_ether_addr *mac_addr);

/**
 * @b rte_eth_rx_queue_info_get() RPC
 *
 * If failure is not expected, the function jumps out in the case of
 * non-zero return value.
 */
extern int rpc_rte_eth_rx_queue_info_get(rcf_rpc_server *rpcs, uint8_t port_id,
                                         uint16_t queue_id,
                                         struct tarpc_rte_eth_rxq_info *qinfo);

/**
 * @b rte_eth_tx_queue_info_get() RPC
 *
 * If failure is not expected, the function jumps out in the case of
 * non-zero return value.
 */
extern int rpc_rte_eth_tx_queue_info_get(rcf_rpc_server *rpcs, uint8_t port_id,
                                         uint16_t queue_id,
                                         struct tarpc_rte_eth_txq_info *qinfo);

/**
 * @b rte_eth_dev_count() RPC.
 */
extern uint8_t rpc_rte_eth_dev_count(rcf_rpc_server *rpcs);

/**
 * @b rte_eth_dev_attach() RPC
 *
 * @param[in]  A string describing the new device to be attached
 * @param[out] Location for the device port ID actually attached
 *
 * @return Status code (by default jumps out on a non-zero value)
 */
extern int rpc_rte_eth_dev_attach(rcf_rpc_server *rpcs,
                                  const char     *devargs,
                                  uint8_t        *port_id);

/**
 * @b rte_eth_dev_detach() RPC.
 *
 * @param[in]  rpcs        RPC server handle
 * @param[in]  port_id     Port number
 * @param[out] devname     A pointer to a device name actually detached.
 *                         The memory must be allocated by the caller.
 *
 * If failure is not expected, the function jumps out in the case of
 * non-zero or negative return value.
 */
extern int rpc_rte_eth_dev_detach(rcf_rpc_server *rpcs, uint8_t port_id,
                                  char *devname);

/**
 * @b rte_eth_dev_rss_reta_query() RPC.
 *
 * @param[in]  rpcs        RPC server handle
 * @param[in]  port_id     Port number
 * @param[in]  reta_size   Redirection table size.
 * @param[out] reta_conf   A pointer to the array of rte_eth_rss_reta_entry64
 *                         structures. The memory must be allocated by the
 *                         caller.
 *
 * If failure is not expected, the function jumps out in the case of non-zero
 * or negative return value.
 */
extern int rpc_rte_eth_dev_rss_reta_query(rcf_rpc_server *rpcs,
                                          uint8_t port_id,
                                          struct tarpc_rte_eth_rss_reta_entry64 *reta_conf,
                                          uint16_t reta_size);

/**
 * @b rte_eth_dev_rss_reta_update() RPC.
 *
 * If failure is not expected, the function jumps out in the case of non-zero
 * or negative return value.
 */
extern int rpc_rte_eth_dev_rss_reta_update(rcf_rpc_server *rpcs,
                                           uint8_t port_id,
                                           struct tarpc_rte_eth_rss_reta_entry64 *reta_conf,
                                           uint16_t reta_size);

/**
 * @b rte_eth_dev_rss_hash_conf_get() RPC.
 *
 * @param[in]  rpcs        RPC server handle
 * @param[in]  port_id     Port number
 * @param[out] rss_conf    A pointer to store the current RSS hash configuration.
 *                         The memory must be allocated by the caller.
 *
 * If failure is not expected, the function jumps out in the case of non-zero
 * or negative return value.
 */
extern int rpc_rte_eth_dev_rss_hash_conf_get(rcf_rpc_server *rpcs,
                                             uint8_t port_id,
                                             struct tarpc_rte_eth_rss_conf *rss_conf);

/**
 * @b rte_eth_dev_flow_ctrl_get() RPC.
 *
 * @param[in]  rpcs        RPC server handle
 * @param[in]  port_id     Port number
 * @param[out] fc_conf     The pointer to the structure of the flow control parameters.
 *                         The memory must be allocated by the caller.
 *
 * If failure is not expected, the function jumps out in the case of non-zero
 * or negative return value.
 */
extern int rpc_rte_eth_dev_flow_ctrl_get(rcf_rpc_server *rpcs, uint8_t port_id,
                                         struct tarpc_rte_eth_fc_conf *fc_conf);

/**
 * @b rte_eth_dev_flow_ctrl_set() RPC
 */
extern int rpc_rte_eth_dev_flow_ctrl_set(rcf_rpc_server *rpcs, uint8_t port_id,
                                         struct tarpc_rte_eth_fc_conf *fc_conf);

/**
 * Convert filter type to string.
 */
extern const char * tapi_rpc_rte_filter_type2str(
                        enum tarpc_rte_filter_type filter_type);

/**
 * @b rte_eth_dev_filter_supported() RPC.
 *
 * If failure is not expected, the function jumps out in the case of non-zero
 * or negative return value.
 */
extern int rpc_rte_eth_dev_filter_supported(rcf_rpc_server *rpcs, uint8_t port_id,
                                            enum tarpc_rte_filter_type filter_type);

/**
 * Add ethdev MAC address to test parameters kvpairs
 *
 * @param rpcs        RPC server handle.
 * @param port_id     Port identifier (port number)
 * @param head        Head of the list
 * @param name        Name that will be used as the key
 *                    for MAC address
 *
 * @retrun Status code
 */
extern te_errno tapi_rpc_add_mac_as_octstring2kvpair(rcf_rpc_server *rpcs,
                                                     uint8_t port_id,
                                                     te_kvpair_h *head,
                                                     const char *name);

/**
 * @b rte_eth_dev_filter_ctrl() RPC.
 *
 * If failure is not expected, the function jumps out in the case of non-zero
 * or negative return value.
 */
extern int rpc_rte_eth_dev_filter_ctrl(rcf_rpc_server *rpcs, uint8_t port_id,
                                      enum tarpc_rte_filter_type filter_type,
                                      enum tarpc_rte_filter_op filter_op,
                                      void *arg);

/**
 * @b rte_eth_dev_rss_hash_update() RPC.
 *
 * If failure is not expected, the function jumps out in the case of non-zero
 * value.
 */
extern int rpc_rte_eth_dev_rss_hash_update(
    rcf_rpc_server *rpcs, uint8_t port_id,
    struct tarpc_rte_eth_rss_conf *rss_conf);

/**
 * @b rte_eth_link_get_nowait() RPC.
 */
extern void rpc_rte_eth_link_get_nowait(rcf_rpc_server *rpcs, uint8_t port_id,
                                        struct tarpc_rte_eth_link *eth_link);

/**
 * @b rte_eth_link_get() RPC.
 */
extern void rpc_rte_eth_link_get(rcf_rpc_server *rpcs, uint8_t port_id,
                                 struct tarpc_rte_eth_link *eth_link);

/**
 * @b rte_eth_stats_get() RPC
 *
 * @param port_id         The port identifier of the Ethernet device
 * @param stats           Location for the responce containing stats
 *
 * @return @c 0 on success; jumps out on error (negative return code)
 */
extern int rpc_rte_eth_stats_get(rcf_rpc_server             *rpcs,
                                 uint8_t                     port_id,
                                 struct tarpc_rte_eth_stats *stats);

/**
 * @b rte_eth_xstats_get_names() RPC.
 *
 * If failure is not expected, the function jumps out in the case of
 * negative return value.
 */
extern int rpc_rte_eth_xstats_get_names(rcf_rpc_server *rpcs, uint8_t port_id,
                                        struct tarpc_rte_eth_xstat_name *xstats_names,
                                        unsigned int size);

/**
 * @b rpc_rte_eth_xstats_get() RPC.
 *
 * If failure is not expected, the function jumps out in the case of
 * negative return value.
 */
extern int rpc_rte_eth_xstats_get(rcf_rpc_server *rpcs, uint8_t port_id,
                                  struct tarpc_rte_eth_xstat *xstats,
                                  unsigned int n);

/**
 * @b rte_eth_xstats_reset() RPC
 */
extern void rpc_rte_eth_xstats_reset(rcf_rpc_server *rpcs, uint8_t port_id);

/**
 * @b rte_eth_dev_get_supported_ptypes() RPC.
 *
 * If failure is not expected, the function jumps out in the case of
 * negative return value.
 */
extern int rpc_rte_eth_dev_get_supported_ptypes(rcf_rpc_server *rpcs, uint8_t port_id,
                                                uint32_t ptype_mask, uint32_t *ptypes,
                                                int num);

/**
 * @b rte_eth_dev_set_mc_addr_list() RPC
 */
extern int rpc_rte_eth_dev_set_mc_addr_list(rcf_rpc_server *rpcs,
                                            uint8_t port_id,
                                            struct tarpc_ether_addr *mc_addr_set,
                                            uint32_t nb_mc_addr);

/**
 * @b rte_eth_dev_fw_version_get() RPC
 *
 * @param[in]  port_id     The port identifier of the device
 * @param[out] fw_version  A buffer to store FW version string
 *                         (allocated and freed by the caller)
 * @param[in]  fw_size     Buffer length
 *
 * @return @c 0 on success; positive length of unmodified
 *         string, if it was truncated to fit the buffer;
 *         jumps out on error (in case of negative value)
 */
extern int rpc_rte_eth_dev_fw_version_get(rcf_rpc_server *rpcs,
                                          uint8_t         port_id,
                                          char           *fw_version,
                                          size_t          fw_size);

/**@} <!-- END te_lib_rpc_rte_ethdev --> */

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_RPC_RTE_ETHDEV_H__ */
