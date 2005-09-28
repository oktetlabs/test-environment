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
static cfg_object cfg_obj_conf_delay;
static cfg_object cfg_obj_conf_delay_ta;

/** Root of configuration objects */
cfg_object cfg_obj_root = 
    { 0, "/", { 0 }, CVT_NONE, CFG_READ_ONLY, NULL, FALSE, NULL,
      &cfg_obj_agent, NULL };
      
/** "/agent" object */
static cfg_object cfg_obj_agent = 
    { 1, "/agent", { 'a', 'g', 'e', 'n', 't', 0 },
      CVT_NONE, CFG_READ_ONLY, NULL, FALSE, &cfg_obj_root, NULL, 
      &cfg_obj_conf_delay };

/** "/conf_delay" object */
static cfg_object cfg_obj_conf_delay = 
    { 2, "/conf_delay", 
      { 'c', 'o', 'n', 'f', '_', 'd', 'e', 'l', 'a', 'y', 0 },
      CVT_STRING, CFG_READ_CREATE, NULL, TRUE, &cfg_obj_root, 
      &cfg_obj_conf_delay_ta, NULL };

/** "/conf_delay/ta" object */
static cfg_object cfg_obj_conf_delay_ta = 
    { 3, "/conf_delay/ta", { 't', 'a', 0 },
      CVT_INTEGER, CFG_READ_CREATE, NULL, TRUE, &cfg_obj_conf_delay, 
      NULL, NULL };

      
/** Pool with configuration objects */
cfg_object **cfg_all_obj = NULL;
/** Size of objects pool */
int cfg_all_obj_size;

/** Root of configuration instances */
cfg_instance cfg_inst_root = 
    { 0x10000, "/:", { 0 }, &cfg_obj_root, TRUE, 0, NULL, NULL, NULL, 
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

/** 
 * Update the current configuration delay after adding/deleting/changing
 * an instance.
 */
static inline void
update_conf_delay(cfg_instance *inst)
{
    if (inst->conf_delay > cfg_conf_delay)
        cfg_conf_delay = inst->conf_delay;
}

/* Locals */
static int pattern_match(char *pattern, char *str);


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
    cfg_all_obj[2] = &cfg_obj_conf_delay;
    cfg_all_obj[3] = &cfg_obj_conf_delay_ta;
    cfg_obj_root.son = &cfg_obj_agent;
    cfg_obj_agent.son = NULL;
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
    for (i = 2; i < cfg_all_obj_size; i++)
    {
        if (cfg_all_obj[i] != NULL)
        {
            free(cfg_all_obj[i]->oid);
            free(cfg_all_obj[i]->def_val);
            free(cfg_all_obj[i]);
        }
    }
    free(cfg_all_obj);
    cfg_all_obj = NULL;
}

/**
 * Process message with user request.
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
    
    /* Now look for empty slot in the objects array */
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
    cfg_free_oid(oid);
    msg->handle = i;
    msg->len = sizeof(*msg);
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
 * Process pattern user request.
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
    cfg_oid         *oid = NULL;
    
    int num_max = (CFG_BUF_LEN - sizeof(*msg)) / sizeof(cfg_handle);
    int num = 0;
    int i;
    int k;
    
    te_bool all = FALSE;
    te_bool inst;

/* Number of slots to be allocated additionally once */
#define ALLOC_STEP  32 

/* Put the handle to the handles array of the message */
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
                    msg->rc = TE_ENOMEM;                               \
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
                    msg->rc = TE_ENOMEM;                               \
                    return msg;                                     \
                }                                                   \
                tmp = (cfg_pattern_msg *)tmp1;                      \
            }                                                       \
        }                                                           \
        tmp->handles[num++] = _handle;                              \
    } while(0)

