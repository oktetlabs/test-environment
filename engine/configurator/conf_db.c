/** @file
 * @brief Configurator
 *
 * Support of Configurator database
 *
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

#define CFG_OBJ_NUM     64      /**< Number of objects */
#define CFG_INST_NUM    128     /**< Number of object instances */

/* Forwards */
static cfg_object cfg_obj_agent;
static cfg_object cfg_obj_agent_rsrc;
static cfg_object cfg_obj_conf_delay;
static cfg_object cfg_obj_conf_delay_ta;

/** Root of configuration objects */
cfg_object cfg_obj_root = 
    { 0, "/", { 0 }, CVT_NONE, CFG_READ_ONLY, NULL, FALSE, NULL,
      &cfg_obj_agent, NULL, CFG_DEP_INITIALIZER };
      
/** "/agent" object */
static cfg_object cfg_obj_agent = 
    { 1, "/agent", { 'a', 'g', 'e', 'n', 't', 0 },
      CVT_NONE, CFG_READ_ONLY, NULL, FALSE, &cfg_obj_root,  
      &cfg_obj_agent_rsrc, &cfg_obj_conf_delay, CFG_DEP_INITIALIZER };

/** "/agent/rsrc" object */
static cfg_object cfg_obj_agent_rsrc = 
    { 2, "/agent/rsrc", { 'r', 's', 'r', 'c', 0 },
      CVT_STRING, CFG_READ_CREATE, NULL, FALSE, &cfg_obj_agent, 
      NULL, NULL, CFG_DEP_INITIALIZER };

/** "/conf_delay" object */
static cfg_object cfg_obj_conf_delay = 
    { 3, "/conf_delay", 
      { 'c', 'o', 'n', 'f', '_', 'd', 'e', 'l', 'a', 'y', 0 },
      CVT_STRING, CFG_READ_CREATE, NULL, TRUE, &cfg_obj_root, 
      &cfg_obj_conf_delay_ta, NULL, CFG_DEP_INITIALIZER };

/** "/conf_delay/ta" object */
static cfg_object cfg_obj_conf_delay_ta = 
    { 4, "/conf_delay/ta", { 't', 'a', 0 },
      CVT_INTEGER, CFG_READ_CREATE, NULL, TRUE, &cfg_obj_conf_delay, 
      NULL, NULL, CFG_DEP_INITIALIZER };

      
/** Pool with configuration objects */
cfg_object **cfg_all_obj = NULL;
/** Size of objects pool */
int cfg_all_obj_size;

/** Root of configuration instances */
cfg_instance cfg_inst_root = 
    { 0x10000, "/:", { 0 }, &cfg_obj_root, TRUE, NULL, NULL, NULL, 
      { 0 } };
/** Pool with configuration instances */
cfg_instance **cfg_all_inst = NULL;
/** Size of instances pool */
int cfg_all_inst_size;
/** Maximum index ever used in the cfg_all_inst */
static int cfg_all_inst_max = 1;

/** Unique sequence number of the next instance */
int cfg_inst_seq_num = 2;

/** Delay for configuration changes accomodation */
uint32_t cfg_conf_delay;

/* Locals */
static int pattern_match(char *pattern, char *str);

/** Description for a dependency referenced 
 *  before its master object
 */
typedef struct cfg_orphan
{
    cfg_object *object;       /**< Dependant object */
    cfg_oid    *master;       /**< Master object */
    te_bool     object_wide;  /**< Whether the dependency is object-wide */
    struct cfg_orphan *next;  /**< Next dependency */
    struct cfg_orphan *prev;  /**< Previous dependency */
} cfg_orphan;

/** List of dependencies referenced before their master objects are
 *  registered 
 */
static cfg_orphan *orphaned_objects;

/** Ordered list of all objects such that a master object always
 *  preceeds all its dependant objects 
 */
static cfg_object *topological_order;

/**
 * Find a place for a new record in the topologically sorted list.
 * The idea is that when a new dependency is added, 
 * we move a dependent object beyond all the objects with 
 * the same or lesser number of master objects.
 *
 * Actually, this is a "real-time" version of an topological sorting
 * algorithm given in Knuth 2.2.3 (Algorithm T).
 *
 * NOTE: there is no explicit checking for loops.
 */
static void
cfg_put_in_order_dep(cfg_object *obj)
{
    cfg_object     *prev;
    cfg_object     *place;
    cfg_dependency *dep_iter;
    
    if (obj->dep_next != NULL && 
        obj->dep_next->ordinal_number <= obj->ordinal_number)
    {
        for (prev = obj, place = obj->dep_next; 
             place != NULL; 
             prev = place, place = place->dep_next)
        {
            if (place->ordinal_number > obj->ordinal_number)
                break;
        }
        if (obj->dep_next != NULL)
            obj->dep_next->dep_prev = obj->dep_prev;
        if (obj->dep_prev != NULL)
            obj->dep_prev->dep_next = obj->dep_next;
        else
            topological_order = obj->dep_next;
        prev->dep_next = obj;
        if (place != NULL)
            place->dep_prev = obj;
        obj->dep_next = place;
        obj->dep_prev = prev;
    }
    for (dep_iter = obj->dependants; 
         dep_iter != NULL; 
         dep_iter = dep_iter->next)
    {
        if (dep_iter->depends->ordinal_number <= obj->ordinal_number)
        {
            dep_iter->depends->ordinal_number = obj->ordinal_number + 1;
            cfg_put_in_order_dep(dep_iter->depends);
        }
    }
}


/**
 * Create a dependency record.
 *
 * @param master        Master object
 * @param obj           Dependant object
 * @param object_wide   If TRUE, the dependency is between
 *                      the master object and dependant instances
 */
static void
cfg_create_dep(cfg_object *master, cfg_object *obj, te_bool object_wide)
{
    cfg_dependency *newdep;

    VERB("Creating a dependency %s to %s", obj->oid, master->oid);

    newdep = calloc(1, sizeof(*newdep));
    if (newdep == NULL)
    {
        ERROR("%s(): calloc() failed", __FUNCTION__);
        return;
    }

    newdep->next = obj->depends_on;
    newdep->depends = master;
    newdep->object_wide = object_wide;
    obj->depends_on = newdep;
    if (master->ordinal_number >= obj->ordinal_number)
    {
        obj->ordinal_number = master->ordinal_number + 1;
    }
    cfg_put_in_order_dep(obj);

    /* 
     * Now we add the object to the dependants list of its master, keeping
     * the list in lexicographic order
     */
    newdep = calloc(1, sizeof(*newdep));
    if (newdep == NULL)
    {
        ERROR("%s(): calloc() failed", __FUNCTION__);
        return;
    }

    newdep->depends = obj;
    newdep->object_wide = object_wide;

    if (master->dependants == NULL)
        master->dependants = newdep;
    else
    {
        cfg_dependency *dep_iter;

        for (dep_iter = master->dependants; ; dep_iter = dep_iter->next)
        {
            if (dep_iter->next == NULL ||
                strcmp(dep_iter->depends->oid, obj->oid) > 0)
            {
                newdep->next = dep_iter->next;
                dep_iter->next = newdep;
                break;
            }
        }
    }
}

