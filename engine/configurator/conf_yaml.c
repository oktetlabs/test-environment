/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief YAML configuration file processing facility
 *
 * Implementation.
 *
 * Copyright (C) 2018-2022 OKTET Labs Ltd. All rights reserved.
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
#include <libgen.h>
#include <yaml.h>

#define CS_YAML_ERR_PREFIX "YAML configuration file parser "

#define YAML_NODE_LINE_COLUMN_FMT   "line %lu column %lu"
#define YAML_NODE_LINE_COLUMN(_n)   \
    (_n)->start_mark.line + 1, (_n)->start_mark.column + 1

#define YAML_TARGET_CONTEXT_INIT \
    { NULL, NULL, NULL, NULL, NULL, NULL, SLIST_HEAD_INITIALIZER(deps), TRUE };

typedef struct parse_config_yaml_ctx {
    char            *file_path;
    yaml_document_t *doc;
    xmlNodePtr       xn_history;
    te_kvpair_h     *expand_vars;
} parse_config_yaml_ctx;

typedef struct config_yaml_target_s {
    const char *command_name;
    const char *target_name;
} config_yaml_target_t;

static const config_yaml_target_t config_yaml_targets[] = {
    { "add", "instance" },
    { "get", "instance" },
    { "set", "instance" },
    { "delete", "instance" },
    { "copy", "instance" },
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
    logic_expr *parsed = NULL;
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
    CS_YAML_NODE_ATTRIBUTE_DESCRIPTION,
    CS_YAML_NODE_ATTRIBUTE_SUBSTITUTION,
    CS_YAML_NODE_ATTRIBUTE_UNKNOWN,
} cs_yaml_node_attribute_type_t;

static struct {
    const char                    *label;
    cs_yaml_node_attribute_type_t  type;
} const cs_yaml_node_attributes[] = {
    { "if",       CS_YAML_NODE_ATTRIBUTE_CONDITION },
    { "oid",      CS_YAML_NODE_ATTRIBUTE_OID },
    { "value",    CS_YAML_NODE_ATTRIBUTE_VALUE },
    { "access",   CS_YAML_NODE_ATTRIBUTE_ACCESS },
    { "type",     CS_YAML_NODE_ATTRIBUTE_TYPE },
    { "volatile", CS_YAML_NODE_ATTRIBUTE_VOLATILE },
    { "depends",  CS_YAML_NODE_ATTRIBUTE_DEPENDENCE },
    { "scope",    CS_YAML_NODE_ATTRIBUTE_SCOPE },
    { "d",        CS_YAML_NODE_ATTRIBUTE_DESCRIPTION },
    { "substitution", CS_YAML_NODE_ATTRIBUTE_SUBSTITUTION },
};

static cs_yaml_node_attribute_type_t
parse_config_yaml_node_get_attribute_type(yaml_node_t *k)
{
    const char   *k_label = (const char *)k->data.scalar.value;
    unsigned int  i;

    for (i = 0; i < TE_ARRAY_LEN(cs_yaml_node_attributes); ++i)
    {
        if (strcasecmp(k_label, cs_yaml_node_attributes[i].label) == 0)
            return cs_yaml_node_attributes[i].type;
    }

    return CS_YAML_NODE_ATTRIBUTE_UNKNOWN;
}

typedef struct cytc_dep_entry {
    SLIST_ENTRY(cytc_dep_entry)  links;
    const xmlChar               *scope;
    const xmlChar               *oid;
} cytc_dep_entry;

typedef SLIST_HEAD(cytc_dep_list_t, cytc_dep_entry) cytc_dep_list_t;

typedef struct cs_yaml_target_context_s {
    const xmlChar   *oid;
    const xmlChar   *value;
    const xmlChar   *access;
    const xmlChar   *type;
    const xmlChar   *xmlvolatile;
    const xmlChar   *substitution;
    cytc_dep_list_t  deps;
    te_bool          cond;
} cs_yaml_target_context_t;

