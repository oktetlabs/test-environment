/** @file
 * @brief Configurator
 *
 * Support of dynamic history
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
 *
 *
 * @author Elena Vengerova <Elena.Vengerova@oktetlabs.ru>
 *
 * $Id$
 */

#include "te_alloc.h"
#include "conf_defs.h"
#define TE_EXPAND_XML 1
#include "te_expand.h"

/* defined in conf_backup.c */
extern int cfg_register_dependency(xmlNodePtr node, const char *dependant);

/** Backup descriptor */
typedef struct cfg_backup {
    struct cfg_backup *next; /**< Next backup associated with this point */
    char              *filename; /**< backup filename */
} cfg_backup;

/** Configurator dynamic history entry */
typedef struct cfg_dh_entry {
    struct cfg_dh_entry *next;    /**< Link to the next entry */
    struct cfg_dh_entry *prev;    /**< Link to the previous entry */
    cfg_backup          *backup;  /**< List of associated backups */
    cfg_msg             *cmd;     /**< Register, add, delete or set
                                       command */
    char                *old_oid; /**< OID for delete reversing */
    cfg_val_type         type;    /**< Type of the old_val */
    cfg_inst_val         old_val; /**< Data for reversing delete and set */
    int                  seq;     /**< Sequence number for debugging */
    te_bool              committed; /**< Wheter the command kept in this
                                         entry is committed or not */
} cfg_dh_entry;

static cfg_dh_entry *first = NULL;
static cfg_dh_entry *last = NULL;
static cfg_backup   *begin_backup = NULL;

/** Release memory allocated for backup list */
static inline void
free_entry_backup(cfg_dh_entry *entry)
{
    cfg_backup *tmp;

    for (tmp = entry->backup; tmp != NULL; tmp = entry->backup)
    {
        entry->backup = entry->backup->next;
        free(tmp->filename);
        free(tmp);
    }
}

/** Release memory allocated for dynamic history entry */
static inline void
free_dh_entry(cfg_dh_entry *entry)
{
    if (entry->type != CVT_NONE)
        cfg_types[entry->type].free(entry->old_val);

    free_entry_backup(entry);

    if (entry->old_oid != NULL)
        free(entry->old_oid);
    free(entry->cmd);
    free(entry);
}

/**
 * Skip 'comment' nodes.
 *
 * @param node      XML node
 *
 * @return Current, one of next or NULL.
 */
static xmlNodePtr
xmlNodeSkipExtra(xmlNodePtr node)
{
    while ((node != NULL) &&
           ((xmlStrcmp(node->name, (const xmlChar *)("comment")) == 0) ||
            (xmlStrcmp(node->name, (const xmlChar *)("text")) == 0)))
    {
        node = node->next;
    }
    return node;
}

/**
 * Go to the next XML node, skip 'comment' nodes.
 *
 * @param node      XML node
 *
 * @return The next XML node.
 */
static xmlNodePtr
xmlNodeChildren(xmlNodePtr node)
{
    assert(node != NULL);
    return xmlNodeSkipExtra(node->children);
}

/**
 * Go to the next XML node, skip 'comment' nodes.
 *
 * @param node      XML node
 *
 * @return The next XML node.
 */
static xmlNodePtr
xmlNodeNext(xmlNodePtr node)
{
    assert(node != NULL);
    return xmlNodeSkipExtra(node->next);
}

/**
 * Get the value of the attribute @b "cond"
 *
 * @param node      XML node
 *
 * @return @c FALSE in case attribute @b cond exists and it equals
 *         @b "false" or empty line. @c TRUE in other cases.
 */
static te_bool
xmlNodeCond(xmlNodePtr node)
{
    te_bool result = TRUE;
    char *cond = xmlGetProp_exp(node, (xmlChar *)"cond");

    if (cond == NULL)
        return TRUE;

    if (strcmp(cond, "false") == 0 || strcmp(cond, "") == 0)
        result = FALSE;
    xmlFree((xmlChar *)cond);

    return result;
}

#define RETERR(_rc, _str...)    \
    do {                        \
        int _err = _rc;         \
                                \
        ERROR(_str);            \
        xmlFree(oid);           \
        xmlFree(val_s);         \
        free(msg);              \
        return _err;            \
    } while (0)

/**
 * Parse handle, oid and object of instance
 * and check that object instance has value
 *
 * @param node          XML node
 * @param handle        instance handle
 * @oaram oid           instance OID
 * @param obj           object of instance
 * @param expand_vars   List of key-value pairs for expansion in file,
 *                      @c NULL if environment variables are used for
 *                      substitutions
 *
 * @return  Status code
 */
static te_errno
cfg_dh_get_instance_info(xmlNodePtr node, cfg_handle *handle,
                          char **oid, cfg_object **obj,
                          const te_kvpair_h *expand_vars)
{
    te_errno rc = 0;

    *oid = xmlGetProp_exp_vars_or_env(node,
          (xmlChar *)"oid", expand_vars);
    if (*oid == NULL)
    {
        ERROR("Incorrect command format");
        rc = TE_EINVAL;
        goto cleanup;
    }
    if (cfg_db_find(*oid, handle) != 0)
    {
        ERROR("Cannot find instance %s", *oid);
        rc = TE_ENOENT;
        goto cleanup;
    }

    if (!CFG_IS_INST(*handle))
    {
        ERROR("OID %s is not instance", *oid);
        rc = TE_EINVAL;
        goto cleanup;
    }

    *obj = CFG_GET_INST(*handle)->obj;
    if ((*obj)->type == CVT_NONE)
    {
        ERROR("Object instance has no value");
        rc = TE_EINVAL;
        goto cleanup;
    }

    return 0;

cleanup:
    free(*oid);
    *oid = NULL;

    return rc;
}

/**
 * Get value from instance and store it in either list of kvpairs or
 * environmental variable.
 *
 * @param node          XML node with get command children
 * @param expand_vars   List of key-value pairs for expansion in file
 *                      NULL if environmental variables used for
 *                      substitutions
 *
 * @return Status code.
 */
static te_errno
cfg_dh_get_value_from_instance(xmlNodePtr node, te_kvpair_h *expand_vars)
{
    te_errno     rc;
    int          ret;
    size_t       len;
    char        *oid = NULL;
    char        *var_name = NULL;
    char        *get_val_s = NULL;
    cfg_get_msg *msg = NULL;
    cfg_inst_val get_val;
    cfg_handle   handle;
    cfg_object  *obj;


    rc = cfg_dh_get_instance_info(node, &handle, &oid,
                                   &obj, expand_vars);
    if (rc != 0)
        return rc;

    var_name = xmlGetProp_exp_vars_or_env(node,
            (const xmlChar *)"value", expand_vars);
    if (var_name == NULL)
    {
        ERROR("Value is required for %s", oid);
        rc = TE_EINVAL;
        goto cleanup;
    }

    len = sizeof(cfg_get_msg) + CFG_MAX_INST_VALUE;
    if ((msg = TE_ALLOC(len)) == NULL)
    {
        rc = TE_ENOMEM;
        goto cleanup;
    }

    msg->handle = handle;
    msg->type = CFG_GET;
    msg->len = sizeof(cfg_get_msg);
    msg->val_type = obj->type;

    cfg_process_msg((cfg_msg **)&msg, TRUE);

    if (msg->rc != 0)
    {
        ERROR("Failed to execute the get command for instance %s", oid);
        rc = msg->rc;
        goto cleanup;
    }

    rc = cfg_types[msg->val_type].get_from_msg((cfg_msg *)msg, &get_val);
    if (rc != 0)
    {
        ERROR("Cannot extract value from message for %s", oid);
        goto cleanup;
    }
    rc = cfg_types[msg->val_type].val2str(get_val, &get_val_s);
    cfg_types[obj->type].free(get_val);
    if (rc != 0)
    {
        ERROR("Cannot convert value to string for %s", oid);
        goto cleanup;
    }

    if (expand_vars != NULL)
    {
        rc = te_kvpair_add(expand_vars, var_name, get_val_s);
        if (rc != 0)
        {
            ERROR("Failed to add new entry in list of kvpairs: %r", rc);
            goto cleanup;
        }
    }
    else
    {
        ret = setenv(var_name, get_val_s, 1);
        if (ret != 0)
        {
            ERROR("Failed to put value in environmental variable");
            rc = TE_OS_RC(TE_CS, errno);
            goto cleanup;
        }
    }

cleanup:
    free(var_name);
    free(get_val_s);
    free(msg);
    free(oid);

    return rc;
}