/**
 * Destroy dependency list.
 *
 * @param deps   Dependency list
 */
static void
cfg_destroy_deps(cfg_dependency *deps)
{
    cfg_dependency *next;
    
    for (; deps != NULL; deps = next)
    {
        next = deps->next;
        free(deps);
    }
}

/**
 * Initialize the database during startup or re-initialization.
 *
 * @return 0 (success) or TE_ENOMEM
 */
int
cfg_db_init(void)
{
    cfg_db_destroy();
    if ((cfg_all_obj = (cfg_object **)calloc(CFG_OBJ_NUM, 
                                             sizeof(void *))) == NULL)
    {
        ERROR("Out of memory");
        return TE_ENOMEM;
    }
    cfg_all_obj_size = CFG_OBJ_NUM;
    cfg_all_obj[0] = &cfg_obj_root;
    cfg_all_obj[1] = &cfg_obj_agent;
    cfg_all_obj[2] = &cfg_obj_agent_rsrc;
    cfg_all_obj[3] = &cfg_obj_conf_delay;
    cfg_all_obj[4] = &cfg_obj_conf_delay_ta;
    cfg_obj_root.son = &cfg_obj_agent;
    cfg_obj_agent.son = &cfg_obj_agent_rsrc;
    cfg_obj_agent_rsrc.brother = NULL;
    cfg_obj_conf_delay.brother = NULL;

    if ((cfg_all_inst = (cfg_instance **)calloc(CFG_INST_NUM, 
                                                sizeof(void *))) == NULL)
    {
        free(cfg_all_obj);
        cfg_all_obj = NULL;
        ERROR("Out of memory");
        return TE_ENOMEM;
    }
    cfg_all_inst_size = CFG_INST_NUM;
    cfg_all_inst[0] = &cfg_inst_root;
    cfg_inst_root.son = NULL;
    
    return cfg_ta_add_agent_instances();
}

/**
 * Destroy the database before shutdown.
 */
void 
cfg_db_destroy(void)
{
    int i;

    if (cfg_all_obj == NULL)
        return;

    INFO("Destroy instances");
    for (i = 1; i < cfg_all_inst_size; i++)
    {
        if (cfg_all_inst[i] != NULL)
        {
            cfg_types[cfg_all_inst[i]->obj->type].
                free(cfg_all_inst[i]->val);
            free(cfg_all_inst[i]->oid);
            free(cfg_all_inst[i]);
        }
    }
    free(cfg_all_inst);
    cfg_all_inst = NULL;

    INFO("Destroy objects");
    for (i = 5; i < cfg_all_obj_size; i++)
    {
        if (cfg_all_obj[i] != NULL)
        {
            free(cfg_all_obj[i]->oid);
            free(cfg_all_obj[i]->def_val);
            cfg_destroy_deps(cfg_all_obj[i]->depends_on);
            cfg_destroy_deps(cfg_all_obj[i]->dependants);
            free(cfg_all_obj[i]);
        }
    }
    free(cfg_all_obj);
    cfg_all_obj = NULL;
}

static void
cfg_maybe_adopt_objects (cfg_object *master, cfg_oid *oid)
{
    cfg_orphan *iter, *next;
    
    for (iter = orphaned_objects; iter != NULL; iter = next)
    {
        next = iter->next;
        if (cfg_oid_cmp(oid, iter->master) == 0)
        {
            VERB("Adopting object '%s' by '%s'", 
                 iter->object->oid, 
                 master->oid);
            cfg_create_dep(master, iter->object, iter->object_wide);
            cfg_free_oid(iter->master);
            if (iter->prev == NULL)
                orphaned_objects = next;
            else
                iter->prev->next = next;
            
            if (next != NULL)
                next->prev = iter->prev;
            free(iter);
        }
    }
}


/**
 * Process a message with the user request: "register"
 * (that is, the request to add an object to the data base).
 *
 * @param msg   message pointer (it is assumed that message buffer
 *              length is long enough, for example for
 *              cfg_process_msg_get_oid
 *              it should be >= CFG_RECV_BUF_LEN)
 */
void 
cfg_process_msg_register(cfg_register_msg *msg)
{
    cfg_oid    *oid = cfg_convert_oid_str(msg->oid);
    cfg_object *father = &cfg_obj_root;
    cfg_object  *obj = NULL;
    
    int i = 0;
    
    if (oid == NULL)
    {
        msg->rc = TE_EINVAL;
        return;
    }
    
    if (oid->inst)
    {
        cfg_free_oid(oid);
        msg->rc = TE_EINVAL;
        return;
    }
        
    /* Look for the father first */
    while (TRUE)
    {
       for (; 
            father != NULL && 
            strcmp(father->subid, 
                   ((cfg_object_subid *)(oid->ids))[i].subid) != 0; 
            father = father->brother);
       
       if (++i == oid->len - 1 || father == NULL)
           break;
           
       father = father->son;
    }
    
    if (father == NULL)
    {
        cfg_free_oid(oid);
        msg->rc = TE_ENOENT;
        return;
    }
      
    /* Check for an obj with the same name */
    for (obj = father->son; 
         obj != NULL && \
         strcmp(obj->subid,
                ((cfg_object_subid *)(oid->ids))[i].subid) != 0; 
         obj = obj->brother);
    
    if (obj != NULL)
    {
        cfg_free_oid(oid);
        ERROR("Attempt to register: object already exists: %s\n",
              obj->oid);
        msg->rc = TE_RC(TE_CS, TE_EEXIST);
        return;
    }
    
    /* Now look for an empty slot in the objects array */
    for (i = 0; i < cfg_all_obj_size && cfg_all_obj[i] != NULL; i++);
    
    if (i == cfg_all_obj_size)
    {
        void *tmp = realloc(cfg_all_obj, 
                        sizeof(void *) * (cfg_all_obj_size + CFG_OBJ_NUM));

        if (tmp == NULL)
        {
            cfg_free_oid(oid);
            msg->rc = TE_ENOMEM;
            return;
        }
        memset(tmp + sizeof(void *) * cfg_all_obj_size, 0, 
               sizeof(void *) * CFG_OBJ_NUM);
        
        cfg_all_obj = (cfg_object **)tmp;
        cfg_all_obj_size += CFG_OBJ_NUM;
    }
    
    if ((cfg_all_obj[i] =
             (cfg_object *)calloc(sizeof(cfg_object), 1)) == NULL)
    {
        cfg_free_oid(oid);
        msg->rc = TE_ENOMEM;
        return;
    }
    if ((cfg_all_obj[i]->oid = strdup(msg->oid)) == NULL)
    {
        cfg_all_obj[i] = NULL;
        cfg_free_oid(oid);
        msg->rc = TE_ENOMEM;
        return;
    }
    
    if (msg->def_val > 0 && 
        (cfg_all_obj[i]->def_val = strdup(msg->oid + msg->def_val)) == NULL)
    {
        free(cfg_all_obj[i]->oid);
        cfg_all_obj[i] = NULL;
        cfg_free_oid(oid);
        msg->rc = TE_ENOMEM;
        return;
    }

    if (father->vol && !msg->vol)
    {
        WARN("Volatile attribute of %s it inherited from the father",
             msg->oid);
        msg->vol = TRUE;
    }
    
    cfg_all_obj[i]->handle = i;
    strcpy(cfg_all_obj[i]->subid, 
           ((cfg_object_subid *)(oid->ids))[oid->len - 1].subid);
    cfg_all_obj[i]->type = msg->val_type;
    cfg_all_obj[i]->access = msg->access;
    cfg_all_obj[i]->vol = msg->vol;
    cfg_all_obj[i]->father = father;
    cfg_all_obj[i]->son = NULL;
    cfg_all_obj[i]->brother = father->son;
    father->son = cfg_all_obj[i];

    cfg_all_obj[i]->ordinal_number = 0;
    cfg_all_obj[i]->depends_on = NULL;
    cfg_all_obj[i]->dependants = NULL;
    cfg_all_obj[i]->dep_next   = topological_order;
    cfg_all_obj[i]->dep_prev   = NULL;
    if (topological_order != NULL)
    {
        topological_order->dep_prev = cfg_all_obj[i];
    }
    topological_order          = cfg_all_obj[i];
    cfg_maybe_adopt_objects(cfg_all_obj[i], oid);
    if (!msg->no_parent_dep && 
        father != &cfg_obj_root && father != &cfg_obj_agent)
    {
        cfg_create_dep(father, cfg_all_obj[i], FALSE);
    }

    cfg_free_oid(oid);
    msg->handle = i;
    msg->len = sizeof(*msg);
}

