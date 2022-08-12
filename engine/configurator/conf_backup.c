/** @file
 * @brief Configurator
 *
 * Backup-related operations
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 *
 */

#include "conf_defs.h"
#include "te_alloc.h"

/**
 * Parses all object dependencies in the configuration file.
 * Note: this function is also used in conf_dh.c.
 *
 * @param node     first dependency node
 */
int
cfg_register_dependency(xmlNodePtr node, const char *dependant)
{
    xmlChar                *oid = NULL;
    xmlChar                *scope = NULL;
    cfg_add_dependency_msg *msg = NULL;
    cfg_handle              dep_handle;

    int      rc = 0;
    unsigned len;

    VERB("Registering dependencies for %s", dependant);

    rc = cfg_db_find(dependant, &dep_handle);
    if (rc != 0)
    {
        ERROR("Cannot find a dependant OID: %r", TE_RC(TE_CS, rc));
        return rc;
    }

    for (; node != NULL && rc == 0; node = node->next)
    {
        if (xmlStrcmp(node->name, (const xmlChar *)"comment") == 0 ||
            xmlStrcmp(node->name, (const xmlChar *)"text") == 0)
            continue;
        if (xmlStrcmp(node->name, (const xmlChar *)"depends") != 0)
        {
            ERROR("Invalid dependency tag: <%s>", node->name);
            return TE_EINVAL;
        }
        oid = xmlGetProp(node, (const xmlChar *)"oid");
        if (oid == NULL)
        {
            ERROR("Missing OID attribute in <depends>");
            return TE_EINVAL;
        }
        scope = xmlGetProp(node, (const xmlChar *)"scope");

        if (node->children != NULL)
        {
            ERROR("<depends> cannot have children");
            xmlFree(oid);
            return TE_EINVAL;
        }
        len = sizeof(*msg) + strlen((char *)oid) + 1;
        msg = calloc(1, len);
        msg->type = CFG_ADD_DEPENDENCY;
        msg->len = len;
        msg->rc = 0;
        msg->handle = dep_handle;
        msg->object_wide = (scope != NULL &&
                            xmlStrcmp(scope,
                                      (const xmlChar *)"object") == 0);
        strcpy(msg->oid, (char *)oid);
        cfg_process_msg((cfg_msg **)&msg, TRUE);
        if (msg->rc != 0)
        {
            ERROR("Cannot add dependency for %s: %r", oid, msg->rc);
            rc = msg->rc;
        }
        free(msg);
        xmlFree(oid);
        xmlFree(scope);
    }
    return rc;
}

/**
 * Parse all objects specified in the configuration file.
 *
 * @param node     first object node
 * @param reg      if TRUE, register objects
 *
 * @return status code (see te_errno.h)
 */