/**
 * Process 'get' command.
 *
 * @param node          XML node with get command children
 * @param expand_vars   List of key-value pairs for expansion in file
 *                      NULL if environmental variables used for
 *                      substitutions
 *
 * @return Status code.
 */
static te_errno
cfg_dh_process_get(xmlNodePtr node, te_kvpair_h *expand_vars)
{
    te_errno rc = 0;

    while (node != NULL &&
           xmlStrcmp(node->name , (const xmlChar *)"instance") == 0)
    {
        if (xmlNodeCond(node))
        {
            rc = cfg_dh_get_value_from_instance(node, expand_vars);
            if (rc != 0)
                return rc;
        }
        node = xmlNodeNext(node);
    }

    if (node != NULL)
    {
        rc = TE_EINVAL;
        ERROR("Incorrect get command format");
    }

    return rc;
}

/**
 * Process 'copy' command.
 *
 * @param node          XML node with get command children
 * @param expand_vars   List of key-value pairs for expansion in file
 *                      NULL if environmental variables used for
 *                      substitutions
 *
 * @return Status code.
 */
static te_errno
cfg_dh_process_copy(xmlNodePtr node, te_kvpair_h *expand_vars)
{
    te_errno      rc;
    size_t        len;
    size_t        oid_len;
    char         *oid = NULL;
    char         *val_s = NULL;
    cfg_copy_msg *msg = NULL;
    cfg_handle    src_handle;

    while (node != NULL &&
           xmlStrcmp(node->name , (const xmlChar *)"instance") == 0)
    {
        if (!xmlNodeCond(node))
        {
            node = xmlNodeNext(node);
            continue;
        }

        oid = xmlGetProp_exp_vars_or_env(node,
               (xmlChar *)"oid", expand_vars);
        if (oid == NULL)
        {
            ERROR("Incorrect command format");
            rc = TE_EINVAL;
            goto cleanup;
        }

        oid_len = strlen(oid);

        len = sizeof(cfg_copy_msg) + oid_len + 1;
        if ((msg = calloc(1, len)) == NULL)
        {
            ERROR("Cannot allocate memory");
            rc = TE_ENOMEM;
            goto cleanup;
        }

        memcpy(msg->dst_oid, oid, oid_len + 1);
        msg->is_obj = (strchr(oid, ':') == NULL) ? TRUE : FALSE;

        msg->type = CFG_COPY;
        msg->len = sizeof(cfg_copy_msg) + oid_len + 1;

        val_s = xmlGetProp_exp_vars_or_env(node,
                (const xmlChar *)"value", expand_vars);
        if (val_s == NULL)
        {
            ERROR("Value is required for %s to copy from", oid);
            rc = TE_EINVAL;
            goto cleanup;
        }

        if ((rc = cfg_db_find(val_s, &src_handle)) != 0)
            goto cleanup;

        msg->src_handle = src_handle;

        cfg_process_msg((cfg_msg **)&msg, TRUE);
        if (msg->rc != 0)
        {
            ERROR("Failed to execute the copy command for instance %s", oid);
            rc = msg->rc;
            goto cleanup;
        }
        free(val_s);
        val_s = NULL;
        free(msg);
        msg = NULL;
        free(oid);
        oid = NULL;

        node = xmlNodeNext(node);
    }
    if (node != NULL)
    {
        ERROR("Incorrect copy command format");
        rc = TE_EINVAL;
    }

cleanup:
    free(oid);
    free(val_s);
    free(msg);

    return rc;
}
/**
 * Process 'add' command.
 *
 * @param node          XML node with add command children
 * @param expand_vars   List of key-value pairs for expansion in file,
 *                      @c NULL if environment variables are used for
 *                      substitutions
 *
 * @return Status code.
 */
static int
cfg_dh_process_add(xmlNodePtr node, const te_kvpair_h *expand_vars)
{
    int          rc;
    size_t       len;
    cfg_add_msg *msg = NULL;
    cfg_inst_val val;
    cfg_object  *obj;
    xmlChar     *oid = NULL;
    xmlChar     *val_s = NULL;

    while (node != NULL &&
           xmlStrcmp(node->name, (const xmlChar *)"instance") == 0)
    {
        if (!xmlNodeCond(node))
            goto next;

        oid = (xmlChar *)xmlGetProp_exp_vars_or_env(node,
              (const xmlChar *)"oid", expand_vars);
        if (oid == NULL)
            RETERR(TE_EINVAL, "Incorrect add command format");

        if ((obj = cfg_get_object((char *)oid)) == NULL)
            RETERR(TE_EINVAL, "Cannot find object for instance %s", oid);

        len = sizeof(cfg_add_msg) + CFG_MAX_INST_VALUE +
              strlen((char *)oid) + 1;
        if ((msg = (cfg_add_msg *)calloc(1, len)) == NULL)
            RETERR(TE_ENOMEM, "Cannot allocate memory");

        msg->type = CFG_ADD;
        msg->len = sizeof(cfg_add_msg);
        msg->val_type = obj->type;
        msg->rc = 0;

        val_s = (xmlChar *)xmlGetProp_exp_vars_or_env(node,
                (const xmlChar *)"value", expand_vars);
        if (val_s != NULL && strlen((char *)val_s) >= CFG_MAX_INST_VALUE)
            RETERR(TE_ENOMEM, "Too long value");

        if (obj->type == CVT_NONE && val_s != NULL)
            RETERR(TE_EINVAL, "Value is prohibited for %s", oid);

        if (val_s == NULL)
            msg->val_type = CVT_NONE; /* Default will be assigned */

        if (val_s != NULL)
        {
            if ((rc = cfg_types[obj->type].str2val((char *)val_s, &val))
                   != 0)
                RETERR(rc, "Value conversion error for %s", oid);

            cfg_types[obj->type].put_to_msg(val, (cfg_msg *)msg);
            cfg_types[obj->type].free(val);
            free(val_s);
            val_s = NULL;
        }

        msg->oid_offset = msg->len;
        msg->len += strlen((char *)oid) + 1;
        strcpy((char *)msg + msg->oid_offset, (char *)oid);
        cfg_process_msg((cfg_msg **)&msg, TRUE);
        if (msg->rc != 0)
            RETERR(msg->rc, "Failed(%r) to execute the add command "
                            "for instance %s", msg->rc, oid);

        free(msg);
        msg = NULL;
        free(oid);
        oid = NULL;

next:
        node = xmlNodeNext(node);
    }
    if (node != NULL)
        RETERR(TE_EINVAL, "Incorrect add command format");

    return 0;
}

