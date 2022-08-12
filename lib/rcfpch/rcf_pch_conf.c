/** @file
 * @brief RCF Portable Command Handler
 *
 * Default configuration command handler implementation.
 *
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 * @author Elena A. Vengerova <Elena.Vengerova@oktetlabs.ru>
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 */

#include "te_config.h"

#include <stdio.h>
#ifdef STDC_HEADERS
#include <stdlib.h>
#include <string.h>
#endif
#if HAVE_STRINGS_H
#include <strings.h>
#endif
#if HAVE_ASSERT_H
#include <assert.h>
#endif
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#if HAVE_STDARG_H
#include <stdarg.h>
#endif
#if HAVE_SIGNAL_H
#include <signal.h>
#endif
#if HAVE_GLOB_H
#include <glob.h>
#endif

#include "rcf_pch_internal.h"

#include "te_errno.h"
#include "te_defs.h"
#include "te_stdint.h"
#include "te_queue.h"
#include "comm_agent.h"
#include "conf_oid.h"
#include "rcf_common.h"
#include "rcf_pch.h"
#include "rcf_ch_api.h"
#include "te_str.h"
#include "te_vector.h"
#include "te_alloc.h"
#include "te_sleep.h"
#include "te_string.h"
#include "cs_common.h"

#define OID_ETC "/..."

/**
 * Root of the Test Agent configuration tree. RCF PCH function is
 * used as list callback.
 *
 * @param _name  node name
 */
#define RCF_PCH_CFG_NODE_AGENT(_name) \
    static rcf_pch_cfg_object _name =                       \
        { "agent", 0, NULL, NULL, NULL, NULL, NULL, NULL,   \
           (rcf_ch_cfg_list)rcf_pch_agent_list,             \
           NULL, NULL, NULL }

/** Structure for temporary storing of instances/objects identifiers */
typedef struct olist {
    struct olist *next;             /**< Pointer to the next element */
    char          oid[CFG_OID_MAX]; /**< Element OID */
} olist;

/** Postponed configuration commit operation */
typedef struct rcf_pch_commit_op_t {
    TAILQ_ENTRY(rcf_pch_commit_op_t)    links;  /**< Tail queue links */
    cfg_oid                            *oid;    /**< OID */
    rcf_ch_cfg_commit                   func;   /**< Commit function */
} rcf_pch_commit_op_t;

/** Head of postponed commits */
static TAILQ_HEAD(rcf_pch_commit_head_t, rcf_pch_commit_op_t)   commits;

static te_bool      is_group = FALSE;       /**< Is group started? */
static unsigned int gid;                    /**< Group identifier */


/** Test Agent root node */
RCF_PCH_CFG_NODE_AGENT(node_agent);

/**
 * Get root of the tree of supported objects.
 *
 * @return Root pointer
 */
static inline rcf_pch_cfg_object *
rcf_pch_conf_root(void)
{
    return &node_agent;
}

/** Free all entries of the list */
static inline void
free_list(olist *list)
{
    olist *tmp;

    for (; list != NULL; list = tmp)
    {
        tmp = list->next;
        free(list);
    }
}

/**
 * Read sub-identifier and instance name from the object or object instance
 * identifier.
 *
 * @param oid           object or object instance identifier
 * @param next_level    location where pointer to the next sub-identifier/
 *                      instance name pair should be placed
 * @param sub_id        location for sub-identifier address (memory is
 *                      allocated for sub-identifier using malloc())
 * @param inst_name     location for instance name address (memory is
 *                      allocated for instance name using malloc())
 *
 * @return Status code
 * @retval 0            success
 * @retval TE_EINVAL       invalid identifier
 * @retval TE_ENOMEM       malloc() failed
 */
static int
parse_one_level(char *oid, char **next_level, char **sub_id,
                char **inst_name)
{
    char *tmp;
    char  c = '/';

    if (inst_name == NULL)
    {
        if (strcmp(oid, "*") == 0 || strcmp(oid, OID_ETC) == 0)
        {
            if ((*sub_id = strdup(oid)) == NULL)
                return TE_ENOMEM;

            *next_level = oid;
            return 0;
        }
        if (*oid++ != '/')
           return TE_EINVAL;

        if ((*next_level = strchr(oid, '/')) == NULL)
        {
            *next_level = oid + strlen(oid);
            c = 0;
        }
        **next_level = 0;
        if ((*sub_id = strdup(oid)) == NULL)
            return TE_ENOMEM;
        **next_level = c;

        if ((strchr(*sub_id, '*') != NULL && strlen(*sub_id) > 1))
        {
            free(*sub_id);
            return TE_EINVAL;
        }

        return 0;
    }

    if (strcmp(oid, "*:*") == 0 || strcmp(oid, OID_ETC) == 0)
    {
        if ((*sub_id = strdup(oid)) == NULL)
            return TE_ENOMEM;

        if ((*inst_name = strdup(oid)) == NULL)
        {
            free(sub_id);
            return TE_ENOMEM;
        }
        *next_level = oid;
        return 0;
    }

    if (*oid != '/')
        return TE_EINVAL;

    oid++;

    if ((*next_level = strchr(oid, '/')) == NULL)
    {
        *next_level = oid + strlen(oid);
        c = 0;
    }
    else
        **next_level = 0;

    if ((tmp = strchr(oid, ':')) != NULL)
    {
        *tmp = 0;
        if ((*sub_id = strdup(oid)) == NULL)
            return TE_ENOMEM;
        if ((*inst_name = strdup(tmp + 1)) == NULL)
        {
            free(sub_id);
            return TE_ENOMEM;
        }
        *tmp = ':';
    }
    else
    {
        if (strcmp(oid, "*") != 0)
            return TE_EINVAL;

        if ((*sub_id = strdup("*")) == NULL)
            return TE_ENOMEM;
        if ((*inst_name = strdup("*")) == NULL)
        {
            free(sub_id);
            return TE_ENOMEM;
        }
    }

    **next_level = c;

    if ((strchr(*sub_id, '*') != NULL && strlen(*sub_id) > 1) ||
        (strchr(*inst_name, '*') != NULL && strlen(*inst_name) > 1) ||
        (**sub_id == '*' && **inst_name != '*'))
    {
        free(*sub_id);
        free(*inst_name);
        return TE_EINVAL;
    }

    return 0;
}

/**
 * Create or update list of object instance identifiers matching to
 * provided wildcard identifier.
 *
 * @param obj           root of the objects subtree
 * @param parsed        already parsed part of initial identifier or NULL
 * @param oid           wildcard identifier (or its tail if part is
 *                      already parsed)
 * @param full_oid      initial identifier
 * @param list          list of identifiers to be updated
 *
 * @return Status code
 * @retval 0               success
 * @retval TE_EINVAL       invalid identifier
 * @retval TE_ENOMEM       malloc() failed
 */
static te_errno
create_wildcard_inst_list(rcf_pch_cfg_object *obj, char *parsed, char *oid,
                          const char *full_oid, olist **list)
{
    char *sub_id = NULL;
    char *inst_name = NULL;
    char *next_level;

    te_bool all;
    te_errno rc;

/**
 * Return from function with resources deallocation.
 *
 * @param _rc     return code
 */
#define RET(_rc) \
    do {                                \
        if ((_rc) != 0)                 \
            free_list(*list);           \
        free(sub_id);                   \
        free(inst_name);                \
        return (_rc);                   \
    } while (FALSE)

    if (*oid == 0 || obj == NULL)
        return 0;

    if (parse_one_level(oid, &next_level, &sub_id, &inst_name) != 0)
        RET(TE_EINVAL);

    all = strcmp(full_oid, "*:*") == 0 || strcmp(sub_id, OID_ETC) == 0;

    for ( ; obj != NULL; obj = obj->brother)
    {
        char *tmp_list = NULL;
        char *tmp_inst_name;
        char *tmp;

        if (!all && *sub_id != '*' && strcmp(obj->sub_id, sub_id) != 0)
            continue;

        if (obj->list == NULL)
        {
            if ((tmp_list = strdup(" ")) == NULL)
                RET(TE_ENOMEM);
        }
        else
        {
            char *dup = (parsed == NULL) ? NULL : strdup(parsed);
            char *tmp;
            char *inst_names[RCF_MAX_PARAMS];
            int   i = -1;

            memset(inst_names, 0, sizeof(inst_names));
            for (tmp = dup;
                 tmp != NULL && i < RCF_MAX_PARAMS;
                 tmp = strchr(tmp, '/'))
            {
                if (i >= 0)
                    inst_names[i] = strchr(tmp, ':') + 1;
                *tmp++ = 0;
                i++;
            }

            rc = (obj->list)(gid, parsed, obj->sub_id, &tmp_list,
                             inst_names[0], inst_names[1], inst_names[2],
                             inst_names[3], inst_names[4], inst_names[5],
                             inst_names[6], inst_names[7], inst_names[8],
                             inst_names[9]);
            if (rc != 0)
            {
                ERROR("List method failed for '%s/%s:', rc=%r", parsed,
                      obj->sub_id, rc);
                free(dup);
                RET(0);
            }
            free(dup);
            if (tmp_list == NULL)
                continue;
        }

        for (tmp_inst_name = tmp_list;
             strlen(tmp_inst_name) > 0;
             tmp_inst_name = tmp)
        {
            char  tmp_parsed[CFG_OID_MAX];

            if ((tmp = strchr(tmp_inst_name, ' ')) == NULL)
            {
                tmp = tmp_inst_name + strlen(tmp_inst_name);
            }
            else
                *tmp++ = 0;

            if (!all && *inst_name != '*' &&
                strcmp(inst_name, tmp_inst_name) != 0)
            {
                continue;
            }

            snprintf(tmp_parsed, CFG_OID_MAX, "%s/%s:%s",
                     parsed == NULL ? "" : parsed,
                     obj->sub_id, tmp_inst_name);
            tmp_parsed[CFG_OID_MAX - 1] = '\0';

            if (*next_level == 0 || all || strcmp(next_level, OID_ETC) == 0)
            {
                olist *new_entry;

                if ((new_entry = (olist *)malloc(sizeof(olist))) == NULL)
                {
                    free(tmp_list);
                    RET(TE_ENOMEM);
                }

                strcpy(new_entry->oid, tmp_parsed);

                new_entry->next = *list;
                *list = new_entry;
            }

            rc = 0;
            if (obj->son != NULL && *next_level != 0)
                rc = create_wildcard_inst_list(obj->son, tmp_parsed,
                                               next_level, full_oid, list);
            if (rc != 0)
            {
                free(tmp_list);
                RET(rc);
            }

            if (*inst_name != '*' && !all)
                break;
        }
        free(tmp_list);

        if (*sub_id != '*' && !all)
            break;
    }

    free(sub_id);
    free(inst_name);

    return 0;

#undef RET
}

