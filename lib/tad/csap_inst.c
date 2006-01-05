/** @file
 * @brief TAD CSAP Instance
 *
 * Traffic Application Domain Command Handler.
 * Implementation of CSAP instance methods.
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
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 * @author Konstantin Abramenko <Konstantin.Abramenko@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER     "TAD CSAP instance"

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>

#include "te_errno.h"
#include "logger_api.h"
#include "logger_ta_fast.h"

#include "csap_id.h"
#include "tad_csap_support.h"
#include "tad_csap_inst.h"
#include "tad_utils.h"


/** Max number of CSAP layers */
#define MAX_CSAP_DEPTH 200

/** Pseudo-protocol label for 'data' CSAPs */
#define CSAP_DATA_PROTO "data"


/** 
 * Free memory allocatad for all common CSAP data.
 *
 * @param csap      Pointer to CSAP descriptor
 */
static void
csap_free(csap_p csap)
{
    if (csap == NULL)
        return;

    VERB("%s(): csap %d, layers %u", __FUNCTION__,
         csap->id, csap->layers);
    
    free(csap->csap_type); 

    if (csap->layers != NULL)
    {
        unsigned int i;

        for (i = 0; i < csap->depth; i++)
        {
            free(csap->layers[i].specific_data);
            asn_free_value(csap->layers[i].csap_layer_pdu);
        }

        free(csap->layers); 
    }

    free(csap);
}


/* See description in tad_csap_inst.h */
csap_handle_t
csap_create(const char *type)
{
    int   i; 
    int   rc;
    int   depth; 
    char *csap_type = strdup(type);
    char *proto;

    csap_p          new_csap = calloc(1, sizeof(csap_instance));

    char *layer_protos[MAX_CSAP_DEPTH];


    ENTRY("%s", type);

    if (new_csap != NULL)
        new_csap->csap_type = csap_type;

/**
 * Macro for failure processing in csap_create function.
 *
 * @param errno_        error status code
 * @param fmt_          format string with possible firther arguments
 *                      for error log message
 */
#define CSAP_CREATE_ERROR(errno_, fmt_...) \
    do {               \
        rc = (errno_); \
        ERROR(fmt_); \
        goto error;    \
    } while (0)

    if ((csap_type == NULL) || (new_csap == NULL))
        CSAP_CREATE_ERROR(TE_ENOMEM, "%s(): no memory for new CSAP", 
                          __FUNCTION__);

    new_csap->id = csap_id_new(new_csap);
    if (new_csap->id == CSAP_INVALID_HANDLE)
        CSAP_CREATE_ERROR(TE_ENOMEM, "Failed to allocate a new CSAP ID");
    VERB("%s(): new id: %u", __FUNCTION__, new_csap->id);

    if ((rc = pthread_mutex_init(&new_csap->data_access_lock, NULL)) != 0)
        CSAP_CREATE_ERROR(rc, "%s(): mutex init fails: %d",
                          __FUNCTION__, rc); 
    depth = 0; 

    if (strncmp(csap_type, CSAP_DATA_PROTO, strlen(CSAP_DATA_PROTO)) != 0)
    {
        layer_protos[depth] = csap_type;
        depth++; 
        new_csap->type = TAD_CSAP_RAW;
    }
    else
        new_csap->type = TAD_CSAP_DATA;

    for (i = 0, proto = strchr(csap_type, '.');
         proto != NULL && *proto != '\0';
         proto = strchr(proto, '.')) 
    {
        *proto = '\0';
        proto++;
        layer_protos[depth] = proto;
        depth++; 
        VERB("%s(): next_layer: %s\n", __FUNCTION__, proto);
    }

    new_csap->depth = depth;

    /* Allocate memory for stack arrays */ 
    new_csap->layers = calloc(depth, sizeof(new_csap->layers[0]));

    if (new_csap->layers == NULL)
        CSAP_CREATE_ERROR(TE_ENOMEM, "%s(): no memory for layers", 
                          __FUNCTION__);

    for (i = 0; i < depth; i++)
    {
        new_csap->layers[i].proto = layer_protos[i];
        new_csap->layers[i].proto_tag = te_proto_from_str(layer_protos[i]);
        new_csap->layers[i].proto_support =
            csap_spt_find(new_csap->layers[i].proto);

        VERB("%s(): layer %d: %s\n", __FUNCTION__,
             i, new_csap->layers[i].proto);

        if (new_csap->layers[i].proto_support == NULL)
            CSAP_CREATE_ERROR(TE_EOPNOTSUPP, 
                              "%s(): no support for proto '%s'", 
                              __FUNCTION__, new_csap->layers[i].proto);
    } 

#undef CSAP_CREATE_ERROR

    EXIT("ID=%u", new_csap->id);
    return new_csap->id;

error:
    if (new_csap != NULL && new_csap->id != CSAP_INVALID_HANDLE)
        csap_id_delete(new_csap->id);
    csap_free(new_csap);

    EXIT("ERROR %r", rc);
    errno = rc;
    return CSAP_INVALID_HANDLE;
}

/* See description in tad_csap_inst.h */
te_errno
csap_destroy(csap_handle_t csap_id)
{
    csap_p  csap = csap_id_delete(csap_id);

    VERB("%s(): CSAP ID %u -> %p", __FUNCTION__, csap_id, csap);

    if (csap == NULL)
        return TE_RC(TE_TAD_CH, TE_ENOENT);

    csap_free(csap);

    return 0;
}

/* See description in tad_csap_inst.h */
csap_p
csap_find(csap_handle_t csap_id)
{
    return csap_id_get(csap_id);
}