/**
 * Process "history" configuration file - execute all commands
 * and add them to dynamic history.
 * Note: this routine does not reboot Test Agents.
 *
 * @param node          <history> node pointer
 * @param expand_vars   List of key-value pairs for expansion in file,
 *                      @c NULL if environment variables are used for
 *                      substitutions
 * @param postsync      is processing performed after sync with TA
 *
 * @return status code (errno.h)
 */
int
cfg_dh_process_file(xmlNodePtr node, te_kvpair_h *expand_vars,
                    te_bool postsync)
{
    xmlNodePtr cmd;
    int        len;
    int        rc;

    if (node == NULL)
        return 0;

    for (cmd = xmlNodeChildren(node); cmd != NULL; cmd = xmlNodeNext(cmd))
    {
        xmlNodePtr tmp = xmlNodeChildren(cmd);
        xmlChar   *oid   = NULL;
        xmlChar   *val_s = NULL;
        char      *attr = NULL;

        if (!xmlNodeCond(cmd))
            continue;

        if (xmlStrcmp(cmd->name, (const xmlChar *)"include") == 0)
            continue;

        if (xmlStrcmp(cmd->name, (const xmlChar *)"history") == 0)
        {
            rc = cfg_dh_process_file(cmd, expand_vars, postsync);
            if (rc != 0)
            {
                ERROR("Processing of embedded history failed %r", rc);
                return rc;
            }
            continue;
        }

        if (xmlStrcmp(cmd->name , (const xmlChar *)"reboot") == 0)
        {
            cfg_reboot_msg *msg = NULL;

            if (!postsync)
                continue;

            if ((attr = (char *)xmlGetProp(cmd,
                                           (const xmlChar *)"ta")) == NULL)
                RETERR(TE_EINVAL, "Incorrect reboot command format");

            if ((msg = (cfg_reboot_msg *)calloc(1, sizeof(*msg) +
                                                strlen(attr) + 1))
                    == NULL)
                RETERR(TE_ENOMEM, "Cannot allocate memory");

            msg->type = CFG_REBOOT;
            msg->len = sizeof(*msg) + strlen(attr) + 1;
            msg->rc = 0;
            msg->restore = FALSE;
            strcpy(msg->ta_name, attr);

            cfg_process_msg((cfg_msg **)&msg, TRUE);
            if (msg->rc != 0)
                RETERR(msg->rc, "Failed to execute the reboot command");

            xmlFree((xmlChar *)attr);
            free(msg);
            continue;
        }

        if (xmlStrcmp(cmd->name , (const xmlChar *)"register") != 0 &&
            xmlStrcmp(cmd->name , (const xmlChar *)"unregister") != 0 &&
            xmlStrcmp(cmd->name , (const xmlChar *)"add") != 0 &&
            xmlStrcmp(cmd->name , (const xmlChar *)"set") != 0 &&
            xmlStrcmp(cmd->name , (const xmlChar *)"get") != 0 &&
            xmlStrcmp(cmd->name , (const xmlChar *)"delete") != 0 &&
            xmlStrcmp(cmd->name , (const xmlChar *)"copy") != 0)
        {
            ERROR("Unknown command %s", cmd->name);
            return TE_EINVAL;
        }

        if (xmlStrcmp(cmd->name , (const xmlChar *)"register") == 0)
        {
            cfg_register_msg *msg = NULL;

            if (postsync)
                continue;

            while (tmp != NULL &&
                   xmlStrcmp(tmp->name , (const xmlChar *)"object") == 0)
            {
                const xmlChar *parent_dep;

                attr = xmlGetProp_exp_vars_or_env(tmp,
                      (xmlChar *)"cond", expand_vars);
                if (attr != NULL)
                {
                    /* in case add condition is specified */
                    if (strcmp(attr, "false") == 0 || strcmp(attr, "") == 0)
                    {
                        xmlFree((xmlChar *)attr);
                        tmp = xmlNodeNext(tmp);
                        continue;
                    }
                    xmlFree((xmlChar *)attr);
                }

                oid = (xmlChar *)xmlGetProp_exp_vars_or_env(tmp,
                      (xmlChar *)"oid", expand_vars);
                if (oid == NULL)
                    RETERR(TE_EINVAL, "Incorrect %s command format",
                           cmd->name);

                val_s = xmlGetProp(tmp, (const xmlChar *)"default");

                parent_dep = xmlGetProp(tmp, (const xmlChar *)"parent-dep");

                len = sizeof(cfg_register_msg) + strlen((char *)oid) + 1 +
                      (val_s == NULL ? 0 : strlen((char *)val_s) + 1);

                if ((msg = (cfg_register_msg *)calloc(1, len)) == NULL)
                    RETERR(TE_ENOMEM, "Cannot allocate memory");

                msg->type = CFG_REGISTER;
                msg->len = len;
                msg->rc = 0;
                msg->access = CFG_READ_CREATE;
                msg->no_parent_dep = (parent_dep != NULL &&
                              xmlStrcmp(parent_dep,
                                        (const xmlChar *)"no") == 0);
                msg->val_type = CVT_NONE;
                msg->substitution = FALSE;

                strcpy(msg->oid, (char *)oid);
                if (val_s != NULL)
                {
                    msg->def_val = strlen(msg->oid) + 1;
                    strcpy(msg->oid + msg->def_val, (char *)val_s);
                }

                attr = (char *)xmlGetProp(tmp, (const xmlChar *)"type");
                if (attr != NULL)
                {
                    if (strcmp(attr, "integer") == 0)
                        msg->val_type = CVT_INTEGER;
                    else if (strcmp(attr, "uint64") == 0)
                        msg->val_type = CVT_UINT64;
                    else if (strcmp(attr, "address") == 0)
                        msg->val_type = CVT_ADDRESS;
                    else if (strcmp(attr, "string") == 0)
                        msg->val_type = CVT_STRING;
                    else if (strcmp(attr, "none") != 0)
                        RETERR(TE_EINVAL, "Unsupported object type %s",
                               attr);
                    xmlFree((xmlChar *)attr);
                    attr = NULL;
                }

                attr = (char *)xmlGetProp(tmp, (const xmlChar *)"volatile");
                if (attr != NULL)
                {
                    if (strcmp(attr, "true") == 0)
                        msg->vol = TRUE;
                    else if (strcmp(attr, "false") != 0)
                        RETERR(TE_EINVAL, "Volatile should be specified "
                                          "using \"true\" or \"false\"");
                    xmlFree((xmlChar *)attr);
                    attr = NULL;
                }

                if (val_s != NULL)
                {
                    cfg_inst_val val;

                    if (cfg_types[msg->val_type].str2val((char *)val_s,
                                                         &val) != 0)
                        RETERR(TE_EINVAL, "Incorrect default value %s",
                               val_s);

                    cfg_types[msg->val_type].free(val);
                }

                attr = (char *)xmlGetProp(tmp, (const xmlChar *)"access");
                if (attr != NULL)
                {
                    if (strcmp(attr, "read_write") == 0)
                        msg->access = CFG_READ_WRITE;
                    else if (strcmp(attr, "read_only") == 0)
                        msg->access = CFG_READ_ONLY;
                    else if (strcmp(attr, "read_create") != 0)
                        RETERR(TE_EINVAL,
                               "Wrong value %s of 'access' attribute",
                               attr);
                    xmlFree((xmlChar *)attr);
                    attr = NULL;
                }

                attr = (char *)xmlGetProp(tmp, (const xmlChar *)"substitution");
                if (attr != NULL)
                {
                    if (strcmp(attr, "true") == 0)
                    {
                        msg->substitution = TRUE;
                    }
                    else if (strcmp(attr, "false") != 0)
                    {
                        RETERR(TE_EINVAL, "substitution should be specified "
                                          "using \"true\" or \"false\"");
                    }
                    xmlFree((xmlChar *)attr);
                    attr = NULL;
                }

                cfg_process_msg((cfg_msg **)&msg, TRUE);
                if (msg->rc != 0)
                    RETERR(msg->rc, "Failed to execute register command "
                                    "for object %s", oid);

                cfg_register_dependency(tmp->children, msg->oid);

                free(val_s);
                val_s = NULL;
                free(msg);
                msg = NULL;
                free(oid);
                oid = NULL;

                tmp = xmlNodeNext(tmp);
            }
            if (tmp != NULL)
                RETERR(TE_EINVAL,
                       "Unexpected node '%s' in register command",
                       tmp->name);
        }   /* register */
        else if (xmlStrcmp(cmd->name , (const xmlChar *)"unregister") == 0)
        {
            cfg_msg *msg = NULL; /* dummy for RETERR to work */

            if (postsync)
                continue;

            while (tmp != NULL)
            {
                if (xmlStrcmp(tmp->name , (const xmlChar *)"object") != 0)
                    RETERR(TE_EINVAL,
                           "Unexpected node '%s' in 'unregister' command",
                           tmp->name);

                oid = (xmlChar *)xmlGetProp_exp_vars_or_env(tmp,
                      (xmlChar *)"oid", expand_vars);
                if (oid == NULL)
                    RETERR(TE_EINVAL, "Incorrect %s command format",
                           cmd->name);

                rc = cfg_db_unregister_obj_by_id_str((char *)oid,
                                                     TE_LL_WARN);
                if (rc != 0)
                    RETERR(rc, "Failed to execute 'unregister' command "
                           "for object %s", oid);

                free(oid);
                oid = NULL;

                tmp = xmlNodeNext(tmp);
            }
        }   /* unregister */
        else if (xmlStrcmp(cmd->name , (const xmlChar *)"add") == 0)
        {
            
            if (!postsync)
                continue;

            rc = cfg_dh_process_add(tmp, expand_vars);
            if (rc != 0)
            {
                ERROR("Failed to process add command");
                return rc;
            }
        }
        else if (xmlStrcmp(cmd->name , (const xmlChar *)"get") == 0)
        {
            if (!postsync)
                continue;

            rc = cfg_dh_process_get(tmp, expand_vars);
            if (rc != 0)
            {
                ERROR("Failed to process get command");
                return rc;
            }
        }
        else if (xmlStrcmp(cmd->name , (const xmlChar *)"set") == 0)
        {
            cfg_set_msg *msg = NULL;
            cfg_inst_val val;
            cfg_handle   handle;
            cfg_object  *obj;

            if (!postsync)
                continue;

            while (tmp != NULL &&
                   xmlStrcmp(tmp->name , (const xmlChar *)"instance") == 0)
            {
                if (!xmlNodeCond(tmp))
                {
                    tmp = xmlNodeNext(tmp);
                    continue;
                }

                rc = cfg_dh_get_instance_info(tmp, &handle, (char **)&oid,
                                               &obj, expand_vars);
                if (rc != 0)
                    return rc;

                len = sizeof(cfg_set_msg) + CFG_MAX_INST_VALUE;
                if ((msg = (cfg_set_msg *)calloc(1, len)) == NULL)
                    RETERR(TE_ENOMEM, "Cannot allocate memory");

                msg->handle = handle;
                msg->type = CFG_SET;
                msg->len = sizeof(cfg_set_msg);
                msg->rc = 0;
                msg->val_type = obj->type;

                val_s = (xmlChar *)xmlGetProp_exp_vars_or_env(tmp,
                        (const xmlChar *)"value", expand_vars);
                if (val_s == NULL)
                    RETERR(TE_EINVAL, "Value is required for %s", oid);

                if ((rc = cfg_types[obj->type].str2val((char *)val_s, &val))
                       != 0)
                    RETERR(rc, "Value conversion error for %s", oid);

                cfg_types[obj->type].put_to_msg(val, (cfg_msg *)msg);
                free(val_s);
                val_s = NULL;
                cfg_types[obj->type].free(val);
                cfg_process_msg((cfg_msg **)&msg, TRUE);

                if (msg->rc != 0)
                    RETERR(msg->rc, "Failed to execute the set command "
                                    "for instance %s", oid);

                free(msg);
                msg = NULL;
                free(oid);
                oid = NULL;

                tmp = xmlNodeNext(tmp);
            }
            if (tmp != NULL)
                RETERR(TE_EINVAL, "Incorrect set command format");
        }
        else if (xmlStrcmp(cmd->name , (const xmlChar *)"delete") == 0)
        {
            cfg_del_msg *msg = NULL;
            cfg_handle   handle;

            if (!postsync)
                continue;

            while (tmp != NULL &&
                   xmlStrcmp(tmp->name, (const xmlChar *)"instance") == 0)
            {
                oid = (xmlChar *)xmlGetProp_exp_vars_or_env(tmp,
                       (xmlChar *)"oid", expand_vars);
                if (oid == NULL)
                    RETERR(TE_EINVAL, "Incorrect %s command format",
                           cmd->name);

                if ((rc = cfg_db_find((char *)oid, &handle)) != 0)
                    RETERR(rc, "Cannot find instance %s", oid);

                if (!CFG_IS_INST(handle))
                    RETERR(TE_EINVAL, "OID %s is not instance", oid);

                if ((msg = (cfg_del_msg *)calloc(1, sizeof(*msg))) == NULL)
                    RETERR(TE_ENOMEM, "Cannot allocate memory");

                msg->type = CFG_DEL;
                msg->len = sizeof(*msg);
                msg->rc = 0;
                msg->handle = handle;

                cfg_process_msg((cfg_msg **)&msg, TRUE);
                if (msg->rc != 0)
                    RETERR(msg->rc, "Failed to execute the delete command "
                                    "for instance %s", oid);

                free(msg);
                msg = NULL;
                free(oid);
                oid = NULL;

                tmp = xmlNodeNext(tmp);
            }
            if (tmp != NULL)
                RETERR(TE_EINVAL, "Incorrect delete command format");
        }
        else if (xmlStrcmp(cmd->name , (const xmlChar *)"copy") == 0)
        {
            if (!postsync)
                continue;

            rc = cfg_dh_process_copy(tmp, expand_vars);
            if (rc != 0)
            {
                ERROR("Failed to process copy command: %r", rc);
                return rc;
            }
        }
        else
        {
            assert(FALSE);
        }
        free(val_s);
        val_s = NULL;
    }

    return 0;
#undef RETERR
}

