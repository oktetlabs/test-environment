/** @file
 * @brief TAD CSAP Support
 *
 * Traffic Application Domain Command Handler.
 *
 * Declarations of types and functions, used in common and 
 * protocol-specific modules implemnting TAD.
 *
 * Copyright (C) 2003-2006 Test Environment authors (see file AUTHORS
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
#include "tad_pkt.h"
#include "tad_recv_pkt.h"


#ifdef __cplusplus
extern "C" {
#endif



/* ============= Types and structures definitions =============== */


/* Forward declaration of type for usage in callback prototypes */
struct tad_tmpl_arg_t; 
typedef struct tad_tmpl_arg_t tad_tmpl_arg_t;

/**
 * Callback type to release resources allocated by CSAP protocol
 * support initialization.
 *
 * @return Status code.
 */
typedef void (*csap_spt_unregister_cb_t)(void);

/**
 * Callback type to initialize CSAP layer.
 *
 * @param csap    CSAP instance.
 * @param layer         Numeric index of layer in CSAP type to be processed.
 *                      Layers are counted from zero, from up to down.
 *
 * @return Status code.
 */ 
typedef te_errno (*csap_layer_init_cb_t)(csap_p       csap,
                                         unsigned int layer);

/**
 * Callback type to destroy CSAP layer.
 * This callback should free all undeground media resources used by 
 * this layer and all memory used for layer-specific data and pointed
 * in respective structure in 'layer-data' in CSAP instance structure. 
 *
 * @param csap    CSAP instance.
 * @param layer         Numeric index of layer in CSAP type to be processed.
 *                      Layers are counted from zero, from up to down.
 *
 * @return Status code.
 */ 
typedef te_errno (*csap_layer_destroy_cb_t)(csap_p       csap,
                                            unsigned int layer);

/**
 * Callback type to read parameter value of CSAP.
 *
 * @param csap    CSAP instance.
 * @param layer         Index of layer in CSAP stack, which param is wanted.
 * @param param         Protocol-specific name of parameter.
 *
 * @return
 *     String with textual presentation of parameter value, or NULL 
 *     if error occured. User have to free memory at returned pointer.
 */ 
typedef char *(*csap_layer_get_param_cb_t)(csap_p        csap,
                                           unsigned int  layer, 
                                           const char   *param);

/**
 * Callback type to confirm Traffic Pattern or Template PDU with
 * CSAP parameters and possibilities.
 * For example, it checks that there is sufficient information for 
 * traffic generating, and writes CSAP defaults to Traffic PDU.
 *
 * @param[in]    csap CSAP instance.
 * @param[in]    layer      numeric index of layer in CSAP type 
 *                          to be processed
 * @param[inout] layer_pdu  ASN.1 value with PDU
 * @param[out]   p_opaque   Location for opaque data pointer to be
 *                          passed later to binary data generation
 *                          callback
 *
 * @return Status code.
 */ 
typedef te_errno (*csap_layer_confirm_pdu_cb_t)(
                       csap_p         csap,
                       unsigned int   layer, 
                       asn_value     *layer_pdu,
                       void         **p_opaque); 

/**
 * Callback to release opaque data prepared by confirm callback.
 *
 * @param csap          CSAP instance
 * @param layer         Numeric index of layer in CSAP type to be processed
 * @param opaque        Opaque data pointer prepared by confirm callback
 */
typedef void (*csap_layer_release_opaque_cb_t)(csap_p        csap,
                                               unsigned int  layer,
                                               void         *opaque);


/**
 * Callback type to generate binary data to be sent to media.
 * If some iteration was specified in traffic template, it done on the
 * upper layer of template processing, this callback is called for every
 * set of iteration parameters values. 
 *
 * @param csap    CSAP instance.
 * @param layer         Numeric index of layer in CSAP type to be processed.
 * @param tmpl_pdu      ASN.1 value with PDU.
 * @param opaque        Opaque data prepared by confirm callback.
 * @param args          Array with values of template iteration parameters,
 *                      must be used to prepare binary data, if references 
 *                      to these paramters are present in traffic
 *                      template PDU.
 * @param arg_num       Length of array above. 
 * @param sdus          List of upper layer packets. May be NULL or
 *                      empty list.
 * @param pdus          List to put packets generated by the layer.
 *
 * @return Status code.
 */ 