/**
 * Create or update list of object identifiers matching to
 * provided wildcard identifier.
 *
 * @param obj           root of the objects subtree
 * @param parsed        already parsed part of initial identifier or NULL
 * @param oid           wildcard identifier (or its tail if part is
 *                      already parsed)
 * @param full_oid      initial identifier
 * @param list          list of identifiers to be updated
 *
 * @return Status code
 * @retval 0               success
 * @retval TE_EINVAL       invalid identifier
 * @retval TE_ENOMEM       malloc() failed
 */
static te_errno
create_wildcard_obj_list(rcf_pch_cfg_object *obj, char *parsed, char *oid,
                         const char *full_oid, olist **list)
{
    char *next_level;
    char *sub_id = NULL;

    te_bool all;

/**
 * Return from function with resources deallocation.
 *
 * @param _rc     return code
 */
#define RET(_rc) \
    do {                                \
        if ((_rc) != 0)                 \
            free_list(*list);           \
        free(sub_id);                   \
        return (_rc);                   \
    } while (FALSE)


    if (*oid == 0 || obj == NULL)
        return 0;

    if (parse_one_level(oid, &next_level, &sub_id, NULL) != 0)
        RET(TE_EINVAL);

    all = *full_oid =='*' || strcmp(sub_id, OID_ETC) == 0;

    for ( ; obj != NULL; obj = obj->brother)
    {
        int   rc = 0;
        char  tmp_parsed[CFG_OID_MAX];

        if (!all && strcmp(obj->sub_id, sub_id) != 0)
            continue;

        sprintf(tmp_parsed, "%s/%s",
                parsed == NULL ? "" : parsed, obj->sub_id);

        if (*next_level == 0 || all || strcmp(next_level, OID_ETC) == 0)
        {
            olist *new_entry;

            if ((new_entry = (olist *)malloc(sizeof(olist))) == NULL)
                RET(TE_ENOMEM);

            strcpy(new_entry->oid, tmp_parsed);
            new_entry->next = *list;
            *list = new_entry;
        }

        if (obj->son != NULL && *next_level != 0)
            rc = create_wildcard_obj_list(obj->son, tmp_parsed,
                                          next_level, full_oid, list);

        if (rc != 0)
            RET(rc);

        if (*sub_id != '*' && !all)
            break;
    }

    free(sub_id);

    return 0;

#undef RET
}


/**
 * Convert the list of objects or object instances identifiers
 * to the single string and always free the list.
 *
 * @param list        list of identifiers to be converted
 * @param answer      location for the answer string address (memory
 *                    is allocated using malloc())
 *
 * @return Status code
 * @retval 0             success
 * @retval TE_ENOMEM     memory allocation failed
 */
static te_errno
convert_to_answer(olist *list, char **answer)
{
    olist  *tmp;
    size_t  len = 1;
    char   *ptr;


    if (list == NULL)
    {
        *answer = strdup("");
        return (*answer == NULL) ? TE_ENOMEM : 0;
    }

    for (tmp = list; tmp != NULL; tmp = tmp->next)
    {
        len += strlen(tmp->oid) + 1;
    }

    if ((*answer = (char *)malloc(len)) == NULL)
    {
        free_list(list);
        return TE_ENOMEM;
    }

    ptr = *answer;
    while (list != NULL)
    {
        tmp = list->next;
        ptr += sprintf(ptr, "%s ", list->oid);
        free(list);
        list = tmp;
    }

    return 0;
}

/**
 * Process wildcard configure get request.
 *
 * @param conn            connection handle
 * @param cbuf            command buffer
 * @param buflen          length of the command buffer
 * @param answer_plen     number of bytes in the command buffer
 *                        to be copied to the answer
 *
 * @param oid             object or instance identifier
 *
 *
 * @return 0 or error returned by communication library
 */
static te_errno
process_wildcard(struct rcf_comm_connection *conn, char *cbuf,
                 size_t buflen, size_t answer_plen, const char *oid)
{
    int    rc;
    char   copy[CFG_OID_MAX];
    char  *tmp;
    olist *list = NULL;

    ENTRY("OID='%s'", oid);
    VERB("Process wildcard request");

    strcpy(copy, oid);
    if (strchr(oid, ':') == NULL)
    {
        VERB("Create list of objects by wildcard");
        rc = create_wildcard_obj_list(rcf_pch_conf_root(),
                                      NULL, copy, oid, &list);
    }
    else
    {
        VERB("Create list of instances by wildcard");

        rc = create_wildcard_inst_list(rcf_pch_conf_root(),
                                       NULL, copy, oid, &list);
    }

    VERB("Wildcard processing result rc=%d list=0x%08x", rc, list);

    if ((rc != 0 )|| ((rc = convert_to_answer(list, &tmp)) != 0))
        SEND_ANSWER("%d", TE_RC(TE_RCF_PCH, rc));

    if ((size_t)snprintf(cbuf + answer_plen, buflen - answer_plen,
                         "0 attach %u",
                         (unsigned int)(strlen(tmp) + 1)) >=
            (buflen - answer_plen))
    {
        free(tmp);
        ERROR("Command buffer too small for reply");
        SEND_ANSWER("%d", TE_RC(TE_RCF_PCH, TE_E2BIG));
    }

    RCF_CH_LOCK;
    rc = rcf_comm_agent_reply(conn, cbuf, strlen(cbuf) + 1);
    VERB("Sent answer to wildcard request '%s' len=%u rc=%d",
         cbuf,  strlen(cbuf) + 1, rc);
    if (rc == 0)
    {
        rc = rcf_comm_agent_reply(conn, tmp, strlen(tmp) + 1);
        VERB("Sent binary attachment len=%u rc=%d", strlen(tmp) + 1, rc);
    }
    RCF_CH_UNLOCK;

    free(tmp);

    return rc;
}


/**
 * Find postponed commit operation with specified parameters.
 *
 * @param f_commit    commit function
 * @param p_oid       configurator object identfier
 *
 * @return Pointer to found commit operation or NULL.
 */
static rcf_pch_commit_op_t *
find_commit_op(rcf_ch_cfg_commit f_commit, const cfg_oid *p_oid)
{
    rcf_pch_commit_op_t *p;

    for (p = TAILQ_FIRST(&commits);
         (p != NULL) &&
         ((p->func != f_commit) || (cfg_oid_cmp(p->oid, p_oid) != 0));
         p = TAILQ_NEXT(p, links));

    return p;
}

/**
 * Immediate or postponed commit of changes.
 *
 * @param commit_obj      configuration tree object with commit function
 * @param pp_oid          pointer to pointer to configuration OID (IN/OUT)
 *
 * @return Status code.
 */
