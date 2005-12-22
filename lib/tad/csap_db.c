/** @file
 * @brief TAD Command Handler
 *
 * Traffic Application Domain Command Handler
 * Implementation of CSAP dynamic DB methods 
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
 * @author Konstantin Abramenko <Konstantin.Abramenko@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER     "TAD CSAP DB"

#include "te_config.h"
#include <stdlib.h>
#include <string.h>

#include "te_errno.h"
#include "tad_utils.h"
#include "logger_api.h"
#include "tad_csap_inst.h"
#include "tad_csap_support.h"


/*
 * Macros definitions 
 */

/** Max number of CSAP layers */
#define MAX_CSAP_DEPTH 200

/** Compilation flag, if true - start CSAP ids from 1 */
#define SIMPLE_CSAP_IDS 1

/** Pseudo-protocol label for 'data' CSAPs */
#define CSAP_DATA_PROTO "data"


/** 
 * Free all memory allocatad for all common CSAP data.
 *
 * @param csap_descr    pointer to CSAP descriptor.
 *
 * @return nothing
 */
static void csap_free(csap_p csap_descr);

struct csap_db_entry;
typedef struct csap_db_entry *csap_db_entry_p;

/**
 * CSAP database entry structure, double linked list. 
 */
typedef struct csap_db_entry {
    csap_db_entry_p next; /**< next entry in list */
    csap_db_entry_p prev; /**< previous entry in list */
    csap_p          inst;  /**< reference to the CSAP instance */
} csap_db_entry;

static csap_db_entry root_csap_db = {&root_csap_db, &root_csap_db, NULL};
static int start_position;

extern char *ta_name;

/**
 * Initialize CSAP database.
 *
 * @return zero on success, otherwise error code 
 */ 
int 
csap_db_init()
{
#if SIMPLE_CSAP_IDS
    start_position = 1;
#else
    /* 
     * Sometimes there was necessity to 'almost unique' CSAP ids on
     * all TA 
     */
    int seed = 0;
    int i;

    for (i = 0; i < strlen(ta_name); i++)
        seed += i * ta_name[i];
    srandom(seed);
    start_position = random();
    INFO("Init with seed %d, start_position %d", 
         seed, start_position);
#endif
    return 0;
}

/**
 * Clear CSAP database.
 *
 * @return zero on success, otherwise error code 
 */ 
int 
csap_db_clear()
{
    /* 
     * Unsupported yet. 
     * There is no RCF command to remove all CSAPs, and there are no
     * any other situation, when such operation may be reasonable.
     */
    return TE_EOPNOTSUPP; 
}

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
/**
 * Create new CSAP. 
 * This method does not perform any actions related to CSAP functionality,
 * neither processing of CSAP init parameters, nor initialyzing some 
 * communication media units (for example, sockets, etc.).
 * It only allocates memory for csap_instance sturture, set fields
 * 'id', 'depth' and 'proto' in it and allocates memory for 'layer_data'.
 * 
 * @param type  Type of CSAP: dot-separated sequence of textual 
 *              layer labels.
 *
 * @return identifier of new CSAP or zero if error occured.
 */ 
