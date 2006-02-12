/** @file
 * @brief TAD PCAP
 *
 * Traffic Application Domain Command Handler.
 * Ethernet with Lib PCAP filtering CSAP support description structures. 
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

#define TE_LGR_USER     "TAD PCAP"

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

#include "te_errno.h"
#include "tad_csap_support.h"
#include "tad_utils.h"

#include "tad_pcap_impl.h"


static csap_spt_type_t pcap_csap_spt = 
{
    proto               : "pcap",
    unregister_cb       : NULL,

    init_cb             : tad_pcap_init_cb,
    destroy_cb          : tad_pcap_destroy_cb,
    get_param_cb        : NULL,

    confirm_tmpl_cb     : NULL,
    generate_pkts_cb    : NULL,
    release_tmpl_cb     : NULL,

    confirm_ptrn_cb     : tad_pcap_confirm_ptrn_cb,
    match_pre_cb        : NULL,
    match_do_cb         : tad_pcap_match_bin_cb,
    match_done_cb       : NULL,
    match_post_cb       : NULL,
    match_free_cb       : NULL,
    release_ptrn_cb     : tad_pcap_release_ptrn_cb,

    generate_pattern_cb : NULL,

    rw_init_cb          : tad_pcap_rw_init_cb,
    rw_destroy_cb       : tad_pcap_rw_destroy_cb,

    prepare_send_cb     : NULL,
    write_cb            : NULL,
    shutdown_send_cb    : NULL,
    
    prepare_recv_cb     : tad_pcap_prepare_recv,
    read_cb             : tad_pcap_read_cb,
    shutdown_recv_cb    : tad_pcap_shutdown_recv,

    write_read_cb       : NULL,
};


/**
 * Register Ethernet-PCAP CSAP callbacks and support structures in TAD
 * Command Handler.
 *
 * @return Status code
 */ 
te_errno
csap_support_pcap_register(void)
{ 
    return csap_spt_add(&pcap_csap_spt);
}
