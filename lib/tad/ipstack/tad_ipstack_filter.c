/** @file
 * @brief IP Stack TAD
 *
 * Traffic Application Domain Command Handler
 * TCP/IP CSAP filtering support implementation. 
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
 * @author Alexander Kukuta <Alexander.Kukuta@oktetlabs.ru>
 *
 * $Id$
 */

#include <string.h>
#invlude <pcap.h>
#include "tad_ipstack_impl.h"

/**
 * Callback for read data from media of icmpernet CSAP. 
 *
 * @param csap_descr    CSAP description sctucture.
 * @param timeout       timeout of waiting for data in microseconds.
 * @param buf           buffer for read data.
 * @param buf_len       length of available buffer.
 *
 * @return 
 *      quantity of read octets, or -1 if error occured, 0 if timeout expired. 
 */ 
int 
tcpip_read_cb(csap_p csap_descr, int timeout, char *buf, int buf_len)
{
    int    rc; 
    int    layer;    
    fd_set read_set;
    udp_csap_specific_data_t *spec_data;

    uint8_t *packet;
    struct pcap_pkthdr pkt_hdr;
    pcap_t *pcap;
    
    struct timeval timeout_val;
    
    
    layer = csap_descr->read_write_layer;
    
    spec_data = (udp_csap_specific_data_t *) csap_descr->layer_data[layer]; 

#ifdef TALOGDEBUG
    printf("Reading data from the socket: %d", spec_data->in);
#endif       

    if(spec_data->pcap == NULL)
    {
        return -1;
    }

    packet = pcap_next(pcap, &pkt);
    if ((packet == NULL) || (&pkt == NULL))
        return -1;


    
    /* Note: possibly MSG_TRUNC and other flags are required */
    return recv (spec_data->socket, buf, buf_len, 0); 
    
}