int
csap_create(const char *type)
{
    int   i; 
    int   rc;
    int   depth; 
    int   next_id;
    char *csap_type = strdup(type);
    char *proto;

    csap_db_entry_p dp;
    csap_db_entry_p new_dp = calloc(1, sizeof(csap_db_entry)); 
    csap_p          new_csap = calloc(1, sizeof(csap_instance));

    char *layer_protos[MAX_CSAP_DEPTH];


    ENTRY("%s", type);

    if (new_csap != NULL)
        new_csap->csap_type = csap_type;

    if ((csap_type == NULL) || (new_csap == NULL) || (new_dp == NULL))
        CSAP_CREATE_ERROR(TE_ENOMEM, "%s(): no memory for new CSAP", 
                          __FUNCTION__);

    new_dp->inst = new_csap;

    /* Find free identifier, set it to new CSAP and insert structure. */

    for (dp = root_csap_db.next, next_id = start_position; 
         dp != &root_csap_db && next_id >= dp->inst->id; 
         dp = dp->next, next_id++);

    VERB("%s(): new id: %d\n", __FUNCTION__, next_id);
    new_csap->id = next_id;
    INSQUE(new_dp, dp->prev);

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
            find_csap_spt(new_csap->layers[i].proto);

        VERB("%s(): layer %d: %s\n", __FUNCTION__,
             i, new_csap->layers[i].proto);

        if (new_csap->layers[i].proto_support == NULL)
            CSAP_CREATE_ERROR(TE_EOPNOTSUPP, 
                              "%s(): no support for proto '%s'", 
                              __FUNCTION__, new_csap->layers[i].proto);
    } 

    EXIT("ID=%u", new_csap->id);
    return new_csap->id;

error:
    if (new_dp != NULL && new_dp->next != NULL)
        REMQUE(new_dp);
    free(new_dp);
    csap_free(new_csap);

    EXIT("ERROR %r", rc);
    errno = rc;
    return CSAP_INVALID_HANDLE;
}


#undef CSAP_CREATE_ERROR

/**
 * Find CSAP DB entry by CSAP identifier.
 *
 * @param csap_id       Identifier of CSAP 
 *
 * @return 
 *    Pointer to structure with CSAP DB entry or NULL if not found. 
 */ 
static csap_db_entry_p
csap_db_entry_find(int id)
{
    csap_db_entry_p dp = root_csap_db.next;

    while ((dp != &root_csap_db) && (dp->inst->id < id)) 
        dp = dp->next;

    return (dp->inst != NULL && dp->inst->id == id) ? dp : NULL;
}


/** 
 * Free all memory allocatad for all common CSAP data.
 *
 * @param csap_descr    pointer to CSAP descriptor.
 *
 * @return nothing
 */
static void
csap_free(csap_p csap_descr)
{
    if (csap_descr == NULL)
        return;

    VERB("%s(): csap %d, layers %x", __FUNCTION__,
         csap_descr->id, csap_descr->layers);
    
    free(csap_descr->csap_type); 

    if (csap_descr->layers != NULL)
    {
        unsigned int i;

        for (i = 0; i < csap_descr->depth; i++)
        {
            free(csap_descr->layers[i].specific_data);
            asn_free_value(csap_descr->layers[i].csap_layer_pdu);
        }

        free(csap_descr->layers); 
    }

    free(csap_descr);
}

/**
 * Destroy CSAP.
 * Before call this DB method, all protocol-specific data in 'layer-data'
 * and underground media resources should be freed. 
 * This method will free all non-NULL pointers in 'layer-data', but 
 * it does not know anything about what structures are pointed by them, 
 * therefore if there are some more pointers in that structures, 
 * memory may be lost. 
 *
 * @param csap_id       Identifier of CSAP to be destroyed.
 *
 * @return zero on success, otherwise error code 
 */ 
int
csap_destroy(int csap_id)
{
    csap_db_entry_p dp = csap_db_entry_find(csap_id); 

    VERB("%s(): csap %d", __FUNCTION__, csap_id);

    if (dp == NULL)
        return TE_ENOENT;


    REMQUE(dp);

    csap_free(dp->inst);
    free(dp);

    return 0;
}

/**
 * Find CSAP by its identifier.
 *
 * @param csap_id       Identifier of CSAP 
 *
 * @return      Pointer to structure with internal CSAP information 
 *              or NULL if not found. 
 *
 * Change data in this structure if you really know what does it mean!
 */ 
csap_p
csap_find(int csap_id)
{
    csap_db_entry_p dp = csap_db_entry_find(csap_id);

    if (dp == NULL) 
        return NULL;

    return dp->inst;
}



