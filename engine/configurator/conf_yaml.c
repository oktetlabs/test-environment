/** @file
 * @brief YAML configuration file processing facility
 *
 * Implementation.
 *
 * Copyright (C) 2018 OKTET Labs. All rights reserved.
 *
 * @author Ivan Malov <Ivan.Malov@oktetlabs.ru>
 */

#include "conf_defs.h"
#include "conf_dh.h"
#include "conf_ta.h"
#include "conf_yaml.h"
#include "te_str.h"

#include <libxml/xinclude.h>
#include <yaml.h>

#define CS_YAML_ERR_PREFIX "YAML configuration file parser "

#define YAML_TARGET_CONTEXT_INIT \
    { NULL, NULL, NULL, NULL, NULL, TRUE, TRUE };

typedef struct config_yaml_target_s {
    const char *command_name;
    const char *target_name_singular;
    const char *target_name_plural;
} config_yaml_target_t;

static const config_yaml_target_t config_yaml_targets[] = {
    { "add", "instance", "instances" },
    { "set", "instance", "instances" },
    { "delete", "instance", "instances" },
    { "register", "object", "objects" },
    { "unregister", "object", "objects" },
    { NULL, NULL, NULL }
};

static const char *
get_yaml_cmd_targets(const char *cmd)
{
    const config_yaml_target_t *target = config_yaml_targets;

    for (; target->command_name != NULL; ++target)
    {
        if (strcmp(cmd, target->command_name) == 0)
            break;
    }

    return target->target_name_plural;
}

static const char *
get_yaml_cmd_target(const char *cmd)
{
    const config_yaml_target_t *target = config_yaml_targets;

    for (; target->command_name != NULL; ++target)
    {
        if (strcmp(cmd, target->command_name) == 0)
            break;
    }

    return target->target_name_singular;
}

/**
 * Evaluate logical expression.
 *
 * @param text  String representation of the expression
 * @param condp Location for the result
 *
 * @return Status code.
 */
static te_errno
parse_config_yaml_cond_exp(const char *text,
                           te_bool    *condp)
{
    char     *text_copy = NULL;
    char     *sp = NULL;
    char     *var_name;
    char     *op_str;
    char     *cmp_str;
    char     *var_str;
    te_errno  rc = 0;

    text_copy = strdup(text);
    if (text_copy == NULL)
        return TE_ENOMEM;

    var_name = strtok_r(text_copy, " ", &sp);
    if (var_name == NULL)
    {
        rc = TE_EINVAL;
        goto out;
    }

    op_str = strtok_r(NULL, " ", &sp);
    if (op_str == NULL)
    {
        rc = TE_EINVAL;
        goto out;
    }

    cmp_str = strtok_r(NULL, " ", &sp);
    if (cmp_str == NULL)
    {
        rc = TE_EINVAL;
        goto out;
    }

    var_str = getenv(var_name);

    if (strcmp(op_str, "==") == 0)
    {
        var_str = (var_str == NULL) ? "" : var_str;

        *condp = (strcmp(var_str, cmp_str) == 0) ? TRUE : FALSE;
    }
    else if (strcmp(op_str, "!=") == 0)
    {
        var_str = (var_str == NULL) ? "" : var_str;

        *condp = (strcmp(var_str, cmp_str) != 0) ? TRUE : FALSE;
    }
    else
    {
        long int var = 0;
        long int cmp;

        if (var_str != NULL)
        {
            rc = te_strtol(var_str, 0, &var);
            if (rc != 0)
                goto out;
        }

        rc = te_strtol(cmp_str, 0, &cmp);
        if (rc != 0)
            goto out;

        if (strcmp(op_str, ">") == 0)
            *condp = (var > cmp) ? TRUE : FALSE;
        else if (strcmp(op_str, ">=") == 0)
            *condp = (var >= cmp) ? TRUE : FALSE;
        else if (strcmp(op_str, "<") == 0)
            *condp = (var < cmp) ? TRUE : FALSE;
        else if (strcmp(op_str, "<=") == 0)
            *condp = (var <= cmp) ? TRUE : FALSE;
        else
            rc = TE_EINVAL;
    }

out:
    free(text_copy);

    return rc;
}

/**
 * Process a condition property of the given parent node.
 *
 * @param d     YAML document handle
 * @param n     Handle of the parent node in the given document
 * @param condp Location for the result or @c NULL
 *
 * @return Status code.
 */
