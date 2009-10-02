/** @file
 * @brief Test API for TAD PPP CSAP
 *
 * Implementation of Test API
 * 
 * Copyright (C) 2006 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/ or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
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
tapi_ppp_add_csap_layer(asn_value **csap_spec)
{
    asn_value  *layer = NULL;

    return tapi_tad_csap_add_layer(csap_spec, ndn_ppp_csap,
                                   "#ppp", &layer);
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

