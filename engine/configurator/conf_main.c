/** @file
 * @brief Configurator
 *
 * Configurator main loop
 *
 *
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
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

#include <libxml/xinclude.h>


/** Format for backup file name */
#define CONF_BACKUP_NAME         "%s/te_cfg_backup_%d_%llu.xml"


DEFINE_LGR_ENTITY("Configurator");

static char buf[CFG_BUF_LEN];
static char tmp_buf[1024];
static char *tmp_dir = NULL;
static char *filename;
/* If true, shutdown after message processing */
static te_bool cfg_shutdown = FALSE;
static int cfg_fatal_err = 0;
static struct ipc_server *server = NULL; /* IPC Server handle */


static void
print_tree(cfg_instance *inst, int indent)
{
    int i;
    static FILE *f;

    if (f == NULL && (f = fopen("instances", "w")) == NULL)
    {
        printf("Cannot open file instances\n");
        return;
    }

    for (i = 0; i < indent; i++)
        fprintf(f, " ");
    fprintf(f, "%s\n", inst->oid);
    for (inst = inst->son; inst != NULL; inst = inst->brother)
        print_tree(inst, indent + 2);

    if (indent == 0)
    {
        fclose(f);
        f = NULL;
    }
}

static void
print_otree(cfg_object *obj, int indent)
{
    int i;

    static FILE *f;

    if (f == NULL && (f = fopen("objects", "w")) == NULL)
    {
        printf("Cannot open file objects\n");
        return;
    }

    for (i = 0; i < indent; i++)
        fprintf(f, " ");
    fprintf(f, "%s\n", obj->oid);
    for (obj = obj->son; obj != NULL; obj = obj->brother)
        print_otree(obj, indent + 2);

    if (indent == 0)
    {
        fclose(f);
        f = NULL;
    }
}


/**
 * Parse and execute the configuration file.
 *
 * @param file  path name of the file
 *
 * @return status code (see te_errno.h)
 */
static int
parse_config(char *file)
{
    xmlDocPtr   doc;
    xmlNodePtr  root;
    int         rc;
    int         subst;

    if (file == NULL)
        return 0;

    if ((doc = xmlParseFile(file)) == NULL)
    {
#if HAVE_XMLERROR
        xmlError *err = xmlGetLastError();

        ERROR("Error occured during parsing configuration file:\n"
                "    %s:%d\n    %s", file, err->line, err->message);
#else
        ERROR("Error occured during parsing configuration file:\n"
                "    %s", file);
#endif
        xmlCleanupParser();
        return EINVAL;
    }

    VERB("Do XInclude sunstitutions in the document");
    subst = xmlXIncludeProcess(doc);
    if (subst < 0)
    {
#if HAVE_XMLERROR
        xmlError *err = xmlGetLastError();

        ERROR("XInclude processing failed: %s", err->message);
#else
        ERROR("XInclude processing failed");
#endif
        xmlCleanupParser();
        return EINVAL; /* FIXME */
    }
    VERB("XInclude made %d substitutions", subst);

    if ((root = xmlDocGetRootElement(doc)) == NULL)
    {
        VERB("Empty configuration file is provided");
        xmlFreeDoc(doc);
        xmlCleanupParser();
        return 0;
    }

    if (xmlStrcmp(root->name, (const xmlChar *)"backup") == 0)
        rc = cfg_backup_process_file(root);
    else if (xmlStrcmp(root->name, (const xmlChar *)"history") == 0)
        rc = cfg_dh_process_file(root);
    else
    {
        ERROR("Incorrect root node '%s' in the configuration file",
              root->name);
        rc = EINVAL;
    }

    xmlFreeDoc(doc);
    xmlCleanupParser();
    return rc;
}

/**
 * Process add user request.
 *
 * @param msg           message pointer
 * @param update_dh     if true, add the command to dynamic history
 */