typedef te_errno (*csap_layer_generate_pkts_cb_t)(
                       csap_p                csap,
                       unsigned int          layer,
                       const asn_value      *tmpl_pdu,
                       void                 *opaque,
                       const tad_tmpl_arg_t *args,
                       size_t                arg_num,
                       tad_pkts             *sdus,
                       tad_pkts             *pdus);


/**
 * Callback to allocate and/or prepare per received packet layer opaque
 * data.
 *
 * It called on match fast path for each matched packet plus one.
 *
 * @param csap              CSAP instance
 * @param layer             Numeric index of the layer
 * @param meta_pkt_layer    Receiver meta-packet layer
 *
 * @return Status code.
 */
typedef te_errno (*csap_layer_match_pre_cb_t)(
                       csap_p              csap,
                       unsigned int        layer,
                       tad_recv_pkt_layer *meta_pkt_layer);

typedef csap_layer_match_pre_cb_t csap_layer_match_post_cb_t;


/**
 * Callback type to parse received packet and match it with pattern. 
 *
 * It called on match fast path for each received packet.
 *
 * @param csap          CSAP instance
 * @param layer         Numeric index of layer in CSAP type to be processed
 * @param ptrn_pdu      Pattern NDS for the layer
 * @param ptrn_opaque   Opaque data prepared by confirm_ptrn_cb
 * @param meta_pkt      Receiver meta-packet
 * @param pdu           Received packet (protocol data unit of the layer)
 * @param sdu           Rest upper layer payload (service data unit of
 *                      the layer)
 *
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
typedef te_errno (*csap_layer_match_do_cb_t)(
                       csap_p           csap,
                       unsigned int     layer,
                       const asn_value *ptrn_pdu,
                       void            *ptrn_opaque,
                       tad_recv_pkt    *meta_pkt,
                       tad_pkt         *pdu,
                       tad_pkt         *sdu);

typedef csap_layer_match_do_cb_t csap_layer_match_done_cb_t;


/**
 * Callback type to generating pattern to filter 
 * just one response to the packet which will be sent by this CSAP 
 * according to this template. 
 *
 * @param csap    CSAP instance.
 * @param layer         Numeric index of layer in CSAP type to be processed.
 * @param tmpl_pdu      ASN value with template PDU.
 * @param pattern_pdu   OUT: ASN value with pattern PDU, generated according
 *                      to passed template PDU and CSAP parameters. 
 *
 * @return Status code.
 */
typedef te_errno (*csap_layer_gen_pattern_cb_t)(
                       csap_p            csap,
                       unsigned int      layer,
                       const asn_value  *tmpl_pdu, 
                       asn_value       **pattern_pdu);


/**
 * Callback type to init CSAP layer part which is responsible for
 * read/write.
 *
 * @param csap          CSAP instance.
 * @param csap_nds      Asn_value with CSAP init parameters
 *
 * @return Status code.
 */ 
typedef te_errno (*csap_rw_init_cb_t)(csap_p           csap,
                                      const asn_value *csap_nds);

/**
 * Callback type to destroy CSAP layer part which is responsible for
 * read/write.
 *
 * @param csap          CSAP instance.
 *
 * @return Status code.
 */ 
typedef te_errno (*csap_rw_destroy_cb_t)(csap_p csap);


/**
 * Callback type to prepare/release low-layer resources 
 * of CSAP used in traffic process.
 * Usually should open/close sockets, etc. 
 *
 * @param csap    CSAP instance. 
 *
 * @return Status code.
 */ 
typedef te_errno (*csap_low_resource_cb_t)(csap_p csap);

/**
 * Callback type to read data from media of the CSAP.
 *
 * @param csap          CSAP instance
 * @param timeout       Timeout of waiting for data in microseconds
 * @param pkt           Packet for received data
 * @param pkt_len       Location for real length of the received packet
 *
 * @return Status code.
 */ 
typedef te_errno (*csap_read_cb_t)(csap_p csap, unsigned int timeout,
                                   tad_pkt *pkt, size_t *pkt_len);

/**
 * Callback type to write data to media of the CSAP.
 *
 * @param csap          CSAP instance
 * @param pkt           Packet to send
 *
 * @return Status code.
 */ 
typedef te_errno (*csap_write_cb_t)(csap_p csap, const tad_pkt *pkt);

