/** @file
 * @brief Test Environment: 
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


struct tad_tmpl_arg_t;



/**
 * Type for reference to callback for init CSAP layer.
 *
 * @param csap_id       Identifier of CSAP.
 * @param csap_nds      Asn_value with CSAP init parameters
 * @param layer         Numeric index of layer in CSAP type to be processed.
 *                      Layers are counted from zero, from up to down.
 *
 * @return zero on success or error code.
 */ 
typedef int (*csap_nbr_init_cb_t)(int csap_id, 
                                  const asn_value *csap_nds, int layer);
/**
 * Type for reference to callback for destroy CSAP layer.
 *      This callback should free all undeground media resources used by 
 *      this layer and all memory used for layer-specific data and pointed
 *      in respective structure in 'layer-data' in CSAP instance struct. 
 *
 * @param csap_id       Identifier of CSAP.
 * @param layer         Numeric index of layer in CSAP type to be processed.
 *                      Layers are counted from zero, from up to down.
 *
 * @return zero on success or error code.
 */ 
typedef int (*csap_nbr_destroy_cb_t)(int csap_id, int layer);

/**
 * Type for reference to callback for confirm PDU with CSAP parameters and 
 *      possibilities.
 *
 * @param csap_id       Identifier of CSAP
 * @param layer         Numeric index of layer in CSAP type to be processed.
 * @param tmpl_pdu      asn_value with PDU (IN/OUT)
 *
 * @return zero on success or error code.
 */ 
typedef int (*csap_confirm_pdu_cb_t)(int csap_id, int layer, 
                                        asn_value * tmpl_pdu); 




/**
 * List of packet fragments, which compose one "message".
 * This list used for passing prepared/parsed packets from one layer
 * to another: from low to up during match, and from up to low 
 * constructing message to be sent. 
 */
struct csap_pkts;
typedef struct csap_pkts * csap_pkts_p;
typedef struct csap_pkts
{
    csap_pkts_p   next; /**< Pointer to next message fragment or NULL */

    void  *data;        /**< Pointer to data of this fragment */
    int    len;         /**< Length of this fragment */

    void (*free_data_cb)(void*); /**< Pointer to callback for free 
                                      fragment data, or NULL if usual 
                                      free() may be used. */
} csap_pkts;

/**
 * Type for reference to callback for generate binary data to be sent 
 * to media.
 *
 * @param csap_id       Identifier of CSAP
 * @param layer         Numeric index of layer in CSAP type to be processed.
 * @param tmpl_pdu      Asn_value with PDU. 
 * @param args          Template iteration parameters array, may be used to
 *                      prepare binary data.
 * @param arg_num       Length of array above. 
 * @param up_payload    Pointer to data which is already generated for 
 *                      upper layers and is payload for this protocol level.
 *                      May be zero.  Presented as list of packets. 
 *                      Almost always this list will contain only one 
 *                      element, but need in fragmentation sometimes may 
 *                      occur. Of cause, on up level only one PDU is passed,
 *                      but upper layer (if any present) may perform 
 *                      fragmentation, and current layer may have 
 *                      possibility to de-fragment payload.
 *                      Callback is responsible for freeing of memory, used
 *                      in up_payload list. 
 * @param pkts          Callback have to fill this structure with list of 
 *                      generated packets. Almost always this list will 
 *                      contain only one element, but need 
 *                      in fragmentation sometimes may occur. (OUT)
 *
 * @return zero on success or error code.
 */ 
typedef int (*csap_gen_bin_cb_t)(int csap_id, int layer,
                                 const asn_value *tmpl_pdu,
                                 const struct tad_tmpl_arg_t *args,
                                 size_t arg_num,
                                 csap_pkts_p up_payload,
                                 csap_pkts_p pkts);

/**
 * Type for reference to callback for parse received packet and match it
 * with pattern. 
 *
 * @param csap_id       Identifier of CSAP
 * @param layer         Numeric index of layer in CSAP type to be processed.
 * @param pattern_pdu   Pattern NDS 
 * @param pkt           Recevied packet
 * @param payload       Rest upper layer payload, if any exists. (OUT)
 * @param parsed_packet Caller of method should pass here empty asn_value
 *                      instance of ASN type 'Generic-PDU'. Callback 
 *                      have to fill this instance with values from 
 *                      parsed and matched packet
 *
 * @return zero on success or error code.
 */
typedef int (*csap_match_bin_cb_t)(int csap_id, int layer,
                                   const asn_value *pattern_pdu,
                                   const csap_pkts *pkt,
                                   csap_pkts *payload,
                                   asn_value *parsed_packet);


/**
 * Type for reference to callback for generating pattern to filter 
 * just one response to the packet which will be sent by this CSAP 
 * according to this template. 
 *
 * @param csap_id       Identifier of CSAP
 * @param layer         Numeric index of layer in CSAP type to be processed.
 * @param tmpl_pdu      ASN value with template PDU.
 * @param pattern_pdu   OUT: ASN value with pattern PDU, generated according
 *                      to passed template PDU and CSAP parameters. 
 *
 * @return zero on success or error code.
 */
typedef int (*csap_gen_pattern_cb_t)(int csap_id, int layer,
                                     const asn_value *tmpl_pdu, 
                                     asn_value **pattern_pdu);




struct csap_layer_neighbour_list_t;
typedef struct csap_layer_neighbour_list_t * csap_layer_neighbour_list_p;

/**
 * Structure for description of CSAP lower neighbours supported. 
 * This list specifies lower neighbours may present under 
 * that ('current') CSAP layer, which has in it's "CSAP type descriptor" 
 * reference to this list. 
 *
 * One 'low neighbour' sturct contains neighbour label and references to 
 * callbacks performing some actions, which may depend on low layer.
 */
typedef struct csap_layer_neighbour_list_t
{
    char *nbr_type; /**< symbolic identifier of neighvour. 
              May have such values: 
              NULL    - this means that layer which neighbours are listed 
                          is single in stack;
              "data"  - for data-CSAPs;
              id of lower neighbour level. */
    csap_layer_neighbour_list_p next; /**< pointer to the next possible
                                           neighbour*/

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
typedef struct csap_spt_type_t 
{
    char *proto;     /**< symbolic label of related protocol level */

    /* protocol-specific callbacks: */ 
    csap_confirm_pdu_cb_t confirm_cb;
    csap_gen_bin_cb_t     generate_cb;
    csap_match_bin_cb_t   match_cb;
    csap_gen_pattern_cb_t generate_pattern_cb;

    /* link to the list with possible (lower) neighbours, see description of
       this structure for more details. */ 
    csap_layer_neighbour_list_p neighbours;
} *csap_spt_type_p, csap_spt_type_t;



/* ============= Function prototypes declarations =============== */


/**
 * Init CSAP support database
 *
 * @param spt_descr     CSAP layer support structure. 
 *
 * @return zero on success, otherwise error code. 
 */
extern int init_csap_spt(void);


/**
 * Add structure for CSAP support for respective protocol.
 *
 * @param spt_descr     CSAP layer support structure. 
 *
 * @return zero on success, otherwise error code. 
 */
extern int add_csap_spt(csap_spt_type_p spt_descr);


/**
 * Find structure for CSAP support respective to passed protocol label.
 *
 * @param proto      protocol label.
 *
 * @return pointer to structure or NULL if not found. 
 */
extern csap_spt_type_p find_csap_spt(const char * proto);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /*  __TE_TAD_CSAP_SUPPORT_H__ */
