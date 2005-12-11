/** @file
 * @brief ARP TAD
 *
 * Traffic Application Domain Command Handler
 * ARP CSAP stack-related callbacks.
 *
 * Copyright (C) 2005 Test Environment authors (see file AUTHORS in
 * the root directory of the distribution).
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER     "TAD ARP stack"

#include "te_config.h"

#include "te_errno.h"
#include "logger_api.h"
#include "logger_ta_fast.h"
#include "asn_usr.h"
#include "tad_csap_support.h"
#include "tad_csap_inst.h"
#include "eth/tad_eth_impl.h"
#include "tad_arp_impl.h"


/* See description in tad_arp_impl.h */
te_errno
tad_arp_eth_init_cb(csap_p csap_descr, unsigned int layer,
                    const asn_value *csap_nds)
{
    arp_csap_specific_data_t *arp_spec_data; 
    eth_csap_specific_data_t *eth_spec_data; 

#if 0
    te_errno    rc;
    int32_t     tmp;
#endif


    UNUSED(csap_nds); /* All data are extractred from its layer */

    F_ENTRY("(%d:%u) nds=%p", csap_descr->id, layer, (void *)csap_nds);

    if (layer + 1 >= csap_descr->depth)
    {
        ERROR("%s(): CSAP %u too large layer %u, depth is %u", 
              __FUNCTION__, csap_descr->id, layer, csap_descr->depth);
        return TE_RC(TE_TAD_CSAP, TE_EINVAL);
    }
    eth_spec_data = (eth_csap_specific_data_t *)
        csap_descr->layers[layer + 1].specific_data;
    assert(eth_spec_data != NULL);
    if (eth_spec_data->eth_type == 0)
        eth_spec_data->eth_type = ETHERTYPE_ARP;

    arp_spec_data = calloc(1, sizeof(*arp_spec_data));
    if (arp_spec_data == NULL)
        return TE_RC(TE_TAD_CSAP, TE_ENOMEM);

    csap_descr->layers[layer].specific_data = arp_spec_data;
    csap_descr->layers[layer].get_param_cb = tad_arp_get_param_cb;

#define TAD_ARP_INIT_GET_INT(_str, _width, _field) \
    do {                                                                \
        rc = asn_read_int32(csap_descr->layers[layer].csap_layer_pdu,   \
                            &tmp, _str ".#plain");                      \
        if (rc == TE_EASNWRONGLABEL)                                    \
        {                                                               \
            INFO("%s(): " _str " is not specified", __FUNCTION__);      \
        }                                                               \
        else if (rc != 0)                                               \
        {                                                               \
            ERROR("%s(): asn_read_int32(" _str ".#plain) failed: %r",   \
                  __FUNCTION__, rc);                                    \
            return TE_RC(TE_TAD_CSAP, rc);                              \
        }                                                               \
        else if ((tmp >> (_width)) != 0)                                \
        {                                                               \
            ERROR("%s() " _str ".#plain value %u does not fit in "      \
                  "%u-bit", __FUNCTION__, (unsigned)tmp, (_width));     \
            return TE_RC(TE_TAD_CSAP, TE_EINVAL);                       \
        }                                                               \
        arp_spec_data->_field = tmp;                                    \
    } while (0)

#if 0
    TAD_ARP_INIT_GET_INT("hw-type",    16, hw_type);
    TAD_ARP_INIT_GET_INT("proto-type", 16, proto_type);
    TAD_ARP_INIT_GET_INT("hw-size",    8,  hw_size);
    TAD_ARP_INIT_GET_INT("proto-size", 8,  proto_size);
#endif

#undef TAD_ARP_INIT_GET_INT

    return 0;
}

/* See description in tad_arp_impl.h */
te_errno
tad_arp_eth_destroy_cb(csap_p csap_descr, unsigned int layer)
{
    F_ENTRY("(%d:%u)", csap_descr->id, layer);

    return 0;
}