/**
 * Callback type to write data to media of CSAP and read
 *  data from media just after write, to get answer to sent request. 
 *
 * @param csap          CSAP instance
 * @param timeout       Timeout of waiting for data in microseconds
 * @param w_pkt         Packet to be sent
 * @param r_pkt         Packet for received data
 * @param r_pkt_len     Location for real length of the received packet
 *
 * @return Status code.
 */ 
typedef te_errno (*csap_write_read_cb_t)(csap_p csap, unsigned int timeout,
                                         const tad_pkt *w_pkt,
                                         tad_pkt *r_pkt, size_t *r_pkt_len);

#if 0
/**
 * Callback type to echo CSAP method. 
 * Method should prepare binary data to be send as "echo" and call 
 * respective write method to send it. 
 * Method may change data stored at passed location.
 *
 * @param csap          CSAP instance. 
 * @param pkt           Got packet, plain binary data. 
 * @param len           Length of packet.
 *
 * @return Status code.
 */
typedef te_errno (*csap_echo_method)(csap_p csap, uint8_t *pkt, 
                                     size_t len);
#endif

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


/**
 * Structure for description of particluar CSAP layer type supported
 * in current TAD build.
 * It contains some pointers to specific layer-dependent callbacks
 * and list with supported lower neighbours. 
 */
typedef struct csap_spt_type_t {
    const char *proto;  /**< Symbolic label of the protocol layer */

    csap_spt_unregister_cb_t        unregister_cb;

    /** @name protocol-specific callbacks */ 
    csap_layer_init_cb_t            init_cb;
    csap_layer_destroy_cb_t         destroy_cb;
    csap_layer_get_param_cb_t       get_param_cb;

    csap_layer_confirm_pdu_cb_t     confirm_tmpl_cb;
    csap_layer_generate_pkts_cb_t   generate_pkts_cb;
    csap_layer_release_opaque_cb_t  release_tmpl_cb;

    csap_layer_confirm_pdu_cb_t     confirm_ptrn_cb;
    csap_layer_match_pre_cb_t       match_pre_cb;
    csap_layer_match_do_cb_t        match_do_cb;
    csap_layer_match_done_cb_t      match_done_cb;
    csap_layer_match_post_cb_t      match_post_cb;
    csap_layer_release_opaque_cb_t  match_free_cb;
    csap_layer_release_opaque_cb_t  release_ptrn_cb;

    csap_layer_gen_pattern_cb_t     generate_pattern_cb;
    /*@}*/

    csap_rw_init_cb_t       rw_init_cb;
    csap_rw_destroy_cb_t    rw_destroy_cb;

    csap_low_resource_cb_t  prepare_send_cb;
    csap_write_cb_t         write_cb;
    csap_low_resource_cb_t  shutdown_send_cb;

    csap_low_resource_cb_t  prepare_recv_cb;
    csap_read_cb_t          read_cb;
    csap_low_resource_cb_t  shutdown_recv_cb;

    csap_write_read_cb_t    write_read_cb;
#if 0
    csap_echo_method     echo_cb;       /**< method for echo */
#endif

} *csap_spt_type_p, csap_spt_type_t;

/**
 * Helper macro to initialize part of csap_spt_type_t structure,
 * if layer does not provide read/write functions.
 */
#define CSAP_SUPPORT_NO_RW \
    rw_init_cb       : NULL,    \
    rw_destroy_cb    : NULL,    \
                                \
    prepare_send_cb  : NULL,    \
    write_cb         : NULL,    \
    shutdown_send_cb : NULL,    \
                                \
    prepare_recv_cb  : NULL,    \
    read_cb          : NULL,    \
    shutdown_recv_cb : NULL,    \
                                \
    write_read_cb    : NULL


/**
 * Init CSAP support database
 *
 * @return Status code.
 */
extern te_errno csap_spt_init(void);

/**
 * Add structure for CSAP support for respective protocol.
 *
 * @param spt_descr     CSAP layer support structure
 *
 * @return Status code.
 */
extern te_errno csap_spt_add(csap_spt_type_p spt_descr);

/**
 * Find structure for CSAP support respective to passed protocol label.
 *
 * @param proto      protocol label
 *
 * @return Pointer to structure or NULL if not found. 
 */
extern csap_spt_type_p csap_spt_find(const char *proto);

/**
 * Destroy CSAP support database.
 */
extern void csap_spt_destroy(void);


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /*  __TE_TAD_CSAP_SUPPORT_H__ */