/**
 * Process a message with the user request: "unregister"
 * (that is, the request to remove an object from the data base).
 *
 * @param msg   pointer to a message containing the request.
 */
void
cfg_process_msg_unregister(cfg_unregister_msg *msg)
{
    msg->rc = cfg_db_unregister_obj_by_id_str(msg->id, TE_LL_WARN);
    return;
}

/**
 * Remove an object from the list of dependants of the master obj.
 *
 * @param master   An object whose list of dependants is to be processed.
 * @param obj      An object to be removed from the list of dependants.
 *
 * return  The dependant object, if found and removed.
 *          NULL, otherwise.
 */
#define CFG_DEP_LIST_RM(head, obj)                                  \
    do {                                                            \
        curr = head; prev = curr;                                   \
        for (;curr != NULL && curr->depends != obj;                 \
             prev = curr, curr = curr->next);                       \
                                                                    \
        if (curr == NULL) /* didn't find the obj in the list */     \
            return NULL;                                            \
                                                                    \
        if (prev == curr) /* head */                                \
            head = curr->next;                                      \
        else                   /* remove the link */                \
            prev->next = curr->next;                                \
    } while (0)

static cfg_object *
forget_dependant(cfg_object *master, cfg_object *obj)
{
    cfg_dependency      *curr, *prev;

    CFG_DEP_LIST_RM(master->dependants, obj);

    return obj;
}

/**
 * Remove an object from the list of masters of the dependant obj.
 *
 * @param dependant   An object whose list of masters is to be processed.
 * @param obj         An object to be removed from the list of masters.
 *
 * return  The master object, if found and removed.
 *          NULL, otherwise.
 */
static cfg_object *
forget_master(cfg_object *dependant, cfg_object *obj)
{
    cfg_dependency      *curr, *prev;

    CFG_DEP_LIST_RM(dependant->depends_on, obj);

    return obj;
}
#undef CFG_DEP_LIST_RM

/**
 * Remove an object from the data base.
 *
 * @param id        id string of an object to be removed.
 * @param log_lvl   Log level for messages about forced actions
 *                  (instance delete, dependancy cut, etc. ) 
 *
 * @return  0 or TE_EINVAL, if error occurred.
 *
 * @author Igor Baryshev <Igor.Baryshev@oktetlabs.ru>
 */
te_errno
cfg_db_unregister_obj_by_id_str(char *id,
                                const unsigned int log_lvl)
{
    cfg_object          *obj;
    cfg_object          *brother;
    cfg_object          *father;
    cfg_object          *master;
    cfg_object          *objtmp;
    cfg_dependency      *depends_on;
    cfg_dependency      *dependants;

    cfg_oid             *idsplit = NULL;
    cfg_object_subid    *id_elmnt, *ids_end;
    int                 len;
    int                 i;
    char                pattern[CFG_OID_MAX];
    char                *iter;
    cfg_handle          *matches = NULL;
    int                 nof_matches = 0;
    te_errno            rc;


    obj = cfg_get_obj_by_obj_id_str(id);
    if (obj == NULL)
    {
        ERROR("no object with id string: %s\n", id);
        return TE_RC(TE_CS, TE_EINVAL);
    }

    if (obj->son != NULL)
    {
        ERROR("can't remove an object: %s, "
              "because a son is present: %s\n",
              id, obj->son->oid);
        return TE_RC(TE_CS, TE_EINVAL);
    }

    if (obj->father == NULL)
    {
        ERROR("can't remove a root object: %s\n", id);
        return TE_RC(TE_CS, TE_EINVAL);
    }


    /* Cut off all the dependants by brutal force
     * (normally, should not happen): */
    for (dependants = obj->dependants; dependants != NULL;
         dependants = dependants->next)
    {
        LOG_MSG(log_lvl, "To remove object: %s, "
                "will break the dependency (on it) of: %s\n",
                id, dependants->depends->oid);

        /* the obj must be there, so assert: */
        objtmp = forget_master(dependants->depends, obj);
        assert(objtmp != NULL);
    }


    /* Form a pattern '|A0:*|A1:*|... and check for instances: */
    idsplit = cfg_convert_oid_str(id);
    id_elmnt = (cfg_object_subid *)(idsplit->ids);
    ids_end = id_elmnt + idsplit->len;
    iter = pattern;
    /* skip empty ids[0], i.e. '/:*' as the first part of the pattern: */
    for (id_elmnt++; id_elmnt < ids_end; id_elmnt++)
    {
        len = strlen(id_elmnt->subid) + 4; /* '/'+':'+'*'+'\n' */
        if (iter + len > pattern + CFG_OID_MAX)
            return TE_RC(TE_CS, TE_EINVAL);

        snprintf(iter, len, "/%s:*", id_elmnt->subid);
        iter += (len -1 ); /* will overwrite '\n' */
    }
    cfg_free_oid(idsplit);

    rc = cfg_db_find_pattern(pattern, (unsigned int *)&nof_matches,
                             &matches);
    if (rc != 0)
    {
        ERROR("cfg_db_find_pattern() failed: %r; pattern: %s\n",
              rc, pattern);
        return rc;
    }

    for (i = 0; i < nof_matches; i++)
    {
        LOG_MSG(log_lvl, "To remove object: %s, "
                "will try to remove instance: %s\n",
                id,
                cfg_all_inst[CFG_INST_HANDLE_TO_INDEX(matches[i])]->oid);
        cfg_db_del(matches[i]);
    }
    free(matches);


    /* remove obj from the list of dependants of each master: */
    depends_on = obj->depends_on;
    for (;depends_on != NULL; depends_on = depends_on->next)
    {
        master = depends_on->depends;
        assert(master != NULL);

        /* the obj must be there, so assert: */
        objtmp = forget_dependant(master, obj);
        assert(objtmp != NULL);
    }


    /* Since we've made sure above that nobody depends on the obj,
     * we just remove the obj from the topological order: */
    if (obj->dep_prev == NULL)       /* head */
    {
        topological_order       = obj->dep_next;
        if (topological_order != NULL)
            topological_order->dep_prev = NULL;
    }
    else if (obj->dep_next == NULL)  /* tail */
    {
        obj->dep_prev->dep_next = NULL;
    }
    else
    {
        obj->dep_prev->dep_next = obj->dep_next;
        obj->dep_next->dep_prev = obj->dep_prev;
    }


    /* Remove the obj from the obj tree: */
    father = obj->father;
    if (father->son == obj)
        father->son = obj->brother;
    else
    {
        for (brother = father->son;
             brother->brother != obj;
             brother = brother->brother);

        assert(brother != NULL);
        brother->brother = obj->brother;
    }


    /* Delete from the array of objects */
    cfg_all_obj[obj->handle] = NULL;

    free(obj->oid);
    free(obj->def_val);
    free(obj);
    return 0;
} /* cfg_db_unregister_obj_by_id_str() */


