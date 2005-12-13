/** @file
 * @brief TAD Command Handler
 *
 * Traffic Application Domain Command Handler
 *
 * Declarations of types and functions, used in common and 
 * protocol-specific modules implemnting TAD.
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
 * @author Konstantin Abramenko <konst@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_TAD_CSAP_SUPPORT_H__
#define __TE_TAD_CSAP_SUPPORT_H__ 

#ifndef PACKAGE_VERSION
#include "config.h"
#endif

#if HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_PTHREAD_H
#include <pthread.h>
#endif


#include "te_stdint.h"
#include "te_errno.h"
#include "asn_usr.h" 
#include "tad_common.h"

#include "tad_csap_inst.h"

#ifdef __cplusplus
extern "C" {
#endif



/* ============= Types and structures definitions =============== */


/* Forward declaration of type for usage in callback prototypes */
struct tad_tmpl_arg_t; 
typedef struct tad_tmpl_arg_t tad_tmpl_arg_t;


/**
 * Callback type to init CSAP layer part which depends on lower
 * neighbour.
 *
 * @param csap_descr    CSAP descriptor structure.
 * @param layer         Numeric index of layer in CSAP type to be processed.
 *                      Layers are counted from zero, from up to down.
 * @param csap_nds      Asn_value with CSAP init parameters
 *
 * @return Status code.
 */ 
typedef te_errno (*csap_nbr_init_cb_t)(csap_p           csap_descr,
                                       unsigned int     layer,
                                       const asn_value *csap_nds);

/**
 * Callback type to destroy CSAP layer part which depends on lower
 * neighbour.
 *
 * @param csap_descr    CSAP descriptor structure.
 * @param layer         Numeric index of layer in CSAP type to be processed.
 *                      Layers are counted from zero, from up to down.
 *
 * @return Status code.
 */ 
typedef te_errno (*csap_nbr_destroy_cb_t)(csap_p       csap_descr,
                                          unsigned int layer);


/**
 * Callback type to initialize CSAP layer.
 *
 * @param csap_descr    CSAP descriptor structure.
 * @param layer         Numeric index of layer in CSAP type to be processed.
 *                      Layers are counted from zero, from up to down.
 * @param csap_nds      ASN.1 value with CSAP init parameters
 *
 * @return Status code.
 */ 
typedef te_errno (*csap_layer_init_cb_t)(csap_p           csap_descr,
                                         unsigned int     layer,
                                         const asn_value *csap_nds);

/**
 * Callback type to destroy CSAP layer.
 * This callback should free all undeground media resources used by 
 * this layer and all memory used for layer-specific data and pointed
 * in respective structure in 'layer-data' in CSAP instance structure. 
 *
 * @param csap_descr    CSAP descriptor structure.
 * @param layer         Numeric index of layer in CSAP type to be processed.
 *                      Layers are counted from zero, from up to down.
 *
 * @return Status code.
 */ 
typedef te_errno (*csap_layer_destroy_cb_t)(csap_p       csap_descr,
                                            unsigned int layer);

/**
 * Callback type to confirm Traffic Pattern or Template PDU with
 * CSAP parameters and possibilities.
 * For example, it checks that there is sufficient information for 
 * traffic generating, and writes CSAP defaults to Traffic PDU.
 *
 * @param[in]    csap_descr    CSAP descriptor structure.
 * @param[in]    layer         numeric index of layer in CSAP type 
 *                             to be processed
 * @param[inout] layer_pdu     ASN.1 value with PDU
 *
 * @return Status code.
 */ 
typedef te_errno (*csap_layer_confirm_pdu_cb_t)(
                       csap_p        csap_descr,
                       unsigned int  layer, 
                       asn_value    *layer_pdu); 


struct csap_pkts;

/**
 * Pointer to packet struct
 */
typedef struct csap_pkts *csap_pkts_p;

/**
 * List of packet fragments, which compose one "message".
 * This list used for passing prepared/parsed packets from one layer
 * to another: from low to up during match, and from up to low 
 * constructing message to be sent. 
 */
