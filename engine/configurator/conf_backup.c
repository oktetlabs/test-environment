/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Configurator
 *
 * Backup-related operations
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#include "conf_defs.h"
#include "te_alloc.h"
#include "conf_cyaml.h"

/**
 * Parses all object dependencies in the configuration file.
 * Note: this function is also used in conf_dh.c.
 *
 * @param node     first dependency node
 */
int
cfg_register_dependency(object_type *object, const char *dependant)
{
    const char *oid = NULL;
    te_bool scope;
    cfg_add_dependency_msg *msg = NULL;
    cfg_handle              dep_handle;

    int      rc = 0;
    unsigned len;
    int i;

    VERB("Registering dependencies for %s", dependant);

    rc = cfg_db_find(dependant, &dep_handle);
    if (rc != 0)
    {
        ERROR("Cannot find a dependant OID: %r", TE_RC(TE_CS, rc));
        return rc;
    }

    for (i = 0; i < object->depends_count; i++)
    {
        oid = object->depends[i].oid;
        if (oid == NULL)
        {
            ERROR("Missing OID attribute in depends[%d]", i);
            return TE_EINVAL;
        }
        scope = (te_bool)(object->depends[i].scope);

        len = sizeof(*msg) + strlen((char *)oid) + 1;
        msg = calloc(1, len);
        msg->type = CFG_ADD_DEPENDENCY;
        msg->len = len;
        msg->rc = 0;
        msg->handle = dep_handle;
        msg->object_wide = scope;
        strcpy(msg->oid, oid);
        cfg_process_msg((cfg_msg **)&msg, TRUE);
        if (msg->rc != 0)
        {
            ERROR("Cannot add dependency for %s: %r", oid, msg->rc);
            rc = msg->rc;
        }
        free(msg);
    }
    return rc;
}

/**
 * Parse all objects specified in the configuration file.
 *
 * @param backup         the backup structure
 * @param first_num_inst the first number of instance in backup
 * @param reg            if TRUE, register objects
 *
 * @return status code (see te_errno.h)
 */
static int
register_objects(backup_seq *backup, unsigned int *first_num_inst, te_bool reg)
{
    int i;
    for (i = 0; i < backup->entries_count; i++)
    {
        cfg_register_msg *msg;

        char *oid  = NULL;
        char *def_val  = NULL;
        int len;

        if (backup->entries[i].object == NULL)
        {
            *first_num_inst = i;
            break;
        }

        if (!reg)
            continue;

        oid = (char *)backup->entries[i].object->oid;
        if (oid == NULL)
        {
            ERROR("Incorrect description of the object number %d", i);
            return TE_EINVAL;
        }

        def_val = (char *)backup->entries[i].object->def_val;

        len = sizeof(cfg_register_msg) + strlen(oid) + 1 +
              (def_val == NULL ? 0 : strlen(def_val) + 1);

        if ((msg = (cfg_register_msg *)calloc(1, len)) == NULL)
        {
            return TE_ENOMEM;
        }

        msg->type = CFG_REGISTER;
        msg->len = len;
        msg->rc = 0;
        msg->access = CFG_READ_CREATE;
        msg->no_parent_dep = backup->entries[i].object->no_parent_dep;
        msg->val_type = CVT_NONE;
        msg->substitution = FALSE;
        strcpy(msg->oid, oid);
        if (def_val != NULL)
        {
            msg->def_val = strlen(msg->oid) + 1;
            strcpy(msg->oid + msg->def_val, def_val);
        }

/**
 * Log error, deallocate resource and return from function.
 *
 * @param _rc   - return code
 * @param _str  - log message
 */
#define RETERR(_rc, _str...)    \
    do {                        \
        int _err = (_rc);       \
                                \
        ERROR(_str);            \
        free(msg);              \
        return _err;            \
    } while (0)

        msg->val_type = (backup->entries[i].object->type);

        msg->unit = (te_bool)backup->entries[i].object->unit;

        if (def_val != NULL)
        {
            cfg_inst_val val;

            if (cfg_types[msg->val_type].str2val(def_val, &val) != 0)
                RETERR(TE_EINVAL, "Incorrect default value %s", def_val);

            cfg_types[msg->val_type].free(val);
        }

        msg->access = backup->entries[i].object->access;

        cfg_process_msg((cfg_msg **)&msg, TRUE);
        if (msg->rc != 0)
            RETERR(msg->rc, "Failed to register object %s", msg->oid);

        cfg_register_dependency(backup->entries[i].object, msg->oid);

        free(msg);
        msg = NULL;
#undef RETERR
    }

    return 0;
}

/**
 * Release memory allocated for list of instances.
 *
 * @param list      location for instance list pointer
 */
