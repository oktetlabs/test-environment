/** @file
 * @brief Configurator
 *
 * Support of dynamic history
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
#include "te_expand.h"
  
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
} cfg_dh_entry;

static cfg_dh_entry *first = NULL;
static cfg_dh_entry *last = NULL;


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

#define RETERR(_rc, _str...)    \
    do {                        \
        int _err = _rc;         \
                                \
        ERROR(_str);            \
        xmlFree(oid);           \
        xmlFree(val_s);         \
        xmlFree(attr);          \
        free(msg);              \
        return _err;            \
    } while (0)

/**
 * Process 'add' command.
 *
 * @param node      XML node with add command  children
 *
 * @return Status code.
 */
static int
cfg_dh_process_add(xmlNodePtr node)
{
    int          rc;
    size_t       len;
    cfg_add_msg *msg = NULL;
    cfg_inst_val val;
    cfg_object  *obj;
    xmlChar     *oid = NULL;
    xmlChar     *val_s = NULL;
    xmlChar     *attr = NULL;
    
    while (node != NULL &&
           xmlStrcmp(node->name , (const xmlChar *)"instance") == 0)
    {
        if ((oid = xmlGetProp_exp(node, (const xmlChar *)"oid")) == NULL)
            RETERR(EINVAL, "Incorrect add command format");

        if ((obj = cfg_get_object(oid)) == NULL)
            RETERR(EINVAL, "Cannot find object for instance %s", oid);
        
        len = sizeof(cfg_add_msg) + CFG_MAX_INST_VALUE + 
              strlen(oid) + 1;
        if ((msg = (cfg_add_msg *)calloc(1, len)) == NULL)
            RETERR(ENOMEM, "Cannot allocate memory");
        
        msg->type = CFG_ADD;
        msg->len = sizeof(cfg_add_msg);
        msg->val_type = obj->type;
        msg->rc = 0;
        
        val_s = xmlGetProp_exp(node, (const xmlChar *)"value");
        if (val_s != NULL && strlen(val_s) >= CFG_MAX_INST_VALUE)
            RETERR(ENOMEM, "Too long value");
            
        if (obj->type == CVT_NONE && val_s != NULL)
            RETERR(EINVAL, "Value is prohibited for %s", oid);
            
        if (val_s == NULL)
            msg->val_type = CVT_NONE; /* Default will be assigned */ 
            
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
        msg->len += strlen(oid) + 1;
        strcpy((char *)msg + msg->oid_offset, oid);
        cfg_process_msg((cfg_msg **)&msg, TRUE);
        if (msg->rc != 0)
            RETERR(msg->rc, "Failed(%d) to execute the add command "
                            "for instance %s", msg->rc, oid);

        free(msg);
        msg = NULL;
        free(oid);
        oid = NULL;
        
        node = xmlNodeNext(node);
    }
    if (node != NULL)
        RETERR(EINVAL, "Incorrect add command format");

    return 0;
}

/**
 * Process "history" configuration file - execute all commands
 * and add them to dynamic history. 
 * Note: this routine does not reboot Test Agents.
 *
 * @param node  <history> node pointer
 *
 * @return status code (errno.h)
 */