static void
process_add(cfg_add_msg *msg, te_bool update_dh)
{
    cfg_handle    handle;
    cfg_instance *inst;
    cfg_object   *obj;
    char         *val_str = "";
    char         *oid = (char *)msg + msg->oid_offset;
    cfg_inst_val  val;
    char         *inst_name_str = (char *)msg + msg->oid_offset;

    msg->rc = cfg_types[msg->val_type].get_from_msg((cfg_msg *)msg, &val);

    if (msg->rc != 0)
        return;

    /* Get the value of the new instance to log it in case of error */
    if (cfg_types[msg->val_type].val2str(val, &val_str) != 0)
        assert(0);

    msg->rc = cfg_db_add(oid, &handle, msg->val_type, val);
    
    if (msg->rc != 0)
    {
        ERROR("Failed to add a new instance %s with value '%s' into "
              "configuration database", inst_name_str, val_str);
        cfg_types[msg->val_type].free(val);
        return;
    }

    inst = CFG_GET_INST(handle);
    obj = inst->obj;

    if (obj->access != CFG_READ_CREATE)
    {
        cfg_db_del(handle);
        cfg_types[obj->type].free(val);
        msg->rc = EACCES;
        ERROR("Failed to add a new instance %s with value %s because "
              "object %s is not read-create", inst_name_str, val_str,
              obj->oid);
        return;
    }

    if (update_dh && (msg->rc = cfg_dh_add_command((cfg_msg *)msg)) != 0)
    {
        cfg_db_del(handle);
        cfg_types[obj->type].free(val);
        ERROR("Failed to add a new instance %s with value %s in DH: "
              "error=%X", inst_name_str, val_str, msg->rc);
        return;
    }

    if (strncmp(oid, "/agent:", strlen("/agent:")) != 0) /* Success */
    {
        cfg_types[obj->type].free(val);
        msg->handle = handle;

        if (obj->type != CVT_NONE)
            free(val_str);

        return;
    }

    while (strcmp(inst->obj->subid, "agent") != 0)
        inst = inst->father;

    if (obj->type != CVT_NONE)
    {
        msg->rc = cfg_types[obj->type].val2str(val, &val_str);
        if (msg->rc != 0)
        {
            cfg_db_del(handle);
            if (update_dh)
                cfg_dh_delete_last_command();
            cfg_types[obj->type].free(val);
            return;
        }
    }

    msg->rc = rcf_ta_cfg_add(inst->name, 0, oid, val_str);

    if (msg->rc != 0)
    {
        cfg_db_del(handle);
        if (update_dh)
            cfg_dh_delete_last_command();
        ERROR("Failed to add a new instance %s with value %s into TA "
              "error=%X", inst->name, val_str, msg->rc);
    }

    cfg_types[obj->type].free(val);
    if (obj->type != CVT_NONE)
        free(val_str);

    cfg_ta_sync(oid, TRUE);
    msg->handle = handle;
}

/**
 * Process set user request.
 *
 * @param msg           message pointer
 * @param update_dh     if true, add the command to dynamic history
 */