static void
free_instances(cfg_instance *list)
{
    cfg_instance *next;

    for (; list != NULL; list = next)
    {
        next = list->bkp_next;
        if (list->obj->type != CVT_NONE)
            cfg_types[list->obj->type].free(list->val);
        free(list->oid);
        free(list);
    }
}

/**
 * Parse instance nodes of the configuration file to list of instances.
 *
 * @param backup           The backup structure
 * @param first_num_inst   Number of the first instance in backup
 * @param list             Location for instance list pointer
 * @param list_size        Where to save number of instances in the list
 *
 * @return Status code (see te_errno.h).
 */
static int
parse_instances(backup_seq *backup, unsigned first_num_inst,
                cfg_instance **list,
                unsigned int *list_size)
{
    cfg_instance *prev = NULL;
    int           rc;

    unsigned int num = 0;
    unsigned i;

    *list = NULL;

/**
 * Log error, deallocate resource and return from function.
 *
 * @param _rc   - return code
 * @param _str  - log message
 */
#define RETERR(_err, _str...)   \
    do {                        \
        ERROR(_str);            \
        free_instances(*list);  \
        return _err;            \
    } while (0)

    for (i = first_num_inst; i < backup->entries_count; i++)
    {
        cfg_instance *tmp;
        const char *oid   = NULL;
        const char *val_s = NULL;

        if (backup->entries[i].instance == NULL)
        {
            RETERR(TE_EINVAL, "Incorrect instance number %d", i);
        }

        oid = backup->entries[i].instance->oid;

        if ((tmp = (cfg_instance *)calloc(sizeof(*tmp), 1)) == NULL)
            RETERR(TE_ENOMEM, "No enough memory");

        tmp->oid = (char *)oid;

        if ((tmp->obj = cfg_get_object(oid)) == NULL)
            RETERR(TE_EINVAL, "Cannot find the object for instance %s",
                   oid);

        if (cfg_db_find(oid, &(tmp->handle)) != 0)
            tmp->handle = CFG_HANDLE_INVALID;

        val_s = backup->entries[i].instance->value;
        if (tmp->obj->type != CVT_NONE)
        {
            if (val_s == NULL)
                RETERR(TE_ENOENT, "Value is necessary for %s", oid);

            if ((rc = cfg_types[tmp->obj->type].str2val((char *)val_s,
                                                        &(tmp->val))) != 0)
            {
                RETERR(rc, "Value conversion error for %s", oid);
            }
            val_s = NULL;
        }
        else if (val_s != NULL)
            RETERR(TE_EINVAL, "Value is prohibited for %s", oid);
#undef RETERR

        if (prev != NULL)
            prev->bkp_next = tmp;
        else
            *list = tmp;

        prev = tmp;
        num++;
    }

    *list_size = num;
    return 0;
}

/**
 * Delete an instance and all its (grand-...)children.
 *
 * @param inst  instance to be deleted
 *
 * @return status code (see te_errno.h)
 */
static int
delete_with_children(cfg_instance *inst, te_bool *has_deps)
{
    cfg_del_msg msg = { .type = CFG_DEL, .len = sizeof(cfg_del_msg),
                        .rc = 0, .handle = 0, .local = FALSE};
    cfg_msg    *p_msg = (cfg_msg *)&msg;
    int         rc;

    cfg_instance *tmp, *next;

    if (cfg_instance_volatile(inst))
        return 0;

    if (inst->obj->access != CFG_READ_CREATE)
        return 0;

    if (inst->obj->dependants != NULL)
        *has_deps = TRUE;

    for (tmp = inst->son; tmp != NULL; tmp = next)
    {
        next = tmp->brother;
        if ((rc = delete_with_children(tmp, has_deps)) != 0)
            return rc;
    }

    msg.handle = inst->handle;

    cfg_process_msg(&p_msg, TRUE);

    /*
     * modifications below are related to OL Bug 6111.
     * In generic ignoring TE_ENOENT is not a good
     * thing - this may hide a bug or postpone it's discovery
     * to upcoming tests.
     */
#if 0                           /* was */
    return TE_RC_GET_ERROR(msg.rc) == TE_ENOENT ? 0 : msg.rc;
#endif
    if (TE_RC_GET_ERROR(msg.rc) == TE_ENOENT)
        ERROR("TE_ENOENT is returned by cfg_process_msg, previously "
              "it was silently ignored. If you think your situation "
              "is valid and not ignoring it causes a bug in your "
              "test package/suite - contact kostik@oktetlabs.ru");
    return msg.rc;

}

static int
topo_qsort_predicate(const void *arg1, const void *arg2)
{
    int idx1 = *(int *)arg1;
    int idx2 = *(int *)arg2;

    return cfg_all_inst[idx2]->obj->ordinal_number -
           cfg_all_inst[idx1]->obj->ordinal_number;
}

