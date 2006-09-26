/** @file
 * @brief TAD ATM
 *
 * Traffic Application Domain Command Handler.
 * ATM CSAP, stack-related callbacks.
 *
 * Copyright (C) 2006 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
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

#define TE_LGR_USER     "TAD ATM"

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

#if HAVE_ASSERT_H
#include <assert.h>
#endif

#include "te_alloc.h"
#include "logger_api.h"

#include "ndn_atm.h" 
#include "tad_eth_sap.h"
#include "tad_atm_impl.h"


/** Ethernet layer read/write specific data */
typedef struct tad_atm_rw_data {
    tad_eth_sap     sap;        /**< Ethernet service access point */
} tad_atm_rw_data;


/* See description tad_atm_impl.h */
te_errno
tad_atm_prepare_send(csap_p csap)
{ 
    tad_atm_rw_data *spec_data = csap_get_rw_data(csap);

    assert(spec_data != NULL);

    return tad_eth_sap_send_open(&spec_data->sap, 0 /* FIXME */);
}

/* See description tad_atm_impl.h */
te_errno
tad_atm_shutdown_send(csap_p csap)
{
    tad_atm_rw_data *spec_data = csap_get_rw_data(csap);

    assert(spec_data != NULL);

    return tad_eth_sap_send_close(&spec_data->sap);
}


/* See description tad_atm_impl.h */
te_errno
tad_atm_prepare_recv(csap_p csap)
{ 
    tad_atm_rw_data *spec_data = csap_get_rw_data(csap);

    assert(spec_data != NULL);

    return tad_eth_sap_recv_open(&spec_data->sap, TAD_ETH_RECV_HOST);
}

/* See description tad_atm_impl.h */
te_errno
tad_atm_shutdown_recv(csap_p csap)
{
    tad_atm_rw_data *spec_data = csap_get_rw_data(csap);

    assert(spec_data != NULL);

    return tad_eth_sap_recv_close(&spec_data->sap);
}


/* See description tad_atm_impl.h */
te_errno
tad_atm_read_cb(csap_p csap, unsigned int timeout,
                tad_pkt *pkt, size_t *pkt_len)
{
    tad_atm_rw_data *spec_data = csap_get_rw_data(csap);

    assert(spec_data != NULL);

    return tad_eth_sap_recv(&spec_data->sap, timeout, pkt, pkt_len);
}


/* See description tad_atm_impl.h */
te_errno
tad_atm_write_cb(csap_p csap, const tad_pkt *pkt)
{ 
    tad_atm_rw_data *spec_data = csap_get_rw_data(csap);

    assert(spec_data != NULL);

    return tad_eth_sap_send(&spec_data->sap, pkt);
}


/* See description tad_atm_impl.h */
te_errno
tad_atm_rw_init_cb(csap_p csap)
{
    te_errno            rc; 
    unsigned int        layer = csap_get_rw_layer(csap);
    char                device_id[TAD_ETH_SAP_IFNAME_SIZE];
    size_t              val_len;
    tad_atm_rw_data    *spec_data; 
    

    val_len = sizeof(device_id);
    rc = asn_read_value_field(csap->layers[layer].nds,
                              device_id, &val_len, "device-id");
    if (rc != 0) 
    {
        ERROR("device-id for ATM not found: %r", rc);
        return TE_RC(TE_TAD_CSAP, rc);
    }
    
    spec_data = TE_ALLOC(sizeof(*spec_data));
    if (spec_data == NULL)
        return TE_RC(TE_TAD_CSAP, TE_ENOMEM);

    rc = tad_eth_sap_attach(device_id, &spec_data->sap);
    if (rc != 0)
    {
        ERROR("Failed to attach Ethernet read-write layer to media: %r",
              rc);
        return rc;
    }
    spec_data->sap.csap = csap;

    csap_set_rw_data(csap, spec_data);

    return 0;
}


/* See description tad_atm_impl.h */
te_errno
tad_atm_rw_destroy_cb(csap_p csap)
{
    tad_atm_rw_data    *spec_data = csap_get_rw_data(csap); 
    te_errno            rc;

    if (spec_data == NULL)
    {
        WARN("Not ATM CSAP %d special data found!", csap->id);
        return 0;
    }
    csap_set_rw_data(csap, NULL); 

    rc = tad_eth_sap_detach(&spec_data->sap);

    free(spec_data);   

    return rc;
}
