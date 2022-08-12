/** @file
 * @brief Traffic flow processing, NDN.
 *
 * Definitions of ASN.1 types for NDN for traffic flow processing.
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 *
 *
 * @author Alexander Kukuta <Alexander.Kukuta@oktetlabs.ru>
 *
 */

#ifndef __TE_NDN_FLOW_H__
#define __TE_NDN_FLOW_H__

#include "te_stdint.h"
#include "asn_usr.h"
#include "ndn.h"


#ifdef __cplusplus
extern "C" {
#endif

extern const asn_type * const ndn_flow_ep;
extern const asn_type * const ndn_flow_pdu;
extern const asn_type * const ndn_flow_traffic;
extern const asn_type * const ndn_flow;

extern asn_type ndn_flow_ep_s;
extern asn_type ndn_flow_pdu_s;
extern asn_type ndn_flow_traffic_s;
extern asn_type ndn_flow_s;

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TE_NDN_ETH_H__ */
