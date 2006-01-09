/** @file
 * @brief TAD Binary Protocol Support
 *
 * Traffic Application Domain Command Handler.
 * Traffic Application Domain Binary Protocol Support implementation.
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

#define TE_LGR_USER     "TAD BPS"

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#endif
#if HAVE_STRINGS_H
#include <strings.h>
#endif
#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#include "te_stdint.h"
#include "te_errno.h"
#include "logger_api.h"
#include "logger_ta_fast.h"

#include "tad_bps.h"


/**
 * @param dst       Destination
 * @param pos       Byte offset from destination
 * @param src       Data source
 * @param len       Length to be copied in bytes
 *
 */
void
tad_bps_write_byte_aligned(uint8_t *dst, unsigned int pos,
                           const uint8_t *src, unsigned int len)
{
    memcpy(dst + pos, src, len);
}

#if 0
tad_bsp_write_not_aligned(uint8_t *dst, unsigned int pos,
                          const uint8_t *src, unsigned int len)
{
    unsigned int    pos_shift = (pos & 7);
    unsigned int    len_shift = (len & 7);
    unsigned int    bytes = (len + 7) >> 3;
    uint16_t tmp;

    assert(len > 0);
    if (pos_shitf != 0)
    {
        
    }
    if (len_shift != 0)
    {
        /* Length in bits is not aligned to bytes. It is assumed that
         * we need to shift all bit */
        
    }
    if ((pos & 7) | (len & 7))
    {
    }
    else
    {
        /* Both initial position and length are byte aligned */
        tad_bps_write_byte_aligned(dst, pos >> 3, src, len >> 3);
    }
}
#endif

/* See description in tad_bps.h */
te_errno
tad_bps_pkt_frag_init(const tad_bps_pkt_frag *descr,
                      unsigned int            fields,
                      const asn_value        *layer_spec,
                      tad_bps_pkt_frag_def   *bps)
{
    te_errno        rc;
    unsigned int    i;
    unsigned int    offset;

    if (descr == NULL || bps == NULL)
    {
        ERROR("%s(): invalid arguments", __FUNCTION__);
        return TE_RC(TE_TAD_BPS, TE_EWRONGPTR);
    }

    bps->fields = fields;
    bps->descr = descr;

    bps->tx_def = calloc(fields, sizeof(bps->tx_def[0]));
    bps->rx_def = calloc(fields, sizeof(bps->rx_def[0]));
    if (bps->tx_def == NULL || bps->rx_def == NULL)
    {
        ERROR("%s(): calloc() failed", __FUNCTION__);
        return TE_RC(TE_TAD_BPS, TE_ENOMEM);
    }

    for (offset = 0, i = 0; i < fields;  offset += descr[i].len, ++i)
    {
        /* FIXME: init should be used here */
        tad_data_unit_clear(bps->tx_def + i);
        tad_data_unit_clear(bps->rx_def + i);

        /* FIXME: Is it necessary to check it */
        if (bps->descr[i].tag_tx_def == ASN_TAG_CONST)
        {
            bps->tx_def[i].du_type = TAD_DU_I32; 
            bps->tx_def[i].val_i32 = bps->descr[i].value;
        }
        else if (bps->descr[i].tag_tx_def != ASN_TAG_INVALID &&
                 bps->descr[i].tag_tx_def != ASN_TAG_USER)
        {
            rc = tad_data_unit_convert(layer_spec,
                                       bps->descr[i].tag_tx_def,
                                       bps->tx_def + i);
            if (rc != 0)
            {
                ERROR("%s(): tad_data_unit_convert failed for '%s' "
                      "send default", __FUNCTION__, bps->descr[i].name);
                return rc;
            }
        }
        /* FIXME: Is it necessary to check it */
        if (bps->descr[i].tag_rx_def == ASN_TAG_CONST)
        {
            bps->rx_def[i].du_type = TAD_DU_I32; 
            bps->rx_def[i].val_i32 = bps->descr[i].value;
        }
        else if (bps->descr[i].tag_rx_def != ASN_TAG_INVALID &&
                 bps->descr[i].tag_rx_def != ASN_TAG_USER)
        {
            rc = tad_data_unit_convert(layer_spec,
                                       bps->descr[i].tag_rx_def,
                                       bps->rx_def + i);
            if (rc != 0)
            {
                ERROR("%s(): tad_data_unit_convert failed for '%s' "
                      "receive default", __FUNCTION__, bps->descr[i].name);
                return rc;
            }
        }
    }

    return 0;
}

/* See description in tad_bps.h */
te_errno
tad_bps_nds_to_data_units(const tad_bps_pkt_frag_def *def,
                          const asn_value            *layer_pdu,
                          tad_bps_pkt_frag_data      *data)
{
    tad_data_unit_t        *dus;
    te_errno                rc;
    unsigned int            i;

    assert(def != NULL);
    assert(layer_pdu != NULL);
    assert(data != NULL);

    dus = calloc(def->fields, sizeof(dus[0]));
    if (dus == NULL)
    {
        ERROR("%s(): calloc() failed", __FUNCTION__);
        return TE_RC(TE_TAD_BPS, TE_ENOMEM);
    }

    for (i = 0; i < def->fields; ++i)
    {
        rc = tad_data_unit_convert(layer_pdu, def->descr[i].tag,
                                   dus + i);
        if (rc != 0)
        {
            ERROR("%s(): Failed to convert '%s' NDS to data unit: %r",
                  __FUNCTION__, def->descr[i].name, rc);
            while (i-- > 0)
                tad_data_unit_clear(dus + i);
            free(dus);
            return rc;
        }
    }

    data->dus = dus;

    return 0;
}

