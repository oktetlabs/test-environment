/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief TAPI TAD ATM
 *
 * @defgroup tapi_tad_atm ATM
 * @ingroup tapi_tad_main
 * @{
 *
 * Declarations of API for TAPI ATM.
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_TAPI_ATM_H__
#define __TE_TAPI_ATM_H__

#include "te_defs.h"
#include "te_stdint.h"
#include "asn_usr.h"
#include "ndn_atm.h"


#ifdef __cplusplus
extern "C" {
#endif


/**
 * Add ATM layer in CSAP specification.
 *
 * @param csap_spec     Location of CSAP specification pointer.
 * @param device        Interface name on TA host or NULL (have to
 *                      be not-NULL, if it is the bottom layer)
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
 *
 * @retval Status code.
 */
extern te_errno tapi_atm_add_csap_layer(asn_value      **csap_spec,
                                        const char      *device,
                                        ndn_atm_type     type,
                                        const uint16_t  *vpi,
                                        const uint16_t  *vci,
                                        te_bool         *congestion,
                                        te_bool         *clp);

/**
 * Add AAL5 layer in CSAP specification.
 *
 * @param csap_spec     Location of CSAP specification pointer.
 * @param cpcs_uu       Pointer to CPCS User-to-User indication or NULL.
 *                      If NULL, default value on transmit is 0 and
 *                      match any on receive.
 * @param cpi           Pointer to Common Part Indicator or NULL.
 *                      If NULL, default value on transmit is 0 and
 *                      match any on receive.
 *
 * @retval Status code.
 */
extern te_errno tapi_atm_aal5_add_csap_layer(asn_value     **csap_spec,
                                             const uint8_t  *cpcs_uu,
                                             const uint8_t  *cpi);


/**
 * Add ATM PDU as the last PDU to the last unit of the traffic
 * template or pattern.
 *
 * @param tmpl_or_ptrn  Location of ASN.1 value with traffic template or
 *                      pattern
 * @param is_pattern    Is the first argument template or pattern
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
 *
 * @return Status code.
 */
extern te_errno tapi_atm_add_pdu(asn_value      **tmpl_or_ptrn,
                                 te_bool          is_pattern,
                                 const uint8_t   *gfc,
                                 const uint16_t  *vpi,
                                 const uint16_t  *vci,
                                 const uint8_t   *payload_type,
                                 te_bool         *clp);

/**
 * Add ATM cell payload to traffic template or pattern unit.
 *
 * @param container     Container to add payload
 * @param pld_len       Length of user payload (should be not greater
 *                      than 48, if less, the rest is padded by zeros)
 * @param pld           Payload
 *
 * @return Status code.
 */
extern te_errno tapi_atm_add_payload(asn_value      *container,
                                     size_t          pld_len,
                                     const uint8_t  *pld);

/**
 * Add AAL5 PDU as the last PDU to the last unit of the traffic
 * template or pattern.
 *
 * @param tmpl_or_ptrn  Location of ASN.1 value with traffic template or
 *                      pattern
 * @param is_pattern    Is the first argument template or pattern
 * @param cpcs_uu       Pointer to CPCS User-to-User indication or NULL.
 *                      If NULL, default value is specified during CSAP
 *                      creation or 0 on transmit/match any on receive.
 * @param cpi           Pointer to Common Part Indicator or NULL.
 *                      If NULL, default value is specified during CSAP
 *                      creation or 0 on transmit/match any on receive.
 *
 * @return Status code.
 */
extern te_errno tapi_atm_aal5_add_pdu(asn_value     **tmpl_or_ptrn,
                                      te_bool         is_pattern,
                                      const uint8_t  *cpcs_uu,
                                      const uint8_t  *cpi);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TE_TAPI_ATM_H__ */

/**@} <!-- END tapi_tad_atm --> */