void
cfg_process_msg_add_dependency(cfg_add_dependency_msg *msg)
{
    cfg_object *obj;
    cfg_handle  master_handle;
    int         rc;

    if (CFG_IS_INST(msg->handle))
    {
        msg->rc = TE_EINVAL;
        return;
    }
    
    if ((obj = CFG_GET_OBJ(msg->handle)) == NULL)
    {
        msg->rc = TE_ENOENT;
        return;
    }

    VERB("Adding %s dependency '%s' for '%s'", 
         msg->object_wide ? "object-wide" : "instance-wide",
         msg->oid, obj->oid);

    
    rc = cfg_db_find(msg->oid, &master_handle);
    if (rc != 0 && rc != TE_ENOENT)
    {
        ERROR("Cannot find the master object: %r", TE_RC(TE_CS, rc));
    }

    if (rc != 0)
    {
        cfg_orphan *orph;
        
        VERB("Creating an orphaned object %s <- %s", msg->oid, obj->oid);
        orph         = calloc(1, sizeof(*orph));
        orph->object = obj;
        orph->object_wide = msg->object_wide;
        orph->master = cfg_convert_oid_str(msg->oid);
        orph->next   = orphaned_objects;
        orph->prev   = NULL;
        if (orphaned_objects != NULL)
            orphaned_objects->prev = orph;
        orphaned_objects = orph;
    }
    else
    {
        cfg_create_dep(CFG_GET_OBJ(master_handle), obj, msg->object_wide);
    }
}

void 
cfg_process_msg_find(cfg_find_msg *msg)
{
    msg->rc = cfg_db_find(msg->oid, &(msg->handle));
}

void 
cfg_process_msg_get_descr(cfg_get_descr_msg *msg)
{
    cfg_object *obj;

    if (CFG_IS_INST(msg->handle) ||
        (obj = CFG_GET_OBJ(msg->handle)) == NULL)
    {
        msg->rc = TE_EINVAL;
        return;
    }
    
    if ((obj = CFG_GET_OBJ(msg->handle)) == NULL)
    {
        msg->rc = TE_ENOENT;
        return;
    }
    
    msg->descr.type = obj->type;
    msg->descr.access = obj->access;
    msg->len = sizeof(*msg);
}

void 
cfg_process_msg_get_oid(cfg_get_oid_msg *msg)
{
#define RET(_item) \
    do {                                                  \
        if (_item == NULL)                                \
        {                                                 \
            msg->rc = TE_ENOENT;                             \
            return;                                       \
        }                                                 \
        strcpy(msg->oid, _item->oid);                     \
        msg->len = sizeof(*msg) + strlen(msg->oid) + 1;   \
    } while (0)

    if (CFG_IS_INST(msg->handle))
    {
        cfg_instance *inst = CFG_GET_INST(msg->handle);
        RET(inst);
    }
    else
    {
        cfg_object *obj = CFG_GET_OBJ(msg->handle);
        RET(obj);
    }
#undef RET    
}

void 
cfg_process_msg_get_id(cfg_get_id_msg *msg)
{
#define RET(_item, _what) \
    do {                                                     \
        if (_item == NULL)                                   \
        {                                                    \
            msg->rc = TE_ENOENT;                                \
            return;                                          \
        }                                                    \
        strcpy(msg->id, _item->_what);                       \
        msg->len = sizeof(*msg) + strlen(msg->id) + 1;       \
    } while (0)

    if (CFG_IS_INST(msg->handle))
    {
        cfg_instance *inst = CFG_GET_INST(msg->handle);
        RET(inst, name);
    }
    else
    {
        cfg_object *obj = CFG_GET_OBJ(msg->handle);
        RET(obj, subid);
    }
#undef RET    
}

void 
cfg_process_msg_family(cfg_family_msg *msg)
{
#define RET(_item) \
    do {                                                                \
        if (_item == NULL)                                              \
        {                                                               \
            msg->rc = TE_ENOENT;                                        \
            return;                                                     \
        }                                                               \
        msg->handle = msg->who == CFG_FATHER ?                          \
                      (_item->father == NULL ? CFG_HANDLE_INVALID :     \
                       _item->father->handle) :                         \
                      msg->who == CFG_SON ?                             \
                      (_item->son == NULL ? CFG_HANDLE_INVALID :        \
                       _item->son->handle) :                            \
                      (_item->brother == NULL ? CFG_HANDLE_INVALID :    \
                       _item->brother->handle);                         \
    } while (0)

    if (CFG_IS_INST(msg->handle))
    {
        cfg_instance *inst = CFG_GET_INST(msg->handle);
        RET(inst);
    }
    else
    {
        cfg_object *obj = CFG_GET_OBJ(msg->handle);
        RET(obj);
    }
#undef RET    
}


/**
 * Process a user request to find all objects or object instances
 * matching a pattern.
 *
 * @param msg   message pointer
 *
 * @return message pointer of pointer to newly allocated message if the
 *         resulting message length > CFG_BUF_LEN
 */
