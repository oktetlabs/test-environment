/** @file
 * @brief Configurator
 *
 * Configurator main loop
 *
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 * @author Elena Vengerova <Elena.Vengerova@oktetlabs.ru>
 *
 */

#include <popt.h>
#include "conf_defs.h"
#if WITH_CONF_YAML
#include "conf_yaml.h"
#endif /* WITH_CONF_YAML */
#include "conf_rcf.h"
#include "conf_ipc.h"

#include <libxml/xinclude.h>
#include "te_kvpair.h"
#include "te_alloc.h"
#include "te_str.h"
#include "te_string.h"
#include "te_vector.h"

#if HAVE_SIGNAL_H
#include <signal.h>
#endif

/** Format for backup file name */
#define CONF_BACKUP_NAME         "%s/te_cfg_backup_%d_%llu.xml"

/** Format for backup file name for subtree*/
#define CONF_SUBTREE_BACKUP_NAME "%s/te_cfg_subree_backup_%d_%llu.xml"

/** Name of the XSL filter file for generation subtree backup */
#define CONF_SUBTREE_BACKUP_FILTER_NAME "subtree_backup.xsl"

/** Name of the file to store a list of the subtrees */
#define CONF_FILTERS_FILE_NAME  "%s/te_cfg_filter_%d_%llu.xml"

static char  buf[CFG_BUF_LEN];
static char  tmp_buf[1024];
static char *tmp_dir = NULL;
static char *filename;

static struct ipc_server *server = NULL; /**< IPC Server handle */

#define MAX_CFG_FILES 16
/** Configuration file names */
static const char *cs_cfg_file[16] =  {NULL, };
static const char *cs_sniff_cfg_file = NULL;  /**< Configuration file name
                                                   for sniffer framework */

/** @name Configurator global options */
#define CS_PRINT_TREES  0x1     /**< Print objects and object instances
                                     trees after initialization */
#define CS_LOG_DIFF     0x2     /**< Log diff if backup verificatino
                                     failed */
#define CS_FOREGROUND   0x4     /**< Run Configurator in foreground */
#define CS_SHUTDOWN     0x8     /**< Shutdown after message processing */
/*@}*/

/** Configurator global flags */
static unsigned int cs_flags = 0;

static te_bool cs_inconsistency_state = FALSE;

static void process_backup(cfg_backup_msg *msg, te_bool release_dh);
static te_errno create_backup(char **bkp_filename);
static te_errno process_backup_op(const char *name, uint8_t op);
static te_errno release_backup(const char *name);
static te_errno restore_backup(const char *name);
static te_errno parse_config(const char *fname, te_kvpair_h *expand_vars);

/**
 Put environment variables in list for expansion in file
 *
 * @param expand_vars   List of key-value pairs for expansion in file
 *
 * @param return        Status code
 */
static te_errno
put_env_vars_in_list(te_kvpair_h *expand_vars)
{
    char  *key;
    char  *value;
    char  *env_var;
    char **env;
    int    rc;

    for (env = environ; *env != NULL; env++)
    {
        env_var = strdup(*env);
        if (env_var == NULL)
        {
            ERROR("%s(): strdup(%s) failed", __func__, *env);
            return TE_RC(TE_CS, TE_ENOMEM);
        }

        key = strtok(env_var, "=");
        value = strtok(NULL, "");

        if (value != NULL)
        {
            rc = te_kvpair_add(expand_vars, key, "%s", value);
            if (rc != 0)
                return rc;
        }

        free(env_var);
    }

    return 0;
}

/**
 * Parse list of key-value pairs from history message
 *
 * @param msg           History message
 * @param expand_vars   List of key-value pairs for expansion in file
 *
 * @param return        Zero if parsing successful, errno otherwise.
 */
static te_errno
parse_kvpair(cfg_process_history_msg *msg, te_kvpair_h *expand_vars)
{
    unsigned int len;
    const char *key;
    const char *value;
    te_errno rc;

    len = sizeof(cfg_config_msg) + strlen(msg->filename) + 1;

    while (len < msg->len)
    {
        key = (const char *)msg + len;
        len += strlen(key) + 1;
        value = (const char *)msg + len;
        len += strlen(value) + 1;

        rc = te_kvpair_add(expand_vars, key, "%s", value);
        if (rc != 0)
            return rc;

    }

    return 0;
}

static void
set_inconsistency_state(void)
{
    ERROR("Configurator is in inconsistent state");
    cs_inconsistency_state = TRUE;
}

/**
 * This function should be called before processing ADD, DEL and SET
 * requests. It tries to avoid two problems described in conf_ta.h file.
 *
 * @param cmd        Command name ("add", "del" or "set")
 * @param oid        OID used in operation
 * @param msg        Message that should be processed
 * @param update_dh  Wheter t update dynamic history?
 *                   (@todo How it should be used?)
 *
 * @return Zero on success, and errno on failure
 */
static int
cfg_avoid_local_cmd_problem(const char *cmd, const char *oid,
                            cfg_msg *msg, te_bool update_dh)
{
    te_bool msg_local = FALSE;

    UNUSED(update_dh);

    assert(msg->type == CFG_ADD || msg->type == CFG_SET ||
           msg->type == CFG_DEL);

    switch (msg->type)
    {
        case CFG_ADD:
            msg_local = ((cfg_add_msg *)msg)->local;
            break;

        case CFG_DEL:
            msg_local = ((cfg_del_msg *)msg)->local;
            break;

        case CFG_SET:
            msg_local = ((cfg_set_msg *)msg)->local;
            break;
    }

    if (local_cmd_seq)
    {
        if (!msg_local)
        {
            CFG_CHECK_NO_LOCAL_SEQ_RC(cmd, msg);
        }
        else
        {
            /* Update maximum allowed commit subtree value */
            if (strlen(max_commit_subtree) > strlen(oid) &&
                strncmp(max_commit_subtree, oid, strlen(oid)) == 0)
            {
                snprintf(max_commit_subtree,
                         sizeof(max_commit_subtree), "%s", oid);
            }
            else if (strlen(max_commit_subtree) <= strlen(oid) &&
                     strncmp(max_commit_subtree, oid,
                     strlen(max_commit_subtree)) == 0)
            {
                /* do nothing */
            }
            else
            {
                int i = 0;

                /* Find common part */
                while (max_commit_subtree[i] != '\0' && oid[i] != '\0' &&
                       max_commit_subtree[i] == oid[i])
                {
                    i++;
                }
                /* At least starting '/' should be the same */
                assert(i > 0);

                if (i > 1 && max_commit_subtree[i - 1] == '/')
                    max_commit_subtree[i - 1] = '\0';
                else
                    max_commit_subtree[i] = '\0';
            }

            VERB("Local %s operation on %s inst - max commit inst %s",
                 cmd, oid, max_commit_subtree);
        }
    }

    if (!local_cmd_seq && msg_local)
    {
        if ((msg->rc = create_backup(&local_cmd_bkp)) != 0)
            return msg->rc;
        local_cmd_seq = TRUE;

        snprintf(max_commit_subtree, sizeof(max_commit_subtree), "%s", oid);

        VERB("Local %s operation - Start local commands sequence", cmd);
    }

    return 0;
}

/**
 * This function should be called when an error occures during processing
 * ADD, DEL and SET requests.
 * In case of local commands processing state, it rolles back all the
 * configuration that was before the first local command (in sequence of
 * local commands) and clears all resources allocated under local commands.
 * In case of non-local command it deletes an instance specified by
 * @p handle parameter if it was local addition.
 *
 * @param type       Command type
 * @param handle     Handle of the instance to delete from Data Base
 *
 * @return Nothing
 */
static void
cfg_wipe_cmd_error(uint8_t type, cfg_handle handle)
{
    if (type != CFG_ADD && type != CFG_SET && type != CFG_DEL)
    {
        ERROR("Please support handling %d type in %s function",
              type, __FUNCTION__);
        assert(0);
        return;
    }

    if (local_cmd_seq)
    {
        int rc;

        /* Restore configuration before the first local SET/ADD/DEL */
        local_cmd_seq = FALSE;
        rc = cfg_dh_restore_backup(local_cmd_bkp, FALSE);
        WARN("Restore backup to configuration which was before "
             "the first local ADD/DEL/SET commands. Restored with code %r",
             rc);
    }
    else if (type == CFG_ADD)
    {
        if (handle != CFG_HANDLE_INVALID)
            cfg_db_del(handle);
    }
}

#if 0
static void
print_tree(cfg_instance *inst, int indent)
{
    static FILE *f = NULL;

    int     i;
    char   *str;

    if (f == NULL && (f = fopen("instances", "w")) == NULL)
    {
        ERROR("Cannot open file instances");
        return;
    }

    for (i = 0; i < indent; i++)
        fprintf(f, " ");
    if (inst->obj->type == CVT_NONE ||
        cfg_types[inst->obj->type].val2str(inst->val, &str) != 0)
    {
        str = NULL;
    }
    fprintf(f, "%s = %s\n", inst->oid, (str != NULL) ? str : "");
    free(str);
    for (inst = inst->son; inst != NULL; inst = inst->brother)
        print_tree(inst, indent + 2);

    if (indent == 0)
    {
        fclose(f);
        f = NULL;
        RING("Configuration model instances tree:\n%Tf", "instances");
    }
}

static void
print_otree(cfg_object *obj, int indent)
{
    int i;

    static FILE *f = NULL;

    if (f == NULL && (f = fopen("objects", "w")) == NULL)
    {
        ERROR("Cannot open file objects");
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
        RING("Configuration model objects tree:\n%Tf", "objects");
    }
}
#endif

/* See description in 'conf_defs.h' */
te_errno
parse_config_dh_sync(xmlNodePtr root_node, te_kvpair_h *expand_vars)
{
    te_errno rc = 0;
    char *backup = NULL;

    if ((rc = create_backup(&backup)) != 0)
    {
        ERROR("Failed to create a backup");
        return rc;
    }
    rc = cfg_dh_process_file(root_node, expand_vars, FALSE);
    if (rc == 0 && (rc = cfg_ta_sync("/:", TRUE)) != 0)
    {
        ERROR("Cannot synchronise database with Test Agents");
        cfg_dh_restore_backup(backup, FALSE);
    }
    if (rc == 0 && (rc = cfg_dh_process_file(root_node, expand_vars, TRUE)) != 0)
    {
        ERROR("Failed to modify database after synchronisation: %r", rc);
        cfg_dh_restore_backup(backup, FALSE);
    }
    if (rc == 0)
        cfg_dh_release_backup(backup);
    free(backup);

    return rc;
}

