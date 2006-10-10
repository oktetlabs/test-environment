/** @file
 * @brief TAD /agent/csap
 *
 * Traffic Application Domain Command Handler.
 * Implementation of /agent/csap configuration tree.
 *
 * Copyright (C) 2006 Test Environment authors (see file AUTHORS
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
 * $Id: csap_id.c 24879 2006-03-06 14:00:33Z arybchik $
 */

#define TE_LGR_USER     "TAD CSAPs"

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

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

    return tad_csap_destroy(csap_id);
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

    UNUSED(ptr);

    if (data->rc != 0)
        return;

    do {
        int p = snprintf(data->list, data->len - data->off,
                         " %u", csap_id) + sizeof('\0');
        
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
}

/**
 * Generate list of CSAPs open on the test agent.
 *
 * The function complies with rcf_ch_cfg_list prototype.
 */
static te_errno
agent_csap_list(unsigned int gid, const char *oid, char **list)
{
    agent_csap_list_cb_data data;

    UNUSED(gid);
    UNUSED(oid);

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