static te_errno
parse_config_yaml_cond(yaml_document_t *d,
                       yaml_node_t     *n,
                       te_bool         *condp)
{
    const char *str = NULL;
    te_bool     cond;
    te_errno    rc = 0;

    if (n->type == YAML_SCALAR_NODE)
    {
        str = (const char *)n->data.scalar.value;

        if (n->data.scalar.length == 0)
        {
            ERROR(CS_YAML_ERR_PREFIX "found the condition node to be "
                  "badly formatted");
            return TE_EINVAL;
        }

        rc = parse_config_yaml_cond_exp(str, &cond);
        if (rc != 0)
        {
            ERROR(CS_YAML_ERR_PREFIX "failed to evaluate the expression "
                  "contained in the condition node");
            return rc;
        }
    }
    else if (n->type == YAML_SEQUENCE_NODE)
    {
        yaml_node_item_t *item = n->data.sequence.items.start;

        cond = TRUE;

        do {
            yaml_node_t *in = yaml_document_get_node(d, *item);
            te_bool      item_cond;

            str = (const char *)in->data.scalar.value;

            if (in->type != YAML_SCALAR_NODE || in->data.scalar.length == 0)
            {
                 ERROR(CS_YAML_ERR_PREFIX "found the condition node to be "
                      "badly formatted");
                 rc = TE_EINVAL;
            }
            else if ((rc = parse_config_yaml_cond_exp(str, &item_cond)) != 0)
            {
                ERROR(CS_YAML_ERR_PREFIX "failed to evaluate the expression "
                      "contained in the condition node");
                rc = TE_EINVAL;
            }

            if (rc != 0)
            {
                ERROR(CS_YAML_ERR_PREFIX "detected some error(s) in the "
                      "condition node at line %lu column %lu",
                      in->start_mark.line, in->start_mark.column);
                return rc;
            }

            /*
             * Once set to FALSE, cond will never become TRUE (AND behaviour).
             * Still, the rest of conditional expressions will be parsed
             * in order to rule out configuration file syntax errors.
             */
            cond = (item_cond) ? cond : FALSE;
        } while (++item < n->data.sequence.items.top);
    }
    else
    {
        ERROR(CS_YAML_ERR_PREFIX "found the condition node to be "
              "badly formatted");
        return TE_EINVAL;
    }

    if (condp != NULL)
        *condp = cond;

    return 0;
}

typedef enum cs_yaml_node_attribute_type_e {
    CS_YAML_NODE_ATTRIBUTE_CONDITION = 0,
    CS_YAML_NODE_ATTRIBUTE_OID,
    CS_YAML_NODE_ATTRIBUTE_VALUE,
    CS_YAML_NODE_ATTRIBUTE_ACCESS,
    CS_YAML_NODE_ATTRIBUTE_TYPE,
    CS_YAML_NODE_ATTRIBUTE_VOLATILE,
    CS_YAML_NODE_ATTRIBUTE_UNKNOWN,
} cs_yaml_node_attribute_type_t;

static struct {
    const char                    *long_label;
    const char                    *short_label;
    cs_yaml_node_attribute_type_t  type;
} const cs_yaml_node_attributes[] = {
    { "cond",     "c",   CS_YAML_NODE_ATTRIBUTE_CONDITION },
    { "oid",      "o",   CS_YAML_NODE_ATTRIBUTE_OID },
    { "value",    "v",   CS_YAML_NODE_ATTRIBUTE_VALUE },
    { "access",   "a",   CS_YAML_NODE_ATTRIBUTE_ACCESS },
    { "type",     "t",   CS_YAML_NODE_ATTRIBUTE_TYPE },
    { "volatile", "vol", CS_YAML_NODE_ATTRIBUTE_VOLATILE },
};

static cs_yaml_node_attribute_type_t
parse_config_yaml_node_get_attribute_type(yaml_node_t *k)
{
    const char   *k_label = (const char *)k->data.scalar.value;
    unsigned int  i;

    for (i = 0; i < TE_ARRAY_LEN(cs_yaml_node_attributes); ++i)
    {
        if (strcasecmp(k_label, cs_yaml_node_attributes[i].long_label) == 0 ||
            strcasecmp(k_label, cs_yaml_node_attributes[i].short_label) == 0)
            return cs_yaml_node_attributes[i].type;
    }

    return CS_YAML_NODE_ATTRIBUTE_UNKNOWN;
}