/**
 * Check that oid belongs to subtree from vector of the subtrees
 *
 * @param subtrees Vector of the subtrees.
 * @param oid      Instance oid to check.
 *
 * @return @c TRUE if oid belongs to subtree
 */
static te_bool
check_oid_contains_subtrees(const te_vec *subtrees, const char *oid)
{
    char * const *subtree;

    if (subtrees == NULL || te_vec_size(subtrees) == 0)
    {
        if (strcmp_start("/", oid) == 0)
            return TRUE;
        else
            return FALSE;
    }

    TE_VEC_FOREACH(subtrees, subtree)
    {
        if (strcmp_start(*subtree, oid) == 0)
            return TRUE;
    }

    return FALSE;
}

/**
 * Delete all instances from CS not mentioned in the configuration file
 *
 * @param list          list of instances mentioned in the configuration file
 * @param[out] has_deps @c TRUE, if there are objects that depend on
 *                      at least one object from the @p list
 * @param subtrees      Vector of the subtrees to delete. May be @c NULL
 *                      for the root subtree.
 *
 * @return status code (see te_errno.h)
 */
static int
remove_excessive(cfg_instance *list, te_bool *has_deps, const te_vec *subtrees)
{
    int rc;
    int n_deletable;
    int i;

    int *sorted = malloc(sizeof(*sorted) * cfg_all_inst_size);

    if (sorted == NULL)
    {
        ERROR("%s(): not enough memory", __FUNCTION__);
        return TE_RC(TE_CS, TE_ENOMEM);
    }

    for (i = 0, n_deletable = 0; i < cfg_all_inst_size; i++)
    {
        if (cfg_all_inst[i] == NULL ||
            !cfg_all_inst[i]->added ||
            cfg_all_inst[i]->obj->access != CFG_READ_CREATE ||
            !check_oid_contains_subtrees(subtrees, cfg_all_inst[i]->oid))
        {
            continue;
        }
        sorted[n_deletable] = i;
        n_deletable++;
    }
    qsort(sorted, n_deletable, sizeof(*sorted), topo_qsort_predicate);

    for (i = 0; i < n_deletable; i++)
    {
        cfg_instance *tmp;

        if (cfg_all_inst[sorted[i]] == NULL)
            continue;

        for (tmp = list; tmp != NULL; tmp = tmp->bkp_next)
        {
            if (strcmp(tmp->oid, cfg_all_inst[sorted[i]]->oid) == 0)
                break;
        }

        if (tmp != NULL)
            continue;

        rc = delete_with_children(cfg_all_inst[sorted[i]], has_deps);
        if (rc != 0)
        {
            free(sorted);
            return rc;
        }
    }
    free(sorted);

    return 0;
}

/**
 * Add instance or change its value.
 *
 * @param inst          Object instance to be added or changed
 * @param local         If @c TRUE, make local changes to be committed later
 * @param has_deps      Will be set to @c TRUE if changes in other instances
 *                      may happen due to dependencies
 * @param change_made   Will be set to @c TRUE if any change was made
 *                      to configuration
 *
 * @return Status code (see errno.h)
 */
static int
add_or_set(cfg_instance *inst, te_bool local, te_bool *has_deps,
           te_bool *change_made)
{
    if (cfg_inst_agent(inst))
        return 0;

    /* Entry may appear after addition of previous ones */
    if (!CFG_INST_HANDLE_VALID(inst->handle))
    {
        inst->handle = CFG_HANDLE_INVALID;
        cfg_db_find(inst->oid, &inst->handle);
    }

    if (inst->handle != CFG_HANDLE_INVALID)
    {
        cfg_set_msg *msg;
        cfg_msg     *p_msg;
        cfg_val_type t;
        int          rc;

        if (CFG_GET_INST(inst->handle) == 0)
            return TE_EINVAL;
        if (inst->obj->type == CVT_NONE ||
            inst->obj->type == CVT_UNSPECIFIED ||
            cfg_types[inst->obj->type].is_equal(
                 inst->val, CFG_GET_INST(inst->handle)->val))
        {
            return 0;
        }
        if (inst->obj->dependants != NULL)
            *has_deps = TRUE;

        msg = (cfg_set_msg *)calloc(sizeof(*msg) +
                                    CFG_MAX_INST_VALUE, 1);
        p_msg = (cfg_msg *)msg;

        if (msg == NULL)
            return TE_ENOMEM;

        msg->type = CFG_SET;
        msg->len = sizeof(*msg);
        msg->handle = inst->handle;
        t = msg->val_type = inst->obj->type;
        cfg_types[t].put_to_msg(inst->val, (cfg_msg *)msg);
        msg->local = local;
        cfg_process_msg(&p_msg, TRUE);
        rc = msg->rc;
        free(msg);

        if (rc == 0)
            *change_made = TRUE;

        return rc;
    }
    else
    {
        cfg_add_msg *msg = (cfg_add_msg *)calloc(sizeof(*msg) +
                                                 CFG_MAX_INST_VALUE +
                                                 strlen(inst->oid) + 1, 1);
        cfg_val_type t;
        int          rc;

        if (msg == NULL)
            return TE_ENOMEM;

        if (inst->obj->dependants != NULL)
            *has_deps = TRUE;

        msg->type = CFG_ADD;
        msg->len = sizeof(*msg);
        t = msg->val_type = inst->obj->type;
        cfg_types[t].put_to_msg(inst->val, (cfg_msg *)msg);
        msg->oid_offset = msg->len;
        msg->len += strlen(inst->oid) + 1;
        strcpy((char *)msg + msg->oid_offset, inst->oid);
        msg->local = local;
        cfg_process_msg((cfg_msg **)&msg, TRUE);

        rc = msg->rc;
        free(msg);

        if (rc == 0)
            *change_made = TRUE;

        return rc;
    }
}