/**
 * Create "history" configuration file with specified name.
 *
 * @param filename      name of the file to be created
 *
 * @return status code (errno.h)
 */
int
cfg_dh_create_file(char *filename)
{
    FILE *f= fopen(filename, "w");

    cfg_dh_entry *tmp;
    xmlChar      *str;

    cfg_dh_optimize();

#define RETERR(_err) \
    do {                   \
        fclose(f);         \
        unlink(filename);  \
        return _err;       \
    } while (0)

    if (f == NULL)
        return TE_OS_RC(TE_CS, errno);

    fprintf(f, "<?xml version=\"1.0\"?>\n");
    fprintf(f, "<history>\n");

    for (tmp = first; tmp != NULL; tmp = tmp->next)
    {
        switch (tmp->cmd->type)
        {
            case CFG_REGISTER:
            {
                cfg_register_msg *msg = (cfg_register_msg *)(tmp->cmd);

                fprintf(f, "\n  <register>\n");
                fprintf(f, "    <object oid=\"%s\" "
                        "access=\"%s\" type=\"%s\"%s",
                        msg->oid,
                        msg->access == CFG_READ_CREATE ?
                            "read_create" :
                        msg->access == CFG_READ_WRITE ?
                            "read_write" : "read_only",
                        msg->val_type == CVT_NONE ?    "none" :
                        msg->val_type == CVT_INTEGER ? "integer" :
                        msg->val_type == CVT_UINT64 ? "uint64" :
                        msg->val_type == CVT_ADDRESS ? "address" :
                                                       "string",
                        (msg->no_parent_dep ? " parent-dep=\"no\"" : ""));
                if (msg->def_val)
                {
                    str = xmlEncodeEntitiesReentrant(NULL,
                              (xmlChar *)(msg->oid + msg->def_val));
                    if (str == NULL)
                        RETERR(TE_RC(TE_CS, TE_ENOMEM));
                    fprintf(f, " default=\"%s\"", str);
                    xmlFree(str);
                }
                fprintf(f, "/>\n  </register>\n");
                break;
            }

            case CFG_ADD:
            case CFG_SET:
            {
                cfg_val_type t = tmp->cmd->type == CFG_ADD ?
                                 ((cfg_add_msg *)(tmp->cmd))->val_type :
                                 ((cfg_set_msg *)(tmp->cmd))->val_type;

                fprintf(f, "\n  <%s>\n",
                        tmp->cmd->type == CFG_ADD ? "add" : "set");
                fprintf(f, "    <instance oid=\"%s\" ",
                        tmp->cmd->type == CFG_ADD ?
                        (char *)(tmp->cmd) +
                        ((cfg_add_msg *)(tmp->cmd))->oid_offset :
                        tmp->old_oid);

                if (t != CVT_NONE)
                {
                    cfg_inst_val val;
                    char        *val_str = NULL;
                    int          rc;

                    rc = cfg_types[t].get_from_msg(tmp->cmd, &val);
                    if (rc != 0)
                        RETERR(rc);

                    rc = cfg_types[t].val2str(val, &val_str);
                    cfg_types[t].free(val);
                    if (rc != 0)
                        RETERR(rc);

                    str = xmlEncodeEntitiesReentrant(NULL,
                                                     (xmlChar *)val_str);
                    free(val_str);
                    if (str == NULL)
                        RETERR(TE_RC(TE_CS, TE_ENOMEM));
                    fprintf(f, "value=\"%s\"", str);
                    xmlFree(str);
                 }
                 fprintf(f, "/>\n  </%s>\n",
                         tmp->cmd->type == CFG_ADD ? "add" : "set");
                 break;
            }

            case CFG_DEL:
            {
                fprintf(f, "\n  <delete>\n    <instance oid=\"%s\"/>\n"
                           "  </delete>\n",
                        tmp->old_oid);
                break;
            }

            case CFG_REBOOT:
            {
                fprintf(f, "\n  <reboot ta=%s/>\n",
                       ((cfg_reboot_msg *)(tmp->cmd))->ta_name);
                break;
            }
        }
    }
    fprintf(f, "\n</history>\n");
    fclose(f);
    return 0;

#undef RETERR
}

