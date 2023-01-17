/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Configurator
 *
 * Support of dynamic history
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#include "te_alloc.h"
#include "conf_defs.h"
#define TE_EXPAND_XML 1
#include "te_expand.h"

/* defined in conf_backup.c */
extern int cfg_register_dependency(object_type *object,
                                       const char *dependant);

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
    te_bool              committed; /**< Whether the command kept in this
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

#define RETERR(_rc, _str...)    \
    do {                        \
        int _err = _rc;         \
                                \
        ERROR(_str);            \
        free(oid);              \
        free(val_s);            \
        free(msg);              \
        return _err;            \
    } while (0)

/**
 * Parse handle, oid and object of instance
 * and check that object instance has value
 *
 * @param inst          instance
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
cfg_dh_get_instance_info(instance_type *inst, cfg_handle *handle,
                         char **oid, cfg_object **obj,
                         const te_kvpair_h *expand_vars)
{
    te_errno rc = 0;

    *oid = strdup(inst->oid);
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
 * @param inst          instance to get value from
 * @param expand_vars   List of key-value pairs for expansion in file
 *                      NULL if environmental variables used for
 *                      substitutions
 *
 * @return Status code.
 */
static te_errno
cfg_dh_get_value_from_instance(instance_type *inst, te_kvpair_h *expand_vars)
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


    rc = cfg_dh_get_instance_info(inst, &handle, &oid, &obj, expand_vars);
    if (rc != 0)
        return rc;

    if (inst->value == NULL)
    {
        ERROR("Value is required for %s", oid);
        rc = TE_EINVAL;
        goto cleanup;
    }
    var_name = strdup(inst->value);

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
 * Process 'copy' command.
 *
 * @param inst          instance to get
 * @param expand_vars   List of key-value pairs for expansion in file
 *                      NULL if environmental variables used for
 *                      substitutions
 *
 * @return Status code.
 */
static te_errno
cfg_dh_process_copy_instance(instance_type *inst, te_kvpair_h *expand_vars)
{
    te_errno      rc;
    size_t        len;
    size_t        oid_len;
    char         *oid = NULL;
    char         *val_s = NULL;
    cfg_copy_msg *msg = NULL;
    cfg_handle    src_handle;

    if (inst->oid == NULL)
    {
        ERROR("Incorrect command format");
        rc = TE_EINVAL;
        goto cleanup;
    }
    oid = strdup(inst->oid);

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

    if (inst->value == NULL)
    {
        ERROR("Value is required for %s to copy from", oid);
        rc = TE_EINVAL;
        goto cleanup;
    }
    val_s = strdup(inst->value);

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

cleanup:
    free(oid);
    free(val_s);
    free(msg);

    return rc;
}

/**
 * Process 'add' command.
 *
 * @param inst          instance that should be added
 * @param expand_vars   list of key-value pairs for expansion in file,
 *                      @c NULL if environment variables are used for
 *                      substitutions
 *
 * @return Status code.
 */
static te_errno
cfg_dh_process_add_instance(instance_type *inst, const te_kvpair_h *expand_vars)
{
    te_errno rc;
    size_t len;
    cfg_add_msg *msg = NULL;
    cfg_inst_val val;
    cfg_object *obj;
    char *oid = NULL;
    char *val_s = NULL;

    if (inst->oid == NULL)
        RETERR(TE_EINVAL, "Incorrect add command format");
    oid = strdup(inst->oid);

    if ((obj = cfg_get_object(oid)) == NULL)
        RETERR(TE_EINVAL, "Cannot find object for instance %s", oid);

    len = sizeof(cfg_add_msg) + CFG_MAX_INST_VALUE +
          strlen(oid) + 1;
    if ((msg = (cfg_add_msg *)calloc(1, len)) == NULL)
        RETERR(TE_ENOMEM, "Cannot allocate memory");

    msg->type = CFG_ADD;
    msg->len = sizeof(cfg_add_msg);
    msg->val_type = obj->type;
    msg->rc = 0;

    val_s = inst->value;
    if (val_s != NULL && strlen(val_s) >= CFG_MAX_INST_VALUE)
        RETERR(TE_ENOMEM, "Too long value");

    if (obj->type == CVT_NONE && val_s != NULL)
        RETERR(TE_EINVAL, "Value is prohibited for %s", oid);

    if (val_s == NULL)
        msg->val_type = CVT_NONE;

    if (val_s != NULL)
    {
        if ((rc = cfg_types[obj->type].str2val(val_s, &val)) != 0)
            RETERR(rc, "Value conversion error for %s", oid);

        cfg_types[obj->type].put_to_msg(val, (cfg_msg *)msg);
        cfg_types[obj->type].free(val);
        free(val_s);
        val_s = NULL;
    }

    msg->oid_offset = msg->len;
    msg->len += strlen((char *)oid) + 1;
    strcpy((char *)msg + msg->oid_offset, oid);
    cfg_process_msg((cfg_msg **)&msg, TRUE);
    if (msg->rc != 0)
        RETERR(msg->rc, "Failed(%r) to execute the add command "
                        "for instance %s", msg->rc, oid);

    free(msg);
    msg = NULL;
    free(oid);
    oid = NULL;

    return 0;
}