static cfg_instance *
topo_sort_instances_rec(cfg_instance *list, unsigned length)
{
    cfg_instance *first;
    cfg_instance *second;
    cfg_instance *result = NULL;
    cfg_instance *prev   = NULL;
    cfg_instance *chosen = NULL;
    unsigned      i;

    if (length <= 1)
        return list;

    first = list;
    for (i = 1; i < length / 2; i++)
        list = list->bkp_next;
    second = list->bkp_next;
    list->bkp_next = NULL;

    first = topo_sort_instances_rec(first, length / 2);
    second = topo_sort_instances_rec(second, length - (length / 2));

    while (first != NULL && second != NULL)
    {
        chosen = (first->obj->ordinal_number < second->obj->ordinal_number ?
                  first : second);

        if (chosen == first)
            first = first->bkp_next;
        else
            second = second->bkp_next;

        if (prev != NULL)
            prev->bkp_next = chosen;
        else
            result        = chosen;
        prev              = chosen;
    }
    chosen = (first != NULL ? first : second);
    if (prev != NULL)
        prev->bkp_next = chosen;
    else
        result        = chosen;
    return result;
}

static cfg_instance *
topo_sort_instances(cfg_instance *list, unsigned int list_size)
{
    cfg_instance *tmp;
    int           seq    = -1;

    list = topo_sort_instances_rec(list, list_size);

    for (tmp = list; tmp != NULL; tmp = tmp->bkp_next)
    {
        if ((int)tmp->obj->ordinal_number < seq)
        {
            ERROR("Dependency order is broken for %s (%u <= %d)",
                  tmp->oid, tmp->obj->ordinal_number, seq);
        }
        seq = tmp->obj->ordinal_number;
    }

    return list;
}

/**
 * Helper function used in restore_entry().
 *
 * @param inst          Instance to restore
 * @param local         Whether to make only local changes to be
 *                      committed later
 * @param need_retry    Will be set to @c TRUE if another attempt
 *                      to restore from backup is needed because
 *                      some instances are missing
 * @param change_made   Will be set to @c TRUE if any change was made
 *                      to configuration
 * @param has_deps      Will be set to @c TRUE if made changes could
 *                      have produced changes in other instances due to
 *                      dependencies
 *
 * @return Status code.
 */
static te_errno
restore_entry_aux(cfg_instance *inst, te_bool local,
                  te_bool *need_retry, te_bool *change_made,
                  te_bool *has_deps)
{
    int rc;
    cfg_instance *child;

    switch (rc = add_or_set(inst, local, has_deps, change_made))
    {
        case 0:
            inst->added = TRUE;
            break;

        case TE_ENOENT:
            /* do nothing */
            *need_retry = TRUE;
            break;

        default:
            ERROR("Failed to add/set instance %s (%r)", inst->oid, rc);
            return rc;
    }

    if (!local)
        return 0;

    /*
     * local=TRUE is used for instances of "unit" objects; all their
     * children should be updated and then all the changes should be
     * committed at once.
     */
    for (child = inst->son; child != NULL; child = child->brother)
    {
        rc = restore_entry_aux(child, local, need_retry,
                               change_made, has_deps);
        if (rc != 0)
            return rc;
    }

    return 0;
}

/**
 * Restore the single instance from backup.
 *
 * @param inst          Instance to restore
 * @param need_retry    Will be set to @c TRUE if another attempt
 *                      to restore from backup is needed because
 *                      some instances are missing
 * @param change_made   Will be set to @c TRUE if any change was made
 *                      to configuration
 * @param has_deps      Will be set to @c TRUE if made changes could
 *                      have produced changes in other instances due to
 *                      dependencies
 *
 * @return Status code.
 */
