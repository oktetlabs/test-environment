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
#include "logic_expr.h"
#include "te_alloc.h"
#include "te_expand.h"

#include <ctype.h>
#include <libxml/xinclude.h>
#include <yaml.h>

#define CS_YAML_ERR_PREFIX "YAML configuration file parser "

#define YAML_TARGET_CONTEXT_INIT \
    { NULL, NULL, NULL, NULL, NULL, NULL, NULL, TRUE };

typedef struct config_yaml_target_s {
    const char *command_name;
    const char *target_name;
} config_yaml_target_t;

static const config_yaml_target_t config_yaml_targets[] = {
    { "add", "instance" },
    { "set", "instance" },
    { "delete", "instance" },
    { "register", "object" },
    { "unregister", "object" },
    { NULL, NULL}
};

static const char *
get_yaml_cmd_target(const char *cmd)
{
    const config_yaml_target_t *target = config_yaml_targets;

    for (; target->command_name != NULL; ++target)
    {
        if (strcmp(cmd, target->command_name) == 0)
            break;
    }

    return target->target_name;
}

static te_errno
get_val(const logic_expr *parsed, void *expand_vars, logic_expr_res *res)
{
    te_errno rc;
    int rc_errno;

    if (expand_vars != NULL)
        rc_errno = te_expand_kvpairs(parsed->u.value, NULL,
                                     (te_kvpair_h *)expand_vars,
                                     &res->value.simple);
    else
        rc_errno = te_expand_env_vars(parsed->u.value, NULL,
                                      &res->value.simple);
    if (rc_errno != 0)
    {
        rc = te_rc_os2te(rc_errno);
        goto out;
    }
    else
    {
        rc = 0;
    }
    res->res_type = LOGIC_EXPR_RES_SIMPLE;

out:
    return rc;
}

/**
 * Evaluate logical expression.
 *
 * @param str               String representation of the expression
 * @param res               Location for the result
 * @param expand_vars       List of key-value pairs for expansion in file,
 *                          @c NULL if environment variables are used for
 *                          substitutions
 *
 * @return Status code.
 */
static te_errno
parse_logic_expr_str(const char *str, te_bool *res, te_kvpair_h *expand_vars)
{
    logic_expr *parsed;
    logic_expr_res parsed_res;
    te_errno rc;

    rc = logic_expr_parse(str, &parsed);
    if (rc != 0)
    {
        ERROR("Failed to parse expression '%s'", str);
        goto out;
    }

    rc = logic_expr_eval(parsed, get_val, expand_vars, &parsed_res);
    if (rc != 0)
    {
        ERROR("Failed to evaluate expression '%s'", str);
        goto out;
    }

    if (parsed_res.res_type != LOGIC_EXPR_RES_BOOLEAN)
    {
        rc = TE_EINVAL;
        logic_expr_free_res(&parsed_res);
        goto out;
    }

    *res = parsed_res.value.boolean;

out:
    logic_expr_free(parsed);

    return rc;
}

static te_errno
parse_config_if_expr(yaml_node_t *n, te_bool *if_expr, te_kvpair_h *expand_vars)
{
    const char *str = NULL;
    te_errno    rc = 0;

    if (n->type == YAML_SCALAR_NODE)
    {
        str = (const char *)n->data.scalar.value;

        if (n->data.scalar.length == 0)
        {
            ERROR(CS_YAML_ERR_PREFIX "found the if-expression node to be "
                  "badly formatted");
            return TE_EINVAL;
        }

        rc = parse_logic_expr_str(str, if_expr, expand_vars);
        if (rc != 0)
        {
            ERROR(CS_YAML_ERR_PREFIX "failed to evaluate the expression "
                  "contained in the condition node");
            return rc;
        }
    }
    else
    {
        ERROR(CS_YAML_ERR_PREFIX "found the if-expression node to be "
              "badly formatted");
        return TE_EINVAL;
    }

    return rc;
}