/**
 * Parse and execute the configuration file.
 *
 * @param file          path name of the file
 * @param expand_vars   List of key-value pairs for expansion in file,
 *                      @c NULL if environment variables are used for
 *                      substitutions
 * @param history       if TRUE, the configuration file must be a history,
 *                      otherwise, it must be a backup
 * @param subtrees      Vector of the subtrees to execute the configuration
 *                      file.
 *
 * @return status code (see te_errno.h)
 */
static int
parse_config_xml(const char *file, te_kvpair_h *expand_vars, te_bool history,
                 const te_vec *subtrees)
{
    xmlDocPtr   doc;
    xmlNodePtr  root;
    int         rc;
    int         subst;

    if (file == NULL)
        return 0;

    RING("Parsing %s", file);

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
        return TE_RC(TE_CS, TE_EINVAL);
    }

    VERB("Do XInclude substitutions in the document");
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
        return TE_RC(TE_CS, TE_EINVAL);
    }
    VERB("XInclude made %d substitutions", subst);

    if ((root = xmlDocGetRootElement(doc)) == NULL)
    {
        VERB("Empty configuration file is provided");
        xmlFreeDoc(doc);
        xmlCleanupParser();
        return 0;
    }

    rcf_log_cfg_changes(TRUE);
    if (xmlStrcmp(root->name, (const xmlChar *)"backup") == 0)
    {
        if (history)
        {
            ERROR("File '%s' is a backup, not history as expected", file);
            rc = TE_RC(TE_CS, TE_EINVAL);
        }
        else
        {
            rc = cfg_backup_process_file(root, TRUE, subtrees);
        }
    }
    else if (xmlStrcmp(root->name, (const xmlChar *)"history") == 0)
    {
        if (!history)
        {
            ERROR("File '%s' is a history, not backup as expected", file);
            rc = TE_RC(TE_CS, TE_EINVAL);
        }
        else
        {
            rc = parse_config_dh_sync(root, expand_vars);
        }
    }
    else
    {
        ERROR("Incorrect root node '%s' in the configuration file",
              root->name);
        rc = TE_RC(TE_CS, TE_EINVAL);
    }
    rcf_log_cfg_changes(FALSE);

    xmlFreeDoc(doc);
    xmlCleanupParser();
    return rc;
}

/**
 * Decides whether "/agent/volatile" subtree should be synchronized or not
 * and synchronize it if necessary.
 *
 * @param inst_name  Instance name
 *
 * @return Status code
 */
static int
cfg_sync_agt_volatile(cfg_msg *msg, const char *inst_name)
{
    char *oid;

    if (!cfg_oid_match_volatile(inst_name, &oid))
        return 0;

    CFG_CHECK_NO_LOCAL_SEQ_RC("sync_agt_volatile", msg);

    msg->rc = cfg_ta_sync(oid, TRUE);
    free(oid);
    return msg->rc;
}

#if 0
/**
 * Find out the subtree to be synchronized.
 *
 * @param ta    Test Agent name
 * @param val   value for /agent/rsrc instance
 *
 * @return OID of the subtree to be synchronized after addition/deletion
 *         of the resource
 */
static char *
rsrc_oid_to_sync(const char *ta, const char *val)
{
    static char buf[CFG_OID_MAX];
    char       *tmp = buf;
    cfg_oid    *oid = cfg_convert_oid_str(val);
    int         i;

    if (oid == NULL)
        return "/:";

    if (oid->inst)
    {
        cfg_free_oid(oid);
        strcpy(buf, val);
        return buf;
    }

    *buf = 0;

    /*
     * It is object identifier. Assume that all instance names
     * except TA name are empty.
     */
    for (i = 1; i < oid->len; i++)
    {
        tmp += sprintf(tmp, "/%s:%s",
                       ((cfg_object_subid *)(oid->ids))[i].subid,
                       i == 1 ?  ta : "");
    }

    cfg_free_oid(oid);

    return buf;
}
#endif

/*
 * Add instance with given OID and value from source instance
 *
 * @param dst_oid       Destination instance OID
 * @param src_inst      Source instance
 * @param dst_handle    Handle of the new instance
 *
 * @return Status code
 */
te_errno
add_inst_with_src_val(const char *dst_oid, const cfg_instance *src_inst,
                    cfg_handle *dst_handle)
{
    cfg_add_msg  *msg = NULL;
    size_t        len;
    te_errno      rc;

    len = sizeof(cfg_add_msg) + CFG_MAX_INST_VALUE +
          strlen(dst_oid) + 1;
    if ((msg = TE_ALLOC(len)) == NULL)
        return TE_ENOMEM;

    msg->type = CFG_ADD;
    msg->len = sizeof(cfg_add_msg);
    msg->val_type = src_inst->obj->type;

    cfg_types[src_inst->obj->type].put_to_msg(src_inst->val, (cfg_msg *)msg);

    msg->oid_offset = msg->len;
    msg->len += strlen(dst_oid) + 1;
    memcpy((char *)msg + msg->oid_offset, dst_oid, strlen(dst_oid) + 1);

    cfg_process_msg((cfg_msg **)&msg, TRUE);

    rc = msg->rc;
    if (rc == 0)
        *dst_handle = msg->handle;

    free(msg);

    return rc;
}

/*
 * Copy value from source instance to destination instance
 *
 * @param dst_handle         Destination instance handle
 * @param src_inst           Source instance
 *
 * @return Status code
 */
te_errno
copy_value(cfg_handle dst_handle, const cfg_instance *src_inst)
{
    cfg_set_msg  *msg = NULL;
    cfg_instance *dst_inst;
    size_t        len;
    te_errno      rc = 0;

    dst_inst = CFG_GET_INST(dst_handle);
    if (dst_inst == NULL)
    {
        ERROR("Failed to get destination instance");
        return TE_ENOENT;
    }

    if ((src_inst->obj->type != CVT_NONE) &&
        (dst_inst->obj->access == CFG_READ_CREATE ||
         dst_inst->obj->access == CFG_READ_WRITE))
    {
        len = sizeof(cfg_msg) + CFG_MAX_INST_VALUE;
        if ((msg = TE_ALLOC(len)) == NULL)
            return TE_ENOMEM;

        msg->type = CFG_SET;
        msg->len = sizeof(cfg_set_msg);
        msg->handle = dst_handle;
        msg->val_type = dst_inst->obj->type;

        cfg_types[dst_inst->obj->type].put_to_msg(src_inst->val, (cfg_msg *)msg);

        cfg_process_msg((cfg_msg **)&msg, TRUE);

        rc = msg->rc;
        free(msg);
    }

    return rc;
}
/**
 * Copy instance from source tree to destination tree
 *
 * @param dst_oid               Destination instance OID
 * @param src_inst              Source tree instance
 * @param dst_child_handle      Handle of the added instance
 *
 * @return  Status code
 */
static te_errno
copy_subtree_instance(const char *dst_oid, const cfg_instance *src_inst,
                      cfg_handle *dst_child_handle)
{
    cfg_handle    dst_handle;
    int           rc = 0;
    te_bool       exists = FALSE;

    rc = cfg_db_find(dst_oid, &dst_handle);
    if (rc == 0)
    {
        if (CFG_IS_INST(dst_handle))
        {
            exists = TRUE;
        }
        else
        {
            ERROR("Destination OID %s is not an instance", dst_oid);
            return TE_EINVAL;
        }
    }

    if (!exists)
    {
        rc = add_inst_with_src_val(dst_oid, src_inst, &dst_handle);
    }
    else
    {
        *dst_child_handle = dst_handle;

        rc = copy_value(dst_handle, src_inst);
    }

    return rc;
}

/**
 * Copy object from source tree to destination tree
 *
 * @param dst_oid               Destination object OID
 * @param src_inst              Source tree object
 * @param dst_child_handle      Handle of the added object
 *
 * @return  Status code
 */
static te_errno
copy_subtree_object(const char *dst_oid, const cfg_object *src_obj,
                    cfg_handle *dst_child_handle)
{
    cfg_register_msg  *msg = NULL;
    int                rc = 0;
    size_t             msg_len;
    size_t             dst_oid_len;
    size_t             def_val_len;

    dst_oid_len = strlen(dst_oid) + 1;
    def_val_len = (src_obj->def_val == NULL) ? 0 : strlen(src_obj->def_val) + 1;

    msg_len = sizeof(cfg_register_msg) + dst_oid_len + def_val_len;
    if ((msg = TE_ALLOC(msg_len)) == NULL)
        return TE_ENOMEM;

    msg->type = CFG_REGISTER;
    msg->val_type = src_obj->type;
    msg->access = src_obj->access;

    memcpy(msg->oid, dst_oid, dst_oid_len);

    if (src_obj->def_val != NULL)
    {
        msg->def_val = dst_oid_len;
        memcpy(msg->oid + dst_oid_len, src_obj->def_val, def_val_len);
    }
    else
    {
        msg->def_val = 0;
    }
    msg->len = msg_len;

    cfg_process_msg((cfg_msg **)&msg, TRUE);

    rc = msg->rc;
    if (rc == 0)
        *dst_child_handle = msg->handle;

    free(msg);
    return rc;
}

/**
 * Copy recursively instance source subtree to destination subtree
 *
 * @param top_dst_oid       Destination subtree OID
 * @param top_src_handle    Source subtree handle
 *
 * @return Status code
 */