cfg_pattern_msg *
cfg_process_msg_pattern(cfg_pattern_msg *msg)
{
    cfg_pattern_msg *tmp = msg;
    
    int num_max = (CFG_BUF_LEN - sizeof(*msg)) / sizeof(cfg_handle);
    int num = 0;

    cfg_handle *matches = NULL;
    int         nof_matches;
    te_errno    rc;
    

/* Number of slots in additional allocation chunks */
#define ALLOC_STEP  32 

/* Put a handle into the array of handles of the message */
#define PUT_HANDLE(_handle) \
    do {                                                            \
        if (num == num_max)                                         \
        {                                                           \
            num_max += ALLOC_STEP;                                  \
            if (tmp == msg)                                         \
            {                                                       \
                tmp = (cfg_pattern_msg *)malloc(sizeof(*msg) +      \
                    num_max * sizeof(cfg_handle));                  \
                if (tmp == NULL)                                    \
                {                                                   \
                    msg->rc = TE_RC(TE_CS, TE_ENOMEM);              \
                    return msg;                                     \
                }                                                   \
                memcpy(tmp, msg, sizeof(*msg) +                     \
                    (num_max - ALLOC_STEP) * sizeof(cfg_handle));   \
            }                                                       \
            else                                                    \
            {                                                       \
                void *tmp1 = realloc(tmp,                           \
                    sizeof(*msg) + num_max * sizeof(cfg_handle));   \
                if (tmp1 == NULL)                                   \
                {                                                   \
                    free(tmp);                                      \
                    msg->rc = TE_RC(TE_CS, TE_ENOMEM);              \
                    return msg;                                     \
                }                                                   \
                tmp = (cfg_pattern_msg *)tmp1;                      \
            }                                                       \
        }                                                           \
        tmp->handles[num] = _handle;                              \
    } while(0)

#define RETERR(_rc) \
    do {                        \
        if (tmp != msg)         \
            free(tmp);          \
        msg->rc = _rc;          \
        return msg;             \
    } while (0)

    
    rc = cfg_db_find_pattern(msg->pattern, (unsigned int *)&nof_matches,
                                            &matches);
    if (rc != 0)
        RETERR(rc);
    
    for (num = 0; num < nof_matches; num++)
        PUT_HANDLE(matches[num]);
        
    free(matches);
    
    VERB("Found %d OIDs by pattern", num);
    tmp->len = sizeof(*msg) + sizeof(cfg_handle) * num;
    return tmp;
    
#undef ALLOC_STEP
#undef PUT_HANDLE
#undef RETERR
}   /* cfg_process_msg_pattern() */



/**
 * Find all objects or object instances matching a pattern.
 *
 * @param pattern       string object identifier possibly containing '*'
 *                      (see Configurator documentation for details)
 * @param p_nmatches    OUT: number of found objects or object instances
 * @param p_matches     OUT: array of object/(object instance) handles;
 *                      memory for the array is allocated using malloc()
 *
 * @return  0 or
 *          TE_EINVAL if a pattern format is incorrect or
 *                    some argument is NULL.
 */
te_errno
cfg_db_find_pattern(const char *pattern,
                    unsigned int *p_nmatches,
                    cfg_handle **p_matches)
{
    cfg_handle  *matches = NULL;
    int         nof_matches = 0;
    cfg_oid     *idsplit = NULL;   
    int         i, k;
    
    te_bool     all = FALSE;
    te_bool     inst;

#define RETERR(_rc) \
    do {                            \
        if (idsplit)                \
            cfg_free_oid(idsplit);  \
        return TE_RC(TE_CS, _rc);   \
    } while (0)

      
    if (strcmp(pattern, "*") == 0)
    {
        all = TRUE;
        inst = FALSE;
        RING("pattern: %s, file: %s, line: %d\n",
             pattern, __FILE__, __LINE__);
    }
    else if (strcmp(pattern, "*:*") == 0)
    {
        all = TRUE;
        inst = TRUE;
    }
    else if ((idsplit = cfg_convert_oid_str(pattern)) == NULL)
        return TE_RC(TE_CS, TE_EINVAL);
    else
        inst = idsplit->inst;

    if (inst)
    {
        matches = calloc(cfg_all_inst_size, sizeof(cfg_handle));
        if (matches == NULL)
            RETERR(TE_ENOMEM);

        for (i = 0; i < cfg_all_inst_size; i++)
        {
            cfg_inst_subid *s1;
            cfg_inst_subid *s2;
            cfg_oid        *tmp_idsplit;
            
            if (cfg_all_inst[i] == NULL)
                continue;
                
            if (all)
            {
                matches[nof_matches++] = cfg_all_inst[i]->handle;
                continue;
            }

            tmp_idsplit = cfg_convert_oid_str(cfg_all_inst[i]->oid);
            if (tmp_idsplit == NULL)
                RETERR(TE_ENOMEM);

            if (tmp_idsplit->len != idsplit->len)
            {
                cfg_free_oid(tmp_idsplit);
                continue;
            }
            s1 = (cfg_inst_subid *)(idsplit->ids);
            s2 = (cfg_inst_subid *)(tmp_idsplit->ids);
            for (k = 0; k < idsplit->len; k++, s1++, s2++)
            {
                if ((s1->subid[0] != '*' && 
                     pattern_match(s1->subid, s2->subid) != 0) ||
                    (s1->name[0] != '*' &&
                     pattern_match(s1->name, s2->name) != 0))
                {
                    break;
                }
            }
            cfg_free_oid(tmp_idsplit);
            if (k == idsplit->len)
                matches[nof_matches++] = cfg_all_inst[i]->handle;
        }
    }
    else
    {
        matches = calloc(cfg_all_obj_size, sizeof(cfg_handle));
        if (matches == NULL)
            RETERR(TE_ENOMEM);

        for (i = 0; i < cfg_all_obj_size; i++)
        {
            cfg_object_subid *s1;
            cfg_object_subid *s2;
            cfg_oid          *tmp_idsplit;
            
            if (cfg_all_obj[i] == NULL)
                continue;
                
            if (all)
            {
                matches[nof_matches++] = cfg_all_obj[i]->handle;
                continue;
            }
            
            tmp_idsplit = cfg_convert_oid_str(cfg_all_obj[i]->oid);
            if (tmp_idsplit == NULL)
                RETERR(TE_ENOMEM);
                
            if (tmp_idsplit->len != idsplit->len)
            {
                cfg_free_oid(tmp_idsplit);
                continue;
            }
            s1 = (cfg_object_subid *)(idsplit->ids);
            s2 = (cfg_object_subid *)(tmp_idsplit->ids);
            for (k = 0; k < idsplit->len; k++, s1++, s2++)
            {
                if (s1->subid[0] != '*' &&
                    pattern_match(s1->subid, s2->subid) != 0) 
                {
                    break;
                }
            }
            cfg_free_oid(tmp_idsplit);
            if (k == idsplit->len)
                matches[nof_matches++] = cfg_all_obj[i]->handle;
        }
    }

    cfg_free_oid(idsplit);
    matches = realloc(matches, nof_matches * sizeof(cfg_handle));
    *p_matches = matches;
    *p_nmatches = nof_matches;    
    return 0;
#undef RETERR
}   /* cfg_db_find_pattern() */



/** 
 * Add instance children if they are read/write.
 *
 * @param inst   pointer on the instance.
 *
 * @return  status code.
 */ 
