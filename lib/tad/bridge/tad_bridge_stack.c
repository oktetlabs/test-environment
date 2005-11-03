/** @file
 * @brief Bridge/STP TAD
 *
 * Traffic Application Domain Command Handler
 * Ethernet CSAP, stack-related callbacks.
 *
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
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
 * @author Konstantin Abramenko <Konstantin.Abramenko@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER     "TAD Bridge Stack"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif /* HAVE_STDLIB_H*/

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H*/


#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#ifdef HAVE_SYS_IF_ETHER_H
#include <sys/sys_ether.h>
#endif


#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif


#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif /* HAVE_FCNTL_H*/

#include <string.h>
#include "tad_bridge_impl.h"
#include "ndn_bridge.h"
#include "ndn_eth.h"
#include "logger_ta_fast.h"
 

/**
 * Free all memory allocated by eth csap specific data
 *
 * @param eth_csap_specific_data_p poiner to structure
 * @param is_complete if ETH_COMPLETE_FREE the free() will be called on passed pointer
 *
 */ 
void 
free_bridge_csap_data(bridge_csap_specific_data_p spec_data, char is_complete)
{ 
    if (is_complete)
        free(spec_data);   
}



/* See description tad_bridge_impl.h */
te_errno
tad_bridge_eth_check_pdus(csap_p csap_descr, asn_value *traffic_nds)
{ 
    char     choice_label[20];
    te_errno rc;

    rc = asn_get_choice(traffic_nds, "pdus.0", choice_label, 
                        sizeof(choice_label));

    if (rc && TE_RC_GET_ERROR(rc) != TE_EASNINCOMPLVAL)
        return TE_RC(TE_TAD_CSAP, rc);

    if ((TE_RC_GET_ERROR(rc) == TE_EASNINCOMPLVAL) || 
        (strcmp(choice_label, "bridge") != 0))
    {
        asn_value *bridge_pdu = asn_init_value(ndn_bridge_pdu); 
        asn_value *asn_pdu    = asn_init_value(ndn_generic_pdu); 
        
        asn_write_component_value(asn_pdu, bridge_pdu, "#bridge");
        asn_insert_indexed(traffic_nds, asn_pdu, 0, "pdus"); 

        asn_free_value(asn_pdu);
        asn_free_value(bridge_pdu);
    }

    rc = asn_get_choice(traffic_nds, "pdus.1", choice_label, 
                        sizeof(choice_label));

    if (rc && TE_RC_GET_ERROR(rc) != TE_EASNINCOMPLVAL)
        return TE_RC(TE_TAD_CSAP, rc);

    if ((TE_RC_GET_ERROR(rc) == TE_EASNINCOMPLVAL) || 
        (strcmp(choice_label, "eth") != 0))

    {
        asn_value *eth_pdu = asn_init_value(ndn_eth_header); 
        asn_value *asn_pdu    = asn_init_value(ndn_generic_pdu); 
        
        asn_write_component_value(asn_pdu, eth_pdu, "#eth");
        asn_insert_indexed(traffic_nds, asn_pdu, 1, "pdus"); 

        asn_free_value(asn_pdu);
        asn_free_value(eth_pdu);
    }

    UNUSED(csap_descr);

    return 0;
}


/* See description tad_bridge_impl.h */
te_errno 
tad_bridge_eth_init_cb(csap_p csap_descr, unsigned int layer,
                       const asn_value *csap_nds)
{
    if (csap_nds == NULL)
        return TE_EWRONGPTR;

    csap_descr->check_pdus_cb = tad_bridge_eth_check_pdus;

    F_VERB("bridge_eth_init_cb called for csap %d, layer %d\n",
           csap_descr->id, layer);
       
    return 0;
}


/* See description tad_bridge_impl.h */
te_errno
tad_bridge_eth_destroy_cb(csap_p csap_descr, unsigned int layer)
{
    UNUSED(csap_descr);
    UNUSED(layer);

    return 0;
}