static int
commit(const rcf_pch_cfg_object *commit_obj, cfg_oid **pp_oid)
{
    /* Modify length of OID to length of commit object OID */
    (*pp_oid)->len = commit_obj->oid_len;

    if (is_group)
    {
        if (find_commit_op(commit_obj->commit, *pp_oid) == NULL)
        {
            rcf_pch_commit_op_t *p;


            p = (rcf_pch_commit_op_t *)calloc(1, sizeof(*p));
            if (p == NULL)
            {
                return TE_ENOMEM;
            }

            p->func = commit_obj->commit;
            p->oid  = *pp_oid;

            /* OID is owned by function */
            *pp_oid = NULL;

            TAILQ_INSERT_TAIL(&commits, p, links);
            VERB("Postponed commit added to the list");
        }
        else
        {
            VERB("Duplicate commit - skip");
        }
        return 0;
    }
    else
    {
        VERB("Immediate commit");
        return commit_obj->commit(gid, *pp_oid);
    }
}

/**
 * Do all postponed commits.
 *
 * @return Status code.
 */
static int
commit_all_postponed(void)
{
    te_errno                rc = 0, ret;
    rcf_pch_commit_op_t    *p;

    ENTRY();
    VERB("Postponed commit of group %u", gid);
    while ((p = TAILQ_FIRST(&commits)) != NULL)
    {
        TAILQ_REMOVE(&commits, p, links);

        ret = p->func(gid, p->oid);
        if (ret != 0)
        {
            ERROR("Commit failed: error=%r", ret);
            if (rc == 0)
                rc = TE_RC(TE_RCF_PCH, ret);
        }
        cfg_free_oid(p->oid);
        free(p);
    }
    EXIT("%r", rc);
    return rc;
}


/**
 * Initialize configuration subtree using specified depth for its root.
 *
 * @param p      root of subtree
 * @param depth  root depth (length of the OID)
 */
static void
rcf_pch_cfg_subtree_init(rcf_pch_cfg_object *p, unsigned int depth)
{
    p->oid_len = depth;
    if (p->son != NULL)
        rcf_pch_cfg_subtree_init(p->son, depth + 1);
    if (p->brother != NULL)
        rcf_pch_cfg_subtree_init(p->brother, depth);
}


/* See description in rcf_ch_api.h */
void
rcf_pch_cfg_init(void)
{
    TAILQ_INIT(&commits);

    if (rcf_ch_conf_init() != 0)
    {
        ERROR("Failed to initialize Test Agent "
              "configuration Command Handler");
    }
    else if (rcf_pch_conf_root() != NULL)
    {
        /*
         * Agent root OID has length equal to 2, because of root OID
         * existence with empty subid and name.
         */
        rcf_pch_cfg_subtree_init(rcf_pch_conf_root(), 2);
    }
}


/* See description in rcf_ch_api.h */
te_errno
rcf_pch_agent_list(unsigned int id, const char *oid,
                   const char *sub_id, char **list)
{
    UNUSED(id);
    UNUSED(oid);
    UNUSED(sub_id);

    return (*list = strdup(rcf_ch_conf_agent())) == NULL ? TE_ENOMEM : 0;
}

/**
 * Get the value of the object
 *
 * @param[in]  obj   Object
 * @param[in]  oid   Object instance identifier
 * @param[out] value value of the object
 *
 * @return Status code
 */
static te_errno
get_object_value(rcf_pch_cfg_object *obj, const char *oid, char *value)
{
#define ALL_INST_NAMES \
    inst_names[0], inst_names[1], inst_names[2], inst_names[3], \
    inst_names[4], inst_names[5], inst_names[6], inst_names[7], \
    inst_names[8], inst_names[9]

    char *inst_names[RCF_MAX_PARAMS] = {NULL,};
    cfg_oid *p_oid = NULL;
    cfg_inst_subid *p_ids;
    unsigned int i;
    te_errno rc = 0;

    p_oid = cfg_convert_oid_str(oid);
    if (p_oid == NULL)
    {
        ERROR("Failed to convert OID string '%s' to structured "
              "representation", oid);
        return TE_RC(TE_RCF_PCH, TE_EINVAL);
    }

    p_ids = (cfg_inst_subid *)(p_oid->ids);

    /*
     * The value starts with 2, since the first one (with index 0) is empty
     * root and the second one is the agent name which are not required to
     * get value.
     */
    for (i = 2; i < p_oid->len; i++)
        inst_names[i - 2] = p_ids[i].name;

    rc = (obj->get)(gid, oid, value, ALL_INST_NAMES);
    if (rc != 0)
        ERROR("Failed to get value for '%s' rc=%r", oid, rc);

    cfg_free_oid(p_oid);
    return rc;
#undef ALL_INST_NAMES
}

/**
 * Get the instance OID by the object OID.
 *
 * The function implements an algorithm for finding the instance OID (@p oid)
 * by object OID (@p object) for the substitution purposes.
 * The idea is to compare sub-identifiers of the object's and instance's OID
 * (@p p_ids) for which the substitution is being performed.
 * If the first N sub-identifiers match, then the first N identifiers and
 * their values are copied to the final instance OID (the value from the
 * configuration tree will be got by this instance OID).
 * It is guaranteed that N is equal to at least one.
 * If the subidentifiers starting from N+1 are not equal, then they are copied
 * with empty values.
 *
 * @param[in]  object Object OID (Source value for substitution)
 * @param[in]  p_ids  Pointer to object instance identifer element
 *                    for which the substitution is being made
 * @param[out] oid    The instance OID
 *
 * @return Status code
 */
static te_errno
get_instance_oid_by_object_oid(const char *object, cfg_inst_subid *p_ids,
                               te_string *oid)
{
    cfg_oid *p_subst_oid = NULL;
    cfg_object_subid *p_subst_ids;
    te_errno rc = 0;
    int i;

    p_subst_oid = cfg_convert_oid_str(object);
    if (p_subst_oid == NULL)
    {
        ERROR("Failed to convert OID string '%s' to structured "
              "representation", object);
        return TE_RC(TE_RCF_PCH, TE_EINVAL);
    }

    p_subst_ids = (cfg_object_subid *)(p_subst_oid->ids);

    for (i = 1; i < p_subst_oid->len; i++)
    {
        if (strcmp(p_subst_ids[i].subid, p_ids[i].subid) != 0)
            break;

        rc = te_string_append(oid, "/%s:%s", p_ids[i].subid,
                              p_ids[i].name);
        if (rc != 0)
            break;
    }

    if (rc != 0)
    {
        cfg_free_oid(p_subst_oid);
        return rc;
    }

    for (; i < p_subst_oid->len; i++)
    {
        rc = te_string_append(oid, "/%s:", p_subst_ids[i].subid);
        if (rc != 0)
            break;
    }

    cfg_free_oid(p_subst_oid);

    return rc;
}

/**
 * Processing of the configurator node substitution
 *
 * @param[in]    obj    Configuration tree node
 * @param[in,out] value The string in which the substitution is performed.
 * @param[in]    sub_id Value of the subid for substitution
 * @param[in]    p_ids  Pointer to array of instance OID elements
 *
 * @return Status code
 */
static int
do_substitutions(rcf_pch_cfg_object *obj, char *value, const char *sub_id,
                 cfg_inst_subid *p_ids)
{
    const rcf_pch_cfg_substitution *subst;
    rcf_pch_cfg_object *node;
    te_string subs = TE_STRING_INIT;
    te_string inst_oid = TE_STRING_INIT;
    te_string value_s = TE_STRING_INIT;
    char ret_val[RCF_MAX_VAL] = "";
    te_errno rc = 0;

    for (subst = obj->subst; subst->name != NULL; subst++)
    {
        if (strcmp(subst->name, "*") == 0)
            break;

        if (strcmp(subst->name, sub_id) == 0)
            break;
    }

    if (subst->name == NULL)
        goto out;

    rc = te_string_append(&value_s, "%s", value);
    if (rc != 0)
        goto out;

    rc = get_instance_oid_by_object_oid(subst->ref_name, p_ids, &inst_oid);
    if (rc != 0)
        goto out;

    rc = rcf_pch_find_node(subst->ref_name, &node);
    if (rc != 0)
        goto out;

    rc = get_object_value(node, inst_oid.ptr, ret_val);
    if (rc != 0)
        goto out;

    rc = te_string_append(&subs,
                        CS_SUBSTITUTION_DELIMITER"%s"CS_SUBSTITUTION_DELIMITER,
                        inst_oid.ptr);
    if (rc != 0)
        goto out;

    rc = subst->apply(&value_s, subs.ptr, ret_val);
    if (rc != 0)
        goto out;

    if (value_s.len >= RCF_MAX_VAL)
    {
        ERROR("The value after substitution is too large");
        rc = ENOBUFS;
        goto out;
    }

    memcpy(value, value_s.ptr, value_s.len + 1);

out:
    te_string_free(&value_s);
    te_string_free(&subs);
    te_string_free(&inst_oid);

    return rc;
}