static int
cfg_db_add_children(cfg_instance *inst)
{
    cfg_object     *obj1 = inst->obj;
    char           *oid_s;
    int             i = 0;
    int             len;
    int             err;
    
    len = strlen(inst->oid);
    obj1 = obj1->son;
    while (obj1)
    {
        if (obj1->access != CFG_READ_WRITE)
        {
            obj1 = obj1->brother;
            continue;
        }

        /* make up instance oid string */
        oid_s = malloc(sizeof(char)*(len + strlen(obj1->subid) + 3));
        if (!oid_s)
            return TE_ENOMEM;
        sprintf(oid_s, "%s/%s:", inst->oid, obj1->subid);
        
        /* Adding instance */
        for (i = 0; i < cfg_all_inst_size && cfg_all_inst[i] != NULL; i++);
        
        if (i == cfg_all_inst_size)
        {
            void *tmp = realloc(cfg_all_inst, 
                sizeof(void *) * (cfg_all_inst_size + CFG_INST_NUM));

            if (tmp == NULL)
            {
                free(oid_s);
                return TE_ENOMEM;
            }
            
            memset(tmp + sizeof(void *) * cfg_all_inst_size, 0, 
                 sizeof(void *) * CFG_INST_NUM);
        
            cfg_all_inst = (cfg_instance **)tmp;
            cfg_all_inst_size += CFG_INST_NUM;
        }
    
        cfg_all_inst[i] = (cfg_instance *)calloc(sizeof(cfg_instance), 1);
        if (cfg_all_inst[i] == NULL)
        {
            free(oid_s);
            return TE_ENOMEM;
        }
        
        /* Setting all parameters*/
        cfg_all_inst[i]->oid = oid_s;
        if (obj1->type != CVT_NONE)
        {
            int err;
        
            if (obj1->def_val != NULL)
                err = cfg_types[obj1->type].str2val(obj1->def_val, 
                                               &(cfg_all_inst[i]->val));
            else
                err = 
                    cfg_types[obj1->type].def_val(&(cfg_all_inst[i]->val));
        
            if (err)
            {
                free(cfg_all_inst[i]->oid);
                free(cfg_all_inst[i]);
                cfg_all_inst[i] = NULL;
                return err;
            }
        }
 
        cfg_all_inst[i]->handle = i | (cfg_inst_seq_num++) << 16;
        cfg_all_inst[i]->name[0] = '\0';
        cfg_all_inst[i]->obj = obj1;
        cfg_all_inst[i]->father = inst;
        cfg_all_inst[i]->son = NULL;
        cfg_all_inst[i]->brother = inst->son;
        inst->son =  cfg_all_inst[i]; 
        
        /* Adding all its children*/
        if ( (err = cfg_db_add_children(cfg_all_inst[i])) != 0)
            return err;
        obj1 = obj1->brother;
    }
    return 0;
}

/** Get delay corresponding to instance OID */
static int
get_delay_by_oid(const char *oid)
{
    char ta[RCF_MAX_NAME];
    char oid_obj[CFG_OID_MAX];
    int  i;
    
    if (!cfg_get_ta_name(oid, ta))
         return 0;

    cfg_oid_inst2obj(oid, oid_obj);
    
    if (*oid_obj == 0)
        return 0;

    for (i = 0; i < cfg_all_inst_max; i++)
    {
        cfg_instance *tmp = cfg_all_inst[i];

        if (tmp != NULL && strcmp(tmp->obj->oid, "/conf_delay") == 0 &&
            strcmp(tmp->val.val_str, oid_obj) == 0)
        {
            for (tmp = tmp->son; tmp != NULL; tmp = tmp->brother)
                if (*tmp->name == 0 || strcmp(tmp->name, ta) == 0)
                    return tmp->val.val_int;
                
            return 0;
        }
    }
    return 0;
}

/** 
 * Update the current configuration delay after adding/deleting/changing
 * an instance.
 *
 * @param oid   instance OID
 */
void 
cfg_conf_delay_update(const char *oid)
{
    uint32_t delay = get_delay_by_oid(oid);
    
    if (delay > cfg_conf_delay)
        cfg_conf_delay = delay;
}

/**
 * Add instance to the database.
 *
 * @param oid_s         object instance identifier
 * @param handle        location for handle of the new instance
 * @param type          instance value type
 * @param val           value to be assigned to the instance
 *
 * @return status code (see te_errno.h)
 */
int 
cfg_db_add(const char *oid_s, cfg_handle *handle,
           cfg_val_type type, cfg_inst_val val)
{
    cfg_oid        *oid = cfg_convert_oid_str(oid_s);
    cfg_object     *obj;
    cfg_instance   *father = &cfg_inst_root;
    cfg_instance   *inst;
    cfg_instance   *prev;
    cfg_inst_subid *s;
    int             i = 0;
    
    if (oid == NULL)
    {
        ERROR("%s: OID is expected to be not NULL", __FUNCTION__);
        return TE_EINVAL;
    }
    
#define RET(_rc) \
    do {                        \
        cfg_free_oid(oid);      \
        return _rc;             \
    } while (0)
    
    if (!oid->inst)
        RET(TE_EINVAL);
    
    s = (cfg_inst_subid *)(oid->ids);
    
    /* Look for the father first */
    while (TRUE)
    {
       for (; 
            father != NULL && 
            (strcmp(father->obj->subid, s->subid) != 0 ||
             strcmp(father->name, s->name) != 0);
            father = father->brother);
            
        s++;
        
        if (++i == oid->len - 1 || father == NULL)
            break;
        
        father = father->son;
    }
    
    if (father == NULL)
        RET(TE_ENOENT);
        
    /* Find an object for the instance */
    for (obj = father->obj->son; 
         obj != NULL && strcmp(obj->subid, s->subid) != 0;
         obj = obj->brother);
         
    if (obj == NULL)
        RET(TE_ENOENT);
        
    if (obj->type != type && type != CVT_NONE)
    {
        ERROR("cfg_db_add: type (%d) expected - bad type (%d)"
              "of object (%s)", type, obj->type, obj->oid);
        ERROR("types: Integer (%d), string (%d), address (%d)", CVT_INTEGER,
              CVT_STRING, CVT_ADDRESS);
        RET(TE_EBADTYPE);
    }
    
    /* Try to find instance with the same name */
    for (inst = father->son, prev = NULL; 
         inst != NULL && strcmp(inst->oid, oid_s) < 0; 
         prev = inst, inst = inst->brother);

    if (inst != NULL && strcmp(inst->oid, oid_s) == 0)
        RET(TE_EEXIST);

    /* Now look for empty slot in the object instances array */
    for (i = 0; i < cfg_all_inst_size && cfg_all_inst[i] != NULL; i++);
    
    if (i == cfg_all_inst_size)
    {
        void *tmp = realloc(cfg_all_inst, 
            sizeof(void *) * (cfg_all_inst_size + CFG_INST_NUM));

        if (tmp == NULL)
            RET(TE_ENOMEM);
            
        memset(tmp + sizeof(void *) * cfg_all_inst_size, 0, 
               sizeof(void *) * CFG_INST_NUM);
        
        cfg_all_inst = (cfg_instance **)tmp;
        cfg_all_inst_size += CFG_INST_NUM;
    }
    
    inst = cfg_all_inst[i] = 
        (cfg_instance *)calloc(sizeof(cfg_instance), 1);
        
    if (cfg_all_inst[i] == NULL)
        RET(TE_ENOMEM);
        
    if ((inst->oid = strdup(oid_s)) == NULL)
    {
        free(inst);
        cfg_all_inst[i] = NULL;
        RET(TE_ENOMEM);
    }
    
    if (obj->type != CVT_NONE)
    {
        int err;
        
        if (type != CVT_NONE)
            err = cfg_types[obj->type].copy(val, &(inst->val));
        else if (obj->def_val != NULL)
            err = cfg_types[obj->type].str2val(obj->def_val, &(inst->val));
        else
            err = cfg_types[obj->type].def_val(&(inst->val));
        
        if (err)
        {
            free(inst->oid);
            free(inst);
            cfg_all_inst[i] = NULL;
            RET(err);
        }
    }
    
    /*
     * It is a new instance in local DB, so that mark it as not 
     * synchronized with Test Agent (not added to Test Agent).
     * It is the deal of uper layer to update this field to 'TRUE',
     * when this object is added to the Test Agent.
     */
    inst->added = FALSE;
    inst->handle = i | (cfg_inst_seq_num++) << 16;
    strcpy(inst->name, s->name);
    inst->obj = obj;
    inst->father = father;
    inst->son = NULL;
        
    if (prev)
    {
        inst->brother = prev->brother;
        prev->brother = inst;
    }
    else
    {
        inst->brother = father->son;
        father->son = inst;
    }
    
    *handle = inst->handle;
    if (cfg_all_inst_max < i)
        cfg_all_inst_max = i;
    
    cfg_free_oid(oid);
    if (strcmp_start(CFG_TA_PREFIX, inst->oid) != 0)
    {
        return cfg_db_add_children(inst);
    }

    return 0;

#undef RET
}