/**
 * Attach backup to the last command.
 *
 * @param filename      name of the backup file
 *
 * @return status code (see te_errno.h)
 */
int
cfg_dh_attach_backup(char *filename)
{
    cfg_backup *tmp;

    if ((tmp = (cfg_backup *)malloc(sizeof(*tmp))) == NULL)
        return TE_ENOMEM;

    if ((tmp->filename = strdup(filename)) == NULL)
    {
        free(tmp);
        return TE_ENOMEM;
    }
    if (last == NULL)
    {
        if (begin_backup == NULL)
        {
            begin_backup = tmp;
            tmp->next = NULL;
        }
        else
        {
            tmp->next = begin_backup;
            begin_backup = tmp;
        }

        VERB("Attach backup %s to the beginning", filename);
    }
    else
    {
        tmp->next = last->backup;
        last->backup = tmp;

        VERB("Attach backup %s to command %d", filename, last->seq);
    }

    return 0;
}

/**
 * Returns TRUE, if backup with specified name is associated
 * with DH entry.
 */
static te_bool
has_backup(cfg_dh_entry *entry, char *filename)
{
    cfg_backup *tmp;

    for (tmp = entry->backup; tmp != NULL; tmp = tmp->next)
        if (strcmp(tmp->filename, filename) == 0)
            break;

    return tmp != NULL;
}

/**
 * Restore backup with specified name using reversed command
 * of the dynamic history. Processed commands are removed
 * from the history.
 *
 * @param filename      name of the backup file
 * @param hard_check    whether hard check should be applied
 *                      on restore backup. For instance if on deleting
 *                      some instance we got ESRCH or ENOENT, we should
 *                      kepp processing without any error.
 * @param shutdown      if @c TRUE - the configurator shuts down.
 *                      Do no stop restoring backup when error @c ENOENT
 *                      occurs (for example for @c CFG_SET command).
 *
 * @return status code (see te_errno.h)
 * @retval TE_ENOENT       there is not command in dynamic history to which
 *                      the specified backup is attached
 */
