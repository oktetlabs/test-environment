/** @file
 * @brief CSAP NDN.
 *
 * Implementation of functions for work with CSAP using NDN ASN.1 types
 *
 * Copyright (C) 2004-2018 OKTET Labs. All rights reserved.
 *
 *
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 * @author Denis Pryazhennikov <Denis.Pryazhennikov@oktetlabs.ru>
 */

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

#define TE_LGR_USER "NDN CSAP"

#include "te_errno.h"
#include "logger_api.h"
#include "ndn.h"
#include "asn_impl.h"



/* See the description in ndn.h */
te_errno
ndn_csap_add_layer(asn_value       **csap_spec,
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
        return TE_EINVAL;
    }

    rc = ndn_init_asn_value(csap_spec, ndn_csap_spec);
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
            return TE_ENOMEM;
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
        return rc;
    }

    gen_layer = asn_init_value(ndn_generic_csap_layer);
    if (gen_layer == NULL)
    {
        ERROR("Failed to initialize ASN.1 value for CSAP specification "
              "generic layer");
        return TE_ENOMEM;
    }
    rc = asn_insert_indexed(layers, gen_layer, -1, "");
    if (rc != 0)
    {
        ERROR("Failed to add a new generic layer in CSAP specification: "
              "%r", rc);
        asn_free_value(gen_layer);
        return rc;
    }

    layer = asn_init_value(layer_type);
    if (layer == NULL)
    {
        ERROR("Failed to initialize ASN.1 value for CSAP specification "
              "layer by type");
        return TE_ENOMEM;
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

asn_value *
ndn_csap_spec_by_traffic_template(const asn_value *tmpl)
{
    unsigned int    n_layers;
    unsigned int    i;
    asn_value      *csap_spec = NULL;
    const asn_type *csap_label_type = NULL;
    te_errno        rc;

    n_layers = (unsigned)asn_get_length(tmpl, "pdus");

    for (i = 0; i < n_layers; i++)
    {
        asn_value      *gen_layer = NULL;
        const char     *layer_choice = NULL;

        rc = asn_get_indexed(tmpl, &gen_layer, i, "pdus");
        if (rc != 0)
        {
            ERROR("Cannot get layer %d from PDU, rc %r", i, rc);
            return NULL;
        }

        layer_choice = asn_get_choice_ptr((const asn_value *)gen_layer);

        rc = asn_impl_find_subtype(ndn_generic_csap_layer, layer_choice,
                                   &csap_label_type);
        if (rc != 0)
        {
            ERROR("Subtype for label '%s' not found, %r", layer_choice, rc);
            return NULL;
        }

        rc = ndn_csap_add_layer(&csap_spec, csap_label_type, layer_choice,
                                NULL);
        if (rc != 0)
        {
            asn_free_value(csap_spec);
            return NULL;
        }
    }

    return csap_spec;
}