/**
 * Check, if the instance has read/create (grand-...)children.
 *
 * @param inst      instance to be verified
 *
 * @return TRUE, if the instance has read/create (grand-...)children.
 */
static te_bool
has_read_create_children(cfg_instance *inst)
{
    cfg_instance *tmp;
    
    for (tmp = inst->son; tmp != NULL; tmp = tmp->brother)
    {
        if (tmp->obj->access == CFG_READ_CREATE || 
            has_read_create_children(tmp))
        {
            return TRUE;
        }
    }
    
    return FALSE;
}

/**
 * Check that it's possible to delete instance from the database.
 *
 * @param handle        object instance handle
 *
 * @return status code (see te_errno.h)
 */
int 
cfg_db_del_check(cfg_handle handle)
{
    cfg_instance *inst = CFG_GET_INST(handle);
    
    if (inst == NULL)
        return TE_ENOENT;
        
    if (has_read_create_children(inst))
        return TE_EHASSON;
        
    if (inst->father == NULL)
        return TE_EISROOT;
        
    return 0;
}

static void
delete_son(cfg_instance *father, cfg_instance *son)
{
    cfg_instance *tmp;
    cfg_instance *next;
    cfg_instance *brother;

    for (tmp = son->son; tmp != NULL; tmp = next)
    {
        next = tmp->brother;
        delete_son(son, tmp);
    }
        
    if (father->son == son)
    {
        father->son = son->brother;
    }
    else
    {
        for (brother = father->son; 
             brother->brother != son;
             brother = brother->brother); 
             
        assert(brother != NULL);
        brother->brother = son->brother;
    }

    /* Delete from the array of object instances */
    cfg_all_inst[CFG_INST_HANDLE_TO_INDEX(son->handle)] = NULL;
    
    /* Free memory allocated for the instance */
    if (son->obj->type != CVT_NONE)
        cfg_types[son->obj->type].free(son->val);
        
    free(son->oid);
    free(son);
}

/**
 * Delete instance from the database.
 *
 * @param handle        object instance handle
 */
void 
cfg_db_del(cfg_handle handle)
{
    delete_son(CFG_GET_INST(handle)->father, CFG_GET_INST(handle));
}

/**
 * Change instance value.
 *
 * @param handle        object instance handle
 * @param val           new value of the object instance
 *
 * @return status code (see te_errno.h)
 */
int 
cfg_db_set(cfg_handle handle, cfg_inst_val val)
{
    cfg_instance *inst = CFG_GET_INST(handle);
    
    assert(inst);
    if (inst->obj->type != CVT_NONE)
    {
        cfg_inst_val val0;
        int err = cfg_types[inst->obj->type].copy(val, &val0);
        
        if (err)
            return err;
            
        cfg_types[inst->obj->type].free(inst->val);
        inst->val = val0;
    }
    
    return 0;
}

/**
 * Get instance value.
 *
 * @param handle        object instance handle
 * @param val           location for the value
 *
 * @return status code (see te_errno.h)
 */
int 
cfg_db_get(cfg_handle handle, cfg_inst_val *val)
{
    cfg_instance *inst = CFG_GET_INST(handle);
    
    if (inst == NULL)
        return TE_ENOENT;
    
    if (inst->obj->type != CVT_NONE)
        return cfg_types[inst->obj->type].copy(inst->val, val);
    else
        return 0;
}

/**
 * Find instance in the database.
 *
 * @param oid_s         object instance identifier
 * @param handle        location for found object or object instance
 *
 * @return status code (see te_errno.h)
 */
int 
cfg_db_find(const char *oid_s, cfg_handle *handle)
{
    cfg_oid *oid = NULL;
    int      i = 0;
    
    if ((oid = cfg_convert_oid_str(oid_s)) == NULL)
       return TE_EINVAL;

#define RET(_handle) \
    do {                   \
        cfg_free_oid(oid); \
        *handle = _handle; \
        return 0;          \
    } while (0)

#define RETERR(_rc) \
    do {                   \
        cfg_free_oid(oid); \
        return _rc;        \
    } while (0)

    if (oid->inst)
    {
        cfg_instance *tmp = &cfg_inst_root;
        cfg_instance *last_subinst = NULL;

        while (TRUE)
        {
            for (; 
                 tmp != NULL && 
                 !(strcmp(tmp->obj->subid, 
                          ((cfg_inst_subid *)(oid->ids))[i].subid) == 0 &&
                   strcmp(tmp->name, 
                          ((cfg_inst_subid *)(oid->ids))[i].name) == 0);
                 tmp = tmp->brother);

            if (++i == oid->len || tmp == NULL)
            {
                break;
            }

            last_subinst = tmp;

            tmp = tmp->son;
        }
        if (tmp == NULL)
        {
            /*
             * In case of local add operation we should take care of
             * its children.
             * Here caller might want to find one of its children to
             * perform local set on this leaf.
             */

            if (last_subinst->obj->access == CFG_READ_CREATE &&
                !last_subinst->added &&
                i == oid->len)
            {
                int         rc;
                const char *subobj_name = 
                    ((cfg_inst_subid *)(oid->ids))[oid->len - 1].subid;
                cfg_object *subobj_tmp = last_subinst->obj->son;

                /* Check that configuration DB accepts such object name */
                while (subobj_tmp != NULL)
                {
                    if (strcmp(subobj_tmp->subid, subobj_name) == 0)
                        break;

                    subobj_tmp = subobj_tmp->brother;
                }
                
                if (subobj_tmp == NULL)
                {
                    ERROR("Instance %s cannot be added into configurator "
                          "tree as child name '%s' has not been registered",
                          oid_s, subobj_name);
                    RETERR(TE_EINVAL);
                }

                /* 
                 * Add a new dummy instance to the local 
                 * configuration tree, and mark it as empty.
                 */
                rc = cfg_db_add(oid_s, handle, CVT_NONE,
                                (cfg_inst_val)0);

                if (rc != 0)
                    RETERR(rc);
                else
                    RET(*handle);
            }
            
            RETERR(TE_ENOENT);
        }
        else
            RET(tmp->handle);
    }
    else
    {
        cfg_object *tmp = &cfg_obj_root;
        while (TRUE)
        {
           for (; 
                tmp != NULL && 
                strcmp(tmp->subid, 
                       ((cfg_object_subid *)(oid->ids))[i].subid) != 0; 
                tmp = tmp->brother);
           
           if (++i == oid->len || tmp == NULL)
               break;
               
           tmp = tmp->son;
        }
        if (tmp == NULL)
            RETERR(TE_ENOENT);
        else
            RET(tmp->handle);
    }
    
#undef RET
#undef RETERR
}