/* See description in rcf_pch.h */
int
rcf_pch_configure(struct rcf_comm_connection *conn,
                  char *cbuf, size_t buflen, size_t answer_plen,
                  const uint8_t *ba, size_t cmdlen,
                  rcf_ch_cfg_op_t op, const char *oid, const char *val)
{
/** All instance names */
#define ALL_INST_NAMES \
    inst_names[0], inst_names[1], inst_names[2], inst_names[3], \
    inst_names[4], inst_names[5], inst_names[6], inst_names[7], \
    inst_names[8], inst_names[9]

    /* Array of instance names */
    char *inst_names[RCF_MAX_PARAMS]; /* 10 */

    cfg_oid            *p_oid = NULL;
    cfg_inst_subid     *p_ids;
    rcf_pch_cfg_object *obj = NULL;
    rcf_pch_cfg_object *next;
    rcf_pch_cfg_object *commit_obj = NULL;
    unsigned int        i;
    int                 rc;

    UNUSED(ba);
    UNUSED(cmdlen);

    ENTRY("op=%d id='%s' val='%s'", op, (oid == NULL) ? "NULL" : oid,
                                        (val == NULL) ? "NULL" : val);
    VERB("Default configuration hanlder is executed");

    if (oid != 0)
    {
        /* Now parse the oid and look for the object */
        if ((strchr(oid, '*') != NULL) || (strstr(oid, OID_ETC) != NULL))
        {
            if (op != RCF_CH_CFG_GET)
            {
                ERROR("Wildcards allowed in get requests only");
                SEND_ANSWER("%d", TE_RC(TE_RCF_PCH, TE_EINVAL));
            }

            rc = process_wildcard(conn, cbuf, buflen, answer_plen, oid);

            EXIT("%r", rc);

            return rc;
        }

        p_oid = cfg_convert_oid_str(oid);
        VERB("Parsed %s ID with %u parts ptr=0x%x",
             (p_oid->inst) ? "instance" : "object",
             p_oid->len, p_oid->ids);
        if (p_oid == NULL)
        {
            /* It may be memory allocation failure, but it's unlikely */
            ERROR("Failed to convert OID string '%s' to structured "
                  "representation", oid);
            SEND_ANSWER("%d", TE_RC(TE_RCF_PCH, TE_EFMT));
        }
        if (!p_oid->inst)
        {
            cfg_free_oid(p_oid);
            ERROR("Instance identifier expected");
            SEND_ANSWER("%d", TE_RC(TE_RCF_PCH, TE_EINVAL));
        }
        if (p_oid->len == 0)
        {
            cfg_free_oid(p_oid);
            ERROR("Zero length OIID");
            SEND_ANSWER("%d", TE_RC(TE_RCF_PCH, TE_EINVAL));
        }

        memset(inst_names, 0, sizeof(inst_names));
        p_ids = (cfg_inst_subid *)(p_oid->ids);

        for (i = 1, obj = 0, next = rcf_pch_conf_root();
             (i < p_oid->len) && (next != NULL);
            )
        {
            obj = next;
            if (strcmp(obj->sub_id, p_ids[i].subid) == 0)
            {
                if (i == 1)
                {
                    if (strcmp(p_ids[i].name, rcf_ch_conf_agent()) != 0)
                    {
                        break;
                    }
                }
                else if ((i - 2) < (sizeof(inst_names) /
                                    sizeof(inst_names[0])))
                {
                    inst_names[i - 2] = p_ids[i].name;
                }
                /* Go to the next subid */
                ++i;
                next = obj->son;
            }
            else
            {
                next = obj->brother;
            }
        }
        if (i < p_oid->len)
        {
            cfg_free_oid(p_oid);
            VERB("Requested OID not found");
            SEND_ANSWER("%d", TE_RC(TE_RCF_PCH, TE_ENOENT));
        }
    }

    if (obj != NULL)
    {
        commit_obj = (obj->commit_parent != NULL) ?
                         obj->commit_parent : obj;
    }

    if (!is_group)
        ++gid;

    switch (op)
    {
        case RCF_CH_CFG_GRP_START:
            VERB("Configuration group %u start", gid);
            is_group = TRUE;
            SEND_ANSWER("0");
            break;

        case RCF_CH_CFG_GRP_END:
            VERB("Configuration group %u end", gid);
            is_group = FALSE;
            SEND_ANSWER("%d", commit_all_postponed());
            break;

        case RCF_CH_CFG_GET:
        {
            char value[RCF_MAX_VAL] = "";
            char ret_val[RCF_MAX_VAL * 2 + 2];

            if (obj->get == NULL)
            {
                cfg_free_oid(p_oid);
                SEND_ANSWER("0");
            }

            rc = (obj->get)(gid, oid, value, ALL_INST_NAMES);
            if (rc != 0)
            {
                cfg_free_oid(p_oid);
                SEND_ANSWER("%d", TE_RC(TE_RCF_PCH, rc));
            }

            if (obj->subst != NULL)
            {
                rc = do_substitutions(obj, value, inst_names[i - 3], p_ids);
                if (rc != 0)
                {
                    ERROR("Failed to replace value in %s rc=%r", value, rc);
                    cfg_free_oid(p_oid);
                    SEND_ANSWER("%d", TE_RC(TE_RCF_PCH, rc));
                }
            }

            cfg_free_oid(p_oid);
            write_str_in_quotes(ret_val, value, RCF_MAX_VAL);
            SEND_ANSWER("0 %s", ret_val);
            break;
        }

        case RCF_CH_CFG_SET:
            rc = (obj->set == NULL) ? TE_EOPNOTSUPP :
                     (obj->set)(gid, oid, val, ALL_INST_NAMES);
            if ((rc == 0) && (commit_obj->commit != NULL))
            {
                rc = commit(commit_obj, &p_oid);
            }
            cfg_free_oid(p_oid);
            SEND_ANSWER("%d", TE_RC(TE_RCF_PCH, rc));
            break;

        case RCF_CH_CFG_ADD:
            rc = (obj->add == NULL) ? TE_EOPNOTSUPP :
                     (obj->add)(gid, oid, val, ALL_INST_NAMES);
            if ((rc == 0) && (commit_obj->commit != NULL))
            {
                rc = commit(commit_obj, &p_oid);
            }
            cfg_free_oid(p_oid);
            SEND_ANSWER("%d", TE_RC(TE_RCF_PCH, rc));
            break;

        case RCF_CH_CFG_DEL:
            rc = (obj->del == NULL) ? TE_EOPNOTSUPP :
                     (obj->del)(gid, oid, ALL_INST_NAMES);
            if ((rc == 0) && (commit_obj->commit != NULL))
            {
                rc = commit(commit_obj, &p_oid);
            }
            cfg_free_oid(p_oid);
            SEND_ANSWER("%d", TE_RC(TE_RCF_PCH, rc));
            break;

        default:
            ERROR("Unknown congfigure operation: op=%d id='%s' val='%s'",
                  op, oid, val);
            SEND_ANSWER("%d", TE_RC(TE_RCF_PCH, TE_EINVAL));
    }

    /* Unreachable */
    assert(FALSE);
    return 0;

#undef ALL_INST_NAMES
}

/* See description in rcf_pch.h */
te_errno
rcf_pch_find_node(const char *oid_str, rcf_pch_cfg_object **node)
{
    rcf_pch_cfg_object  *tmp = rcf_pch_conf_root();
    cfg_oid             *oid = cfg_convert_oid_str(oid_str);
    int                  i = 1;

    if (oid == NULL || oid->inst || oid->len < 2)
    {
        ERROR("%s(): OID '%s' cannot be resolved",
              __FUNCTION__, oid_str);
        cfg_free_oid(oid);
        return TE_RC(TE_RCF_PCH, TE_EINVAL);
    }

    while (TRUE)
    {
        for (; tmp != NULL; tmp = tmp->brother)
        {
            if (strcmp(((cfg_object_subid *)(oid->ids))[i].subid,
                       tmp->sub_id) == 0)
            {
                break;
            }
        }

        if (tmp == NULL)
        {
            cfg_free_oid(oid);
            return TE_RC(TE_RCF_PCH, TE_ENOENT);
        }
        if (++i == oid->len)
            break;
        tmp = tmp->son;
    }
    cfg_free_oid(oid);

    *node = tmp;
    return 0;
}

/* See description in rcf_pch.h */
te_errno
rcf_pch_add_node(const char *father, rcf_pch_cfg_object *node)
{
    rcf_pch_cfg_object *tmp = NULL;
    rcf_pch_cfg_object *next;
    te_errno            rc;

    rc = rcf_pch_find_node(father, &tmp);
    if (rc != 0)
    {
        ERROR("%s(): failed to find '%s' in configuration tree",
              __FUNCTION__, father);
        return rc;
    }

    next = tmp->son;
    tmp->son = node;
    while (node->brother != NULL)
        node = node->brother;
    node->brother = next;

    return 0;
}