static int
cfg_dh_restore_backup_ext(char *filename, te_bool hard_check, te_bool shutdown)
{
    cfg_dh_entry *limit = NULL;
    cfg_dh_entry *tmp;
    cfg_backup   *tmp_bkp;
    cfg_dh_entry *prev;
    char         *id;

    int rc;
    int result = 0;
    int unreg_obj_LL;

    cfg_dh_optimize();

    /* At first, look for an instance with the backup */
    if (filename != NULL)
    {
        for (limit = first; limit != NULL; limit = limit->next)
        {
            if (limit->backup != NULL && has_backup(limit, filename))
                break;
        }

        if (limit == NULL)
        {
            for (tmp_bkp = begin_backup; tmp_bkp != NULL; tmp_bkp = tmp_bkp->next)
            {
                if (strcmp(tmp_bkp->filename, filename) == 0)
                    break;
            }
        }

        if (limit == NULL && tmp_bkp == NULL)
        {
            ERROR("Position of the backup in dynamic history is not found");
            return TE_ENOENT;
        }
    }

    if (filename != NULL)
    {
        if (limit != NULL)
            VERB("Restore backup %s up to command %d", filename, limit->seq);
        else
            VERB("Restore backup %s up to beginning", filename);
    }

    /* If reversing the start up history, sweep warnings under the rug: */
    if (filename == NULL)
        unreg_obj_LL = TE_LL_VERB;
    else
        unreg_obj_LL = TE_LL_WARN;

    for (tmp = last; tmp != limit; tmp = prev)
    {
        prev = tmp->prev;
        switch (tmp->cmd->type)
        {
            case CFG_UNREGISTER:
                break;
            case CFG_REGISTER:
            {
                id = ((cfg_register_msg *)(tmp->cmd))->oid;
                rc = cfg_db_unregister_obj_by_id_str(id, unreg_obj_LL);
                if (rc != 0)
                {
                    ERROR("%s(): cfg_db_unregister_obj_by_id_str() "
                          "failed: %r, id: %s", __FUNCTION__, rc, id);
                }
                break;
            }

            case CFG_ADD:
            {
                cfg_del_msg msg = { .type = CFG_DEL, .len = sizeof(msg),
                                    .rc = 0, .handle = 0, .local = FALSE };
                cfg_msg    *p_msg = (cfg_msg *)&msg;

                rc = cfg_db_find((char *)(tmp->cmd) +
                                 ((cfg_add_msg *)(tmp->cmd))->oid_offset,
                                 &(msg.handle));

                if (rc != 0)
                {
                    if (TE_RC_GET_ERROR(rc) != TE_ENOENT)
                    {
                        ERROR("%s(): cfg_db_find() failed: %r",
                              __FUNCTION__, rc);
                        TE_RC_UPDATE(result, msg.rc);
                    }
                    break;
                }

                /* Roll back only messages that were committed */
                if (tmp->committed)
                {
                    cfg_process_msg(&p_msg, FALSE);
                }
                else
                {
                    VERB("Do not restore %s as it is locally added",
                         (char *)(tmp->cmd) +
                                 ((cfg_add_msg *)(tmp->cmd))->oid_offset);

                    cfg_db_del(msg.handle);
                    break;
                }

                if (msg.rc != 0 && (hard_check ||
                    (msg.rc != TE_RC(TE_TA_UNIX, TE_ESRCH) &&
                     msg.rc != TE_RC(TE_TA_UNIX, TE_ENOENT))))
                {
                    ERROR("%s(): add failed: %r", __FUNCTION__, msg.rc);
                    TE_RC_UPDATE(result, msg.rc);
                }

                if (!hard_check &&
                    (msg.rc == TE_RC(TE_TA_UNIX, TE_ESRCH) ||
                     msg.rc == TE_RC(TE_TA_UNIX, TE_ENOENT)))
                {
                    /* We should manually delete instance from CFG DB */
                    cfg_db_del(msg.handle);
                }

                break;
            }

            case CFG_SET:
            {
                cfg_set_msg *msg =
                    (cfg_set_msg *)calloc(sizeof(cfg_set_msg) +
                                          CFG_MAX_INST_VALUE, 1);

                if (msg == NULL)
                {
                    ERROR("calloc() failed");
                    return TE_ENOMEM;
                }

                if ((rc = cfg_db_find(tmp->old_oid, &msg->handle)) != 0)
                {
                    free(msg);
                    if (!shutdown || TE_RC_GET_ERROR(rc) != TE_ENOENT)
                    {
                        ERROR("cfg_db_find(%s) failed: %r", tmp->old_oid, rc);
                        return rc;
                    }

                    /*
                     * Let is try to restore the remaining entries,
                     * even if this one failed
                     */
                    ERROR("cfg_db_find(%s) returned %r, trying to restore "
                          "the rest", tmp->old_oid, rc);
                    break;
                }

                msg->type = CFG_SET;
                msg->len = sizeof(*msg);
                msg->val_type = ((cfg_set_msg *)(tmp->cmd))->val_type;
                cfg_types[msg->val_type].put_to_msg(tmp->old_val,
                                                    (cfg_msg *)msg);

                if (tmp->committed)
                {
                    cfg_process_msg((cfg_msg **)&msg, FALSE);
                }
                else
                {
                    VERB("Do not restore %s as it is locally modified",
                         tmp->old_oid);
                }

                rc = msg->rc;
                free(msg);
                if (rc != 0)
                {
                    ERROR("%s(): set failed: %r", __FUNCTION__, rc);
                    TE_RC_UPDATE(result, rc);
                }
                break;
            }

            case CFG_DEL:
            {
                cfg_add_msg *msg = (cfg_add_msg *)
                                       calloc(sizeof(cfg_add_msg) +
                                              CFG_MAX_INST_VALUE +
                                              strlen(tmp->old_oid) + 1, 1);

                if (msg == NULL)
                {
                    ERROR("calloc() failed");
                    return TE_ENOMEM;
                }

                msg->type = CFG_ADD;
                msg->len = sizeof(*msg);
                msg->val_type = tmp->type;
                cfg_types[tmp->type].put_to_msg(tmp->old_val,
                                                (cfg_msg *)msg);
                msg->oid_offset = msg->len;
                msg->len += strlen(tmp->old_oid) + 1;
                strcpy((char *)msg + msg->oid_offset, tmp->old_oid);

                if (tmp->committed)
                {
                    cfg_process_msg((cfg_msg **)&msg, FALSE);
                }
                else
                {
                    cfg_del_msg  *del_msg = NULL;
                    cfg_instance *inst = NULL;

                    VERB("Do not add %s as it is locally modified",
                         tmp->old_oid);

                    del_msg = (cfg_del_msg *)tmp->cmd;
                    if (del_msg == NULL)
                    {
                        ERROR("No cfg_del_msg was attached to local remove "
                              "command for %s", tmp->old_oid);
                        TE_RC_UPDATE(result, TE_ENOENT);
                    }
                    else
                    {
                        inst = CFG_GET_INST(del_msg->handle);
                        if (inst == NULL)
                        {
                            ERROR("Failed to find an instance %s which was "
                                  "scheduled for removal", tmp->old_oid);
                            TE_RC_UPDATE(result, TE_ENOENT);
                        }
                        else
                        {
                            inst->remove = FALSE;
                        }
                    }
                }

                rc = msg->rc;
                free(msg);
                if (rc != 0)
                {
                    ERROR("%s(): delete failed: %r", __FUNCTION__, rc);
                    TE_RC_UPDATE(result, rc);
                }
                break;
            }
        }
        VERB("Restored command %d", tmp->seq);
        free_dh_entry(tmp);
        if (prev != NULL)
            prev->next = NULL;
        last = prev;
    }
    if (limit == NULL)
        first = NULL;

    return result;
}

/* See description in conf_dh.h */
int
cfg_dh_restore_backup(char *filename, te_bool hard_check)
{
    return cfg_dh_restore_backup_ext(filename, hard_check, FALSE);
}

/* See description in conf_dh.h */
int
cfg_dh_restore_backup_on_shutdown()
{
    return cfg_dh_restore_backup_ext(NULL, TRUE, TRUE);
}


/**
 * Push a command to the history.
 *
 * @param msg     message with set, add or delete user request.
 * @param local   whether this command is local or not.
 * @param old_val The old value of the instance. It is used for @c CFG_SET and
 *                @c CFG_DEL command.
 *
 * @return status code
 * @retval 0            success
 * @retval TE_ENOMEM    cannot allocate memory
 * @retval TE_EINVAL    bad message or message, which should not be in
 *                      history
 */