static te_errno
parse_config_yaml_cmd_add_dependency_attribute(yaml_node_t    *k,
                                               yaml_node_t    *v,
                                               cytc_dep_entry *dep_ctx)
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
            if (dep_ctx->oid != NULL)
            {
                ERROR(CS_YAML_ERR_PREFIX "detected multiple OID specifiers "
                      "of the dependce node: only one can be present");
                return TE_EINVAL;
            }

            dep_ctx->oid  = (const xmlChar *)v->data.scalar.value;
            break;

        case CS_YAML_NODE_ATTRIBUTE_SCOPE:
            if (dep_ctx->scope != NULL)
            {
                ERROR(CS_YAML_ERR_PREFIX "detected multiple scope specifiers "
                      "of the dependce node: only one can be present");
                return TE_EINVAL;
            }

            dep_ctx->scope = (const xmlChar *)v->data.scalar.value;
            break;

        case CS_YAML_NODE_ATTRIBUTE_DESCRIPTION:
            /* Ignore the description */
            break;

        default:
            if (v->type == YAML_SCALAR_NODE && v->data.scalar.length == 0)
            {
                dep_ctx->oid = (const xmlChar *)k->data.scalar.value;
            }
            else
            {
                ERROR(CS_YAML_ERR_PREFIX "failed to recognise the "
                      "attribute type in the target '%s'",
                      (const char *)k->data.scalar.value);
                return TE_EINVAL;
            }
            break;
    }

    return 0;
}

/**
 * Process an entry of the given dependency node
 *
 * @param  d       YAML document handle
 * @param  n       Handle of the parent node in the given document
 * @param  dep_ctx Entry context to store parsed properties
 *
 * @return         Status code
 */
static te_errno
parse_config_yaml_dependency_entry(yaml_document_t *d,
                                   yaml_node_t     *n,
                                   cytc_dep_entry  *dep_ctx)
{
    te_errno rc;

    if (n->type == YAML_MAPPING_NODE)
    {
        yaml_node_pair_t *pair = n->data.mapping.pairs.start;

        do {
            yaml_node_t *k = yaml_document_get_node(d, pair->key);
            yaml_node_t *v = yaml_document_get_node(d, pair->value);

            rc = parse_config_yaml_cmd_add_dependency_attribute(k, v, dep_ctx);
            if (rc != 0)
            {
                ERROR(CS_YAML_ERR_PREFIX "failed to process "
                      "attribute at " YAML_NODE_LINE_COLUMN_FMT "",
                      YAML_NODE_LINE_COLUMN(k));
                return TE_EINVAL;
            }
        } while (++pair < n->data.mapping.pairs.top);
    }
    else
    {
        ERROR(CS_YAML_ERR_PREFIX "found the dependency node to be "
              "badly formatted");
        return TE_EINVAL;
    }

    return 0;
}

/**
 * Process a dependency node of the given parent node.
 *
 * @param d     YAML document handle
 * @param n     Handle of the parent node in the given document
 * @param c     Location for the result
 *
 * @return Status code.
 */