static te_errno
restore_entry(cfg_instance *inst, te_bool *need_retry,
              te_bool *change_made, te_bool *has_deps)
{
    te_errno rc;
    cfg_commit_msg *msg = NULL;
    size_t size;
    te_bool change_made_aux = FALSE;

    rc = restore_entry_aux(inst, inst->obj->unit,
                           need_retry, &change_made_aux, has_deps);
    if (rc != 0)
        return rc;

    if (change_made_aux)
        *change_made = TRUE;

    if (!inst->obj->unit || !change_made_aux)
        return 0;

    size = sizeof(*msg) + strlen(inst->oid) + 1;
    msg = calloc(size, 1);
    if (msg == NULL)
    {
        ERROR("%s(): failed to allocate commit message", __FUNCTION__);
        return TE_ENOMEM;
    }

    msg->type = CFG_COMMIT;
    msg->len = size;
    strcpy(msg->oid, inst->oid);

    cfg_process_msg((cfg_msg **)&msg, TRUE);
    rc = msg->rc;
    free(msg);

    return rc;
}

/**
 * Comparator used for sorting array of instance pointers
 * according to instance OIDs in alphabetical order.
 *
 * @param arg1      Pointer to the first instance pointer
 * @param arg2      Pointer to the second instance pointer
 *
 * @return Comparison result.
 */
static int
alpha_qsort_predicate(const void *arg1, const void *arg2)
{
    const char *oid1 = (*(cfg_instance **)arg1)->oid;
    const char *oid2 = (*(cfg_instance **)arg2)->oid;
    unsigned int i;

    for (i = 0; ; i++)
    {
        /*
         * '/' must be the first symbol after null byte in our alphabet to
         * ensure that any instance is followed by its children, not by
         * some unrelated nodes.
         * In ASCII '-' precedes '/' for instance, so without this code
         * it can order instances like
         *
         * a/b/c
         * a/b/c-d
         * a/b/c/y
         *
         * instead of
         *
         * a/b/c
         * a/b/c/y
         * a/b/c-d
         *
         * and code in fill_children() will work incorrectly.
         */

        if (oid1[i] == '/' && oid2[i] != '/' && oid2[i] != '\0')
            return -1;
        else if (oid1[i] != '/' && oid1[i] != '\0' && oid2[i] == '/')
            return 1;
        else if (oid1[i] < oid2[i])
            return -1;
        else if (oid1[i] > oid2[i])
            return 1;

        if (oid1[i] == '\0' || oid2[i] == '\0')
            break;
    }

    return 0;
}

/**
 * Fill children lists in list of instances passed to
 * restore_entries().
 *
 * @param list         List of instances
 * @param list_size    Number of instances in the list
 *
 * @return Status code.
 */
static te_errno
fill_children(cfg_instance *list, unsigned int list_size)
{
    cfg_instance **refs = NULL;
    cfg_instance *ref = NULL;
    unsigned int i;
    unsigned int j;
    te_errno rc = 0;

    int level;
    int prev_level = -1;
    cfg_instance *parent = NULL;

    refs = calloc(list_size, sizeof(cfg_instance *));
    if (refs == NULL)
    {
        ERROR("%s(): failed to allocate memory for instance pointers array",
              __FUNCTION__);
        return TE_ENOMEM;
    }

    for (ref = list, i = 0; ref != NULL; ref = ref->bkp_next, i++)
    {
        if (i >= list_size)
        {
            ERROR("%s(): list is longer than expected", __FUNCTION__);
            rc = TE_EINVAL;
            goto finish;
        }
        refs[i] = ref;
    }

    /*
     * Sort list of instances by OID to make it easy to determine
     * children for every instance. Any instance is followed by
     * its direct children after such sorting.
     */
    qsort(refs, list_size, sizeof(*refs), alpha_qsort_predicate);

    for (i = 0; i < list_size; i++)
    {
        /*
         * Compute current level at hierarchy of instances
         * by counting "/" in OID.
         */
        level = 0;
        for (j = 0; refs[i]->oid[j] != '\0'; j++)
        {
            if (refs[i]->oid[j] == '/')
                level++;
        }

        /*
         * Based on current level in hierarchy and the level of the
         * previous instance, find out what instance is a father
         * of the current one.
         */
        parent = (i == 0 ? NULL : refs[i - 1]->father);
        if (prev_level >= 0 && prev_level < level)
        {
            if (prev_level < level - 1)
            {
                ERROR("%s(): an instance %s has no immediate parent",
                      __FUNCTION__, refs[i]->oid);
                rc = TE_EINVAL;
                goto finish;
            }
            parent = refs[i - 1];
        }
        else if (prev_level > level)
        {
            while (parent != NULL && prev_level > level)
            {
                parent = parent->father;
                prev_level--;
            }
        }

        if (parent != NULL)
        {
            if (strcmp_start(parent->oid, refs[i]->oid) != 0)
            {
                ERROR("%s(): %s does not seem to be parent of %s",
                      __FUNCTION__, parent->oid, refs[i]->oid);
                rc = TE_EINVAL;
                goto finish;
            }

            refs[i]->brother = parent->son;
            parent->son = refs[i];
            refs[i]->father = parent;
        }

        prev_level = level;
    }

finish:

    free(refs);
    return rc;
}