/** Find family of the node */
static rcf_pch_cfg_object *
find_father(rcf_pch_cfg_object *node, rcf_pch_cfg_object *ancestor,
            rcf_pch_cfg_object **brother)
{
    rcf_pch_cfg_object *tmp1, *tmp2;

    for (tmp1 = ancestor->son, tmp2 = NULL;
         tmp1 != NULL;
         tmp2 = tmp1, tmp1= tmp1->brother)
    {
        if (tmp1 == node)
        {
            *brother = tmp2;
            return ancestor;
        }

        if ((tmp2 = find_father(node, tmp1, brother)) != NULL)
            return tmp2;
    }

    return NULL;
}

/**
 * Delete subtree into the configuration tree.
 *
 * @param node          node to be deleted
 *
 * @return Status code
 */
te_errno
rcf_pch_del_node(rcf_pch_cfg_object *node)
{
    rcf_pch_cfg_object *brother;
    rcf_pch_cfg_object *father = find_father(node, rcf_pch_conf_root(),
                                             &brother);

    if (father == NULL)
    {
        VERB("Failed to find node family");
        return TE_RC(TE_RCF_PCH, TE_ENOENT);
    }

    if (brother != NULL)
        brother->brother = node->brother;
    else
        father->son = node->brother;

    /*
     * OBJ nodes are static variables that can be reused at any next stage,
     * which means that when we delete a separate node we should be able
     * to add it later.
     *
     * While adding a node we take into account its brothers, because it is
     * possible to add a set of nodes (linked together at one call).
     * When we delete a node we specify __exact__ node to remove from
     * the list, so it means we need to clean it from any links to
     * brother nodes. Otherwise we would have a situation when we have
     * not added node with links to brothers. Then if we add this node again
     * we may have a situation when linked list of nodes becomes circular
     * list (without trailing NULL). Then this will cause dead loop if we
     * try to find a node that does not exist in the list.
     * This problem was revealed while doing:
     * - add 'hdcpserver';
     * - del 'dhcpserver';
     * - add 'dhcpserver';
     * - search for 'pppoeserver' - dead loop in create_wildcard_inst_list()
     *                              function because we did not register
     *                              'pppoeserver'.
     */
    node->brother = NULL;

    return 0;
}

/** Information about dynamically grabbed resource - see rcf_pch.h */
typedef struct rsrc_info {
    struct rsrc_info *next;

    const char                   *name; /**< Generic resource name */
    rcf_pch_rsrc_grab_callback    grab; /**< Grab callback */
    rcf_pch_rsrc_release_callback release; /**< Release callback */
} rsrc_info;

/** Information about all dynamically grabbed resources */
static rsrc_info *rsrc_info_list;

/** Find the resource with specified generic name */
static rsrc_info *
rsrc_lookup(const char *name)
{
    rsrc_info *tmp;

    for (tmp = rsrc_info_list; tmp != NULL && name != NULL; tmp = tmp->next)
        if (strcmp(name, tmp->name) == 0)
            return tmp;

    return NULL;
}

/**
 * Specify callbacks for dynamically registarable resource.
 *
 * @param name          resource generic name
 * @param reg           grabbing callback
 * @param unreg         releasing callback or NULL
 *
 * @return Status code
 */
te_errno
rcf_pch_rsrc_info(const char *name,
                  rcf_pch_rsrc_grab_callback grab,
                  rcf_pch_rsrc_release_callback release)
{
    rsrc_info *tmp;

    if (name == NULL || grab == NULL)
        return TE_RC(TE_RCF_PCH, TE_EINVAL);

    if (rsrc_lookup(name) != NULL)
        return TE_RC(TE_RCF_PCH, TE_EEXIST);

    if ((tmp = malloc(sizeof(*tmp))) == NULL)
        return TE_RC(TE_RCF_PCH, TE_ENOMEM);

    if ((tmp->name = strdup(name)) == NULL)
    {
        free(tmp);
        return TE_RC(TE_RCF_PCH, TE_ENOMEM);
    }

    tmp->grab = grab;
    tmp->release = release;
    tmp->next = rsrc_info_list;
    rsrc_info_list = tmp;

    return 0;
}

te_errno
rcf_pch_rsrc_grab_dummy(const char *name)
{
    UNUSED(name);
    return 0;
}

te_errno
rcf_pch_rsrc_release_dummy(const char *name)
{
    UNUSED(name);
    return 0;
}

#ifndef __CYGWIN__
/**
 * Discard /agent:<name> prefix from resourse name.
 *
 * @param name          Name of the resource
 *
 * @return Name to be used to create lock.
 */
static const char *
rsrc_lock_name(const char *name)
{
    const char *tmp;

    if ((strcmp_start("/agent:", name) == 0) &&
        ((tmp = strchr(name + 1, '/')) != NULL) &&
        (*++tmp != '\0'))
    {
        return tmp;
    }
    return name;
}

/**
 * Generate path to lock requested resource.
 *
 * @param name          Resource to lock
 * @param path          Location for path
 * @param pathlen       Size of the location for path
 *
 * @return Pointer to @a path or NULL if @a pathlen is too short.
 */
static char *
rsrc_lock_path(const char *name, char *path, size_t pathlen)
{
    const char     *lock_name = rsrc_lock_name(name);
    unsigned int    i;

    if (snprintf(path, pathlen, "%s/te_ta_lock_%s",
                 te_lockdir, lock_name) >= (int)pathlen)
    {
        ERROR("Too long pathname for lock: %s/te_ta_lock_%s",
              te_lockdir, lock_name);
        return NULL;
    }

    for (i = strlen(te_lockdir) + 1; path[i] != '\0'; i++)
        if (path[i] == '/')
            path[i] = '%';

    return path;
}

typedef enum rsrc_lock_type {
    RSRC_LOCK_SHARED,
    RSRC_LOCK_EXCLUSIVE,
    RSRC_LOCK_UNDEFINED,
} rsrc_lock_type;

static const char *rsrc_lock_type_names[] = {
    [RSRC_LOCK_SHARED] = "shared",
    [RSRC_LOCK_EXCLUSIVE] = "exclusive",
    [RSRC_LOCK_UNDEFINED] = "undefined",
};

typedef struct rsrc_lock {
    rsrc_lock_type type; /**< Lock type */
    te_vec pids; /**< Vector of PIDs of processes that hold the lock. */
} rsrc_lock;

static te_errno
delete_rsrc_lock_file(const char *fname)
{
    te_errno rc;

    if (unlink(fname) != 0)
    {
        rc = TE_OS_RC(TE_RCF_PCH, errno);

        ERROR("Failed to delete lock %s: %r", fname, rc);
        return rc;
    }

    return 0;
}

static te_errno
update_rsrc_lock_file(rsrc_lock *lock, const char *fname, int fd)
{
    te_string str = TE_STRING_INIT;
    te_bool empty = TRUE;
    te_errno rc;
    pid_t *pid;

    TE_VEC_FOREACH(&lock->pids, pid)
    {
        if (*pid >= 0)
        {
            empty = FALSE;
            break;
        }
    }

    if (empty || lock->type == RSRC_LOCK_UNDEFINED)
        return delete_rsrc_lock_file(fname);

    rc = te_string_append(&str, "%s", rsrc_lock_type_names[lock->type]);
    if (rc != 0)
        return rc;

    TE_VEC_FOREACH(&lock->pids, pid)
    {
        rc = te_string_append(&str, " %u", (unsigned int)*pid);
        if (rc != 0)
        {
            te_string_free(&str);
            return rc;
        }
    }

    if (ftruncate(fd, 0) != 0 ||
        pwrite(fd, str.ptr, str.len, 0) != (ssize_t)str.len)
    {
        te_string_free(&str);
        return TE_RC(TE_RCF_PCH, TE_EFAIL);
    }
    te_string_free(&str);

    return 0;
}

static rsrc_lock *
alloc_rsrc_lock(rsrc_lock_type type)
{
    rsrc_lock *result;

    result = TE_ALLOC(sizeof(*result));
    if (result == NULL)
        return NULL;

    result->pids = TE_VEC_INIT(pid_t);
    result->type = type;

    return result;
}

static void
free_rsrc_lock(rsrc_lock *lock)
{
    if (lock == NULL)
        return;

    te_vec_free(&lock->pids);
    free(lock);
}

/*
 * Add a process with @p my_pid to a lock in the mode specified
 * by @p may_share.
 *
 * @param lock          Existing lock
 * @param may_share     Process wants to lock resource as shared (@c TRUE),
 *                      or exclusive (@c FALSE)
 * @param my_pid        PID of a process that needs a lock
 *
 * @return              Status code
 */