static te_errno
copy_inst_subtree_recursively(const char *top_dst_oid,
                              cfg_handle top_src_handle)
{
    int rc = 0;
    cfg_handle dst_handle;
    const cfg_instance *src_inst;
    cfg_instance *inst;
    char *child_oid;
    size_t dst_oid_len, src_oid_len, src_child_oid_len;

    if (!CFG_IS_INST(top_src_handle))
    {
        ERROR("Source is not an instance");
        return TE_EINVAL;
    }

    src_inst = CFG_GET_INST(top_src_handle);
    if (src_inst == NULL)
    {
        ERROR("Failed to get source instance");
        return TE_ENOENT;
    }

    rc = copy_subtree_instance(top_dst_oid, src_inst, &dst_handle);
    if (rc != 0)
        return rc;

    dst_oid_len = strlen(top_dst_oid);
    src_oid_len = strlen(src_inst->oid);
    for (inst = src_inst->son; inst != NULL; inst = inst->brother)
    {
        src_child_oid_len = strlen(inst->oid);

        child_oid = TE_ALLOC(dst_oid_len + src_child_oid_len - src_oid_len + 1);
        if (child_oid == NULL)
            return TE_ENOMEM;
        memcpy(child_oid, top_dst_oid, dst_oid_len);
        memcpy(child_oid + dst_oid_len, inst->oid + src_oid_len,
               src_child_oid_len - src_oid_len + 1);

        rc = copy_inst_subtree_recursively(child_oid, inst->handle);
        free(child_oid);

        if (rc != 0)
            return rc;
    }
    return rc;
}

/**
 * Copy recursively object source subtree to destination subtree
 *
 * @param top_dst_oid       Destination subtree OID
 * @param top_src_handle    Source subtree handle
 *
 * @return Status code
 */
static te_errno
copy_obj_subtree_recursively(const char *top_dst_oid, cfg_handle top_src_handle)
{
    int               rc = 0;
    cfg_handle        dst_handle;
    const cfg_object *src_obj;
    cfg_object       *obj;
    char             *child_oid;
    size_t            dst_oid_len;
    size_t            src_oid_len;
    size_t            src_child_oid_len;

    src_obj = CFG_GET_OBJ(top_src_handle);
    if (src_obj == NULL)
    {
        ERROR("Failed to get source object");
        return TE_ENOENT;
    }

    rc = copy_subtree_object(top_dst_oid, src_obj, &dst_handle);
    if (rc != 0)
        return rc;

    dst_oid_len = strlen(top_dst_oid);
    src_oid_len = strlen(src_obj->oid);
    for (obj = src_obj->son; obj != NULL; obj = obj->brother)
    {
        src_child_oid_len = strlen(obj->oid);

        child_oid = TE_ALLOC(dst_oid_len + src_child_oid_len -
                             src_oid_len + 1);
        if (child_oid == NULL)
            return TE_ENOMEM;
        memcpy(child_oid, top_dst_oid, dst_oid_len);
        memcpy(child_oid + dst_oid_len, obj->oid + src_oid_len,
               src_child_oid_len - src_oid_len + 1);

        rc = copy_obj_subtree_recursively(child_oid, obj->handle);
        free(child_oid);

        if (rc != 0)
            return rc;
    }
    return rc;
}

/**
 * Process 'copy' user request.
 *
 * @param msg   message
 */
static void
process_copy(cfg_copy_msg *msg)
{
    char *backup_name = NULL;
    te_errno rc;

    if ((msg->rc = create_backup(&backup_name)) != 0)
    {
        ERROR("Failed to create a backup");
        return;
    }

    if (msg->is_obj)
        msg->rc = copy_obj_subtree_recursively(msg->dst_oid, msg->src_handle);
    else
        msg->rc = copy_inst_subtree_recursively(msg->dst_oid, msg->src_handle);

    if (msg->rc != 0)
    {
        /*
         * Since error was triggered by copying,
         * restore_backup() return value can be ignored
         */
        restore_backup(backup_name);
    }

    rc = release_backup(backup_name);
    if (rc != 0)
    {
        /* We can do nothing helpful here except logging */
        ERROR("Release of backup '%s' failed: %r", backup_name, rc);
        /*
         * Do not change copy status code, since it is either already
         * done successfully or failed and have appropriate status set.
         */
    }

    free(backup_name);
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
    cfg_handle    handle = 0;
    cfg_instance *inst;
    cfg_object   *obj;
    char         *oid = (char *)msg + msg->oid_offset;
    cfg_inst_val  val;
    char         *val_str = "";
    char         *ta;

    if (cfg_avoid_local_cmd_problem("add", oid, (cfg_msg *)msg,
                                    update_dh) != 0)
        return;

    /* Synchronize /agent/volatile subtree if necessary */
    if (cfg_sync_agt_volatile((cfg_msg *)msg, oid) != 0)
    {
        cfg_wipe_cmd_error(CFG_ADD, CFG_HANDLE_INVALID);
        return;
    }

    if ((msg->rc = cfg_types[msg->val_type].get_from_msg((cfg_msg *)msg,
                                                         &val)) != 0)
    {
        cfg_wipe_cmd_error(CFG_ADD, CFG_HANDLE_INVALID);
        return;
    }

    if ((msg->rc = cfg_db_add(oid, &handle, msg->val_type, val)) != 0)
    {
        ERROR("Failed to add a new instance %s into configuration "
              "database; errno %r", oid, msg->rc);
        cfg_types[msg->val_type].free(val);
        cfg_wipe_cmd_error(CFG_ADD, CFG_HANDLE_INVALID);
        return;
    }
    cfg_types[msg->val_type].free(val);

    inst = CFG_GET_INST(handle);
    assert(inst != NULL);
    obj = inst->obj;
    if (cfg_instance_volatile(inst))
        update_dh = FALSE;

    if (obj->access != CFG_READ_CREATE)
    {
        msg->rc = TE_EACCES;
        ERROR("Failed to add a new instance %s: "
             "object %s is not read-create", oid, obj->oid);
        cfg_wipe_cmd_error(CFG_ADD, handle);
        return;
    }

    if (strcmp_start(CFG_TA_PREFIX, oid) != 0)
    {
        if (strcmp_start(CFG_RCF_PREFIX, inst->oid) == 0 &&
            (msg->rc = cfg_rcf_add(inst)) != 0)
        {
            cfg_db_del(handle);
            return;
        }

        if (update_dh)
        {
            msg->rc = cfg_dh_push_command((cfg_msg *)msg, msg->local, NULL);
            if (msg->rc != 0)
            {
                ERROR("Failed to add a new instance %s in DH: %r",
                      oid, msg->rc);
                if (strcmp_start(CFG_RCF_PREFIX, inst->oid) == 0)
                    cfg_rcf_del(inst);
                cfg_wipe_cmd_error(CFG_ADD, handle);
                return;
            }
        }

        /*
         * Instance is not from agent subtree, but we set 'added'
         * to TRUE to avoid possible problems with this instance.
         */
        inst->added = TRUE;

        msg->handle = handle;
        return;
    }

    if (msg->local)
    {
        if (update_dh)
        {
            msg->rc = cfg_dh_push_command((cfg_msg *)msg, msg->local, NULL);
            if (msg->rc != 0)
            {
                ERROR("Failed to add a new instance %s in DH: %r",
                      oid, msg->rc);
                cfg_wipe_cmd_error(CFG_ADD, handle);
                return;
            }
        }

        /* Local add operation */
        VERB("Local add operation for %s OID", oid);
        msg->handle = handle;
        return;
    }

    while (inst->father != &cfg_inst_root)
        inst = inst->father;

    ta = inst->name;

    if (obj->type != CVT_NONE)
    {
        if ((msg->rc = cfg_db_get(handle, &val)) != 0)
        {
            cfg_db_del(handle);
            return;
        }

        msg->rc = cfg_types[obj->type].val2str(val, &val_str);
        cfg_types[obj->type].free(val);

        if (msg->rc != 0)
        {
            cfg_db_del(handle);
            return;
        }
    }

    msg->rc = rcf_ta_cfg_add(ta, 0, oid, val_str);
    if (msg->rc != 0)
    {
        cfg_db_del(handle);

        WARN("Failed to add a new instance %s with value '%s' into TA "
             "error=%r", oid, val_str, msg->rc);
        if (obj->type != CVT_NONE)
            free(val_str);
        return;
    }

#if 0
    if (strstr(oid, "/rsrc:") != NULL)
        oid = rsrc_oid_to_sync(ta, val_str);
#endif

    if (obj->type != CVT_NONE)
        free(val_str);

    if ((msg->rc = cfg_ta_sync(oid, TRUE)) != 0)
    {
        ERROR("Failed to synchronize subtree %s "
              "error=%r", oid, msg->rc);
        if ((inst = CFG_GET_INST(handle)) != NULL)
        {
            rcf_ta_cfg_del(ta, 0, inst->oid);
            cfg_db_del(handle);
        }
        return;
    }

    /* Add value with substitution to msg to add to DH */
    if (obj->type != CVT_NONE)
    {
        cfg_types[obj->type].put_to_msg(CFG_GET_INST(handle)->val, (cfg_msg *)msg);
        msg->oid_offset = msg->len;
        msg->len += strlen(CFG_GET_INST(handle)->oid) + 1;
        strcpy((char *)msg + msg->oid_offset, CFG_GET_INST(handle)->oid);
        oid = (char *)msg + msg->oid_offset;
    }

    if (update_dh)
    {
        msg->rc = cfg_dh_push_command((cfg_msg *)msg, msg->local, NULL);
        if (msg->rc != 0)
        {
            te_errno rc;

            ERROR("Failed to add a new instance %s in DH: %r",
                  oid, msg->rc);
            rc = rcf_ta_cfg_del(ta, 0, CFG_GET_INST(handle)->oid);
            if (rc != 0)
            {
                ERROR("Failed to delete %s: %r", CFG_GET_INST(handle)->oid,
                      rc);
                set_inconsistency_state();
            }
            cfg_wipe_cmd_error(CFG_ADD, handle);
            return;
        }
    }

    /*
     * We've just added a new instance to the Agent, so mark that
     * in instance data structure.
     */
    if ((inst = CFG_GET_INST(handle)) != NULL)
    {
        inst->added = TRUE;

        cfg_ta_sync_dependants(inst);

        cfg_conf_delay_update(inst->oid);
    }
    else
    {
        /*
         * FIXME: We have added instance, but it disappeared too fast
         * during synchronization.
         */
    }

    msg->handle = handle;
}