static void
process_set(cfg_set_msg *msg, te_bool update_dh)
{
    cfg_handle    handle = msg->handle;
    cfg_instance *inst;
    cfg_object   *obj;
    char         *val_str = "";
    cfg_inst_val  val;
    cfg_inst_val  old_val;

    if ((inst = CFG_GET_INST(handle)) == NULL)
    {
        ERROR("Invalid handle in set request");
        msg->rc = ENOENT;
        return;
    }
    obj = inst->obj;

    if ((msg->rc = cfg_types[obj->type].
                   get_from_msg((cfg_msg *)msg, &val)) != 0)
        return;

    if (obj->access != CFG_READ_WRITE &&
        obj->access != CFG_READ_CREATE)
    {
        cfg_types[obj->type].free(val);
        msg->rc = EACCES;
        return;
    }

    if ((msg->rc = cfg_db_get(handle, &old_val)) != 0)
    {
        ERROR("Failed to get old value from DB: error=%d", msg->rc);
        cfg_types[obj->type].free(val);
        return;
    }

    if (update_dh && (msg->rc = cfg_dh_add_command((cfg_msg *)msg)) != 0)
    {
        ERROR("Failed to add command in DH: error=%d", msg->rc);
        cfg_types[obj->type].free(val);
        cfg_types[obj->type].free(old_val);
        return;
    }

    if ((msg->rc = cfg_db_set(handle, val)) != 0)
    {
        ERROR("Failed to set new value in DB: error=%d", msg->rc);
        cfg_types[obj->type].free(val);
        cfg_types[obj->type].free(old_val);
        if (update_dh)
            cfg_dh_delete_last_command();
        return;
    }

    if (strncmp(inst->oid, "/agent:", strlen("/agent:")) != 0) /* Success */
    {
        cfg_types[obj->type].free(val);
        cfg_types[obj->type].free(old_val);
        return;
    }

    /* Agents subtree modification */

    if (msg->local)
    {
        /* Local set operation */
        return;
    }

    while (strcmp(inst->obj->subid, "agent") != 0)
        inst = inst->father;

    if (obj->type != CVT_NONE)
    {
        msg->rc = cfg_types[obj->type].val2str(val, &val_str);
        if (msg->rc != 0)
        {
            if (update_dh)
                cfg_dh_delete_last_command();
            cfg_db_set(handle, old_val);
            cfg_types[obj->type].free(val);
            cfg_types[obj->type].free(old_val);
            return;
        }
    }

    msg->rc = rcf_ta_cfg_set(inst->name, 0,
                             CFG_GET_INST(handle)->oid, val_str);

    if (msg->rc != 0)
    {
        if (update_dh)
            cfg_dh_delete_last_command();
        cfg_db_set(handle, old_val);
    }

    cfg_types[obj->type].free(old_val);
    cfg_types[obj->type].free(val);
    if (obj->type != CVT_NONE)
        free(val_str);
}

/**
 * Process delete user request.
 *
 * @param msg           message pointer
 * @param update_dh     if true, add the command to dynamic history
 */
static void
process_del(cfg_del_msg *msg, te_bool update_dh)
{
    cfg_handle    handle = msg->handle;
    cfg_instance *inst;
    cfg_object   *obj;

    if ((inst = CFG_GET_INST(handle)) == NULL)
    {
        msg->rc = ENOENT;
        return;
    }
    obj = inst->obj;

    if (obj->access != CFG_READ_CREATE)
    {
        ERROR("Only READ-CREATE objects can be removed from "
              "the configuration tree. object: %s", obj->oid);
        msg->rc = EACCES;
        return;
    }

    if ((msg->rc = cfg_db_del_check(handle)) != 0)
    {
        ERROR("%s: cfg_db_del_check fails %X", __FUNCTION__, msg->rc);
        return;
    }

    if (update_dh && (msg->rc = cfg_dh_add_command((cfg_msg *)msg)) != 0)
    {
        ERROR("%s: Failed to add into DH errno %X", __FUNCTION__, msg->rc);
        return;
    }

    if (strncmp(inst->oid, "/agent:", strlen("/agent:")) != 0)
    {
        cfg_db_del(handle);
        return;
    }

    while (strcmp(inst->obj->subid, "agent") != 0)
        inst = inst->father;

    msg->rc = rcf_ta_cfg_del(inst->name, 0, CFG_GET_INST(handle)->oid);

    if (msg->rc != 0)
    {
        ERROR("%s: rcf_ta_cfg_del returns %X", __FUNCTION__, msg->rc);
        if (update_dh)
            cfg_dh_delete_last_command();
        return;
    }

    cfg_db_del(handle);
}

/**
 * Process get user request.
 *
 * @param msg           message pointer
 */