/**
 * Add/update entries, mentioned in the configuration file.
 *
 * @param list        List of instances mentioned in the configuration file
 * @param list_size   Number of instances in the list
 * @param subtrees    Vector of the subtrees to restore. May be @c NULL for
 *                    the root subtree
 *
 * @return Status code (see te_errno.h).
 */
static int
restore_entries(cfg_instance *list, unsigned int list_size,
                const te_vec *subtrees)
{
    int           rc;
    te_bool       change_made = FALSE;
    int           n_iterations    = 0;
    te_bool       need_retry      = FALSE;
    cfg_instance *iter;
    te_bool       deps_might_fire = TRUE;

    /*
     * Lists of children are not filled for instances read from a backup
     * file. Fill these lists here.
     * This will be helpful for instances of "unit" objects, which
     * should be restored in a single requests group (commit) together
     * with its children.
     */
    rc = fill_children(list, list_size);
    if (rc != 0)
        return rc;

    list = topo_sort_instances(list, list_size);

    while (deps_might_fire)
    {
        deps_might_fire = FALSE;
        if ((rc = remove_excessive(list, &deps_might_fire, subtrees)) != 0)
        {
            ERROR("Failed to remove excessive entries");
            free_instances(list);
            return rc;
        }

        do
        {
            change_made = FALSE;
            need_retry  = FALSE;
            for (iter = list; iter != NULL; iter = iter->bkp_next)
            {
                if (iter->added || iter->obj->unit_part)
                    continue;

                VERB("Restoring instance %s", iter->oid);

                rc = restore_entry(iter, &need_retry, &change_made,
                                   &deps_might_fire);
                if (rc != 0)
                {
                    free_instances(list);
                    return rc;
                }
            }

        } while (change_made && need_retry);

        if (need_retry)
        {
            free_instances(list);
            return TE_ENOENT;
        }

        if (deps_might_fire)
            cfg_ta_sync("/:", TRUE);
        if (n_iterations++ > 10)
        {
            WARN("Loop dependency suspected, aborting");
            break;
        }
    }

    free_instances(list);

    return 0;
}

/**
 * Process backup structure obtained from "backup" configuration file
 * or backup file.
 *
 * @param backup   the backup structure
 * @param restore  if TRUE, the configuration should be restored after
 *                 unsuccessful dynamic history restoring
 * @param subtrees Vector of the subtrees to restore. May be @c NULL for
 *                 the root.
 *
 * @return status code (errno.h)
 */
int
cfg_backup_process_structure(backup_seq *backup, te_bool restore,
                        const te_vec *subtrees)
{
    cfg_instance *list;
    unsigned int list_size;
    unsigned int first_num_inst;

    int           rc;

    if (backup == NULL || backup->entries_count == 0)
        return 0;

    RING("Processing backup structure from file");

    if ((rc = register_objects(backup, &first_num_inst, !restore)) != 0)
        return rc;

    if ((rc = parse_instances(backup, first_num_inst, &list, &list_size)) != 0)
        return rc;

    if (!restore)
    {
        if ((rc = cfg_ta_sync("/:", TRUE)) != 0)
        {
            ERROR("Cannot synchronize database with Test Agents");
            return rc;
        }
    }

    return restore_entries(list, list_size, subtrees);
}




/**
 * Save current version of the TA subtree,
 * synchronize DB with TA and restore TA configuration.
 *
 * @param ta    TA name
 *
 * @return status code (see te_errno.h)
 */
int
cfg_backup_restore_ta(char *ta)
{
    cfg_instance  *list   = NULL;
    unsigned int list_size = 0;

    cfg_instance  *prev   = NULL;
    cfg_instance  *tmp;
    char           buf[CFG_SUBID_MAX + CFG_INST_NAME_MAX + 1];
    int            rc;
    int            i;

    sprintf(buf, CFG_TA_PREFIX"%s", ta);

    if ((rc = cfg_ta_sync(buf, TRUE)) != 0)
    {
        return rc;
    }

    /* Create list of instances on the TA */
    for (i = 0; i < cfg_all_inst_size; i++)
    {
        cfg_instance *inst = cfg_all_inst[i];

        if (inst == NULL || strncmp(inst->oid, buf, strlen(buf)) != 0)
        {
            continue;
        }

        if ((tmp = (cfg_instance *)calloc(sizeof(*tmp), 1)) == NULL)
        {
            free_instances(list);
            return TE_ENOMEM;
        }
        if ((tmp->oid = strdup(inst->oid)) == NULL)
        {
            free_instances(list);
            free(tmp);
            return TE_ENOMEM;
        }
        if (cfg_types[inst->obj->type].copy(inst->val, &tmp->val) != 0)
        {
            free_instances(list);
            free(tmp->oid);
            free(tmp);
            return TE_ENOMEM;
        }
        tmp->handle = inst->handle;
        tmp->obj = inst->obj;

        if (prev ==  NULL)
            list = tmp;
        else
            prev->bkp_next = tmp;

        list_size++;
        prev = tmp;
    }

    return restore_entries(list, list_size, NULL);
}

