/** @file
 * @brief ACSE CWMP Dispatcher
 *
 * ACS Emulator support
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 */

#define TE_LGR_USER     "ACSE internal DB"

#include "te_config.h"

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#include <stddef.h>
#include <string.h>

#include "acse_internal.h"

#include "te_defs.h"
#include "te_errno.h"
#include "te_queue.h"
#include "te_defs.h"
#include "logger_api.h"

/** The list af acs instances */
acs_list_t acs_list = LIST_HEAD_INITIALIZER(&acs_list);


/**
 * Add an ACS object to internal DB
 *
 * @param acs_name      Name of the ACS
 *
 * @return              status code
 */
te_errno
db_add_acs(const char *acs_name)
{
    acs_t *acs_item;

    if (acs_name == NULL || strlen(acs_name) == 0)
        return TE_RC(TE_ACSE, TE_EINVAL);

    acs_item = db_find_acs(acs_name);

    if (acs_item != NULL)
        return TE_RC(TE_ACSE, TE_EEXIST);

    if ((acs_item = malloc(sizeof(*acs_item))) == NULL)
        return TE_RC(TE_ACSE, TE_ENOMEM);

    memset(acs_item, 0, sizeof(*acs_item));
    acs_item->name = strdup(acs_name);
    acs_item->traffic_log = TRUE;

    LIST_INIT(&acs_item->cpe_list);
    LIST_INSERT_HEAD(&acs_list, acs_item, links);

    /* default AUTH mode for CWMP sessions from CPE */
    acs_item->auth_mode = ACSE_AUTH_DIGEST;

    return 0;
}


/**
 * Add a CPE record for particular ACS object to internal DB
 *
 * @param acs_name      Name of the ACS
 * @param cpe_name      Name of the CPE
 *
 * @return              status code
 */
te_errno
db_add_cpe(const char *acs_name, const char *cpe_name)
{
    acs_t *acs_item;
    cpe_t *cpe_item;

    if (acs_name == NULL || strlen(acs_name) == 0 ||
        cpe_name == NULL || strlen(cpe_name) == 0)
        return TE_RC(TE_ACSE, TE_EINVAL);

    if ((acs_item = db_find_acs(acs_name)) == NULL)
        return TE_RC(TE_ACSE, TE_ENOENT);

    if ((cpe_item = db_find_cpe(acs_item, cpe_name)) != NULL)
        return TE_RC(TE_ACSE, TE_EEXIST);

    if ((cpe_item = malloc(sizeof(*cpe_item))) == NULL)
        return TE_RC(TE_ACSE, TE_ENOMEM);

    memset(cpe_item, 0, sizeof(*cpe_item));
    cpe_item->name = strdup(cpe_name);
    cpe_item->acs = acs_item;
    cpe_item->enabled = TRUE;
    cpe_item->cr_state = CR_NONE;
    cpe_item->traffic_log = TRUE;

    TAILQ_INIT(&(cpe_item->rpc_queue));
    TAILQ_INIT(&(cpe_item->rpc_results));

    LIST_INIT(&cpe_item->inform_list);

    LIST_INSERT_HEAD(&(acs_item->cpe_list), cpe_item, links);


    return 0;
}

/* see description in acse_internal.h */
acs_t *
db_find_acs(const char *acs_name)
{
    acs_t *item;

    LIST_FOREACH(item, &acs_list, links)
    {
        if (strcmp(item->name, acs_name) == 0)
            return item;
    }

    return NULL;
}

/* see description in acse_internal.h */
cpe_t *
db_find_cpe(acs_t *acs_item, const char *cpe_name)
{
    cpe_t *cpe_item;

    if (acs_item != NULL)
    {
        LIST_FOREACH(cpe_item, &acs_item->cpe_list, links)
        {
            if (strcmp(cpe_item->name, cpe_name) == 0)
                return cpe_item;
        }
    }

    return NULL;
}