typedef struct cs_yaml_target_context_s {
    const xmlChar *oid;
    const xmlChar *value;
    const xmlChar *access;
    const xmlChar *type;
    const xmlChar *xmlvolatile;
    te_bool        check_cond;
    te_bool        cond;
} cs_yaml_target_context_t;

static te_errno
parse_config_yaml_cmd_add_target_attribute(yaml_document_t            *d,
                                           yaml_node_t                *k,
                                           yaml_node_t                *v,
                                           cs_yaml_target_context_t   *c)
{
    cs_yaml_node_attribute_type_t attribute_type;
    te_errno                      rc = 0;

    if (k->type != YAML_SCALAR_NODE || k->data.scalar.length == 0 ||
        (v->type != YAML_SCALAR_NODE && v->type != YAML_SEQUENCE_NODE))
    {
        ERROR(CS_YAML_ERR_PREFIX "found the target attribute node to be "
              "badly formatted");
        return TE_EINVAL;
    }

    attribute_type = parse_config_yaml_node_get_attribute_type(k);
    switch (attribute_type)
    {
        case CS_YAML_NODE_ATTRIBUTE_CONDITION:
            rc = parse_config_yaml_cond(d, v,
                                        (c->check_cond) ? &c->cond : NULL);
            if (rc != 0)
            {
                ERROR(CS_YAML_ERR_PREFIX "failed to process the condition "
                      "attribute node of the target");
                return rc;
            }

            /*
             * Once at least one conditional node (which itself may contain
             * multiple statements) is found to yield TRUE, this result
             * will never be overridden by the rest of conditional
             * nodes of the instance in question (OR behaviour).
             * Still, the rest of the nodes will be parsed.
             */
             if (c->cond == TRUE)
                 c->check_cond = FALSE;
            break;

        case CS_YAML_NODE_ATTRIBUTE_OID:
            if (c->oid != NULL)
            {
                ERROR(CS_YAML_ERR_PREFIX "detected multiple OID specifiers "
                      "of the target: only one can be present");
                return TE_EINVAL;
            }

            c->oid = (const xmlChar *)v->data.scalar.value;
            break;

        case CS_YAML_NODE_ATTRIBUTE_VALUE:
            if (c->value != NULL)
            {
                ERROR(CS_YAML_ERR_PREFIX "detected multiple value specifiers "
                      "of the target: only one can be present");
                return TE_EINVAL;
            }

            c->value = (const xmlChar *)v->data.scalar.value;
            break;

        case CS_YAML_NODE_ATTRIBUTE_ACCESS:
            if (c->access != NULL)
            {
                ERROR(CS_YAML_ERR_PREFIX "detected multiple access specifiers "
                      "of the target: only one can be present");
                return TE_EINVAL;
            }

            c->access = (const xmlChar *)v->data.scalar.value;
            break;

        case CS_YAML_NODE_ATTRIBUTE_TYPE:
            if (c->type != NULL)
            {
                ERROR(CS_YAML_ERR_PREFIX "detected multiple type specifiers "
                      "of the target: only one can be present");
                return TE_EINVAL;
            }

            c->type = (const xmlChar *)v->data.scalar.value;
            break;

        case CS_YAML_NODE_ATTRIBUTE_VOLATILE:
            if (c->xmlvolatile != NULL)
            {
                ERROR(CS_YAML_ERR_PREFIX "detected multiple volatile specifiers "
                      "of the target: only one can be present");
                return TE_EINVAL;
            }

            c->xmlvolatile = (const xmlChar *)v->data.scalar.value;
            break;

        default:
            if (v->type == YAML_SCALAR_NODE && v->data.scalar.length == 0)
            {
                c->oid = (const xmlChar *)k->data.scalar.value;
            }
            else
            {
                ERROR(CS_YAML_ERR_PREFIX "failed to recognise the "
                      "attribute type in the target");
                return TE_EINVAL;
            }
            break;
    }

    return 0;
}