typedef enum cs_yaml_node_attribute_type_e {
    CS_YAML_NODE_ATTRIBUTE_CONDITION = 0,
    CS_YAML_NODE_ATTRIBUTE_OID,
    CS_YAML_NODE_ATTRIBUTE_VALUE,
    CS_YAML_NODE_ATTRIBUTE_ACCESS,
    CS_YAML_NODE_ATTRIBUTE_TYPE,
    CS_YAML_NODE_ATTRIBUTE_VOLATILE,
    CS_YAML_NODE_ATTRIBUTE_DEPENDENCE,
    CS_YAML_NODE_ATTRIBUTE_SCOPE,
    CS_YAML_NODE_ATTRIBUTE_UNKNOWN,
} cs_yaml_node_attribute_type_t;

static struct {
    const char                    *long_label;
    const char                    *short_label;
    cs_yaml_node_attribute_type_t  type;
} const cs_yaml_node_attributes[] = {
    { "if",       "if",  CS_YAML_NODE_ATTRIBUTE_CONDITION },
    { "oid",      "o",   CS_YAML_NODE_ATTRIBUTE_OID },
    { "value",    "v",   CS_YAML_NODE_ATTRIBUTE_VALUE },
    { "access",   "a",   CS_YAML_NODE_ATTRIBUTE_ACCESS },
    { "type",     "t",   CS_YAML_NODE_ATTRIBUTE_TYPE },
    { "volatile", "vol", CS_YAML_NODE_ATTRIBUTE_VOLATILE },
    { "depends",  "d",   CS_YAML_NODE_ATTRIBUTE_DEPENDENCE },
    { "scope",    "s",   CS_YAML_NODE_ATTRIBUTE_SCOPE }
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
    const xmlChar *dependence_oid;
    const xmlChar *scope;
    te_bool        cond;
} cs_yaml_target_context_t;