static void
process_get(cfg_get_msg *msg)
{
    cfg_handle    handle = msg->handle;
    cfg_instance *inst;
    cfg_object   *obj;

    if ((inst = CFG_GET_INST(handle)) == NULL)
    {
        msg->rc = ENOENT;
        return;
    }
    obj = inst->obj;

    if (msg->sync && (msg->rc = cfg_ta_sync(inst->oid, FALSE)) != 0)
        return;

    msg->val_type = obj->type;
    msg->len = sizeof(*msg);
    cfg_types[obj->type].put_to_msg(inst->val, (cfg_msg *)msg);
}

/* Returns time since Epoche in milliseconds */
static unsigned long long
get_time_ms()
{
    struct timeval tv = {0, 0};
    gettimeofday(&tv, NULL);
    return (unsigned long long)(time(NULL)) * 1000 +
           (unsigned long long)(tv.tv_usec / 1000);
}

/**
 * Process backup user request.
 *
 * @param msg           message pointer
 */
static void
process_backup(cfg_backup_msg *msg)
{
    switch (msg->op)
    {
        case CFG_BACKUP_CREATE:
        {
            sprintf(msg->filename, CONF_BACKUP_NAME,
                    tmp_dir, getpid(), get_time_ms());

            if ((msg->rc = cfg_backup_create_file(msg->filename)) != 0)
                return;

            if ((msg->rc = cfg_dh_attach_backup(msg->filename)) != 0)
                unlink(msg->filename);

            msg->len += strlen(msg->filename) + 1;

            break;
        }

        case CFG_BACKUP_RESTORE:
        {
            /* Try to restore using dynamic history */
            if ((msg->rc = cfg_dh_restore_backup(msg->filename)) == 0)
                return;

            return; /* FIXME */

/**
 * If error is returned by @e x, log error message and return from
 * function.
 *
 * @param x     - expression which returns status code
 */
#define CHECKERR(x) \
     do {                                                           \
         if ((msg->rc = (x)) != 0)                                  \
         {                                                          \
             ERROR("Restoring of backup failed - cannot continue"); \
             cfg_fatal_err = msg->rc;                               \
             return;                                                \
         }                                                          \
     } while (0)

            /* Re-initialize the Configurator with specified backup file */

            cfg_dh_destroy();
            CHECKERR(cfg_db_init());
            CHECKERR(cfg_ta_reboot_all());
            CHECKERR(parse_config(msg->filename));

#undef CHECKERR
            break;
        }

        case CFG_BACKUP_VERIFY:
        {
            char diff_file[RCF_MAX_PATH];
            
            if ((msg->rc = cfg_backup_create_file(filename)) != 0)
                return;
            sprintf(diff_file, "%s/te_cs.diff", getenv("TE_TMP"));
            sprintf(tmp_buf, "diff -u %s %s >%s 2>&1", msg->filename,
                             filename, diff_file);
            msg->rc = ((system(tmp_buf) == 0) ? 0 : ETEBACKUP);
            if (msg->rc == 0)
                cfg_dh_release_after(msg->filename);
            else
                INFO("Backup diff: %tf", diff_file);
            unlink(diff_file);            
            break;
        }

        case CFG_BACKUP_RELEASE:
        {
            msg->rc = cfg_dh_release_backup(msg->filename);
            break;
        }
    }
}

/**
 * Process reboot user request.
 *
 * @param msg           message pointer
 * @param update_dh     if true, add the command to dynamic history
 */
static void
process_reboot(cfg_reboot_msg *msg, te_bool update_dh)
{
    if (update_dh && (msg->rc = cfg_dh_add_command((cfg_msg *)msg)) != 0)
        return;

    msg->rc = rcf_ta_reboot(msg->ta_name, NULL, NULL);

    if (msg->rc == 0 && msg->restore)
    {
        if ((msg->rc = cfg_backup_restore_ta(msg->ta_name)) != 0)
        {
            ERROR("Restoring of the TA state after reboot failed - "
                  "cannot continue");
            cfg_fatal_err = msg->rc;
        };
    }
}

/**
 * Log message.
 *
 * @param msg       Message to be logged
 * @param before    Logging before or after processing
 */