static te_errno
add_rsrc_lock(rsrc_lock *lock, te_bool may_share, pid_t my_pid)
{
    te_bool has_my_pid = FALSE;
    te_errno rc;
    pid_t *pid;

    if (te_vec_size(&lock->pids) > 0)
    {
        te_bool first_pid_mine = TE_VEC_GET(pid_t, &lock->pids, 0) == my_pid;

        if (lock->type == RSRC_LOCK_UNDEFINED)
        {
            ERROR("Undefined state of a lock with PIDs");
            return TE_RC(TE_RCF_PCH, TE_EINVAL);
        }

        if (may_share && lock->type == RSRC_LOCK_EXCLUSIVE && !first_pid_mine)
            return TE_RC(TE_RCF_PCH, TE_EPERM);
        else if (!may_share && (te_vec_size(&lock->pids) > 1 || !first_pid_mine))
            return TE_RC(TE_RCF_PCH, TE_EPERM);
    }

    lock->type = may_share ? RSRC_LOCK_SHARED : RSRC_LOCK_EXCLUSIVE;

    TE_VEC_FOREACH(&lock->pids, pid)
    {
        if (*pid == my_pid)
            has_my_pid = TRUE;
    }

    if (!has_my_pid)
    {
        rc = TE_VEC_APPEND(&lock->pids, my_pid);
        if (rc != 0)
            return rc;
    }

    return 0;
}

/*
 * Remove a provess with @p my_pid from a lock
 *
 * @param lock          Existing lock
 * @param my_pid        PID
 *
 * @return              Status code
 */
static te_errno
remove_rsrc_lock(rsrc_lock *lock, pid_t my_pid)
{
    te_bool my_pid_removed = FALSE;
    size_t i;

    for (i = 0; i < te_vec_size(&lock->pids); i++)
    {
        if (TE_VEC_GET(pid_t, &lock->pids, i) == my_pid)
        {
            te_vec_remove_index(&lock->pids, i);
            my_pid_removed = TRUE;
            break;
        }
    }

    if (!my_pid_removed)
    {
        ERROR("Failed to remove lock, PID of the running TA is not found");
        return TE_RC(TE_RCF_PCH, TE_EFAIL);
    }

    return 0;
}

static te_errno
set_lock_rsrc_lock_file(int fd, short l_type)
{
    struct flock lock;
    te_errno rc;

    lock.l_type = l_type;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;

    rc = fcntl(fd, F_SETLKW, &lock);
    if (rc != 0)
    {
        WARN("Failed to set lock for resource lock file");
        return TE_RC(TE_RCF_PCH, TE_EFAIL);
    }

    return 0;
}

static te_errno
lock_rsrc_lock_file(int fd)
{
    return set_lock_rsrc_lock_file(fd, F_WRLCK);
}

static te_errno
unlock_rsrc_lock_file(int fd)
{
    return set_lock_rsrc_lock_file(fd, F_UNLCK);
}

/*
 * Read resource lock file. PIDs of dead processes are excluded.
 *
 * @param fd        File descriptor of a lock file
 * @param lock      Location for lock information
 * @param my_pid    PID of a process that reads the file
 *
 * @return Status code.
 */
static te_errno
read_rsrc_lock_file(int fd, rsrc_lock **lock, pid_t my_pid)
{
    rsrc_lock *result = NULL;
    char *data = NULL;
    te_errno rc = 0;
    off_t size;
    char *p;

    if ((size = lseek(fd, 0, SEEK_END)) < 0)
    {
        ERROR("Failed to seek in lock file");
        rc = TE_RC(TE_RCF_PCH, TE_EFAIL);
        goto out;
    }

    result = alloc_rsrc_lock(RSRC_LOCK_UNDEFINED);
    if (result == NULL)
    {
        rc = TE_RC(TE_RCF_PCH, TE_ENOMEM);
        goto out;
    }

    if (size == 0)
    {
        *lock = result;
        rc = 0;
        goto out;
    }

    /* Allocate with null terminator */
    data = TE_ALLOC(size + 1);
    if (data == NULL)
    {
        rc = TE_RC(TE_RCF_PCH, TE_ENOMEM);
        goto out;
    }

    if (pread(fd, data, size, 0) != size)
    {
        ERROR("Failed to read lock file");
        rc = TE_RC(TE_RCF_PCH, TE_EFAIL);
        goto out;
    }

    if (strcmp_start(rsrc_lock_type_names[RSRC_LOCK_SHARED], data) == 0)
    {
        result->type = RSRC_LOCK_SHARED;
    }
    else if (strcmp_start(rsrc_lock_type_names[RSRC_LOCK_EXCLUSIVE], data) == 0)
    {
        result->type = RSRC_LOCK_EXCLUSIVE;
    }
    else
    {
        ERROR("Invalid lock file prefix format");
        rc = TE_RC(TE_RCF_PCH, TE_EFAIL);
        goto out;
    }

    p = data;
    while ((p = strchr(p, ' ')) != NULL && *(++p) != '\0')
    {
        pid_t pid;

        if ((pid = atoi(p)) == 0)
        {
            ERROR("Format of the lock file is not recognized");
            rc = TE_RC(TE_RCF_PCH, TE_EPERM);
            goto out;
        }

        /*
         * Zero signal just check a possibility to send signal.
         * Add pid to vector only if a process is running.
         */
        if (pid == my_pid || kill(pid, 0) == 0)
        {
            rc = TE_VEC_APPEND(&result->pids, pid);
            if (rc != 0)
            {
                ERROR("Failed to append a PID to vector");
                rc = TE_RC(TE_RCF_PCH, TE_EFAIL);
                goto out;
            }
        }
        else
        {
            WARN("Lock of a dead process %d is ignored", (int)pid);
        }
    }

    *lock = result;

out:
    free(data);
    if (rc != 0)
    {
        if (result != 0)
            te_vec_free(&result->pids);

        free(result);
    }

    return rc;
}

/**
 * Check if lock with specified path exists and is owned by other agent
 * processes. Other processes are the ones that have a PID other than @p my_pid.
 *
 * @param fname         Path to lock file
 * @param my_pid        PID of a process to check lock for
 *
 * @return Status code.
 * @retval 0            Lock does not exist or is owned only by the specified
 *                      process (shared or exclusive).
 * @retval TE_EPERM     Lock of other process found.
 * @retval other        Unexpected failure.
 */
te_errno
check_lock(const char *fname, pid_t my_pid)
{
    rsrc_lock *lock = NULL;
    te_errno rc = 0;
    pid_t *pid;
    int fd;

    if ((fd = open(fname, O_RDWR)) < 0)
    {
        if (errno == ENOENT)
        {
            return 0;
        }
        else
        {
            rc = te_rc_os2te(errno);

            ERROR("%s(): open(%s) failed unexpectedly: %r",
                  __FUNCTION__, fname, rc);
            /*
             * TE_EFAIL is used so that TE_EPERM is not returned here
             * which means that lock of other process is found.
             */
            return TE_RC(TE_RCF_PCH, TE_EFAIL);
        }
    }

    rc = lock_rsrc_lock_file(fd);
    if (rc != 0)
        goto out;

    rc = read_rsrc_lock_file(fd, &lock, my_pid);
    if (rc != 0)
    {
        ERROR("Failed to read lock '%s'", fname);
        goto out;
    }

    TE_VEC_FOREACH(&lock->pids, pid)
    {
        if (*pid != my_pid)
        {
            ERROR("Lock of the PID %d is found for %s", (int)*pid, fname);
            rc = TE_RC(TE_RCF_PCH, TE_EPERM);
            goto out;
        }
    }

    rc = update_rsrc_lock_file(lock, fname, fd);

out:
    close(fd);
    free_rsrc_lock(lock);

    return rc;
}

/* See the description in rcf_pch.h */
te_errno
rcf_pch_rsrc_check_locks(const char *rsrc_ptrn)
{
    char    path_ptrn[RCF_MAX_PATH];
    glob_t  gb;
    int     ret;

    if (rsrc_lock_path(rsrc_ptrn, path_ptrn, sizeof(path_ptrn)) == NULL)
        return TE_RC(TE_RCF_PCH, TE_ENAMETOOLONG);

    memset(&gb, 0, sizeof(gb));
    ret = glob(path_ptrn, GLOB_NOSORT, NULL, &gb);
    if (ret == 0)
    {
        int      i;
        te_errno rc = 0;

        for (i = 0; i < (int)gb.gl_pathc; ++i)
        {
            rc = check_lock(gb.gl_pathv[i], getpid());
            if (rc != 0)
                break;
        }
        globfree(&gb);
        return rc;
    }
    else if (ret == GLOB_NOMATCH)
    {
        return 0;
    }
    else if (ret == GLOB_NOSPACE)
    {
        return TE_RC(TE_RCF_PCH, TE_ENOMEM);
    }
    else
    {
        return TE_RC(TE_RCF_PCH, TE_EFAULT);
    }
}

/*
 * Update lock file for a resource.
 * The lock is first found, then dead TA PIDs are removed from lock,
 * then PID of current process is added/deleted (@p add_lock),
 * then updated lock file is written (or removed if no PIDs are left).
 *
 * @param name                  Resource name
 * @param shared             @c TRUE if resource is shared
 * @param add_lock              @c TRUE - add lock @c FALSE - remove lock
 * @param my_pid                PID of a process to update lock for
 * @param fallback_shared    @c TRUE - try to lock as shared if
 *                              exclusive locking failed
 * @param attempts_timeout_ms   Retry attempts to lock until the timeout
 *                              passes (in milliseconds)
 *
 * @return Status code.
 */
