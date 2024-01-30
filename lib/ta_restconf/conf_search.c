/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2024 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief RESTCONF agent library
 *
 * Implementation of /config/search configuration tree.
 */

#define TE_LGR_USER "TA RESTCONF Conf Search"

#include "te_config.h"

#include "te_alloc.h"
#include "te_str.h"
#include "te_string.h"
#include "te_queue.h"
#include "tq_string.h"
#include "rcf_pch.h"
#include "ta_restconf.h"
#include "ta_restconf_internal.h"


/* Check search instance for validity */
#define SEARCH_CHECK(instance) \
    do {                                        \
        if (instance == NULL)                   \
            return TE_RC(TE_TA, TE_ENOENT);     \
    } while (0)

/** Search instance. */
typedef struct search_inst {
    LIST_ENTRY(search_inst) links;
    /** Unique instance name (data-resource-identifier). */
    char *name;
    /** List of children (first level subtrees of data-resource-identifier). */
    tqh_strings children;
} search_inst;

/* Head of search instances list. */
static LIST_HEAD(, search_inst) searches =
    LIST_HEAD_INITIALIZER(searches);


static search_inst *
search_inst_find(const char *name)
{
    search_inst *inst;

    LIST_FOREACH(inst, &searches, links)
    {
        if (strcmp(inst->name, name) == 0)
            return inst;
    }

    return NULL;
}

static void
search_inst_free(search_inst *inst)
{
    tq_strings_free(&inst->children, free);

    free(inst->name);
    free(inst);
}

static te_errno
child_add(unsigned int gid, const char *oid, const char *value,
          const char *empty1, const char *empty2,
          const char *search, const char *name)
{
    search_inst *inst = search_inst_find(search);

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(empty1);
    UNUSED(empty2);

    SEARCH_CHECK(inst);

    if (tq_strings_add_uniq_dup(&inst->children, name) != 0)
    {
        ERROR("Instance with such name already exists: '%s'", name);
        return TE_RC(TE_TA, TE_EEXIST);
    }

    return 0;
}

static te_errno
child_del(unsigned int gid, const char *oid,
          const char *empty1, const char *empty2,
          const char *search, const char *name)
{
    search_inst *inst = search_inst_find(search);
    tqe_string *c;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(empty1);
    UNUSED(empty2);

    SEARCH_CHECK(inst);

    for (c = TAILQ_FIRST(&inst->children);
         c != NULL && strcmp(name, c->v) != 0;
         c = TAILQ_NEXT(c, links));

    if (c == NULL)
    {
        ERROR("Instance with such name doesn't exist: '%s'", name);
        return TE_RC(TE_TA, TE_ENOENT);
    }

    TAILQ_REMOVE(&inst->children, c, links);
    free(c->v);
    free(c);

    return 0;
}

static te_errno
child_list(unsigned int gid, const char *oid,
           const char *sub_id, char **list,
           const char *unused1, const char *unused2,
           const char *search)
{
    search_inst *inst = search_inst_find(search);
    tqe_string *c;
    te_string str = TE_STRING_INIT;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(sub_id);
    UNUSED(unused1);
    UNUSED(unused2);

    SEARCH_CHECK(inst);

    TAILQ_FOREACH(c, &inst->children, links)
    {
        te_string_append(&str, "%s%s",
                         (str.ptr != NULL) ? " " : "", c->v);
    }

    *list = str.ptr;
    return 0;
}

static te_errno
search_add(unsigned int gid, const char *oid, const char *value,
           const char *empty1, const char *empty2,
           const char *name)
{
    search_inst *inst;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(empty1);
    UNUSED(empty2);

    inst = search_inst_find(name);
    if (inst != NULL)
    {
        ERROR("Instance with such name already exists: '%s'", name);
        return TE_RC(TE_TA, TE_EEXIST);
    }

    inst = TE_ALLOC(sizeof(*inst));

    inst->name = TE_STRDUP(name);
    TAILQ_INIT(&inst->children);

    LIST_INSERT_HEAD(&searches, inst, links);
    return 0;
}

static te_errno
search_del(unsigned int gid, const char *oid,
           const char *empty1, const char *empty2,
           const char *name)
{
    search_inst *inst;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(empty1);
    UNUSED(empty2);

    inst = search_inst_find(name);
    if (inst == NULL)
    {
        ERROR("Instance with such name doesn't exist: '%s'", name);
        return TE_RC(TE_TA, TE_ENOENT);
    }

    LIST_REMOVE(inst, links);
    search_inst_free(inst);
    return 0;
}

static te_errno
search_list(unsigned int gid, const char *oid,
            const char *sub_id, char **list)
{
    search_inst *inst;
    te_string str = TE_STRING_INIT;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(sub_id);

    LIST_FOREACH(inst, &searches, links)
    {
        te_string_append(&str, "%s%s",
                         (str.ptr != NULL) ? " " : "", inst->name);
    }

    *list = str.ptr;
    return 0;
}

RCF_PCH_CFG_NODE_COLLECTION(node_child, "child",
                            NULL, NULL,
                            child_add, child_del,
                            child_list, NULL);

RCF_PCH_CFG_NODE_COLLECTION(node_search, "search",
                            &node_child, NULL,
                            search_add, search_del,
                            search_list, NULL);


te_errno
ta_restconf_conf_search_init(void)
{
    LIST_INIT(&searches);

    return rcf_pch_add_node("/agent/restconf/config", &node_search);
}