static te_errno
process_set_error_after_send(cfg_handle handle, const char *ta,
                             cfg_inst_val old_val)
{
    char *val_str = NULL;
    te_errno rc;

    cfg_wipe_cmd_error(CFG_SET, CFG_HANDLE_INVALID);
    rc = cfg_types[CFG_GET_INST(handle)->obj->type].val2str(old_val, &val_str);
    if (rc != 0)
    {
        ERROR("Failed to convert value to the string representation: %r", rc);
        set_inconsistency_state();
        goto out;
    }

    rc = cfg_db_set(handle, old_val);
    if (rc != 0)
    {
        ERROR("Failed to set value for %s to %s into local DB: %r",
               CFG_GET_INST(handle)->oid, val_str, rc);
        set_inconsistency_state();
        goto out;
    }

    rc = rcf_ta_cfg_set(ta, 0, CFG_GET_INST(handle)->oid, val_str);
    if (rc != 0)
    {
        ERROR("Failed to set the value for %s to %s: %r",
              CFG_GET_INST(handle)->oid, val_str, rc);
        set_inconsistency_state();
    }

out:
    free(val_str);
    return rc;
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
    char         *val_str = NULL;
    cfg_inst_val  val;
    cfg_inst_val  old_val;

    if ((inst = CFG_GET_INST(handle)) == NULL)
    {
        ERROR("Invalid handle in set request");
        msg->rc = TE_ENOENT;
        return;
    }

    if (cfg_avoid_local_cmd_problem("set", inst->oid,
                                    (cfg_msg *)msg, update_dh) != 0)
        return;

    obj = inst->obj;
    if (cfg_instance_volatile(inst))
        update_dh = FALSE;

    if ((msg->rc = cfg_types[obj->type].
                   get_from_msg((cfg_msg *)msg, &val)) != 0)
    {
        cfg_wipe_cmd_error(CFG_SET, CFG_HANDLE_INVALID);
        return;
    }

    if (obj->access != CFG_READ_WRITE &&
        obj->access != CFG_READ_CREATE)
    {
        cfg_types[obj->type].free(val);
        msg->rc = TE_EACCES;
        cfg_wipe_cmd_error(CFG_SET, CFG_HANDLE_INVALID);
        return;
    }

    if ((msg->rc = cfg_db_get(handle, &old_val)) != 0)
    {
        ERROR("Failed to get old value from DB: error=%r", msg->rc);
        cfg_types[obj->type].free(val);
        cfg_wipe_cmd_error(CFG_SET, CFG_HANDLE_INVALID);
        return;
    }

    if ((msg->rc = cfg_db_set(handle, val)) != 0)
    {
        ERROR("Failed to set new value in DB: error=%r", msg->rc);
        cfg_wipe_cmd_error(CFG_SET, CFG_HANDLE_INVALID);
        goto cleanup;
    }

    if (strcmp_start(CFG_TA_PREFIX, inst->oid) != 0)
    {
        if (strcmp_start(CFG_RCF_PREFIX, inst->oid) == 0 &&
            (msg->rc = cfg_rcf_set(inst)) != 0)
        {
            cfg_db_set(handle, old_val);
            cfg_wipe_cmd_error(CFG_SET, CFG_HANDLE_INVALID);
            goto cleanup;
        }

        if (update_dh)
        {
            msg->rc = cfg_dh_push_command((cfg_msg *)msg, msg->local, &old_val);
            if (msg->rc != 0)
            {
                ERROR("Failed to add command in DH: %r", msg->rc);;
                cfg_db_set(handle, old_val);
                if (strcmp_start(CFG_RCF_PREFIX, inst->oid) == 0)
                    cfg_rcf_set(inst);
                cfg_wipe_cmd_error(CFG_SET, CFG_HANDLE_INVALID);
            }
        }
        goto cleanup;
    }

    /* Agents subtree modification */

    if (msg->local)
    {
        if (update_dh)
        {
            msg->rc = cfg_dh_push_command((cfg_msg *)msg, msg->local, &old_val);
            if (msg->rc != 0)
            {
                ERROR("Failed to add command in DH: %r", msg->rc);;
                cfg_db_set(handle, old_val);
                cfg_wipe_cmd_error(CFG_SET, CFG_HANDLE_INVALID);
            }
        }
        /* Local set operation */
        goto cleanup;
    }

    while (inst->father != &cfg_inst_root)
        inst = inst->father;

    if (obj->type != CVT_NONE)
    {
        cfg_types[obj->type].free(val);
        /*
         * When processing a backup file or restore by the history the values
         * is contained substitutions, i.e they contain references to another
         * instance. They should be expanded before send to the RCF.
         * The expand perform in the cfg_db_get() function
         */
        if ((msg->rc = cfg_db_get(handle, &val)) != 0)
        {
            ERROR("Failed to get value from DB: %r", msg->rc);
            cfg_db_set(handle, old_val);
            cfg_types[obj->type].free(old_val);
            cfg_wipe_cmd_error(CFG_SET, CFG_HANDLE_INVALID);
            return;
        }

        msg->rc = cfg_types[obj->type].val2str(val, &val_str);
        if (msg->rc != 0)
        {
            cfg_db_set(handle, old_val);
            /*
             * Note that the cfg_wipe_cmd_error() function is missing here.
             * Perhaps this is due to the fact that the configurator model
             * assumes that there cannot be non-local command between any two
             * local commands.
             */
            goto cleanup;
        }
    }

    msg->rc = rcf_ta_cfg_set(inst->name, 0,
                             CFG_GET_INST(handle)->oid, val_str);

    if (msg->rc != 0)
    {
        cfg_db_set(handle, old_val);
        cfg_ta_sync_dependants(CFG_GET_INST(handle));
        cfg_conf_delay_update(CFG_GET_INST(handle)->oid);
        goto cleanup;
    }

    if (obj->substitution)
    {
        msg->rc = cfg_ta_sync(CFG_GET_INST(handle)->oid, TRUE);
        if (msg->rc != 0)
        {
            te_errno rc;

            ERROR("Failed to synchronize subtree %s: %r",
                CFG_GET_INST(handle)->oid, msg->rc);
            rc = process_set_error_after_send(handle, inst->name, old_val);
            if (rc != 0)
                ERROR("Failed to rollback the latest changes: %r", rc);

            goto cleanup;
        }
    }

    if (update_dh)
    {
        cfg_types[obj->type].put_to_msg(CFG_GET_INST(handle)->val,
                                        (cfg_msg *)msg);
        msg->rc = cfg_dh_push_command((cfg_msg *)msg, msg->local, &old_val);
        if (msg->rc != 0)
        {
            te_errno rc;

            ERROR("Failed to add command in DH: %r", msg->rc);
            rc = process_set_error_after_send(handle, inst->name, old_val);
            if (rc != 0)
                ERROR("Failed to rollback the latest changes: %r", rc);

            goto cleanup;
        }
    }

    cfg_ta_sync_dependants(CFG_GET_INST(handle));

    cfg_conf_delay_update(CFG_GET_INST(handle)->oid);

cleanup:
    cfg_types[obj->type].free(old_val);
    cfg_types[obj->type].free(val);
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
    char         *oid;

    if ((inst = CFG_GET_INST(handle)) == NULL)
    {
        msg->rc = TE_ENOENT;
        cfg_wipe_cmd_error(CFG_DEL, CFG_HANDLE_INVALID);
        return;
    }

    obj = inst->obj;
    if (cfg_instance_volatile(inst))
        update_dh = FALSE;

    if (obj->access != CFG_READ_CREATE)
    {
        ERROR("Only READ-CREATE objects can be removed from "
              "the configuration tree. object: %s", obj->oid);
        msg->rc = TE_EACCES;
        cfg_wipe_cmd_error(CFG_DEL, CFG_HANDLE_INVALID);
        return;
    }

    msg->rc = cfg_db_del_check(handle);
    if (msg->rc == TE_EHASSON)
    {
        /*
         * Temporary ignore TE_EHASSON errors in order to pass VLAN
         * deletion (it has automatic children in mcast_link_addr and
         * net_addr in the case of enabled IPv6 support).
         */
        msg->rc = 0;
    }
    else if (msg->rc != 0)
    {
        ERROR("%s: cfg_db_del_check fails %r", __FUNCTION__, msg->rc);
        cfg_wipe_cmd_error(CFG_DEL, CFG_HANDLE_INVALID);
        return;
    }

    if (cfg_avoid_local_cmd_problem("del", inst->oid, (cfg_msg *)msg,
                                    update_dh) != 0)
        return;

    if (update_dh &&
        (msg->rc = cfg_dh_push_command((cfg_msg *)msg, msg->local,
                                       &(inst->val))) != 0)
    {
        ERROR("%s: Failed to add into DH errno %r",
              __FUNCTION__, msg->rc);
        cfg_wipe_cmd_error(CFG_DEL, CFG_HANDLE_INVALID);
        return;
    }

    if (strcmp_start(CFG_RCF_PREFIX, inst->oid) == 0 &&
        (msg->rc = cfg_rcf_del(inst)) != 0)
    {
        if (update_dh)
            cfg_dh_delete_last_command();
        cfg_wipe_cmd_error(CFG_DEL, CFG_HANDLE_INVALID);
        return;
    }

    if (msg->local)
    {
        if (!inst->added)
        {
            ERROR("Removing locally added instance which was not committed "
                  "is not supported");
            msg->rc = TE_EOPNOTSUPP;
            cfg_wipe_cmd_error(CFG_DEL, CFG_HANDLE_INVALID);
            return;
        }
        inst->remove = TRUE;
        return;
    }

    if (strcmp_start("/agent", inst->oid) != 0)
    {
        cfg_db_del(handle);
        return;
    }

    /*
     * We should not try to delete locally added instances from
     * Test Agent, as it leads to ESRCH error.
     */
    if (inst->added)
    {
        cfg_instance *inst_aux = inst;

        while (inst_aux->father != &cfg_inst_root)
            inst_aux = inst_aux->father;

        msg->rc = rcf_ta_cfg_del(inst_aux->name, 0, inst->oid);

        if (msg->rc == 0)
        {
            VERB("Instance %s successfully deleted from the Agent",
                 inst->name);
        }
        else if (TE_RC_GET_ERROR(msg->rc) == TE_ENOENT && obj->vol)
        {
            VERB("Instance %s of volatile object has disappeared from "
                 "the Agent", inst->name);
            msg->rc = 0;
        }
        else
        {
            ERROR("%s: rcf_ta_cfg_del returns %r", __FUNCTION__, msg->rc);
            cfg_wipe_cmd_error(CFG_DEL, CFG_HANDLE_INVALID);
            if (update_dh)
            {
                cfg_dh_delete_last_command();
                return;
            }
            else if (TE_RC_GET_ERROR(msg->rc) == TE_ENOENT)
            {
                ERROR("%s: [kostik] ignoring %x TE_ENOENT",
                      __FUNCTION__, msg->rc);
                msg->rc = 0;
                /* During restoring backup the entry disappears */
            }
        }
    }

    /*
     * Instance may be removed as result of synchronization, so copy
     * of oid should be saved before it, and CFG_GET_INST(handle)
     * should be rechecked after it.
     */
    oid = strdup(inst->oid);
    if (oid == NULL)
    {
        ERROR("%s(): out of memory", __FUNCTION__);
        msg->rc = TE_ENOMEM;
        cfg_wipe_cmd_error(CFG_DEL, CFG_HANDLE_INVALID);
        return;
    }

    cfg_ta_sync_dependants(inst);

    cfg_conf_delay_update(oid);
    free(oid);

    if (CFG_GET_INST(handle) != NULL)
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
    cfg_inst_val val;

    if ((inst = CFG_GET_INST(handle)) == NULL)
    {
        msg->rc = TE_ENOENT;
        return;
    }
    obj = inst->obj;

    if ((obj->vol || msg->sync) &&
        strcmp_start("/agent", inst->oid) == 0)
    {
        msg->rc = cfg_ta_sync(inst->oid, FALSE);
        if (msg->rc != 0)
        {
            ERROR("Failed to synchronize %s: %r", inst->oid, msg->rc);
            return;
        }

        /* Volatile objects can be deleted after synchronization */
        if ((inst = CFG_GET_INST(handle)) == NULL)
        {
            msg->rc = TE_ENOENT;
            return;
        }
    }

    msg->val_type = obj->type;
    msg->len = sizeof(*msg);

    /*
     * Some instances have a substitution in its values.
     * Therefore, before returning the values, it must be expanded.
     * cfg_db_get() function does it.
     */
    if ((msg->rc = cfg_db_get(handle, &val)) != 0)
    {
        ERROR("Failed to get value for %s, rc=%r", inst->oid, msg->rc);
        return;
    }

    cfg_types[obj->type].put_to_msg(val, (cfg_msg *)msg);
    cfg_types[obj->type].free(val);
}