int
cfg_dh_push_command(cfg_msg *msg, te_bool local, const cfg_inst_val *old_val)
{
    cfg_dh_entry *entry = (cfg_dh_entry *)calloc(sizeof(cfg_dh_entry), 1);

    if (entry == NULL)
    {
        ERROR("Memory allocation failure");
        return TE_ENOMEM;
    }

    if ((entry->cmd = (cfg_msg *)calloc(msg->len, 1)) == NULL)
    {
        ERROR("Memory allocation failure");
        free(entry);
        return TE_ENOMEM;
    }

    /* Local commands are not comitted yet, and vice versa */
    entry->committed = !local;

    memcpy(entry->cmd, msg, msg->len);
    if (msg->type == CFG_ADD)
    {
        ((cfg_add_msg *)(entry->cmd))->oid_offset =
            ((cfg_add_msg *)msg)->oid_offset;
    }

#define RETERR(_err) \
    do {                    \
        free(entry->cmd);   \
        free(entry);        \
        return _err;        \
    } while (0)

    /* Construct the reverse command first */
    switch (msg->type)
    {
        case CFG_REGISTER:
        case CFG_ADD:
        case CFG_REBOOT:
            entry->type = CVT_NONE;
            break;

        case CFG_SET:
        case CFG_DEL:
        {
            cfg_instance *inst = CFG_GET_INST(((cfg_del_msg *)msg)->handle);

            if (inst == NULL)
            {
                ERROR("Failed to get instance by handle 0x%08x",
                      ((cfg_del_msg *)msg)->handle);
                RETERR(TE_ENOENT);
            }

            entry->type = inst->obj->type;

            if (inst->obj->type != CVT_NONE)
                if (cfg_types[inst->obj->type].copy(*old_val,
                                                    &(entry->old_val)) != 0)
                    RETERR(TE_ENOMEM);

            if ((entry->old_oid = strdup(inst->oid)) == NULL)
                RETERR(TE_ENOMEM);

            break;
        }

        default:
            /* Should not occur */
            RETERR(TE_EINVAL);
    }

    if (first == NULL)
    {
        first = last = entry;
        entry->next = NULL;
        entry->prev = NULL;
        entry->seq = 1;
    }
    else
    {
        last->next = entry;
        entry->seq = last->seq + 1;
        entry->prev = last;
        last = entry;
    }

    if (msg->type == CFG_REBOOT)
    {
        for (entry = first; entry != NULL; entry = entry->next)
            free_entry_backup(entry);
    }

    VERB("Add command %d", last->seq);

    return 0;
#undef RETERR
}

/**
 * Delete last command from the history.
 */
void
cfg_dh_delete_last_command(void)
{
    cfg_dh_entry *tmp = last;

    if (first == NULL)
        return;

    if (first == last)
    {
        first = last = NULL;
    }
    else
    {
        cfg_dh_entry *prev;

        for (prev = first; prev->next != last; prev = prev->next);

        prev->next = NULL;
        last = prev;
    }

    VERB("Delete last command %d", tmp->seq);
    free_dh_entry(tmp);
}

/**
 * Destroy dynamic hostory before shut down.
 */
void
cfg_dh_destroy(void)
{
    cfg_dh_entry *tmp, *next;

    for (tmp = first; tmp != NULL; tmp = next)
    {
        next = tmp->next;
        free_dh_entry(tmp);
    }

    last = first = NULL;
}

/**
 * Remove useless command sequences.
 */
void
cfg_dh_optimize(void)
{
    cfg_dh_entry *tmp;

    /* Optimize add/delete commands */
    for (tmp = first; tmp != NULL; )
    {
        cfg_dh_entry *tmp_del = NULL;
        char         *oid;
        te_bool       found = FALSE;
        unsigned int  len;

        if (tmp->cmd->type != CFG_ADD || tmp->backup != NULL)
        {
            tmp = tmp->next;
            continue;
        }

        oid = (char *)(tmp->cmd) + ((cfg_add_msg *)(tmp->cmd))->oid_offset;
        len = strlen(oid);

        /*
         * Look for delete command for this instance.
         * If command with backup or reboot command or
         * command changing the object, which is not in tmp subtree is
         * met, break the search.
         */
         for (tmp_del = tmp->next; tmp_del != NULL; tmp_del = tmp_del->next)
         {
             char *tmp_oid;

             if (tmp_del->backup != NULL ||
                 tmp_del->cmd->type == CFG_REBOOT)
                 break;

             if (tmp_del->cmd->type == CFG_DEL &&
                 strcmp(tmp_del->old_oid, oid) == 0)
             {
                 found = TRUE;
                 break;
             }

             tmp_oid = tmp_del->cmd->type == CFG_ADD ?
                       (char *)(tmp_del->cmd) +
                       ((cfg_add_msg *)(tmp_del->cmd))->oid_offset :
                       tmp_del->old_oid;

             if (tmp_oid == NULL || strlen(tmp_oid) > len ||
                 strncmp(tmp_oid, oid, len) != 0)
             {
                 break;
             }
         }

         if (!found)
         {
             tmp = tmp->next;
             continue;
         }

         /* Now we need to remove [tmp ... tmp_del] commands */
         if (tmp->prev != NULL)
             tmp->prev->next = tmp_del->next;
         else
             first = tmp_del->next;

         if (tmp_del->next != NULL)
             tmp_del->next->prev = tmp->prev;
         else
             last = tmp->prev;

         tmp_del->next = NULL;

         for (; tmp != NULL; tmp = tmp_del)
         {
             tmp_del = tmp->next;
             VERB("Optimize: delete command %d", tmp->seq);
             free_dh_entry(tmp);
         }
    }

    /* Optimize set commands */
    for (tmp = first; tmp != NULL && tmp->next != NULL; )
    {
        if (tmp->cmd->type == CFG_SET && tmp->next->cmd->type == CFG_SET &&
            tmp->backup == NULL &&
            ((cfg_set_msg *)(tmp->cmd))->handle ==
            ((cfg_set_msg *)(tmp->next->cmd))->handle)
        {
            cfg_dh_entry *next = tmp->next;
            cfg_inst_val  next_old_val = next->old_val;
            cfg_val_type  type = ((cfg_set_msg *)(tmp->cmd))->val_type;

            /*
             * Copy old value of this command to old value
             * of the next command.
             */
            if (cfg_types[type].copy(tmp->old_val, &next->old_val) != 0)
            {
                next->old_val = next_old_val;
                return;
            }
            cfg_types[type].free(next_old_val);

            if (tmp->prev != NULL)
                tmp->prev->next = tmp->next;
            else
                first = tmp->next;
            tmp->next->prev = tmp->prev;
            free_dh_entry(tmp);
            tmp = next;
        }
        else
            tmp = tmp->next;
    }
}

/**
 * Release history after backup.
 *
 * @param filename      name of the backup file
 */
void
cfg_dh_release_after(char *filename)
{
    cfg_dh_entry *limit, *next, *cur;

    if (filename == NULL)
        return;

    for (limit = last; limit != NULL; limit = limit->prev)
    {
        if (limit->backup != NULL)
        {
            if (has_backup(limit, filename))
                break;
            return;
        }
    }

    if (limit == NULL || limit == last)
        return;

    for (cur = limit->next; cur != NULL; cur = next)
    {
        next = cur->next;
        VERB("Release after: delete command %d", cur->seq);
        free_dh_entry(cur);
    }
    last = limit;
    last->next = NULL;
}

/**
 * Forget about this backup.
 *
 * @param filename      name of the backup file
 *
 * @return status code
 */
int
cfg_dh_release_backup(char *filename)
{
    cfg_dh_entry *tmp;

    for (tmp = first; tmp != NULL; tmp = tmp->next)
    {
        cfg_backup *cur, *prev;

        for (cur = tmp->backup, prev = NULL;
             cur != NULL && strcmp(cur->filename, filename) != 0;
             prev = cur, cur = cur->next);

        if (cur == NULL)
            continue;

        if (prev)
            prev->next = cur->next;
        else
            tmp->backup = cur->next;

        free(cur->filename);
        free(cur);

        return 0;
    }

    return 0;
}