static te_errno
parse_config_yaml_cmd_add_dependence_attribute(yaml_node_t                *k,
                                               yaml_node_t                *v,
                                               cs_yaml_target_context_t   *c)
{
    cs_yaml_node_attribute_type_t attribute_type;

    if (k->type != YAML_SCALAR_NODE || k->data.scalar.length == 0 ||
        (v->type != YAML_SCALAR_NODE && v->type != YAML_SEQUENCE_NODE))
    {
        ERROR(CS_YAML_ERR_PREFIX "found the dependce attribute node to be "
              "badly formatted");
        return TE_EINVAL;
    }

    attribute_type = parse_config_yaml_node_get_attribute_type(k);
    switch (attribute_type)
    {
        case CS_YAML_NODE_ATTRIBUTE_OID:
            if (c->dependence_oid != NULL)
            {
                ERROR(CS_YAML_ERR_PREFIX "detected multiple OID specifiers "
                      "of the dependce node: only one can be present");
                return TE_EINVAL;
            }

            c->dependence_oid  = (const xmlChar *)v->data.scalar.value;
            break;

        case CS_YAML_NODE_ATTRIBUTE_SCOPE:
            if (c->scope != NULL)
            {
                ERROR(CS_YAML_ERR_PREFIX "detected multiple scope specifiers "
                      "of the dependce node: only one can be present");
                return TE_EINVAL;
            }

            c->scope = (const xmlChar *)v->data.scalar.value;
            break;

        default:
            if (v->type == YAML_SCALAR_NODE && v->data.scalar.length == 0)
            {
                c->dependence_oid = (const xmlChar *)k->data.scalar.value;
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

/**
 * Process a dependence node of the given parent node.
 *
 * @param d     YAML document handle
 * @param n     Handle of the parent node in the given document
 * @param c     Location for the result
 *
 * @return Status code.
 */
static te_errno
parse_config_yaml_dependence(yaml_document_t            *d,
                             yaml_node_t                *n,
                             cs_yaml_target_context_t   *c)
{
    te_errno rc;

    if (n->type == YAML_SCALAR_NODE)
    {
        if (n->data.scalar.length == 0)
        {
            ERROR(CS_YAML_ERR_PREFIX "found the dependence node to be "
                  "badly formatted");
            return TE_EINVAL;
        }

        c->dependence_oid = (const xmlChar *)n->data.scalar.value;
    }
    else if (n->type == YAML_SEQUENCE_NODE)
    {
        yaml_node_item_t *item = n->data.sequence.items.start;
        yaml_node_t *in = yaml_document_get_node(d, *item);

        if (in->type == YAML_MAPPING_NODE)
        {
            yaml_node_pair_t *pair = in->data.mapping.pairs.start;

            do {
                yaml_node_t *k = yaml_document_get_node(d, pair->key);
                yaml_node_t *v = yaml_document_get_node(d, pair->value);

                rc = parse_config_yaml_cmd_add_dependence_attribute(k, v, c);
                if (rc != 0)
                {
                    ERROR(CS_YAML_ERR_PREFIX "failed to process "
                          "attribute at line %lu column %lu",
                          k->start_mark.line, k->start_mark.column);
                    return TE_EINVAL;
                }
            } while (++pair < in->data.mapping.pairs.top);
        }
        else
        {
            ERROR(CS_YAML_ERR_PREFIX "found the dependence node to be "
                  "badly formatted");
            return TE_EINVAL;
        }
    }
    else
    {
        ERROR(CS_YAML_ERR_PREFIX "found the dependce node to be "
              "badly formatted");
        return TE_EINVAL;
    }

    return 0;
}

static te_errno
parse_config_yaml_cmd_add_target_attribute(yaml_document_t        *d,
                                       yaml_node_t                *k,
                                       yaml_node_t                *v,
                                       cs_yaml_target_context_t   *c,
                                       te_kvpair_h                *expand_vars)
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
            rc = parse_config_if_expr(v, &c->cond, expand_vars);
            if (rc != 0)
            {
              ERROR(CS_YAML_ERR_PREFIX "failed to process the condition "
                    "attribute node of the target");
              return rc;
            }
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

        case CS_YAML_NODE_ATTRIBUTE_DEPENDENCE:
            rc = parse_config_yaml_dependence(d, v, c);
            if (rc != 0)
            {
                ERROR(CS_YAML_ERR_PREFIX "failed to process the dependce "
                      "node of the object");
                return rc;
            }
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
    const xmlChar *prop_name_scope = (const xmlChar *)"scope";
    xmlNodePtr     dependence_node;

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

    if (c->dependence_oid != NULL)
    {
        dependence_node = xmlNewNode(NULL, BAD_CAST "depends");
        if (dependence_node == NULL)
        {
            ERROR(CS_YAML_ERR_PREFIX "failed to allocate dependence "
                  "node for XML output");
            return TE_ENOMEM;
        }

        if (xmlNewProp(dependence_node, prop_name_oid, c->dependence_oid) ==
            NULL)
        {
            ERROR(CS_YAML_ERR_PREFIX "failed to set OID for the dependence "
                  "node in XML output");
            return TE_ENOMEM;
        }

        if (c->scope != NULL &&
            xmlNewProp(dependence_node, prop_name_scope, c->scope) == NULL)
        {
            ERROR(CS_YAML_ERR_PREFIX "failed to embed the target scope "
                  "attribute in XML output");
            return TE_ENOMEM;
        }

        if (xmlAddChild(xn_target, dependence_node) != dependence_node)
        {
            ERROR(CS_YAML_ERR_PREFIX "failed to embed dependence node in "
                  "XML output");
            return TE_EINVAL;
        }
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
 * @param d                 YAML document handle
 * @param n                 Handle of the target node in the given YAML document
 * @param xn_cmd            Handle of command node in the XML document being
 *                          created
 * @param cmd               String representation of command
 * @param expand_vars       List of key-value pairs for expansion in file,
 *                          @c NULL if environment variables are used for
 *                          substitutions
 *
 * @return Status code.
 */
static te_errno
parse_config_yaml_cmd_process_target(yaml_document_t *d,
                                     yaml_node_t     *n,
                                     xmlNodePtr       xn_cmd,
                                     const char      *cmd,
                                     te_kvpair_h     *expand_vars)
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

            rc = parse_config_yaml_cmd_add_target_attribute(d, k, v, &c,
                                                            expand_vars);
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
 * @param d                 YAML document handle
 * @param n                 Handle of the target sequence in the given YAML
 *                          document
 * @param xn_cmd            Handle of command node in the XML document being
 *                          created
 * @param cmd               String representation of command
 * @param expand_vars       List of key-value pairs for expansion in file,
 *                          @c NULL if environment variables are used for
 *                          substitutions
 *
 * @return Status code.
 */
static te_errno
parse_config_yaml_cmd_process_targets(yaml_document_t *d,
                                      yaml_node_t     *n,
                                      xmlNodePtr       xn_cmd,
                                      const char      *cmd,
                                      te_kvpair_h     *expand_vars)
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

        rc = parse_config_yaml_cmd_process_target(d, in, xn_cmd, cmd,
                                                  expand_vars);
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

static te_errno parse_config_yaml_cmd(yaml_document_t *d,
                                      xmlNodePtr       xn_history,
                                      yaml_node_t     *parent,
                                      te_kvpair_h     *expand_vars);

/**
 * Process dynamic history specified command in the given YAML document.
 *
 * @param d                 YAML document handle
 * @param n                 Handle of the command node in the given YAML document
 * @param xn_history        Handle of "history" node in the XML document being
 *                          created
 * @param cmd               String representation of command
 * @param expand_vars       List of key-value pairs for expansion in file,
 *                          @c NULL if environment variables are used for
 *                          substitutions
 *
 * @return Status code.
 */
static te_errno
parse_config_yaml_specified_cmd(yaml_document_t *d,
                                yaml_node_t     *n,
                                xmlNodePtr       xn_history,
                                const char      *cmd,
                                te_kvpair_h     *expand_vars)
{
    xmlNodePtr  xn_cmd = NULL;
    te_errno    rc = 0;
    te_bool     cond = FALSE;

    xn_cmd = xmlNewNode(NULL, BAD_CAST cmd);
    if (xn_cmd == NULL)
    {
        ERROR(CS_YAML_ERR_PREFIX "failed to allocate %s command "
              "node for XML output", cmd);
        return TE_ENOMEM;
    }

    if (n->type == YAML_SEQUENCE_NODE)
    {
        if (strcmp(cmd, "cond") == 0)
        {
            ERROR(CS_YAML_ERR_PREFIX "found the %s command node to be "
                  "badly formatted", cmd);
            rc = TE_EINVAL;
            goto out;
        }

        rc = parse_config_yaml_cmd_process_targets(d, n, xn_cmd, cmd,
                                                   expand_vars);
        if (rc != 0)
        {
            ERROR(CS_YAML_ERR_PREFIX "detected some error(s) in the %s "
                  "command's nested node at line %lu column %lu",
                  cmd, n->start_mark.line, n->start_mark.column);
            goto out;
        }
    }
    else if (n->type == YAML_MAPPING_NODE)
    {
        if (strcmp(cmd, "cond") != 0)
        {
            ERROR(CS_YAML_ERR_PREFIX "found the %s command node to be "
                  "badly formatted", cmd);
            rc = TE_EINVAL;
            goto out;
        }

        yaml_node_pair_t *pair = n->data.mapping.pairs.start;
        do {
            yaml_node_t *k = yaml_document_get_node(d, pair->key);
            yaml_node_t *v = yaml_document_get_node(d, pair->value);
            const char  *k_label = (const char *)k->data.scalar.value;

            if (strcmp(k_label, "if") == 0)
            {
                rc = parse_config_if_expr(v, &cond, expand_vars);
            }
            else if (strcmp(k_label, "then") == 0)
            {
                if (cond)
                    rc = parse_config_yaml_cmd(d, xn_history, v, expand_vars);
            }
            else if (strcmp(k_label, "else") == 0)
            {
                if (!cond)
                    rc = parse_config_yaml_cmd(d, xn_history, v, expand_vars);
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

    if (xn_cmd->children != NULL)
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

static te_errno
parse_config_root_commands(yaml_document_t *d,
                           xmlNodePtr       xn_history,
                           yaml_node_t     *n,
                           te_kvpair_h     *expand_vars)
{
    yaml_node_pair_t *pair = n->data.mapping.pairs.start;
    yaml_node_t *k = yaml_document_get_node(d, pair->key);
    yaml_node_t *v = yaml_document_get_node(d, pair->value);
    te_errno rc = 0;

    if ((strcmp((const char *)k->data.scalar.value, "add") == 0) ||
        (strcmp((const char *)k->data.scalar.value, "set") == 0) ||
        (strcmp((const char *)k->data.scalar.value, "register") == 0) ||
        (strcmp((const char *)k->data.scalar.value, "unregister") == 0) ||
        (strcmp((const char *)k->data.scalar.value, "delete") == 0) ||
        (strcmp((const char *)k->data.scalar.value, "cond") == 0))
    {
        rc = parse_config_yaml_specified_cmd(d, v, xn_history,
                                       (const char *)k->data.scalar.value,
                                       expand_vars);
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
    }

    return rc;
}

/**
 * Explore sequence of commands of the given parent node in the given YAML
 * document to detect and process dynamic history commands.
 *
 * @param d                 YAML document handle
 * @param xn_history        Handle of "history" node in the document being
 *                          created
 * @param parent            Parent node of sequence of commands
 * @param expand_vars       List of key-value pairs for expansion in file,
 *                          @c NULL if environment variables are used for
 *                          substitutions
 *
 * @return Status code.
 */
static te_errno
parse_config_yaml_cmd(yaml_document_t *d,
                      xmlNodePtr       xn_history,
                      yaml_node_t     *parent,
                      te_kvpair_h     *expand_vars)
{
    yaml_node_item_t *item = NULL;
    te_errno          rc = 0;

    if (parent->type != YAML_SEQUENCE_NODE)
    {
        ERROR(CS_YAML_ERR_PREFIX "expected sequence node");
        return TE_EFMT;
    }

    item = parent->data.sequence.items.start;
    do {
        yaml_node_t *n = yaml_document_get_node(d, *item);

        if (n->type != YAML_MAPPING_NODE)
        {
            ERROR(CS_YAML_ERR_PREFIX "found the command node to be "
                  "badly formatted");
            rc = TE_EINVAL;
        }

        rc = parse_config_root_commands(d, xn_history, n, expand_vars);
        if (rc != 0)
            break;
    } while (++item < parent->data.sequence.items.top);

    return rc;
}

/* See description in 'conf_yaml.h' */
te_errno
parse_config_yaml(const char *filename, te_kvpair_h *expand_vars)
{
    FILE            *f = NULL;
    yaml_parser_t    parser;
    yaml_document_t  dy;
    xmlNodePtr       xn_history = NULL;
    yaml_node_t     *root = NULL;
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

    root = yaml_document_get_root_node(&dy);
    if (root == NULL)
    {
        ERROR(CS_YAML_ERR_PREFIX "failed to get the root node");
        return TE_EINVAL;
    }

    if (root->type == YAML_SCALAR_NODE &&
        strcmp(root->data.scalar.value, "") == 0)
    {
        INFO(CS_YAML_ERR_PREFIX "empty");
        rc = 0;
        goto out;
    }

    rc = parse_config_yaml_cmd(&dy, xn_history, root, expand_vars);
    if (rc != 0)
    {
        ERROR(CS_YAML_ERR_PREFIX "encountered some error(s)");
        goto out;
    }

    if (xn_history->children != NULL)
    {
        rcf_log_cfg_changes(TRUE);
        rc = parse_config_dh_sync(xn_history, expand_vars);
        rcf_log_cfg_changes(FALSE);
    }

out:
    xmlFreeNode(xn_history);
    yaml_document_delete(&dy);
    yaml_parser_delete(&parser);

    return rc;
}