static int
register_objects(xmlNodePtr *node, te_bool reg)
{
    xmlNodePtr cur = *node;

    for (; cur != NULL; cur = cur->next)
    {
        cfg_register_msg *msg;

        xmlChar   *oid  = NULL;
        xmlChar   *def_val  = NULL;
        xmlChar   *attr = NULL;
        xmlChar   *parent_dep = NULL;
        int        len;

        if ((xmlStrcmp(cur->name , (const xmlChar *)"comment") == 0) ||
            (xmlStrcmp(cur->name , (const xmlChar *)"text") == 0))
            continue;

        if (xmlStrcmp(cur->name , (const xmlChar *)"object") != 0)
            break;

        if (!reg)
            continue;

        if ((oid = xmlGetProp(cur, (const xmlChar *)"oid")) == NULL)
        {
            ERROR("Incorrect description of the object %s", cur->name);
            return TE_EINVAL;
        }

        def_val = xmlGetProp(cur, (const xmlChar *)"default");

        parent_dep = xmlGetProp(cur, (const xmlChar *)"parent-dep");

        len = sizeof(cfg_register_msg) + strlen((char *)oid) + 1 +
              (def_val == NULL ? 0 : strlen((char *)def_val) + 1);

        if ((msg = (cfg_register_msg *)calloc(1, len)) == NULL)
        {
            xmlFree(oid);
            xmlFree(def_val);
            return TE_ENOMEM;
        }

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
        if (def_val != NULL)
        {
            msg->def_val = strlen(msg->oid) + 1;
            strcpy(msg->oid + msg->def_val, (char *)def_val);
        }
        xmlFree(oid);
        xmlFree(def_val);

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
        ERROR(_str);          \
        if (attr != NULL)       \
            xmlFree(attr);      \
        free(msg);              \
        return _err;            \
    } while (0)

        if ((attr = xmlGetProp(cur, (const xmlChar *)"type")) != NULL)
        {
            if (strcmp((char *)attr, "integer") == 0)
                msg->val_type = CVT_INTEGER;
            else if (strcmp((char *)attr, "uint64") == 0)
                msg->val_type = CVT_UINT64;
            else if (strcmp((char *)attr, "address") == 0)
                msg->val_type = CVT_ADDRESS;
            else if (strcmp((char *)attr, "string") == 0)
                msg->val_type = CVT_STRING;
            else if (strcmp((char *)attr, "none") != 0)
                RETERR(TE_EINVAL, "Unsupported object type %s", attr);
            xmlFree(attr);
        }

        if ((attr = xmlGetProp(cur, (const xmlChar *)"volatile")) != NULL)
        {
            if (strcmp((char *)attr, "true") == 0)
                msg->vol = TRUE;
            else if (strcmp((char *)attr, "false") != 0)
                RETERR(TE_EINVAL, "Volatile should be specified using "
                       "\"true\" or \"false\"");
            xmlFree(attr);
        }

        if (def_val != NULL)
        {
            cfg_inst_val val;

            if (cfg_types[msg->val_type].str2val((char *)def_val, &val)
                    != 0)
                RETERR(TE_EINVAL, "Incorrect default value %s", def_val);

            cfg_types[msg->val_type].free(val);
        }

        if ((attr = xmlGetProp(cur, (const xmlChar *)"access")) != NULL)
        {
            if (strcmp((char*)attr, "read_write") == 0)
                msg->access = CFG_READ_WRITE;
            else if (strcmp((char *)attr, "read_only") == 0)
                msg->access = CFG_READ_ONLY;
            else if (strcmp((char *)attr, "read_create") != 0)
                RETERR(TE_EINVAL,
                       "Wrong value %s of \"access\" attribute", attr);
            xmlFree(attr);
        }
        attr = NULL;

        cfg_process_msg((cfg_msg **)&msg, TRUE);
        if (msg->rc != 0)
            RETERR(msg->rc, "Failed to register object %s", msg->oid);

        cfg_register_dependency(cur->children, msg->oid);

        free(msg);
        msg = NULL;
#undef RETERR
    }

    *node = cur;
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
        next = list->brother;
        if (list->obj->type != CVT_NONE)
            cfg_types[list->obj->type].free(list->val);
        free(list->oid);
        free(list);
    }
}

/**
 * Parse instance nodes of the configuration file to list of instances.
 *
 * @param node      first instance node
 * @param list      location for instance list pointer
 *
 * @return status code (see te_errno.h)
 */