/**
 * Count the number of instances in structure
 */
static unsigned int count_instances(cfg_instance *inst)
{
    unsigned int i = 0;
    if (inst != &cfg_inst_root && !cfg_inst_agent(inst) &&
        !cfg_instance_volatile(inst))
        i = 1;

    for (inst = inst->son; inst != NULL; inst = inst->brother)
        i+= count_instances(inst);
    return i;
}

/**
 * Count the number of objects in structure
 */
static unsigned int count_objects(cfg_object *obj)
{
    unsigned int i = 0;

    if (obj != &cfg_obj_root && !cfg_object_agent(obj))
        i = 1;


    for (obj = obj->son; obj != NULL; obj = obj->brother)
        i+= count_objects(obj);
    return i;
}

/**
 * Put description of the object and its (grand-...)children to
 * the backup structure.
 *
 * @param backup backup structure
 * @param pi     pointer to number of the item to put in array
 *               backup->entries[]. The number incremented at the end.
 * @param obj    object
 */
static void
put_objects(backup_seq *backup, unsigned int *pi,  cfg_object *obj)
{
    unsigned int i = *pi;

    backup->entries[i].object = TE_ALLOC(sizeof(object_type));
    if (obj != &cfg_obj_root && !cfg_object_agent(obj))
    {
        backup->entries[i].object->oid = strdup(obj->oid);
        backup->entries[i].object->access = obj->access;
        backup->entries[i].object->type = obj->type;
        if (obj->def_val == NULL)
            backup->entries[i].object->def_val = NULL;
        else
            backup->entries[i].object->def_val = strdup(obj->def_val);
        backup->entries[i].object->unit = obj->unit;
        if (obj->depends_on != NULL)
        {
            unsigned int j;
            cfg_dependency *dep;

            for (dep = obj->depends_on; dep != NULL; dep = dep->next)
                backup->entries[i].object->depends_count++;

            backup->entries[i].object->depends =
                           TE_ALLOC(sizeof(depends_entry) *
                                    backup->entries[i].object->depends_count);

            for (dep = obj->depends_on, j = 0;
                 dep != NULL;
                 dep = dep->next, j++)
            {
                backup->entries[i].object->depends[j].oid =
                                                     strdup(dep->depends->oid);
                backup->entries[i].object->depends[j].scope = dep->object_wide;
            }
        }
    (*pi)++;
    }
    for (obj = obj->son; obj != NULL; obj = obj->brother)
        put_objects(backup, pi, obj);
}

/**
 * Put description of the object instance and its (grand-...)children to
 * the configuration file.
 *
 * @param backup backup structure
 * @param pi     pointer to number of the item to put in array
 *               backup->entries[]. The number incremented at the end.
 * @param inst   object instance
 *
 * @return 0 (success) or TE_ENOMEM
 */
static int
put_instances(backup_seq *backup, unsigned int *pi,
                  cfg_instance *inst)
{
    int i = *pi;
    if (inst != &cfg_inst_root && !cfg_inst_agent(inst) &&
        !cfg_instance_volatile(inst))
    {
        backup->entries[i].instance = TE_ALLOC(sizeof(instance_type));
        backup->entries[i].instance->oid = strdup(inst->oid);

        if (inst->obj->type != CVT_NONE)
        {
            char    *val_str = NULL;
            int      rc;

            rc = cfg_types[inst->obj->type].val2str(inst->val, &val_str);
            if (rc != 0)
            {
                printf("Conversion failed for instance %s type %d\n",
                       inst->oid, inst->obj->type);
                return rc;
            }

            backup->entries[i].instance->value = strdup(val_str);
            free(val_str);
         }
    (*pi)++;
    }
    for (inst = inst->son; inst != NULL; inst = inst->brother)
        if (put_instances(backup, pi, inst) != 0)
            return TE_ENOMEM;

    return 0;
}

static te_errno
put_instance_by_oid(backup_seq *backup, unsigned int *pi,
                        const char *oid)
{
    cfg_instance *inst;

    inst = cfg_get_ins_by_ins_id_str(oid);
    if (inst == NULL)
    {
        ERROR("Failed to find instance with OID %s", oid);
        return TE_ENOENT;
    }

    return put_instances(backup, pi, inst);
}

