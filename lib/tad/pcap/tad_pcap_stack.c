/** @file
 * @brief TAD PCAP
 *
 * Traffic Application Domain Command Handler.
 * Ethernet-PCAP CSAP stack-related callbacks.
 *
 * Copyright (C) 2005 Test Environment authors (see file AUTHORS
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
 * @author Konstantin Abramenko <Konstantin.Abramenko@oktetlabs.ru>
 * @author Alexander Kukuta <Alexander.Kukuta@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER     "TAD Ethernet-PCAP"

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#if HAVE_SYS_IF_ETHER_H
#include <sys/sys_ether.h>
#endif
#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#if HAVE_FCNTL_H
#include <fcntl.h>
#endif

#include <assert.h>

#include <string.h>

#include "te_alloc.h"
#include "logger_api.h"
#include "logger_ta_fast.h"
#include "tad_common.h"
#include "ndn_pcap.h" 

#include "tad_eth_sap.h"
#include "tad_pcap_impl.h"



/** Ethernet layer read/write specific data */
typedef struct tad_pcap_rw_data {
    tad_eth_sap     sap;        /**< Ethernet service access point */
    unsigned int    recv_mode;  /**< Default receive mode */
} tad_pcap_rw_data;



/* See description tad_pcap_impl.h */
te_errno
tad_pcap_prepare_recv(csap_p csap)
{ 
    tad_pcap_rw_data *spec_data = csap_get_rw_data(csap);

    assert(spec_data != NULL);

    return tad_eth_sap_recv_open(&spec_data->sap, spec_data->recv_mode);
}

/* See description tad_pcap_impl.h */
te_errno
tad_pcap_shutdown_recv(csap_p csap)
{
    tad_pcap_rw_data *spec_data = csap_get_rw_data(csap);

    assert(spec_data != NULL);

    return tad_eth_sap_recv_close(&spec_data->sap);
}


/* See description tad_pcap_impl.h */
te_errno
tad_pcap_read_cb(csap_p csap, unsigned int timeout,
                tad_pkt *pkt, size_t *pkt_len)
{
    tad_pcap_rw_data *spec_data = csap_get_rw_data(csap);

    assert(spec_data != NULL);

    return tad_eth_sap_recv(&spec_data->sap, timeout, pkt, pkt_len);
}


/* See description tad_pcap_impl.h */
te_errno
tad_pcap_rw_init_cb(csap_p csap)
{
    te_errno            rc; 
    unsigned int        layer = csap_get_rw_layer(csap);
    char                device_id[TAD_ETH_SAP_IFNAME_SIZE];
    size_t              val_len;
    tad_pcap_rw_data   *spec_data; 
    const asn_value    *pcap_csap_spec; 
    

    pcap_csap_spec = csap->layers[layer].nds;
    
    val_len = sizeof(device_id);
    rc = asn_read_value_field(pcap_csap_spec, device_id, &val_len,
                              "ifname");
    if (rc != 0) 
    {
        ERROR("device-id for Ethernet not found: %r", rc);
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

    val_len = sizeof(spec_data->recv_mode);
    rc = asn_read_value_field(pcap_csap_spec, &spec_data->recv_mode, 
                              &val_len, "receive-mode");
    if (rc != 0)
    {
        spec_data->recv_mode = TAD_ETH_RECV_DEF;
    }

    csap_set_rw_data(csap, spec_data);

    return 0;
}


/* See description tad_pcap_impl.h */
te_errno
tad_pcap_rw_destroy_cb(csap_p csap)
{
    tad_pcap_rw_data    *spec_data = csap_get_rw_data(csap); 
    te_errno            rc;

    if (spec_data == NULL)
    {
        WARN("Not ethernet CSAP %d special data found!", csap->id);
        return 0;
    }
    csap_set_rw_data(csap, NULL); 

    rc = tad_eth_sap_detach(&spec_data->sap);

    free(spec_data);   

    return rc;
}