typedef struct csap_pkts {
    csap_pkts_p next;   /**< Pointer to next message fragment or NULL */

    void       *data;   /**< Pointer to data of this fragment */
    size_t      len;    /**< Length of this fragment */

    void (*free_data_cb)(void *); /**< Pointer to callback for free 
                                       fragment data, or NULL if usual 
                                       free() may be used. */
} csap_pkts;


/**
 * Callback type to generate binary data to be sent to media.
 * If some iteration was specified in traffic template, it done on the
 * upper layer of template processing, this callback is called for every
 * set of iteration parameters values. 
 *
 * @param csap_descr    CSAP descriptor structure.
 * @param layer         Numeric index of layer in CSAP type to be processed.
 * @param tmpl_pdu      Asn_value with PDU. 
 * @param args          Array with values of template iteration parameters,
 *                      must be used to prepare binary data, if references 
 *                      to these paramters are present in traffic
 *                      template PDU.
 * @param arg_num       Length of array above. 
 * @param up_payload    Pointer to data which is already generated for 
 *                      upper layers and is payload for this protocol layer.
 *                      May be zero.  Presented as list of packets. 
 *                      Almost always this list will contain only one 
 *                      element, but need in fragmentation sometimes may 
 *                      occur. Of cause, to high layer only one PDU
 *                      is passed, but lower layers (if any present) may
 *                      perform fragmentation, and current layer may have 
 *                      possibility to de-fragment payload.
 *                      Callback is responsible for freeing of memory, used
 *                      in up_payload list. 
 * @param pkts          Callback have to fill this structure with list of 
 *                      generated packets. Almost always this list will 
 *                      contain only one element, but need 
 *                      in fragmentation sometimes may occur. (OUT)
 *
 * @return Status code.
 */ 
typedef te_errno (*csap_layer_gen_bin_cb_t)(
                       csap_p                csap_descr,
                       unsigned int          layer,
                       const asn_value      *tmpl_pdu,
                       const tad_tmpl_arg_t *args,
                       size_t                arg_num,
                       csap_pkts_p           up_payload,
                       csap_pkts_p           pkts);

/**
 * Callback type to parse received packet and match it with pattern. 
 *
 * @param csap_descr    CSAP descriptor structure.
 * @param layer         Numeric index of layer in CSAP type to be processed
 * @param pattern_pdu   Pattern NDS 
 * @param pkt           Recevied packet, may be list of fragments, which 
 *                      all should be defragmented by this callback and 
 *                      information should be put into single PDU
 * @param payload       Rest upper layer payload, if present (OUT)
 * @param parsed_packet Caller of method should pass here empty asn_value
 *                      instance of ASN type 'Generic-PDU'. Callback 
 *                      have to fill this instance with values from 
 *                      parsed and matched packet.
 *
 * @return Status code.
 */
typedef te_errno (*csap_layer_match_bin_cb_t)(
                       csap_p           csap_descr,
                       unsigned int     layer,
                       const asn_value *pattern_pdu,
                       const csap_pkts *pkt,
                       csap_pkts       *payload,
                       asn_value       *parsed_packet);

/**
 * Callback type to generating pattern to filter 
 * just one response to the packet which will be sent by this CSAP 
 * according to this template. 
 *
 * @param csap_descr    CSAP descriptor structure.
 * @param layer         Numeric index of layer in CSAP type to be processed.
 * @param tmpl_pdu      ASN value with template PDU.
 * @param pattern_pdu   OUT: ASN value with pattern PDU, generated according
 *                      to passed template PDU and CSAP parameters. 
 *
 * @return Status code.
 */
typedef te_errno (*csap_layer_gen_pattern_cb_t)(
                       csap_p            csap_descr,
                       unsigned int      layer,
                       const asn_value  *tmpl_pdu, 
                       asn_value       **pattern_pdu);