static te_errno
embed_yaml_target_in_xml(xmlNodePtr xn_cmd, xmlNodePtr xn_target,
                         cs_yaml_target_context_t *c)
{
    const xmlChar *prop_name_oid = (const xmlChar *)"oid";
    const xmlChar *prop_name_value = (const xmlChar *)"value";
    const xmlChar *prop_name_access = (const xmlChar *)"access";
    const xmlChar *prop_name_type = (const xmlChar *)"type";

    if (c->oid == NULL)
    {
        ERROR(CS_YAML_ERR_PREFIX "failed to find target OID specifier");
        return TE_EINVAL;
    }

    if (!c->cond)
        return 0;

    if (xmlNewProp(xn_target, prop_name_oid, c->oid) == NULL)
    {
        ERROR(CS_YAML_ERR_PREFIX "failed to set OID for the target"
              "node in XML output");
        return TE_ENOMEM;
    }

    if (c->value != NULL &&
        xmlNewProp(xn_target, prop_name_value, c->value) == NULL)
    {
        ERROR(CS_YAML_ERR_PREFIX "failed to embed the target value "
              "attribute in XML output");
        return TE_ENOMEM;
    }

    if (c->access != NULL &&
        xmlNewProp(xn_target, prop_name_access, c->access) == NULL)
    {
        ERROR(CS_YAML_ERR_PREFIX "failed to embed the target access"
              "attribute in XML output");
        return TE_ENOMEM;
    }

    if (c->type != NULL &&
        xmlNewProp(xn_target, prop_name_type, c->type) == NULL)
    {
        ERROR(CS_YAML_ERR_PREFIX "failed to embed the target type "
              "attribute in XML output");
        return TE_ENOMEM;
    }

    if (c->xmlvolatile != NULL &&
        xmlNewProp(xn_target, prop_name_type, c->xmlvolatile) == NULL)
    {
        ERROR(CS_YAML_ERR_PREFIX "failed to embed the target volatile "
              "attribute in XML output");
        return TE_ENOMEM;
    }

    if (xmlAddChild(xn_cmd, xn_target) == xn_target)
    {
        return 0;
    }
    else
    {
        ERROR(CS_YAML_ERR_PREFIX "failed to embed the target in "
              "XML output");
        return TE_EINVAL;
    }
}

/**
 * Process the given target node in the given YAML document.
 *
 * @param d      YAML document handle
 * @param n      Handle of the target node in the given YAML document
 * @param xn_cmd Handle of command node in the XML document being created
 * @param cmd    String representation of command
 *
 * @return Status code.
 */
static te_errno
parse_config_yaml_cmd_process_target(yaml_document_t *d,
                                     yaml_node_t     *n,
                                     xmlNodePtr       xn_cmd,
                                     const char      *cmd)
{
    xmlNodePtr                  xn_target = NULL;
    cs_yaml_target_context_t    c = YAML_TARGET_CONTEXT_INIT;
    const char                 *target;
    te_errno                    rc = 0;

    target = get_yaml_cmd_target(cmd);
    if (target == NULL)
        return TE_EINVAL;

    xn_target = xmlNewNode(NULL, BAD_CAST target);
    if (xn_target == NULL)
    {
        ERROR(CS_YAML_ERR_PREFIX "failed to allocate %s"
              "node for XML output", target);
        return TE_ENOMEM;
    }

    if (n->type == YAML_SCALAR_NODE)
    {
        if (n->data.scalar.length == 0)
        {
            ERROR(CS_YAML_ERR_PREFIX "found the %s node to be "
                  "badly formatted", target);
            rc = TE_EINVAL;
            goto out;
        }

        c.oid = (const xmlChar *)n->data.scalar.value;
    }
    else if (n->type == YAML_MAPPING_NODE)
    {
        yaml_node_pair_t *pair = n->data.mapping.pairs.start;

        do {
            yaml_node_t *k = yaml_document_get_node(d, pair->key);
            yaml_node_t *v = yaml_document_get_node(d, pair->value);

            rc = parse_config_yaml_cmd_add_target_attribute(d, k, v, &c);
            if (rc != 0)
            {
                ERROR(CS_YAML_ERR_PREFIX "failed to process %s"
                      "attribute at line %lu column %lu",
                      target, k->start_mark.line, k->start_mark.column);
                goto out;
            }
        } while (++pair < n->data.mapping.pairs.top);
    }
    else
    {
        ERROR(CS_YAML_ERR_PREFIX "found the %s node to be "
              "badly formatted", target);
        rc = TE_EINVAL;
        goto out;
    }

    rc = embed_yaml_target_in_xml(xn_cmd, xn_target, &c);
    if (rc != 0)
        goto out;
    return 0;

out:
    xmlFreeNode(xn_target);

    return rc;
}