static te_errno
parse_config_yaml_dependency(yaml_document_t            *d,
                             yaml_node_t                *n,
                             cs_yaml_target_context_t   *c)
{
    cytc_dep_entry *dep_entry;
    te_errno        rc;

    if (n->type == YAML_SCALAR_NODE)
    {
        if (n->data.scalar.length == 0)
        {
            ERROR(CS_YAML_ERR_PREFIX "found the dependency node to be "
                  "badly formatted");
            return TE_EINVAL;
        }

        dep_entry = TE_ALLOC(sizeof(*dep_entry));
        if (dep_entry == NULL) {
            ERROR(CS_YAML_ERR_PREFIX "failed to allocate memory");
            return TE_ENOMEM;
        }

        dep_entry->oid = (const xmlChar *)n->data.scalar.value;

        /* Error path resides in parse_config_yaml_cmd_process_target(). */
        SLIST_INSERT_HEAD(&c->deps, dep_entry, links);
    }
    else if (n->type == YAML_SEQUENCE_NODE)
    {
        yaml_node_item_t *item = n->data.sequence.items.start;

        do {
            yaml_node_t *in = yaml_document_get_node(d, *item);

            dep_entry = TE_ALLOC(sizeof(*dep_entry));
            if (dep_entry == NULL) {
                ERROR(CS_YAML_ERR_PREFIX "failed to allocate memory");
                return TE_ENOMEM;
            }

            rc = parse_config_yaml_dependency_entry(d, in, dep_entry);
            if (rc != 0)
            {
                free(dep_entry);
                return rc;
            }

            /* Error path resides in parse_config_yaml_cmd_process_target(). */
            SLIST_INSERT_HEAD(&c->deps, dep_entry, links);
        } while (++item < n->data.sequence.items.top);
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
            rc = parse_config_yaml_dependency(d, v, c);
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

        case CS_YAML_NODE_ATTRIBUTE_DESCRIPTION:
            /* Ignore the description */
            break;

        case CS_YAML_NODE_ATTRIBUTE_SUBSTITUTION:
            if (c->substitution != NULL)
            {
                ERROR(CS_YAML_ERR_PREFIX "detected multiple substitution "
                      "specifiers of the target: only one can be present");
                return TE_EINVAL;
            }

            c->substitution = (const xmlChar *)v->data.scalar.value;
            break;

        default:
            if (v->type == YAML_SCALAR_NODE && v->data.scalar.length == 0)
            {
                c->oid = (const xmlChar *)k->data.scalar.value;
            }
            else
            {
                ERROR(CS_YAML_ERR_PREFIX "failed to recognise the "
                      "attribute type in the target '%s'",
                      (const char *)k->data.scalar.value);
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
    const xmlChar  *prop_name_oid = (const xmlChar *)"oid";
    const xmlChar  *prop_name_value = (const xmlChar *)"value";
    const xmlChar  *prop_name_access = (const xmlChar *)"access";
    const xmlChar  *prop_name_type = (const xmlChar *)"type";
    const xmlChar  *prop_name_scope = (const xmlChar *)"scope";
    const xmlChar  *prop_name_volatile = (const xmlChar *)"volatile";
    const xmlChar  *prop_name_substitution = (const xmlChar *)"substitution";

    xmlNodePtr      dependency_node;
    cytc_dep_entry *dep_entry;

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
        xmlNewProp(xn_target, prop_name_volatile, c->xmlvolatile) == NULL)
    {
        ERROR(CS_YAML_ERR_PREFIX "failed to embed the target volatile "
              "attribute in XML output");
        return TE_ENOMEM;
    }

    if (c->substitution != NULL &&
        xmlNewProp(xn_target, prop_name_substitution, c->substitution) == NULL)
    {
        ERROR(CS_YAML_ERR_PREFIX "failed to embed the target substitution"
              "attribute in XML output");
        return TE_ENOMEM;
    }

    SLIST_FOREACH(dep_entry, &c->deps, links)
    {
        dependency_node = xmlNewNode(NULL, BAD_CAST "depends");
        if (dependency_node == NULL)
        {
            ERROR(CS_YAML_ERR_PREFIX "failed to allocate dependency "
                  "node for XML output");
            return TE_ENOMEM;
        }

        if (xmlNewProp(dependency_node, prop_name_oid, dep_entry->oid) ==
            NULL)
        {
            ERROR(CS_YAML_ERR_PREFIX "failed to set OID for the dependency "
                  "node in XML output");
            xmlFreeNode(dependency_node);
            return TE_ENOMEM;
        }

        if (dep_entry->scope != NULL &&
            xmlNewProp(dependency_node, prop_name_scope,
                       dep_entry->scope) == NULL)
        {
            ERROR(CS_YAML_ERR_PREFIX "failed to embed the target scope "
                  "attribute in XML output");
            xmlFreeNode(dependency_node);
            return TE_ENOMEM;
        }

        if (xmlAddChild(xn_target, dependency_node) != dependency_node)
        {
            ERROR(CS_YAML_ERR_PREFIX "failed to embed dependency node in "
                  "XML output");
            xmlFreeNode(dependency_node);
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

static te_errno
parse_config_yaml_include_doc(parse_config_yaml_ctx *ctx, yaml_node_t *n)
{
    char *file_name;
    char *dir_name;
    te_string file_path = TE_STRING_INIT;
    te_errno rc = 0;
    char *saved_current_yaml_file_path = NULL;
    xmlNodePtr xn_history = ctx->xn_history;
    te_kvpair_h *expand_vars = ctx->expand_vars;
    const char *current_yaml_file_path = ctx->file_path;

    if (n->data.scalar.length == 0)
    {
        ERROR(CS_YAML_ERR_PREFIX "found include node to be badly formatted");
        rc = TE_EINVAL;
        goto out;
    }

    file_name = (char *)n->data.scalar.value;
    saved_current_yaml_file_path = strdup(current_yaml_file_path);
    if (saved_current_yaml_file_path == NULL)
    {
        rc = TE_ENOMEM;
        goto out;
    }

    dir_name = dirname(saved_current_yaml_file_path);
    rc = te_string_append(&file_path, "%s/%s", dir_name, file_name);
    if (rc != 0)
        goto out;

    if (access(file_path.ptr, F_OK) == 0)
    {
        rc = parse_config_yaml(file_path.ptr, expand_vars, xn_history);
        if (rc != 0)
            goto out;
    }
    else
    {
        te_string_free(&file_path);

        dir_name = getenv("TE_INSTALL");
        if (dir_name == NULL)
        {
            rc = TE_EINVAL;
            goto out;
        }

        rc = te_string_append(&file_path, "%s/default/share/cm/%s",
                              dir_name, file_name);
        if (rc != 0)
            goto out;

        if (access(file_path.ptr, F_OK) == 0)
        {
            rc = parse_config_yaml(file_path.ptr, expand_vars, xn_history);
            if (rc != 0)
                goto out;
        }
        else
        {
            ERROR(CS_YAML_ERR_PREFIX "document %s specified in include node is not found",
                  file_name);
            rc = TE_EINVAL;
            goto out;
        }
    }

out:
    te_string_free(&file_path);
    free(saved_current_yaml_file_path);

    return rc;
}

/**
 * Free memory allocated for the needs of the given YAML target context
 *
 * @param c The context
 */
static void
cytc_cleanup(cs_yaml_target_context_t *c)
{
    cytc_dep_entry *dep_entry_tmp;
    cytc_dep_entry *dep_entry;

    SLIST_FOREACH_SAFE(dep_entry, &c->deps, links, dep_entry_tmp)
    {
        SLIST_REMOVE(&c->deps, dep_entry, cytc_dep_entry, links);
        free(dep_entry);
    }
}

/**
 * Process the given target node in the given YAML document.
 *
 * @param ctx               Current doc context
 * @param n                 Handle of the target node in the given YAML document
 * @param xn_cmd            Handle of command node in the XML document being
 *                          created
 * @param cmd               String representation of command
 *
 * @return Status code.
 */
static te_errno
parse_config_yaml_cmd_process_target(parse_config_yaml_ctx *ctx, yaml_node_t *n,
                                     xmlNodePtr xn_cmd, const char *cmd)
{
    yaml_document_t            *d = ctx->doc;
    te_kvpair_h                *expand_vars = ctx->expand_vars;
    xmlNodePtr                  xn_target = NULL;
    cs_yaml_target_context_t    c = YAML_TARGET_CONTEXT_INIT;
    const char                 *target;
    te_errno                    rc = 0;

    /*
     * Case of several included files, e.g.
     * - include:
     *      - filename1
     *      ...
     *      - filenameN
     */
    if (strcmp(cmd, "include") == 0)
    {
        rc = parse_config_yaml_include_doc(ctx, n);
        goto out;
    }

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
                      "attribute at " YAML_NODE_LINE_COLUMN_FMT "",
                      target, YAML_NODE_LINE_COLUMN(k));
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

    cytc_cleanup(&c);

    return 0;

out:
    xmlFreeNode(xn_target);
    cytc_cleanup(&c);

    return rc;
}

/**
 * Process the sequence of target nodes for the specified
 * command in the given YAML document.
 *
 * @param ctx               Current doc context
 * @param n                 Handle of the target sequence in the given YAML
 *                          document
 * @param xn_cmd            Handle of command node in the XML document being
 *                          created
 * @param cmd               String representation of command
 *
 * @return Status code.
 */
static te_errno
parse_config_yaml_cmd_process_targets(parse_config_yaml_ctx *ctx,
                                      yaml_node_t *n, xmlNodePtr xn_cmd,
                                      const char *cmd)
{
    yaml_document_t *d = ctx->doc;
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

        rc = parse_config_yaml_cmd_process_target(ctx, in, xn_cmd, cmd);
        if (rc != 0)
        {
            ERROR(CS_YAML_ERR_PREFIX "failed to process the target in the "
                  "%s command's list at " YAML_NODE_LINE_COLUMN_FMT "",
                  cmd, YAML_NODE_LINE_COLUMN(in));
            return rc;
        }
    } while (++item < n->data.sequence.items.top);

    return 0;
}

static te_errno parse_config_yaml_cmd(parse_config_yaml_ctx *ctx,
                                      yaml_node_t           *parent);

/**
 * Process dynamic history specified command in the given YAML document.
 *
 * @param ctx               Current doc context
 * @param n                 Handle of the command node in the given YAML document
 * @param cmd               String representation of command
 *
 * @return Status code.
 */
static te_errno
parse_config_yaml_specified_cmd(parse_config_yaml_ctx *ctx, yaml_node_t *n,
                                const char *cmd)
{
    yaml_document_t *d = ctx->doc;
    te_kvpair_h     *expand_vars = ctx->expand_vars;
    xmlNodePtr       xn_history = ctx->xn_history;

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

        rc = parse_config_yaml_cmd_process_targets(ctx, n, xn_cmd, cmd);
        if (rc != 0)
        {
            ERROR(CS_YAML_ERR_PREFIX "detected some error(s) in the %s "
                  "command's nested node at " YAML_NODE_LINE_COLUMN_FMT "",
                  cmd, YAML_NODE_LINE_COLUMN(n));
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
                    rc = parse_config_yaml_cmd(ctx, v);
            }
            else if (strcmp(k_label, "else") == 0)
            {
                if (!cond)
                    rc = parse_config_yaml_cmd(ctx, v);
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
                      "command's nested node at " YAML_NODE_LINE_COLUMN_FMT "",
                      cmd, YAML_NODE_LINE_COLUMN(k));
                goto out;
            }
        } while (++pair < n->data.mapping.pairs.top);
    }
    /*
     * Case of single included file, e.g.
     * - include: filename
     */
    else if (n->type == YAML_SCALAR_NODE)
    {
        if (strcmp(cmd, "include") != 0)
        {
            ERROR(CS_YAML_ERR_PREFIX "found the %s command node to be "
            "badly formatted", cmd);
            rc = TE_EINVAL;
            goto out;
        }

        rc = parse_config_yaml_include_doc(ctx, n);
        if (rc != 0)
            goto out;
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
parse_config_root_commands(parse_config_yaml_ctx *ctx,
                           yaml_node_t           *n)
{
    yaml_document_t *d = ctx->doc;
    yaml_node_pair_t *pair = n->data.mapping.pairs.start;
    yaml_node_t *k = yaml_document_get_node(d, pair->key);
    yaml_node_t *v = yaml_document_get_node(d, pair->value);
    te_errno rc = 0;

    if ((strcmp((const char *)k->data.scalar.value, "add") == 0) ||
        (strcmp((const char *)k->data.scalar.value, "get") == 0) ||
        (strcmp((const char *)k->data.scalar.value, "set") == 0) ||
        (strcmp((const char *)k->data.scalar.value, "register") == 0) ||
        (strcmp((const char *)k->data.scalar.value, "unregister") == 0) ||
        (strcmp((const char *)k->data.scalar.value, "delete") == 0) ||
        (strcmp((const char *)k->data.scalar.value, "copy") == 0) ||
        (strcmp((const char *)k->data.scalar.value, "include") == 0) ||
        (strcmp((const char *)k->data.scalar.value, "cond") == 0))
    {
         rc = parse_config_yaml_specified_cmd(ctx, v,
                                        (const char *)k->data.scalar.value);
    }
    else if (strcmp((const char *)k->data.scalar.value, "comment") == 0)
    {
        /* Ignore comments */
    }
    else
    {
        ERROR(CS_YAML_ERR_PREFIX "failed to recognise the command '%s'",
              (const char *)k->data.scalar.value);
        rc = TE_EINVAL;
    }

    if (rc != 0)
    {
        ERROR(CS_YAML_ERR_PREFIX "detected some error(s) in "
              "the command node at " YAML_NODE_LINE_COLUMN_FMT " in file %s",
              YAML_NODE_LINE_COLUMN(k), ctx->file_path);
    }

    return rc;
}

/**
 * Explore sequence of commands of the given parent node in the given YAML
 * document to detect and process dynamic history commands.
 *
 * @param ctx               Current doc context
 * @param parent            Parent node of sequence of commands
 *
 * @return Status code.
 */
static te_errno
parse_config_yaml_cmd(parse_config_yaml_ctx *ctx,
                      yaml_node_t           *parent)
{
    yaml_document_t  *d = ctx->doc;
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

        rc = parse_config_root_commands(ctx, n);
        if (rc != 0)
            break;
    } while (++item < parent->data.sequence.items.top);

    return rc;
}