/**
 * Create "backup" cyaml struture and then serialize it configuration file
 * with specified name.
 *
 * @param filename      name of the file to be created
 * @param subtrees      Vector of the subtrees to create a backup file.
 *                      @c NULL to create backup fo all the subtrees
 *
 * @return status code (errno.h)
 */
int
cfg_backup_create_file(const char *filename, const te_vec *subtrees)
{
    backup_seq *backup = TE_ALLOC(sizeof(backup_seq));
    te_errno rc = 0;
    unsigned int objects_count = count_objects(&cfg_obj_root);
    unsigned int instances_count = count_instances(&cfg_inst_root);
    unsigned int i;

    backup->entries_count = objects_count + instances_count;
    backup->entries = TE_ALLOC(sizeof(backup_entry) *
                               backup->entries_count);
    i = 0;
    put_objects(backup, &i, &cfg_obj_root);

    if (subtrees != NULL && te_vec_size(subtrees) != 0)
    {
        char * const *subtree;

        TE_VEC_FOREACH(subtrees, subtree)
        {
            rc = put_instance_by_oid(backup, &i, *subtree);
            if (rc != 0)
            {
                cfg_yaml_free_backup_seq(backup);
                return rc;
            }
        }
    }
    else
    {
        rc = put_instances(backup, &i, &cfg_inst_root);

        if (rc != 0)
        {
            cfg_yaml_free_backup_seq(backup);
            return rc;
        }
    }

    cfg_yaml_save_backup_file(filename, backup);
    cfg_yaml_free_backup_seq(backup);
    return rc;
}
static te_errno
cfg_backup_wrapper(const char *filename, const te_vec *subtrees, uint8_t op)
{
    cfg_backup_msg *msg;
    size_t len;
    te_errno rc;
    char subtrees_str[RCF_MAX_PATH];

    msg = TE_ALLOC(sizeof(cfg_backup_msg) + PATH_MAX);

    msg->type = CFG_BACKUP;
    msg->op = op;
    msg->len = sizeof(cfg_backup_msg);
    msg->subtrees_num = 0;
    msg->subtrees_offset = msg->len;

    if (subtrees != 0 && te_vec_size(subtrees) != 0)
    {
        char * const *subtree;
        /* -1 since snprintf returns number of bytes without '\0' */
        int written = -1;
        int total_written = 0;

        TE_VEC_FOREACH(subtrees, subtree)
        {
            /* -1 since we should to write the value after '\0' */
            written = snprintf(subtrees_str + written + 1, RCF_MAX_PATH,
                               "%s", *subtree);
            total_written = total_written + written + 1;
            msg->subtrees_num++;
        }

        memcpy((char *)msg + msg->subtrees_offset, subtrees_str,
               total_written);
        msg->len += total_written;
    }

    msg->filename_offset = msg->len;
    len = strlen(filename) + 1;
    memcpy((char *)msg + msg->filename_offset, filename, len);

    cfg_process_msg((cfg_msg **)&msg, FALSE);

    rc = msg->rc;
    free(msg);
    return rc;
}

te_errno
cfg_backup_verify(const char *filename, const te_vec *subtrees)
{
    return cfg_backup_wrapper(filename, subtrees, CFG_BACKUP_VERIFY);
}

te_errno
cfg_backup_restore_nohistory(const char *filename, const te_vec *subtrees)
{
    return cfg_backup_wrapper(filename, subtrees, CFG_BACKUP_RESTORE_NOHISTORY);
}

te_errno
cfg_backup_verify_and_restore(const char *filename, const te_vec *subtrees)
{
    te_errno rc;

    rc = cfg_backup_verify(filename, subtrees);
    if (rc == 0)
        return rc;

    WARN("Configuration differs from backup - try to restore the backup...");

    rc = cfg_backup_restore_nohistory(filename, subtrees);
    if (rc != 0)
    {
        ERROR("%s(): failed to restore from the backup: %r", __FUNCTION__, rc);
        return rc;
    }

    rc = cfg_backup_verify(filename, subtrees);
    if (rc != 0)
        ERROR("%s(): failed to restore subtrees: %r", __FUNCTION__, rc);

    return rc;
}

te_errno
cfg_backup_verify_and_restore_ta_subtrees(const char *filename,
                                          const te_vec *ta_list)
{
    te_vec subtrees = TE_VEC_INIT(char *);
    char * const *ta;
    te_errno rc;

    if (te_vec_size(ta_list) == 0)
        return 0;

    TE_VEC_FOREACH(ta_list, ta)
    {
        rc = te_vec_append_str_fmt(&subtrees, "/agent:%s", *ta);
        if (rc != 0)
            goto out;
    }

    rc = cfg_backup_verify_and_restore(filename, &subtrees);

out:
    te_vec_deep_free(&subtrees);
    return rc;
}