/**
 * Process the sequence of target nodes for the specified
 * command in the given YAML document.
 *
 * @param d      YAML document handle
 * @param n      Handle of the target sequence in the given YAML document
 * @param xn_cmd Handle of command node in the XML document being created
 * @param cmd    String representation of command
 *
 * @return Status code.
 */
static te_errno
parse_config_yaml_cmd_process_targets(yaml_document_t *d,
                                      yaml_node_t     *n,
                                      xmlNodePtr       xn_cmd,
                                      const char      *cmd)
{
    yaml_node_item_t *item = n->data.sequence.items.start;

    if (n->type != YAML_SEQUENCE_NODE)
    {
        ERROR(CS_YAML_ERR_PREFIX "found the %s command's list of targets "
              "to be badly formatted", cmd);
        return TE_EINVAL;
    }

    do {
        yaml_node_t *in = yaml_document_get_node(d, *item);
        te_errno     rc = 0;

        rc = parse_config_yaml_cmd_process_target(d, in, xn_cmd, cmd);
        if (rc != 0)
        {
            ERROR(CS_YAML_ERR_PREFIX "failed to process the target in the "
                  "%s command's list at line %lu column %lu",
                  cmd, in->start_mark.line, in->start_mark.column);
            return rc;
        }
    } while (++item < n->data.sequence.items.top);

    return 0;
}

/**
 * Process dynamic history specified command in the given YAML document.
 *
 * @param d          YAML document handle
 * @param n          Handle of the command node in the given YAML document
 * @param xn_history Handle of "history" node in the XML document being created
 * @param cmd        String representation of command
 *
 * @return Status code.
 */
static te_errno
parse_config_yaml_specified_cmd(yaml_document_t *d,
                                yaml_node_t     *n,
                                xmlNodePtr       xn_history,
                                const char      *cmd)
{
    xmlNodePtr  xn_cmd = NULL;
    te_bool     check_cond = TRUE;
    te_bool     cond = TRUE;
    te_errno    rc = 0;
    const char *targets;

    xn_cmd = xmlNewNode(NULL, BAD_CAST cmd);
    if (xn_cmd == NULL)
    {
        ERROR(CS_YAML_ERR_PREFIX "failed to allocate %s command "
              "node for XML output", cmd);
        return TE_ENOMEM;
    }

    targets = get_yaml_cmd_targets(cmd);
    if (targets == NULL)
    {
        ERROR(CS_YAML_ERR_PREFIX "failed to determine %s command target", cmd);
        return TE_EINVAL;
    }

    if (n->type == YAML_MAPPING_NODE)
    {
        yaml_node_pair_t *pair = n->data.mapping.pairs.start;

        do {
            yaml_node_t *k = yaml_document_get_node(d, pair->key);
            yaml_node_t *v = yaml_document_get_node(d, pair->value);
            const char  *k_label = (const char *)k->data.scalar.value;

            if (k->type != YAML_SCALAR_NODE || k->data.scalar.length == 0)
            {
                ERROR(CS_YAML_ERR_PREFIX "found the node nested in the "
                      "%s command to be badly formatted", cmd);
                rc = TE_EINVAL;
            }
            else if (parse_config_yaml_node_get_attribute_type(k) ==
                     CS_YAML_NODE_ATTRIBUTE_CONDITION)
            {
                rc = parse_config_yaml_cond(d, v, (check_cond) ? &cond : NULL);

                /*
                 * Once at least one conditional node (which itself may contain
                 * multiple statements) is found to yield TRUE, this result
                 * will never be overridden by the rest of conditional
                 * nodes of the current add command (OR behaviour).
                 * Still, the rest of the nodes will be parsed.
                 */
                if (rc == 0)
                {
                    if (cond == TRUE)
                        check_cond = FALSE;
                }
            }
            else if (strcmp(k_label, targets) == 0)
            {
                rc = parse_config_yaml_cmd_process_targets(d, v, xn_cmd, cmd);
            }
            else
            {
                ERROR(CS_YAML_ERR_PREFIX "failed to recognise %s "
                      "command's child", cmd);
                rc = TE_EINVAL;
            }

            if (rc != 0)
            {
                ERROR(CS_YAML_ERR_PREFIX "detected some error(s) in the %s "
                      "command's nested node at line %lu column %lu",
                      cmd, k->start_mark.line, k->start_mark.column);
                goto out;
            }
        } while (++pair < n->data.mapping.pairs.top);
    }
    else
    {
        ERROR(CS_YAML_ERR_PREFIX "found the %s command node to be "
              "badly formatted", cmd);
        rc = TE_EINVAL;
        goto out;
    }

    if (cond == TRUE && xn_cmd->children != NULL)
    {
        if (xmlAddChild(xn_history, xn_cmd) == xn_cmd)
        {
            return 0;
        }
        else
        {
            ERROR(CS_YAML_ERR_PREFIX "failed to embed %s command "
                  "to XML output", cmd);
            rc = TE_EINVAL;
        }
    }

out:
    xmlFreeNode(xn_cmd);

    return rc;
}
/**
 * Explore keys and values in the root node of the given YAML
 * document to detect and process dynamic history commands.
 *
 * @param d          YAML document handle
 * @param xn_history Handle of "history" node in the document being created
 *
 * @return Status code.
 */
