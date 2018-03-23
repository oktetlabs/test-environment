/** @file
 * @brief Test API for TAD PPP CSAP
 *
 * Implementation of Test API
 * 
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
 *
 * @author Alexander Kukuta <Alexander.Kukuta@oktetlabs.ru>
 *
 * $Id: $
 */

#define TE_LGR_USER "TAPI PPP"

#include "te_config.h"

#include "te_errno.h"
#include "logger_api.h"
#include "asn_usr.h"
#include "tapi_ndn.h"
#include "ndn_ipstack.h"
#include "ndn_ppp.h"
#include "tapi_ppp.h"

#include "tapi_test.h"


/* See the description in tapi_ppp.h */
te_errno
tapi_ppp_add_csap_layer(asn_value **csap_spec,
                        uint16_t protocol)
{
    asn_value  *layer = NULL;

    CHECK_RC(tapi_tad_csap_add_layer(csap_spec, ndn_ppp_csap,
                                     "#ppp", &layer));
    if (protocol != 0)
        CHECK_RC(asn_write_value_field(layer, &protocol,
                                       sizeof(protocol),
                                       "protocol.#plain"));

    return 0;
}

/* See the description in tapi_ppp.h */
te_errno
tapi_ppp_add_pdu(asn_value          **tmpl_or_ptrn,
                 asn_value          **pdu,
                 te_bool              is_pattern,
                 uint16_t             protocol)
{
    asn_value  *tmp_pdu;

    CHECK_RC(tapi_tad_tmpl_ptrn_add_layer(tmpl_or_ptrn, is_pattern,
                                          ndn_ppp_message, "#ppp",
                                          &tmp_pdu));

    if (protocol != 0)
        CHECK_RC(asn_write_value_field(tmp_pdu, &protocol,
                                       sizeof(protocol),
                                       "protocol.#plain"));

    if (pdu != NULL)
        *pdu = tmp_pdu;

    return 0;
}

