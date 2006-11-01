/** @file
 * @brief Tester API for NDN
 *
 * Implementation of Tester API for NDN.
 *
 * Copyright (C) 2006 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
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
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
 */ 

#define TE_LGR_USER     "TAPI NDN"

#include "te_config.h"

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_STRING_H 
#include <string.h>
#endif
#if HAVE_STRINGS_H
#include <strings.h>
#endif

#include "te_stdint.h"
#include "te_errno.h"
#include "logger_api.h"
#include "ndn.h"

#include "tapi_ndn.h"


/* See the description in tapi_ndn.h */
te_errno
tapi_tad_init_asn_value(asn_value **value, const asn_type *type)
{
    if (value == NULL)
    {
        ERROR("Location of ASN.1 value with CSAP specification have "
              "to be provided");
        return TE_RC(TE_TAPI, TE_EINVAL);
    }
    if (*value == NULL)
    {
        *value = asn_init_value(type);
        if (*value == NULL)
        {
            ERROR("Failed to initialize ASN.1 value for CSAP "
                  "specification");
            return TE_RC(TE_TAPI, TE_ENOMEM);
        }
    }
    return 0;
}

/* See the description in tapi_ndn.h */
te_errno
tapi_tad_csap_add_layer(asn_value       **csap_spec,
                        const asn_type   *layer_type,
                        const char       *layer_choice,
                        asn_value       **layer_spec)
{
    te_errno    rc;
    asn_value  *layers;
    asn_value  *gen_layer;
    asn_value  *layer;

    if (layer_type == NULL || layer_choice == NULL)
    {
        ERROR("%s(): ASN.1 type of the layer have to be specified",
              __FUNCTION__);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    rc = tapi_tad_init_asn_value(csap_spec, ndn_csap_spec);
    if (rc != 0)
        return rc;

    /*
     * Get or create CSAP layers sequence
     */
    /* FIXME: Remove type cast */
    rc = asn_get_child_value(*csap_spec, (const asn_value **)&layers,
                             PRIVATE, NDN_CSAP_LAYERS);
    if (rc == TE_EASNINCOMPLVAL)
    {
        layers = asn_init_value(ndn_csap_layers);
        if (layers == NULL)
        {
            ERROR("Failed to initiaze ASN.1 value for CSAP layers "
                  "sequence");
            return TE_RC(TE_TAPI, TE_ENOMEM);
        }
        rc = asn_put_child_value(*csap_spec, layers,
                                 PRIVATE, NDN_CSAP_LAYERS);
        if (rc != 0)
        {
            ERROR("Failed to put 'layers' in ASN.1 value: %r", rc);
            asn_free_value(layers);
            return rc;
        }
    }
    else if (rc != 0)
    {
        ERROR("Failed to get 'layers' from ASN.1 value: %r", rc);
        return TE_RC(TE_TAPI, rc);
    }

    gen_layer = asn_init_value(ndn_generic_csap_layer);
    if (gen_layer == NULL)
    {
        ERROR("Failed to initialize ASN.1 value for CSAP specification "
              "generic layer");
        return TE_RC(TE_TAPI, TE_ENOMEM);
    }
    rc = asn_insert_indexed(layers, gen_layer, -1, "");
    if (rc != 0)
    {
        ERROR("Failed to add a new generic layer in CSAP specification: "
              "%r", rc);
        asn_free_value(gen_layer);
        return TE_RC(TE_TAPI, rc);
    }
    
    layer = asn_init_value(layer_type);
    if (layer == NULL)
    {
        ERROR("Failed to initialize ASN.1 value for CSAP specification "
              "layer by type");
        return TE_RC(TE_TAPI, TE_ENOMEM);
    }
    rc = asn_put_child_value_by_label(gen_layer, layer, layer_choice);
    if (rc != 0)
    {
        ERROR("Failed to put layer as choice of generic CSAP "
              "specification layer: %r", rc);
        asn_free_value(layer);
        return rc;
    }

    if (layer_spec != NULL)
        *layer_spec = layer;

    return 0;
}

/* See the description in tapi_ndn.h */
te_errno
tapi_tad_new_ptrn_unit(asn_value **obj_spec, asn_value **unit_spec)
{
    te_errno    rc;
    asn_value  *unit;
    asn_value  *pdus;

    rc = tapi_tad_init_asn_value(obj_spec, ndn_traffic_pattern);
    if (rc != 0)
        return rc;

    unit = asn_init_value(ndn_traffic_pattern_unit);
    if (unit == NULL)
    {
        ERROR("Failed to initialize traffic pattern unit");
        return TE_RC(TE_TAPI, TE_ENOMEM);
    }

    rc = asn_insert_indexed(*obj_spec, unit, 0, "");
    if (rc != 0)
    {
        ERROR("Failed to add a new unit in traffic pattern: "
              "%r", rc);
        asn_free_value(unit);
        return TE_RC(TE_TAPI, rc);
    }

    pdus = asn_init_value(ndn_generic_pdu_sequence);
    if (pdus == NULL)
    {
        ERROR("Failed to initiaze ASN.1 value for generic PDUs "
              "sequence");
        return TE_RC(TE_TAPI, TE_ENOMEM);
    }
    rc = asn_put_child_value_by_label(unit, pdus, "pdus");
    if (rc != 0)
    {
        ERROR("Failed to put 'pdus' in ASN.1 value: %r", rc);
        asn_free_value(pdus);
        return rc;
    }

    if (unit_spec != NULL)
        *unit_spec = unit;

    return 0;
}


/* See the description in tapi_ndn.h */
static te_errno
tapi_tad_tmpl_ptrn_get_unit(asn_value **obj_spec,
                            te_bool     is_pattern,
                            asn_value **unit_spec)
{
    te_errno    rc;

    assert(unit_spec != NULL);

    /*
     * Check the root object and initialize it, if it is necessary.
     */
    rc = tapi_tad_init_asn_value(obj_spec, (is_pattern) ?
                                               ndn_traffic_pattern :
                                               ndn_traffic_template);
    if (rc != 0)
        return rc;

    /*
     * Get traffic template/pattern unit or create a new
     */
    if (is_pattern)
    {
        int len = asn_get_length(*obj_spec, "");

        if (len < 0)
        {
            ERROR("%s(): asn_get_length() failed unexpectedly: %r",
                  __FUNCTION__, rc);
            return TE_RC(TE_TAPI, TE_EINVAL);
        }

        if (len == 0)
        {
            rc = tapi_tad_new_ptrn_unit(obj_spec, unit_spec);
            if (rc != 0)
                return TE_RC(TE_TAPI, rc);
        }
        else
        {
            rc = asn_get_indexed(*obj_spec, unit_spec, len - 1, NULL);
            if (rc != 0)
            {
                ERROR("Failed to get ASN.1 value by index %d: %r",
                      len - 1, rc);
                return TE_RC(TE_TAPI, rc);
            }
        }
    }
    else
    {
        *unit_spec = *obj_spec;
    }

    return 0;
}

/* See the description in tapi_ndn.h */
te_errno
tapi_tad_tmpl_ptrn_add_layer(asn_value       **obj_spec,
                             te_bool           is_pattern,
                             const asn_type   *pdu_type,
                             const char       *pdu_choice,
                             asn_value       **pdu_spec)
{
    te_errno    rc;
    asn_value  *unit_spec;
    asn_value  *pdus;
    asn_value  *gen_pdu;
    asn_value  *pdu;

    if (pdu_type == NULL || pdu_choice == NULL)
    {
        ERROR("%s(): ASN.1 type of the layer have to be specified",
              __FUNCTION__);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    /*
     * Check the root object and initialize it, if it is necessary.
     */
    rc = tapi_tad_tmpl_ptrn_get_unit(obj_spec, is_pattern, &unit_spec);
    if (rc != 0)
        return rc;

    /*
     * Get or create PDUs sequence
     */
    /* FIXME: Remove type cast */
    rc = asn_get_child_value(unit_spec, (const asn_value **)&pdus,
                             PRIVATE, (is_pattern) ? NDN_PU_PDUS :
                                                     NDN_TMPL_PDUS);
    if (rc == TE_EASNINCOMPLVAL)
    {
        pdus = asn_init_value(ndn_generic_pdu_sequence);
        if (pdus == NULL)
        {
            ERROR("Failed to initiaze ASN.1 value for generic PDUs "
                  "sequence");
            return TE_RC(TE_TAPI, TE_ENOMEM);
        }
        rc = asn_put_child_value_by_label(unit_spec, pdus, "pdus");
        if (rc != 0)
        {
            ERROR("Failed to put 'pdus' in ASN.1 value: %r", rc);
            asn_free_value(pdus);
            return rc;
        }
    }
    else if (rc != 0)
    {
        ERROR("Failed to get 'pdus' from ASN.1 value: %r", rc);
        return TE_RC(TE_TAPI, rc);
    }

    /*
     * Create a new generic PDU and insert it in PDUs sequence as the
     * last
     */
    gen_pdu = asn_init_value(ndn_generic_pdu);
    if (gen_pdu == NULL)
    {
        ERROR("Failed to initialize ASN.1 value for generic PDU");
        return TE_RC(TE_TAPI, TE_ENOMEM);
    }
    rc = asn_insert_indexed(pdus, gen_pdu, -1, "");
    if (rc != 0)
    {
        ERROR("Failed to add a new generic PDU in sequence: %r", rc);
        asn_free_value(gen_pdu);
        return TE_RC(TE_TAPI, rc);
    }
    
    pdu = asn_init_value(pdu_type);
    if (pdu == NULL)
    {
        ERROR("Failed to initialize ASN.1 value for PDU by type");
        return TE_RC(TE_TAPI, TE_ENOMEM);
    }

    rc = asn_put_child_value_by_label(gen_pdu, pdu, pdu_choice);
    if (rc != 0)
    {
        ERROR("Failed to put PDU as choice of generic PDU: %r", rc);
        asn_free_value(pdu);
        return rc;
    }

    if (pdu_spec != NULL)
        *pdu_spec = pdu;

    return 0;
}

/* See the description in tapi_ndn.h */
te_errno
tapi_tad_tmpl_ptrn_add_payload_plain(asn_value  **obj_spec,
                                     te_bool      is_pattern,
                                     const void  *payload,
                                     size_t       length)
{
    te_errno    rc;
    asn_value  *unit_spec;

    rc = tapi_tad_tmpl_ptrn_get_unit(obj_spec, is_pattern, &unit_spec);
    if (rc != 0)
        return rc;

    if (payload == NULL && length != 0)
        rc = asn_write_int32(unit_spec, length, "payload.#length");
    else
        rc = asn_write_value_field(unit_spec, payload, length,
                                   "payload.#bytes");
    if (rc != 0)
    {
        ERROR("Addition of payload failed: %r", rc);
    }

    return TE_RC(TE_TAPI, rc);
}