static int
parse_instances(xmlNodePtr node, cfg_instance **list)
{
    cfg_instance *prev = NULL;
    xmlNodePtr    cur = node;
    int           rc;

    *list = NULL;

/**
 * Log error, deallocate resource and return from function.
 *
 * @param _rc   - return code
 * @param _str  - log message
 */
#define RETERR(_err, _str...)   \
    do {                        \
        ERROR(_str);          \
        if (oid != NULL)        \
            xmlFree(oid);       \
        if (val_s != NULL)      \
            xmlFree(val_s);     \
        free_instances(*list);  \
        return _err;            \
    } while (0)

    for (; cur != NULL; cur = cur->next)
    {
        cfg_instance *tmp;
        xmlChar      *oid   = NULL;
        xmlChar      *val_s = NULL;

        if ((xmlStrcmp(cur->name , (const xmlChar *)"comment") == 0) ||
            (xmlStrcmp(cur->name , (const xmlChar *)"text") == 0))
            continue;

        if (xmlStrcmp(cur->name , (const xmlChar *)"instance") != 0)
        {
            RETERR(TE_EINVAL, "Incorrect node %s", cur->name);
        }

        if (cur->xmlChildrenNode != NULL ||
            (oid = xmlGetProp(cur, (const xmlChar *)"oid")) == NULL)
        {
            RETERR(TE_EINVAL, "Incorrect description of the object "
                           "instance %s", cur->name);
        }

        if ((tmp = (cfg_instance *)calloc(sizeof(*tmp), 1)) == NULL)
            RETERR(TE_ENOMEM, "No enough memory");

        tmp->oid = (char *)oid;

        if ((tmp->obj = cfg_get_object((char *)oid)) == NULL)
            RETERR(TE_EINVAL, "Cannot find the object for instance %s",
                   oid);

        if (cfg_db_find((char *)oid, &(tmp->handle)) != 0)
            tmp->handle = CFG_HANDLE_INVALID;

        val_s = xmlGetProp(cur, (const xmlChar *)"value");
        if (tmp->obj->type != CVT_NONE)
        {
            if (val_s == NULL)
                RETERR(TE_ENOENT, "Value is necessary for %s", oid);

            if ((rc = cfg_types[tmp->obj->type].str2val((char *)val_s,
                                                        &(tmp->val))) != 0)
            {
                RETERR(rc, "Value conversion error for %s", oid);
            }
            xmlFree(val_s);
            val_s = NULL;
        }
        else if (val_s != NULL)
            RETERR(TE_EINVAL, "Value is prohibited for %s", oid);
#undef RETERR

        if (prev != NULL)
            prev->brother = tmp;
        else
            *list = tmp;

        prev = tmp;
    }

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
     * thig - this may hide a bug or pospone it's discovery
     * to upcomming tests.
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
 * @param subtrees Vector of the subtreees.
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

        for (tmp = list; tmp != NULL; tmp = tmp->brother)
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
 * @param inst  object instance to be added or changed
 *
 * @return status code (see errno.h)
 */
static int
add_or_set(cfg_instance *inst, te_bool *has_deps)
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

        if ((inst->obj->type != CVT_INTEGER &&
             inst->obj->type != CVT_UINT64 &&
             inst->obj->type != CVT_STRING &&
             inst->obj->type != CVT_ADDRESS) ||
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
        cfg_process_msg(&p_msg, TRUE);
        rc = msg->rc;
        free(msg);
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
        cfg_process_msg((cfg_msg **)&msg, TRUE);

        rc = msg->rc;
        free(msg);
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
        list = list->brother;
    second = list->brother;
    list->brother = NULL;

    first = topo_sort_instances_rec(first, length / 2);
    second = topo_sort_instances_rec(second, length - (length / 2));

    while (first != NULL && second != NULL)
    {
        chosen = (first->obj->ordinal_number < second->obj->ordinal_number ?
                  first : second);

        if (chosen == first)
            first = first->brother;
        else
            second = second->brother;

        if (prev != NULL)
            prev->brother = chosen;
        else
            result        = chosen;
        prev              = chosen;
    }
    chosen = (first != NULL ? first : second);
    if (prev != NULL)
        prev->brother = chosen;
    else
        result        = chosen;
    return result;
}

static cfg_instance *
topo_sort_instances(cfg_instance *list)
{
    unsigned      length = 0;
    cfg_instance *tmp;
    int           seq    = -1;

    for (tmp = list; tmp != NULL; tmp = tmp->brother)
        length++;

    list = topo_sort_instances_rec(list, length);

    for (tmp = list; tmp != NULL; tmp = tmp->brother)
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
 * Add/update entries, mentioned in the configuration file.
 *
 * @param list     list of instances mentioned in the configuration file
 * @param subtrees Vector of the subtrees to restore. May be @c NULL for
 *                 the root subtree.
 *
 * @return status code (see te_errno.h)
 */
static int
restore_entries(cfg_instance *list, const te_vec *subtrees)
{
    int           rc;
    int           n_processed     = 0;
    int           n_iterations    = 0;
    te_bool       need_retry      = FALSE;
    cfg_instance *iter;
    te_bool       deps_might_fire = TRUE;

    list = topo_sort_instances(list);

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
            n_processed = 0;
            need_retry  = FALSE;
            for (iter = list; iter != NULL; iter = iter->brother)
            {
                if (iter->added)
                    continue;

                VERB("Restoring instance %s", iter->oid);

                switch (rc = add_or_set(iter, &deps_might_fire))
                {
                    case 0:
                        iter->added = TRUE;
                        n_processed++;
                        break;
                    case TE_ENOENT:
                        /* do nothing */
                        need_retry = TRUE;
                        break;
                    default:
                        ERROR("Failed to add/set instance %s (%r)", iter->oid, rc);
                        free_instances(list);
                        return rc;
                }
            }
        } while(n_processed != 0 && need_retry);

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
 * Process "backup" configuration file or backup file.
 *
 * @param node     <backup> node pointer
 * @param restore  if TRUE, the configuration should be restored after
 *                 unsuccessful dynamic history restoring
 * @param subtrees Vector of the subtrees to restore. May be @c NULL for
 *                 the root.
 *
 * @return status code (errno.h)
 */
int
cfg_backup_process_file(xmlNodePtr node, te_bool restore,
                        const te_vec *subtrees)
{
    cfg_instance *list;
    xmlNodePtr    cur         = node->xmlChildrenNode;
    int           rc;

    if (cur == NULL)
        return 0;

    RING("Processing backup file");

    if ((rc = register_objects(&cur, !restore)) != 0)
        return rc;

    if ((rc = parse_instances(cur, &list)) != 0)
        return rc;

    if (!restore)
    {
        if ((rc = cfg_ta_sync("/:", TRUE)) != 0)
        {
            ERROR("Cannot synchronize database with Test Agents");
            return rc;
        }
    }

    return restore_entries(list, subtrees);
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
            prev->brother = tmp;

        prev = tmp;
    }

    return restore_entries(list, NULL);
}

/**
 * Put description of the object and its (grand-...)children to
 * the configuration file.
 *
 * @param f      opened configuration file
 * @param obj   object
 */
static void
put_object(FILE *f, cfg_object *obj)
{
    if (obj != &cfg_obj_root && !cfg_object_agent(obj))
    {
        fprintf(f, "\n  <object oid=\"%s\" "
                "access=\"%s\" type=\"%s\"",
                obj->oid,
                obj->access == CFG_READ_CREATE ? "read_create" :
                obj->access == CFG_READ_WRITE ? "read_write" : "read_only",
                obj->type == CVT_NONE ? "none" :
                obj->type == CVT_INTEGER ? "integer" :
                obj->type == CVT_UINT64 ? "uint64" :
                obj->type == CVT_ADDRESS ? "address" : "string");

        if (obj->def_val != NULL)
        {
            xmlChar *xml_str = xmlEncodeEntitiesReentrant(NULL,
                                   (const xmlChar *)obj->def_val);

            if (xml_str == NULL)
            {
                ERROR("xmlEncodeEntitiesReentrant() failed");
                return;
            }
            fprintf(f, " default=\"%s\"", xml_str);
            xmlFree(xml_str);
        }
        if (obj->depends_on == NULL)
            fprintf(f, "/>\n");
        else
        {
            cfg_dependency *dep;
            fputs(">\n", f);
            for (dep = obj->depends_on; dep != NULL; dep = dep->next)
            {
                fprintf(f, "    <depends oid=\"%s\" scope=\"%s\"/>\n",
                        dep->depends->oid,
                        dep->object_wide ? "object" : "instance");
            }
            fputs("  </object>\n", f);
        }
    }
    for (obj = obj->son; obj != NULL; obj = obj->brother)
        put_object(f, obj);
}

/**
 * Put description of the object instance and its (grand-...)children to
 * the configuration file.
 *
 * @param f      opened configuration file
 * @param inst   object instance
 *
 * @return 0 (success) or TE_ENOMEM
 */
static int
put_instance(FILE *f, cfg_instance *inst)
{
    if (inst != &cfg_inst_root && !cfg_inst_agent(inst) &&
        !cfg_instance_volatile(inst))
    {
        fprintf(f, "\n  <instance oid=\"%s\"", inst->oid);

        if (inst->obj->type != CVT_NONE)
        {
            char    *val_str = NULL;
            xmlChar *xml_str;
            int      rc;

            rc = cfg_types[inst->obj->type].val2str(inst->val, &val_str);
            if (rc != 0)
            {
                printf("Conversion failed for instance %s type %d\n",
                       inst->oid, inst->obj->type);
                return rc;
            }

            xml_str = xmlEncodeEntitiesReentrant(NULL, (xmlChar *)val_str);
            free(val_str);
            if (xml_str == NULL)
                return TE_ENOMEM;

            fprintf(f, " value=\"%s\"", xml_str);
            free(xml_str);
         }
         fprintf(f, "/>\n");
    }
    for (inst = inst->son; inst != NULL; inst = inst->brother)
        if (put_instance(f, inst) != 0)
            return TE_ENOMEM;

    return 0;
}

static te_errno
put_instance_by_oid(FILE *f, const char *oid)
{
    cfg_instance *inst;

    inst = cfg_get_ins_by_ins_id_str(oid);
    if (inst == NULL)
    {
        ERROR("Failed to find instance with OID %s", oid);
        return TE_ENOENT;
    }

    return put_instance(f, inst);
}

/**
 * Create "backup" configuration file with specified name.
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
    FILE *f= fopen(filename, "w");
    te_errno rc;

    if (f == NULL)
        return TE_OS_RC(TE_CS, errno);;

    fprintf(f, "<?xml version=\"1.0\"?>\n");
    fprintf(f, "<backup>\n");

    put_object(f, &cfg_obj_root);

    if (subtrees != NULL && te_vec_size(subtrees) != 0)
    {
        char * const *subtree;

        TE_VEC_FOREACH(subtrees, subtree)
        {
            rc = put_instance_by_oid(f, *subtree);
            if (rc != 0)
            {
                fclose(f);
                unlink(filename);
                return rc;
            }
        }
    }
    else
    {
        rc = put_instance(f, &cfg_inst_root);
        if (rc != 0)
        {
            fclose(f);
            unlink(filename);
            return rc;
        }
    }

    fprintf(f, "\n</backup>\n");
    fclose(f);
    return 0;
}

te_errno
cfg_backup_create_filter_file(const char *filename, const te_vec *subtrees)
{
    FILE *f;
    char * const *subtree;

    f = fopen(filename, "w");
    if (f == NULL)
    {
        ERROR("Failed to open '%s': %s", filename, strerror(errno));
        return te_rc_os2te(errno);
    }

    fprintf(f, "<?xml version=\"1.0\"?>\n");
    fprintf(f, "<filters>\n");

    TE_VEC_FOREACH(subtrees, subtree)
    {
        fprintf(f, "<subtree>%s</subtree>\n", *subtree);
    }

    fprintf(f, "</filters>\n");
    fclose(f);
    return 0;
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