/* Returns time since Epoche in milliseconds */
static unsigned long long
get_time_ms(void)
{
    struct timeval tv = {0, 0};
    gettimeofday(&tv, NULL);
    return (unsigned long long)(time(NULL)) * 1000 +
           (unsigned long long)(tv.tv_usec / 1000);
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
    char        buf[128];
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
    else if (msg->rc == TE_RC(TE_CS, TE_EACCES))
    {
        level = TE_LL_VERB;
        addon = " local sequence started";
    }
    else if (msg->rc == TE_RC(TE_RCF_PCH, TE_EPERM))
    {
        level = TE_LL_WARN;
        addon = buf;
        snprintf(buf, sizeof(buf), " failed (errno=%s-%s)",
                 te_rc_mod2str(msg->rc), te_rc_err2str(msg->rc));
    }
    else
    {
        level = TE_LL_ERROR;
        addon = buf;
        snprintf(buf, sizeof(buf), " failed (errno=%s-%s)",
                 te_rc_mod2str(msg->rc), te_rc_err2str(msg->rc));
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

            LOG_MSG(level,
                    "Register object %s (%s, %s, %s)%s",
                    m->oid,
                    m->val_type == CVT_NONE ? "void" :
                    m->val_type == CVT_STRING ? "string" :
                    m->val_type == CVT_INTEGER ? "integer" :
                    m->val_type == CVT_UINT64 ? "uint64" :
                    m->val_type == CVT_ADDRESS ? "address" :
                    "unknown type",
                    m->access == CFG_READ_WRITE ? "read/write" :
                    m->access == CFG_READ_ONLY ? "read/only" :
                    m->access == CFG_READ_CREATE ? "read/create" :
                    "unknown access",
                    m->def_val > 0 ? m->oid + m->def_val : "NULL",
                    addon);
            break;
        }

        case CFG_UNREGISTER:
        {
            cfg_unregister_msg *m = (cfg_unregister_msg *)msg;

            LOG_MSG(level,
                    "Msg: request to unregister object: %s", m->id);
            break;
        }

        case CFG_ADD_DEPENDENCY:
        {
            cfg_add_dependency_msg *ad = (cfg_add_dependency_msg *)msg;
            GET_STRS(cfg_add_dependency_msg);

            LOG_MSG(level,
                    "Add dependency %s on %s %s",
                    ad->oid, s1, s2);
            break;
        }

        case CFG_FIND:
            if (!before && TE_RC_GET_ERROR(msg->rc) == TE_ENOENT)
                level = TE_LL_INFO;
            LOG_MSG(level,
                    "Find OID %s%s", ((cfg_find_msg *)msg)->oid, addon);
            break;

        case CFG_GET_DESCR:
            GET_STRS(cfg_get_descr_msg);
            LOG_MSG(level, "Get descr for %s%s%s", s1, s2, addon);
            break;

        case CFG_GET_OID:
            GET_STRS(cfg_get_oid_msg);
            LOG_MSG(level, "Get OID for %s%s%s", s1, s2, addon);
            break;

        case CFG_GET_ID:
            GET_STRS(cfg_get_id_msg);
            LOG_MSG(level, "Get ID for %s%s%s", s1, s2, addon);
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
                LOG_MSG(level, "Pattern for OID %s%s",
                        ((cfg_pattern_msg *)msg)->pattern, addon);
            }
            break;

        case CFG_FAMILY:
            GET_STRS(cfg_family_msg);
            LOG_MSG(level, "Get family (get %s) for %s%s%s",
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
            LOG_MSG(level, "Add instance %s value %s%s",
                    (char *)m + m->oid_offset,
                    val_str == NULL ? "(unknown)" :
                    (before) ? "(not processed yet)" : val_str, addon);

            if (val_str_free)
                free(val_str);
            break;
        }

        case CFG_DEL:
            GET_STRS(cfg_del_msg);
            LOG_MSG(level, "Delete %s%s%s", s1, s2, addon);
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
            LOG_MSG(level, "Set for %s%s value %s%s", s1, s2,
                    val_str == NULL ? "(unknown)" : val_str, addon);

            if (val_str_free)
                free(val_str);
            break;
        }

        case CFG_COMMIT:
            LOG_MSG(level, "Commit for %s", ((cfg_commit_msg *)msg)->oid);
            break;

        case CFG_GET:
            GET_STRS(cfg_get_msg);
            LOG_MSG(level, "Get %s%s%s", s1, s2, addon);
            break;

        case CFG_COPY:
        {
            cfg_copy_msg *m = (cfg_copy_msg *)msg;
            cfg_instance *inst = NULL;

            if (CFG_IS_INST(m->src_handle))
                inst = CFG_GET_INST(m->src_handle);

            if (inst != NULL)
                s1 = inst->oid;
            else
                s1 = "unknown source instance";

            LOG_MSG(level, "Copy from %s to %s%s", s1, m->dst_oid, addon);
            break;
        }

        case CFG_SYNC:
            LOG_MSG(level, "Synchronize %s%s%s",
                    ((cfg_sync_msg *)msg)->oid,
                    ((cfg_sync_msg *)msg)->subtree ? " (subtree)" : "",
                    addon);
            break;

        case CFG_REBOOT:
            LOG_MSG(level, "Reboot Test Agent %s%s",
                    ((cfg_reboot_msg *)msg)->ta_name, addon);
            break;

        case CFG_BACKUP:
        {
            uint8_t op = ((cfg_backup_msg *)msg)->op;
            char *bkp_filename = (char *)((cfg_backup_msg *)msg) +
                                 ((cfg_backup_msg *)msg)->filename_offset;

            if (!before && (op == CFG_BACKUP_VERIFY) &&
                (TE_RC_GET_ERROR(msg->rc) == TE_EBACKUP))
            {
                level = TE_LL_INFO;
            }
            LOG_MSG(level,
                    "%s backup %s%s%s",
                    op == CFG_BACKUP_CREATE ? "Create" :
                    op == CFG_BACKUP_RESTORE ? "Restore" :
                    op == CFG_BACKUP_RESTORE_NOHISTORY ?
                    "Restore w/o history" :
                    op == CFG_BACKUP_RELEASE ? "Release" :
                    op == CFG_BACKUP_VERIFY ? "Verify" : "unknown",
                    op == CFG_BACKUP_CREATE ? "" : bkp_filename,
                    op == CFG_BACKUP_CREATE ? "" : " ", addon);
            break;
        }

        case CFG_CONFIG:
            LOG_MSG(level, "Create configuration file %s (%s)%s",
                    ((cfg_config_msg *)msg)->filename,
                    ((cfg_config_msg *)msg)->history ?
                    "history" : "backup", addon);
            break;

        case CFG_PROCESS_HISTORY:
            LOG_MSG(level, "Process history configuration file %s",
                    ((cfg_process_history_msg *)msg)->filename);
            break;

        case CFG_CONF_DELAY:
            LOG_MSG(level, "Wait configuration changes");
            break;

        case CFG_CONF_TOUCH:
            LOG_MSG(level, "Change of %s by non-CS means is reported",
                    ((cfg_conf_touch_msg *)msg)->oid);
            break;

        case CFG_SHUTDOWN:
            LOG_MSG(level, "Shutdown command%s", addon);
            break;

        case CFG_TREE_PRINT:
            cfg_db_tree_print_msg_log((cfg_tree_print_msg *)msg, level);
            break;

        default:
            ERROR("Unknown command %x", msg->type);
    }
#undef GET_STRS
}

/**
 * Check if the current DB changes from the backup.
 *
 * @param filename      backup filename
 * @param log           if TRUE, log changes
 * @param error_msg     if not NULL, log failure with specified message
 * @param subtrees       Subtree to verification. @c NULL to verify all trees
 *
 * @return 0 if DB state does not differ from backup; status code otherwise
 */