/**
 * Notify history DB about successfull commit operation.
 * The result of calling of this function is that some entries in DH DB
 * enter committed state.
 *
 * @param oid  OID of the instance that was successfully committed
 *
 * @return status code (errno.h)
 */
int
cfg_dh_apply_commit(const char *oid)
{
    cfg_dh_entry *tmp;
    const char   *entry_oid;
    size_t        oid_len;

    if (oid == NULL)
    {
        /* Let think that we've committed the whole tree */
        oid = "";
    }

    oid_len = strlen(oid);

    for (tmp = first; tmp != NULL; tmp = tmp->next)
    {
        switch (tmp->cmd->type)
        {
            /*
             * We are only interested in ADD, DEL and SET commands,
             * which could be local.
             */
            case CFG_ADD:
            case CFG_DEL:
            case CFG_SET:
            {
                entry_oid = (tmp->cmd->type == CFG_ADD) ?
                    (const char *)(tmp->cmd) +
                        ((cfg_add_msg *)(tmp->cmd))->oid_offset :
                    tmp->old_oid;

                if (strncmp(entry_oid, oid, oid_len) == 0)
                    tmp->committed = TRUE;

                break;
            }

            default:
                break;
        }
    }

    return 0;
}

/**
 * Execute 'register' command from the DH
 *
 * @param msg Register message
 *
 * @return Status code
 */
static te_errno
restore_cmd_register(cfg_register_msg *msg)
{
    cfg_object *obj;

    obj = cfg_get_obj_by_obj_id_str(msg->oid);
    if (obj != NULL)
        return 0;

    cfg_process_msg((cfg_msg **)&msg, FALSE);
    if (msg->rc != 0)
    {
        ERROR("%s(): failed to execute register command for "
              "object %s: %r", __FUNCTION__, msg->oid, msg->rc);
    }

    return msg->rc;
}

static te_bool
check_oid_contains_ta(const char *oid, const te_vec *ta_list)
{
    te_bool is_found = FALSE;
    char * const *ta;
    char *ta_oid;
    int agent_subid_pos;

    if (strcmp_start(CFG_TA_PREFIX, oid) == 0)
        agent_subid_pos = 1;
    else if (strcmp_start(CFG_RCF_PREFIX, oid) == 0)
        agent_subid_pos = 2;
    else
        return is_found;

    ta_oid = cfg_oid_str_get_inst_name(oid, agent_subid_pos);
    if (ta_oid == NULL)
        return is_found;

    TE_VEC_FOREACH(ta_list, ta)
    {
        if (strcmp(ta_oid, *ta) == 0)
        {
            is_found = TRUE;
            break;
        }
    }

    free(ta_oid);
    return is_found;
}

/**
 * Execute 'add' command from the DH
 *
 * @param msg Add message
 * @param ta  Name of the TA
 *
 * @return Status code
 */
static te_errno
restore_cmd_add(cfg_add_msg *msg, const te_vec *ta_list)
{
    char *oid;
    cfg_instance *inst;
    char *val_str = NULL;
    cfg_inst_val val;
    int rv;

    oid = (char *)msg + msg->oid_offset;

    if (!check_oid_contains_ta(oid, ta_list))
        return 0;

    inst = cfg_get_ins_by_ins_id_str(oid);
    if (inst != NULL)
    {
        ERROR("%s(): failed to add %s from DH", __FUNCTION__, oid);
        return TE_EFAIL;
    }

    cfg_process_msg((cfg_msg **)&msg, FALSE);
    if (msg->rc != 0)
    {
        ERROR("%s(): failed to add a new instance %s: %r",
              __FUNCTION__, oid, msg->rc);
        return msg->rc;
    }

    rv = cfg_db_get(msg->handle, &val);
    if (rv != 0)
    {
        ERROR("Failed to get value for %s", oid);
        return rv;
    }

    cfg_types[CFG_GET_INST(msg->handle)->obj->type].val2str(val, &val_str);
    RING("Added %s%s = %s", msg->local ? "locally " : "", oid,
         (val_str != NULL) ? val_str : "(none)");
    free(val_str);
    cfg_types[CFG_GET_INST(msg->handle)->obj->type].free(val);

    return 0;
}

/**
 * Execute 'set' or 'delete' command from the DH
 *
 * @param entry Dynamic history entry
 * @param ta    Name of the TA
 *
 * @return Status code
 */
static te_errno
restore_cmd_set_del(cfg_dh_entry *entry, const te_vec *ta_list)
{
    cfg_instance *inst;
    cfg_set_msg *msg = (cfg_set_msg *)(entry->cmd);
    char *val_str = NULL;
    cfg_inst_val val;
    te_errno rc;

    if (!check_oid_contains_ta(entry->old_oid, ta_list))
        return 0;

    inst = cfg_get_ins_by_ins_id_str(entry->old_oid);
    if (inst == NULL)
    {
        ERROR("%s(): failed to get instance by oid %s",
              __FUNCTION__, entry->old_oid);
        return TE_EFAIL;
    }

    /* DH stores invalid handle */
    msg->handle = inst->handle;

    cfg_types[inst->obj->type].free(entry->old_val);
    rc = cfg_types[inst->obj->type].copy(inst->val, &(entry->old_val));
    if (rc != 0)
    {
        ERROR("%s(): failed to copy value: %r", __FUNCTION__, rc);
        return rc;
    }

    cfg_process_msg((cfg_msg **)&msg, FALSE);
    if (msg->rc != 0)
    {
        ERROR("%s(): failed to set/delete %s: %r",
              __FUNCTION__, inst->oid, msg->rc);
        return msg->rc;
    }

    if (msg->type == CFG_SET)
    {
        rc = cfg_db_get(msg->handle, &val);
        if (rc != 0)
        {
            ERROR("Failed to get value for %s: %r", inst->oid, rc);
            return rc;
        }

        cfg_types[inst->obj->type].val2str(val, &val_str);
        RING("Set %s%s = %s", msg->local ? "locally " : "",
             inst->oid, val_str);
        cfg_types[inst->obj->type].free(val);
        free(val_str);
    }
    else
    {
        RING("Deleted %s%s", (msg->local ? "locally " : ""),
             inst->oid);
    }

    return 0;
}

te_errno
cfg_dh_restore_agents(const te_vec *ta_list)
{
    cfg_dh_entry *entry;
    te_errno rc;

    for (entry = first; entry != NULL; entry = entry->next)
    {
        switch (entry->cmd->type)
        {
            case CFG_REGISTER:
                rc = restore_cmd_register((cfg_register_msg *)(entry->cmd));
                break;

            case CFG_ADD:
                rc = restore_cmd_add((cfg_add_msg *)(entry->cmd), ta_list);
                break;

            case CFG_SET:
            case CFG_DEL:
                rc = restore_cmd_set_del(entry, ta_list);
                break;

            default:
                ERROR("%s(): unknown command in the DH: %d", __FUNCTION__,
                      entry->cmd->type);
                return TE_EINVAL;
        }

        if (rc != 0)
            return rc;

        if (entry->backup != NULL)
        {
            rc = cfg_backup_verify_and_restore_ta_subtrees(
                                                entry->backup->filename,
                                                ta_list);
            if (rc != 0)
                return rc;
        }
    }

    return 0;
}