/* See description in tad_bps.h */
te_errno
tad_bps_confirm_send(const tad_bps_pkt_frag_def  *def,
                     const tad_bps_pkt_frag_data *pkt)
{
    unsigned int i;

    for (i = 0; i < def->fields; ++i)
    {
        if (pkt->dus[i].du_type == TAD_DU_UNDEF &&
            def->tx_def[i].du_type == TAD_DU_UNDEF &&
            def->descr[i].tag_tx_def != ASN_TAG_USER)
        {
            ERROR("Missing specification for field #%u '%s' to send",
                  i, def->descr[i].name);
            return TE_RC(TE_TAD_BPS, TE_ETADMISSNDS);
        }
    }
    return 0;
}


/* See description in tad_bps.h */
size_t
tad_bps_pkt_frag_bitlen(const tad_bps_pkt_frag *descr, unsigned int fields)
{
    unsigned int    i;
    size_t          len;

    for (len = 0, i = 0;
         i < fields && descr[i].len > 0;
         len += descr[i].len, ++i);

    return (i < fields) ? 0 : len;
}

/* See description in tad_bps.h */
size_t
tad_bps_pkt_frag_data_bitlen(const tad_bps_pkt_frag_def *def,
                             const tad_bps_pkt_frag_data *pkt)
{
    size_t  len;

    assert(def != NULL);
    
    len = tad_bps_pkt_frag_bitlen(def->descr, def->fields);
    if (len == 0 && pkt != NULL)
    {
        /* 
         * Length of the fragment is not fixed. It is assumed that it
         * must consist of set of character or octet strings.
         */
        unsigned int           i;
        const tad_data_unit_t *du;

        for (i = 0; i < def->fields; ++i)
        {
            if (def->descr[i].len > 0)
            {
                len += def->descr[i].len;
            }
            else if ((du = pkt->dus + i, du->du_type != TAD_DU_UNDEF) ||
                     (du = def->tx_def + i, du->du_type != TAD_DU_UNDEF))
            {
                if (du->du_type == TAD_DU_OCTS)
                {
                    len += du->val_data.len << 3;
                }
                else
                {
                    /* 
                     * We don't know the length for the rest types of
                     * data units.
                     */
                    len = 0;
                    break;
                }
            }
            else
            {
                /* We don't know the length of this field */
                len = 0;
                break;
            }
        }
    }
    return len;
}


typedef uint32_t myint_t;

/**
 * Write specified number of bits of an integer to memory array starting
 * from specified offset in bits.
 *
 * @param ptr           Destination memory array
 * @param off           Offset in destination in bits
 * @param value         Integer value
 * @param bits          Number of bits to write
 */
static void
write_bits(uint8_t *ptr, size_t off, myint_t value, size_t bits)
{
    size_t  off_byte = off >> 3;
    size_t  off_bit  = off & 7;
    size_t  space_bits = 8 - off_bit;
    uint8_t mask = (~0 << space_bits);
    int     left_bits = bits - space_bits;
    int     shift = (sizeof(myint_t) << 3) - off_bit - bits;
    myint_t tmp;
    uint8_t write;
    uint8_t read;

    if (left_bits < 0)
        mask = mask | ~(~0 << -left_bits);

    if (shift == 0)
        tmp = value;
    else if (shift > 0)
        tmp = value << shift;
    else
        tmp = value >> shift;
    tmp = htonl(tmp);
    write = *((uint8_t *)&tmp) & ~mask;
    read = ptr[off_byte] & mask;
    ptr[off_byte] = read | write;
    if (left_bits > 0)
        write_bits(ptr, off + space_bits, value, left_bits);
}

/* See description in tad_bps.h */
te_errno
tad_bps_pkt_frag_gen_bin(const tad_bps_pkt_frag_def *def,
                         const tad_bps_pkt_frag_data *pkt,
                         const tad_tmpl_arg_t *args, size_t arg_num,
                         uint8_t *bin, unsigned int *bitoff,
                         unsigned int max_bitlen)
{
    te_errno                rc;
    unsigned int            i;
    const tad_data_unit_t  *du;
    size_t                  len;

    if (def == NULL || bin == NULL)
    {
        return TE_RC(TE_TAD_BPS, TE_EWRONGPTR);
    }
    if ((*bitoff & 7) != 0 || (max_bitlen & 7) != 0)
    {
        ERROR("Not bit-aligned offsets and lengths are not supported");
        return TE_RC(TE_TAD_BPS, TE_EOPNOTSUPP);
    }

    for (i = 0; i < def->fields; ++i)
    {
        if (pkt->dus[i].du_type != TAD_DU_UNDEF)
            du = pkt->dus + i;
        else if (def->tx_def[i].du_type != TAD_DU_UNDEF)
            du = def->tx_def + i;
        else
            assert(FALSE);

        if (def->descr[i].len > 0)
            len = def->descr[i].len;
        else if (du->du_type == TAD_DU_OCTS)
            len = du->val_data.len << 3;
        else
            assert(FALSE);

        if (((*bitoff & 7) == 0) && ((len & 7) == 0))
        {
            rc = tad_data_unit_to_bin(du, args, arg_num,
                                      bin + (*bitoff >> 3), len >> 3);
            if (rc != 0)
            {
                ERROR("%s(): tad_data_unit_to_bin() failed: %r",
                      __FUNCTION__, rc);
                return rc;
            }
        }
        else if (du->du_type == TAD_DU_I32)
        {
            write_bits(bin, *bitoff, du->val_i32, len);
        }
        else
        {
            ERROR("Not bit-aligned offsets and lengths are supported "
                  "for plain integers only");
            return TE_RC(TE_TAD_BPS, TE_EOPNOTSUPP);
        }

        *bitoff += len;
    }
    return 0;
}