static inline te_errno
verify_backup(const char *backup, te_bool log, const char *msg,
              const te_vec *subtrees)
{
    char diff_file[RCF_MAX_PATH];
    int  rc;

    if ((rc = cfg_backup_create_file(filename, subtrees)) != 0)
        return rc;

    TE_SPRINTF(diff_file, "%s/te_cs.diff", getenv("TE_TMP"));
    sprintf(tmp_buf, "diff -u %s %s >%s 2>&1", backup,
            filename, diff_file);

    rc = ((system(tmp_buf) == 0) ? 0 : TE_EBACKUP);
    if (rc != 0)
    {
        if (msg != NULL)
            WARN("%s\n%Tf", msg, diff_file);
        else if (log)
        {
            if (cs_flags & CS_LOG_DIFF)
                TE_LOG(TE_LL_INFO, TE_LGR_ENTITY, TE_LGR_USER,
                       "Backup diff:\n%Tf", diff_file);
            else
                INFO("Backup diff:\n%Tf", diff_file);
        }
    }
    unlink(diff_file);

    return rc;
}

/**
 * Check the running agents
 *
 * @return 0 if all agents are running normally;
 *         @c TE_ETADEAD if at least one of the agents is dead;
 *         stasus code otherwise
 */
static te_errno
check_agents(void)
{
    te_vec dead_agents = TE_VEC_INIT(char *);
    te_errno rc;

    rc = rcf_get_dead_agents(&dead_agents);
    if (rc != 0)
    {
        te_vec_deep_free(&dead_agents);
        return rc;
    }

    if (te_vec_size(&dead_agents) != 0)
        rc = TE_ETADEAD;

    te_vec_deep_free(&dead_agents);

    return rc;
}

static te_errno
subtrees_str2vec(const char *str, te_vec *vec, unsigned int subtrees_num)
{
    const char *ch = str;
    const char *current = str;
    unsigned int i = 0;
    te_errno rc = 0;

    while (i != subtrees_num)
    {
        if (*ch != '\0')
        {
            ch++;
            continue;
        }

        rc = te_vec_append_str_fmt(vec, "%s", current);
        if (rc != 0)
            break;

        ch++;
        current = ch;
        i++;
    }

    return rc;
}

static te_errno
get_path_to_xslt_filter(te_string *path)
{
    char *dir_name = NULL;

    dir_name = getenv("TE_INSTALL");
    if (dir_name == NULL)
    {
        ERROR("Failed to get TE_INSTALL");
        return TE_ENOENT;
    }

    return te_string_append(path, "%s/default/share/xsl/%s", dir_name,
                            CONF_SUBTREE_BACKUP_FILTER_NAME);
}

/**
 * Filter the backup file by the specified subtree and
 * save the result
 *
 * @param current_backup Backup file
 * @param subtree        Vector of subtrees for which to filter the backup file
 * @param target_backup  OUT: Result file name
 *
 * @return Status code
 */
static te_errno
filter_backup_by_subtrees(const char *current_backup, const te_vec *subtrees,
                          te_string *target_backup)
{
    te_string filter_cmd = TE_STRING_INIT;
    te_string xslt_file = TE_STRING_INIT;
    te_string filter_filename = TE_STRING_INIT;
    te_errno rc;

    if (subtrees == NULL || te_vec_size(subtrees) == 0)
        return te_string_append(target_backup, "%s", current_backup);

    rc = te_string_append(&filter_filename, CONF_FILTERS_FILE_NAME,
                          tmp_dir, getpid(), get_time_ms());
    if (rc != 0)
        goto out;

    rc = cfg_backup_create_filter_file(filter_filename.ptr, subtrees);
    if (rc != 0)
        goto out;

    rc = te_string_append(target_backup, CONF_SUBTREE_BACKUP_NAME,
                          tmp_dir, getpid(), get_time_ms());
    if (rc != 0)
        goto out;

    rc = get_path_to_xslt_filter(&xslt_file);
    if (rc != 0)
        goto out;

    rc = te_string_append(&filter_cmd,
                          "xsltproc --stringparam filters %s %s %s > %s",
                          filter_filename.ptr, xslt_file.ptr,
                          current_backup, target_backup->ptr);
    if (rc != 0)
        goto out;

    rc = system(filter_cmd.ptr);
    if (rc != 0)
        ERROR("Failed to extract subtrees from the backup file");

out:
    te_string_free(&filter_cmd);
    te_string_free(&xslt_file);
    te_string_free(&filter_filename);

    return rc;
}

static te_errno
delete_rcf_conf_agent(cfg_handle handle)
{
    cfg_instance *agent_instance;
    cfg_handle status_handle;
    cfg_msg *msg;
    const size_t msg_size = sizeof(cfg_set_msg) + CFG_OID_MAX;
    te_errno rc;

    agent_instance = CFG_GET_INST(handle);
    if (agent_instance == NULL)
    {
        ERROR("Failed to get agent instance");
        return TE_EINVAL;
    }

    msg = TE_ALLOC(msg_size);
    if (msg == NULL)
        return TE_ENOMEM;

    cfg_ipc_mk_find_fmt((cfg_find_msg *)msg, msg_size,
                        "%s/status:", agent_instance->oid);
    cfg_process_msg(&msg, FALSE);
    if (msg->rc != 0)
    {
        rc = msg->rc;
        ERROR("Failed to find %s/status: %r", agent_instance->oid, rc);
        free(msg);
        return rc;
    }
    status_handle = ((cfg_find_msg *)msg)->handle;

    /*
     * /rcf/agent API allows to perform some actions with the agent
     * (including deletion) if the agent is down. Therefore, first we
     * need to set the status to 0.
     */
    cfg_ipc_mk_set_int((cfg_set_msg *)msg, msg_size, status_handle,
                       FALSE, 0);
    cfg_process_msg(&msg, FALSE);
    if (msg->rc != 0)
    {
        rc = msg->rc;
        ERROR("Failed to disable TA %s: %r", agent_instance->oid, rc);
        free(msg);
        return rc;
    }

    cfg_ipc_mk_del((cfg_del_msg *)msg, msg_size, handle, FALSE);
    cfg_process_msg(&msg, FALSE);

    rc = msg->rc;
    if (rc != 0)
        ERROR("Failed to delete instance %s: %r", agent_instance->oid, rc);

    free(msg);

    return rc;
}

static te_errno
reboot_dead_agents(const te_vec *dead_agents)
{
    te_vec reboot_list = TE_VEC_INIT(char *);
    char * const *dead_agent;
    cfg_find_msg *msg = NULL;
    const size_t msg_size = sizeof(cfg_find_msg) + CFG_OID_MAX;
    te_errno rc = 0;

    msg = TE_ALLOC(msg_size);
    if (msg == NULL)
        goto out;

    TE_VEC_FOREACH(dead_agents, dead_agent)
    {
        cfg_ipc_mk_find_fmt(msg, msg_size, "/rcf:/agent:%s",
                            *dead_agent);

        cfg_process_msg((cfg_msg **)&msg, FALSE);
        if (TE_RC_GET_ERROR(msg->rc) == TE_ENOENT)
        {
            rc = te_vec_append_str_fmt(&reboot_list, "%s", *dead_agent);
            if (rc != 0)
                goto out;
        }
        else if (msg->rc == 0)
        {
            rc = delete_rcf_conf_agent(msg->handle);
            if (rc != 0)
                goto out;
        }
        else
        {
            ERROR("Failed to find /rcf:/agent: instance: %r", msg->rc);
            goto out;
        }

        RING("TA '%s' is dead and will be rebooted", *dead_agent);
    }

    rc = conf_ta_reboot_agents(&reboot_list);
    if (rc != 0)
        goto out;

out:
    te_vec_deep_free(&reboot_list);
    free(msg);
    return rc;
}

static te_errno
check_and_reanimate_agents(const te_vec *already_dead_agents)
{
    te_errno rc = 0;
    te_vec dead_agents = TE_VEC_INIT(char *);

    rc = rcf_get_dead_agents(&dead_agents);
    if (rc != 0)
        goto out;

    if (te_vec_size(&dead_agents) == 0 && already_dead_agents == NULL)
        return 0;

    if (te_vec_size(&dead_agents) != 0)
    {
        rc = reboot_dead_agents(&dead_agents);
        if (rc != 0)
            goto out;
    }

    if (already_dead_agents != NULL)
    {
        rc = te_vec_append_vec(&dead_agents, already_dead_agents);
        if (rc != 0)
            goto out;
    }

    cfg_ta_sync("/:", TRUE);

    rc = cfg_dh_restore_agents(&dead_agents);
    if (rc != 0)
        goto out;

out:
    te_vec_deep_free(&dead_agents);
    return rc;
}

/**
 * Process backup user request.
 *
 * @param msg           message pointer
 * @param release_dh    Release DH if restore or verify action was a success.
 *                      Does not affect on create and release backup actions
 */
