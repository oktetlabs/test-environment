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
#include "conf_yaml.h"
#include "te_str.h"

#include <libxml/xinclude.h>
#include <yaml.h>

/**
 * Evaluate logical expression.
 *
 * @param text  String representation of the expression
 * @param condp Location for the result
 *
 * @return Status code.
 */
static te_errno
transform_conf_yaml_cond_exp(const char *text,
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
    if (var_str == NULL)
    {
        rc = TE_EINVAL;
        goto out;
    }

    if (strcmp(op_str, "==") == 0)
    {
        *condp = (strcmp(var_str, cmp_str) == 0) ? TRUE : FALSE;
    }
    else if (strcmp(op_str, "!=") == 0)
    {
        *condp = (strcmp(var_str, cmp_str) != 0) ? TRUE : FALSE;
    }
    else
    {
        long int var;
        long int cmp;

        rc = te_strtol(var_str, 0, &var);
        if (rc != 0)
            goto out;

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
 * @param condp Location for the result
 *
 * @return Status code.
 */
static te_errno
transform_conf_yaml_cond(yaml_document_t *d,
                         yaml_node_t     *n,
                         te_bool         *condp)
{
    const char *str = NULL;
    te_errno    rc = 0;

    if (n->type == YAML_SCALAR_NODE)
    {
        str = (const char *)n->data.scalar.value;

        if (n->data.scalar.length == 0)
            return TE_EINVAL;

        return transform_conf_yaml_cond_exp(str, condp);
    }
    else if (n->type == YAML_SEQUENCE_NODE)
    {
        yaml_node_item_t *item = n->data.sequence.items.start;

        do {
            yaml_node_t *in = yaml_document_get_node(d, *item);

            if (in->type != YAML_SCALAR_NODE || in->data.scalar.length == 0)
                return TE_EINVAL;

            str = (const char *)in->data.scalar.value;

            rc = transform_conf_yaml_cond_exp(str, condp);
            if (rc != 0)
                return rc;

            if (*condp == FALSE)
                return 0;
        } while (++item < n->data.sequence.items.top);
    }
    else
    {
        return TE_EINVAL;
    }

    return 0;
}

/**
 * Process the given object instance node in the given YAML document.
 *
 * @param d      YAML document handle
 * @param n      Handle of the instance node in the given YAML document
 * @param xn_add Handle of "add" node in the XML document being created
 *
 * @return Status code.
 */
static te_errno
transform_conf_yaml_cmd_add_instance(yaml_document_t *d,
                                     yaml_node_t     *n,
                                     xmlNodePtr       xn_add)
{
    xmlNodePtr     xn_instance = NULL;
    xmlAttrPtr     xa_oid = NULL;
    const xmlChar *n_label = NULL;
    const xmlChar *prop_name = (const xmlChar *)"oid";
    te_bool        check_cond = TRUE;
    te_bool        cond = TRUE;
    te_errno       rc = 0;

    if (n->data.scalar.length == 0)
        return TE_EINVAL;

    xn_instance = xmlNewNode(NULL, BAD_CAST "instance");
    if (xn_instance == NULL)
        return TE_ENOMEM;

    if (n->type == YAML_SCALAR_NODE)
    {
        if (n->data.scalar.length == 0)
        {
            rc = TE_EINVAL;
            goto out;
        }

        n_label = (const xmlChar *)n->data.scalar.value;
    }
    else if (n->type == YAML_MAPPING_NODE)
    {
        yaml_node_pair_t *pair = n->data.mapping.pairs.start;
        yaml_node_t      *k_first = yaml_document_get_node(d, pair->key);

        if (k_first->data.scalar.length == 0)
        {
            rc = TE_EINVAL;
            goto out;
        }

        n_label = (const xmlChar *)k_first->data.scalar.value;

        do {
            yaml_node_t *k = yaml_document_get_node(d, pair->key);
            yaml_node_t *v = yaml_document_get_node(d, pair->value);
            const char  *k_label = (const char *)k->data.scalar.value;

            if (k->data.scalar.length == 0)
                continue;

            if (strcmp(k_label, "cond") == 0 && check_cond == TRUE)
            {
                rc = transform_conf_yaml_cond(d, v, &cond);
                if (rc != 0)
                    goto out;

                if (cond == TRUE)
                    check_cond = FALSE;
            }
        } while (++pair < n->data.mapping.pairs.top);
    }
    else
    {
        cond = FALSE;
    }

    xa_oid = xmlNewProp(xn_instance, prop_name, n_label);
    if (xa_oid == NULL)
    {
        rc = TE_ENOMEM;
        goto out;
    }

    if (cond)
    {
        if (xmlAddChild(xn_add, xn_instance) == xn_instance)
            return 0;
        else
            rc = TE_EINVAL;
    }

out:
    xmlFreeNode(xn_instance);

    return rc;
}

/**
 * Process the sequence of instance nodes for the "add"
 * command in the given YAML document.
 *
 * @param d      YAML document handle
 * @param n      Handle of the instance sequence in the given YAML document
 * @param xn_add Handle of "add" node in the XML document being created
 *
 * @return Status code.
 */
static te_errno
transform_conf_yaml_cmd_add_instances(yaml_document_t *d,
                                      yaml_node_t     *n,
                                      xmlNodePtr       xn_add)
{
    yaml_node_item_t *item = n->data.sequence.items.start;

    if (n->type != YAML_SEQUENCE_NODE)
        return TE_EINVAL;

    do {
        yaml_node_t *in = yaml_document_get_node(d, *item);
        te_errno     rc = 0;

        rc = transform_conf_yaml_cmd_add_instance(d, in, xn_add);
        if (rc != 0)
            return rc;
    } while (++item < n->data.sequence.items.top);

    return 0;
}

/**
 * Process dynamic history "add" command in the given YAML document.
 *
 * @param d          YAML document handle
 * @param n          Handle of the "add" node in the given YAML document
 * @param xn_history Handle of "history" node in the XML document being created
 *
 * @return Status code.
 */
static te_errno
transform_conf_yaml_cmd_add(yaml_document_t *d,
                            yaml_node_t     *n,
                            xmlNodePtr       xn_history)
{
    xmlNodePtr xn_add = NULL;
    te_bool    check_cond = TRUE;
    te_bool    cond = TRUE;
    te_errno   rc = 0;

    xn_add = xmlNewNode(NULL, BAD_CAST "add");
    if (xn_add == NULL)
        return TE_ENOMEM;

    if (n->type == YAML_MAPPING_NODE)
    {
        yaml_node_pair_t *pair = n->data.mapping.pairs.start;

        do {
            yaml_node_t *k = yaml_document_get_node(d, pair->key);
            yaml_node_t *v = yaml_document_get_node(d, pair->value);
            const char  *k_label = (const char *)k->data.scalar.value;

            if (k->data.scalar.length == 0)
                continue;

            if (strcmp(k_label, "cond") == 0 && check_cond == TRUE)
            {
                rc = transform_conf_yaml_cond(d, v, &cond);
                if (rc != 0)
                    goto out;

                if (cond == TRUE)
                    check_cond = FALSE;
            }

            if (strcmp(k_label, "instances") == 0)
            {
                rc = transform_conf_yaml_cmd_add_instances(d, v, xn_add);
                if (rc != 0)
                    goto out;
            }
        } while (++pair < n->data.mapping.pairs.top);
    }
    else
    {
        cond = FALSE;
    }

    if (cond == TRUE && xn_add->children != NULL)
    {
        if (xmlAddChild(xn_history, xn_add) == xn_add)
            return 0;
        else
            rc = TE_EINVAL;
    }

out:
    xmlFreeNode(xn_add);

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
transform_conf_yaml_cmd(yaml_document_t *d,
                        xmlNodePtr       xn_history)
{
    yaml_node_t      *root = NULL;
    yaml_node_pair_t *pair = NULL;
    te_errno          rc = 0;

    root = yaml_document_get_root_node(d);
    if (root == NULL)
        return TE_EINVAL;

    pair = root->data.mapping.pairs.start;
    do {
        yaml_node_t *k = yaml_document_get_node(d, pair->key);
        yaml_node_t *v = yaml_document_get_node(d, pair->value);

        if (k->data.scalar.length == 0)
            continue;

        if (strcmp((const char *)k->data.scalar.value, "add") == 0)
        {
            rc = transform_conf_yaml_cmd_add(d, v, xn_history);
            if (rc != 0)
                return rc;
        }
    } while (++pair < root->data.mapping.pairs.top);

    return 0;
}

/**
 * Dump the given XML document to a file.
 *
 * The function will change the file extension to XML.
 *
 * @param d        Handle of the XML document
 * @param filename The original file path
 *
 * @return Status code.
 */
static te_errno
transform_conf_yaml_dump_xml(xmlDocPtr   d,
                             const char *filename)
{
    char     *filename_xml = NULL;
    FILE     *f = NULL;
    char     *filename_ext = NULL;
    size_t    filename_ext_len;
    te_errno  rc = 0;
    int       ret;

    filename_xml = strdup(filename);
    if (filename_xml == NULL)
        return TE_ENOMEM;

    filename_ext = strchr(filename_xml, '.');
    if (filename_ext == NULL)
    {
        rc = TE_EINVAL;
        goto out;
    }

    ++filename_ext;

    filename_ext_len = strlen(filename_ext);
    ret = snprintf(filename_ext, filename_ext_len + 1, "xml");
    if (ret != (int)filename_ext_len)
    {
        rc = TE_EINVAL;
        goto out;
    }

    f = fopen(filename_xml, "w");
    if (f == NULL)
    {
        rc = TE_OS_RC(TE_CS, errno);
        goto out;
    }

    if (xmlDocFormatDump(f, d, 1) == -1)
        rc = TE_ENOMEM;

    fclose(f);

out:
    free(filename_xml);

    return rc;
}

/* See description in 'conf_yaml.h' */
te_errno
transform_conf_yaml(const char *filename)
{
    FILE            *f = NULL;
    yaml_parser_t    parser;
    yaml_document_t  dy;
    xmlDocPtr        dx = NULL;
    xmlNodePtr       xn_history = NULL;
    te_errno         rc = 0;

    f = fopen(filename, "rb");
    if (f == NULL)
        return TE_OS_RC(TE_CS, errno);

    yaml_parser_initialize(&parser);
    yaml_parser_set_input_file(&parser, f);
    yaml_parser_load(&parser, &dy);
    fclose(f);

    dx = xmlNewDoc(BAD_CAST "1.0");
    if (dx == NULL)
    {
        rc = TE_ENOMEM;
        goto out;
    }

    xn_history = xmlNewNode(NULL, BAD_CAST "history");
    if (xn_history == NULL)
    {
        rc = TE_ENOMEM;
        goto out;
    }

    rc = transform_conf_yaml_cmd(&dy, xn_history);
    if (rc != 0)
        goto out;

    if (xn_history->children != NULL)
        xmlDocSetRootElement(dx, xn_history);
    else
        xmlFreeNode(xn_history);

    rc = transform_conf_yaml_dump_xml(dx, filename);

out:
    xmlFreeDoc(dx);
    yaml_document_delete(&dy);
    yaml_parser_delete(&parser);

    return rc;
}

