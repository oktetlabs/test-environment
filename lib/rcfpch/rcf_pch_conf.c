/** @file
 * @brief RCF Portable Command Handler
 *
 * Default configuration command handler implementation.
 *
 *
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 *
 * @author Elena A. Vengerova <Elena.Vengerova@oktetlabs.ru>
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
 */

#include "te_config.h"

#include <stdio.h>
#ifdef STDC_HEADERS
#include <stdlib.h>
#include <string.h>
#endif
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#ifdef HAVE_ASSERT_H
#include <assert.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_QUEUE_H
#include <sys/queue.h>
#endif

#include "te_errno.h"
#include "te_defs.h"
#include "te_stdint.h"
#include "comm_agent.h"
#include "conf_oid.h"
#include "rcf_common.h"
#include "rcf_pch.h"
#include "rcf_ch_api.h"
#include "rcf_pch_internal.h"

#define OID_ETC "/..."

/** Structure for temporary storing of instances/objects identifiers */
typedef struct olist {
    struct olist *next;             /**< Pointer to the next element */
    char          oid[RCF_MAX_ID];  /**< Element OID */
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
 * @return error code
 * @retval 0            success
 * @retval EINVAL       invalid identifier
 * @retval ENOMEM       malloc() failed
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
                return ENOMEM;
                
            *next_level = oid;
            return 0;
        }
        if (*oid++ != '/')
           return EINVAL;

        if ((*next_level = strchr(oid, '/')) == NULL)
        {
            *next_level = oid + strlen(oid);
            c = 0;
        }
        **next_level = 0;
        if ((*sub_id = strdup(oid)) == NULL)
            return ENOMEM;
        **next_level = c;

        if ((strchr(*sub_id, '*') != NULL && strlen(*sub_id) > 1))
        {
            free(*sub_id);
            return EINVAL;
        }

        return 0;
    }

    if (strcmp(oid, "*:*") == 0 || strcmp(oid, OID_ETC) == 0)
    {
        if ((*sub_id = strdup(oid)) == NULL)
            return ENOMEM;
            
        if ((*inst_name = strdup(oid)) == NULL)
        {
            free(sub_id);
            return ENOMEM;
        }
        *next_level = oid;
        return 0;
    }

    if (*oid != '/')
        return EINVAL;

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
            return ENOMEM;
        if ((*inst_name = strdup(tmp + 1)) == NULL)
        {
            free(sub_id);
            return ENOMEM;
        }
        *tmp = ':';
    }
    else
    {
        if (strcmp(oid, "*") != 0)
            return EINVAL;

        if ((*sub_id = strdup("*")) == NULL)
            return ENOMEM;
        if ((*inst_name = strdup("*")) == NULL)
        {
            free(sub_id);
            return ENOMEM;
        }
    }

    **next_level = c;

    if ((strchr(*sub_id, '*') != NULL && strlen(*sub_id) > 1) ||
        (strchr(*inst_name, '*') != NULL && strlen(*inst_name) > 1) ||
        (**sub_id == '*' && **inst_name != '*'))
    {
        free(*sub_id);
        free(*inst_name);
        return EINVAL;
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
 * @return error code
 * @retval 0            success
 * @retval EINVAL       invalid identifier
 * @retval ENOMEM       malloc() failed
 */
static int
create_wildcard_inst_list(rcf_pch_cfg_object *obj, char *parsed, char *oid,
                          const char *full_oid, olist **list)
{
    char *sub_id = NULL;
    char *inst_name = NULL;
    char *next_level;
    
    te_bool all;
    
/**
 * Return from function with resources deallocation.
 *
 * @param _rc   - return code
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
        RET(EINVAL);
        
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
                RET(ENOMEM);
        }
        else
        {
            char *dup = parsed == NULL ? NULL : strdup(parsed);
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

            if ((obj->list)(gid, parsed, &tmp_list,
                            inst_names[0], inst_names[1], inst_names[2],
                            inst_names[3], inst_names[4], inst_names[5],
                            inst_names[6], inst_names[7], inst_names[8],
                            inst_names[9]) != 0 || tmp_list == NULL)
            {
                free(dup);
                RET(0);
            }
            free(dup);
        }

        for (tmp_inst_name = tmp_list;
             strlen(tmp_inst_name) > 0;
             tmp_inst_name = tmp)
        {
            int   len;
            int   rc = 0;
            char *tmp_parsed;
            
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

            len = strlen(obj->sub_id) + strlen(tmp_inst_name) + 3;
            tmp_parsed = (char *)
                calloc(1, len + (parsed == NULL ? 0 : strlen(parsed)));

            sprintf(tmp_parsed, "%s/%s:%s", parsed == NULL ? "" : parsed,
                    obj->sub_id, tmp_inst_name);

            if (*next_level == 0 || all)
            {
                olist *new_entry;

                if ((new_entry = (olist *)malloc(sizeof(olist))) == NULL)
                {
                    free(tmp_parsed);
                    free(tmp_list);
                    RET(ENOMEM);
                }

                strcpy(new_entry->oid, tmp_parsed);

                new_entry->next = *list;
                *list = new_entry;
            }

            if (obj->son != NULL && *next_level != 0)
                rc = create_wildcard_inst_list(obj->son, tmp_parsed,
                                               next_level, full_oid, list);
            free(tmp_parsed);

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
 * @return error code
 * @retval 0            success
 * @retval EINVAL       invalid identifier
 * @retval ENOMEM       malloc() failed
 */
static int
create_wildcard_obj_list(rcf_pch_cfg_object *obj, char *parsed, char *oid,
                         const char *full_oid, olist **list)
{
    char *next_level;
    char *sub_id = NULL;
    
    te_bool all;

/**
 * Return from function with resources deallocation.
 *
 * @param _rc   - return code
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
        RET(EINVAL);
      
    all = *full_oid =='*' || strcmp(sub_id, OID_ETC) == 0;

    for ( ; obj != NULL; obj = obj->brother)
    {
        int   len;
        int   rc = 0;
        char *tmp_parsed;

        if (!all && strcmp(obj->sub_id, sub_id) != 0)
            continue;

        len = strlen(obj->sub_id) + 2;
        tmp_parsed = (char *)calloc(1, len +
            ((parsed == NULL) ? 0 : strlen(parsed)));
        sprintf(tmp_parsed, "%s/%s",
                parsed == NULL ? "" : parsed, obj->sub_id);

        if (*next_level == 0 || all)
        {
            olist *new_entry;

            if ((new_entry = (olist *)malloc(sizeof(olist))) == NULL)
            {
                free(tmp_parsed);
                RET(ENOMEM);
            }

            strcpy(new_entry->oid, tmp_parsed);
            new_entry->next = *list;
            *list = new_entry;
        }

        if (obj->son != NULL && *next_level != 0)
            rc = create_wildcard_obj_list(obj->son, tmp_parsed,
                                          next_level, full_oid, list);

        free(tmp_parsed);
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
 * @param list      - list of identifiers to be converted
 * @param answer    - location for the answer string address (memory
 *                    is allocated using malloc())
 *
 * @return error code
 * @retval 0        - success
 * @retval ENOMEM   - memory allocation failed
 */
static int
convert_to_answer(olist *list, char **answer)
{
    olist  *tmp;
    size_t  len = 1;
    char   *ptr;


    if (list == NULL)
    {
        *answer = strdup("");
        return (*answer == NULL) ? ENOMEM : 0;
    }

    for (tmp = list; tmp != NULL; tmp = tmp->next)
    {
        len += strlen(tmp->oid) + 1;
    }

    if ((*answer = (char *)malloc(len)) == NULL)
    {
        free_list(list);
        return ENOMEM;
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
 * @param conn          - connection handle
 * @param cbuf          - command buffer
 * @param buflen        - length of the command buffer
 * @param answer_plen   - number of bytes in the command buffer
 *                        to be copied to the answer
 *
 * @param oid           - object or instance identifier
 *
 *
 * @return 0 or error returned by communication library
 */
static int
process_wildcard(struct rcf_comm_connection *conn, char *cbuf,
                 size_t buflen, size_t answer_plen, const char *oid)
{
    int    rc;
    char   copy[RCF_MAX_ID];
    char  *tmp;
    olist *list = NULL;

    ENTRY("OID='%s'", oid);
    VERB("Process wildcard request");
    
    strcpy(copy, oid);
    if (strchr(oid, ':') == NULL)
    {
        VERB("Create list of objects by wildcard");
        rc = create_wildcard_obj_list(rcf_ch_conf_root(),
                                      NULL, copy, oid, &list);
    }
    else
    {
        VERB("Create list of instances by wildcard");
        
        rc = create_wildcard_inst_list(rcf_ch_conf_root(),
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
        SEND_ANSWER("%d", TE_RC(TE_RCF_PCH, E2BIG));
    }

    rcf_ch_lock();
    rc = rcf_comm_agent_reply(conn, cbuf, strlen(cbuf) + 1);
    VERB("Sent answer to wildcard request '%s' len=%u rc=%d",
         cbuf,  strlen(cbuf) + 1, rc);
    if (rc == 0)
    {
        rc = rcf_comm_agent_reply(conn, tmp, strlen(tmp) + 1);
        VERB("Sent binary attachment len=%u rc=%d",
                             strlen(tmp) + 1, rc);
    }
    rcf_ch_unlock();
    
    free(tmp);

    return rc;
}


/**
 * Find postponed commit operation with specified parameters.
 *
 * @param f_commit  - commit function
 * @param p_oid     - configurator object identfier
 *
 * @return Pointer to found commit operation or NULL.
 */
static rcf_pch_commit_op_t *
find_commit_op(rcf_ch_cfg_commit f_commit, const cfg_oid *p_oid)
{
    rcf_pch_commit_op_t *p;

    for (p = commits.tqh_first;
         (p != NULL) &&
         ((p->func != f_commit) || (cfg_oid_cmp(p->oid, p_oid) != 0));
         p = p->links.tqe_next);

    return p;
}
        
/**
 * Immediate or postponed commit of changes.
 *
 * @param commit_obj    - configuration tree object with commit function
 * @param pp_oid        - pointer to pointer to configuration OID (IN/OUT)
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
                return ENOMEM;
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
    int                     rc = 0, ret;
    rcf_pch_commit_op_t    *p;

    ENTRY();
    VERB("Postponed commit of group %u", gid);
    while ((p = commits.tqh_first) != NULL)
    {
        TAILQ_REMOVE(&commits, p, links);

        ret = p->func(gid, p->oid);
        if (ret != 0)
        {
            ERROR("Commit failed: error=%d", ret);
            if (rc == 0)
                rc = ret;
        }
        cfg_free_oid(p->oid);
        free(p);
    }
    EXIT("%d", rc);
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

    if (rcf_ch_conf_root() != NULL)
    {
        /*
         * Agent root OID has length equal to 2, because of root OID
         * existence with empty subid and name.
         */
        rcf_pch_cfg_subtree_init(rcf_ch_conf_root(), 2);
    }
}


/* See description in rcf_ch_api.h */
int
rcf_pch_agent_list(unsigned int id, const char *oid, char **list)
{
    UNUSED(id);
    UNUSED(oid);

    return (*list = strdup(rcf_ch_conf_agent())) == NULL ? ENOMEM : 0;
}


/* See description in rcf_ch_api.h */
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
                SEND_ANSWER("%d", TE_RC(TE_RCF_PCH, EINVAL));
            }

            rc = process_wildcard(conn, cbuf, buflen, answer_plen, oid);

            EXIT("%d", rc);

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
            SEND_ANSWER("%d", TE_RC(TE_RCF_PCH, ETEBADFORMAT));
        }
        if (!p_oid->inst)
        {
            cfg_free_oid(p_oid);
            ERROR("Instance identifier expected");
            SEND_ANSWER("%d", TE_RC(TE_RCF_PCH, EINVAL));
        }
        if (p_oid->len == 0)
        {
            cfg_free_oid(p_oid);
            ERROR("Zero length OIID");
            SEND_ANSWER("%d", TE_RC(TE_RCF_PCH, EINVAL));
        }

        memset(inst_names, 0, sizeof(inst_names));
        p_ids = (cfg_inst_subid *)(p_oid->ids);

        for (i = 1, obj = 0, next = rcf_ch_conf_root();
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
            SEND_ANSWER("%d", TE_RC(TE_RCF_PCH, ETENOSUCHNAME));
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

            cfg_free_oid(p_oid);

            if (rc == 0)
            {
                write_str_in_quotes(ret_val, value, RCF_MAX_VAL);
                SEND_ANSWER("0 %s", ret_val);
            }
            else
            {
                SEND_ANSWER("%d", TE_RC(TE_RCF_PCH, rc));
            }
            break;
        }

        case RCF_CH_CFG_SET:
            rc = (obj->set == NULL) ? EOPNOTSUPP :
                     (obj->set)(gid, oid, val, ALL_INST_NAMES);
            if ((rc == 0) && (commit_obj->commit != NULL))
            {
                rc = commit(commit_obj, &p_oid);
            }
            cfg_free_oid(p_oid);
            SEND_ANSWER("%d", TE_RC(TE_RCF_PCH, rc));
            break;

        case RCF_CH_CFG_ADD:
            rc = (obj->add == NULL) ? EOPNOTSUPP :
                     (obj->add)(gid, oid, val, ALL_INST_NAMES);
            if ((rc == 0) && (commit_obj->commit != NULL))
            {
                rc = commit(commit_obj, &p_oid);
            }
            cfg_free_oid(p_oid);
            SEND_ANSWER("%d", TE_RC(TE_RCF_PCH, rc));
            break;

        case RCF_CH_CFG_DEL:
            rc = (obj->del == NULL) ? EOPNOTSUPP :
                     (obj->del)(gid, oid, ALL_INST_NAMES);
            if ((rc == 0) && (commit_obj->commit != NULL))
            {
                rc = commit(commit_obj, &p_oid);
            }
            cfg_free_oid(p_oid);
            SEND_ANSWER("%d", TE_RC(TE_RCF_PCH, rc));
            break;

        default:
            assert(FALSE);
    }

    /* Unreachable */
    assert(FALSE);
    return 0;

#undef ALL_INST_NAMES
}