static void
process_backup(cfg_backup_msg *msg, te_bool release_dh)
{
    char *backup_filename;
    char *subtrees = NULL;
    te_vec subtrees_vec = TE_VEC_INIT(char *);

    if (msg->subtrees_num != 0)
    {
        subtrees = (char *)msg + msg->subtrees_offset;
        msg->rc = subtrees_str2vec(subtrees, &subtrees_vec, msg->subtrees_num);
        if (msg->rc != 0)
        {
            ERROR("Failed to convert string with subtrees to vector: %r",
                  msg->rc);
            return;
        }
    }

    backup_filename = (char *)msg + msg->filename_offset;

    switch (msg->op)
    {
        case CFG_BACKUP_CREATE:
        {
            sprintf(backup_filename, CONF_BACKUP_NAME,
                    tmp_dir, getpid(), get_time_ms());

            if ((msg->rc = cfg_backup_create_file(backup_filename,
                                                  &subtrees_vec)) != 0)
            {
                break;;
            }

            if ((msg->rc = cfg_dh_attach_backup(backup_filename)) != 0)
                unlink(backup_filename);

            msg->len += strlen(backup_filename) + 1;

            break;
        }

        case CFG_BACKUP_RESTORE:
        case CFG_BACKUP_RESTORE_NOHISTORY:
        {
            te_string backup = TE_STRING_INIT;

            msg->rc = check_and_reanimate_agents(NULL);
            if (msg->rc != 0)
                return;

            rcf_log_cfg_changes(TRUE);

            if (msg->op != CFG_BACKUP_RESTORE_NOHISTORY)
            {
                /* Try to restore using dynamic history */
                msg->rc = cfg_dh_restore_backup(backup_filename, TRUE);
                if (msg->rc != 0)
                {
                    WARN("Restoring backup from history failed; "
                         "restore from the file");
                }
                else
                {
                    cfg_conf_delay_reset();
                    cfg_ta_sync("/:", TRUE);

                    msg->rc = verify_backup(backup_filename, FALSE,
                                            "Restoring backup from history "
                                            "failed:", NULL);
                    if (msg->rc == 0)
                    {
                        rcf_log_cfg_changes(FALSE);
                        break;
                    }
                }

                if (TE_RC_GET_ERROR(msg->rc) == TE_ETADEAD)
                {
                    rcf_check_agents();
                }
                cfg_ta_sync("/:", TRUE);
            }

            /*
             * If subtrees is NULL @p backup string will contain
             * filename specified by the user
             */
            msg->rc = filter_backup_by_subtrees(backup_filename, &subtrees_vec,
                                                &backup);
            if (msg->rc != 0)
            {
                ERROR("Restore backup failed: %r", msg->rc);
                te_string_free(&backup);
                break;
            }

            msg->rc = parse_config_xml(backup.ptr, NULL, FALSE, &subtrees_vec);
            rcf_log_cfg_changes(FALSE);

            if (release_dh)
                cfg_dh_release_after(backup_filename);

            te_string_free(&backup);
            break;
        }

        case CFG_BACKUP_VERIFY:
        {
            te_errno rc;
            te_string backup = TE_STRING_INIT;

            /*
             * If subtrees is NULL @p backup string will contain
             * filename specified by the user
             */
            rc = filter_backup_by_subtrees(backup_filename, &subtrees_vec,
                                           &backup);
            if (rc != 0)
            {
                ERROR("Backup verification failed: %r", rc);
                msg->rc = rc;
                te_string_free(&backup);
                break;
            }

            rc = check_agents();
            if (rc != 0){
                ERROR("Backup verification failed: %r", rc);
                msg->rc = rc;
                te_string_free(&backup);
                break;
            }

            msg->rc = verify_backup(backup.ptr, TRUE, NULL, &subtrees_vec);
            if (msg->rc != 0)
            {
                cfg_ta_sync("/:", TRUE);
                msg->rc = verify_backup(backup.ptr, TRUE, NULL, &subtrees_vec);
            }

            if (msg->rc == 0 && release_dh)
                cfg_dh_release_after(backup_filename);

            te_string_free(&backup);

            break;
        }

        case CFG_BACKUP_RELEASE:
        {
            msg->rc = cfg_dh_release_backup(backup_filename);
            break;
        }
    }

    te_vec_deep_free(&subtrees_vec);
}

/**
 * Create backup.
 *
 * @param bkp_filename          backup filename pointer
 */
static te_errno
create_backup(char **bkp_filename)
{
    te_errno rc = 0;
    cfg_backup_msg *bkp_msg = TE_ALLOC(sizeof(cfg_backup_msg) +
                                       sizeof(max_commit_subtree));
    if (bkp_msg == NULL)
        return TE_ENOMEM;

    if (*bkp_filename != NULL)
        cfg_dh_release_backup(*bkp_filename);

    bkp_msg->type = CFG_BACKUP;
    bkp_msg->op = CFG_BACKUP_CREATE;
    bkp_msg->len = sizeof(cfg_backup_msg);
    bkp_msg->subtrees_num = 0;
    bkp_msg->subtrees_offset = bkp_msg->len;
    bkp_msg->filename_offset = bkp_msg->len;

    process_backup(bkp_msg, TRUE);
    rc = bkp_msg->rc;
    if (rc != 0)
    {
        ERROR("%s() Failed to create backup %r", __FUNCTION__, rc);
        free(bkp_msg);
        return rc;
    }

    *bkp_filename = strdup((const char *)bkp_msg + bkp_msg->filename_offset);
    if (*bkp_filename == NULL)
    {
        cfg_dh_release_backup((char *)bkp_msg + bkp_msg->filename_offset);
        rc = TE_ENOMEM;
    }

    free(bkp_msg);

    return rc;
}

/**
 * Verify/release/restore backup.
 *
 * @param name      Backup name
 * @param op        Backup operation
 *
 * @return Status code
 */
static te_errno
process_backup_op(const char *name, uint8_t op)
{
    te_errno rc = 0;
    size_t len;
    cfg_backup_msg *bkp_msg = TE_ALLOC(sizeof(cfg_backup_msg) +
                                       sizeof(max_commit_subtree));
    if (bkp_msg == NULL)
        return TE_ENOMEM;

    bkp_msg->type = CFG_BACKUP;
    bkp_msg->op = op;
    bkp_msg->len = sizeof(cfg_backup_msg);
    bkp_msg->subtrees_num = 0;
    bkp_msg->subtrees_offset = bkp_msg->len;
    bkp_msg->filename_offset = bkp_msg->len;

    len = strlen(name) + 1;
    bkp_msg->len += len;
    memcpy((char *)bkp_msg + bkp_msg->filename_offset, name, len);

    process_backup(bkp_msg, TRUE);
    rc = bkp_msg->rc;

    free(bkp_msg);
    return rc;
}

/*
 * Release backup
 *
 * @param name  Backup name
 *
 * @return Status code
 */
static te_errno
release_backup(const char *name)
{
    return process_backup_op(name, CFG_BACKUP_RELEASE);
}

/*
 * Restore backup
 *
 * @param name  Backup name
 *
 * @return Status code
 */
static te_errno
restore_backup(const char *name)
{
    return process_backup_op(name, CFG_BACKUP_RESTORE);
}

/**
 * Process reboot user request.
 *
 * @param msg           message pointer
 */
static void
process_reboot(cfg_reboot_msg *msg)
{
    msg->rc = rcf_ta_reboot(msg->ta_name, NULL, NULL,
                            msg->reboot_type);
    if (msg->rc != 0)
    {
        ERROR("Failed to reboot Test Agent %s: %r", msg->ta_name, msg->rc);
        return;
    }

    if (msg->restore)
    {
        te_vec agents = TE_VEC_INIT(char *);

        msg->rc = te_vec_append_str_fmt(&agents, "%s", msg->ta_name);
        if (msg->rc != 0)
            return;

        msg->rc = check_and_reanimate_agents(&agents);
        if (msg->rc != 0)
            ERROR("Failed to reanimate agent(s): %r", msg->rc);

        te_vec_free(&agents);
    }
}

/**
 * Process register user request.
 *
 * @param msg           message pointer
 * @param update_dh     if true, add the command to dynamic history
 */
static void
process_register(cfg_register_msg *msg, te_bool update_dh)
{
    cfg_object *obj;

    if (update_dh)
    {
        if ((msg->rc = cfg_dh_push_command((cfg_msg *)msg, FALSE, NULL)) != 0)
            return;
    }

    cfg_process_msg_register(msg);

    if (msg->rc != 0)
    {
        if (update_dh)
            cfg_dh_delete_last_command();

        return;
    }

    obj = CFG_GET_OBJ(msg->handle);
    if (obj->access == CFG_READ_WRITE || obj->access == CFG_READ_ONLY)
    {
        if ((msg->rc = cfg_add_all_inst_by_obj(obj)) != 0)
            return;
    }

    if (strcmp_start("/agent", obj->oid) == 0)
    {
        /*
         * Since synchronization is a side effect
         * of this function, errors will be logged,
         * but can be ignored.
         */
        cfg_ta_sync_obj(obj->father, TRUE);
    }
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
            process_register((cfg_register_msg *)(*msg), update_dh);
            break;

        case CFG_UNREGISTER:
            cfg_process_msg_unregister((cfg_unregister_msg *)*msg);
            break;

        case CFG_ADD_DEPENDENCY:
            cfg_process_msg_add_dependency((cfg_add_dependency_msg *)*msg);
            break;

        case CFG_FIND:
            /* Synchronize /agent/volatile subtree if necessary */
            if (cfg_sync_agt_volatile(*msg,
                                      ((cfg_find_msg *)*msg)->oid) != 0)
            {
                break;
            }
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
            /* Synchronize /agent/volatile subtree if necessary */
            if (cfg_sync_agt_volatile(*msg,
                ((cfg_pattern_msg *)*msg)->pattern) != 0)
            {
                break;
            }

            *msg = (cfg_msg *)cfg_process_msg_pattern(
                                  (cfg_pattern_msg *)*msg);
            break;

        case CFG_FAMILY:
            CFG_CHECK_NO_LOCAL_SEQ_BREAK("family", *msg);
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
            CFG_CHECK_NO_LOCAL_SEQ_BREAK("get", *msg);
            process_get((cfg_get_msg *)(*msg));
            break;

        case CFG_COPY:
        {
            process_copy((cfg_copy_msg *)(*msg));
            break;
        }

        case CFG_SYNC:
            (*msg)->rc = cfg_ta_sync(((cfg_sync_msg *)(*msg))->oid,
                                     ((cfg_sync_msg *)(*msg))->subtree);
            break;

        case CFG_REBOOT:
            process_reboot((cfg_reboot_msg *)(*msg));
            break;

        case CFG_BACKUP:
            /*
             * @p update_dh in this case is used as the flag to
             * release or not release DH
             */
            process_backup((cfg_backup_msg *)(*msg), update_dh);
            cfg_conf_delay_reset();
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
                                 ((cfg_config_msg *)(*msg))->filename, NULL);
            }
            break;

        case CFG_PROCESS_HISTORY:
            {
                te_kvpair_h expand_vars;
                cfg_process_history_msg *history_msg;

                history_msg = (cfg_process_history_msg *) (*msg);
                te_kvpair_init(&expand_vars);

                if (history_msg->len == sizeof(cfg_config_msg) +
                                        strlen(history_msg->filename) + 1)
                {
                    (*msg)->rc = put_env_vars_in_list(&expand_vars);
                }
                else
                {
                    (*msg)->rc = parse_kvpair((cfg_process_history_msg *) (*msg), &expand_vars);
                }

                if ((*msg)->rc == 0)
                    (*msg)->rc = parse_config(history_msg->filename, &expand_vars);

                te_kvpair_fini(&expand_vars);
                break;
            }

        case CFG_CONF_DELAY:
            cfg_conf_delay_reset();
            break;

        case CFG_CONF_TOUCH:
            cfg_conf_delay_update(((cfg_conf_touch_msg *)(*msg))->oid);
            break;

        case CFG_SHUTDOWN:
            /* Remove commands initiated by configuration file */
            rcf_log_cfg_changes(TRUE);
            cfg_dh_restore_backup_on_shutdown();
            rcf_log_cfg_changes(FALSE);
            cs_flags |= CS_SHUTDOWN;
            break;

        case CFG_TREE_PRINT:
            cfg_process_msg_tree_print((cfg_tree_print_msg *)*msg);
            break;

        default: /* Should not occur */
            ERROR("Unknown message is received");
            break;
    }

    (*msg)->rc = TE_RC(TE_CS, (*msg)->rc);

    log_msg(*msg, FALSE);
}