int 
cfg_dh_process_file(xmlNodePtr node)
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
        xmlChar   *attr = NULL;
        
        if (xmlStrcmp(cmd->name , (const xmlChar *)"include") == 0)
            continue;

        if (xmlStrcmp(cmd->name , (const xmlChar *)"history") == 0)
        {
            rc = cfg_dh_process_file(cmd);
            if (rc != 0)
            {
                ERROR("Processing of embedded history failed 0x%x", rc);
                return rc;
            }
            continue;
        }

        if (xmlStrcmp(cmd->name , (const xmlChar *)"reboot") == 0)
        {
            cfg_reboot_msg *msg = NULL;
            
            if ((attr = xmlGetProp(cmd, (const xmlChar *)"ta")) == NULL)
                RETERR(EINVAL, "Incorrect reboot command format");

            if ((msg = (cfg_reboot_msg *)calloc(1, sizeof(*msg) + 
                                                strlen(attr) + 1)) == NULL)
                RETERR(ENOMEM, "Cannot allocate memory");
            
            msg->type = CFG_REBOOT;
            msg->len = sizeof(*msg) + strlen(attr) + 1;
            msg->rc = 0;
            msg->restore = FALSE;
            strcpy(msg->ta_name, attr);
            
            cfg_process_msg((cfg_msg **)&msg, TRUE);
            if (msg->rc != 0)
                RETERR(msg->rc, "Failed to execute the reboot command");
                
            xmlFree(attr);
            free(msg);
            continue;
        }
        
        if (xmlStrcmp(cmd->name , (const xmlChar *)"register") != 0 &&
            xmlStrcmp(cmd->name , (const xmlChar *)"add") != 0 &&
            xmlStrcmp(cmd->name , (const xmlChar *)"set") != 0 &&
            xmlStrcmp(cmd->name , (const xmlChar *)"delete") != 0)
        {
            ERROR("Unknown command %s", cmd->name);
            return EINVAL;
        }
        
        if (xmlStrcmp(cmd->name , (const xmlChar *)"register") == 0)
        {
            cfg_register_msg *msg = NULL;

            while (tmp != NULL &&
                   xmlStrcmp(tmp->name , (const xmlChar *)"object") == 0)
            {
                if ((oid = xmlGetProp_exp(tmp, (xmlChar *)"oid")) == NULL)
                    RETERR(EINVAL, "Incorrect %s command format",
                           cmd->name);

                val_s = xmlGetProp(tmp, (const xmlChar *)"default");

                len = sizeof(cfg_register_msg) + strlen(oid) + 1 +
                      (val_s == NULL ? 0 : strlen(val_s) + 1);
                      
                if ((msg = (cfg_register_msg *)calloc(1, len)) == NULL)
                    RETERR(ENOMEM, "Cannot allocate memory");
                
                msg->type = CFG_REGISTER;
                msg->len = len;
                msg->rc = 0;
                msg->access = CFG_READ_CREATE;
                msg->val_type = CVT_NONE;

                strcpy(msg->oid, oid);
                if (val_s != NULL)
                {
                    msg->def_val = strlen(msg->oid) + 1;
                    strcpy(msg->oid + msg->def_val, val_s);
                }
                
                attr = xmlGetProp(tmp, (const xmlChar *)"type");
                if (attr != NULL)
                {
                    if (strcmp(attr, "integer") == 0)
                        msg->val_type = CVT_INTEGER;
                    else if (strcmp(attr, "address") == 0)
                        msg->val_type = CVT_ADDRESS;
                    else if (strcmp(attr, "string") == 0)
                        msg->val_type = CVT_STRING;
                    else if (strcmp(attr, "none") != 0)
                        RETERR(EINVAL, "Unsupported object type %s", attr);
                    xmlFree(attr);
                }
                
                if (val_s != NULL)
                {
                    cfg_inst_val val;
                    
                    if (cfg_types[msg->val_type].str2val(val_s, &val) != 0)
                        RETERR(EINVAL, "Incorrect default value %s", val_s);
                    
                    cfg_types[msg->val_type].free(val);
                }
                
                attr = xmlGetProp(tmp, (const xmlChar *)"access");
                if (attr != NULL)
                {
                    if (strcmp(attr, "read_write") == 0)
                        msg->access = CFG_READ_WRITE;
                    else if (strcmp(attr, "read_only") == 0)
                        msg->access = CFG_READ_ONLY;
                    else if (strcmp(attr, "read_create") != 0)
                        RETERR(EINVAL, 
                               "Wrong value %s of 'access' attribute",
                               attr);
                    xmlFree(attr);
                }

                cfg_process_msg((cfg_msg **)&msg, TRUE);
                if (msg->rc != 0)
                    RETERR(msg->rc, "Failed to execute register command "
                                    "for object %s", oid);

                free(val_s);
                val_s = NULL;
                free(msg);
                msg = NULL;
                free(oid);
                oid = NULL;

                tmp = xmlNodeNext(tmp);
            }
            if (tmp != NULL)
                RETERR(EINVAL, "Unexpected node '%s' in register command",
                       tmp->name);
        }
        else if (xmlStrcmp(cmd->name , (const xmlChar *)"add") == 0)
        {
            rc = cfg_dh_process_add(tmp);
            if (rc != 0)
            {
                ERROR("Failed to process add command");
                return rc;
            }
        } 
        else if (xmlStrcmp(cmd->name , (const xmlChar *)"set") == 0)
        {
            cfg_set_msg *msg = NULL;
            cfg_inst_val val;
            cfg_handle   handle;
            cfg_object  *obj;
            
            while (tmp != NULL &&
                   xmlStrcmp(tmp->name , (const xmlChar *)"instance") == 0)
            {
                if ((oid = xmlGetProp_exp(tmp, (xmlChar *)"oid")) == NULL)
                    RETERR(EINVAL, "Incorrect %s command format",
                           cmd->name);

                if ((rc = cfg_db_find(oid, &handle)) != 0)
                    RETERR(ENOENT, "Cannot find instance %s", oid);
                
                if (!CFG_IS_INST(handle))
                    RETERR(EINVAL,"OID %s is not instance", oid);
                
                len = sizeof(cfg_set_msg) + CFG_MAX_INST_VALUE;
                if ((msg = (cfg_set_msg *)calloc(1, len)) == NULL)
                    RETERR(ENOMEM, "Cannot allocate memory");
                
                msg->handle = handle;
                msg->type = CFG_SET;
                msg->len = sizeof(cfg_set_msg);
                msg->rc = 0;
                obj = CFG_GET_INST(handle)->obj;
                msg->val_type = obj->type;
                
                if (obj->type == CVT_NONE)
                    RETERR(EINVAL, "Cannot perform set for %s", oid);

                val_s = xmlGetProp_exp(tmp, (const xmlChar *)"value");
                if (val_s == NULL)
                    RETERR(EINVAL, "Value is required for %s", oid);
                
                if ((rc = cfg_types[obj->type].str2val(val_s, &val)) != 0)
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
                RETERR(EINVAL, "Incorrect set command format");
        } 
        else if (xmlStrcmp(cmd->name , (const xmlChar *)"delete") == 0)
        {
            cfg_del_msg *msg = NULL;
            cfg_handle   handle;
            
            while (tmp != NULL &&
                   xmlStrcmp(tmp->name , (const xmlChar *)"ta") == 0)
            {
                if ((oid = xmlGetProp_exp(tmp, (xmlChar *)"oid")) == NULL)
                    RETERR(EINVAL, "Incorrect %s command format",
                           cmd->name);

                if ((rc = cfg_db_find(oid, &handle)) != 0)
                    RETERR(rc, "Cannot find instance %s", oid);
                
                if (!CFG_IS_INST(handle))
                    RETERR(EINVAL, "OID %s is not instance", oid);
                
                if ((msg = (cfg_del_msg *)calloc(1, sizeof(*msg))) == NULL)
                    RETERR(ENOMEM, "Cannot allocate memory");
                
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
                RETERR(EINVAL, "Incorrect delete command format");
        } 
        else
        {
            assert(FALSE);
        }
        if (val_s != NULL)
            free(val_s);
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
    
    cfg_dh_optimize();
    
#define RETERR(_err) \
    do {                   \
        fclose(f);         \
        unlink(filename);  \
        return _err;       \
    } while (0)    

    if (f == NULL)
        return errno;
        
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
                        "access=\"%s\" type=\"%s\"",
                        msg->oid, 
                        msg->access == CFG_READ_CREATE ?
                            "read_create" :
                        msg->access == CFG_READ_WRITE ?
                            "read_write" : "read_only",
                        msg->val_type == CVT_NONE ?    "none" :
                        msg->val_type == CVT_INTEGER ? "integer" :
                        msg->val_type == CVT_ADDRESS ? "address" :
                                                       "string");
                if (msg->def_val)
                    fprintf(f, " default=\"%s\"", msg->oid + msg->def_val);
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
                        
                    fprintf(f, "value=\"%s\"", val_str);
                    free(val_str);
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
    
    if (last == NULL)
        return 0;
        
    if ((tmp = (cfg_backup *)malloc(sizeof(*tmp))) == NULL)
        return ENOMEM;
        
    if ((tmp->filename = strdup(filename)) == NULL)
    {
        free(tmp);
        return ENOMEM;
    }
    
    tmp->next = last->backup;
    last->backup = tmp;
    
    VERB("Attach backup %s to command %d", filename, last->seq);
        
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
 *
 * @return status code (see te_errno.h)
 * @retval ENOENT       there is not command in dynamic history to which
 *                      the specified backup is attached
 */
int 
cfg_dh_restore_backup(char *filename)
{
    cfg_dh_entry *limit = NULL;
    cfg_dh_entry *tmp;
    cfg_dh_entry *prev;
    
    int rc;
    int result = 0;
    
    cfg_dh_optimize();
    
    /* At first, look for an instance with the backup */
    if (filename != NULL)
    {
        for (limit = first; limit != NULL; limit = limit->next)
            if (limit->backup != NULL && has_backup(limit, filename))
                break;
            
        if (limit == NULL)
        {
            ERROR("Position of the backup in dynamic history is not found");
            return ENOENT;
        }
    }
    
    if (filename != NULL && limit != NULL)
        VERB("Restore backup %s up to command %d", filename, limit->seq);
        
    for (tmp = last; tmp != limit; tmp = prev)
    {
        prev = tmp->prev;
        switch (tmp->cmd->type)
        {
            case CFG_REGISTER:
                break;
                
            case CFG_ADD:
            {
                cfg_del_msg msg = { CFG_DEL, sizeof(msg), 0, 0};
                cfg_msg    *p_msg = (cfg_msg *)&msg;
               
                rc = cfg_db_find((char *)(tmp->cmd) + 
                                 ((cfg_add_msg *)(tmp->cmd))->oid_offset, 
                                 &(msg.handle));
               
                if (rc != 0)
                {
                    if (rc != ENOENT)
                    {
                        ERROR("%s(): cfg_db_find() failed: %X", 
                              __FUNCTION__, rc);
                        TE_RC_UPDATE(result, msg.rc);
                    }
                    break;
                }
               
                cfg_process_msg(&p_msg, FALSE);
               
                if (msg.rc != 0)
                {
                    ERROR("%s(): add failed: %X", __FUNCTION__, msg.rc);
                    TE_RC_UPDATE(result, msg.rc);
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
                    return ENOMEM;
                }
                    
                if ((rc = cfg_db_find(tmp->old_oid, &msg->handle)) != 0) 
                {
                    ERROR("cfg_db_find(%s) failed: %X", tmp->old_oid, rc);
                    free(msg);
                    return rc;
                }
                    
                msg->type = CFG_SET;
                msg->len = sizeof(*msg);
                msg->val_type = ((cfg_set_msg *)(tmp->cmd))->val_type;
                cfg_types[msg->val_type].put_to_msg(tmp->old_val, 
                                                    (cfg_msg *)msg);
                
                cfg_process_msg((cfg_msg **)&msg, FALSE);
               
                rc = msg->rc;
                free(msg);
                if (rc != 0)
                {
                    ERROR("%s(): set failed: %X", __FUNCTION__, rc);
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
                    return ENOMEM;
                }
                    
                msg->type = CFG_ADD;
                msg->len = sizeof(*msg);
                msg->val_type = tmp->type;
                cfg_types[tmp->type].put_to_msg(tmp->old_val, 
                                                (cfg_msg *)msg);
                msg->oid_offset = msg->len;
                msg->len += strlen(tmp->old_oid) + 1;
                strcpy((char *)msg + msg->oid_offset, tmp->old_oid);
               
                cfg_process_msg((cfg_msg **)&msg, FALSE);
               
                rc = msg->rc;
                free(msg);
                if (rc != 0)
                {
                    ERROR("%s(): delete failed: %X", __FUNCTION__, rc);
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

/**
 * Add a command to the history.
 *
 * @param msg   message with set, add or delete user request.
 *
 * @return status code
 * @retval 0        success
 * @retval ENOMEM   cannot allocate memory
 * @retval EINVAL   bad message or message, which should not be in history
 */
int 
cfg_dh_add_command(cfg_msg *msg)
{
    cfg_dh_entry *entry = (cfg_dh_entry *)calloc(sizeof(cfg_dh_entry), 1);
    
    if (entry == NULL)
    {
        ERROR("Memory allocation failure");
        return ENOMEM;
    }
        
    if ((entry->cmd = (cfg_msg *)calloc(msg->len, 1)) == NULL)
    {
        ERROR("Memory allocation failure");
        free(entry);
        return ENOMEM;
    }
    
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
                RETERR(ENOENT);
            }

            entry->type = inst->obj->type;
    
            if (inst->obj->type != CVT_NONE)
                if (cfg_types[inst->obj->type].copy(inst->val, 
                                                    &(entry->old_val)) != 0)
                    RETERR(ENOMEM);
                    
            if ((entry->old_oid = strdup(inst->oid)) == NULL)
                RETERR(ENOMEM);
                    
            break;
        }
            
        default:
            /* Should not occur */
            RETERR(EINVAL);
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

