/** @file
 * @brief High level test API to configure tested network
 *
 * @defgroup ts_tapi_network High level TAPI to configure network
 * @ingroup te_ts_tapi
 * @{
 *
 * This API can be used to set up network configurations like resources
 * reservation, assigning IP addresses etc with minimum efforts.
 *
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights served.
 *
 * 
 *
 *
 * @author Andrey Dmitrov <Andrey.Dmitrov@oktetlabs.ru>
 */

#ifndef __TE_TAPI_NETWORK_H_
#define __TE_TAPI_NETWORK_H_

#include "te_defs.h"
#include "rcf_rpc.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Reserve resources, set IP addresses and static ARP (if required) in
 * accordance to the current Configurator configuration.
 *
 * @note The function jumps to @b cleanup in case of failure.
 *
 * @param ipv6_supp  @c TRUE if IPv6 addressing is supported.
 */
extern void tapi_network_setup(te_bool ipv6_supp);

/**
 * Flush ARP table for the interface @p ifname.
 *
 * @param rpcs      RPC server handle
 * @param ifname    Interface name
 *
 * @return Status code.
 */
extern te_errno tapi_neight_flush(rcf_rpc_server *rpcs, const char *ifname);

/**
 * Flush ARP table for all interfaces on test agent @p rpcs->ta.
 *
 * @param rpcs      RPC server handle
 *
 * @return Status code.
 */
extern te_errno tapi_neight_flush_ta(rcf_rpc_server *rpcs);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_NETWORK_H_ */

/**@} <!-- END ts_tapi_network --> */