/* see description in acse_internal.h */
te_errno
db_remove_acs(acs_t *acs)
{
    if (acs == NULL)
        return TE_EINVAL;

    if (acs->conn_listen != NULL)
    {
        WARN("attempt to remove active ACS object '%s'", acs->name);
        return TE_EBUSY;
    }
    RING("db_remove_acs '%s', ptr %p, sess ptr %p",
         acs->name, acs, acs->session);
    if (acs->session != NULL)
    {
        if (acs->session->channel != NULL)
            acse_remove_channel(acs->session->channel);
        if (acs->session != NULL)
            cwmp_close_session(acs->session);
        acs->session = NULL;
    }
    while (! (LIST_EMPTY(&acs->cpe_list)))
    {
        cpe_t    *cpe_item = LIST_FIRST(&acs->cpe_list);
        te_errno  rc       = db_remove_cpe(cpe_item);
        if (rc != 0)
        {
            WARN("remove ACS failed because CPE remove failed %r", rc);
            return rc;
        }
    }
    LIST_REMOVE(acs, links);
    free(acs);
    return 0;
}

/* see description in acse_internal.h */
te_errno
db_clear_cpe(cpe_t *cpe)
{
    if (NULL == cpe)
        return TE_EINVAL;

    while (!(LIST_EMPTY(&cpe->inform_list)))
    {
        cpe_inform_t *inf_rec = LIST_FIRST(&cpe->inform_list);
        LIST_REMOVE(inf_rec, links);
        mheap_free_user(MHEAP_NONE, inf_rec);
    }

    while (!(TAILQ_EMPTY(&cpe->rpc_queue)))
    {
        cpe_rpc_item_t *rpc_item = TAILQ_FIRST(&cpe->rpc_queue);
        TAILQ_REMOVE(&cpe->rpc_queue, rpc_item, links);
        acse_rpc_item_free(rpc_item);
    }

    while (!(TAILQ_EMPTY(&cpe->rpc_results)))
    {
        cpe_rpc_item_t *rpc_item = TAILQ_FIRST(&cpe->rpc_results);
        TAILQ_REMOVE(&cpe->rpc_results, rpc_item, links);
        acse_rpc_item_free(rpc_item);
    }
    return 0;
}

/* see description in acse_internal.h */
te_errno
db_remove_cpe(cpe_t *cpe)
{
    if (NULL == cpe)
        return TE_EINVAL;

    if (NULL == cpe->acs)
    {
        ERROR("No acs ptr in CPE");
        return TE_EFAULT;
    }

    if (cpe->session != NULL || cpe->cr_state == CR_WAIT_AUTH)
    {
        WARN("attempt to remove busy CPE record '%s/%s', "
              "session %p, cr state %d",
             cpe->acs->name, cpe->name, cpe->session, (int)cpe->cr_state);
        return TE_EBUSY;
    }

    db_clear_cpe(cpe);

    LIST_REMOVE(cpe, links);
    free(cpe);
    return 0;
}


/* see description in acse_internal.h */
te_errno
acse_rpc_item_free(cpe_rpc_item_t *rpc_item)
{
    if (NULL == rpc_item)
        return 0; /* nothing to free */

    if (MHEAP_NONE == rpc_item->heap)
    {
        if (rpc_item->params->to_cpe.p != NULL)
            free(rpc_item->params->to_cpe.p);
    }
    else
        mheap_free_user(rpc_item->heap, rpc_item);

    free(rpc_item->params);
    free(rpc_item);
    return 0;
}


te_errno
db_clear(void)
{
    while (!LIST_EMPTY(&acs_list))
    {
        acs_t *acs = LIST_FIRST(&acs_list);
        acse_disable_acs(acs);
        LIST_REMOVE(acs, links);

        while (!LIST_EMPTY(&(acs->cpe_list)))
        {
            cpe_t *cpe = LIST_FIRST(&(acs->cpe_list));
            db_remove_cpe(cpe);
        }
        free(acs);
    }
    return 0;
}

