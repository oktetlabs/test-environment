/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2022 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief TE RCF Engine - parser of TCE configuration
 *
 * Internal functions to process the TCE configuration.
 */

#include "te_config.h"

#include <string.h>
#include <yaml.h>

#include "te_defs.h"
#include "te_errno.h"
#include "logger_api.h"
#include "rcf_tce_parser.h"

static rcf_tce_comp_conf_t *
tce_comp_conf_alloc(void)
{
    rcf_tce_comp_conf_t *conf;

    if ((conf = calloc(1, sizeof(*conf))) == NULL)
        TE_FATAL_ERROR("TCE: failed to allocate 'ta_comp'");

    return conf;
}

static rcf_tce_type_conf_t *
tce_type_conf_alloc(void)
{
    rcf_tce_type_conf_t *conf;

    if ((conf = calloc(1, sizeof(*conf))) == NULL)
        TE_FATAL_ERROR("TCE: failed to allocate 'ta_type'");

    SLIST_INIT(&conf->comp);

    return conf;
}

static rcf_tce_conf_t *
tce_conf_alloc(void)
{
    rcf_tce_conf_t *conf;

    if ((conf = calloc(1, sizeof(*conf))) == NULL)
        TE_FATAL_ERROR("TCE: failed to allocate 'tce_conf'");

    SLIST_INIT(&conf->types);

    return conf;
}

static void
tce_comp_conf_free(rcf_tce_comp_conf_t *conf)
{
    free(conf->name);
    free(conf->build);
    free(conf);
}

static void
tce_type_conf_free(rcf_tce_type_conf_t *conf)
{
    rcf_tce_comp_conf_t *comp;
    rcf_tce_comp_conf_t *temp;

    free(conf->name);
    free(conf->base);

    SLIST_FOREACH_SAFE(comp, &conf->comp, next, temp)
        tce_comp_conf_free(comp);

    free(conf);
}

void
rcf_tce_conf_free(rcf_tce_conf_t *conf)
{
    rcf_tce_local_conf_t *local;
    rcf_tce_type_conf_t *type;
    rcf_tce_type_conf_t *temp;

    if (conf == NULL)
        return;

    local = &conf->local;
    free(local->tebin);
    free(local->tcews);

    SLIST_FOREACH_SAFE(type, &conf->types, next, temp)
        tce_type_conf_free(type);

    free(conf);
}

typedef struct {
    yaml_document_t *doc;
    yaml_node_pair_t *next;
    yaml_node_pair_t *last;
} miter_t;

static bool
miter_init(miter_t *iter, yaml_document_t *doc, yaml_node_t *node)
{
    if (node == NULL || node->type != YAML_MAPPING_NODE)
        return false;

    *iter = (miter_t) {
        .doc = doc,
        .next = node->data.mapping.pairs.start,
        .last = node->data.mapping.pairs.top,
    };

    return true;
}

static bool
miter_next(miter_t *iter, const char **name, yaml_node_t **node)
{
    yaml_node_t *key;

    if (iter->next == iter->last)
        return false;

    key = yaml_document_get_node(iter->doc, iter->next->key);
    if (key == NULL || key->type != YAML_SCALAR_NODE)
        return false;

    *name = (const char *)key->data.scalar.value;
    *node = yaml_document_get_node(iter->doc, iter->next->value);

    iter->next++;

    return true;
}

static bool
miter_find(miter_t *iter, const char *name, yaml_node_t **node)
{
    const char *key;

    while (miter_next(iter, &key, node))
    {
        if (strcmp(name, key) == 0)
            return true;
    }

    return false;
}

const char *
find_mprop(yaml_document_t *doc, yaml_node_t *node, const char *name)
{
    miter_t iter;
    yaml_node_t *next;

    if (!miter_init(&iter, doc, node) || !miter_find(&iter, name, &next) ||
        next == NULL || next->type != YAML_SCALAR_NODE)
    {
        return NULL;
    }

   return (const char *)next->data.scalar.value;
}

static te_errno
set_mprop(yaml_document_t *doc, yaml_node_t *node, const char *node_name,
          const char *name, char **value)
{
    const char *prop;

    if ((prop = find_mprop(doc, node, name)) == NULL)
    {
        ERROR("TCE: wrong conf: '%s' has no property '%s'", node_name, name);
        return TE_EFMT;
    }

    if ((*value = strdup(prop)) == NULL)
    {
        TE_FATAL_ERROR("TCE: failed to allocate property '%s' of '%s'", name,
                       node_name);
    }

    return 0;
}