/**
 * Process "history" configuration file - execute all commands
 * and add them to dynamic history.
 * Note: this routine does not reboot Test Agents.
 *
 * @param history       history structure
 * @param expand_vars   List of key-value pairs for expansion in file,
 *                      @c NULL if environment variables are used for
 *                      substitutions
 * @param postsync      is processing performed after sync with TA
 *
 * @return status code (errno.h)
 */
int
cfg_dh_process_file(history_seq *history, te_kvpair_h *expand_vars,
                    te_bool postsync)
{
    int len;
    int rc;
    int i, j;


    if (history == NULL)
        return 0;

    for (i = 0; i < history->entries_count; i++)
    {
        char *oid = NULL;
        char *val_s = NULL;


        if (!postsync)
        {
            /* register */
            for (j = 0; j < history->entries[i].reg_count; j++)
            {
                cfg_register_msg *msg = NULL;

                oid = (char *)history->entries[i].reg[j].oid;
                if (oid == NULL)
                    RETERR(TE_EINVAL,
                           "Incorrect register command format (oid)");

                val_s = (char *)history->entries[i].reg[j].def_val;

                len = sizeof(cfg_register_msg) + strlen(oid) + 1 +
                      (val_s == NULL ? 0 : strlen(val_s) + 1);

                if ((msg = (cfg_register_msg *)calloc(1, len)) == NULL)
                    RETERR(TE_ENOMEM, "Cannot allocate memory");

                msg->type = CFG_REGISTER;
                msg->len = len;
                msg->rc = 0;
                msg->access = history->entries[i].reg[j].access;

                /* parent-dep is currently not used */
                msg->no_parent_dep = history->entries[i].reg[j].no_parent_dep;

                msg->val_type = history->entries[i].reg[j].type;
                msg->substitution = history->entries[i].reg[j].substitution;
                msg->unit = history->entries[i].reg[j].unit;

                strcpy(msg->oid, oid);
                if (val_s != NULL)
                {
                    msg->def_val = strlen(msg->oid) + 1;
                    strcpy(msg->oid + msg->def_val, val_s);
                }

                msg->vol = history->entries[i].reg[j].volat;

                if (val_s != NULL)
                {
                    cfg_inst_val val;

                    if (cfg_types[msg->val_type].str2val(val_s,
                                                         &val) != 0)
                        RETERR(TE_EINVAL, "Incorrect default value %s",
                               val_s);

                    cfg_types[msg->val_type].free(val);
                }

                cfg_process_msg((cfg_msg **)&msg, TRUE);
                if (msg->rc != 0)
                    RETERR(msg->rc, "Failed to execute register command "
                                    "for object %s", oid);

                cfg_register_dependency(&(history->entries[i].reg[j]),
                                        msg->oid);
                free(msg);
                msg = NULL;
                oid = NULL;
            }
            /* unregister */
            for (j = 0; j < history->entries[i].unreg_count; j++)
            {
                cfg_msg *msg = NULL;  /* dummy for RETERR to work */

                oid = (char *)history->entries[i].unreg[j].oid;

                rc = cfg_db_unregister_obj_by_id_str((char *)oid,
                                                     TE_LL_WARN);
                if (rc != 0)
                    RETERR(rc, "Failed to execute 'unregister' command "
                           "for object %s", oid);
                oid = NULL;
            }
        }
        else /* i.e (postsync) */
        {
            if (history->entries[i].reboot_ta != NULL)
            {
                cfg_reboot_msg *msg = NULL;
                char *ta = history->entries[i].reboot_ta;

                if (ta == NULL)
                    RETERR(TE_EINVAL, "Incorrect reboot command format");

                if ((msg = (cfg_reboot_msg *)calloc(1, sizeof(*msg) +
                                                    strlen(ta) + 1))
                        == NULL)
                    RETERR(TE_ENOMEM, "Cannot allocate memory");

                msg->type = CFG_REBOOT;
                msg->len = sizeof(*msg) + strlen(ta) + 1;
                msg->rc = 0;
                msg->restore = FALSE;
                strcpy(msg->ta_name, ta);

                cfg_process_msg((cfg_msg **)&msg, TRUE);
                if (msg->rc != 0)
                    RETERR(msg->rc, "Failed to execute the reboot command");

                free(msg);
            }
            /* add */
            for (j = 0; j < history->entries[i].add_count; j++)
            {
                rc = cfg_dh_process_add_instance(&history->entries[i].add[j],
                                                 expand_vars);
                if (rc != 0)
                {
                    ERROR("Failed to process add command");
                    return rc;
                }
            }
            /* get */
            for (j = 0; j < history->entries[i].get_count; j++)
            {
                rc = cfg_dh_get_value_from_instance(&history->entries[i].get[j],
                                                    expand_vars);
                if (rc != 0)
                {
                    ERROR("Failed to process add command");
                    return rc;
                }
            }
            /* set */
            for (j = 0; j < history->entries[i].set_count; j++)
            {
                cfg_set_msg *msg = NULL;
                cfg_inst_val val;
                cfg_handle   handle;
                cfg_object  *obj;

                rc = cfg_dh_get_instance_info(&history->entries[i].set[j],
                                              &handle, &oid,
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

                val_s = history->entries[i].set[j].value;
                if (val_s == NULL)
                    RETERR(TE_EINVAL, "Value is required for %s", oid);

                if ((rc = cfg_types[obj->type].str2val(val_s, &val)) != 0)
                    RETERR(rc, "Value conversion error for %s", oid);

                cfg_types[obj->type].put_to_msg(val, (cfg_msg *)msg);
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
            }
            /* delete */
            for (j = 0; j < history->entries[i].delete_count; j++)
            {
                cfg_del_msg *msg = NULL;
                cfg_handle   handle;

                oid = history->entries[i].delete[j].oid;
                if (oid == NULL)
                    RETERR(TE_EINVAL, "Incorrect delete command format (oid)");

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
                oid = NULL;
            }
            /* copy */
            for (j = 0; j < history->entries[i].copy_count; j++)
            {
                rc = cfg_dh_process_copy_instance(&history->entries[i].copy[j],
                                                  expand_vars);
                if (rc != 0)
                {
                    ERROR("Failed to process copy command: %r", rc);
                    return rc;
                }
            }
        }
    }
    return 0;
#undef RETERR
}

/**
 * Create YAML "history" configuration file with specified name.
 *
 * @param filename      name of the file to be created
 *
 * @return status code (errno.h)
 */
int
cfg_dh_create_file(char *filename)
{

    cfg_dh_entry *tmp;

    history_seq *history = malloc(sizeof(history_seq));
    unsigned int i;
    te_errno rc = 0;

    cfg_dh_optimize();

#define RETERR(_err) \
    do {                   \
        fclose(f);         \
        unlink(filename);  \
        return _err;       \
    } while (0)

    history->entries_count = 0;
    for (tmp = first; tmp != NULL; tmp = tmp->next)
    {
        history->entries_count++;
    }
    history->entries = malloc(sizeof(history_entry) *
                              history->entries_count);
    for (i = 0; i < history->entries_count; i++)
    {
        history->entries[i].reg = NULL;
        history->entries[i].reg_count = 0;
        history->entries[i].unreg = NULL;
        history->entries[i].unreg_count = 0;
        history->entries[i].add = NULL;
        history->entries[i].add_count = 0;
        history->entries[i].get = NULL;
        history->entries[i].get_count = 0;
        history->entries[i].delete = NULL;
        history->entries[i].delete_count = 0;
        history->entries[i].copy = NULL;
        history->entries[i].copy_count = 0;
        history->entries[i].set = NULL;
        history->entries[i].set_count = 0;
        history->entries[i].reboot_ta = NULL;
    }

    for (tmp = first, i = 0; tmp != NULL; tmp = tmp->next, i++)
    {
        switch (tmp->cmd->type)
        {
            case CFG_REGISTER:
            {
                cfg_register_msg *msg = (cfg_register_msg *)(tmp->cmd);

                history->entries[i].reg_count = 1;
                history->entries[i].reg = malloc(sizeof(object_type));

                history->entries[i].reg[0].oid = strdup(msg->oid);
                history->entries[i].reg[0].access = msg->access;
                history->entries[i].reg[0].type = msg->val_type;
                history->entries[i].reg[0].no_parent_dep = msg->no_parent_dep;
                history->entries[i].reg[0].def_val = NULL;

                if (msg->def_val != 0)
                {
                    history->entries[i].reg[0].def_val =
                                               strdup(msg->oid + msg->def_val);
                }

                history->entries[i].reg[0].unit = false;
                history->entries[i].reg[0].volat = false;
                history->entries[i].reg[0].substitution = false;
                history->entries[i].reg[0].depends = NULL;
                history->entries[i].reg[0].depends_count = 0;
                break;
            }

            case CFG_ADD:
            {
                cfg_val_type t = ((cfg_add_msg *)(tmp->cmd))->val_type;

                history->entries[i].add_count = 1;
                history->entries[i].add = malloc(sizeof(instance_type));

                history->entries[i].add[0].oid = strdup((char *)(tmp->cmd) +
                                    ((cfg_add_msg *)(tmp->cmd))->oid_offset);
                history->entries[i].add[0].value = NULL;

                if (t != CVT_NONE)
                {
                    cfg_inst_val val;
                    char *val_str = NULL;

                    rc = cfg_types[t].get_from_msg(tmp->cmd, &val);
                    if (rc != 0)
                        goto cleanup;

                    rc = cfg_types[t].val2str(val, &val_str);
                    history->entries[i].add[0].value = strdup(val_str);
                    cfg_types[t].free(val);
                    free(val_str);
                    if (rc != 0)
                        goto cleanup;
                }
                break;
            }

            case CFG_SET:
            {
                cfg_val_type t = ((cfg_set_msg *)(tmp->cmd))->val_type;

                history->entries[i].set_count = 1;
                history->entries[i].set = malloc(sizeof(instance_type));

                history->entries[i].set[0].oid = strdup(tmp->old_oid);
                history->entries[i].set[0].value = NULL;

                if (t != CVT_NONE)
                {
                    cfg_inst_val val;
                    char *val_str = NULL;
                    te_errno rc;

                    rc = cfg_types[t].get_from_msg(tmp->cmd, &val);
                    if (rc != 0)
                        goto cleanup;

                    rc = cfg_types[t].val2str(val, &val_str);
                    history->entries[i].add[0].value = strdup(val_str);
                    cfg_types[t].free(val);
                    free(val_str);
                    if (rc != 0)
                        goto cleanup;
                }
                break;
            }

            case CFG_DEL:
            {
                history->entries[i].delete_count = 1;
                history->entries[i].delete = malloc(sizeof(instance_type));

                history->entries[i].delete[0].oid = strdup(tmp->old_oid);
                history->entries[i].delete[0].value = NULL;
                break;
            }

            case CFG_REBOOT:
            {
                history->entries[i].reboot_ta =
                    strdup(((cfg_reboot_msg *)(tmp->cmd))->ta_name);
                break;
            }
            default:
            {
                /* do nothing */
            }
        }
    }
    history->entries_count = i;

    rc = cfg_yaml_save_history_file(filename, history);

cleanup:
    cfg_yaml_free_hist_seq(history);
    return rc;

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
 *                      keep processing without any error.
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
 * Destroy dynamic history before shut down.
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
 * Notify history DB about successful commit operation.
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
