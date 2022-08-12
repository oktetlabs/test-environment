/** @file
 * @brief TAD Sender/Receiver
 *
 * Traffic Application Domain Command Handler.
 * Declarations of types and functions common for TAD Sender and Receiver.
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 * @author Konstantin Abramenko <konst@oktetlabs.ru>
 *
 */

#ifndef __TE_TAD_SEND_RECV_H__
#define __TE_TAD_SEND_RECV_H__

#include "te_defs.h"
#include "logger_api.h"
#include "asn_usr.h"
#include "tad_csap_inst.h"


#ifdef __cplusplus
extern "C" {
#endif


/**
 * Generate Traffic Pattern NDS by template for send/receive command.
 *
 * @param csap          Structure with CSAP parameters
 * @param template      Traffic template
 * @param pattern       Generated Traffic Pattern
 *
 * @return Status code.
 */
extern te_errno tad_send_recv_generate_pattern(csap_p      csap,
                                               asn_value  *template,
                                               asn_value **pattern);

/**
 * Confirm traffic template or pattern PDUS set with CSAP settings and
 * protocol defaults.
 * This function changes passed ASN value, user have to ensure that changes
 * will be set in traffic template or pattern ASN value which will be used
 * in next operation. This may be done by such ways:
 *
 * Pass pointer got by asn_get_descendent method, or write modified value
 * into original NDS.
 *
 * @param csap    CSAP descriptor.
 * @param recv          Is receive flow or send flow.
 * @param pdus          ASN value with SEQUENCE OF Generic-PDU (IN/OUT).
 * @param layer_opaque  Array for per-layer opaque data pointers
 *
 * @return zero on success, otherwise error code.
 */
extern int tad_confirm_pdus(csap_p csap, te_bool recv,
                            asn_value *pdus, void **layer_opaque);


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /*  __TE_TAD_SEND_RECV_H__ */
