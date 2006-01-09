/** @file
 * @brief TAPI ATM
 *
 * Declarations of API for TAPI ATM.
 *
 * Copyright (C) 2006 Test Environment authors (see file AUTHORS 
 * in the root directory of the distribution).
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
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id: tapi_arp.h 20428 2005-11-01 18:54:10Z arybchik $
 */ 

#ifndef __TE_TAPI_ATM_H__
#define __TE_TAPI_ATM_H__

#include "te_defs.h"
#include "te_stdint.h"
#include "tad_common.h"
#include "asn_usr.h"
#include "ndn_atm.h"


#ifdef __cplusplus
extern "C" {
#endif


/**
 * Create ASN.1 value for CSAP ATM layer.
 *
 * @param type          ATM cell header format.
 * @param vpi           Pointer to Virtual Path Identifier or NULL.
 *                      If NULL, it have to be specified in traffic
 *                      templates and match any, if it is not specified
 *                      in traffic pattern.
 * @param vci           Pointer to Virtual Channel Identifier or NULL.
 *                      If NULL, it have to be specified in traffic
 *                      templates and match any, if it is not specified
 *                      in traffic pattern.
 * @param congestion    Pointer to congestion bit value or NULL.
 *                      If NULL, default value on transmit is 0 and
 *                      match any on receive.
 * @param clp           Pointer to Cell Loss Priority bit value or NULL.
 *                      If NULL, default value on transmit is 0 and
 *                      match any on receive.
 * @param atm_layer     Location for prepared CSAP ATM layer
 *                      specification pointer.
 *
 * @retval Status code.
 */
extern te_errno tapi_atm_csap_layer(ndn_atm_type     type,
                                    const uint16_t  *vpi,
                                    const uint16_t  *vci,
                                    te_bool         *congestion,
                                    te_bool         *clp,
                                    asn_value      **atm_layer);

/**
 * Create ASN.1 value for CSAP AAL5 layer.
 *
 * @param cpcs_uu       Pointer to CPCS User-to-User indication or NULL.
 *                      If NULL, default value on transmit is 0 and
 *                      match any on receive.
 * @param cpi           Pointer to Common Part Indicator or NULL.
 *                      If NULL, default value on transmit is 0 and
 *                      match any on receive.
 * @param aal5_layer    Location for prepared CSAP AAL5 layer
 *                      specification pointer.
 *
 * @retval Status code.
 */
extern te_errno tapi_aal5_csap_layer(const uint8_t  *cpcs_uu,
                                     const uint8_t  *cpi,
                                     asn_value     **aal5_layer);


/**
 * Create ATM CSAP that runs over a file descriptor.
 *
 * @param ta_name       Test Agent name
 * @param sid           RCF session
 * @param fd            File descriptor to read/write data
 * @param csap          Identifier of created CSAP
 *
 * @sa tapi_atm_csap_layer() for description of the rest of parameters.
 *
 * @retval Status code.
 */
extern te_errno tapi_atm_csap_create(const char     *ta_name,
                                     int             sid,
                                     int             fd,
                                     ndn_atm_type    type,
                                     const uint16_t *vpi,
                                     const uint16_t *vci,
                                     te_bool        *congestion,
                                     te_bool        *clp,
                                     csap_handle_t  *csap);

/**
 * Create simple ATM cell traffic template with plain values.
 *
 * @param gfc           Pointer to GFC field value or NULL, if
 *                      unspecified (default value is 0)
 * @param vpi           Pointer to VPI or NULL (default value have to
 *                      be specified during CSAP creation)
 * @param vci           Pointer to VCI or NULL (default value have to
 *                      be specified during CSAP creation)
 * @param payload_type  Pointer to payload type or NULL (default
 *                      value is zero or'ed with congestion bit
 *                      specified during CSAP creation)
 * @parma clp           Pointer to CLP or NULL (default value is
 *                      specified during CSAP creation or 0)
 * @param pld_len       Length of user payload (should be not greater
 *                      than 48, if less, the rest is padded by zeros)
 * @param pld           Payload
 * @param tmpl          Location for pointer to created traffic template
 *
 * @return Status code.
 */
extern te_errno tapi_atm_simple_template(const uint8_t   *gfc,
                                         const uint16_t  *vpi,
                                         const uint16_t  *vci,
                                         const uint8_t   *payload_type,
                                         te_bool         *clp,
                                         size_t           pld_len,
                                         const uint8_t   *pld,
                                         asn_value      **tmpl);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TE_TAPI_ATM_H__ */
