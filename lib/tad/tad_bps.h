/** @file
 * @brief TAD Binary Protocol Support
 *
 * Traffic Application Domain Command Handler.
 * Traffic Application Domain Binary Protocol Support definitions
 * of data types and provided functions.
 *
 * Copyright (C) 2005-2006 Test Environment authors (see file AUTHORS
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

#ifndef __TE_TAD_BPS_H__
#define __TE_TAD_BPS_H__

#include "tad_utils.h"


#define ASN_TAG_INVALID     ((asn_tag_value)-1)
#define ASN_TAG_USER        ((asn_tag_value)-2)
#define ASN_TAG_CONST       ((asn_tag_value)-3)

#define BPS_FLD_SIMPLE(_x)  _x, _x, _x, 0
#define BPS_FLD_NO_DEF(_x)  _x, ASN_TAG_INVALID, ASN_TAG_INVALID, 0
#define BPS_FLD_CONST(_val) ASN_TAG_INVALID, ASN_TAG_CONST, \
                            ASN_TAG_CONST, (_val)

/**
 * Binary protocol packet fragment field description.
 */
typedef struct tad_bps_pkt_frag {
    const char     *name;       /**< Name of the field in PDU NDS */
    unsigned int    len;        /**< Length of the field in bits
                                    (0 means variable length) */
    asn_tag_value   tag;        /**< ASN.1 tag of the field in header */
    asn_tag_value   tag_tx_def; /**< ASN.1 tag of the field in CSAP
                                     parameters default for sending */
    asn_tag_value   tag_rx_def; /**< ASN.1 tag of the field in CSAP
                                     parameters default for receiving */
    uint32_t        value;      /**< Constant value */
} tad_bps_pkt_frag;

/**
 * Internal data of BPS for binary packet fragment definition.
 */
typedef struct tad_bps_pkt_frag_def {
    unsigned int            fields; /**< Number of fields */
    const tad_bps_pkt_frag *descr;  /**< Array of fields */
    
    tad_data_unit_t        *tx_def; /**< Array of TAD data units for
                                         fragment fields Tx defaults */
    tad_data_unit_t        *rx_def; /**< Array of TAD data units for
                                         fragment fields Rx defaults */
                                         
} tad_bps_pkt_frag_def;

/**
 * Internal data of PBS for binary packet fragment.
 */
typedef struct tad_bps_pkt_frag_data {
    tad_data_unit_t    *dus;    /**< Array of TAD data units for
                                     fragment fields */
} tad_bps_pkt_frag_data;


/**
 * Initialize TAD binary PDU support for the binary PDU type.
 *
 * There is no necessity to do any clean up in this routing in the case
 * of failure. Thinks should be just consistent to allow
 * tad_bps_pkt_frag_free() to do its work.
 *
 * @param descr         Binary PDU description
 * @param fields        Number of fields in description
 * @param layer_spec    NDS with default values
 * @param bsp           BPS internal data to be initialized
 *
 * @return Status code.
 */
extern te_errno tad_bps_pkt_frag_init(const tad_bps_pkt_frag *descr,
                                      unsigned int            fields,
                                      const asn_value        *layer_spec,
                                      tad_bps_pkt_frag_def   *bps);

/**
 * Free resources allocated by tad_bps_pkt_frag_init().
 *
 * @param bsp           BPS internal data to be freed
 */
extern void tad_bps_pkt_frag_free(tad_bps_pkt_frag_def *bps);


extern te_errno tad_bps_nds_to_data_units(
                    const tad_bps_pkt_frag_def *def,
                    const asn_value            *layer_pdu,
                    tad_bps_pkt_frag_data      *data);

/**
 * Free resources allocated for packet fragment data.
 *
 * @param def           Binary packet fragment definition filled in by
 *                      tad_bps_pkt_frag_init() function
 * @param data          Binary packet fragment instance data filled in
 *                      by tad_bps_nds_to_data_units() function
 */
extern void tad_bps_free_pkt_frag_data(const tad_bps_pkt_frag_def *def,
                                       tad_bps_pkt_frag_data      *data);


/**
 * Confirm that template plus defaults are enough to generate binary
 * packet.
 *
 * @param def           Binary packet fragment definition filled in by
 *                      tad_bps_pkt_frag_init() function
 * @param pkt           Binary packet fragment data allocated and
 *                      filled in by tad_bps_nds_to_data_units()
 *
 * @return Status code.
 * @retval TE_ETADMISSNDS   Data are not sufficient.
 */
extern te_errno tad_bps_confirm_send(const tad_bps_pkt_frag_def  *def,
                                     const tad_bps_pkt_frag_data *pkt);

/**
 * Calculate length of the binary packet fragment in bits using
 * fragment specification only.
 *
 * @param descr         Binary packet fragment description
 * @param fields        Number of fields in description
 *
 * @return Length in bits or 0, if it is unknown.
 */
extern size_t tad_bps_pkt_frag_bitlen(const tad_bps_pkt_frag *descr,
                                      unsigned int fields);

/**
 * Calculate length of the binary packet fragment in bits using
 * fragment specification or current values in data units.
 *
 * @param def           Binary packet fragment definition filled in by
 *                      tad_bps_pkt_frag_init() function
 * @param pkt           Binary packet fragment data allocated and
 *                      filled in by tad_bps_nds_to_data_units()
 *
 * @return Length in bits or 0, if it is unknown.
 */
extern size_t tad_bps_pkt_frag_data_bitlen(
                  const tad_bps_pkt_frag_def  *def,
                  const tad_bps_pkt_frag_data *pkt);

extern te_errno tad_bps_pkt_frag_gen_bin(
                    const tad_bps_pkt_frag_def  *def,
                    const tad_bps_pkt_frag_data *pkt,
                    const tad_tmpl_arg_t        *args,
                    size_t                       arg_num,
                    uint8_t                     *bin,
                    unsigned int                *bitoff,
                    unsigned int                 max_bitlen);

#endif /* !__TE_TAD_BPS_H__ */