/*=====================================================================
 * Structures for CSAP types support specifications. 
 *
 * Overview: 
 *
 * CSAPs have layered structure, which layer respective to some protocol, 
 * neighbour layers respective to neighbour protocols. 
 * CSAP type is sequence of symbolic protocol labels, from upper to lower, 
 * separated by dots. 
 * For example, currently supported multy-layered CSAP types are: 
 * 'bridge.eth', 'ip4.eth', 'tcp.ip4.eth', 'udp.ip4.eth'.
 *
 * Lowest layer sends/receives data by means which are not related to TAD,
 * for example, NET-SNMP library, usual TCP/UDP network socket, 
 * Ethernet packet socket, etc.
 *
 * Layer, which has some under it, only prepares data to be sent or 
 * matches data, received from lower layer. 
 *
 * The following structures hold specification of supported CSAP types. 
 * They are organized as some heap with 'protocol' specifications, 
 * which have lists with supported lower layers for particular protocol. 
 *
 * For example, having only three supported CSAP types 'udp', 'ip4', 
 * and 'udp.ip4', we will have the following heap: 
 *
 * 'udp', list with two lower neighbours: NULL and 'ip4';
 * 'ip4', list with one lower neighbour: NULL.
 *
 * Note there is impossible to specify support only TWO CSAP types
 * 'udp.ip4' and 'udp'. Moreover, it is insensible, becouse support of 
 * IPv4 protol layer (based, for example, on IPv4 raw sockets) is almost
 * independent on upper protocols.
 */



struct csap_layer_neighbour_list_t;
/**
 * Pointer to neigbour list struct
 */
typedef struct csap_layer_neighbour_list_t *csap_layer_neighbour_list_p;

/**
 * Structure for description of CSAP lower neighbours supported. 
 * This list specifies lower neighbours may present under 
 * that ('current') CSAP layer, which has in it's "CSAP type descriptor" 
 * reference to this list. 
 *
 * One 'low neighbour' sturct contains neighbour label and references to 
 * callbacks performing some actions, which may depend on low layer.
 * These callbacks are responsible for non-TAD external means used for 
 * traffic operations. 
 */
typedef struct csap_layer_neighbour_list_t {
    csap_layer_neighbour_list_p next; /**< pointer to the next possible
                                           neighbour*/

    char *nbr_type; /**< symbolic identifier of neighbour. 
              May have such values: 
              NULL    - this means that layer which neighbours are listed 
                        is single in stack;
              "data"  - for data-CSAPs;
              id of lower neighbour layer. */

    csap_nbr_init_cb_t    init_cb;    /**< Callback for initialize 
                                           'current' CSAP layer regarding 
                                           with this low layer. */
    csap_nbr_destroy_cb_t destroy_cb; /**< Callback for free resources, 
                                           related to 'current' CSAP layer*/

   
} csap_layer_neighbour_list_t;

/**
 * Structure for description of particluar CSAP layer type supported
 * in current TAD build.
 * It contains some pointers to specific layer-dependent callbacks
 * and list with supported lower neighbours. 
 */
typedef struct csap_spt_type_t {
    char *proto;    /**< Symbolic label of related protocol layer */

    /** @name protocol-specific callbacks */ 
    csap_layer_init_cb_t          init_cb;
    csap_layer_destroy_cb_t       destroy_cb;
    csap_layer_confirm_pdu_cb_t   confirm_cb;
    csap_layer_gen_bin_cb_t       generate_cb;
    csap_layer_match_bin_cb_t     match_cb;
    csap_layer_gen_pattern_cb_t   generate_pattern_cb;
    /*@}*/

    csap_layer_neighbour_list_p neighbours; /**< Link to the list
                                                 with possible (lower)
                                                 neighbours */ 
} *csap_spt_type_p, csap_spt_type_t;



/* ============= Function prototypes declarations =============== */


/**
 * Init CSAP support database
 *
 * @param spt_descr     CSAP layer support structure
 *
 * @return Status code.
 */
extern te_errno init_csap_spt(void);


/**
 * Add structure for CSAP support for respective protocol.
 *
 * @param spt_descr     CSAP layer support structure
 *
 * @return Status code.
 */
extern te_errno add_csap_spt(csap_spt_type_p spt_descr);


/**
 * Find structure for CSAP support respective to passed protocol label.
 *
 * @param proto      protocol label
 *
 * @return Pointer to structure or NULL if not found. 
 */
extern csap_spt_type_p find_csap_spt(const char *proto);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /*  __TE_TAD_CSAP_SUPPORT_H__ */
