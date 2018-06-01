/** @file
 * @brief TAD CSAP Support Database
 *
 * Traffic Application Domain Command Handler.
 * Implementation of CSAP support DB methods.
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
 *
 * @author Konstantin Abramenko <Konstantin.Abramenko@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER     "CSAP support"

#include "te_config.h"

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#endif
#if HAVE_ASSERT_H
#include <assert.h>
#endif

#include "te_errno.h"
#include "te_queue.h"
#include "logger_api.h"

#include "tad_csap_support.h"


/**
 * CSAP protocol layer support DB entry
 */
typedef struct csap_spt_entry {
    STAILQ_ENTRY(csap_spt_entry) links;  /**< List links */

    csap_spt_type_p   spt_data; /**< Pointer to support descriptor */

} csap_spt_entry;


/** Head of the CSAP protocol support list */
static STAILQ_HEAD(, csap_spt_entry) csap_spt_root;


/* See the description in tad_csap_support.h */
te_errno
csap_spt_init(void)
{
    STAILQ_INIT(&csap_spt_root);
    return 0;
}

/**
 * Add structure for CSAP support for respective protocol.
 *
 * @param spt_descr     CSAP layer support structure.
 *
 * @return Zero on success, otherwise error code.
 *
 * @todo check for uniqueness of protocol labels.
 */
te_errno
csap_spt_add(csap_spt_type_p spt_descr)
{
    csap_spt_entry   *new_spt_entry;

    if (spt_descr == NULL)
        return TE_EINVAL;

    new_spt_entry = malloc(sizeof(*new_spt_entry));
    if (new_spt_entry == NULL)
        return TE_ENOMEM;

    new_spt_entry->spt_data = spt_descr;
    STAILQ_INSERT_TAIL(&csap_spt_root, new_spt_entry, links);

    INFO("Registered '%s' protocol support", spt_descr->proto);

    return 0;
}

/* See the description in tad_csap_support.h */
csap_spt_type_p
csap_spt_find(const char *proto)
{
    csap_spt_entry   *spt_entry;

    VERB("%s(): asked proto %s", __FUNCTION__, proto);

    STAILQ_FOREACH(spt_entry, &csap_spt_root, links)
    {
        assert(spt_entry->spt_data != NULL);
        VERB("%s(): test proto %s", __FUNCTION__,
             spt_entry->spt_data->proto);

        if (strcmp(spt_entry->spt_data->proto, proto) == 0)
            return spt_entry->spt_data;
    }
    return NULL;
}

/* See the description in tad_csap_support.h */
void
csap_spt_destroy(void)
{
    csap_spt_entry   *entry;

    while ((entry = STAILQ_FIRST(&csap_spt_root)) != NULL)
    {
        STAILQ_REMOVE(&csap_spt_root, entry, csap_spt_entry, links);

        assert(entry->spt_data != NULL);
        if (entry->spt_data->unregister_cb != NULL)
            entry->spt_data->unregister_cb();

        free(entry);
    }
}