/**
 * Find object for specified instance object identifier.
 *
 * @param oid_s   object instance identifier in string representation
 *
 * @return object structure pointer or NULL
 */
cfg_object *
cfg_get_object(const char *oid_s)
{
    cfg_oid        *oid = cfg_convert_oid_str(oid_s);
    cfg_object     *tmp = &cfg_obj_root;
    cfg_inst_subid *s;
    int             i = 0;
    
    if (oid == NULL)
        return NULL;
    
    if (!oid->inst)
    {
        cfg_free_oid(oid);
        return NULL;
    }
    
    s = (cfg_inst_subid *)(oid->ids);
    
    while (TRUE)
    {
       for (; 
            tmp != NULL && 
            strcmp(tmp->subid, s->subid) != 0; 
            tmp = tmp->brother);
            
       if (++i == oid->len || tmp == NULL)
           break;
           
       tmp = tmp->son;
       s++;
    }
    
    cfg_free_oid(oid);
    
    return tmp;
}

/**
 * Find the object for a specified object identifier.
 *
 * @param obj_id_str   object identifier in string representation
 *
 * @return pointer to the object structure or NULL.
 */
cfg_object *
cfg_get_obj_by_obj_id_str(const char *obj_id_str)
{
    cfg_oid             *idsplit = cfg_convert_oid_str(obj_id_str);
    cfg_object          *obj = &cfg_obj_root;
    cfg_object_subid    *ids;
    int                 i;

    if (idsplit == NULL)
        return NULL;

    if (idsplit->inst)
    {
        cfg_free_oid(idsplit);
        return NULL;
    }
    
    ids = (cfg_object_subid *)(idsplit->ids);

    for (i = 0;;)
    {
        while (obj != NULL && strcmp(obj->subid, ids->subid) != 0)
            obj = obj->brother;
        if (++i == idsplit->len || obj == NULL)
            break;

        obj = obj->son;
        ids++;
    }

    cfg_free_oid(idsplit);
    return obj;
}

/**
 * Find the instance for a specified instance identifier.
 *
 * @param ins_id_str   instance identifier in string representation
 *
 * @return pointer to the instance structure or NULL.
 */
cfg_instance *
cfg_get_ins_by_ins_id_str(const char *ins_id_str)
{
    cfg_oid             *idsplit = cfg_convert_oid_str(ins_id_str);
    cfg_instance        *ins = &cfg_inst_root;
    cfg_inst_subid      *ids;
    int                 i;

    if (idsplit == NULL)
        return NULL;

    if (!idsplit->inst)
    {
        cfg_free_oid(idsplit);
        return NULL;
    }
    
    ids = (cfg_inst_subid *)(idsplit->ids);

    for (i = 0;;)
    {
        while (ins != NULL && \
               (strcmp(ins->obj->subid, ids->subid) != 0 || \
                strcmp(ins->name,       ids->name ) != 0   )  )
            ins = ins->brother;
        if (++i == idsplit->len || ins == NULL)
            break;

        ins = ins->son;
        ids++;
    }

    cfg_free_oid(idsplit);
    return ins;
}

/*
 * Decide if the string matches to pattern
 *
 * @param pattern        pattern
 * @param string        string to match
 *
 * @return 
 *    0  - string matches to pattern
 *    -1 - string doesn't match to pattern
 */         
static int
pattern_match(char *pattern, char *str)
{
    char *star;
    char *segment;
    
    if (pattern == NULL || str == NULL)
        return -1;
        
    if ((star = strchr(pattern, '*')) == NULL)
    {
        return (strcmp(pattern, str) == 0) ? 0 : -1;
    }
    segment = star + 1;
    
    if (strlen(str) < strlen(segment) + (size_t)(star - pattern))
        return -1;
    
    if (strncmp(pattern, str, star - pattern) != 0)
        return -1;

    if ((strcmp(segment, str + strlen(str) - strlen(segment))) != 0)
        return -1;
    
    return 0;
}

/** Check if the object or its sons matching oid are volatile */
static te_bool
oid_match_volatile(cfg_object *obj, const cfg_inst_subid *subids, 
                   int subids_num)
{
    cfg_object *tmp;
    
    if (obj->vol)
        return TRUE;
        
    if (subids_num == 0)
        return FALSE;
        
    for (tmp = obj->son; tmp != NULL; tmp = tmp->brother)
    {
        if ((strcmp(subids[0].subid, "*") == 0 ||
             strcmp(subids[0].subid, tmp->subid) == 0) &&
            oid_match_volatile(tmp, subids + 1, subids_num - 1))
        {
            return TRUE;
        }
    }
        
    return FALSE;
}

/**
 * Check if the object identifier (possibly wildcard) matches some
 * volatile object on the Test Agent.
 *
 * @param oid_s         object identifier in string representation
 * @param ta            location for TA name pointer
 *
 * @return TRUE (match) or FALSE (does not match)
 */
te_bool 
cfg_oid_match_volatile(const char *oid_s, char **ta)
{
    cfg_oid *oid = cfg_convert_oid_str(oid_s);
    
    if (oid == NULL)
    {
        ERROR("Incorrect OID %s is passed to %s", oid_s, 
              __FUNCTION__);
        return FALSE;
    }
    
    cfg_inst_subid *subids = (cfg_inst_subid *)(oid->ids);
    
    te_bool match = FALSE;
    
    if (strcmp(oid_s, "*:*") == 0)
    {
        if (ta != NULL && (*ta = strdup("*")) == NULL)
            return FALSE;
            
        return TRUE;
    }
    
    if (oid == NULL)
        return FALSE;
        
    if (!oid->inst)
        return FALSE;
        
    match = (oid->len > 1) && (strcmp(subids[1].subid, "agent") == 0) &&
            oid_match_volatile(&cfg_obj_root, subids + 1, oid->len - 1) &&
            ((ta != NULL && (*ta = strdup(subids[1].name)) != NULL) || 
              ta == NULL);

    cfg_free_oid(oid);
    
    return match;
}

void
cfg_process_msg_tree_print(cfg_tree_print_msg *msg)
{
    char *flname = NULL;

    if (msg->flname_len != 0)
        flname = (msg->buf) + (msg->id_len);
    msg->rc = cfg_db_tree_print(flname, msg->log_lvl, msg->buf);
}