static te_errno
ta_rsrc_update_lock(const char *name, te_bool *shared, te_bool add_lock,
                    pid_t my_pid, te_bool fallback_shared,
                    unsigned int attempts_timeout_ms)
{
    char        fname[RCF_MAX_PATH];
    rsrc_lock  *lock = NULL;
    int         fd;
    te_errno    rc = 0;
    te_bool     result_shared = *shared;

    if (rsrc_lock_path(name, fname, sizeof(fname)) == NULL)
        return TE_RC(TE_RCF_PCH, TE_ENAMETOOLONG);

    if ((fd = open(fname, O_CREAT | O_RDWR, 0666)) < 0)
        return TE_OS_RC(TE_RCF_PCH, errno);

    while (1)
    {
        unsigned int sleep_ms;

        rc = lock_rsrc_lock_file(fd);
        if (rc != 0)
            goto out;

        rc = read_rsrc_lock_file(fd, &lock, my_pid);
        if (rc != 0)
        {
            ERROR("Failed to read lock '%s'", fname);
            goto out;
        }

        if (add_lock)
            rc = add_rsrc_lock(lock, result_shared, my_pid);
        else
            rc = remove_rsrc_lock(lock, my_pid);

        if (rc == 0)
            break;

        sleep_ms = attempts_timeout_ms > 1000 ? 1000 : attempts_timeout_ms;
        attempts_timeout_ms -= sleep_ms;

        if (sleep_ms > 0)
        {
            rc = unlock_rsrc_lock_file(fd);
            if (rc != 0)
                goto out;

            RING("Retrying updating lock file");
            te_msleep(sleep_ms);
        }
        else if (!result_shared && fallback_shared)
        {
            rc = unlock_rsrc_lock_file(fd);
            if (rc != 0)
                goto out;

            result_shared = TRUE;
        }
        else
        {
            ERROR("Failed to %s %s lock", add_lock ? "acquire" : "release",
                  result_shared ? "shared" : "exclusive");
            goto out;
        }
    }

    rc = update_rsrc_lock_file(lock, fname, fd);
    if (rc == 0)
        *shared = result_shared;

out:
    close(fd);
    free_rsrc_lock(lock);

    return rc;
}

te_errno
ta_rsrc_create_lock(const char *name, te_bool *shared,
                    te_bool fallback_shared,
                    unsigned int attempts_timeout_ms)
{
    return ta_rsrc_update_lock(name, shared, TRUE, getpid(),
                               fallback_shared, attempts_timeout_ms);
}

/*
 * Delete a lock for the resource with specified name.
 *
 * @param name     resource name
 */
void
ta_rsrc_delete_lock(const char *name)
{
    te_bool shared = FALSE;

    ta_rsrc_update_lock(name, &shared, FALSE, getpid(), FALSE, 0);
}
#endif /* !__CYGWIN__ */

/** Registered resources list entry */
typedef struct rsrc {
    struct rsrc *next;  /**< Next element in the list */
    char        *id;    /**< Name of the instance in the OID */
    char        *name;  /**< Resource name (instance value) */
    te_bool      shared; /**< Resource is shared if @c TRUE */
    unsigned int fallback_shared; /**<
                                      * Try to grab as shared if exclusive
                                      * grab failed.
                                      */
    unsigned int attempts_timeout_ms; /**< Grab retry timeout */
} rsrc;

/** List of registered resources */
static rsrc *rsrc_lst;

static te_errno
rsrc_find_by_id(const char *id, rsrc **resource)
{
    rsrc *tmp;

    for (tmp = rsrc_lst; tmp != NULL; tmp = tmp->next)
    {
        if (strcmp(tmp->id, id) == 0)
        {
            if (resource != NULL)
                *resource = tmp;

            return 0;
        }
    }

    return TE_RC(TE_RCF_PCH, TE_ENOENT);
}

/**
 * Get instance list for object "/agent/rsrc".
 *
 * @param gid           group identifier (unused)
 * @param oid           full identifier of the father instance
 * @param sub_id        ID of the object to be listed (unused)
 * @param list          location for the list pointer
 *
 * @return Status code
 * @retval 0            success
 * @retval TE_ENOMEM    cannot allocate memory
 */
static te_errno
rsrc_list(unsigned int gid, const char *oid,
          const char *sub_id, char **list)
{
#define MEM_BULK        1024
    int   len = MEM_BULK;
    char *buf = calloc(1, len);
    int   offset = 0;
    rsrc *tmp;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(sub_id);

    if (buf == NULL)
        return TE_RC(TE_RCF_PCH, TE_ENOMEM);

    for (tmp = rsrc_lst; tmp != NULL; tmp = tmp->next)
    {
        if (len - offset <= (int)strlen(tmp->id) + 2)
        {
            char *new_buf;

            len += MEM_BULK;
            if ((new_buf = realloc(buf, len)) == NULL)
            {
                free(buf);
                return TE_RC(TE_RCF_PCH, TE_ENOMEM);
            }
            else
            {
                buf = new_buf;
            }
        }
        offset += sprintf(buf + offset, "%s ", tmp->id);
    }

    *list = buf;

    return 0;
#undef MEM_BULK
}

/**
 * Get resource name.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         name location
 * @param id            resource instance name
 *
 * @return Status code
 */
static te_errno
rsrc_get(unsigned int gid, const char *oid, char *value, const char *id)
{
    rsrc *tmp;
    te_errno rc;

    UNUSED(gid);
    UNUSED(oid);

    rc = rsrc_find_by_id(id, &tmp);
    if (rc != 0)
        return rc;

    te_strlcpy(value, tmp->name == NULL ? "" : tmp->name, RCF_MAX_VAL);
    return 0;
}

/**
 * Convert resource name to generic resource name.
 *
 * @param name  resource name
 *
 * @return Generic name or NULL in the case of incorrect name
 *
 * @note non-reenterable
 */
static char *
rsrc_gen_name(const char *name)
{
    static char buf[CFG_OID_MAX];

    *buf = 0;

    if (name == NULL)
        return NULL;

    if (strchr(name, '/') == NULL || strchr(name, ':') == NULL)
        return (char *)name;

    cfg_oid_inst2obj(name, buf);
    if (*buf == 0)
        return NULL;

    return buf;
}

/**
 * Lock/unlock resource.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         resource name
 * @param id            instance name
 *
 * @return Status code
 */
static te_errno
rsrc_set(unsigned int gid, const char *oid, const char *value,
         const char *id)
{
    char *name_to_set = NULL;
    const char *rsrc_name;
    rsrc_info *info;
    te_errno rc;
    rsrc *tmp;

    UNUSED(gid);
    UNUSED(oid);

    rc = rsrc_find_by_id(id, &tmp);
    if (rc != 0)
        return rc;

    if (tmp->name == NULL && *value == '\0')
        return 0;

    if (tmp->name != NULL && *value != '\0')
    {
        ERROR("Cannot change resource '%s' value from '%s' to '%s'",
              tmp->id, tmp->name, value);
        return TE_RC(TE_RCF_PCH, TE_EINVAL);
    }

    rsrc_name = tmp->name != NULL ? tmp->name : value;

    if ((info = rsrc_lookup(rsrc_gen_name(rsrc_name))) == NULL)
    {
        ERROR("Unknown resource '%s'", rsrc_name);
        return TE_RC(TE_RCF_PCH, TE_ENOENT);
    }

    if (*value != '\0')
    {
        if (rcf_pch_rsrc_accessible_may_share("%s", value))
            return TE_RC(TE_RCF_PCH, TE_EEXIST);

        name_to_set = strdup(value);
        if (name_to_set == NULL)
            return TE_RC(TE_RCF_PCH, TE_ENOMEM);

#ifndef __CYGWIN__
        if ((rc = ta_rsrc_create_lock(value, &tmp->shared,
                                      tmp->fallback_shared,
                                      tmp->attempts_timeout_ms)) != 0)
        {
            free(name_to_set);
            return rc;
        }
#endif

        if ((rc = info->grab(value)) != 0)
        {
#ifndef __CYGWIN__
            ta_rsrc_delete_lock(value);
#endif
            free(name_to_set);
            return rc;
        }
    }
    else
    {
#ifndef __CYGWIN__
        ta_rsrc_delete_lock(tmp->name);
#endif
        rc = info->release(tmp->name);
        if (rc != 0)
            return rc;
    }

    free(tmp->name);
    tmp->name = name_to_set;

    return 0;
}

/**
 * Delete resource.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param id            resource instance identifier
 *
 * @return Status code
 */
