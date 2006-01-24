/** @file
 * @brief Configurator
 *
 * Backup-related operations
 *
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
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
 *
 * @author Elena Vengerova <Elena.Vengerova@oktetlabs.ru>
 *
 * $Id$
 */

#include "conf_defs.h"

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
        if (node->children != NULL)
        {
            ERROR("<depends> cannot have children");
            xmlFree(oid);
            return TE_EINVAL;
        }
        len = sizeof(*msg) + strlen(oid) + 1;
        msg = calloc(1, len);
        msg->type = CFG_ADD_DEPENDENCY;
        msg->len = len;
        msg->rc = 0;
        msg->handle = dep_handle;
        strcpy(msg->oid, oid);
        cfg_process_msg((cfg_msg **)&msg, TRUE);
        if (msg->rc != 0)
        {
            ERROR("Cannot add dependency for %s: %r", oid, msg->rc);
            rc = msg->rc;
        }
        free(msg);
        xmlFree(oid);
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

        len = sizeof(cfg_register_msg) + strlen(oid) + 1 +
              (def_val == NULL ? 0 : strlen(def_val) + 1);
              
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
        msg->val_type = CVT_NONE;
        strcpy(msg->oid, oid);
        if (def_val != NULL)
        {
            msg->def_val = strlen(msg->oid) + 1;
            strcpy(msg->oid + msg->def_val, def_val);
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
            if (strcmp(attr, "integer") == 0)
                msg->val_type = CVT_INTEGER;
            else if (strcmp(attr, "address") == 0)
                msg->val_type = CVT_ADDRESS;
            else if (strcmp(attr, "string") == 0)
                msg->val_type = CVT_STRING;
            else if (strcmp(attr, "none") != 0)
                RETERR(TE_EINVAL, "Unsupported object type %s", attr);
            xmlFree(attr);
        }

        if ((attr = xmlGetProp(cur, (const xmlChar *)"volatile")) != NULL)
        {
            if (strcmp(attr, "true") == 0)
                msg->vol = TRUE;
            else if (strcmp(attr, "false") != 0)
                RETERR(TE_EINVAL, "Volatile should be specified using "
                       "\"true\" or \"false\"");
            xmlFree(attr);
        }

        if (def_val != NULL)
        {
            cfg_inst_val val;
            
            if (cfg_types[msg->val_type].str2val(def_val, &val) != 0)
                RETERR(TE_EINVAL, "Incorrect default value %s", def_val);
            
            cfg_types[msg->val_type].free(val);
        }

        if ((attr = xmlGetProp(cur, (const xmlChar *)"access")) != NULL)
        {
            if (strcmp(attr, "read_write") == 0)
                msg->access = CFG_READ_WRITE;
            else if (strcmp(attr, "read_only") == 0)
                msg->access = CFG_READ_ONLY;
            else if (strcmp(attr, "read_create") != 0)
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

        tmp->oid = oid;

        if ((tmp->obj = cfg_get_object(oid)) == NULL)
            RETERR(TE_EINVAL, "Cannot find the object for instance %s",
                   oid);
                   
        if (cfg_db_find((char *)oid, &(tmp->handle)) != 0)
            tmp->handle = CFG_HANDLE_INVALID;
        
        val_s = xmlGetProp(cur, (const xmlChar *)"value");
        if (tmp->obj->type != CVT_NONE)
        {
            if (val_s == NULL)
                RETERR(TE_ENOENT, "Value is necessary for %s", oid);

            if ((rc = cfg_types[tmp->obj->type].str2val(val_s, 
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
delete_with_children(cfg_instance *inst)
{
    cfg_del_msg msg = { CFG_DEL, sizeof(cfg_del_msg), 0, 0};
    cfg_msg    *p_msg = (cfg_msg *)&msg;
    int         rc;
    
    cfg_instance *tmp, *next;
    
    if (cfg_instance_volatile(inst))
        return 0;
        
    if (inst->obj->access != CFG_READ_CREATE)
        return 0;
        
    for (tmp = inst->son; tmp != NULL; tmp = next)
    {
        next = tmp->brother;
        if ((rc = delete_with_children(tmp)) != 0)
            return rc;
    }
        
    msg.handle = inst->handle;
    
    cfg_process_msg(&p_msg, TRUE);
    
    return TE_RC_GET_ERROR(msg.rc) == TE_ENOENT ? 0 : msg.rc;
}


/**
 * Return all read/create instances, not mentioned in the configuration
 * file.
 *
 * @param root  root of subtree for which excessive entries should
 *              be removed
 * @param list  list of instances mentioned in the configuration file
 *
 * @return status code (see te_errno.h)
 */
static int
remove_excessive(char *root, cfg_instance *list)
{
    int rc;
    int i;
    
    for (i = 0; i < cfg_all_inst_size; i++)
    {
        cfg_instance *tmp;
        
        if (cfg_all_inst[i] == NULL || 
            cfg_all_inst[i]->obj->access != CFG_READ_CREATE ||
            strncmp(cfg_all_inst[i]->oid, root, strlen(root)) != 0)
        {
            continue;
        }
        for (tmp = list; tmp != NULL; tmp = tmp->brother)
        {
            if (strcmp(tmp->oid, cfg_all_inst[i]->oid) == 0)
                break;
        }
        
        if (tmp != NULL)
            continue;
            
        if ((rc = delete_with_children(cfg_all_inst[i])) != 0)
            return rc;
    }
    
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
add_or_set(cfg_instance *inst)
{
    if (cfg_inst_agent(inst))
        return 0;
        
    /* Entry may appear after addition of previous ones */
    if (inst->handle == CFG_HANDLE_INVALID)
        cfg_db_find(inst->oid, &inst->handle);
        
    if (inst->handle != CFG_HANDLE_INVALID)
    {
        cfg_set_msg *msg;
        cfg_msg     *p_msg;
        cfg_val_type t;
        int          rc;
        
        if (CFG_GET_INST(inst->handle) == 0)
            return TE_EINVAL;
        
        if ((inst->obj->type != CVT_INTEGER &&
             inst->obj->type != CVT_STRING &&
             inst->obj->type != CVT_ADDRESS) ||
             cfg_types[inst->obj->type].is_equal(
                 inst->val, CFG_GET_INST(inst->handle)->val))
        {
            return 0;
        }
        
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

/**
 * Add/update entries, mentioned in the configuration file.
 *
 * @param list  list of instances mentioned in the configuration file
 *
 * @return status code (see te_errno.h)
 */
static int
restore_entries(cfg_instance *list)
{
    while (list != NULL)
    {
        cfg_instance *prev = NULL;
        cfg_instance *tmp = list;
        te_bool       done = FALSE;
        int           rc;

        
        while (tmp != NULL)
        {
            RING("Restoring instance %s", tmp->oid);

            switch (rc = add_or_set(tmp))
            {
                case 0:
                {
                    done = TRUE;
                    if (prev != NULL)
                    {
                        prev->brother = tmp->brother;
                        tmp->brother = NULL;
                        free_instances(tmp);
                        tmp = prev->brother;
                    }
                    else
                    {
                        list = tmp->brother;
                        tmp->brother = NULL;
                        free_instances(tmp);
                        tmp = list;
                    }
                    break;
                }
                
                case TE_ENOENT:
                    prev = tmp;
                    tmp = tmp->brother;
                    break;
                    
                default:
                    free_instances(list);
                    return rc;
            }
        }
        if (list != NULL && !done)
        {
            free_instances(list);
            return TE_ENOENT;
        }
    }
    
    return 0;
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
 * Process "backup" configuration file or backup file.
 *
 * @param node    <backup> node pointer
 * @param restore if TRUE, the configuration should be restored after
 *                unsuccessful dynamic history restoring
 *
 * @return status code (errno.h)
 */
int 
cfg_backup_process_file(xmlNodePtr node, te_bool restore)
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
            free_instances(list);
            return rc;
        }
    }

    cfg_order_objects_topologically();

    if ((rc = remove_excessive("/", list)) != 0)
    {
        ERROR("Failed to remove excessive entries");
        free_instances(list);
        return rc;
    }

    list = topo_sort_instances(list);
    
    return restore_entries(list);
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

    /* Topologically sort instances */
    cfg_order_objects_topologically();

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

    if ((rc = cfg_ta_sync(buf, TRUE)) != 0)
    {
        free_instances(list);
        return rc;
    }
    
    if ((rc = remove_excessive(buf, list)) != 0)
    {
        free_instances(list);
        return rc;
    }

    list = topo_sort_instances(list);
    
    return restore_entries(list);
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
                obj->type == CVT_ADDRESS ? "address" : "string");
                
        if (obj->def_val != NULL)
        {
            xmlChar *xml_str = xmlEncodeEntitiesReentrant(NULL,
                                                          obj->def_val);

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
                fprintf(f, "    <depends oid=\"%s\" />\n", 
                        dep->depends->oid);
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
        fprintf(f, "\n  <instance oid=\"%s\" ", inst->oid);
        
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

            xml_str = xmlEncodeEntitiesReentrant(NULL, val_str);
            free(val_str);
            if (xml_str == NULL)
                return TE_ENOMEM;

            fprintf(f, "value=\"%s\"", xml_str);
            free(xml_str);
         }
         fprintf(f, "/>\n"); 
    }
    for (inst = inst->son; inst != NULL; inst = inst->brother)
        if (put_instance(f, inst) != 0)
            return TE_ENOMEM;
        
    return 0;
}

/**
 * Create "backup" configuration file with specified name.
 *
 * @param filename      name of the file to be created
 *
 * @return status code (errno.h)
 */
int 
cfg_backup_create_file(const char *filename)
{
    FILE *f= fopen(filename, "w");
    
    if (f == NULL)
        return TE_OS_RC(TE_CS, errno);;
        
    fprintf(f, "<?xml version=\"1.0\"?>\n");
    fprintf(f, "<backup>\n");

    put_object(f, &cfg_obj_root);
    if (put_instance(f, &cfg_inst_root) != 0)
    {
        fclose(f);
        unlink(filename);
        return TE_ENOMEM;
    }
                
    fprintf(f, "\n</backup>\n");
    fclose(f);
    return 0;
}