static void
log_msg(cfg_msg *msg, te_bool before)
{
    uint16_t    level;
    const char *addon;
    char        buf[32];
    char       *s1;
    char       *s2;

    if (before)
    {
        level = TE_LL_VERB;
        addon = " ...";
    }
    else if (msg->rc == 0)
    {
        level = TE_LL_INFO;
        addon = " OK";
    }
    else
    {
        level = TE_LL_ERROR;
        addon = buf;
        snprintf(buf, sizeof(buf), " FAILED (errno=0x%x)", msg->rc);
    }
/**
 * Construct strings to be printed for handle identification.
 *
 * @param _type     - type of configurator message
 */
#define GET_STRS(_type) \
    do {                                                \
        cfg_handle handle = ((_type *)msg)->handle;     \
                                                        \
        if (CFG_IS_INST(handle))                        \
        {                                               \
            cfg_instance *_inst = CFG_GET_INST(handle); \
            if (_inst != NULL)                          \
            {                                           \
                s1 = "instance ";                       \
                s2 = _inst->oid;                        \
            }                                           \
            else                                        \
            {                                           \
                s1 = "unknown instance";                \
                s2 = "";                                \
            }                                           \
        }                                               \
        else                                            \
        {                                               \
            cfg_object *_obj = CFG_GET_OBJ(handle);     \
            if (_obj != NULL)                           \
            {                                           \
                s1 = "object ";                         \
                s2 = _obj->oid;                         \
            }                                           \
            else                                        \
            {                                           \
                s1 = "unknown object";                  \
                s2 = "";                                \
            }                                           \
        }                                               \
    } while(0)

    switch (msg->type)
    {
        case CFG_REGISTER:
        {
            cfg_register_msg *m = (cfg_register_msg *)msg;

            LGR_MESSAGE(level, TE_LGR_USER,
                        "Register object %s (%s, %s)%s",
                        m->oid,
                        m->descr.type == CVT_NONE ? "void" :
                        m->descr.type == CVT_STRING ? "string" :
                        m->descr.type == CVT_INTEGER ? "integer" :
                        m->descr.type == CVT_ADDRESS ? "address" :
                        "unknown type",
                        m->descr.access == CFG_READ_WRITE ? "read/write" :
                        m->descr.access == CFG_READ_ONLY ? "read/only" :
                        m->descr.access == CFG_READ_CREATE ? "read/create" :
                        "unknown access", addon);
            break;
        }

        case CFG_FIND:
            if (!before && TE_RC_GET_ERROR(msg->rc) == ENOENT)
                level = TE_LL_INFO;
            LGR_MESSAGE(level, TE_LGR_USER,
                        "Find OID %s%s", ((cfg_find_msg *)msg)->oid, addon);
            break;

        case CFG_GET_DESCR:
            GET_STRS(cfg_get_descr_msg);
            LGR_MESSAGE(level, TE_LGR_USER,
                        "Get descr for %s%s%s", s1, s2, addon);
            break;

        case CFG_GET_OID:
            GET_STRS(cfg_get_oid_msg);
            LGR_MESSAGE(level, TE_LGR_USER,
                        "Get OID for %s%s%s", s1, s2, addon);
            break;

        case CFG_GET_ID:
            GET_STRS(cfg_get_id_msg);
            LGR_MESSAGE(level, TE_LGR_USER,
                        "Get ID for %s%s%s", s1, s2, addon);
            break;

        case CFG_PATTERN:
            if (before || msg->rc != 0)
            {
                /*
                 * Print only when some errors occurred because after
                 * successful processing of the pattern it is overwritten by
                 * cfg_process_msg_pattern() function and we cannot get the
                 * value.
                 */
                LGR_MESSAGE(level, TE_LGR_USER,
                            "Pattern for OID %s%s",
                            ((cfg_pattern_msg *)msg)->pattern, addon);
            }
            break;

        case CFG_FAMILY:
            GET_STRS(cfg_family_msg);
            LGR_MESSAGE(level, TE_LGR_USER,
                        "Get family (get %s) for %s%s%s",
                        ((cfg_family_msg *)msg)->who == CFG_FATHER ?
                            "father" :
                        ((cfg_family_msg *)msg)->who == CFG_BROTHER ?
                            "brother" :
                        ((cfg_family_msg *)msg)->who == CFG_SON ?
                            "son" : "unknown member",
                        s1, s2, addon);
            break;

        case CFG_ADD:
        {
            cfg_add_msg *m = (cfg_add_msg *)msg;
            cfg_inst_val val;
            char        *val_str;
            te_bool      val_str_free = FALSE;

            if (m->val_type == CVT_NONE)
            {
                val_str = "(none)";
            }
            else if (cfg_types[m->val_type].get_from_msg(msg, &val) != 0)
            {
                val_str = NULL;
            }
            else
            {
                if (cfg_types[m->val_type].val2str(val, &val_str) != 0)
                    val_str = NULL;
                else
                    val_str_free = TRUE;
                cfg_types[m->val_type].free(val);
            }
            LGR_MESSAGE(level, TE_LGR_USER,
                        "Add instance %s value %s%s",
                        (char *)m + m->oid_offset,
                        val_str == NULL ? "(unknown)" :
                        (before) ? "(not processed yet)" : val_str, addon);

            if (val_str_free)
                free(val_str);
            break;
        }

        case CFG_DEL:
            GET_STRS(cfg_del_msg);
            LGR_MESSAGE(level, TE_LGR_USER,
                        "Delete %s%s%s", s1, s2, addon);
            break;

        case CFG_SET:
        {
            cfg_set_msg *m = (cfg_set_msg *)msg;
            cfg_inst_val val;
            char        *val_str = NULL;
            te_bool      val_str_free = FALSE;

            GET_STRS(cfg_set_msg);

            if (cfg_types[m->val_type].get_from_msg(msg, &val) != 0)
                val_str = NULL;
            else
            {
                if (cfg_types[m->val_type].val2str(val, &val_str) != 0)
                    val_str = NULL;
                else
                    val_str_free = TRUE;
                cfg_types[m->val_type].free(val);
            }
            LGR_MESSAGE(level, TE_LGR_USER,
                        "Set for %s%s value %s%s", s1, s2,
                        val_str == NULL ? "(unknown)" : val_str, addon);

            if (val_str_free)
                free(val_str);
            break;
        }

        case CFG_GET:
            GET_STRS(cfg_get_msg);
            LGR_MESSAGE(level, TE_LGR_USER,
                        "Get %s%s%s", s1, s2, addon);
            break;

        case CFG_SYNC:
            LGR_MESSAGE(level, TE_LGR_USER,
                        "Synchronize %s%s%s",
                        ((cfg_sync_msg *)msg)->oid,
                        ((cfg_sync_msg *)msg)->subtree ? " (subtree)" : "",
                        addon);
            break;

        case CFG_REBOOT:
            LGR_MESSAGE(level, TE_LGR_USER,
                        "Reboot Test Agent %s%s",
                        ((cfg_reboot_msg *)msg)->ta_name, addon);
            break;

        case CFG_BACKUP:
        {
            uint8_t op = ((cfg_backup_msg *)msg)->op;

            if (!before && (op == CFG_BACKUP_VERIFY) &&
                (TE_RC_GET_ERROR(msg->rc) == ETEBACKUP))
            {
                level = TE_LL_INFO;
            }
            LGR_MESSAGE(level, TE_LGR_USER,
                        "%s backup %s%s%s",
                        op == CFG_BACKUP_CREATE ? "Create" :
                        op == CFG_BACKUP_RESTORE ? "Restore" :
                        op == CFG_BACKUP_RELEASE ? "Release" :
                        op == CFG_BACKUP_VERIFY ? "Verify" : "unknown",
                        op == CFG_BACKUP_CREATE ? "" :
                            ((cfg_backup_msg *)msg)->filename,
                        op == CFG_BACKUP_CREATE ? "" : " ", addon);
            break;
        }

        case CFG_CONFIG:
            LGR_MESSAGE(level, TE_LGR_USER,
                        "Create configuration file %s (%s)%s",
                        ((cfg_config_msg *)msg)->filename,
                        ((cfg_config_msg *)msg)->history ?
                            "history" : "backup", addon);
            break;

        case CFG_SHUTDOWN:
            LGR_MESSAGE(level, TE_LGR_USER,
                        "Shutdown command%s", addon);
            break;

        default:
            ERROR("Unknown command");
    }
#undef GET_STRS
}

