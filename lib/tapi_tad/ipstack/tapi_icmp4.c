/** @file
 * @brief Test API for TAD. IP stack CSAP
 *
 * Implementation of Test API
 * 
 * Copyright (C) 2006 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
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
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER "TAPI ICMPv4"

#include "te_config.h"

#include "te_errno.h"
#include "logger_api.h"
#include "asn_usr.h"
#include "tapi_ndn.h"
#include "ndn_ipstack.h"
#include "tapi_icmp4.h"

#include "tapi_test.h"


/* See the description in tapi_icmp4.h */
te_errno
tapi_icmp4_add_csap_layer(asn_value **csap_spec)
{
    return tapi_tad_csap_add_layer(csap_spec, ndn_ip4_csap, "#icmp4",
                                   NULL);
}


/* See the description in tapi_icmp4.h */
te_errno
tapi_icmp4_add_pdu(asn_value **tmpl_or_ptrn, asn_value **pdu,
                   te_bool is_pattern,
                   int type, int code)
{
    asn_value  *tmp_pdu;

    if (type > 0xff || code > 0xff)
        return TE_RC(TE_TAPI, TE_EINVAL);

    CHECK_RC(tapi_tad_tmpl_ptrn_add_layer(tmpl_or_ptrn, is_pattern,
                                          ndn_icmp4_message, "#icmp4",
                                          &tmp_pdu));

    if (type >= 0)
        CHECK_RC(asn_write_int32(tmp_pdu, type, "type.#plain"));
    if (code >= 0)
        CHECK_RC(asn_write_int32(tmp_pdu, code, "code.#plain"));

    if (pdu != NULL)
        *pdu = tmp_pdu;

    return 0;
}
