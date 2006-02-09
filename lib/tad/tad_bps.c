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
void
tad_bps_pkt_frag_free(tad_bps_pkt_frag_def *bps)
{
    unsigned int    i;

    if (bps == NULL)
        return;

    for (i = 0; i < bps->fields; ++i)
    {
        tad_data_unit_clear(bps->tx_def + i);
        tad_data_unit_clear(bps->rx_def + i);
    }
    free(bps->tx_def);
    free(bps->rx_def);
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
void
tad_bps_free_pkt_frag_data(const tad_bps_pkt_frag_def *def,
                           tad_bps_pkt_frag_data      *data)
{
    tad_data_unit_t    *dus;
    unsigned int        i;

    assert(def != NULL);
    assert(data != NULL);

    dus = data->dus;
    data->dus = NULL;
    for (i = 0; i < def->fields; ++i)
    {
        tad_data_unit_clear(dus + i);
    }
    free(dus);
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



/* See description in tad_bps.h */
te_errno
tad_bps_pkt_frag_match_pre(const tad_bps_pkt_frag_def *def,
                           tad_bps_pkt_frag_data *pkt_data)
{
    unsigned int    i;

    if (def == NULL || pkt_data == NULL)
    {
        return TE_RC(TE_TAD_BPS, TE_EWRONGPTR);
    }

    pkt_data->dus = calloc(def->fields, sizeof(*pkt_data->dus));
    if (pkt_data->dus == NULL)
        return TE_RC(TE_TAD_BPS, TE_ENOMEM);

    for (i = 0; i < def->fields; ++i)
    {
        pkt_data->dus[i].du_type = def->descr[i].plain_du;
        if (pkt_data->dus[i].du_type == TAD_DU_OCTS)
        {
            assert((def->descr[i].len & 7) == 0);
            pkt_data->dus[i].val_data.len = def->descr[i].len >> 3;
            pkt_data->dus[i].val_data.oct_str =
                calloc(1, def->descr[i].len);
            if (pkt_data->dus[i].val_data.oct_str == NULL)
                return TE_RC(TE_TAD_BPS, TE_ENOMEM);
        }
    }
    return 0;
}

/**
 * Read data starting from specified offset in bits and of specified
 * length in bits to prepared data unit.
 *
 * @param pkt           Packet
 * @param bitoff        Offset in bits
 * @param bitlen        Number of bits to read starting from offset
 * @param du            Destination data unit
 */
void
tad_bin_to_data_unit(const tad_pkt *pkt, unsigned int bitoff,
                     unsigned int bitlen, tad_data_unit_t *du)
{
    assert(du != NULL);

    ENTRY("pkt=%p bitoff=%u bitlen=%u du_type=%u",
          pkt, bitoff, bitlen, du->du_type);

    switch (du->du_type)
    {
        case TAD_DU_I32:
            assert(bitlen <= 32);
            du->val_i32 = 0; /* FIXME: May assert be used here */
            tad_pkt_read_bits(pkt, bitoff, bitlen,
                ((uint8_t *)(&du->val_i32)) + ((32 - bitlen) >> 3));
            du->val_i32 = ntohl(du->val_i32);
            break;

        case TAD_DU_I64:
            assert(bitlen <= 64);
            assert(du->val_i64 == 0);
            tad_pkt_read_bits(pkt, bitoff, bitlen,
                ((uint8_t *)(&du->val_i64)) + ((64 - bitlen) >> 3));
            du->val_i64 = tad_ntohll(du->val_i64);
            break;

        case TAD_DU_OCTS:
            assert(bitlen == (du->val_data.len << 3));
            tad_pkt_read_bits(pkt, bitoff, bitlen, du->val_data.oct_str);
            break;

        default:
            /* The rest is not supported/meaningless */
            assert(FALSE);
    }
    EXIT();
}

te_errno
tad_data_unit_match(const tad_data_unit_t *ptrn,
                    const tad_data_unit_t *value)
{
    if (ptrn->du_type != value->du_type)
    {
        ERROR("%s(): Matching of data units of different types is not "
              "yet supported", __FUNCTION__);
        return TE_EOPNOTSUPP;
    }

    switch (value->du_type)
    {
        case TAD_DU_I32:
            if (ptrn->val_i32 == value->val_i32)
                return 0;
            else
            {
                VERB("%s(): match failed %u vs %u", __FUNCTION__,
                     (unsigned)ptrn->val_i32, (unsigned)value->val_i32);
                return TE_ETADNOTMATCH;
            }
            break;

        case TAD_DU_I64:
            if (ptrn->val_i64 == value->val_i64)
                return 0;
            else
                return TE_ETADNOTMATCH;
            break;

        case TAD_DU_OCTS:
            if ((ptrn->val_data.len == value->val_data.len) &&
                (memcmp(ptrn->val_data.oct_str, value->val_data.oct_str,
                        ptrn->val_data.len) == 0))
                return 0;
            else
                return TE_ETADNOTMATCH;
            break;

        default:
            ERROR("%s(): Matching of data units of type %u is not yet "
                  "supported", __FUNCTION__, value->du_type);
            return EOPNOTSUPP;
    }
}

/* See description in tad_bps.h */
te_errno
tad_bps_pkt_frag_match_do(const tad_bps_pkt_frag_def *def,
                          const tad_bps_pkt_frag_data *ptrn,
                          tad_bps_pkt_frag_data *pkt_data,
                          const tad_pkt *pkt, unsigned int *bitoff)
{
    te_errno                rc = 0;
    unsigned int            i;
    const tad_data_unit_t  *du;
    size_t                  len;

    if (def == NULL || ptrn == NULL || pkt_data == NULL ||
        pkt == NULL || bitoff == NULL)
    {
        return TE_RC(TE_TAD_BPS, TE_EWRONGPTR);
    }

    for (i = 0; i < def->fields; ++i, *bitoff += len)
    {
        if (ptrn->dus[i].du_type != TAD_DU_UNDEF)
            du = ptrn->dus + i;
        else if (def->tx_def[i].du_type != TAD_DU_UNDEF)
            du = def->rx_def + i;
        else
            du = NULL;

        if (def->descr[i].len > 0)
            len = def->descr[i].len;
#if 0 /* FIXME: It is necessary to understand what does it mean for Rx */
        else if (du->du_type == TAD_DU_OCTS)
            len = du->val_data.len << 3;
#endif
        else
            assert(FALSE);

        if ((pkt_data->dus[i].du_type == TAD_DU_UNDEF) ||
            ((du == NULL) && (def->descr[i].tag_rx_def != ASN_TAG_USER)))
        {
            continue;
        }

        tad_bin_to_data_unit(pkt, *bitoff, len, pkt_data->dus + i);

        if (du != NULL)
        {
            rc = tad_data_unit_match(du, pkt_data->dus + i);
            if (rc != 0)
            {
                VERB("%s(): match failed for '%s': %r",
                     __FUNCTION__, def->descr[i].name, rc);
                break;
            }
        }
    }

    return rc;
}

/* See description in tad_bps.h */
te_errno
tad_data_unit_to_nds(asn_value *nds, const char *name,
                     tad_data_unit_t *du)
{
    te_errno    rc;
    char        tmp[32];

    snprintf(tmp, sizeof(tmp), "%s.#plain", name);

    switch (du->du_type)
    {
        case TAD_DU_I32:
            rc = asn_write_int32(nds, du->val_i32, tmp);
            break;
        case TAD_DU_I64:
            rc = TE_RC(TE_TAD_CH, TE_EOPNOTSUPP);
            break;
        case TAD_DU_OCTS:
            rc = asn_write_value_field(nds, du->val_data.oct_str,
                                       du->val_data.len, tmp);
            break;
        default:
            rc = TE_EFAULT;
    }

    return rc;
}

/* See description in tad_bps.h */
te_errno
tad_bps_pkt_frag_match_post(const tad_bps_pkt_frag_def *def,
                            tad_bps_pkt_frag_data *pkt_data,
                            const tad_pkt *pkt, unsigned int *bitoff,
                            asn_value *nds)
{
    te_errno                rc = 0;
    unsigned int            i;
    size_t                  len;

    if (nds == NULL)
        return 0;

    if (def == NULL || pkt_data == NULL || pkt == NULL || bitoff == NULL)
    {
        return TE_RC(TE_TAD_BPS, TE_EWRONGPTR);
    }

    for (i = 0; (rc == 0) && (i < def->fields); ++i, *bitoff += len)
    {
        if (def->descr[i].len > 0)
            len = def->descr[i].len;
        else
            assert(FALSE);

        /* FIXME: Optimize - do not read twice */
        tad_bin_to_data_unit(pkt, *bitoff, len, pkt_data->dus + i);

        rc = tad_data_unit_to_nds(nds, def->descr[i].name, pkt_data->dus + i);
    }

    return rc;
}