/**
 * Process message with user request.
 *
 * @param msg           location of message pointer (message may be updated
 *                      or re-allocated by the function)
 * @param update_dh     if true, add the command to dynamic history
 */
void
cfg_process_msg(cfg_msg **msg, te_bool update_dh)
{
    log_msg(*msg, TRUE);

    switch ((*msg)->type)
    {
        case CFG_REGISTER:
            if (update_dh)
            {
                if (((*msg)->rc = cfg_dh_add_command(*msg)) != 0)
                    break;
            }
            cfg_process_msg_register((cfg_register_msg *)*msg);
            if ((*msg)->rc != 0 && update_dh)
                cfg_dh_delete_last_command();
            break;

        case CFG_FIND:
            cfg_process_msg_find((cfg_find_msg *)*msg);
            break;

        case CFG_GET_DESCR:
            cfg_process_msg_get_descr((cfg_get_descr_msg *)*msg);
            break;

        case CFG_GET_OID:
            cfg_process_msg_get_oid((cfg_get_oid_msg *)*msg);
            break;

        case CFG_GET_ID:
            cfg_process_msg_get_id((cfg_get_id_msg *)*msg);
            break;

        case CFG_PATTERN:
            *msg = (cfg_msg *)cfg_process_msg_pattern(
                                  (cfg_pattern_msg *)*msg);
            break;

        case CFG_FAMILY:
            cfg_process_msg_family((cfg_family_msg *)*msg);
            break;

        case CFG_ADD:
            process_add((cfg_add_msg *)(*msg), update_dh);
            break;

        case CFG_DEL:
            process_del((cfg_del_msg *)(*msg), update_dh);
            break;

        case CFG_SET:
            process_set((cfg_set_msg *)(*msg), update_dh);
            break;

        case CFG_COMMIT:
            (*msg)->rc = cfg_tas_commit(((cfg_commit_msg *)(*msg))->oid);
            break;

        case CFG_GET:
            process_get((cfg_get_msg *)(*msg));
            break;

        case CFG_SYNC:
            (*msg)->rc = cfg_ta_sync(((cfg_sync_msg *)(*msg))->oid,
                                     ((cfg_sync_msg *)(*msg))->subtree);
            break;

        case CFG_REBOOT:
            process_reboot((cfg_reboot_msg *)(*msg), update_dh);
            break;

        case CFG_BACKUP:
            process_backup((cfg_backup_msg *)(*msg));
            break;

        case CFG_CONFIG:
            if (((cfg_config_msg *)(*msg))->history)
            {
                (*msg)->rc = cfg_dh_create_file(
                                 ((cfg_config_msg *)(*msg))->filename);
            }
            else
            {
                (*msg)->rc = cfg_backup_create_file(
                                 ((cfg_config_msg *)(*msg))->filename);
            }
            break;

        case CFG_SHUTDOWN:
            /* Remove commands initiated by configuration file */
            cfg_dh_restore_backup(NULL);
            cfg_shutdown = TRUE;
            break;

        default: /* Should not occur */
            ERROR("Unknown message is received");
            break;
    }
    
    (*msg)->rc = TE_RC(TE_CS, (*msg)->rc);

    log_msg(*msg, FALSE);
}


