/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Tester API for ATM CSAP
 *
 * Implementation of Tester API for ATM CSAP.
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#define TE_LGR_USER     "TAPI ATM"

#include "te_config.h"

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#endif
#if HAVE_STRINGS_H
#include <strings.h>
#endif

#include "te_stdint.h"
#include "te_errno.h"
#include "logger_api.h"
#include "ndn.h"
#include "ndn_atm.h"
#include "tapi_ndn.h"

#include "tapi_atm.h"

#include "tapi_test.h"


/* See the description in tapi_atm.h */
te_errno
tapi_atm_add_csap_layer(asn_value      **csap_spec,
                        const char      *device,
                        ndn_atm_type     type,
                        const uint16_t  *vpi,
                        const uint16_t  *vci,
                        te_bool         *congestion,
                        te_bool         *clp)
{
    asn_value  *layer;

    CHECK_RC(tapi_tad_csap_add_layer(csap_spec, ndn_atm_csap, "#atm",
                                     &layer));

    if (device != NULL)
        CHECK_RC(asn_write_string(layer, device, "device-id.#plain"));

    CHECK_RC(asn_write_int32(layer, type, "type"));

    if (vpi != NULL)
        CHECK_RC(asn_write_int32(layer, *vpi, "vpi.#plain"));
    if (vci != NULL)
        CHECK_RC(asn_write_int32(layer, *vci, "vci.#plain"));
    if (congestion != NULL)
        CHECK_RC(asn_write_int32(layer, *congestion,
                                 "congestion.#plain"));
    if (clp != NULL)
        CHECK_RC(asn_write_int32(layer, *clp, "clp.#plain"));

    return 0;
}

/* See the description in tapi_atm.h */
te_errno
tapi_atm_aal5_add_csap_layer(asn_value     **csap_spec,
                             const uint8_t  *cpcs_uu,
                             const uint8_t  *cpi)
{
    asn_value  *layer;

    CHECK_RC(tapi_tad_csap_add_layer(csap_spec, ndn_aal5_csap, "#aal5",
                                     &layer));

    if (cpcs_uu != NULL)
        CHECK_RC(asn_write_int32(layer, *cpcs_uu, "cpcs-uu.#plain"));
    if (cpi != NULL)
        CHECK_RC(asn_write_int32(layer, *cpi, "cpi.#plain"));

    return 0;
}

/* See the description in tapi_atm.h */
te_errno
tapi_atm_add_pdu(asn_value      **tmpl_or_ptrn,
                 te_bool          is_pattern,
                 const uint8_t   *gfc,
                 const uint16_t  *vpi,
                 const uint16_t  *vci,
                 const uint8_t   *payload_type,
                 te_bool         *clp)
{
    asn_value  *pdu;

    CHECK_RC(tapi_tad_tmpl_ptrn_add_layer(tmpl_or_ptrn, is_pattern,
                                          ndn_atm_header, "#atm",
                                          &pdu));

    if (gfc != NULL)
        CHECK_RC(asn_write_int32(pdu, *gfc, "gfc.#plain"));
    if (vpi != NULL)
        CHECK_RC(asn_write_int32(pdu, *vpi, "vpi.#plain"));
    if (vci != NULL)
        CHECK_RC(asn_write_int32(pdu, *vci, "vci.#plain"));
    if (payload_type != NULL)
        CHECK_RC(asn_write_int32(pdu, *payload_type,
                                 "payload-type.#plain"));
    if (clp != NULL)
        CHECK_RC(asn_write_int32(pdu, *clp, "clp.#plain"));

    return 0;
}

/* See the description in tapi_atm.h */
te_errno
tapi_atm_add_payload(asn_value      *container,
                     size_t          pld_len,
                     const uint8_t  *pld)
{
    uint8_t payload[ATM_PAYLOAD_LEN];

    if (pld_len > ATM_PAYLOAD_LEN)
    {
        ERROR("Too long (%u) ATM cell payload", (unsigned)pld_len);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }
    if (container == NULL)
    {
        ERROR("%s(): Container for payload have to be provided",
              __FUNCTION__);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    memcpy(payload, pld, pld_len);
    memset(payload + pld_len, 0, ATM_PAYLOAD_LEN - pld_len);
    CHECK_RC(asn_write_value_field(container, payload, sizeof(payload),
                                   "payload.#bytes"));

    return 0;
}

/* See the description in tapi_atm.h */
te_errno
tapi_atm_aal5_add_pdu(asn_value     **tmpl_or_ptrn,
                      te_bool         is_pattern,
                      const uint8_t  *cpcs_uu,
                      const uint8_t  *cpi)
{
    asn_value  *pdu;

    CHECK_RC(tapi_tad_tmpl_ptrn_add_layer(tmpl_or_ptrn, is_pattern,
                                          ndn_aal5_cpcs_trailer, "#aal5",
                                          &pdu));

    if (cpcs_uu != NULL)
        CHECK_RC(asn_write_int32(pdu, *cpcs_uu, "cpcs-uu.#plain"));
    if (cpi != NULL)
        CHECK_RC(asn_write_int32(pdu, *cpi, "cpi.#plain"));

    return 0;
}