/**
 * Free globally allocated resources.
 */
static void
free_resources(void)
{
    VERB("Destroy history");
    cfg_dh_destroy();

    VERB("Destroy database");
    cfg_db_destroy();

    VERB("Free resources");
    free(cfg_get_buf);

    VERB("Closing server");
    ipc_close_server(server);

    free(filename);
}

/**
 * Process command line options and parameters specified in argv.
 * The procedure contains "Option table" that should be updated
 * if some new options are going to be added.
 *
 * @param argc  Number of elements in array "argv".
 * @param argv  Array of strings that represents all command line arguments
 *
 * @return EXIT_SUCCESS or EXIT_FAILURE
 */
static int
process_cmd_line_opts(int argc, char **argv)
{
    poptContext  optCon;
    int          rc;
    int          cfg_file_num = 0;
    const char  *cfg_files = NULL;

    int i;
    for (i = 0; i < argc; i++)
      VERB("arg %d - %s", i, argv[i]);

    /* Option Table */
    struct poptOption options_table[] = {
        { "print-trees", '\0', POPT_ARG_NONE | POPT_BIT_SET, &cs_flags,
          CS_PRINT_TREES, "Print objects and object instances trees "
          "after initialization.", NULL },

        { "log-diff", '\0', POPT_ARG_NONE | POPT_BIT_SET, &cs_flags,
          CS_LOG_DIFF, "Log diff if backup verification failed.", NULL },

        { "foreground", 'f', POPT_ARG_NONE | POPT_BIT_SET, &cs_flags,
          CS_FOREGROUND,
          "Run in foreground (usefull for debugging).", NULL },

        { "sniff-conf", '\0', POPT_ARG_STRING, &cs_sniff_cfg_file, 0,
          "Auxiliary conf file for the sniffer framework.", NULL },

        POPT_AUTOHELP
        POPT_TABLEEND
    };

    /* Process command line options */
    optCon = poptGetContext(NULL, argc, (const char **)argv,
                            options_table, 0);

    poptSetOtherOptionHelp(optCon, "[OPTIONS] <cfg-file>");

    rc = poptGetNextOpt(optCon);
    if (rc != -1)
    {
        /* An error occurred during option processing */
        ERROR("%s: %s", poptBadOption(optCon, POPT_BADOPTION_NOALIAS),
              poptStrerror(rc));
        poptFreeContext(optCon);
        return EXIT_FAILURE;
    }

    cfg_files = poptGetArg(optCon);
    INFO("%s: cfg_files=%s", __FUNCTION__, cfg_files);
    cs_cfg_file[0] = strtok((char *)cfg_files, " ");
    INFO("%s: cs_cfg_file=%s", __FUNCTION__, cs_cfg_file[0]);
    while( cfg_file_num < MAX_CFG_FILES &&
           cs_cfg_file[cfg_file_num++] != NULL) {
      cs_cfg_file[cfg_file_num] = strtok(NULL, " ");
      INFO("%s: cs_cfg_file=%s", __FUNCTION__, cs_cfg_file[cfg_file_num]);
    }
    if (cs_cfg_file[0] == NULL)
    {
      ERROR("No configuration file in command-line arguments");
      poptFreeContext(optCon);
      return EXIT_FAILURE;
    }

    if (poptGetArg(optCon) != NULL)
    {
        ERROR("Unexpected arguments in command-line");
        poptFreeContext(optCon);
        return EXIT_FAILURE;
    }

    poptFreeContext(optCon);

    return EXIT_SUCCESS;
}

/**
 * Handler for SIGPIPE signal.
 *
 * @param signum        Signal number (unused)
 */
static void
cfg_sigpipe_handler(int signum)
{
    UNUSED(signum);
    WARN("SIGPIPE received");
}

/**
 * Figure out configuration file type (XML or YAML)
 * and proceed with parsing.
 *
 * @param fname         Name of the configuration file
 * @param expand_vars   List of key-value pairs for expansion in file
 *
 * @return Status code.
 */
static te_errno
parse_config(const char *fname, te_kvpair_h *expand_vars)
{
    FILE *f;
    int   ret;
    char  str[6];

    f = fopen(fname, "r");
    if (f == NULL)
    {
        ERROR("Failed to open configuration file '%s'", fname);
        return TE_OS_RC(TE_CS, errno);
    }

    if (fseek(f, 0, SEEK_SET) < 0)
    {
        ERROR("Failed to set position in configuration file '%s'", fname);
        fclose(f);
        return TE_OS_RC(TE_CS, errno);
    }

    ret = fscanf(f, " %5s", str);
    if (ret == EOF)
    {
        int error_code_set = ferror(f);

        ERROR("Failed to read the first non-whitespace characters "
              "from configuration file '%s'", fname);
        fclose(f);
        if (error_code_set != 0)
                return TE_OS_RC(TE_CS, errno);
        else
                return TE_RC(TE_CS, TE_ENODATA);
    }

    fclose(f);

    if (strcmp(str, "<?xml") == 0)
        return parse_config_xml(fname, expand_vars, TRUE, NULL);
#if WITH_CONF_YAML
    else if (strcmp(str, "---") == 0)
        return parse_config_yaml(fname, expand_vars, NULL);
#endif /* !WITH_CONF_YAML */

    ERROR("Failed to recognise the format of configuration file '%s'", fname);
    return TE_RC(TE_CS, TE_EOPNOTSUPP);
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
    int result = EXIT_FAILURE;
    int rc;
    int cfg_file_id;


    te_log_init("Configurator", ten_log_message);

    if (atexit(free_resources) != 0)
    {
        ERROR("atexit() failed");
        goto exit;
    }

    if ((rc = process_cmd_line_opts(argc, argv)) != EXIT_SUCCESS)
    {
        ERROR("Fatal error during command line options processing");
        goto exit;
    }

    VERB("Starting...");

#if HAVE_SIGNAL_H
    (void)signal(SIGPIPE, cfg_sigpipe_handler);
#endif

    ipc_init();
    if (ipc_register_server(CONFIGURATOR_SERVER, CONFIGURATOR_IPC,
                            &server) != 0)
    {
        ERROR("Failed to register IPC server");
        goto exit;
    }
    assert(server != NULL);

    if ((tmp_dir = getenv("TE_TMP")) == NULL)
    {
        ERROR("Fatal error: TE_TMP is empty");
        goto exit;
    }

    if ((filename = (char *)malloc(strlen(tmp_dir) +
                                   strlen("/te_cfg_tmp.xml") + 1)) == NULL)
    {
        ERROR("No enough memory");
        goto exit;
    }
    sprintf(filename, "%s/te_cfg_tmp.xml", tmp_dir);

    if ((rc = cfg_db_init()) != 0)
    {
        ERROR("Fatal error: cannot initialize database");
        goto exit;
    }

    for (cfg_file_id = 0;
         cs_cfg_file[cfg_file_id] != NULL && cfg_file_id < MAX_CFG_FILES;
         cfg_file_id++)
    {
        INFO("-> %s", cs_cfg_file[cfg_file_id]);
        rc = parse_config(cs_cfg_file[cfg_file_id], NULL);
        if (rc != 0)
        {
            ERROR("Fatal error during configuration file parsing: %d - %s",
                  cfg_file_id, cs_cfg_file[cfg_file_id]);
            goto exit;
        }
    }

    if (cs_sniff_cfg_file != NULL &&
        (rc = parse_config_xml(cs_sniff_cfg_file, NULL, TRUE, NULL)) != 0)
    {
        ERROR("Fatal error during sniffer configuration file parsing");
        goto exit;
    }

    if (cs_flags & CS_PRINT_TREES)
    {
        /* print_otree(&cfg_obj_root, 0); */
        /* print_tree(&cfg_inst_root, 0); */
        cfg_db_tree_print("objects", TE_LL_RING, "/");
        cfg_db_tree_print("instances", TE_LL_RING, "/:");
    }

    /*
     * Go to background, if foreground mode is not requested.
     * No threads should be created before become a daemon.
     */
    if ((~cs_flags & CS_FOREGROUND) && (daemon(TRUE, FALSE) != 0))
    {
        ERROR("daemon() failed");
        goto exit;
    }

    INFO("Initialization is finished");
    cfg_conf_delay = 0;

    while (TRUE)
    {
        struct ipc_server_client *user = NULL;

        cfg_msg *msg = (cfg_msg *)buf;
        size_t   len = CFG_BUF_LEN;

        if ((rc = ipc_receive_message(server, buf, &len, &user)) != 0)
        {
            ERROR("Failed receive user request: errno=%r", rc);
            continue;
        }

        if (cs_inconsistency_state && msg->type != CFG_SHUTDOWN)
        {
            ERROR("Configurator is in inconsistent state");
            msg->rc = TE_RC(TE_CS, TE_EFAULT);
        }
        else
        {
            msg->rc = 0;
            cfg_process_msg(&msg, TRUE);
        }

        rc = ipc_send_answer(server, user, (char *)msg, msg->len);
        if (rc != 0)
        {
            ERROR("Cannot send an answer to user: errno=%r", rc);
        }

        if ((char *)msg != buf)
            free(msg);

        if (cs_flags & CS_SHUTDOWN)
        {
            result = EXIT_SUCCESS;
            break;
        }
    }

exit:
    if (result != EXIT_SUCCESS)
        ERROR("Error exit");
    else
        RING("Exit");

    return result;
}