/* See description in 'conf_yaml.h' */
te_errno
parse_config_yaml(const char *filename, te_kvpair_h *expand_vars,
                  xmlNodePtr xn_history_root)
{
    FILE                   *f = NULL;
    yaml_parser_t           parser;
    yaml_document_t         dy;
    xmlNodePtr              xn_history = NULL;
    yaml_node_t            *root = NULL;
    te_errno                rc = 0;
    char                   *current_yaml_file_path;
    parse_config_yaml_ctx   ctx;

    f = fopen(filename, "rb");
    if (f == NULL)
    {
        ERROR(CS_YAML_ERR_PREFIX "failed to open the target file '%s'",
              filename);
        return TE_OS_RC(TE_CS, errno);
    }

    yaml_parser_initialize(&parser);
    yaml_parser_set_input_file(&parser, f);
    yaml_parser_load(&parser, &dy);
    fclose(f);

    current_yaml_file_path = strdup(filename);
    if (current_yaml_file_path == NULL)
    {
        rc = TE_ENOMEM;
        goto out;
    }

    if (xn_history_root == NULL)
    {
        xn_history = xmlNewNode(NULL, BAD_CAST "history");
        if (xn_history == NULL)
        {
            ERROR(CS_YAML_ERR_PREFIX "failed to allocate "
                  "main history node for XML output");
            rc = TE_ENOMEM;
            goto out;
        }
    }
    else
    {
        xn_history = xn_history_root;
    }

    root = yaml_document_get_root_node(&dy);
    if (root == NULL)
    {
        ERROR(CS_YAML_ERR_PREFIX "failed to get the root node in file %s'",
              filename);
        rc = TE_EINVAL;
        goto out;
    }

    if (root->type == YAML_SCALAR_NODE &&
        root->data.scalar.value[0] == '\0')
    {
        INFO(CS_YAML_ERR_PREFIX "empty file '%s'", filename);
        rc = 0;
        goto out;
    }

    memset(&ctx, 0, sizeof(ctx));
    ctx.file_path = current_yaml_file_path;
    ctx.doc = &dy;
    ctx.xn_history = xn_history;
    ctx.expand_vars = expand_vars;
    rc = parse_config_yaml_cmd(&ctx, root);
    if (rc != 0)
    {
        ERROR(CS_YAML_ERR_PREFIX
              "encountered some error(s) on file '%s' processing",
              filename);
        goto out;
    }

    if (xn_history_root == NULL && xn_history->children != NULL)
    {
        rcf_log_cfg_changes(TRUE);
        rc = parse_config_dh_sync(xn_history, expand_vars);
        rcf_log_cfg_changes(FALSE);
    }

out:
    if (xn_history_root == NULL)
        xmlFreeNode(xn_history);
    yaml_document_delete(&dy);
    yaml_parser_delete(&parser);
    free(current_yaml_file_path);

    return rc;
}