#define RETERR(_rc) \
    do {                        \
        if (tmp != msg)         \
            free(tmp);          \
        if (oid)                \
            cfg_free_oid(oid);  \
        msg->rc = _rc;          \
        return msg;             \
    } while (0)

    if (strcmp(msg->pattern, "*") == 0)
    {
        all = TRUE;
        inst = FALSE;
    }
    else if (strcmp(msg->pattern, "*:*") == 0)
    {
        all = TRUE;
        inst = TRUE;
    }
    else if ((oid = cfg_convert_oid_str(msg->pattern)) == NULL)
    {
        msg->rc = TE_EINVAL;
        return msg;
    }
    else
        inst = oid->inst;

    if (inst)
    {
        for (i = 0; i < cfg_all_inst_size; i++)
        {
            cfg_inst_subid *s1;
            cfg_inst_subid *s2;
            cfg_oid        *tmp_oid;
            
            if (cfg_all_inst[i] == NULL)
                continue;
                
            if (all)
            {
                PUT_HANDLE(cfg_all_inst[i]->handle);
                continue;
            }
            
            tmp_oid = cfg_convert_oid_str(cfg_all_inst[i]->oid);
            if (tmp_oid == NULL)
                RETERR(TE_ENOMEM);
                
            if (tmp_oid->len != oid->len)
            {
                cfg_free_oid(tmp_oid);
                continue;
            }
            s1 = (cfg_inst_subid *)(oid->ids);
            s2 = (cfg_inst_subid *)(tmp_oid->ids);
            for (k = 0; k < oid->len; k++, s1++, s2++)
            {
                if ((s1->subid[0] != '*' && 
                     pattern_match(s1->subid, s2->subid) != 0) ||
                    (s1->name[0] != '*' &&
                     pattern_match(s1->name, s2->name) != 0))
                {
                    break;
                }
            }
            cfg_free_oid(tmp_oid);
            if (k == oid->len)
                PUT_HANDLE(cfg_all_inst[i]->handle);
        }
    }
    else
    {
        for (i = 0; i < cfg_all_obj_size; i++)
        {
            cfg_object_subid *s1;
            cfg_object_subid *s2;
            cfg_oid        *tmp_oid;
            
            if (cfg_all_obj[i] == NULL)
                continue;
                
            if (all)
            {
                PUT_HANDLE(i);
                continue;
            }
            
            tmp_oid = cfg_convert_oid_str(cfg_all_obj[i]->oid);
            if (tmp_oid == NULL)
                RETERR(TE_ENOMEM);
                
            if (tmp_oid->len != oid->len)
            {
                cfg_free_oid(tmp_oid);
                continue;
            }
            s1 = (cfg_object_subid *)(oid->ids);
            s2 = (cfg_object_subid *)(tmp_oid->ids);
            for (k = 0; k < oid->len; k++, s1++, s2++)
            {
                if (s1->subid[0] != '*' &&
                    pattern_match(s1->subid, s2->subid) != 0) 
                {
                    break;
                }
            }
            cfg_free_oid(tmp_oid);
            if (k == oid->len)
                PUT_HANDLE(i);
        }
    }    
    
    VERB("Found %d OIDs by pattern", num);
    tmp->len = sizeof(*msg) + sizeof(cfg_handle) * num;
    cfg_free_oid(oid);
    return tmp;
    
#undef ALLOC_STEP
#undef PUT_HANDLE
#undef RETERR
}

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

/** Calculate configuration changes delay for new instance */
static void
calculate_delay(cfg_instance *inst)
{
    int i;
    
    for (i = 0; i < cfg_all_inst_max; i++)
    {
        cfg_instance *tmp = cfg_all_inst[i];
        
        if (tmp != NULL && strcmp(tmp->obj->oid, "/conf_delay") == 0 &&
            strcmp(tmp->val.val_str, inst->obj->oid) == 0)
        {
            char  ta[RCF_MAX_NAME] = { 0, };
            char *s = inst->oid + strlen("/agent:");
            int   n = strchr(s, '/') - s;
            
            if (n == 0)
                break;
            
            for (tmp = tmp->son; tmp != NULL; tmp = tmp->brother)
            {
                if (*tmp->name == 0 || strcmp(tmp->name, ta) == 0)
                {
                    inst->conf_delay = tmp->val.val_int;
                    return;
                }
            }
                
            break;
        }
    }
    
    inst->conf_delay = inst->father->conf_delay;
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
        RET(TE_EBADTYPE);
    
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
    
    cfg_all_inst[i] = (cfg_instance *)calloc(sizeof(cfg_instance), 1);
    if (cfg_all_inst[i] == NULL)
        RET(TE_ENOMEM);
        
    if ((cfg_all_inst[i]->oid = strdup(oid_s)) == NULL)
    {
        free(cfg_all_inst[i]);
        cfg_all_inst[i] = NULL;
        RET(TE_ENOMEM);
    }
    
    if (obj->type != CVT_NONE)
    {
        int err;
        
        if (type != CVT_NONE)
            err = cfg_types[obj->type].copy(val, &(cfg_all_inst[i]->val));
        else if (obj->def_val != NULL)
            err = cfg_types[obj->type].str2val(obj->def_val, 
                                               &(cfg_all_inst[i]->val));
        else
            err = cfg_types[obj->type].def_val(&(cfg_all_inst[i]->val));
        
        if (err)
        {
            free(cfg_all_inst[i]->oid);
            free(cfg_all_inst[i]);
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
    cfg_all_inst[i]->added = FALSE;

    cfg_all_inst[i]->handle = i | (cfg_inst_seq_num++) << 16;
    strcpy(cfg_all_inst[i]->name, s->name);
    cfg_all_inst[i]->obj = obj;
    cfg_all_inst[i]->father = father;
    cfg_all_inst[i]->son = NULL;
        
    if (prev)
    {
        cfg_all_inst[i]->brother = prev->brother;
        prev->brother = cfg_all_inst[i];
    }
    else
    {
        cfg_all_inst[i]->brother = father->son;
        father->son = cfg_all_inst[i];
    }
    
    *handle = cfg_all_inst[i]->handle;
    if (cfg_all_inst_max < i)
        cfg_all_inst_max = i;
    
    cfg_free_oid(oid);
    if (strcmp_start("/agent", cfg_all_inst[i]->oid) != 0)
    {
        return cfg_db_add_children(cfg_all_inst[i]);
    }
    else
    {
        calculate_delay(cfg_all_inst[i]);
        update_conf_delay(cfg_all_inst[i]);
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
        
    update_conf_delay(son);
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
    update_conf_delay(inst);
    
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