/* Wait for shutdown message after initialization failure */
static void
wait_shutdown()
{
    while (TRUE)
    {
        struct ipc_server_client *user = NULL;

        cfg_msg *msg = (cfg_msg *)buf;
        size_t   len = CFG_BUF_LEN;
        int      rc;

        if ((rc = ipc_receive_message(server, buf, &len, &user)) != 0)
        {
            ERROR("Failed receive user request: errno=%d", rc);
            continue;
        }

        if (msg->type != CFG_SHUTDOWN)
            msg->rc = ETENOCONF;

        rc = ipc_send_answer(server, user, (char *)msg, msg->len);
        if (rc != 0)
        {
            ERROR("Cannot send an answer to user: errno=%d", rc);
        }

        if (msg->type == CFG_SHUTDOWN)
            return;
    }
}


/**
 * Main loop of the Configurator: initialization and processing user
 * requests.
 *
 * @param argc      number of arguments
 * @param argv      arguments
 *
 * @retval 0        success
 * @retval other    failure
 */
int
main(int argc, char **argv)
{
    int rc = 1;
    const char *tree_opt = "--cs-print-trees";

    ipc_init();
    if ((server = ipc_register_server(CONFIGURATOR_SERVER)) == NULL)
        goto error;

    VERB("Starting...");

    if (argc < 2 || argc > 3)
    {
        ERROR("Wrong arguments");
        rc = EINVAL;
        goto error;
    }

    if ((tmp_dir = getenv("TE_TMP")) == NULL)
    {
        ERROR("Fatal error: TE_TMP is empty");
        rc = ENOENT;
        goto error;
    }

    if ((filename = (char *)malloc(strlen(tmp_dir) +
                                   strlen("/te_cfg_tmp.xml") + 1)) == NULL)
    {
        ERROR("No enough memory");
        rc = ENOMEM;
        goto error;
    }
    sprintf(filename, "%s/te_cfg_tmp.xml", tmp_dir);

    if ((rc = cfg_db_init()) != 0)
    {
        ERROR("Fatal error: cannot initialize database");
        goto error;
    }

    if ((rc = parse_config(argv[1])) != 0)
        goto error;
    
    if (argc > 2 && argv[2][0] != 0) 
    {
        if (strcmp(tree_opt, argv[2]) != 0)
        {
            ERROR("Invalid option provided: opt=%s", argv[2]);
            rc = EINVAL;
            goto error;
        }
        print_otree(&cfg_obj_root, 0);
        print_tree(&cfg_inst_root, 0);
    }

    while (TRUE)
    {
        struct ipc_server_client *user = NULL;

        cfg_msg *msg = (cfg_msg *)buf;
        size_t   len = CFG_BUF_LEN;

        if ((rc = ipc_receive_message(server, buf, &len, &user)) != 0)
        {
            ERROR("Failed receive user request: errno=0x%x", rc);
            continue;
        }

        msg->rc = 0;

        cfg_process_msg(&msg, TRUE);

        rc = ipc_send_answer(server, user, (char *)msg, msg->len);
        if (rc != 0)
        {
            ERROR("Cannot send an answer to user: errno=0x%x", rc);
        }

        if ((char *)msg != buf)
            free(msg);

        if (cfg_fatal_err != 0)
        {
            rc = cfg_fatal_err;
            goto error;
        }

        if (cfg_shutdown)
        {

            print_otree(&cfg_obj_root, 0);
            print_tree(&cfg_inst_root, 0);
            goto error;
        }
    }

error:
    VERB("Destroy history");
    cfg_dh_destroy();

    VERB("Destroy database");
    cfg_db_destroy();

    VERB("Free resources");
    free(cfg_ta_list);
    free(cfg_get_buf);

    if ((rc != 0) && (server != NULL))
        wait_shutdown();

    VERB("Closing server");
    if (server != NULL)
        ipc_close_server(server);

    free(filename);

    if (rc != 0)
        ERROR("Error exit");
    else
        VERB("Exit");

    log_client_close();
    rcf_api_cleanup();

    return rc;
}

