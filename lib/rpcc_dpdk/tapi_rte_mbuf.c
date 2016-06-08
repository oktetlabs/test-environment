/** @file
 * @brief Test API to work with RTE mbufs
 *
 * Implementation of test API to work with RTE mbufs
 *
 *
 * Copyright (C) 2016 Test Environment authors (see file AUTHORS in
 * the root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 *
 * @author Ivan Malov <Ivan.Malov@oktetlabs.ru>
 */

#include "te_config.h"

#ifdef HAVE_NET_ETHERNET_H
#include <net/ethernet.h>
#endif

#include "tapi_rte_mbuf.h"
#include "tapi_rpc_rte_mbuf.h"
#include "tapi_mem.h"
#include "te_bufs.h"

rpc_rte_mbuf_p
tapi_rte_mk_mbuf_eth(rcf_rpc_server *rpcs,
                     rpc_rte_mempool_p mp,
                     const uint8_t *dst_addr, const uint8_t *src_addr,
                     const uint16_t ether_type,
                     const uint8_t *payload, size_t len)
{
    rpc_rte_mbuf_p m;
    uint8_t *frame;
    struct ether_header *eh;

    m = rpc_rte_pktmbuf_alloc(rpcs, mp);

    frame = tapi_calloc(1, sizeof(*eh) + len);

    eh = (struct ether_header *)frame;

    memcpy(eh->ether_dhost, dst_addr, sizeof(eh->ether_dhost));
    memcpy(eh->ether_shost, src_addr, sizeof(eh->ether_shost));

    eh->ether_type = ether_type;

    if (payload != NULL)
        memcpy(frame + sizeof(*eh), payload, len);
    else
        te_fill_buf(frame + sizeof(*eh), len);

    (void)rpc_rte_pktmbuf_append_data(rpcs, m, frame, sizeof(*eh) + len);

    return (m);
}

uint8_t *
tapi_rte_get_mbuf_data(rcf_rpc_server *rpcs,
                       rpc_rte_mbuf_p m, size_t offset, size_t *bytes_read)
{
    uint32_t pkt_len;
    uint8_t *data_buf;

    pkt_len = rpc_rte_pktmbuf_get_pkt_len(rpcs, m);

    data_buf = tapi_calloc(1, pkt_len - offset);

    *bytes_read = (size_t)rpc_rte_pktmbuf_read_data(rpcs,
                                                    m, offset, pkt_len - offset,
                                                    data_buf, pkt_len - offset);

    return (data_buf);
}