static te_errno
parse_ta_comp(yaml_document_t *doc, yaml_node_t *node,
              rcf_tce_comp_conf_t **comp)
{
    rcf_tce_comp_conf_t *temp;
    te_errno rc;

    temp = tce_comp_conf_alloc();

    if ((rc = set_mprop(doc, node, "ta_comp", "name", &temp->name)) != 0 ||
        (rc = set_mprop(doc, node, "ta_comp", "build", &temp->build)) != 0)
    {
        tce_comp_conf_free(temp);
        return rc;
    }

    *comp = temp;

    return 0;
}

static te_errno
parse_ta_type(yaml_document_t *doc, yaml_node_t *node,
              rcf_tce_type_conf_t **conf)
{
    rcf_tce_type_conf_t *temp;
    rcf_tce_comp_conf_t *comp;
    miter_t iter;
    te_errno rc;

    temp = tce_type_conf_alloc();

    if ((rc = set_mprop(doc, node, "ta_type", "name", &temp->name)) != 0 ||
        (rc = set_mprop(doc, node, "ta_type", "base", &temp->base)) != 0)
    {
        goto err;
    }

    if (!miter_init(&iter, doc, node))
    {
        ERROR("TCE: wrong conf: 'ta_type' mapping expected");
        rc = TE_EFMT;
        goto err;
    }

    while (miter_find(&iter, "ta_comp", &node))
    {
        if ((rc = parse_ta_comp(doc, node, &comp)) != 0)
            goto err;

        SLIST_INSERT_HEAD(&temp->comp, comp, next);
    }

    *conf = temp;

    return 0;

err:
    tce_type_conf_free(temp);

    return rc;
}

static te_errno
parse_ta_types(yaml_document_t *doc, yaml_node_t *node, rcf_tce_conf_t *conf)
{
    rcf_tce_type_conf_t *type;
    miter_t iter;
    te_errno rc;

    if (!miter_init(&iter, doc, node))
    {
        ERROR("TCE: invalid conf: 'ta_type' expected");
        return TE_EFMT;
    }

    while (miter_find(&iter, "ta_type", &node))
    {
        if ((rc = parse_ta_type(doc, node, &type)) != 0)
            return TE_EFMT;

        SLIST_INSERT_HEAD(&conf->types, type, next);
    }

    return 0;
}

static te_errno
parse_te_local(yaml_document_t *doc, yaml_node_t *node, rcf_tce_conf_t *conf)
{
    rcf_tce_local_conf_t *local = &conf->local;
    miter_t iter;
    te_errno rc;

    if (!miter_init(&iter, doc, node) || !miter_find(&iter, "te_local", &node))
    {
        ERROR("TCE: invalid conf: 'te_local' expected");
        return TE_EFMT;
    }

    if ((rc = set_mprop(doc, node, "te_local", "tebin", &local->tebin)) != 0 ||
        (rc = set_mprop(doc, node, "te_local", "tcews", &local->tcews)) != 0)
    {
        return rc;
    }

    return 0;
}

static te_errno
load_doc(yaml_parser_t *parser, const char *file, yaml_document_t *doc)
{
    FILE *fp;
    te_errno rc = 0;

    if ((fp = fopen(file, "r")) == NULL)
    {
        ERROR("TCE: failed to read TCE conf '%s'", file);
        return TE_EINVAL;
    }

    yaml_parser_initialize(parser);
    yaml_parser_set_input_file(parser, fp);

    if (yaml_parser_load(parser, doc) != 1)
    {
        ERROR("TCE: failed to parse TCE conf '%s'", file);
        rc = TE_EFMT;
    }

    fclose(fp);

    return rc;
}

te_errno
rcf_tce_conf_parse(rcf_tce_conf_t **conf, const char *file)
{
    rcf_tce_conf_t *temp = NULL;
    yaml_parser_t parser;
    yaml_document_t doc;
    yaml_node_t *node;
    te_errno rc;

    if ((rc = load_doc(&parser, file, &doc)) != 0)
        return rc;

    rc = TE_EFMT;

    if ((node = yaml_document_get_root_node(&doc)) == NULL)
    {
        ERROR("TCE: invalid conf: '---' expected");
        goto out;
    }

    temp = tce_conf_alloc();

    if ((rc = parse_te_local(&doc, node, temp)) != 0)
        goto out;

    if ((rc = parse_ta_types(&doc, node, temp)) != 0)
        goto out;

    *conf = temp;
    temp = NULL;

out:
    rcf_tce_conf_free(temp);

    yaml_document_delete(&doc);
    yaml_parser_delete(&parser);

    return rc;
}
