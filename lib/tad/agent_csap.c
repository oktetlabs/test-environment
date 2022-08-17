/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief TAD /agent/csap
 *
 * Traffic Application Domain Command Handler.
 * Implementation of /agent/csap configuration tree.
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#define TE_LGR_USER     "TAD CSAPs"

#include "te_config.h"

#if defined(WITH_CS)

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_STDIO_H
#include <stdio.h>
#endif
#if HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include "te_defs.h"
#include "te_errno.h"
#include "logger_api.h"
#include "rcf_ch_api.h"
#include "rcf_pch.h"

#include "csap_id.h"
#include "tad_agent_csap.h"


/**
 * Initiate destruction of CSAP via Configurator.
 *
 * The function complies with rcf_ch_cfg_del prototype.
 */
static te_errno
agent_csap_del(unsigned int gid, const char *oid, const char *csap)
{
    char           *end;
    unsigned long   tmp;
    csap_handle_t   csap_id;

    UNUSED(gid);
    UNUSED(oid);

    assert(csap != NULL);
    tmp = strtoul(csap, &end, 10);
    if (end == csap || *end != '\0')
    {
        ERROR("Invalid string representation of CSAP ID '%s'", csap);
        return TE_RC(TE_TAD_CH, TE_EINVAL);
    }
    if ((csap_id = tmp) != tmp)
    {
        ERROR("Number %lu is too big to be CSAP ID", tmp);
        return TE_RC(TE_TAD_CH, TE_EDOM);
    }

    return tad_csap_destroy_by_id(csap_id);
}


/** Opaque data for agent_csap_list_cb() function. */
typedef struct agent_csap_list_cb_data {
    te_errno    rc;     /**< Status code */
    char       *list;   /**< Pointer to the list */
    size_t      len;    /**< Amount of memory allocated for the list */
    size_t      off;    /**< Used length in the list */
} agent_csap_list_cb_data;

/**
 * Callback function for CSAP enumeration routine.
 *
 * The function compilies with csap_id_enum_cb prototype.
 */
static void
agent_csap_list_cb(csap_handle_t csap_id, void *ptr, void *opaque)
{
    agent_csap_list_cb_data *data = opaque;
    te_bool                  again;
    int                      p;

    UNUSED(ptr);

    if (data->rc != 0)
        return;

    do {
        p = snprintf(data->list + data->off, data->len - data->off,
                     "%s%u", (data->off != 0) ? " " : "", csap_id) +
            1 /* \0 */;

        again = (p > (int)(data->len - data->off));
        if (again)
        {
            data->len = data->off +
                MAX((size_t)p, (data->len - data->off) + 16);
            data->list = realloc(data->list, data->len);
            if (data->list == NULL)
            {
                data->rc = TE_ENOMEM;
                return;
            }
        }
    } while (again);
    data->off += p - 1;
}

/**
 * Generate list of CSAPs open on the test agent.
 *
 * The function complies with rcf_ch_cfg_list prototype.
 */
static te_errno
agent_csap_list(unsigned int gid, const char *oid,
                const char *sub_id, char **list)
{
    agent_csap_list_cb_data data;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(sub_id);

    data.rc = 0;
    data.list = NULL;
    data.len = data.off = 0;
    csap_id_enum(agent_csap_list_cb, &data);

    if (data.rc == 0)
        *list = data.list;
    else
        free(data.list);

    return data.rc;
}


RCF_PCH_CFG_NODE_COLLECTION(agent_csap, "csap", NULL /* son */,
                            NULL /* brother */, NULL /* add */,
                            agent_csap_del, agent_csap_list,
                            NULL /* commit */);


/* See the description in tad_agent_csap.h */
te_errno
tad_agent_csap_init(void)
{
    return rcf_pch_add_node("/agent", &agent_csap);
}

/* See the description in tad_agent_csap.h */
te_errno
tad_agent_csap_fini(void)
{
    return rcf_pch_del_node(&agent_csap);
}

#endif /* WITH_CS */