static te_errno
parse_config_yaml_cmd(yaml_document_t *d,
                      xmlNodePtr       xn_history)
{
    yaml_node_t      *root = NULL;
    yaml_node_pair_t *pair = NULL;
    te_errno          rc = 0;

    root = yaml_document_get_root_node(d);
    if (root == NULL)
    {
        ERROR(CS_YAML_ERR_PREFIX "failed to get the root node");
        return TE_EINVAL;
    }

    pair = root->data.mapping.pairs.start;
    do {
        yaml_node_t *k = yaml_document_get_node(d, pair->key);
        yaml_node_t *v = yaml_document_get_node(d, pair->value);

        if (k->type != YAML_SCALAR_NODE || k->data.scalar.length == 0)
        {
            ERROR(CS_YAML_ERR_PREFIX "found the command node to be "
                  "badly formatted");
            rc = TE_EINVAL;
        }
        else if ((strcmp((const char *)k->data.scalar.value, "add") == 0) ||
                 (strcmp((const char *)k->data.scalar.value, "set") == 0) ||
                 (strcmp((const char *)k->data.scalar.value, "register") == 0) ||
                 (strcmp((const char *)k->data.scalar.value, "unregister") == 0) ||
                 (strcmp((const char *)k->data.scalar.value, "delete") == 0))
        {
            rc = parse_config_yaml_specified_cmd(d, v, xn_history,
                                           (const char *)k->data.scalar.value);
        }
        else
        {
            ERROR(CS_YAML_ERR_PREFIX "failed to recognise the command");
            rc = TE_EINVAL;
        }

        if (rc != 0)
        {
            ERROR(CS_YAML_ERR_PREFIX "detected some error(s) in "
                  "the command node at line %lu column %lu",
                  k->start_mark.line, k->start_mark.column);
            break;
        }
    } while (++pair < root->data.mapping.pairs.top);

    return rc;
}

/* See description in 'conf_yaml.h' */
te_errno
parse_config_yaml(const char *filename)
{
    FILE            *f = NULL;
    yaml_parser_t    parser;
    yaml_document_t  dy;
    xmlNodePtr       xn_history = NULL;
    te_errno         rc = 0;

    f = fopen(filename, "rb");
    if (f == NULL)
    {
        ERROR(CS_YAML_ERR_PREFIX "failed to open the target file");
        return TE_OS_RC(TE_CS, errno);
    }

    yaml_parser_initialize(&parser);
    yaml_parser_set_input_file(&parser, f);
    yaml_parser_load(&parser, &dy);
    fclose(f);

    xn_history = xmlNewNode(NULL, BAD_CAST "history");
    if (xn_history == NULL)
    {
        ERROR(CS_YAML_ERR_PREFIX "failed to allocate "
              "main history node for XML output");
        rc = TE_ENOMEM;
        goto out;
    }

    rc = parse_config_yaml_cmd(&dy, xn_history);
    if (rc != 0)
    {
        ERROR(CS_YAML_ERR_PREFIX "encountered some error(s)");
        goto out;
    }

    if (xn_history->children != NULL)
    {
        rcf_log_cfg_changes(TRUE);
        rc = parse_config_dh_sync(xn_history, NULL);
        rcf_log_cfg_changes(FALSE);
    }

out:
    xmlFreeNode(xn_history);
    yaml_document_delete(&dy);
    yaml_parser_delete(&parser);

    return rc;
}