static te_errno
rsrc_del(unsigned int gid, const char *oid, const char *id)
{
    rsrc *cur, *prev;

    UNUSED(gid);
    UNUSED(oid);

    for (cur = rsrc_lst, prev = NULL;
         cur != NULL;
         prev = cur, cur = cur->next)
    {
        if (strcmp(id, cur->id) == 0)
        {
            rsrc_info *info;
            int        rc;

            if (cur->name != NULL)
            {
                if ((info = rsrc_lookup(rsrc_gen_name(cur->name))) == NULL)
                {
                    ERROR("Resource structures of RCFPCH are corrupted");
                    return TE_RC(TE_RCF_PCH, TE_EFAIL);
                }

                if (info->release == NULL)
                {
                    ERROR("Cannot release the resource %s: release callback "
                          "is not provided", cur->name);
                    return TE_RC(TE_RCF_PCH, TE_EPERM);
                }

                if ((rc = info->release(cur->name)) != 0)
                    return rc;

#ifndef __CYGWIN__
                ta_rsrc_delete_lock(cur->name);
#endif
            }

            if (prev != NULL)
                prev->next = cur->next;
            else
                rsrc_lst = cur->next;

            free(cur->name);
            free(cur->id);
            free(cur);
            return 0;
        }
    }

    return TE_RC(TE_RCF_PCH, TE_ENOENT);
}

/**
 * Add a resource.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         resource name
 * @param id            instance name
 *
 * @return Status code
 */
static te_errno
rsrc_add(unsigned int gid, const char *oid, const char *value,
         const char *id)
{
    rsrc *tmp = NULL;

    if (rsrc_find_by_id(id, NULL) == 0)
        return TE_RC(TE_RCF_PCH, TE_EEXIST);

    if ((tmp = calloc(sizeof(*tmp), 1)) == NULL ||
        (tmp->id = strdup(id)) == NULL)
    {
        if (tmp != NULL)
        {
            free(tmp->id);
            free(tmp);
        }

        return TE_RC(TE_RCF_PCH, TE_ENOMEM);
    }

    tmp->next = rsrc_lst;
    rsrc_lst = tmp;

    /* Resource is locked only if the resource name is set */
    if (*value != '\0')
    {
        te_errno rc;

        rc = rsrc_set(gid, oid, value, id);
        if (rc != 0)
        {
            rsrc_del(gid, oid, id);
            return rc;
        }
    }

    return 0;
}

/**
 * Check if the resource is accessible with given shared/exclusive access right.
 *
 * @param fmt           format string for resource name
 * @param shared     check for accessibility
 *                      in shared/exclusive (@c TRUE)
 *                      or only exclusive (@c FALSE) mode.
 *
 * @return  TRUE is the resource is accessible with given shared/exclusive
 *          access right
 *
 * @note The function should be called from TA main thread only.
 */
static te_bool
rsrc_accessible_generic(te_bool shared, const char *fmt, va_list ap)
{
    rsrc   *tmp;
    char    buf[RCF_MAX_VAL];

    if (fmt == NULL)
        return FALSE;

    if (vsnprintf(buf, sizeof(buf), fmt, ap) >= (int)sizeof(buf))
    {
        ERROR("Too long resource name");
        return FALSE;
    }

    for (tmp = rsrc_lst; tmp != NULL; tmp = tmp->next)
    {
        VERB("%s(): check '%s'(%d) vs '%s'(%d)", __FUNCTION__,
             tmp->name, tmp->shared, buf, shared);
        if (tmp->name != NULL && strcmp(tmp->name, buf) == 0 &&
            !(tmp->shared && !shared))
        {
            VERB("%s(): match", __FUNCTION__);
            return TRUE;
        }
    }
    VERB("%s(): no match", __FUNCTION__);

    return FALSE;
}

te_bool
rcf_pch_rsrc_accessible(const char *fmt, ...)
{
    va_list ap;
    te_bool result;

    va_start(ap, fmt);
    result = rsrc_accessible_generic(FALSE, fmt, ap);
    va_end(ap);

    return result;
}

te_bool
rcf_pch_rsrc_accessible_may_share(const char *fmt, ...)
{
    va_list ap;
    te_bool result;

    va_start(ap, fmt);
    result = rsrc_accessible_generic(TRUE, fmt, ap);
    va_end(ap);

    return result;
}

/*
 * Get shared property of a resource.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         property location
 * @param id            resource instance name
 *
 * @return Status code
 */
static te_errno
rsrc_shared_get(unsigned int gid, const char *oid, char *value,
                   const char *id)
{
    te_errno rc;
    rsrc *tmp;

    UNUSED(gid);
    UNUSED(oid);

    rc = rsrc_find_by_id(id, &tmp);
    if (rc != 0)
        return rc;

    te_strlcpy(value, tmp->shared ? "1" : "0", RCF_MAX_VAL);
    return 0;
}

/*
 * Set shared property of a resource.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         property value
 * @param id            instance name
 *
 * @return Status code
 */
static te_errno
rsrc_shared_set(unsigned int gid, const char *oid, const char *value,
                   const char *id)
{
    te_bool shared;
    te_errno rc;
    rsrc *tmp;

    UNUSED(gid);
    UNUSED(oid);

    rc = te_strtol_bool(value, &shared);
    if (rc != 0)
        return rc;

    rc = rsrc_find_by_id(id, &tmp);
    if (rc != 0)
        return rc;

    if (tmp->name != NULL)
    {
        rc = ta_rsrc_create_lock(tmp->name, &shared, tmp->fallback_shared,
                                 tmp->attempts_timeout_ms);
        if (rc != 0)
            return rc;
    }

    tmp->shared = shared;

    return 0;
}

static unsigned int *
rsrc_property_ptr_by_oid(const char *oid, const char *id)
{
    cfg_oid *coid = cfg_convert_oid_str(oid);
    unsigned int *result = NULL;
    char *prop_subid;
    rsrc *tmp;

    if (coid == NULL)
        goto exit;

    if (rsrc_find_by_id(id, &tmp) != 0)
        goto exit;

    prop_subid = cfg_oid_inst_subid(coid, 3);
    if (prop_subid == NULL)
        goto exit;

    if (strcmp(prop_subid, "acquire_attempts_timeout") == 0)
        result = &tmp->attempts_timeout_ms;
    else if (strcmp(prop_subid, "fallback_shared") == 0)
        result = &tmp->fallback_shared;

exit:
    cfg_free_oid(coid);

    if (result == NULL)
        ERROR("Failed to get property by oid '%s'", oid);

    return result;
}

static te_errno
rsrc_property_set(unsigned int gid, const char *oid, const char *value,
                  const char *resource)
{
    unsigned int *property;
    unsigned int property_value;

    UNUSED(gid);

    property = rsrc_property_ptr_by_oid(oid, resource);
    if (property == NULL)
        return TE_RC(TE_RCF_PCH, TE_ENOENT);

    if (te_strtoui(value, 0, &property_value) != 0)
        return TE_RC(TE_RCF_PCH, TE_EINVAL);

    *property = property_value;

    return 0;
}

static te_errno
rsrc_property_get(unsigned int gid, const char *oid, char *value,
                  const char *resource)
{
    unsigned int *property;

    UNUSED(gid);

    property = rsrc_property_ptr_by_oid(oid, resource);
    if (property == NULL)
        return TE_RC(TE_RCF_PCH, TE_ENOENT);

    te_snprintf(value, RCF_MAX_VAL, "%u", *property);

    return 0;
}

static rcf_pch_cfg_object node_rsrc_acquire_timeout =
    { "acquire_attempts_timeout", 0, NULL, NULL,
      (rcf_ch_cfg_get)rsrc_property_get,
      (rcf_ch_cfg_set)rsrc_property_set,
      NULL, NULL, NULL, NULL, NULL, NULL };

static rcf_pch_cfg_object node_rsrc_fallback_shared =
    { "fallback_shared", 0, NULL, &node_rsrc_acquire_timeout,
      (rcf_ch_cfg_get)rsrc_property_get,
      (rcf_ch_cfg_set)rsrc_property_set,
      NULL, NULL, NULL, NULL, NULL, NULL };

/** Resource's shared property node */
static rcf_pch_cfg_object node_rsrc_shared =
    { "shared", 0, NULL, &node_rsrc_fallback_shared,
      (rcf_ch_cfg_get)rsrc_shared_get, (rcf_ch_cfg_set)rsrc_shared_set,
      NULL, NULL, NULL, NULL, NULL, NULL };

/** Resource node */
static rcf_pch_cfg_object node_rsrc =
    { "rsrc", 0, &node_rsrc_shared, NULL,
      (rcf_ch_cfg_get)rsrc_get, (rcf_ch_cfg_set)rsrc_set,
      (rcf_ch_cfg_add)rsrc_add, (rcf_ch_cfg_del)rsrc_del,
      (rcf_ch_cfg_list)rsrc_list, NULL, NULL, NULL };

/**
 * Link resource configuration tree.
 */
void
rcf_pch_rsrc_init(void)
{
    rcf_pch_add_node("/agent", &node_rsrc);
}
